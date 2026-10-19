#include <kernel/memory/vmm.h>
#include <kernel/core/boot_info.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>

namespace kernel::memory {

namespace {

PhysAddr g_kernel_pml4{0};

inline u64 get_hhdm() {
    return boot::get_info().hhdm_offset;
}

inline PageTable* phys_to_virt(PhysAddr phys) {
    return reinterpret_cast<PageTable*>(get_hhdm() + phys.as_u64());
}

PhysAddr read_cr3() {
    u64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return PhysAddr(cr3 & 0x000FFFFFFFFFF000ULL);
}

void write_cr3(PhysAddr pml4) {
    asm volatile("mov %0, %%cr3" : : "r"(pml4.as_u64()) : "memory");
}

} // namespace

void Vmm::invlpg(VirtAddr virt) {
    asm volatile("invlpg (%0)" : : "r"(virt.as_u64()) : "memory");
}

void Vmm::init() {
    g_kernel_pml4 = read_cr3();
    if (g_kernel_pml4.as_u64() == 0) {
        panic("VMM: Failed to read kernel PML4 from CR3!");
    }

    log::info("[VMM] 4-level Paging Virtual Memory Manager initialized.");
    log::write_kv_u64_hex(log::Level::Info, "Kernel PML4 Physical Base", g_kernel_pml4.as_u64());
}

PhysAddr Vmm::get_kernel_pml4() {
    return g_kernel_pml4;
}

void Vmm::switch_address_space(PhysAddr pml4_phys) {
    write_cr3(pml4_phys);
}

void Vmm::map_page(VirtAddr virt, PhysAddr phys, u64 flags) {
    u64 v = virt.as_u64();
    u64 pml4_idx = (v >> 39) & 0x1FF;
    u64 pdpt_idx = (v >> 30) & 0x1FF;
    u64 pd_idx   = (v >> 21) & 0x1FF;
    u64 pt_idx   = (v >> 12) & 0x1FF;

    PageTable* pml4 = phys_to_virt(read_cr3());

    // 1. Traverse or allocate PDPT
    if (!pml4->entries[pml4_idx].is_present()) {
        PhysAddr new_frame = Pmm::alloc_frame();
        PageTable* new_tbl = phys_to_virt(new_frame);
        for (int i = 0; i < 512; ++i) new_tbl->entries[i].clear();
        pml4->entries[pml4_idx].set(new_frame, PagingFlags::Present | PagingFlags::Writable | (flags & PagingFlags::User));
    } else if (flags & PagingFlags::User) {
        pml4->entries[pml4_idx].value |= PagingFlags::User | PagingFlags::Writable;
    }
    PageTable* pdpt = phys_to_virt(pml4->entries[pml4_idx].get_phys_addr());

    // 2. Traverse or allocate Page Directory
    if (!pdpt->entries[pdpt_idx].is_present()) {
        PhysAddr new_frame = Pmm::alloc_frame();
        PageTable* new_tbl = phys_to_virt(new_frame);
        for (int i = 0; i < 512; ++i) new_tbl->entries[i].clear();
        pdpt->entries[pdpt_idx].set(new_frame, PagingFlags::Present | PagingFlags::Writable | (flags & PagingFlags::User));
    } else if (flags & PagingFlags::User) {
        pdpt->entries[pdpt_idx].value |= PagingFlags::User | PagingFlags::Writable;
    }
    PageTable* pd = phys_to_virt(pdpt->entries[pdpt_idx].get_phys_addr());

    // 3. Traverse or allocate Page Table
    if (!pd->entries[pd_idx].is_present()) {
        PhysAddr new_frame = Pmm::alloc_frame();
        PageTable* new_tbl = phys_to_virt(new_frame);
        for (int i = 0; i < 512; ++i) new_tbl->entries[i].clear();
        pd->entries[pd_idx].set(new_frame, PagingFlags::Present | PagingFlags::Writable | (flags & PagingFlags::User));
    } else if (flags & PagingFlags::User) {
        pd->entries[pd_idx].value |= PagingFlags::User | PagingFlags::Writable;
    }
    PageTable* pt = phys_to_virt(pd->entries[pd_idx].get_phys_addr());

    // 4. Set leaf page mapping
    pt->entries[pt_idx].set(phys, flags | PagingFlags::Present);
    invlpg(virt);
}

void Vmm::unmap_page(VirtAddr virt) {
    u64 v = virt.as_u64();
    u64 pml4_idx = (v >> 39) & 0x1FF;
    u64 pdpt_idx = (v >> 30) & 0x1FF;
    u64 pd_idx   = (v >> 21) & 0x1FF;
    u64 pt_idx   = (v >> 12) & 0x1FF;

    PageTable* pml4 = phys_to_virt(read_cr3());
    if (!pml4->entries[pml4_idx].is_present()) return;

    PageTable* pdpt = phys_to_virt(pml4->entries[pml4_idx].get_phys_addr());
    if (!pdpt->entries[pdpt_idx].is_present()) return;

    PageTable* pd = phys_to_virt(pdpt->entries[pdpt_idx].get_phys_addr());
    if (!pd->entries[pd_idx].is_present()) return;

    PageTable* pt = phys_to_virt(pd->entries[pd_idx].get_phys_addr());
    pt->entries[pt_idx].clear();

    invlpg(virt);
}

PhysAddr Vmm::translate(VirtAddr virt) {
    u64 v = virt.as_u64();
    u64 pml4_idx = (v >> 39) & 0x1FF;
    u64 pdpt_idx = (v >> 30) & 0x1FF;
    u64 pd_idx   = (v >> 21) & 0x1FF;
    u64 pt_idx   = (v >> 12) & 0x1FF;
    u64 offset   = v & 0xFFF;

    PageTable* pml4 = phys_to_virt(read_cr3());
    if (!pml4->entries[pml4_idx].is_present()) return PhysAddr(0);

    PageTable* pdpt = phys_to_virt(pml4->entries[pml4_idx].get_phys_addr());
    if (!pdpt->entries[pdpt_idx].is_present()) return PhysAddr(0);

    PageTable* pd = phys_to_virt(pdpt->entries[pdpt_idx].get_phys_addr());
    if (!pd->entries[pd_idx].is_present()) return PhysAddr(0);

    // 2 MiB huge page check
    if (pd->entries[pd_idx].is_huge()) {
        u64 huge_offset = v & 0x1FFFFF;
        return PhysAddr(pd->entries[pd_idx].get_phys_addr().as_u64() + huge_offset);
    }

    PageTable* pt = phys_to_virt(pd->entries[pd_idx].get_phys_addr());
    if (!pt->entries[pt_idx].is_present()) return PhysAddr(0);

    return PhysAddr(pt->entries[pt_idx].get_phys_addr().as_u64() + offset);
}

PhysAddr Vmm::create_address_space() {
    PhysAddr pml4_frame = Pmm::alloc_frame();
    PageTable* new_pml4 = phys_to_virt(pml4_frame);
    PageTable* kernel_pml4 = phys_to_virt(g_kernel_pml4);

    // Clear lower half (User space: entries 0-255)
    for (int i = 0; i < 256; ++i) {
        new_pml4->entries[i].clear();
    }

    // Mirror higher half (Kernel space: entries 256-511)
    for (int i = 256; i < 512; ++i) {
        new_pml4->entries[i] = kernel_pml4->entries[i];
    }

    return pml4_frame;
}

} // namespace kernel::memory
