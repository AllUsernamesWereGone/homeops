#include "bitcoin_target.h"

#include <string.h>
#include <stdio.h>
#include <string.h>

void bitcoin_target_from_nbits(uint32_t nbits, bitcoin_target_t *target) {
    // Clear the full 256-bit target before writing the decoded compact value.
    memset(target->bytes, 0, BITCOIN_TARGET_SIZE);

    // Bitcoin stores difficulty target in compact form:
    // highest byte = exponent, lower 23 bits = mantissa.
    uint8_t exponent = (uint8_t)(nbits >> 24);
    uint32_t mantissa = nbits & 0x007fffffUL;

    // Store mantissa in big-endian byte order because target->bytes is compared
    // as a normal 256-bit big-endian number.
    uint8_t mantissa_bytes[3] = {
        (uint8_t)(mantissa >> 16),
        (uint8_t)(mantissa >> 8),
        (uint8_t)(mantissa)
    };

    // Very small targets have exponent <= 3, meaning the mantissa must be shifted right.
    if (exponent <= 3) {
        mantissa >>= 8 * (3 - exponent);

        target->bytes[31] = (uint8_t)(mantissa);

        if (mantissa > 0xff) {
            target->bytes[30] = (uint8_t)(mantissa >> 8);
        }

        if (mantissa > 0xffff) {
            target->bytes[29] = (uint8_t)(mantissa >> 16);
        }

        return;
    }

    // For normal Bitcoin targets, the exponent tells where the 3-byte mantissa starts
    // inside the 32-byte target.
    int start = BITCOIN_TARGET_SIZE - exponent;

    if (start < 0) {
        start = 0;
    }

    for (int i = 0; i < 3; i++) {
        int index = start + i;

        if (index >= 0 && index < BITCOIN_TARGET_SIZE) {
            target->bytes[index] = mantissa_bytes[i];
        }
    }
}

bool bitcoin_target_from_difficulty_u32(
    uint32_t difficulty,
    bitcoin_target_t *target
) {
    if (target == NULL || difficulty == 0) {
        return false;
    }

    /*
     * Bitcoin difficulty-1 target:
     * 00000000ffff0000000000000000000000000000000000000000000000000000
     *
     * Pool share target = difficulty_1_target / pool_difficulty
     */
    static const uint8_t difficulty_1_target[32] = {
        0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    uint32_t remainder = 0;

    for (size_t i = 0; i < 32; i++) {
        uint64_t value = ((uint64_t)remainder << 8) | difficulty_1_target[i];

        target->bytes[i] = (uint8_t)(value / difficulty);
        remainder = (uint32_t)(value % difficulty);
    }

    return true;
}

void bitcoin_target_print(
    const char *label,
    const bitcoin_target_t *target
) {
    if (target == NULL) {
        return;
    }

    if (label != NULL) {
        printf("%s", label);
    }

    for (size_t i = 0; i < BITCOIN_TARGET_SIZE; i++) {
        printf("%02x", target->bytes[i]);
    }

    printf("\n");
}

bool bitcoin_hash_meets_target(const uint8_t raw_hash[BITCOIN_TARGET_SIZE],
                               const bitcoin_target_t *target) {
    uint8_t display_order_hash[BITCOIN_TARGET_SIZE];

    // SHA-256 output is stored in raw byte order.
    // Bitcoin block hashes are normally displayed and compared as the reversed byte order.
    for (int i = 0; i < BITCOIN_TARGET_SIZE; i++) {
        display_order_hash[i] = raw_hash[BITCOIN_TARGET_SIZE - 1 - i];
    }

    // Compare hash and target as 256-bit big-endian numbers.
    // The hash is valid if hash <= target.
    for (int i = 0; i < BITCOIN_TARGET_SIZE; i++) {
        if (display_order_hash[i] < target->bytes[i]) {
            return true;
        }

        if (display_order_hash[i] > target->bytes[i]) {
            return false;
        }
    }

    // Equal is also valid.
    return true;
}

bool bitcoin_target_self_test(void) {
    bitcoin_target_t target;

    // Decode the compact target from the Bitcoin genesis block.
    bitcoin_target_from_nbits(0x1d00ffffUL, &target);

    static const uint8_t expected_target[BITCOIN_TARGET_SIZE] = {
        0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    for (int i = 0; i < BITCOIN_TARGET_SIZE; i++) {
        if (target.bytes[i] != expected_target[i]) {
            return false;
        }
    }

    // Raw byte order of the Bitcoin genesis block hash.
    // Display order would be:
    // 000000000019d6689c085ae165831e93...
    static const uint8_t genesis_raw_hash[BITCOIN_TARGET_SIZE] = {
        0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
        0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
        0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
        0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    return bitcoin_hash_meets_target(genesis_raw_hash, &target);
}