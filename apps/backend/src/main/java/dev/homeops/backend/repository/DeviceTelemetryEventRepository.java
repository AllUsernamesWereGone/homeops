package dev.homeops.backend.repository;

import dev.homeops.backend.entity.device.DeviceTelemetryEvent;
import org.springframework.data.jpa.repository.JpaRepository;

public interface DeviceTelemetryEventRepository extends JpaRepository<DeviceTelemetryEvent, Long> {
}
