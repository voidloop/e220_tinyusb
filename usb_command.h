#ifndef _LORA_BRIDGE_USB_COMMAND_H_
#define _LORA_BRIDGE_USB_COMMAND_H_

#include "radio.h"

#define USB_COMMAND_READ_PARAMS   0xB0
#define USB_COMMAND_WRITE_PARAMS  0xB1

#define USB_COMMAND_SUCCESS  0x00
#define USB_COMMAND_FAILED   0x01

bool usb_command_read_params(radio_inst_t const *radio, uint8_t *response, uint8_t const *buffer, uint32_t bufsize);

bool usb_command_write_params(radio_inst_t const *radio, uint8_t *response, uint8_t const *buffer, uint32_t bufsize);

#endif //_LORA_BRIDGE_USB_COMMAND_H_
