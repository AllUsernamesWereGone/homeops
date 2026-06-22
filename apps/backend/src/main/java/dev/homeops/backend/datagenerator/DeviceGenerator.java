package dev.homeops.backend.datagenerator;

import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceStatus;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.device.DeviceType;
import dev.homeops.backend.repository.DeviceRepository;
import jakarta.annotation.PostConstruct;
import org.springframework.context.annotation.Profile;
import org.springframework.stereotype.Component;

import java.util.Set;

@Component
@Profile("generateData")
public class DeviceGenerator {
    private final DeviceRepository deviceRepository;

    public DeviceGenerator(DeviceRepository deviceRepository) {
        this.deviceRepository = deviceRepository;
    }

    @PostConstruct
    public void initializeDevices() {
        createIfMissing(
            "raspberry-pi-4",
            "Raspberry Pi 4",
            DeviceType.SERVER,
            DeviceStatus.ONLINE,
            DeviceTransport.DOCKER,
            "Staging Docker host",
            "Home Lab",
            "Current Raspberry Pi deployment target for HomeOps.",
            Set.of(
                DeviceCapability.DOCKER_HOST,
                DeviceCapability.FAN_SWITCH,
                DeviceCapability.MQTT_BROKER
            )
        );

        createIfMissing(
            "mini-pc",
            "Mini PC",
            DeviceType.SERVER,
            DeviceStatus.ALERT,
            DeviceTransport.DOCKER,
            "Future production host",
            "Home Lab",
            "Future main host for backend, frontend, MQTT broker, database and monitoring.",
            Set.of(
                DeviceCapability.DOCKER_HOST,
                DeviceCapability.FAN_SWITCH,
                DeviceCapability.MQTT_BROKER
            )
        );

        createIfMissing(
            "greenhouse-esp32-01",
            "Greenhouse ESP32 Controller",
            DeviceType.MICROCONTROLLER,
            DeviceStatus.ALERT,
            DeviceTransport.MQTT,
            "Plant sensor and actuator controller",
            "Greenhouse",
            "Future ESP32 controller for temperature, humidity, light, fan and grow light control.",
            Set.of(
                DeviceCapability.TEMPERATURE_SENSOR,
                DeviceCapability.HUMIDITY_SENSOR,
                DeviceCapability.LIGHT_SENSOR,
                DeviceCapability.FAN_SWITCH,
                DeviceCapability.LIGHT_SWITCH
            )
        );

        createIfMissing(
            "antminer-s7",
            "Antminer S7",
            DeviceType.MINER,
            DeviceStatus.UNKNOWN,
            DeviceTransport.HTTP,
            "Bitcoin miner monitoring target",
            "Home Lab",
            "Future mining status and hashrate monitoring target.",
            Set.of(
                DeviceCapability.HASHRATE_MONITORING,
                DeviceCapability.TEMPERATURE_MONITORING
            )
        );

        createIfMissing(
            "old-laptop",
            "Old Laptop",
            DeviceType.LAPTOP,
            DeviceStatus.UNKNOWN,
            DeviceTransport.SSH,
            "Future CI runner or worker node",
            "Home Lab",
            "Old laptop with broken screen, possible future CI runner or lightweight server.",
            Set.of(
                DeviceCapability.CI_RUNNER,
                DeviceCapability.CAMERA_SNAPSHOT
            )
        );
    }

    private void createIfMissing(
        String deviceId,
        String displayName,
        DeviceType type,
        DeviceStatus status,
        DeviceTransport transport,
        String role,
        String location,
        String description,
        Set<DeviceCapability> capabilities
    ) {
        if (deviceRepository.existsByDeviceId(deviceId)) {
            return;
        }

        Device device = Device.builder()
            .deviceId(deviceId)
            .displayName(displayName)
            .type(type)
            .status(status)
            .transport(transport)
            .role(role)
            .location(location)
            .description(description)
            .enabled(true)
            .capabilities(capabilities)
            .build();

        deviceRepository.save(device);
    }
}
