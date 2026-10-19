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

# Kernel executable
add_executable(kernel.elf
    src/boot/boot.S
    src/kernel/arch/x86_64/entry.cpp
    src/kernel/drivers/serial.cpp
    src/kernel/core/logger.cpp
    src/kernel/lib/panic.cpp
    src/kernel/core/boot_info.cpp
)

target_include_directories(kernel.elf PRIVATE include)
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
