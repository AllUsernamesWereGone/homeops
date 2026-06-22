package dev.homeops.backend.repository;

import dev.homeops.backend.entity.device.Device;
import org.springframework.data.jpa.repository.EntityGraph;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

public interface DeviceRepository extends JpaRepository<Device, UUID> {

    @EntityGraph(attributePaths = "capabilities")
    List<Device> findAllByOrderByDisplayNameAsc();

    @EntityGraph(attributePaths = "capabilities")
    Optional<Device> findByDeviceId(String deviceId);

    boolean existsByDeviceId(String deviceId);
}
