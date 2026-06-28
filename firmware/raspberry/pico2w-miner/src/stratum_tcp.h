#ifndef STRATUM_TCP_H
#define STRATUM_TCP_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*stratum_tcp_line_callback_t)(const char *line);

bool stratum_tcp_connect_blocking(const char *host, uint16_t port, uint32_t timeout_ms);
bool stratum_tcp_is_connected(void);

void stratum_tcp_set_line_callback(stratum_tcp_line_callback_t callback);

bool stratum_tcp_send_line(const char *line);
bool stratum_tcp_send_json_line(const char *json);

void stratum_tcp_close(void);

#endif