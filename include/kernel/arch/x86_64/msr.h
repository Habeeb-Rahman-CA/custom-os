#pragma once

#include <kernel/core/types.h>

namespace kernel::arch::x86_64 {

constexpr u32 MSR_EFER  = 0xC0000080;
constexpr u32 MSR_STAR  = 0xC0000081;
constexpr u32 MSR_LSTAR = 0xC0000082;
constexpr u32 MSR_FMASK = 0xC0000084;

[[maybe_unused]] static inline u64 rdmsr(u32 msr) {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return (static_cast<u64>(high) << 32) | low;
}

[[maybe_unused]] static inline void wrmsr(u32 msr, u64 value) {
    u32 low  = static_cast<u32>(value & 0xFFFFFFFF);
    u32 high = static_cast<u32>((value >> 32) & 0xFFFFFFFF);
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
}

} // namespace kernel::arch::x86_64
