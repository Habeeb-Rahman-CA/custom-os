#pragma once

#include <kernel/core/types.h>

namespace kernel::memory {

constexpr usize PAGE_SIZE = 4096;

class Pmm {
public:
    static void init();

    // Allocate a single 4 KiB physical page frame
    static PhysAddr alloc_frame();

    // Allocate 'count' contiguous 4 KiB physical page frames
    static PhysAddr alloc_frames(usize count);

    // Free a previously allocated page frame
    static void free_frame(PhysAddr addr);

    // Free 'count' contiguous page frames
    static void free_frames(PhysAddr addr, usize count);

    // Statistics
    static usize get_total_frames();
    static usize get_used_frames();
    static usize get_free_frames();

    static usize get_total_bytes() { return get_total_frames() * PAGE_SIZE; }
    static usize get_used_bytes()  { return get_used_frames() * PAGE_SIZE; }
    static usize get_free_bytes()  { return get_free_frames() * PAGE_SIZE; }

    static void dump_stats();
};

} // namespace kernel::memory
