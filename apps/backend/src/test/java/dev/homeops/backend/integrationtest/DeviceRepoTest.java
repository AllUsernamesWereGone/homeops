package dev.homeops.backend.integrationtest;

import dev.homeops.backend.basetest.TestData;
import dev.homeops.backend.entity.device.Device;
import dev.homeops.backend.entity.device.DeviceCapability;
import dev.homeops.backend.repository.DeviceRepository;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.data.jpa.test.autoconfigure.DataJpaTest;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.test.context.ActiveProfiles;

import java.util.List;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

@DataJpaTest
@ActiveProfiles("test")
class DeviceRepoTest implements TestData {

    @Autowired
    private DeviceRepository deviceRepository;

    @Test
    void givenDevice_whenSave_thenCanFindByDeviceIdWithCapabilities() {
        Device savedDevice = deviceRepository.saveAndFlush(TestData.createGreenhouseDevice());

        Optional<Device> result = deviceRepository.findByDeviceId(savedDevice.getDeviceId());

        assertTrue(result.isPresent());
        assertEquals(DEVICE_ID_GREENHOUSE, result.get().getDeviceId());
        assertEquals(DEVICE_NAME_GREENHOUSE, result.get().getDisplayName());
        assertTrue(result.get().getCapabilities().contains(DeviceCapability.TEMPERATURE_SENSOR));
        assertTrue(result.get().getCapabilities().contains(DeviceCapability.HUMIDITY_SENSOR));
    }

    @Test
    void givenDevices_whenFindAllByOrderByDisplayNameAsc_thenReturnSortedByDisplayName() {
        deviceRepository.save(TestData.createRaspberryPiDevice());
        deviceRepository.save(TestData.createGreenhouseDevice());
        deviceRepository.flush();

        List<Device> result = deviceRepository.findAllByOrderByDisplayNameAsc();

        assertEquals(2, result.size());
        assertEquals(DEVICE_NAME_GREENHOUSE, result.get(0).getDisplayName());
        assertEquals(DEVICE_NAME_RASPBERRY_PI, result.get(1).getDisplayName());
    }

    @Test
    void givenExistingDevice_whenExistsByDeviceId_thenReturnTrue() {
        deviceRepository.saveAndFlush(TestData.createGreenhouseDevice());

        assertTrue(deviceRepository.existsByDeviceId(DEVICE_ID_GREENHOUSE));
        assertFalse(deviceRepository.existsByDeviceId(DEVICE_ID_MISSING));
    }

    @Test
    void givenExistingDevice_whenFindByDeviceIdWithWrongId_thenReturnEmpty() {
        deviceRepository.saveAndFlush(TestData.createGreenhouseDevice());

        Optional<Device> result = deviceRepository.findByDeviceId(DEVICE_ID_MISSING);

        assertTrue(result.isEmpty());
    }

    @Test
    void givenDuplicateDeviceId_whenSave_thenThrowDataIntegrityViolation() {
        Device firstDevice = TestData.createTestDevice();

        Device secondDevice = Device.builder()
            .deviceId(DEVICE_ID_TEST)
            .displayName("Second Test Device")
            .type(firstDevice.getType())
            .status(firstDevice.getStatus())
            .transport(firstDevice.getTransport())
            .role(firstDevice.getRole())
            .location(firstDevice.getLocation())
            .description(firstDevice.getDescription())
            .enabled(true)
            .capabilities(firstDevice.getCapabilities())
            .build();

        deviceRepository.save(firstDevice);
        deviceRepository.save(secondDevice);

        assertThrows(DataIntegrityViolationException.class, () -> deviceRepository.flush());
    }
}
