#include <stdio.h>
#include "libusb.h"

int main(int argc, char **argv) {
	int ret;

	libusb_context *context = NULL;
	ret = libusb_init(&context);
	if (ret < 0) {
		printf("libusb_init: %d\n", ret);
		return ret;
	}
	
	libusb_device_handle *handle;
	handle = libusb_open_device_with_vid_pid(context, 0x04e8, 0x6860);
	if (handle == NULL) {
		handle = libusb_open_device_with_vid_pid(context, 0x04e8, 0x685d);
	}
	if (handle == NULL) {
		printf("Error: libusb_open_device_with_vid_pid\n");
		return 1;
	}

	uint8_t	bmRequestType = 0x40;
	uint8_t	bRequest      = 0xF0;
	uint16_t wValue       = (0x01<<8) | 0x01;
	uint16_t wIndex       = 0x00;
	unsigned char *data   = NULL;
	uint16_t wLength      = 0;
	unsigned int timeout  = 0;

	ret = libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);

	libusb_exit(context);
	return ret;
}
