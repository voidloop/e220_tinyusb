#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <tusb.h>

#include "e220.h"

#ifndef PICO_DEFAULT_LED_PIN
#error LoRa bridge requires a board with a regular LED
#endif

#ifndef MIN
#define MIN(a, b) ((a > b) ? b : a)
#endif

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BUFFER_SIZE 64
#define MAX_SENT 400

enum {
    BLINK_FAILED = 100,
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

radio_inst_t radio = {
        .uart = uart0,
        .tx_pin = 0,
        .rx_pin = 1,
        .m0_pin = 2,
        .m1_pin = 3,
        .aux_pin = 6
};

char buf[BUFFER_SIZE];
uint32_t sent = 0;
uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);

void cdc_task(void);

//--------------------------------------------------------------------+
// Main functions
//--------------------------------------------------------------------+

inline void loop(void) {
    tud_task();
    led_blinking_task();

    cdc_task();
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    if (!radio_init(&radio)) {
        blink_interval_ms = BLINK_FAILED;
        while (true) {
            led_blinking_task();
        }
    }

    tusb_init();

    while (true) {
        loop();
    }
}

#pragma clang diagnostic pop

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
    blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+

void cdc_task(void) {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    {
        // Reset sent counter if the module is not busy
        if (gpio_get(radio.aux_pin) == 1) {
            sent = 0;
        }

        // Send available data if the module buffer is not full
        if (sent < MAX_SENT && tud_cdc_available()) {
            uint32_t len = tud_cdc_read(buf, MIN(MAX_SENT - sent, BUFFER_SIZE));
            uart_write_blocking(radio.uart, (uint8_t *) buf, len);
            sent += len;
        }

        // Read available data
        while (uart_is_readable(radio.uart)) {
            char ch = uart_getc(radio.uart);
            tud_cdc_write_char(ch);
            tud_cdc_write_flush();
        }
    }
}

// Invoked when cdc line state changed e.g. connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void) itf;
    (void) rts;
    (void) dtr;
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // This example doesn't use multiple report and report ID
    (void) itf;
    (void) report_id;
    (void) report_type;

    // echo back anything we received from host
    tud_hid_report(0, buffer, bufsize);
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+

void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Blink every interval ms
    if (now_ms - start_ms < blink_interval_ms) {
        return; // not enough time
    }

    start_ms += blink_interval_ms;

    gpio_put(LED_PIN, led_state);
    led_state = 1 - led_state; // toggle
}