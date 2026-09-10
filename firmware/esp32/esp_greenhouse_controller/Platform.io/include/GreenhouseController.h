#pragma once

#include <Arduino.h>

#include "AppConfig.h"
#include "FanController.h"
#include "LampController.h"
#include "MqttManager.h"
#include "NetworkManager.h"
#include "PowerManager.h"
#include "RtcState.h"
#include "SensorManager.h"

class GreenhouseController {
public:
    void begin();
    void loop();

private:
    bool outputsOff() const;
    bool applyPendingCommand();
    void runCycle();
    void serviceWhileAwake(uint32_t durationMs);

    AppConfig appConfig_;
    RtcState rtc_;
    FanController fan_;
    LampController lamp_;
    SensorManager sensors_;
    NetworkManager network_;
    MqttManager mqtt_;
    PowerManager power_;

    bool initialized_ = false;
};
