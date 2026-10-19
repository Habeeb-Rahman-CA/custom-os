# Kernel Security, Isolation & Invariants

## 1. Threat Model & Philosophy

CustomOS is an educational operating system. It aims to clearly demonstrate modern systems security fundamentals:
- **Ring Separation**: Hardware-enforced execution privilege boundary (Ring 0 vs. Ring 3).
- **Address Space Isolation**: Distinct CR3 page tables for every user process.
- **Fail-Fast Defense**: Unhandled anomalies must immediately trigger a kernel panic with diagnostic telemetry rather than silently corrupting memory.

---

## 2. Implemented Hardware Defenses

### 1. Supervisor-Only Higher Half
- The higher-half kernel range (`0xffffffff80000000` and HHDM `0xffff800000000000`) has the User/Supervisor bit cleared (`U/S = 0`).
- Any attempt by user-mode code to read or write kernel virtual addresses triggers a Page Fault (`#PF`) with error code bit 2 (`U = 1`).

### 2. No-Execute Bit (NX / XD)
- Enforced on all data segments (`.rodata`, `.data`, `.bss`, kernel heap, user stack, and user heap).
- Executable permission is granted exclusively to `.text` segments.

### 3. Stack Guard & Red Zone Suppression
- Compiled with `-mno-red-zone`. The 128-byte red zone below `%rsp` is disabled to prevent hardware interrupts from overwriting transient stack frames.
- IST (Interrupt Stack Table) ensures Double Fault (`#DF`) executes on an independent stack frame.

---

## 3. Pointer & Parameter Sanitization

All system calls validate:
1. **User Buffer Ranges**: Verifies pointers do not cross into kernel address space.
2. **String Length Bounds**: Null-termination and buffer boundaries must not overflow kernel buffers.
3. **Integer Wraparound**: Memory sizes check `base + length >= base`.

---

## 4. Current Limitations & Educational Scope

- **ASLR**: Kernel and user processes are currently linked at deterministic base addresses.
- **SMEP / SMAP**: Hardware SMEP (Supervisor Mode Execution Prevention) and SMAP (Supervisor Mode Access Prevention) flags in `%cr4` will be explicitly activated in Milestone 9.
- **Capability-Based Access Control**: File permissions will implement standard UNIX `rwx` bits in Milestone 11.
