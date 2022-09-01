#include <tusb.h>

#define USBD_VID 0x2E8A /* Raspberry Pi */
#define USBD_PID 0x000A /* Raspberry Pi Pico SDK CDC */

#define USBD_MAX_POWER_MA 250

#define USBD_CDC_0_EP_CMD 0x81
#define USBD_CDC_0_EP_OUT 0x02
#define USBD_CDC_0_EP_IN 0x82
#define USBD_CDC_EP_CMD_SIZE 8

#define USBD_HID_0_EP_OUT 0x03
#define USBD_HID_0_EP_IN 0x83

#define USBD_STR_0 0x00
#define USBD_STR_MANUF 0x01
#define USBD_STR_PRODUCT 0x02
#define USBD_STR_SERIAL 0x03
#define USBD_STR_CDC 0x04
#define USBD_STR_HID 0x00

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

static const tusb_desc_device_t usbd_desc_device = {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = USBD_VID,
        .idProduct = USBD_PID,
        .bcdDevice = 0x0100,
        .iManufacturer = USBD_STR_MANUF,
        .iProduct = USBD_STR_PRODUCT,
        .iSerialNumber = USBD_STR_SERIAL,
        .bNumConfigurations = 1,
};

// Invoked when received GET DEVICE DESCRIPTOR request
// Application return pointer to descriptor
const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *) &usbd_desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] = {
        TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_EP_BUFSIZE)
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor,
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) {
    (void) itf;
    return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

enum {
    USBD_ITF_CDC_0 = 0,
    USBD_IFT_CDC_0_DATA __attribute__((unused)),
    USBD_ITF_NUM_HID,
    USBD_ITF_MAX
};

static const uint8_t usbd_desc_cfg[USBD_DESC_LEN] = {
        TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_MAX, USBD_STR_0, USBD_DESC_LEN,
                              TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, USBD_MAX_POWER_MA),

        TUD_CDC_DESCRIPTOR(USBD_ITF_CDC_0, USBD_STR_CDC, USBD_CDC_0_EP_CMD,
                           USBD_CDC_EP_CMD_SIZE, USBD_CDC_0_EP_OUT, USBD_CDC_0_EP_IN,
                           CFG_TUD_CDC_EP_BUFSIZE),

        TUD_HID_INOUT_DESCRIPTOR(USBD_ITF_NUM_HID, USBD_STR_HID, HID_ITF_PROTOCOL_NONE,
                                 sizeof(desc_hid_report), USBD_HID_0_EP_OUT, USBD_HID_0_EP_IN,
                                 CFG_TUD_HID_EP_BUFSIZE, 10),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return usbd_desc_cfg;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

#define DESC_STR_MAX 20

static const char *const usbd_desc_str[] = {
        [USBD_STR_MANUF] = "Raspberry Pi",
        [USBD_STR_PRODUCT] = "Pico",
        [USBD_STR_SERIAL] = "000000000000",
        [USBD_STR_CDC] = "Board CDC",
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    static uint16_t desc_str[DESC_STR_MAX];
    uint8_t len;

    if (index == 0) {
        desc_str[1] = 0x0409;
        len = 1;
    } else {
        const char *str;

        if (index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0]))
            return NULL;

        str = usbd_desc_str[index];
        for (len = 0; len < DESC_STR_MAX - 1 && str[len]; ++len)
            desc_str[1 + len] = str[len];
    }

    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);

    return desc_str;
}
