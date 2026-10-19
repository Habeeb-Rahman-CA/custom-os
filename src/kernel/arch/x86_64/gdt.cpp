#include <kernel/arch/x86_64/gdt.h>
#include <kernel/core/logger.h>

namespace kernel::arch::x86_64 {

namespace {

struct [[gnu::packed]] GdtTable {
    GdtDescriptor null_desc;       // 0x00
    GdtDescriptor kernel_code;     // 0x08
    GdtDescriptor kernel_data;     // 0x10
    GdtDescriptor user_data;       // 0x18
    GdtDescriptor user_code;       // 0x20
    TssDescriptor tss;             // 0x28 (16 bytes in x86-64)
};

alignas(16) GdtTable g_gdt{};
alignas(16) Tss g_tss{};
GdtPointer g_gdt_ptr{};

alignas(16) u8 g_df_stack[0x2000];          // 8 KiB dedicated stack for Double Faults (IST1)
alignas(16) u8 g_kernel_rsp0_stack[0x4000];  // 16 KiB dedicated stack for Ring 3 -> Ring 0 transitions (RSP0)

extern "C" void gdt_flush(u64 gdt_ptr_addr, u16 code_sel, u16 data_sel, u16 tss_sel);

} // namespace

void Gdt::set_kernel_stack(u64 rsp0) {
    g_tss.rsp0 = rsp0;
}

Tss& Gdt::get_tss() {
    return g_tss;
}

void Gdt::init() {
    // 0x00: Null descriptor
    g_gdt.null_desc = GdtDescriptor{};

    // 0x08: Kernel Code 64-bit (Ring 0, Exec/Read, Conforming=0, Code=1)
    // Access: 0x9A = Present(1) | Ring0(00) | 1 | Exec(1) | Read(1) | 0
    // Granularity: 0x20 = LongMode(1) | 0
    g_gdt.kernel_code.access = 0x9A;
    g_gdt.kernel_code.granularity = 0x20;

    // 0x10: Kernel Data 64-bit (Ring 0, Read/Write)
    // Access: 0x92 = Present(1) | Ring0(00) | 1 | Read/Write(1) | 0
    g_gdt.kernel_data.access = 0x92;
    g_gdt.kernel_data.granularity = 0x00;

    // 0x18: User Data 64-bit (Ring 3, Read/Write)
    // Access: 0xF2 = Present(1) | Ring3(11) | 1 | Read/Write(1) | 0
    g_gdt.user_data.access = 0xF2;
    g_gdt.user_data.granularity = 0x00;

    // 0x20: User Code 64-bit (Ring 3, Exec/Read, Long Mode)
    // Access: 0xFA = Present(1) | Ring3(11) | 1 | Exec(1) | Read(1) | 0
    // Granularity: 0x20 = LongMode(1)
    g_gdt.user_code.access = 0xFA;
    g_gdt.user_code.granularity = 0x20;

    // Setup Double Fault IST1 stack and Ring 0 transition stack RSP0
    g_tss.ist1 = reinterpret_cast<u64>(g_df_stack) + sizeof(g_df_stack);
    g_tss.rsp0 = reinterpret_cast<u64>(g_kernel_rsp0_stack) + sizeof(g_kernel_rsp0_stack);

    // 0x28: TSS Descriptor (16 bytes in x86-64)
    u64 tss_base = reinterpret_cast<u64>(&g_tss);
    u32 tss_limit = sizeof(Tss) - 1;

    g_gdt.tss.limit_low = static_cast<u16>(tss_limit & 0xFFFF);
    g_gdt.tss.base_low  = static_cast<u16>(tss_base & 0xFFFF);
    g_gdt.tss.base_mid  = static_cast<u8>((tss_base >> 16) & 0xFF);
    // Access: 0x89 = Present(1) | DPL=0(00) | 0 | Type=Available 64-bit TSS (1001)
    g_gdt.tss.access    = 0x89;
    g_gdt.tss.granularity = static_cast<u8>((tss_limit >> 16) & 0x0F);
    g_gdt.tss.base_mid_high = static_cast<u8>((tss_base >> 24) & 0xFF);
    g_gdt.tss.base_high     = static_cast<u32>((tss_base >> 32) & 0xFFFFFFFF);
    g_gdt.tss.reserved      = 0;

    // Setup GDT Pointer
    g_gdt_ptr.limit = sizeof(GdtTable) - 1;
    g_gdt_ptr.base  = reinterpret_cast<u64>(&g_gdt);

    gdt_flush(reinterpret_cast<u64>(&g_gdt_ptr), KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR, TSS_SELECTOR);

    log::info("[GDT] 64-bit Global Descriptor Table & TSS loaded with IST1.");
}

} // namespace kernel::arch::x86_64
