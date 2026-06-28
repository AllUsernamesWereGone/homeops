#ifndef STRATUM_H
#define STRATUM_H

#include <stdbool.h>
#include <stdint.h>

#define STRATUM_EXTRANONCE1_MAX_LEN 64
#define STRATUM_DIFFICULTY_MAX_LEN 32

#define STRATUM_JOB_ID_MAX_LEN 64
#define STRATUM_PREVHASH_MAX_LEN 80
#define STRATUM_COINBASE_PART_MAX_LEN 2048
#define STRATUM_VERSION_MAX_LEN 16
#define STRATUM_NBITS_MAX_LEN 16
#define STRATUM_NTIME_MAX_LEN 16

#define STRATUM_MAX_MERKLE_BRANCHES 24
#define STRATUM_MERKLE_BRANCH_MAX_LEN 80

typedef struct {
    bool subscribed;
    bool authorized;

    char extranonce1[STRATUM_EXTRANONCE1_MAX_LEN];
    uint32_t extranonce2_size;

    bool has_difficulty;
    char difficulty_text[STRATUM_DIFFICULTY_MAX_LEN];

    bool has_job;
    uint32_t job_sequence;
    char job_id[STRATUM_JOB_ID_MAX_LEN];
    char prevhash[STRATUM_PREVHASH_MAX_LEN];
    char coinb1[STRATUM_COINBASE_PART_MAX_LEN];
    char coinb2[STRATUM_COINBASE_PART_MAX_LEN];

    uint8_t merkle_branch_count;
    char merkle_branches[STRATUM_MAX_MERKLE_BRANCHES][STRATUM_MERKLE_BRANCH_MAX_LEN];

    char version[STRATUM_VERSION_MAX_LEN];
    char nbits[STRATUM_NBITS_MAX_LEN];
    char ntime[STRATUM_NTIME_MAX_LEN];

    bool clean_jobs;
} stratum_state_t;

bool stratum_start_session(
    const char *btc_address,
    const char *worker_name,
    const char *pool_password
);

bool stratum_send_subscribe(void);



bool stratum_send_authorize(
    const char *btc_address,
    const char *worker_name,
    const char *pool_password
);

bool stratum_send_submit(
    const char *btc_address,
    const char *worker_name,
    const char *job_id,
    const char *extranonce2_hex,
    const char *ntime_hex,
    uint32_t nonce
);

void stratum_handle_line(const char *line);

const stratum_state_t *stratum_get_state(void);
void stratum_print_state(void);

#endif