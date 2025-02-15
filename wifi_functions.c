#include "wifi.h"

uint32_t g_average_time;
int g_lost_rounds;
struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;

bool connect_with_timeout() {
    int result = cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASS,
                                               CYW43_AUTH_WPA2_MIXED_PSK);
    if (result != 0) {
        printf("Failed to start connection: %d\n", result);
        return false;
    }

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (true) {
        if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) ==
            CYW43_LINK_UP)
            return true;

        uint32_t elapsed_time =
            to_ms_since_boot(get_absolute_time()) - start_time;
        if (elapsed_time >= WIFI_TIMEOUT_MS) return false;

        sleep_ms(100);
    }
}

// Callback quando recebe resposta do ThingSpeak
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                                err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb,
                                     err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP\n");
        return err;
    }

    printf("Conectado ao ThingSpeak!\n");

    char request[256];

    snprintf(
        request, sizeof(request),
        "GET "
        "https://api.thingspeak.com/update?api_key=%s&field1=%d&field2=%d\r\n",
        API_KEY, g_average_time, g_lost_rounds);

    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, http_recv_callback);

    return ERR_OK;
}

static void dns_callback(const char *name, const ip_addr_t *ipaddr,
                         void *callback_arg) {
    if (ipaddr) {
        printf("Endereço IP do ThingSpeak: %s\n", ipaddr_ntoa(ipaddr));
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT,
                    http_connected_callback);
    } else {
        printf("Falha na resolução de DNS\n");
    }
}

void send_data(uint32_t average_time, int lost_rounds) {
    g_average_time = average_time;
    g_lost_rounds = lost_rounds;
    dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
}
