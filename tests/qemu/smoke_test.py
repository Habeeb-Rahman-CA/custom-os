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
    "[Stage 1/3] Serial console & early logging online",
    "[Stage 2/3] Parsing Limine bootloader protocol responses",
    "Bootloader Name: Limine",
    "HHDM Direct Map Base: 0xffff",
    "Memory Map Entries:",
    "[Stage 3/3] Verifying early kernel invariants",
    "Kernel invariant self-checks PASSED",
    ">>> Milestone 1: Toolchain, bootloader, linker & QEMU verified! <<<"
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
