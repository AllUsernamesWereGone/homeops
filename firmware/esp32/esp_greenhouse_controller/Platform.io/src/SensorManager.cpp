#include "SensorManager.h"

#include <cmath>
#include "HardwareConfig.h"

SensorManager::SensorManager()
    : dht_(Hardware::DHT_SENSE_PIN, Hardware::DHT_TYPE) {}

void SensorManager::begin() {
    pinMode(Hardware::PHOTO_SENSE_PIN, INPUT);
    dht_.begin();
}

SensorData SensorManager::read(FanController &fan) {
    SensorData data;

    float tempSum = 0.0f;
    float humSum = 0.0f;
    uint8_t validDht = 0;

    for (uint8_t i = 0; i < Hardware::DHT_SAMPLES; ++i) {
        float t = dht_.readTemperature();
        float h = dht_.readHumidity();
        bool valid = !std::isnan(t) && !std::isnan(h) &&
                     t >= Hardware::DHT_TEMP_MIN_C && t <= Hardware::DHT_TEMP_MAX_C &&
                     h >= Hardware::DHT_HUM_MIN_PCT && h <= Hardware::DHT_HUM_MAX_PCT;

        if (valid) {
            tempSum += t;
            humSum += h;
            ++validDht;
        }

        if (i + 1 < Hardware::DHT_SAMPLES) delay(Hardware::DHT_SAMPLE_GAP_MS);
    }

    if (validDht >= Hardware::DHT_MIN_VALID_SAMPLES) {
        data.temperatureC = tempSum / validDht;
        data.humidityPct = humSum / validDht;
    }

    uint64_t photoMvSum = 0;
    bool photoValid = true;
    for (uint8_t i = 0; i < Hardware::PHOTO_SAMPLES; ++i) {
        uint32_t mv = analogReadMilliVolts(Hardware::PHOTO_SENSE_PIN);
        if (mv > Hardware::PHOTO_VALID_MAX_MV) photoValid = false;
        photoMvSum += mv;
        if (i + 1 < Hardware::PHOTO_SAMPLES) delay(Hardware::PHOTO_SAMPLE_GAP_MS);
    }

    if (photoValid) {
        data.photoVolts =
            (photoMvSum / static_cast<float>(Hardware::PHOTO_SAMPLES)) / 1000.0f;
    }

    data.fanRpm = fan.sampleRpm();
    return data;
}
