package dev.homeops.backend.service.impl;

import dev.homeops.backend.service.DeviceTelemetryIngestionService;
import dev.homeops.backend.service.MqttService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.integration.mqtt.support.MqttHeaders;
import org.springframework.messaging.Message;
import org.springframework.stereotype.Service;

@Service
public class MqttServiceImpl implements MqttService {

    private static final Logger LOGGER = LoggerFactory.getLogger(MqttServiceImpl.class);

    private final DeviceTelemetryIngestionService telemetryIngestionService;

    public MqttServiceImpl(DeviceTelemetryIngestionService telemetryIngestionService) {
        this.telemetryIngestionService = telemetryIngestionService;
    }

    @Override
    public void handleIncomingMessage(Message<?> message) {
        String topic = String.valueOf(message.getHeaders().get(MqttHeaders.RECEIVED_TOPIC));
        String payload = String.valueOf(message.getPayload());

        LOGGER.info("MQTT message received topic={} payload={}", topic, payload);

        try {
            telemetryIngestionService.ingest(topic, payload);
        } catch (RuntimeException exception) {
            LOGGER.warn("MQTT message ignored topic={} reason={}", topic, exception.getMessage());
            LOGGER.debug("MQTT message handling failed", exception);
        }
    }
}
