import {DeviceCapability} from '../../../api/models/device-capability';
import {DeviceDto} from '../../../api/models/device-dto';
import {DeviceTelemetryStateDto} from '../../../api/models/device-telemetry-state-dto';

export interface DeviceControlVm {
  id: string;
  kind: 'switch' | 'number';
  label: string;
  target: string;
  property: string;
  currentValue?: string | number | boolean | null;
  unit?: string;
  min?: number;
  max?: number;
  step?: number;
  command: string;
  description?: string;
}

export interface DeviceControlChange {
  control: DeviceControlVm;
  value: DeviceControlVm['currentValue'];
}

const TELEMETRY_LABELS: Record<string, string> = {
  temperature: 'Temperature',
  temperatureC: 'Temperature',
  humidity: 'Humidity',
  humidityPercent: 'Humidity',
  lightLux: 'Light',
  soilMoisturePercent: 'Soil moisture',
  wifiRssi: 'Wi-Fi RSSI',
  uptimeSeconds: 'Uptime',
  hashrate: 'Hashrate',
  fanRpm: 'Fan RPM',
  powerWatts: 'Power',
  voltageV: 'Voltage',
  currentA: 'Current',
  mq2Raw: 'MQ-2 raw',
  mq2MilliVolts: 'MQ-2 voltage'
};

const TELEMETRY_UNITS: Record<string, string> = {
  temperature: 'C',
  temperatureC: 'C',
  humidity: '%',
  humidityPercent: '%',
  lightLux: 'lux',
  soilMoisturePercent: '%',
  wifiRssi: 'dBm',
  uptimeSeconds: 's',
  hashrate: 'H/s',
  fanRpm: 'rpm',
  powerWatts: 'W',
  voltageV: 'V',
  currentA: 'A',
  mq2MilliVolts: 'mV'
};

export function buildTelemetryCards(
  state: DeviceTelemetryStateDto | undefined
): Array<{ key: string; label: string; value: string; unit?: string; description?: string }> {
  const telemetry = getTelemetryValues(state);

  return Object.entries(telemetry)
    .map(([key, entry]) => {
      if (entry.value === undefined || entry.value === null) {
        return undefined;
      }

      const unit = entry.unit ?? TELEMETRY_UNITS[key];

      const card: {
        key: string;
        label: string;
        value: string;
        unit?: string;
        description?: string;
      } = {
        key,
        label: TELEMETRY_LABELS[key] ?? formatCamelCase(key),
        value: formatValue(entry.value),
        description: key
      };

      if (unit !== undefined) {
        card.unit = unit;
      }

      return card;
    })
    .filter(card => card !== undefined);
}

export function buildControlCards(
  device: DeviceDto | undefined,
  state: DeviceTelemetryStateDto | undefined
): DeviceControlVm[] {
  const capabilities = new Set<DeviceCapability>(device?.capabilities ?? []);
  const telemetry = getTelemetryValues(state);
  const data = getStateData(state);
  const fanOutput = getRecord(getRecord(data, 'outputs'), 'fan1');
  const lightOutput = getRecord(getRecord(data, 'outputs'), 'light1');

  const controls: DeviceControlVm[] = [];

  if (capabilities.has('FAN_SWITCH')) {
    controls.push(createSwitchControl(
      'fan1',
      asString(fanOutput['state']) ?? 'UNKNOWN'
    ));
  }

  if (capabilities.has('FAN_RPM_CONTROL')) {
    controls.push(createFanRpmControl(
      'fan1',
      asNumber(fanOutput['rpm']) ?? asNumber(telemetry['fanRpm']?.value)
    ));
  }

  if (capabilities.has('LIGHT_SWITCH')) {
    controls.push(createSwitchControl(
      'light1',
      asString(lightOutput['state']) ?? 'UNKNOWN'
    ));
  }

  return controls;
}

function getTelemetryValues(
  state: DeviceTelemetryStateDto | undefined
): Record<string, { value: unknown; unit?: string }> {
  const data = getStateData(state);
  const result: Record<string, { value: unknown; unit?: string }> = {};

  for (const [key, value] of Object.entries(data)) {
    if (key === 'readings' || key === 'outputs') {
      continue;
    }

    if (isDisplayableValue(value)) {
      result[key] = {value};
    }
  }

  const readings = getRecord(data, 'readings');

  for (const [key, rawReading] of Object.entries(readings)) {
    if (isDisplayableValue(rawReading)) {
      result[key] = {value: rawReading};
      continue;
    }

    const reading = asRecord(rawReading);
    const value = reading['value'];

    if (!isDisplayableValue(value)) {
      continue;
    }

    const unit = asString(reading['unit']);

    result[key] = unit === undefined
      ? {value}
      : {value, unit};
  }

  return result;
}

function createSwitchControl(target: string, state: string): DeviceControlVm {
  return {
    id: `${target}-state`,
    kind: 'switch',
    label: formatTargetLabel(target),
    target,
    property: 'state',
    currentValue: state,
    command: 'SET_OUTPUT',
    description: 'Switch output on or off.'
  };
}

function createFanRpmControl(target: string, rpm?: number): DeviceControlVm {
  return {
    id: `${target}-rpm`,
    kind: 'number',
    label: `${formatTargetLabel(target)} RPM`,
    target,
    property: 'rpm',
    currentValue: rpm ?? 0,
    unit: 'rpm',
    min: 0,
    max: 5000,
    step: 50,
    command: 'SET_FAN_RPM',
    description: 'Set the fan target speed.'
  };
}

function getStateData(state: DeviceTelemetryStateDto | undefined): Record<string, unknown> {
  return asRecord(state?.data);
}

function getRecord(source: Record<string, unknown>, key: string): Record<string, unknown> {
  return asRecord(source[key]);
}

function asRecord(value: unknown): Record<string, unknown> {
  if (typeof value === 'object' && value !== null && !Array.isArray(value)) {
    return value as Record<string, unknown>;
  }

  return {};
}

function asNumber(value: unknown): number | undefined {
  return typeof value === 'number' && Number.isFinite(value)
    ? value
    : undefined;
}

function asString(value: unknown): string | undefined {
  return typeof value === 'string'
    ? value
    : undefined;
}

function isDisplayableValue(value: unknown): boolean {
  return typeof value === 'string'
    || typeof value === 'number'
    || typeof value === 'boolean';
}

function formatValue(value: unknown): string {
  if (typeof value === 'number') {
    return Number.isInteger(value)
      ? value.toString()
      : value.toFixed(1);
  }

  if (typeof value === 'string' || typeof value === 'boolean') {
    return String(value);
  }

  return 'Unknown';
}

function formatTargetLabel(target: string): string {
  return target
    .replace(/([a-z])([0-9])/g, '$1 $2')
    .replaceAll('-', ' ')
    .replaceAll('_', ' ')
    .replace(/\b\w/g, character => character.toUpperCase());
}

function formatCamelCase(value: string): string {
  return value
    .replace(/([a-z])([A-Z])/g, '$1 $2')
    .replaceAll('_', ' ')
    .replaceAll('-', ' ')
    .replace(/\b\w/g, character => character.toUpperCase());
}
