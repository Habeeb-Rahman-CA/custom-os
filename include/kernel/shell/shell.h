#pragma once

#include <kernel/core/types.h>

namespace kernel::shell {

class Shell {
public:
    static void init();

    // Execute a command string directly
    static void execute_command(const char* cmdline);

    // Run interactive REPL loop
    static void run_repl();
};

} // namespace kernel::shell

