#pragma once

#include <Arduino.h>
#include "SystemTypes.h"

class RtcState {
public:
    void begin();
    void incrementBootCount();

    uint8_t fanPwm() const;
    uint8_t lampPwm() const;
    uint32_t bootCount() const;
    int16_t validityStatus() const;

    void setFanPwm(uint8_t value);
    void setLampPwm(uint8_t value);

private:
    int16_t validityStatus_ = STATUS_ERROR;
};
