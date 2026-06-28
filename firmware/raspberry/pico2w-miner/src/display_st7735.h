#ifndef DISPLAY_ST7735_H
#define DISPLAY_ST7735_H

#include <stdint.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 160

#define DISPLAY_COLOR_BLACK   0x0000
#define DISPLAY_COLOR_WHITE   0xffff
#define DISPLAY_COLOR_RED     0xf800
#define DISPLAY_COLOR_GREEN   0x07e0
#define DISPLAY_COLOR_BLUE    0x001f
#define DISPLAY_COLOR_YELLOW  0xffe0
#define DISPLAY_COLOR_CYAN    0x07ff
#define DISPLAY_COLOR_MAGENTA 0xf81f

void display_init(void);

void display_fill(uint16_t color);

void display_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

void display_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale);

void display_set_brightness_percent(uint8_t percent);


#endif