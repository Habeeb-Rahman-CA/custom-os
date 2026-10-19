#pragma once

#include <kernel/core/types.h>

namespace kernel::executable {

struct [[gnu::packed]] Elf64Header {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

struct [[gnu::packed]] Elf64Phdr {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
};

constexpr u32 PT_LOAD = 1;

constexpr u32 PF_X = 1 << 0;
constexpr u32 PF_W = 1 << 1;
constexpr u32 PF_R = 1 << 2;

struct LoadedElf {
    u64 entry_point{0};
    u64 user_stack_top{0};
    PhysAddr pml4{0};
    bool valid{false};
};

class ElfLoader {
public:
    static LoadedElf load(const u8* elf_data, usize elf_size);
};

} // namespace kernel::executable

