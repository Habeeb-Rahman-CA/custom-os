#pragma once

#include <kernel/core/types.h>

namespace kernel::drivers::keyboard {

void init();

bool has_char();
char get_char();

} // namespace kernel::drivers::keyboard
