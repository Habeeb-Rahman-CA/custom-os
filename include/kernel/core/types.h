#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = size_t;
using isize = int64_t;
using ssize_t = int64_t;
using uptr  = uintptr_t;

// Physical and Virtual address wrappers for type safety
struct PhysAddr {
    u64 val{0};

    constexpr PhysAddr() = default;
    constexpr explicit PhysAddr(u64 addr) : val(addr) {}

    [[nodiscard]] constexpr u64 as_u64() const { return val; }
    [[nodiscard]] constexpr bool is_aligned(u64 alignment) const {
        return (val & (alignment - 1)) == 0;
    }
};

struct VirtAddr {
    u64 val{0};

    constexpr VirtAddr() = default;
    constexpr explicit VirtAddr(u64 addr) : val(addr) {}

    [[nodiscard]] constexpr u64 as_u64() const { return val; }
    [[nodiscard]] constexpr void* as_ptr() const { return reinterpret_cast<void*>(val); }
    [[nodiscard]] constexpr bool is_aligned(u64 alignment) const {
        return (val & (alignment - 1)) == 0;
    }
};

} // namespace kernel

