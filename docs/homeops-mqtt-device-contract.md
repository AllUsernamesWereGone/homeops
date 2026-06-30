# HomeOps MQTT Device Contract

Last updated: 2026-07-01
Project: HomeOps
Purpose: Agreement document for ESP32/Pico firmware, MQTT broker setup, Spring Boot backend integration, and future
frontend behavior.

---

## 1. Goal

HomeOps devices should communicate through MQTT using a stable, versioned message contract.

The main idea is:

```text
Angular frontend
  -> Spring Boot backend
  -> MQTT broker
  -> ESP32 / Pico / sensors / actuators
```

The frontend does not talk directly to MQTT devices.

The backend is responsible for:

- device registry
- validation
- command creation
- command history
- telemetry storage
- latest state storage
- MQTT publish/subscribe
- REST API for Angular

The broker is responsible only for routing MQTT messages.

The device is responsible for:

- connecting to Wi-Fi
- connecting to MQTT
- publishing telemetry
- publishing current state
- subscribing to commands
- executing commands
- publishing command results
- publishing errors when something fails

---

## 2. Things We Need To Agree On

This is the complete agreement checklist between backend, firmware, and infrastructure.

---

## 3. Broker / Network Agreement

### 3.1 Broker Host

Agree on where the MQTT broker runs.

Examples:

```text
mini PC
```

Decision needed:

```text
MQTT broker host: __________________________
MQTT broker port: __________________________
```

Default recommendation:

```text
Host: homeops-pi or broker container name in Docker Compose
Port: 1883 for plain MQTT during local development
Port: 8883 for TLS later
```

---

### 3.2 Broker Software

Recommended broker:

```text
Eclipse Mosquitto
```

Decision needed:

```text
Broker software: Mosquitto
Broker config location: infra/mqtt/
```

---

### 3.3 Authentication

Decide whether devices need username/password from the beginning.

Options:

```text
Option A: no auth during local development only
Option B: username/password from the beginning
Option C: TLS certificates later
```

Recommendation:

```text
Use username/password early, even if simple.
Use TLS later when the system is stable.
```

Decision needed:

```text
MQTT username: __________________________
MQTT password source: .env / secret / firmware config
```

Do not hardcode real production passwords in Git.

---

### 3.4 MQTT Protocol Version

Decision needed:

```text
MQTT version: 3.1.1 or 5.0
```

Recommendation:

```text
Use MQTT 3.1.1 first because most ESP32 libraries support it well.
```

---

### 3.5 Quality of Service

MQTT QoS levels:

```text
QoS 0 = send once, no confirmation
QoS 1 = at least once, can duplicate
QoS 2 = exactly once, more overhead
```

Recommended first version:

```text
Telemetry: QoS 0
State: QoS 1
Commands: QoS 1
Command results: QoS 1
Errors: QoS 1
Hello/boot messages: QoS 1
```

Reason:

- Telemetry can tolerate occasional packet loss.
- Commands and command results should not silently disappear.
- QoS 1 means the backend and device must handle duplicate messages safely.

Decision needed:

```text
Telemetry QoS: ______
State QoS: ______
Command QoS: ______
Command result QoS: ______
```

---

### 3.6 Retained Messages

MQTT retained messages stay on the broker and are sent to new subscribers.

Recommended first version:

```text
telemetry: retained = false
state: retained = true or false, decide carefully
status / availability: retained = true
command: retained = false
command-result: retained = false
hello: retained = false
error: retained = false
```

Strong recommendation:

```text
Do not retain command messages.
```

Reason:

A retained command can be dangerous. A device could reconnect and accidentally execute an old command.

Decision needed:

```text
Should state be retained? yes/no
Should status be retained? yes/no
Commands retained? no
```

---

### 3.7 Last Will and Testament

Each device should register an MQTT Last Will message so the broker can mark it offline if it disconnects unexpectedly.

Recommended LWT topic:

```text
homeops/devices/{deviceId}/status
```

Recommended offline payload:

```json
{
  "schemaVersion": 1,
  "messageId": "generated-by-device-before-connect",
  "messageType": "STATUS",
  "deviceId": "greenhouse-esp32-01",
  "source": "BROKER_LWT",
  "sentAt": null,
  "data": {
    "status": "OFFLINE",
    "reason": "CONNECTION_LOST"
  }
}
```

When the device connects normally, it publishes:

```json
{
  "schemaVersion": 1,
  "messageId": "01STATUSONLINE",
  "messageType": "STATUS",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:00:00Z",
  "data": {
    "status": "ONLINE",
    "reason": "BOOT_OR_RECONNECT"
  }
}
```

Decision needed:

```text
LWT enabled: yes
LWT topic: homeops/devices/{deviceId}/status
LWT retained: yes
```

---

## 4. Device Identity Agreement

### 4.1 Device ID Format

Device IDs must be stable and human-readable.

Recommended format:

```text
lowercase-kebab-case
```

Examples:

```text
greenhouse-esp32-01
rack-fan-controller-01
pico2w-miner
antminer-s7
homeops-pi
```

Rules:

```text
Allowed characters: a-z, 0-9, -
No spaces
No uppercase letters
No special characters except dash
Must not change after first registration
Must be unique in HomeOps
```

Decision needed:

```text
Device ID assigned by: backend / firmware / manual config
```

Recommendation:

```text
Manual config first.
Later backend-assisted provisioning.
```

---

### 4.2 Device Type

Each device should have a type.

Examples:

```text
SERVER,
MICROCONTROLLER,
SENSOR,
ACTUATOR,
CAMERA,
MINER,
LAPTOP,
ROUTER,
OTHER
ANTMINER_MONITOR
RASPBERRY_PI
GENERIC_DEVICE
```

Decision needed:

```text
Supported device types for v1:
- ______________________
- ______________________
- ______________________
```

---

### 4.3 Device Capabilities

Capabilities describe what the device can do or measure.

Examples:

```text
TEMPERATURE_SENSOR,
HUMIDITY_SENSOR,
LIGHT_SENSOR,
FAN_SWITCH,
LIGHT_SWITCH,
CAMERA_SNAPSHOT,
HASHRATE_MONITORING,
TEMPERATURE_MONITORING,
CPU_METRICS,
MEMORY_METRICS,
DISK_METRICS,
DOCKER_HOST,
MQTT_BROKER,
CI_RUNNER,
OTHER
```

Decision needed:

```text
First device capabilities:
- ______________________
- ______________________
- ______________________
```

---

## 5. Topic Agreement

Use one stable topic prefix.

Recommended prefix:

```text
homeops/devices
```

Topic structure:

```text
homeops/devices/{deviceId}/hello
homeops/devices/{deviceId}/status
homeops/devices/{deviceId}/telemetry
homeops/devices/{deviceId}/state
homeops/devices/{deviceId}/command
homeops/devices/{deviceId}/command-result
homeops/devices/{deviceId}/error
homeops/devices/{deviceId}/config
```

---

### 5.1 Backend Subscriptions

The Spring Boot backend should subscribe to:

```text
homeops/devices/+/hello
homeops/devices/+/status
homeops/devices/+/telemetry
homeops/devices/+/state
homeops/devices/+/command-result
homeops/devices/+/error
```

Optional later:

```text
homeops/devices/+/config
```

---

### 5.2 Device Subscriptions

Each device subscribes only to its own command topic:

```text
homeops/devices/{deviceId}/command
```

Optional later:

```text
homeops/devices/{deviceId}/config
```

Do not make every device subscribe to all commands.

---

### 5.3 Backend Publications

The backend publishes to:

```text
homeops/devices/{deviceId}/command
```

Optional later:

```text
homeops/devices/{deviceId}/config
```

---

### 5.4 Device Publications

The device publishes to:

```text
homeops/devices/{deviceId}/hello
homeops/devices/{deviceId}/status
homeops/devices/{deviceId}/telemetry
homeops/devices/{deviceId}/state
homeops/devices/{deviceId}/command-result
homeops/devices/{deviceId}/error
```

---

## 6. Message Envelope Agreement

All MQTT messages should use the same outer envelope.

```json
{
  "schemaVersion": 1,
  "messageId": "01HYZQ6M7N8P9Q0R1S2T3U4V5W",
  "correlationId": null,
  "messageType": "TELEMETRY",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:30:00Z",
  "data": {}
}
```

---

### 6.1 Required Envelope Fields

```text
schemaVersion
messageId
messageType
deviceId
source
sentAt
data
```

---

### 6.2 Optional Envelope Fields

```text
correlationId
```

Use `correlationId` when replying to another message, especially command results.

---

### 6.3 Field Meanings

| Field           | Meaning                                                                           |
|-----------------|-----------------------------------------------------------------------------------|
| `schemaVersion` | Version of the message contract. Start with `1`.                                  |
| `messageId`     | Unique ID for this message. Used for deduplication and tracing.                   |
| `correlationId` | ID of the original message this message answers. Used mostly for command results. |
| `messageType`   | Type of message, for example `TELEMETRY` or `COMMAND`.                            |
| `deviceId`      | Stable device ID. Must match the topic device ID.                                 |
| `source`        | Who created the message.                                                          |
| `sentAt`        | Timestamp from sender, ISO-8601 UTC if available.                                 |
| `data`          | Dynamic payload. Shape depends on `messageType`.                                  |

---

### 6.4 Message ID Format

Options:

```text
UUID
ULID
simple generated string on microcontroller
```

Recommendation:

```text
Backend: UUID or ULID
ESP32: UUID-like random string, counter-based ID, or millis-based ID for first version
```

Example ESP32-friendly format:

```text
greenhouse-esp32-01-12345678
```

Decision needed:

```text
Message ID strategy: __________________________
```

---

### 6.5 Timestamp Format

Recommended:

```text
ISO-8601 UTC
```

Example:

```text
2026-07-01T18:30:00Z
```

If the device has no valid time yet:

```json
"sentAt": null
```

Backend must then use `receivedAt` from server time.

Decision needed:

```text
Device has NTP time? yes/no
If no, backend receivedAt is authoritative.
```

---

### 6.6 Source Values

Allowed `source` values:

```text
DEVICE
BACKEND
BROKER_LWT
SYSTEM
```

---

### 6.7 Message Type Values

Allowed `messageType` values for v1:

```text
HELLO
STATUS
TELEMETRY
STATE
COMMAND
COMMAND_RESULT
ERROR
CONFIG
```

Optional later:

```text
DISCOVERY
CONFIG_REQUEST
CONFIG_RESULT
OTA_REQUEST
OTA_RESULT
```

---

## 7. Payload Agreement By Message Type

The `data` object changes depending on `messageType`.

---

## 8. HELLO Message

Sent by device after boot or reconnect.

Topic:

```text
homeops/devices/{deviceId}/hello
```

Purpose:

- announce device is alive
- send firmware version
- send hardware type
- send capabilities
- send available outputs/targets

Example:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000001",
  "correlationId": null,
  "messageType": "HELLO",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:00:00Z",
  "data": {
    "firmwareVersion": "0.1.0",
    "hardware": "esp32-devkit-v1",
    "deviceType": "GREENHOUSE_CONTROLLER",
    "capabilities": [
      "TEMPERATURE",
      "HUMIDITY",
      "LIGHT",
      "SOIL_MOISTURE",
      "FAN_CONTROL",
      "LIGHT_CONTROL",
      "PUMP_CONTROL"
    ],
    "outputs": [
      "fan1",
      "light1",
      "pump1"
    ]
  }
}
```

Backend behavior:

```text
- validate deviceId
- update lastSeenAt
- optionally auto-register unknown device as PENDING/UNKNOWN
- update firmwareVersion if stored
- update capabilities if agreed
```

Agreement needed:

```text
Should unknown devices auto-register? yes/no
Should capabilities from HELLO overwrite backend config? yes/no
```

Recommendation:

```text
Unknown devices can be stored as DISCOVERED but not automatically trusted.
Backend config should be authoritative for production.
```

---

## 9. STATUS Message

Sent by device when online/offline state changes.

Topic:

```text
homeops/devices/{deviceId}/status
```

Example online message:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000002",
  "correlationId": null,
  "messageType": "STATUS",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:00:01Z",
  "data": {
    "status": "ONLINE",
    "reason": "BOOT_OR_RECONNECT"
  }
}
```

Example offline LWT message:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-lwt",
  "correlationId": null,
  "messageType": "STATUS",
  "deviceId": "greenhouse-esp32-01",
  "source": "BROKER_LWT",
  "sentAt": null,
  "data": {
    "status": "OFFLINE",
    "reason": "CONNECTION_LOST"
  }
}
```

Allowed statuses:

```text
ONLINE
OFFLINE
STARTING
ERROR
MAINTENANCE
```

Backend behavior:

```text
- update device.status
- update device.lastSeenAt for ONLINE/STARTING/ERROR
- do not update lastSeenAt for broker offline messages unless desired
```

---

## 10. TELEMETRY Message

Sent by device for sensor readings.

Topic:

```text
homeops/devices/{deviceId}/telemetry
```

Purpose:

- temperature
- humidity
- light
- soil moisture
- Wi-Fi RSSI
- hashrate
- RPM
- voltage
- current
- other measured values

Recommended dynamic structure:

```json
{
  "readings": {
    "temperatureC": {
      "value": 24.6,
      "unit": "C"
    },
    "humidityPercent": {
      "value": 63.2,
      "unit": "%"
    }
  }
}
```

Full example:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000003",
  "correlationId": null,
  "messageType": "TELEMETRY",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:30:00Z",
  "data": {
    "readings": {
      "temperatureC": {
        "value": 24.6,
        "unit": "C"
      },
      "humidityPercent": {
        "value": 63.2,
        "unit": "%"
      },
      "lightLux": {
        "value": 418,
        "unit": "lux"
      },
      "soilMoisturePercent": {
        "value": 37,
        "unit": "%"
      },
      "wifiRssi": {
        "value": -61,
        "unit": "dBm"
      },
      "uptimeSeconds": {
        "value": 8421,
        "unit": "s"
      }
    }
  }
}
```

Backend behavior:

```text
- parse envelope
- validate messageType = TELEMETRY
- validate topic deviceId matches payload deviceId
- store raw payload as telemetry history
- update latest state key-values from readings
- update device.lastSeenAt
```

---

## 11. STATE Message

Sent by device for current actuator/output state.

Topic:

```text
homeops/devices/{deviceId}/state
```

Purpose:

- report actual fan state
- report actual light state
- report pump state
- report mode
- report relay state

Example:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000004",
  "correlationId": null,
  "messageType": "STATE",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:30:05Z",
  "data": {
    "outputs": {
      "fan1": {
        "state": "OFF"
      },
      "light1": {
        "state": "ON"
      },
      "pump1": {
        "state": "OFF"
      }
    },
    "mode": "MANUAL",
    "wifi": {
      "rssi": -61
    },
    "uptimeSeconds": 8421
  }
}
```

Backend behavior:

```text
- update latest state
- update output states
- update device.lastSeenAt
- optionally store raw state event
```

---

## 12. COMMAND Message

Sent by backend to device.

Topic:

```text
homeops/devices/{deviceId}/command
```

Purpose:

- turn fan on/off
- turn light on/off
- turn pump on/off
- request state
- restart device
- change mode
- update config later

Example:

```json
{
  "schemaVersion": 1,
  "messageId": "backend-01HYZQAABBCCDDEEFF00112233",
  "correlationId": null,
  "messageType": "COMMAND",
  "deviceId": "greenhouse-esp32-01",
  "source": "BACKEND",
  "sentAt": "2026-07-01T18:31:00Z",
  "data": {
    "command": "SET_OUTPUT",
    "target": "fan1",
    "value": "ON"
  }
}
```

Allowed v1 commands:

```text
SET_OUTPUT
REQUEST_STATE
SET_MODE
RESTART
```

Optional later:

```text
SET_CONFIG
CALIBRATE_SENSOR
START_OTA
CANCEL_COMMAND
```

---

### 12.1 SET_OUTPUT Command

Use for fans, lights, pumps, relays.

```json
{
  "command": "SET_OUTPUT",
  "target": "fan1",
  "value": "ON"
}
```

Allowed values:

```text
ON
OFF
TOGGLE - optional, less recommended
```

Recommendation:

```text
Prefer explicit ON/OFF over TOGGLE.
```

Reason:

`TOGGLE` is risky if backend and device state are out of sync.

---

### 12.2 REQUEST_STATE Command

Backend asks device to publish current state.

```json
{
  "command": "REQUEST_STATE"
}
```

Device response:

```text
Device publishes STATE message.
```

Use cases:

```text
- backend startup
- user manually refreshes device
- device reconnects
- suspicious stale state
```

---

### 12.3 SET_MODE Command

Use for manual/automatic device mode.

```json
{
  "command": "SET_MODE",
  "value": "AUTO"
}
```

Allowed values:

```text
MANUAL
AUTO
MAINTENANCE
```

---

### 12.4 RESTART Command

Use carefully.

```json
{
  "command": "RESTART"
}
```

Recommendation:

```text
Do not implement dangerous commands before basic flow works.
```

---

## 13. COMMAND_RESULT Message

Sent by device after receiving/executing a command.

Topic:

```text
homeops/devices/{deviceId}/command-result
```

Purpose:

- confirm command received
- confirm command applied
- report failure
- link result to original command using correlationId

Example success:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000005",
  "correlationId": "backend-01HYZQAABBCCDDEEFF00112233",
  "messageType": "COMMAND_RESULT",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:31:01Z",
  "data": {
    "command": "SET_OUTPUT",
    "target": "fan1",
    "requestedValue": "ON",
    "status": "APPLIED",
    "actualValue": "ON"
  }
}
```

Example failure:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000006",
  "correlationId": "backend-01HYZQAABBCCDDEEFF00112233",
  "messageType": "COMMAND_RESULT",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:31:01Z",
  "data": {
    "command": "SET_OUTPUT",
    "target": "fan1",
    "requestedValue": "ON",
    "status": "FAILED",
    "actualValue": "OFF",
    "errorCode": "OUTPUT_NOT_AVAILABLE",
    "errorMessage": "fan1 is not configured on this device"
  }
}
```

Allowed command result statuses:

```text
RECEIVED
APPLIED
REJECTED
FAILED
TIMEOUT
```

Recommendation:

```text
For v1, device can directly return APPLIED / REJECTED / FAILED.
RECEIVED can be added later if needed.
```

Backend behavior:

```text
- find command by correlationId
- update command status
- store result payload
- update latest state if actualValue is included
- mark command as failed if status is FAILED or REJECTED
```

---

## 14. ERROR Message

Sent by device when it detects a problem.

Topic:

```text
homeops/devices/{deviceId}/error
```

Example:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000007",
  "correlationId": null,
  "messageType": "ERROR",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:35:00Z",
  "data": {
    "code": "SENSOR_READ_FAILED",
    "message": "Could not read BME280 sensor",
    "severity": "WARN"
  }
}
```

Allowed severity values:

```text
INFO
WARN
ERROR
CRITICAL
```

Backend behavior:

```text
- store error event
- update latest device error state
- optionally set device.status = ERROR for ERROR/CRITICAL
```

---

## 15. CONFIG Message

This can be added after basic telemetry and commands work.

Topic:

```text
homeops/devices/{deviceId}/config
```

Purpose:

- telemetry interval
- thresholds
- automatic mode settings
- output defaults

Example backend-to-device config:

```json
{
  "schemaVersion": 1,
  "messageId": "backend-config-000001",
  "correlationId": null,
  "messageType": "CONFIG",
  "deviceId": "greenhouse-esp32-01",
  "source": "BACKEND",
  "sentAt": "2026-07-01T19:00:00Z",
  "data": {
    "telemetryIntervalSeconds": 30,
    "stateIntervalSeconds": 60,
    "autoMode": {
      "enabled": false,
      "fanOnAboveTemperatureC": 30.0,
      "fanOffBelowTemperatureC": 27.0
    }
  }
}
```

Recommendation:

```text
Do not start with remote config.
Add it after basic telemetry, state, commands, and command results work.
```

---

## 16. Dynamic Data Agreement

The system should be dynamic, but not chaotic.

Use this structure for sensor values:

```json
{
  "readings": {
    "keyName": {
      "value": 123,
      "unit": "unit"
    }
  }
}
```

Example:

```json
{
  "readings": {
    "temperatureC": {
      "value": 24.6,
      "unit": "C"
    },
    "humidityPercent": {
      "value": 63.2,
      "unit": "%"
    }
  }
}
```

---

### 16.1 Reading Key Naming Rules

Use stable key names.

Recommended style:

```text
camelCase
```

Examples:

```text
temperatureC
humidityPercent
lightLux
soilMoisturePercent
wifiRssi
uptimeSeconds
hashrate
fanRpm
powerWatts
voltageV
currentA
```

Rules:

```text
No spaces
No random renaming
Use units in the key only when helpful
Prefer explicit names over short names
```

Bad examples:

```text
temp
h
moist
sensor1
value
```

Good examples:

```text
temperatureC
humidityPercent
soilMoisturePercent
```

---

### 16.2 Unit Agreement

Common units:

| Measurement   | Key                   | Unit                                   |
|---------------|-----------------------|----------------------------------------|
| Temperature   | `temperatureC`        | `C`                                    |
| Humidity      | `humidityPercent`     | `%`                                    |
| Light         | `lightLux`            | `lux`                                  |
| Soil moisture | `soilMoisturePercent` | `%`                                    |
| Wi-Fi RSSI    | `wifiRssi`            | `dBm`                                  |
| Uptime        | `uptimeSeconds`       | `s`                                    |
| Fan speed     | `fanRpm`              | `rpm`                                  |
| Power         | `powerWatts`          | `W`                                    |
| Voltage       | `voltageV`            | `V`                                    |
| Current       | `currentA`            | `A`                                    |
| Hashrate      | `hashrate`            | `H/s`, `kH/s`, `GH/s` depending device |

Decision needed:

```text
Use Celsius only? yes/no
Use percent as 0-100? yes/no
Use lux for light? yes/no
```

Recommendation:

```text
Use Celsius.
Use percent as 0-100.
Use lux if using a real light sensor like BH1750.
```

---

### 16.3 Value Types

Allowed value types:

```text
number
string
boolean
```

Avoid nested complex objects inside individual readings unless needed.

Good:

```json
"temperatureC": {"value": 24.6, "unit": "C"}
```

Avoid:

```json
"temperature": {"current": {"raw": {"x": 24.6}}}
```

---

## 17. Output / Actuator Agreement

Outputs are things the device can control.

Examples:

```text
fan1
fan2
light1
pump1
relay1
```

Rules:

```text
Use lowercase names
Use stable names
Do not rename output names after frontend/backend uses them
Prefer specific names over generic relay numbers when possible
```

Example state:

```json
{
  "outputs": {
    "fan1": {
      "state": "ON"
    },
    "light1": {
      "state": "OFF"
    }
  }
}
```

Allowed output states:

```text
ON
OFF
UNKNOWN
ERROR
```

For PWM/dimmer later:

```json
{
  "outputs": {
    "fan1": {
      "state": "ON",
      "speedPercent": 60
    }
  }
}
```

Do not add PWM until simple ON/OFF works.

---

## 18. Command Safety Agreement

Commands must be safe and explicit.

Rules:

```text
Backend validates before publishing.
Device validates before executing.
Device must ignore commands for the wrong deviceId.
Device must reject unknown commands.
Device must reject unknown targets.
Device must publish COMMAND_RESULT after command handling.
Commands must not be retained.
Prefer ON/OFF over TOGGLE.
```

Dangerous commands should be disabled at first:

```text
mains-voltage switching
heater control
pump automation without safeguards
OTA update
factory reset
```

---

## 19. Duplicate Message Agreement

Because QoS 1 can deliver duplicates, both backend and device should tolerate duplicate `messageId` values.

Backend behavior:

```text
If messageId already processed, ignore or mark duplicate.
Do not store duplicate telemetry events unless intentionally allowed.
Do not execute duplicate backend command creation.
```

Device behavior:

```text
If command messageId was already processed, do not execute it again.
Publish the same or equivalent command result if possible.
```

Decision needed:

```text
Store processed message IDs for how long? __________________
```

Recommendation:

```text
Backend stores message IDs permanently with telemetry/command history.
ESP32 stores only last few command IDs in memory if possible.
```

---

## 20. Timeout Agreement

The backend should not wait forever for command results.

Recommended values:

```text
Command result timeout: 5-15 seconds for normal commands
Device offline timeout: 2-3 missed heartbeats or telemetry intervals
```

Example:

```text
Backend sends command at 18:31:00.
No COMMAND_RESULT by 18:31:15.
Backend marks command as TIMEOUT.
```

Decision needed:

```text
Command timeout seconds: ______
Offline timeout seconds: ______
```

---

## 21. Telemetry Cadence Agreement

Decide how often devices publish telemetry.

Recommended first version:

```text
Greenhouse telemetry: every 30 seconds
State message: every 60 seconds or after output change
Status: on connect/disconnect via LWT
Command result: immediately after command
```

Decision needed:

```text
Telemetry interval: ______ seconds
State interval: ______ seconds
```

Avoid sending telemetry too often before the backend and database are ready.

---

## 22. Boot / Reconnect Behavior

On boot or reconnect, device should publish in this order:

```text
1. STATUS ONLINE
2. HELLO
3. STATE
4. TELEMETRY
```

Reason:

The backend learns that the device is online, then learns what it is, then gets current state and readings.

---

## 23. Backend Source of Truth Agreement

Use this rule:

```text
Backend database = intended state and latest known state
Device = actual physical state
```

Example:

```text
Backend sends: fan1 ON
Device fails: relay unavailable
Backend stores command FAILED
Backend latest actual fan1 state remains OFF or UNKNOWN
```

The backend must not blindly assume that a command succeeded.

A command is only successful after a matching `COMMAND_RESULT` confirms it.

---

## 24. JSON Validation Agreement

Backend should validate:

```text
schemaVersion is supported
messageId exists
messageType is known
deviceId exists
topic deviceId matches payload deviceId
source is allowed
sentAt is valid or null
data exists
required data fields exist for messageType
```

Device should validate:

```text
messageType = COMMAND for command topic
payload deviceId matches its own deviceId
command is known
target is known if required
value is valid if required
```

Invalid messages should not crash backend or firmware.

---

## 25. Error Codes Agreement

Recommended first error codes:

```text
UNKNOWN_COMMAND
UNKNOWN_TARGET
INVALID_VALUE
OUTPUT_NOT_AVAILABLE
SENSOR_READ_FAILED
DEVICE_BUSY
CONFIG_INVALID
INTERNAL_ERROR
```

Use these in `ERROR` messages or failed `COMMAND_RESULT` messages.

---

## 26. Security Agreement

Minimum security rules:

```text
MQTT broker is not exposed publicly.
Use Tailscale/private network for remote access.
Use broker username/password.
Do not commit secrets to Git.
Do not allow arbitrary command topics from untrusted clients.
Backend should validate every command before publishing.
```

Later:

```text
TLS for MQTT
separate MQTT users per device
ACLs per topic
read-only backend subscriptions where possible
```

Recommended Mosquitto ACL idea later:

```text
Device greenhouse-esp32-01 can publish:
homeops/devices/greenhouse-esp32-01/+

Device greenhouse-esp32-01 can subscribe:
homeops/devices/greenhouse-esp32-01/command

Backend can subscribe:
homeops/devices/+/+

Backend can publish:
homeops/devices/+/command
```

---

## 27. What The Broker Messages Transport

MQTT messages transport JSON documents.

They transport:

```text
who sent the message
which device it is about
what kind of message it is
when it was sent
unique message identifier
optional correlation to another message
dynamic data payload
```

The broker does not interpret the JSON.

The broker only routes based on the topic.

Example topic:

```text
homeops/devices/greenhouse-esp32-01/telemetry
```

Example transport payload:

```json
{
  "schemaVersion": 1,
  "messageId": "greenhouse-esp32-01-000003",
  "correlationId": null,
  "messageType": "TELEMETRY",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:30:00Z",
  "data": {
    "readings": {
      "temperatureC": {
        "value": 24.6,
        "unit": "C"
      }
    }
  }
}
```

---

## 28. Backend Changes Needed

This section describes what should be added or changed in the Spring Boot backend.

---

### 28.1 Add MQTT Dependency

Add an MQTT client dependency.

Recommended options:

```text
Eclipse Paho MQTT client
Spring Integration MQTT
HiveMQ MQTT client
```

Recommendation for your current stage:

```text
Use Spring Integration MQTT if you want Spring-style integration.
Use Eclipse Paho directly if you want simpler explicit control.
```

Either is fine. The important part is keeping your own service layer clean.

---

### 28.2 Add MQTT Configuration Properties

Add config values in `application.yml` or profile-specific config.

Needed properties:

```yaml
homeops:
  mqtt:
    enabled: true
    broker-url: tcp://localhost:1883
    client-id: homeops-backend
    username: ${HOMEOPS_MQTT_USERNAME:}
    password: ${HOMEOPS_MQTT_PASSWORD:}
    topic-prefix: homeops/devices
    command-qos: 1
    telemetry-qos: 0
```

Environment variables in Docker Compose:

```text
HOMEOPS_MQTT_USERNAME
HOMEOPS_MQTT_PASSWORD
HOMEOPS_MQTT_BROKER_URL
```

---

### 28.3 Add DTO / Model For Envelope

Create a backend representation of the envelope.

Conceptual structure:

```java
public class MqttEnvelope {
  private int schemaVersion;
  private String messageId;
  private String correlationId;
  private String messageType;
  private String deviceId;
  private String source;
  private Instant sentAt;
  private JsonNode data;
}
```

Use `JsonNode` for `data` because it is dynamic.

---

### 28.4 Add Message Type Enum

Create enum:

```text
DONE
```

---

### 28.5 Add Source Enum

Create enum:

```text
DEVICE
BACKEND
BROKER_LWT
SYSTEM
```

---

### 28.6 Add Command Status Enum

Create enum:

```text
PENDING
PUBLISHED
RECEIVED
APPLIED
REJECTED
FAILED
TIMEOUT
```

For v1, this is enough:

```text
PENDING
PUBLISHED
APPLIED
REJECTED
FAILED
TIMEOUT
```

---

### 28.7 Add MQTT Listener Service

Add a service responsible for incoming MQTT messages.

Responsibilities:

```text
- receive topic + payload
- parse JSON
- validate envelope
- check topic deviceId matches payload deviceId
- route by messageType
- call telemetry/state/command services
- never crash on bad messages
```

Suggested class names:

```text
MqttMessageListener
MqttInboundService
MqttEnvelopeParser
MqttMessageRouter
```

---

### 28.8 Add MQTT Publisher Service

Add a service responsible for outgoing MQTT messages.

Responsibilities:

```text
- build command envelope
- serialize JSON
- publish to homeops/devices/{deviceId}/command
- set QoS
- ensure retained=false for commands
```

Suggested class name:

```text
MqttCommandPublisher
```

---

### 28.9 Add Telemetry Event Storage

Add entity/table for raw telemetry/history events.

Recommended fields:

```text
id
messageId
deviceId
messageType
source
sentAt
receivedAt
topic
payloadJson
schemaVersion
```

Purpose:

```text
- debugging
- history
- replay later
- charts later
```

---

### 28.10 Add Latest Device State Storage

Add entity/table for latest key-value state.

Recommended fields:

```text
id
deviceId
stateKey
stateValue
unit
valueType
source
updatedAt
```

Example rows:

```text
greenhouse-esp32-01 | temperatureC | 24.6 | C | NUMBER
greenhouse-esp32-01 | fan1.state   | ON   |   | STRING
greenhouse-esp32-01 | wifiRssi     | -61  | dBm | NUMBER
```

This avoids hardcoding one column per sensor.

---

### 28.11 Add Device Command Storage

Add entity/table for command history.

Recommended fields:

```text
id
deviceId
commandMessageId
correlationId
command
target
requestedValue
status
createdAt
publishedAt
completedAt
resultPayloadJson
errorCode
errorMessage
```

Purpose:

```text
- audit trail
- frontend command history
- timeout handling
- debugging failed device actions
```

---

### 28.12 Add Device Error Storage Optional

Optional but useful later.

Recommended fields:

```text
id
deviceId
messageId
code
message
severity
payloadJson
receivedAt
```

For first version, errors can also be stored in generic telemetry event storage.

---

### 28.13 Update Device Entity

Your existing device entity likely needs only small additions later.

Useful fields:

```text
lastSeenAt
firmwareVersion
hardware
lastStatus
lastErrorAt
lastErrorCode
```

Do not overload the main Device entity with every telemetry value.

Keep telemetry and state in separate tables.

---

### 28.14 Add Backend Command API

Add endpoint:

```text
POST /api/devices/{deviceId}/commands
```

Request body:

```json
{
  "command": "SET_OUTPUT",
  "target": "fan1",
  "value": "ON"
}
```

Backend behavior:

```text
- validate device exists
- validate device is enabled
- validate command is allowed
- validate target is allowed if known
- create command row with PENDING
- publish MQTT command
- update command row to PUBLISHED
- return command DTO to frontend
```

---

### 28.15 Add Latest State API

Add endpoint:

```text
GET /api/devices/{deviceId}/state
```

Response example:

```json
{
  "deviceId": "greenhouse-esp32-01",
  "updatedAt": "2026-07-01T18:31:01Z",
  "values": {
    "temperatureC": {
      "value": 24.6,
      "unit": "C"
    },
    "fan1.state": {
      "value": "ON",
      "unit": null
    }
  }
}
```

---

### 28.16 Add Telemetry History API Later

Later endpoint:

```text
GET /api/devices/{deviceId}/telemetry?from=...&to=...&key=temperatureC
```

Use this for charts later.

Do not build complex chart APIs before the first real device works.

---

### 28.17 Add Command History API Later

Later endpoint:

```text
GET /api/devices/{deviceId}/commands
```

Useful for debugging:

```text
command sent
command applied
command failed
timeout
```

---

### 28.18 Add Scheduled Command Timeout Task

Add scheduled task:

```text
Every 5-10 seconds:
- find commands with status PUBLISHED older than timeout threshold
- mark them TIMEOUT
```

Example:

```text
PUBLISHED command older than 15 seconds -> TIMEOUT
```

---

### 28.19 Add Offline Detection

There are two ways:

```text
1. MQTT LWT status message
2. backend scheduled task checks lastSeenAt age
```

Use both eventually.

For first version:

```text
Use LWT if easy.
Also mark device stale/offline if lastSeenAt too old.
```

---

### 28.20 Add Tests

Backend tests to add:

```text
Envelope parsing test
Invalid JSON does not crash listener
Topic deviceId mismatch is rejected
Telemetry updates latest state
State updates latest state
Command API creates command and publishes MQTT
Command result updates command status
Duplicate messageId is handled
Unknown device behavior works as decided
```

---

## 29. Recommended Implementation Order

Do not implement everything at once.

Recommended order:

```text
1. Add Mosquitto to Docker Compose
2. Add mqtt-test.sh script with mosquitto_pub/sub
3. Define envelope classes and enums in backend
4. Add backend MQTT subscribe to telemetry topic
5. Store raw incoming MQTT events
6. Parse TELEMETRY and update latest state
7. Add GET /api/devices/{deviceId}/state
8. Add backend command API
9. Publish COMMAND to MQTT
10. Simulate device command-result with mosquitto_pub
11. Store/update command result
12. Add real ESP32 firmware using the same contract
13. Add Angular state display and command buttons
```

---

## 30. First Minimal End-To-End Test

Use a fake device before real ESP32 firmware.

Subscribe to commands:

```bash
mosquitto_sub -h localhost -t 'homeops/devices/greenhouse-esp32-01/command' -v
```

Publish telemetry:

```bash
mosquitto_pub -h localhost -t 'homeops/devices/greenhouse-esp32-01/telemetry' -m '{
  "schemaVersion": 1,
  "messageId": "test-telemetry-001",
  "correlationId": null,
  "messageType": "TELEMETRY",
  "deviceId": "greenhouse-esp32-01",
  "source": "DEVICE",
  "sentAt": "2026-07-01T18:30:00Z",
  "data": {
    "readings": {
      "temperatureC": { "value": 24.6, "unit": "C" },
      "humidityPercent": { "value": 63.2, "unit": "%" }
    }
  }
}'
```

Expected backend result:

```text
- backend logs incoming MQTT message
- device lastSeenAt updates
- telemetry event is stored
- latest state has temperatureC and humidityPercent
```

---

## 31. Final Agreement Checklist

Before backend/firmware implementation, agree on these exact points:

```text
[ ] Broker host
[ ] Broker port
[ ] Broker software
[ ] MQTT auth yes/no
[ ] MQTT username/password handling
[ ] MQTT protocol version
[ ] QoS per message type
[ ] Retained messages yes/no per topic
[ ] LWT enabled yes/no
[ ] Topic prefix
[ ] Topic structure
[ ] Device ID format
[ ] First device IDs
[ ] First device types
[ ] First capabilities
[ ] Envelope fields
[ ] Message ID strategy
[ ] Timestamp strategy
[ ] Allowed message types
[ ] Allowed source values
[ ] HELLO payload
[ ] STATUS payload
[ ] TELEMETRY payload
[ ] STATE payload
[ ] COMMAND payload
[ ] COMMAND_RESULT payload
[ ] ERROR payload
[ ] Dynamic reading key naming
[ ] Units
[ ] Output/actuator names
[ ] Allowed commands
[ ] Allowed command result statuses
[ ] Error codes
[ ] Telemetry interval
[ ] State interval
[ ] Command timeout
[ ] Offline timeout
[ ] Unknown device behavior
[ ] Duplicate message behavior
[ ] Backend source-of-truth rules
[ ] Device reconnect behavior
[ ] Security rules
[ ] Backend database tables
[ ] Backend REST endpoints
[ ] First fake-device test
[ ] First real ESP32 test
```

---

## 32. Recommended v1 Decisions

These are the recommended defaults for HomeOps v1.

```text
Broker: Mosquitto
MQTT version: 3.1.1
Topic prefix: homeops/devices
Telemetry QoS: 0
State QoS: 1
Command QoS: 1
Command result QoS: 1
Commands retained: no
Status retained: yes
LWT: yes
Envelope schemaVersion: 1
Payload format: JSON
Dynamic payload field: data
Dynamic sensor format: data.readings.{key}.value/unit
Device ID format: lowercase-kebab-case
Command timeout: 15 seconds
Telemetry interval: 30 seconds
State interval: 60 seconds or on change
Backend data model: raw event history + latest key-value state
Frontend: talks only to backend, not MQTT
```

---

## 33. Practical Rule

The contract should be strict at the envelope level and flexible inside `data`.

That means:

```text
Stable envelope
Flexible dynamic payload
Backend validates the envelope
Backend stores raw JSON
Backend extracts known values into latest state
Frontend displays selected values cleanly
```

This gives the project enough structure to avoid chaos, but enough flexibility to support greenhouse sensors, fans, Pico
miners, Antminer monitoring, and future ESP32 projects.
