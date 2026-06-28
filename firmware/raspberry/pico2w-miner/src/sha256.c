#include "sha256.h"
#include "pico.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32u - (n))))
#define SHR(x, n)  ((x) >> (n))

#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define BIG_SIGMA0(x) (ROTR((x), 2) ^ ROTR((x), 13) ^ ROTR((x), 22))
#define BIG_SIGMA1(x) (ROTR((x), 6) ^ ROTR((x), 11) ^ ROTR((x), 25))
#define SMALL_SIGMA0(x) (ROTR((x), 7) ^ ROTR((x), 18) ^ SHR((x), 3))
#define SMALL_SIGMA1(x) (ROTR((x), 17) ^ ROTR((x), 19) ^ SHR((x), 10))

static const uint32_t SHA256_INITIAL_STATE[8] = {
    0x6a09e667UL,
    0xbb67ae85UL,
    0x3c6ef372UL,
    0xa54ff53aUL,
    0x510e527fUL,
    0x9b05688cUL,
    0x1f83d9abUL,
    0x5be0cd19UL
};

static const uint32_t K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static inline uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}

static inline void write_be32(uint8_t *p, uint32_t x) {
    p[0] = (uint8_t)(x >> 24);
    p[1] = (uint8_t)(x >> 16);
    p[2] = (uint8_t)(x >> 8);
    p[3] = (uint8_t)x;
}

static inline void sha256_init_state(uint32_t state[8]) {
    memcpy(state, SHA256_INITIAL_STATE, sizeof(SHA256_INITIAL_STATE));
}

/*
 * SHA-256 compression function.
 *
 * This function is the main hot path. Keeping it in RAM can improve speed on
 * the Pico because very hot code avoids repeated flash/cache pressure.
 */
static void __not_in_flash_func(sha256_compress)(
    uint32_t state[8],
    const uint8_t block[64]
) {
    uint32_t w[64];

    for (int i = 0; i < 16; i++) {
        w[i] = read_be32(block + (i * 4));
    }

    for (int i = 16; i < 64; i++) {
        w[i] = SMALL_SIGMA1(w[i - 2]) +
               w[i - 7] +
               SMALL_SIGMA0(w[i - 15]) +
               w[i - 16];
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + BIG_SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        uint32_t t2 = BIG_SIGMA0(a) + MAJ(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void sha256(const uint8_t *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE]) {
    uint32_t state[8];
    sha256_init_state(state);

    size_t original_len = len;

    while (len >= 64) {
        sha256_compress(state, data);
        data += 64;
        len -= 64;
    }

    uint8_t block[128];
    memset(block, 0, sizeof(block));

    memcpy(block, data, len);
    block[len] = 0x80;

    size_t final_block_len = (len < 56) ? 64 : 128;
    uint64_t total_bit_len = (uint64_t)original_len * 8u;

    block[final_block_len - 8] = (uint8_t)(total_bit_len >> 56);
    block[final_block_len - 7] = (uint8_t)(total_bit_len >> 48);
    block[final_block_len - 6] = (uint8_t)(total_bit_len >> 40);
    block[final_block_len - 5] = (uint8_t)(total_bit_len >> 32);
    block[final_block_len - 4] = (uint8_t)(total_bit_len >> 24);
    block[final_block_len - 3] = (uint8_t)(total_bit_len >> 16);
    block[final_block_len - 2] = (uint8_t)(total_bit_len >> 8);
    block[final_block_len - 1] = (uint8_t)total_bit_len;

    sha256_compress(state, block);

    if (final_block_len == 128) {
        sha256_compress(state, block + 64);
    }

    for (int i = 0; i < 8; i++) {
        write_be32(out + (i * 4), state[i]);
    }
}

/*
 * Fast path for Bitcoin block headers.
 *
 * Bitcoin block headers are exactly 80 bytes.
 * The first 64 bytes usually stay unchanged while mining; only the nonce in
 * the last 16-byte tail changes. Therefore we cache the SHA-256 state after
 * compressing the first 64-byte block.
 *
 * This reduces the normal mining path from:
 *
 *   first SHA-256:  block 1 + block 2
 *   second SHA-256: block 3
 *
 * to:
 *
 *   cached first SHA-256 block 1
 *   per nonce: block 2 + block 3
 *
 * This cache is intended for the current single-core miner.
 */
static void __not_in_flash_func(double_sha256_80_cached)(
    const uint8_t header[80],
    uint8_t out[SHA256_DIGEST_SIZE]
) {
    static bool cache_valid = false;
    static uint8_t cached_first_block[64];
    static uint32_t cached_midstate[8];

    uint32_t state[8];

    if (!cache_valid || memcmp(cached_first_block, header, 64) != 0) {
        sha256_init_state(cached_midstate);
        sha256_compress(cached_midstate, header);

        memcpy(cached_first_block, header, 64);
        cache_valid = true;
    }

    memcpy(state, cached_midstate, sizeof(state));

    /*
     * Build the second block of the first SHA-256.
     *
     * The full message length is 80 bytes = 640 bits.
     * This block contains:
     *   - header bytes 64..79
     *   - 0x80 padding byte
     *   - zeros
     *   - 64-bit big-endian length field: 640
     */
    uint8_t block1_tail[64];
    memset(block1_tail, 0, sizeof(block1_tail));

    memcpy(block1_tail, header + 64, 16);
    block1_tail[16] = 0x80;

    block1_tail[62] = 0x02;
    block1_tail[63] = 0x80;

    sha256_compress(state, block1_tail);

    /*
     * Convert the first SHA-256 state into a 32-byte digest.
     */
    uint8_t first_hash[SHA256_DIGEST_SIZE];

    for (int i = 0; i < 8; i++) {
        write_be32(first_hash + (i * 4), state[i]);
    }

    /*
     * Second SHA-256.
     *
     * The input is exactly 32 bytes = 256 bits.
     * Therefore it always fits in one final block.
     */
    uint8_t block2[64];
    memset(block2, 0, sizeof(block2));

    memcpy(block2, first_hash, SHA256_DIGEST_SIZE);
    block2[SHA256_DIGEST_SIZE] = 0x80;

    block2[62] = 0x01;
    block2[63] = 0x00;

    uint32_t second_state[8];
    sha256_init_state(second_state);

    sha256_compress(second_state, block2);

    for (int i = 0; i < 8; i++) {
        write_be32(out + (i * 4), second_state[i]);
    }
}

void double_sha256(const uint8_t *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE]) {
    /*
     * Miner hot path: Bitcoin block header.
     */
    if (len == 80) {
        double_sha256_80_cached(data, out);
        return;
    }

    /*
     * Generic fallback for all non-80-byte inputs.
     */
    uint8_t first[SHA256_DIGEST_SIZE];

    sha256(data, len, first);
    sha256(first, SHA256_DIGEST_SIZE, out);
}

static bool bytes_equal(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

bool sha256_self_test(void) {
    uint8_t out[SHA256_DIGEST_SIZE];

    /*
     * SHA-256("abc")
     */
    static const uint8_t expected_abc[SHA256_DIGEST_SIZE] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };

    sha256((const uint8_t *)"abc", 3, out);

    if (!bytes_equal(out, expected_abc, SHA256_DIGEST_SIZE)) {
        return false;
    }

    /*
     * Bitcoin genesis block header double-SHA256 test.
     *
     * Displayed block hash:
     * 000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
     *
     * Raw SHA-256 digest byte order:
     * 6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000
     *
     * This also tests the optimized 80-byte fast path.
     */
    static const uint8_t genesis_header[80] = {
        0x01, 0x00, 0x00, 0x00,

        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,

        0x3b, 0xa3, 0xed, 0xfd,
        0x7a, 0x7b, 0x12, 0xb2,
        0x7a, 0xc7, 0x2c, 0x3e,
        0x67, 0x76, 0x8f, 0x61,
        0x7f, 0xc8, 0x1b, 0xc3,
        0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa,
        0x4b, 0x1e, 0x5e, 0x4a,

        0x29, 0xab, 0x5f, 0x49,
        0xff, 0xff, 0x00, 0x1d,
        0x1d, 0xac, 0x2b, 0x7c
    };

    static const uint8_t expected_genesis_raw[SHA256_DIGEST_SIZE] = {
        0x6f, 0xe2, 0x8c, 0x0a,
        0xb6, 0xf1, 0xb3, 0x72,
        0xc1, 0xa6, 0xa2, 0x46,
        0xae, 0x63, 0xf7, 0x4f,
        0x93, 0x1e, 0x83, 0x65,
        0xe1, 0x5a, 0x08, 0x9c,
        0x68, 0xd6, 0x19, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    double_sha256(genesis_header, sizeof(genesis_header), out);

    if (!bytes_equal(out, expected_genesis_raw, SHA256_DIGEST_SIZE)) {
        return false;
    }

    return true;
}