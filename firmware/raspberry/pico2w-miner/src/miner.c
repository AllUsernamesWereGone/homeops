#include "miner.h"
#include "sha256.h"
#include "bitcoin_job.h"
#include "bitcoin_target.h"
#include "pico/stdlib.h"

#include <string.h>
#include <stdio.h>

#define MINER_TIME_CHECK_BATCH 2048 //change this for checks, and adjust accoridngly 2048 might be too much

void miner_init(miner_state_t *state) {
    // Nonce is the 32-bit value that changes for every hash attempt.
    state->nonce = 0;
    // Number of hashes calculated during the current mining window.
    // This is reset in miner_run_for_ms().
    state->hashes_done = 0;
    // Best demo result seen since boot, measured by leading zero bits.
    state->best_zero_bits = 0;

    // Demo share statistics.
    state->shares_found = 0;
    state->last_share_nonce = 0;
    state->last_share_zero_bits = 0;
    state->share_found_in_window = false;

    state->real_share_found_in_window = false;
    state->real_block_found_in_window = false;
    state->submit_share_pending = false;

    state->last_real_share_nonce = 0;
    state->last_real_block_nonce = 0;

    memset(state->last_real_share_hash, 0, sizeof(state->last_real_share_hash));
    memset(state->last_real_block_hash, 0, sizeof(state->last_real_block_hash));

    memset(state->last_hash, 0, MINER_HASH_SIZE);
    memset(state->best_hash, 0xff, MINER_HASH_SIZE);
    memset(state->last_share_hash, 0, MINER_HASH_SIZE);
}


uint32_t miner_count_leading_zero_bits(const uint8_t hash[MINER_HASH_SIZE]) {
    uint32_t zero_bits = 0;

    // Counts zero bits from the beginning of the given byte array.
    // Important: this uses the byte order passed into the function.
    for (int i = 0; i < MINER_HASH_SIZE; i++) {
        uint8_t byte = hash[i];

        if (byte == 0) {
            zero_bits += 8;
            continue;
        }

        for (int bit = 7; bit >= 0; bit--) {
            if ((byte & (1u << bit)) == 0) {
                zero_bits++;
            } else {
                return zero_bits;
            }
        }
    }

    return zero_bits;
}

static bool hash_less_display_order(
    const uint8_t a[MINER_HASH_SIZE],
    const uint8_t b[MINER_HASH_SIZE]
) {
    for (int i = 0; i < MINER_HASH_SIZE; i++) {
        if (a[i] < b[i]) {
            return true;
        }

        if (a[i] > b[i]) {
            return false;
        }
    }

    return false;
}

void miner_hash_to_hex(const uint8_t hash[MINER_HASH_SIZE], char out[MINER_HASH_HEX_SIZE]) {
    static const char hex[] = "0123456789abcdef";

    // Convert hash bytes directly to lowercase hexadecimal.
    for (int i = 0; i < MINER_HASH_SIZE; i++) {
        out[i * 2] = hex[(hash[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[hash[i] & 0x0f];
    }

    out[64] = '\0';
}

void miner_hash_to_display_hex(const uint8_t hash[MINER_HASH_SIZE], char out[MINER_HASH_HEX_SIZE]) {
    static const char hex[] = "0123456789abcdef";

    // Bitcoin block hashes are normally displayed with the byte order reversed.
    for (int i = 0; i < MINER_HASH_SIZE; i++) {
        uint8_t byte = hash[MINER_HASH_SIZE - 1 - i];

        out[i * 2] = hex[(byte >> 4) & 0x0f];
        out[i * 2 + 1] = hex[byte & 0x0f];
    }

    out[64] = '\0';
}


static inline void miner_write_nonce_le(uint8_t header[BITCOIN_BLOCK_HEADER_SIZE], uint32_t nonce) {
    header[BITCOIN_NONCE_OFFSET + 0] = (uint8_t)(nonce);
    header[BITCOIN_NONCE_OFFSET + 1] = (uint8_t)(nonce >> 8);
    header[BITCOIN_NONCE_OFFSET + 2] = (uint8_t)(nonce >> 16);
    header[BITCOIN_NONCE_OFFSET + 3] = (uint8_t)(nonce >> 24);
}


void miner_run_for_ms(miner_state_t *state, const bitcoin_job_t *job, uint32_t duration_ms) {
    uint8_t header[BITCOIN_BLOCK_HEADER_SIZE];

    memcpy(header, job->header, BITCOIN_BLOCK_HEADER_SIZE);

    absolute_time_t end_time = make_timeout_time_ms(duration_ms);

    state->hashes_done = 0;
    state->share_found_in_window = false;
    state->real_share_found_in_window = false;
    state->real_block_found_in_window = false;

    /*
     * Do NOT reset submit_share_pending here.
     * That flag must stay true until main.c successfully sends mining.submit.
     */

    while (!time_reached(end_time)) {
        for (uint32_t batch = 0; batch < MINER_TIME_CHECK_BATCH; batch++) {
            miner_write_nonce_le(header, state->nonce);

            double_sha256(header, BITCOIN_BLOCK_HEADER_SIZE, state->last_hash);

            if (hash_less_display_order(state->last_hash, state->best_hash)) {
                memcpy(state->best_hash, state->last_hash, MINER_HASH_SIZE);
                state->best_zero_bits = miner_count_leading_zero_bits(state->best_hash);
            }

            if (bitcoin_hash_meets_target(state->last_hash, &job->share_target)) {
                uint32_t zero_bits = miner_count_leading_zero_bits(state->last_hash);

                state->real_share_found_in_window = true;
                state->submit_share_pending = true;

                state->last_real_share_nonce = state->nonce;
                memcpy(state->last_real_share_hash, state->last_hash, MINER_HASH_SIZE);

                state->shares_found++;
                state->share_found_in_window = true;
                state->last_share_nonce = state->nonce;
                state->last_share_zero_bits = zero_bits;
                memcpy(state->last_share_hash, state->last_hash, MINER_HASH_SIZE);

                printf("MINER: REAL POOL SHARE FOUND! nonce=%lu\n",
                    (unsigned long)state->nonce);

                if (
                    job->has_network_target &&
                    bitcoin_hash_meets_target(state->last_hash, &job->network_target)
                ) {
                    state->real_block_found_in_window = true;
                    state->last_real_block_nonce = state->nonce;
                    memcpy(state->last_real_block_hash, state->last_hash, MINER_HASH_SIZE);

                    printf("MINER: POSSIBLE REAL BLOCK FOUND! nonce=%lu\n",
                        (unsigned long)state->nonce);
                }
            }

            state->nonce++;
            state->hashes_done++;
        }
    }
}

static bool bytes_equal(const uint8_t *a, const uint8_t *b, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

bool miner_genesis_header_self_test(void) {
    // Full Bitcoin genesis block header with the real genesis nonce.
    static const uint8_t genesis_header[80] = {
        // version: 1
        0x01, 0x00, 0x00, 0x00,

        // previous block hash: 32 bytes of zero
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        // merkle root, serialized in block-header byte order
        0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,
        0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
        0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a,

        // time: 1231006505 -> 0x495fab29 -> little-endian
        0x29, 0xab, 0x5f, 0x49,

        // bits: 0x1d00ffff -> little-endian
        0xff, 0xff, 0x00, 0x1d,

        // nonce: 2083236893 -> 0x7c2bac1d -> little-endian
        0x1d, 0xac, 0x2b, 0x7c
    };

    // Expected double-SHA256 result in raw byte order.
    static const uint8_t expected_raw_hash[MINER_HASH_SIZE] = {
        0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
        0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
        0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
        0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    uint8_t actual_hash[MINER_HASH_SIZE];

    double_sha256(genesis_header, sizeof(genesis_header), actual_hash);

    return bytes_equal(actual_hash, expected_raw_hash, MINER_HASH_SIZE);
}