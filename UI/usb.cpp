#include <stdio.h>
#include <unistd.h>
#include <libusb.h>
#include "usb.h"
#include <QDebug>

int try_to_enable_mirror_link(libusb_device *device) {
    libusb_device_handle *handle = NULL;
    libusb_open(device, &handle);
    if (handle == NULL) {
        qDebug() << " - Error: cannot open the device - permission denied";
        return 1;
    }

    uint8_t bmRequestType = 0x40;
    uint8_t bRequest      = 0xF0;
    uint16_t wValue       = (0x01<<8) | 0x01;
    uint16_t wIndex       = 0x00;
    unsigned char *data   = NULL;
    uint16_t wLength      = 0;
    unsigned int timeout  = 100;

    int ret = libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);
    if (ret == LIBUSB_SUCCESS) {
        qDebug() << " - USB transfer SUCCESS - possible a MirrorLink device";
    } else {
        qDebug() << " - USB transfer error: " << libusb_error_name(ret);
    }

    libusb_close(handle);
    return 0;
}

int hotplug_callback(libusb_context *context, libusb_device *device, libusb_hotplug_event event, void *user_data) {
    (void) context;
    (void) user_data;
    struct libusb_device_descriptor descriptor;
    libusb_get_device_descriptor(device, &descriptor);

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        qDebug() << "USB device detected : "
                 << QString::number(descriptor.idVendor, 16) << ":"
                 << QString::number(descriptor.idProduct, 16);
        try_to_enable_mirror_link(device);
    } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        qDebug() << "USB device disconnected : " << descriptor.idVendor << ":" << descriptor.idProduct;
    }
    return 0;
}

void USBThread::run() {
    int ret;

    libusb_context *context = NULL;
    ret = libusb_init(&context);
    if (ret < 0) {
        printf("libusb_init: %d", ret);
//        return ret;
        return;
    }

    libusb_hotplug_callback_handle callback = { 0 };
    libusb_hotplug_register_callback(context,
            (libusb_hotplug_event) (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
            LIBUSB_HOTPLUG_ENUMERATE,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            hotplug_callback, NULL,
            &callback);

    while (1) {
        libusb_handle_events(context);
    }

    libusb_exit(context);
    return;
}
