#include "bitcoin_hex.h"

#include <stdio.h>
#include <string.h>

static bool hex_char_to_nibble(char c, uint8_t *out) {
    if (c >= '0' && c <= '9') {
        *out = (uint8_t)(c - '0');
        return true;
    }

    if (c >= 'a' && c <= 'f') {
        *out = (uint8_t)(10 + c - 'a');
        return true;
    }

    if (c >= 'A' && c <= 'F') {
        *out = (uint8_t)(10 + c - 'A');
        return true;
    }

    return false;
}

bool bitcoin_hex_to_bytes(
    const char *hex,
    uint8_t *out,
    size_t out_size,
    size_t *bytes_written
) {
    if (hex == NULL || out == NULL) {
        return false;
    }

    size_t hex_len = strlen(hex);

    if ((hex_len % 2) != 0) {
        return false;
    }

    size_t byte_len = hex_len / 2;

    if (byte_len > out_size) {
        return false;
    }

    for (size_t i = 0; i < byte_len; i++) {
        uint8_t high = 0;
        uint8_t low = 0;

        if (!hex_char_to_nibble(hex[i * 2], &high)) {
            return false;
        }

        if (!hex_char_to_nibble(hex[i * 2 + 1], &low)) {
            return false;
        }

        out[i] = (uint8_t)((high << 4) | low);
    }

    if (bytes_written != NULL) {
        *bytes_written = byte_len;
    }

    return true;
}

bool bitcoin_hex_to_bytes_exact(
    const char *hex,
    uint8_t *out,
    size_t expected_len
) {
    size_t written = 0;

    if (!bitcoin_hex_to_bytes(hex, out, expected_len, &written)) {
        return false;
    }

    return written == expected_len;
}

void bitcoin_reverse_bytes(uint8_t *bytes, size_t len) {
    if (bytes == NULL) {
        return;
    }

    for (size_t i = 0; i < len / 2; i++) {
        uint8_t tmp = bytes[i];
        bytes[i] = bytes[len - 1 - i];
        bytes[len - 1 - i] = tmp;
    }
}

void bitcoin_write_u32_le(uint8_t out[4], uint32_t value) {
    out[0] = (uint8_t)(value & 0xff);
    out[1] = (uint8_t)((value >> 8) & 0xff);
    out[2] = (uint8_t)((value >> 16) & 0xff);
    out[3] = (uint8_t)((value >> 24) & 0xff);
}

bool bitcoin_read_hex_u32(const char *hex, uint32_t *out) {
    if (hex == NULL || out == NULL) {
        return false;
    }

    if (strlen(hex) != 8) {
        return false;
    }

    uint8_t bytes[4];

    if (!bitcoin_hex_to_bytes_exact(hex, bytes, sizeof(bytes))) {
        return false;
    }

    *out =
        ((uint32_t)bytes[0] << 24) |
        ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] << 8) |
        ((uint32_t)bytes[3]);

    return true;
}

bool bitcoin_hex_self_test(void) {
    uint8_t bytes[4];

    if (!bitcoin_hex_to_bytes_exact("00ff10aB", bytes, sizeof(bytes))) {
        printf("bitcoin_hex_self_test: hex decode failed\n");
        return false;
    }

    if (
        bytes[0] != 0x00 ||
        bytes[1] != 0xff ||
        bytes[2] != 0x10 ||
        bytes[3] != 0xab
    ) {
        printf("bitcoin_hex_self_test: decoded bytes wrong\n");
        return false;
    }

    bitcoin_reverse_bytes(bytes, sizeof(bytes));

    if (
        bytes[0] != 0xab ||
        bytes[1] != 0x10 ||
        bytes[2] != 0xff ||
        bytes[3] != 0x00
    ) {
        printf("bitcoin_hex_self_test: reverse failed\n");
        return false;
    }

    uint32_t value = 0;

    if (!bitcoin_read_hex_u32("1d00ffff", &value)) {
        printf("bitcoin_hex_self_test: read hex u32 failed\n");
        return false;
    }

    if (value != 0x1d00ffffu) {
        printf("bitcoin_hex_self_test: read hex u32 wrong\n");
        return false;
    }

    uint8_t le[4];
    bitcoin_write_u32_le(le, value);

    if (
        le[0] != 0xff ||
        le[1] != 0xff ||
        le[2] != 0x00 ||
        le[3] != 0x1d
    ) {
        printf("bitcoin_hex_self_test: write u32 le failed\n");
        return false;
    }

    printf("bitcoin_hex_self_test: PASS\n");
    return true;
}