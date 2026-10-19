#pragma once

#include <kernel/core/types.h>
#include <kernel/arch/x86_64/io.h>

namespace kernel::scheduler {

class Spinlock {
public:
    constexpr Spinlock() = default;

    void acquire() {
        while (__atomic_test_and_set(&m_locked, __ATOMIC_ACQUIRE)) {
            asm volatile("pause");
        }
    }

    void release() {
        __atomic_clear(&m_locked, __ATOMIC_RELEASE);
    }

private:
    volatile bool m_locked{false};
};

// Interrupt-safe spinlock that disables CPU interrupts during critical sections
class IrqSpinlock {
public:
    constexpr IrqSpinlock() = default;

    u64 acquire() {
        u64 rflags;
        asm volatile("pushfq; pop %0; cli" : "=r"(rflags) : : "memory");
        while (__atomic_test_and_set(&m_locked, __ATOMIC_ACQUIRE)) {
            asm volatile("pause");
        }
        return rflags;
    }

    void release(u64 rflags) {
        __atomic_clear(&m_locked, __ATOMIC_RELEASE);
        if (rflags & 0x200) {
            asm volatile("sti" : : : "memory");
        }
    }

private:
    volatile bool m_locked{false};
};

class IrqLockGuard {
public:
    explicit IrqLockGuard(IrqSpinlock& lock) : m_lock(lock), m_rflags(m_lock.acquire()) {}

    ~IrqLockGuard() {
        m_lock.release(m_rflags);
    }

    IrqLockGuard(const IrqLockGuard&) = delete;
    IrqLockGuard& operator=(const IrqLockGuard&) = delete;

private:
    IrqSpinlock& m_lock;
    u64 m_rflags;
};

class LockGuard {
public:
    explicit LockGuard(Spinlock& lock) : m_lock(lock) {
        m_lock.acquire();
    }

    ~LockGuard() {
        m_lock.release();
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    Spinlock& m_lock;
};

} // namespace kernel::scheduler
