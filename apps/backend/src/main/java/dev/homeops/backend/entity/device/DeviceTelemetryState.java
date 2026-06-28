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
import jakarta.persistence.OneToOne;
import jakarta.persistence.PostLoad;
import jakarta.persistence.PostPersist;
import jakarta.persistence.Table;
import jakarta.persistence.Transient;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.springframework.data.domain.Persistable;

import java.time.Instant;
import java.util.UUID;

@Getter
@Setter
@NoArgsConstructor
@Entity
@Table(name = "device_telemetry_state")
public class DeviceTelemetryState implements Persistable<UUID> {

    @Id
    @Column(name = "device_pk", nullable = false)
    private UUID id;

    @OneToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(
        name = "device_pk",
        referencedColumnName = "id",
        nullable = false,
        insertable = false,
        updatable = false
    )
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

    @Transient
    private boolean newEntity = true;

    public DeviceTelemetryState(Device device) {
        this.id = device.getId();
        this.device = device;
    }

    @Override
    public boolean isNew() {
        return newEntity;
    }

    @PostLoad
    @PostPersist
    void markNotNew() {
        this.newEntity = false;
    }
}
