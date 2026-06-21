package dev.homeops.backend.endpoint.dto;

public record SystemInfoDto(
    String name,
    String version,
    String profile,
    String status
) {
}
