package dev.homeops.backend.dto.device;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;
import tools.jackson.databind.JsonNode;

public record DeviceCommandRequestDto(
    @NotBlank
    @Size(max = 255)
    String command,

    @NotBlank
    @Size(max = 255)
    String target,

    @NotBlank
    @Size(max = 255)
    String property,

    @NotNull
    JsonNode value
) {
}
