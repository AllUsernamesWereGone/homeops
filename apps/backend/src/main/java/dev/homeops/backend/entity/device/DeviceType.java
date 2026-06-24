package dev.homeops.backend.entity.device;

import io.swagger.v3.oas.annotations.media.Schema;

@Schema(enumAsRef = true)
public enum DeviceType {
    SERVER,
    MICROCONTROLLER,
    SENSOR,
    ACTUATOR,
    CAMERA,
    MINER,
    LAPTOP,
    ROUTER,
    OTHER
}
