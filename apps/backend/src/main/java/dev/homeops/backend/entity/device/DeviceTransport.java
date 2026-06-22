package dev.homeops.backend.entity.device;

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
