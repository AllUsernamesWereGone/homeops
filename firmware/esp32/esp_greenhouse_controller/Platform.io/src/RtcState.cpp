#include "RtcState.h"

#include <esp_attr.h>

namespace {
constexpr uint32_t RTC_MAGIC = 0x45535033;

struct RtcStorage {
    uint32_t magic;
    uint8_t fanPwm;
    uint8_t lampPwm;
    uint32_t bootCount;
};

RTC_DATA_ATTR RtcStorage rtcStorage;
}

void RtcState::begin() {
    bool valid = rtcStorage.magic == RTC_MAGIC;

    if (!valid) {
        rtcStorage.magic = RTC_MAGIC;
        rtcStorage.fanPwm = 0;
        rtcStorage.lampPwm = 0;
        rtcStorage.bootCount = 0;
    }

    validityStatus_ = valid ? STATUS_OK : STATUS_ERROR;
}

void RtcState::incrementBootCount() {
    ++rtcStorage.bootCount;
}

uint8_t RtcState::fanPwm() const {
    return rtcStorage.fanPwm;
}

uint8_t RtcState::lampPwm() const {
    return rtcStorage.lampPwm;
}

uint32_t RtcState::bootCount() const {
    return rtcStorage.bootCount;
}

int16_t RtcState::validityStatus() const {
    return validityStatus_;
}

void RtcState::setFanPwm(uint8_t value) {
    rtcStorage.fanPwm = value;
}

void RtcState::setLampPwm(uint8_t value) {
    rtcStorage.lampPwm = value;
}
