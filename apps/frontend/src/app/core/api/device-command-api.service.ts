import {Injectable} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {Observable} from 'rxjs';
import {map} from 'rxjs/operators';

import {ApiConfiguration} from '../../api/api-configuration';
import {publishCommand} from '../../api/functions';
import {DeviceCommandRequestDto} from '../../api/models/device-command-request-dto';
import {DeviceCommandResultDto} from '../../api/models/device-command-result-dto';
import {JsonNode} from '../../api/models/json-node';

@Injectable({
  providedIn: 'root'
})
export class DeviceCommandApiService {

  constructor(
    private http: HttpClient,
    private apiConfiguration: ApiConfiguration
  ) {
  }

  publishCommand(
    deviceId: string,
    request: DeviceCommandRequestDto
  ): Observable<DeviceCommandResultDto> {
    return publishCommand(
      this.http,
      this.apiConfiguration.rootUrl,
      {
        deviceId,
        body: request
      }
    ).pipe(
      map(response => response.body)
    );
  }

  toJsonNode(value: string | number | boolean | null | undefined): JsonNode {
    return value as unknown as JsonNode;
  }
}
