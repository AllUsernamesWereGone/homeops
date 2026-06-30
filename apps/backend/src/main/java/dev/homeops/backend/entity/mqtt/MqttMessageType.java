package dev.homeops.backend.entity.mqtt;

import java.util.Locale;

public enum MqttMessageType {

    TELEMETRY,
    STATUS,
    COMMAND,
    COMMAND_RESULT,
    HELLO,
    STATE,
    ERROR,
    CONFIG;

    public static MqttMessageType fromTopicSegment(String value) {
        return switch (value.toLowerCase(Locale.ROOT)) {
            case "telemetry" -> TELEMETRY;
            case "status" -> STATUS;
            case "command" -> COMMAND;
            case "command-result" -> COMMAND_RESULT;
            case "hello" -> HELLO;
            case "state" -> STATE;
            case "error" -> ERROR;
            case "config" -> CONFIG;
            default -> throw new IllegalArgumentException(
                "Unsupported MQTT message type: " + value
            );
        };
    }
}
