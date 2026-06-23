import {ApplicationConfig} from '@angular/core';
import {provideRouter} from '@angular/router';
import {provideHttpClient} from '@angular/common/http';

import {routes} from './app.routes';
import {ApiConfiguration} from './api/api-configuration';

export function apiConfigurationFactory(): ApiConfiguration {
  const config = new ApiConfiguration();
  config.rootUrl = '';
  return config;
}

export const appConfig: ApplicationConfig = {
  providers: [
    provideRouter(routes),
    provideHttpClient(),
    {
      provide: ApiConfiguration,
      useFactory: apiConfigurationFactory
    }
  ]
};
