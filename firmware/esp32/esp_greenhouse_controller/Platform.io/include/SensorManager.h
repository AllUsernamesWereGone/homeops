#pragma once

#include <DHT.h>
#include "FanController.h"
#include "SystemTypes.h"

class SensorManager {
public:
    SensorManager();
    void begin();
    SensorData read(FanController &fan);

private:
    DHT dht_;
};
