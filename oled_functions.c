#include <oled.h>

struct render_area ssd1306_frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};
uint8_t ssd[ssd1306_buffer_length];

// Declaração das matrizes de mensagens
char m_wifi_connecting[8][17] = {
    "   CONECTANDO   ", "                ", "     WI-FI      ",
    "                ", "                ", "                ",
    "                ", "                ",
};
char m_wifi_connected[8][17] = {
    "    ONLINE!     ", "                ", "                ",
    "                ", "                ", "                ",
    "                ", "                ",
};
char m_wifi_not_connected[8][17] = {
    "WI-FI N\xc3O CO-   ", "                ", "NECTADO         ",
    "                ",    "TESTE OFFLINE   ", "                ",
    "                ",    "                ",
};
char m_before_test[3][17] = {
    "  INICIAR TESTE ",
    "                ",
    "APERTE BOT\xc3O A",
};
char m_prepare_3[3][17] = {
    "                ",
    "                ",
    " INICIANDO EM 3 ",
};
char m_prepare_2[3][17] = {
    "                ",
    "                ",
    " INICIANDO EM 2 ",
};
char m_prepare_1[3][17] = {
    "                ",
    "                ",
    " INICIANDO EM 1 ",
};
char m_attention_alert[3][17] = {
    "                ",
    "                ",
    "    ATEN\xe7\xc3O!    ",
};
char m_final[7][17] = {
    "   TEMPO MEDIO  ", "                ", "                ",
    "     PERDAS     ", "                ", "PRESSIONE A     ",
    "                ",
};
char m_sending_data[7][17] = {
    "    ENVIANDO    ", "                ", "    DADOS...    ",
    "                ", "                ", "                ",
    "                ",
};
char m_data_was_sent[7][17] = {
    "     DADOS      ", "                ", "    ENVIADOS    ",
    "                ", "                ", "                ",
    "                ",
};

// Enumeração para os diferentes tipos de mensagens

// Função que retorna o ponteiro para a matriz de mensagens
char (*get_message(message_type type))[17] {
    switch (type) {
        case M_WIFI_CONNECTING:
            return m_wifi_connecting;
        case M_WIFI_CONNECTED:
            return m_wifi_connected;
        case M_WIFI_NOT_CONNECTED:
            return m_wifi_not_connected;
        case M_BEFORE_TEST:
            return m_before_test;
        case M_PREPARE_3:
            return m_prepare_3;
        case M_PREPARE_2:
            return m_prepare_2;
        case M_PREPARE_1:
            return m_prepare_1;
        case M_ATTENTION_ALERT:
            return m_attention_alert;
        case M_FINAL:
            return m_final;
        case M_SENDING_DATA:
            return m_sending_data;
        case M_DATA_WAS_SENT:
            return m_data_was_sent;
        default:
            return NULL;
    }
}

void setup_oled() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void write_in_oled(message_type mt, int lines_amount) {
    char(*text)[17] = get_message(mt);
    int y = 0;
    for (uint i = 0; i < lines_amount * 17; i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &ssd1306_frame_area);
}

void clear_display_ssd1306() {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &ssd1306_frame_area);
}