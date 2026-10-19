# CustomOS Architecture Overview

## 1. System Philosophy

CustomOS is designed as a modular, monolithic educational kernel built with modern C++20.
The kernel balances clarity, modularity, and systems correctness.
Key architectural principles:
- **Clean Subsystem Boundaries**: Architecture-specific details (`src/kernel/arch/x86_64`) are decoupled from high-level subsystems (virtual memory, task scheduler, virtual filesystem).
- **Explicit Lifetime & Ownership**: No hidden heap allocations. Resource management is visible and accountable.
- **Fail-Fast Diagnostics**: Assertions (`KASSERT`) and panic routines immediately halt the CPU with file, line, and register diagnostics rather than permitting corrupted execution.
- **Strict Ring Separation**: Privilege rings (Ring 0 Kernel, Ring 3 User) and page table permissions (Supervisor vs. User) are strictly enforced from the first user process.

---

## 2. Kernel Subsystems Breakdown

```
+-------------------------------------------------------------------+
|                        User Space Applications                    |
|                (Shell, Core Utilities, ELF Programs)              |
+-------------------------------------------------------------------+
                                  |
                                  | SYSCALL / SYSRET
                                  v
+-------------------------------------------------------------------+
|                       System Call Interface                       |
|               (write, read, open, close, exit, yield, ...)        |
+-------------------------------------------------------------------+
|                   Virtual File System (VFS)                       |
|               (Root FS, DevFS, In-Memory FS, FAT/Ext2)            |
+---------------------------------+---------------------------------+
|        Process & Scheduler      |     Virtual Memory Manager      |
|  (Task, PCB, Round-Robin Queue) | (Paging, Page Tables, Address   |
|                                 |  Spaces, Kernel Heap)           |
+---------------------------------+---------------------------------+
|                   Physical Frame Allocator (PMM)                  |
|               (Bitmap Allocator over Bootloader MemMap)           |
+-------------------------------------------------------------------+
|                     Device Driver Layer                           |
|          (UART 16550, Framebuffer, PIT/LAPIC, PS/2 Keyboard)      |
+-------------------------------------------------------------------+
|               Architecture Support (x86-64)                       |
|          (GDT, TSS, IDT, Exceptions, Hardware IRQs, Port I/O)    |
+-------------------------------------------------------------------+
```

---

## 3. Subsystem Descriptions

### 1. Architecture Layer (`arch/x86_64`)
Encapsulates all assembly instructions (`inb`, `outb`, `cli`, `sti`, `hlt`, `invlpg`, `wrmsr`, `rdmsr`), CPU descriptor tables (GDT, IDT), TSS for privilege level stacks, and interrupt handler stubs.

### 2. Physical Memory Manager (`memory/pmm`)
Tracks physical RAM page frames using a bitmap or buddy allocator. Parses the bootloader memory map to identify usable RAM and avoid reserved hardware/firmware zones.

### 3. Virtual Memory Manager (`memory/vmm`)
Controls 4-level x86-64 page tables (PML4 -> PDPT -> PD -> PT). Maps and unmaps 4 KiB / 2 MiB pages, manages higher-half kernel mappings, and constructs isolated lower-half address spaces for user processes.

### 4. Kernel Heap Allocator (`memory/heap`)
Provides dynamic memory allocation (`kmalloc`, `kfree`) on top of virtual pages using a slab or free-list allocator.

### 5. Interrupt & Timer Infrastructure (`interrupts`, `drivers/timer`)
Handles CPU faults (0-31), remaps the 8259 PIC or configures the Local APIC, and configures the timer to drive preemptive scheduling.

### 6. Process & Scheduler (`scheduler`, `process`)
Manages execution contexts, registers, kernel stacks, task states (Ready, Running, Blocked, Terminated), and round-robin preemptive task switching.

### 7. Virtual File System (`filesystem`)
Abstract file node hierarchy providing standard VFS operations (`open`, `read`, `write`, `close`, `readdir`, `stat`).

### 8. User Space & System Calls (`syscall`, `executable`, `user`)
Configures Model-Specific Registers (MSRs) for fast `SYSCALL`/`SYSRET` transitions, validates user pointers, and executes ELF64 user programs in Ring 3.
