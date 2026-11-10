import {Injectable} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {Observable} from 'rxjs';
import {map} from 'rxjs/operators';

import {ApiConfiguration} from '../../api/api-configuration';
import {findLatestStateByDeviceId} from '../../api/functions';
import {DeviceTelemetryStateDto} from '../../api/models/device-telemetry-state-dto';

@Injectable({
  providedIn: 'root'
})
export class DeviceStateApiService {

  constructor(
    private http: HttpClient,
    private apiConfiguration: ApiConfiguration
  ) {
  }

  findLatestByDeviceId(deviceId: string): Observable<DeviceTelemetryStateDto> {
    return findLatestStateByDeviceId(this.http, this.apiConfiguration.rootUrl, {deviceId}).pipe(
      map(response => response.body)
    );
  }
}
