#ifndef _HTTP_CLIENT_H
#define _HTTP_CLIENT_H

#include <stdint.h>
#include "str.h"

typedef struct _http_req {
	str_t method;
	str_t path;
	str_t header;
	str_t body;
} http_req;

typedef struct _http_rsp {
	uint16_t errcode;
	str_t body;
} http_rsp;

http_req *http_client_make_req(char *method, const char *path);

void http_client_add_header(http_req *req, const char *header);

int http_client_set_body(http_req *req, const char *body);

http_rsp *http_client_send(const char *ip, uint16_t port, http_req *req);

uint16_t http_client_get_errcode(http_rsp *rsp);

char *http_client_get_body(http_rsp *rsp);

void http_client_free_rsp(http_rsp *rsp);

#endif
