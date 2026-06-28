import {Routes} from '@angular/router';
import {DeviceFormPage} from './pages/device-form-page/device-form-page';
import {DeviceDetailPage} from './pages/device-detail-page/device-detail-page';
import {DeviceListPage} from './pages/device-list-page/device-list-page';

export const deviceRoutes: Routes = [
  {
    path: '',
    component: DeviceListPage
  },
  {
    path: 'new',
    component: DeviceFormPage
  },
  {
    path: ':deviceId',
    component: DeviceDetailPage,
    data: {
      rightSidebar: true
    }
  },
  {
    path: ':deviceId/edit',
    component: DeviceFormPage,
    data: {
      rightSidebar: true
    }
  }
]
