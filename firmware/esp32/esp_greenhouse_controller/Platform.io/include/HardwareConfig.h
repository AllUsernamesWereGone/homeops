#pragma once

#include <Arduino.h>
#include <DHT.h>
#include <time.h>

namespace Hardware {
constexpr uint8_t FAN_SENSE_PIN = 34;
constexpr uint8_t FAN_PWM_PIN = 18;
constexpr uint8_t FAN_POWER_PIN = 27;
constexpr uint8_t PHOTO_SENSE_PIN = 32;
constexpr uint8_t DHT_SENSE_PIN = 25;
constexpr uint8_t LAMP_PWM_PIN = 19;

constexpr uint32_t FAN_PWM_FREQ_HZ = 25000;
constexpr uint32_t LAMP_PWM_FREQ_HZ = 2000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint16_t PWM_MAX = 255;
constexpr uint8_t FAN_TACH_PULSES_PER_REV = 4;

constexpr uint8_t PHOTO_SAMPLES = 10;
constexpr uint32_t PHOTO_SAMPLE_GAP_MS = 25;
constexpr uint32_t PHOTO_VALID_MAX_MV = 3600;

constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t DHT_SAMPLES = 2;
constexpr uint8_t DHT_MIN_VALID_SAMPLES = 1;
constexpr uint32_t DHT_SAMPLE_GAP_MS = 2100;
constexpr float DHT_TEMP_MIN_C = -20.0f;
constexpr float DHT_TEMP_MAX_C = 60.0f;
constexpr float DHT_HUM_MIN_PCT = 0.0f;
constexpr float DHT_HUM_MAX_PCT = 100.0f;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 8000;
constexpr uint32_t TIME_SYNC_TIMEOUT_MS = 5000;
constexpr uint32_t MQTT_COMMAND_WINDOW_MS = 2000;
constexpr uint32_t ACTIVE_CYCLE_INTERVAL_MS = 60000;
constexpr uint32_t DEEP_SLEEP_MS = 60000;
constexpr uint32_t TACH_SAMPLE_MS = 500;
constexpr uint32_t MQTT_LOOP_DELAY_MS = 10;
constexpr uint16_t MQTT_KEEPALIVE_SEC = 30;
constexpr uint16_t MQTT_SOCKET_TIMEOUT_SEC = 5;
constexpr uint16_t MQTT_BUFFER_SIZE = 768;
constexpr time_t MIN_VALID_EPOCH = 1704067200;

constexpr char ENV_FILE_PATH[] = "/.env";
constexpr char MQTT_CA_CERT_PATH[] = "/mqtt_ca.pem";
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.cloudflare.com";
}
