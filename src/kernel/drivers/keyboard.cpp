#include <kernel/drivers/keyboard.h>
#include <kernel/arch/x86_64/io.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/pic.h>
#include <kernel/core/logger.h>

namespace kernel::drivers::keyboard {

namespace {

constexpr u16 KEYBOARD_DATA_PORT = 0x60;
constexpr usize BUFFER_SIZE = 128;

char g_char_buffer[BUFFER_SIZE]{};
usize g_head = 0;
usize g_tail = 0;
bool g_shift = false;

// Scancode Set 1 standard ASCII mapping
const char scancode_map[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

const char scancode_shift_map[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void keyboard_irq_handler(interrupts::InterruptFrame* frame) {
    (void)frame;
    u8 scancode = arch::x86_64::inb(KEYBOARD_DATA_PORT);

    // Left or right shift press
    if (scancode == 0x2A || scancode == 0x36) {
        g_shift = true;
        return;
    }
    // Left or right shift release
    if (scancode == 0xAA || scancode == 0xB6) {
        g_shift = false;
        return;
    }

    // Ignore key release codes (bit 7 set)
    if (scancode & 0x80) {
        return;
    }

    char c = g_shift ? scancode_shift_map[scancode] : scancode_map[scancode];
    if (c != 0) {
        usize next_head = (g_head + 1) % BUFFER_SIZE;
        if (next_head != g_tail) {
            g_char_buffer[g_head] = c;
            g_head = next_head;
        }
    }
}

} // namespace

void init() {
    interrupts::Idt::register_handler(33, keyboard_irq_handler);
    interrupts::Pic::unmask_irq(1);
    log::info("[KEYBOARD] PS/2 keyboard driver initialized (IRQ 1 unmasked).");
}

bool has_char() {
    return g_head != g_tail;
}

char get_char() {
    while (!has_char()) {
        asm volatile("sti; hlt; cli");
    }
    char c = g_char_buffer[g_tail];
    g_tail = (g_tail + 1) % BUFFER_SIZE;
    return c;
}

} // namespace kernel::drivers::keyboard
