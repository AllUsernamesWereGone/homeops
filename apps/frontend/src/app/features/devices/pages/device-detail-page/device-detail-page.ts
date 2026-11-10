import {CommonModule} from '@angular/common';
import {HttpErrorResponse} from '@angular/common/http';
import {Component, computed, OnInit, signal} from '@angular/core';
import {ActivatedRoute, Router, RouterLink} from '@angular/router';

import {DeviceDto} from '../../../../api/models/device-dto';
import {DeviceApiService} from '../../../../core/api/device-api.service';
import {DeviceStateApiService} from '../../../../core/api/device-state-api.service';
import {getApiErrorMessage} from '../../../../shared/utils/api-error.util';
import {
  buildControlCards,
  buildTelemetryCards,
  DeviceControlChange,
  DeviceControlVm
} from '../../utils/device-detail-view-model.util';
import {DeviceControlModal} from '../../components/device-control-modal/device-control-modal';
import {DeviceTelemetryStateDto} from '../../../../api/models/device-telemetry-state-dto';
import {DeviceCommandApiService} from '../../../../core/api/device-command-api.service';

@Component({
  selector: 'app-device-detail-page',
  standalone: true,
  imports: [CommonModule, RouterLink, DeviceControlModal],
  templateUrl: './device-detail-page.html',
  styleUrl: './device-detail-page.scss'
})
export class DeviceDetailPage implements OnInit {

  device = signal<DeviceDto | undefined>(undefined);
  deviceState = signal<DeviceTelemetryStateDto | undefined>(undefined);

  loading = signal<boolean>(true);
  stateLoading = signal<boolean>(false);

  errorMessage = signal<string | undefined>(undefined);
  stateErrorMessage = signal<string | undefined>(undefined);
  controlMessage = signal<string | undefined>(undefined);

  selectedControl = signal<DeviceControlVm | undefined>(undefined);

  telemetryCards = computed(() => buildTelemetryCards(this.deviceState()));
  controlCards = computed(() => buildControlCards(this.device(), this.deviceState()));

  commandPublishing = signal<boolean>(false);

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private deviceApiService: DeviceApiService,
    private deviceStateApiService: DeviceStateApiService,
    private deviceCommandApiService: DeviceCommandApiService
  ) {
  }

  ngOnInit(): void {
    const deviceId = this.route.snapshot.paramMap.get('deviceId');

    if (!deviceId) {
      this.errorMessage.set('Missing device id.');
      this.loading.set(false);
      return;
    }

    this.loadDevice(deviceId);
    this.loadLatestState(deviceId);
  }

  loadDevice(deviceId: string): void {
    this.loading.set(true);
    this.errorMessage.set(undefined);

    this.deviceApiService.findById(deviceId).subscribe({
      next: device => {
        this.device.set(device);
        this.loading.set(false);
      },
      error: error => {
        console.error('Failed to load device', error);
        this.errorMessage.set('Could not load device.');
        this.loading.set(false);
      }
    });
  }

  loadLatestState(deviceId?: string): void {
    const effectiveDeviceId = deviceId ?? this.device()?.deviceId;

    if (!effectiveDeviceId) {
      return;
    }

    this.stateLoading.set(true);
    this.stateErrorMessage.set(undefined);

    this.deviceStateApiService.findLatestByDeviceId(effectiveDeviceId).subscribe({
      next: state => {
        this.deviceState.set(state);
        this.stateLoading.set(false);
      },
      error: error => {
        console.warn('Failed to load latest device state', error);
        this.deviceState.set(undefined);
        this.stateErrorMessage.set(this.toStateErrorMessage(error));
        this.stateLoading.set(false);
      }
    });
  }

  openControl(control: DeviceControlVm): void {
    this.controlMessage.set(undefined);
    this.selectedControl.set(control);
  }

  closeControlModal(): void {
    this.selectedControl.set(undefined);
  }

  applyControlChange(change: DeviceControlChange): void {
    const deviceId = this.device()?.deviceId;

    if (!deviceId) {
      this.controlMessage.set('Cannot publish command because the device id is missing.');
      this.selectedControl.set(undefined);
      return;
    }

    this.commandPublishing.set(true);
    this.controlMessage.set(undefined);

    this.deviceCommandApiService.publishCommand(deviceId, {
      command: change.control.command,
      target: change.control.target,
      property: change.control.property,
      value: this.deviceCommandApiService.toJsonNode(change.value)
    }).subscribe({
      next: result => {
        this.selectedControl.set(undefined);
        this.commandPublishing.set(false);

        this.controlMessage.set(
          `Published ${result.command} to ${result.topic}.`
        );
      },
      error: error => {
        console.error('Failed to publish command', error);
        this.commandPublishing.set(false);
        this.controlMessage.set('Could not publish device command.');
      }
    });
  }

  deleteDevice(): void {
    const device = this.device();

    if (!device) {
      return;
    }

    const deviceId = device.deviceId;

    if (!deviceId) {
      this.errorMessage.set('Cannot delete device because the device id is missing.');
      return;
    }

    const displayName = device.displayName ?? deviceId;
    const confirmed = confirm(`Delete device "${displayName}"?`);

    if (!confirmed) {
      return;
    }

    this.deviceApiService.delete(deviceId).subscribe({
      next: () => this.router.navigate(['/devices']),
      error: error => {
        console.error('Failed to delete device', error);
        this.errorMessage.set('Could not delete device.');
      }
    });
  }

  formatDateTime(value?: string): string {
    if (!value) {
      return 'Never';
    }

    return new Date(value).toLocaleString();
  }

  formatCapability(capability: string): string {
    return capability
      .toLowerCase()
      .replaceAll('_', ' ')
      .replace(/\b\w/g, character => character.toUpperCase());
  }

  private toStateErrorMessage(error: unknown): string {
    if (error instanceof HttpErrorResponse && error.status === 404) {
      return 'No latest telemetry or state has been stored for this device yet.';
    }

    return getApiErrorMessage(error, 'Could not load latest device state.');
  }
}
