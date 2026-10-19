/*
 * Fast System Call Entry/Exit for x86-64 (SYSCALL / SYSRET)
 */

.extern g_current_rsp0
.extern syscall_dispatch_entry

.section .bss
.align 16
g_temp_user_rsp:
    .quad 0

.section .text
.global syscall_entry
.type syscall_entry, @function

syscall_entry:
    /*
     * Interrupts are disabled upon entry via MSR_FMASK (IF masked).
     * Save user RSP to scratch variable temporarily, then switch
     * to the running task's private kernel stack.
     */
    mov %rsp, g_temp_user_rsp(%rip)
    mov g_current_rsp0(%rip), %rsp

    /*
     * Save user context directly onto the task's private kernel stack.
     * This makes system calls completely re-entrant across preemptive switches.
     */
    push g_temp_user_rsp(%rip)  /* User RSP */
    push %rcx                   /* User RIP */
    push %r11                   /* User RFLAGS */
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    /*
     * Re-marshal arguments for C calling convention:
     * System call arguments:
     *   Syscall num: %rax -> %rdi
     *   Arg 1:       %rdi -> %rsi
     *   Arg 2:       %rsi -> %rdx
     *   Arg 3:       %rdx -> %rcx
     *   Arg 4:       %r10 -> %r8
     *   Arg 5:       %r8  -> %r9
     */
    mov %r8,  %r9
    mov %r10, %r8
    mov %rdx, %rcx
    mov %rsi, %rdx
    mov %rdi, %rsi
    mov %rax, %rdi

    cld
    call syscall_dispatch_entry

    /* %rax now contains the syscall return value */

    /* Restore callee-saved registers and user context from the task's kernel stack */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    pop %r11                   /* User RFLAGS */
    pop %rcx                   /* User RIP */
    pop %rsp                   /* User RSP restored directly into %rsp */

    /* Return to Ring 3 */
    sysretq

.size syscall_entry, . - syscall_entry
