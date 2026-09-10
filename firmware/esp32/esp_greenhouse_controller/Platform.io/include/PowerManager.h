#pragma once

#include "FanController.h"
#include "LampController.h"
#include "MqttManager.h"
#include "NetworkManager.h"

class PowerManager {
public:
    void releaseDeepSleepHolds();
    void enterDeepSleep(
        FanController &fan,
        LampController &lamp,
        MqttManager &mqtt,
        NetworkManager &network,
        bool debug);
};
