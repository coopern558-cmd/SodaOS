#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
    uint8_t buttons;
    uint8_t changed;
} mouse_state_t;

void mouse_init(int32_t screen_w, int32_t screen_h);
void mouse_poll(mouse_state_t* out_state);

#endif