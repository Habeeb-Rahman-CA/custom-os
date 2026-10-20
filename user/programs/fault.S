/*
 * User Mode Crash/Fault Test Program (Ring 3)
 * Deliberately triggers a Page Fault (#PF) to verify user exception isolation
 */

.section .text
.global _start
_start:
    /* Write to NULL virtual address (unmapped) -> triggers #PF */
    mov $0, %rax
    movb $'X', (%rax)

    /* If it somehow continued, terminate via syscall */
    mov $1, %rax        /* SYS_EXIT */
    mov $1, %rdi        /* status = 1 */
    syscall

.spin:
    jmp .spin

