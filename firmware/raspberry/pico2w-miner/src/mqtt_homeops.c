#include "mqtt_homeops.h"

#include <stdio.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

#include "../secrets.h"

typedef enum {
    HOMEOPS_MQTT_IDLE = 0,
    HOMEOPS_MQTT_DNS_WAIT,
    HOMEOPS_MQTT_CONNECTING,
    HOMEOPS_MQTT_CONNECTED,
    HOMEOPS_MQTT_FAILED
} homeops_mqtt_state_t;

static mqtt_client_t *mqtt_client = NULL;
static homeops_mqtt_state_t mqtt_state = HOMEOPS_MQTT_IDLE;

static ip_addr_t resolved_broker_ip;
static const char *current_broker_host = NULL;

static void mqtt_homeops_request_callback(void *arg, err_t result) {
    (void)arg;

    if (result == ERR_OK) {
        printf("HOMEOPS MQTT: publish accepted by lwIP\n");
    } else {
        printf("HOMEOPS MQTT: publish failed result=%d\n", result);
    }
}

static void mqtt_homeops_connection_callback(
    mqtt_client_t *client,
    void *arg,
    mqtt_connection_status_t status
) {
    (void)client;
    (void)arg;

    if (status == MQTT_CONNECT_ACCEPTED) {
        mqtt_state = HOMEOPS_MQTT_CONNECTED;
        printf("HOMEOPS MQTT: connected to broker\n");
        return;
    }

    mqtt_state = HOMEOPS_MQTT_FAILED;
    printf("HOMEOPS MQTT: connection failed status=%d\n", status);
}

static void mqtt_homeops_dns_callback(
    const char *name,
    const ip_addr_t *ipaddr,
    void *arg
) {
    (void)arg;

    if (ipaddr == NULL) {
        printf("HOMEOPS MQTT: DNS failed for %s\n", name);
        mqtt_state = HOMEOPS_MQTT_FAILED;
        return;
    }

    resolved_broker_ip = *ipaddr;
    printf("HOMEOPS MQTT: DNS %s -> %s\n", name, ipaddr_ntoa(&resolved_broker_ip));

    mqtt_state = HOMEOPS_MQTT_CONNECTING;

    static struct mqtt_connect_client_info_t client_info;

    memset(&client_info, 0, sizeof(client_info));

    client_info.client_id = HOMEOPS_MQTT_CLIENT_ID;
    client_info.client_user = NULL;
    client_info.client_pass = NULL;
    client_info.keep_alive = 30;
    client_info.will_topic = NULL;
    client_info.will_msg = NULL;
    client_info.will_qos = 0;
    client_info.will_retain = 0;

    err_t err = mqtt_client_connect(
        mqtt_client,
        &resolved_broker_ip,
        HOMEOPS_MQTT_PORT,
        mqtt_homeops_connection_callback,
        NULL,
        &client_info
    );

    if (err != ERR_OK) {
        printf("HOMEOPS MQTT: mqtt_client_connect failed err=%d\n", err);
        mqtt_state = HOMEOPS_MQTT_FAILED;
    }
}

static bool mqtt_homeops_connect_to_host_blocking(
    const char *host,
    uint32_t timeout_ms
) {
    if (host == NULL || host[0] == '\0') {
        return false;
    }

    current_broker_host = host;
    mqtt_state = HOMEOPS_MQTT_IDLE;
    memset(&resolved_broker_ip, 0, sizeof(resolved_broker_ip));

    if (mqtt_client != NULL) {
        cyw43_arch_lwip_begin();
        mqtt_disconnect(mqtt_client);
        mqtt_client_free(mqtt_client);
        mqtt_client = NULL;
        cyw43_arch_lwip_end();
    }

    mqtt_client = mqtt_client_new();

    if (mqtt_client == NULL) {
        printf("HOMEOPS MQTT: mqtt_client_new failed\n");
        mqtt_state = HOMEOPS_MQTT_FAILED;
        return false;
    }

    printf("HOMEOPS MQTT: resolving broker %s\n", host);

    cyw43_arch_lwip_begin();

    err_t dns_result = dns_gethostbyname(
        host,
        &resolved_broker_ip,
        mqtt_homeops_dns_callback,
        NULL
    );

    if (dns_result == ERR_OK) {
        printf("HOMEOPS MQTT: immediate DNS %s -> %s\n",
            host,
            ipaddr_ntoa(&resolved_broker_ip)
        );

        cyw43_arch_lwip_end();

        mqtt_homeops_dns_callback(host, &resolved_broker_ip, NULL);
    } else if (dns_result == ERR_INPROGRESS) {
        mqtt_state = HOMEOPS_MQTT_DNS_WAIT;
        cyw43_arch_lwip_end();
    } else {
        printf("HOMEOPS MQTT: dns_gethostbyname failed err=%d for %s\n",
            dns_result,
            host
        );

        mqtt_state = HOMEOPS_MQTT_FAILED;
        cyw43_arch_lwip_end();
        return false;
    }

    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);

    while (!time_reached(timeout_time)) {
        if (mqtt_state == HOMEOPS_MQTT_CONNECTED) {
            printf("HOMEOPS MQTT: using broker %s\n", current_broker_host);
            return true;
        }

        if (mqtt_state == HOMEOPS_MQTT_FAILED) {
            return false;
        }

        sleep_ms(50);
    }

    printf("HOMEOPS MQTT: connect timeout for %s\n", host);

    mqtt_homeops_disconnect();

    return false;
}

bool mqtt_homeops_connect_blocking(uint32_t timeout_ms) {
    printf("HOMEOPS MQTT: trying broker candidates\n");

    for (int i = 0; i < HOMEOPS_MQTT_BROKER_COUNT; i++) {
        const char *host = HOMEOPS_MQTT_BROKERS[i];

        printf("HOMEOPS MQTT: candidate %d/%d: %s\n",
            i + 1,
            HOMEOPS_MQTT_BROKER_COUNT,
            host
        );

        if (mqtt_homeops_connect_to_host_blocking(host, timeout_ms)) {
            return true;
        }

        printf("HOMEOPS MQTT: candidate failed: %s\n", host);
        sleep_ms(500);
    }

    printf("HOMEOPS MQTT: all broker candidates failed\n");
    return false;
}

bool mqtt_homeops_is_connected(void) {
    if (mqtt_client == NULL) {
        return false;
    }

    return mqtt_client_is_connected(mqtt_client) != 0;
}

bool mqtt_homeops_publish_telemetry(
    uint32_t hashrate,
    int32_t wifi_rssi,
    uint32_t uptime_seconds,
    uint32_t shares_found,
    bool pool_connected,
    bool has_active_job
) {
    if (!mqtt_homeops_is_connected()) {
        printf("HOMEOPS MQTT: publish skipped, not connected\n");
        return false;
    }

    char payload[384];

    int written = snprintf(
        payload,
        sizeof(payload),
        "{"
            "\"schemaVersion\":1,"
            "\"data\":{"
                "\"hashrate\":%lu,"
                "\"wifiRssi\":%ld,"
                "\"uptimeSeconds\":%lu,"
                "\"sharesFound\":%lu,"
                "\"poolConnected\":%s,"
                "\"hasActiveJob\":%s"
            "}"
        "}",
        (unsigned long)hashrate,
        (long)wifi_rssi,
        (unsigned long)uptime_seconds,
        (unsigned long)shares_found,
        pool_connected ? "true" : "false",
        has_active_job ? "true" : "false"
    );

    if (written < 0 || written >= (int)sizeof(payload)) {
        printf("HOMEOPS MQTT: telemetry payload too large\n");
        return false;
    }

    cyw43_arch_lwip_begin();

    err_t err = mqtt_publish(
        mqtt_client,
        HOMEOPS_MQTT_TOPIC,
        payload,
        (u16_t)strlen(payload),
        0,
        0,
        mqtt_homeops_request_callback,
        NULL
    );

    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("HOMEOPS MQTT: mqtt_publish failed err=%d\n", err);
        return false;
    }

    printf("HOMEOPS MQTT: published telemetry: %s\n", payload);

    return true;
}

void mqtt_homeops_disconnect(void) {
    if (mqtt_client == NULL) {
        mqtt_state = HOMEOPS_MQTT_IDLE;
        return;
    }

    cyw43_arch_lwip_begin();

    mqtt_disconnect(mqtt_client);
    mqtt_client_free(mqtt_client);
    mqtt_client = NULL;

    cyw43_arch_lwip_end();

    mqtt_state = HOMEOPS_MQTT_IDLE;

    printf("HOMEOPS MQTT: disconnected\n");
}