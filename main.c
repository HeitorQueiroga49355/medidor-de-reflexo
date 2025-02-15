#include <stdio.h>
#include <string.h>

#include "global_variables.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "np.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "ws2818b.pio.h"

// Variáveis relacionadas a wi-fi e thingSpeak
#define WIFI_SSID "brisa-63489"
#define WIFI_PASS "ud3henxz"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define API_KEY "TWSP5RIPE7LLEVJ2"  // Chave de escrita do ThingSpeak
#define WIFI_TIMEOUT_MS 10000

struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;

// variáveis para setup de SSD1306
struct render_area ssd1306_frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};
// bool need_clear_screen = false;
uint8_t ssd[ssd1306_buffer_length];

// variáveis para setup de matriz de leds
struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
uint sm;

struct round_data {
    int sleep_time_before_start, reaction_time;
    possible_actions action;
};
typedef struct round_data round_data;
round_data
    rounds[ROUNDS_AMOUNT];  // Esse array conterá os dados dos testes, incluindo
                            // o botão a apertado, o tempo de espera antes de
                            // ser mostrado e o tempo de reação do usuário. Ele
                            // é resetado a cada teste.
possible_actions button_to_be_pressed = pa_NULL, last_pressed_button = pa_NULL;
int round_index = 0;
uint32_t round_start_time = 0;  // O delta entre ele e o tempo atual determinará
                                // o tempo de reação do usuário mais abaixo
bool is_reading_round_data =
    false;  // Indica se o já está disponível para o usuário responder dentre de
            // uma determinada rodada
uint32_t average_time;
int lost_rounds;

// variáveis gerais
short int current_step = -1;
volatile uint32_t last_interruption_time =
    0;  // Última vez que foi pressionado algum botão,
        // usado para debounce
bool is_to_reset =
    false;  // Caso o usuário queira resetar o teste durante a execução
bool is_online = false;

// textos a serem mostrados no display
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

// Função callback de interrupção de botões
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_interruption_time > DEBOUNCE_DELAY_MS_BTN) {
        if (gpio == BTN_A)
            last_pressed_button = b_A;
        else if (gpio == BTN_B)
            last_pressed_button = b_B;
        else if (gpio == JOYSTICK_BTN)
            last_pressed_button = JOY_B;
        last_interruption_time = current_time;
    }
}

// Variáveis do joystick
uint16_t vry_value, vrx_value;

// np = neopixel = matriz de leds
void np_init(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    PIO np_pio = pio0;

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

void setup_oled() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void setup_joystick() {
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);

    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true,
                                       &gpio_callback);
}

void setup_push_buttons() {
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true,
                                       &gpio_callback);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true,
                                       &gpio_callback);
}

int get_random_number(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }

    return (rand() % (max - min + 1)) + min;
}

// Aqui é criado o teste e apagado os dados do teste anterior
void setup_rounds_data() {
    for (int i = 0; i < ROUNDS_AMOUNT; i++) {
        rounds[i].sleep_time_before_start = get_random_number(500, 5000);
        rounds[i].action = get_random_number(first_action, last_action);
        rounds[i].reaction_time = -1;
    }
}

void clear_display_ssd1306() {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &ssd1306_frame_area);
}

void write_in_oled(char text[][17], int lines_amount) {
    int y = 0;
    for (uint i = 0; i < lines_amount * 17; i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &ssd1306_frame_area);
}

void update_screen() {
    clear_display_ssd1306();
    switch (current_step) {
        case -1:
            write_in_oled(m_wifi_connecting, 8);
            break;
        case -2:
            write_in_oled(m_wifi_connected, 8);
            break;
        case -3:
            write_in_oled(m_wifi_not_connected, 8);
            break;
        case 0:
            write_in_oled(m_before_test, 3);
            break;
        case 1:
            write_in_oled(m_prepare_3, 3);
            break;
        case 2:
            write_in_oled(m_prepare_2, 3);
            break;
        case 3:
            write_in_oled(m_prepare_1, 3);
            break;
        case 4:
            write_in_oled(m_attention_alert, 3);
            break;
        case 5:
            write_in_oled(m_sending_data, 7);
            break;
        case 6:
            write_in_oled(m_data_was_sent, 7);
            break;
        case 7:
            write_in_oled(m_final, 7);
            break;
        default:
            break;
    }
}

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

void connect_internet() {
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        current_step = -3;
        update_screen();
        sleep_ms(3000);
    } else {
        cyw43_arch_enable_sta_mode();
        printf("Conectando ao Wi-Fi...\n");

        if (!connect_with_timeout()) {
            printf("Falha ao conectar ao Wi-Fi\n");
            current_step = -3;
            update_screen();
            sleep_ms(5000);
        } else {
            current_step = -2;
            update_screen();
            sleep_ms(5000);
            is_online = true;
            printf("Wi-Fi conectado!\n");
        }
    }
}

void setup() {
    stdio_init_all();

    setup_oled();
    ssd1306_init();
    calculate_render_area_buffer_length(&ssd1306_frame_area);

    setup_push_buttons();
    setup_joystick();

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);

    np_init(NP_PIN);

    setup_rounds_data();
    np_write(get_np_symbol(10));

    update_screen();

    connect_internet();
}

// Função de debug - Não é utilizada na versão final do sistema
void print_round_data() {
    for (int i = 0; i < ROUNDS_AMOUNT; i++) {
        printf("%d - %d - %d\n", rounds[i].reaction_time, rounds[i].action,
               rounds[i].sleep_time_before_start);
    }
    printf("\n");
}

bool is_the_test_going_on() { return current_step == 4; }

bool is_in_waiting_screens() { return current_step == 0 || current_step == 7; }

void joystick_read_axis() {
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    vry_value = adc_read();

    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    vrx_value = adc_read();
}

void handle_joystick_press() {
    joystick_read_axis();
    if (vry_value > 3800)
        last_pressed_button = UP;
    else if (vry_value < 200)
        last_pressed_button = DOWN;
    else if (vrx_value > 3800)
        last_pressed_button = RIGHT;
    else if (vrx_value < 200)
        last_pressed_button = LEFT;
}

void handle_button_actions_during_sleep(bool *is_to_break,
                                        uint32_t *current_time) {
    if (last_pressed_button != pa_NULL) {
        if (last_pressed_button == JOY_B) {
            current_step = 0;
            last_pressed_button = pa_NULL;
            is_to_reset = true;
            *is_to_break = true;
            return;
        }
        if (is_the_test_going_on() && is_reading_round_data) {
            if (last_pressed_button == button_to_be_pressed) {
                rounds[round_index].reaction_time =
                    *current_time - round_start_time;

                last_pressed_button = pa_NULL;
                *is_to_break = true;
            }
            last_pressed_button = pa_NULL;
        }
        if (is_in_waiting_screens() && last_pressed_button == b_A) {
            last_pressed_button = pa_NULL;

            if (current_step == 0)
                current_step = 1;
            else
                current_step = 0;
            *is_to_break = true;
        }
    }
}

void sleep_with_break(uint32_t time) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t goal_time = current_time + time;
    bool is_to_break = false;
    while (current_time < goal_time) {
        if (is_the_test_going_on() &&
            rounds[round_index].reaction_time >=
                0)  // Essa condição é necessária pois, caso o usuário tenha
                    // clicado corretamente enquanto o buzzer estava tocando,
                    // não precisa ser chamado outro temporizador
            break;

        handle_joystick_press();
        handle_button_actions_during_sleep(&is_to_break, &current_time);
        last_pressed_button = pa_NULL;
        if (is_to_break) break;
        current_time = to_ms_since_boot(get_absolute_time());

        // Caso não esteja durante a execução do teste, o tempo de espera é
        // maior, para economia de processamento
        if (!is_the_test_going_on())
            sleep_ms(50);
        else
            sleep_ms(1);
    }
}

void pwm_init_buzzer(uint pin, int FREQ_BUZZER) {
    gpio_set_function(pin, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(pin);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config,
                          clock_get_hz(clk_sys) / (FREQ_BUZZER * 28672));
    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(pin, 0);
}

void beep(uint pin, float duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);

    pwm_set_gpio_level(pin, 2048);

    sleep_with_break(duration_ms);

    pwm_set_gpio_level(pin, 0);

    sleep_with_break(100);
}

void buzzer_alert() {
    pwm_init_buzzer(BUZZER, BUZZER_FREQUENCY);
    beep(BUZZER, BEEP_DURATION);
}

void start_test() {
    for (round_index = 0; round_index < ROUNDS_AMOUNT; round_index++) {
        is_reading_round_data = false;
        button_to_be_pressed = rounds[round_index].action;
        sleep_with_break(rounds[round_index].sleep_time_before_start);
        if (is_to_reset) break;

        round_start_time = to_ms_since_boot(get_absolute_time());
        is_reading_round_data = true;
        np_write(get_np_symbol(rounds[round_index].action));
        buzzer_alert();
        sleep_with_break(3000);
        np_write(get_np_symbol(10));

        if (is_to_reset) break;
    }
}

void calculate_average_reaction_time() {
    lost_rounds = 0;
    average_time = 0;
    for (int i = 0; i < ROUNDS_AMOUNT; i++)
        if (rounds[i].reaction_time == -1) lost_rounds++;
    int valid_rounds_amount = ROUNDS_AMOUNT - lost_rounds;
    for (int i = 0; i < valid_rounds_amount; i++) {
        if (rounds[i].reaction_time != -1)
            average_time += rounds[i].reaction_time / valid_rounds_amount;
    }
}

void create_final_message() {
    snprintf(m_final[2], 17, "%15d", average_time);
    snprintf(m_final[4], 17, "%15d", lost_rounds);
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
        API_KEY, average_time, lost_rounds);

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

void upload_data() {
    dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
}

int main() {
    setup();

    while (true) {
        is_to_reset = false;
        current_step = 0;
        update_screen();
        while (current_step == 0) sleep_with_break(10000);
        if (is_to_reset) continue;
        srand(to_ms_since_boot(
            get_absolute_time()));  // Após o usuário clicar em iniciar o código
                                    // ira gerar uma nova semente de rand e
                                    // criar os dados da rodada
        setup_rounds_data();

        // Os três trechos abaixo se trata da contagem regressiva para início do
        // teste
        update_screen();
        sleep_with_break(1000);
        if (is_to_reset) continue;
        current_step = 2;

        update_screen();
        sleep_with_break(1000);
        if (is_to_reset) continue;
        current_step = 3;

        update_screen();
        sleep_with_break(1000);
        if (is_to_reset) continue;
        current_step = 4;

        // Nesse trecho é chamado a função de iniciar o teste
        update_screen();
        start_test();
        if (is_to_reset) continue;
        calculate_average_reaction_time();
        create_final_message();

        if (is_online) {
            current_step = 5;
            update_screen();
            sleep_ms(1000);
            upload_data();
            current_step = 6;

            update_screen();
            sleep_ms(2000);
        }
        current_step = 7;

        update_screen();
        while (current_step == 7) sleep_with_break(10000);
    }
}