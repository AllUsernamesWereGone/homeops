package dev.homeops.backend.entity.device;

import dev.homeops.backend.entity.mqtt.MqttMessageType;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.FetchType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.Lob;
import jakarta.persistence.MapsId;
import jakarta.persistence.OneToOne;
import jakarta.persistence.Table;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

import java.time.Instant;
import java.util.UUID;

@Getter
@Setter
@NoArgsConstructor
@Entity
@Table(name = "device_telemetry_state")
public class DeviceTelemetryState {

    @Id
    @Column(name = "device_id")
    private UUID deviceId;

    @MapsId
    @OneToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "device_id")
    private Device device;

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

    public DeviceTelemetryState(Device device) {
        this.device = device;
        this.deviceId = device.getId();
    }
}
