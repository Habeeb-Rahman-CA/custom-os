# Interrupts and CPU Exception Architecture

This document describes the x86-64 interrupt handling mechanism, descriptor tables, and exception dispatching in CustomOS.

---

## 1. Global Descriptor Table (GDT) & Task State Segment (TSS)

In 64-bit mode, segmentation is largely disabled (base address forced to 0), but the GDT remains essential for:
1. Setting the **Code Segment Descriptor** privilege levels (Ring 0 vs. Ring 3).
2. Loading the **Task State Segment (TSS)**.

### TSS & IST (Interrupt Stack Table)
The TSS holds:
- **`RSP0`**: The kernel stack pointer loaded automatically by CPU hardware when an interrupt or exception triggers while executing in user mode (Ring 3).
- **`IST1` - `IST7`**: Dedicated stack pointers used for critical non-maskable exceptions (such as Double Fault `#DF` or Machine Check `#MC`), ensuring stack overflow faults can still be diagnosed without a triple fault.

---

## 2. Interrupt Descriptor Table (IDT)

The IDT contains 256 entries (16 bytes each on x86-64):
- **Vectors 0 - 31**: Dedicated CPU Architecture Exceptions.
- **Vectors 32 - 47**: Remapped Legacy Hardware IRQs (Timer, Keyboard, COM ports).
- **Vectors 48 - 255**: Available for Software Interrupts / System Calls / Spurious IRQs.

### CPU Exception Vectors
| Vector | Mnemonic | Name | Error Code? |
|---|---|---|---|
| 0 | `#DE` | Divide Error | No |
| 1 | `#DB` | Debug Exception | No |
| 2 | `#NMI` | Non-Maskable Interrupt | No |
| 3 | `#BP` | Breakpoint | No |
| 4 | `#OF` | Overflow | No |
| 6 | `#UD` | Invalid Opcode | No |
| 8 | `#DF` | Double Fault | Yes (0) |
| 13 | `#GP` | General Protection Fault | Yes |
| 14 | `#PF` | Page Fault | Yes |

---

## 3. Interrupt Handler Calling Convention & Register Preservation

When an interrupt fires, the CPU hardware pushes:
- `SS`, `RSP`, `RFLAGS`, `CS`, `RIP`
- An Error Code (for exceptions that generate one; for others, the assembly stub pushes dummy `0`).

The common assembly interrupt stub must:
1. Save all general-purpose registers (`RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`, `RBP`, `R8` - `R15`) onto the stack.
2. Pass a pointer to the saved register frame (`InterruptFrame*`) to the C++ interrupt dispatcher.
3. Upon return, restore all general-purpose registers and execute `IRETQ`.

---

## 4. Hardware IRQs & Interrupt Controllers

- **Milestone 3**: Configure 8259 PIC (remap IRQ 0-7 to vectors 32-39, IRQ 8-15 to vectors 40-47).
- **Milestone 6+**: APIC / IO-APIC migration for reliable multi-core interrupt routing.
- **Acknowledgment (EOI)**: Handlers for hardware IRQs must write End-Of-Interrupt (`0x20` to master/slave PIC) before returning.

