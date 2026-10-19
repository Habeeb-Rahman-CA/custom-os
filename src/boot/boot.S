/*
 * CustomOS Bootstrap Entry Point
 * Architecture: x86-64
 */

.section .text
.global _start
.type _start, @function

_start:
    /* Ensure interrupts are disabled */
    cli

    /* Clear direction flag for standard C/C++ ABI */
    cld

    /* Establish dedicated kernel bootstrap stack */
    lea stack_top(%rip), %rsp

    /* Clear frame pointer to mark bottom of stack trace */
    xor %rbp, %rbp

    /* Call freestanding C++ kernel entry point */
    call kernel_main

    /* If kernel_main returns, halt CPU safely */
halt_loop:
    cli
    hlt
    jmp halt_loop

.size _start, . - _start

/* Kernel bootstrap stack (16 KiB, 16-byte aligned for System V ABI) */
.section .bss
.align 16
.global stack_bottom
.global stack_top

stack_bottom:
    .skip 0x4000
stack_top:

