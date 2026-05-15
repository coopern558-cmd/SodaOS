#include "rtc.h"

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

static uint8_t bcd_to_bin(uint8_t v)
{
    return (uint8_t)((v & 0x0F) + ((v >> 4) * 10));
}

void rtc_read_time(uint8_t* hour, uint8_t* minute)
{
    uint8_t h;
    uint8_t m;
    uint8_t reg_b;

    while (1) {
        outb(0x70, 0x0A);
        if ((inb(0x71) & 0x80) == 0) {
            break;
        }
    }

    outb(0x70, 0x04);
    h = inb(0x71);
    outb(0x70, 0x02);
    m = inb(0x71);
    outb(0x70, 0x0B);
    reg_b = inb(0x71);

    if ((reg_b & 0x04) == 0) {
        h = bcd_to_bin(h);
        m = bcd_to_bin(m);
    }

    if ((reg_b & 0x02) == 0 && (h & 0x80)) {
        h = (uint8_t)(((h & 0x7F) + 12) % 24);
    }

    *hour = h;
    *minute = m;
}