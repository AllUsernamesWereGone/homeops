package dev.homeops.backend.exception.exceptionhandler;

import dev.homeops.backend.dto.error.ApiErrorResponse;
import dev.homeops.backend.dto.error.ValidationErrorDto;
import dev.homeops.backend.exception.BadRequestException;
import dev.homeops.backend.exception.ConflictException;
import dev.homeops.backend.exception.NotFoundException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.ConstraintViolationException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.MDC;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.HttpStatusCode;
import org.springframework.http.ResponseEntity;
import org.springframework.web.ErrorResponseException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.web.context.request.ServletWebRequest;
import org.springframework.web.context.request.WebRequest;
import org.springframework.web.server.ResponseStatusException;
import org.springframework.web.servlet.mvc.method.annotation.ResponseEntityExceptionHandler;

import java.lang.invoke.MethodHandles;
import java.util.List;

@RestControllerAdvice
public class GlobalExceptionHandler extends ResponseEntityExceptionHandler {

    private static final Logger LOGGER = LoggerFactory.getLogger(MethodHandles.lookup().lookupClass());

    @ExceptionHandler(NotFoundException.class)
    public ResponseEntity<ApiErrorResponse> handleNotFound(
        NotFoundException ex,
        HttpServletRequest request
    ) {
        return buildResponse(HttpStatus.NOT_FOUND, ex.getMessage(), request);
    }

    @ExceptionHandler(BadRequestException.class)
    public ResponseEntity<ApiErrorResponse> handleBadRequest(
        BadRequestException ex,
        HttpServletRequest request
    ) {
        return buildResponse(HttpStatus.BAD_REQUEST, ex.getMessage(), request);
    }

    @ExceptionHandler(ConflictException.class)
    public ResponseEntity<ApiErrorResponse> handleConflict(
        ConflictException ex,
        HttpServletRequest request
    ) {
        return buildResponse(HttpStatus.CONFLICT, ex.getMessage(), request);
    }

    @ExceptionHandler(ResponseStatusException.class)
    public ResponseEntity<ApiErrorResponse> handleResponseStatusException(
        ResponseStatusException ex,
        HttpServletRequest request
    ) {
        HttpStatusCode statusCode = ex.getStatusCode();
        HttpStatus status = HttpStatus.resolve(statusCode.value());

        if (status == null) {
            status = HttpStatus.INTERNAL_SERVER_ERROR;
        }

        String message = ex.getReason() != null ? ex.getReason() : status.getReasonPhrase();
        return buildResponse(status, message, request);
    }

    @ExceptionHandler(ConstraintViolationException.class)
    public ResponseEntity<ApiErrorResponse> handleConstraintViolation(
        ConstraintViolationException ex,
        HttpServletRequest request
    ) {
        List<ValidationErrorDto> validationErrors = ex.getConstraintViolations()
            .stream()
            .map(violation -> new ValidationErrorDto(
                violation.getPropertyPath().toString(),
                violation.getMessage(),
                violation.getInvalidValue()
            ))
            .toList();

        ApiErrorResponse body = ApiErrorResponse.withValidationErrors(
            HttpStatus.BAD_REQUEST.value(),
            HttpStatus.BAD_REQUEST.name(),
            "Validation failed",
            request.getRequestURI(),
            getRequestId(),
            validationErrors
        );

        LOGGER.warn("Validation failed: {}", validationErrors);
        return ResponseEntity.badRequest().body(body);
    }

    @ExceptionHandler(DataIntegrityViolationException.class)
    public ResponseEntity<ApiErrorResponse> handleDataIntegrityViolation(
        DataIntegrityViolationException ex,
        HttpServletRequest request
    ) {
        LOGGER.warn("Database constraint violation", ex);

        return buildResponse(
            HttpStatus.CONFLICT,
            "The request conflicts with existing data",
            request
        );
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity<ApiErrorResponse> handleUnexpectedException(
        Exception ex,
        HttpServletRequest request
    ) {
        LOGGER.error("Unexpected exception", ex);

        return buildResponse(
            HttpStatus.INTERNAL_SERVER_ERROR,
            "An unexpected error occurred",
            request
        );
    }

    @Override
    protected ResponseEntity<Object> handleMethodArgumentNotValid(
        MethodArgumentNotValidException ex,
        HttpHeaders headers,
        HttpStatusCode status,
        WebRequest request
    ) {
        List<ValidationErrorDto> validationErrors = ex.getBindingResult()
            .getFieldErrors()
            .stream()
            .map(error -> new ValidationErrorDto(
                error.getField(),
                error.getDefaultMessage(),
                error.getRejectedValue()
            ))
            .toList();

        String path = getPath(request);

        ApiErrorResponse body = ApiErrorResponse.withValidationErrors(
            HttpStatus.BAD_REQUEST.value(),
            HttpStatus.BAD_REQUEST.name(),
            "Validation failed",
            path,
            getRequestId(),
            validationErrors
        );

        LOGGER.warn("Validation failed: {}", validationErrors);

        return new ResponseEntity<>(body, headers, HttpStatus.BAD_REQUEST);
    }

    @Override
    protected ResponseEntity<Object> handleErrorResponseException(
        ErrorResponseException ex,
        HttpHeaders headers,
        HttpStatusCode status,
        WebRequest request
    ) {
        HttpStatus httpStatus = HttpStatus.resolve(status.value());

        if (httpStatus == null) {
            httpStatus = HttpStatus.INTERNAL_SERVER_ERROR;
        }

        String message = ex.getBody().getDetail() != null
            ? ex.getBody().getDetail()
            : httpStatus.getReasonPhrase();

        ApiErrorResponse body = ApiErrorResponse.of(
            httpStatus.value(),
            httpStatus.name(),
            message,
            getPath(request),
            getRequestId()
        );

        return new ResponseEntity<>(body, headers, httpStatus);
    }

    private ResponseEntity<ApiErrorResponse> buildResponse(
        HttpStatus status,
        String message,
        HttpServletRequest request
    ) {
        if (status.is5xxServerError()) {
            LOGGER.error("{}: {}", status, message);
        } else {
            LOGGER.warn("{}: {}", status, message);
        }

        ApiErrorResponse body = ApiErrorResponse.of(
            status.value(),
            status.name(),
            message,
            request.getRequestURI(),
            getRequestId()
        );

        return ResponseEntity.status(status).body(body);
    }

    private String getRequestId() {
        String requestId = MDC.get("r");
        return requestId != null ? requestId : "unknown";
    }

    private String getPath(WebRequest request) {
        if (request instanceof ServletWebRequest servletWebRequest) {
            return servletWebRequest.getRequest().getRequestURI();
        }

        return "unknown";
    }
}
