#pragma once

#include <kernel/core/types.h>

namespace kernel {

[[noreturn]] void panic(const char* message, const char* file = nullptr, int line = 0);

} // namespace kernel

#define KASSERT(cond)                                                          \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::kernel::panic("Assertion failed: " #cond, __FILE__, __LINE__);   \
        }                                                                      \
    } while (0)

