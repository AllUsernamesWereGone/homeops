#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SHA256_DIGEST_SIZE 32

void sha256(const uint8_t *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE]);

void double_sha256(const uint8_t *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE]);

bool sha256_self_test(void);

#endif