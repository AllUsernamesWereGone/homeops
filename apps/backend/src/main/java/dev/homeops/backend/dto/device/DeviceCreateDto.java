package dev.homeops.backend.dto.device;

import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.device.DeviceType;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;

import java.util.Set;

public record DeviceCreateDto(
    @NotBlank
    @Size(min = 3, max = 80)
    String deviceId,

    @NotBlank
    @Size(max = 120)
    String displayName,

    @NotNull
    DeviceType type,

    DeviceTransport transport,

    @Size(max = 255)
    String role,

    @Size(max = 120)
    String location,

    @Size(max = 1000)
    String description,

    Set<DeviceCapability> capabilities
) {
}
