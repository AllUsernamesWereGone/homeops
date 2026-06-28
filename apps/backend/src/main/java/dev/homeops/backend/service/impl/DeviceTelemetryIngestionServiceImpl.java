package dev.homeops.backend.service.impl;


import dev.homeops.backend.config.mqtt.MqttProperties;
import dev.homeops.backend.dto.mqtt.MqttEnvelopeDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceStatus;
import dev.homeops.backend.entity.device.DeviceTelemetryEvent;
import dev.homeops.backend.entity.device.DeviceTelemetryState;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.mqtt.MqttMessageType;
import dev.homeops.backend.entity.mqtt.MqttTopicInfo;
import dev.homeops.backend.repository.DeviceRepository;
import dev.homeops.backend.repository.DeviceTelemetryEventRepository;
import dev.homeops.backend.repository.DeviceTelemetryStateRepository;
import dev.homeops.backend.service.DeviceTelemetryIngestionService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.json.JsonMapper;

import java.time.Instant;

@Service
public class DeviceTelemetryIngestionServiceImpl implements DeviceTelemetryIngestionService {

    private static final Logger LOGGER =
        LoggerFactory.getLogger(DeviceTelemetryIngestionServiceImpl.class);

    private final DeviceRepository deviceRepository;
    private final DeviceTelemetryStateRepository stateRepository;
    private final DeviceTelemetryEventRepository eventRepository;
    private final MqttProperties mqttProperties;
    private final JsonMapper jsonMapper;

    public DeviceTelemetryIngestionServiceImpl(
        DeviceRepository deviceRepository,
        DeviceTelemetryStateRepository stateRepository,
        DeviceTelemetryEventRepository eventRepository,
        MqttProperties mqttProperties,
        JsonMapper jsonMapper
    ) {
        this.deviceRepository = deviceRepository;
        this.stateRepository = stateRepository;
        this.eventRepository = eventRepository;
        this.mqttProperties = mqttProperties;
        this.jsonMapper = jsonMapper;
    }

    @Override
    @Transactional
    public void ingest(String topic, String payload) {
        MqttTopicInfo topicInfo = MqttTopicInfo.fromTopic(
            topic,
            mqttProperties.getTopicPrefix()
        );

        MqttEnvelopeDto envelope = parseEnvelope(payload);
        Device device = findEnabledDevice(topicInfo.deviceId());

        Instant receivedAt = Instant.now();
        Instant reportedAt = envelope.timestamp() == null
            ? receivedAt
            : envelope.timestamp();

        Integer schemaVersion = envelope.schemaVersion() == null
            ? 1
            : envelope.schemaVersion();

        String dataJson = serializeData(envelope.data());

        touchDevice(device, receivedAt);
        saveLatestState(
            device,
            topicInfo.messageType(),
            schemaVersion,
            reportedAt,
            receivedAt,
            dataJson,
            payload
        );
        saveTelemetryEvent(
            device,
            topic,
            topicInfo.messageType(),
            schemaVersion,
            reportedAt,
            receivedAt,
            dataJson,
            payload
        );

        LOGGER.info(
            "Stored MQTT {} for deviceId={}",
            topicInfo.messageType(),
            topicInfo.deviceId()
        );
    }

    private MqttEnvelopeDto parseEnvelope(String payload) {
        return jsonMapper.readValue(payload, MqttEnvelopeDto.class);
    }

    private Device findEnabledDevice(String deviceId) {
        Device device = deviceRepository.findByDeviceId(deviceId)
            .orElseThrow(() -> new IllegalArgumentException(
                "MQTT message received for unknown deviceId: " + deviceId
            ));

        if (!device.isEnabled()) {
            throw new IllegalArgumentException(
                "MQTT message received for disabled deviceId: " + deviceId
            );
        }

        return device;
    }

    private void touchDevice(Device device, Instant receivedAt) {
        device.setStatus(DeviceStatus.ONLINE);
        device.setTransport(DeviceTransport.MQTT);
        device.setLastSeenAt(receivedAt);
    }

    private void saveLatestState(
        Device device,
        MqttMessageType messageType,
        Integer schemaVersion,
        Instant reportedAt,
        Instant receivedAt,
        String dataJson,
        String rawPayload
    ) {
        if (device.getId() == null) {
            throw new IllegalStateException(
                "Cannot store telemetry for device without internal ID: "
                    + device.getDeviceId()
            );
        }

        DeviceTelemetryState state = stateRepository.findById(device.getId())
            .orElseGet(() -> new DeviceTelemetryState(device));

        state.setMessageType(messageType);
        state.setSchemaVersion(schemaVersion);
        state.setReportedAt(reportedAt);
        state.setReceivedAt(receivedAt);
        state.setDataJson(dataJson);
        state.setRawPayload(rawPayload);

        stateRepository.save(state);
    }

    private void saveTelemetryEvent(
        Device device,
        String topic,
        MqttMessageType messageType,
        Integer schemaVersion,
        Instant reportedAt,
        Instant receivedAt,
        String dataJson,
        String rawPayload
    ) {
        DeviceTelemetryEvent event = new DeviceTelemetryEvent(
            device,
            topic,
            messageType,
            schemaVersion,
            reportedAt,
            receivedAt,
            dataJson,
            rawPayload
        );

        eventRepository.save(event);
    }

    private String serializeData(JsonNode data) {
        JsonNode safeData = data == null
            ? jsonMapper.createObjectNode()
            : data;

        return jsonMapper.writeValueAsString(safeData);
    }
}
