#include <stdint.h>

#include "apps.h"
#include "keyboard.h"
#include "terminal.h"
#include "memory.h"
#include "fs.h"
#include "graphics.h"
#include "debug.h"
#include "mouse.h"

#define SODAOS_SAFE_BOOT 0

void kmain(uint32_t multiboot_magic, uint32_t multiboot_info_addr)
{
#if SODAOS_SAFE_BOOT
    volatile uint16_t* const vga = (volatile uint16_t*)0xB8000;
    const char* msg = "SodaOS SAFE BOOT OK";
    for (uint32_t i = 0; msg[i] != '\0'; ++i) {
        vga[i] = ((uint16_t)0x1F << 8) | (uint8_t)msg[i];
    }
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#endif

    uint8_t extended = 0;
    uint32_t key_logs = 0;
    mouse_state_t mouse_state;

    debug_init();
    debug_log("[SodaOS] kmain start\n");
    debug_log_hex32("[SodaOS] multiboot magic=", multiboot_magic);
    debug_log_hex32("[SodaOS] multiboot info =", multiboot_info_addr);

    graphics_init(multiboot_magic, multiboot_info_addr);
    debug_log("[SodaOS] graphics init done\n");
    memory_init();
    fs_init();
    terminal_init();
    apps_init();
    mouse_init((int32_t)graphics_screen_width(), (int32_t)graphics_screen_height());
    debug_log("[SodaOS] subsystems init done\n");

    if (kmalloc(256) && kmalloc(512)) {
        // Heap sanity allocations to populate memory stats.
    }

    apps_render_boot();
    apps_render_desktop();
    debug_log("[SodaOS] desktop rendered\n");

    for (;;) {
        mouse_poll(&mouse_state);
        if (mouse_state.changed) {
            apps_handle_mouse(mouse_state.x, mouse_state.y, mouse_state.buttons);
        }

        uint8_t sc = keyboard_read_scancode();

        if (sc == 0) {
            __asm__ __volatile__("nop");
            continue;
        }

        if (sc == 0xE0) {
            extended = 1;
            continue;
        }

        if (sc >= 0x80) {
            extended = 0;
            continue;
        }

        if (key_logs < 16) {
            debug_log("[SodaOS] key event\n");
            key_logs++;
        }
        apps_handle_key(sc, extended);
        extended = 0;
    }
}
