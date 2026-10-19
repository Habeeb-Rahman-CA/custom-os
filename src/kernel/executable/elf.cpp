#include <kernel/executable/elf.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/core/boot_info.h>
#include <kernel/core/logger.h>

namespace kernel::executable {

namespace {

void memory_copy(void* dst, const void* src, usize n) {
    auto* d = reinterpret_cast<u8*>(dst);
    const auto* s = reinterpret_cast<const u8*>(src);
    for (usize i = 0; i < n; ++i) d[i] = s[i];
}

void memory_set(void* dst, u8 val, usize n) {
    auto* d = reinterpret_cast<u8*>(dst);
    for (usize i = 0; i < n; ++i) d[i] = val;
}

} // namespace

LoadedElf ElfLoader::load(const u8* elf_data, usize elf_size) {
    LoadedElf result{};

    if (!elf_data || elf_size < sizeof(Elf64Header)) {
        log::error("[ELF] Image too small to contain valid ELF64 header.");
        return result;
    }

    const auto* header = reinterpret_cast<const Elf64Header*>(elf_data);

    // Validate Magic (\x7fELF)
    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E'  ||
        header->e_ident[2] != 'L'  ||
        header->e_ident[3] != 'F') {
        log::error("[ELF] Invalid ELF magic signature!");
        return result;
    }

    // 64-bit class check
    if (header->e_ident[4] != 2) {
        log::error("[ELF] Unsupported architecture (expected 64-bit ELF)!");
        return result;
    }

    // Little-endian check
    if (header->e_ident[5] != 1) {
        log::error("[ELF] Unsupported endianness (expected little-endian)!");
        return result;
    }

    // x86-64 machine check (0x3E)
    if (header->e_machine != 0x3E) {
        log::error("[ELF] Invalid target machine (expected EM_X86_64)!");
        return result;
    }

    // Create isolated user address space
    PhysAddr process_pml4 = memory::Vmm::create_address_space();
    PhysAddr kernel_pml4 = memory::Vmm::get_kernel_pml4();

    // Switch to new process address space to populate mappings
    memory::Vmm::switch_address_space(process_pml4);

    u64 hhdm = boot::get_info().hhdm_offset;

    // Iterate Program Headers
    const u8* phdr_base = elf_data + header->e_phoff;
    for (u16 i = 0; i < header->e_phnum; ++i) {
        const auto* phdr = reinterpret_cast<const Elf64Phdr*>(phdr_base + (i * header->e_phentsize));

        if (phdr->p_type == PT_LOAD) {
            u64 vaddr_start = phdr->p_vaddr & ~0xFFFULL;
            u64 vaddr_end   = (phdr->p_vaddr + phdr->p_memsz + memory::PAGE_SIZE - 1) & ~0xFFFULL;
            usize num_pages = (vaddr_end - vaddr_start) / memory::PAGE_SIZE;

            u64 flags = memory::PagingFlags::Present | memory::PagingFlags::User;
            if (phdr->p_flags & PF_W) flags |= memory::PagingFlags::Writable;
            if (!(phdr->p_flags & PF_X)) flags |= memory::PagingFlags::NoExecute;

            for (usize p = 0; p < num_pages; ++p) {
                u64 page_virt = vaddr_start + (p * memory::PAGE_SIZE);
                PhysAddr frame = memory::Pmm::alloc_frame();
                u8* frame_ptr = reinterpret_cast<u8*>(hhdm + frame.as_u64());

                // Zero physical frame
                memory_set(frame_ptr, 0, memory::PAGE_SIZE);

                // Check intersection with file payload [phdr->p_vaddr, phdr->p_vaddr + phdr->p_filesz)
                u64 file_start = phdr->p_vaddr;
                u64 file_end   = phdr->p_vaddr + phdr->p_filesz;

                u64 copy_start = (file_start > page_virt) ? file_start : page_virt;
                u64 page_limit = page_virt + memory::PAGE_SIZE;
                u64 copy_end   = (file_end < page_limit) ? file_end : page_limit;

                if (copy_end > copy_start) {
                    usize copy_len = copy_end - copy_start;
                    usize frame_offset = copy_start - page_virt;
                    usize file_offset = phdr->p_offset + (copy_start - phdr->p_vaddr);

                    memory_copy(frame_ptr + frame_offset, elf_data + file_offset, copy_len);
                }

                memory::Vmm::map_page(VirtAddr(page_virt), frame, flags);
            }
        }
    }

    // Allocate 64 KiB User Stack at 0x00007ffffff00000
    constexpr u64 USER_STACK_BASE = 0x00007ffffff00000ULL;
    constexpr usize STACK_PAGES = 16;
    for (usize s = 0; s < STACK_PAGES; ++s) {
        PhysAddr stack_frame = memory::Pmm::alloc_frame();
        memory::Vmm::map_page(VirtAddr(USER_STACK_BASE + (s * memory::PAGE_SIZE)),
                              stack_frame,
                              memory::PagingFlags::Present | memory::PagingFlags::Writable |
                              memory::PagingFlags::User    | memory::PagingFlags::NoExecute);
    }

    u64 user_stack_top = USER_STACK_BASE + (STACK_PAGES * memory::PAGE_SIZE) - 16;

    // Switch back to kernel address space
    memory::Vmm::switch_address_space(kernel_pml4);

    result.entry_point = header->e_entry;
    result.user_stack_top = user_stack_top;
    result.pml4 = process_pml4;
    result.valid = true;

    log::info("[ELF] ELF64 executable validated and mapped into isolated address space.");
    return result;
}

} // namespace kernel::executable
