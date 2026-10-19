# Kernel Security, Isolation & Invariants

## 1. Threat Model & Philosophy

CustomOS implements modern x86-64 systems security and fault-isolation fundamentals:
- **Ring Separation**: Hardware-enforced execution privilege boundary (Ring 0 Kernel vs. Ring 3 User Mode).
- **Process Fault Isolation**: Unhandled exceptions in user mode terminate only the faulting process without crashing or panicking the kernel.
- **Address Space Isolation**: Distinct PML4 CR3 page tables for every user process.
- **Fail-Fast Kernel Diagnostics**: Kernel-level anomalies trigger a structured panic with full register and memory dumps.

---

## 2. Implemented Hardware Defenses

### 1. Supervisor-Only Higher Half
- The higher-half kernel range (`0xffffffff80000000` and HHDM `0xffff800000000000`) has the User/Supervisor bit cleared (`U/S = 0`).
- Any attempt by user-mode code to read or write kernel virtual addresses triggers a Page Fault (`#PF`) with protection violation error flags.

### 2. User-Mode Exception Fault Isolation
- When a CPU exception (`#PF`, `#GP`, `#UD`, `#DE`) occurs, the interrupt dispatcher inspects the Code Segment selector (`frame->cs & 3 == 3`).
- If the fault originated in Ring 3, the kernel logs the faulting PID, process name, exception type, RIP, error code, and faulting virtual address (`CR2`).
- The faulting user task is safely terminated via `Scheduler::exit()`, and the parent shell or kernel scheduler seamlessly continues running. The kernel never panics on user faults.

### 3. Re-entrant Per-Task Kernel Stacks
- Every task maintains its own private 16 KiB kernel stack.
- On each task switch, `TSS.rsp0` and `g_current_rsp0` are updated to point to the newly scheduled task's stack top.
- During fast system calls (`SYSCALL`), user RSP is pushed onto the task's private kernel stack, making syscalls completely re-entrant across preemptive switches.

### 4. No-Execute Bit (NX / XD)
- Enforced on all data segments (`.rodata`, `.data`, `.bss`, kernel heap, user stack, and user heap).
- Executable permission is granted exclusively to `.text` segments.

### 5. Stack Guard & Red Zone Suppression
- Compiled with `-mno-red-zone`. The 128-byte red zone below `%rsp` is disabled to prevent hardware interrupts from overwriting transient stack frames.
- Dedicated Interrupt Stack Table (IST1) ensures Double Fault (`#DF`) executes on an independent stack.

---

## 3. Pointer & Parameter Sanitization

All system calls implement strict input validation:
1. **User Space Boundary Check**: Buffers must reside entirely below `0x0000800000000000`.
2. **Page-by-Page Mapping Verification**: Every 4 KiB page spanned by a user buffer is validated via `Vmm::translate()`. If any page is unmapped or invalid, the syscall immediately returns `-EFAULT` (`-14`).
3. **Integer Overflow Check**: Memory lengths verify `base + size >= base`.
4. **Bounded String Traversal**: User string reads are page-checked and bounded to prevent out-of-bounds kernel memory leaks.

---

## 4. Memory Allocator Hardening

1. **Double-Free Detection**: Physical page frames and kernel heap blocks detect duplicate frees and corrupt header magic numbers.
2. **Heap Memory Poisoning**: Freed heap blocks are scrubbed with pattern `0xDD` to immediately expose and prevent use-after-free bugs.
3. **Interrupt-Safe Spinlocks**: `IrqSpinlock` guards the PMM and Kernel Heap against concurrent access and timer interrupt re-entrancy.
