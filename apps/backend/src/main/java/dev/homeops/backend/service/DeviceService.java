package dev.homeops.backend.service;

import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;

import java.util.List;

public interface DeviceService {

    List<DeviceDto> findAll();

    DeviceDto findByDeviceId(String deviceId);

    DeviceDto create(DeviceCreateDto request);

    DeviceDto update(String deviceId, DeviceUpdateDto request);

    void delete(String deviceId);
}
