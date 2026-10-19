#pragma once

#include <kernel/core/types.h>

namespace kernel::console {

class FbConsole {
public:
    static void init();
    static void clear(u32 color = 0x001B2838); // Dark navy default background
    static void put_char(char c, u32 fg = 0x00FFFFFF, u32 bg = 0x001B2838);
    static void write_string(const char* str, u32 fg = 0x00FFFFFF, u32 bg = 0x001B2838);
    static bool is_available();
};

} // namespace kernel::console
