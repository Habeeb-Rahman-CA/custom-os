#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/spinlock.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/vmm.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/serial.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>

extern "C" void switch_to(kernel::u64* old_rsp, kernel::u64 new_rsp, kernel::u64 new_cr3);
extern "C" void task_start_trampoline();

extern "C" void task_exit_helper() {
    kernel::scheduler::Scheduler::exit();
}

namespace kernel::scheduler {

namespace {

Task* g_current_task = nullptr;
Task* g_task_list_head = nullptr;
u64   g_next_pid = 1;
usize g_task_count = 0;
Spinlock g_sched_lock;

} // namespace

void Scheduler::init() {
    // Create initial task for the currently running kernel execution thread (PID 0)
    auto* main_task = new Task();
    main_task->pid = 0;
    main_task->name = "kinit";
    main_task->state = TaskState::Running;
    main_task->cr3 = memory::Vmm::get_kernel_pml4().as_u64();
    main_task->next = nullptr;

    g_current_task = main_task;
    g_task_list_head = main_task;
    g_task_count = 1;

    log::info("[SCHED] Round-Robin Task Scheduler initialized (PID 0 online).");
}

static Task* create_task_internal(const char* name, void* entry_func, void* arg) {
    LockGuard guard(g_sched_lock);

    auto* task = new Task();
    task->pid = g_next_pid++;
    task->name = name ? name : "task";
    task->state = TaskState::Ready;
    task->stack_size = 16384; // 16 KiB
    task->stack_base = new u8[task->stack_size];
    task->cr3 = memory::Vmm::get_kernel_pml4().as_u64();

    // Setup initial stack for switch_to
    u64 top = (reinterpret_cast<u64>(task->stack_base + task->stack_size)) & ~0xFULL;
    u64* sp = reinterpret_cast<u64*>(top);

    // Return address for ret in switch_to
    *(--sp) = reinterpret_cast<u64>(task_start_trampoline); // Popped by ret

    // Callee-saved registers popped in reverse order by switch_to:
    // switch_to pops: r15, r14, r13, r12, rbp, rbx
    *(--sp) = reinterpret_cast<u64>(entry_func); // Popped into %rbx!
    *(--sp) = 0;                                 // Popped into %rbp
    *(--sp) = reinterpret_cast<u64>(arg);        // Popped into %r12 (passed as %rdi to entry_func)!
    *(--sp) = 0;                                 // Popped into %r13
    *(--sp) = 0;                                 // Popped into %r14
    *(--sp) = 0;                                 // Popped into %r15

    task->rsp = reinterpret_cast<u64>(sp);

    // Insert into task list
    task->next = g_task_list_head;
    g_task_list_head = task;
    g_task_count++;

    return task;
}

Task* Scheduler::create_task(const char* name, void (*entry_func)()) {
    return create_task_internal(name, reinterpret_cast<void*>(entry_func), nullptr);
}

Task* Scheduler::create_task(const char* name, void (*entry_func)(void*), void* arg) {
    return create_task_internal(name, reinterpret_cast<void*>(entry_func), arg);
}

void Scheduler::yield() {
    Task* prev = g_current_task;
    if (!prev) return;

    // Find next ready task
    Task* next_task = nullptr;
    Task* curr = prev->next ? prev->next : g_task_list_head;

    while (curr != prev) {
        if (curr->state == TaskState::Ready) {
            next_task = curr;
            break;
        }
        curr = curr->next ? curr->next : g_task_list_head;
    }

    if (!next_task && prev->state == TaskState::Running) {
        return; // Current task remains running
    }

    if (!next_task && prev->state != TaskState::Running) {
        // Idle halt until an interrupt wakes a task
        while (!next_task) {
            asm volatile("sti; hlt; cli");
            curr = g_task_list_head;
            while (curr) {
                if (curr->state == TaskState::Ready) {
                    next_task = curr;
                    break;
                }
                curr = curr->next;
            }
        }
    }

    if (prev->state == TaskState::Running) {
        prev->state = TaskState::Ready;
    }

    next_task->state = TaskState::Running;
    g_current_task = next_task;

    // Update TSS.rsp0 and g_current_rsp0 to the newly scheduled task's kernel stack
    if (next_task->stack_base && next_task->stack_size > 0) {
        u64 top_stack = reinterpret_cast<u64>(next_task->stack_base + next_task->stack_size);
        arch::x86_64::Gdt::set_kernel_stack(top_stack);
    } else {
        arch::x86_64::Gdt::set_kernel_stack(arch::x86_64::Gdt::get_default_rsp0());
    }

    switch_to(&prev->rsp, next_task->rsp, next_task->cr3);
}

void Scheduler::sleep(u64 ms) {
    if (!g_current_task) return;
    g_current_task->wake_tick = drivers::timer::get_ticks() + (ms / 10);
    g_current_task->state = TaskState::Sleeping;
    yield();
}

void Scheduler::exit() {
    if (!g_current_task) return;
    g_current_task->state = TaskState::Terminated;
    yield();
    // Unreachable
    for (;;) { asm volatile("hlt"); }
}

void Scheduler::on_timer_tick() {
    u64 current_tick = drivers::timer::get_ticks();

    // Wake sleeping tasks
    Task* curr = g_task_list_head;
    while (curr) {
        if (curr->state == TaskState::Sleeping && current_tick >= curr->wake_tick) {
            curr->state = TaskState::Ready;
        }
        curr = curr->next;
    }
}

Task* Scheduler::get_current_task() { return g_current_task; }
usize Scheduler::get_task_count()   { return g_task_count; }

void Scheduler::dump_tasks() {
    log::info("--- Active Task Registry ---");
    Task* curr = g_task_list_head;
    while (curr) {
        drivers::serial::write_string("PID: ");
        drivers::serial::write_u64_dec(curr->pid);
        drivers::serial::write_string(" | Name: ");
        drivers::serial::write_string(curr->name);
        drivers::serial::write_string(" | State: ");
        switch (curr->state) {
            case TaskState::Ready:      drivers::serial::write_string("READY\n"); break;
            case TaskState::Running:    drivers::serial::write_string("RUNNING\n"); break;
            case TaskState::Sleeping:   drivers::serial::write_string("SLEEPING\n"); break;
            case TaskState::Terminated: drivers::serial::write_string("TERMINATED\n"); break;
        }
        curr = curr->next;
    }
    log::info("----------------------------");
}

} // namespace kernel::scheduler
