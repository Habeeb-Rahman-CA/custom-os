/*
 * User Mode Hello World Program (Ring 3)
 * Demonstrates SYSCALL ABI
 */

.section .data
msg:
    .ascii "[USER] >>> Hello from User Mode (Ring 3) via SYSCALL! <<<\n"
.set len, . - msg

.section .text
.global _start
_start:
    /* sys_write(fd=1, buf=msg, count=len) */
    mov $3, %rax        /* SYS_WRITE */
    mov $1, %rdi        /* stdout */
    lea msg(%rip), %rsi
    mov $len, %rdx
    syscall

    /* sys_exit(0) */
    mov $1, %rax        /* SYS_EXIT */
    xor %rdi, %rdi      /* status = 0 */
    syscall

.spin:
    jmp .spin
