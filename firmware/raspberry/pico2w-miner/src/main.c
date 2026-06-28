/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"

#include "miner.h"
#include "sha256.h"
#include "bitcoin_job.h"
#include "bitcoin_target.h"
#include "bitcoin_hex.h"
#include "display_st7735.h"
#include "display_ui.h"
#include "wifi.h"
#include "stratum_tcp.h"
#include "stratum.h"
#include "mqtt_homeops.h"

#include "../secrets.h"

#include <stdio.h>

#define SHARE_BLINK_INTERVAL_MS 10000
#define SHARE_BLINK_ON_MS 120

#define MINER_RUN_SLICE_MS 1000
#define DISPLAY_UPDATE_INTERVAL_MS 10000
#define SERIAL_STATS_INTERVAL_MS 10000

#define ENABLE_SERIAL_STATS 1 // 0 has ~18400; 1 has ~17400

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250

#define HOMEOPS_MQTT_CONNECT_TIMEOUT_MS 15000
#define HOMEOPS_MQTT_RECONNECT_INTERVAL_MS 30000


#endif

// Initialize the onboard LED.
// Normal Pico boards expose the LED as a regular GPIO.
// Pico W / Pico 2 W boards expose it through the CYW43 WiFi chip.
int pico_led_init(void) {
    #if defined(PICO_DEFAULT_LED_PIN)
        // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
        // so we can use normal GPIO functionality to turn the led on and off
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
        return PICO_OK;
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        // For Pico W devices we need to initialise the driver etc
        return PICO_OK;
    #else
        return PICO_ERROR_GENERIC;
    #endif
}

// Set the onboard LED state.
// This hides the hardware difference between normal Pico and Pico W / Pico 2 W.
void pico_set_led(bool led_on) {
    #if defined(PICO_DEFAULT_LED_PIN)
        // Just set the GPIO on or off
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        // Ask the wifi "driver" to set the GPIO on or off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    #endif
}

bool self_tests(){

    // Verify that the SHA-256 implementation works before mining.
    // If this fails, every produced hash would be invalid.
    if (sha256_self_test()) {
        printf("SHA-256 self-test passed\n");
    } else {
        printf("SHA-256 self-test FAILED\n");
        return false;
    }

    // Verify that double-SHA256 of the Bitcoin genesis header produces the expected hash.
    // This catches mistakes in block-header layout, nonce byte order, or SHA-256 usage.
    if (miner_genesis_header_self_test()) {
        printf("Genesis block header test passed\n");
    } else {
        printf("Genesis block header test FAILED\n");
        return false;
    }

    // Verify compact Bitcoin target decoding and hash-vs-target comparison.
    if (bitcoin_target_self_test()) {
        printf("Bitcoin target self-test passed\n");
    } else {
        printf("Bitcoin target self-test FAILED\n");
        return false;
    }

    if (!bitcoin_hex_self_test()) {
        printf("bitcoin_hex_self_test failed\n");
        return false;
    } else {
        printf("bitcoin_hex_self_test passed\n");
    }



    return true;
}

static void pico_led_set(bool on) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
#else
    (void)on;
#endif
}

static void update_share_led_blink(bool share_found_latched) {
    static absolute_time_t next_blink_time;
    static absolute_time_t led_off_time;
    static bool initialized = false;
    static bool led_is_on = false;

    if (!initialized) {
        next_blink_time = make_timeout_time_ms(SHARE_BLINK_INTERVAL_MS);
        led_off_time = get_absolute_time();
        led_is_on = false;
        pico_led_set(false);
        initialized = true;
    }

    if (!share_found_latched) {
        pico_led_set(false);
        led_is_on = false;
        next_blink_time = make_timeout_time_ms(SHARE_BLINK_INTERVAL_MS);
        return;
    }

    if (!led_is_on && time_reached(next_blink_time)) {
        pico_led_set(true);
        led_is_on = true;
        led_off_time = make_timeout_time_ms(SHARE_BLINK_ON_MS);
        next_blink_time = make_timeout_time_ms(SHARE_BLINK_INTERVAL_MS);
    }

    if (led_is_on && time_reached(led_off_time)) {
        pico_led_set(false);
        led_is_on = false;
    }
}


static uint32_t count_target_leading_zero_bits(const bitcoin_target_t *target) {
    if (target == NULL) {
        return 0;
    }

    uint32_t zero_bits = 0;

    for (int i = 0; i < BITCOIN_TARGET_SIZE; i++) {
        uint8_t byte = target->bytes[i];

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

static uint32_t get_approx_needed_bits(const bitcoin_job_t *job) {
    if (job == NULL) {
        return 0;
    }

    if (job->has_share_target) {
        return count_target_leading_zero_bits(&job->share_target);
    }

    return job->target_zero_bits;
}


int main() {
    stdio_init_all();

    // Give the USB serial connection time to become available after boot.
    sleep_ms(5000);


    if(!self_tests()){
        while (true){
            printf("Self tests failed.\n");
            sleep_ms(2000);
        }
    }
    printf("Pico 2 W Bitcoin Self tests passed\n");


    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    display_init();
    display_set_brightness_percent(DISPLAY_BRIGHTNESS_PERCENT);
    display_ui_show_boot();

    printf("Display initialized\n");

    printf("WiFi: starting connection attempts\n");

    bool wifi_connected = wifi_connect_to_first_available();

    bool mqtt_connected = false;
    absolute_time_t next_mqtt_reconnect = make_timeout_time_ms(1000);

    if (wifi_connected) {
        mqtt_connected = mqtt_homeops_connect_blocking(HOMEOPS_MQTT_CONNECT_TIMEOUT_MS);

        if (!mqtt_connected) {
            next_mqtt_reconnect = make_timeout_time_ms(HOMEOPS_MQTT_RECONNECT_INTERVAL_MS);
        }
    }

    if (wifi_connected) {
        printf("Pool: connecting to %s:%d\n", STRATUM_HOST, STRATUM_PORT);

        bool pool_connected = stratum_tcp_connect_blocking(
            STRATUM_HOST,
            STRATUM_PORT,
            15000
        );

        if (pool_connected) {
            printf("Pool: TCP connected\n");

            bool stratum_started = stratum_start_session(
                BTC_ADDRESS,
                WORKER_NAME,
                POOL_PASSWORD
            );

            if (stratum_started) {
                printf("Pool: Stratum session started\n");
            } else {
                printf("Pool: Stratum session failed to start\n");
            }
        } else {
            printf("Pool: TCP connection failed, continuing local mining only\n");
        }
    }


    // Holds runtime mining statistics such as nonce, hashrate, best hash, and shares.
    miner_state_t miner;
    miner_init(&miner);

    // Demo mining job.
    // This is currently not a real pool job. It uses a fixed demo block header.
    bitcoin_job_t job;
    memset(&job, 0, sizeof(job));

    bool has_active_job = false;


    char hash_hex[MINER_HASH_HEX_SIZE];

    uint32_t extranonce2_counter = 0;
    uint32_t loaded_job_sequence = 0;

    absolute_time_t next_display_update = make_timeout_time_ms(1000);
    absolute_time_t next_serial_stats_update = make_timeout_time_ms(SERIAL_STATS_INTERVAL_MS);

    uint64_t serial_hash_accumulator = 0;
    absolute_time_t serial_window_start = get_absolute_time();

    bool real_share_latched = false;


    while (true) {
        const stratum_state_t *state = stratum_get_state();

        if (wifi_connected && !mqtt_homeops_is_connected() && time_reached(next_mqtt_reconnect)) {
            printf("HOMEOPS MQTT: reconnect attempt\n");

            mqtt_connected = mqtt_homeops_connect_blocking(HOMEOPS_MQTT_CONNECT_TIMEOUT_MS);

            if (!mqtt_connected) {
                next_mqtt_reconnect = make_timeout_time_ms(HOMEOPS_MQTT_RECONNECT_INTERVAL_MS);
            }
        }

        if (
            state->subscribed &&
            state->authorized &&
            state->has_job &&
            state->job_sequence != loaded_job_sequence
        ) {
            printf("Main: new Stratum job available, building real mining header\n");

            uint32_t active_extranonce2 = extranonce2_counter++;

            bool built = bitcoin_job_from_stratum(
                &job,
                state,
                active_extranonce2
            );

            if (built) {
                loaded_job_sequence = state->job_sequence;
                has_active_job = true;

                miner_init(&miner);

                printf("Main: switched to Stratum job sequence %lu\n",
                    (unsigned long)loaded_job_sequence);
            } else {
                printf("Main: failed to build Stratum job, continuing previous job\n");
            }
        }

        if (!has_active_job || !job.has_share_target) {
        if (time_reached(next_display_update)) {
            display_ui_update_simple(&miner, &job, "WAIT", 0);
            next_display_update = make_timeout_time_ms(DISPLAY_UPDATE_INTERVAL_MS);
        }

            sleep_ms(100);
            continue;
        }

        miner_run_for_ms(&miner, &job, MINER_RUN_SLICE_MS);
        serial_hash_accumulator += miner.hashes_done;

        if (miner.real_share_found_in_window) {
            real_share_latched = true;
        }

        update_share_led_blink(real_share_latched);

        if (miner.submit_share_pending && job.has_submit_data) {
            printf("Main: submitting pending real pool share\n");

            bool submitted = stratum_send_submit(
                BTC_ADDRESS,
                WORKER_NAME,
                job.submit_job_id,
                job.submit_extranonce2,
                job.submit_ntime,
                miner.last_real_share_nonce
            );

            if (submitted) {
                printf("Main: share submitted to pool\n");
                miner.submit_share_pending = false;
            } else {
                printf("Main: share submit failed, will retry later\n");
            }
        }

        if (time_reached(next_display_update)) {
            const char *status = "OK";

            if (!wifi_connected) {
                status = "ERR";
            } else if (!job.has_share_target) {
                status = "WAIT";
            }

            uint32_t needed_bits = get_approx_needed_bits(&job);

            display_ui_update_simple(&miner, &job, status, needed_bits);

            next_display_update = make_timeout_time_ms(DISPLAY_UPDATE_INTERVAL_MS);
        }

        #if ENABLE_SERIAL_STATS
        if (time_reached(next_serial_stats_update)) {

            absolute_time_t now = get_absolute_time();
            int64_t elapsed_us = absolute_time_diff_us(serial_window_start, now);

            uint32_t hashrate = 0;
            if (elapsed_us > 0) {
                hashrate = (uint32_t)(
                    (serial_hash_accumulator * 1000000ull) / (uint64_t)elapsed_us
                );
            }

            uint32_t uptime_seconds = (uint32_t)(to_ms_since_boot(get_absolute_time()) / 1000u);

            mqtt_connected = mqtt_homeops_publish_telemetry(
                hashrate,
                0,
                uptime_seconds,
                miner.shares_found,
                stratum_tcp_is_connected(),
                has_active_job
            );

            if (!mqtt_connected) {
                next_mqtt_reconnect = make_timeout_time_ms(HOMEOPS_MQTT_RECONNECT_INTERVAL_MS);
            }

            miner_hash_to_display_hex(miner.last_hash, hash_hex);

            uint32_t needed_bits = get_approx_needed_bits(&job);

            if (job.has_share_target) {
                printf("SHA256d rate: %lu H/s | Nonce: %lu | Target: real Stratum share target\n",
                    (unsigned long)hashrate,
                    (unsigned long)miner.nonce);
            } else {
                printf("SHA256d rate: %lu H/s | Nonce: %lu | Target: demo %lu zero bits\n",
                    (unsigned long)hashrate,
                    (unsigned long)miner.nonce,
                    (unsigned long)job.target_zero_bits);
            }

            printf("Best: %lu / Needed: %lu bits | Shares: %lu\n",
                (unsigned long)miner.best_zero_bits,
                (unsigned long)needed_bits,
                (unsigned long)miner.shares_found);

            printf("Last hash: %.16s...\n", hash_hex);

            if (miner.real_share_found_in_window) {
                miner_hash_to_display_hex(miner.last_real_share_hash, hash_hex);

                printf("REAL POOL SHARE FOUND! Nonce: %lu\n",
                    (unsigned long)miner.last_real_share_nonce);

                printf("Share hash: %s\n", hash_hex);
            } else if (!job.has_share_target && miner.share_found_in_window) {
                miner_hash_to_display_hex(miner.last_share_hash, hash_hex);

                printf("FOUND TEST SHARE! Nonce: %lu | Zero bits: %lu\n",
                    (unsigned long)miner.last_share_nonce,
                    (unsigned long)miner.last_share_zero_bits);

                printf("Share hash: %s\n", hash_hex);
            }

            printf("\n");

            serial_hash_accumulator = 0;
            serial_window_start = now;
            next_serial_stats_update = make_timeout_time_ms(SERIAL_STATS_INTERVAL_MS);
        }
        #endif
    }
}
