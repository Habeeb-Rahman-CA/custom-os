# CustomOS: Educational x86-64 Operating System

CustomOS is an educational, freestanding, 64-bit operating system written from scratch in modern C++20 and x86-64 assembly, targeting the Limine bootloader protocol and QEMU virtual machine environment.

The design goal of CustomOS is to explore operating system fundamentals from the bare hardware up:
- Bootloader-to-kernel handoff and CPU state initialization.
- CPU descriptor tables (GDT, IDT) and interrupt dispatching.
- Multi-tier memory architecture (Physical Frame Allocator, Virtual Paging Manager, Kernel Heap).
- Device drivers and non-blocking I/O (UART 16550 Serial, Framebuffer, PIT/APIC timers, PS/2 keyboard).
- Multitasking, preemptive scheduling, and privilege ring separation (Ring 0 Kernel vs. Ring 3 User).
- System-call dispatching (`SYSCALL`/`SYSRET`) and POSIX-compatible abstractions.
- Virtual File System (VFS) and ELF64 binary loading.

---

## Current Status: Milestone 1 Complete

- **Milestone 1**: Toolchain, bootloader protocol (Limine v8), higher-half linker script, freestanding C++20 bootstrap, 16550 UART serial driver, early logging, assertions/panic diagnostics, automated QEMU smoke test.
- **Next Milestone**: Milestone 2 & 3 (CPU Exceptions, Global Descriptor Table, Interrupt Descriptor Table, and Interrupt Infrastructure).

---

## Directory Structure

```
.
├── CMakeLists.txt              # CMake build definitions
├── Makefile                    # Make build targets
├── README.md                   # Project overview & quickstart
├── limine.conf                 # Limine bootloader configuration
├── linker.ld                   # x86-64 higher-half linker script
├── docs/                       # Architecture & subsystem documentation
│   ├── architecture.md         # System design & privilege boundaries
│   ├── boot-process.md         # Bootloader handoff, ABI, and memory layout
│   ├── memory-management.md    # Physical, virtual, and heap memory design
│   ├── interrupts.md           # IDT, exceptions, and IRQ architecture
│   ├── scheduling.md           # Tasks, contexts, and scheduling policies
│   ├── system-calls.md         # Ring 3 transition and SYSCALL/SYSRET ABI
│   ├── filesystem.md           # VFS abstractions and disk formats
│   ├── security.md             # Invariants, privilege boundaries & mitigations
│   └── roadmap.md              # Long-term milestone execution roadmap
├── include/
│   └── kernel/
│       ├── arch/x86_64/        # Architecture port I/O and CPU state
│       ├── core/               # Limine protocol, boot info, logger, panic
│       ├── drivers/            # UART serial, console, timer
│       └── ...
├── src/
│   ├── boot/boot.S             # Assembly bootstrap & stack allocation
│   └── kernel/                 # Subsystem implementations
├── tests/
│   ├── qemu/smoke_test.py      # Automated QEMU boot validation test
│   └── unit/test_types.cpp     # Host-side C++ unit tests
└── tools/
    └── build/create_disk.sh    # GPT / FAT32 disk image generator
```

---

## Prerequisites & Development Environment

The project requires standard tools available on Linux (tested on Fedora 43 / Ubuntu 24.04):
- **C++ Compiler**: `g++` (GCC >= 13 with C++20 support)
- **Assembler & Linker**: GNU `as`, GNU `ld` (binutils)
- **Emulator**: `qemu-system-x86_64` (>= 8.0)
- **Disk Utilities**: `parted`, `mtools` (`mformat`, `mcopy`, `mmd`), `dd`
- **Testing**: `python3` (>= 3.8)

---

## Build, Run & Test Commands

### 1. Build the Bootable Disk Image
```bash
make
```
This compiles `build/kernel.elf` using `-mcmodel=kernel -ffreestanding -mno-red-zone` and packages it into a bootable GPT disk image `build/os.hdd`.

### 2. Run in QEMU (Serial Console)
```bash
make run
```
Launches QEMU in headless mode (`-display none`), directing serial port COM1 to terminal `stdio`. Press `Ctrl+A` then `X` to exit QEMU.

### 3. Run Automated Tests
```bash
make test
```
Executes:
1. Host unit tests (`tests/unit/test_types.cpp`)
2. Headless QEMU integration smoke test (`tests/qemu/smoke_test.py`)

### 4. Debug with GDB
In terminal 1:
```bash
make gdb
```
In terminal 2:
```bash
make debug
```

---

## Compiler & Linker Flags

| Flag | Purpose |
|---|---|
| `-std=c++20` | Use modern C++20 features (concepts, constexpr, designated initializers) |
| `-ffreestanding` | Freestanding environment without hosted standard C/C++ runtime |
| `-fno-builtin` | Prevent compiler from substituting custom functions with glibc built-ins |
| `-fno-exceptions` | Disable C++ exception handling tables (`.eh_frame`) |
| `-fno-rtti` | Disable Run-Time Type Information (`typeid`, `dynamic_cast`) |
| `-fno-pie -fno-pic` | Build position-dependent kernel code for direct higher-half linking |
| `-mno-red-zone` | Disable 128-byte x86-64 red zone so interrupt handlers cannot corrupt stack |
| `-mcmodel=kernel` | Generate 32-bit sign-extended code addresses for the `-2GiB` top virtual range |
| `-mgeneral-regs-only` | Prevent auto-vectorization using SSE/AVX before FPU/SIMD state is initialized |
| `-nostdlib -static` | Link without standard C runtime startup files or libraries |
| `-z max-page-size=0x1000` | Align ELF segments to standard 4096-byte page boundaries |
| `-T linker.ld` | Enforce explicit virtual layout starting at `0xffffffff80000000` |
