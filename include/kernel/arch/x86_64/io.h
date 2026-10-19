#pragma once

#include <kernel/core/types.h>

namespace kernel::arch::x86_64 {

[[maybe_unused]] static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

[[maybe_unused]] static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

[[maybe_unused]] static inline void outw(u16 port, u16 val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

[[maybe_unused]] static inline u16 inw(u16 port) {
    u16 ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

[[maybe_unused]] static inline void outl(u16 port, u32 val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

[[maybe_unused]] static inline u32 inl(u16 port) {
    u32 ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

[[maybe_unused]] static inline void io_wait() {
    // Port 0x80 is commonly used as an unused port for small I/O delay
    outb(0x80, 0);
}

[[maybe_unused]] static inline void cli() {
    asm volatile("cli" ::: "memory");
}

[[maybe_unused]] static inline void sti() {
    asm volatile("sti" ::: "memory");
}

[[noreturn]] static inline void hlt_loop() {
    for (;;) {
        asm volatile("cli; hlt");
    }
}

} // namespace kernel::arch::x86_64
