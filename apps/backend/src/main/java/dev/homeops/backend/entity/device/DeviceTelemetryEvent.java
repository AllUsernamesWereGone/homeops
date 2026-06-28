package dev.homeops.backend.entity.device;

import dev.homeops.backend.entity.mqtt.MqttMessageType;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.Lob;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.Table;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

import java.time.Instant;

@Getter
@Setter
@NoArgsConstructor
@Entity
@Table(name = "device_telemetry_event")
public class DeviceTelemetryEvent {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "device_id", nullable = false)
    private Device device;

    @Column(nullable = false, length = 255)
    private String topic;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false, length = 32)
    private MqttMessageType messageType;

    @Column(nullable = false)
    private Integer schemaVersion;

    @Column(nullable = false)
    private Instant reportedAt;

    @Column(nullable = false)
    private Instant receivedAt;

    @Lob
    @Column(nullable = false)
    private String dataJson;

    @Lob
    @Column(nullable = false)
    private String rawPayload;

    public DeviceTelemetryEvent(
        Device device,
        String topic,
        MqttMessageType messageType,
        Integer schemaVersion,
        Instant reportedAt,
        Instant receivedAt,
        String dataJson,
        String rawPayload
    ) {
        this.device = device;
        this.topic = topic;
        this.messageType = messageType;
        this.schemaVersion = schemaVersion;
        this.reportedAt = reportedAt;
        this.receivedAt = receivedAt;
        this.dataJson = dataJson;
        this.rawPayload = rawPayload;
    }
}
