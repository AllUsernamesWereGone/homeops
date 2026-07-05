import {DeviceCapability} from '../../../api/models/device-capability';
import {DeviceDto} from '../../../api/models/device-dto';
import {DeviceTelemetryStateDto} from '../../../api/models/device-telemetry-state-dto';


export type DeviceControlKind = 'switch' | 'number' | 'action';

export type DeviceControlValue = string | number | boolean | null;

export interface DeviceTelemetryCardVm {
  key: string;
  label: string;
  value: string;
  unit?: string;
  description?: string;
}

export interface DeviceControlVm {
  id: string;
  kind: DeviceControlKind;
  label: string;
  target: string;
  property: string;
  currentValue?: DeviceControlValue;
  unit?: string;
  min?: number;
  max?: number;
  step?: number;
  command: string;
  description?: string;
}

export interface DeviceControlChange {
  control: DeviceControlVm;
  value: DeviceControlValue;
}

const READING_LABELS: Record<string, string> = {
  temperatureC: 'Temperature',
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

export function buildTelemetryCards(
  state: DeviceTelemetryStateDto | undefined
): DeviceTelemetryCardVm[] {
  const readings = getRecord(getStateData(state), 'readings');

  return Object.entries(readings)
    .map(([key, rawReading]) => {
      const reading = asRecord(rawReading);
      const value = reading['value'];
      const unit = asString(reading['unit']);

      if (value === undefined || value === null) {
        return undefined;
      }

      const card: DeviceTelemetryCardVm = {
        key,
        label: formatReadingLabel(key),
        value: formatValue(value),
        description: key
      };

      if (unit !== undefined) {
        card.unit = unit;
      }

      return card;
    })
    .filter((card): card is DeviceTelemetryCardVm => card !== undefined);
}

export function buildControlCards(
  device: DeviceDto | undefined,
  state: DeviceTelemetryStateDto | undefined
): DeviceControlVm[] {
  const capabilities = new Set<DeviceCapability>(device?.capabilities ?? []);
  const outputs = getRecord(getStateData(state), 'outputs');
  const controls: DeviceControlVm[] = [];

  Object.entries(outputs).forEach(([target, rawOutput]) => {
    const output = asRecord(rawOutput);

    controls.push(buildSwitchControl(target, asString(output['state']) ?? 'UNKNOWN'));

    const rpm = asNumber(output['rpm']);

    if (rpm !== undefined || target.toLowerCase().includes('fan')) {
      controls.push(buildFanRpmControl(target, rpm));
    }

    const speedPercent = asNumber(output['speedPercent']);

    if (speedPercent !== undefined) {
      controls.push(buildPercentControl(target, 'speedPercent', 'Fan speed', speedPercent));
    }

    const brightnessPercent = asNumber(output['brightnessPercent']);

    if (brightnessPercent !== undefined) {
      controls.push(buildPercentControl(target, 'brightnessPercent', 'Brightness', brightnessPercent));
    }
  });

  if (controls.length === 0 && capabilities.has('FAN_SWITCH')) {
    controls.push(buildSwitchControl('fan1', 'UNKNOWN'));
  }

  if (controls.length === 0 && capabilities.has('LIGHT_SWITCH')) {
    controls.push(buildSwitchControl('light1', 'UNKNOWN'));
  }

  const fanRpmReading = asRecord(getRecord(getStateData(state), 'readings')['fanRpm']);
  const fanRpm = asNumber(fanRpmReading['value']);

  if (fanRpm !== undefined && !controls.some(control => control.id === 'fan1-rpm')) {
    controls.push(buildFanRpmControl('fan1', fanRpm));
  }

  return controls;
}

function getStateData(state: DeviceTelemetryStateDto | undefined): Record<string, unknown> {
  return asRecord(state?.data);
}

function getRecord(source: Record<string, unknown>, key: string): Record<string, unknown> {
  return asRecord(source[key]);
}

function buildSwitchControl(target: string, state: string): DeviceControlVm {
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

function buildFanRpmControl(target: string, rpm?: number): DeviceControlVm {
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
    description: 'Set a target fan speed. Backend command publishing comes next.'
  };
}

function buildPercentControl(
  target: string,
  property: string,
  label: string,
  currentValue: number
): DeviceControlVm {
  return {
    id: `${target}-${property}`,
    kind: 'number',
    label: `${formatTargetLabel(target)} ${label}`,
    target,
    property,
    currentValue,
    unit: '%',
    min: 0,
    max: 100,
    step: 5,
    command: 'SET_OUTPUT_LEVEL',
    description: 'Set an output level.'
  };
}

function formatReadingLabel(key: string): string {
  return READING_LABELS[key] ?? formatCamelCase(key);
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
    .replace(/\b\w/g, character => character.toUpperCase());
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
