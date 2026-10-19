#include <kernel/process/process.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/memory/heap.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>

extern "C" [[noreturn]] void enter_user_mode(kernel::u64 rip, kernel::u64 rsp, kernel::u64 cr3);

namespace kernel::process {

namespace {

u64 g_next_pid = 100;

} // namespace

void ProcessManager::init() {
    log::info("[PROC] Process Manager initialized.");
}

Process* ProcessManager::spawn_from_elf(const char* name, const u8* elf_data, usize elf_size) {
    auto loaded = executable::ElfLoader::load(elf_data, elf_size);
    if (!loaded.valid) {
        log::error("[PROC] Failed to load ELF executable for process!");
        return nullptr;
    }

    auto* proc = new Process();
    proc->pid = g_next_pid++;
    proc->name = name ? name : "user_proc";
    proc->pml4 = loaded.pml4;
    proc->user_entry = loaded.entry_point;
    proc->user_stack = loaded.user_stack_top;

    log::info("[PROC] User process spawned successfully.");
    log::write_kv_u64_dec(log::Level::Info, "Process PID", proc->pid);
    log::write_kv_u64_hex(log::Level::Info, "User Entry RIP", proc->user_entry);
    log::write_kv_u64_hex(log::Level::Info, "User Stack RSP", proc->user_stack);

    return proc;
}

[[noreturn]] void ProcessManager::execute_user_process(Process* proc) {
    if (!proc) {
        panic("Attempted to execute null user process!");
    }

    log::info("[PROC] Jumping into Ring 3 user space execution...");
    enter_user_mode(proc->user_entry, proc->user_stack, proc->pml4.as_u64());

    for (;;) { asm volatile("hlt"); }
}

} // namespace kernel::process

