#include "NetworkManager.h"

#include <WiFi.h>
#include <time.h>
#include "HardwareConfig.h"
#include "secrets.h"

void NetworkManager::begin(const RuntimeConfig &config) {
    config_ = &config;
}

bool NetworkManager::ensureWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) return true;
    if (config_ == nullptr) return false;

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, false);
    delay(50);

    if (!config_->dhcp) {
        if (!WiFi.config(
                config_->ip,
                config_->gateway,
                config_->subnet,
                config_->dns1,
                config_->dns2)) {
            if (config_->debug) Serial.println("WiFi static IP configuration failed");
            return false;
        }
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < Hardware::WIFI_CONNECT_TIMEOUT_MS) {
        delay(50);
    }

    bool ok = WiFi.status() == WL_CONNECTED;
    if (config_->debug) {
        Serial.printf("WiFi: %s, RSSI=%ld\n", ok ? "connected" : "failed", ok ? WiFi.RSSI() : -1L);
    }
    return ok;
}

bool NetworkManager::timeValid() const {
    return time(nullptr) >= Hardware::MIN_VALID_EPOCH;
}

bool NetworkManager::syncUtcTime() {
    if (!connected()) return false;

    configTime(0, 0, Hardware::NTP_SERVER_1, Hardware::NTP_SERVER_2);
    if (timeValid()) return true;

    uint32_t start = millis();
    while (millis() - start < Hardware::TIME_SYNC_TIMEOUT_MS) {
        if (timeValid()) return true;
        delay(50);
    }

    return false;
}

bool NetworkManager::formatUtc(char *buffer, size_t size, int64_t &epochOut) const {
    time_t now = time(nullptr);
    if (now < Hardware::MIN_VALID_EPOCH) {
        snprintf(buffer, size, "%s", "-1");
        epochOut = -1;
        return false;
    }

    struct tm utcTm;
    if (gmtime_r(&now, &utcTm) == nullptr ||
        strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", &utcTm) == 0) {
        snprintf(buffer, size, "%s", "-1");
        epochOut = -1;
        return false;
    }

    epochOut = static_cast<int64_t>(now);
    return true;
}

int32_t NetworkManager::rssi() const {
    return connected() ? WiFi.RSSI() : -1;
}

bool NetworkManager::connected() const {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::shutdown() {
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
}
