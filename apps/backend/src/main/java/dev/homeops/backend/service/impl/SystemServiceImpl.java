package dev.homeops.backend.service.impl;

import dev.homeops.backend.dto.SystemInfoDto;
import dev.homeops.backend.service.SystemService;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.env.Environment;
import org.springframework.stereotype.Service;

import java.util.Arrays;

@Service
public class SystemServiceImpl implements SystemService {

    private final Environment environment;

    @Value("${spring.application.name:homeops-backend}")
    private String applicationName;

    @Value("${app.version:0.0.1-SNAPSHOT}")
    private String version;

    public SystemServiceImpl(Environment environment) {
        this.environment = environment;
    }

    @Override
    public SystemInfoDto getSystemInfo() {
        String profiles = getActiveProfiles();

        return new SystemInfoDto(
            applicationName,
            version,
            profiles,
            "UP"
        );
    }

    private String getActiveProfiles() {
        String[] activeProfiles = environment.getActiveProfiles();

        if (activeProfiles.length == 0) {
            return "default";
        }

        return String.join(",", Arrays.asList(activeProfiles));
    }
}
