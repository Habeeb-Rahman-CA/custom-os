#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>

namespace kernel::memory {

namespace {

constexpr u32 HEAP_MAGIC = 0xCAFEBABE;
constexpr u64 HEAP_START_VIRT = 0xffffffff90000000ULL;
constexpr usize INITIAL_HEAP_PAGES = 16; // 64 KiB initial heap

struct BlockHeader {
    usize size;         // Usable payload size in bytes
    bool is_free;
    u32 magic;
    BlockHeader* next;
};

BlockHeader* g_heap_head = nullptr;
u64 g_heap_current_top = HEAP_START_VIRT;
usize g_allocated_bytes = 0;
usize g_total_heap_bytes = 0;

void expand_heap(usize additional_bytes) {
    usize pages_needed = (additional_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    for (usize i = 0; i < pages_needed; ++i) {
        PhysAddr frame = Pmm::alloc_frame();
        Vmm::map_page(VirtAddr(g_heap_current_top), frame, PagingFlags::Present | PagingFlags::Writable | PagingFlags::NoExecute);
        g_heap_current_top += PAGE_SIZE;
        g_total_heap_bytes += PAGE_SIZE;
    }
}

} // namespace

void heap_init() {
    g_heap_current_top = HEAP_START_VIRT;
    expand_heap(INITIAL_HEAP_PAGES * PAGE_SIZE);

    g_heap_head = reinterpret_cast<BlockHeader*>(HEAP_START_VIRT);
    g_heap_head->size = (INITIAL_HEAP_PAGES * PAGE_SIZE) - sizeof(BlockHeader);
    g_heap_head->is_free = true;
    g_heap_head->magic = HEAP_MAGIC;
    g_heap_head->next = nullptr;

    log::info("[HEAP] Dynamic Kernel Heap allocator initialized (64 KiB initial).");
}

void* kmalloc(usize size) {
    if (size == 0) return nullptr;

    // Align allocation to 16 bytes
    usize aligned_size = (size + 15) & ~15ULL;

    BlockHeader* curr = g_heap_head;
    BlockHeader* prev = nullptr;

    while (curr) {
        if (curr->magic != HEAP_MAGIC) {
            panic("Heap corruption detected during kmalloc!");
        }

        if (curr->is_free && curr->size >= aligned_size) {
            // Can we split this block?
            if (curr->size >= aligned_size + sizeof(BlockHeader) + 16) {
                auto* new_block = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<u8*>(curr) + sizeof(BlockHeader) + aligned_size
                );
                new_block->size = curr->size - aligned_size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->magic = HEAP_MAGIC;
                new_block->next = curr->next;

                curr->size = aligned_size;
                curr->next = new_block;
            }

            curr->is_free = false;
            g_allocated_bytes += curr->size;
            return reinterpret_cast<void*>(reinterpret_cast<u8*>(curr) + sizeof(BlockHeader));
        }

        prev = curr;
        curr = curr->next;
    }

    // No suitable block found; expand heap
    usize bytes_to_add = (aligned_size + sizeof(BlockHeader) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    u64 old_top = g_heap_current_top;
    expand_heap(bytes_to_add);

    auto* new_chunk = reinterpret_cast<BlockHeader*>(old_top);
    new_chunk->size = bytes_to_add - sizeof(BlockHeader);
    new_chunk->is_free = true;
    new_chunk->magic = HEAP_MAGIC;
    new_chunk->next = nullptr;

    if (prev) {
        prev->next = new_chunk;
    } else {
        g_heap_head = new_chunk;
    }

    return kmalloc(size);
}

void kfree(void* ptr) {
    if (!ptr) return;

    auto* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<u8*>(ptr) - sizeof(BlockHeader)
    );

    if (header->magic != HEAP_MAGIC) {
        panic("Heap corruption or invalid pointer in kfree!");
    }

    if (header->is_free) {
        panic("Heap double-free detected in kfree!");
    }

    header->is_free = true;
    g_allocated_bytes -= header->size;

    // Coalesce adjacent free blocks
    BlockHeader* curr = g_heap_head;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(BlockHeader) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void* krealloc(void* ptr, usize new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return nullptr;
    }

    auto* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<u8*>(ptr) - sizeof(BlockHeader)
    );
    if (header->size >= new_size) {
        return ptr;
    }

    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        auto* src = reinterpret_cast<u8*>(ptr);
        auto* dst = reinterpret_cast<u8*>(new_ptr);
        for (usize i = 0; i < header->size; ++i) {
            dst[i] = src[i];
        }
        kfree(ptr);
    }
    return new_ptr;
}

usize get_heap_allocated_bytes() { return g_allocated_bytes; }
usize get_heap_total_bytes()     { return g_total_heap_bytes; }

} // namespace kernel::memory

void* operator new(size_t size) {
    return kernel::memory::kmalloc(size);
}

void* operator new[](size_t size) {
    return kernel::memory::kmalloc(size);
}

void operator delete(void* p) noexcept {
    kernel::memory::kfree(p);
}

void operator delete[](void* p) noexcept {
    kernel::memory::kfree(p);
}

void operator delete(void* p, size_t) noexcept {
    kernel::memory::kfree(p);
}

void operator delete[](void* p, size_t) noexcept {
    kernel::memory::kfree(p);
}
