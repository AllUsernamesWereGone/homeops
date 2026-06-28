package dev.homeops.backend.service;

import org.springframework.messaging.Message;

public interface MqttService {

    void handleIncomingMessage(Message<?> message);
}
