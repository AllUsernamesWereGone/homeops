#include <Arduino.h>
#include "GreenhouseController.h"

GreenhouseController greenhouse;

void setup() {
    greenhouse.begin();
}

void loop() {
    greenhouse.loop();
}
