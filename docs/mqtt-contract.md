# HomeOps MQTT Device Contract

Last updated: 2026-07-01
Project: HomeOps

## 1. Goal

HomeOps devices should communicate through MQTT using a stable, versioned message contract.

The main idea is:

```text
Angular frontend
  -> Spring Boot backend
  -> MQTT broker
  -> ESP32 / Pico / sensors / actuators
```

The backend is responsible for:
```text
- device registry
- validation
- command creation
- command history
- telemetry storage
- latest state storage
- MQTT publish/subscribe
- REST API for Angular
```
The broker is responsible only for routing MQTT messages.

The device is responsible for:
```text
- connecting to Wi-Fi
- connecting to MQTT
- publishing telemetry
- publishing current state
- subscribing to commands
- executing commands
- publishing command results
- publishing errors when something fails
```
---


### 2.1 Quality of Service

MQTT QoS levels:

```text
QoS 0 = send once, no confirmation
QoS 1 = at least once, can duplicate
QoS 2 = exactly once, more overhead
```


### 2.2 Backend Subscriptions

The Spring Boot backend should subscribe to:

```text
homeops/devices/+/hello
homeops/devices/+/status
homeops/devices/+/telemetry
homeops/devices/+/state
homeops/devices/+/command-result
homeops/devices/+/error
```

### 2.3 Device Subscriptions

Each device subscribes only to its own command topic:

```text
homeops/devices/{deviceId}/command
```

### 2.4 Backend Publications

The backend publishes to:

```text
homeops/devices/{deviceId}/command
```
---

### 2.5 Device Publications

The device publishes to:

```text
homeops/devices/{deviceId}/hello
homeops/devices/{deviceId}/status
homeops/devices/{deviceId}/telemetry
homeops/devices/{deviceId}/state
homeops/devices/{deviceId}/command-result
homeops/devices/{deviceId}/error
```


## 3. Message Envelope Agreement

All MQTT messages should use the same outer envelope.


### 3.1 Schema Version 1
```json
{
  "schemaVersion": 1,
  "deviceId": "<DEVICE_ID>",
  "data": {}
}
```

### 3.2 Schema Version 2
```json
{
  "schemaVersion": 2,
  "deviceId": "<DEVICE_ID>",
  "TBD": null,
  "TBD": null,
  "TBD": null,
  "data": {}
}
```

