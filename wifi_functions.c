#include "wifi.h"

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

