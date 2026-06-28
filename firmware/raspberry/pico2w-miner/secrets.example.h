#ifndef SECRETS_EXAMPLE_H
#define SECRETS_EXAMPLE_H

#define BTC_ADDRESS "bc1q-your-bitcoin-address-here"

#define STRATUM_HOST "eusolo.ckpool.org"
#define STRATUM_PORT 3333

#define WORKER_NAME "pico2w"
#define POOL_PASSWORD "x"

#define DISPLAY_BRIGHTNESS_PERCENT 35

typedef struct {
    const char *ssid;
    const char *password;
} wifi_network_secret_t;

#define WIFI_NETWORK_COUNT 3

static const wifi_network_secret_t WIFI_NETWORKS[WIFI_NETWORK_COUNT] = {
    {"WIFI_NAME", "WIFI_password"},
    {"WIFI_NAME2", "WIFI_password2"},
    {"WIFI_NAME3", "WIFI_password3"}
};

#endif