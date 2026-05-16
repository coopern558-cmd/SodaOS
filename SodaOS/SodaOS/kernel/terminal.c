#include "terminal.h"
#include "graphics.h"
#include "keyboard.h"
#include "fs.h"
#include "memory.h"

#define TERM_MAX_LINES 10
#define TERM_LINE_LEN 56
#define TERM_INPUT_LEN 54

static char g_term_lines[TERM_MAX_LINES][TERM_LINE_LEN + 1];
static uint32_t g_term_line_count = 0;
static char g_term_input[TERM_INPUT_LEN + 1];
static uint32_t g_term_input_len = 0;

static int str_eq(const char* a, const char* b)
{
    uint32_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        ++i;
    }
    return a[i] == b[i];
}

static int str_starts_with(const char* s, const char* prefix)
{
    uint32_t i = 0;
    while (prefix[i] != '\0') {
        if (s[i] != prefix[i]) {
            return 0;
        }
        ++i;
    }
    return 1;
}

static void str_copy(char* dst, const char* src, uint32_t max)
{
    uint32_t i = 0;
    if (max == 0) return;
    while (i + 1 < max && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void u32_to_dec(uint32_t value, char* out, uint32_t max)
{
    char tmp[11];
    uint32_t len = 0;
    uint32_t i = 0;
    if (max == 0) return;
    if (value == 0) {
        out[0] = '0';
        if (max > 1) out[1] = '\0';
        return;
    }
    while (value > 0 && len < 10) {
        tmp[len++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (i < len && i + 1 < max) {
        out[i] = tmp[len - 1 - i];
        ++i;
    }
    out[i] = '\0';
}

static void term_add_line(const char* line)
{
    if (g_term_line_count < TERM_MAX_LINES) {
        str_copy(g_term_lines[g_term_line_count], line, TERM_LINE_LEN + 1);
        g_term_line_count++;
        return;
    }

    for (uint32_t i = 1; i < TERM_MAX_LINES; ++i) {
        str_copy(g_term_lines[i - 1], g_term_lines[i], TERM_LINE_LEN + 1);
    }
    str_copy(g_term_lines[TERM_MAX_LINES - 1], line, TERM_LINE_LEN + 1);
}

static void terminal_execute(void)
{
    char line[TERM_LINE_LEN + 1];
    const char* file_data;

    line[0] = '>';
    line[1] = ' ';
    str_copy(&line[2], g_term_input, TERM_LINE_LEN - 1);
    term_add_line(line);

    if (str_eq(g_term_input, "help")) {
        term_add_line("help ls cat /home/* mem clear echo ...");
    } else if (str_eq(g_term_input, "ls")) {
        for (uint32_t i = 0; i < fs_count(); ++i) {
            const char* p = fs_path_at(i);
            if (p) term_add_line(p);
        }
    } else if (str_eq(g_term_input, "cat /home/readme.txt")) {
        file_data = fs_read("/home/readme.txt");
        term_add_line(file_data ? file_data : "missing");
    } else if (str_eq(g_term_input, "cat /home/notes.txt")) {
        file_data = fs_read("/home/notes.txt");
        term_add_line((file_data && file_data[0] != '\0') ? file_data : "(notes file is empty)");
    } else if (str_eq(g_term_input, "mem")) {
        char used[12], free_mem[12];
        u32_to_dec(memory_used(), used, sizeof(used));
        u32_to_dec(memory_free(), free_mem, sizeof(free_mem));
        term_add_line("memory bytes used/free:");
        term_add_line(used);
        term_add_line(free_mem);
    } else if (str_starts_with(g_term_input, "echo ")) {
        term_add_line(&g_term_input[5]);
    } else if (str_eq(g_term_input, "clear")) {
        g_term_line_count = 0;
    } else if (g_term_input_len > 0) {
        term_add_line("unknown command");
    }

    g_term_input_len = 0;
    g_term_input[0] = '\0';
}

void terminal_init(void)
{
    g_term_line_count = 0;
    g_term_input_len = 0;
    g_term_input[0] = '\0';
    term_add_line("SodaOS terminal ready. Type help.");
}

void terminal_render(void)
{
    graphics_clear_screen(0x10);
    graphics_draw_topbar("Terminal", "v0.4");
    graphics_draw_panel(3, 4, 20, 72, 0x1B, 0x07);

    for (uint32_t i = 0; i < g_term_line_count; ++i) {
        graphics_put_string_clipped(g_term_lines[i], 5 + i, 7, TERM_LINE_LEN, 0x0F);
    }

    graphics_put_string_at(">", 17, 7, 0x0F);
    graphics_put_string_clipped(g_term_input, 17, 9, TERM_INPUT_LEN, 0x0F);
    graphics_put_string_at("Commands: help ls cat /home/notes.txt mem clear", 20, 7, 0x08);
    graphics_put_string_at("ESC to desktop", 21, 7, 0x08);
    graphics_present();
}

void terminal_handle_key(uint8_t scancode)
{
    if (scancode == 0x1C) {
        terminal_execute();
        terminal_render();
    } else if (scancode == 0x0E) {
        if (g_term_input_len > 0) {
            g_term_input_len--;
            g_term_input[g_term_input_len] = '\0';
            terminal_render();
        }
    } else {
        char ch = keyboard_scancode_to_ascii(scancode);
        if (ch != 0 && g_term_input_len < TERM_INPUT_LEN) {
            g_term_input[g_term_input_len++] = ch;
            g_term_input[g_term_input_len] = '\0';
            terminal_render();
        }
    }
}
