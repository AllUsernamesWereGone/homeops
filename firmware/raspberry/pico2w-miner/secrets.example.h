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


//#define MQTT_HOST "192.168.0.x"
#define HOMEOPS_MQTT_PORT 1883
#define HOMEOPS_MQTT_CLIENT_ID "placeholder"
#define HOMEOPS_MQTT_TOPIC "your path"

/*
 * Broker candidates are tried in order.
 *
 * Preferred:
 *   test-pi
 *   test-pi.local
 *
 * Fallback:
 *   current Pi LAN IP
 *
 * If the Pi IP changes and hostname lookup does not work, only this fallback
 * list needs to change.
 */
#define MQTT_BROKER_COUNT 3

static const char *MQTT_BROKERS[MQTT_BROKER_COUNT] = {
    "test-pi",
    "test-pi.local",
    "192.168.0.x"
};

//#define MQTT_USERNAME "placeholder"
//#define MQTT_PASSWORD "change-me"

#endif
