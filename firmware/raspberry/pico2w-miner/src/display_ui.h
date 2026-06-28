#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "miner.h"
#include "bitcoin_job.h"

#include <stdint.h>

void display_ui_show_boot(void);

void display_ui_update(const miner_state_t *miner, const bitcoin_job_t *job);

void display_ui_update_simple(
    const miner_state_t *miner,
    const bitcoin_job_t *job,
    const char *status,
    uint32_t needed_bits
);

#endif