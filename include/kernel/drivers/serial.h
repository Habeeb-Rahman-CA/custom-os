#pragma once

#include <kernel/core/types.h>

namespace kernel::drivers::serial {

// Standard COM1 base I/O port
constexpr u16 COM1 = 0x3F8;

// Initialize COM1 with 115200 baud, 8N1, FIFO enabled
void init(u16 port = COM1);

// Write a single character to serial port
void write_char(char c, u16 port = COM1);

// Write a null-terminated string to serial port
void write_string(const char* str, u16 port = COM1);

// Write an unsigned 64-bit integer formatted in hexadecimal (with 0x prefix)
void write_u64_hex(u64 val, u16 port = COM1);

// Write an unsigned 64-bit integer formatted in decimal
void write_u64_dec(u64 val, u16 port = COM1);

} // namespace kernel::drivers::serial
