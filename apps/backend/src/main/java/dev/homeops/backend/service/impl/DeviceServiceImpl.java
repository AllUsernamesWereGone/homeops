package dev.homeops.backend.service.impl;

import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.mapper.DeviceMapper;
import dev.homeops.backend.repository.DeviceRepository;
import dev.homeops.backend.service.DeviceService;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

import java.util.List;

@Service
public class DeviceServiceImpl implements DeviceService {

    private final DeviceRepository deviceRepository;
    private final DeviceMapper deviceMapper;

    public DeviceServiceImpl(DeviceRepository deviceRepository, DeviceMapper deviceMapper) {
        this.deviceRepository = deviceRepository;
        this.deviceMapper = deviceMapper;
    }

    @Override
    @Transactional(readOnly = true)
    public List<DeviceDto> findAll() {
        return deviceRepository.findAllByOrderByDisplayNameAsc()
            .stream()
            .map(deviceMapper::toDto)
            .toList();
    }

    @Override
    @Transactional(readOnly = true)
    public DeviceDto findByDeviceId(String deviceId) {
        Device device = findEntityByDeviceId(deviceId);
        return deviceMapper.toDto(device);
    }

    @Override
    @Transactional
    public DeviceDto create(DeviceCreateDto request) {
        String normalizedDeviceId = deviceMapper.normalizeDeviceId(request.deviceId());

        if (deviceRepository.existsByDeviceId(normalizedDeviceId)) {
            throw new ResponseStatusException(
                HttpStatus.CONFLICT,
                "Device " + normalizedDeviceId + " already exists"
            );
        }

        Device device = deviceMapper.toEntity(request);
        Device savedDevice = deviceRepository.save(device);

        return deviceMapper.toDto(savedDevice);
    }

    @Override
    @Transactional
    public DeviceDto update(String deviceId, DeviceUpdateDto request) {
        Device device = findEntityByDeviceId(deviceId);
        assertVersionMatches(device, request.version());

        deviceMapper.updateEntity(device, request);
        Device savedDevice = deviceRepository.save(device);

        return deviceMapper.toDto(savedDevice);
    }

    @Override
    @Transactional
    public void delete(String deviceId) {
        Device device = findEntityByDeviceId(deviceId);
        deviceRepository.delete(device);
    }

    private Device findEntityByDeviceId(String deviceId) {
        String normalizedDeviceId = deviceMapper.normalizeDeviceId(deviceId);

        return deviceRepository.findByDeviceId(normalizedDeviceId)
            .orElseThrow(() -> new ResponseStatusException(
                HttpStatus.NOT_FOUND,
                "Device " + normalizedDeviceId + " not found"
            ));
    }

    private void assertVersionMatches(Device device, Long requestVersion) {
        if (!device.getVersion().equals(requestVersion)) {
            throw new ResponseStatusException(
                HttpStatus.CONFLICT,
                "Device was modified by another request"
            );
        }
    }
}
