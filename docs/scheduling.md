# Multitasking & Scheduling Architecture

## 1. Concurrency Model

CustomOS implements a hybrid multitasking model progressing across milestones:
- **Milestone 7**: Cooperative kernel threads switching via explicit voluntary yield (`sched_yield`).
- **Milestone 8**: Preemptive multitasking driven by hardware timer interrupts (PIT / Local APIC).

---

## 2. Task Control Block (TCB)

Each task is represented by a `Task` structure containing:
- **Task Identity**: Unique `PID`, task name, parent PID.
- **Execution State**: `READY`, `RUNNING`, `SLEEPING`, `TERMINATED`.
- **Kernel Stack**: Dedicated page-aligned stack buffer.
- **Saved CPU Context**: Callee-saved registers (`RBX`, `RBP`, `R12`, `R13`, `R14`, `R15`, `RSP`, `RIP`).
- **Address Space**: Pointer to the task's page directory table (`CR3`).
- **Scheduling Metrics**: CPU time, slice remaining, sleep tick deadline.

---

## 3. Context Switching (`switch_to`)

Context switching is split into architecture-specific assembly and generic policy:

```assembly
/* switch_to(Task* old_task, Task* new_task) */
.global switch_to
switch_to:
    /* 1. Save callee-saved registers of old_task */
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15

    /* 2. Save current RSP to old_task->rsp */
    mov %rsp, (%rdi)

    /* 3. Load RSP from new_task->rsp */
    mov (%rsi), %rsp

    /* 4. Restore callee-saved registers of new_task */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx

    /* 5. Return to new_task's RIP */
    ret
```

---

## 4. Scheduling Policy

- **Round-Robin**: Tasks in the `READY` queue receive a fixed time slice (quantum e.g. 10ms).
- When a timer interrupt occurs:
  1. Decrement current task's remaining quantum.
  2. If quantum expires, mark task `READY`, append to tail of run queue, and switch to next task.
- Sleeping tasks are queued in a timer list and awakened when current ticks exceed their deadline.
