import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { SystemInfoDto } from '../dtos/system-info.dto';

@Injectable({
  providedIn: 'root'
})
export class SystemApiService {

  private readonly baseUrl = '/api/v1/system';

  constructor(private http: HttpClient) {
  }

  getSystemInfo(): Observable<SystemInfoDto> {
    return this.http.get<SystemInfoDto>(`${this.baseUrl}/info`);
  }
}
