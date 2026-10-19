#pragma once

#include <kernel/core/types.h>
#include <kernel/memory/pmm.h>

namespace kernel::memory {

namespace PagingFlags {
    constexpr u64 Present   = 1ULL << 0;
    constexpr u64 Writable  = 1ULL << 1;
    constexpr u64 User      = 1ULL << 2;
    constexpr u64 WriteThrough = 1ULL << 3;
    constexpr u64 CacheDisable = 1ULL << 4;
    constexpr u64 NoExecute = 1ULL << 63;
}

struct [[gnu::packed]] PageTableEntry {
    u64 value{0};

    [[nodiscard]] bool is_present() const { return (value & PagingFlags::Present) != 0; }
    [[nodiscard]] bool is_writable() const { return (value & PagingFlags::Writable) != 0; }
    [[nodiscard]] bool is_user() const { return (value & PagingFlags::User) != 0; }
    [[nodiscard]] bool is_huge() const { return (value & (1ULL << 7)) != 0; }

    [[nodiscard]] PhysAddr get_phys_addr() const {
        return PhysAddr(value & 0x000FFFFFFFFFF000ULL);
    }

    void set(PhysAddr phys, u64 flags) {
        value = (phys.as_u64() & 0x000FFFFFFFFFF000ULL) | flags;
    }

    void clear() { value = 0; }
};

struct alignas(4096) PageTable {
    PageTableEntry entries[512];
};

class Vmm {
public:
    static void init();

    // Map a single 4 KiB virtual page to a physical page frame
    static void map_page(VirtAddr virt, PhysAddr phys, u64 flags);

    // Unmap a virtual page and flush TLB
    static void unmap_page(VirtAddr virt);

    // Translate virtual address to physical address
    static PhysAddr translate(VirtAddr virt);

    // Flush a single page from the CPU TLB
    static void invlpg(VirtAddr virt);

    // Allocate an empty isolated address space for a new process (copies higher-half kernel)
    static PhysAddr create_address_space();

    // Switch active page directory (%cr3)
    static void switch_address_space(PhysAddr pml4_phys);

    // Get current kernel PML4 physical address
    static PhysAddr get_kernel_pml4();
};

} // namespace kernel::memory

