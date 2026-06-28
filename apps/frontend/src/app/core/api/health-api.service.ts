import {Injectable} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {Observable} from 'rxjs';
import {ApiConfiguration} from '../../api/api-configuration';
import {map} from 'rxjs/operators';
import {getHealth} from '../../api/functions';


@Injectable({
  providedIn: 'root'
})
export class HealthApiService {


  constructor(
    private http: HttpClient,
    private apiConfiguration: ApiConfiguration
  ) {
  }

  getHealth(): Observable<string> {
    return getHealth(this.http, this.apiConfiguration.rootUrl, {}).pipe(
      map(response => response.body?.trim() ?? '')
    );
  }
}
