#ifndef BITCOIN_JOB_H
#define BITCOIN_JOB_H

#include <stdbool.h>
#include <stdint.h>

#include "stratum.h"
#include "bitcoin_target.h"

#define BITCOIN_BLOCK_HEADER_SIZE 80
#define BITCOIN_NONCE_OFFSET 76

typedef struct {
    uint8_t header[BITCOIN_BLOCK_HEADER_SIZE];

    /*
     * Old debug fallback.
     * Used only when no real share target exists.
     */
    uint32_t target_zero_bits;

    /*
     * Real Bitcoin network target from nBits.
     * If a hash meets this, that would be an actual candidate block.
     */
    bitcoin_target_t network_target;
    bool has_network_target;

    /*
     * Pool share target from mining.set_difficulty.
     * If a hash meets this, we submit it to the pool.
     */
    bitcoin_target_t share_target;
    bool has_share_target;

    /*
     * Data needed for mining.submit.
     */
    bool has_submit_data;
    char submit_job_id[STRATUM_JOB_ID_MAX_LEN];
    char submit_extranonce2[40];
    char submit_ntime[STRATUM_NTIME_MAX_LEN];
} bitcoin_job_t;

void bitcoin_job_init_demo(bitcoin_job_t *job);

void bitcoin_job_write_nonce(uint8_t header[BITCOIN_BLOCK_HEADER_SIZE], uint32_t nonce);

bool bitcoin_job_from_stratum(bitcoin_job_t *job, const stratum_state_t *state, uint32_t extranonce2);

#endif