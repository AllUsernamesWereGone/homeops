package dev.homeops.backend.dto.mqtt;

import tools.jackson.databind.JsonNode;

import java.time.Instant;

public record MqttEnvelopeDto(
    Integer schemaVersion,
    String deviceId,
    Instant timestamp,
    JsonNode data
) {
}
