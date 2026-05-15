#include "graphics.h"
#include "rtc.h"

#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

typedef struct __attribute__((packed)) {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} multiboot_info_t;

static int g_fb_enabled = 0;
static uint8_t* g_fb = (uint8_t*)0;
static uint32_t g_fb_width = 0;
static uint32_t g_fb_height = 0;
static uint32_t g_fb_pitch = 0;
static uint8_t g_fb_bpp = 0;

static const uint32_t g_palette[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

static inline uint16_t vga_entry(char c, uint8_t color)
{
    return ((uint16_t)color << 8) | (uint8_t)c;
}

static void fb_put_pixel(uint32_t x, uint32_t y, uint32_t rgb)
{
    if (!g_fb_enabled || x >= g_fb_width || y >= g_fb_height) {
        return;
    }

    if (g_fb_bpp == 32) {
        uint32_t* p = (uint32_t*)(g_fb + y * g_fb_pitch + x * 4);
        *p = rgb;
    } else if (g_fb_bpp == 24) {
        uint8_t* p = g_fb + y * g_fb_pitch + x * 3;
        p[0] = (uint8_t)(rgb & 0xFF);
        p[1] = (uint8_t)((rgb >> 8) & 0xFF);
        p[2] = (uint8_t)((rgb >> 16) & 0xFF);
    }
}

static void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb)
{
    for (uint32_t yy = 0; yy < h; ++yy) {
        for (uint32_t xx = 0; xx < w; ++xx) {
            fb_put_pixel(x + xx, y + yy, rgb);
        }
    }
}

static char font_upcase(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - ('a' - 'A'));
    }
    return c;
}

static uint8_t font5x7_row(char c, uint8_t row)
{
    c = font_upcase(c);
    if (row > 6) {
        return 0;
    }

    switch (c) {
        case 'A': { static const uint8_t r[7]={0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; return r[row]; }
        case 'B': { static const uint8_t r[7]={0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; return r[row]; }
        case 'C': { static const uint8_t r[7]={0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; return r[row]; }
        case 'D': { static const uint8_t r[7]={0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}; return r[row]; }
        case 'E': { static const uint8_t r[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; return r[row]; }
        case 'F': { static const uint8_t r[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; return r[row]; }
        case 'G': { static const uint8_t r[7]={0x0E,0x11,0x10,0x13,0x11,0x11,0x0E}; return r[row]; }
        case 'H': { static const uint8_t r[7]={0x11,0x11,0x11,0x1F,0x11,0x11,0x11}; return r[row]; }
        case 'I': { static const uint8_t r[7]={0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}; return r[row]; }
        case 'J': { static const uint8_t r[7]={0x01,0x01,0x01,0x01,0x11,0x11,0x0E}; return r[row]; }
        case 'K': { static const uint8_t r[7]={0x11,0x12,0x14,0x18,0x14,0x12,0x11}; return r[row]; }
        case 'L': { static const uint8_t r[7]={0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; return r[row]; }
        case 'M': { static const uint8_t r[7]={0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; return r[row]; }
        case 'N': { static const uint8_t r[7]={0x11,0x19,0x15,0x13,0x11,0x11,0x11}; return r[row]; }
        case 'O': { static const uint8_t r[7]={0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; return r[row]; }
        case 'P': { static const uint8_t r[7]={0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; return r[row]; }
        case 'Q': { static const uint8_t r[7]={0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}; return r[row]; }
        case 'R': { static const uint8_t r[7]={0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; return r[row]; }
        case 'S': { static const uint8_t r[7]={0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; return r[row]; }
        case 'T': { static const uint8_t r[7]={0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; return r[row]; }
        case 'U': { static const uint8_t r[7]={0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; return r[row]; }
        case 'V': { static const uint8_t r[7]={0x11,0x11,0x11,0x11,0x11,0x0A,0x04}; return r[row]; }
        case 'W': { static const uint8_t r[7]={0x11,0x11,0x11,0x15,0x15,0x15,0x0A}; return r[row]; }
        case 'X': { static const uint8_t r[7]={0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}; return r[row]; }
        case 'Y': { static const uint8_t r[7]={0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; return r[row]; }
        case 'Z': { static const uint8_t r[7]={0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}; return r[row]; }
        case '0': { static const uint8_t r[7]={0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}; return r[row]; }
        case '1': { static const uint8_t r[7]={0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}; return r[row]; }
        case '2': { static const uint8_t r[7]={0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}; return r[row]; }
        case '3': { static const uint8_t r[7]={0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}; return r[row]; }
        case '4': { static const uint8_t r[7]={0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}; return r[row]; }
        case '5': { static const uint8_t r[7]={0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}; return r[row]; }
        case '6': { static const uint8_t r[7]={0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}; return r[row]; }
        case '7': { static const uint8_t r[7]={0x1F,0x01,0x02,0x04,0x08,0x08,0x08}; return r[row]; }
        case '8': { static const uint8_t r[7]={0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}; return r[row]; }
        case '9': { static const uint8_t r[7]={0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}; return r[row]; }
        case '.': { static const uint8_t r[7]={0,0,0,0,0,0x06,0x06}; return r[row]; }
        case ',': { static const uint8_t r[7]={0,0,0,0,0,0x06,0x04}; return r[row]; }
        case ':': { static const uint8_t r[7]={0,0x06,0x06,0,0x06,0x06,0}; return r[row]; }
        case ';': { static const uint8_t r[7]={0,0x06,0x06,0,0x06,0x04,0}; return r[row]; }
        case '-': { static const uint8_t r[7]={0,0,0,0x1F,0,0,0}; return r[row]; }
        case '/': { static const uint8_t r[7]={0x01,0x02,0x04,0x08,0x10,0,0}; return r[row]; }
        case '\\':{ static const uint8_t r[7]={0x10,0x08,0x04,0x02,0x01,0,0}; return r[row]; }
        case '[': { static const uint8_t r[7]={0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}; return r[row]; }
        case ']': { static const uint8_t r[7]={0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}; return r[row]; }
        case '+': { static const uint8_t r[7]={0,0x04,0x04,0x1F,0x04,0x04,0}; return r[row]; }
        case '=': { static const uint8_t r[7]={0,0x1F,0,0x1F,0,0,0}; return r[row]; }
        case '>': { static const uint8_t r[7]={0x10,0x08,0x04,0x02,0x04,0x08,0x10}; return r[row]; }
        case '<': { static const uint8_t r[7]={0x01,0x02,0x04,0x08,0x04,0x02,0x01}; return r[row]; }
        case '(': { static const uint8_t r[7]={0x02,0x04,0x08,0x08,0x08,0x04,0x02}; return r[row]; }
        case ')': { static const uint8_t r[7]={0x08,0x04,0x02,0x02,0x02,0x04,0x08}; return r[row]; }
        case '\'':{ static const uint8_t r[7]={0x04,0x04,0x02,0,0,0,0}; return r[row]; }
        case '\"':{ static const uint8_t r[7]={0x0A,0x0A,0x05,0,0,0,0}; return r[row]; }
        case '!': { static const uint8_t r[7]={0x04,0x04,0x04,0x04,0x04,0,0x04}; return r[row]; }
        case '?': { static const uint8_t r[7]={0x0E,0x11,0x01,0x02,0x04,0,0x04}; return r[row]; }
        case ' ': return 0x00;
        default:  { static const uint8_t r[7]={0x1F,0x11,0x15,0x15,0x11,0x11,0x1F}; return r[row]; }
    }
}

static void fb_draw_char_block(char c, uint32_t cell_x, uint32_t cell_y, uint32_t fg, uint32_t bg)
{
    uint32_t x0 = cell_x * 8;
    uint32_t y0 = cell_y * 16;

    fb_fill_rect(x0, y0, 8, 16, bg);
    for (uint32_t row = 0; row < 7; ++row) {
        uint8_t bits = font5x7_row(c, (uint8_t)row);
        for (uint32_t col = 0; col < 5; ++col) {
            if ((bits >> (4 - col)) & 1U) {
                uint32_t px = x0 + 1 + col;
                uint32_t py = y0 + 2 + row * 2;
                fb_put_pixel(px, py, fg);
                fb_put_pixel(px, py + 1, fg);
            }
        }
    }
}

void graphics_init(uint32_t multiboot_magic, uint32_t multiboot_info_addr)
{
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        return;
    }

    multiboot_info_t* mbi = (multiboot_info_t*)(uintptr_t)multiboot_info_addr;
    if ((mbi->flags & (1U << 12)) == 0) {
        return;
    }
    if (!(mbi->framebuffer_bpp == 32 || mbi->framebuffer_bpp == 24)) {
        return;
    }

    g_fb = (uint8_t*)(uintptr_t)mbi->framebuffer_addr;
    g_fb_pitch = mbi->framebuffer_pitch;
    g_fb_width = mbi->framebuffer_width;
    g_fb_height = mbi->framebuffer_height;
    g_fb_bpp = mbi->framebuffer_bpp;
    g_fb_enabled = (g_fb != (uint8_t*)0);
}

int graphics_is_framebuffer(void)
{
    return g_fb_enabled;
}

uint32_t graphics_screen_width(void)
{
    return g_fb_enabled ? g_fb_width : (VGA_WIDTH * 8);
}

uint32_t graphics_screen_height(void)
{
    return g_fb_enabled ? g_fb_height : (VGA_HEIGHT * 16);
}

void graphics_fill_rect_px(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb)
{
    if (!g_fb_enabled) {
        return;
    }
    fb_fill_rect(x, y, w, h, rgb);
}

void graphics_draw_rect_px(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb)
{
    if (!g_fb_enabled || w < 2 || h < 2) {
        return;
    }
    fb_fill_rect(x, y, w, 2, rgb);
    fb_fill_rect(x, y + h - 2, w, 2, rgb);
    fb_fill_rect(x, y, 2, h, rgb);
    fb_fill_rect(x + w - 2, y, 2, h, rgb);
}

void graphics_draw_cursor(uint32_t x, uint32_t y, uint32_t rgb)
{
    if (!g_fb_enabled) {
        return;
    }

    for (uint32_t i = 0; i < 12; ++i) {
        fb_put_pixel(x, y + i, rgb);
    }
    for (uint32_t i = 0; i < 8; ++i) {
        fb_put_pixel(x + i, y + 10, rgb);
    }
}

void graphics_clear_screen(uint8_t color)
{
    if (g_fb_enabled) {
        uint32_t bg = g_palette[(color >> 4) & 0x0F];
        fb_fill_rect(0, 0, g_fb_width, g_fb_height, bg);
        return;
    }

    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', color);
        }
    }
}

void graphics_draw_box(uint32_t row, uint32_t col, uint32_t h, uint32_t w, uint8_t color)
{
    if (g_fb_enabled) {
        uint32_t bg = g_palette[(color >> 4) & 0x0F];
        fb_fill_rect(col * 8, row * 16, w * 8, h * 16, bg);
        return;
    }

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            if (row + y < VGA_HEIGHT && col + x < VGA_WIDTH) {
                VGA_MEMORY[(row + y) * VGA_WIDTH + (col + x)] = vga_entry(' ', color);
            }
        }
    }
}

void graphics_draw_panel(uint32_t row, uint32_t col, uint32_t h, uint32_t w, uint8_t frame, uint8_t fill)
{
    if (h < 2 || w < 2) {
        return;
    }

    if (g_fb_enabled) {
        uint32_t fill_rgb = g_palette[(fill >> 4) & 0x0F];
        uint32_t frame_rgb = g_palette[frame & 0x0F];
        uint32_t x = col * 8;
        uint32_t y = row * 16;
        uint32_t ww = w * 8;
        uint32_t hh = h * 16;
        fb_fill_rect(x, y, ww, hh, fill_rgb);
        fb_fill_rect(x, y, ww, 2, frame_rgb);
        fb_fill_rect(x, y + hh - 2, ww, 2, frame_rgb);
        fb_fill_rect(x, y, 2, hh, frame_rgb);
        fb_fill_rect(x + ww - 2, y, 2, hh, frame_rgb);
        return;
    }

    graphics_draw_box(row, col, h, w, fill);
    for (uint32_t x = 0; x < w; ++x) {
        VGA_MEMORY[row * VGA_WIDTH + (col + x)] = vga_entry('-', frame);
        VGA_MEMORY[(row + h - 1) * VGA_WIDTH + (col + x)] = vga_entry('-', frame);
    }
    for (uint32_t y = 0; y < h; ++y) {
        VGA_MEMORY[(row + y) * VGA_WIDTH + col] = vga_entry('|', frame);
        VGA_MEMORY[(row + y) * VGA_WIDTH + (col + w - 1)] = vga_entry('|', frame);
    }

    VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry('+', frame);
    VGA_MEMORY[row * VGA_WIDTH + (col + w - 1)] = vga_entry('+', frame);
    VGA_MEMORY[(row + h - 1) * VGA_WIDTH + col] = vga_entry('+', frame);
    VGA_MEMORY[(row + h - 1) * VGA_WIDTH + (col + w - 1)] = vga_entry('+', frame);
}

void graphics_put_string_at(const char* text, uint32_t row, uint32_t col, uint8_t color)
{
    if (g_fb_enabled) {
        uint32_t fg = g_palette[color & 0x0F];
        uint32_t bg = g_palette[(color >> 4) & 0x0F];
        for (uint32_t i = 0; text[i] != '\0'; ++i) {
            uint32_t x = col + i;
            if (x >= VGA_WIDTH || row >= VGA_HEIGHT) {
                break;
            }
            fb_draw_char_block(text[i], x, row, fg, bg);
        }
        return;
    }

    for (uint32_t i = 0; text[i] != '\0'; ++i) {
        uint32_t x = col + i;
        if (x >= VGA_WIDTH || row >= VGA_HEIGHT) {
            break;
        }
        VGA_MEMORY[row * VGA_WIDTH + x] = vga_entry(text[i], color);
    }
}

void graphics_put_string_clipped(const char* text, uint32_t row, uint32_t col, uint32_t max_chars, uint8_t color)
{
    if (g_fb_enabled) {
        uint32_t fg = g_palette[color & 0x0F];
        uint32_t bg = g_palette[(color >> 4) & 0x0F];
        for (uint32_t i = 0; text[i] != '\0' && i < max_chars; ++i) {
            uint32_t x = col + i;
            if (x >= VGA_WIDTH || row >= VGA_HEIGHT) {
                break;
            }
            fb_draw_char_block(text[i], x, row, fg, bg);
        }
        return;
    }

    for (uint32_t i = 0; text[i] != '\0' && i < max_chars; ++i) {
        uint32_t x = col + i;
        if (x >= VGA_WIDTH || row >= VGA_HEIGHT) {
            break;
        }
        VGA_MEMORY[row * VGA_WIDTH + x] = vga_entry(text[i], color);
    }
}

void graphics_draw_loading_bar(uint32_t filled, uint32_t total, uint32_t row, uint32_t col, uint8_t color)
{
    if (g_fb_enabled) {
        uint32_t fg = g_palette[color & 0x0F];
        uint32_t bg = g_palette[(color >> 4) & 0x0F];
        uint32_t x = col * 8;
        uint32_t y = row * 16;
        uint32_t w = (total + 2) * 8;
        fb_fill_rect(x, y, w, 16, bg);
        fb_fill_rect(x + 2, y + 2, 4, 12, fg);
        fb_fill_rect(x + w - 6, y + 2, 4, 12, fg);
        if (filled > 0) {
            uint32_t fill_px = filled * 8;
            fb_fill_rect(x + 8, y + 4, fill_px, 8, fg);
        }
        return;
    }

    VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry('[', color);
    for (uint32_t i = 0; i < total; ++i) {
        char ch = (i < filled) ? '#' : '-';
        VGA_MEMORY[row * VGA_WIDTH + col + 1 + i] = vga_entry(ch, color);
    }
    VGA_MEMORY[row * VGA_WIDTH + col + 1 + total] = vga_entry(']', color);
}

void graphics_draw_topbar(const char* title, const char* version)
{
    uint8_t h = 0;
    uint8_t m = 0;
    char time_text[6];

    rtc_read_time(&h, &m);
    time_text[0] = (char)('0' + (h / 10));
    time_text[1] = (char)('0' + (h % 10));
    time_text[2] = ':';
    time_text[3] = (char)('0' + (m / 10));
    time_text[4] = (char)('0' + (m % 10));
    time_text[5] = '\0';

    graphics_draw_box(0, 0, 2, VGA_WIDTH, 0x1F);
    graphics_put_string_at(" SodaOS", 0, 1, 0x1E);
    graphics_put_string_at(title, 0, 10, 0x1F);
    graphics_put_string_at(version, 0, 67, 0x1F);
    graphics_put_string_at(time_text, 0, 74, 0x1F);
}
