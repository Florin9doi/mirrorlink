#ifndef _REMOTE_SERVER_H
#define _REMOTE_SERVER_H

#include <stdint.h>
#include "../Utils/str.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ACTION_GET_APPLICATION_LIST,
	ACTION_LAUNCH_APPLICATION,
	ACTION_SET_CLIENT_PROFILE,
	ACTION_MAX
};

enum {
	SERVICE_TYPE_APP,
	SERVICE_TYPE_CLP,
	SERVICE_TYPE_NOT,
	SERVICE_TYPE_MAX
};

typedef struct _service_info {
	str_t surl;
	str_t curl;
	str_t eurl;
} service_info;

typedef struct _App App;
struct _App {
	int appID;
	char *name;
	char *description;
	char *iconPath;
    App *next;
};

typedef struct _remote_server {
	str_t ip;
	str_t uuid;
	uint16_t port;
	service_info sinfo[SERVICE_TYPE_MAX];

	App     *appList;
	char     vnc_ip[32];
	uint16_t vnc_port;
} remote_server;

typedef uint8_t (*action_parser)(remote_server *server, char *content);

typedef struct _action {
	uint32_t stype;
	char *name;
	action_parser handler;
} action;

remote_server *remote_server_create(const char *ip, uint16_t port, const char *path);

uint16_t remote_server_get_application_list(remote_server *server, uint32_t pid, char *filter);

uint16_t remote_server_launch_application(remote_server *server, uint32_t appid, uint32_t pid);

uint16_t remote_server_set_client_profile(remote_server *server, uint32_t pid);

void remote_server_destory(remote_server *server);

uint16_t remote_server_invoke_action(remote_server *server, uint16_t stype, char *action, char *args, action_parser handler);
uint8_t get_application_list_parse(remote_server *server, char *content);
uint8_t launch_application_parse(remote_server *server, char *content);
uint8_t set_client_profile_parse(remote_server *server, char *content);

#ifdef __cplusplus
}
#endif

#endif
