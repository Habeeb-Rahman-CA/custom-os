/*
 * Fast System Call Entry/Exit for x86-64 (SYSCALL / SYSRET)
 */

.section .bss
.align 16
.global syscall_kernel_stack_top
syscall_kernel_stack_bottom:
    .skip 0x4000
syscall_kernel_stack_top:

.section .data
.global g_saved_user_rsp
g_saved_user_rsp:
    .quad 0

.section .text
.global syscall_entry
.type syscall_entry, @function
.extern syscall_dispatch_entry

syscall_entry:
    /* Save user RSP and switch to kernel syscall stack */
    mov %rsp, g_saved_user_rsp(%rip)
    lea syscall_kernel_stack_top(%rip), %rsp

    /* Save user return context (RCX = RIP, R11 = RFLAGS) and callee-saved registers */
    push %rcx
    push %r11
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

    /* Restore callee-saved registers and user context */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    pop %r11
    pop %rcx

    /* Restore user stack pointer */
    mov g_saved_user_rsp(%rip), %rsp

    /* Return to Ring 3 */
    sysretq

.size syscall_entry, . - syscall_entry
