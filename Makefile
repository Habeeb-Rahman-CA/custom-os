# CustomOS Makefile
# Target: x86-64 Freestanding Kernel with Limine Bootloader

CXX      ?= g++
AS       ?= as
LD       ?= ld
PYTHON   ?= python3

BUILD_DIR := build
SRC_DIR   := src
INC_DIR   := include

# Compiler flags for x86-64 freestanding kernel
CXXFLAGS  := -std=c++20 \
             -Wall -Wextra -Wpedantic \
             -ffreestanding \
             -fno-builtin \
             -fno-exceptions \
             -fno-rtti \
             -fno-pie \
             -fno-pic \
             -mno-red-zone \
             -mcmodel=kernel \
             -mgeneral-regs-only \
             -O2 -g \
             -I$(INC_DIR)

ASFLAGS   := --64
LDFLAGS   := -nostdlib -static -z max-page-size=0x1000 -T linker.ld

# Kernel object files
OBJS := $(BUILD_DIR)/boot.o \
        $(BUILD_DIR)/entry.o \
        $(BUILD_DIR)/serial.o \
        $(BUILD_DIR)/logger.o \
        $(BUILD_DIR)/panic.o \
        $(BUILD_DIR)/boot_info.o \
        $(BUILD_DIR)/gdt.o \
        $(BUILD_DIR)/gdt_flush.o \
        $(BUILD_DIR)/idt.o \
        $(BUILD_DIR)/idt_stubs.o \
        $(BUILD_DIR)/pic.o \
        $(BUILD_DIR)/pmm.o \
        $(BUILD_DIR)/vmm.o \
        $(BUILD_DIR)/heap.o \
        $(BUILD_DIR)/timer.o \
        $(BUILD_DIR)/keyboard.o \
        $(BUILD_DIR)/framebuffer_console.o \
        $(BUILD_DIR)/scheduler.o \
        $(BUILD_DIR)/switch_to.o \
        $(BUILD_DIR)/syscall_entry.o \
        $(BUILD_DIR)/syscall.o \
        $(BUILD_DIR)/elf.o \
        $(BUILD_DIR)/builtin_elf.o \
        $(BUILD_DIR)/user_jump.o \
        $(BUILD_DIR)/process.o \
        $(BUILD_DIR)/vfs.o \
        $(BUILD_DIR)/shell.o

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
DISK_IMAGE := $(BUILD_DIR)/os.hdd
LIMINE_DIR := external/limine

.PHONY: all clean kernel disk run run-uefi run-gui test test-unit test-qemu gdb debug

all: disk

# Compile Assembly
$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot/boot.S | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

# Compile C++ kernel sources
$(BUILD_DIR)/entry.o: $(SRC_DIR)/kernel/arch/x86_64/entry.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/serial.o: $(SRC_DIR)/kernel/drivers/serial.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/logger.o: $(SRC_DIR)/kernel/core/logger.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/panic.o: $(SRC_DIR)/kernel/lib/panic.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/boot_info.o: $(SRC_DIR)/kernel/core/boot_info.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: $(SRC_DIR)/kernel/arch/x86_64/gdt.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt_flush.o: $(SRC_DIR)/kernel/arch/x86_64/gdt_flush.S | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/idt.o: $(SRC_DIR)/kernel/interrupts/idt.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/idt_stubs.o: $(SRC_DIR)/kernel/interrupts/idt_stubs.S | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/pic.o: $(SRC_DIR)/kernel/interrupts/pic.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/pmm.o: $(SRC_DIR)/kernel/memory/pmm.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/vmm.o: $(SRC_DIR)/kernel/memory/vmm.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/heap.o: $(SRC_DIR)/kernel/memory/heap.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/timer.o: $(SRC_DIR)/kernel/drivers/timer.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: $(SRC_DIR)/kernel/drivers/keyboard.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/framebuffer_console.o: $(SRC_DIR)/kernel/console/framebuffer_console.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: $(SRC_DIR)/kernel/scheduler/scheduler.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/switch_to.o: $(SRC_DIR)/kernel/scheduler/switch_to.S | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/hello.elf: user/programs/hello.S | $(BUILD_DIR)
	@echo "[USER-CC] $< -> $@"
	@gcc -nostdlib -static -fno-pie -no-pie $< -o $@

$(BUILD_DIR)/fault.elf: user/programs/fault.S | $(BUILD_DIR)
	@echo "[USER-CC] $< -> $@"
	@gcc -nostdlib -static -fno-pie -no-pie $< -o $@

$(BUILD_DIR)/builtin_elf.o: $(SRC_DIR)/kernel/executable/builtin_elf.S $(BUILD_DIR)/hello.elf $(BUILD_DIR)/fault.elf | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/syscall_entry.o: $(SRC_DIR)/kernel/syscall/syscall_entry.S | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: $(SRC_DIR)/kernel/syscall/syscall.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/elf.o: $(SRC_DIR)/kernel/executable/elf.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/user_jump.o: $(SRC_DIR)/kernel/process/user_jump.S | $(BUILD_DIR)
	@echo "[AS]  $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/process.o: $(SRC_DIR)/kernel/process/process.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/vfs.o: $(SRC_DIR)/kernel/filesystem/vfs.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/shell.o: $(SRC_DIR)/kernel/shell/shell.cpp | $(BUILD_DIR)
	@echo "[CXX] $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Link Kernel ELF
$(KERNEL_ELF): $(OBJS) linker.ld
	@echo "[LD]  $(OBJS) -> $@"
	@$(LD) $(LDFLAGS) $(OBJS) -o $@

kernel: $(KERNEL_ELF)

# Create raw bootable GPT disk image
$(DISK_IMAGE): $(KERNEL_ELF) limine.conf tools/build/create_disk.sh
	@echo "[DISK] Generating bootable disk image $@..."
	@./tools/build/create_disk.sh $(KERNEL_ELF) $(DISK_IMAGE) $(LIMINE_DIR)

disk: $(DISK_IMAGE)

# QEMU BIOS launch (serial to terminal, display off)
run: $(DISK_IMAGE)
	qemu-system-x86_64 -M q35 -m 256M -drive file=$(DISK_IMAGE),format=raw -serial stdio -display none

# QEMU UEFI launch (using OVMF)
run-uefi: $(DISK_IMAGE)
	qemu-system-x86_64 -M q35 -m 256M -bios /usr/share/OVMF/OVMF_CODE.fd -drive file=$(DISK_IMAGE),format=raw -serial stdio -display none

# QEMU with graphical window
run-gui: $(DISK_IMAGE)
	qemu-system-x86_64 -M q35 -m 256M -drive file=$(DISK_IMAGE),format=raw -serial stdio

# GDB debug server (waits for gdb connection on :1234)
gdb: $(DISK_IMAGE)
	qemu-system-x86_64 -M q35 -m 256M -drive file=$(DISK_IMAGE),format=raw -serial stdio -display none -s -S

debug: $(KERNEL_ELF)
	gdb -ex "target remote :1234" -ex "symbol-file $(KERNEL_ELF)"

# Tests
test-unit: $(BUILD_DIR)
	@echo "[TEST] Building and running host unit tests..."
	@$(CXX) -std=c++20 -I$(INC_DIR) tests/unit/test_types.cpp -o $(BUILD_DIR)/test_types
	@$(BUILD_DIR)/test_types

test-qemu: $(DISK_IMAGE)
	@echo "[TEST] Running automated QEMU smoke test..."
	@$(PYTHON) tests/qemu/smoke_test.py $(DISK_IMAGE) 10

test: test-unit test-qemu

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory."

