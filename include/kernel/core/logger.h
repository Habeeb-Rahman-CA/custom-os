#pragma once

#include <kernel/core/types.h>

namespace kernel::log {

enum class Level {
    Debug,
    Info,
    Warn,
    Error,
    Panic
};

void init();
void write(Level level, const char* msg);
void write_kv(Level level, const char* key, const char* val);
void write_kv_u64_hex(Level level, const char* key, u64 val);
void write_kv_u64_dec(Level level, const char* key, u64 val);

inline void info(const char* msg)  { write(Level::Info, msg); }
inline void warn(const char* msg)  { write(Level::Warn, msg); }
inline void error(const char* msg) { write(Level::Error, msg); }

} // namespace kernel::log
