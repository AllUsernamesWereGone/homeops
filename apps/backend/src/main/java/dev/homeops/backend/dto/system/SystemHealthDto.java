package dev.homeops.backend.dto.system;

import java.time.Instant;

public record SystemHealthDto(
    String status,
    Instant checkedAt,
    boolean backendUp
) {
}
