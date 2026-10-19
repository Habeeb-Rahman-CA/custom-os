# Memory Management Architecture

CustomOS structures memory management into three decoupled layers:
1. **Physical Memory Manager (PMM)**: Tracks raw physical 4 KiB RAM frames.
2. **Virtual Memory Manager (VMM)**: Manages 4-level x86-64 page tables and virtual address mappings.
3. **Kernel Heap Allocator**: Allocates and frees arbitrary-sized dynamic memory blocks.

---

## 1. Physical Memory Manager (PMM)

### Bootloader Memory Map
Upon boot, Limine provides a normalized physical memory map through `struct limine_memmap_request`. Entries are classified as:
- **`USABLE`**: General RAM available for dynamic allocation.
- **`RESERVED` / `ACPI`**: Memory-mapped I/O, BIOS regions, ACPI tables. Must never be allocated.
- **`BOOTLOADER_RECLAIMABLE`**: Memory containing bootloader tables and staging buffers. Can be reclaimed after initialization completes.
- **`KERNEL_AND_MODULES`**: Memory occupied by the kernel's ELF binary segments.

### Frame Allocator Strategy
CustomOS implements a **Bitmap Page-Frame Allocator**:
- 1 bit represents one 4 KiB physical page frame (`0 = Free`, `1 = In Use`).
- A 1 GiB machine requires only $1024 \times 1024 \times 1024 / (4096 \times 8) = 32\text{ KiB}$ of bitmap metadata.
- Placed in physical memory immediately following the kernel binary.
- API:
  - `PhysAddr allocate_frame()`
  - `void free_frame(PhysAddr frame)`
  - `void mark_region_reserved(PhysAddr base, usize length)`

---

## 2. Virtual Memory Manager (VMM)

x86-64 uses a 4-level paging hierarchy translating a 48-bit canonical virtual address to a physical address:

```
Virtual Address Bits:
 47        39 38       30 29       21 20       12 11        0
+------------+-----------+-----------+-----------+-----------+
|   PML4     |   PDPT    | Page Dir  | Page Tbl  |  Offset   |
| (9 bits)   | (9 bits)  | (9 bits)  | (9 bits)  | (12 bits) |
+------------+-----------+-----------+-----------+-----------+
```

### Virtual Address Space Partitioning
- **`0x0000000000000000` - `0x00007fffffffffff` (Lower Half, 128 TiB)**:
  Dedicated to user-space processes. Replaced on every process context switch via `%cr3`.
- **`0xffff800000000000` - `0xffff80ffffffffff` (HHDM, 1 TiB)**:
  Direct linear map of all physical memory, provided by Limine. Accessing physical frame `P` is achieved via `(void*)(HHDM_OFFSET + P)`.
- **`0xffffffff80000000` - `0xffffffffffffffff` (Kernel Base, 2 GiB)**:
  Kernel code, static data, stacks, and kernel heap. Shared identically across all address spaces.

---

## 3. Kernel Heap Allocator

Dynamic allocations (`kmalloc`, `kfree`) operate over a dedicated virtual heap range:
- Uses a **Freelist / Buddy / Slab Allocator**.
- Expands heap backing pages on-demand by requesting physical frames from the PMM and mapping them into the kernel page table via the VMM.
- Invariants:
  - Strict size bounds and integer overflow checks on `size_t`.
  - Double-free detection and canary headers to catch buffer overflows.
