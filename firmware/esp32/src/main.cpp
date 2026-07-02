#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Preferences.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include "secrets.h"
#include <ArduinoJson.h>

#define PIN_FAN_SENSE   34
#define PIN_FAN_PWM     18
#define PIN_FAN_PWR     27
#define PIN_PHOTO_SENSE 32
#define PIN_DHT_SENSE   25
#define PIN_BUTTON_SIM  19

#define BUTTON_PULSE_MS     150
#define BUTTON_SETTLE_MS    250

#define PHOTO_SAMPLES       10
#define PHOTO_SAMPLE_GAP_MS 50
#define PHOTO_DELTA_THRESH  0.15f

#define LAMP_LEVEL_MIN  0
#define LAMP_LEVEL_MAX  4
#define PWM_TARGET_MIN  0
#define PWM_TARGET_MAX  255

#define PWM_CHANNEL    0
#define PWM_FREQ       25000
#define PWM_RESOLUTION 8

#define DHT_TYPE DHT11
#define DHT_SAMPLES         5
#define DHT_SAMPLE_GAP_MS   1000
#define DHT_MIN_VALID       3
#define DHT_TEMP_MIN        -20.0f
#define DHT_TEMP_MAX        60.0f
#define DHT_HUM_MIN         0.0f
#define DHT_HUM_MAX         100.0f

DHT dht(PIN_DHT_SENSE, DHT_TYPE);

struct DHTData {
    float temp;
    float hum;
    bool ok;
};

#define ESP_SLEEP_SECONDS       60
#define MQTT_COMMAND_WAIT_MS    2000
#define WIFI_CONNECT_TIMEOUT_MS 6000
#define TACH_SAMPLE_MS          500

RTC_DATA_ATTR bool      DEBUG              = true;

RTC_DATA_ATTR int8_t    rtc_lamp_level     = 0;
RTC_DATA_ATTR int8_t    rtc_lamp_target    = 0;
RTC_DATA_ATTR bool      rtc_lamp_synced    = false;

RTC_DATA_ATTR uint8_t   rtc_pwm_target     = 0;
RTC_DATA_ATTR bool      rtc_fan_pwr        = false;
RTC_DATA_ATTR uint32_t  rtc_boot_count     = 0;

Preferences prefs;

WiFiClient wifiClient;
WiFiUDP ntpUDP;

PubSubClient mqtt(wifiClient);

NTPClient timeClient(ntpUDP, "pool.ntp.org");

volatile uint32_t tachPulseCount = 0;
void IRAM_ATTR onTachPulse() { tachPulseCount++; }

int16_t cmdTargetLevel = -1;
int16_t cmdTargetPwm   = -1;

char topicCmd[48];
char topicState[48];

void loadPersistedState() {
    prefs.begin("lamp", true);

    rtc_lamp_level   = prefs.getChar("lamp_level", 0);
    rtc_lamp_target  = prefs.getChar("lamp_target", 0);
    rtc_pwm_target   = prefs.getUChar("pwm_target", 0);
    rtc_lamp_synced  = prefs.getBool("lamp_synced", false);
    rtc_fan_pwr      = prefs.getBool("pwr", false);

    prefs.end();

    if (DEBUG) {
        Serial.println("Loaded persisted state:");
        Serial.print("lamp_level: "); Serial.println(rtc_lamp_level);
        Serial.print("lamp_target: "); Serial.println(rtc_lamp_target);
        Serial.print("pwm_target: "); Serial.println(rtc_pwm_target);
        Serial.print("lamp_synced: "); Serial.println(rtc_lamp_synced);
        Serial.print("fan_pwr: "); Serial.println(rtc_fan_pwr);
    }
}

void savePersistedState() {
    prefs.begin("lamp", false);

    prefs.putChar("lamp_level", rtc_lamp_level);
    prefs.putChar("lamp_target", rtc_lamp_target);
    prefs.putUChar("pwm_target", rtc_pwm_target);
    prefs.putBool("lamp_synced", rtc_lamp_synced);
    prefs.putBool("pwr", rtc_fan_pwr);

    prefs.end();

    if (DEBUG) {
        Serial.println("Saved persisted state:");
        Serial.print("lamp_level: "); Serial.println(rtc_lamp_level);
        Serial.print("lamp_target: "); Serial.println(rtc_lamp_target);
        Serial.print("pwm_target: "); Serial.println(rtc_pwm_target);
        Serial.print("lamp_synced: "); Serial.println(rtc_lamp_synced);
        Serial.print("fan_pwr: "); Serial.println(rtc_fan_pwr);
    }
}

float readPhotoVolts() {
    uint32_t sum = 0;

    for (int i = 0; i < PHOTO_SAMPLES; i++) {
        sum += analogRead(PIN_PHOTO_SENSE);
        delay(PHOTO_SAMPLE_GAP_MS);
    }

    float volts = (sum / (float)PHOTO_SAMPLES) * (3.3f / 4095.0f);

    if (DEBUG) {
        Serial.print("Photo sensor voltage: ");
        Serial.println(volts);
    }

    return volts;
}

DHTData readDHT() {
    DHTData d;
    float tempSum = 0.0f, humSum = 0.0f;
    int validCount = 0;

    for (int i = 0; i < DHT_SAMPLES; i++) {
        float t = dht.readTemperature();
        float h = dht.readHumidity();

        bool valid = !(isnan(t) || isnan(h)) &&
                     t >= DHT_TEMP_MIN && t <= DHT_TEMP_MAX &&
                     h >= DHT_HUM_MIN  && h <= DHT_HUM_MAX;

        if (valid) {
            tempSum += t;
            humSum  += h;
            validCount++;
        } else if (DEBUG) {
            Serial.print("DHT sample rejected: t=");
            Serial.print(t);
            Serial.print(" h=");
            Serial.println(h);
        }

        if (i < DHT_SAMPLES - 1) delay(DHT_SAMPLE_GAP_MS);
    }

    if (validCount >= DHT_MIN_VALID) {
        d.temp = tempSum / validCount;
        d.hum  = humSum / validCount;
        d.ok   = true;
    } else {
        d.temp = -1;
        d.hum  = -1;
        d.ok   = false;
        if (DEBUG) {
            Serial.print("DHT read failed: only ");
            Serial.print(validCount);
            Serial.print("/");
            Serial.print(DHT_SAMPLES);
            Serial.println(" samples valid");
        }
    }

    return d;
}

uint32_t sampleTachRPM() {
    noInterrupts();
    tachPulseCount = 0;
    interrupts();

    delay(TACH_SAMPLE_MS);

    noInterrupts();
    uint32_t p = tachPulseCount;
    interrupts();

    uint32_t rpm = (uint32_t)((p * 60000.0f / TACH_SAMPLE_MS) / 4.0f);

    if (DEBUG) {
        Serial.print("Tach pulses: ");
        Serial.print(p);
        Serial.print(" | RPM: ");
        Serial.println(rpm);
    }

    return rpm;
}

uint32_t setFanState(uint8_t pwm_target) {

    if (pwm_target == 0) {
        rtc_fan_pwr = false;
        rtc_pwm_target = 0;
        digitalWrite(PIN_FAN_PWR, LOW);
        ledcWrite(PWM_CHANNEL, 255);
    } else {
        pwm_target = constrain(pwm_target, 1, 255);

        bool wasOff = !rtc_fan_pwr;

        rtc_fan_pwr = true;
        rtc_pwm_target = pwm_target;
        digitalWrite(PIN_FAN_PWR, HIGH);

        if (wasOff) {
            ledcWrite(PWM_CHANNEL, 0);
            delay(300);
        }

        ledcWrite(PWM_CHANNEL, 255 - pwm_target);
    }

    delay(300);
    uint32_t rpm = sampleTachRPM();

    if (DEBUG) {
        Serial.println("=== FAN STATE SET ===");
        Serial.print("Target PWM: ");
        Serial.println(pwm_target);
        Serial.print("Actual RPM: ");
        Serial.println(rpm);
    }

    return rpm;
}

void initFanSystem() {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_FAN_PWM, PWM_CHANNEL);

    if (DEBUG) {
        Serial.println("=== FAN BOOT INIT ===");
        Serial.print("Restoring RTC PWM target: ");
        Serial.println(rtc_pwm_target);
    }

    setFanState(rtc_fan_pwr ? rtc_pwm_target : 0);
}

void pulseButton() {
    digitalWrite(PIN_BUTTON_SIM, HIGH);
    delay(BUTTON_PULSE_MS);
    digitalWrite(PIN_BUTTON_SIM, LOW);
    delay(BUTTON_SETTLE_MS);

    if (DEBUG) {
        Serial.println("Button pressed");
    }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<192> doc;
  DeserializationError err = deserializeJson(doc, payload, length);

  if (err) {
    if (DEBUG) {
      Serial.print("JSON parse failed: ");
      Serial.println(err.c_str());
    }
    return;
  }

  bool updated = false;

  if (doc.containsKey("lamp_level")) {
    JsonVariant v = doc["lamp_level"];
    if (v.is<int>()) {
      int val = v.as<int>();
      if (val >= LAMP_LEVEL_MIN && val <= LAMP_LEVEL_MAX) {
        cmdTargetLevel = val;
        updated = true;
      } else {
        cmdTargetLevel = -1;
        if (DEBUG) {
          Serial.print("lamp_level out of range, rejected: ");
          Serial.println(val);
        }
      }
    } else {
      cmdTargetLevel = -1;
      if (DEBUG) Serial.println("Parsing JSON lamp_level: wrong type, ignoring");
    }
  }

  if (doc.containsKey("pwm_target")) {
    JsonVariant v = doc["pwm_target"];
    if (v.is<int>()) {
      int val = v.as<int>();
      if (val >= PWM_TARGET_MIN && val <= PWM_TARGET_MAX) {
        cmdTargetPwm = val;
        updated = true;
      } else {
        cmdTargetPwm = -1;
        if (DEBUG) {
          Serial.print("pwm_target out of range, rejected: ");
          Serial.println(val);
        }
      }
    } else {
      cmdTargetPwm = -1;
      if (DEBUG) Serial.println("Parsing JSON pwm_target: wrong type, ignoring");
    }
  }

  if (DEBUG && updated) {
    Serial.println("MQTT command parsed:");
    Serial.print("lamp_level: ");
    Serial.println(cmdTargetLevel);
    Serial.print("pwm_target: ");
    Serial.println(cmdTargetPwm);
  }
}
bool connectWiFi() {
    WiFi.mode(WIFI_STA);

    if (DEBUG) {
        Serial.println("WiFi: starting connection...");
        Serial.print("SSID: ");
        Serial.println(WIFI_SSID);
    }

    IPAddress local_IP(192, 168, 50, 29);
    IPAddress gateway(192, 168, 50, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.config(local_IP, gateway, subnet);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    unsigned long t = millis();
    int lastStatus = -1;

    while (WiFi.status() != WL_CONNECTED &&
           millis() - t < WIFI_CONNECT_TIMEOUT_MS) {

        int status = WiFi.status();

        if (DEBUG && status != lastStatus) {
            Serial.print("WiFi status changed: ");
            Serial.print(status);
            Serial.print(" -> ");

            switch (status) {
                case WL_IDLE_STATUS:     Serial.println("IDLE"); break;
                case WL_NO_SSID_AVAIL:   Serial.println("NO SSID AVAILABLE"); break;
                case WL_SCAN_COMPLETED:  Serial.println("SCAN COMPLETED"); break;
                case WL_CONNECTED:       Serial.println("CONNECTED"); break;
                case WL_CONNECT_FAILED:  Serial.println("CONNECT FAILED"); break;
                case WL_CONNECTION_LOST: Serial.println("CONNECTION LOST"); break;
                case WL_DISCONNECTED:    Serial.println("DISCONNECTED"); break;
                default:                 Serial.println("UNKNOWN"); break;
            }

            lastStatus = status;
        }

        delay(200);
    }

    bool ok = (WiFi.status() == WL_CONNECTED);

    if (DEBUG) {
        if (ok) {
            Serial.println("WiFi connected");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("RSSI: ");
            Serial.println(WiFi.RSSI());
        } else {
            Serial.println("WiFi connection failed (timeout)");
            Serial.print("Final status: ");
            Serial.println(WiFi.status());
        }
    }

    return ok;
}

bool connectMqtt() {
    String id = String(DEVICE_ID);

    bool connected = mqtt.connect(id.c_str());

    if (DEBUG) {
        if (connected) {
            Serial.println("MQTT connection successful");
        } else {
            Serial.print("MQTT connection failed, state: ");
            Serial.println(mqtt.state());
        }
    }

    return connected;
}

void publishState(uint32_t rpm, DHTData d, float pv) {
    StaticJsonDocument<256> doc;

    if (d.ok) {
        doc["temperature"] = d.temp;
        doc["humidity"] = d.hum;
    } else {
        doc["temperature"] = -1;
        doc["humidity"] = -1;
        if (DEBUG) Serial.println("publishState: DHT invalid, publishing -1");
    }

    if (isnan(pv) || pv < 0) {
        doc["photo_volt"] = -1;
        if (DEBUG) Serial.println("publishState: photo_volt invalid, publishing -1");
    } else {
        doc["photo_volt"] = pv;
    }

    doc["rpm"] = rpm;
    doc["pwm"] = rtc_pwm_target;
    doc["lamp_level"] = rtc_lamp_level;

    char buf[256];
    size_t len = serializeJson(doc, buf, sizeof(buf));

    if (DEBUG) {
        Serial.println("MQTT payload:");
        Serial.println(buf);
    }

    bool pubOk = mqtt.publish(topicState, buf, len);
    if (DEBUG) {
        Serial.print("Publish result: ");
        Serial.println(pubOk ? "OK" : "FAILED");
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    pinMode(PIN_FAN_SENSE, INPUT_PULLUP);
    pinMode(PIN_FAN_PWR, OUTPUT);
    pinMode(PIN_BUTTON_SIM, OUTPUT);
    digitalWrite(PIN_FAN_PWR, LOW);
    digitalWrite(PIN_BUTTON_SIM, LOW);

    attachInterrupt(digitalPinToInterrupt(PIN_FAN_SENSE), onTachPulse, FALLING);

    dht.begin();

    rtc_boot_count++;

    if (DEBUG) {
        Serial.println("=== BOOT ===");
        Serial.print("Boot count: ");
        Serial.println(rtc_boot_count);
    }

    loadPersistedState();
    initFanSystem();

    snprintf(topicCmd, sizeof(topicCmd), "greenhouse/%s/command", DEVICE_ID);
    snprintf(topicState, sizeof(topicState), "greenhouse/%s/telemetry", DEVICE_ID);

}

void runCycle() {
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    if (!wifiOk) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        wifiOk = connectWiFi();
    } else if (DEBUG) {
        Serial.println("WiFi already connected, skipping reconnect");
    }

    bool mqttOk = false;

    if (wifiOk) {
        if (mqtt.connected()) {
            if (!mqtt.loop()) {
                if (DEBUG) Serial.println("MQTT stale, forcing reconnect");
                mqtt.disconnect();
            } else {
                mqttOk = true;
                if (DEBUG) Serial.println("MQTT already connected, skipping reconnect");
            }
        }

        if (!mqttOk) {
            mqtt.setServer(MQTT_HOST, MQTT_PORT);
            mqtt.setCallback(onMqttMessage);
            mqtt.setBufferSize(512);
            mqttOk = connectMqtt();

            if (mqttOk) {
                mqtt.subscribe(topicCmd);
                if (DEBUG) {
                    Serial.print("Subscribed to: ");
                    Serial.println(topicCmd);
                }
            }
        }
    } else if (DEBUG) {
        Serial.println("Skipping MQTT, no WiFi");
    }

    DHTData dhtData    = readDHT();
    float   photoVolts = readPhotoVolts();
    uint32_t rpm        = sampleTachRPM();

    if (DEBUG) {
        Serial.println("=== SENSOR READINGS ===");
        Serial.print("Temp C: ");   Serial.println(dhtData.temp);
        Serial.print("Hum %: ");    Serial.println(dhtData.hum);
        Serial.print("Photo V: ");  Serial.println(photoVolts);
        Serial.print("RPM: ");      Serial.println(rpm);
    }

    if (mqttOk) {
        publishState(rpm, dhtData, photoVolts);
    }

    cmdTargetLevel = -1;
    cmdTargetPwm   = -1;

    if (mqttOk) {
        if (DEBUG) Serial.println("Waiting for MQTT command...");

        unsigned long waitStart = millis();
        while (millis() - waitStart < MQTT_COMMAND_WAIT_MS) {
            mqtt.loop();
            delay(20);
        }
    }

    bool stateChanged = false;

    if (cmdTargetPwm >= 0) {
        if (DEBUG) {
            Serial.print("Applying pwm_target command: ");
            Serial.println(cmdTargetPwm);
        }
        rpm = setFanState((uint8_t)cmdTargetPwm);
        stateChanged = true;
    }

    if (cmdTargetLevel >= 0 && cmdTargetLevel != rtc_lamp_level) {
        int steps = abs((int)cmdTargetLevel - (int)rtc_lamp_level);

        if (DEBUG) {
            Serial.print("Applying lamp_level command: ");
            Serial.print(rtc_lamp_level);
            Serial.print(" -> ");
            Serial.println(cmdTargetLevel);
            Serial.print("Button presses needed: ");
            Serial.println(steps);
        } 
        
        //actual Stepping Logic Incomplete, TODO

        for (int i = 0; i < steps; i++) {
            pulseButton();
        }

        rtc_lamp_level  = cmdTargetLevel;
        rtc_lamp_target = cmdTargetLevel;
        rtc_lamp_synced = true;
        stateChanged = true;
    }

    if (stateChanged) {
        savePersistedState();

        if (mqttOk) {
            mqtt.publish(topicCmd, "", true);
            publishState(rpm, dhtData, photoVolts);
        }
    } else if (DEBUG) {
        Serial.println("No command applied, nothing to persist");
    }
}

void loop() {
    runCycle();

    if (rtc_pwm_target > 0) {
        if (DEBUG) {
            Serial.println("Fan active — staying awake, will re-poll shortly");
        }
        delay(ESP_SLEEP_SECONDS * 1000UL);
    } else {
        if (mqtt.connected()) mqtt.disconnect();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        if (DEBUG) {
            Serial.print("Fan off — entering DEEP sleep for ");
            Serial.print(ESP_SLEEP_SECONDS);
            Serial.println("s");
            Serial.flush();
        }
        esp_sleep_enable_timer_wakeup((uint64_t)ESP_SLEEP_SECONDS * 1000000ULL);
        esp_deep_sleep_start();
    }
}
