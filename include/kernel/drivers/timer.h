#pragma once

#include <kernel/core/types.h>

namespace kernel::drivers::timer {

void init(u32 frequency_hz = 100);

u64 get_ticks();
u64 get_uptime_ms();
u64 get_uptime_sec();

void sleep_ms(u64 ms);

} // namespace kernel::drivers::timer
