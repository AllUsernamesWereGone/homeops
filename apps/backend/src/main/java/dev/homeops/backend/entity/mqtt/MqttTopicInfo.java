package dev.homeops.backend.entity.mqtt;

public record MqttTopicInfo(
    String topicPrefix,
    String deviceId,
    MqttMessageType messageType
) {

    public static MqttTopicInfo fromTopic(String topic, String expectedTopicPrefix) {
        String[] parts = topic.split("/");

        if (parts.length != 4) {
            throw new IllegalArgumentException(
                "Invalid MQTT topic format: " + topic
            );
        }

        if (!expectedTopicPrefix.equals(parts[0])) {
            throw new IllegalArgumentException(
                "Unexpected MQTT topic prefix: " + parts[0]
            );
        }

        if (!"devices".equals(parts[1])) {
            throw new IllegalArgumentException(
                "Invalid MQTT topic category: " + parts[1]
            );
        }

        String deviceId = parts[2];
        MqttMessageType messageType = MqttMessageType.fromTopicSegment(parts[3]);

        return new MqttTopicInfo(expectedTopicPrefix, deviceId, messageType);
    }
}
