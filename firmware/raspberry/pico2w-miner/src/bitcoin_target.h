#ifndef BITCOIN_TARGET_H
#define BITCOIN_TARGET_H

#include <stdbool.h>
#include <stdint.h>

#define BITCOIN_TARGET_SIZE 32

typedef struct {
    uint8_t bytes[BITCOIN_TARGET_SIZE]; // big-endian display/comparison order
} bitcoin_target_t;

void bitcoin_target_from_nbits(uint32_t nbits, bitcoin_target_t *target);

bool bitcoin_hash_meets_target(
    const uint8_t raw_hash[BITCOIN_TARGET_SIZE],
    const bitcoin_target_t *target
);

bool bitcoin_target_self_test(void);

bool bitcoin_target_from_difficulty_u32(
    uint32_t difficulty,
    bitcoin_target_t *target
);

void bitcoin_target_print(
    const char *label,
    const bitcoin_target_t *target
);

#endif