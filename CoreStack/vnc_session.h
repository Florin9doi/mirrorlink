#ifndef _VNC_SESSION_H
#define _VNC_SESSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct server_dispinfo {
	uint8_t maxv;
	uint8_t minv;
	uint16_t :10;
	uint16_t sfat:1;
	uint16_t :1;
	uint16_t sfds:1;
	uint16_t sfus:1;
	uint16_t sfr:1;
	uint16_t sfos:1;
	uint16_t rpwidth;
	uint16_t rpheight;
	uint32_t :6;
	uint32_t sgray:1;
	uint32_t dgray:1;
	uint32_t other16:1;
	uint32_t :3;
	uint32_t rgb343:1;
	uint32_t rgb444:1;
	uint32_t rgb555:1;
	uint32_t rgb565:1;
	uint32_t other24:1;
	uint32_t :6;
	uint32_t rgb888:1;
	uint32_t other32:1;
	uint32_t :6;
	uint32_t argb888:1;
} __attribute((packed));

struct server_evinfo {
	uint16_t kblc;
	uint16_t kbcc;
	uint16_t uilc;
	uint16_t uicc;
	uint32_t knob;
	uint32_t device;
	uint32_t multimedia;
	uint32_t :16;
	uint32_t fun_num:8;
	uint32_t evmap:1;
	uint32_t kel:1;
	uint32_t vkt:1;
	uint32_t keypad:1;
	uint32_t tepm:8;
	uint32_t tnse:8;
	uint32_t pebm:8;
	uint32_t :6;
	uint32_t tes:1;
	uint32_t pes:1;
} __attribute((packed));

typedef struct _context_info {
	uint32_t appid;
	uint16_t tlac;
	uint16_t tlcc;
	uint32_t ac;
	uint32_t cc;
	uint32_t cr;
} __attribute((packed)) context_info;

typedef struct _vnc_session_cb {
	void (*on_device_status)(int fd, uint32_t status);
	uint8_t *(*on_desktop_size_changed)(int fd, uint16_t w, uint16_t h);
	void (*on_framebuffer_update)(int fd, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	void (*on_context_infomation)(int fd, context_info *info);
} vnc_session_cb;

typedef struct _vnc_session {
	int fd;
	uint16_t rfb_width;
	uint16_t rfb_height;
	struct server_dispinfo sdinfo;
	struct server_evinfo seinfo;
	uint8_t status;
	vnc_session_cb *cb;
} vnc_session;

struct pointer_event {
	uint8_t mask;
	uint16_t x;
	uint16_t y;
} __attribute((packed));

struct key_event {
	uint8_t flag;
	uint16_t padding;
	uint32_t key;
} __attribute((packed));

int vnc_session_start(const char *ip, uint16_t port);

void vnc_session_task(int fd, vnc_session_cb *cb);

void vnc_session_send_pointer_event(int fd, struct pointer_event pointer);

void vnc_session_send_key_event(int fd, struct key_event key);

void vnc_session_send_device_status(int fd, uint32_t status);

void vnc_session_send_byebye(int fd);

#ifdef __cplusplus
}
#endif

#endif
