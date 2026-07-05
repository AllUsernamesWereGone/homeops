package dev.homeops.backend.integrationtest;

import dev.homeops.backend.basetest.TestData;
import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;
import dev.homeops.backend.endpoint.DeviceEndpoint;
import dev.homeops.backend.service.DeviceService;
import dev.homeops.backend.service.DeviceTelemetryStateService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.webmvc.test.autoconfigure.WebMvcTest;
import org.springframework.http.MediaType;
import org.springframework.test.context.bean.override.mockito.MockitoBean;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;

import static org.hamcrest.Matchers.hasSize;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.delete;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.put;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(DeviceEndpoint.class)
class DeviceEndpointTest implements TestData {

    @Autowired
    private MockMvc mockMvc;

    @MockitoBean
    private DeviceService deviceService;

    @MockitoBean
    private DeviceTelemetryStateService deviceTelemetryStateService;


    @Test
    void givenDevices_whenFindAll_thenReturnDevices() throws Exception {
        when(deviceService.findAll()).thenReturn(List.of(
            TestData.createGreenhouseDeviceDto(),
            TestData.createRaspberryPiDeviceDto()
        ));

        mockMvc.perform(get(DeviceEndpoint.BASE_PATH))
            .andExpect(status().isOk())
            .andExpect(jsonPath("$", hasSize(2)))
            .andExpect(jsonPath("$[0].deviceId").value(DEVICE_ID_GREENHOUSE))
            .andExpect(jsonPath("$[0].displayName").value(DEVICE_NAME_GREENHOUSE))
            .andExpect(jsonPath("$[0].type").value("MICROCONTROLLER"))
            .andExpect(jsonPath("$[0].status").value("ONLINE"))
            .andExpect(jsonPath("$[1].deviceId").value(DEVICE_ID_RASPBERRY_PI))
            .andExpect(jsonPath("$[1].displayName").value(DEVICE_NAME_RASPBERRY_PI));
    }

    @Test
    void givenExistingDevice_whenFindByDeviceId_thenReturnDevice() throws Exception {
        when(deviceService.findByDeviceId(DEVICE_ID_GREENHOUSE))
            .thenReturn(TestData.createGreenhouseDeviceDto());

        mockMvc.perform(get(DeviceEndpoint.BASE_PATH + "/{deviceId}", DEVICE_ID_GREENHOUSE))
            .andExpect(status().isOk())
            .andExpect(jsonPath("$.deviceId").value(DEVICE_ID_GREENHOUSE))
            .andExpect(jsonPath("$.displayName").value(DEVICE_NAME_GREENHOUSE))
            .andExpect(jsonPath("$.type").value("MICROCONTROLLER"))
            .andExpect(jsonPath("$.transport").value("MQTT"));
    }

    @Test
    void givenValidCreateRequest_whenCreate_thenReturnCreatedDevice() throws Exception {
        when(deviceService.create(any(DeviceCreateDto.class)))
            .thenReturn(TestData.createTestDeviceDto());

        mockMvc.perform(post(DeviceEndpoint.BASE_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content("""
                    {
                      "deviceId": "test-device-01",
                      "displayName": "Test Device",
                      "type": "OTHER",
                      "transport": "MANUAL",
                      "role": "Testing",
                      "location": "Desk",
                      "description": "Temporary test device.",
                      "capabilities": []
                    }
                    """))
            .andExpect(status().isCreated())
            .andExpect(jsonPath("$.deviceId").value(DEVICE_ID_TEST))
            .andExpect(jsonPath("$.displayName").value(DEVICE_NAME_TEST));
    }

    @Test
    void givenInvalidCreateRequest_whenCreate_thenReturnBadRequest() throws Exception {
        mockMvc.perform(post(DeviceEndpoint.BASE_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content("""
                    {
                      "deviceId": "",
                      "displayName": "",
                      "type": null,
                      "transport": "MANUAL",
                      "role": "Testing",
                      "location": "Desk",
                      "description": "Temporary test device.",
                      "capabilities": []
                    }
                    """))
            .andExpect(status().isBadRequest());
    }

    @Test
    void givenValidUpdateRequest_whenUpdate_thenReturnUpdatedDevice() throws Exception {
        when(deviceService.update(eq(DEVICE_ID_TEST), any(DeviceUpdateDto.class)))
            .thenReturn(TestData.createTestDeviceDto());

        mockMvc.perform(put(DeviceEndpoint.BASE_PATH + "/{deviceId}", DEVICE_ID_TEST)
                .contentType(MediaType.APPLICATION_JSON)
                .content("""
                    {
                      "displayName": "Updated Test Device",
                      "type": "OTHER",
                      "status": "ONLINE",
                      "transport": "MANUAL",
                      "role": "Updated testing role",
                      "location": "Updated desk",
                      "description": "Updated test device.",
                      "enabled": true,
                      "capabilities": [],
                      "version": 0
                    }
                    """))
            .andExpect(status().isOk())
            .andExpect(jsonPath("$.deviceId").value(DEVICE_ID_TEST))
            .andExpect(jsonPath("$.displayName").value(DEVICE_NAME_TEST));
    }

    @Test
    void givenInvalidUpdateRequest_whenUpdate_thenReturnBadRequest() throws Exception {
        mockMvc.perform(put(DeviceEndpoint.BASE_PATH + "/{deviceId}", DEVICE_ID_TEST)
                .contentType(MediaType.APPLICATION_JSON)
                .content("""
                    {
                      "displayName": "",
                      "type": null,
                      "status": null,
                      "transport": null,
                      "role": "Updated testing role",
                      "location": "Updated desk",
                      "description": "Updated test device.",
                      "enabled": true,
                      "capabilities": [],
                      "version": null
                    }
                    """))
            .andExpect(status().isBadRequest());
    }

    @Test
    void givenDeviceId_whenDelete_thenReturnNoContent() throws Exception {
        mockMvc.perform(delete(DeviceEndpoint.BASE_PATH + "/{deviceId}", DEVICE_ID_TEST))
            .andExpect(status().isNoContent());

        verify(deviceService).delete(DEVICE_ID_TEST);
    }

    @Test
    void givenMalformedJson_whenCreate_thenReturnBadRequest() throws Exception {
        mockMvc.perform(post(DeviceEndpoint.BASE_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content("{ invalid json }"))
            .andExpect(status().isBadRequest());
    }

    @Test
    void givenWrongEnumValue_whenCreate_thenReturnBadRequest() throws Exception {
        mockMvc.perform(post(DeviceEndpoint.BASE_PATH)
                .contentType(MediaType.APPLICATION_JSON)
                .content("""
                    {
                      "deviceId": "invalid-enum-device",
                      "displayName": "Invalid Enum Device",
                      "type": "NOT_A_TYPE",
                      "transport": "MANUAL",
                      "role": "Testing",
                      "location": "Desk",
                      "description": "Invalid enum test.",
                      "capabilities": []
                    }
                    """))
            .andExpect(status().isBadRequest());
    }

    @Test
    void givenExistingState_whenFindLatestStateByDeviceId_thenReturnState() throws Exception {
        when(deviceTelemetryStateService.findLatestByDeviceId(DEVICE_ID_GREENHOUSE))
            .thenReturn(TestData.createGreenhouseTelemetryStateDto());

        mockMvc.perform(get(DeviceEndpoint.BASE_PATH + "/{deviceId}/state", DEVICE_ID_GREENHOUSE))
            .andExpect(status().isOk())
            .andExpect(jsonPath("$.deviceId").value(DEVICE_ID_GREENHOUSE))
            .andExpect(jsonPath("$.messageType").value("TELEMETRY"))
            .andExpect(jsonPath("$.schemaVersion").value(1))
            .andExpect(jsonPath("$.data.readings.temperatureC.value").value(24.6))
            .andExpect(jsonPath("$.data.readings.temperatureC.unit").value("C"));
    }
}
