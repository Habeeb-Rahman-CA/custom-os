#include <kernel/drivers/timer.h>
#include <kernel/arch/x86_64/io.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/pic.h>
#include <kernel/core/logger.h>
#include <kernel/scheduler/scheduler.h>

namespace kernel::drivers::timer {

namespace {

constexpr u16 PIT_CHANNEL0 = 0x40;
constexpr u16 PIT_COMMAND  = 0x43;
constexpr u32 PIT_BASE_FREQ = 1193182;

volatile u64 g_ticks = 0;
u32 g_frequency = 100;

void timer_irq_handler(interrupts::InterruptFrame* frame) {
    (void)frame;
    g_ticks = g_ticks + 1;
    scheduler::Scheduler::on_timer_tick();
}

} // namespace

void init(u32 frequency_hz) {
    g_frequency = frequency_hz;
    u16 divisor = static_cast<u16>(PIT_BASE_FREQ / frequency_hz);

    // Command byte: Channel 0, lobyte/hibyte, Mode 3 (Square Wave Generator), 16-bit binary
    arch::x86_64::outb(PIT_COMMAND, 0x36);
    arch::x86_64::outb(PIT_CHANNEL0, static_cast<u8>(divisor & 0xFF));
    arch::x86_64::outb(PIT_CHANNEL0, static_cast<u8>((divisor >> 8) & 0xFF));

    // Register handler for IRQ 0 (Vector 32)
    interrupts::Idt::register_handler(32, timer_irq_handler);

    // Unmask IRQ 0 on PIC
    interrupts::Pic::unmask_irq(0);

    log::info("[TIMER] PIT timer configured at 100 Hz (IRQ 0 unmasked).");
}

u64 get_ticks() {
    return g_ticks;
}

u64 get_uptime_ms() {
    return (g_ticks * 1000) / g_frequency;
}

u64 get_uptime_sec() {
    return get_uptime_ms() / 1000;
}

void sleep_ms(u64 ms) {
    u64 target = get_uptime_ms() + ms;
    while (get_uptime_ms() < target) {
        asm volatile("sti; hlt; cli");
    }
}

} // namespace kernel::drivers::timer
