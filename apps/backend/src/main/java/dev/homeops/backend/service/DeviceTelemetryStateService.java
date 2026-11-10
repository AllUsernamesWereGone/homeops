package dev.homeops.backend.service;

import dev.homeops.backend.dto.device.DeviceTelemetryStateDto;

public interface DeviceTelemetryStateService {

    DeviceTelemetryStateDto findLatestByDeviceId(String deviceId);
}
