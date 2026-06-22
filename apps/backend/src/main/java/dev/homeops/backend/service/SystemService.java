package dev.homeops.backend.service;

import dev.homeops.backend.dto.SystemInfoDto;

public interface SystemService {

    /**
     * Returns basic runtime information about the HomeOps backend.
     *
     * @return current system information
     */
    SystemInfoDto getSystemInfo();
}
