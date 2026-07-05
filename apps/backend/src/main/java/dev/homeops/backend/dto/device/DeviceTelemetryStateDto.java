package dev.homeops.backend.dto.device;

import dev.homeops.backend.entity.mqtt.MqttMessageType;
import tools.jackson.databind.JsonNode;

import java.time.Instant;

public record DeviceTelemetryStateDto(
    String deviceId,
    MqttMessageType messageType,
    Integer schemaVersion,
    Instant reportedAt,
    Instant receivedAt,
    JsonNode data
) {
}
