#!/usr/bin/env python

"""
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
"""

"""
    LUFA Generic HID device demo host test script. This script will send a
    continuous stream of generic reports to the device, to show a variable LED
    pattern on the target board. Send and received report data is printed to
    the terminal.

    Requires PyUSB >= 1.0.0 (https://github.com/pyusb/pyusb).
"""




import sys
from time import sleep
import usb.core
import usb.util
def get_and_init_hid_device():
    device = usb.core.find(idVendor=0x2786, idProduct=0x6837)

    if device is None:
        sys.exit("Could not find USB device.")

    if device.is_kernel_driver_active(0):
        try:
            device.detach_kernel_driver(0)
        except usb.core.USBError as exception:
            sys.exit("Could not detatch kernel driver: %s" % str(exception))

    try:
        device.set_configuration()
    except usb.core.USBError as exception:
        sys.exit("Could not set configuration: %s" % str(exception))

    # device.reset()

    return device


def send_report(device, nonce):
    data = list(range(64))
    # Report data for the demo is LED on/off data
    report_data = [0xff, 0xff, 0xff, 0xff,
                   0x81, 0x00, 0x42, nonce] + data[:64-8] + [0xff, 0xff, 0xff, 0xff, 0x0, nonce] + data[64-8:]

    endpoint = usb.util.find_descriptor(
        device[0][(0, 0)],
        # match the first OUT endpoint
        custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)

    # Send the generated report to the device
    for i in range(0, len(report_data), 64):
        print(f"writing: {report_data[i:i+64]}")
        device.write(
            endpoint.bEndpointAddress,
            report_data[i:i+64])


def receive_report(hid_device):
    endpoint = hid_device[0][(0, 0)][0]
    for _ in range(2):
        report_data = hid_device.read(
            endpoint.bEndpointAddress, endpoint.wMaxPacketSize)
        print(f"recevied: {list(report_data)}")


def main():
    hid_device = get_and_init_hid_device()

    print("Connected to device 0x%04X/0x%04X - %s [%s]" %
          (hid_device.idVendor, hid_device.idProduct,
           usb.util.get_string(hid_device, hid_device.iProduct),
           usb.util.get_string(hid_device, hid_device.iManufacturer)))

    p = 0
    try:
        while (True):
            send_report(hid_device, p)

            receive_report(hid_device)

            p += 1

            sleep(.2)
    except KeyboardInterrupt:
        usb.util.dispose_resources(hid_device)
        hid_device.attach_kernel_driver(0)


if __name__ == '__main__':
    main()
