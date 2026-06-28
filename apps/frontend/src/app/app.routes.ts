import {Routes} from '@angular/router';
import {AppShellComponent} from './layout/app-shell/app-shell.component';


export const routes: Routes = [
  {
    path: '',
    component: AppShellComponent,
    children: [
      {
        path: '',
        loadChildren: () =>
          import('./features/dashboard/dashboard.routes').then(m => m.dashboardRoutes)
      },
      {
        path: 'devices',
        loadChildren: () =>
          import('./features/devices/devices.routes').then(m => m.deviceRoutes)
      }
    ]
  },
  {
    path: '**',
    redirectTo: ''
  }
];
