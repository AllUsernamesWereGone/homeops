package dev.homeops.backend.repository;

import dev.homeops.backend.entity.device.DeviceTelemetryState;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.UUID;

public interface DeviceTelemetryStateRepository extends JpaRepository<DeviceTelemetryState, UUID> {
}
