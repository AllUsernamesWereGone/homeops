package dev.homeops.backend.dto;

public record SystemInfoDto(
    String name,
    String version,
    String profile,
    String status
) {
}
