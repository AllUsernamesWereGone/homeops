package dev.homeops.backend.service.impl;

import dev.homeops.backend.dto.device.DeviceTelemetryStateDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceTelemetryState;
import dev.homeops.backend.exception.NotFoundException;
import dev.homeops.backend.mapper.DeviceMapper;
import dev.homeops.backend.repository.DeviceRepository;
import dev.homeops.backend.repository.DeviceTelemetryStateRepository;
import dev.homeops.backend.service.DeviceTelemetryStateService;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.json.JsonMapper;

@Service
public class DeviceTelemetryStateServiceImpl implements DeviceTelemetryStateService {

    private final DeviceRepository deviceRepository;
    private final DeviceTelemetryStateRepository stateRepository;
    private final DeviceMapper deviceMapper;
    private final JsonMapper jsonMapper;

    public DeviceTelemetryStateServiceImpl(
        DeviceRepository deviceRepository,
        DeviceTelemetryStateRepository stateRepository,
        DeviceMapper deviceMapper,
        JsonMapper jsonMapper
    ) {
        this.deviceRepository = deviceRepository;
        this.stateRepository = stateRepository;
        this.deviceMapper = deviceMapper;
        this.jsonMapper = jsonMapper;
    }

    @Override
    @Transactional(readOnly = true)
    public DeviceTelemetryStateDto findLatestByDeviceId(String deviceId) {
        String normalizedDeviceId = deviceMapper.normalizeDeviceId(deviceId);

        Device device = deviceRepository.findByDeviceId(normalizedDeviceId)
            .orElseThrow(() -> new NotFoundException(
                "Device " + normalizedDeviceId + " not found"
            ));

        DeviceTelemetryState state = stateRepository.findById(device.getId())
            .orElseThrow(() -> new NotFoundException(
                "No latest state found for device " + normalizedDeviceId
            ));

        JsonNode data = jsonMapper.readTree(state.getDataJson());

        return new DeviceTelemetryStateDto(
            device.getDeviceId(),
            state.getMessageType(),
            state.getSchemaVersion(),
            state.getReportedAt(),
            state.getReceivedAt(),
            data
        );
    }
}
