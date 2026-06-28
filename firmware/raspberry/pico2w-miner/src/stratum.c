#include "stratum.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratum_tcp.h"

#define STRATUM_SUBSCRIBE_ID 1
#define STRATUM_AUTHORIZE_ID 2
#define STRATUM_SUBMIT_ID 3

static stratum_state_t stratum_state;

static void stratum_reset_job(void) {
    stratum_state.has_job = false;

    stratum_state.job_id[0] = '\0';
    stratum_state.prevhash[0] = '\0';
    stratum_state.coinb1[0] = '\0';
    stratum_state.coinb2[0] = '\0';

    stratum_state.merkle_branch_count = 0;

    for (uint8_t i = 0; i < STRATUM_MAX_MERKLE_BRANCHES; i++) {
        stratum_state.merkle_branches[i][0] = '\0';
    }

    stratum_state.version[0] = '\0';
    stratum_state.nbits[0] = '\0';
    stratum_state.ntime[0] = '\0';

    stratum_state.clean_jobs = false;
}

static void stratum_reset_state(void) {
    memset(&stratum_state, 0, sizeof(stratum_state));
}

static bool build_worker_username(
    const char *btc_address,
    const char *worker_name,
    char *out,
    size_t out_size
) {
    if (btc_address == NULL || btc_address[0] == '\0' || out == NULL || out_size == 0) {
        return false;
    }

    if (worker_name == NULL || worker_name[0] == '\0') {
        worker_name = "pico2w";
    }

    int written = snprintf(
        out,
        out_size,
        "%s.%s",
        btc_address,
        worker_name
    );

    return written > 0 && written < (int)out_size;
}

static bool line_has_id(const char *line, int id) {
    char pattern_a[24];
    char pattern_b[24];

    snprintf(pattern_a, sizeof(pattern_a), "\"id\":%d", id);
    snprintf(pattern_b, sizeof(pattern_b), "\"id\": %d", id);

    return strstr(line, pattern_a) != NULL || strstr(line, pattern_b) != NULL;
}

static bool line_has_method(const char *line, const char *method) {
    char pattern_a[96];
    char pattern_b[96];

    snprintf(pattern_a, sizeof(pattern_a), "\"method\":\"%s\"", method);
    snprintf(pattern_b, sizeof(pattern_b), "\"method\": \"%s\"", method);

    return strstr(line, pattern_a) != NULL || strstr(line, pattern_b) != NULL;
}

static bool copy_next_quoted_string(const char **cursor, char *out, size_t out_size) {
    if (cursor == NULL || *cursor == NULL || out == NULL || out_size == 0) {
        return false;
    }

    const char *p = *cursor;

    while (*p != '\0' && *p != '"') {
        p++;
    }

    if (*p != '"') {
        return false;
    }

    p++;

    size_t len = 0;

    while (*p != '\0' && *p != '"') {
        if (*p == '\\' && p[1] != '\0') {
            p++;
        }

        if (len + 1 < out_size) {
            out[len++] = *p;
        }

        p++;
    }

    if (*p != '"') {
        return false;
    }

    out[len] = '\0';
    *cursor = p + 1;

    return true;
}

static bool read_next_uint(const char **cursor, uint32_t *out) {
    if (cursor == NULL || *cursor == NULL || out == NULL) {
        return false;
    }

    const char *p = *cursor;

    while (*p != '\0' && !isdigit((unsigned char)*p)) {
        p++;
    }

    if (*p == '\0') {
        return false;
    }

    char *endptr = NULL;
    unsigned long value = strtoul(p, &endptr, 10);

    if (endptr == p) {
        return false;
    }

    *out = (uint32_t)value;
    *cursor = endptr;

    return true;
}

static bool copy_next_number_text(const char **cursor, char *out, size_t out_size) {
    if (cursor == NULL || *cursor == NULL || out == NULL || out_size == 0) {
        return false;
    }

    const char *p = *cursor;

    while (
        *p != '\0' &&
        !isdigit((unsigned char)*p) &&
        *p != '-' &&
        *p != '+'
    ) {
        p++;
    }

    if (*p == '\0') {
        return false;
    }

    size_t len = 0;

    while (
        *p != '\0' &&
        (
            isdigit((unsigned char)*p) ||
            *p == '.' ||
            *p == '-' ||
            *p == '+' ||
            *p == 'e' ||
            *p == 'E'
        )
    ) {
        if (len + 1 < out_size) {
            out[len++] = *p;
        }

        p++;
    }

    out[len] = '\0';
    *cursor = p;

    return len > 0;
}

static bool parse_submit_response(const char *line) {
    if (!line_has_id(line, STRATUM_SUBMIT_ID)) {
        return false;
    }

    if (strstr(line, "\"result\":true") != NULL ||
        strstr(line, "\"result\": true") != NULL) {
        printf("STRATUM: share accepted by pool\n");
        return true;
    }

    if (strstr(line, "\"result\":false") != NULL ||
        strstr(line, "\"result\": false") != NULL) {
        printf("STRATUM: share rejected by pool\n");
        printf("STRATUM: reject line: %s\n", line);
        return true;
    }

    printf("STRATUM: submit response received, result unclear\n");
    printf("STRATUM: submit line: %s\n", line);

    return true;
}

static const char *find_matching_square_bracket(const char *open_bracket) {
    if (open_bracket == NULL || *open_bracket != '[') {
        return NULL;
    }

    int depth = 0;
    bool in_string = false;
    bool escape = false;

    for (const char *p = open_bracket; *p != '\0'; p++) {
        char c = *p;

        if (escape) {
            escape = false;
            continue;
        }

        if (in_string && c == '\\') {
            escape = true;
            continue;
        }

        if (c == '"') {
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            continue;
        }

        if (c == '[') {
            depth++;
        } else if (c == ']') {
            depth--;

            if (depth == 0) {
                return p;
            }
        }
    }

    return NULL;
}

static const char *find_second_item_in_result_array(const char *line) {
    const char *result = strstr(line, "\"result\"");
    if (result == NULL) {
        return NULL;
    }

    const char *array_start = strchr(result, '[');
    if (array_start == NULL) {
        return NULL;
    }

    int depth = 0;
    bool in_string = false;
    bool escape = false;

    for (const char *p = array_start; *p != '\0'; p++) {
        char c = *p;

        if (escape) {
            escape = false;
            continue;
        }

        if (in_string && c == '\\') {
            escape = true;
            continue;
        }

        if (c == '"') {
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            continue;
        }

        if (c == '[') {
            depth++;
        } else if (c == ']') {
            depth--;
        } else if (c == ',' && depth == 1) {
            return p + 1;
        }
    }

    return NULL;
}

static bool parse_subscribe_response(const char *line) {
    if (!line_has_id(line, STRATUM_SUBSCRIBE_ID)) {
        return false;
    }

    const char *p = find_second_item_in_result_array(line);
    if (p == NULL) {
        printf("STRATUM: subscribe parse failed, no extranonce1 position\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.extranonce1, sizeof(stratum_state.extranonce1))) {
        printf("STRATUM: subscribe parse failed, no extranonce1\n");
        return false;
    }

    if (!read_next_uint(&p, &stratum_state.extranonce2_size)) {
        printf("STRATUM: subscribe parse failed, no extranonce2_size\n");
        return false;
    }

    stratum_state.subscribed = true;

    printf("STRATUM: subscribed\n");
    printf("STRATUM: extranonce1=%s\n", stratum_state.extranonce1);
    printf("STRATUM: extranonce2_size=%lu\n", (unsigned long)stratum_state.extranonce2_size);

    return true;
}

static bool parse_authorize_response(const char *line) {
    if (!line_has_id(line, STRATUM_AUTHORIZE_ID)) {
        return false;
    }

    if (strstr(line, "\"result\":true") != NULL || strstr(line, "\"result\": true") != NULL) {
        stratum_state.authorized = true;
        printf("STRATUM: authorized successfully\n");
        return true;
    }

    if (strstr(line, "\"result\":false") != NULL || strstr(line, "\"result\": false") != NULL) {
        stratum_state.authorized = false;
        printf("STRATUM: authorization rejected\n");
        return true;
    }

    printf("STRATUM: authorize response received, but result was unclear\n");
    return false;
}

static bool parse_set_difficulty(const char *line) {
    if (!line_has_method(line, "mining.set_difficulty")) {
        return false;
    }

    const char *params = strstr(line, "\"params\"");
    if (params == NULL) {
        printf("STRATUM: difficulty parse failed, no params\n");
        return false;
    }

    const char *p = params;

    if (!copy_next_number_text(&p, stratum_state.difficulty_text, sizeof(stratum_state.difficulty_text))) {
        printf("STRATUM: difficulty parse failed, no value\n");
        return false;
    }

    stratum_state.has_difficulty = true;

    printf("STRATUM: difficulty=%s\n", stratum_state.difficulty_text);

    return true;
}

static bool parse_merkle_branch_array(const char **cursor) {
    if (cursor == NULL || *cursor == NULL) {
        return false;
    }

    const char *branch_start = strchr(*cursor, '[');
    if (branch_start == NULL) {
        return false;
    }

    const char *branch_end = find_matching_square_bracket(branch_start);
    if (branch_end == NULL) {
        return false;
    }

    const char *p = branch_start + 1;

    uint8_t count = 0;

    while (p < branch_end && count < STRATUM_MAX_MERKLE_BRANCHES) {
        while (p < branch_end && *p != '"') {
            p++;
        }

        if (p >= branch_end) {
            break;
        }

        const char *temp = p;

        if (!copy_next_quoted_string(&temp, stratum_state.merkle_branches[count], sizeof(stratum_state.merkle_branches[count]))) {
            break;
        }

        count++;
        p = temp;
    }

    stratum_state.merkle_branch_count = count;
    *cursor = branch_end + 1;

    return true;
}

static bool parse_clean_jobs_bool(const char *p) {
    const char *params_end = strchr(p, ']');

    const char *true_pos = strstr(p, "true");
    const char *false_pos = strstr(p, "false");

    if (true_pos != NULL && (params_end == NULL || true_pos < params_end)) {
        return true;
    }

    if (false_pos != NULL && (params_end == NULL || false_pos < params_end)) {
        return false;
    }

    return false;
}

static bool parse_notify(const char *line) {
    if (!line_has_method(line, "mining.notify")) {
        return false;
    }

    const char *params = strstr(line, "\"params\"");
    if (params == NULL) {
        printf("STRATUM: notify parse failed, no params\n");
        return false;
    }

    const char *p = strchr(params, '[');
    if (p == NULL) {
        printf("STRATUM: notify parse failed, no params array\n");
        return false;
    }

    p++;

    stratum_reset_job();

    if (!copy_next_quoted_string(&p, stratum_state.job_id, sizeof(stratum_state.job_id))) {
        printf("STRATUM: notify parse failed, no job_id\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.prevhash, sizeof(stratum_state.prevhash))) {
        printf("STRATUM: notify parse failed, no prevhash\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.coinb1, sizeof(stratum_state.coinb1))) {
        printf("STRATUM: notify parse failed, no coinb1\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.coinb2, sizeof(stratum_state.coinb2))) {
        printf("STRATUM: notify parse failed, no coinb2\n");
        return false;
    }

    if (!parse_merkle_branch_array(&p)) {
        printf("STRATUM: notify parse failed, no merkle branch array\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.version, sizeof(stratum_state.version))) {
        printf("STRATUM: notify parse failed, no version\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.nbits, sizeof(stratum_state.nbits))) {
        printf("STRATUM: notify parse failed, no nbits\n");
        return false;
    }

    if (!copy_next_quoted_string(&p, stratum_state.ntime, sizeof(stratum_state.ntime))) {
        printf("STRATUM: notify parse failed, no ntime\n");
        return false;
    }

    stratum_state.clean_jobs = parse_clean_jobs_bool(p);
    stratum_state.has_job = true;
    stratum_state.job_sequence++;

    printf("STRATUM: mining job received\n");
    printf("STRATUM: job_id=%s\n", stratum_state.job_id);
    printf("STRATUM: prevhash=%s\n", stratum_state.prevhash);
    printf("STRATUM: version=%s\n", stratum_state.version);
    printf("STRATUM: nbits=%s\n", stratum_state.nbits);
    printf("STRATUM: ntime=%s\n", stratum_state.ntime);
    printf("STRATUM: merkle branches=%u\n", stratum_state.merkle_branch_count);
    printf("STRATUM: clean_jobs=%s\n", stratum_state.clean_jobs ? "true" : "false");

    return true;
}

bool stratum_send_subscribe(void) {
    const char *message =
        "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"pico2w-miner/0.1\"]}";

    printf("STRATUM: sending mining.subscribe\n");

    return stratum_tcp_send_json_line(message);
}

bool stratum_send_authorize(
    const char *btc_address,
    const char *worker_name,
    const char *pool_password
) {
    if (btc_address == NULL || btc_address[0] == '\0') {
        printf("STRATUM: missing BTC address\n");
        return false;
    }

    if (pool_password == NULL || pool_password[0] == '\0') {
        pool_password = "x";
    }

    char username[160];

    if (!build_worker_username(
            btc_address,
            worker_name,
            username,
            sizeof(username)
        )) {
        printf("STRATUM: username too long\n");
        return false;
    }

    char message[320];

    int written = snprintf(
        message,
        sizeof(message),
        "{\"id\":%d,\"method\":\"mining.authorize\",\"params\":[\"%s\",\"%s\"]}",
        STRATUM_AUTHORIZE_ID,
        username,
        pool_password
    );

    if (written < 0 || written >= (int)sizeof(message)) {
        printf("STRATUM: authorize message too long\n");
        return false;
    }

    printf("STRATUM: sending mining.authorize as %s\n", username);

    return stratum_tcp_send_json_line(message);
}

bool stratum_send_submit(
    const char *btc_address,
    const char *worker_name,
    const char *job_id,
    const char *extranonce2_hex,
    const char *ntime_hex,
    uint32_t nonce
) {
    if (
        job_id == NULL || job_id[0] == '\0' ||
        extranonce2_hex == NULL || extranonce2_hex[0] == '\0' ||
        ntime_hex == NULL || ntime_hex[0] == '\0'
    ) {
        printf("STRATUM: cannot submit, missing submit data\n");
        return false;
    }

    char username[160];

    if (!build_worker_username(
            btc_address,
            worker_name,
            username,
            sizeof(username)
        )) {
        printf("STRATUM: cannot submit, username too long\n");
        return false;
    }

    /*
     * Submit the exact 4 nonce bytes used in the block header.
     * The header writes nonce little-endian, so nonce=1 becomes 01000000.
     */
    char nonce_hex[9];

    snprintf(
        nonce_hex,
        sizeof(nonce_hex),
        "%02x%02x%02x%02x",
        (unsigned)(nonce & 0xff),
        (unsigned)((nonce >> 8) & 0xff),
        (unsigned)((nonce >> 16) & 0xff),
        (unsigned)((nonce >> 24) & 0xff)
    );

    char message[384];

    int written = snprintf(
        message,
        sizeof(message),
        "{\"id\":3,\"method\":\"mining.submit\",\"params\":[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]}",
        username,
        job_id,
        extranonce2_hex,
        ntime_hex,
        nonce_hex
    );

    if (written < 0 || written >= (int)sizeof(message)) {
        printf("STRATUM: submit message too long\n");
        return false;
    }

    printf("STRATUM: submitting share\n");
    printf("STRATUM: job_id=%s extranonce2=%s ntime=%s nonce=%s\n",
           job_id,
           extranonce2_hex,
           ntime_hex,
           nonce_hex);

    return stratum_tcp_send_json_line(message);
}

bool stratum_start_session(
    const char *btc_address,
    const char *worker_name,
    const char *pool_password
) {
    stratum_reset_state();

    stratum_tcp_set_line_callback(stratum_handle_line);

    bool subscribe_ok = stratum_send_subscribe();
    if (!subscribe_ok) {
        printf("STRATUM: failed to send mining.subscribe\n");
        return false;
    }

    bool authorize_ok = stratum_send_authorize(
        btc_address,
        worker_name,
        pool_password
    );

    if (!authorize_ok) {
        printf("STRATUM: failed to send mining.authorize\n");
        return false;
    }

    printf("STRATUM: subscribe and authorize messages sent\n");

    return true;
}

void stratum_handle_line(const char *line) {
    if (line == NULL || line[0] == '\0') {
        return;
    }

    printf("STRATUM LINE: %s\n", line);

    if (parse_subscribe_response(line)) {
        return;
    }

    if (parse_authorize_response(line)) {
        return;
    }

    if (parse_submit_response(line)) {
        return;
    }

    if (parse_set_difficulty(line)) {
        return;
    }

    if (parse_notify(line)) {
        return;
    }

    printf("STRATUM: unhandled line\n");
}

const stratum_state_t *stratum_get_state(void) {
    return &stratum_state;
}

void stratum_print_state(void) {
    printf("========== STRATUM STATE ==========\n");
    printf("subscribed: %s\n", stratum_state.subscribed ? "yes" : "no");
    printf("authorized: %s\n", stratum_state.authorized ? "yes" : "no");

    printf("extranonce1: %s\n", stratum_state.extranonce1);
    printf("extranonce2_size: %lu\n", (unsigned long)stratum_state.extranonce2_size);

    printf("has_difficulty: %s\n", stratum_state.has_difficulty ? "yes" : "no");
    printf("difficulty: %s\n", stratum_state.difficulty_text);

    printf("has_job: %s\n", stratum_state.has_job ? "yes" : "no");

    if (stratum_state.has_job) {
        printf("job_id: %s\n", stratum_state.job_id);
        printf("prevhash: %s\n", stratum_state.prevhash);
        printf("coinb1 length: %u\n", (unsigned)strlen(stratum_state.coinb1));
        printf("coinb2 length: %u\n", (unsigned)strlen(stratum_state.coinb2));
        printf("merkle branches: %u\n", stratum_state.merkle_branch_count);
        printf("version: %s\n", stratum_state.version);
        printf("nbits: %s\n", stratum_state.nbits);
        printf("ntime: %s\n", stratum_state.ntime);
        printf("clean_jobs: %s\n", stratum_state.clean_jobs ? "true" : "false");
    }

    printf("===================================\n");
}