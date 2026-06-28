#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

bool wifi_connect_to_first_available(void);

const char *wifi_get_connected_ssid(void);

#endif