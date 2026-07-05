package dev.homeops.backend.dto.device;

import java.time.Instant;

public record DeviceCommandResultDto(
    String deviceId,
    String topic,
    String command,
    String target,
    String property,
    Instant publishedAt
) {
}
