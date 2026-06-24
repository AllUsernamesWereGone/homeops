package dev.homeops.backend.entity.device;

import io.swagger.v3.oas.annotations.media.Schema;

@Schema(enumAsRef = true)
public enum DeviceStatus {
    ONLINE,
    OFFLINE,
    UNKNOWN,
    ALERT,
    ERROR
}
