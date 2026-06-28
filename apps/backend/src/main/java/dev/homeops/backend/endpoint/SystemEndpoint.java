package dev.homeops.backend.endpoint;

import dev.homeops.backend.dto.SystemInfoDto;
import dev.homeops.backend.service.SystemService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping(value = SystemEndpoint.BASE_PATH, produces = MediaType.APPLICATION_JSON_VALUE)
public class SystemEndpoint {

    private final SystemService systemService;

    public static final String BASE_PATH = "/api/v1/system";

    @Autowired
    public SystemEndpoint(SystemService systemService) {
        this.systemService = systemService;
    }

    @GetMapping("/info")
    public SystemInfoDto getSystemInfo() {
        return systemService.getSystemInfo();
    }
}
