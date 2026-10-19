/*
 * GDT and TSS flush helper for x86-64
 * Arguments (System V AMD64 ABI):
 *   %rdi: Address of GdtPointer
 *   %rsi: Kernel Code Selector (0x08)
 *   %rdx: Kernel Data Selector (0x10)
 *   %rcx: TSS Selector (0x28)
 */

.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    /* Load the GDT */
    lgdt (%rdi)

    /* Reload data segment registers */
    mov %dx, %ds
    mov %dx, %es
    mov %dx, %ss
    xor %eax, %eax
    mov %ax, %fs
    mov %ax, %gs

    /* Far return to reload %cs with new 64-bit code selector */
    push %rsi                   /* New CS */
    lea .reload_cs(%rip), %rax
    push %rax                   /* Return RIP */
    lretq

.reload_cs:
    /* Load Task Register */
    ltr %cx
    ret

.size gdt_flush, . - gdt_flush

