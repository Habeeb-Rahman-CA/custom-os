/*
 * IDT Assembly Entry Stubs for x86-64
 */

.section .text
.extern exception_dispatch

.macro ISR_NOERR index
.global isr_stub_\index
isr_stub_\index:
    push $0                 /* Dummy error code */
    push $\index            /* Interrupt vector */
    jmp common_isr_stub
.endm

.macro ISR_ERR index
.global isr_stub_\index
isr_stub_\index:
    /* Error code pushed by hardware */
    push $\index            /* Interrupt vector */
    jmp common_isr_stub
.endm

/* CPU Exceptions (0-31) */
ISR_NOERR 0   /* #DE Divide Error */
ISR_NOERR 1   /* #DB Debug */
ISR_NOERR 2   /* #NMI Non-Maskable Interrupt */
ISR_NOERR 3   /* #BP Breakpoint */
ISR_NOERR 4   /* #OF Overflow */
ISR_NOERR 5   /* #BR Bound Range Exceeded */
ISR_NOERR 6   /* #UD Invalid Opcode */
ISR_NOERR 7   /* #NM Device Not Available */
ISR_ERR   8   /* #DF Double Fault (Error Code) */
ISR_NOERR 9   /* Coprocessor Segment Overrun */
ISR_ERR   10  /* #TS Invalid TSS (Error Code) */
ISR_ERR   11  /* #NP Segment Not Present (Error Code) */
ISR_ERR   12  /* #SS Stack Fault (Error Code) */
ISR_ERR   13  /* #GP General Protection (Error Code) */
ISR_ERR   14  /* #PF Page Fault (Error Code) */
ISR_NOERR 15  /* Reserved */
ISR_NOERR 16  /* #MF x87 FPU Floating-Point Error */
ISR_ERR   17  /* #AC Alignment Check (Error Code) */
ISR_NOERR 18  /* #MC Machine Check */
ISR_NOERR 19  /* #XM SIMD Floating-Point Exception */
ISR_NOERR 20  /* #VE Virtualization Exception */
ISR_ERR   21  /* #CP Control Protection Exception */
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29  /* #HV Hypervisor Injection */
ISR_ERR   30  /* #VC VMM Communication */
ISR_ERR   31  /* #SX Security Exception */

/* Hardware & Software IRQs (32-255) */
.altmacro
.macro GEN_IRQS start, end
    .set i, \start
    .rept \end - \start + 1
        ISR_NOERR %i
        .set i, i + 1
    .endr
.endm

GEN_IRQS 32, 255

common_isr_stub:
    /* Save general purpose registers */
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    /* Pass pointer to InterruptFrame in %rdi */
    mov %rsp, %rdi
    cld
    call exception_dispatch

    /* Restore general purpose registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    /* Remove vector and error code */
    add $16, %rsp
    iretq

/* Pointer Table */
.section .rodata
.global isr_stub_table
.align 8
isr_stub_table:
.macro DEF_TABLE_ENTRY index
    .quad isr_stub_\index
.endm

.set j, 0
.rept 256
    DEF_TABLE_ENTRY %j
    .set j, j + 1
.endr

