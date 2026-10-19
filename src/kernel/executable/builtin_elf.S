/*
 * Embeds user test ELF program into kernel binary
 */

.section .rodata
.global g_hello_elf_start
.global g_hello_elf_end
.align 16

g_hello_elf_start:
    .incbin "build/hello.elf"
g_hello_elf_end:
