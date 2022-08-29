#include <hardware/gpio.h>
#include <pico/time.h>
#include <hardware/uart.h>
#include <memory.h>
#include "e220.h"

bool read_parameters(radio_inst_t *radio, parameters_t *params);

bool write_parameters(radio_inst_t *radio, parameters_t *params, bool save);

void wait_aux_high(radio_inst_t *radio);

void set_operating_mode(radio_inst_t *radio, operating_mode_t mode);

bool radio_init(radio_inst_t *radio) {
    gpio_init(radio->aux_pin);
    gpio_set_dir(radio->aux_pin, GPIO_IN);

    gpio_init(radio->m0_pin);
    gpio_set_dir(radio->m0_pin, GPIO_OUT);

    gpio_init(radio->m1_pin);
    gpio_set_dir(radio->m1_pin, GPIO_OUT);

    gpio_set_function(radio->tx_pin, GPIO_FUNC_UART);
    gpio_set_function(radio->rx_pin, GPIO_FUNC_UART);

    uart_init(radio->uart, 9600);
    uart_set_hw_flow(radio->uart, false, false);
    uart_set_format(radio->uart, 8, 1, UART_PARITY_NONE);

    parameters_t current_params;
    if (!read_parameters(radio, &current_params))
        return false;

    parameters_t default_params;
    default_params.addh = RH_E220_DEFAULT_ADDRESS_HIGH;
    default_params.addl = RH_E220_DEFAULT_ADDRESS_LOW;
    default_params.chan = RH_E220_DEFAULT_CHANNEL;
    default_params.sped = RH_E220_DEFAULT_UART_BAUD |
                          RH_E220_DEFAULT_UART_MODE |
                          RH_E220_DEFAULT_DATA_RATE;
    default_params.opt1 = RH_E220_DEFAULT_TX_POWER;
    default_params.opt2 = RH_E220_DEFAULT_WOR_CYCLE;

#ifdef RH_E220_RSSI_BYTE_ENABLED
    default_params.opt2 |= RH_E220_PARAM_OPT2_RSSI_BYTE_ENABLE;
#endif

    if (memcmp(&default_params, &current_params, sizeof(parameters_t)) != 0) {
        if (!write_parameters(radio, &default_params, true))
            return false;
    }

    return true;
}

void wait_aux_high(radio_inst_t *radio) {
    while (gpio_get(radio->aux_pin) == false);
}

void set_operating_mode(radio_inst_t *radio, operating_mode_t mode) {
    wait_aux_high(radio);
    sleep_ms(10);

    switch (mode) {
        default:
        case MODE_NORMAL:
            gpio_put(radio->m0_pin, false);
            gpio_put(radio->m1_pin, false);
            break;

        case MODE_WAKE_UP:
            gpio_put(radio->m0_pin, true);
            gpio_put(radio->m1_pin, false);
            break;

        case MODE_POWER_SAVING:
            gpio_put(radio->m0_pin, false);
            gpio_put(radio->m1_pin, true);
            break;

        case MODE_SLEEP:
            gpio_put(radio->m0_pin, true);
            gpio_put(radio->m1_pin, true);
            break;
    }

    wait_aux_high(radio);
    sleep_ms(10); // Takes a little while to start its response
}

bool read_parameters(radio_inst_t *radio, parameters_t *params) {
    set_operating_mode(radio, MODE_SLEEP);

    uint8_t command[] = {RH_E220_COMMAND_READ_PARAMS, 0x00, sizeof(parameters_t)};
    uart_write_blocking(radio->uart, command, sizeof(command));

    // Fail if can't read uart
    if (!uart_is_readable_within_us(radio->uart, 1000 * 1000))
        return false;

    uart_read_blocking(radio->uart, command, sizeof(command));

    if (command[0] == 0xFF &&
        command[1] == 0xFF &&
        command[2] == 0xFF) {
        return false;
    }

    uart_read_blocking(radio->uart, (uint8_t *) params, sizeof(parameters_t));
    set_operating_mode(radio, MODE_NORMAL);
    return true;
}

bool write_parameters(radio_inst_t *radio, parameters_t *params, bool save) {
    set_operating_mode(radio, MODE_SLEEP);

    uint8_t head = save ? RH_E220_COMMAND_WRITE_PARAMS_SAVE : RH_E220_COMMAND_WRITE_PARAMS_NOSAVE;
    uint8_t command[] = {head, 0x00, sizeof(parameters_t)};

    uart_write_blocking(radio->uart, command, sizeof(command));
    uart_write_blocking(radio->uart, (uint8_t *) params, sizeof(parameters_t));

    // Fail if can't read uart
    if (!uart_is_readable_within_us(radio->uart, 1000 * 1000))
        return false;

    uart_read_blocking(radio->uart, command, sizeof(command));

    if (command[0] == 0xFF &&
        command[1] == 0xFF &&
        command[2] == 0xFF) {
        return false;
    }

    uart_read_blocking(radio->uart, (uint8_t *) params, sizeof(parameters_t));
    set_operating_mode(radio, MODE_NORMAL);
    return true;
}