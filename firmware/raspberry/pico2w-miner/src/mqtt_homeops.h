#ifndef MQTT_HOMEOPS_H
#define MQTT_HOMEOPS_H

#include <stdbool.h>
#include <stdint.h>

bool mqtt_homeops_connect_blocking(uint32_t timeout_ms);

bool mqtt_homeops_is_connected(void);

bool mqtt_homeops_publish_telemetry(
    uint32_t hashrate,
    int32_t wifi_rssi,
    uint32_t uptime_seconds,
    uint32_t shares_found,
    bool pool_connected,
    bool has_active_job
);

void mqtt_homeops_disconnect(void);

#endif
