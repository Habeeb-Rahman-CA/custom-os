# CustomOS Engineering Roadmap

CustomOS is developed strictly following an incremental milestone progression. Each milestone must compile without warnings, boot reliably in QEMU, include automated tests, and provide documentation explaining underlying concepts.

---

## Milestone Execution Matrix

| Milestone | Subsystem / Focus | Status | Deliverables |
|---|---|---|---|
| **Milestone 1** | **Toolchain, Bootloader, Linker & QEMU** | **DONE** | Limine v8 boot protocol, C++20 freestanding toolchain, 16550 UART serial driver, higher-half linker script, early logging, assertions/panic diagnostics, automated QEMU smoke test. |
| **Milestone 2** | **Kernel Logging, Assertions & Diagnostics** | In Progress | Serial printf-style formatting, memory map inspection, colored severity logs, panic stack frame trace. |
| **Milestone 3** | **CPU Exceptions & Interrupt Infrastructure** | Planned | GDT, TSS, IDT setup, 32 CPU exception stubs, registers preservation, page fault error code decoder, interrupt testing harness. |
| **Milestone 4** | **Physical Memory Allocator (PMM)** | Planned | Bitmap page-frame allocator, 4 KiB frame allocation/freeing, reserved range tracking, frame usage statistics. |
| **Milestone 5** | **Virtual Memory Manager (VMM) & Kernel Heap** | Planned | 4-level paging manager, map/unmap page tables, permission enforcement, slab/linked-list kernel heap allocator (`kmalloc`, `kfree`). |
| **Milestone 6** | **Timer, Console & Keyboard Input** | Planned | PIT/APIC periodic timer interrupt, framebuffer text console renderer with PSF font, PS/2 keyboard IRQ handler and input buffer. |
| **Milestone 7** | **Kernel Tasks & Context Switching** | Planned | Task Control Block (TCB), cooperative context switching (`switch_to`), kernel thread creation, dual-task concurrency test. |
| **Milestone 8** | **Preemptive Scheduling & Synchronization** | Planned | Timer-driven preemption, round-robin run queue, sleep/wake states, spinlocks, mutexes. |
| **Milestone 9** | **User Mode & System Calls** | Planned | Ring 0 -> Ring 3 transition via `IRETQ` / `SYSRET`, `MSR_LSTAR` syscall entry point, pointer validation, basic syscalls (`write`, `yield`, `exit`). |
| **Milestone 10** | **Process Model & ELF64 Loader** | Planned | Process address space isolation, ELF64 parser and segment loader, user stack allocation, user process execution. |
| **Milestone 11** | **In-Memory Filesystem (RamFS) & File Descriptors** | Planned | VFS node abstraction, file descriptor table, standard operations (`open`, `read`, `write`, `close`). |
| **Milestone 12** | **Persistent Storage & Disk Driver** | Planned | VirtIO-Block or ATA PIO disk driver, partition parsing, FAT32/ext2 filesystem integration. |
| **Milestone 13** | **User-Space Runtime & Command-Line Shell** | Planned | User-space CRT startup (`crt0.o`), user library (`libc`), interactive shell (`help`, `ls`, `cat`, `meminfo`, `run`). |
| **Milestone 14** | **Inter-Process Communication & Diagnostics** | Planned | Pipes, signals, scheduler metrics, uptime, context-switch counters. |
| **Milestone 15** | **Advanced Expansion (Optional)** | Planned | Multiprocessor (SMP), PCI bus scanning, VirtIO-Net networking, GUI window manager. |
