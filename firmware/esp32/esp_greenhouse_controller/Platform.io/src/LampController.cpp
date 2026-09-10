#include "LampController.h"

#include "HardwareConfig.h"

bool LampController::begin(uint8_t initialTarget) {
    pinMode(Hardware::LAMP_PWM_PIN, OUTPUT);
    digitalWrite(Hardware::LAMP_PWM_PIN, LOW);

    attached_ = ledcAttach(
        Hardware::LAMP_PWM_PIN,
        Hardware::LAMP_PWM_FREQ_HZ,
        Hardware::PWM_RESOLUTION_BITS);

    target_ = initialTarget;
    outputOk_ = attached_ && writeHardware(target_);
    return outputOk_;
}

bool LampController::writeHardware(uint8_t target) {
    if (!attached_) return false;
    return ledcWrite(Hardware::LAMP_PWM_PIN, target);
}

bool LampController::applyTarget(uint8_t target) {
    target_ = target;
    outputOk_ = writeHardware(target_);
    return outputOk_;
}

void LampController::prepareForDeepSleep() {
    if (attached_) outputOk_ = ledcWrite(Hardware::LAMP_PWM_PIN, 0);
}

uint8_t LampController::target() const {
    return target_;
}

bool LampController::isOff() const {
    return target_ == 0;
}

bool LampController::outputOk() const {
    return outputOk_;
}
