#include <kernel/core/panic.h>
#include <kernel/core/logger.h>
#include <kernel/arch/x86_64/io.h>
#include <kernel/drivers/serial.h>

namespace kernel {

[[noreturn]] void panic(const char* message, const char* file, int line) {
    arch::x86_64::cli();

    drivers::serial::write_string("\n==================== KERNEL PANIC ====================\n");
    if (message) {
        log::write(log::Level::Panic, message);
    } else {
        log::write(log::Level::Panic, "Unknown fatal error encountered.");
    }

    if (file) {
        drivers::serial::write_string("[PANIC] Location: ");
        drivers::serial::write_string(file);
        drivers::serial::write_char(':');
        drivers::serial::write_u64_dec(static_cast<u64>(line));
        drivers::serial::write_char('\n');
    }

    drivers::serial::write_string("[PANIC] System halted. Halting CPU cores...\n");
    drivers::serial::write_string("=======================================================\n");

    arch::x86_64::hlt_loop();
}

} // namespace kernel
