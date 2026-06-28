#include "display_st7735.h"

#include <stddef.h>
#include <stdbool.h>

#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

#include "pico/stdlib.h"

// SPI pin map for the Waveshare 1.8 inch ST7735 display on Pico / Pico 2 W
#define DISPLAY_SPI_PORT spi1


#define DISPLAY_PWM_WRAP 1000

static uint backlight_pwm_slice = 0;
static uint backlight_pwm_channel = 0;
static bool backlight_pwm_ready = false;

#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_CS    9
#define PIN_DC    8
#define PIN_RST  12
#define PIN_BL   13

// Display memory offsets for this ST7735 module
#define XSTART 2
#define YSTART 1

// ST7735 commands
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2a
#define ST7735_RASET   0x2b
#define ST7735_RAMWR   0x2c
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3a
#define ST7735_INVOFF  0x20

static void display_select(void) {
    gpio_put(PIN_CS, 0);
}

static void display_deselect(void) {
    gpio_put(PIN_CS, 1);
}

static void display_write_command(uint8_t command) {
    display_select();
    gpio_put(PIN_DC, 0);
    spi_write_blocking(DISPLAY_SPI_PORT, &command, 1);
    display_deselect();
}

static void display_write_data(const uint8_t *data, size_t len) {
    display_select();
    gpio_put(PIN_DC, 1);
    spi_write_blocking(DISPLAY_SPI_PORT, data, len);
    display_deselect();
}

static void display_write_u8(uint8_t data) {
    display_write_data(&data, 1);
}

static void display_backlight_pwm_init(void) {
    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);

    backlight_pwm_slice = pwm_gpio_to_slice_num(PIN_BL);
    backlight_pwm_channel = pwm_gpio_to_channel(PIN_BL);

    pwm_set_wrap(backlight_pwm_slice, DISPLAY_PWM_WRAP);
    pwm_set_chan_level(backlight_pwm_slice, backlight_pwm_channel, DISPLAY_PWM_WRAP);
    pwm_set_enabled(backlight_pwm_slice, true);

    backlight_pwm_ready = true;
}

void display_set_brightness_percent(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }

    if (!backlight_pwm_ready) {
        display_backlight_pwm_init();
    }

    uint16_t level = (uint16_t)((DISPLAY_PWM_WRAP * percent) / 100);

    pwm_set_chan_level(
        backlight_pwm_slice,
        backlight_pwm_channel,
        level
    );
}

static void display_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint16_t x0 = x + XSTART;
    uint16_t y0 = y + YSTART;
    uint16_t x1 = x0 + width - 1;
    uint16_t y1 = y0 + height - 1;

    uint8_t column_data[] = {
        (uint8_t)(x0 >> 8),
        (uint8_t)(x0),
        (uint8_t)(x1 >> 8),
        (uint8_t)(x1)
    };

    uint8_t row_data[] = {
        (uint8_t)(y0 >> 8),
        (uint8_t)(y0),
        (uint8_t)(y1 >> 8),
        (uint8_t)(y1)
    };

    display_write_command(ST7735_CASET);
    display_write_data(column_data, sizeof(column_data));

    display_write_command(ST7735_RASET);
    display_write_data(row_data, sizeof(row_data));

    display_write_command(ST7735_RAMWR);
}

void display_init(void) {
    spi_init(DISPLAY_SPI_PORT, 40 * 1000 * 1000);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 0);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);

    display_backlight_pwm_init();


    gpio_put(PIN_RST, 0);
    sleep_ms(50);
    gpio_put(PIN_RST, 1);
    sleep_ms(120);

    display_write_command(ST7735_SWRESET);
    sleep_ms(150);

    display_write_command(ST7735_SLPOUT);
    sleep_ms(120);

    display_write_command(ST7735_COLMOD);
    display_write_u8(0x05); // 16-bit RGB565 color

    display_write_command(ST7735_MADCTL);
    display_write_u8(0x00); // portrait, RGB order

    display_write_command(ST7735_INVOFF);
    sleep_ms(10);

    display_write_command(ST7735_DISPON);
    sleep_ms(100);

    display_fill(DISPLAY_COLOR_BLACK);
}

void display_fill(uint16_t color) {
    display_set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    uint8_t high = (uint8_t)(color >> 8);
    uint8_t low = (uint8_t)(color);

    uint8_t buffer[128 * 2];

    for (int i = 0; i < 128; i++) {
        buffer[i * 2] = high;
        buffer[i * 2 + 1] = low;
    }

    uint32_t pixels = DISPLAY_WIDTH * DISPLAY_HEIGHT;

    display_select();
    gpio_put(PIN_DC, 1);

    while (pixels > 0) {
        uint32_t chunk_pixels = pixels > 128 ? 128 : pixels;
        spi_write_blocking(DISPLAY_SPI_PORT, buffer, chunk_pixels * 2);
        pixels -= chunk_pixels;
    }

    display_deselect();
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }

    if (x + width > DISPLAY_WIDTH) {
        width = DISPLAY_WIDTH - x;
    }

    if (y + height > DISPLAY_HEIGHT) {
        height = DISPLAY_HEIGHT - y;
    }

    display_set_window(x, y, width, height);

    uint8_t high = (uint8_t)(color >> 8);
    uint8_t low = (uint8_t)(color);

    uint8_t buffer[128 * 2];

    for (int i = 0; i < 128; i++) {
        buffer[i * 2] = high;
        buffer[i * 2 + 1] = low;
    }

    uint32_t pixels = width * height;

    display_select();
    gpio_put(PIN_DC, 1);

    while (pixels > 0) {
        uint32_t chunk_pixels = pixels > 128 ? 128 : pixels;
        spi_write_blocking(DISPLAY_SPI_PORT, buffer, chunk_pixels * 2);
        pixels -= chunk_pixels;
    }

    display_deselect();
}

static void font5x7_get(char c, uint8_t out[5]) {
    out[0] = 0x00;
    out[1] = 0x00;
    out[2] = 0x00;
    out[3] = 0x00;
    out[4] = 0x00;

    switch (c) {
        case '0': out[0]=0x3e; out[1]=0x51; out[2]=0x49; out[3]=0x45; out[4]=0x3e; break;
        case '1': out[0]=0x00; out[1]=0x42; out[2]=0x7f; out[3]=0x40; out[4]=0x00; break;
        case '2': out[0]=0x42; out[1]=0x61; out[2]=0x51; out[3]=0x49; out[4]=0x46; break;
        case '3': out[0]=0x21; out[1]=0x41; out[2]=0x45; out[3]=0x4b; out[4]=0x31; break;
        case '4': out[0]=0x18; out[1]=0x14; out[2]=0x12; out[3]=0x7f; out[4]=0x10; break;
        case '5': out[0]=0x27; out[1]=0x45; out[2]=0x45; out[3]=0x45; out[4]=0x39; break;
        case '6': out[0]=0x3c; out[1]=0x4a; out[2]=0x49; out[3]=0x49; out[4]=0x30; break;
        case '7': out[0]=0x01; out[1]=0x71; out[2]=0x09; out[3]=0x05; out[4]=0x03; break;
        case '8': out[0]=0x36; out[1]=0x49; out[2]=0x49; out[3]=0x49; out[4]=0x36; break;
        case '9': out[0]=0x06; out[1]=0x49; out[2]=0x49; out[3]=0x29; out[4]=0x1e; break;

        case 'A': out[0]=0x7e; out[1]=0x11; out[2]=0x11; out[3]=0x11; out[4]=0x7e; break;
        case 'B': out[0]=0x7f; out[1]=0x49; out[2]=0x49; out[3]=0x49; out[4]=0x36; break;
        case 'C': out[0]=0x3e; out[1]=0x41; out[2]=0x41; out[3]=0x41; out[4]=0x22; break;
        case 'D': out[0]=0x7f; out[1]=0x41; out[2]=0x41; out[3]=0x22; out[4]=0x1c; break;
        case 'E': out[0]=0x7f; out[1]=0x49; out[2]=0x49; out[3]=0x49; out[4]=0x41; break;
        case 'F': out[0]=0x7f; out[1]=0x09; out[2]=0x09; out[3]=0x09; out[4]=0x01; break;
        case 'G': out[0]=0x3e; out[1]=0x41; out[2]=0x49; out[3]=0x49; out[4]=0x7a; break;
        case 'H': out[0]=0x7f; out[1]=0x08; out[2]=0x08; out[3]=0x08; out[4]=0x7f; break;
        case 'I': out[0]=0x00; out[1]=0x41; out[2]=0x7f; out[3]=0x41; out[4]=0x00; break;
        case 'J': out[0]=0x20; out[1]=0x40; out[2]=0x41; out[3]=0x3f; out[4]=0x01; break;
        case 'K': out[0]=0x7f; out[1]=0x08; out[2]=0x14; out[3]=0x22; out[4]=0x41; break;
        case 'L': out[0]=0x7f; out[1]=0x40; out[2]=0x40; out[3]=0x40; out[4]=0x40; break;
        case 'M': out[0]=0x7f; out[1]=0x02; out[2]=0x0c; out[3]=0x02; out[4]=0x7f; break;
        case 'N': out[0]=0x7f; out[1]=0x04; out[2]=0x08; out[3]=0x10; out[4]=0x7f; break;
        case 'O': out[0]=0x3e; out[1]=0x41; out[2]=0x41; out[3]=0x41; out[4]=0x3e; break;
        case 'P': out[0]=0x7f; out[1]=0x09; out[2]=0x09; out[3]=0x09; out[4]=0x06; break;
        case 'Q': out[0]=0x3e; out[1]=0x41; out[2]=0x51; out[3]=0x21; out[4]=0x5e; break;
        case 'R': out[0]=0x7f; out[1]=0x09; out[2]=0x19; out[3]=0x29; out[4]=0x46; break;
        case 'S': out[0]=0x46; out[1]=0x49; out[2]=0x49; out[3]=0x49; out[4]=0x31; break;
        case 'T': out[0]=0x01; out[1]=0x01; out[2]=0x7f; out[3]=0x01; out[4]=0x01; break;
        case 'U': out[0]=0x3f; out[1]=0x40; out[2]=0x40; out[3]=0x40; out[4]=0x3f; break;
        case 'V': out[0]=0x1f; out[1]=0x20; out[2]=0x40; out[3]=0x20; out[4]=0x1f; break;
        case 'W': out[0]=0x3f; out[1]=0x40; out[2]=0x38; out[3]=0x40; out[4]=0x3f; break;
        case 'X': out[0]=0x63; out[1]=0x14; out[2]=0x08; out[3]=0x14; out[4]=0x63; break;
        case 'Y': out[0]=0x07; out[1]=0x08; out[2]=0x70; out[3]=0x08; out[4]=0x07; break;
        case 'Z': out[0]=0x61; out[1]=0x51; out[2]=0x49; out[3]=0x45; out[4]=0x43; break;

        case ':': out[0]=0x00; out[1]=0x36; out[2]=0x36; out[3]=0x00; out[4]=0x00; break;
        case '/': out[0]=0x20; out[1]=0x10; out[2]=0x08; out[3]=0x04; out[4]=0x02; break;
        case '.': out[0]=0x00; out[1]=0x60; out[2]=0x60; out[3]=0x00; out[4]=0x00; break;
        case '-': out[0]=0x08; out[1]=0x08; out[2]=0x08; out[3]=0x08; out[4]=0x08; break;
        case ' ': break;

        default:
            out[0]=0x7f; out[1]=0x41; out[2]=0x5d; out[3]=0x41; out[4]=0x7f;
            break;
    }
}

static void display_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint8_t scale) {
    uint8_t columns[5];
    font5x7_get(c, columns);

    if (scale == 0) {
        scale = 1;
    }

    for (uint8_t col = 0; col < 5; col++) {
        for (uint8_t row = 0; row < 7; row++) {
            if ((columns[col] & (1u << row)) != 0) {
                display_fill_rect(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

void display_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale) {
    uint16_t cursor_x = x;

    if (scale == 0) {
        scale = 1;
    }

    while (*text != '\0') {
        display_draw_char(cursor_x, y, *text, color, scale);
        cursor_x += 6 * scale;
        text++;
    }
}