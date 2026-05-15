#include "mouse.h"

#define MOUSE_INVERT_X 1
#define MOUSE_INVERT_Y 0

static int32_t g_w = 640;
static int32_t g_h = 480;
static int32_t g_x = 320;
static int32_t g_y = 240;
static uint8_t g_buttons = 0;
static uint8_t g_cycle = 0;
static uint8_t g_packet[3];
static uint8_t g_ready = 0;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void mouse_wait_write(void)
{
    for (uint32_t i = 0; i < 100000; ++i) {
        if ((inb(0x64) & 0x02) == 0) {
            return;
        }
    }
}

static void mouse_wait_read(void)
{
    for (uint32_t i = 0; i < 100000; ++i) {
        if (inb(0x64) & 0x01) {
            return;
        }
    }
}

static void mouse_write(uint8_t data)
{
    mouse_wait_write();
    outb(0x64, 0xD4);
    mouse_wait_write();
    outb(0x60, data);
}

static uint8_t mouse_read(void)
{
    mouse_wait_read();
    return inb(0x60);
}

void mouse_init(int32_t screen_w, int32_t screen_h)
{
    uint8_t status;

    g_w = screen_w;
    g_h = screen_h;
    g_x = screen_w / 2;
    g_y = screen_h / 2;
    g_buttons = 0;
    g_cycle = 0;
    g_ready = 0;

    mouse_wait_write();
    outb(0x64, 0xA8);

    mouse_wait_write();
    outb(0x64, 0x20);
    mouse_wait_read();
    status = inb(0x60);
    status |= 0x02;

    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, status);

    mouse_write(0xF6);
    (void)mouse_read();

    mouse_write(0xF4);
    (void)mouse_read();

    g_ready = 1;
}

void mouse_poll(mouse_state_t* out_state)
{
    uint8_t status;
    uint8_t any_change = 0;
    out_state->changed = 0;
    out_state->dx = 0;
    out_state->dy = 0;
    out_state->x = g_x;
    out_state->y = g_y;
    out_state->buttons = g_buttons;

    if (!g_ready) {
        return;
    }

    // Drain available AUX (mouse) bytes so movement cannot backlog/desync packets.
    for (uint32_t n = 0; n < 64; ++n) {
        int32_t dx;
        int32_t dy;
        uint8_t buttons;

        status = inb(0x64);
        if ((status & 0x01) == 0 || (status & 0x20) == 0) {
            break;
        }

        g_packet[g_cycle] = inb(0x60);

        // First byte of PS/2 packet must have bit 3 set.
        if (g_cycle == 0 && (g_packet[0] & 0x08) == 0) {
            continue;
        }

        g_cycle++;
        if (g_cycle < 3) {
            continue;
        }
        g_cycle = 0;

        // Discard overflow packets that can produce wild jumps/glitches.
        if (g_packet[0] & 0xC0) {
            continue;
        }

        dx = (int8_t)g_packet[1];
        dy = (int8_t)g_packet[2];
        buttons = g_packet[0] & 0x07;

        // Clamp large deltas to keep cursor motion stable when packets get noisy.
        if (dx > 20) dx = 20;
        if (dx < -20) dx = -20;
        if (dy > 20) dy = 20;
        if (dy < -20) dy = -20;

        if (MOUSE_INVERT_X) {
            g_x -= dx;
        } else {
            g_x += dx;
        }

        if (MOUSE_INVERT_Y) {
            g_y += dy;
        } else {
            g_y -= dy;
        }

        if (g_x < 0) g_x = 0;
        if (g_y < 0) g_y = 0;
        if (g_x >= g_w) g_x = g_w - 1;
        if (g_y >= g_h) g_y = g_h - 1;

        if (dx != 0 || dy != 0 || buttons != g_buttons) {
            g_buttons = buttons;
            out_state->dx = dx;
            out_state->dy = dy;
            out_state->x = g_x;
            out_state->y = g_y;
            out_state->buttons = g_buttons;
            any_change = 1;
        }
    }

    out_state->changed = any_change;
}
