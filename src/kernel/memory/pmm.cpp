#include <kernel/memory/pmm.h>
#include <kernel/core/boot_info.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>

namespace kernel::memory {

namespace {

u8*  g_bitmap = nullptr;
usize g_total_frames = 0;
usize g_used_frames  = 0;
usize g_free_frames  = 0;
usize g_last_scanned_idx = 0;

inline void bitmap_set(usize frame_idx) {
    g_bitmap[frame_idx / 8] |= static_cast<u8>(1 << (frame_idx % 8));
}

inline void bitmap_clear(usize frame_idx) {
    g_bitmap[frame_idx / 8] &= static_cast<u8>(~(1 << (frame_idx % 8)));
}

inline bool bitmap_test(usize frame_idx) {
    return (g_bitmap[frame_idx / 8] & (1 << (frame_idx % 8))) != 0;
}

} // namespace

void Pmm::init() {
    auto* memmap = boot::get_raw_memmap();
    if (!memmap) {
        panic("PMM: Bootloader memory map is null!");
    }

    u64 hhdm = boot::get_info().hhdm_offset;
    u64 highest_addr = 0;

    for (u64 i = 0; i < memmap->entry_count; ++i) {
        auto* entry = memmap->entries[i];
        u64 top = entry->base + entry->length;
        if (top > highest_addr) {
            highest_addr = top;
        }
    }

    g_total_frames = highest_addr / PAGE_SIZE;
    usize bitmap_bytes = (g_total_frames + 7) / 8;
    usize bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // Find a usable region to place the bitmap
    u64 bitmap_phys = 0;
    for (u64 i = 0; i < memmap->entry_count; ++i) {
        auto* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= (bitmap_pages * PAGE_SIZE)) {
            bitmap_phys = entry->base;
            break;
        }
    }

    if (bitmap_phys == 0) {
        panic("PMM: Failed to locate physical memory for page frame bitmap!");
    }

    g_bitmap = reinterpret_cast<u8*>(hhdm + bitmap_phys);

    // Initialize all frames as allocated (0xFF)
    for (usize i = 0; i < bitmap_bytes; ++i) {
        g_bitmap[i] = 0xFF;
    }

    g_used_frames = g_total_frames;
    g_free_frames = 0;

    // Mark usable RAM frames as free (0)
    for (u64 i = 0; i < memmap->entry_count; ++i) {
        auto* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            usize start_frame = entry->base / PAGE_SIZE;
            usize count = entry->length / PAGE_SIZE;

            for (usize j = 0; j < count; ++j) {
                usize frame_idx = start_frame + j;
                if (frame_idx < g_total_frames) {
                    bitmap_clear(frame_idx);
                    g_free_frames++;
                    g_used_frames--;
                }
            }
        }
    }

    // Reserve frame 0 (protect real mode IVT / BIOS data area)
    if (!bitmap_test(0)) {
        bitmap_set(0);
        g_used_frames++;
        g_free_frames--;
    }

    // Reserve physical frames consumed by the bitmap itself
    usize bitmap_start_frame = bitmap_phys / PAGE_SIZE;
    for (usize i = 0; i < bitmap_pages; ++i) {
        usize idx = bitmap_start_frame + i;
        if (!bitmap_test(idx)) {
            bitmap_set(idx);
            g_used_frames++;
            g_free_frames--;
        }
    }

    log::info("[PMM] Physical Frame Allocator (Bitmap) initialized.");
    dump_stats();
}

PhysAddr Pmm::alloc_frame() {
    for (usize i = 0; i < g_total_frames; ++i) {
        usize idx = (g_last_scanned_idx + i) % g_total_frames;

        if (!bitmap_test(idx)) {
            bitmap_set(idx);
            g_used_frames++;
            g_free_frames--;
            g_last_scanned_idx = (idx + 1) % g_total_frames;
            return PhysAddr(idx * PAGE_SIZE);
        }
    }

    panic("PMM: Out of physical memory (page allocation failure)!");
}

PhysAddr Pmm::alloc_frames(usize count) {
    if (count == 0) return PhysAddr(0);
    if (count == 1) return alloc_frame();

    usize consecutive = 0;
    usize start_idx = 0;

    for (usize i = 0; i < g_total_frames; ++i) {
        if (!bitmap_test(i)) {
            if (consecutive == 0) start_idx = i;
            consecutive++;
            if (consecutive == count) {
                for (usize j = 0; j < count; ++j) {
                    bitmap_set(start_idx + j);
                }
                g_used_frames += count;
                g_free_frames -= count;
                return PhysAddr(start_idx * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }

    panic("PMM: Out of contiguous physical memory frames!");
}

void Pmm::free_frame(PhysAddr addr) {
    if (!addr.is_aligned(PAGE_SIZE)) {
        panic("PMM: Attempted to free unaligned physical address!");
    }

    usize idx = addr.as_u64() / PAGE_SIZE;
    if (idx >= g_total_frames) {
        panic("PMM: Attempted to free out-of-bounds physical frame!");
    }

    if (!bitmap_test(idx)) {
        panic("PMM: Duplicate or invalid free detected (double-free)!");
    }

    bitmap_clear(idx);
    g_used_frames--;
    g_free_frames++;
}

void Pmm::free_frames(PhysAddr addr, usize count) {
    for (usize i = 0; i < count; ++i) {
        free_frame(PhysAddr(addr.as_u64() + i * PAGE_SIZE));
    }
}

usize Pmm::get_total_frames() { return g_total_frames; }
usize Pmm::get_used_frames()  { return g_used_frames; }
usize Pmm::get_free_frames()  { return g_free_frames; }

void Pmm::dump_stats() {
    log::write_kv_u64_dec(log::Level::Info, "PMM Total Physical Frames", g_total_frames);
    log::write_kv_u64_dec(log::Level::Info, "PMM Usable Physical Frames", g_free_frames);
    log::write_kv_u64_dec(log::Level::Info, "PMM Used Physical Frames", g_used_frames);
    log::write_kv_u64_dec(log::Level::Info, "PMM Free Memory (MiB)", (g_free_frames * PAGE_SIZE) / (1024 * 1024));
}

} // namespace kernel::memory
