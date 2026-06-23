import {CommonModule} from '@angular/common'
import {Component, OnInit, signal} from '@angular/core'
import {Router, RouterLink} from '@angular/router'
import {DeviceDto} from '../../../../api/models/device-dto'
import {DeviceApiService} from '../../../../core/api/device-api.service'

@Component({
  selector: 'app-device-list-page',
  standalone: true,
  imports: [CommonModule, RouterLink],
  templateUrl: './device-list-page.html',
  styleUrl: './device-list-page.scss'
})
export class DeviceListPage implements OnInit {

  devices = signal<DeviceDto[]>([])
  loading = signal<boolean>(true)
  errorMessage = signal<string | undefined>(undefined)

  constructor(private deviceApiService: DeviceApiService,
              private router: Router) {
  }

  ngOnInit(): void {
    this.loadDevices()
  }

  loadDevices(): void {
    this.loading.set(true)
    this.errorMessage.set(undefined)

    this.deviceApiService.findAll().subscribe({
      next: devices => {
        this.devices.set(devices)
        this.loading.set(false)
      },
      error: error => {
        console.error('Failed to load devices', error)
        this.errorMessage.set('Could not load devices from the backend.')
        this.loading.set(false)
      }
    })
  }

  deleteDevice(device: DeviceDto): void {
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
      next: () => {
        this.devices.update(devices =>
          devices.filter(currentDevice => currentDevice.deviceId !== deviceId)
        );
      },
      error: error => {
        console.error('Failed to delete device', error);
        this.errorMessage.set('Could not delete device.');
      }
    });
  }
}
