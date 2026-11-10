package dev.homeops.backend.service;

import dev.homeops.backend.dto.device.DeviceCommandRequestDto;
import dev.homeops.backend.dto.device.DeviceCommandResultDto;

public interface DeviceCommandService {
    DeviceCommandResultDto publishCommand(String deviceId, DeviceCommandRequestDto request);
}
