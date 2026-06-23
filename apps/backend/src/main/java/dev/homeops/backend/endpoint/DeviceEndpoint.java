package dev.homeops.backend.endpoint;

import dev.homeops.backend.dto.device.DeviceCreateDto;
import dev.homeops.backend.dto.device.DeviceDto;
import dev.homeops.backend.dto.device.DeviceUpdateDto;
import dev.homeops.backend.service.DeviceService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.Parameter;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import java.lang.invoke.MethodHandles;
import java.util.List;

@RestController
@RequestMapping(value = DeviceEndpoint.BASE_PATH, produces = MediaType.APPLICATION_JSON_VALUE)
@Tag(name = "Device", description = "Operations to manage devices.")
public class DeviceEndpoint {

    public static final String BASE_PATH = "/api/v1/device";
    private static final Logger LOG = LoggerFactory.getLogger(MethodHandles.lookup().lookupClass());

    private final DeviceService deviceService;

    public DeviceEndpoint(DeviceService deviceService) {
        this.deviceService = deviceService;
    }

    /**
     * Returns all registered devices.
     *
     * @return list of registered devices
     */
    @GetMapping
    @Operation(
        summary = "List registered devices",
        description = "Returns all devices known to HomeOps. This endpoint represents the device registry, not live telemetry history."
    )
    @ApiResponses({
        @ApiResponse(responseCode = "200", description = "Devices returned successfully")
    })
    public List<DeviceDto> findAll() {
        LOG.trace("GET {}", BASE_PATH);
        return deviceService.findAll();
    }

    /**
     * Returns one registered device by its stable device id.
     *
     * @param deviceId stable device id, for example greenhouse-esp32-01
     * @return matching device
     */
    @GetMapping("/{deviceId}")
    @Operation(
        summary = "Get device by id",
        description = "Returns one registered device by its stable external device id. The device id is also intended to be used in MQTT topics."
    )
    @ApiResponses({
        @ApiResponse(responseCode = "200", description = "Device returned successfully"),
        @ApiResponse(responseCode = "404", description = "Device was not found")
    })
    public DeviceDto findByDeviceId(
        @Parameter(description = "Stable external device id", example = "greenhouse-esp32-01")
        @PathVariable String deviceId
    ) {
        LOG.trace("GET {}/{}", BASE_PATH, deviceId);
        return deviceService.findByDeviceId(deviceId);
    }

    /**
     * Creates a new device registry entry.
     *
     * @param request device creation request
     * @return created device
     */
    @PostMapping(consumes = MediaType.APPLICATION_JSON_VALUE)
    @ResponseStatus(HttpStatus.CREATED)
    @Operation(
        summary = "Create device",
        description = "Creates a new device registry entry. This does not connect to the physical device yet; it only registers the device in HomeOps."
    )
    @ApiResponses({
        @ApiResponse(responseCode = "201", description = "Device created successfully"),
        @ApiResponse(responseCode = "400", description = "Device request is invalid"),
        @ApiResponse(responseCode = "409", description = "A device with this device id already exists")
    })
    public DeviceDto create(@Valid @RequestBody DeviceCreateDto request) {
        LOG.trace("POST {}", BASE_PATH);
        return deviceService.create(request);
    }

    /**
     * Updates an existing device registry entry.
     *
     * @param deviceId stable device id
     * @param request  device update request
     * @return updated device
     */
    @PutMapping(value = "/{deviceId}", consumes = MediaType.APPLICATION_JSON_VALUE)
    @Operation(
        summary = "Update device",
        description = "Updates an existing device registry entry. The request must contain the current version for optimistic locking."
    )
    @ApiResponses({
        @ApiResponse(responseCode = "200", description = "Device updated successfully"),
        @ApiResponse(responseCode = "400", description = "Device request is invalid"),
        @ApiResponse(responseCode = "404", description = "Device was not found"),
        @ApiResponse(responseCode = "409", description = "Device was modified by another request")
    })
    public DeviceDto update(
        @Parameter(description = "Stable external device id", example = "greenhouse-esp32-01")
        @PathVariable String deviceId,
        @Valid @RequestBody DeviceUpdateDto request
    ) {
        LOG.trace("PUT {}/{}", BASE_PATH, deviceId);
        return deviceService.update(deviceId, request);
    }

    /**
     * Deletes a device registry entry.
     *
     * @param deviceId stable device id
     */
    @DeleteMapping("/{deviceId}")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    @Operation(
        summary = "Delete device",
        description = "Deletes a device registry entry. This does not physically remove or shut down the device."
    )
    @ApiResponses({
        @ApiResponse(responseCode = "204", description = "Device deleted successfully"),
        @ApiResponse(responseCode = "404", description = "Device was not found")
    })
    public void delete(
        @Parameter(description = "Stable external device id", example = "greenhouse-esp32-01")
        @PathVariable String deviceId
    ) {
        LOG.trace("DELETE {}/{}", BASE_PATH, deviceId);
        deviceService.delete(deviceId);
    }
}
