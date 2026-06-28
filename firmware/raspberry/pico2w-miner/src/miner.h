#ifndef MINER_H
#define MINER_H

#include "bitcoin_job.h"

#include <stdint.h>
#include <stdbool.h>

#define MINER_HASH_SIZE 32
#define MINER_HASH_HEX_SIZE 65

// Structure to hold the state of the miner
typedef struct {
    uint32_t nonce;
    uint32_t hashes_done;

    uint8_t last_hash[32];
    uint8_t best_hash[32];
    uint32_t best_zero_bits;

    uint32_t shares_found;
    uint32_t last_share_nonce;
    uint32_t last_share_zero_bits;
    bool share_found_in_window;
    uint8_t last_share_hash[32];

    bool real_share_found_in_window;
    bool real_block_found_in_window;
    bool submit_share_pending;

    uint32_t last_real_share_nonce;
    uint32_t last_real_block_nonce;

    uint8_t last_real_share_hash[32];
    uint8_t last_real_block_hash[32];
} miner_state_t;

// declares a function that initializes the miner state.
void miner_init(miner_state_t *state);
// declares a function that runs the dummy miner for a fixed number of milliseconds.
void miner_run_for_ms(miner_state_t *state, const bitcoin_job_t *job, uint32_t duration_ms);

uint32_t miner_count_leading_zero_bits(const uint8_t hash[MINER_HASH_SIZE]);

void miner_hash_to_hex(const uint8_t hash[MINER_HASH_SIZE], char out[MINER_HASH_HEX_SIZE]);

void miner_hash_to_display_hex(const uint8_t hash[MINER_HASH_SIZE], char out[MINER_HASH_HEX_SIZE]);

bool miner_genesis_header_self_test(void);


#endif