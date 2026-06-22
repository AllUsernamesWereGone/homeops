package dev.homeops.backend.config;

import io.swagger.v3.oas.models.OpenAPI;
import io.swagger.v3.oas.models.info.Info;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class OpenApiConfig {

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
