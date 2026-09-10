#pragma once

#include <Arduino.h>

class FanController {
public:
    bool begin(uint8_t initialTarget);
    bool applyTarget(uint8_t target);
    int32_t sampleRpm();
    void prepareForDeepSleep();

    uint8_t target() const;
    bool isOff() const;
    bool outputOk() const;

private:
    static void ARDUINO_ISR_ATTR onTachPulse();
    static volatile uint32_t tachPulseCount_;

    static uint8_t outputDuty(uint8_t target);
    bool writeHardware(uint8_t target);

    uint8_t target_ = 0;
    bool attached_ = false;
    bool outputOk_ = false;
};
