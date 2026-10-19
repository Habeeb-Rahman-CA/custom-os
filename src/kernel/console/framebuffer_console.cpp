#include <kernel/console/framebuffer_console.h>
#include <kernel/core/boot_info.h>
#include <kernel/core/logger.h>

namespace kernel::console {

namespace {

u32* g_fb_ptr = nullptr;
u64  g_width = 0;
u64  g_height = 0;
u64  g_pitch = 0; // Pitch in pixels (pitch_bytes / 4)

u64 g_cursor_x = 0;
u64 g_cursor_y = 0;

constexpr u64 CHAR_WIDTH = 8;
constexpr u64 CHAR_HEIGHT = 16;

// Minimal embedded 8x16 ASCII font table (common printable characters)
// For simplicity, characters are drawn using an 8x16 bitmap.
// Default character fallback: draws a block or border for printable characters.
const u8 default_font[128][16] = {
    // 0-31 non-printable
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    // 32 (space)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 33 (!)
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00},
    // 65 (A)
    {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 66 (B)
    {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 67 (C)
    {0x00, 0x00, 0x3C, 0x66, 0x60, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

void draw_glyph(u64 x, u64 y, char c, u32 fg, u32 bg) {
    if (!g_fb_ptr) return;

    u8 char_idx = static_cast<u8>(c);
    const u8* glyph = default_font[char_idx < 128 ? char_idx : 0];

    for (u64 row = 0; row < CHAR_HEIGHT; ++row) {
        u8 bits = glyph[row];
        // If glyph row is empty for other chars, generate readable block pattern
        if (bits == 0 && c > 32 && c != ' ') {
            if (row == 2 || row == CHAR_HEIGHT - 3) bits = 0x7E;
            else if (row > 2 && row < CHAR_HEIGHT - 3) bits = 0x42;
        }

        for (u64 col = 0; col < CHAR_WIDTH; ++col) {
            u32 color = (bits & (1 << (7 - col))) ? fg : bg;
            g_fb_ptr[(y + row) * g_pitch + (x + col)] = color;
        }
    }
}

void scroll() {
    if (!g_fb_ptr) return;
    u64 rows_to_move = g_height - CHAR_HEIGHT;

    for (u64 y = 0; y < rows_to_move; ++y) {
        for (u64 x = 0; x < g_width; ++x) {
            g_fb_ptr[y * g_pitch + x] = g_fb_ptr[(y + CHAR_HEIGHT) * g_pitch + x];
        }
    }

    for (u64 y = rows_to_move; y < g_height; ++y) {
        for (u64 x = 0; x < g_width; ++x) {
            g_fb_ptr[y * g_pitch + x] = 0x001B2838;
        }
    }

    g_cursor_y -= CHAR_HEIGHT;
}

} // namespace

void FbConsole::init() {
    const auto& fb = boot::get_info().fb_info;
    if (!fb.available || fb.address == 0) {
        log::warn("[CONSOLE] Framebuffer not available; operating in serial-only mode.");
        return;
    }

    g_fb_ptr = reinterpret_cast<u32*>(fb.address);
    g_width  = fb.width;
    g_height = fb.height;
    g_pitch  = fb.pitch / 4; // Bytes to 32-bit pixel count

    clear();
    log::info("[CONSOLE] Framebuffer graphical console initialized.");
}

bool FbConsole::is_available() {
    return g_fb_ptr != nullptr;
}

void FbConsole::clear(u32 color) {
    if (!g_fb_ptr) return;
    for (u64 y = 0; y < g_height; ++y) {
        for (u64 x = 0; x < g_width; ++x) {
            g_fb_ptr[y * g_pitch + x] = color;
        }
    }
    g_cursor_x = 0;
    g_cursor_y = 0;
}

void FbConsole::put_char(char c, u32 fg, u32 bg) {
    if (!g_fb_ptr) return;

    if (c == '\n') {
        g_cursor_x = 0;
        g_cursor_y += CHAR_HEIGHT;
        if (g_cursor_y + CHAR_HEIGHT > g_height) {
            scroll();
        }
        return;
    }

    if (c == '\r') {
        g_cursor_x = 0;
        return;
    }

    if (c == '\b') {
        if (g_cursor_x >= CHAR_WIDTH) {
            g_cursor_x -= CHAR_WIDTH;
            draw_glyph(g_cursor_x, g_cursor_y, ' ', bg, bg);
        }
        return;
    }

    draw_glyph(g_cursor_x, g_cursor_y, c, fg, bg);
    g_cursor_x += CHAR_WIDTH;
    if (g_cursor_x + CHAR_WIDTH > g_width) {
        g_cursor_x = 0;
        g_cursor_y += CHAR_HEIGHT;
        if (g_cursor_y + CHAR_HEIGHT > g_height) {
            scroll();
        }
    }
}

void FbConsole::write_string(const char* str, u32 fg, u32 bg) {
    if (!str || !g_fb_ptr) return;
    for (usize i = 0; str[i] != '\0'; ++i) {
        put_char(str[i], fg, bg);
    }
}

} // namespace kernel::console

