#include "debug.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_write_char(char c)
{
    while ((inb(0x3FD) & 0x20) == 0) {
    }
    outb(0x3F8, (uint8_t)c);
}

void debug_init(void)
{
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x03);
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);
    outb(0x3FA, 0xC7);
    outb(0x3FC, 0x0B);
}

void debug_log(const char* msg)
{
    for (uint32_t i = 0; msg[i] != '\0'; ++i) {
        if (msg[i] == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(msg[i]);
    }
}

void debug_log_hex32(const char* prefix, uint32_t value)
{
    const char* hex = "0123456789ABCDEF";
    char buf[11];
    for (uint32_t i = 0; prefix[i] != '\0'; ++i) {
        serial_write_char(prefix[i]);
    }
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; ++i) {
        uint32_t shift = (uint32_t)(28 - i * 4);
        buf[2 + i] = hex[(value >> shift) & 0xF];
    }
    buf[10] = '\0';
    debug_log(buf);
    debug_log("\n");
}