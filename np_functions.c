#include "np.h"

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