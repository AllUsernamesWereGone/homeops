package dev.homeops.backend.entity.device;

import io.swagger.v3.oas.annotations.media.Schema;

@Schema(enumAsRef = true)
public enum DeviceCapability {
    TEMPERATURE_SENSOR,
    HUMIDITY_SENSOR,
    LIGHT_SENSOR,

    FAN_SWITCH,
    FAN_RPM_CONTROL,
    LIGHT_SWITCH,

    CAMERA_SNAPSHOT,
    HASHRATE_MONITORING,
    TEMPERATURE_MONITORING,

    CPU_METRICS,
    MEMORY_METRICS,
    DISK_METRICS,

    DOCKER_HOST,
    MQTT_BROKER,
    CI_RUNNER,

    OTHER
}
