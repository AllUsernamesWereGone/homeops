#include "AppConfig.h"

#include <LittleFS.h>
#include "HardwareConfig.h"

bool AppConfig::parseEnvBool(String value, bool &out) {
    value.trim();
    if (value.length() >= 2 &&
        ((value[0] == '"' && value[value.length() - 1] == '"') ||
         (value[0] == '\'' && value[value.length() - 1] == '\''))) {
        value = value.substring(1, value.length() - 1);
        value.trim();
    }

    value.toLowerCase();
    if (value == "on") {
        out = true;
        return true;
    }
    if (value == "off") {
        out = false;
        return true;
    }
    return false;
}

bool AppConfig::parseIpv4(String value, IPAddress &out) {
    value.trim();
    if (value.length() >= 2 &&
        ((value[0] == '"' && value[value.length() - 1] == '"') ||
         (value[0] == '\'' && value[value.length() - 1] == '\''))) {
        value = value.substring(1, value.length() - 1);
        value.trim();
    }

    int a, b, c, d;
    char extra;
    if (sscanf(value.c_str(), "%d.%d.%d.%d%c", &a, &b, &c, &d, &extra) != 4) return false;
    if (a < 0 || a > 255 || b < 0 || b > 255 ||
        c < 0 || c > 255 || d < 0 || d > 255) return false;

    out = IPAddress(a, b, c, d);
    return true;
}

bool AppConfig::begin() {
    config_ = RuntimeConfig{};

    if (!LittleFS.begin(false)) return false;

    File file = LittleFS.open(Hardware::ENV_FILE_PATH, "r");
    if (!file) return false;

    bool haveDebug = false;
    bool debugValid = false;
    bool haveTls = false;
    bool tlsValid = false;
    bool haveDhcp = false;
    bool dhcpValid = false;

    bool haveIp = false;
    bool ipValid = false;
    bool haveGateway = false;
    bool gatewayValid = false;
    bool haveSubnet = false;
    bool subnetValid = false;
    bool haveDns1 = false;
    bool dns1Valid = false;
    bool dns2Valid = true;
    bool syntaxValid = true;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int separator = line.indexOf('=');
        if (separator <= 0) {
            syntaxValid = false;
            continue;
        }

        String key = line.substring(0, separator);
        String value = line.substring(separator + 1);
        key.trim();
        value.trim();

        if (key == "DEBUG_MODE") {
            haveDebug = true;
            debugValid = parseEnvBool(value, config_.debug);
        } else if (key == "MQTT_TLS_MODE") {
            haveTls = true;
            tlsValid = parseEnvBool(value, config_.tls);
        } else if (key == "DHCP_MODE") {
            haveDhcp = true;
            dhcpValid = parseEnvBool(value, config_.dhcp);
        } else if (key == "IP_ADDRESS") {
            haveIp = true;
            ipValid = parseIpv4(value, config_.ip);
        } else if (key == "GATEWAY") {
            haveGateway = true;
            gatewayValid = parseIpv4(value, config_.gateway);
        } else if (key == "SUBNET") {
            haveSubnet = true;
            subnetValid = parseIpv4(value, config_.subnet);
        } else if (key == "DNS1") {
            haveDns1 = true;
            dns1Valid = parseIpv4(value, config_.dns1);
        } else if (key == "DNS2") {
            config_.dns2Present = true;
            dns2Valid = parseIpv4(value, config_.dns2);
        }
    }

    file.close();

    bool basicConfigValid = syntaxValid &&
                            haveDebug && debugValid &&
                            haveTls && tlsValid &&
                            haveDhcp && dhcpValid;

    bool networkConfigValid = false;
    if (haveDhcp && dhcpValid) {
        if (config_.dhcp) {
            networkConfigValid = true;
        } else {
            networkConfigValid = haveIp && ipValid &&
                                 haveGateway && gatewayValid &&
                                 haveSubnet && subnetValid &&
                                 haveDns1 && dns1Valid &&
                                 (!config_.dns2Present || dns2Valid);
        }
    }

    config_.valid = basicConfigValid && networkConfigValid;
    return config_.valid;
}

const RuntimeConfig &AppConfig::runtime() const {
    return config_;
}
