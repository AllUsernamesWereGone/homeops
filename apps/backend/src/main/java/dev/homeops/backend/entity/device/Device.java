package dev.homeops.backend.entity.device;

import jakarta.persistence.CollectionTable;
import jakarta.persistence.Column;
import jakarta.persistence.ElementCollection;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.PrePersist;
import jakarta.persistence.PreUpdate;
import jakarta.persistence.Table;
import jakarta.persistence.UniqueConstraint;
import jakarta.persistence.Version;
import lombok.AccessLevel;
import lombok.Builder;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import lombok.Singular;

import java.time.Instant;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.UUID;

@Entity
@Table(
    name = "devices",
    uniqueConstraints = {
        @UniqueConstraint(name = "uk_devices_device_id", columnNames = "device_id")
    }
)
@Getter
@Setter
@NoArgsConstructor(access = AccessLevel.PROTECTED)
public class Device {

    @Id
    @GeneratedValue(strategy = GenerationType.UUID)
    private UUID id;

    @Column(name = "device_id", nullable = false, length = 80)
    private String deviceId;

    @Column(name = "display_name", nullable = false)
    private String displayName;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false, length = 40)
    private DeviceType type;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false, length = 40)
    private DeviceStatus status = DeviceStatus.UNKNOWN;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false, length = 40)
    private DeviceTransport transport = DeviceTransport.UNKNOWN;

    @Column(length = 255)
    private String role;

    @Column(length = 120)
    private String location;

    @Column(length = 1000)
    private String description;

    @Column(name = "last_seen_at")
    private Instant lastSeenAt;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt;

    //TODO add user later
    //@Column(name = "created_by", nullable = false, updatable = false)
    //private String createdBy;

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt;

    //TODO add user later
    //@Column(name = "updated_by", nullable = false)
    //private String updatedBy;

    @Version
    @Column(nullable = false)
    private Long version;

    @Column(nullable = false)
    private boolean enabled = true;

    @ElementCollection(fetch = FetchType.LAZY)
    @CollectionTable(
        name = "device_capabilities",
        joinColumns = @JoinColumn(name = "device_pk"),
        uniqueConstraints = {
            @UniqueConstraint(
                name = "uk_device_capabilities_device_capability",
                columnNames = {"device_pk", "capability"}
            )
        }
    )
    @Enumerated(EnumType.STRING)
    @Column(name = "capability", nullable = false)
    private Set<DeviceCapability> capabilities = new LinkedHashSet<>();

    @PrePersist
    protected void onCreate() {
        //TODO add users on create
        Instant now = Instant.now();
        createdAt = now;
        updatedAt = now;
    }

    @PreUpdate
    protected void onUpdate() {
        //TODO add users on update
        updatedAt = Instant.now();
    }


    @Builder
    private Device(
        String deviceId,
        String displayName,
        DeviceType type,
        String location,
        String role,
        String description,
        DeviceStatus status,
        DeviceTransport transport,
        boolean enabled,
        @Singular Set<DeviceCapability> capabilities
    ) {
        this.deviceId = deviceId;
        this.displayName = displayName;
        this.type = type;
        this.location = location;
        this.role = role;
        this.description = description;
        this.status = status != null ? status : DeviceStatus.UNKNOWN;
        this.transport = transport != null ? transport : DeviceTransport.UNKNOWN;
        this.enabled = enabled;

        if (capabilities != null) {
            this.capabilities.addAll(capabilities);
        }
    }


}
