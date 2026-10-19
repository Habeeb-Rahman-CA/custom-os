#include <kernel/interrupts/pic.h>
#include <kernel/arch/x86_64/io.h>
#include <kernel/core/logger.h>

namespace kernel::interrupts {

namespace {

constexpr u16 PIC1_COMMAND = 0x20;
constexpr u16 PIC1_DATA    = 0x21;
constexpr u16 PIC2_COMMAND = 0xA0;
constexpr u16 PIC2_DATA    = 0xA1;

constexpr u8 ICW1_INIT = 0x10;
constexpr u8 ICW1_ICW4 = 0x01;
constexpr u8 ICW4_8086 = 0x01;
constexpr u8 PIC_EOI   = 0x20;

} // namespace

void Pic::init() {
    // Save existing masks
    u8 a1 = arch::x86_64::inb(PIC1_DATA);
    u8 a2 = arch::x86_64::inb(PIC2_DATA);

    // ICW1: Start initialization in cascade mode
    arch::x86_64::outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    arch::x86_64::io_wait();
    arch::x86_64::outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    arch::x86_64::io_wait();

    // ICW2: Vector offsets (Master = 32, Slave = 40)
    arch::x86_64::outb(PIC1_DATA, 0x20);
    arch::x86_64::io_wait();
    arch::x86_64::outb(PIC2_DATA, 0x28);
    arch::x86_64::io_wait();

    // ICW3: Master has slave on IRQ2 (0000 0100 = 4), Slave cascade identity (2)
    arch::x86_64::outb(PIC1_DATA, 0x04);
    arch::x86_64::io_wait();
    arch::x86_64::outb(PIC2_DATA, 0x02);
    arch::x86_64::io_wait();

    // ICW4: 8086/88 mode
    arch::x86_64::outb(PIC1_DATA, ICW4_8086);
    arch::x86_64::io_wait();
    arch::x86_64::outb(PIC2_DATA, ICW4_8086);
    arch::x86_64::io_wait();

    // Mask all IRQs initially except cascade IRQ2
    arch::x86_64::outb(PIC1_DATA, 0xFB); // 1111 1011 (IRQ2 unmasked for cascade)
    arch::x86_64::outb(PIC2_DATA, 0xFF); // All slave IRQs masked

    (void)a1; (void)a2;
    log::info("[PIC] 8259 PIC remapped to vectors 32-47 with IRQs masked.");
}

void Pic::send_eoi(u8 irq) {
    if (irq >= 8) {
        arch::x86_64::outb(PIC2_COMMAND, PIC_EOI);
    }
    arch::x86_64::outb(PIC1_COMMAND, PIC_EOI);
}

void Pic::mask_irq(u8 irq) {
    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    u8 offset = (irq < 8) ? irq : (irq - 8);
    u8 mask = arch::x86_64::inb(port) | (1 << offset);
    arch::x86_64::outb(port, mask);
}

void Pic::unmask_irq(u8 irq) {
    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    u8 offset = (irq < 8) ? irq : (irq - 8);
    u8 mask = arch::x86_64::inb(port) & ~(1 << offset);
    arch::x86_64::outb(port, mask);
}

void Pic::disable() {
    arch::x86_64::outb(PIC1_DATA, 0xFF);
    arch::x86_64::outb(PIC2_DATA, 0xFF);
}

} // namespace kernel::interrupts

