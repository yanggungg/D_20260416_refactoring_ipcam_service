/**
 * @file nf_onvif_ns.c
 * @brief ONVIF namespace 및 공통 API.
 * @author Jae-young Kim
 * @date 2012-02-07
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */

#ifndef __NF_ONVIF_NAMESPACE_C__
#define __NF_ONVIF_NAMESPACE_C__

#include <stdsoap2.h>
#include <onvifH.h>

#include <nf_ipcam_defs.h>

#include <libxml/xmlreader.h>

#include <nf_util_time.h>

static pthread_mutex_t _gsoap_ssl_mtx = PTHREAD_MUTEX_INITIALIZER;

SOAP_NMAC struct Namespace namespaces[] =
{
    {"SOAP-ENV", "http://www.w3.org/2003/05/soap-envelope", "http://schemas.xmlsoap.org/soap/envelope/", NULL},
    {"SOAP-ENC", "http://www.w3.org/2003/05/soap-encoding", "http://schemas.xmlsoap.org/soap/encoding/", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
    {"chan", "http://schemas.microsoft.com/ws/2005/02/duplex", NULL, NULL},
    {"wsa5", "http://www.w3.org/2005/08/addressing", "http://schemas.xmlsoap.org/ws/2004/08/addressing", NULL},
    {"c14n", "http://www.w3.org/2001/10/xml-exc-c14n#", NULL, NULL},
    {"wsu", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd", NULL, NULL},
    {"xenc", "http://www.w3.org/2001/04/xmlenc#", NULL, NULL},
    {"wsc", "http://schemas.xmlsoap.org/ws/2005/02/sc", NULL, NULL},
    {"ds", "http://www.w3.org/2000/09/xmldsig#", NULL, NULL},
    {"wsse", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd", "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd", NULL},
    {"xmime", "http://tempuri.org/xmime.xsd", NULL, NULL},
    {"xop", "http://www.w3.org/2004/08/xop/include", NULL, NULL},
    {"tt", "http://www.onvif.org/ver10/schema", NULL, NULL},
    {"wsrfbf2", "http://docs.oasis-open.org/wsrf/bf-2", NULL, NULL},
    {"wsnt1", "http://docs.oasis-open.org/wsn/t-1", NULL, NULL},
    {"wsrfr2", "http://docs.oasis-open.org/wsrf/r-2", NULL, NULL},
    {"analytics1", "http://www.onvif.org/ver20/analytics/wsdl/RuleEngineBinding", NULL, NULL},
    {"analytics2", "http://www.onvif.org/ver20/analytics/wsdl/AnalyticsEngineBinding", NULL, NULL},
    {"va", "http://www.onvif.org/ver20/analytics/wsdl", NULL, NULL},
    {"device", "http://www.onvif.org/ver10/device/wsdl", NULL, NULL},
    {"devio", "http://www.onvif.org/ver10/deviceIO/wsdl", NULL, NULL},
    {"event2", "http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding", NULL, NULL},
    {"event3", "http://www.onvif.org/ver10/events/wsdl/EventBinding", NULL, NULL},
    {"event", "http://www.onvif.org/ver10/events/wsdl", NULL, NULL},
    {"event4", "http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding", NULL, NULL},
    {"event5", "http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding", NULL, NULL},
    {"event6", "http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding", NULL, NULL},
    {"event7", "http://www.onvif.org/ver10/events/wsdl/PullPointBinding", NULL, NULL},
    {"event8", "http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding", NULL, NULL},
    {"event9", "http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding", NULL, NULL},
    {"wsnb2", "http://docs.oasis-open.org/wsn/b-2", NULL, NULL},
    {"imaging", "http://www.onvif.org/ver20/imaging/wsdl", NULL, NULL},
    {"media", "http://www.onvif.org/ver10/media/wsdl", NULL, NULL},
    {"media2", "http://www.onvif.org/ver20/media/wsdl", NULL, NULL},
    {"ptz", "http://www.onvif.org/ver20/ptz/wsdl", NULL, NULL},
    {NULL, NULL, NULL, NULL}
};

struct soap_handle
{
	int ch;
	struct soap *soap;
};

extern int soap_wsse_add_UsernameTokenDigestWithManulTime
(struct soap*, const char*, const char*, const char*, time_t);
extern int soap_wsse_add_UsernameTokenDigest
(struct soap*, const char*, const char*, const char*);
extern int soap_wsse_add_UsernameTokenText
(struct soap*, const char*, const char*, const char*);

/**
 * @brief soap struct를 초기화한다.(현재는 timeout설정만 함)
 * @param ovf_soap Soap 구조체.
 */
extern void nf_onvif_soap_init_set(struct soap *ovf_soap)
{
	ovf_soap->connect_timeout = 10;
	ovf_soap->send_timeout = 2;
	ovf_soap->recv_timeout = 5;
}

extern void soap_handle_init_and_set_auth(const int ch, struct soap_handle *handle)
{
	handle->ch = ch;
	handle->soap = soap_new();
	_nf_onvif_add_auth2(ch, handle->soap);
	handle->soap->connect_timeout = 10;
	handle->soap->send_timeout = 2;
	handle->soap->recv_timeout = 5;
}

/**
 * @brief 카메라의 시간을 조회한다.(digest 인증 용도)
 * @param endpoint ONVIF device xaddr.
 * @return 카메라 시간.
 */
extern time_t _nf_onvif_get_time(const char* endpoint)
{
	mtable* runtime = get_runtime();

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _device__GetSystemDateAndTime _req;
	struct _device__GetSystemDateAndTimeResponse _res;
	struct tm _tm;

	time_t cam_time;

	int rtn = soap_call___device__GetSystemDateAndTime(soap, endpoint , NULL, &_req, &_res);


    if(rtn != 0 ||
            _res.SystemDateAndTime == NULL ||
            _res.SystemDateAndTime->UTCDateTime == NULL ||
            _res.SystemDateAndTime->UTCDateTime->Time == NULL ||
            _res.SystemDateAndTime->UTCDateTime->Date == NULL)
    {/*
        printf("[%s:%d] rtn[%d] _res.SystemDateAndTime[%p] endpoint[%s]\n", __func__, __LINE__, rtn, _res.SystemDateAndTime, endpoint);
        if(rtn == 0){
            if(_res.SystemDateAndTime){
                printf("[%s:%d] UTCDateTime[%p]\n", __func__, __LINE__, _res.SystemDateAndTime->UTCDateTime);
                if(_res.SystemDateAndTime->UTCDateTime){
                    printf("[%s:%d] Time[%p] Date[%p]\n", __func__, __LINE__, _res.SystemDateAndTime->UTCDateTime->Time, _res.SystemDateAndTime->UTCDateTime->Date);
                }
            }
        }
		*/

		cam_time = 0;
    }
    else
	{
		memset(&_tm, 0x00, sizeof(struct tm));
		_tm.tm_sec   = _res.SystemDateAndTime->UTCDateTime->Time->Second;
		_tm.tm_min   = _res.SystemDateAndTime->UTCDateTime->Time->Minute;
		_tm.tm_hour  = _res.SystemDateAndTime->UTCDateTime->Time->Hour;
		_tm.tm_mday  = _res.SystemDateAndTime->UTCDateTime->Date->Day;
		_tm.tm_mon   = _res.SystemDateAndTime->UTCDateTime->Date->Month-1;
		_tm.tm_year  = _res.SystemDateAndTime->UTCDateTime->Date->Year-1900;

		cam_time = mktime(&_tm);
		tzset();
		cam_time -= timezone;
	}

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return cam_time;
}

extern time_t _nf_onvif_get_time2(const int ch)
{
	int rtn = 0;
	struct soap *soap;
	char *endpoint;
	mtable* runtime = get_runtime();
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(ERROR, "\n%s\n", buf);
			goto ends_label;
		}
	}

	struct _device__GetSystemDateAndTime _req;
	struct _device__GetSystemDateAndTimeResponse _res;
	struct tm _tm;

	time_t cam_time;

	rtn = soap_call___device__GetSystemDateAndTime(soap, endpoint , NULL, &_req, &_res);

	if(rtn != 0 ||
	        _res.SystemDateAndTime == NULL ||
	        _res.SystemDateAndTime->UTCDateTime == NULL ||
	        _res.SystemDateAndTime->UTCDateTime->Time == NULL ||
	        _res.SystemDateAndTime->UTCDateTime->Date == NULL)
	{
		/*
	    printf("[%s:%d] rtn[%d] _res.SystemDateAndTime[%p] endpoint[%s]\n", __func__, __LINE__, rtn, _res.SystemDateAndTime, endpoint);
	    if(_res.SystemDateAndTime){
		    printf("[%s:%d] UTCDateTime[%p]\n", __func__, __LINE__, _res.SystemDateAndTime->UTCDateTime);
			if(_res.SystemDateAndTime->UTCDateTime){
				printf("[%s:%d] Time[%p] Date[%p]\n", __func__, __LINE__, rtn, _res.SystemDateAndTime->UTCDateTime->Time, _res.SystemDateAndTime->UTCDateTime->Date);
			}
		}
		*/

		cam_time = 0;
	}
	else
	{
		memset(&_tm, 0x00, sizeof(struct tm));
		_tm.tm_sec   = _res.SystemDateAndTime->UTCDateTime->Time->Second;
		_tm.tm_min   = _res.SystemDateAndTime->UTCDateTime->Time->Minute;
		_tm.tm_hour  = _res.SystemDateAndTime->UTCDateTime->Time->Hour;
		_tm.tm_mday  = _res.SystemDateAndTime->UTCDateTime->Date->Day;
		_tm.tm_mon   = _res.SystemDateAndTime->UTCDateTime->Date->Month-1;
		_tm.tm_year  = _res.SystemDateAndTime->UTCDateTime->Date->Year-1900;

		cam_time = mktime(&_tm);
		tzset();
		cam_time -= timezone;
	}
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return cam_time;
}

// NVR,DVR의 시간을 기준으로 카메라와의 time sync를 위해 시간을 설정
// GMT를 넘겨준다.
extern int nf_onvif_set_time_sync(const int ch)
{
	int rtn;
	char* endpoint;
	time_t timer, gmt_timer;
	struct tm *t;
	struct tm *gmt;

	//NF_TZINFO_ACTIVEX tzout;
	int gmt_diff;
	int diff_hour, diff_min;
	gchar key_tz[64];
	gchar _tz_str[64];
	gchar key_dst[64];
	guint _tz;
	guint _dst;
	snprintf(key_tz, 64, "sys.date.tz_index");
	snprintf(key_dst, 64, "sys.date.daylight");
	_tz = nf_sysdb_get_uint(key_tz);
	_dst = nf_sysdb_get_bool(key_dst);
	snprintf(_tz_str, 64, "%s", nf_zoneinfo_get_string(_tz));

	// convert posix format
	//printf("[khkh] (1) _tz_str : (%s) \n", _tz_str);
	//nf_zoneinfo_get_activex_posix_onvif(_tz_str, &tzout);
	//printf("[khkh] (2) _tz_str : (%s), tzout (%s) \n", _tz_str, tzout.tz_arr[0].string);

	struct tt__Time _time;
	struct tt__Date _date;
	mtable* runtime = get_runtime();

	
	// WS Security 
	struct soap_handle handle;
	soap_handle_init_and_set_auth(ch, &handle);

	// set endpoint
	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	// get time from DVR/NVR(localtime) 
	timer = time(NULL);
	t = localtime(&timer);

	/*
	_date.Year = t->tm_year + 1900;
	_date.Month = t->tm_mon + 1;
	_date.Day = t->tm_mday;

	_time.Hour = t->tm_hour;
	_time.Minute = t->tm_min;
	_time.Second = t->tm_sec;
	*/

	//printf("[khkh] Local Time : %d-%d-%d , %d:%d:%d \n",
//			t->tm_year+1900, t->tm_mon +1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);


	// Make GMT
	gmt_timer = time(NULL);
	gmt = gmtime(&gmt_timer);

	//printf("[khkh] GMT Time : %d-%d-%d , %d:%d:%d \n",
//			gmt->tm_year+1900, gmt->tm_mon +1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
	_date.Year = gmt->tm_year + 1900;
	_date.Month = gmt->tm_mon + 1;
	_date.Day = gmt->tm_mday;

	_time.Hour = gmt->tm_hour;
	_time.Minute = gmt->tm_min;
	_time.Second = gmt->tm_sec;

	
	// _onvif_set_time call
	rtn = _onvif_set_time(&handle, endpoint, _dst, _tz_str ,_date, _time);
	return rtn;
}

extern int _onvif_set_time(const struct soap_handle* handle, const char* endpoint, int daylight, const char* timezone, const struct tt__Date date, const struct tt__Time time)
{
	struct soap *soap = NULL;
	//soap = soap_new();
	//nf_onvif_soap_init_set(soap);
	soap = handle->soap;

	struct _device__SetSystemDateAndTime _req;
	struct _device__SetSystemDateAndTimeResponse _res;

	struct tt__TimeZone tz;
	struct tt__DateTime utc;

	// ex) tz.TZ = "JST-9";
	//tz.TZ = timezone;  // TimeZone 일단 보류
	memset(&tz, 0x0, sizeof(tz));

	utc.Time = &time;
	utc.Date = &date;
	//_req.TimeZone = &tz;
	_req.TimeZone = NULL;
	_req.UTCDateTime = &utc;


	_req.DateTimeType = tt__SetDateTimeType__Manual;

	if(daylight==1)
		_req.DaylightSavings = xsd__boolean__true_;
	else
		_req.DaylightSavings = xsd__boolean__false_;


	int rtn = soap_call___device__SetSystemDateAndTime(soap, endpoint, NULL, &_req, &_res);

	if(rtn != 0)
	{
		printf("[khkh] (%d) failed to set sync time ... \n", handle->ch);
	}
	else // 성공 OK 
	{

		//printf("[khkh] (%d) Success to set sync time ... \n", handle->ch);
	}

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn; // check
}

// 모든 채널에 연결된 카메라의 time_sync를 맞춰주는 함수
// return 값 0 - Success , 그외 Fail
extern int nf_onvif_set_time_sync_all()
{
	int i =0;
	int rtn = 0;
	int tmp = 0;
	mtable* runtime = NULL;
	runtime = get_runtime();

	struct tm *t;
	time_t timer;
	timer = time(NULL);
	t = localtime(&timer);


	printf("(%s)(%d) Synchronizes the time of the currently connected camera, [%d-%d-%d, %d:%d:%d]\n\n",
			__func__, __LINE__,
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);

	for(i=0; i<32; i++)
	{
		// MGMT_STATE_READY, OPENMODE_STATE_READY 둘 다 아닐 경우, 즉 연결이 안된 채널인 경우 패스
		if((runtime[i].state & MGMT_STATE_READY)==0 && (runtime[i].state & OPENMODE_STATE_READY)==0)
		{
			//printf("[khkh] CH(%) is not READY \n");
			continue;
		}
		else
		{
			//printf("[khkh] CH(%d) is ready , conn_type : %d \n", i, runtime[i].conn_type);

			if(runtime[i].conn_type==0){
				tmp = nf_ipcam_set_time_info(i);
				if(tmp==0)
				{
					//printf("[khkh] CH(%d) failed to call , rtn :(%d)  (%s)... \n", i,rtn, __func__);
				}
				else
				{
					//printf("[khkh] success ... \n");
				}
			}
			else
			{
				tmp = nf_onvif_set_time_sync(i);
				if(tmp)
				{	
					//printf("[khkh] CH(%d) failed to call , rtn :(%d)  (%s)... \n", i,rtn, __func__);
					rtn = tmp;
				}else
				{
				//printf("[khkh] success ... \n");
				}
			}
		}
	}	

	return rtn;
}

/**
 * @brief soap struct에 사용자 인증 정보를 추가한다.
 * @param _soap soap 구조체.
 * @param auth 인증 방식. None, Text, Digest.
 * @param user 사용자 ID.
 * @param pass 사용자 password.
 * @param endpoint ONVIF device xaddr(시간 조회용).
 * @return 0 - 성공, 기타 - 실패.
 */
extern int _nf_onvif_add_auth(struct soap *_soap, int auth, const char* user, const char* pass, const char* endpoint)
{
	struct timespec now_time;
	int rtn;

	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		/*clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Auth: TEXT\n",
				D_SECS(now_time),
				__FUNCTION__
				);*/
		rtn = soap_wsse_add_UsernameTokenText(_soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		/*clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Auth: DIGEST\n",
				D_SECS(now_time),
				__FUNCTION__
				);*/
		time_t cam_time;
		cam_time = _nf_onvif_get_time(endpoint);

		if (cam_time != 0)
		{
			rtn = soap_wsse_add_UsernameTokenDigestWithManualTime(_soap, NULL, user, pass, cam_time);
		}
		else
		{
			rtn = soap_wsse_add_UsernameTokenDigest(_soap, NULL, user, pass);
		}
	}
	else
	{
		/*clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);*/
		rtn = 0;
	}

	return rtn;
}

extern int _nf_onvif_add_auth2(const int ch, const struct soap *_soap)
{
	int auth;
	char *user, *pass;
	mtable* runtime = NULL;
	runtime = get_runtime();
	struct timespec now_time;
	int rtn;

	auth = runtime[ch].onvif.auth_method;
	user = runtime[ch].username;
	pass = runtime[ch].password;

	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		rtn = soap_wsse_add_UsernameTokenText(_soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		time_t cam_time;
		cam_time = _nf_onvif_get_time2(ch);

		if (cam_time != 0)
		{
			rtn = soap_wsse_add_UsernameTokenDigestWithManualTime(_soap, NULL, user, pass, cam_time);
		}
		else
		{
			rtn = soap_wsse_add_UsernameTokenDigest(_soap, NULL, user, pass);
		}
	}
	else
	{
		rtn = 0;
	}

	return rtn;
}

/**
 * @brief Soap ssl 초기화를 실시한다.
 * @param soap soap 구조체.
 * @param a
 * @param b
 * @param c
 * @param d
 * @param e
 * @param f
 */
extern int SOAP_SSL_CLIENT_CONTEXT(struct soap* soap, unsigned short a, const char* b, const char* c, const char* d, const char* e, const char* f)
{
	//pthread_mutex_lock(&_gsoap_ssl_mtx);
	return  soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
	//pthread_mutex_unlock(&_gsoap_ssl_mtx);
}

/**
 * @brief gsoap ssl의 multithread safe문제를 해결하기 위한 lock.(S1에만 적용)
 */
extern void nf_onvif_mtx_lock()
{
	pthread_mutex_lock(&_gsoap_ssl_mtx);
}

/**
 * @brief gsoap ssl의 multithread safe문제를 해결하기 위한 unlock.(S1에만 적용)
 */
extern void nf_onvif_mtx_unlock()
{
	pthread_mutex_unlock(&_gsoap_ssl_mtx);
}

/**
 * @brief Xml 문자열에서 특정 Name의 Value/Type 값을 조회한다.
 * @param[in] input_xml_str 입력 Xml 문자열.
 * @param[in] attr_nm 조회할 Name 속성값.
 * @param[out] output_attr_val 대상 Value/Type 문자열.
 * @param[in] remove_prefix 1 - namespace관련 prefix를 삭제함.('tt:Simpleitem'에서 'tt:' prefix 삭제)
 * @return 1 - 조회 성공, 0 - 실패.
 *
 * @deprecated Metadata 종류가 다양해져 Value/Type으로 조회 불가능.
 * @see nf_onvif_metadata_parser
 */
extern int nf_get_xml_val_from_name(const char* input_xml_str, const char* attr_nm, char* output_attr_val, int remove_prefix)
{
	xmlTextReaderPtr reader;
	xmlChar *name = NULL, *value = NULL;
	int i = 0, len = 0, rtn, found = 0;

	// FIXME remove namespace prefix manually..
	char prefix_removed[META_STREAM_LEN_MAX];
	memset(prefix_removed, 0x00, META_STREAM_LEN_MAX);
	char *x1 = NULL, *x2 = NULL;

	if(remove_prefix)
	{
		for(i = 0; i < strlen(input_xml_str); i++)
		{
			if(input_xml_str[i] == '<')
			{
				x1 = &input_xml_str[i];
			}
			if(input_xml_str[i] == '/' && i > 0 && input_xml_str[i-1] == '<')
			{
				x1 = &input_xml_str[i];
			}
			if(input_xml_str[i] == ':')
			{
				x2 = &input_xml_str[i];
				if(x1 != NULL)
				{
					len -= (x2 - x1 - 1);
					x1 = NULL;
					x2 = NULL;
					continue;
				}
			}
			prefix_removed[len] = input_xml_str[i];
			len++;
		}
		prefix_removed[len] = '\0';
	}
	else
	{
		snprintf(prefix_removed, META_STREAM_LEN_MAX, "%s", input_xml_str);
		//memcpy(prefix_removed, input_xml_str, strlen(input_xml_str));
	}

	//printf("%s | converted string : %s\n", __FUNCTION__, prefix_removed);
	reader = xmlReaderForMemory(prefix_removed, strlen(prefix_removed), NULL, NULL, 0);
	if(reader == NULL)
	{
		printf("%s | %s is not xml string!!\n", __FUNCTION__, input_xml_str);
		return 0;
	}

	rtn = xmlTextReaderRead(reader);
	if(rtn < 0)
	{
		printf("%s | %s parsing error!!\n", __FUNCTION__, input_xml_str);
		return 0;
	}

	while(rtn == 1)
	{
		if(xmlTextReaderHasAttributes(reader) == 1)
		{
			name = xmlTextReaderGetAttribute(reader, (xmlChar *)"Name");
			if(xmlStrEqual(name, (xmlChar *)attr_nm))
			{
				value = xmlTextReaderGetAttribute(reader, (xmlChar *)"Value");
				if(value)
				{
					found = 1;
					memcpy(output_attr_val, value, strlen(value));
					break;
				}

				value = xmlTextReaderGetAttribute(reader, (xmlChar *)"Type");
				if(value)
				{
					found = 1;
					memcpy(output_attr_val, value, strlen(value));
					break;
				}
			}
			if(name)
			{
				free(name);
				name = NULL;
			}
		}
		rtn = xmlTextReaderRead(reader);
	}
	if(name)
	{
		free(name);
		name = NULL;
	}
	if(value)
	{
		free(value);
		value = NULL;
	}

	xmlFreeTextReader(reader);

	return found;
}

#endif	//__NF_ONVIF_NAMESPACE_C__
