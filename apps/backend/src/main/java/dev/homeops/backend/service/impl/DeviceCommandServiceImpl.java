package dev.homeops.backend.service.impl;

import dev.homeops.backend.config.mqtt.MqttProperties;
import dev.homeops.backend.dto.device.DeviceCommandRequestDto;
import dev.homeops.backend.dto.device.DeviceCommandResultDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.exception.BadRequestException;
import dev.homeops.backend.exception.NotFoundException;
import dev.homeops.backend.mapper.DeviceMapper;
import dev.homeops.backend.repository.DeviceRepository;
import dev.homeops.backend.service.DeviceCommandService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.integration.mqtt.support.MqttHeaders;
import org.springframework.messaging.Message;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.support.MessageBuilder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import tools.jackson.databind.json.JsonMapper;
import tools.jackson.databind.node.ObjectNode;

import java.time.Instant;

@Service
public class DeviceCommandServiceImpl implements DeviceCommandService {

    private static final Logger LOGGER = LoggerFactory.getLogger(DeviceCommandServiceImpl.class);

    private final DeviceRepository deviceRepository;
    private final DeviceMapper deviceMapper;
    private final MqttProperties mqttProperties;
    private final JsonMapper jsonMapper;
    private final MessageChannel mqttOutboundChannel;

    public DeviceCommandServiceImpl(
        DeviceRepository deviceRepository,
        DeviceMapper deviceMapper,
        MqttProperties mqttProperties,
        JsonMapper jsonMapper,
        MessageChannel mqttOutboundChannel
    ) {
        this.deviceRepository = deviceRepository;
        this.deviceMapper = deviceMapper;
        this.mqttProperties = mqttProperties;
        this.jsonMapper = jsonMapper;
        this.mqttOutboundChannel = mqttOutboundChannel;
    }

    @Override
    @Transactional(readOnly = true)
    public DeviceCommandResultDto publishCommand(String deviceId, DeviceCommandRequestDto request) {
        String normalizedDeviceId = deviceMapper.normalizeDeviceId(deviceId);

        Device device = deviceRepository.findByDeviceId(normalizedDeviceId)
            .orElseThrow(() -> new NotFoundException(
                "Device " + normalizedDeviceId + " not found"
            ));

        if (!device.isEnabled()) {
            throw new BadRequestException(
                "Cannot publish command to disabled device " + normalizedDeviceId
            );
        }

        Instant publishedAt = Instant.now();
        String topic = mqttProperties.commandTopic(normalizedDeviceId);
        String payload = buildPayload(normalizedDeviceId, request, publishedAt);

        Message<String> message = MessageBuilder
            .withPayload(payload)
            .setHeader(MqttHeaders.TOPIC, topic)
            .setHeader(MqttHeaders.QOS, 1)
            .setHeader(MqttHeaders.RETAINED, false)
            .build();

        boolean sent = mqttOutboundChannel.send(message);

        if (!sent) {
            throw new BadRequestException(
                "Could not publish MQTT command for device " + normalizedDeviceId
            );
        }

        LOGGER.info(
            "Published MQTT command deviceId={} topic={} command={} target={} property={}",
            normalizedDeviceId,
            topic,
            request.command(),
            request.target(),
            request.property()
        );

        return new DeviceCommandResultDto(
            normalizedDeviceId,
            topic,
            request.command(),
            request.target(),
            request.property(),
            publishedAt
        );
    }

    private String buildPayload(
        String deviceId,
        DeviceCommandRequestDto request,
        Instant publishedAt
    ) {
        ObjectNode data = jsonMapper.createObjectNode();
        data.put("command", request.command());
        data.put("target", request.target());
        data.put("property", request.property());
        data.set("value", request.value());

        ObjectNode envelope = jsonMapper.createObjectNode();
        envelope.put("schemaVersion", 1);
        envelope.put("deviceId", deviceId);
        envelope.put("timestamp", publishedAt.toString());
        envelope.set("data", data);

        return jsonMapper.writeValueAsString(envelope);
    }
}
