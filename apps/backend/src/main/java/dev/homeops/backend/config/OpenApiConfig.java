package dev.homeops.backend.config;

import dev.homeops.backend.dto.error.ApiErrorResponse;
import dev.homeops.backend.dto.error.ValidationErrorDto;
import io.swagger.v3.core.converter.ModelConverters;
import io.swagger.v3.oas.models.Components;
import io.swagger.v3.oas.models.OpenAPI;
import io.swagger.v3.oas.models.info.Info;
import io.swagger.v3.oas.models.media.Content;
import io.swagger.v3.oas.models.media.MediaType;
import io.swagger.v3.oas.models.responses.ApiResponse;
import org.springdoc.core.customizers.OpenApiCustomizer;
import org.springdoc.core.customizers.OperationCustomizer;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;


@Configuration
public class OpenApiConfig {

    private static final String ERROR_RESPONSE_REF = "#/components/schemas/ApiErrorResponse";

    @Bean
    public OpenApiCustomizer errorSchemaCustomizer() {
        return openApi -> {
            if (openApi.getComponents() == null) {
                openApi.setComponents(new Components());
            }

            ModelConverters.getInstance()
                .read(ApiErrorResponse.class)
                .forEach(openApi.getComponents()::addSchemas);

            ModelConverters.getInstance()
                .read(ValidationErrorDto.class)
                .forEach(openApi.getComponents()::addSchemas);
        };
    }

    @Bean
    public OperationCustomizer commonErrorResponsesCustomizer() {
        return (operation, handlerMethod) -> {
            operation.getResponses().addApiResponse(
                "400",
                errorResponse("Bad request")
            );
            operation.getResponses().addApiResponse(
                "401",
                errorResponse("Unauthorized")
            );
            operation.getResponses().addApiResponse(
                "403",
                errorResponse("Forbidden")
            );
            operation.getResponses().addApiResponse(
                "404",
                errorResponse("Not found")
            );
            operation.getResponses().addApiResponse(
                "409",
                errorResponse("Conflict")
            );
            operation.getResponses().addApiResponse(
                "500",
                errorResponse("Internal server error")
            );

            return operation;
        };
    }

    private ApiResponse errorResponse(String description) {
        return new ApiResponse()
            .description(description)
            .content(new Content().addMediaType(
                "application/json",
                new MediaType().schema(
                    new io.swagger.v3.oas.models.media.Schema<>()
                        .$ref(ERROR_RESPONSE_REF)
                )
            ));
    }

    @Bean
    public OpenAPI homeOpsOpenApi(
        @Value("${spring.application.name:homeops-backend}") String applicationName,
        @Value("${app.version:0.0.1-SNAPSHOT}") String version
    ) {
        return new OpenAPI()
            .info(new Info()
                .title("HomeOps API")
                .version(version)
                .description("Backend API for the HomeOps home lab."));
    }
}
