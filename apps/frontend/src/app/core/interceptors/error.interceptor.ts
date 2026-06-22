import {HttpErrorResponse, HttpInterceptorFn} from '@angular/common/http';
import {catchError, throwError} from 'rxjs';
import {getApiErrorMessage, getRequestId} from '../../shared/utils/api-error.util';

const REQUEST_ID_HEADER = 'X-Request-Id';

export const errorInterceptor: HttpInterceptorFn = (request, next) => {
  const requestId = createRequestId();

  const requestWithId = request.clone({
    setHeaders: {
      [REQUEST_ID_HEADER]: requestId,
    },
  });

  return next(requestWithId).pipe(
    catchError((error: HttpErrorResponse) => {
      const message = getApiErrorMessage(error);
      const effectiveRequestId = getRequestId(error) ?? requestId;

      if (error.status === 0) {
        console.error(`[${effectiveRequestId}] Backend unavailable`, error);
      } else if (error.status >= 500) {
        console.error(`[${effectiveRequestId}] Server error: ${message}`, error);
      } else {
        console.warn(`[${effectiveRequestId}] Request failed: ${message}`, error);
      }

      return throwError(() => error);
    })
  );
};

function createRequestId(): string {
  if (typeof crypto !== 'undefined' && 'randomUUID' in crypto) {
    return crypto.randomUUID();
  }

  return Math.random().toString(36).slice(2) + Date.now().toString(36);
}
