import { CommonModule } from '@angular/common';
import {Component, OnInit, signal} from '@angular/core';
import { SystemApiService } from '../../../../core/api/system-api.service';
import { SystemInfoDto } from '../../../../core/dtos/system-info.dto';

@Component({
  selector: 'app-dashboard-page',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './dashboard-page.html',
  styleUrls: ['./dashboard-page.scss']
})
export class DashboardPage implements OnInit {

  systemInfo = signal<SystemInfoDto | undefined>(undefined);
  loading = signal<boolean>(true);
  errorMessage = signal<string | undefined>(undefined);

  constructor(private systemApiService: SystemApiService) {
  }

  ngOnInit(): void {
    this.loadSystemInfo();
  }

  loadSystemInfo(): void {
    this.loading.set(true);
    this.errorMessage.set(undefined);

    this.systemApiService.getSystemInfo().subscribe({
      next: systemInfo => {
        this.systemInfo.set(systemInfo);
        this.loading.set(false);
      },
      error: error => {
        console.error('Failed to load system info', error);
        this.errorMessage.set('Could not load backend system information.');
        this.loading.set(false);
      }
    });
  }
}
