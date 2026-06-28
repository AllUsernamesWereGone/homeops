#include "bitcoin_job.h"
#include "bitcoin_hex.h"
#include "sha256.h"
#include "stratum.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BITCOIN_MAX_COINBASE_BYTES 4096

void bitcoin_job_write_nonce(uint8_t header[BITCOIN_BLOCK_HEADER_SIZE], uint32_t nonce) {
     // Bitcoin block headers store the nonce as 4 little-endian bytes.
    header[BITCOIN_NONCE_OFFSET + 0] = (uint8_t)(nonce);
    header[BITCOIN_NONCE_OFFSET + 1] = (uint8_t)(nonce >> 8);
    header[BITCOIN_NONCE_OFFSET + 2] = (uint8_t)(nonce >> 16);
    header[BITCOIN_NONCE_OFFSET + 3] = (uint8_t)(nonce >> 24);
}

void bitcoin_job_init_demo(bitcoin_job_t *job) {

    job->has_network_target = false;
    job->has_share_target = false;
    job->has_submit_data = false;
    job->submit_job_id[0] = '\0';
    job->submit_extranonce2[0] = '\0';
    job->submit_ntime[0] = '\0';
    // Demo block header based on the Bitcoin genesis block.
    // This is useful for local testing because the expected hash is known.
    static const uint8_t demo_header[BITCOIN_BLOCK_HEADER_SIZE] = {
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

        // nonce: starts at 0 for demo mining
        0x00, 0x00, 0x00, 0x00
    };

    memcpy(job->header, demo_header, BITCOIN_BLOCK_HEADER_SIZE);

    // Simplified demo target.
    // A hash counts as a test share if it has at least this many leading zero bits.
    // This is not the real Bitcoin difficulty target.
    job->target_zero_bits = 16;
}

static bool append_hex_bytes(
    uint8_t *out,
    size_t out_size,
    size_t *offset,
    const char *hex
) {
    if (out == NULL || offset == NULL || hex == NULL) {
        return false;
    }

    size_t written = 0;

    if (!bitcoin_hex_to_bytes(hex, out + *offset, out_size - *offset, &written)) {
        return false;
    }

    *offset += written;
    return true;
}

static bool format_extranonce2_hex(
    uint32_t extranonce2,
    uint32_t extranonce2_size,
    char *out,
    size_t out_size
) {
    if (out == NULL || out_size == 0) {
        return false;
    }

    if (extranonce2_size == 0 || extranonce2_size > 16) {
        return false;
    }

    if (out_size < (extranonce2_size * 2 + 1)) {
        return false;
    }

    uint8_t bytes[16];
    memset(bytes, 0, sizeof(bytes));

    /*
     * Same encoding as used in the coinbase:
     * extranonce2 = 1, size = 8 -> 0000000000000001
     */
    for (uint32_t i = 0; i < extranonce2_size && i < 4; i++) {
        bytes[extranonce2_size - 1 - i] =
            (uint8_t)((extranonce2 >> (8 * i)) & 0xff);
    }

    for (uint32_t i = 0; i < extranonce2_size; i++) {
        snprintf(out + i * 2, 3, "%02x", bytes[i]);
    }

    out[extranonce2_size * 2] = '\0';

    return true;
}

static uint32_t parse_pool_difficulty_u32(const char *difficulty_text) {
    if (difficulty_text == NULL || difficulty_text[0] == '\0') {
        return 0;
    }

    /*
     * The current pool sends "10000".
     * This simple parser intentionally handles integer difficulty first.
     */
    char *endptr = NULL;
    unsigned long value = strtoul(difficulty_text, &endptr, 10);

    if (value == 0 || value > 0xffffffffUL) {
        return 0;
    }

    return (uint32_t)value;
}

static bool write_extranonce2_bytes(
    uint8_t *out,
    size_t out_size,
    uint32_t extranonce2
) {
    if (out == NULL || out_size == 0) {
        return false;
    }

    if (out_size > 16) {
        printf("bitcoin_job: extranonce2_size too large: %u\n", (unsigned)out_size);
        return false;
    }

    memset(out, 0, out_size);

    // Use fixed-width big-endian representation:
    // extranonce2 = 1, size = 4 -> 00 00 00 01
    for (size_t i = 0; i < out_size && i < 4; i++) {
        out[out_size - 1 - i] = (uint8_t)((extranonce2 >> (8 * i)) & 0xff);
    }

    return true;
}

static bool build_coinbase_transaction(
    const stratum_state_t *state,
    uint32_t extranonce2,
    uint8_t *coinbase,
    size_t coinbase_size,
    size_t *coinbase_len
) {
    if (state == NULL || coinbase == NULL || coinbase_len == NULL) {
        return false;
    }

    size_t offset = 0;

    if (!append_hex_bytes(coinbase, coinbase_size, &offset, state->coinb1)) {
        printf("bitcoin_job: failed to decode coinb1\n");
        return false;
    }

    if (!append_hex_bytes(coinbase, coinbase_size, &offset, state->extranonce1)) {
        printf("bitcoin_job: failed to decode extranonce1\n");
        return false;
    }

    if (state->extranonce2_size == 0 || state->extranonce2_size > 16) {
        printf("bitcoin_job: invalid extranonce2_size: %lu\n",
               (unsigned long)state->extranonce2_size);
        return false;
    }

    if (offset + state->extranonce2_size > coinbase_size) {
        printf("bitcoin_job: coinbase buffer too small for extranonce2\n");
        return false;
    }

    if (!write_extranonce2_bytes(
            coinbase + offset,
            state->extranonce2_size,
            extranonce2
        )) {
        printf("bitcoin_job: failed to write extranonce2\n");
        return false;
    }

    offset += state->extranonce2_size;

    if (!append_hex_bytes(coinbase, coinbase_size, &offset, state->coinb2)) {
        printf("bitcoin_job: failed to decode coinb2\n");
        return false;
    }

    *coinbase_len = offset;
    return true;
}

static bool calculate_merkle_root(
    const stratum_state_t *state,
    const uint8_t *coinbase,
    size_t coinbase_len,
    uint8_t merkle_root[32]
) {
    if (state == NULL || coinbase == NULL || merkle_root == NULL) {
        return false;
    }

    // First hash is the coinbase transaction hash.
    double_sha256(coinbase, coinbase_len, merkle_root);

    for (uint8_t i = 0; i < state->merkle_branch_count; i++) {
        uint8_t branch[32];

        if (!bitcoin_hex_to_bytes_exact(
                state->merkle_branches[i],
                branch,
                sizeof(branch)
            )) {
            printf("bitcoin_job: failed to decode merkle branch %u\n", i);
            return false;
        }

        uint8_t concat[64];

        memcpy(concat, merkle_root, 32);
        memcpy(concat + 32, branch, 32);

        double_sha256(concat, sizeof(concat), merkle_root);
    }

    return true;
}

static bool write_u32_hex_le_to_header(
    uint8_t *out,
    const char *hex,
    const char *field_name
) {
    uint32_t value = 0;

    if (!bitcoin_read_hex_u32(hex, &value)) {
        printf("bitcoin_job: invalid %s hex: %s\n", field_name, hex);
        return false;
    }

    bitcoin_write_u32_le(out, value);
    return true;
}

static bool write_prevhash_from_stratum(
    uint8_t out[32],
    const char *prevhash_hex
) {
    uint8_t raw[32];

    if (!bitcoin_hex_to_bytes_exact(prevhash_hex, raw, sizeof(raw))) {
        printf("bitcoin_job: invalid prevhash hex\n");
        return false;
    }

    /*
     * Common Stratum V1 prevhash handling:
     *
     * mining.notify gives the previous hash as 8 x 4-byte words.
     * For the block header, each 4-byte word is reversed.
     *
     * Example word:
     *   input:  41 28 bf 63
     *   header: 63 bf 28 41
     */
    for (size_t word = 0; word < 8; word++) {
        size_t base = word * 4;

        out[base + 0] = raw[base + 3];
        out[base + 1] = raw[base + 2];
        out[base + 2] = raw[base + 1];
        out[base + 3] = raw[base + 0];
    }

    return true;
}

bool bitcoin_job_from_stratum(
    bitcoin_job_t *job,
    const stratum_state_t *state,
    uint32_t extranonce2
) {
    if (job == NULL || state == NULL) {
        return false;
    }

    if (!state->subscribed) {
        printf("bitcoin_job: cannot build job, not subscribed\n");
        return false;
    }

    if (!state->authorized) {
        printf("bitcoin_job: cannot build job, not authorized\n");
        return false;
    }

    if (!state->has_job) {
        printf("bitcoin_job: cannot build job, no mining.notify job yet\n");
        return false;
    }

    uint8_t coinbase[BITCOIN_MAX_COINBASE_BYTES];
    size_t coinbase_len = 0;

    if (!build_coinbase_transaction(
            state,
            extranonce2,
            coinbase,
            sizeof(coinbase),
            &coinbase_len
        )) {
        return false;
    }

    printf("bitcoin_job: coinbase length=%u bytes\n", (unsigned)coinbase_len);

    uint8_t merkle_root[32];

    if (!calculate_merkle_root(
            state,
            coinbase,
            coinbase_len,
            merkle_root
        )) {
        return false;
    }

    memset(job->header, 0, BITCOIN_BLOCK_HEADER_SIZE);

    // Header layout:
    // 0..3   version
    // 4..35  previous block hash
    // 36..67 merkle root
    // 68..71 ntime
    // 72..75 nbits
    // 76..79 nonce

    if (!write_u32_hex_le_to_header(job->header + 0, state->version, "version")) {
        return false;
    }

    if (!write_prevhash_from_stratum(job->header + 4, state->prevhash)) {
        return false;
    }

    memcpy(job->header + 36, merkle_root, 32);

    if (!write_u32_hex_le_to_header(job->header + 68, state->ntime, "ntime")) {
        return false;
    }

    if (!write_u32_hex_le_to_header(job->header + 72, state->nbits, "nbits")) {
        return false;
    }

    uint32_t nbits_value = 0;

    if (!bitcoin_read_hex_u32(state->nbits, &nbits_value)) {
        printf("bitcoin_job: failed to parse nbits for target\n");
        return false;
    }

    bitcoin_target_from_nbits(nbits_value, &job->network_target);
    job->has_network_target = true;

    bitcoin_target_print("bitcoin_job: network target=", &job->network_target);

    uint32_t pool_difficulty = parse_pool_difficulty_u32(state->difficulty_text);

    if (pool_difficulty > 0) {
        if (bitcoin_target_from_difficulty_u32(pool_difficulty, &job->share_target)) {
            job->has_share_target = true;
            bitcoin_target_print("bitcoin_job: share target=", &job->share_target);
        } else {
            job->has_share_target = false;
            printf("bitcoin_job: failed to build share target\n");
        }
    } else {
        job->has_share_target = false;
        printf("bitcoin_job: no valid pool difficulty yet\n");
    }

    bitcoin_job_write_nonce(job->header, 0);

    /*
     * Temporary:
     * The old miner still uses leading-zero-bit fake share detection.
     * Later we should replace this with bitcoin_hash_meets_target()
     * using the real nBits/share target.
     */
    job->target_zero_bits = 32;

    job->has_submit_data = false;

    strncpy(job->submit_job_id, state->job_id, sizeof(job->submit_job_id) - 1);
    job->submit_job_id[sizeof(job->submit_job_id) - 1] = '\0';

    strncpy(job->submit_ntime, state->ntime, sizeof(job->submit_ntime) - 1);
    job->submit_ntime[sizeof(job->submit_ntime) - 1] = '\0';

    if (!format_extranonce2_hex(
            extranonce2,
            state->extranonce2_size,
            job->submit_extranonce2,
            sizeof(job->submit_extranonce2)
        )) {
        printf("bitcoin_job: failed to format extranonce2 for submit\n");
        return false;
    }

    job->has_submit_data = true;

    printf("bitcoin_job: submit extranonce2=%s\n", job->submit_extranonce2);

    printf("bitcoin_job: built real Stratum block header\n");
    printf("bitcoin_job: job_id=%s extranonce2=%lu\n",
           state->job_id,
           (unsigned long)extranonce2);

    return true;
}