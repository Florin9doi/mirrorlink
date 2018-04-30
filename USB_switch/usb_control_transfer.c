#include <stdio.h>
#include <unistd.h>
#include "libusb.h"

// struct mirror_link_supported_devices {
//     int vid;
//     int pid;
// } static mirror_link_list[] = {
//     {0x04e8, 0x6860}, // Samsung S5
//     {0x1004, 0x6300}, // LG G4 - Charging
//     {0x1004, 0x631d}, // LG G4 - PTP
//     {0x1004, 0x633e}, // LG G4 - MTP
//     {0x1004, 0x6349}, // LG G4 - MIDI
//     // {0x1004, 0x635b}, // LG G4 - MirrorLink
// };

int try_to_enable_mirror_link(libusb_context *context, int vid, int pid) {
    libusb_device_handle *handle = NULL;
    handle = libusb_open_device_with_vid_pid(context, vid, pid);
    if (handle == NULL) {
        printf(" - Error: cannot open the device - permission denied\n");
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
        printf(" - USB transfer SUCCESS - possible a MirrorLink device\n");
    } else {
        printf(" - USB transfer error: %s\n", libusb_error_name(ret));
    }

    libusb_close(handle);
    return 0;
}

int hotplug_callback(libusb_context *context, libusb_device *device, libusb_hotplug_event event, void *user_data) {
    struct libusb_device_descriptor descriptor = { 0 };
    libusb_get_device_descriptor(device, &descriptor);

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        printf("USB device detected : 0x%04x:0x%04x\n", descriptor.idVendor, descriptor.idProduct);
        // int idx = 0;
        // for (idx = 0; idx < sizeof(mirror_link_list) / sizeof(mirror_link_list[0]); idx++) {
        //     if (mirror_link_list[idx].vid == descriptor.idVendor && mirror_link_list[idx].pid == descriptor.idProduct) {
        //         printf("MirrorLink device found\n");
        //         enable_mirror_link(context, descriptor.idVendor, descriptor.idProduct);
        //         break;
        //     }
        // }
        try_to_enable_mirror_link(context, descriptor.idVendor, descriptor.idProduct);
    } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        printf("USB device disconnected : 0x%04x:0x%04x\n", descriptor.idVendor, descriptor.idProduct);
    }
    return 0;
}

int main(int argc, char **argv) {
    int ret;

    libusb_context *context = NULL;
    ret = libusb_init(&context);
    if (ret < 0) {
        printf("libusb_init: %d\n", ret);
        return ret;
    }

    libusb_hotplug_callback_handle callback = { 0 };
    libusb_hotplug_register_callback(context,
            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
            LIBUSB_HOTPLUG_ENUMERATE,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            hotplug_callback, NULL,
            &callback);

    while (1) {
        libusb_handle_events(context);
        usleep(100);
    }

    libusb_exit(context);
    return ret;
}
