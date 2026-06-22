package dev.homeops.backend.unittest;

import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.entity.device.DeviceStatus;
import dev.homeops.backend.entity.device.DeviceTransport;
import dev.homeops.backend.entity.device.DeviceType;
import dev.homeops.backend.exception.ConflictException;
import dev.homeops.backend.exception.NotFoundException;
import dev.homeops.backend.mapper.DeviceMapper;
import dev.homeops.backend.repository.DeviceRepository;
import dev.homeops.backend.service.impl.DeviceServiceImpl;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import java.util.List;
import java.util.Optional;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
public class DeviceServiceTest {

    @Mock
    private DeviceRepository deviceRepository;

    private DeviceServiceImpl deviceService;

    @BeforeEach
    void setUp() {
        deviceService = new DeviceServiceImpl(deviceRepository, new DeviceMapper());
    }

    @Test
    void givenDevices_whenFindAll_thenReturnDeviceDtos() {
        Device raspberryPi = createDevice("raspberry-pi-4", "Raspberry Pi 4", DeviceType.SERVER);
        Device greenhouse = createDevice("greenhouse-esp32-01", "Greenhouse ESP32", DeviceType.MICROCONTROLLER);

        when(deviceRepository.findAllByOrderByDisplayNameAsc())
            .thenReturn(List.of(greenhouse, raspberryPi));

        List<DeviceDto> result = deviceService.findAll();

        assertEquals(2, result.size());
        assertEquals("greenhouse-esp32-01", result.get(0).deviceId());
        assertEquals("raspberry-pi-4", result.get(1).deviceId());

        verify(deviceRepository).findAllByOrderByDisplayNameAsc();
    }

    @Test
    void givenExistingDevice_whenFindByDeviceId_thenReturnDeviceDto() {
        Device device = createDevice("greenhouse-esp32-01", "Greenhouse ESP32", DeviceType.MICROCONTROLLER);

        when(deviceRepository.findByDeviceId("greenhouse-esp32-01"))
            .thenReturn(Optional.of(device));

        DeviceDto result = deviceService.findByDeviceId("greenhouse-esp32-01");

        assertEquals("greenhouse-esp32-01", result.deviceId());
        assertEquals("Greenhouse ESP32", result.displayName());
        assertEquals(DeviceType.MICROCONTROLLER, result.type());
    }

    @Test
    void givenDeviceIdWithUppercase_whenFindByDeviceId_thenNormalizeAndReturnDeviceDto() {
        Device device = createDevice("greenhouse-esp32-01", "Greenhouse ESP32", DeviceType.MICROCONTROLLER);

        when(deviceRepository.findByDeviceId("greenhouse-esp32-01"))
            .thenReturn(Optional.of(device));

        DeviceDto result = deviceService.findByDeviceId(" Greenhouse-ESP32-01 ");

        assertEquals("greenhouse-esp32-01", result.deviceId());
        verify(deviceRepository).findByDeviceId("greenhouse-esp32-01");
    }

    @Test
    void givenMissingDevice_whenFindByDeviceId_thenThrowNotFound() {
        when(deviceRepository.findByDeviceId("missing-device"))
            .thenReturn(Optional.empty());

        NotFoundException exception = assertThrows(
            NotFoundException.class,
            () -> deviceService.findByDeviceId("missing-device")
        );

        assertEquals("Device missing-device not found", exception.getMessage());
    }

    @Test
    void givenDuplicateDeviceId_whenCreate_thenThrowConflict() {
        DeviceCreateDto request = createRequest("greenhouse-esp32-01");

        when(deviceRepository.existsByDeviceId("greenhouse-esp32-01"))
            .thenReturn(true);

        ConflictException exception = assertThrows(
            ConflictException.class,
            () -> deviceService.create(request)
        );

        assertEquals("Device greenhouse-esp32-01 already exists", exception.getMessage());
    }

    @Test
    void givenValidRequest_whenCreate_thenNormalizeDeviceIdAndSaveDevice() {
        DeviceCreateDto request = new DeviceCreateDto(
            " Greenhouse-ESP32-01 ",
            " Greenhouse ESP32 ",
            DeviceType.MICROCONTROLLER,
            DeviceTransport.MQTT,
            "Plant controller",
            "Greenhouse",
            "Controls greenhouse sensors and actuators.",
            Set.of(DeviceCapability.TEMPERATURE_SENSOR, DeviceCapability.HUMIDITY_SENSOR)
        );

        when(deviceRepository.existsByDeviceId("greenhouse-esp32-01"))
            .thenReturn(false);
        when(deviceRepository.save(any(Device.class)))
            .thenAnswer(invocation -> invocation.getArgument(0));

        DeviceDto result = deviceService.create(request);

        assertEquals("greenhouse-esp32-01", result.deviceId());
        assertEquals("Greenhouse ESP32", result.displayName());
        assertEquals(DeviceType.MICROCONTROLLER, result.type());
        assertEquals(DeviceStatus.UNKNOWN, result.status());
        assertEquals(DeviceTransport.MQTT, result.transport());
        assertTrue(result.enabled());
        assertTrue(result.capabilities().contains(DeviceCapability.TEMPERATURE_SENSOR));

        ArgumentCaptor<Device> captor = ArgumentCaptor.forClass(Device.class);
        verify(deviceRepository).save(captor.capture());

        Device savedDevice = captor.getValue();
        assertEquals("greenhouse-esp32-01", savedDevice.getDeviceId());
        assertEquals("Greenhouse ESP32", savedDevice.getDisplayName());
    }

    @Test
    void givenCreateRequestWithoutStatusTransportAndEnabled_whenCreate_thenUseDefaults() {
        DeviceCreateDto request = new DeviceCreateDto(
            "test-device-01",
            "Test Device",
            DeviceType.OTHER,
            null,
            null,
            null,
            null,
            null
        );

        when(deviceRepository.existsByDeviceId("test-device-01"))
            .thenReturn(false);
        when(deviceRepository.save(any(Device.class)))
            .thenAnswer(invocation -> invocation.getArgument(0));

        DeviceDto result = deviceService.create(request);

        assertEquals(DeviceStatus.UNKNOWN, result.status());
        assertEquals(DeviceTransport.UNKNOWN, result.transport());
        assertTrue(result.enabled());
        assertNotNull(result.capabilities());
        assertTrue(result.capabilities().isEmpty());
    }

    @Test
    void givenExistingDeviceAndMatchingVersion_whenUpdate_thenUpdateAndReturnDevice() {
        Device device = createDevice("greenhouse-esp32-01", "Old Name", DeviceType.MICROCONTROLLER);
        device.setVersion(3L);

        DeviceUpdateDto request = new DeviceUpdateDto(
            "New Greenhouse Controller",
            DeviceType.MICROCONTROLLER,
            DeviceStatus.ONLINE,
            DeviceTransport.MQTT,
            "Updated role",
            "Updated location",
            "Updated description",
            true,
            Set.of(DeviceCapability.TEMPERATURE_SENSOR, DeviceCapability.LIGHT_SENSOR),
            3L
        );

        when(deviceRepository.findByDeviceId("greenhouse-esp32-01"))
            .thenReturn(Optional.of(device));
        when(deviceRepository.save(any(Device.class)))
            .thenAnswer(invocation -> invocation.getArgument(0));

        DeviceDto result = deviceService.update("greenhouse-esp32-01", request);

        assertEquals("New Greenhouse Controller", result.displayName());
        assertEquals(DeviceStatus.ONLINE, result.status());
        assertEquals("Updated role", result.role());
        assertTrue(result.capabilities().contains(DeviceCapability.LIGHT_SENSOR));
        assertFalse(result.capabilities().contains(DeviceCapability.HUMIDITY_SENSOR));
    }

    @Test
    void givenExistingDeviceAndWrongVersion_whenUpdate_thenThrowConflict() {
        Device device = createDevice("greenhouse-esp32-01", "Greenhouse ESP32", DeviceType.MICROCONTROLLER);
        device.setVersion(3L);

        DeviceUpdateDto request = new DeviceUpdateDto(
            "Greenhouse ESP32",
            DeviceType.MICROCONTROLLER,
            DeviceStatus.ONLINE,
            DeviceTransport.MQTT,
            "Role",
            "Location",
            "Description",
            true,
            Set.of(DeviceCapability.TEMPERATURE_SENSOR),
            2L
        );

        when(deviceRepository.findByDeviceId("greenhouse-esp32-01"))
            .thenReturn(Optional.of(device));

        ConflictException exception = assertThrows(
            ConflictException.class,
            () -> deviceService.update("greenhouse-esp32-01", request)
        );

        assertEquals("Device was modified by another request", exception.getMessage());
    }

    @Test
    void givenExistingDevice_whenDelete_thenDeleteDevice() {
        Device device = createDevice("greenhouse-esp32-01", "Greenhouse ESP32", DeviceType.MICROCONTROLLER);

        when(deviceRepository.findByDeviceId("greenhouse-esp32-01"))
            .thenReturn(Optional.of(device));

        deviceService.delete("greenhouse-esp32-01");

        verify(deviceRepository).delete(device);
    }

    private DeviceCreateDto createRequest(String deviceId) {
        return new DeviceCreateDto(
            deviceId,
            "Test Device",
            DeviceType.OTHER,
            DeviceTransport.MANUAL,
            "Testing",
            "Desk",
            "Temporary test device",
            Set.of()
        );
    }

    private Device createDevice(String deviceId, String displayName, DeviceType type) {
        Device device = Device.builder()
            .deviceId(deviceId)
            .displayName(displayName)
            .type(type)
            .status(DeviceStatus.UNKNOWN)
            .transport(DeviceTransport.MQTT)
            .role("Test role")
            .location("Test location")
            .description("Test description")
            .enabled(true)
            .capabilities(Set.of(DeviceCapability.TEMPERATURE_SENSOR, DeviceCapability.HUMIDITY_SENSOR))
            .build();

        device.setVersion(0L);
        return device;
    }

}
