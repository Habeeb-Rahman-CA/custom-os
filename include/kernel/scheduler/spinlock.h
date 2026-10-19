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

class LockGuard {
public:
    explicit LockGuard(Spinlock& lock) : m_lock(lock) {
        m_lock.acquire();
    }

    ~LockGuard() {
        m_lock.release();
    }

private:
    Spinlock& m_lock;
};

} // namespace kernel::scheduler
