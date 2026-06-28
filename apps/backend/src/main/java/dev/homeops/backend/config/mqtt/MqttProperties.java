package dev.homeops.backend.config.mqtt;

import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;

@Setter
@Getter
@ConfigurationProperties(prefix = "homeops.mqtt")
public class MqttProperties {

    private boolean enabled = true;
    private String brokerUrl = "tcp://localhost:1883";
    private String clientId = "homeops-backend";
    private String topicPrefix = "homeops";


    public String telemetryTopicPattern() {
        return topicPrefix + "/devices/+/telemetry";
    }

    public String statusTopicPattern() {
        return topicPrefix + "/devices/+/status";
    }

    public String commandResultTopicPattern() {
        return topicPrefix + "/devices/+/command-result";
    }

    public String commandTopic(String deviceId) {
        return topicPrefix + "/devices/" + deviceId + "/command";
    }
}
