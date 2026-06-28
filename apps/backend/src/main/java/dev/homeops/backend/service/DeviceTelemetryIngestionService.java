package dev.homeops.backend.service;

public interface DeviceTelemetryIngestionService {

    void ingest(String topic, String payload);
}
