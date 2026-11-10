package dev.homeops.backend.basetest;

import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceDto;
import dev.homeops.backend.dto.device.DeviceTelemetryStateDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceStatus;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.device.DeviceType;
import dev.homeops.backend.entity.mqtt.MqttMessageType;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.json.JsonMapper;

import java.time.Instant;
import java.util.Set;

public interface TestData {

    String DEVICE_ID_GREENHOUSE = "greenhouse-esp32-01";
    String DEVICE_ID_RASPBERRY_PI = "raspberry-pi-4";
    String DEVICE_ID_TEST = "test-device-01";
    String DEVICE_ID_MISSING = "missing-device";

    String DEVICE_NAME_GREENHOUSE = "Greenhouse ESP32 Controller";
    String DEVICE_NAME_RASPBERRY_PI = "Raspberry Pi 4";
    String DEVICE_NAME_TEST = "Test Device";

    Instant TEST_INSTANT = Instant.parse("2026-06-22T12:00:00Z");

    static Device createGreenhouseDevice() {
        return Device.builder()
            .deviceId(DEVICE_ID_GREENHOUSE)
            .displayName(DEVICE_NAME_GREENHOUSE)
            .type(DeviceType.MICROCONTROLLER)
            .status(DeviceStatus.UNKNOWN)
            .transport(DeviceTransport.MQTT)
            .role("Plant sensor and actuator controller")
            .location("Greenhouse")
            .description("Future ESP32 controller for greenhouse automation.")
            .enabled(true)
            .capabilities(Set.of(
                DeviceCapability.TEMPERATURE_SENSOR,
                DeviceCapability.HUMIDITY_SENSOR,
                DeviceCapability.LIGHT_SENSOR,
                DeviceCapability.FAN_SWITCH,
                DeviceCapability.LIGHT_SWITCH
            ))
            .build();
    }

    static Device createRaspberryPiDevice() {
        return Device.builder()
            .deviceId(DEVICE_ID_RASPBERRY_PI)
            .displayName(DEVICE_NAME_RASPBERRY_PI)
            .type(DeviceType.SERVER)
            .status(DeviceStatus.ONLINE)
            .transport(DeviceTransport.DOCKER)
            .role("Staging Docker host")
            .location("Home Lab")
            .description("Current Raspberry Pi deployment target for HomeOps.")
            .enabled(true)
            .capabilities(Set.of(
                DeviceCapability.DOCKER_HOST,
                DeviceCapability.MQTT_BROKER
            ))
            .build();
    }

    static Device createTestDevice() {
        return Device.builder()
            .deviceId(DEVICE_ID_TEST)
            .displayName(DEVICE_NAME_TEST)
            .type(DeviceType.OTHER)
            .status(DeviceStatus.UNKNOWN)
            .transport(DeviceTransport.MANUAL)
            .role("Testing")
            .location("Desk")
            .description("Temporary test device.")
            .enabled(true)
            .capabilities(Set.of(DeviceCapability.TEMPERATURE_SENSOR))
            .build();
    }

    static DeviceCreateDto createDeviceCreateRequest() {
        return new DeviceCreateDto(
            DEVICE_ID_TEST,
            DEVICE_NAME_TEST,
            DeviceType.OTHER,
            DeviceTransport.MANUAL,
            "Testing",
            "Desk",
            "Temporary test device.",
            Set.of(DeviceCapability.TEMPERATURE_SENSOR)
        );
    }

    static DeviceUpdateDto createDeviceUpdateRequest(long version) {
        return new DeviceUpdateDto(
            "Updated Test Device",
            DeviceType.OTHER,
            DeviceStatus.ONLINE,
            DeviceTransport.MANUAL,
            "Updated testing role",
            "Updated desk",
            "Updated test device.",
            true,
            Set.of(DeviceCapability.FAN_SWITCH),
            version
        );
    }

    static DeviceDto createGreenhouseDeviceDto() {
        return new DeviceDto(
            DEVICE_ID_GREENHOUSE,
            DEVICE_NAME_GREENHOUSE,
            DeviceType.MICROCONTROLLER,
            DeviceStatus.ONLINE,
            DeviceTransport.MQTT,
            "Plant sensor and actuator controller",
            "Greenhouse",
            "Future ESP32 controller for greenhouse automation.",
            true,
            Set.of(
                DeviceCapability.TEMPERATURE_SENSOR,
                DeviceCapability.HUMIDITY_SENSOR
            ),
            null,
            TEST_INSTANT,
            TEST_INSTANT,
            0L
        );
    }

    static DeviceDto createRaspberryPiDeviceDto() {
        return new DeviceDto(
            DEVICE_ID_RASPBERRY_PI,
            DEVICE_NAME_RASPBERRY_PI,
            DeviceType.SERVER,
            DeviceStatus.ONLINE,
            DeviceTransport.DOCKER,
            "Staging Docker host",
            "Home Lab",
            "Current Raspberry Pi deployment target for HomeOps.",
            true,
            Set.of(
                DeviceCapability.DOCKER_HOST,
                DeviceCapability.MQTT_BROKER
            ),
            TEST_INSTANT,
            TEST_INSTANT,
            TEST_INSTANT,
            0L
        );
    }

    static DeviceDto createTestDeviceDto() {
        return new DeviceDto(
            DEVICE_ID_TEST,
            DEVICE_NAME_TEST,
            DeviceType.OTHER,
            DeviceStatus.ONLINE,
            DeviceTransport.MANUAL,
            "Testing",
            "Desk",
            "Temporary test device.",
            true,
            Set.of(DeviceCapability.FAN_SWITCH),
            null,
            TEST_INSTANT,
            TEST_INSTANT,
            0L
        );
    }

    static DeviceTelemetryStateDto createGreenhouseTelemetryStateDto() {
        JsonMapper jsonMapper = new JsonMapper();

        JsonNode data = jsonMapper.readTree("""
            {
              "readings": {
                "temperatureC": {
                  "value": 24.6,
                  "unit": "C"
                },
                "humidityPercent": {
                  "value": 63.2,
                  "unit": "%"
                },
                "fanRpm": {
                  "value": 1200,
                  "unit": "rpm"
                }
              },
              "outputs": {
                "fan1": {
                  "state": "ON",
                  "rpm": 1200
                },
                "light1": {
                  "state": "OFF"
                }
              }
            }
            """);

        return new DeviceTelemetryStateDto(
            DEVICE_ID_GREENHOUSE,
            MqttMessageType.TELEMETRY,
            1,
            TEST_INSTANT,
            TEST_INSTANT,
            data
        );
    }
}
