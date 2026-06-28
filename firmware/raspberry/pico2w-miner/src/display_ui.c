#include "display_ui.h"

#include <stdio.h>

#include "display_st7735.h"

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xffff
#define COLOR_GREEN 0x07e0
#define COLOR_YELLOW 0xffe0
#define COLOR_RED 0xf800
#define COLOR_CYAN 0x07ff

void display_ui_show_boot(void) {
    display_fill(COLOR_BLACK);

    display_draw_text(4, 10, "PICO2W", COLOR_CYAN, COLOR_BLACK);
    display_draw_text(4, 30, "BTC MINER", COLOR_WHITE, COLOR_BLACK);
    display_draw_text(4, 55, "BOOTING...", COLOR_YELLOW, COLOR_BLACK);
}

void display_ui_update_simple(
    const miner_state_t *miner,
    const bitcoin_job_t *job,
    const char *status,
     uint32_t needed_bits
) {
    char line[32];

    display_fill(COLOR_BLACK);

    display_draw_text(4, 8, "BTC MINER", COLOR_CYAN, COLOR_BLACK);

    snprintf(
        line,
        sizeof(line),
        "RATE %lu H/s",
        (unsigned long)miner->hashes_done
    );
    display_draw_text(4, 35, line, COLOR_WHITE, COLOR_BLACK);

    snprintf(
        line,
        sizeof(line),
        "STATUS %s",
        status != NULL ? status : "OK"
    );

    uint16_t status_color = COLOR_GREEN;

    if (status != NULL && status[0] == 'E') {
        status_color = COLOR_RED;
    } else if (status != NULL && status[0] == 'W') {
        status_color = COLOR_YELLOW;
    }

    display_draw_text(4, 62, line, status_color, COLOR_BLACK);

    snprintf(
        line,
        sizeof(line),
        "BEST %lu/%lu BITS",
        (unsigned long)miner->best_zero_bits,
        (unsigned long)needed_bits
    );
    display_draw_text(4, 90, line, COLOR_WHITE, COLOR_BLACK);

    if (job->has_share_target) {
        display_draw_text(4, 118, "REAL STRATUM", COLOR_GREEN, COLOR_BLACK);
    } else {
        display_draw_text(4, 118, "DEMO MODE", COLOR_YELLOW, COLOR_BLACK);
    }
}

void display_ui_update(const miner_state_t *miner, const bitcoin_job_t *job) {
    char line[32];

    display_fill(DISPLAY_COLOR_BLACK);

    display_draw_text(2, 2, "PICO2W MINER", DISPLAY_COLOR_CYAN, 1);

    snprintf(line, sizeof(line), "RATE:%lu H/S", (unsigned long)miner->hashes_done);
    display_draw_text(2, 18, line, DISPLAY_COLOR_WHITE, 1);

    snprintf(line, sizeof(line), "SHARES:%lu", (unsigned long)miner->shares_found);
    display_draw_text(2, 30, line, DISPLAY_COLOR_GREEN, 1);

    snprintf(line, sizeof(line), "BEST:%lu BITS", (unsigned long)miner->best_zero_bits);
    display_draw_text(2, 42, line, DISPLAY_COLOR_YELLOW, 1);

    snprintf(line, sizeof(line), "TARGET:%lu", (unsigned long)job->target_zero_bits);
    display_draw_text(2, 54, line, DISPLAY_COLOR_MAGENTA, 1);

    snprintf(line, sizeof(line), "NONCE:%lu", (unsigned long)miner->nonce);
    display_draw_text(2, 66, line, DISPLAY_COLOR_WHITE, 1);

    if (miner->share_found_in_window) {
        display_draw_text(2, 84, "SHARE FOUND", DISPLAY_COLOR_GREEN, 1);
    } else {
        display_draw_text(2, 84, "MINING...", DISPLAY_COLOR_BLUE, 1);
    }
}