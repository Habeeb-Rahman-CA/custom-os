/*
 * Context Switching for x86-64
 * C signature: extern "C" void switch_to(u64* old_rsp, u64 new_rsp, u64 new_cr3);
 * Arguments:
 *   %rdi: old_rsp pointer
 *   %rsi: new_rsp value
 *   %rdx: new_cr3 value
 */

.section .text
.global switch_to
.type switch_to, @function

switch_to:
    /* Save current task's callee-saved registers */
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15

    /* Save current RSP to old task TCB */
    mov %rsp, (%rdi)

    /* Switch CR3 if necessary */
    test %rdx, %rdx
    jz .load_new_stack
    mov %cr3, %rax
    cmp %rdx, %rax
    je .load_new_stack
    mov %rdx, %cr3

.load_new_stack:
    /* Load new task's RSP */
    mov %rsi, %rsp

    /* Restore new task's callee-saved registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx

    ret

.size switch_to, . - switch_to

/*
 * Initial task trampoline
 * Invoked by switch_to returning to this entry point for a freshly created task.
 * %rbx contains the target entry_func.
 */
.global task_start_trampoline
.type task_start_trampoline, @function
.extern task_exit_helper

task_start_trampoline:
    /* %r12 contains optional user argument, pass in %rdi per AMD64 ABI */
    mov %r12, %rdi
    /* Call entry function pointed to by %rbx */
    call *%rbx

    /* If entry function returns, call scheduler exit helper */
    call task_exit_helper

.halt_forever:
    cli
    hlt
    jmp .halt_forever

.size task_start_trampoline, . - task_start_trampoline

