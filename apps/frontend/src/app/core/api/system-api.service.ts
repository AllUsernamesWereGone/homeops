import {Injectable} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {Observable} from 'rxjs';
import {SystemInfoDto} from '../../api/models/system-info-dto';
import {ApiConfiguration} from '../../api/api-configuration';
import {map} from 'rxjs/operators';
import {getSystemInfo} from '../../api/functions';


@Injectable({
  providedIn: 'root'
})
export class SystemApiService {


  constructor(
    private http: HttpClient,
    private apiConfiguration: ApiConfiguration
  ) {
  }

  getSystemInfo(): Observable<SystemInfoDto> {
    return getSystemInfo(this.http, this.apiConfiguration.rootUrl, {}).pipe(
      map(response => response.body)
    );
  }
}
