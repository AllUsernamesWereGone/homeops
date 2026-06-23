import {HttpErrorResponse} from '@angular/common/http';
import {ApiErrorResponse} from '../../api/models/api-error-response';
import {ValidationErrorDto} from '../../api/models/validation-error-dto';

export function getApiErrorResponse(error: unknown): ApiErrorResponse | null {
  if (!(error instanceof HttpErrorResponse)) {
    return null;
  }

  if (isApiErrorResponse(error.error)) {
    return error.error;
  }

  return null;
}

export function getApiErrorMessage(
  error: unknown,
  fallback = 'Something went wrong. Please try again.'
): string {
  if (error instanceof HttpErrorResponse) {
    if (error.status === 0) {
      return 'The backend is currently unavailable.';
    }

    const apiError = getApiErrorResponse(error);

    if (apiError?.message) {
      return apiError.message;
    }

    if (typeof error.error === 'string' && error.error.trim().length > 0) {
      return error.error;
    }

    if (error.message) {
      return error.message;
    }
  }

  return fallback;
}

export function getValidationErrors(error: unknown): ValidationErrorDto[] {
  return getApiErrorResponse(error)?.validationErrors ?? [];
}

export function getRequestId(error: unknown): string | null {
  if (!(error instanceof HttpErrorResponse)) {
    return null;
  }

  const apiError = getApiErrorResponse(error);

  if (apiError?.requestId) {
    return apiError.requestId;
  }

  return error.headers.get('X-Request-Id');
}

function isApiErrorResponse(value: unknown): value is ApiErrorResponse {
  if (!value || typeof value !== 'object') {
    return false;
  }

  const candidate = value as Partial<ApiErrorResponse>;

  return typeof candidate.status === 'number'
    && typeof candidate.error === 'string'
    && typeof candidate.message === 'string';
}
