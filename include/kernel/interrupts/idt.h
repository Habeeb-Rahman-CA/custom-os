#pragma once

#include <kernel/core/types.h>

namespace kernel::interrupts {

struct [[gnu::packed]] InterruptFrame {
    // Preserved general purpose registers (pushed in reverse order of pop)
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    // Vector and error code
    u64 vector;
    u64 error_code;

    // Pushed automatically by CPU on interrupt
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};

struct [[gnu::packed]] IdtEntry {
    u16 offset_low{0};
    u16 selector{0};
    u8  ist{0};
    u8  type_attr{0};
    u16 offset_mid{0};
    u32 offset_high{0};
    u32 zero{0};

    void set_handler(u64 handler_addr, u16 sel = 0x08, u8 ist_index = 0, u8 dpl = 0);
};

struct [[gnu::packed]] IdtPointer {
    u16 limit{0};
    u64 base{0};
};

using HandlerFunc = void (*)(InterruptFrame* frame);

class Idt {
public:
    static void init();
    static void register_handler(u8 vector, HandlerFunc handler);
    static void dump_frame(const InterruptFrame* frame);
};

} // namespace kernel::interrupts

