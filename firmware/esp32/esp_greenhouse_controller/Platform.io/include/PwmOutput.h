#pragma once

#include <Arduino.h>
#include <esp_arduino_version.h>

namespace PwmOutput {

inline bool attach(
    uint8_t pin,
    uint8_t channel,
    uint32_t frequencyHz,
    uint8_t resolutionBits) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)channel;
    return ledcAttach(pin, frequencyHz, resolutionBits);
#else
    if (ledcSetup(channel, frequencyHz, resolutionBits) <= 0.0) return false;
    ledcAttachPin(pin, channel);
    return true;
#endif
}

inline bool write(uint8_t pin, uint8_t channel, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)channel;
    return ledcWrite(pin, duty);
#else
    (void)pin;
    ledcWrite(channel, duty);
    return true;
#endif
}

}  // namespace PwmOutput
