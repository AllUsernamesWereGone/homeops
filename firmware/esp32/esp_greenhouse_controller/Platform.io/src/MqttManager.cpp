#include "MqttManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <cstring>

#include "HardwareConfig.h"
#include "secrets.h"

MqttManager *MqttManager::instance_ = nullptr;

bool MqttManager::begin(const RuntimeConfig &config, NetworkManager &network) {
    config_ = &config;
    network_ = &network;
    instance_ = this;

    int cmdLen = snprintf(
        topicCommand_, sizeof(topicCommand_),
        "homeops/devices/%s/command", DEVICE_ID);
    int statusLen = snprintf(
        topicStatus_, sizeof(topicStatus_),
        "homeops/devices/%s/telemetry", DEVICE_ID);

    if (cmdLen <= 0 || cmdLen >= static_cast<int>(sizeof(topicCommand_)) ||
        statusLen <= 0 || statusLen >= static_cast<int>(sizeof(topicStatus_))) {
        return false;
    }

    if (config_->tls) {
        if (!loadTlsCertificate()) return false;
        mqtt_.setClient(secureClient_);
    } else {
        tlsCertOk_ = true;
        mqtt_.setClient(plainClient_);
    }

    mqtt_.setServer(MQTT_HOST, MQTT_PORT);
    mqtt_.setCallback(callbackBridge);
    mqtt_.setKeepAlive(Hardware::MQTT_KEEPALIVE_SEC);
    mqtt_.setSocketTimeout(Hardware::MQTT_SOCKET_TIMEOUT_SEC);
    return mqtt_.setBufferSize(Hardware::MQTT_BUFFER_SIZE);
}

bool MqttManager::loadTlsCertificate() {
    tlsCertOk_ = false;

    File file = LittleFS.open(Hardware::MQTT_CA_CERT_PATH, "r");
    if (!file) {
        if (config_ != nullptr && config_->debug) {
            Serial.println("TLS CA file missing: /mqtt_ca.pem");
        }
        return false;
    }

    if (file.size() < 64) {
        if (config_ != nullptr && config_->debug) {
            Serial.println("TLS CA file is empty or invalid");
        }
        file.close();
        return false;
    }

    size_t certSize = file.size();
    tlsCertOk_ = secureClient_.loadCACert(file, certSize);
    file.close();

    if (!tlsCertOk_ && config_ != nullptr && config_->debug) {
        Serial.println("TLS CA file could not be loaded");
    }
    return tlsCertOk_;
}

void MqttManager::callbackBridge(char *topic, byte *payload, unsigned int length) {
    if (instance_ != nullptr) instance_->handleMessage(topic, payload, length);
}

void MqttManager::handleMessage(char *topic, byte *payload, unsigned int length) {
    if (strcmp(topic, topicCommand_) != 0) return;

    pendingCommand_ = CommandState{};
    lastReceivedFanPwm_ = STATUS_NOT_RECEIVED;
    lastReceivedLampPwm_ = STATUS_NOT_RECEIVED;

    if (length == 0) {
        lastCommandStatus_ = STATUS_NOT_RECEIVED;
        return;
    }

    pendingCommand_.received = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error || !doc.is<JsonObject>()) {
        pendingCommand_.invalid = true;
        lastReceivedFanPwm_ = STATUS_ERROR;
        lastReceivedLampPwm_ = STATUS_ERROR;
        lastCommandStatus_ = STATUS_ERROR;
        return;
    }

    JsonObjectConst obj = doc.as<JsonObjectConst>();
    for (JsonPairConst pair : obj) {
        const char *key = pair.key().c_str();
        JsonVariantConst value = pair.value();

        if (strcmp(key, "fan_pwm") == 0) {
            pendingCommand_.fanPresent = true;
            if (value.is<int>()) {
                int pwm = value.as<int>();
                if (pwm >= 0 && pwm <= Hardware::PWM_MAX) {
                    pendingCommand_.fanPwm = pwm;
                    lastReceivedFanPwm_ = pwm;
                } else {
                    pendingCommand_.invalid = true;
                    lastReceivedFanPwm_ = STATUS_ERROR;
                }
            } else {
                pendingCommand_.invalid = true;
                lastReceivedFanPwm_ = STATUS_ERROR;
            }
        } else if (strcmp(key, "lamp_pwm") == 0) {
            pendingCommand_.lampPresent = true;
            if (value.is<int>()) {
                int pwm = value.as<int>();
                if (pwm >= 0 && pwm <= Hardware::PWM_MAX) {
                    pendingCommand_.lampPwm = pwm;
                    lastReceivedLampPwm_ = pwm;
                } else {
                    pendingCommand_.invalid = true;
                    lastReceivedLampPwm_ = STATUS_ERROR;
                }
            } else {
                pendingCommand_.invalid = true;
                lastReceivedLampPwm_ = STATUS_ERROR;
            }
        }
    }

    if (!pendingCommand_.fanPresent && !pendingCommand_.lampPresent) {
        pendingCommand_.invalid = true;
    }

    lastCommandStatus_ = pendingCommand_.invalid ? STATUS_ERROR : STATUS_OK;
}

bool MqttManager::ensureConnected() {
    if (mqtt_.connected() && subscribed_) return true;
    if (config_ == nullptr || network_ == nullptr || !network_->connected()) return false;
    if (config_->tls && (!tlsCertOk_ || !network_->timeValid())) return false;

    mqtt_.disconnect();
    subscribed_ = false;

    bool mqttConnected = false;
#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    mqttConnected = mqtt_.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD);
#else
    mqttConnected = mqtt_.connect(DEVICE_ID);
#endif

    if (!mqttConnected) {
        if (config_->debug) Serial.printf("MQTT connect failed: state=%d\n", mqtt_.state());
        return false;
    }

    subscribed_ = mqtt_.subscribe(topicCommand_, 1);
    if (!subscribed_) {
        mqtt_.disconnect();
        return false;
    }

    return true;
}

bool MqttManager::waitForCommand(uint32_t durationMs) {
    if (!mqtt_.connected()) return false;
    if (pendingCommand_.received) return true;

    uint32_t start = millis();
    while (millis() - start < durationMs) {
        if (!mqtt_.loop()) {
            subscribed_ = false;
            return false;
        }
        if (pendingCommand_.received) return true;
        delay(Hardware::MQTT_LOOP_DELAY_MS);
    }

    return false;
}

bool MqttManager::serviceUntilCommand(uint32_t durationMs) {
    if (!mqtt_.connected()) return false;

    uint32_t start = millis();
    while (millis() - start < durationMs) {
        if (!mqtt_.loop()) {
            subscribed_ = false;
            return false;
        }
        if (pendingCommand_.received) return true;
        delay(Hardware::MQTT_LOOP_DELAY_MS);
    }

    return false;
}

const CommandState &MqttManager::pendingCommand() const {
    return pendingCommand_;
}

bool MqttManager::hasPendingCommand() const {
    return pendingCommand_.received;
}

void MqttManager::beginCycle() {
    if (pendingCommand_.received) return;
    lastCommandStatus_ = STATUS_NOT_RECEIVED;
    lastReceivedFanPwm_ = STATUS_NOT_RECEIVED;
    lastReceivedLampPwm_ = STATUS_NOT_RECEIVED;
}

void MqttManager::clearPendingCommand() {
    pendingCommand_ = CommandState{};
}

bool MqttManager::publishStatus(
    const SensorData &sensors,
    const FanController &fan,
    const LampController &lamp,
    const RtcState &rtc,
    bool wifiOk,
    bool timeOk,
    bool sleeping) {

    if (!mqtt_.connected() || network_ == nullptr || config_ == nullptr) return false;

    char utc[24];
    int64_t epoch = -1;
    bool utcOk = timeOk && network_->formatUtc(utc, sizeof(utc), epoch);
    if (!utcOk) {
        snprintf(utc, sizeof(utc), "%s", "-1");
        epoch = -1;
    }

    JsonDocument doc;
    doc["schema_version"] = 2;
    doc["device_id"] = DEVICE_ID;
    doc["utc"] = utc;
    doc["epoch"] = epoch;

    JsonObject data = doc["data"].to<JsonObject>();
    data["temperature_c"] = sensors.temperatureC;
    data["humidity_pct"] = sensors.humidityPct;
    data["photo_v"] = sensors.photoVolts;
    data["fan_rpm"] = sensors.fanRpm;
    data["fan_pwm"] = fan.target();
    data["lamp_pwm"] = lamp.target();
    data["fan_pwm_ok"] = fan.outputOk() ? STATUS_OK : STATUS_ERROR;
    data["lamp_pwm_ok"] = lamp.outputOk() ? STATUS_OK : STATUS_ERROR;
    data["wifi_ok"] = wifiOk ? STATUS_OK : STATUS_ERROR;
    data["mqtt_ok"] = mqtt_.connected() ? STATUS_OK : STATUS_ERROR;
    data["wifi_rssi_dbm"] = wifiOk ? network_->rssi() : STATUS_ERROR;
    data["time_ok"] = utcOk ? STATUS_OK : STATUS_ERROR;
    data["tls_mode"] = config_->tls ? 1 : 0;
    data["tls_cert_loaded"] = config_->tls ? (tlsCertOk_ ? STATUS_OK : STATUS_ERROR) : 0;
    data["rtc_state_ok"] = rtc.validityStatus();
    data["last_command_ok"] = lastCommandStatus_;
    data["last_rx_fan_pwm"] = lastReceivedFanPwm_;
    data["last_rx_lamp_pwm"] = lastReceivedLampPwm_;
    data["sleeping"] = sleeping;
    data["boot_count"] = rtc.bootCount();

    char payload[Hardware::MQTT_BUFFER_SIZE];
    size_t required = measureJson(doc);
    if (required + 1 > sizeof(payload)) return false;

    size_t length = serializeJson(doc, payload, sizeof(payload));
    bool ok = mqtt_.publish(
        topicStatus_,
        reinterpret_cast<const uint8_t *>(payload),
        length,
        false);

    if (config_->debug && ok) Serial.println(payload);
    return ok;
}

bool MqttManager::clearRetainedCommand() {
    return mqtt_.connected() && mqtt_.publish(topicCommand_, "", true);
}

bool MqttManager::connected() {
    return mqtt_.connected();
}

void MqttManager::disconnect() {
    if (mqtt_.connected()) mqtt_.disconnect();
    subscribed_ = false;
}
