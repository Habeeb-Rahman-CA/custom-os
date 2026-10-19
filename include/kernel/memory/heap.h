#pragma once

#include <kernel/core/types.h>

namespace kernel::memory {

void heap_init();

void* kmalloc(usize size);
void  kfree(void* ptr);
void* krealloc(void* ptr, usize new_size);

usize get_heap_allocated_bytes();
usize get_heap_total_bytes();

} // namespace kernel::memory

// Global placement and standard new/delete for freestanding C++20
void* operator new(size_t size);
void* operator new[](size_t size);
void  operator delete(void* p) noexcept;
void  operator delete[](void* p) noexcept;
void  operator delete(void* p, size_t) noexcept;
void  operator delete[](void* p, size_t) noexcept;

inline void* operator new(size_t, void* p) noexcept { return p; }
inline void* operator new[](size_t, void* p) noexcept { return p; }
