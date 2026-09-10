#pragma once

#include <Arduino.h>
#include "AppConfig.h"

class NetworkManager {
public:
    void begin(const RuntimeConfig &config);
    bool ensureWifiConnected();
    bool syncUtcTime();
    bool timeValid() const;
    bool formatUtc(char *buffer, size_t size, int64_t &epochOut) const;
    int32_t rssi() const;
    bool connected() const;
    void shutdown();

private:
    const RuntimeConfig *config_ = nullptr;
};
