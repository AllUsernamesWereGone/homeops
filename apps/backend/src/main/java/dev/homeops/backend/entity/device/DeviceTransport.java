package dev.homeops.backend.entity.device;

import io.swagger.v3.oas.annotations.media.Schema;

@Schema(enumAsRef = true)
public enum DeviceTransport {
    MQTT,
    HTTP,
    SSH,
    DOCKER,
    MANUAL,
    WEBSOCKET,
    OTHER,
    UNKNOWN
}
