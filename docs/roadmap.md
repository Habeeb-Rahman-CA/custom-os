# CustomOS Engineering Roadmap

CustomOS is developed strictly following an incremental milestone progression. Each milestone must compile without warnings, boot reliably in QEMU, include automated tests, and provide documentation explaining underlying concepts.

---

## Milestone Execution Matrix

| Milestone | Subsystem / Focus | Status | Deliverables |
|---|---|---|---|
| **Milestone 1** | **Toolchain, Bootloader, Linker & QEMU** | **DONE** | Limine v8 boot protocol, C++20 freestanding toolchain, 16550 UART serial driver, higher-half linker script, early logging, assertions/panic diagnostics, automated QEMU smoke test. |
| **Milestone 2** | **Kernel Logging, Assertions & Diagnostics** | **DONE** | Structured severity logging (`[INFO]`, `[WARN]`, `[ERROR]`, `[PANIC]`), memory map inspection, panic handler with file/line telemetry. |
| **Milestone 3** | **CPU Exceptions & Interrupt Infrastructure** | **DONE** | 64-bit GDT, TSS with IST1, 256-entry IDT, 8259 PIC remapped to 32-47, full register frame preservation, page fault CR2 decoder, controlled `#BP` exception test. |
| **Milestone 4** | **Physical Memory Allocator (PMM)** | **DONE** | Bitmap page-frame allocator, 4 KiB frame allocation/freeing, reserved range tracking, double-free detection, frame statistics. |
| **Milestone 5** | **Virtual Memory Manager (VMM) & Kernel Heap** | **DONE** | 4-level paging manager (PML4, PDPT, PD, PT), page mapping/unmapping, isolated process address spaces, dynamic kernel heap (`kmalloc`, `kfree`, C++ `new`/`delete`). |
| **Milestone 6** | **Timer, Console & Keyboard Input** | **DONE** | 8254 PIT periodic timer at 100 Hz, PS/2 keyboard IRQ handler and circular buffer, Framebuffer graphical text console with bitmap font renderer. |
| **Milestone 7** | **Kernel Tasks & Context Switching** | **DONE** | Task Control Block (TCB), callee-saved register context switching (`switch_to`), task stack initialization, cooperative multitasking. |
| **Milestone 8** | **Preemptive Scheduling & Synchronization** | **DONE** | Round-robin scheduler, timer-driven preemption, sleep/wake states, atomic spinlocks, dual-task concurrency test (Task Alpha & Task Beta). |
| **Milestone 9** | **User Mode & System Calls** | **DONE** | Ring 0 -> Ring 3 transition via `IRETQ` / `SYSRET`, fast system call MSR configuration (`MSR_STAR`, `MSR_LSTAR`, `MSR_FMASK`, `MSR_EFER` SCE), user pointer and buffer bounds validation. |
| **Milestone 10** | **Process Model & ELF64 Loader** | **DONE** | Process address space isolation, ELF64 parser and segment loader (`PT_LOAD`), user stack allocation at `0x00007ffffff00000`, Ring 3 user process execution. |
| **Milestone 11** | **In-Memory Filesystem (RamFS) & File Descriptors** | **DONE** | VFS node abstraction, file descriptor table (`open`, `read`, `write`, `close`), directory tree (`/`, `/bin`, `/dev`, `/etc`, `/hello.txt`). |
| **Milestone 12** | **Storage & Executable Packaging** | **DONE** | Embedded executable packaging (`/bin/hello.elf` in RamFS), raw GPT disk generation via `tools/build/create_disk.sh`. |
| **Milestone 13** | **User-Space Runtime & Command-Line Shell** | **DONE** | Interactive and automated shell (`help`, `clear`, `echo`, `meminfo`, `tasks`, `uptime`, `ls`, `cat`, `pwd`, `run`, `exit`), executing user-mode programs in Ring 3. |
| **Milestone 14** | **Subsystem Verification & Diagnostics** | **DONE** | Comprehensive end-to-end automated QEMU smoke test (`tests/qemu/smoke_test.py`) validating all 14 subsystem acceptance criteria. |
| **Milestone 15** | **Advanced Expansion (Optional Roadmap)** | Future | Multiprocessor (SMP), PCI bus scanning, VirtIO-Net networking, GUI window manager. |
