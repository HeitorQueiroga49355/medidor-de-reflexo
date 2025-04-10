#include <stdio.h>

#include "global_variables.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "np.h"
#include "oled.h"
#include "wifi.h"

// variáveis para setup de matriz de leds


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

void update_screen() {
    clear_display_ssd1306();
    switch (current_step) {
        case -1:
            write_in_oled(M_WIFI_CONNECTING, 8);
            break;
        case -2:
            write_in_oled(M_WIFI_CONNECTED, 8);
            break;
        case -3:
            write_in_oled(M_WIFI_NOT_CONNECTED, 8);
            break;
        case 0:
            write_in_oled(M_BEFORE_TEST, 3);
            break;
        case 1:
            write_in_oled(M_PREPARE_3, 3);
            break;
        case 2:
            write_in_oled(M_PREPARE_2, 3);
            break;
        case 3:
            write_in_oled(M_PREPARE_1, 3);
            break;
        case 4:
            write_in_oled(M_ATTENTION_ALERT, 3);
            break;
        case 5:
            write_in_oled(M_SENDING_DATA, 7);
            break;
        case 6:
            write_in_oled(M_DATA_WAS_SENT, 7);
            break;
        case 7:
            write_in_oled(M_FINAL, 7);
            break;
        default:
            break;
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
bool is_to_break = false;
uint32_t goal_time_ms = 0;

int64_t sleep_timer_callback(alarm_id_t id, void *user_data) {
    // Atualiza o tempo atual
    uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());

    // Verifica se deve interromper o sono
    if (is_the_test_going_on() && rounds[round_index].reaction_time >= 0) {
        return 0;  // Não reagendar o temporizador
    }

    handle_joystick_press();
    handle_button_actions_during_sleep(&is_to_break, &current_time_ms);
    last_pressed_button = pa_NULL;

    if (is_to_break || current_time_ms >= goal_time_ms) {
        return 0;  // Não reagendar o temporizador
    }

    // Reagenda o temporizador de acordo com o estado do teste
    if (!is_the_test_going_on()) {
        return 50 * 1000;  // 50 ms em microssegundos
    } else {
        return 1 * 1000;  // 1 ms em microssegundos
    }
}

void active_sleep_ms(uint32_t time_ms) {
    goal_time_ms = to_ms_since_boot(get_absolute_time()) + time_ms;
    is_to_break = false;

    // Inicia o temporizador
    add_alarm_in_us(0, sleep_timer_callback, NULL, true);

    // Aguarda até que a condição seja satisfeita
    while (!is_to_break &&
           to_ms_since_boot(get_absolute_time()) < goal_time_ms) {
        tight_loop_contents();  // Aguarda sem fazer nada
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

    active_sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0);

    active_sleep_ms(100);
}

void buzzer_alert() {
    pwm_init_buzzer(BUZZER, BUZZER_FREQUENCY);
    beep(BUZZER, BEEP_DURATION);
}

void start_test() {
    for (round_index = 0; round_index < ROUNDS_AMOUNT; round_index++) {
        is_reading_round_data = false;
        button_to_be_pressed = rounds[round_index].action;
        active_sleep_ms(rounds[round_index].sleep_time_before_start);
        if (is_to_reset) break;

        round_start_time = to_ms_since_boot(get_absolute_time());
        is_reading_round_data = true;
        np_write(get_np_symbol(rounds[round_index].action));
        buzzer_alert();
        active_sleep_ms(3000);
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
    char(*message)[17] = get_message(M_FINAL);

    snprintf(message[2], 17, "%15d", average_time);
    snprintf(message[4], 17, "%15d", lost_rounds);
}

int main() {
    setup();

    while (true) {
        is_to_reset = false;
        current_step = 0;
        update_screen();
        while (current_step == 0) active_sleep_ms(10000);
        if (is_to_reset) continue;
        srand(to_ms_since_boot(
            get_absolute_time()));  // Após o usuário clicar em iniciar o código
                                    // ira gerar uma nova semente de rand e
                                    // criar os dados da rodada
        setup_rounds_data();

        // Os três trechos abaixo se trata da contagem regressiva para início do
        // teste
        update_screen();
        active_sleep_ms(1000);
        if (is_to_reset) continue;
        current_step = 2;

        update_screen();
        active_sleep_ms(1000);
        if (is_to_reset) continue;
        current_step = 3;

        update_screen();
        active_sleep_ms(1000);
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
            send_data(average_time, lost_rounds);
            current_step = 6;

            update_screen();
            sleep_ms(2000);
        }
        current_step = 7;

        update_screen();
        while (current_step == 7) active_sleep_ms(10000);
    }
}