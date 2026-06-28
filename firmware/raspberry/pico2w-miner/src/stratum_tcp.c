#include "stratum_tcp.h"

#include <stdio.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define STRATUM_TCP_LINE_BUFFER_SIZE 8192
#define STRATUM_TCP_RX_CHUNK_SIZE 128

typedef enum {
    STRATUM_TCP_IDLE = 0,
    STRATUM_TCP_DNS_WAIT,
    STRATUM_TCP_CONNECTING,
    STRATUM_TCP_CONNECTED,
    STRATUM_TCP_FAILED
} stratum_tcp_state_t;

static struct tcp_pcb *tcp_client_pcb = NULL;
static stratum_tcp_state_t tcp_state = STRATUM_TCP_IDLE;

static ip_addr_t resolved_ip;
static uint16_t target_port = 0;

static char line_buffer[STRATUM_TCP_LINE_BUFFER_SIZE];
static size_t line_buffer_len = 0;
static bool dropping_oversized_line = false;

static stratum_tcp_line_callback_t line_callback = NULL;

static void stratum_tcp_reset_line_buffer(void) {
    line_buffer_len = 0;
    line_buffer[0] = '\0';
    dropping_oversized_line = false;
}

static void stratum_tcp_close_raw(void) {
    if (tcp_client_pcb != NULL) {
        tcp_arg(tcp_client_pcb, NULL);
        tcp_err(tcp_client_pcb, NULL);
        tcp_recv(tcp_client_pcb, NULL);

        err_t err = tcp_close(tcp_client_pcb);
        if (err != ERR_OK) {
            tcp_abort(tcp_client_pcb);
        }

        tcp_client_pcb = NULL;
    }
}

static void stratum_tcp_fail_raw(const char *reason) {
    printf("TCP: failed: %s\n", reason);
    stratum_tcp_close_raw();
    stratum_tcp_reset_line_buffer();
    tcp_state = STRATUM_TCP_FAILED;
}

static void stratum_tcp_process_received_byte(char c) {
    if (c == '\r') {
        return;
    }

    if (dropping_oversized_line) {
        if (c == '\n') {
            printf("TCP: finished dropping oversized line\n");
            stratum_tcp_reset_line_buffer();
        }

        return;
    }

    if (c == '\n') {
        line_buffer[line_buffer_len] = '\0';

        if (line_buffer_len > 0) {
            printf("STRATUM RX: %s\n", line_buffer);

            if (line_callback != NULL) {
                line_callback(line_buffer);
            }
        }

        stratum_tcp_reset_line_buffer();
        return;
    }

    if (line_buffer_len >= STRATUM_TCP_LINE_BUFFER_SIZE - 1) {
        printf("TCP: received line too long, dropping until newline\n");
        line_buffer_len = 0;
        line_buffer[0] = '\0';
        dropping_oversized_line = true;
        return;
    }

    line_buffer[line_buffer_len++] = c;
}

static void stratum_tcp_process_received_bytes(const char *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        stratum_tcp_process_received_byte(data[i]);
    }
}

static void stratum_tcp_error_callback(void *arg, err_t err) {
    (void)arg;

    printf("TCP: error callback: %d\n", err);

    tcp_client_pcb = NULL;
    stratum_tcp_reset_line_buffer();
    tcp_state = STRATUM_TCP_FAILED;
}

static err_t stratum_tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;

    if (p == NULL) {
        printf("TCP: server closed connection\n");

        tcp_client_pcb = NULL;
        stratum_tcp_reset_line_buffer();
        tcp_state = STRATUM_TCP_FAILED;

        tcp_close(tpcb);
        return ERR_OK;
    }

    if (err == ERR_OK) {
        printf("TCP: received %u bytes\n", p->tot_len);

        char rx_chunk[STRATUM_TCP_RX_CHUNK_SIZE];

        uint16_t offset = 0;
        while (offset < p->tot_len) {
            uint16_t remaining = p->tot_len - offset;
            uint16_t to_copy = remaining;

            if (to_copy > sizeof(rx_chunk)) {
                to_copy = sizeof(rx_chunk);
            }

            uint16_t copied = pbuf_copy_partial(p, rx_chunk, to_copy, offset);
            if (copied == 0) {
                break;
            }

            stratum_tcp_process_received_bytes(rx_chunk, copied);
            offset += copied;
        }

        tcp_recved(tpcb, p->tot_len);
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t stratum_tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)arg;

    if (err != ERR_OK) {
        printf("TCP: connect callback failed: %d\n", err);
        tcp_state = STRATUM_TCP_FAILED;
        return err;
    }

    printf("TCP: connected to pool\n");

    tcp_client_pcb = tpcb;
    tcp_state = STRATUM_TCP_CONNECTED;

    stratum_tcp_reset_line_buffer();

    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, stratum_tcp_recv_callback);
    tcp_err(tpcb, stratum_tcp_error_callback);

    return ERR_OK;
}

static void stratum_tcp_start_connect_to_ip(void) {
    tcp_client_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);

    if (tcp_client_pcb == NULL) {
        stratum_tcp_fail_raw("tcp_new failed");
        return;
    }

    tcp_arg(tcp_client_pcb, NULL);
    tcp_err(tcp_client_pcb, stratum_tcp_error_callback);

    printf("TCP: connecting to %s:%u\n", ipaddr_ntoa(&resolved_ip), target_port);

    tcp_state = STRATUM_TCP_CONNECTING;

    err_t err = tcp_connect(
        tcp_client_pcb,
        &resolved_ip,
        target_port,
        stratum_tcp_connected_callback
    );

    if (err != ERR_OK) {
        printf("TCP: tcp_connect returned error %d\n", err);
        stratum_tcp_fail_raw("tcp_connect failed");
    }
}

static void stratum_tcp_dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
    (void)arg;

    if (ipaddr == NULL) {
        printf("DNS: failed for %s\n", name);
        tcp_state = STRATUM_TCP_FAILED;
        return;
    }

    resolved_ip = *ipaddr;

    printf("DNS: %s -> %s\n", name, ipaddr_ntoa(&resolved_ip));

    stratum_tcp_start_connect_to_ip();
}

bool stratum_tcp_connect_blocking(const char *host, uint16_t port, uint32_t timeout_ms) {
    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);

    tcp_state = STRATUM_TCP_IDLE;
    target_port = port;

    memset(&resolved_ip, 0, sizeof(resolved_ip));
    stratum_tcp_reset_line_buffer();

    printf("DNS: resolving %s\n", host);

    cyw43_arch_lwip_begin();

    err_t dns_result = dns_gethostbyname(
        host,
        &resolved_ip,
        stratum_tcp_dns_callback,
        NULL
    );

    if (dns_result == ERR_OK) {
        printf("DNS: immediate result %s -> %s\n", host, ipaddr_ntoa(&resolved_ip));
        stratum_tcp_start_connect_to_ip();
    } else if (dns_result == ERR_INPROGRESS) {
        tcp_state = STRATUM_TCP_DNS_WAIT;
    } else {
        printf("DNS: dns_gethostbyname failed: %d\n", dns_result);
        tcp_state = STRATUM_TCP_FAILED;
    }

    cyw43_arch_lwip_end();

    while (!time_reached(timeout_time)) {
        if (tcp_state == STRATUM_TCP_CONNECTED) {
            return true;
        }

        if (tcp_state == STRATUM_TCP_FAILED) {
            return false;
        }

        sleep_ms(50);
    }

    printf("TCP: connection timed out\n");
    stratum_tcp_close();

    return false;
}

bool stratum_tcp_is_connected(void) {
    return tcp_state == STRATUM_TCP_CONNECTED;
}

void stratum_tcp_set_line_callback(stratum_tcp_line_callback_t callback) {
    line_callback = callback;
}

bool stratum_tcp_send_line(const char *line) {
    if (line == NULL) {
        printf("TCP: cannot send NULL line\n");
        return false;
    }

    if (tcp_client_pcb == NULL || tcp_state != STRATUM_TCP_CONNECTED) {
        printf("TCP: cannot send, not connected\n");
        return false;
    }

    size_t len = strlen(line);

    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        len--;
    }

    cyw43_arch_lwip_begin();

    if (tcp_sndbuf(tcp_client_pcb) < len + 1) {
        cyw43_arch_lwip_end();
        printf("TCP: send buffer too small\n");
        return false;
    }

    err_t err = tcp_write(tcp_client_pcb, line, len, TCP_WRITE_FLAG_COPY);

    if (err == ERR_OK) {
        err = tcp_write(tcp_client_pcb, "\n", 1, TCP_WRITE_FLAG_COPY);
    }

    if (err == ERR_OK) {
        err = tcp_output(tcp_client_pcb);
    }

    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("TCP: send failed: %d\n", err);
        return false;
    }

    printf("TCP: sent: %.*s\n", (int)len, line);
    return true;
}

bool stratum_tcp_send_json_line(const char *json) {
    return stratum_tcp_send_line(json);
}

void stratum_tcp_close(void) {
    cyw43_arch_lwip_begin();

    stratum_tcp_close_raw();

    tcp_state = STRATUM_TCP_IDLE;
    stratum_tcp_reset_line_buffer();

    cyw43_arch_lwip_end();

    printf("TCP: closed\n");
}