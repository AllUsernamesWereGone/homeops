import {CommonModule} from '@angular/common'
import {Component, OnInit, signal} from '@angular/core'
import {ActivatedRoute, Router, RouterLink} from '@angular/router'
import {DeviceDto} from '../../../../api/models/device-dto'
import {DeviceApiService} from '../../../../core/api/device-api.service'

@Component({
  selector: 'app-device-detail-page',
  standalone: true,
  imports: [CommonModule, RouterLink],
  templateUrl: './device-detail-page.html',
  styleUrl: './device-detail-page.scss'
})
export class DeviceDetailPage implements OnInit {

  device = signal<DeviceDto | undefined>(undefined)
  loading = signal<boolean>(true)
  errorMessage = signal<string | undefined>(undefined)

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private deviceApiService: DeviceApiService
  ) {
  }

  ngOnInit(): void {
    const deviceId = this.route.snapshot.paramMap.get('deviceId')

    if (!deviceId) {
      this.errorMessage.set('Missing device id.')
      this.loading.set(false)
      return
    }

    this.loadDevice(deviceId)
  }

  loadDevice(deviceId: string): void {
    this.loading.set(true)
    this.errorMessage.set(undefined)

    this.deviceApiService.findById(deviceId).subscribe({
      next: device => {
        this.device.set(device)
        this.loading.set(false)
      },
      error: error => {
        console.error('Failed to load device', error)
        this.errorMessage.set('Could not load device.')
        this.loading.set(false)
      }
    })
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

}
