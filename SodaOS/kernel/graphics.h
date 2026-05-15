#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void graphics_init(uint32_t multiboot_magic, uint32_t multiboot_info_addr);
int graphics_is_framebuffer(void);
uint32_t graphics_screen_width(void);
uint32_t graphics_screen_height(void);
void graphics_fill_rect_px(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb);
void graphics_draw_rect_px(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb);
void graphics_draw_cursor(uint32_t x, uint32_t y, uint32_t rgb);
void graphics_clear_screen(uint8_t color);
void graphics_draw_box(uint32_t row, uint32_t col, uint32_t h, uint32_t w, uint8_t color);
void graphics_draw_panel(uint32_t row, uint32_t col, uint32_t h, uint32_t w, uint8_t frame, uint8_t fill);
void graphics_put_string_at(const char* text, uint32_t row, uint32_t col, uint8_t color);
void graphics_put_string_clipped(const char* text, uint32_t row, uint32_t col, uint32_t max_chars, uint8_t color);
void graphics_draw_loading_bar(uint32_t filled, uint32_t total, uint32_t row, uint32_t col, uint8_t color);
void graphics_draw_topbar(const char* title, const char* version);

#endif
