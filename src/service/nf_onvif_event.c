/**
 * @file nf_onvif_event.c
 * @brief ONVIF Event 구현.
 * @author
 * @date 16/07/2012
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */

#ifndef __NF_ONVIF_EVENT_C__
#define __NF_ONVIF_EVENT_C__

#include <stdsoap2.h>
#include <onvifH.h>

#include "wsaapi.h"
#include <nf_ipcam_defs.h>

#if 0
#define NF_ONVIF_DBG printf
#else
#define NF_ONVIF_DBG while(0) printf
#endif
#define NF_ONVIF_MAJOR printf
#define D_SECS(a) ((a).tv_sec),(((a).tv_nsec)/1000)


/* Static msg exchange funcs */
ONVIF_MSG _nf_onvif_event_get_event_properties(ipcam_onvif_auth_info_t *auth_info, const char *endpoint);
ONVIF_MSG _nf_onvif_event_create_pullpoint_subscription(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, char *xaddr);
ONVIF_MSG _nf_onvif_event_pull_messages(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* xaddr_event, int* bmotion, int* balarm, int* time_remain);
ONVIF_MSG _nf_onvif_event_renew(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* xaddr_event);
ONVIF_MSG _nf_onvif_event_unsubscribe(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, char* xaddr_event);

/* -------------   ONVIF related APIs for the IPX scenario   --------------- */


/**
 * @brief ONVIF event pullpoint를 생성한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_create_event(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), -1);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT] != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	_nf_onvif_event_get_event_properties(&auth_info, endpoint);

	memset(runtime[ch].onvif.xaddr_event, 0x00, 128);
	rtn = _nf_onvif_event_create_pullpoint_subscription(
			&auth_info,	endpoint, runtime[ch].onvif.xaddr_event);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) CreatePullPointSubsciption not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: CreatePullPointSubsciption REQ error(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					rtn
					);
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

/**
 * @brief ONVIF event를 pull해온다.
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 *
 * Timeout 시간이 가까워지면 renew를 통해 pullpoint를 갱신한다.
 */
ONVIF_API nf_onvif_get_event(int ch)
{
	char endpoint[256], xaddr_event[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0, bmotion = 0, balarm = 0, time_remain = 0;

	ipcam_onvif_auth_info_t auth_info;

#if 0
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_MAJOR("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
#endif
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), -1);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT] != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.xaddr_event != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT]);
	memset(xaddr_event, 0x00, 256);
	strcpy(xaddr_event, runtime[ch].onvif.xaddr_event);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];


	rtn = _nf_onvif_event_pull_messages(
			&auth_info,	endpoint, xaddr_event, &bmotion, &balarm, &time_remain);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) PullMessages not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: PullMessages REQ error(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					rtn
					);
		}
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(bmotion == 1)
		{
			runtime[ch].motion_flag = 90;
			NF_ONVIF_MAJOR("[%lu.%06lu] \033[1;40;32m%s\033[0m | ch(%d) Motion Enable!! \n",
								D_SECS(now_time),
								__FUNCTION__, ch
								);
		}
		else if (bmotion == 2)
		{
		}
		else if (bmotion == 3)
		{
			NF_ONVIF_MAJOR("[%lu.%06lu] \033[1;40;32m%s\033[0m | ch(%d) Motion Enable!! OFFFF!! \n",
								D_SECS(now_time),
								__FUNCTION__, ch
								);
			runtime[ch].motion_flag = 3;
		}
		else
		{
			NF_ONVIF_MAJOR("[%lu.%06lu] \033[1;40;32m%s\033[0m | ch(%d) Motion OFFFF!! \n",
								D_SECS(now_time),
								__FUNCTION__, ch
								);
			runtime[ch].motion_flag = 0;
		}
		if(balarm)
		{
			NF_ONVIF_MAJOR("[%lu.%06lu] \033[1;40;32m%s\033[0m | ch(%d) Alarm OPEN!! \n",
								D_SECS(now_time),
								__FUNCTION__, ch
								);
			runtime[ch].alarm_flag = 1;
		}

		if(time_remain != 0 && time_remain < 10)
		{
			rtn = _nf_onvif_event_renew(
					&auth_info,	endpoint, xaddr_event);
		}
	}

#if 0
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_MAJOR("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
#endif
	return rtn;

}

/**
 * @brief ONVIF event pullpoint를 삭제한다.(event 오류가 계속 발생할 시)
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_delete_event(int ch)
{
	char endpoint[256], xaddr_event[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;
	
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), -1);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT] != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.xaddr_event != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT]);
	memset(xaddr_event, 0x00, 256);
	strcpy(xaddr_event, runtime[ch].onvif.xaddr_event);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_event_unsubscribe(
			&auth_info,	endpoint, xaddr_event);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) Unsubscribe not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: Unsubscribe REQ error(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					rtn
					);
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;

}

/* -------------   ONVIF MSG exchanging methods - Internal use   --------------- */
ONVIF_MSG _nf_onvif_event_get_event_properties(ipcam_onvif_auth_info_t *auth_info, const char *endpoint)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _event__GetEventProperties req;
	struct _event__GetEventPropertiesResponse res;

	char *action = "GetEventProperties";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	rtn = soap_call___event3__GetEventProperties(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}

	if(res.__sizeTopicNamespaceLocation > 0)
	{
		for(i = 0; i < res.__sizeTopicNamespaceLocation; i++)
		{
			NF_ONVIF_DBG("%s | %d topicnamespacelocation %s\n", __FUNCTION__, i, res.TopicNamespaceLocation[i]);
		}
	}
	if(res.wsnt1__TopicSet != NULL)
	{
		if(res.wsnt1__TopicSet->documentation != NULL)
		{
#if 0
			if(res.wsnt1__TopicSet->documentation->__mixed != NULL)
				NF_ONVIF_DBG("%s | topicset->doc->mixed %s\n",  __FUNCTION__, res.wsnt1__TopicSet->documentation->__mixed);
#endif
			if(res.wsnt1__TopicSet->documentation->__size > 0)
			{
				for(i = 0; i < res.wsnt1__TopicSet->documentation->__size; i++)
				{
					NF_ONVIF_DBG("%s | topicset->doc->any %d %s\n", __FUNCTION__, i, res.wsnt1__TopicSet->documentation->__any[i]);
				}
			}
		}
#if 0
		if(res.wsnt1__TopicSet->__anyAttribute != NULL)
			NF_ONVIF_DBG("%s | topicset->anyattr %s\n", __FUNCTION__, res.wsnt1__TopicSet->__anyAttribute);
#endif

		if(res.wsnt1__TopicSet->__size > 0)
		{
			for(i = 0; i < res.wsnt1__TopicSet->__size; i++)
			{
				NF_ONVIF_DBG("%s | topicset->any %d %s\n", __FUNCTION__, i, res.wsnt1__TopicSet->__any[i]);
			}
		}
	}


	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GetEventProperties Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

static void _get_address_head(char* addr, char* head)
{
	gchar *s, *e;
	const gchar *find_https = "https://";
	const gchar *find_http = "http://";
	gchar *find_str = NULL;

	if (addr == NULL) { return ; }
	if (head == NULL) { return ; }

	if (nf_ipcam_is_vendor_s1())
	{
		find_str = find_https;
	}
	else
	{
		find_str = find_http;
	}

	s = addr;
	e = strstr(addr, find_str);
	if (e == NULL)
	{
		return ;
	}
	e += strlen(find_str);
	e = strstr(e, "/");
	if (e == NULL)
	{
		return ;
	}
	e++;

	memset(head, 0x00, 128);
	strncpy(head, s, (e-s));
}

static void _get_address_tail(char* addr, char* tail)
{
	gchar *s, *e;
	const gchar *find_https = "https://";
	const gchar *find_http = "http://";
	gchar *find_str = NULL;

	if (addr == NULL) { return ; }
	if (tail == NULL) { return ; }

	if (nf_ipcam_is_vendor_s1())
	{
		find_str = find_https;
	}
	else
	{
		find_str = find_http;
	}

	s = strstr(addr, find_str);
	if (s == NULL)
	{
		return ;
	}
	s += strlen(find_str);
	s = strstr(s, "/");
	if (s == NULL)
	{
		return ;
	}
	s++;

	strcpy(tail, s);
}
ONVIF_MSG _nf_onvif_event_create_pullpoint_subscription(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, char* xaddr)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _event__CreatePullPointSubscription req;
	struct _event__CreatePullPointSubscriptionResponse res;

	char *action = "CreatePullPointSubscription";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _event__CreatePullPointSubscription));

	req.InitialTerminationTime = soap_malloc(soap, 20);
	strcpy(req.InitialTerminationTime, "PT60S");


	rtn = soap_call___event3__CreatePullPointSubscription(soap, endpoint, NULL, &req, &res);


	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	{
		if(res.SubscriptionReference.Address != NULL)
		{
			char addr_head[128];
			char addr_tail[128];
			NF_ONVIF_DBG("%s | addr %s\n", __FUNCTION__, res.SubscriptionReference.Address);
			/*if(nf_ipcam_is_vendor_s1())
			{
				char *http_ptr;
				snprintf(xaddr, 128, "https://");
				http_ptr = strstr(res.SubscriptionReference.Address, "http://");
				if (http_ptr != NULL)
				{
					http_ptr += 7;
					strcat(xaddr, http_ptr);
				}
				else
				{
					strncpy(xaddr, res.SubscriptionReference.Address, 128);
				}
			}
			else*/
			{
				memset(addr_head, 0x00, 128);
				_get_address_head(endpoint, addr_head);
				_get_address_tail(res.SubscriptionReference.Address, addr_tail);
				strncpy(xaddr, addr_head, strlen(addr_head));
				strcat(xaddr, addr_tail);
				//strncpy(xaddr, res.SubscriptionReference.Address, 128);
			}

			//strncpy(xaddr, res.SubscriptionReference.Address, 127);
		}
		if(res.SubscriptionReference.Metadata != NULL)
		{

		}
		if(res.SubscriptionReference.ReferenceParameters != NULL)
		{

		}

		NF_ONVIF_DBG("%s | ct %lu\n", __FUNCTION__, res.wsnb2__CurrentTime);
		NF_ONVIF_DBG("%s | tt %lu\n", __FUNCTION__, res.wsnb2__TerminationTime);
	}

	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | CreatePullPointSubscription Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_event_pull_messages(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* xaddr_event,  int* bmotion, int* balarm, int* time_remain)
{
	int i = 0, j = 0;
	int rtn = 0, found = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _event__PullMessages req;
	struct _event__PullMessagesResponse res;

	char *action = "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesRequest";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _event__PullMessages));
	req.Timeout = 10;
	req.MessageLimit = 10;

	const char *RequestMessageID = soap_wsa_rand_uuid(soap);

	soap_wsa_request(soap, RequestMessageID, xaddr_event, action);
	//soap_wsa_request(soap, NULL, endpoint, action);

	//rtn = soap_call___event2__PullMessages(soap, endpoint, NULL, &req, &res);
	rtn = soap_call___event2__PullMessages(soap, xaddr_event, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	{
		NF_ONVIF_DBG("%s | ct %lu\n", __FUNCTION__, res.CurrentTime);
		NF_ONVIF_DBG("%s | tt %lu\n", __FUNCTION__, res.TerminationTime);
		*time_remain = res.TerminationTime - res.CurrentTime;

		if(res.__sizeNotificationMessage > 0)
		{
			int _temp[res.__sizeNotificationMessage];
			int _first,_last;
			*bmotion = 0;
			*balarm = 0;
			for(i = 0; i < res.__sizeNotificationMessage; i++)
			{
				struct wsnb2__NotificationMessageHolderType* msg = &res.wsnb2__NotificationMessage[i];
				_temp[i] = (-1);
				if(msg->Topic != NULL)
				{
					if(msg->Topic->Dialect != NULL)
					{
						NF_ONVIF_DBG("%s | %d dialect %s\n", __FUNCTION__, i, msg->Topic->Dialect);
					}
				}
#if 0
				if(msg->Message.__any)
				{
					NF_ONVIF_DBG("%s | %d message %s\n", __FUNCTION__, i, msg->Message.__any);

					char value_in_msg[32];

					found = nf_get_xml_val_from_name(msg->Message.__any, "MotionActive", value_in_msg, 1);
					if(found == 1)
					{
						if(strstr(value_in_msg, "true") || strstr(value_in_msg, "1"))
						{
							_temp[i] = 1;
							*bmotion = 1;
						}
						if(strstr(value_in_msg, "false") || strstr(value_in_msg, "0"))
						{
							_temp[i] = 0;
							*bmotion = 0;
						}
					}

					found = nf_get_xml_val_from_name(msg->Message.__any, "State", value_in_msg, 1);
					if(found == 1)
					{
						if(strstr(value_in_msg, "true") || strstr(value_in_msg, "1"))
						{
							_temp[i] = 1;
							*bmotion = 1;
						}
						if(strstr(value_in_msg, "false") || strstr(value_in_msg, "0"))
						{
							_temp[i] = 0;
							*bmotion = 0;
						}
					}


					if(strstr(msg->Message.__any, "State=\"close") != NULL)
					{
						*balarm = 1;
					}
				}
#endif
			}
			_first = (-1);
			_last = (-1);
			for (i=0; i<res.__sizeNotificationMessage; i++)
			{
				if (_temp[i] < 0) { continue; }
				if (_first < 0) { _first = _temp[i]; _last = _temp[i]; }
				else { _last = _temp[i]; }
			}

			if (_first == 1 && _last == 0)
			{
				// motion ON -> OFF
				*bmotion = 3;
			}
		}
		else
		{
			*bmotion = 2;
		}
	}

motion_event_done:
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | PullMessages Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_event_renew(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* xaddr_event)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _wsnb2__Renew req;
	struct _wsnb2__RenewResponse res;

	char *action = "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewRequest";

	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _wsnb2__Renew));
	req.TerminationTime = (char *)soap_malloc(soap, 10);
	strcpy(req.TerminationTime, "PT30S");

	const char *RequestMessageID = soap_wsa_rand_uuid(soap);

	soap_wsa_request(soap, RequestMessageID, xaddr_event, action);
	//soap_wsa_request(soap, NULL, endpoint, action);

	//rtn = soap_call___event4__Renew(soap, endpoint, NULL, &req, &res);
	rtn = soap_call___event4__Renew(soap, xaddr_event, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	{
		//NF_ONVIF_DBG("%s | tt %lu\n", __FUNCTION__, res.wsnb2__TerminationTime);
	}

	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Renew Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = 0;

	return rtn;
}

ONVIF_MSG _nf_onvif_event_unsubscribe(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, char* xaddr_event)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _wsnb2__Unsubscribe req;
	struct _wsnb2__UnsubscribeResponse res;

	char *action = "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeRequest";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _wsnb2__Unsubscribe));

	const char *RequestMessageID = soap_wsa_rand_uuid(soap);

	soap_wsa_request(soap, RequestMessageID, xaddr_event, action);

	//rtn = soap_call___event4__Unsubscribe(soap, endpoint, NULL, &req, &res);
	rtn = soap_call___event4__Unsubscribe(soap, xaddr_event, NULL, &req, &res);


	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}


	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Unsubscribe Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

#endif //__NF_ONVIF_EVENT_C__

