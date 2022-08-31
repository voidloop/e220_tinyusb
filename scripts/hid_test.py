# Install python3 HID package https://pypi.org/project/hid/
import hid

USB_VID = 0x2E8A

print("Opening HID device with VID = 0x%X" % USB_VID)

for d in hid.enumerate(USB_VID):
    print(d)
    dev = hid.Device(d['vendor_id'], d['product_id'])
    if dev:
        while True:
            # Get input from console and encode to UTF8 for array of chars.
            # hid generic inout is single report therefore by HIDAPI requirement
            # it must be proceeded with 0x00 as dummy reportID
            str_out = b'\xb0'
            str_out += input("Send text to HID Device : ").encode('utf-8')
            dev.write(str_out)
            str_in = dev.read(64)
            print("Received from HID Device:", str_in, '\n')
