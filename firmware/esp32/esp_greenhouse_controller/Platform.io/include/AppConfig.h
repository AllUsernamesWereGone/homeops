#pragma once

#include <Arduino.h>
#include <IPAddress.h>

struct RuntimeConfig {
    bool debug = false;
    bool tls = false;
    bool dhcp = false;
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2 = IPAddress(0, 0, 0, 0);
    bool dns2Present = false;
    bool valid = false;
};

class AppConfig {
public:
    bool begin();
    const RuntimeConfig &runtime() const;

private:
    static bool parseEnvBool(String value, bool &out);
    static bool parseIpv4(String value, IPAddress &out);

    RuntimeConfig config_;
};
