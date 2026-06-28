package dev.homeops.backend.entity.mqtt;

import java.util.Locale;

public enum MqttMessageType {

    TELEMETRY,
    STATUS,
    COMMAND_RESULT;

    public static MqttMessageType fromTopicSegment(String value) {
        return switch (value.toLowerCase(Locale.ROOT)) {
            case "telemetry" -> TELEMETRY;
            case "status" -> STATUS;
            case "command-result" -> COMMAND_RESULT;
            default -> throw new IllegalArgumentException(
                "Unsupported MQTT message type: " + value
            );
        };
    }
}
