#pragma once

#include <Arduino.h>

class LampController {
public:
    bool begin(uint8_t initialTarget);
    bool applyTarget(uint8_t target);
    void prepareForDeepSleep();

    uint8_t target() const;
    bool isOff() const;
    bool outputOk() const;

private:
    bool writeHardware(uint8_t target);

    uint8_t target_ = 0;
    bool attached_ = false;
    bool outputOk_ = false;
};
