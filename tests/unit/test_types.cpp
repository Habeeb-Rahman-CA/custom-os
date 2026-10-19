#include <kernel/core/types.h>
#include <cassert>
#include <iostream>

using namespace kernel;

void test_phys_addr() {
    PhysAddr p1(0x1000);
    assert(p1.as_u64() == 0x1000);
    assert(p1.is_aligned(0x1000));
    assert(p1.is_aligned(0x200));
    assert(!p1.is_aligned(0x2000));

    PhysAddr p2(0x1005);
    assert(!p2.is_aligned(0x1000));
    assert(!p2.is_aligned(2));
}

void test_virt_addr() {
    VirtAddr v1(0xffffffff80000000ULL);
    assert(v1.as_u64() == 0xffffffff80000000ULL);
    assert(v1.is_aligned(0x1000));
    assert(v1.is_aligned(0x200000)); // 2MiB aligned

    VirtAddr v2(0xffffffff80000004ULL);
    assert(!v2.is_aligned(0x1000));
    assert(v2.is_aligned(4));
}

int main() {
    std::cout << "[RUNNING] Host unit tests for kernel types...\n";
    test_phys_addr();
    test_virt_addr();
    std::cout << "[PASSED] All host unit tests passed!\n";
    return 0;
}

