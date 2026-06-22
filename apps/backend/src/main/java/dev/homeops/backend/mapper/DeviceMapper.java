package dev.homeops.backend.mapper;

import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceTransport;
import org.springframework.stereotype.Component;

import java.util.LinkedHashSet;
import java.util.Locale;
import java.util.Set;


@Component
public class DeviceMapper {

    public DeviceDto toDto(Device device) {
        return new DeviceDto(
            device.getDeviceId(),
            device.getDisplayName(),
            device.getType(),
            device.getStatus(),
            device.getTransport(),
            device.getRole(),
            device.getLocation(),
            device.getDescription(),
            device.isEnabled(),
            new LinkedHashSet<>(device.getCapabilities()),
            device.getLastSeenAt(),
            device.getCreatedAt(),
            device.getUpdatedAt(),
            device.getVersion()
        );
    }

    public Device toEntity(DeviceCreateDto request) {
        return Device.builder()
            .deviceId(normalizeDeviceId(request.deviceId()))
            .displayName(request.displayName().trim())
            .type(request.type())
            .transport(request.transport() != null ? request.transport() : DeviceTransport.UNKNOWN)
            .role(trimToNull(request.role()))
            .location(trimToNull(request.location()))
            .description(trimToNull(request.description()))
            .enabled(true)
            .capabilities(copyCapabilities(request.capabilities()))
            .build();
    }

    public void updateEntity(Device device, DeviceUpdateDto request) {
        device.setDisplayName(request.displayName().trim());
        device.setType(request.type());
        device.setStatus(request.status());
        device.setTransport(request.transport());
        device.setRole(trimToNull(request.role()));
        device.setLocation(trimToNull(request.location()));
        device.setDescription(trimToNull(request.description()));
        device.setEnabled(request.enabled());

        device.getCapabilities().clear();
        device.getCapabilities().addAll(copyCapabilities(request.capabilities()));
    }

    public String normalizeDeviceId(String deviceId) {
        return deviceId.trim().toLowerCase(Locale.ROOT);
    }

    private Set<DeviceCapability> copyCapabilities(Set<DeviceCapability> capabilities) {
        if (capabilities == null) {
            return new LinkedHashSet<>();
        }

        return new LinkedHashSet<>(capabilities);
    }

    private String trimToNull(String value) {
        if (value == null || value.isBlank()) {
            return null;
        }

        return value.trim();
    }
}
