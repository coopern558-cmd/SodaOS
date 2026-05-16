#include "apps.h"
#include "graphics.h"
#include "terminal.h"
#include "memory.h"
#include "fs.h"
#include "keyboard.h"

#define APP_COUNT 4
#define NOTES_MAX_LEN 480
#define WM_WINDOW_COUNT 3

typedef struct {
    const char* label;
    const char* status;
    void (*render)(void);
} App;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    const char* title;
    const char* body;
    uint8_t visible;
    uint8_t minimized;
} WmWindow;

typedef enum {
    UI_MODE_LOGIN = 0,
    UI_MODE_DESKTOP = 1
} UiMode;

static uint8_t g_selected = 0;
static uint8_t g_in_app = 0;
static uint8_t g_active_app = 0;
static UiMode g_mode = UI_MODE_LOGIN;

static char g_notes[NOTES_MAX_LEN + 1] = "";
static uint32_t g_notes_len = 0;
static char g_username[24] = "guest";
static uint8_t g_username_len = 5;

static int32_t g_mouse_x = 0;
static int32_t g_mouse_y = 0;
static uint8_t g_mouse_buttons = 0;
static uint8_t g_last_mouse_buttons = 0;
static uint8_t g_mouse_redraw_div = 0;
static int32_t g_last_render_mouse_x = 0;
static int32_t g_last_render_mouse_y = 0;
static int32_t g_drag_window = -1;
static int32_t g_drag_last_x = 0;
static int32_t g_drag_last_y = 0;
static int32_t g_taskbar_hover = -1;
static WmWindow g_windows[WM_WINDOW_COUNT];
static uint8_t g_wm_focus = 0;

static void app_render_files(void);
static void app_render_notes(void);
static void app_render_settings(void);
static void wm_init(void);
static void wm_focus_next(void);
static void wm_move_focused(int32_t dx, int32_t dy);
static void wm_draw_window(const WmWindow* win, uint8_t focused);
static void wm_draw_taskbar(void);
static void wm_toggle_minimize(uint8_t index);
static int32_t wm_taskbar_hit_test(int32_t x, int32_t y);
static int32_t wm_hit_test(int32_t x, int32_t y);
static void wm_focus_index(uint8_t index);
static void render_wallpaper(void);
static void render_login_screen(void);
static void render_desktop_shell(void);

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

static void wm_init(void)
{
    g_windows[0].x = 290; g_windows[0].y = 80;  g_windows[0].w = 280; g_windows[0].h = 160;
    g_windows[0].title = "FILES WINDOW";
    g_windows[0].body = "RAMFS PREVIEW";
    g_windows[0].visible = 1;
    g_windows[0].minimized = 0;

    g_windows[1].x = 430; g_windows[1].y = 180; g_windows[1].w = 290; g_windows[1].h = 170;
    g_windows[1].title = "NOTES WINDOW";
    g_windows[1].body = "QUICK EDIT PREVIEW";
    g_windows[1].visible = 1;
    g_windows[1].minimized = 0;

    g_windows[2].x = 180; g_windows[2].y = 250; g_windows[2].w = 260; g_windows[2].h = 150;
    g_windows[2].title = "SETTINGS WINDOW";
    g_windows[2].body = "SYSTEM STATUS";
    g_windows[2].visible = 1;
    g_windows[2].minimized = 0;

    g_wm_focus = 0;
}

static void wm_focus_next(void)
{
    for (uint8_t i = 1; i <= WM_WINDOW_COUNT; ++i) {
        uint8_t idx = (uint8_t)((g_wm_focus + i) % WM_WINDOW_COUNT);
        if (g_windows[idx].visible && !g_windows[idx].minimized) {
            g_wm_focus = idx;
            return;
        }
    }
}

static void wm_focus_index(uint8_t index)
{
    if (index < WM_WINDOW_COUNT && g_windows[index].visible) {
        if (g_windows[index].minimized) {
            g_windows[index].minimized = 0;
        }
        g_wm_focus = index;
    }
}

static int32_t wm_hit_test(int32_t x, int32_t y)
{
    for (int32_t i = WM_WINDOW_COUNT - 1; i >= 0; --i) {
        const WmWindow* w = &g_windows[i];
        if (!w->visible || w->minimized) {
            continue;
        }
        if (x >= w->x && x < (w->x + w->w) && y >= w->y && y < (w->y + w->h)) {
            return i;
        }
    }
    return -1;
}

static int32_t wm_taskbar_hit_test(int32_t x, int32_t y)
{
    // Taskbar is rendered at text row 22 with height of ~3 text rows.
    // Convert that to pixel-space so hit testing stays correct across resolutions.
    int32_t taskbar_top = 22 * 16;
    int32_t taskbar_bottom = taskbar_top + (3 * 16);

    if (y < taskbar_top || y >= taskbar_bottom) {
        return -1;
    }
    if (x >= 90 && x < 190) return 0;
    if (x >= 200 && x < 300) return 1;
    if (x >= 310 && x < 440) return 2;
    return -1;
}

static void wm_move_focused(int32_t dx, int32_t dy)
{
    WmWindow* win = &g_windows[g_wm_focus];
    int32_t screen_w = (int32_t)graphics_screen_width();
    int32_t screen_h = (int32_t)graphics_screen_height();

    if (!win->visible) {
        return;
    }
    if (win->minimized) {
        return;
    }

    win->x += dx;
    win->y += dy;

    if (win->x < 150) win->x = 150;
    if (win->y < 60) win->y = 60;
    if (win->x + win->w > screen_w - 20) win->x = screen_w - 20 - win->w;
    if (win->y + win->h > screen_h - 70) win->y = screen_h - 70 - win->h;
}

static void wm_move_index(uint8_t index, int32_t dx, int32_t dy)
{
    if (index >= WM_WINDOW_COUNT) {
        return;
    }

    g_wm_focus = index;
    wm_move_focused(dx, dy);
}

static void wm_draw_window(const WmWindow* win, uint8_t focused)
{
    uint32_t frame = focused ? 0x7BE8FF : 0x476284;
    uint32_t body  = focused ? 0x1C3554 : 0x162B45;
    uint32_t title = focused ? 0x2783C9 : 0x234F7D;
    uint32_t title_text = focused ? 0x1F : 0x17;
    uint32_t btn_close = focused ? 0xE03A3A : 0x7E2C2C;
    uint32_t btn_min = focused ? 0xD7A600 : 0x7A6920;

    graphics_fill_rect_px((uint32_t)win->x, (uint32_t)win->y, (uint32_t)win->w, (uint32_t)win->h, body);
    graphics_draw_rect_px((uint32_t)win->x, (uint32_t)win->y, (uint32_t)win->w, (uint32_t)win->h, frame);
    graphics_fill_rect_px((uint32_t)win->x, (uint32_t)win->y, (uint32_t)win->w, 22, title);
    graphics_fill_rect_px((uint32_t)(win->x + win->w - 30), (uint32_t)(win->y + 5), 9, 9, btn_min);
    graphics_fill_rect_px((uint32_t)(win->x + win->w - 17), (uint32_t)(win->y + 5), 9, 9, btn_close);
    graphics_put_string_at(win->title, (uint32_t)(win->y / 16 + 1), (uint32_t)(win->x / 8 + 1), (uint8_t)title_text);
    graphics_put_string_at(win->body, (uint32_t)(win->y / 16 + 3), (uint32_t)(win->x / 8 + 1), 0x1F);
}

static void wm_draw_taskbar(void)
{
    graphics_draw_box(22, 0, 3, VGA_WIDTH, 0x18);
    graphics_put_string_at(" TASKBAR ", 22, 1, 0x1E);
    graphics_put_string_at("[F1] FILES", 22, 12, g_wm_focus == 0 ? 0x70 : (g_taskbar_hover == 0 ? 0x3F : 0x1F));
    graphics_put_string_at("[F2] NOTES", 22, 26, g_wm_focus == 1 ? 0x70 : (g_taskbar_hover == 1 ? 0x3F : 0x1F));
    graphics_put_string_at("[F3] SETTINGS", 22, 40, g_wm_focus == 2 ? 0x70 : (g_taskbar_hover == 2 ? 0x3F : 0x1F));
    graphics_put_string_at("TAB NEXT  WASD MOVE  ENTER OPEN", 22, 52, 0x1F);
}

static void wm_toggle_minimize(uint8_t index)
{
    if (index >= WM_WINDOW_COUNT || !g_windows[index].visible) {
        return;
    }
    g_windows[index].minimized = g_windows[index].minimized ? 0 : 1;

    if (g_windows[index].minimized && g_wm_focus == index) {
        wm_focus_next();
    } else if (!g_windows[index].minimized) {
        g_wm_focus = index;
    }
}

static void render_wallpaper(void)
{
    uint32_t w = graphics_screen_width();
    uint32_t h = graphics_screen_height();

    graphics_fill_rect_px(0, 0, w, h, 0x0B1A2E);
    graphics_fill_rect_px(0, h / 2, w, h / 2, 0x081421);
    graphics_fill_rect_px(40, 70, 220, 220, 0x10365A);
    graphics_fill_rect_px(90, 120, 220, 220, 0x154872);
    graphics_fill_rect_px(w - 350, 120, 260, 210, 0x17314A);
    graphics_fill_rect_px(w - 300, 160, 220, 170, 0x1E486C);
}

static void render_desktop_shell(void)
{
    graphics_clear_screen(0x10);
    if (graphics_is_framebuffer()) {
        render_wallpaper();
    }
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
        graphics_put_string_at("WINDOWS V1 ACTIVE", 13, 35, 0x1E);
        graphics_put_string_at("UP DOWN PICK APP", 15, 35, 0x17);
        graphics_put_string_at("ENTER OPEN  ESC BACK", 16, 35, 0x17);
        graphics_put_string_at("TAB/WASD WINDOW CTRL", 18, 35, 0x17);
        graphics_put_string_at("F1..F3 OR ENTER ON TASKBAR", 19, 35, 0x17);

        for (uint8_t i = 0; i < WM_WINDOW_COUNT; ++i) {
            if (g_windows[i].visible) {
                wm_draw_window(&g_windows[i], i == g_wm_focus);
            }
        }
        wm_draw_taskbar();
    } else {
        graphics_draw_panel(3, 32, 18, 46, 0x1B, 0x17);
        graphics_put_string_at(" SODAOS WORKSPACE ", 3, 35, 0x1E);
        graphics_put_string_at("SELECTED:", 6, 35, 0x1F);
        graphics_put_string_at(g_apps[g_selected].label, 6, 46, 0x17);
        graphics_put_string_at(g_apps[g_selected].status, 9, 35, 0x17);
        graphics_put_string_at("UP DOWN ENTER  ESC BACK", 15, 35, 0x17);
    }

    if (!graphics_is_framebuffer()) {
        graphics_draw_box(23, 0, 2, VGA_WIDTH, 0x1F);
        graphics_put_string_at(" START [SODA]   TERMINAL   FILES   NOTES   SETTINGS ", 23, 1, 0x1F);
    } else {
        graphics_draw_cursor((uint32_t)g_mouse_x, (uint32_t)g_mouse_y, 0xFFFFFF);
    }
    graphics_present();
}

static void render_login_screen(void)
{
    graphics_clear_screen(0x10);
    if (graphics_is_framebuffer()) {
        uint32_t w = graphics_screen_width();
        uint32_t h = graphics_screen_height();
        graphics_fill_rect_px(0, 0, w, h, 0x081A2C);
        graphics_fill_rect_px(0, h / 2, w, h / 2, 0x051320);
        graphics_fill_rect_px((w / 2) - 210, (h / 2) - 150, 420, 300, 0x0F2E4A);
        graphics_draw_rect_px((w / 2) - 210, (h / 2) - 150, 420, 300, 0x6BDFFF);
    }
    graphics_draw_topbar("SODAOS LOGIN", "V0.7");
    graphics_draw_panel(5, 18, 14, 44, 0x1B, 0x17);
    graphics_put_string_at("WELCOME TO SODAOS", 8, 27, 0x1E);
    graphics_put_string_at("USERNAME:", 11, 24, 0x1F);
    graphics_draw_panel(12, 24, 3, 30, 0x1B, 0x10);
    graphics_put_string_at(g_username, 13, 26, 0x1F);
    graphics_put_string_at("TYPE NAME, ENTER TO START", 16, 24, 0x17);
    graphics_put_string_at("BACKSPACE TO EDIT", 17, 24, 0x17);
    graphics_present();
}

void apps_render_boot(void)
{
    graphics_clear_screen(0x10);
    if (graphics_is_framebuffer()) {
        uint32_t w = graphics_screen_width();
        uint32_t h = graphics_screen_height();
        graphics_fill_rect_px(0, 0, w, h, 0x081A2C);
        graphics_fill_rect_px(0, h / 2, w, h / 2, 0x061321);
        graphics_fill_rect_px((w / 2) - 180, (h / 2) - 120, 360, 240, 0x0F2E4A);
        graphics_draw_rect_px((w / 2) - 180, (h / 2) - 120, 360, 240, 0x7BE8FF);
    }
    graphics_draw_panel(6, 16, 13, 48, 0x1E, 0x17);
    graphics_put_string_at("SodaOS", 9, 36, 0x1E);
    graphics_put_string_at("STARTING MODERN SHELL", 12, 29, 0x1F);
    for (uint32_t i = 0; i <= 30; ++i) {
        graphics_draw_loading_bar(i, 30, 14, 24, 0x1B);
        graphics_present();
        busy_wait(3500000);
    }
    graphics_present();
}

void apps_render_desktop(void)
{
    if (g_mode == UI_MODE_LOGIN) {
        render_login_screen();
        return;
    }
    render_desktop_shell();
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
    graphics_present();
}

static void app_render_notes(void)
{
    graphics_clear_screen(0x10);
    graphics_draw_topbar("NOTES", "V0.6");
    graphics_draw_panel(3, 4, 20, 72, 0x1B, 0x17);
    graphics_put_string_at("TYPE TO EDIT NOTES STORED IN /HOME/NOTES.TXT", 5, 7, 0x1E);
    graphics_put_string_clipped(g_notes, 8, 7, 66, 0x17);
    graphics_put_string_at("ENTER SPACE  BACKSPACE DELETE  ESC DESKTOP", 20, 7, 0x1F);
    graphics_present();
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
    graphics_present();
}

void apps_init(void)
{
    const char* note_data;
    g_selected = 0;
    g_in_app = 0;
    g_active_app = 0;
    g_mode = UI_MODE_LOGIN;
    g_notes_len = 0;
    g_notes[0] = '\0';

    g_mouse_x = (int32_t)(graphics_screen_width() / 2);
    g_mouse_y = (int32_t)(graphics_screen_height() / 2);
    g_last_render_mouse_x = g_mouse_x;
    g_last_render_mouse_y = g_mouse_y;
    g_drag_window = -1;
    g_taskbar_hover = -1;
    wm_init();

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
    if (g_mode == UI_MODE_LOGIN) {
        if (scancode == 0x1C) {
            g_mode = UI_MODE_DESKTOP;
            apps_render_desktop();
            return;
        }
        if (scancode == 0x0E) {
            if (g_username_len > 0) {
                g_username_len--;
                g_username[g_username_len] = '\0';
                render_login_screen();
            }
            return;
        }
        if ((uint32_t)(g_username_len + 1U) < (uint32_t)sizeof(g_username)) {
            char ch = keyboard_scancode_to_ascii(scancode);
            if (ch != 0) {
                g_username[g_username_len++] = ch;
                g_username[g_username_len] = '\0';
                render_login_screen();
            }
        }
        return;
    }

    if (!g_in_app) {
        if (scancode == 0x0F) { // Tab
            wm_focus_next();
            render_desktop_shell();
        } else if (scancode == 0x1E) { // A
            wm_move_focused(-12, 0);
            render_desktop_shell();
        } else if (scancode == 0x20) { // D
            wm_move_focused(12, 0);
            render_desktop_shell();
        } else if (scancode == 0x11) { // W
            wm_move_focused(0, -12);
            render_desktop_shell();
        } else if (scancode == 0x1F) { // S
            wm_move_focused(0, 12);
            render_desktop_shell();
        } else if (scancode == 0x3B) { // F1
            g_wm_focus = 0;
            g_selected = 1;
            render_desktop_shell();
        } else if (scancode == 0x3C) { // F2
            g_wm_focus = 1;
            g_selected = 2;
            render_desktop_shell();
        } else if (scancode == 0x3D) { // F3
            g_wm_focus = 2;
            g_selected = 3;
            render_desktop_shell();
        } else if (scancode == 0x32) { // M
            wm_toggle_minimize(g_wm_focus);
            render_desktop_shell();
        } else if (extended && scancode == 0x48 && g_selected > 0) {
            g_selected--;
            render_desktop_shell();
        } else if (extended && scancode == 0x50 && g_selected + 1 < APP_COUNT) {
            g_selected++;
            render_desktop_shell();
        } else if (scancode == 0x1C) {
            if (g_wm_focus == 0) g_selected = 1;
            else if (g_wm_focus == 1) g_selected = 2;
            else g_selected = 3;
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
        render_desktop_shell();
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

    if (g_mode != UI_MODE_DESKTOP || g_in_app || !graphics_is_framebuffer()) {
        g_last_mouse_buttons = g_mouse_buttons;
        return;
    }

    if ((g_mouse_buttons & 0x1) && !(g_last_mouse_buttons & 0x1)) {
        int32_t hit = wm_hit_test(g_mouse_x, g_mouse_y);
        if (hit >= 0) {
            wm_focus_index((uint8_t)hit);
            if (g_mouse_y < g_windows[hit].y + 22) {
                g_drag_window = hit;
                g_drag_last_x = g_mouse_x;
                g_drag_last_y = g_mouse_y;
                if (g_mouse_x >= g_windows[hit].x + g_windows[hit].w - 30 &&
                    g_mouse_x < g_windows[hit].x + g_windows[hit].w - 21) {
                    wm_toggle_minimize((uint8_t)hit);
                    g_drag_window = -1;
                }
            }
            if (hit == 0) g_selected = 1;
            else if (hit == 1) g_selected = 2;
            else g_selected = 3;
            render_desktop_shell();
        } else {
            int32_t task = wm_taskbar_hit_test(g_mouse_x, g_mouse_y);
            if (task >= 0) {
                wm_focus_index((uint8_t)task);
                if (task == 0) g_selected = 1;
                else if (task == 1) g_selected = 2;
                else g_selected = 3;
                if (g_windows[task].minimized) {
                    g_windows[task].minimized = 0;
                }
                render_desktop_shell();
            }
        }
    }

    if (!(g_mouse_buttons & 0x1)) {
        g_drag_window = -1;
    }

    if (g_drag_window >= 0 && (g_mouse_buttons & 0x1)) {
        dx = g_mouse_x - g_drag_last_x;
        dy = g_mouse_y - g_drag_last_y;
        if (dx != 0 || dy != 0) {
            wm_move_index((uint8_t)g_drag_window, dx, dy);
            g_drag_last_x = g_mouse_x;
            g_drag_last_y = g_mouse_y;
            render_desktop_shell();
        }
        g_last_render_mouse_x = g_mouse_x;
        g_last_render_mouse_y = g_mouse_y;
        g_last_mouse_buttons = g_mouse_buttons;
        return;
    }

    // Repaint cursor/mouse interactions at a controlled rate.
    dx = g_mouse_x - g_last_render_mouse_x;
    if (dx < 0) dx = -dx;
    dy = g_mouse_y - g_last_render_mouse_y;
    if (dy < 0) dy = -dy;

    {
        int32_t hover = wm_taskbar_hit_test(g_mouse_x, g_mouse_y);
        if (hover != g_taskbar_hover) {
            g_taskbar_hover = hover;
            render_desktop_shell();
            g_last_render_mouse_x = g_mouse_x;
            g_last_render_mouse_y = g_mouse_y;
            g_last_mouse_buttons = g_mouse_buttons;
            g_mouse_redraw_div = 0;
            return;
        }
    }

    g_mouse_redraw_div++;
    if (dx >= 2 || dy >= 2 || g_mouse_buttons != g_last_mouse_buttons || g_mouse_redraw_div >= 6) {
        g_mouse_redraw_div = 0;
        render_desktop_shell();
        g_last_render_mouse_x = g_mouse_x;
        g_last_render_mouse_y = g_mouse_y;
    }

    g_last_mouse_buttons = g_mouse_buttons;
}
