#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "conn.h"
#include "http_client.h"
#include "buffer.h"
#include "str.h"

http_req *http_client_make_req(char *method, const char *path)
{
	http_req *req = (http_req *)calloc(1, sizeof(*req));
	str_append(&(req->method), method);
	str_append(&(req->path), path);
	return req;
}

void http_client_add_header(http_req *req, const char *header)
{
	str_append(&(req->header), header);
}

int http_client_set_body(http_req *req, const char *body)
{
	if (strcmp(req->method, "GET")) {
		str_append(&(req->body), body);
		return 0;
	}
	return -1;
}

http_rsp *http_client_send(const char *ip, uint16_t port, http_req *req)
{
	int fd;
	char buf[100];
	str_t wbuf = 0;
	buffer rbuf;
	http_rsp *rsp;
	int quit = 0;
	buffer_init(&rbuf, 0);
	fd = conn_open(ip, port);
	if (fd < 0) {
		printf("tcp connect failed, ip = %s, port = %d\n", ip, port);
		return 0;
	}
	sprintf(buf, "%s %s HTTP/1.0\r\n", req->method, req->path);
	str_append(&wbuf, buf);
	sprintf(buf, "Host: %s:%d\r\n", ip, port);
	str_append(&wbuf, buf);
	if (req->body) {
		sprintf(buf, "Content-Length:%d\r\n", (int)strlen(req->body));
		str_append(&(req->header), buf);
	}
	if (req->header) {
		str_append(&wbuf, req->header);
	}
	str_append(&wbuf, "\r\n");
	if (!strcmp(req->method, "POST") && req->body) {
		str_append(&wbuf, req->body);
	}
	free(req->method);
	free(req->body);
	free(req->header);
	free(req->path);
	free(req);
	conn_write(fd, wbuf, strlen(wbuf));
	rsp = (http_rsp *)calloc(1, sizeof(*rsp));
	while (0 == quit) {
		char *pos;
		uint32_t len = 0;
		int dummy = 0;
		int sr;
		quit = conn_read_all(fd, &rbuf);
		if (!rbuf.len) {
			continue;
		}
		buffer_append(&rbuf, 1);
		pos = (char *)rbuf.buf;
		sr = sscanf(pos, "HTTP/1.%1d %3d %*s\r\n", &dummy, (int *)&(rsp->errcode));
		if (sr < 2) {
			continue;
		}
		pos = strstr(pos, "Content-Length:");
		if (0 == pos) {
			pos = (char *)rbuf.buf;
			pos = strstr(pos, "CONTENT-LENGTH:");
			if (0 == pos) {
				continue;
			}
		}
		sr = sscanf(pos, "Content-Length: %d\r\n", &len);
		if (sr < 1) {
			sr = sscanf(pos, "CONTENT-LENGTH: %d\r\n", &len);
			if (sr < 1) {
				continue;
			}
		}
		if (len > 0) {
			pos = strstr(pos, "\r\n\r\n") + 4;
			if ((char *)4 == pos) {
				continue;
			}
			if (strlen(pos) >= len) {
				str_append(&(rsp->body), pos);
				break;
			}
		} else {
			break;
		}
	}
	if (0 == rsp->errcode) {
		free(rsp);
		rsp = 0;
	}
	free(wbuf);
	buffer_clear(&rbuf);
	conn_close(fd);
	return rsp;
}

uint16_t http_client_get_errcode(http_rsp *rsp)
{
	if (rsp) {
		return rsp->errcode;
	} else {
		return 0;
	}
}

char *http_client_get_body(http_rsp *rsp)
{
	if (rsp) {
		return rsp->body;
	} else {
		return 0;
	}
}

void http_client_free_rsp(http_rsp *rsp)
{
	if (rsp) {
		free(rsp->body);
	}
	free(rsp);
}

