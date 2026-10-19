#include <kernel/drivers/serial.h>
#include <kernel/arch/x86_64/io.h>

namespace kernel::drivers::serial {

namespace {

bool is_transmit_empty(u16 port) {
    return (arch::x86_64::inb(port + 5) & 0x20) != 0;
}

} // namespace

void init(u16 port) {
    // Disable interrupts
    arch::x86_64::outb(port + 1, 0x00);

    // Enable DLAB (set baud rate divisor)
    arch::x86_64::outb(port + 3, 0x80);

    // Set divisor to 1 (115200 baud)
    arch::x86_64::outb(port + 0, 0x01);
    arch::x86_64::outb(port + 1, 0x00);

    // 8 bits, no parity, one stop bit (8N1)
    arch::x86_64::outb(port + 3, 0x03);

    // Enable FIFO, clear TX/RX queues, 14-byte threshold
    arch::x86_64::outb(port + 2, 0xC7);

    // Set RTS/DSR, Auxiliary Output 2 (enables IRQs in hardware, though disabled in IER)
    arch::x86_64::outb(port + 4, 0x0B);
}

void write_char(char c, u16 port) {
    // If newline, prepend carriage return for proper terminal display
    if (c == '\n') {
        while (!is_transmit_empty(port)) {
            arch::x86_64::io_wait();
        }
        arch::x86_64::outb(port, '\r');
    }

    while (!is_transmit_empty(port)) {
        arch::x86_64::io_wait();
    }
    arch::x86_64::outb(port, static_cast<u8>(c));
}

void write_string(const char* str, u16 port) {
    if (!str) return;
    for (usize i = 0; str[i] != '\0'; ++i) {
        write_char(str[i], port);
    }
}

void write_u64_hex(u64 val, u16 port) {
    write_string("0x", port);
    const char hex_chars[] = "0123456789abcdef";
    bool leading = true;
    for (int shift = 60; shift >= 0; shift -= 4) {
        u8 nibble = static_cast<u8>((val >> shift) & 0x0F);
        if (nibble != 0 || shift == 0) {
            leading = false;
        }
        if (!leading) {
            write_char(hex_chars[nibble], port);
        }
    }
}

void write_u64_dec(u64 val, u16 port) {
    if (val == 0) {
        write_char('0', port);
        return;
    }
    char buf[21];
    int idx = 0;
    while (val > 0) {
        buf[idx++] = static_cast<char>('0' + (val % 10));
        val /= 10;
    }
    for (int i = idx - 1; i >= 0; --i) {
        write_char(buf[i], port);
    }
}

} // namespace kernel::drivers::serial
