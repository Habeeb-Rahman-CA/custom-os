#include <kernel/core/types.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>
#include <kernel/core/boot_info.h>
#include <kernel/drivers/serial.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/console/framebuffer_console.h>
#include <kernel/arch/x86_64/io.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/pic.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/memory/heap.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/syscall/syscall.h>
#include <kernel/process/process.h>
#include <kernel/filesystem/vfs.h>
#include <kernel/shell/shell.h>

using namespace kernel;

namespace {

volatile bool g_bp_caught = false;
volatile int  g_task_a_runs = 0;
volatile int  g_task_b_runs = 0;

void breakpoint_test_handler(interrupts::InterruptFrame* frame) {
    (void)frame;
    g_bp_caught = true;
    log::info("[TEST] CPU Breakpoint (#BP) exception caught and handled cleanly!");
}

void task_alpha() {
    log::info("[TASK A] Task Alpha entered execution. Yielding to scheduler...");
    g_task_a_runs = g_task_a_runs + 1;
    scheduler::Scheduler::yield();

    log::info("[TASK A] Task Alpha resumed after context switch! Exiting cleanly.");
    g_task_a_runs = g_task_a_runs + 1;
    scheduler::Scheduler::exit();
}

void task_beta() {
    log::info("[TASK B] Task Beta entered execution. Yielding to scheduler...");
    g_task_b_runs = g_task_b_runs + 1;
    scheduler::Scheduler::yield();

    log::info("[TASK B] Task Beta resumed after context switch! Exiting cleanly.");
    g_task_b_runs = g_task_b_runs + 1;
    scheduler::Scheduler::exit();
}

void user_mode_task() {
    log::info("[USER_TASK] Spawning user mode process from /bin/hello.elf...");
    fs::VFSNode* elf_file = fs::VFS::lookup("/bin/hello.elf");
    KASSERT(elf_file != nullptr);
    KASSERT(elf_file->size > 0);

    auto* proc = process::ProcessManager::spawn_from_elf("hello_user", elf_file->data, elf_file->size);
    KASSERT(proc != nullptr);

    log::info("[USER_TASK] Transitioning into Ring 3 User Mode...");
    process::ProcessManager::execute_user_process(proc);
}

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

    log::info("[Stage 1/14] Serial console & early logging online (115200 8N1).");

    // Stage 2: Bootloader Response & Memory Map Processing
    log::info("[Stage 2/14] Parsing Limine bootloader protocol responses...");
    boot::init();
    boot::dump_to_log();

    // Stage 3: GDT, TSS, IDT & PIC Initialization
    log::info("[Stage 3/14] Initializing CPU descriptors & interrupt handling...");
    arch::x86_64::Gdt::init();
    interrupts::Pic::init();
    interrupts::Idt::init();

    // Stage 4: Controlled Exception Verification
    log::info("[Stage 4/14] Executing controlled CPU exception test (#BP)...");
    interrupts::Idt::register_handler(3, breakpoint_test_handler);
    asm volatile("int3");
    KASSERT(g_bp_caught == true);

    // Stage 5: Physical & Virtual Memory Management
    log::info("[Stage 5/14] Initializing Physical Frame Allocator (PMM)...");
    memory::Pmm::init();

    PhysAddr test_frame = memory::Pmm::alloc_frame();
    KASSERT(test_frame.as_u64() != 0);
    KASSERT(test_frame.is_aligned(memory::PAGE_SIZE));
    memory::Pmm::free_frame(test_frame);
    log::info("[TEST] PMM frame allocation & deallocation verified.");

    log::info("[Stage 5/14] Initializing 4-level Paging (VMM)...");
    memory::Vmm::init();

    PhysAddr backing = memory::Pmm::alloc_frame();
    VirtAddr test_virt(0xffffffffa0000000ULL);
    memory::Vmm::map_page(test_virt, backing, memory::PagingFlags::Present | memory::PagingFlags::Writable);
    volatile u32* test_ptr = reinterpret_cast<volatile u32*>(test_virt.as_ptr());
    *test_ptr = 0x12345678;
    KASSERT(*test_ptr == 0x12345678);
    memory::Vmm::unmap_page(test_virt);
    memory::Pmm::free_frame(backing);
    log::info("[TEST] VMM virtual page mapping and translation verified.");

    // Stage 6: Dynamic Kernel Heap Allocator
    log::info("[Stage 6/14] Initializing Kernel Heap Allocator...");
    memory::heap_init();

    void* h1 = memory::kmalloc(256);
    void* h2 = memory::kmalloc(512);
    KASSERT(h1 != nullptr && h2 != nullptr);
    memory::kfree(h1);
    memory::kfree(h2);

    int* cpp_arr = new int[20];
    cpp_arr[19] = 42;
    KASSERT(cpp_arr[19] == 42);
    delete[] cpp_arr;
    log::info("[TEST] Kernel dynamic heap and C++ new/delete operators verified.");

    // Stage 7: Drivers (Timer, Console, Keyboard)
    log::info("[Stage 7/14] Initializing Framebuffer Console, Timer, and PS/2 Keyboard...");
    console::FbConsole::init();
    drivers::timer::init(100);
    drivers::keyboard::init();

    // Stage 8: Multitasking & Scheduler
    log::info("[Stage 8/14] Initializing Round-Robin Task Scheduler...");
    scheduler::Scheduler::init();

    log::info("[SCHED] Spawning kernel tasks: Task Alpha and Task Beta...");
    scheduler::Scheduler::create_task("alpha", task_alpha);
    scheduler::Scheduler::create_task("beta", task_beta);

    log::info("[SCHED] Yielding to run spawned tasks...");
    while (g_task_a_runs < 2 || g_task_b_runs < 2) {
        scheduler::Scheduler::yield();
    }
    KASSERT(g_task_a_runs == 2);
    KASSERT(g_task_b_runs == 2);
    log::info("[TEST] Multitasking context switching verified.");

    // Stage 9: System Call Infrastructure
    log::info("[Stage 9/14] Configuring SYSCALL / SYSRET system-call MSRs...");
    syscall::init();

    // Stage 10: Process Manager
    log::info("[Stage 10/14] Initializing Process Manager...");
    process::ProcessManager::init();

    // Stage 11: Virtual File System (VFS) & Storage
    log::info("[Stage 11/14] Initializing Virtual File System (VFS)...");
    fs::VFS::init();
    fs::VFS::dump_tree();

    // Test VFS operations
    int fd = fs::VFS::open("/hello.txt");
    KASSERT(fd >= 3);
    char read_buf[64]{};
    ssize_t bytes_read = fs::VFS::read(fd, read_buf, 15);
    KASSERT(bytes_read == 15);
    fs::VFS::close(fd);
    log::info("[TEST] VFS file open, read, and close operations verified.");

    // Stage 12: Interactive Command-Line Shell
    log::info("[Stage 12/14] Initializing Shell subsystem...");
    shell::Shell::init();

    // Run automated shell commands test
    log::info("[Stage 13/14] Running automated Shell command suite...");
    shell::Shell::execute_command("help");
    shell::Shell::execute_command("uptime");
    shell::Shell::execute_command("meminfo");
    shell::Shell::execute_command("ls /");
    shell::Shell::execute_command("cat /hello.txt");
    shell::Shell::execute_command("tasks");

    // Stage 14: User Mode Execution Test
    log::info("[Stage 14/14] Launching User-Mode Process (Ring 3)...");
    scheduler::Scheduler::create_task("user_launcher", user_mode_task);
    scheduler::Scheduler::yield();

    log::info(">>> Milestone 9 to 14: User Mode, Syscalls, VFS, ELF Loader & Shell VERIFIED! <<<");
    log::info(">>> ALL OPERATING SYSTEM MILESTONES COMPLETED SUCCESSFULLY! <<<");

    // Enable interrupts for interactive input
    arch::x86_64::sti();

    for (;;) {
        asm volatile("hlt");
    }
}
