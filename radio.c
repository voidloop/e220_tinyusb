#include <hardware/gpio.h>
#include <pico/time.h>
#include <hardware/uart.h>

#include "radio.h"

bool radio_init(radio_inst_t const *radio) {
    gpio_init(radio->aux_pin);
    gpio_set_dir(radio->aux_pin, GPIO_IN);

    gpio_init(radio->m0_pin);
    gpio_set_dir(radio->m0_pin, GPIO_OUT);

    gpio_init(radio->m1_pin);
    gpio_set_dir(radio->m1_pin, GPIO_OUT);

    gpio_set_function(radio->tx_pin, GPIO_FUNC_UART);
    gpio_set_function(radio->rx_pin, GPIO_FUNC_UART);

    uart_set_hw_flow(radio->uart, false, false);
    set_radio_uart_config_mode(radio);

    parameters_t params;
    if (!read_parameters(radio, &params))
        return false;

    set_radio_uart(radio, params.sped);
    return true;
}

bool read_parameters(radio_inst_t const *radio, parameters_t *params) {
    set_operating_mode(radio, MODE_SLEEP);

    uint8_t command[] = {RADIO_COMMAND_READ_PARAMS, 0x00, sizeof(parameters_t)};
    uart_write_blocking(radio->uart, command, sizeof(command));

    // Fail?
    if (!uart_is_readable_within_us(radio->uart, 1000 * 1000)) {
        set_operating_mode(radio, MODE_NORMAL);
        return false;
    }

    uart_read_blocking(radio->uart, command, sizeof(command));

    if (command[0] == 0xFF &&
        command[1] == 0xFF &&
        command[2] == 0xFF) {
        set_operating_mode(radio, MODE_NORMAL);
        return false;
    }
    uart_read_blocking(radio->uart, (uint8_t *) params, sizeof(parameters_t));

    set_operating_mode(radio, MODE_NORMAL);
    return true;
}

bool write_parameters(radio_inst_t const *radio, parameters_t const *params, bool save) {
    set_operating_mode(radio, MODE_SLEEP);

    uint8_t head = save ? RADIO_COMMAND_WRITE_PARAMS_SAVE : RADIO_COMMAND_WRITE_PARAMS_NOSAVE;
    uint8_t command[] = {head, 0x00, sizeof(parameters_t)};

    uart_write_blocking(radio->uart, command, sizeof(command));
    uart_write_blocking(radio->uart, (uint8_t *) params, sizeof(parameters_t));

    // Fail?
    if (!uart_is_readable_within_us(radio->uart, 1000 * 1000)) {
        set_operating_mode(radio, MODE_NORMAL);
        return false;
    }

    uart_read_blocking(radio->uart, command, sizeof(command));

    if (command[0] == 0xFF &&
        command[1] == 0xFF &&
        command[2] == 0xFF) {
        set_operating_mode(radio, MODE_NORMAL);
        return false;
    }

    uart_read_blocking(radio->uart, (uint8_t *) params, sizeof(parameters_t));
    set_operating_mode(radio, MODE_NORMAL);
    return true;
}

void set_operating_mode(radio_inst_t const *radio, operating_mode_t mode) {
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
    sleep_ms(50); // Takes a little while to start its response
}

void wait_aux_high(radio_inst_t const *radio) {
    while (gpio_get(radio->aux_pin) == false);
}

void set_radio_uart_config_mode(radio_inst_t const *radio) {
    set_radio_uart(radio, RADIO_PARAM_SPED_UART_BAUD_9600 | RADIO_PARAM_SPED_UART_MODE_8N1);
}

void set_radio_uart(radio_inst_t const *radio, uint8_t sped) {
    // Baud rate
    switch (sped & RADIO_PARAM_SPED_UART_BAUD_MASK) {
        case RADIO_PARAM_SPED_UART_BAUD_1200:
            uart_init(radio->uart, 1200);
            break;
        case RADIO_PARAM_SPED_UART_BAUD_2400:
            uart_init(radio->uart, 2400);
            break;
        case RADIO_PARAM_SPED_UART_BAUD_4800:
            uart_init(radio->uart, 4800);
            break;
        case RADIO_PARAM_SPED_UART_BAUD_19200:
            uart_init(radio->uart, 19200);
            break;
        case RADIO_PARAM_SPED_UART_BAUD_38400:
            uart_init(radio->uart, 38400);
            break;
        case RADIO_PARAM_SPED_UART_BAUD_57600:
            uart_init(radio->uart, 57600);
            break;
        case RADIO_PARAM_SPED_UART_BAUD_115200:
            uart_init(radio->uart, 115200);
            break;
        default:
            uart_init(radio->uart, 9600);
            break;
    }

    // Parity
    switch (sped & RADIO_PARAM_SPED_UART_MODE_MASK) {
        case RADIO_PARAM_SPED_UART_MODE_8E1:
            uart_set_format(radio->uart, 8, 1, UART_PARITY_EVEN);
            break;
        case RADIO_PARAM_SPED_UART_MODE_8O1:
            uart_set_format(radio->uart, 8, 1, UART_PARITY_ODD);
            break;
        default:
            uart_set_format(radio->uart, 8, 1, UART_PARITY_NONE);
            break;
    }
}