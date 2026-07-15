#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"
#include "board.h"
#include "logging/logging.h"
#include "config.h"

#define GPIO_POWER 4
#define GPIO_QUAD_A 22
#define GPIO_QUAD_B 26
#define GPIO_QUAD_BTN 27
#define GPIO_BACK_BTN 28
#define GPIO_LED 25

#define BUTTON_DEBOUNCE_COUNT 50000
#define PMEMTEST_OVERCLOCK_KHZ 300000

void board_pre_init() {
    // Increase core voltage slightly (default is 1.10V) to better handle overclock
    ULOG_INFO("Setting Core Voltage to 1.2V...");
    vreg_set_voltage(VREG_VOLTAGE_1_20);

    // Overclock! It should panic if it can't reach this as we have set second argument to true
    ULOG_INFO("Overclocking to %d KHz...", PMEMTEST_OVERCLOCK_KHZ);
    set_sys_clock_khz(PMEMTEST_OVERCLOCK_KHZ, true);
}

void board_init() {
    // Set up encoder, led and button GPIO pins.
    gpio_init(GPIO_LED);
    gpio_init(GPIO_QUAD_A);
    gpio_init(GPIO_QUAD_B);
    gpio_init(GPIO_QUAD_BTN);
    gpio_init(GPIO_BACK_BTN);

    gpio_set_dir(GPIO_LED, GPIO_OUT);
    gpio_set_dir(GPIO_QUAD_A, GPIO_IN);
    gpio_set_dir(GPIO_QUAD_B, GPIO_IN);
    gpio_set_dir(GPIO_QUAD_BTN, GPIO_IN);
    gpio_set_dir(GPIO_BACK_BTN, GPIO_IN);

    config_t *cfg = config();

    if(cfg->led_on)
    {
        ULOG_INFO("Lighting LED...");
        gpio_put(GPIO_LED, 1);
    } else {

        ULOG_INFO("Not lighting LED...");
        gpio_put(GPIO_LED, 0);
    }

    ULOG_INFO("Configuring RAM chip power pin and making sure is it turned off...");
    gpio_init(GPIO_POWER);
    board_ram_power_off();

}

// Routines for turning on-board power on and off
void board_ram_power_on()
{
    ULOG_INFO("Turning on RAM chip power...");
    gpio_set_dir(GPIO_POWER, true);
    gpio_put(GPIO_POWER, false);
    sleep_ms(100);
}

void board_ram_power_off()
{
    ULOG_INFO("Turning off RAM chip power...");
    gpio_set_dir(GPIO_POWER, false);
}

typedef struct {
    uint32_t pin;
    uint32_t hcount;
} pin_debounce_t;

static pin_debounce_t action_btn = {GPIO_QUAD_BTN, 0};
static pin_debounce_t back_btn = {GPIO_BACK_BTN, 0};


// Returns true only *once* when a button is pushed. No key repeat.
bool board_button_pushed(pin_debounce_t *pin_b)
{
    if (!gpio_get(pin_b->pin)) {
        if (pin_b->hcount == 0) {
            pin_b->hcount = BUTTON_DEBOUNCE_COUNT;
            return true;
        }
    } else {
        if (pin_b->hcount > 0) {
            pin_b->hcount--;
        }
    }
    return false;
}

bool board_encoder_pushed() {
    static pin_debounce_t quad_btn_debounce = {GPIO_QUAD_BTN, 0};
    return board_button_pushed(&quad_btn_debounce);
}

bool board_back_pushed() {
    static pin_debounce_t back_btn_debounce = {GPIO_BACK_BTN, 0};
    return board_button_pushed(&back_btn_debounce);
}

static int32_t encoder_last_position = 0;
static int32_t encoder_position = 0;
static uint8_t encoder_prev_ab = 0;
uint8_t do_board_encoder(void) {
    uint32_t v = gpio_get_all();
    uint8_t ab = ((v >> GPIO_QUAD_A) & 1) << 1 | (((v >> GPIO_QUAD_B) & 1));

    static const int8_t lut[16] = {
        0, -1, +1, 0,
        +1, 0, 0, -1,
        -1, 0, 0, +1,
        0, +1, -1, 0
    };

    int8_t delta = lut[(encoder_prev_ab << 2) | ab];
    encoder_position += delta;

    encoder_prev_ab = ab;

    int32_t diff = encoder_position - encoder_last_position;

    config_t *cfg = config();
    int32_t spc = cfg->enc_states_per_click;

    if (diff >= spc) {
        encoder_last_position += spc;
        return BOARD_ENCODER_ROTATION_CLOCKWISE;

    } else if (diff <= -spc) {
        encoder_last_position -= spc;
        return BOARD_ENCODER_ROTATION_ANTICLOCKWISE;
    } else {
        return BOARD_ENCODER_ROTATION_NONE;
    }
}