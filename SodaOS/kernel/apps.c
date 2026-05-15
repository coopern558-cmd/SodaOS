#include "apps.h"
#include "graphics.h"
#include "terminal.h"
#include "memory.h"
#include "fs.h"
#include "keyboard.h"

#define APP_COUNT 4
#define NOTES_MAX_LEN 480

typedef struct {
    const char* label;
    const char* status;
    void (*render)(void);
} App;

static uint8_t g_selected = 0;
static uint8_t g_in_app = 0;
static uint8_t g_active_app = 0;

static char g_notes[NOTES_MAX_LEN + 1] = "";
static uint32_t g_notes_len = 0;

static int32_t g_mouse_x = 0;
static int32_t g_mouse_y = 0;
static uint8_t g_mouse_buttons = 0;
static int32_t g_last_render_mouse_x = 0;
static int32_t g_last_render_mouse_y = 0;
static uint8_t g_last_render_mouse_buttons = 0;
static uint8_t g_mouse_redraw_div = 0;

static void app_render_files(void);
static void app_render_notes(void);
static void app_render_settings(void);

static App g_apps[APP_COUNT] = {
    {"1  TERMINAL", "TINY COMMAND SHELL FOR SODAOS", terminal_render},
    {"2  FILES", "RAM FILE BROWSER", app_render_files},
    {"3  NOTES", "EDITABLE NOTES IN RAMFS", app_render_notes},
    {"4  SETTINGS", "MEMORY AND CLOCK INFO", app_render_settings}
};

static void busy_wait(uint32_t cycles)
{
    volatile uint32_t i;
    for (i = 0; i < cycles; ++i) {
        __asm__ __volatile__("nop");
    }
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


static void app_render_desktop_selected(void)
{
    graphics_clear_screen(0x10);
    graphics_draw_topbar("DESKTOP", "V0.6");

    graphics_draw_panel(3, 2, 18, 28, 0x1B, 0x17);
    graphics_put_string_at(" APPS ", 3, 4, 0x1E);
    for (uint8_t i = 0; i < APP_COUNT; ++i) {
        graphics_put_string_at(g_apps[i].label, 6 + i * 3, 5, (i == g_selected) ? 0x70 : 0x17);
    }

    if (graphics_is_framebuffer()) {
        graphics_draw_panel(3, 32, 18, 46, 0x1B, 0x17);
        graphics_put_string_at(" SODAOS HOME ", 3, 35, 0x1E);
        graphics_put_string_at("SELECTED APP", 6, 35, 0x1F);
        graphics_put_string_at(g_apps[g_selected].label, 8, 35, 0x1F);
        graphics_put_string_at(g_apps[g_selected].status, 10, 35, 0x17);
        graphics_put_string_at("KEYBOARD", 13, 35, 0x1E);
        graphics_put_string_at("UP DOWN TO PICK APP", 15, 35, 0x17);
        graphics_put_string_at("ENTER TO OPEN  ESC BACK", 16, 35, 0x17);
        graphics_put_string_at("MOUSE CURSOR ENABLED", 18, 35, 0x17);
        graphics_draw_cursor((uint32_t)g_mouse_x, (uint32_t)g_mouse_y, 0xFFFFFF);
    } else {
        graphics_draw_panel(3, 32, 18, 46, 0x1B, 0x17);
        graphics_put_string_at(" SODAOS WORKSPACE ", 3, 35, 0x1E);
        graphics_put_string_at("SELECTED:", 6, 35, 0x1F);
        graphics_put_string_at(g_apps[g_selected].label, 6, 46, 0x17);
        graphics_put_string_at(g_apps[g_selected].status, 9, 35, 0x17);
        graphics_put_string_at("UP DOWN ENTER  ESC BACK", 15, 35, 0x17);
    }

    graphics_draw_box(23, 0, 2, VGA_WIDTH, 0x1F);
    graphics_put_string_at(" START [SODA]   TERMINAL   FILES   NOTES   SETTINGS ", 23, 1, 0x1F);
}

void apps_render_boot(void)
{
    graphics_clear_screen(0x10);
    graphics_draw_panel(6, 16, 13, 48, 0x1F, 0x1F);
    graphics_put_string_at("SodaOS", 9, 36, 0x1B);
    graphics_put_string_at("MODERN SHELL IS STARTING", 12, 27, 0x1F);
    for (uint32_t i = 0; i <= 30; ++i) {
        graphics_draw_loading_bar(i, 30, 14, 24, 0x3F);
        busy_wait(3500000);
    }
}

void apps_render_desktop(void)
{
    app_render_desktop_selected();
}

static void app_render_files(void)
{
    graphics_clear_screen(0x10);
    graphics_draw_topbar("FILES", "V0.6");
    graphics_draw_panel(3, 8, 18, 64, 0x1B, 0x17);
    graphics_put_string_at("RAM FILESYSTEM", 5, 11, 0x1E);
    for (uint32_t i = 0; i < fs_count(); ++i) {
        const char* p = fs_path_at(i);
        if (p) graphics_put_string_at(p, 8 + i, 11, 0x17);
    }
    graphics_put_string_at("TIP USE TERMINAL CAT /HOME/NOTES.TXT", 12, 11, 0x17);
    graphics_put_string_at("ESC TO DESKTOP", 19, 11, 0x1F);
}

static void app_render_notes(void)
{
    graphics_clear_screen(0x10);
    graphics_draw_topbar("NOTES", "V0.6");
    graphics_draw_panel(3, 4, 20, 72, 0x1B, 0x17);
    graphics_put_string_at("TYPE TO EDIT NOTES STORED IN /HOME/NOTES.TXT", 5, 7, 0x1E);
    graphics_put_string_clipped(g_notes, 8, 7, 66, 0x17);
    graphics_put_string_at("ENTER SPACE  BACKSPACE DELETE  ESC DESKTOP", 20, 7, 0x1F);
}

static void app_render_settings(void)
{
    char used[12], free_mem[12], allocs[12];
    u32_to_dec(memory_used(), used, sizeof(used));
    u32_to_dec(memory_free(), free_mem, sizeof(free_mem));
    u32_to_dec(memory_alloc_count(), allocs, sizeof(allocs));

    graphics_clear_screen(0x10);
    graphics_draw_topbar("SETTINGS", "V0.6");
    graphics_draw_panel(3, 10, 16, 60, 0x1B, 0x17);
    graphics_put_string_at("THEME PROFILE OPTIONS COMING NEXT", 7, 14, 0x17);
    graphics_put_string_at("HEAP USED BYTES:", 10, 14, 0x17);
    graphics_put_string_at(used, 10, 32, 0x17);
    graphics_put_string_at("HEAP FREE BYTES:", 11, 14, 0x17);
    graphics_put_string_at(free_mem, 11, 32, 0x17);
    graphics_put_string_at("ALLOC COUNT:", 12, 14, 0x17);
    graphics_put_string_at(allocs, 12, 32, 0x17);
    graphics_put_string_at("CLOCK READS FROM RTC CMOS", 14, 14, 0x17);
    graphics_put_string_at("ESC TO DESKTOP", 16, 14, 0x1F);
}

void apps_init(void)
{
    const char* note_data;
    g_selected = 0;
    g_in_app = 0;
    g_active_app = 0;
    g_notes_len = 0;
    g_notes[0] = '\0';

    g_mouse_x = (int32_t)(graphics_screen_width() / 2);
    g_mouse_y = (int32_t)(graphics_screen_height() / 2);
    g_last_render_mouse_x = g_mouse_x;
    g_last_render_mouse_y = g_mouse_y;
    g_last_render_mouse_buttons = 0;
    g_mouse_redraw_div = 0;

    note_data = fs_read("/home/notes.txt");
    if (note_data) {
        while (note_data[g_notes_len] != '\0' && g_notes_len < NOTES_MAX_LEN) {
            g_notes[g_notes_len] = note_data[g_notes_len];
            g_notes_len++;
        }
        g_notes[g_notes_len] = '\0';
    }

}

void apps_handle_key(uint8_t scancode, uint8_t extended)
{
    if (!g_in_app) {
        if (extended && scancode == 0x48 && g_selected > 0) {
            g_selected--;
            app_render_desktop_selected();
        } else if (extended && scancode == 0x50 && g_selected + 1 < APP_COUNT) {
            g_selected++;
            app_render_desktop_selected();
        } else if (scancode == 0x1C) {
            g_active_app = g_selected;
            g_in_app = 1;
            g_apps[g_active_app].render();
        } else if (scancode >= 0x02 && scancode <= 0x05) {
            g_active_app = (uint8_t)(scancode - 0x02);
            g_selected = g_active_app;
            g_in_app = 1;
            g_apps[g_active_app].render();
        }
        return;
    }

    if (scancode == 0x01) {
        g_in_app = 0;
        app_render_desktop_selected();
        return;
    }

    if (g_active_app == 0) {
        terminal_handle_key(scancode);
    } else if (g_active_app == 2) {
        if (scancode == 0x0E) {
            if (g_notes_len > 0) {
                g_notes_len--;
                g_notes[g_notes_len] = '\0';
            }
        } else if (scancode == 0x1C) {
            if (g_notes_len < NOTES_MAX_LEN) {
                g_notes[g_notes_len++] = ' ';
                g_notes[g_notes_len] = '\0';
            }
        } else {
            char ch = keyboard_scancode_to_ascii(scancode);
            if (ch != 0 && g_notes_len < NOTES_MAX_LEN) {
                g_notes[g_notes_len++] = ch;
                g_notes[g_notes_len] = '\0';
            }
        }
        fs_write("/home/notes.txt", g_notes);
        app_render_notes();
    }
}

void apps_handle_mouse(int32_t x, int32_t y, uint8_t buttons)
{
    int32_t dx;
    int32_t dy;
    g_mouse_x = x;
    g_mouse_y = y;
    g_mouse_buttons = buttons;

    dx = g_mouse_x - g_last_render_mouse_x;
    dy = g_mouse_y - g_last_render_mouse_y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (!g_in_app && graphics_is_framebuffer()) {
        // Throttle repaints to reduce cursor flicker from full-screen redraws.
        g_mouse_redraw_div++;
        if ((dx >= 2 || dy >= 2 || buttons != g_last_render_mouse_buttons) && g_mouse_redraw_div >= 2) {
            app_render_desktop_selected();
            g_last_render_mouse_x = g_mouse_x;
            g_last_render_mouse_y = g_mouse_y;
            g_last_render_mouse_buttons = buttons;
            g_mouse_redraw_div = 0;
        }
    }
}
