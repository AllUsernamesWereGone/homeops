#include "GreenhouseController.h"

#include <esp_arduino_version.h>
#include "HardwareConfig.h"
#include "secrets.h"

#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "This project targets Arduino-ESP32 core 3.x or newer."
#endif

#ifndef WIFI_SSID
#error "WIFI_SSID must be defined in secrets.h"
#endif
#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD must be defined in secrets.h"
#endif
#ifndef MQTT_HOST
#error "MQTT_HOST must be defined in secrets.h"
#endif
#ifndef MQTT_PORT
#error "MQTT_PORT must be defined in secrets.h"
#endif
#ifndef DEVICE_ID
#error "DEVICE_ID must be defined in secrets.h"
#endif
#if defined(MQTT_USERNAME) != defined(MQTT_PASSWORD)
#error "Define both MQTT_USERNAME and MQTT_PASSWORD, or neither."
#endif

void GreenhouseController::begin() {
    rtc_.begin();
    rtc_.incrementBootCount();

    pinMode(Hardware::FAN_POWER_PIN, OUTPUT);
    digitalWrite(Hardware::FAN_POWER_PIN, LOW);
    pinMode(Hardware::LAMP_PWM_PIN, OUTPUT);
    digitalWrite(Hardware::LAMP_PWM_PIN, LOW);

    power_.releaseDeepSleepHolds();
    fan_.begin(rtc_.fanPwm());
    lamp_.begin(rtc_.lampPwm());
    sensors_.begin();

    if (!appConfig_.begin()) {
        initialized_ = false;
        return;
    }

    const RuntimeConfig &config = appConfig_.runtime();
    if (config.debug) {
        Serial.begin(115200);
        delay(50);
    }

    network_.begin(config);
    if (!mqtt_.begin(config, network_)) {
        initialized_ = false;
        return;
    }

    initialized_ = true;

    if (config.debug) {
        Serial.printf(
            "Boot %lu, fan_pwm=%u, lamp_pwm=%u, TLS=%d, DHCP=%d, MQTT port=%u\n",
            static_cast<unsigned long>(rtc_.bootCount()),
            fan_.target(),
            lamp_.target(),
            config.tls ? 1 : 0,
            config.dhcp ? 1 : 0,
            MQTT_PORT);
    }
}

bool GreenhouseController::outputsOff() const {
    return fan_.isOff() && lamp_.isOff();
}

bool GreenhouseController::applyPendingCommand() {
    const CommandState &command = mqtt_.pendingCommand();
    if (!command.received || command.invalid) return false;

    bool changed = false;

    if (command.fanPresent && command.fanPwm != fan_.target()) {
        uint8_t target = static_cast<uint8_t>(command.fanPwm);
        rtc_.setFanPwm(target);
        fan_.applyTarget(target);
        changed = true;
    }

    if (command.lampPresent && command.lampPwm != lamp_.target()) {
        uint8_t target = static_cast<uint8_t>(command.lampPwm);
        rtc_.setLampPwm(target);
        lamp_.applyTarget(target);
        changed = true;
    }

    return changed;
}

void GreenhouseController::runCycle() {
    mqtt_.beginCycle();

    bool wifiOk = network_.ensureWifiConnected();
    bool timeOk = wifiOk && network_.syncUtcTime();
    bool mqttOk = wifiOk && mqtt_.ensureConnected();

    SensorData sensorData = sensors_.read(fan_);

    if (mqttOk && !mqtt_.hasPendingCommand()) {
        mqtt_.waitForCommand(Hardware::MQTT_COMMAND_WINDOW_MS);
    }

    bool changed = applyPendingCommand();
    if (changed) sensorData = sensors_.read(fan_);

    const CommandState &command = mqtt_.pendingCommand();
    bool clearRetained = command.received && !command.invalid;
    bool sleeping = outputsOff();

    if (mqtt_.connected()) {
        mqtt_.publishStatus(
            sensorData,
            fan_,
            lamp_,
            rtc_,
            wifiOk,
            timeOk,
            sleeping);

        if (clearRetained) mqtt_.clearRetainedCommand();
        delay(20);
    }

    mqtt_.clearPendingCommand();

    if (sleeping) {
        power_.enterDeepSleep(
            fan_, lamp_, mqtt_, network_, appConfig_.runtime().debug);
    }
}

void GreenhouseController::serviceWhileAwake(uint32_t durationMs) {
    if (!network_.connected() || !mqtt_.connected()) return;
    mqtt_.serviceUntilCommand(durationMs);
}

void GreenhouseController::loop() {
    if (!initialized_) {
        if (outputsOff()) {
            power_.enterDeepSleep(
                fan_, lamp_, mqtt_, network_, appConfig_.runtime().debug);
        }
        delay(1000);
        return;
    }

    uint32_t cycleStart = millis();
    runCycle();

    uint32_t elapsed = millis() - cycleStart;
    if (elapsed < Hardware::ACTIVE_CYCLE_INTERVAL_MS) {
        serviceWhileAwake(Hardware::ACTIVE_CYCLE_INTERVAL_MS - elapsed);
    }
}
