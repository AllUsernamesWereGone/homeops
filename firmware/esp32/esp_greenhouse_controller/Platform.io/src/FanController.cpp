#include "FanController.h"

#include "HardwareConfig.h"

volatile uint32_t FanController::tachPulseCount_ = 0;

void ARDUINO_ISR_ATTR FanController::onTachPulse() {
    ++tachPulseCount_;
}

uint8_t FanController::outputDuty(uint8_t target) {
    return target == 0
        ? static_cast<uint8_t>(Hardware::PWM_MAX)
        : static_cast<uint8_t>(Hardware::PWM_MAX - target);
}

bool FanController::begin(uint8_t initialTarget) {
    pinMode(Hardware::FAN_POWER_PIN, OUTPUT);
    digitalWrite(Hardware::FAN_POWER_PIN, LOW);

    pinMode(Hardware::FAN_SENSE_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(Hardware::FAN_SENSE_PIN), onTachPulse, FALLING);

    attached_ = ledcAttach(
        Hardware::FAN_PWM_PIN,
        Hardware::FAN_PWM_FREQ_HZ,
        Hardware::PWM_RESOLUTION_BITS);

    target_ = initialTarget;
    outputOk_ = attached_ && writeHardware(target_);
    return outputOk_;
}

bool FanController::writeHardware(uint8_t target) {
    if (!attached_) return false;

    if (target == 0) {
        bool pwmOk = ledcWrite(Hardware::FAN_PWM_PIN, outputDuty(0));
        digitalWrite(Hardware::FAN_POWER_PIN, LOW);
        return pwmOk;
    }

    bool wasOff = digitalRead(Hardware::FAN_POWER_PIN) == LOW;
    digitalWrite(Hardware::FAN_POWER_PIN, HIGH);

    if (wasOff) {
        if (!ledcWrite(Hardware::FAN_PWM_PIN, 0)) return false;
        delay(300);
    }

    return ledcWrite(Hardware::FAN_PWM_PIN, outputDuty(target));
}

bool FanController::applyTarget(uint8_t target) {
    target_ = target;
    outputOk_ = writeHardware(target_);
    return outputOk_;
}

int32_t FanController::sampleRpm() {
    if (isOff()) return 0;

    noInterrupts();
    tachPulseCount_ = 0;
    interrupts();

    delay(Hardware::TACH_SAMPLE_MS);

    noInterrupts();
    uint32_t pulses = tachPulseCount_;
    interrupts();

    if (pulses == 0) return -1;

    return static_cast<int32_t>(
        (static_cast<uint64_t>(pulses) * 60000ULL) /
        (static_cast<uint64_t>(Hardware::TACH_SAMPLE_MS) * Hardware::FAN_TACH_PULSES_PER_REV));
}

void FanController::prepareForDeepSleep() {
    if (attached_) {
        outputOk_ = ledcWrite(Hardware::FAN_PWM_PIN, outputDuty(0));
    }
    digitalWrite(Hardware::FAN_POWER_PIN, LOW);
}

uint8_t FanController::target() const {
    return target_;
}

bool FanController::isOff() const {
    return target_ == 0;
}

bool FanController::outputOk() const {
    return outputOk_;
}
