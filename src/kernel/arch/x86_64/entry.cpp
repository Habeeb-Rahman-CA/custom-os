#include <kernel/core/types.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>
#include <kernel/core/boot_info.h>
#include <kernel/drivers/serial.h>
#include <kernel/arch/x86_64/io.h>

using namespace kernel;

namespace {

void print_banner() {
    drivers::serial::write_string(
        "\n"
        "======================================================\n"
        "  _____           _                  ____   _____     \n"
        " / ____|         | |                / __ \\ / ____|    \n"
        "| |    _   _ ___| |_ ___  _ __ ___ | |  | | (___      \n"
        "| |   | | | / __| __/ _ \\| '_ ` _ \\| |  | |\\___ \\     \n"
        "| |___| |_| \\__ \\ || (_) | | | | | | |__| |____) |    \n"
        " \\_____\\__,_|___/\\__\\___/|_| |_| |_|\\____/|_____/     \n"
        "   CustomOS Kernel v0.1.0 (x86_64 Freestanding)       \n"
        "======================================================\n"
    );
}

} // namespace

extern "C" void kernel_main() {
    // Stage 1: Serial Driver & Logger Initialization
    drivers::serial::init();
    log::init();

    print_banner();

    log::info("[Stage 1/3] Serial console & early logging online (115200 8N1).");

    // Stage 2: Bootloader Response & Memory Map Processing
    log::info("[Stage 2/3] Parsing Limine bootloader protocol responses...");
    boot::init();
    boot::dump_to_log();

    // Stage 3: Invariant & Assertion Self-Test
    log::info("[Stage 3/3] Verifying early kernel invariants...");
    KASSERT(boot::get_info().valid);
    KASSERT(boot::get_info().hhdm_offset != 0);
    KASSERT(boot::get_info().mem_stats.entry_count > 0);
    KASSERT(boot::get_info().mem_stats.usable_bytes > 0);

    log::info("Kernel invariant self-checks PASSED.");
    log::info(">>> Milestone 1: Toolchain, bootloader, linker & QEMU verified! <<<");
    log::info("Kernel entering safe halt state awaiting next milestone implementation.");

    // Safe idle halt
    arch::x86_64::hlt_loop();
}
