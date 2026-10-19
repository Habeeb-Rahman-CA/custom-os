#!/usr/bin/env python3
"""
CustomOS Milestone 1 Smoke Test
Launches QEMU, monitors serial output, and validates expected boot milestones.
"""

import sys
import subprocess
import time
import os

EXPECTED_STRINGS = [
    "CustomOS Kernel v0.1.0",
    "[GDT] 64-bit Global Descriptor Table & TSS loaded with IST1",
    "[PIC] 8259 PIC remapped to vectors 32-47",
    "[IDT] 256-entry Interrupt Descriptor Table loaded",
    "[TEST] CPU Breakpoint (#BP) exception caught and handled cleanly!",
    "[PMM] Physical Frame Allocator (Bitmap) initialized",
    "[TEST] PMM frame allocation & deallocation verified",
    "[VMM] 4-level Paging Virtual Memory Manager initialized",
    "[TEST] VMM virtual page mapping and translation verified",
    "[HEAP] Dynamic Kernel Heap allocator initialized",
    "[TEST] Kernel dynamic heap and C++ new/delete operators verified",
    "[TIMER] PIT timer configured at 100 Hz",
    "[KEYBOARD] PS/2 keyboard driver initialized",
    "[SCHED] Round-Robin Task Scheduler initialized",
    "[TASK A] Task Alpha entered execution",
    "[TASK B] Task Beta entered execution",
    "[SYSCALL] Fast System Call (SYSCALL/SYSRET) infrastructure configured",
    "[PROC] Process Manager initialized",
    "[VFS] Virtual File System initialized with in-memory root",
    "[TEST] VFS file open, read, and close operations verified",
    "[SHELL] Command-Line Shell subsystem initialized",
    "CustomOS Shell Built-in Commands:",
    "Physical RAM Total:",
    "Welcome to CustomOS!",
    "[PROC] Jumping into Ring 3 user space execution...",
    "[USER] >>> Hello from User Mode (Ring 3) via SYSCALL! <<<",
    "[SECURITY] Testing User-Mode Fault Isolation",
    "[SECURITY/FAULT] Process PID",
    "[TEST] User-mode fault isolation verified: process terminated cleanly without kernel panic!",
    ">>> ALL OPERATING SYSTEM MILESTONES COMPLETED SUCCESSFULLY! <<<"
]

def run_test(disk_img="build/os.hdd", timeout_sec=10):
    if not os.path.exists(disk_img):
        print(f"[FAIL] Disk image not found: {disk_img}", file=sys.stderr)
        return 1

    cmd = [
        "qemu-system-x86_64",
        "-M", "q35",
        "-m", "256M",
        "-drive", f"file={disk_img},format=raw",
        "-serial", "stdio",
        "-display", "none"
    ]

    print(f"[INFO] Spawning QEMU: {' '.join(cmd)}")
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    start_time = time.time()
    collected_output = []
    found_indices = set()

    try:
        while time.time() - start_time < timeout_sec:
            line = proc.stdout.readline()
            if line:
                collected_output.append(line)
                sys.stdout.write(line)
                sys.stdout.flush()

                for idx, expected in enumerate(EXPECTED_STRINGS):
                    if expected in line:
                        found_indices.add(idx)

                if len(found_indices) == len(EXPECTED_STRINGS):
                    print("\n[SUCCESS] All expected Milestone 1 boot markers detected!")
                    return 0
            elif proc.poll() is not None:
                break
            time.sleep(0.05)

    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()

    print("\n[FAIL] Test timed out or process exited before all markers were found.", file=sys.stderr)
    print(f"Detected {len(found_indices)} of {len(EXPECTED_STRINGS)} markers:", file=sys.stderr)
    for idx, expected in enumerate(EXPECTED_STRINGS):
        status = "FOUND" if idx in found_indices else "MISSING"
        print(f"  [{status}] {expected}", file=sys.stderr)

    return 1

if __name__ == "__main__":
    disk = sys.argv[1] if len(sys.argv) > 1 else "build/os.hdd"
    timeout = int(sys.argv[2]) if len(sys.argv) > 2 else 10
    sys.exit(run_test(disk, timeout))

