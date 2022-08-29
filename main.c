#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <tusb.h>

#ifndef PICO_DEFAULT_LED_PIN
#error LoRa bridge requires a board with a regular LED
#endif

#ifndef MIN
#define MIN(a, b) ((a > b) ? b : a)
#endif

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BUFFER_SIZE 64
#define MAX_SENT 400
#define AUX_PIN 6
#define UART uart0
#define TX_PIN 0
#define RX_PIN 1
#define M0_PIN 3
#define M1_PIN 4

enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

char buf[BUFFER_SIZE];
uint32_t sent = 0;
uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);

void cdc_task(void);

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

inline void loop(void) {
    tud_task();
    led_blinking_task();

    cdc_task();
}


int main(void) {
    tusb_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(AUX_PIN);
    gpio_set_dir(AUX_PIN, GPIO_IN);

    gpio_init(M0_PIN);
    gpio_set_dir(M0_PIN, GPIO_OUT);

    gpio_init(M1_PIN);
    gpio_set_dir(M1_PIN, GPIO_OUT);

    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    uart_init(UART, 9600);
    uart_set_hw_flow(UART, false, false);
    uart_set_format(UART, 8, 1, UART_PARITY_NONE);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        loop();
    }
#pragma clang diagnostic pop
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb() {
    blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb() {
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
void tud_resume_cb() {
    blink_interval_ms = BLINK_MOUNTED;
}

void cdc_task(void) {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    {
        // Reset sent counter if the module is not busy
        if (gpio_get(AUX_PIN) == 1) {
            sent = 0;
        }

        // Send available data if the module buffer is not full
        if (sent < MAX_SENT && tud_cdc_available()) {
            uint32_t len = tud_cdc_read(buf, MIN(MAX_SENT - sent, BUFFER_SIZE));
            uart_write_blocking(UART, (uint8_t *)buf, len);
            sent += len;

            len = sprintf((char *) buf, "pos==%lu\r\n", sent);
            tud_cdc_write(buf, len);
            tud_cdc_write_flush();
        }

        // Read available data
        while (uart_is_readable(UART)) {
            char ch = uart_getc(UART);
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

//    // TODO set some indicator
//    if (dtr) {
//        // Terminal connected
//    } else {
//        // Terminal disconnected
//    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Blink every interval ms
    if (now_ms - start_ms < blink_interval_ms) return; // not enough time
    start_ms += blink_interval_ms;

    gpio_put(LED_PIN, led_state);
    led_state = 1 - led_state; // toggle
}