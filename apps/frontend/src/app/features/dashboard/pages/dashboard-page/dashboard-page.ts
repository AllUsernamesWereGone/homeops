import {CommonModule} from '@angular/common';
import {Component, computed, OnInit, signal} from '@angular/core';
import {forkJoin} from 'rxjs';
import {SystemInfoDto} from '../../../../api/models/system-info-dto';
import {DeviceDto} from '../../../../api/models/device-dto';
import {SystemApiService} from '../../../../core/api/system-api.service';
import {DeviceApiService} from '../../../../core/api/device-api.service';


@Component({
  selector: 'app-dashboard-page',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './dashboard-page.html',
  styleUrl: './dashboard-page.scss'
})
export class DashboardPage implements OnInit {

  systemInfo = signal<SystemInfoDto | undefined>(undefined);
  devices = signal<DeviceDto[]>([]);

  loading = signal<boolean>(true);
  errorMessage = signal<string | undefined>(undefined);

  onlineDeviceCount = computed(() =>
    this.devices().filter(device => device.status === 'ONLINE').length
  );


  constructor(
    private systemApiService: SystemApiService,
    private deviceApiService: DeviceApiService
  ) {
  }

  ngOnInit(): void {
    this.loadDashboard();
  }

  loadDashboard(): void {
    this.loading.set(true);
    this.errorMessage.set(undefined);

    forkJoin({
      systemInfo: this.systemApiService.getSystemInfo(),
      devices: this.deviceApiService.findAll()
    }).subscribe({
      next: result => {
        this.systemInfo.set(result.systemInfo);
        this.devices.set(result.devices);
        this.loading.set(false);
      },
      error: error => {
        console.error('Failed to load dashboard data', error);
        this.errorMessage.set('Could not load dashboard data from the backend.');
        this.loading.set(false);
      }
    });
  }

  formatCapability(capability: string): string {
    return capability
      .toLowerCase()
      .replaceAll('_', ' ')
      .replace(/\b\w/g, character => character.toUpperCase());
  }

  formatDateTime(value?: string): string {
    if (!value) {
      return 'Never';
    }

    return new Date(value).toLocaleString();
  }
}
