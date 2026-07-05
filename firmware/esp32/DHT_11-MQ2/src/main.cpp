#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN 32          // only correct if DHT11 DATA is on GPIO32
#define DHT_TYPE DHT11

#define MQ2_ANALOG_PIN 34   // MQ-2 AO should go to GPIO34 through divider

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 DHT11 + MQ-2 test starting...");

  dht.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(MQ2_ANALOG_PIN, ADC_11db);
}

void loop() {
  float humidity = dht.readHumidity();
  float temperatureC = dht.readTemperature();

  int mq2Raw = analogRead(MQ2_ANALOG_PIN);
  int mq2MilliVolts = analogReadMilliVolts(MQ2_ANALOG_PIN);

  static int minValue = 4095;
  static int maxValue = 0;

  if (mq2Raw < minValue) {
    minValue = mq2Raw;
  }

  if (mq2Raw > maxValue) {
    maxValue = mq2Raw;
  }

  Serial.println("----- sensor reading -----");

  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println("DHT11 read failed");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperatureC);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  Serial.print("MQ-2 raw: ");
  Serial.println(mq2Raw);

  Serial.print("min: ");
  Serial.print(minValue);

  Serial.print(" | max: ");
  Serial.println(maxValue);

  Serial.print("GPIO voltage: ");
  Serial.print(mq2MilliVolts);
  Serial.println(" mV");

  Serial.print("Timestamp: ");
  Serial.println(millis());

  delay(2000);
}