package dev.homeops.backend.dto.device;

import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceStatus;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.device.DeviceType;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;

import java.util.Set;

public record DeviceUpdateDto(
    @NotBlank
    @Size(max = 120)
    String displayName,

    @NotNull
    DeviceType type,

    @NotNull
    DeviceStatus status,

    @NotNull
    DeviceTransport transport,

    @Size(max = 255)
    String role,

    @Size(max = 120)
    String location,

    @Size(max = 1000)
    String description,

    @NotNull
    Boolean enabled,

    Set<DeviceCapability> capabilities,

    @NotNull
    Long version
) {
}
