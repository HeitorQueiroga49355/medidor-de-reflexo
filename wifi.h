#ifndef WIFI_H
#define WIFI_H

#include "global_variables.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

// Variáveis relacionadas a wi-fi e thingSpeak
#define WIFI_SSID "brisa-63489"
#define WIFI_PASS "ud3henxz"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define API_KEY "TWSP5RIPE7LLEVJ2"  // Chave de escrita do ThingSpeak
#define WIFI_TIMEOUT_MS 10000

bool connect_with_timeout();

#endif