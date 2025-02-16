#ifndef NP_H
#define NP_H

#include "global_variables.h"
#include "hardware/pio.h"

struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
extern uint sm;
extern PIO np_pio;

int *get_np_symbol(possible_actions action);
void np_init(uint pin);
void np_write(const uint *symbol);

#endif