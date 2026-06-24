import {Injectable} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {Observable} from 'rxjs';
import {DeviceDto} from '../../api/models/device-dto';
import {DeviceCreateDto} from '../../api/models/device-create-dto';
import {DeviceUpdateDto} from '../../api/models/device-update-dto';
import {ApiConfiguration} from '../../api/api-configuration';
import {map} from 'rxjs/operators';
import {create, delete$, findAll, findByDeviceId, update} from '../../api/functions';


@Injectable({
  providedIn: 'root'
})
export class DeviceApiService {

  constructor(
    private http: HttpClient,
    private apiConfiguration: ApiConfiguration
  ) {
  }

  findAll(): Observable<DeviceDto[]> {
    return findAll(this.http, this.apiConfiguration.rootUrl, {},).pipe(
      map(response => response.body ?? [])
    );
  }

  findById(deviceId: string): Observable<DeviceDto> {
    return findByDeviceId(this.http, this.apiConfiguration.rootUrl, {deviceId}).pipe(
      map(response => response.body)
    );
  }

  create(request: DeviceCreateDto): Observable<DeviceDto> {
    return create(this.http, this.apiConfiguration.rootUrl, {body: request}).pipe(
      map(response => response.body)
    );
  }

  update(deviceId: string, request: DeviceUpdateDto): Observable<DeviceDto> {
    return update(this.http, this.apiConfiguration.rootUrl, {
      deviceId,
      body: request
    }).pipe(
      map(response => response.body)
    );
  }

  delete(deviceId: string): Observable<void> {
    return delete$(this.http, this.apiConfiguration.rootUrl, {deviceId}).pipe(
      map(() => undefined)
    );
  }
}
