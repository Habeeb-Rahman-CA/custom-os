#pragma once

#include <kernel/core/types.h>

namespace kernel::syscall {

constexpr u64 SYS_EXIT   = 1;
constexpr u64 SYS_READ   = 2;
constexpr u64 SYS_WRITE  = 3;
constexpr u64 SYS_OPEN   = 4;
constexpr u64 SYS_CLOSE  = 5;
constexpr u64 SYS_YIELD  = 6;
constexpr u64 SYS_SLEEP  = 7;
constexpr u64 SYS_GETPID = 8;

void init();

bool validate_user_buffer(const void* ptr, usize size);
bool validate_user_string(const char* str);

u64 dispatch(u64 num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

} // namespace kernel::syscall

