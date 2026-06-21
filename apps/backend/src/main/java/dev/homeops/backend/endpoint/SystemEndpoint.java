package dev.homeops.backend.endpoint;

import dev.homeops.backend.endpoint.dto.SystemInfoDto;
import dev.homeops.backend.service.SystemService;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/v1/system")
public class SystemEndpoint {

    private final SystemService systemService;

    public SystemEndpoint(SystemService systemService) {
        this.systemService = systemService;
    }

    @GetMapping("/info")
    public SystemInfoDto getSystemInfo() {
        return systemService.getSystemInfo();
    }
}
