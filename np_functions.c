#include "np.h"
#include "global_variables.h"
#include "ws2818b.pio.h"

uint sm;
PIO np_pio;

int *get_np_symbol(possible_actions action) {
  static int symbol[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  switch (action) {
      case b_A:
          static int symbol_A[] = {NPI, 0,   0,   0,   NPI, NPI, 0, 0,   0,
                                   NPI, NPI, NPI, NPI, NPI, NPI, 0, NPI, 0,
                                   NPI, 0,   0,   0,   NPI, 0,   0};
          return symbol_A;

      case b_B:
          static int symbol_B[] = {0,   NPI, NPI, NPI, NPI, NPI, 0,   0, NPI,
                                   0,   0,   0,   NPI, NPI, NPI, NPI, 0, 0,
                                   NPI, 0,   0,   NPI, NPI, NPI, NPI};
          return symbol_B;

      case UP:
          static int symbol_UP[] = {0,   0,   NPI, 0,   0,   0,   0, NPI, 0,
                                    0,   NPI, 0,   NPI, 0,   NPI, 0, NPI, NPI,
                                    NPI, 0,   0,   0,   NPI, 0,   0};
          return symbol_UP;

      case RIGHT:
          static int symbol_RIGHT[] = {
              0,   0,   NPI, 0, 0, 0,   0, 0, NPI, 0,   NPI, NPI, NPI,
              NPI, NPI, 0,   0, 0, NPI, 0, 0, 0,   NPI, 0,   0};
          return symbol_RIGHT;

      case DOWN:
          static int symbol_DOWN[] = {
              0, 0,   NPI, 0, 0,   0, NPI, NPI, NPI, 0,   NPI, 0, NPI,
              0, NPI, 0,   0, NPI, 0, 0,   0,   0,   NPI, 0,   0};
          return symbol_DOWN;

      case LEFT:
          static int symbol_LEFT[] = {0, 0,   NPI, 0,   0,   0,   NPI, 0,   0,
                                      0, NPI, NPI, NPI, NPI, NPI, 0,   NPI, 0,
                                      0, 0,   0,   0,   NPI, 0,   0};
          return symbol_LEFT;

      default:
          return symbol;  // Retorna o array default se a ação não for
                          // reconhecida
  }
}

// np = neopixel = matriz de leds
void np_init(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
}

void np_write(const uint *symbol) {
    for (uint i = 0; i < NP_LED_COUNT; i++) {
        pio_sm_put_blocking(np_pio, sm, symbol[i]);
        pio_sm_put_blocking(np_pio, sm, symbol[i]);
        pio_sm_put_blocking(np_pio, sm, symbol[i]);
    }

    sleep_us(100);  // Espera 100us, sinal de RESET do datasheet.
}