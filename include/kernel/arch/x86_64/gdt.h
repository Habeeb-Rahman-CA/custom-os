#pragma once

#include <kernel/core/types.h>

namespace kernel::arch::x86_64 {

struct [[gnu::packed]] GdtDescriptor {
    u16 limit_low{0};
    u16 base_low{0};
    u8  base_mid{0};
    u8  access{0};
    u8  granularity{0};
    u8  base_high{0};
};

struct [[gnu::packed]] TssDescriptor {
    u16 limit_low{0};
    u16 base_low{0};
    u8  base_mid{0};
    u8  access{0};
    u8  granularity{0};
    u8  base_mid_high{0};
    u32 base_high{0};
    u32 reserved{0};
};

struct [[gnu::packed]] Tss {
    u32 reserved0{0};
    u64 rsp0{0};
    u64 rsp1{0};
    u64 rsp2{0};
    u64 reserved1{0};
    u64 ist1{0};
    u64 ist2{0};
    u64 ist3{0};
    u64 ist4{0};
    u64 ist5{0};
    u64 ist6{0};
    u64 ist7{0};
    u64 reserved2{0};
    u16 reserved3{0};
    u16 iomap_base{sizeof(Tss)};
};

struct [[gnu::packed]] GdtPointer {
    u16 limit{0};
    u64 base{0};
};

// Selectors
constexpr u16 KERNEL_CODE_SELECTOR = 0x08;
constexpr u16 KERNEL_DATA_SELECTOR = 0x10;
constexpr u16 USER_DATA_SELECTOR   = 0x18 | 3;
constexpr u16 USER_CODE_SELECTOR   = 0x20 | 3;
constexpr u16 TSS_SELECTOR         = 0x28;

class Gdt {
public:
    static void init();
    static void set_kernel_stack(u64 rsp0);
    static u64 get_default_rsp0();
    static Tss& get_tss();
};

} // namespace kernel::arch::x86_64

extern "C" kernel::u64 g_current_rsp0;

