#pragma once

#include <kernel/core/types.h>
#include <kernel/core/limine.h>

namespace kernel::boot {

struct MemoryStats {
    u64 total_bytes{0};
    u64 usable_bytes{0};
    u64 reserved_bytes{0};
    u64 reclaimable_bytes{0};
    u64 entry_count{0};
};

struct FramebufferInfo {
    u64 address{0};
    u64 width{0};
    u64 height{0};
    u64 pitch{0};
    u16 bpp{0};
    bool available{false};
};

struct BootInfo {
    const char* bootloader_name{"Unknown"};
    const char* bootloader_version{"Unknown"};
    u64 hhdm_offset{0};
    MemoryStats mem_stats;
    FramebufferInfo fb_info;
    bool valid{false};
};

// Process Limine bootloader responses and populate BootInfo
BootInfo& get_info();
void init();
void dump_to_log();

} // namespace kernel::boot
