package dev.homeops.backend.dto.error;

public record ValidationErrorDto(
    String field,
    String message,
    Object rejectedValue
) {
}
