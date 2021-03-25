#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libxml/parser.h"
#include "libxml/tree.h"

#include "http_client.h"
#include "remote_server.h"

static action action_map[ACTION_MAX] = {
	{SERVICE_TYPE_APP, "GetApplicationList", get_application_list_parse},
	{SERVICE_TYPE_APP, "LaunchApplication", launch_application_parse},
	{SERVICE_TYPE_CLP, "SetClientProfile", set_client_profile_parse}
};

remote_server *remote_server_create(const char *ip, uint16_t port, const char *path) {
	http_req *rq = http_client_make_req("GET", path);
	http_rsp *rp = http_client_send(ip, port, rq);
	remote_server *server = 0;
	if (rp && (200 == http_client_get_errcode(rp))) {
		xmlDocPtr doc = xmlParseDoc((const xmlChar *)http_client_get_body(rp));
		xmlNodePtr root = xmlDocGetRootElement(doc);
		xmlNodePtr c1 = xmlFirstElementChild(root);
		printf("get description successfully\n");
		for (; c1 != 0; c1 = xmlNextElementSibling(c1)) {
			if (!xmlStrcmp(c1->name, (const xmlChar *)"device")) {
				xmlNodePtr c2 = xmlFirstElementChild(c1);
				for (; c2 != 0; c2 = xmlNextElementSibling(c2)) {
					if (!xmlStrcmp(c2->name, (const xmlChar *)"deviceType") && !xmlStrcmp(xmlNodeGetContent(c2), (const xmlChar *)"urn:schemas-upnp-org:device:TmServerDevice:1")) {
						printf("this is a mirrorlink server device.\n");
						server = (remote_server *)calloc(1, sizeof(*server));
						str_append(&(server->ip), ip);
						server->port = port;
					} else if (!xmlStrcmp(c2->name, (const xmlChar *)"UDN")) {
						str_append(&(server->uuid), (char*)xmlNodeGetContent(c2));
						printf("uuid = %s\n", server->uuid);
					} else if (!xmlStrcmp(c2->name, (const xmlChar *)"serviceList")) {
						xmlNodePtr c3 = xmlFirstElementChild(c2);
						for (; c3 != 0; c3 = xmlNextElementSibling(c3)) {
							if (!xmlStrcmp(c3->name, (const xmlChar *)"service")) {
								xmlNodePtr c4 = xmlFirstElementChild(c3);
								uint8_t type = SERVICE_TYPE_MAX;
								for (; c4 != 0; c4 = xmlNextElementSibling(c4)) {
									if (!xmlStrcmp(c4->name, (const xmlChar *)"serviceType")) {
										printf("service type is %s\n", xmlNodeGetContent(c4));
										xmlChar *stype = xmlNodeGetContent(c4);
										if (!xmlStrcmp(stype,
													(const xmlChar *)"urn:schemas-upnp-org:service:TmApplicationServer:1")) {
											type = SERVICE_TYPE_APP;
										} else if (!xmlStrcmp(stype,
													(const xmlChar *)"urn:schemas-upnp-org:service:TmClientProfile:1")) {
											type = SERVICE_TYPE_CLP;
										} else if (!xmlStrcmp(stype,
													(const xmlChar *)"urn:schemas-upnp-org:service:TmNotificationServer:1")) {
											type = SERVICE_TYPE_NOT;
										}
									} else if (!xmlStrcmp(c4->name, (const xmlChar *)"SCPDURL")) {
										str_append(&(server->sinfo[type].surl), (char*)xmlNodeGetContent(c4));
										printf("subscribe url is %s\n", server->sinfo[type].surl);
									} else if (!xmlStrcmp(c4->name, (const xmlChar *)"controlURL")) {
										str_append(&(server->sinfo[type].curl), (char*)xmlNodeGetContent(c4));
										printf("control url is %s\n", server->sinfo[type].curl);
									} else if (!xmlStrcmp(c4->name, (const xmlChar *)"eventSubURL")) {
										str_append(&(server->sinfo[type].eurl), (char*)xmlNodeGetContent(c4));
										printf("event url is %s\n", server->sinfo[type].eurl);
									}
								}
							}
						}
					}
				}
			}
		}
		xmlFreeDoc(doc);
	}
	http_client_free_rsp(rp);
	return server;
}

uint16_t remote_server_get_application_list(remote_server *server, uint32_t pid, char *filter) {
	char buf[100];

	if (!server) {
		return 0;
	}

	sprintf(buf, "<AppListingFilter>%s</AppListingFilter><ProfileID>%d</ProfileID>", filter, pid);
	return remote_server_invoke_action(server, action_map[ACTION_GET_APPLICATION_LIST].stype,
					action_map[ACTION_GET_APPLICATION_LIST].name, buf, action_map[ACTION_GET_APPLICATION_LIST].handler);
}

uint16_t remote_server_launch_application(remote_server *server, uint32_t appid, uint32_t pid) {
	char buf[100];

	if (!server) {
		return 0;
	}

	sprintf(buf, "<AppID>0x%08x</AppID><ProfileID>%d</ProfileID>", appid, pid);
	return remote_server_invoke_action(server, action_map[ACTION_LAUNCH_APPLICATION].stype, action_map[ACTION_LAUNCH_APPLICATION].name, buf, action_map[ACTION_LAUNCH_APPLICATION].handler);
}

uint16_t remote_server_set_client_profile(remote_server *server, uint32_t pid) {
	str_t param = 0;
	uint16_t ret = 0;
	char *profile = 
		"<ClientProfile>"
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<clientProfile>"
		"<clientID>Cl_1</clientID>"
		"<friendlyName>Client One</friendlyName>"
		"<manufacturer>man_2</manufacturer>"
		"<modelName>CL_Model2</modelName>"
		"<modelNumber>2009</modelNumber>"
		"<iconPreference>"
		"<mimetype>image/png</mimetype>"
		"<width>240</width>"
		"<height>240</height>"
		"<depth>24</depth>"
		"</iconPreference>"
		"<connectivity>"
		"<bluetooth>"
		"<bdAddr>1A2B3C4D5E6F</bdAddr>"
		"<startConnection>false</startConnection>"
		"</bluetooth>"
		"</connectivity>"
		"<rtpStreaming>"
		"<payloadType>0,99</payloadType>"
		"<audioIPL>4800</audioIPL>"
		"<audioMPL>9600</audioMPL>"
		"</rtpStreaming>"
		"<services>"
		"<notification>"
		"<notiUiSupport>true</notiUiSupport>"
		"<maxActions>3</maxActions>"
		"<actionNameMaxLength>15</actionNameMaxLength>"
		"<notiTitleMaxLength>25</notiTitleMaxLength>"
		"<notiBodyMaxLength>100</notiBodyMaxLength>"
		"</notification>"
		"</services>"
		"<mirrorLinkVersion>"
		"<majorVersion>1</majorVersion>"
		"<minorVersion>1</minorVersion>"
		"</mirrorLinkVersion>"
		"<misc>"
		"<driverDistractionSupport>true</driverDistractionSupport>"
		"</misc>"
		"</clientProfile>"
		"</ClientProfile>";
	char buf[50];

	if (!server) {
		return 0;
	}

	sprintf(buf, "<ProfileID>%d</ProfileID>", pid);
	str_append(&param, buf);
	str_append(&param, profile);
	ret = remote_server_invoke_action(server, action_map[ACTION_SET_CLIENT_PROFILE].stype, action_map[ACTION_SET_CLIENT_PROFILE].name, param, action_map[ACTION_SET_CLIENT_PROFILE].handler);
	free(param);
	return ret;
}

uint16_t remote_server_invoke_action(remote_server *server, uint16_t stype, char *action, char *args, action_parser handler) {
	http_req *rq = 0;
	http_rsp *rp = 0;
	char buf[300] = {0};
	str_t req = 0;
	char *service = 0;
	uint16_t ret = 0;

	switch (stype) {
		case SERVICE_TYPE_APP:
			service = "TmApplicationServer:1";
			break;
		case SERVICE_TYPE_CLP:
			service = "TmClientProfile:1";
			break;
		case SERVICE_TYPE_NOT:
			service = "TmNotificationServer:1";
			break;
	}
	rq = http_client_make_req("POST", server->sinfo[stype].curl);
	http_client_add_header(rq, buf);
	http_client_add_header(rq, "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n");
	sprintf(buf, "SOAPACTION: \"urn:schemas-upnp-org:service:%s#%s\"\r\n", service, action);
	http_client_add_header(rq, buf);
	sprintf(buf, "<?xml version=\"1.0\"?>"
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
			"<s:Body>"
			"<u:%s xmlns:u=\"urn:schemas-upnp-org:service:%s\">", action, service);
	str_append(&req, buf);
	str_append(&req, args);
	sprintf(buf, "</u:%s>"
			"</s:Body>"
			"</s:Envelope>", action);
	str_append(&req, buf);
	http_client_set_body(rq, req);
	free(req);
	rp = http_client_send(server->ip, server->port, rq);
	if (rp) {
		switch (http_client_get_errcode(rp)) {
			case 200:
				{
					printf("action invoke successfully.\n");
					if (handler) {
						if (handler(server, http_client_get_body(rp))) {
							ret = 200;
						}
					} else {
						printf("response body is %s\n", http_client_get_body(rp));
					}
				}
				break;
			default:
				printf("action invoke error %d\n", http_client_get_errcode(rp));
				ret = http_client_get_errcode(rp);
				break;
		}
	}
	http_client_free_rsp(rp);
	return ret;
}

uint8_t get_application_list_parse(remote_server *server, char *content) {
	xmlDocPtr doc = xmlParseDoc((const xmlChar *)content);
	xmlNodePtr root = xmlDocGetRootElement(doc);
	if ((!xmlStrcmp(root->name, (const xmlChar *)"Envelope"))) {
		xmlNodePtr envelope = xmlFirstElementChild(root);
		if ((!xmlStrcmp(envelope->name, (const xmlChar *)"Body"))) {
			xmlNodePtr body = xmlFirstElementChild(envelope);
			if ((!xmlStrcmp(body->name, (const xmlChar *)"GetApplicationListResponse"))) {
				xmlNodePtr getApplicationListResponse = xmlFirstElementChild(body);
				if ((!xmlStrcmp(getApplicationListResponse->name, (const xmlChar *)"AppListing"))) {

					unsigned char *appListNode = xmlNodeGetContent(getApplicationListResponse);
					xmlDocPtr doc2 = xmlParseDoc((const xmlChar *)appListNode);
					xmlNodePtr root2 = xmlDocGetRootElement(doc2);

					xmlNodePtr appNode = xmlFirstElementChild(root2);
					for (; appNode; appNode = xmlNextElementSibling(appNode)) {

						App *app = calloc(1, sizeof(App));

						if ((!xmlStrcmp(appNode->name, (const xmlChar *)"app"))) {
							xmlNodePtr param = xmlFirstElementChild(appNode);
							for (; param; param = xmlNextElementSibling(param)) {
								if ((!xmlStrcmp(param->name, (const xmlChar *)"appID"))) {
									char *appID = (char*)xmlNodeGetContent(param);
									sscanf(appID, "%x", &app->appID);
								}
								if ((!xmlStrcmp(param->name, (const xmlChar *)"name"))) {
									app->name = (char*)xmlNodeGetContent(param);
								}
								if ((!xmlStrcmp(param->name, (const xmlChar *)"description"))) {
									app->description = (char*)xmlNodeGetContent(param);
								}
								if ((!xmlStrcmp(param->name, (const xmlChar *)"iconList"))) {
									xmlNodePtr iconNode = xmlFirstElementChild(param);
									xmlNodePtr iconParam = xmlFirstElementChild(iconNode);
									for (; iconParam; iconParam = xmlNextElementSibling(iconParam)) {
										if ((!xmlStrcmp(iconParam->name, (const xmlChar *)"url"))) {
											app->iconPath = (char*)xmlNodeGetContent(iconParam);
										}
									}
								}
							}
							app->next = server->appList;
							server->appList = app;
						}
					}
					xmlFreeDoc(doc2);
				}
			}
		}
	}
	xmlFreeDoc(doc);
	return 0;
}

uint8_t launch_application_parse(remote_server *server, char *content) {
	xmlDocPtr doc = xmlParseDoc((const xmlChar *)content);
	xmlNodePtr root = xmlDocGetRootElement(doc);
	if ((!xmlStrcmp(root->name, (const xmlChar *)"Envelope"))) {
		xmlNodePtr envelope = xmlFirstElementChild(root);
		if ((!xmlStrcmp(envelope->name, (const xmlChar *)"Body"))) {
			xmlNodePtr body = xmlFirstElementChild(envelope);
			if ((!xmlStrcmp(body->name, (const xmlChar *)"LaunchApplicationResponse"))) {
				xmlNodePtr launchApplicationResponse = xmlFirstElementChild(body);
				if ((!xmlStrcmp(launchApplicationResponse->name, (const xmlChar *)"AppURI"))) {
					char *appURI = (char*)xmlNodeGetContent(launchApplicationResponse);

					// TODO: other protocols
					if (sscanf(appURI, "VNC://%[^:]:%hu", server->vnc_ip, &server->vnc_port) == 2) {
						printf("aunch_application_parse: %s, parsed: %s, %d\n", appURI, server->vnc_ip, server->vnc_port);
					} else
						return 1;
				}
			}
		}
	}
	xmlFreeDoc(doc);
	return 0;
}

uint8_t set_client_profile_parse(remote_server *server, char *content) {
	(void) server;
	(void) content;
	return 0;
}

void remote_server_destory(remote_server *server) {
	uint8_t i;
	if (!server) {
		return;
	}
	free(server->ip);
	free(server->uuid);
	for (i = 0; i < SERVICE_TYPE_MAX; i++) {
		free(server->sinfo[i].surl);
		free(server->sinfo[i].curl);
		free(server->sinfo[i].eurl);
	}
	free(server);
}
