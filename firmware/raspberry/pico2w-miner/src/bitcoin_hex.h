#ifndef BITCOIN_HEX_H
#define BITCOIN_HEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool bitcoin_hex_to_bytes(
    const char *hex,
    uint8_t *out,
    size_t out_size,
    size_t *bytes_written
);

bool bitcoin_hex_to_bytes_exact(
    const char *hex,
    uint8_t *out,
    size_t expected_len
);

void bitcoin_reverse_bytes(uint8_t *bytes, size_t len);

void bitcoin_write_u32_le(uint8_t out[4], uint32_t value);

bool bitcoin_read_hex_u32(const char *hex, uint32_t *out);

bool bitcoin_hex_self_test(void);

#endif