#ifndef OLED_H
#define OLED_H

#include <string.h>
#include <stdio.h>

#include "global_variables.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

typedef enum {
    M_WIFI_CONNECTING,
    M_WIFI_CONNECTED,
    M_WIFI_NOT_CONNECTED,
    M_BEFORE_TEST,
    M_PREPARE_3,
    M_PREPARE_2,
    M_PREPARE_1,
    M_ATTENTION_ALERT,
    M_FINAL,
    M_SENDING_DATA,
    M_DATA_WAS_SENT
} message_type;

// variáveis para setup de SSD1306
extern struct render_area ssd1306_frame_area;
extern uint8_t ssd[ssd1306_buffer_length];

void setup_oled();
void clear_display_ssd1306();
char (*get_message(message_type type))[17];
void write_in_oled(message_type mt, int lines_amount);
#endif