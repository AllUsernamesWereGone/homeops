#include "PowerManager.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include "HardwareConfig.h"

void PowerManager::releaseDeepSleepHolds() {
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(static_cast<gpio_num_t>(Hardware::FAN_POWER_PIN));
    gpio_hold_dis(static_cast<gpio_num_t>(Hardware::LAMP_PWM_PIN));
}

void PowerManager::enterDeepSleep(
    FanController &fan,
    LampController &lamp,
    MqttManager &mqtt,
    NetworkManager &network,
    bool debug) {

    fan.prepareForDeepSleep();
    lamp.prepareForDeepSleep();

    gpio_hold_en(static_cast<gpio_num_t>(Hardware::FAN_POWER_PIN));
    gpio_hold_en(static_cast<gpio_num_t>(Hardware::LAMP_PWM_PIN));
    gpio_deep_sleep_hold_en();

    mqtt.disconnect();
    network.shutdown();

    if (debug) {
        Serial.printf(
            "Deep sleep for %lu ms\n",
            static_cast<unsigned long>(Hardware::DEEP_SLEEP_MS));
        Serial.flush();
    }

    esp_sleep_enable_timer_wakeup(
        static_cast<uint64_t>(Hardware::DEEP_SLEEP_MS) * 1000ULL);
    esp_deep_sleep_start();
}
