cmake_minimum_required(VERSION 3.20)
project(CustomOS LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Kernel freestanding compilation flags
set(KERNEL_CXX_FLAGS
    -ffreestanding
    -fno-builtin
    -fno-exceptions
    -fno-rtti
    -fno-pie
    -fno-pic
    -mno-red-zone
    -mcmodel=kernel
    -mgeneral-regs-only
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -g
)

set(KERNEL_LINK_FLAGS
    -nostdlib
    -static
    -z max-page-size=0x1000
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}")

# User-space ELF binary targets
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/hello.elf
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/build
    COMMAND gcc -nostdlib -static -fno-pie -no-pie ${CMAKE_CURRENT_SOURCE_DIR}/user/programs/hello.S -o ${CMAKE_CURRENT_BINARY_DIR}/build/hello.elf
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/user/programs/hello.S
    COMMENT "Compiling user hello.elf program"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/fault.elf
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/build
    COMMAND gcc -nostdlib -static -fno-pie -no-pie ${CMAKE_CURRENT_SOURCE_DIR}/user/programs/fault.S -o ${CMAKE_CURRENT_BINARY_DIR}/build/fault.elf
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/user/programs/fault.S
    COMMENT "Compiling user fault.elf program"
)

# Kernel executable
add_executable(kernel.elf
    src/boot/boot.S
    src/kernel/arch/x86_64/entry.cpp
    src/kernel/drivers/serial.cpp
    src/kernel/core/logger.cpp
    src/kernel/lib/panic.cpp
    src/kernel/core/boot_info.cpp
    src/kernel/arch/x86_64/gdt.cpp
    src/kernel/arch/x86_64/gdt_flush.S
    src/kernel/interrupts/idt.cpp
    src/kernel/interrupts/idt_stubs.S
    src/kernel/interrupts/pic.cpp
    src/kernel/memory/pmm.cpp
    src/kernel/memory/vmm.cpp
    src/kernel/memory/heap.cpp
    src/kernel/drivers/timer.cpp
    src/kernel/drivers/keyboard.cpp
    src/kernel/console/framebuffer_console.cpp
    src/kernel/scheduler/scheduler.cpp
    src/kernel/scheduler/switch_to.S
    src/kernel/syscall/syscall_entry.S
    src/kernel/syscall/syscall.cpp
    src/kernel/executable/elf.cpp
    src/kernel/executable/builtin_elf.S
    src/kernel/process/user_jump.S
    src/kernel/process/process.cpp
    src/kernel/filesystem/vfs.cpp
    src/kernel/shell/shell.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/build/hello.elf
    ${CMAKE_CURRENT_BINARY_DIR}/build/fault.elf
)

target_include_directories(kernel.elf PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(kernel.elf PRIVATE ${KERNEL_CXX_FLAGS})
target_link_options(kernel.elf PRIVATE ${KERNEL_LINK_FLAGS})

# Disk image generation target
add_custom_target(disk
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/build/create_disk.sh
            ${CMAKE_CURRENT_BINARY_DIR}/kernel.elf
            ${CMAKE_CURRENT_BINARY_DIR}/os.hdd
            ${CMAKE_CURRENT_SOURCE_DIR}/external/limine
    DEPENDS kernel.elf
    COMMENT "Generating bootable raw GPT disk image (os.hdd)"
)

# Host unit tests
add_executable(test_types tests/unit/test_types.cpp)
target_include_directories(test_types PRIVATE include)
target_compile_features(test_types PRIVATE cxx_std_20)

enable_testing()
add_test(NAME HostTypesTest COMMAND test_types)
