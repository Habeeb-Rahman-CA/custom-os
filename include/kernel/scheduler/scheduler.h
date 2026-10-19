#pragma once

#include <kernel/core/types.h>

namespace kernel::scheduler {

enum class TaskState {
    Ready,
    Running,
    Sleeping,
    Terminated
};

struct Task {
    u64 pid{0};
    const char* name{"unnamed"};
    TaskState state{TaskState::Ready};

    u64 rsp{0};
    u64 cr3{0};

    u8* stack_base{nullptr};
    usize stack_size{0};

    u64 wake_tick{0};
    Task* next{nullptr};
};

class Scheduler {
public:
    static void init();

    // Create a new kernel task
    static Task* create_task(const char* name, void (*entry_func)());

    // Yield CPU to the next ready task
    static void yield();

    // Sleep current task for specified milliseconds
    static void sleep(u64 ms);

    // Terminate current task
    static void exit();

    // Timer tick hook for preemptive scheduling
    static void on_timer_tick();

    // Diagnostics
    static Task* get_current_task();
    static usize get_task_count();
    static void dump_tasks();
};

} // namespace kernel::scheduler
