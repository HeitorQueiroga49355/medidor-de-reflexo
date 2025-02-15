#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#define NP_LED_COUNT 25
#define NP_PIN 7
#define LEDR 13
#define LEDG 11
#define LEDB 12
#define BTN_A 5
#define BTN_B 6
#define JOYSTICK_BTN 22
#define DEBOUNCE_DELAY_MS_BTN 300
#define VRX 26
#define VRY 27
#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1
#define BUZZER 21
#define LED_STEP 100
#define DIVIDER_PWM 16.0
#define PERIOD 4096
#define ROUNDS_AMOUNT 5
#define NPI 1000000
#define BUZZER_FREQUENCY 2000
#define BEEP_DURATION 300
#define I2C_SDA 14
#define I2C_SCL 15

typedef enum {
    pa_NULL = -1,
    b_A,
    b_B,
    UP,
    RIGHT,
    DOWN,
    LEFT,
    JOY_B,
    first_action = b_A,
    last_action = LEFT
} possible_actions;

#endif