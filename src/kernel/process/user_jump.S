/*
 * Ring 0 -> Ring 3 transition helper via IRETQ
 * C signature: extern "C" [[noreturn]] void enter_user_mode(u64 rip, u64 rsp, u64 cr3);
 * Arguments:
 *   %rdi: User RIP
 *   %rsi: User RSP
 *   %rdx: Process CR3
 */

.section .text
.global enter_user_mode
.type enter_user_mode, @function

enter_user_mode:
    /* Switch to target process page directory */
    test %rdx, %rdx
    jz .skip_cr3
    mov %rdx, %cr3
.skip_cr3:

    /*
     * Construct hardware interrupt return frame on stack:
     *   SS:     User Data Segment (0x18 | 3 = 0x1B)
     *   RSP:    User Stack Pointer (%rsi)
     *   RFLAGS: Interrupts Enabled (0x202)
     *   CS:     User Code Segment (0x20 | 3 = 0x23)
     *   RIP:    User Instruction Pointer (%rdi)
     */
    push $0x1B
    push %rsi
    push $0x202
    push $0x23
    push %rdi

    /* Clear segment registers for user space */
    xor %eax, %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    /* Transition into Ring 3 */
    iretq

.size enter_user_mode, . - enter_user_mode
