#include <kernel/core/logger.h>
#include <kernel/drivers/serial.h>

namespace kernel::log {

namespace {

void print_prefix(Level level) {
    switch (level) {
        case Level::Debug:
            drivers::serial::write_string("[DEBUG] ");
            break;
        case Level::Info:
            drivers::serial::write_string("[INFO]  ");
            break;
        case Level::Warn:
            drivers::serial::write_string("[WARN]  ");
            break;
        case Level::Error:
            drivers::serial::write_string("[ERROR] ");
            break;
        case Level::Panic:
            drivers::serial::write_string("[PANIC] ");
            break;
    }
}

} // namespace

void init() {
    drivers::serial::init();
}

void write(Level level, const char* msg) {
    print_prefix(level);
    drivers::serial::write_string(msg);
    drivers::serial::write_char('\n');
}

void write_kv(Level level, const char* key, const char* val) {
    print_prefix(level);
    drivers::serial::write_string(key);
    drivers::serial::write_string(": ");
    drivers::serial::write_string(val);
    drivers::serial::write_char('\n');
}

void write_kv_u64_hex(Level level, const char* key, u64 val) {
    print_prefix(level);
    drivers::serial::write_string(key);
    drivers::serial::write_string(": ");
    drivers::serial::write_u64_hex(val);
    drivers::serial::write_char('\n');
}

void write_kv_u64_dec(Level level, const char* key, u64 val) {
    print_prefix(level);
    drivers::serial::write_string(key);
    drivers::serial::write_string(": ");
    drivers::serial::write_u64_dec(val);
    drivers::serial::write_char('\n');
}

} // namespace kernel::log
