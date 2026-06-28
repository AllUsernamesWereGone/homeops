import {Component, computed, inject, signal} from '@angular/core';
import {RouterLink} from '@angular/router';

import {HealthApiService} from '../../../core/api/health-api.service';
import {timeout} from 'rxjs';

@Component({
  selector: 'app-header',
  standalone: true,
  imports: [
    RouterLink
  ],
  templateUrl: './header.component.html',
  styleUrl: './header.component.scss'
})
export class HeaderComponent {

  private readonly healthApiService = inject(HealthApiService);

  protected readonly backendUp = signal<boolean | null>(null);
  private readonly healthCheckIntervalId: number;


  protected readonly healthLabel = computed(() => {
    const backendUp = this.backendUp();

    if (backendUp === null) {
      return 'Checking...';
    }

    return backendUp ? 'Server Up' : 'Server Down';
  });

  protected readonly healthTitle = computed(() => {
    const backendUp = this.backendUp();

    if (backendUp === null) {
      return 'Checking backend status...';
    }

    return backendUp
      ? 'Backend health endpoint returned OK.'
      : 'The frontend cannot reach the backend health endpoint.';
  });

  constructor() {
    this.checkHealth();

    this.healthCheckIntervalId = window.setInterval(() => {
      this.checkHealth();
    }, 10_000);
  }

  ngOnDestroy(): void {
    window.clearInterval(this.healthCheckIntervalId);
  }

  private checkHealth(): void {
    this.healthApiService.getHealth()
      .pipe(timeout(3_000))
      .subscribe({
        next: response => {
          this.backendUp.set(response.trim() === 'OK');
        },
        error: () => {
          this.backendUp.set(false);
        },
      });
  }
}
