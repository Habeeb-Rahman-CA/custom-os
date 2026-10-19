# System Call Interface Architecture

## 1. Privilege Boundaries & Transition

x86-64 provides high-performance hardware system-call instructions:
- **`SYSCALL`**: Invoked by Ring 3 user code to jump to Ring 0.
- **`SYSRET`**: Invoked by Ring 0 kernel to return back to Ring 3.

### Model-Specific Registers (MSRs)
The kernel configures three MSRs during initialization:
1. **`IA32_EFER` (0xC0000080)**: Bit 0 (SCE - System Call Extension) enabled.
2. **`IA32_STAR` (0xC0000081)**: Upper 32 bits configure the CS/SS segment selectors for Ring 0 and Ring 3.
3. **`IA32_LSTAR` (0xC0000082)**: Long System Target Address Register; holds the virtual address of the kernel syscall assembly entry point.
4. **`IA32_FMASK` (0xC0000084)**: Flag Mask Register; masks off CPU flags (such as `IF` to disable interrupts on entry).

---

## 2. Calling Convention (x86-64 ABI)

CustomOS follows standard 64-bit Unix calling conventions for system calls:
- **Syscall Number**: `%rax`
- **Arguments**: `%rdi` (arg 1), `%rsi` (arg 2), `%rdx` (arg 3), `%r10` (arg 4), `%r8` (arg 5), `%r9` (arg 6)
  *(Note: `%r10` replaces `%rcx` because the CPU `SYSCALL` instruction clobbers `%rcx` with the return `RIP`).*
- **Return Value**: `%rax` (negative values represent error codes, e.g. `-EINVAL`).

---

## 3. Core System Calls

| Syscall | Number | Signature | Description |
|---|---|---|---|
| `sys_exit` | 1 | `void exit(int status)` | Terminate current process |
| `sys_read` | 2 | `ssize_t read(int fd, void* buf, size_t count)` | Read from file descriptor |
| `sys_write` | 3 | `ssize_t write(int fd, const void* buf, size_t count)` | Write to file descriptor |
| `sys_open` | 4 | `int open(const char* path, int flags)` | Open file and allocate FD |
| `sys_close`| 5 | `int close(int fd)` | Close file descriptor |
| `sys_yield`| 6 | `void yield()` | Yield remainder of CPU quantum |
| `sys_sleep`| 7 | `void sleep(uint64_t ms)` | Sleep for given duration |
| `sys_getpid`| 8| `pid_t getpid()` | Retrieve current process ID |

---

## 4. User Pointer Validation & Security

The kernel must never blindly dereference a pointer passed from user space:
1. **Canonical Address Check**: Pointer must not point into the higher-half kernel range (`>= 0x0000800000000000`).
2. **Mapped Page Check**: The virtual page containing the buffer must exist and have User (`U/S = 1`) permissions in the active page directory.
3. **Buffer Bounds**: `[buf, buf + count)` must be contiguous within user space without integer wrap-around.
