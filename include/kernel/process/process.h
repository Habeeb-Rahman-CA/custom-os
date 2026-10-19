#pragma once

#include <kernel/core/types.h>
#include <kernel/executable/elf.h>

namespace kernel::process {

struct Process {
    u64 pid{0};
    const char* name{"unnamed"};
    PhysAddr pml4{0};
    u64 user_entry{0};
    u64 user_stack{0};
};

class ProcessManager {
public:
    static void init();

    // Spawn a process from an in-memory ELF image
    static Process* spawn_from_elf(const char* name, const u8* elf_data, usize elf_size);

    // Launch execution of the process in Ring 3
    [[noreturn]] static void execute_user_process(Process* proc);
};

} // namespace kernel::process

