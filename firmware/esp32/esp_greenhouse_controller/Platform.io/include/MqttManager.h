#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "SystemTypes.h"

struct RuntimeConfig;
class DeviceNetworkManager;
class FanController;
class LampController;
class RtcState;

class MqttManager {
public:
    bool begin(const RuntimeConfig &config, DeviceNetworkManager &network);
    bool ensureConnected();
    bool waitForCommand(uint32_t durationMs);
    bool serviceUntilCommand(uint32_t durationMs);

    const CommandState &pendingCommand() const;
    bool hasPendingCommand() const;
    void beginCycle();
    void clearPendingCommand();

    bool publishStatus(
        const SensorData &sensors,
        const FanController &fan,
        const LampController &lamp,
        const RtcState &rtc,
        bool wifiOk,
        bool timeOk,
        bool sleeping);

    bool clearRetainedCommand();
    bool connected();
    void disconnect();

private:
    static void callbackBridge(char *topic, byte *payload, unsigned int length);
    void handleMessage(char *topic, byte *payload, unsigned int length);
    bool loadTlsCertificate();

    static MqttManager *instance_;

    const RuntimeConfig *config_ = nullptr;
    DeviceNetworkManager *network_ = nullptr;
    WiFiClient plainClient_;
    WiFiClientSecure secureClient_;
    PubSubClient mqtt_;

    CommandState pendingCommand_;
    int16_t lastCommandStatus_ = STATUS_NOT_RECEIVED;
    int16_t lastReceivedFanPwm_ = STATUS_NOT_RECEIVED;
    int16_t lastReceivedLampPwm_ = STATUS_NOT_RECEIVED;

    char topicCommand_[96] = {};
    char topicStatus_[96] = {};
    bool subscribed_ = false;
    bool tlsCertOk_ = false;
};
