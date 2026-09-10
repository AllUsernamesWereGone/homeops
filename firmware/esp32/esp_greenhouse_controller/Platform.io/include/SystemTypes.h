#pragma once

#include <Arduino.h>

constexpr int16_t STATUS_OK = 1;
constexpr int16_t STATUS_ERROR = -1;
constexpr int16_t STATUS_NOT_RECEIVED = -2;

struct SensorData {
    float temperatureC = -1.0f;
    float humidityPct = -1.0f;
    float photoVolts = -1.0f;
    int32_t fanRpm = -1;
};

struct CommandState {
    bool received = false;
    bool fanPresent = false;
    bool lampPresent = false;
    bool invalid = false;
    int16_t fanPwm = STATUS_NOT_RECEIVED;
    int16_t lampPwm = STATUS_NOT_RECEIVED;
};
