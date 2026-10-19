/*
 * Embeds user test ELF programs into kernel binary
 */

.section .rodata
.global g_hello_elf_start
.global g_hello_elf_end
.align 16

g_hello_elf_start:
    .incbin "build/hello.elf"
g_hello_elf_end:

.global g_fault_elf_start
.global g_fault_elf_end
.align 16

g_fault_elf_start:
    .incbin "build/fault.elf"
g_fault_elf_end:
