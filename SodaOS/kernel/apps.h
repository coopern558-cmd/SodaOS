#ifndef APPS_H
#define APPS_H

#include <stdint.h>

void apps_init(void);
void apps_render_boot(void);
void apps_render_desktop(void);
void apps_handle_key(uint8_t scancode, uint8_t extended);
void apps_handle_mouse(int32_t x, int32_t y, uint8_t buttons);

#endif
