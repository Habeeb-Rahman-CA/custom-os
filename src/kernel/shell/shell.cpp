#include <kernel/shell/shell.h>
#include <kernel/filesystem/vfs.h>
#include <kernel/process/process.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/heap.h>
#include <kernel/drivers/serial.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/console/framebuffer_console.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/core/logger.h>

namespace kernel::shell {

namespace {

int string_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void print(const char* str) {
    drivers::serial::write_string(str);
    console::FbConsole::write_string(str);
}

void println(const char* str = "") {
    print(str);
    print("\n");
}

void cmd_help() {
    println("CustomOS Shell Built-in Commands:");
    println("  help       - Display available commands");
    println("  clear      - Clear graphical console screen");
    println("  echo       - Print arguments to console");
    println("  meminfo    - Display memory usage (PMM and Heap)");
    println("  tasks      - List active tasks and execution states");
    println("  uptime     - Display system uptime and timer ticks");
    println("  ls [path]  - List directory files in VFS");
    println("  cat <file> - Print contents of a file");
    println("  pwd        - Print current working directory");
    println("  run <elf>  - Load and execute an ELF binary in user mode");
    println("  exit       - Exit shell session");
}

void cmd_clear() {
    console::FbConsole::clear();
}

void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        print(argv[i]);
        if (i + 1 < argc) print(" ");
    }
    println();
}

void cmd_meminfo() {
    println("--- Memory Subsystem Statistics ---");
    print("Physical RAM Total: ");
    drivers::serial::write_u64_dec(memory::Pmm::get_total_bytes() / (1024 * 1024));
    println(" MiB");

    print("Physical RAM Free:  ");
    drivers::serial::write_u64_dec(memory::Pmm::get_free_bytes() / (1024 * 1024));
    println(" MiB");

    print("Kernel Heap In-Use: ");
    drivers::serial::write_u64_dec(memory::get_heap_allocated_bytes());
    println(" bytes");

    print("Kernel Heap Mapped: ");
    drivers::serial::write_u64_dec(memory::get_heap_total_bytes() / 1024);
    println(" KiB");
    println("------------------------------------");
}

void cmd_tasks() {
    scheduler::Scheduler::dump_tasks();
}

void cmd_uptime() {
    print("System Uptime: ");
    drivers::serial::write_u64_dec(drivers::timer::get_uptime_sec());
    print(" seconds (");
    drivers::serial::write_u64_dec(drivers::timer::get_ticks());
    println(" timer ticks)");
}

void cmd_pwd() {
    println("/");
}

void cmd_ls(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "/";
    fs::VFSNode* dir = fs::VFS::lookup(path);
    if (!dir) {
        println("ls: directory not found");
        return;
    }
    if (dir->type != fs::NodeType::Directory) {
        println("ls: not a directory");
        return;
    }

    fs::VFSNode* child = dir->first_child;
    while (child) {
        if (child->type == fs::NodeType::Directory) {
            print("[DIR]  ");
        } else {
            print("[FILE] ");
        }
        print(child->name);
        if (child->type == fs::NodeType::File) {
            print(" (");
            drivers::serial::write_u64_dec(child->size);
            print(" bytes)");
        }
        println();
        child = child->next_sibling;
    }
}

void cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        println("Usage: cat <filename>");
        return;
    }
    fs::VFSNode* file = fs::VFS::lookup(argv[1]);
    if (!file) {
        println("cat: file not found");
        return;
    }
    if (file->type == fs::NodeType::Directory) {
        println("cat: cannot cat a directory");
        return;
    }

    char buf[128];
    usize offset = 0;
    while (offset < file->size) {
        ssize_t bytes = file->read(offset, buf, sizeof(buf) - 1);
        if (bytes <= 0) break;
        buf[bytes] = '\0';
        print(buf);
        offset += bytes;
    }
    println();
}

void cmd_run(int argc, char** argv) {
    if (argc < 2) {
        println("Usage: run <path_to_elf>");
        return;
    }
    fs::VFSNode* elf_file = fs::VFS::lookup(argv[1]);
    if (!elf_file) {
        println("run: executable not found in VFS");
        return;
    }

    println("[SHELL] Spawning user process from ELF...");
    auto* proc = process::ProcessManager::spawn_from_elf(argv[1], elf_file->data, elf_file->size);
    if (!proc) {
        println("run: failed to load ELF executable");
        return;
    }

    println("[SHELL] Transferring execution to Ring 3 user mode...");
    process::ProcessManager::execute_user_process(proc);
}

} // namespace

void Shell::init() {
    log::info("[SHELL] Command-Line Shell subsystem initialized.");
}

void Shell::execute_command(const char* cmdline) {
    if (!cmdline || !*cmdline) return;

    // Tokenize
    char buf[128];
    usize len = 0;
    while (cmdline[len] && len + 1 < sizeof(buf)) {
        buf[len] = cmdline[len];
        len++;
    }
    buf[len] = '\0';

    char* argv[16];
    int argc = 0;
    char* p = buf;

    while (*p && argc < 16) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) {
            *p++ = '\0';
        }
    }

    if (argc == 0) return;

    print("custom_os> ");
    println(cmdline);

    if (string_compare(argv[0], "help") == 0) {
        cmd_help();
    } else if (string_compare(argv[0], "clear") == 0) {
        cmd_clear();
    } else if (string_compare(argv[0], "echo") == 0) {
        cmd_echo(argc, argv);
    } else if (string_compare(argv[0], "meminfo") == 0) {
        cmd_meminfo();
    } else if (string_compare(argv[0], "tasks") == 0) {
        cmd_tasks();
    } else if (string_compare(argv[0], "uptime") == 0) {
        cmd_uptime();
    } else if (string_compare(argv[0], "pwd") == 0) {
        cmd_pwd();
    } else if (string_compare(argv[0], "ls") == 0) {
        cmd_ls(argc, argv);
    } else if (string_compare(argv[0], "cat") == 0) {
        cmd_cat(argc, argv);
    } else if (string_compare(argv[0], "run") == 0) {
        cmd_run(argc, argv);
    } else if (string_compare(argv[0], "exit") == 0) {
        println("[SHELL] Shell session terminated.");
    } else {
        print("Unknown command: ");
        println(argv[0]);
        println("Type 'help' for available commands.");
    }
}

void Shell::run_repl() {
    char input_buf[128];
    usize idx = 0;

    print("\ncustom_os> ");
    while (true) {
        char c = drivers::keyboard::get_char();
        if (c == '\r' || c == '\n') {
            input_buf[idx] = '\0';
            println();
            if (idx > 0) {
                execute_command(input_buf);
                idx = 0;
            }
            print("custom_os> ");
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                print("\b \b");
            }
        } else if (idx + 1 < sizeof(input_buf)) {
            input_buf[idx++] = c;
            drivers::serial::write_char(c);
            console::FbConsole::put_char(c);
        }
    }
}

} // namespace kernel::shell
