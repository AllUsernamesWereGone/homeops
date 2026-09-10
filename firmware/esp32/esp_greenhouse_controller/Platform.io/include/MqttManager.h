#pragma once

#include <Arduino.h>
#include <NetworkClient.h>
#include <NetworkClientSecure.h>
#include <PubSubClient.h>

#include "AppConfig.h"
#include "FanController.h"
#include "LampController.h"
#include "NetworkManager.h"
#include "RtcState.h"
#include "SystemTypes.h"

class MqttManager {
public:
    bool begin(const RuntimeConfig &config, NetworkManager &network);
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
    NetworkManager *network_ = nullptr;
    NetworkClient plainClient_;
    NetworkClientSecure secureClient_;
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
