#include <kernel/syscall/syscall.h>
#include <kernel/arch/x86_64/msr.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/memory/vmm.h>
#include <kernel/drivers/serial.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/console/framebuffer_console.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/core/logger.h>

extern "C" void syscall_entry();

extern "C" kernel::u64 syscall_dispatch_entry(kernel::u64 num, kernel::u64 a1, kernel::u64 a2, kernel::u64 a3, kernel::u64 a4, kernel::u64 a5) {
    return kernel::syscall::dispatch(num, a1, a2, a3, a4, a5);
}

namespace kernel::syscall {

namespace {

constexpr u64 USER_SPACE_LIMIT = 0x0000800000000000ULL;

} // namespace

bool validate_user_buffer(const void* ptr, usize size) {
    if (!ptr || size == 0) return false;
    u64 addr = reinterpret_cast<u64>(ptr);
    if (addr >= USER_SPACE_LIMIT) return false;
    if (addr + size > USER_SPACE_LIMIT) return false;
    if (addr + size < addr) return false; // Overflow check

    // Validate that every 4 KiB page spanned by the buffer is mapped in VMM
    u64 end = addr + size;
    u64 page = addr & ~(memory::PAGE_SIZE - 1);
    while (page < end) {
        auto phys = memory::Vmm::translate(VirtAddr(page));
        if (phys.as_u64() == 0) {
            return false;
        }
        page += memory::PAGE_SIZE;
    }
    return true;
}

bool validate_user_string(const char* str) {
    if (!str) return false;
    u64 addr = reinterpret_cast<u64>(str);
    if (addr >= USER_SPACE_LIMIT) return false;

    // Scan until null terminator or limit, checking each page boundary
    u64 last_checked_page = 0;
    while (addr < USER_SPACE_LIMIT) {
        u64 current_page = addr & ~(memory::PAGE_SIZE - 1);
        if (current_page != last_checked_page) {
            auto phys = memory::Vmm::translate(VirtAddr(current_page));
            if (phys.as_u64() == 0) {
                return false;
            }
            last_checked_page = current_page;
        }

        if (*reinterpret_cast<const char*>(addr) == '\0') {
            return true;
        }
        addr++;
    }
    return false;
}

void init() {
    // 1. Enable System Call Extensions in EFER
    u64 efer = arch::x86_64::rdmsr(arch::x86_64::MSR_EFER);
    arch::x86_64::wrmsr(arch::x86_64::MSR_EFER, efer | 1); // SCE = 1

    // 2. Configure Segment Selectors in STAR:
    // Bits 47:32 = Kernel CS (0x08), Kernel SS (0x10)
    // Bits 63:48 = User CS/SS base (0x10) -> sysret loads SS = (0x10 + 8) | 3 = 0x18 | 3, CS = (0x10 + 16) | 3 = 0x20 | 3
    u64 star = (0x0010ULL << 48) | (0x0008ULL << 32);
    arch::x86_64::wrmsr(arch::x86_64::MSR_STAR, star);

    // 3. Register Syscall Handler Target in LSTAR
    arch::x86_64::wrmsr(arch::x86_64::MSR_LSTAR, reinterpret_cast<u64>(syscall_entry));

    // 4. Set Flag Mask in FMASK (mask IF bit 9 to disable interrupts on entry)
    arch::x86_64::wrmsr(arch::x86_64::MSR_FMASK, 0x200);

    log::info("[SYSCALL] Fast System Call (SYSCALL/SYSRET) infrastructure configured.");
}

u64 dispatch(u64 num, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    (void)arg4; (void)arg5;

    switch (num) {
        case SYS_EXIT:
            scheduler::Scheduler::exit();
            return 0;

        case SYS_WRITE: {
            u64 fd = arg1;
            const char* buf = reinterpret_cast<const char*>(arg2);
            usize count = static_cast<usize>(arg3);

            if (!validate_user_buffer(buf, count)) {
                return static_cast<u64>(-14); // -EFAULT
            }

            if (fd == 1 || fd == 2) { // stdout or stderr
                for (usize i = 0; i < count; ++i) {
                    drivers::serial::write_char(buf[i]);
                    console::FbConsole::put_char(buf[i]);
                }
                return count;
            }
            return static_cast<u64>(-9); // -EBADF
        }

        case SYS_READ: {
            u64 fd = arg1;
            char* buf = reinterpret_cast<char*>(arg2);
            usize count = static_cast<usize>(arg3);

            if (!validate_user_buffer(buf, count)) {
                return static_cast<u64>(-14); // -EFAULT
            }

            if (fd == 0) { // stdin
                for (usize i = 0; i < count; ++i) {
                    buf[i] = drivers::keyboard::get_char();
                    drivers::serial::write_char(buf[i]); // Echo
                    console::FbConsole::put_char(buf[i]);
                    if (buf[i] == '\n') return i + 1;
                }
                return count;
            }
            return static_cast<u64>(-9); // -EBADF
        }

        case SYS_YIELD:
            scheduler::Scheduler::yield();
            return 0;

        case SYS_SLEEP:
            scheduler::Scheduler::sleep(arg1);
            return 0;

        case SYS_GETPID: {
            auto* task = scheduler::Scheduler::get_current_task();
            return task ? task->pid : 0;
        }

        default:
            return static_cast<u64>(-38); // -ENOSYS
    }
}

} // namespace kernel::syscall

