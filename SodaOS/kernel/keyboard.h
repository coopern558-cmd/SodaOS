#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

uint8_t keyboard_read_scancode(void);
char keyboard_scancode_to_ascii(uint8_t sc);

#endif