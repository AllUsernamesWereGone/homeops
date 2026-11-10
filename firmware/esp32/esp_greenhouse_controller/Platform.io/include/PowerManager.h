#pragma once

class DeviceNetworkManager;
class FanController;
class LampController;
class MqttManager;

class PowerManager {
public:
    void releaseDeepSleepHolds();
    void enterDeepSleep(
        FanController &fan,
        LampController &lamp,
        MqttManager &mqtt,
        DeviceNetworkManager &network,
        bool debug);
};
