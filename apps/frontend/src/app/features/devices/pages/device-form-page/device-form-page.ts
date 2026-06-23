import {CommonModule} from '@angular/common';
import {Component, inject, OnInit, signal} from '@angular/core';
import {FormBuilder, ReactiveFormsModule, Validators} from '@angular/forms';
import {ActivatedRoute, Router, RouterLink} from '@angular/router';

import {DeviceCreateDto} from '../../../../api/models/device-create-dto';
import {DeviceDto} from '../../../../api/models/device-dto';
import {DeviceUpdateDto} from '../../../../api/models/device-update-dto';
import {DEVICE_TYPE} from '../../../../api/models/device-type-array';
import {DEVICE_STATUS} from '../../../../api/models/device-status-array';
import {DEVICE_TRANSPORT} from '../../../../api/models/device-transport-array';
import {DeviceCapability} from '../../../../api/models/device-capability';
import {DEVICE_CAPABILITY} from '../../../../api/models/device-capability-array';

import {DeviceApiService} from '../../../../core/api/device-api.service';

@Component({
  selector: 'app-device-form-page',
  standalone: true,
  imports: [CommonModule, ReactiveFormsModule, RouterLink],
  templateUrl: './device-form-page.html',
  styleUrl: './device-form-page.scss'
})
export class DeviceFormPage implements OnInit {

  deviceId = signal<string | undefined>(undefined);
  loadedDevice = signal<DeviceDto | undefined>(undefined);

  loading = signal<boolean>(false);
  saving = signal<boolean>(false);
  errorMessage = signal<string | undefined>(undefined);

  readonly deviceTypes = DEVICE_TYPE
  readonly deviceStatuses = DEVICE_STATUS;
  readonly deviceTransports = DEVICE_TRANSPORT;
  readonly deviceCapabilities = DEVICE_CAPABILITY;
  private readonly formBuilder = inject(FormBuilder);

  form = this.formBuilder.nonNullable.group({
    deviceId: ['', [Validators.required]],
    displayName: ['', [Validators.required]],
    type: [DEVICE_TYPE[0], Validators.required],
    status: [DEVICE_STATUS[0], Validators.required],
    transport: [DEVICE_TRANSPORT[0], Validators.required],
    role: [''],
    location: [''],
    description: [''],
    enabled: [true],
    capabilities: [[] as DeviceCapability[]]
  });

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private deviceApiService: DeviceApiService
  ) {
  }

  ngOnInit(): void {
    const deviceId = this.route.snapshot.paramMap.get('deviceId') ?? undefined;
    this.deviceId.set(deviceId);

    if (deviceId) {
      this.loadDevice(deviceId);
    }
  }

  get editMode(): boolean {
    return !!this.deviceId();
  }

  loadDevice(deviceId: string): void {
    this.loading.set(true);
    this.errorMessage.set(undefined);

    this.deviceApiService.findById(deviceId).subscribe({
      next: device => {
        this.loadedDevice.set(device);

        this.form.patchValue({
          deviceId: device.deviceId ?? '',
          displayName: device.displayName ?? '',
          type: device.type ?? DEVICE_TYPE[0],
          status: device.status ?? DEVICE_STATUS[0],
          transport: device.transport ?? DEVICE_TRANSPORT[0],
          role: device.role ?? '',
          location: device.location ?? '',
          description: device.description ?? '',
          enabled: device.enabled ?? true,
          capabilities: device.capabilities ?? []
        });

        this.form.controls.deviceId.disable();
        this.loading.set(false);
      },
      error: error => {
        console.error('Failed to load device', error);
        this.errorMessage.set('Could not load device.');
        this.loading.set(false);
      }
    });
  }

  save(): void {
    if (this.form.invalid) {
      this.form.markAllAsTouched();
      return;
    }

    this.saving.set(true);
    this.errorMessage.set(undefined);

    if (this.editMode) {
      this.updateDevice();
    } else {
      this.createDevice();
    }
  }

  private createDevice(): void {
    const rawValue = this.form.getRawValue();

    const request: DeviceCreateDto = {
      deviceId: rawValue.deviceId,
      displayName: rawValue.displayName,
      type: rawValue.type,
      transport: rawValue.transport,
      role: this.emptyToUndefined(rawValue.role),
      location: this.emptyToUndefined(rawValue.location),
      description: this.emptyToUndefined(rawValue.description),
      capabilities: rawValue.capabilities
    };

    this.deviceApiService.create(request).subscribe({
      next: device => {
        const createdDeviceId = device.deviceId ?? request.deviceId;
        this.router.navigate(['/devices', createdDeviceId]);
      },
      error: error => {
        console.error('Failed to create device', error);
        this.errorMessage.set('Could not create device.');
        this.saving.set(false);
      }
    });
  }

  private updateDevice(): void {
    const deviceId = this.deviceId();
    const rawValue = this.form.getRawValue();
    const loadedDevice = this.loadedDevice();

    if (!deviceId) {
      this.errorMessage.set('Missing device id.');
      this.saving.set(false);
      return;
    }

    if (loadedDevice?.version === undefined) {
      this.errorMessage.set('Missing device version. Reload the page and try again.');
      this.saving.set(false);
      return;
    }

    const request: DeviceUpdateDto = {
      displayName: rawValue.displayName,
      type: rawValue.type,
      status: rawValue.status,
      transport: rawValue.transport,
      role: this.emptyToUndefined(rawValue.role),
      location: this.emptyToUndefined(rawValue.location),
      description: this.emptyToUndefined(rawValue.description),
      enabled: rawValue.enabled,
      capabilities: rawValue.capabilities,
      version: loadedDevice.version
    };

    this.deviceApiService.update(deviceId, request).subscribe({
      next: device => {
        const updatedDeviceId = device.deviceId ?? deviceId;
        this.router.navigate(['/devices', updatedDeviceId]);
      },
      error: error => {
        console.error('Failed to update device', error);
        this.errorMessage.set('Could not update device.');
        this.saving.set(false);
      }
    });
  }

  private emptyToUndefined(value: string): string | undefined {
    const trimmed = value.trim();
    return trimmed.length === 0 ? undefined : trimmed;
  }
}
