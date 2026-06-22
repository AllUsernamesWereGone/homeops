package dev.homeops.backend.dto.device;

import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceStatus;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.device.DeviceType;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.Set;

@Schema(description = "Device registry entry exposed by the HomeOps API.")
public record DeviceDto(
    @Schema(description = "Stable external device id used in API paths and MQTT topics.", example = "greenhouse-esp32-01")
    String deviceId,

    @Schema(description = "Human-readable device name.", example = "Greenhouse ESP32 Controller")
    String displayName,

    @Schema(description = "General device category.", example = "MICROCONTROLLER")
    DeviceType type,

    @Schema(description = "Current registry status of the device.", example = "PLANNED")
    DeviceStatus status,

    @Schema(description = "Primary communication or management transport.", example = "MQTT")
    DeviceTransport transport,

    @Schema(description = "Short functional role of the device.", example = "Plant sensor and actuator controller")
    String role,

    @Schema(description = "Physical or logical location.", example = "Greenhouse")
    String location,

    @Schema(description = "Longer free-text description.")
    String description,

    @Schema(description = "Whether this device is enabled in HomeOps.")
    boolean enabled,

    @Schema(description = "Capabilities supported by this device.")
    Set<DeviceCapability> capabilities,

    @Schema(description = "Last time the device was observed online or reported data.")
    Instant lastSeenAt,

    @Schema(description = "Creation timestamp.")
    Instant createdAt,

    @Schema(description = "Last update timestamp.")
    Instant updatedAt,

    @Schema(description = "Optimistic locking version.")
    Long version

) {
}
