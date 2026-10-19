# Boot Process & Early Kernel Foundation

This document details the boot architecture of CustomOS, the handoff between the Limine bootloader and the kernel, the CPU state upon entry, and the higher-half memory layout.

---

## 1. Bootloader vs. Kernel: Division of Responsibility

In early PC architecture, operating systems often used custom two-stage bootloaders written in real-mode x86 assembly (512-byte MBR stage 1 loading stage 2). Modern operating systems delegate pre-boot firmware negotiation, storage parsing, and CPU mode switching to a dedicated bootloader:

| Responsibility | Bootloader (Limine) | Kernel (CustomOS) |
|---|---|---|
| Firmware Interaction | BIOS int 13h / UEFI BootServices | Direct hardware / ACPI / APIC |
| CPU Mode Switching | Real Mode (16-bit) -> Protected Mode (32-bit) -> Long Mode (64-bit) | Executes strictly in 64-bit Long Mode |
| Filesystem Parsing | Reads GPT / FAT32 partition to find kernel ELF | Implements internal VFS after boot |
| Memory Probing | Queries E820 / UEFI memory map | Manages frames via page frame allocator |
| Paging Setup | Constructs bootstrap 4-level page tables with HHDM | Replaces bootstrap tables with its own kernel & process page tables |
| Higher-Half Mapping | Maps kernel ELF at `0xffffffff80000000` | Operates in higher-half address space |

By standardizing on the **Limine Boot Protocol**, CustomOS receives the machine in a clean, predictable 64-bit state without writing thousands of lines of fragile firmware-specific assembly.

---

## 2. Kernel Memory Layout & Linker Script

CustomOS is linked as an **ELF64 Executable** mapped into the higher half of the canonical 64-bit virtual address space at `-2GiB`:

```
Virtual Address Space:
+--------------------------------------+ 0xffffffffffffffff
|               ...                    |
+--------------------------------------+ 0xffffffff80000000 (Kernel Base)
| .text    (Read + Execute)            | 4 KiB aligned
| .rodata  (Read Only)                 | 4 KiB aligned
| .data    (Read + Write)              | Includes Limine requests
| .bss     (Read + Write, Zeroed)      | Includes 16 KiB Bootstrap Stack
+--------------------------------------+
|               ...                    |
+--------------------------------------+ 0xffff800000000000 (HHDM Base)
| Higher-Half Direct Map (All Phys RAM)|
+--------------------------------------+
|               ...                    |
+--------------------------------------+ 0x00007fffffffffff (User Space Base)
| Future User Space Programs           |
+--------------------------------------+ 0x0000000000000000
```

### ELF Segment Protections (`linker.ld`)
The linker script groups code and data into distinct ELF Program Headers (`PT_LOAD`):
1. **`text` (`R E`)**: Executable code (`.text`). Write-protected in paging.
2. **`rodata` (`R`)**: Read-only constants, string literals, and virtual tables. Execution-disabled (`NX`).
3. **`data` & `.bss` (`RW`)**: Mutable globals, Limine request structures, and bootstrap stack. Execution-disabled (`NX`).

---

## 3. Entry-Point ABI & Early CPU State

When Limine jumps to `_start` in `src/boot/boot.S`, the processor is guaranteed to be in the following state:
- **Long Mode (64-bit)**: Paging enabled, 4-level paging active.
- **A20 Gate**: Enabled (address bit 20 is not masked).
- **Interrupts**: Disabled (`IF = 0` in RFLAGS).
- **Direction Flag**: Cleared (`DF = 0`).
- **Code Segment (`%cs`)**: 64-bit flat code descriptor (base 0, limit 4GB, ring 0).
- **Data Segments (`%ds`, `%es`, `%ss`)**: Flat data descriptors.
- **Paging**:
  - The kernel ELF is mapped at `0xffffffff80000000`.
  - All physical memory is mapped linearly at the HHDM base (e.g. `0xffff800000000000`).

### Bootstrap Stack Setup
Although the bootloader provides an initial stack, the kernel immediately establishes its own known, private, 16-byte aligned bootstrap stack in `src/boot/boot.S`:

```assembly
_start:
    cli
    cld
    lea stack_top(%rip), %rsp
    xor %rbp, %rbp
    call kernel_main
```

Setting `%rbp = 0` establishes the bottom of the call frame, allowing future stack trace unwinding to stop cleanly.

---

## 4. Limine Protocol v3 Handshake

Communication between Limine and CustomOS occurs via request structures placed in the `.limine_requests` section:

```cpp
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_req = { ... };

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_req = { ... };

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_req = { ... };

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;
```

When Limine loads the ELF:
1. It verifies `limine_requests_start_marker` and `limine_requests_end_marker`.
2. It inspects `limine_base_revision`. If supported, it sets `limine_base_revision[2] = 0`.
3. It allocates response structures and sets the `.response` pointer of each request.
4. If an unsupported request is encountered, its `.response` pointer remains `nullptr`.

---

## 5. Early Kernel Diagnostics & Serial Console

The earliest hardware device brought online is the **16550 UART Serial Controller** at port `0x3F8` (COM1).
- Configured to **115200 baud, 8 data bits, no parity, 1 stop bit (8N1)**.
- FIFOs enabled and queues cleared.
- All UART interrupts disabled.
- Provides immediate synchronous diagnostics that cannot be lost if graphical video output or memory paging fails.

### Kernel Initialization Stages
1. **Stage 1**: Serial console initialized; version banner printed.
2. **Stage 2**: Bootloader responses verified; memory map parsed; HHDM base recorded.
3. **Stage 3**: Critical kernel invariants asserted; CPU placed into safe halt loop awaiting Milestone 2 interrupt handlers.

---

## 6. Kernel Invariants for Milestone 1

1. **Freestanding Integrity**: No host standard library functions (`malloc`, `printf`, `exit`) or C++ runtime structures (`std::vector`, `.eh_frame`) are linked.
2. **Higher-Half Execution**: All symbols resolve in `0xffffffff80000000`..`0xffffffffffffffff`.
3. **Interrupt Safety**: Interrupts remain disabled (`cli`) until the IDT is fully populated in Milestone 3.
4. **Stack Alignment**: `%rsp` is 16-byte aligned prior to any C++ function call conforming to the System V AMD64 ABI.
