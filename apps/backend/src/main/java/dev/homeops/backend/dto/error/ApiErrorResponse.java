package dev.homeops.backend.dto.error;

import com.fasterxml.jackson.annotation.JsonInclude;

import java.time.Instant;
import java.util.List;

@JsonInclude(JsonInclude.Include.NON_EMPTY)
public record ApiErrorResponse(
    Instant timestamp,
    int status,
    String error,
    String message,
    String path,
    String requestId,
    List<ValidationErrorDto> validationErrors
) {
    public static ApiErrorResponse of(
        int status,
        String error,
        String message,
        String path,
        String requestId
    ) {
        return new ApiErrorResponse(
            Instant.now(),
            status,
            error,
            message,
            path,
            requestId,
            null
        );
    }

    public static ApiErrorResponse withValidationErrors(
        int status,
        String error,
        String message,
        String path,
        String requestId,
        List<ValidationErrorDto> validationErrors
    ) {
        return new ApiErrorResponse(
            Instant.now(),
            status,
            error,
            message,
            path,
            requestId,
            validationErrors
        );
    }
}
