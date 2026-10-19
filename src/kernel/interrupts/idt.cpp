#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/pic.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>
#include <kernel/drivers/serial.h>
#include <kernel/arch/x86_64/gdt.h>

using namespace kernel;

namespace kernel::interrupts {

namespace {

alignas(16) IdtEntry g_idt[256]{};
IdtPointer g_idt_ptr{};
HandlerFunc g_handlers[256]{nullptr};

extern "C" u64 isr_stub_table[256];

} // namespace

const char* const exception_names[32] = {
    "Divide Error (#DE)",
    "Debug (#DB)",
    "Non-Maskable Interrupt (#NMI)",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "Bound Range Exceeded (#BR)",
    "Invalid Opcode (#UD)",
    "Device Not Available (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Reserved",
    "x87 FPU Floating-Point Error (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception (#HV)",
    "VMM Communication Exception (#VC)",
    "Security Exception (#SX)",
    "Reserved"
};

u64 read_cr2() {
    u64 val;
    asm volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

void IdtEntry::set_handler(u64 handler_addr, u16 sel, u8 ist_index, u8 dpl) {
    offset_low  = static_cast<u16>(handler_addr & 0xFFFF);
    selector    = sel;
    ist         = ist_index & 0x07;
    // 0x8E = Present | DPL=dpl | 0 | 64-bit Interrupt Gate (0xE)
    type_attr   = static_cast<u8>(0x8E | ((dpl & 3) << 5));
    offset_mid  = static_cast<u16>((handler_addr >> 16) & 0xFFFF);
    offset_high = static_cast<u32>((handler_addr >> 32) & 0xFFFFFFFF);
    zero        = 0;
}

void Idt::init() {
    for (int i = 0; i < 256; ++i) {
        // Double fault (#DF = 8) uses IST1 stack
        u8 ist = (i == 8) ? 1 : 0;
        // Breakpoint (#BP = 3) allows invocation from Ring 3 (DPL=3)
        u8 dpl = (i == 3) ? 3 : 0;
        g_idt[i].set_handler(isr_stub_table[i], arch::x86_64::KERNEL_CODE_SELECTOR, ist, dpl);
    }

    g_idt_ptr.limit = sizeof(g_idt) - 1;
    g_idt_ptr.base  = reinterpret_cast<u64>(&g_idt);

    asm volatile("lidt (%0)" : : "r"(&g_idt_ptr) : "memory");

    log::info("[IDT] 256-entry Interrupt Descriptor Table loaded.");
}

void Idt::register_handler(u8 vector, HandlerFunc handler) {
    g_handlers[vector] = handler;
}

void Idt::dump_frame(const InterruptFrame* frame) {
    drivers::serial::write_string("\n========== CPU REGISTER DUMP ==========\n");
    drivers::serial::write_string("Vector:      ");
    drivers::serial::write_u64_dec(frame->vector);
    if (frame->vector < 32) {
        drivers::serial::write_string(" (");
        drivers::serial::write_string(exception_names[frame->vector]);
        drivers::serial::write_string(")");
    }
    drivers::serial::write_char('\n');

    drivers::serial::write_string("Error Code:  ");
    drivers::serial::write_u64_hex(frame->error_code);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("RIP:         ");
    drivers::serial::write_u64_hex(frame->rip);
    drivers::serial::write_string("  CS:     ");
    drivers::serial::write_u64_hex(frame->cs);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("RSP:         ");
    drivers::serial::write_u64_hex(frame->rsp);
    drivers::serial::write_string("  SS:     ");
    drivers::serial::write_u64_hex(frame->ss);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("RFLAGS:      ");
    drivers::serial::write_u64_hex(frame->rflags);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("RAX: "); drivers::serial::write_u64_hex(frame->rax);
    drivers::serial::write_string("  RBX: "); drivers::serial::write_u64_hex(frame->rbx);
    drivers::serial::write_string("  RCX: "); drivers::serial::write_u64_hex(frame->rcx);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("RDX: "); drivers::serial::write_u64_hex(frame->rdx);
    drivers::serial::write_string("  RSI: "); drivers::serial::write_u64_hex(frame->rsi);
    drivers::serial::write_string("  RDI: "); drivers::serial::write_u64_hex(frame->rdi);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("RBP: "); drivers::serial::write_u64_hex(frame->rbp);
    drivers::serial::write_string("  R8:  "); drivers::serial::write_u64_hex(frame->r8);
    drivers::serial::write_string("  R9:  "); drivers::serial::write_u64_hex(frame->r9);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("R10: "); drivers::serial::write_u64_hex(frame->r10);
    drivers::serial::write_string("  R11: "); drivers::serial::write_u64_hex(frame->r11);
    drivers::serial::write_string("  R12: "); drivers::serial::write_u64_hex(frame->r12);
    drivers::serial::write_char('\n');

    drivers::serial::write_string("R13: "); drivers::serial::write_u64_hex(frame->r13);
    drivers::serial::write_string("  R14: "); drivers::serial::write_u64_hex(frame->r14);
    drivers::serial::write_string("  R15: "); drivers::serial::write_u64_hex(frame->r15);
    drivers::serial::write_char('\n');

    if (frame->vector == 14) {
        u64 cr2 = read_cr2();
        drivers::serial::write_string("Faulting CR2 Virtual Address: ");
        drivers::serial::write_u64_hex(cr2);
        drivers::serial::write_char('\n');

        drivers::serial::write_string("Page Fault Flags: [ ");
        if (frame->error_code & (1 << 0)) drivers::serial::write_string("PROTECTION_VIOLATION ");
        else drivers::serial::write_string("PAGE_NOT_PRESENT ");

        if (frame->error_code & (1 << 1)) drivers::serial::write_string("WRITE ");
        else drivers::serial::write_string("READ ");

        if (frame->error_code & (1 << 2)) drivers::serial::write_string("USER ");
        else drivers::serial::write_string("KERNEL ");

        if (frame->error_code & (1 << 3)) drivers::serial::write_string("RSVD_WRITE ");
        if (frame->error_code & (1 << 4)) drivers::serial::write_string("INSTR_FETCH ");
        drivers::serial::write_string("]\n");
    }
    drivers::serial::write_string("========================================\n");
}

} // namespace kernel::interrupts

// C entry point called from assembly common_isr_stub
extern "C" void exception_dispatch(kernel::interrupts::InterruptFrame* frame) {
    u64 vec = frame->vector;

    if (kernel::interrupts::g_handlers[vec]) {
        kernel::interrupts::g_handlers[vec](frame);
    } else if (vec < 32) {
        // User Mode Exception Fault Isolation (Ring 3)
        bool is_user_mode = ((frame->cs & 3) == 3);
        if (is_user_mode) {
            auto* current_task = kernel::scheduler::Scheduler::get_current_task();
            kernel::drivers::serial::write_string("\n[SECURITY/FAULT] Process PID ");
            if (current_task) {
                kernel::drivers::serial::write_u64_dec(current_task->pid);
                kernel::drivers::serial::write_string(" ('");
                kernel::drivers::serial::write_string(current_task->name);
                kernel::drivers::serial::write_string("') ");
            }
            kernel::drivers::serial::write_string("terminated due to unhandled CPU exception: ");
            if (vec < 32 && kernel::interrupts::exception_names[vec]) {
                kernel::drivers::serial::write_string(kernel::interrupts::exception_names[vec]);
            } else {
                kernel::drivers::serial::write_u64_dec(vec);
            }
            kernel::drivers::serial::write_string(" at RIP: ");
            kernel::drivers::serial::write_u64_hex(frame->rip);
            kernel::drivers::serial::write_string(" (ErrorCode: ");
            kernel::drivers::serial::write_u64_hex(frame->error_code);
            kernel::drivers::serial::write_string(")\n");

            if (vec == 14) { // Page Fault
                u64 cr2 = kernel::interrupts::read_cr2();
                kernel::drivers::serial::write_string("[SECURITY/FAULT] Faulting Virtual Address (CR2): ");
                kernel::drivers::serial::write_u64_hex(cr2);
                kernel::drivers::serial::write_char('\n');
            }

            // Safely terminate the faulting user task without crashing the kernel
            kernel::scheduler::Scheduler::exit();
            return;
        }

        kernel::interrupts::Idt::dump_frame(frame);
        kernel::panic("Unhandled Kernel CPU Exception occurred!");
    }

    // Hardware IRQs (32 - 47) require End of Interrupt signal to PIC
    if (vec >= 32 && vec <= 47) {
        kernel::interrupts::Pic::send_eoi(static_cast<kernel::u8>(vec - 32));
    }
}
