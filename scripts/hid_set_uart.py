# Install python3 HID package https://pypi.org/project/hid/
from time import sleep

import hid
import sys

USB_VID = 0x2E8A

print("Opening HID device with VID = 0x%X" % USB_VID)


def configure(dev):
    command = bytes([0xB0, 0x00])
    dev.write(command)
    data = dev.read(64)
    print("Received from HID Device:")
    print(data, '\n')

    data = [x for x in data]
    if (data[0], data[1]) != (0xB0, 0x00):
        print("Error")
        sys.exit(1)

    print('Writing data...')
    data[0], data[1] = [0xB1, 0x01]
    data[4] |= 0b11100000

    dev.write(bytes(data))
    data = dev.read(64)
    print("Received from HID Device:")
    print(data, '\n')


def main():
    for d in hid.enumerate(USB_VID):
        print(d)
        dev = hid.Device(d['vendor_id'], d['product_id'])
        if dev:
            configure(dev)
            sleep(.5)


if __name__ == '__main__':
    main()
