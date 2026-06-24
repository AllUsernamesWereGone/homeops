import {Routes} from '@angular/router';


export const routes: Routes = [
  {
    path: '',
    loadChildren: () =>
      import('./features/dashboard/dashboard.routes').then(m => m.dashboardRoutes)
  },
  {
    path: 'devices',
    loadChildren: () =>
      import('./features/devices/devices.routes').then(m => m.deviceRoutes)
  },
  {
    path: '**',
    redirectTo: ''
  }
]
