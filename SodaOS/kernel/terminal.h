#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

void terminal_init(void);
void terminal_render(void);
void terminal_handle_key(uint8_t scancode);

#endif