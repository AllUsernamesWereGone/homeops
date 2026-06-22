package dev.homeops.backend.config;

import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.MDC;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;

import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.util.Optional;
import java.util.UUID;

@Component
public class RequestLoggingFilter extends OncePerRequestFilter {

    private static final Logger LOGGER = LoggerFactory.getLogger(MethodHandles.lookup().lookupClass());

    private static final String MDC_REQUEST_ID = "r";
    private static final String MDC_IP = "ip";
    private static final String MDC_USER = "u";

    @Override
    protected void doFilterInternal(
        HttpServletRequest request,
        HttpServletResponse response,
        FilterChain filterChain
    ) throws ServletException, IOException {
        long start = System.currentTimeMillis();

        String requestId = getOrCreateRequestId(request);
        String clientIp = getClientIp(request);
        String user = getUser(request);

        MDC.put(MDC_REQUEST_ID, requestId);
        MDC.put(MDC_IP, clientIp);
        MDC.put(MDC_USER, user);

        String method = request.getMethod();
        String path = getRequestPath(request);

        LOGGER.info(">>> {} {}", method, path);

        try {
            response.setHeader("X-Request-Id", requestId);
            filterChain.doFilter(request, response);
        } finally {
            long durationMs = System.currentTimeMillis() - start;

            LOGGER.info(
                "<<< {} {} status={} time={}ms",
                method,
                path,
                response.getStatus(),
                durationMs
            );

            MDC.clear();
        }
    }

    private String getOrCreateRequestId(HttpServletRequest request) {
        return Optional.ofNullable(request.getHeader("X-Request-Id"))
            .filter(value -> !value.isBlank())
            .orElse(UUID.randomUUID().toString().substring(0, 8));
    }

    private String getClientIp(HttpServletRequest request) {
        String forwardedFor = request.getHeader("X-Forwarded-For");

        if (forwardedFor != null && !forwardedFor.isBlank()) {
            return forwardedFor.split(",")[0].trim();
        }

        String realIp = request.getHeader("X-Real-IP");

        if (realIp != null && !realIp.isBlank()) {
            return realIp;
        }

        return request.getRemoteAddr();
    }

    private String getUser(HttpServletRequest request) {
        if (request.getUserPrincipal() == null) {
            return "-";
        }

        return request.getUserPrincipal().getName();
    }

    private String getRequestPath(HttpServletRequest request) {
        String uri = request.getRequestURI();
        String query = request.getQueryString();

        if (query == null || query.isBlank()) {
            return uri;
        }

        return uri + "?" + query;
    }
}
