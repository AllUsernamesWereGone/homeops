#include "wifi.h"

#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "../secrets.h"

#define WIFI_CONNECT_TIMEOUT_MS 45000

static const char *connected_ssid = NULL;

bool wifi_connect_to_first_available(void) {
    if (cyw43_arch_init() != 0) {
        printf("WiFi: CYW43 init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    for (int i = 0; i < WIFI_NETWORK_COUNT; i++) {
        const char *ssid = WIFI_NETWORKS[i].ssid;
        const char *password = WIFI_NETWORKS[i].password;

        printf("WiFi: trying %s\n", ssid);

        int result = cyw43_arch_wifi_connect_timeout_ms(
            ssid,
            password,
            CYW43_AUTH_WPA2_MIXED_PSK,
            WIFI_CONNECT_TIMEOUT_MS
        );

        if (result == 0) {
            connected_ssid = ssid;
            printf("WiFi: connected to %s\n", connected_ssid);
            return true;
        }

        printf("WiFi: failed to connect to %s, error %d\n", ssid, result);

        sleep_ms(1000);
    }

    printf("WiFi: no configured network available\n");
    return false;
}

const char *wifi_get_connected_ssid(void) {
    if (connected_ssid == NULL) {
        return "not connected";
    }

    return connected_ssid;
}
