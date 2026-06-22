import {Injectable} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {Observable} from 'rxjs';
import {DeviceDto} from '../../api/models/device-dto';


@Injectable({
  providedIn: 'root'
})
export class DeviceApiService {

  private readonly baseUrl = '/api/v1/device';

  constructor(private http: HttpClient) {
  }

  findAll(): Observable<DeviceDto[]> {
    return this.http.get<DeviceDto[]>(this.baseUrl);
  }

  findByDeviceId(deviceId: string): Observable<DeviceDto> {
    return this.http.get<DeviceDto>(`${this.baseUrl}/${deviceId}`);
  }
}
