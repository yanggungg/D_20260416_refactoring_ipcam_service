#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <gobjmedia.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// #include <gst/nf/gstnfbuddybuffer.h>

#include <jansson.h>

#include "nf_common.h"
#include "nf_util_time.h"
#include "nf_util_netif.h"
//#include "nf_action.h"
//#include "nf_webra.h"
#include "nf_api_ipcam.h"
//#include "nf_webra_def.h"
#include "nf_codec_header.h"

#include "onvif_common.h"
#include "nvs_onvif_app_util.h"
#include "../cross_host/onvif_cross_util.h"
#include "nf_util_function.h"
#include "nf_ipcam_zmq_utils.h"
#include "itx_ai_def.h"
#include "nf_api_dva_eventlog.h"

#include "nf_meta_data.h"
#include "../../service/nf_issm_ctl_live.h"

#define MAX_UNIT_MSG_SIZE	(4096)
#define MAX_NOTI_MSG_CNT	(16) // MAX_NOTIFY_BUFLEN / MAX_UNIT_MSG_SIZE. This is approximative value.
#define MAX_NOTIFY_DATA_LEN (1024*32)
#define MAX_NOTIFY_BUFLEN	(1024*64)
#define MAX_NOTIFY_H_BUFLEN	(1024*64)

static unsigned char onvif_mraw_data[ONVIF_CH][1024]={0,};
static char send_raw_data[128]={0,};

//#define ONVIF_METADATA_DEBUG

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	extern guint _nf_action_num_alarm;
#endif

typedef enum _NF_ONVIF_PROP_E
{
	PROP_0,
	PROP_SENSOR,
	PROP_MOTION,
	PROP_USER_ALARM,
	PROP_VLOSS,	
	PROP_ALARM,
	PROP_LOG,
	
	PROP_ANALOG_REC,
	PROP_IPCAM_REC,

	PROP_ENC_STATUS,
	PROP_NET_STATUS,	
	PROP_NET_RXTX,
	
	PROP_FS_STATUS,
	PROP_DISK_FULL,
	PROP_DISK_OVERWR,	
	PROP_DISK_USAGE,
	PROP_DISK_SMART,
	
	PROP_SYSDB_CAM_TITLE,
	PROP_SYSDB_COVERT,
	PROP_SYSDB_PTZ,
	PROP_SYSDB_TIME_FORMAT,
	PROP_SYSDB_TIME_ZONE,

	PROP_GUI_SETUP,
	PROP_SYSDB_CHANGE,
	PROP_TIME_CHANGE,
	
	PROP_DVR_STATUS,
	PROP_DVR_WDTIMER,

#ifdef ENABLE_PND_PROP
	PROP_PND_EVENT,
	PROP_PND_PROGRESS,
#endif

#ifdef ENABLE_IPCAM_PROP
	PROP_IPCAM_SENSOR,		
	PROP_IPCAM_MOTION,		
	PROP_IPCAM_USER_ALARM,	
	PROP_IPCAM_VLOSS,		
	PROP_IPCAM_ALARM,	
	PROP_IPCAM_ENC_STATUS,
#endif

	PROP_DISK_WRITE_FAIL,
	PROP_DISK_EXHAUST,
	PROP_DISK_NODISK,
	PROP_DISK_SMART_REQCHK,
	
	PROP_SYS_FAN,	
	PROP_SYS_TEMPERATURE,	
	PROP_SYS_POE_STATUS,
	
	PROP_DVR_LOGIN_FAIL,
	PROP_NET_LOGIN_FAIL,	
	
	PROP_NET_WAN_STATUS,
	PROP_NET_DDNS_STATUS,
	PROP_MOTION_RAW_DATA,
				
	PROP_SYS_BOOTING,	
	PROP_BUZZER,

	PROP_LOGIN_USER,
	PROP_NET_IP_CHANGED,
	
	PROP_SYS_POE_PORT,
#if defined(_HDI_0412) || defined(_HDY_0818)|| defined(_HDY_1618)
	PROP_STD_TYPE,
#endif
	
	PROP_SIGNAL_STATUS,

#if defined(ENABLE_EVENT_TAMPER)
	PROP_EVENT_TAMPER,
#endif
	PROP_SYSDB_TMP_CHANGE,
	PROP_VA,
	PROP_VA_COUNTER,
	PROP_AI,
	PROP_AI_COUNTER,
	LAST_PROP
	/* FILL ME */
} NF_ONVIF_PROP;

typedef enum _ONVIF_META_TYPE_
{
	ONVIF_VA_DATA,
	ONVIF_VA_EVENT_DATA,
}ONVIF_META_TYPE;

typedef struct __META_RECV_MANAGER__
{
	int is_run;
	GThread *thread_id;
	GAsyncQueue *meta_queue;
} META_RECV_MANAGER;

typedef struct __EVENT_RECV_MANAGER__
{
	int is_run;
	GThread *thread_id;
	GAsyncQueue *event_queue;
} EVENT_RECV_MANAGER;

typedef struct _ONVIF_SHAPE_STRUCT_
{
	double bbox[4];
	double center_gravity[2];
}ONVIF_SHAPE;

typedef struct _ONVIF_CLASS_STRUCT_
{
	char class_type[64];
	double likelihood;
}ONVIF_CLASS;

typedef struct _ONVIF_VA_META_
{
	int object_id;
	char topic;
	double trans_x;
	double trans_y;
	double scale_x;
	double scale_y;
	
	ONVIF_SHAPE va_shape;
	ONVIF_CLASS va_class;
}ONVIF_VA_META;

typedef struct _ONVIF_VA_META_FRAME_{
	unsigned int ch;
	unsigned int meta_count;
	unsigned long timestamp;
	unsigned int timestampl;

	ONVIF_VA_META meta_data[VA_META_DATA_MAX];
}ONVIF_VA_META_FRAME;

static pthread_mutex_t		g_event_msgbuf_mutex;
static pthread_mutex_t		g_event_subscription_mutex;
static pthread_mutex_t		g_event_notify_data;

struct event_subscription	g_onvif_event_sub[MAX_SUBSCRIPTION_CNT];
ONVIF_EVENT_MSGBUF	g_onvif_event_msgbuf;
static GAsyncQueue *meta_queue = NULL;
static GAsyncQueue *event_queue = NULL;

extern GAsyncQueue *onvif_get_meta_queue();
extern GAsyncQueue *onvif_get_event_queue();

static void dlva_metadata_thread_func (META_RECV_MANAGER *self);
static void ai_generic_event_thread_func (EVENT_RECV_MANAGER *self);

static gboolean nf_dva_metadata_send(VA_EVT_DATA *p_data);
static gboolean nf_onvif_va_data_send(VA_EVT_DATA *p_data);


const char* get_topic_name(unsigned int key);
static void get_macaddress(char *tmp_mac);
#if defined (DVR_BUILT_IN_VA)
static int nf_onvif_make_dva_msg(DVA_MSG *msg, char *data);
#endif

#define METADATA_SIZE (1024)
// Base64
static const char MimeBase64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static int DecodeMimeBase64[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 00-0F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 10-1F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  // 20-2F 
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  // 30-3F 
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  // 40-4F 
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  // 50-5F 
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  // 60-6F 
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  // 70-7F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 80-8F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 90-9F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // A0-AF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // B0-BF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // C0-CF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // D0-DF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // E0-EF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   // F0-FF 
};

unsigned int copy_from_event_subscription_list(struct event_subscription	* dest)
{
	memset(dest, 0x00, sizeof(struct event_subscription)*MAX_SUBSCRIPTION_CNT);
	LOCK_onvif_event_subs();
	memcpy(dest, &g_onvif_event_sub, sizeof(struct event_subscription)*MAX_SUBSCRIPTION_CNT);
	UNLOCK_onvif_event_subs();
	return 1;	
}


void onvif_event_mem_init()
{
	int i = 0;
	
	pthread_mutex_init(&g_event_msgbuf_mutex, NULL);
	memset(&g_onvif_event_msgbuf, 0x00, sizeof(EVENT_MSGBUF_SHM));	

	for ( i =0; i < MAX_MSG_BUF; i++) {
		g_onvif_event_msgbuf.msg_buf[i].ss_id = INVALID_SSID;
	}
	
	pthread_mutex_init(&g_event_subscription_mutex, NULL);
	memset(g_onvif_event_sub, 0x00, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);

	for ( i =0; i < MAX_SUBSCRIPTION_CNT; i++) {
		g_onvif_event_sub[i].ss_id = INVALID_SSID;
		g_onvif_event_sub[i].is_first_req = 1;
	}

	pthread_mutex_init(&g_event_notify_data, NULL);
}

#define METADATA_SIZE (1024)

char* make_buf_metadata_head(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<tt:MetadataStream" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tt=\"http://www.onvif.org/ver10/schema\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tns1=\"http://www.onvif.org/ver10/topics\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tnsitx=\"http://www.itxsecurity.com/onvif/event0/topics\">\n");
	p = stpcpy(p, tmp);

	return p;
}

char *make_buf_videoanalytics(char *send_buf, struct e_msg *msg, unsigned int msg_cnt)
{
	char tmp[128] = {0, }, tmp_time[128] = {0, }, *p;
	int i=0;
	double x=ONVIF_ROI_W, y=ONVIF_ROI_H;
	x = x/2.0;
	y = y/2.0;

	p = send_buf;
	/* make event messages */
	for (i = 0; i < (int)msg_cnt; i++)
	{
		sprintf(tmp, "<tt:VideoAnalytics>");
		p = stpcpy(p, tmp);
		strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
		sprintf(tmp, "<tt:Frame UtcTime=\"%sZ\">", tmp_time);
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:Transformation>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:Translation x=\"%d\" y=\"%d\"/>", Translation_X, Translation_Y);
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:Scale x=\"%lf\" y=\"%lf\"/>", 1.0/x, -1.0/y);
		p = stpcpy(p, tmp);
		sprintf(tmp, "</tt:Transformation>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:Extension>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:MotionInCells Columns=\"%d\" Rows=\"%d\" Cells=\"%s\"/>",ONVIF_ROI_W, ONVIF_ROI_H, send_raw_data);
		p = stpcpy(p, tmp);
		sprintf(tmp, "</tt:Extension>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</tt:Frame>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</tt:VideoAnalytics>\n");
		p = stpcpy(p, tmp);
	}	
	return p;
}

char* make_buf_metadata_event_head(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "<tt:Event>\n");
	p = stpcpy(p, tmp);
	
	return p;
}

char* make_buf_metadata_tail(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "</tt:MetadataStream>\n");
	p = stpcpy(p, tmp);
	
	return p;
}

char* make_buf_metadata_event_tail(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "</tt:Event>\n");
	p = stpcpy(p, tmp);
	
	return p;
}

char* make_buf_metadata_va_head(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "<tt:VideoAnalytics>\n");
	p = stpcpy(p, tmp);
	
	return p;
}
char* make_buf_metadata_va_tail(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "</tt:VideoAnalytics>\n");
	p = stpcpy(p, tmp);
	
	return p;
}

char* make_base64json_va_buf_notification_no_dest(char *send_buf, char *dva_msg, int key, int msg_cnt)
{
	char tmp_time[128] = {0, }, *p;
	char *tmp = NULL;
	int i=0;
	unsigned int time_tick = 0;
	struct sysinfo info;
	time_t now;
	
	 sysinfo(&info);
	 now = time(NULL);
	 time_tick = now;

	tmp = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if ( tmp == NULL )
		return NULL;
	p = send_buf;
	/* make event messages */
	for (i = 0; i < (int)msg_cnt; i++)
	{
		sprintf(tmp, "<wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">");
		p = stpcpy(p, tmp);
		if ( key == E_KEY_DVA_EVENT )
			sprintf(tmp, "%s", "tnsitx:ITX/VAEventdata");
		else if ( key == E_KEY_DVA )
			sprintf(tmp, "%s", "tnsitx:ITX/Metadata");
		else
			sprintf(tmp, "%s", "tnsitx:ITX/Metadata");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Topic>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Message>");
		p = stpcpy(p, tmp);

		//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh			
		switch( key ){
			case E_KEY_DVA_EVENT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", "Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"ITX Metadata\">", "DLVA_V1.0");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Base64JSON\">", dva_msg);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_DVA:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", "Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"ITX Metadata\">", "DLVA_V1.0");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Base64JSON\">", dva_msg);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
			default:
				break;
		}			
		sprintf(tmp, "</wsnt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
	}
	if ( tmp )
		free(tmp);
	
	return p;
}

char* make_buf_notification_no_dest(char *send_buf, struct e_msg *msg, unsigned int msg_cnt)
{
	char tmp[128] = {0, }, tmp_time[128] = {0, }, *p;
	int i=0;


	p = send_buf;
	/* make event messages */
	for (i = 0; i < (int)msg_cnt; i++)
	{
		sprintf(tmp, "<wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">");
		p = stpcpy(p, tmp);
		sprintf(tmp, "%s", get_topic_name(msg[i].key));
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Topic>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Message>");
		p = stpcpy(p, tmp);

		//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh			
		switch( msg[i].key ){
			case E_KEY_SENSOR:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%d\" Name=\"Port\">", msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].alarm.value?"close":"open");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_ITX_MOTION:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"MotionActive\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION0:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_analytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea0\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);
				break;
			case E_KEY_VIDEOANALYTICS_MOTION1:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_analytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea1\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION2:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_analytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea2\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION3:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_analytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea3\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_MOTION_ALARM:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_DIGITAL_INPUT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s%d\" Name=\"InputToken\">", DIGITAL_INPUT_TOKEN_PREFIX, msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"LogicalState\">", msg[i].alarm.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_RELAY:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s%d\" Name=\"InputToken\">", RELAY_TOKEN_PREFIX, msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"LogicalState\">", msg[i].alarm.value?"Active":"Inactive");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_VIDEO_SIGNAL_LOSS:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;		
			case E_KEY_EVENT_CONFIGURATION_CHANGE:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_ADVANCED_SENSOR:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%d\" Name=\"Port\">", msg[i].ad_alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				if( msg[i].ad_alarm.state == SENSOR_ACTIVE ){
					sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", "close");
				}
				else if( msg[i].ad_alarm.state == SENSOR_INACTIVE ){
					sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", "open");
				}
				else if( msg[i].ad_alarm.state == SENSOR_SABOTAGE ){
					sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", "sabotage");
				}
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_CHANGE_PROFILE:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
					sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Token\">", msg[i].prof_change.token);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_CHANGE_CONFIGURATION:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Token\">", msg[i].config_change.token);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
					p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Type\">", msg[i].config_change.type);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
//ONVIF_NEW_EVENT				
#if 0
			case E_KEY_ONVIF_NEW_EVENT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].new.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].new.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);
				break;
#endif
			default:
				break;
		}			
		sprintf(tmp, "</wsnt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
	}
	
	return p;
}

static void dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] ", str , 
					pheader->chan, 
					pheader->flags, 
					pheader->frame_type, 
					pheader->frame_rate, 
					pheader->resolution,
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size	 );
	g_message("%s", pheader->gst_buffer);
}

unsigned int get_ch_from_emsg(struct e_msg *emsg)
{
	unsigned int ch = 0;
	
	switch ( emsg->key  ) {
		case E_KEY_SENSOR:
			ch = emsg->alarm.ch;
			break;
		case E_KEY_ITX_MOTION:
			ch = emsg->motion.ch;
			break;
		case E_KEY_VIDEOANALYTICS_MOTION0:
			ch = emsg->motion.ch;
			break;
		case E_KEY_VIDEOANALYTICS_MOTION1:
			ch = emsg->motion.ch;
			break;
		case E_KEY_VIDEOANALYTICS_MOTION2:
			ch = emsg->motion.ch;
			break;
		case E_KEY_VIDEOANALYTICS_MOTION3:
			ch = emsg->motion.ch;
			break;
		case E_KEY_MOTION_ALARM:
			ch = emsg->motion.ch;
			break;
		case E_KEY_RECORDING_JOB_STATE_CHANGE:
			break;
		case E_KEY_CONFIGURATION_CHANGE:
			break;
		case E_KEY_DATA_DELETION:
			break;
		case E_KEY_RECORDING_AND_TRACK:
			break;
		case E_KEY_DIGITAL_INPUT:
			ch = emsg->alarm.ch;
			break;
		case E_KEY_RELAY:
			ch = emsg->alarm.ch;			
			break;					
		case E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE:
			ch = emsg->image.ch;
			break;
		case E_KEY_VIDEO_SIGNAL_LOSS:
			ch = emsg->image.ch;			
			break;
		case E_KEY_EVENT_CONFIGURATION_CHANGE:
			break;
		case E_KEY_ADVANCED_SENSOR:
			ch = emsg->ad_alarm.ch;
			break;						
//ONVIF_NEW_EVENT
#if 0
		case E_KEY_ONVIF_NEW_EVENT:
			/* Filter check */
			break;
#endif
		default:
			break;
	}

	return ch;
}
	
gboolean	convert_to_emsg_from_data(struct e_msg *emsg, unsigned int key, void *data, unsigned int state, int ss_id, time_t now, unsigned int uptime)
{

	emsg->key = key;
	emsg->state = state;
	emsg->ss_id = ss_id;
	emsg->time_tick = now;
	emsg->uptime_tick = uptime;
	
	switch ( emsg->key  ) {
		case E_KEY_SENSOR:
			memcpy(&emsg->alarm, data, sizeof(struct e_alarm_type));
			break;
		case E_KEY_ITX_MOTION:
			memcpy(&emsg->motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_MOTION_ALARM:
			memcpy(&emsg->motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION0:
			memcpy(&emsg->motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION1:
			memcpy(&emsg->motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION2:
			memcpy(&emsg->motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION3:
			memcpy(&emsg->motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_RECORDING_JOB_STATE_CHANGE:
			/* Filter check */
			break;
		case E_KEY_CONFIGURATION_CHANGE:
			/* Filter check */
			break;
		case E_KEY_DATA_DELETION:
			/* Filter check */
			break;
		case E_KEY_RECORDING_AND_TRACK:
			/* Filter check */
			break;
		case E_KEY_DIGITAL_INPUT:
			/* Filter check */
			memcpy(&emsg->alarm, data, sizeof(struct e_alarm_type));
			break;
		case E_KEY_RELAY:
			/* Filter check */
			memcpy(&emsg->relay, data, sizeof(struct e_relay_type));			
			break;
		case E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE:
			memcpy(&emsg->image, data, sizeof(struct e_image_type));
			break;
		case E_KEY_VIDEO_SIGNAL_LOSS:
			memcpy(&emsg->image, data, sizeof(struct e_image_type));
			break;			
		case E_KEY_EVENT_CONFIGURATION_CHANGE:
			memcpy(&emsg->event_conf, data, sizeof(struct e_event_config_type));
			break;
		case E_KEY_ADVANCED_SENSOR:
			memcpy(&emsg->ad_alarm, data, sizeof(struct e_ad_alarm_type));
			break;
		case E_KEY_CHANGE_PROFILE:
			memcpy(&emsg->prof_change, data, sizeof(struct e_profile_change_type));
			break;
		case E_KEY_CHANGE_CONFIGURATION:
			memcpy(&emsg->config_change, data, sizeof(struct e_configuration_change_type));
			break;
//ONVIF_NEW_EVENT
#if 0
		case E_KEY_ONVIF_NEW_EVENT:
			/* Filter check */
			break;
#endif
		default:
			return 0;
	}
	return 1;
}

static gboolean _nf_rec_metadata_handoff( guint ch, gpointer frame, gchar * buf )
{
	gint                clen, ret, stream_id;
	GobjBuddyBuffer        *gst_buf;
	ICODEC_HEADER       *pheader;

	g_return_val_if_fail( ch < NUM_TOTAL_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	pheader = (ICODEC_HEADER       *)frame;
	
//	clen = ALIGN( gint, sizeof(ICODEC_HEADER)+ pheader->frame_size, 64 );
	clen = sizeof(ICODEC_HEADER)+ pheader->frame_size;

	gst_buf = (GobjBuddyBuffer *)gobj_buddy_buffer_new_malloc(clen);

	#if 1 
		if(gst_buf == NULL)
		{    
			g_warning("%s GST_BUFFER_FAILED ch[%d] size[%d]", __FUNCTION__,ch, clen);
			return 0;
		}    
	#else
		g_assert(gst_buf != NULL);
	#endif

	// GST_BUFFER_SIZE(gst_buf) = (guint)clen; 

	memcpy( gobj_buddy_buffer_buf_get_addr((GObject*)gst_buf), frame, (sizeof(ICODEC_HEADER)));
	memcpy( gobj_buddy_buffer_buf_get_addr((GObject*)gst_buf)+sizeof(ICODEC_HEADER), buf, pheader->frame_size );	


	pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr((GObject*)gst_buf);	
	pheader->gst_buffer = gst_buf;
	pheader->chan = (guchar)ch;
	// gst_buf->frame = pheader;

	//dump_icodec_header("handoff", pheader);
	
	nf_issm_ctl_put_frame(gst_buf);

	g_object_unref(gst_buf);
	 
	return 1;
}
gboolean nf_rec_metadata_handoff(unsigned int key, void *data, unsigned int state, int ss_id, time_t now, unsigned int uptime)
{
	ICODEC_HEADER       pheader;
	GTimeVal		req_timeval;
	
	// make icodec header
	memset(&req_timeval, 0x00, sizeof(GTimeVal));
	g_get_current_time(&req_timeval);
	
	pheader.codec = NF_CODEC_TYPE_METADATA;
	pheader.flags = 0;
	pheader.version = NF_CODEC_VERSION_1;
	pheader.frame_type = NF_FRAME_TYPE_METADATA;
	pheader.resolution = 0;
	pheader.frame_rate = NF_FPS_CR01;
	pheader.timestampl = (guchar)(req_timeval.tv_usec/1000/5);
	pheader.timestamp = (guint)req_timeval.tv_sec;
	pheader.gst_buffer = 0;	
	// make xml message
#if 0 //[[ hun_0140415_BEGIN -- del

		p = make_buf_head(send_buf);
		p = make_buf_notification_msg(p, emsg, msg_cnt, host_ip, tmp_port, ss_id);
		p = make_buf_tail(p);
		buflen = (unsigned int)(p - send_buf);		
#endif //]] hun_0140415_END -- del

	struct e_msg emsg;
	unsigned int buflen=0, ch=0;
	int ret = 0;
	char send_buf[MAX_NOTIFY_BUFLEN] = {0, }, *p=NULL;

	// convert to e_msg
	memset(&emsg, 0x00, sizeof(struct e_msg));
	ret = convert_to_emsg_from_data(&emsg, key, data, state, ss_id, now, uptime);
	if( ret == 0 ){
		return 0;
	}

	ch = get_ch_from_emsg(&emsg);

	// make notification message
	memset(send_buf, 0x00, sizeof(char)*MAX_NOTIFY_BUFLEN);
	p = make_buf_metadata_head(send_buf);

	if(emsg.key == E_KEY_VIDEOANALYTICS_MOTION0 || 
	   emsg.key == E_KEY_VIDEOANALYTICS_MOTION1 || 
	   emsg.key == E_KEY_VIDEOANALYTICS_MOTION2 || 
	   emsg.key == E_KEY_VIDEOANALYTICS_MOTION3)
	{
		p = make_buf_videoanalytics(p, &emsg, 1);
	}
	
	p = make_buf_metadata_event_head(p);	
	p = make_buf_notification_no_dest(p, &emsg, 1);
	p = make_buf_metadata_event_tail(p);	
	p = make_buf_metadata_tail(p);
	buflen = (unsigned int)(p - send_buf);

	pheader.chan = ch;
	pheader.frame_size = buflen;	

	_nf_rec_metadata_handoff(ch, (gpointer)&pheader, send_buf );	
}

/*
1. Description : add new subscription to event subsctiprion list.
2. IN 
 - Termination time
 - Filter option
3. OUT 
 - Subscription ID.
*/
void random_bytes( unsigned char *dest, int len );

int OV_event_set_subscription_is_first_req(int ss_id, int p_value)
{ 
	int i=0; 
	
	LOCK_onvif_event_subs();

	if ( !ss_id )
	{
		UNLOCK_onvif_event_subs();
		return -1;
	}
	else
	{
		for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
		{
			if ( g_onvif_event_sub[i].ss_id == ss_id )
			{
		 		g_onvif_event_sub[i].is_first_req = p_value;
				UNLOCK_onvif_event_subs();
				return 0;
			}
		}
	}
	UNLOCK_onvif_event_subs();
	
	/* fail */
	return -1;

}

typedef struct _notify_param {
    int id;
    char addr[128];
} notify_param;
notify_param func_d;

char* make_http_header(char *send_buf, char *uri, char *tmp_str_addr, unsigned int tmp_port, unsigned int buflen)
{
	char tmp[128] = {0, }, *p;

	p = send_buf;
	/* make HTTP header */
	sprintf(tmp, "POST %s HTTP/1.1\r\n", uri);
	p = stpcpy(p, tmp);
	sprintf(tmp, "Host: %s:%d\r\n", tmp_str_addr, tmp_port);
	p = stpcpy(p, tmp);
	sprintf(tmp, "Content-Type: application/soap+xml; charset=utf-8; action=\"http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify\"\r\n");
	p = stpcpy(p, tmp);
	sprintf(tmp, "Connection: keep-alive\r\n");
	p = stpcpy(p, tmp);
	sprintf(tmp, "Content-Length: %d\r\n", buflen);
	p = stpcpy(p, tmp);
	sprintf(tmp, "SOAPAction: \"http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify\"\r\n\r\n");
	p = stpcpy(p, tmp);

    return p;
}

//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh
//ONVIF_NEW_EVENT
//const char *str_new_topic = "tns1:UserAlarm/tnsitx:AlarmIN\0";
const char* get_topic_name(unsigned int key)
{
	switch( key ){
        case E_KEY_SENSOR:
            return str_tns1_tnsitx_Alarm;
        case E_KEY_ITX_MOTION:
            return str_tns1_tnsitx_Motion;
        case E_KEY_MOTION_ALARM:
            return str_tns1_General_Motion;
		case E_KEY_VIDEOANALYTICS_MOTION0:
			return str_tns1_VideoAnalytics_Motion;
		case E_KEY_VIDEOANALYTICS_MOTION1:
			return str_tns1_VideoAnalytics_Motion;
		case E_KEY_VIDEOANALYTICS_MOTION2:
			return str_tns1_VideoAnalytics_Motion;
		case E_KEY_VIDEOANALYTICS_MOTION3:
			return str_tns1_VideoAnalytics_Motion;
        case E_KEY_RECORDING_JOB_STATE_CHANGE:
            return str_tns1_tnsitx_jobState_change;
        case E_KEY_DIGITAL_INPUT:
            return str_tns1_digital_input;
        case E_KEY_RELAY:
            return str_tns1_relay;
        case E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE:
            return str_tns1_GlobalSceneChange;
        case E_KEY_VIDEO_SIGNAL_LOSS:
            return str_tns1_VideoSignalLoss;
        case E_KEY_EVENT_CONFIGURATION_CHANGE:
            return str_tns1_tnsitx_EventConfChange;
        case E_KEY_ADVANCED_SENSOR:
            return str_tns1_tnsitx_AdvancedSensor;
        case E_KEY_CHANGE_PROFILE:
            return str_tns1_ProfileChange;
        case E_KEY_CHANGE_CONFIGURATION:
            return str_tns1_ConfigurationChange;
//ONVIF_NEW_EVENT
//      case E_KEY_ONVIF_NEW_EVENT:
//          return str_new_topic;
        default:
            return str_tns1_tnsitx_Alarm;
    }
}

char* make_buf_notification_msg(char *send_buf, struct e_msg *msg, unsigned int msg_cnt, char* host_ip, unsigned int tmp_port, int ss_id)
{
	char tmp[128] = {0, }, tmp_time[128] = {0, }, *p;
	int i=0;

	p = send_buf;
	/* make event messages */
	for (i = 0; i < (int)msg_cnt; i++)
	{
		sprintf(tmp, "<wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:SubscriptionReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "http://%s:%d/onvif/subscription?subscription=%d", host_ip, tmp_port, ss_id );
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:SubscriptionReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">");
		p = stpcpy(p, tmp);
		sprintf(tmp, "%s", get_topic_name(msg[i].key));
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Topic>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:ProducerReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "http://%s:%d/onvif/subscription?subscription=%d", host_ip, tmp_port, ss_id );
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:ProducerReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Message>");
		p = stpcpy(p, tmp);
			

		//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh			
		switch( msg[i].key ){
			case E_KEY_SENSOR:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%d\" Name=\"Port\">", msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].alarm.value?"close":"open");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_ITX_MOTION:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"MotionActive\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION0:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vanalytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea0\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION1:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vanalytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea1\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION2:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vanalytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea2\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_VIDEOANALYTICS_MOTION3:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vsource%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"ab_vanalytics%d\" Name=\"VideoAnalyticsConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"MotionArea3\" Name=\"Rule\">");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"IsMotion\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_MOTION_ALARM:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_DIGITAL_INPUT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s%d\" Name=\"InputToken\">", DIGITAL_INPUT_TOKEN_PREFIX, msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"LogicalState\">", msg[i].alarm.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_RELAY:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s%d\" Name=\"InputToken\">", RELAY_TOKEN_PREFIX, msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"LogicalState\">", msg[i].alarm.value?"Active":"Inactive");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].image.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].image.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_VIDEO_SIGNAL_LOSS:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].image.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].image.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_EVENT_CONFIGURATION_CHANGE:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"0x%X\" Name=\"State\">", msg[i].event_conf.next);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;					
            case E_KEY_CHANGE_PROFILE:
                strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
                sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
                p = stpcpy(p, tmp);
                sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Token\">", msg[i].prof_change.token);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
                p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
                break;
            case E_KEY_CHANGE_CONFIGURATION:
                strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
                sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
                p = stpcpy(p, tmp);
                sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Token\">", msg[i].config_change.token);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
                p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"Type\">", msg[i].config_change.type);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
                p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);
				break;					
			case E_KEY_ADVANCED_SENSOR:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:SimpleItem Value=\"%d\" Name=\"Port\">", msg[i].ad_alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<tt:Data>");
				p = stpcpy(p, tmp);
				if( msg[i].ad_alarm.state == SENSOR_ACTIVE ){
					sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", "close");
				}
				else if( msg[i].ad_alarm.state == SENSOR_INACTIVE ){
					sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", "open");
				}
				else if( msg[i].ad_alarm.state == SENSOR_SABOTAGE ){
					sprintf(tmp, "<tt:SimpleItem Value=\"%s\" Name=\"State\">", "sabotage");
				}				
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</tt:Message>");
				p = stpcpy(p, tmp);				
				break;
//ONVIF_NEW_EVENT				
#if 0
			case E_KEY_ONVIF_NEW_EVENT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", msg[i].state?"Changed":"Initialized", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].new.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].new.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);
				break;
#endif
			default:
				break;
		}			
		sprintf(tmp, "</wsnt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
	}
	
	return p;
}

char* make_buf_head(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<SOAP-ENV:Envelope" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns5=\"http://docs.oasis-open.org/wsrf/bf-2\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xmime=\"http://tempuri.org/xmime.xsd\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xop=\"http://www.w3.org/2004/08/xop/include\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns7=\"http://docs.oasis-open.org/wsn/t-1\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tt=\"http://www.onvif.org/ver10/schema\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns4=\"http://docs.oasis-open.org/wsrf/r-2\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns10=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns11=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns12=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns13=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns14=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns15=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns16=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns9=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns1=\"http://www.onvif.org/ver10/events/wsdl\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tnsitx=\"http://www.itxsecurity.com/onvif/event0/topics\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tns1=\"http://www.onvif.org/ver10/topics\">");
	p = stpcpy(p, tmp);
	sprintf(tmp, " <SOAP-ENV:Header>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<wsa5:Action>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify");
	p = stpcpy(p, tmp);
	sprintf(tmp, "</wsa5:Action>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "</SOAP-ENV:Header>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<SOAP-ENV:Body>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<wsnt:Notify>");
	p = stpcpy(p, tmp);

	return p;
}

char* make_buf_tail(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "</wsnt:Notify>");
	p = stpcpy(p, (const char*)tmp);
	sprintf(tmp, "</SOAP-ENV:Body>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "</SOAP-ENV:Envelope>\n\n");
	p = stpcpy(p, tmp);

	return p;
}

void __w3curlparse(const char *szurl,
                   char *szprotocol,
                   char *szuser,
                   char *szpassword,
                   char *szaddress,
                   unsigned long *nport,
                   char *szuri )
{
    char szport[1024] = "\0";
    unsigned long npos = 0;
    int bflag = false;

    while (strlen(szurl) > 0 && npos < strlen(szurl) && strncmp((szurl + npos), ":", 1))
        ++npos;

    if (!strncmp((szurl + npos + 1), "/", 1))
    {    // is protocol
        if (szprotocol)
        {
            strncpy(szprotocol, szurl, npos);
            szprotocol[npos] = 0;
        }
        bflag = true;
    }
    else
    {    // is host
        if (szprotocol)
        {
            strncpy(szprotocol, "http", 4);
            szprotocol[5] = '\0';
        }
    }

    unsigned long nsp = 0, usp = 0;

    if (bflag)
    {
        usp = nsp = npos += 3;
    }
    else
    {
        usp = nsp = npos = 0;
    }

    while (strlen(szurl) > 0 && usp < strlen(szurl) && strncmp((szurl + usp), "@", 1))
        ++usp;

    if (usp < strlen(szurl))
    { // find username and find password
        unsigned long ssp = nsp;
        while (strlen(szurl) > 0 && npos < strlen(szurl) && strncmp((szurl + ssp), ":", 1))
            ++ssp;

        if (ssp < usp)
        { // find
            strncpy(szuser, szurl + nsp, ssp - nsp);
            szuser[ssp - nsp + 1] = '\0';
            strncpy(szpassword, szurl + ssp + 1, usp - ssp - 1);
            szpassword[usp - ssp] = '\0';
        }

        nsp = npos = usp + 1;
    }

    bflag = false;
    while (strlen(szurl) > 0 && npos < strlen(szurl) && strncmp((szurl + npos), "/", 1))
        ++npos;

    unsigned long nf = nsp;

    for (;nf <= npos;nf++)
    {
        if (!strncmp((szurl + nf), ":", 1))
        { // find PORT
            bflag = true;
            break;
        }
    }

    if (bflag)
    {
        char sztmp[1024] = "\0";
	strncpy(sztmp, (szurl + nf + 1), npos - nf);	
        *nport = (unsigned long)atol(sztmp);
        strncpy(szaddress, (szurl + nsp), nf - nsp);
    }
    else if (!strncmp(szprotocol, "https", strlen("https")))
    {
        *nport = (unsigned long)443;
        strncpy(szaddress, (szurl + nsp), npos - nsp);
    }
    else if (!strncmp(szprotocol, "ftp", strlen("fps")))
    {
        *nport = (unsigned long)23;
        strncpy(szaddress, (szurl + nsp), npos - nsp);
    }
    else
    {
        *nport = (unsigned long)80;
        strncpy(szaddress, (szurl + nsp), npos - nsp);
    }
    if (npos < strlen(szurl))
    { // find URI
        strncpy(szuri, (szurl + npos), strlen(szurl) - npos);
    }
    else
    {
        szuri[0] = '/';
        szuri[1] = '\0';
    }

    return ;
}

int sendn(int socket, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while(nleft > 0) {
		if((nwritten = send(socket, ptr, nleft, MSG_DONTWAIT)) < 0) {
			OV_DEBUG("%s:%d error : %s\n", __FUNCTION__, __LINE__, strerror(errno));
			if (errno == EAGAIN  || errno == EWOULDBLOCK ) {
				usleep(500*1000);
				continue;				
			}
			else{
				if(nleft == n){
					OV_DEBUG("send error!![%d]", nleft);					
					return -1;
				}
				else
					break;				
			}
		}
		else if(nwritten == 0){
			break;
		}

		nleft -= (size_t)nwritten;
		ptr += nwritten;
	}

	return (int)(n-nleft);
}


static void notifier_func(void *arg)
{
	char c_addr[128] = {0,}, c_protocol[16] = {0,}, tmp_str_addr[128] = {0, }, uri[128] = {0, }, host_ip[COMMON_SIZE], send_buf[MAX_NOTIFY_BUFLEN] = {0, }, send_buf_h[MAX_NOTIFY_H_BUFLEN] = {0, }, tmp[256], *p;
	notify_param f;
	in_addr_t tmp_addr = 0;
	struct sockaddr_in server_addr;	
	int ss_id=0, client_socket=0, fail_cnt=0, nsent=0, nleft=0, ret=0;
	struct e_msg  emsg[MAX_NOTI_MSG_CNT];
	unsigned int msg_cnt = 0, tmp_port=0, buflen=0, buflen_h=0;

	memset(&f, 0x00, sizeof(notify_param));	
	LOCK_onvif_event_notify_data();
	memcpy(&f, (notify_param *)arg, sizeof(notify_param));
	memset(arg, 0x00, sizeof(notify_param));
	UNLOCK_onvif_event_notify_data();

	memset(c_addr, 0x00, sizeof(char)*128);
	memset(c_protocol, 0x00, sizeof(char)*16);    
	strncpy(c_addr, f.addr, sizeof(char)*128 - 1);	

	/*WARNNING: c_addr must have port number ex) 192.168.10.11:8080 */
	memset(tmp_str_addr, 0x00, sizeof(char)*128);
	memset(uri, 0x00, sizeof(char)*128);
	__w3curlparse(c_addr, c_protocol, NULL, NULL, tmp_str_addr, (unsigned long *)&tmp_port, uri);
	tmp_addr = inet_addr(tmp_str_addr);
	ss_id = f.id;

	printf("SKSHIN] notifier thread, ss_id = (%d)\n", ss_id);
	if (!ss_id) {
		printf("ss_id failed..");
		return;
	}
	if ( !tmp_port ){
		tmp_port = 8080;
	}
	if ( !tmp_addr ){
		tmp_addr = inet_addr("192.168.10.151");
	}

	memset(host_ip, 0x00, sizeof(char)*COMMON_SIZE);
	if (get_ipaddress(host_ip) < 0) {
		return;
	}

	
	/* Connect consumer server */
	client_socket = socket( PF_INET, SOCK_STREAM, 0);
	if ( -1 == client_socket) {
		return;
	}

	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((uint16_t )tmp_port);
	server_addr.sin_addr.s_addr = tmp_addr;

	/* wait for listening */
	while ( connect( client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 ) {
		if (fail_cnt <= 10)	{
			sleep(1);
			fail_cnt++;
		}
		else {
			printf("connection[%s:%d] failed..", tmp_str_addr, tmp_port);
			close(client_socket);
			return;
		}
	}

	memset(emsg, 0x00, sizeof(struct e_msg)*MAX_NOTI_MSG_CNT);

	while(1){
		printf("SKSHIN] notifier thread, ss_id = (%d)\n", ss_id);
		ret = OV_BLOCK_event_pull_messages(ss_id, emsg, MAX_NOTI_MSG_CNT, 60);
		if( ret < 0){
			usleep(50000);
			break; // timeout ...
		}
		if( ret == 0 ){
			usleep(500000); // 500 ms
			continue;
		}
		msg_cnt = (unsigned int)ret;

		/* Make a Message */
		memset(send_buf, 0x00, sizeof(char)*MAX_NOTIFY_BUFLEN);
		p = make_buf_head(send_buf);
		p = make_buf_notification_msg(p, emsg, msg_cnt, host_ip, tmp_port, ss_id);
		p = make_buf_tail(p);
		buflen = (unsigned int)(p - send_buf);		

		memset(send_buf_h, 0x00, sizeof(char)*MAX_NOTIFY_H_BUFLEN);
		p = make_http_header(send_buf_h, uri, tmp_str_addr, tmp_port, buflen);
		buflen_h = (unsigned int)(p - send_buf_h);

		ret = sendn( client_socket, send_buf_h, buflen_h);
		if( ret != (int)buflen_h )	{
			break;
		}

		ret = sendn( client_socket, send_buf, buflen);		
		if( ret != (int)buflen ) {
			break;
		}	

	}
	
	close(client_socket);
	
	
	return;
}



int OV_event_start_notify(int ss_id, char *dest_address)
{
    pthread_t tid = 0;

    memset(&func_d, 0x00, sizeof(notify_param));
    OV_DEBUG("create bn [%d][%s]", ss_id, dest_address);
    func_d.id = ss_id;
    memset(func_d.addr, 0x00, sizeof(char)*128);
    strncpy(func_d.addr, dest_address, sizeof(char)*128 - 1);

    if ( pthread_create(&tid, NULL, (void*)notifier_func, (void*)&func_d) != 0)
    {
        return 0;
    }

    pthread_detach(tid);
    return 1;
}



unsigned int get_all_current_ptz_enable(void)
{
	int i = 0;
	unsigned int flags = 0, protocol=0;
	char buff[128] = {0, };

	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		memset(buff, 0x00, sizeof(char)*128);
		snprintf(buff, sizeof(char)*128, "cam.ptz.P%d.protocol", i);
		protocol = nf_sysdb_get_uint(buff);
		if( protocol != 0 ){ //not diabled
			flags |= (1<<i);
		}
	}
	return flags;
}


unsigned int get_all_current_motion_enable(void)
{
	int i = 0;
	unsigned int flags = 0;
	char buff[128] = {0, };

	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		memset(buff, 0x00, sizeof(char)*128);
		snprintf(buff, sizeof(char)*128, "alarm.motion.M%d.act", i);
		if ( nf_sysdb_get_bool(buff) != 0 )
		{ //not diabled
			flags |= (1 << i);
		}
	}
	return flags;
}


unsigned int get_all_current_audio_enable(void)	
{
	int i = 0;
	unsigned int flags = 0;
	char buff[128] = {0, };

	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		memset(buff, 0x00, sizeof(char)*128);
		snprintf(buff, sizeof(char)*128, "rec.audio.A%d.stream.S0.enable", i);
		if ( nf_sysdb_get_bool(buff) != 0 )
		{ //not diabled
			flags |= (1 << i);
		}
	}
	return flags;
}

unsigned int get_all_current_tamper_enable(void)
{
	int i = 0;
	unsigned int flags = 0;
	char buff[128] = {0, };

	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		memset(buff, 0x00, sizeof(char)*128);
		snprintf(buff, sizeof(char)*128, "act.tamper.T%d.active", i);
		if ( nf_sysdb_get_uint(buff) != 0 )
		{ //not diabled
			flags |= (1 << i);
		}
	}
	return flags;
}
	
/*
1. Description : 
2. IN 
 - Subscription ID
3. OUT 
 - result
*/
int OV_event_set_sync(int ss_id)
{
	unsigned int	i=0;
	struct e_motion_type motion;
	struct e_alarm_type alarm;
	struct e_ad_alarm_type ad_alarm;	
	struct e_image_type image;
	struct e_event_config_type conf;	
	NF_NOTIFY_INFO *pnotify = NULL;

	if ( !ss_id ) {
		return -1;
	}

	pnotify = nf_notify_get("motion");
	if(pnotify) {
		for( i=0; i< NUM_ACTIVE_CH; i++){
			motion.ch = i;
			motion.value = (pnotify->d.params[0]>>i)&0x1;
			printf("SKSHIN] MOTION INITIALIZED, ss_id=(%u)\n", ss_id);
			append_onvif_event_msg(E_KEY_MOTION_ALARM, &motion, E_Initialized, ss_id);
			// append_onvif_event_msg(E_KEY_VIDEOANALYTICS_MOTION0, &motion, E_Initialized, ss_id);
			// append_onvif_event_msg(E_KEY_VIDEOANALYTICS_MOTION1, &motion, E_Initialized, ss_id);
			// append_onvif_event_msg(E_KEY_VIDEOANALYTICS_MOTION2, &motion, E_Initialized, ss_id);
			// append_onvif_event_msg(E_KEY_VIDEOANALYTICS_MOTION3, &motion, E_Initialized, ss_id);
		}
		nf_notify_free(pnotify);
	}

	pnotify = nf_notify_get("sensor");
	if(pnotify) {
		for( i=0; i< NUM_ALARM; i++){
			alarm.ch = ad_alarm.ch = i;
			alarm.value = (pnotify->d.params[0]>>i)&0x1;
			if( (pnotify->d.params[3]>>i)&0x1 ){
				ad_alarm.state = SENSOR_INACTIVE;
			}
			else{
				if ( (pnotify->d.params[2]>>i)&0x1 ) {					
					//sensor is active
					ad_alarm.state = SENSOR_ACTIVE;
				}
				else if ( (pnotify->d.params[1]>>i)&0x1 )  {
					//sensor is tampering
					ad_alarm.state = SENSOR_SABOTAGE;
				}
				else{
					//invalid value
					ad_alarm.state = SENSOR_INACTIVE;					
				}
			}
			append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Initialized, ss_id);
			append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Initialized, ss_id);
			append_onvif_event_msg(E_KEY_ADVANCED_SENSOR, &ad_alarm, E_Initialized, ss_id);			
			append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Initialized, ss_id);		
			append_onvif_event_msg(E_KEY_ADVANCED_SENSOR, &ad_alarm, E_Initialized, ss_id);			
		}
		nf_notify_free(pnotify);
	}
#if defined(ENABLE_EVENT_TAMPER)
	pnotify = nf_notify_get("tamper");
	if(pnotify) {
		for( i=0; i< NUM_ALARM; i++){
			image.ch = i;
			image.value = (pnotify->d.params[0]>>i)&0x1;
			append_onvif_event_msg(E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE, &image, E_Initialized, ss_id);
		}
		nf_notify_free(pnotify);
	}
#endif
	pnotify = nf_notify_get("vloss");
	if(pnotify) {
		for( i=0; i< NUM_ALARM; i++){
			image.ch = i;
			image.value = (pnotify->d.params[0]>>i)&0x1;
			append_onvif_event_msg(E_KEY_VIDEO_SIGNAL_LOSS, &image, E_Initialized, ss_id);
		}
		nf_notify_free(pnotify);
	}

	int ptz_mask = 0, motion_mask = 0, audio_mask = 0, tamper_mask=0;
	ptz_mask = get_all_current_ptz_enable();
	conf.category = PTZ_ON_OFF_CHANGED;
	conf.next = ptz_mask;
	append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Initialized, ss_id);	

	motion_mask = get_all_current_motion_enable();
	conf.category = MOTION_DETECTION_ON_OFF_CHANGED;
	conf.next = motion_mask;
	append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Initialized, ss_id);	
	
//Audio sync is disabled in UTM7GNE due to VX Tool (PELCO) PTZ issue
#if 0	
	audio_mask = get_all_current_audio_enable();
	conf.category = AUDIO_ON_OFF_CHANGED;
	conf.next = audio_mask;
	append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Initialized, ss_id);	
#endif

#if defined(ENABLE_EVENT_TAMPER)
	tamper_mask = get_all_current_tamper_enable();
	conf.category = TAMPER_DETECTION_ON_OFF_CHANGED;
	conf.next = tamper_mask;	
	append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Initialized, ss_id);	
#endif

/*
	tamper_mask = 0xffffff;
	conf.category = 0;
	conf.next = tamper_mask;	
	append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &conf, E_Initialized, ss_id);	
*/
	return 1;
}

int OV_event_set_sync_trt(int ss_id)
{
	struct e_event_config_type conf;	

	if ( !ss_id ) {
		return -1;
	}

	int ptz_mask = 0;
	ptz_mask = get_all_current_ptz_enable();
	conf.category = PTZ_ON_OFF_CHANGED;
	conf.next = ptz_mask;

	append_onvif_meta_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Initialized, -1);	

	return 1;
}

//////////////////////////////////////////////////////////////////

#define SEM_KEY         ((int)0x21) //key for semaphore
SEARCH_REC_SHM	rec_shm;

struct sembuf sb_lock = {0, -1, 0};
struct sembuf sb_unlock = {0, 1, 0};

void wait_and_lock( int semid )
{
    semop( semid, &sb_lock, 1);
}

void signal_and_unlock( int semid )
{
    semop( semid, &sb_unlock, 1);
}

int get_end_point_rec_info_shm(unsigned int token, time_t *endPoint)
{
	int semid;
	struct search_session	*tmp_sess;

	/* init semaphore */
	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	tmp_sess = &rec_shm.sess[token];

	int last_point;
	GTimeVal tval;
	
	/* If the search had not yet begun, the StartPoint shall be returned.
	 */
	if(tmp_sess->search_state == SEARCH_STATE_QUEUED)
	{
		if(tmp_sess->FindType == FIND_RECORDING)
		{
			*endPoint = 0;
		}
		else if(tmp_sess->FindType == FIND_EVENT)
		{
			*endPoint = (tmp_sess->StartPoint > 0) ? tmp_sess->StartPoint : 0;
		}
		g_printf("[End Search] search had not yet begun. StartPoint shall be returned.\n");
	}

	/* If the search was completed, the original EndPoint supplied by the Find operation shall be returned.
       When issuing EndSearch on a FindRecordings request the EndPoint is undefined and shall not be used since the
	   FindRecordings request doesn't have StartPoint/EndPoint.
	*/
	else if(tmp_sess->search_state == SEARCH_STATE_COMPLETED)
	{
		if(tmp_sess->FindType == FIND_EVENT)
		{
			*endPoint = (tmp_sess->EndPoint > 0) ? tmp_sess->EndPoint : 0;
		}
		else if(tmp_sess->FindType == FIND_RECORDING)
		{
			*endPoint = 0;
		}
		g_printf("[End Search] search is completed. original EndPoint shall be returned.\n");
	}
	
	/* If the search was interrupted before completion,
	   the point in time that the search had reached shall be returned. 
	 */
	else if(tmp_sess->search_state == SEARCH_STATE_SEARCHING)
	{
		g_printf(">>>>>>>>>>>>>> Set EndSearch >>>>>>>>>>>>>>>>>\n");
		set_end_search_token(token, 1);
		
		/* wait until search thread terminated */
		usleep(500*1000);
	}
	
	if(tmp_sess->wp == -1)
	{
		//buffer empty
		*endPoint = 0;
		signal_and_unlock( semid );
		return -1;
	}

	last_point = tmp_sess->wp;
	if(last_point < 0)
		last_point = MAX_MSG_BUF - 1;
	
	GUINT64_TO_GTIMEVAL( tmp_sess->rec_info[last_point].start_time, tval);	
	*endPoint = tval.tv_sec;

	signal_and_unlock( semid );
	return last_point;
}

int init_rp_rec_info_shm(unsigned int token)
{
	int semid;
	struct search_session	*tmp_sess;

	/* init semaphore */
	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	tmp_sess = &rec_shm.sess[token];
	tmp_sess->rp = 0;

	signal_and_unlock( semid );
	return 0;
}

int pop_rec_info_shm(unsigned int token, recording_info *rec_info, char *search_state)
{
#if 1
	int semid;
	struct search_session	*tmp_sess;

	/* init semaphore */
	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	tmp_sess = &rec_shm.sess[token];
	*search_state = tmp_sess->search_state;
	
#if 1
	if(tmp_sess->wp == -1)
	{
		// buffer empty
		signal_and_unlock( semid );
		return -1;	
	}
#else
	if( tmp_sess->rp == tmp_sess->wp ){
		// There are not any messages in buffer...
		signal_and_unlock( semid );
		return -1;
	}
#endif

	/* pop msg */		
g_printf("wp[%d] rp[%d] \n", tmp_sess->wp, tmp_sess->rp);
	if( tmp_sess->rp - 1 == tmp_sess->wp ){
		// There are not any messages in buffer...
		signal_and_unlock( semid );
		return -1;
	}

	if(tmp_sess->rp == MAX_MSG_BUF)
		tmp_sess->rp = 0;


	memcpy(rec_info, &tmp_sess->rec_info[tmp_sess->rp], sizeof(recording_info));
	
#if 0
	/* print buf */
	int i;
	g_printf("=====================");
	for(i=0; i< MAX_MSG_BUF; i++){
		if( rec_shm.sess[token].rp == i )
		{
			g_printf("<");

		g_printf("[%d] state[%d] search token[%d] ch[%d] s[%lld] e[%lld]\n",
			i, rec_shm.sess[token].search_state, rec_shm.sess[token].token, rec_shm.sess[token].rec_info[i].ch, rec_shm.sess[token].rec_info[i].start_time, rec_shm.sess[token].rec_info[i].end_time);

		*search_state = tmp_sess->search_state;
		g_printf("search_state[%d]\n", *search_state);
		}
	}
#endif
	tmp_sess->rp++; 
	
	signal_and_unlock( semid );
	return 0;
#else
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;

	memset(&tmp_sess, 0x00, sizeof(struct search_session));
	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));
	
	tmp_sess = &ebuf.sess[token];

	/* pop msg */
	if( tmp_sess->rp == tmp_sess->wp ){
		// There are not any messages in buffer...
		signal_and_unlock( semid );
		return -1;
	}
	
	memcpy(rec_info, &tmp_sess->rec_info[tmp_sess->rp], sizeof(recording_info));
	
#if 0
	/* print buf */
	int i;
	OV_DEBUG("=====================");
	for(i=0; i< MAX_MSG_BUF; i++){
		if( ebuf.sess[token].rp == i )
			OV_DEBUG("<");

		OV_DEBUG("[%d] state[%d] search token[%d] ch[%d] s[%lld] e[%lld]",
			i, ebuf.sess[token].search_state, ebuf.sess[token].token, ebuf.sess[token].rec_info[i].ch, ebuf.sess[token].rec_info[i].start_time, ebuf.sess[token].rec_info[i].end_time);
	}
#endif
	tmp_sess->rp++; 

	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;
#endif
}

int init_search_rec_shm(unsigned int token, REC_FIND_TYPE FindType, time_t StartPoint, time_t EndPoint)
{
#if 1
	int semid;
	struct search_session	*tmp_sess;

	/* init semaphore */
	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	tmp_sess = &rec_shm.sess[token];
	memset(tmp_sess, 0x00, sizeof(struct search_session));

	//bug fix
	tmp_sess->wp = -1;
	tmp_sess->FindType   = FindType;
	tmp_sess->StartPoint = StartPoint;
	tmp_sess->EndPoint   = EndPoint;
	
	g_printf("===================== Session[%d] is initialized", tmp_sess->token);
	
	signal_and_unlock( semid );
	return 0;
#else
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;

	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));

	tmp_sess = &ebuf.sess[token];

#if 1
	OV_DEBUG("===================== Session[%d] is initialized",tmp_sess->token);
#endif
	memset(tmp_sess, 0x00, sizeof(struct search_session));
	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;
#endif
}


int set_search_rec_state_shm(unsigned int token, char search_state)
{
#if 1
	int semid;
	struct search_session	*tmp_sess;

	/* init semaphore */
	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	tmp_sess = &rec_shm.sess[token];
	tmp_sess->search_state = search_state;
	g_printf("===================== Session[%d] is [%d]\n",tmp_sess->token, tmp_sess->search_state);
	
	signal_and_unlock( semid );	
	return 0;
#else
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;

	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));

	tmp_sess = &ebuf.sess[token];

	tmp_sess->search_state = search_state;
#if 1
	OV_DEBUG("===================== Session[%d] is [%d]",tmp_sess->token, tmp_sess->search_state);
#endif


	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;
#endif
}

int push_rec_info_shm(unsigned int token, recording_info *rec_info)
{
#if 1
	int semid;
	int is_init_state = 0;
	struct search_session	*tmp_sess;

	/* init semaphore */
	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	tmp_sess = &rec_shm.sess[token];
	
	/* check if buff is full */
	if(tmp_sess->wp == -1)
		is_init_state = 1;
		
	tmp_sess->wp++;
	if( tmp_sess->wp >= MAX_MSG_BUF )
		tmp_sess->wp = 0;

#if 1 // OVERWRITE
	if( tmp_sess->wp == tmp_sess->rp && !is_init_state ){
		/* todo
		 *  bugfix
		 */
		tmp_sess->rp++;
		if( tmp_sess->rp >= MAX_MSG_BUF )
			tmp_sess->rp = 0;
	}
#else // RETURN
	if( tmp_sess->wp == tmp_sess->ep ){
		signal_and_unlock( semid );
		return -1;
	}		
#endif //]] Jeonghun_0130914_END -- overwirte

	/* append msg */
	memcpy(&tmp_sess->rec_info[tmp_sess->wp], rec_info, sizeof(recording_info));
 
#if 1
	/* print buf */
	int i;
	g_printf("=====================");
	for(i=0; i< MAX_MSG_BUF; i++){
		if( rec_shm.sess[token].wp == i )
		{
			g_printf(">");

		g_printf("[%d] state[%d] search token[%d] ch[%d] s[%lld] e[%lld]\n",
			i, rec_shm.sess[token].search_state, rec_shm.sess[token].token, rec_shm.sess[token].rec_info[i].ch, rec_shm.sess[token].rec_info[i].start_time, rec_shm.sess[token].rec_info[i].end_time);
		}
	}
#endif
	
	signal_and_unlock( semid );	
	return 0;
#else
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;
	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));

	tmp_sess = &ebuf.sess[token];

	/* check if buff is full */
	tmp_sess->wp++;
	if( tmp_sess->wp >= MAX_MSG_BUF )
		tmp_sess->wp = 0;

#if 1 // OVERWRITE
	if( tmp_sess->wp == tmp_sess->rp ){
		// Overwrite ..
		tmp_sess->rp++;
		if( tmp_sess->rp >= MAX_MSG_BUF )
			tmp_sess->rp = 0;
	}		
#else // RETURN
	if( tmp_sess->wp == tmp_sess->ep ){
		signal_and_unlock( semid );
		return -1;
	}		
#endif //]] Jeonghun_0130914_END -- overwirte

	/* append msg */
	memcpy(&tmp_sess->rec_info[tmp_sess->wp], rec_info, sizeof(recording_info));
 
#if 0
	/* print buf */
	int i;
	OV_DEBUG("=====================");
	for(i=0; i< MAX_MSG_BUF; i++){
		if( ebuf.sess[token].wp == i )
			OV_DEBUG(">");

		OV_DEBUG("[%d] state[%d] search token[%d] ch[%d] s[%lld] e[%lld]",
			i, ebuf.sess[token].search_state, ebuf.sess[token].token, ebuf.sess[token].rec_info[i].ch, ebuf.sess[token].rec_info[i].start_time, ebuf.sess[token].rec_info[i].end_time);
	}
#endif


	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;
#endif
}

int event_status_shm_write(int event_id, unsigned int event_status_flag)
{
    int shmid, ret;
    time_t now;
    void *mem;
    EVENT_STATUS_SHM event_status;

    int semid;
    /*Get the shared memory segment using EVENT_SHM_ID  */
    shmid = shmget(EVENT_SHM_ID , 0, 0);
    if( shmid < 0){
        return -1;
    }

    semid= semget( EVENT_SEM_ID, 0 ,IPC_CREAT );
    wait_and_lock( semid );

    mem = shmat(shmid, (const void *)0, 0);
    if( mem < 0){
        signal_and_unlock( semid );
        return -1;
    }

    memcpy(&event_status, mem, sizeof(EVENT_STATUS_SHM));

    now = time(NULL);

	switch(event_id) {
		case OV_PROP_MOTION:
	        event_status.motion_flags = event_status_flag;
		break;
		case OV_PROP_SENSOR:
    	    event_status.alarm_flags = event_status_flag;			
		break;
		case OV_PROP_RECORDING_JOB_STATE_CHANGE:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",__FUNCTION__, __LINE__);	
			event_status.recording_job_state_chage = event_status_flag;
		break;
		case OV_PROP_CONFIGURATION_CHANGE:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",__FUNCTION__, __LINE__);	
			event_status.recording_configuration_change = event_status_flag;
		break;
		case OV_PROP_DATA_DELETION:
			event_status.recording_data_deletion = event_status_flag;
		break;
		case OV_PROP_RECORDING_AND_TRACK:
			event_status.recording_recording_and_track = event_status_flag;			
		break;
		default:
		break;			
	
	}
	



    memcpy(mem, &event_status, sizeof(EVENT_STATUS_SHM));

    ret = shmdt(mem);
    signal_and_unlock( semid );
    if( ret < 0){
        return -1;
    }

    return 0;
}

int event_msgbuf_shm_append(unsigned int event_id, unsigned int ch, unsigned int flags)
{
    int shmid, ret;
    void *mem;
    EVENT_MSGBUF_SHM    ebuf;
    struct sysinfo info;
    time_t now; 

    int semid;
	
     sysinfo(&info);
     now = time(NULL);

//	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",__FUNCTION__, __LINE__);	
	

    /*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
    shmid = shmget(EVENT_MSGBUF_SHM_ID , 0, 0);
    if( shmid < 0){
        return -1;
    }

    semid= semget( EVENT_MSGBUF_SEM_ID, 0 ,IPC_CREAT );
    wait_and_lock( semid );
 
    mem = shmat(shmid, (const void *)0, 0);
    if( mem < 0){
        signal_and_unlock( semid );
        return -1;
    }
    memcpy(&ebuf, mem, sizeof(EVENT_MSGBUF_SHM));

    /* append msg */
    ebuf.start++;
    if( ebuf.start >= MAX_MSG_BUF )
        ebuf.start = 0;

    ebuf.msg_buf[ebuf.start].key = event_id;
    ebuf.msg_buf[ebuf.start].ch = ch;	
    ebuf.msg_buf[ebuf.start].value = flags;
    ebuf.msg_buf[ebuf.start].time_tick = now;
    ebuf.msg_buf[ebuf.start].uptime_tick = info.uptime;

#if 0
    /* print buf */
    int i;
OV_DEBUG("=====================");
    for(i=0; i< MAX_MSG_BUF; i++){
        if( ebuf.start == i )
            OV_DEBUG(">");

        OV_DEBUG("[%d][%d][%d][%d][%ld]",
                i, ebuf.msg_buf[i].key, ebuf.msg_buf[i].value, ebuf.msg_buf[i].time_tick, ebuf.msg_buf[i].uptime_tick);
    }
#endif

    memcpy(mem, &ebuf, sizeof(EVENT_MSGBUF_SHM));
    ret = shmdt(mem);
    signal_and_unlock( semid );
    if( ret < 0){
        return -1;
    }

    return 0;
}

//ONVIF_NEW_EVENT
static void
_add_onvif_event_msg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0, };
	gint ch = 0;
	gchar buff[128];

	g_return_if_fail(pinfo != NULL);

	guint prop_id = (guint)data;
	guint new_val, m_new, m_old;
	guint *curr_val, *rise_val;
	gint i;

	//	gint div_count = _nf_action->email_state.div_count;

	g_assert( prop_id < LAST_PROP );

#ifdef DEBUG_ACTION_LOG

	if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB]
	        || _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB_SIMPLE] )
		g_message("%s prop_id[%d] [%ld.%06ld] type[%d] [0x%02x]", __FUNCTION__,
		          prop_id,
		          pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
		          pinfo->type, pinfo->d.params[0] );
#endif

	new_val	= pinfo->d.params[0];

	if ( old_val[prop_id] == new_val )
	{
#ifdef DEBUG_ACTION_LOG
		if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
			g_message("%s  same value, skip old[0x%08x] new[0x%08x]",
			          __FUNCTION__, old_val[prop_id] , new_val);
#endif

		return ;
	}

	if (prop_id == PROP_ALARM )
	{
		struct e_relay_type e_relay;

		e_relay.ch = 0;
		e_relay.active = RELAY_STATE_ACTIVE;
		printf("\e[31m relay value[%d] \e[0m\n", e_relay.active);
		old_val[prop_id] = RELAY_STATE_ACTIVE;
		append_onvif_event_msg(E_KEY_RELAY, &e_relay, E_Changed, 0);
	}

	if (prop_id == PROP_MOTION )
	{
		//		event_status_shm_write(OV_PROP_MOTION, new_val);		 //Search ME:
		ch = NUM_ACTIVE_CH;
	}
	else if (prop_id == PROP_SENSOR)
	{
		//		event_status_shm_write(OV_PROP_SENSOR, new_val);
#if defined(ENABLE_SENSOR_IPCAM)
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			ch = _nf_action_num_alarm;
		#else
			ch = NUM_ALARM;
		#endif
#else

		ch = NUM_ACTIVE_CH;
#endif

	}
	else if (prop_id == PROP_VLOSS )
	{
		ch = NUM_ACTIVE_CH;
#if defined(ENABLE_EVENT_TAMPER)

	}
	else if (prop_id == PROP_EVENT_TAMPER )
	{
		ch = NUM_ACTIVE_CH;
#endif

	}
	else
	{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);
		return ;
	}

	for (i = 0; i < ch; i++)
	{
		m_new = (new_val >> i) & 1;
		m_old = (old_val[prop_id] >> i) & 1;

		if ( m_new != m_old )
		{
#ifdef DEBUG_ACTION_LOG
			if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
				g_message("%s prop_id[%d] ch[%d] [%d]-->[%d]", __FUNCTION__,
				          prop_id, i, m_old, m_new);
#endif

			struct e_motion_type motion;
			struct e_alarm_type alarm;
			struct e_image_type image;

			switch (prop_id)
			{
			case PROP_MOTION:
				/* check motion enable */
				memset(buff, 0x00, sizeof(char)*128);
				sprintf(buff, "alarm.motion.M%d.act", i);
				if ( nf_sysdb_get_bool(buff) == FALSE )
				{
					continue;
				}
				if (m_new)
				{
					/* add General Motion event */
					motion.ch = i;
					motion.value = 1;
					// OV_DEBUG("Motion ON ch[%d]", i);
				}
				else
				{
					motion.ch = i;
					motion.value = 0;
					// OV_DEBUG("Motion OFF ch[%d]", i);
				}
				
				append_onvif_event_msg(E_KEY_MOTION_ALARM, &motion, E_Changed, 0);
				//append_onvif_event_msg(E_KEY_ITX_MOTION, &motion, E_Changed, 0);
				
				break;
			case PROP_SENSOR:
				/* check sensor enable */
				/*
				memset(buff, 0x00, sizeof(char)*128);
				sprintf(buff, "alarm.sensor.S%d.enable", i);
				if ( nf_sysdb_get_bool(buff) == FALSE )
				{
					continue;
				}
				*/
				if (m_new)
				{
					alarm.ch = i;
					alarm.value = 1;
					OV_DEBUG("Alarm In ON ch[%d]", i);
				}
				else
				{
					alarm.ch = i;
					alarm.value = 0;
					OV_DEBUG("Alarm In OFF ch[%d]", i);
				}
				append_onvif_event_msg(E_KEY_SENSOR, &alarm, E_Changed, 0);
				append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Changed, 0);
				
				break;
#if defined(ENABLE_EVENT_TAMPER)
			case PROP_EVENT_TAMPER:
				/* check tamper enable */
				memset(buff, 0x00, sizeof(char)*128);
				sprintf(buff, "alarm.tamper.P%d.enable", i);
				if ( nf_sysdb_get_bool(buff) == FALSE )
				{
					continue;
				}
				if (m_new)
				{
					image.ch = i;
					image.value = 1;
					OV_DEBUG("Tamper ON ch[%d]", i);
				}
				else
				{
					image.ch = i;
					image.value = 0;
					OV_DEBUG("Tamper OFF ch[%d]", i);
				}
				append_onvif_event_msg(E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE, &image, E_Changed, 0);				
				break;
#endif
			case PROP_VLOSS:
				/* check vloss enable */
				memset(buff, 0x00, sizeof(char)*128);
				sprintf(buff, "act.vloss.V%d.buzzer", i);
				if ( nf_sysdb_get_bool(buff) == FALSE )
				{
					continue;
				}
				if (m_new)
				{
					image.ch = i;
					image.value = 1;
					OV_DEBUG("VLOSS ON ch[%d]", i);
				}
				else
				{
					image.ch = i;
					image.value = 0;
					OV_DEBUG("VLOSS OFF ch[%d]", i);
				}
				append_onvif_event_msg(E_KEY_VIDEO_SIGNAL_LOSS, &image, E_Changed, 0);				
				break;
			default:
				break;

			}
		}
	}

#ifdef DEBUG_ACTION_LOG
	if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
		g_message("%s curr[0x%08x] rise[0x%08x]", __FUNCTION__, *curr_val, *rise_val);
#endif

	old_val[prop_id] = new_val;

}


//ONVIF_NEW_EVENT
static void _add_onvif_emsg_all_param_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val0[LAST_PROP] = {0, };
	static guint old_val1[LAST_PROP] = {0, };
	static guint old_val2[LAST_PROP] = {0, };
	static guint old_val3[LAST_PROP] = {0, };
	gint ch = 0;
	gchar buff[128];

	g_return_if_fail(pinfo != NULL);

	guint prop_id = (guint)data;
	guint new_val0, m_new0, m_old0;
	guint new_val1, m_new1, m_old1;
	guint new_val2, m_new2, m_old2;
	guint new_val3, m_new3, m_old3;
	guint *curr_val, *rise_val;
	gint i;

	struct e_ad_alarm_type ad_alarm;
	struct e_alarm_type alarm;

	//	gint div_count = _nf_action->email_state.div_count;

	g_assert( prop_id < LAST_PROP );

#ifdef DEBUG_ACTION_LOG

	if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB]
	        || _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB_SIMPLE] )
		g_message("%s prop_id[%d] [%ld.%06ld] type[%d] [0x%02x][0x%02x][0x%02x][0x%02x]", __FUNCTION__, prop_id, pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec, pinfo->type, pinfo->d.params[0], pinfo->type, pinfo->d.params[1], pinfo->type, pi
nfo->d.params[2], pinfo->type, pinfo->d.params[3] );
#endif

	new_val0	= pinfo->d.params[0];
	new_val1	= pinfo->d.params[1];
	new_val2	= pinfo->d.params[2];
	new_val3	= pinfo->d.params[3];

	if ( old_val0[prop_id] == new_val0 && old_val1[prop_id] == new_val1 && old_val2[prop_id] == new_val2 && old_val3[prop_id] == new_val3)
	{
#ifdef DEBUG_ACTION_LOG
		if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
			g_message("%s  same value, skip old[0x%08x] new[0x%08x]",
			          __FUNCTION__, old_val[prop_id] , new_val);
#endif

		return ;
	}

	if (prop_id == PROP_SENSOR)
	{
		//		event_status_shm_write(OV_PROP_SENSOR, new_val);
#if defined(ENABLE_SENSOR_IPCAM)
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		ch = _nf_action_num_alarm;
	#else
		ch = NUM_ALARM;
	#endif
#else

		ch = NUM_ACTIVE_CH;
#endif

	}
	else
	{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);
		return ;
	}

	for (i = 0; i < ch; i++)
	{
		m_new0 = (new_val0 >> i) & 1;
		m_old0 = (old_val0[prop_id] >> i) & 1;
		m_new1 = (new_val1 >> i) & 1;
		m_old1 = (old_val1[prop_id] >> i) & 1;
		m_new2 = (new_val2 >> i) & 1;
		m_old2 = (old_val2[prop_id] >> i) & 1;
		m_new3 = (new_val3 >> i) & 1;
		m_old3 = (old_val3[prop_id] >> i) & 1;


		switch (prop_id)
		{
		case PROP_SENSOR:

			if ( m_new0 != m_old0	)
			{
				/* check sensor enable */
				/*
				memset(buff, 0x00, sizeof(char)*128);
				sprintf(buff, "alarm.sensor.S%d.enable", i);
				if ( nf_sysdb_get_bool(buff) == FALSE )
				{
					continue;
				}
				*/
				ad_alarm.ch = alarm.ch = i;

				if (m_new0)
				{
					alarm.value = 1;
				}
				else
				{

					alarm.value = 0;
				}

				if (m_new2)
				{ 
					// active
					ad_alarm.state = SENSOR_ACTIVE;
				}
				else if (m_new1)
				{ 
					//sabotage
					ad_alarm.state = SENSOR_SABOTAGE;
				}
				else
				{
					ad_alarm.state = SENSOR_INACTIVE;
				}

				append_onvif_event_msg(E_KEY_SENSOR, &alarm, E_Changed, 0);
				append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Changed, 0);
				append_onvif_event_msg(E_KEY_ADVANCED_SENSOR, &ad_alarm, E_Changed, 0);
			}
			else if (m_new1 != m_old1 || m_new2 != m_old2)
			{
				/* check sensor enable */
				/*
				memset(buff, 0x00, sizeof(char)*128);
				sprintf(buff, "alarm.sensor.S%d.enable", i);
				if ( nf_sysdb_get_bool(buff) == FALSE )
				{
					continue;
				}
				*/
				ad_alarm.ch = i;
				if (m_new2)
				{ 
					// active
					ad_alarm.state = SENSOR_ACTIVE;
				}
				else if (m_new1)
				{ 
					//sabotage
					ad_alarm.state = SENSOR_SABOTAGE;
				}
				else
				{
					ad_alarm.state = SENSOR_INACTIVE;
				}
				append_onvif_event_msg(E_KEY_ADVANCED_SENSOR, &ad_alarm, E_Changed, 0);
			}

			break;
		default:
			break;

		}



#ifdef DEBUG_ACTION_LOG
		if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
		{
			g_message("%s prop_id[%d] ch[%d] 0[%d]-->[%d]", __FUNCTION__, prop_id, i, m_old0, m_new0);
			g_message("%s prop_id[%d] ch[%d] 1[%d]-->[%d]", __FUNCTION__, prop_id, i, m_old1, m_new1);
			g_message("%s prop_id[%d] ch[%d] 2[%d]-->[%d]", __FUNCTION__, prop_id, i, m_old2, m_new2);
			g_message("%s prop_id[%d] ch[%d] 3[%d]-->[%d]", __FUNCTION__, prop_id, i, m_old3, m_new3);
		}
#endif

	}


#ifdef DEBUG_ACTION_LOG
	if ( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
		g_message("%s curr[0x%08x] rise[0x%08x]", __FUNCTION__, *curr_val, *rise_val);
#endif

	old_val0[prop_id] = new_val0;
	old_val1[prop_id] = new_val1;
	old_val2[prop_id] = new_val2;
	old_val3[prop_id] = new_val3;
}


static void
_add_onvif_conf_chage_event_msg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[NUM_OF_EVENT_CONF] = {0, };
	guint new_val = 0;
	struct e_event_config_type conf;

	if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM ){
		new_val = get_all_current_ptz_enable();
		if( new_val != old_val[PTZ_ON_OFF_CHANGED] ){
			conf.category = PTZ_ON_OFF_CHANGED;
			conf.next = new_val;
			conf.prev = old_val[PTZ_ON_OFF_CHANGED];
			append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Changed, 0);				
		}
		old_val[PTZ_ON_OFF_CHANGED] = new_val;		
	}
	else if(pinfo->d.params[0] == NF_SYSDB_CATE_ALARM ){
		new_val = get_all_current_motion_enable();
		if( new_val != old_val[MOTION_DETECTION_ON_OFF_CHANGED] ){
			conf.category = MOTION_DETECTION_ON_OFF_CHANGED;
			conf.next = new_val;
			conf.prev = old_val[MOTION_DETECTION_ON_OFF_CHANGED];
			append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Changed, 0);				
		}
		old_val[MOTION_DETECTION_ON_OFF_CHANGED] = new_val;		
	}
	else if(pinfo->d.params[0] == NF_SYSDB_CATE_REC ){
		new_val = get_all_current_audio_enable();
		if( new_val != old_val[AUDIO_ON_OFF_CHANGED] ){
			conf.category = AUDIO_ON_OFF_CHANGED;
			conf.next = new_val;
			conf.prev = old_val[AUDIO_ON_OFF_CHANGED];			
			append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Changed, 0);				
		}
		old_val[AUDIO_ON_OFF_CHANGED] = new_val;
	}
	else if(pinfo->d.params[0] == NF_SYSDB_CATE_ACT ){
		new_val = get_all_current_tamper_enable();
		if( new_val != old_val[TAMPER_DETECTION_ON_OFF_CHANGED] ){
			conf.category = TAMPER_DETECTION_ON_OFF_CHANGED;
			conf.next = new_val;
			conf.prev = old_val[TAMPER_DETECTION_ON_OFF_CHANGED];			
			append_onvif_event_msg(E_KEY_EVENT_CONFIGURATION_CHANGE, &conf, E_Changed, 0);				
		}
		old_val[TAMPER_DETECTION_ON_OFF_CHANGED] = new_val;
	}
	else if(pinfo->d.params[0] == NF_SYSDB_CATE_UPDATE ){
		old_val[PTZ_ON_OFF_CHANGED] = get_all_current_ptz_enable();
		old_val[MOTION_DETECTION_ON_OFF_CHANGED] = get_all_current_motion_enable();
		old_val[AUDIO_ON_OFF_CHANGED] = get_all_current_audio_enable();
		old_val[TAMPER_DETECTION_ON_OFF_CHANGED] = get_all_current_tamper_enable();		
	}
	
	return;
}

static void get_macaddress(char *tmp_mac)
{
	int fd;
	struct ifreq ifbuf;
	unsigned char *hwaddr;
	char *eth_str = nf_onvif_get_eth_str();

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		_TTY_LOG_ONVIF( "%s socket failed\n", __FUNCTION__);
		return;
	}

	strcpy(ifbuf.ifr_name, eth_str);
	ioctl(fd, SIOCGIFHWADDR, &ifbuf);

	hwaddr = (unsigned char *)ifbuf.ifr_hwaddr.sa_data;

	sprintf(tmp_mac, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", hwaddr[0], hwaddr[1],
			hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

	close(fd);
}
#if defined (DVR_BUILT_IN_VA)
static void
_add_onvif_dva_change_event_msg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DVA_MSG msg;
	guchar ch = 0;
	guchar dva_type = 0;
	char *msg_data = NULL;
	char *encode_data = NULL;
	int key = -1;
	int ret = 0;
	
	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}

	msg_data = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if ( msg_data == NULL )
		return ;

	encode_data = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if ( encode_data == NULL )
	{
		free(msg_data);
		return ;
	}

	memset(msg_data, 0x00, MAX_NOTIFY_DATA_LEN);
	memset(encode_data, 0x00, MAX_NOTIFY_DATA_LEN);
	memset(&msg, 0x00, sizeof(DVA_MSG));
	
	memcpy(&msg, pinfo->p.ptr, sizeof(DVA_MSG));
	
	ch = msg.ch;
	dva_type = msg.type;

	ret = nf_onvif_make_dva_msg(&msg, msg_data);
	if ( ret < 0 )
	{
		free(msg_data);
		free(encode_data);
		return ;
	}

	base64_encode(msg_data, strlen(msg_data), encode_data);

#ifdef ONVIF_METADATA_DEBUG
	printf("\e[31m [%s][%d] [%s] \e[0m\n", __FUNCTION__, __LINE__, msg_data);
#endif

	nf_rec_base64json_va_metadata_handoff(E_KEY_DVA_EVENT, encode_data, ch);
	
	free(msg_data);
	free(encode_data);	
}
#endif

static void
_add_onvif_vca_counter_change_event_msg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	GTimeVal		req_timeval;

	memset(&req_timeval, 0x00, sizeof(GTimeVal));
	g_get_current_time(&req_timeval);

	guchar ch = 0;
	char encode_data[4096];
	int i = 0, j = 0;
	int ret_len = 0;
	gint *p;
	ivca_meta_cnt_t *pevt;
	ai_rule_event_t msg;
	NF_LOG_INFO info;
	char buff[COMMON_SIZE] = {0,};

	static int va_counter_value[NUM_CHANNEL][IVCA_MAX_CNTRS] = {-1,};
	static int ai_counter_value[NUM_CHANNEL][IVCA_MAX_CNTRS] = {-1,};
	int index = -1;

	guint prop_id = (guint)data;

	g_assert( prop_id < LAST_PROP );

	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}

	memset(&info, 0x00, sizeof(NF_LOG_INFO));
	memset(&msg, 0x00, sizeof(ai_rule_event_t));

	p = pinfo->p.ptr;
	pevt = p + 2;
	if (p[0] >= 16) return -1;
	if ( p[1] <= 0 ) return -1;

	msg.timestampl = (guint)(req_timeval.tv_usec);
	msg.timestamp = (guint)req_timeval.tv_sec;

	info.timestamp = msg.timestamp;
	info.timestampl = msg.timestampl;

	for ( i = 0; i < p[1]; i++)
	{
		msg.ch = p[0];
		ch = msg.ch;
		msg.type = 0;
		msg.rule_id = pevt[i].id + 16;
		msg.object_id = pevt[i].value;
		info.ch = ch;
		
		if ( prop_id == PROP_VA_COUNTER )
		{
			index = nf_onvif_find_rule_id(ONVIF_CLASSIC_VA_COUNTER, ch, pevt[i].id);

			if ( index >= 0 )
			{
				if ( va_counter_value[ch][index] != pevt[i].value )
				{
					va_counter_value[ch][index] = pevt[i].value;
					
					nf_meta_make_base64json(-1, info, &msg, encode_data, &ret_len, NULL, NULL);
				#ifdef ONVIF_METADATA_DEBUG
					printf("\e[31m [%s][%d] %s \e[0m\n", __FUNCTION__, __LINE__, encode_data);
				#endif
					nf_rec_base64json_va_metadata_handoff(E_KEY_DVA_EVENT, encode_data, ch);
				}
			}
		}
		if ( prop_id == PROP_AI_COUNTER )
		{
			index = nf_onvif_find_rule_id(ONVIF_AI_COUNTER, ch, pevt[i].id);

			if ( index >= 0 )
			{
				if ( ai_counter_value[ch][index] != pevt[i].value )
				{
					ai_counter_value[ch][index] = pevt[i].value;
					nf_meta_make_base64json(DVA_AI_DETECTION, info, &msg, encode_data, &ret_len, NULL, NULL);
				#ifdef ONVIF_METADATA_DEBUG
					printf("\e[31m [%s][%d] %s \e[0m\n", __FUNCTION__, __LINE__, encode_data);
				#endif
					nf_rec_base64json_va_metadata_handoff(E_KEY_DVA_EVENT, encode_data, ch);
				}
			}
		}
	}

	return 0;
}

static void
_add_onvif_vca_change_event_msg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	
	guchar ch = 0;
	char *encode_data = NULL;
	int i = 0;
	int ret_len = 0;
	gint *p;
	ivca_rule_event_t*va_evt;
	ivca_rule_event_t va_data;
	ai_rule_event_t *ai_evt;
	ai_rule_event_t ai_data;
	NF_LOG_INFO info;

	guint prop_id = (guint)data;

	g_assert( prop_id < LAST_PROP );
	
	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}

	encode_data = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if (encode_data == NULL )
		return ;

	memset(&info, 0x00, sizeof(NF_LOG_INFO));

	if ( prop_id == PROP_VA )
	{
		p = pinfo->p.ptr;
		va_evt = p + 2;
		if (p[0] >= 16)
		{
			free(encode_data);
			return -1;
		}


		if(!get_vca_enable(p[0])) {
			free(encode_data);
			return;	
		}

		
		if(!get_vca_enable(p[0])) {
			free(encode_data);
			return;	
		}

		for ( i = 0; i < p[1]; i++)
		{
			ch = va_evt[i].ch;
			info.ch = ch;
			info.timestamp = va_evt[i].timestamp;
			memcpy(&va_data, &va_evt[i], sizeof(ivca_rule_event_t));
			if ( va_data.type == IVCA_ET_COUNTER )
				va_data.rule_id += 16;
			nf_meta_make_base64json(-1, info, &va_data, encode_data, &ret_len, NULL, NULL);
			
#ifdef ONVIF_METADATA_DEBUG
			printf("\e[31m [%s][%d] %s \e[0m\n", __FUNCTION__, __LINE__, encode_data);
#endif

			nf_rec_base64json_va_metadata_handoff(E_KEY_DVA_EVENT, encode_data, ch);
			
			nf_rec_va_metadata_handoff(E_KEY_VA_EVENT, &va_data, ch);
			memset(encode_data, 0x00, sizeof(MAX_NOTIFY_DATA_LEN));
			memset(&va_data, 0x00, sizeof(ivca_rule_event_t));
		}
	}
	else if ( prop_id == PROP_AI )
	{
		p = pinfo->p.ptr;
		ai_evt = p + 2;
		if (p[0] >= 16)
		{
			free(encode_data);
			return -1;
		}
		
		for ( i = 0; i < p[1]; i++)
		{
			ch = ai_evt[i].ch;
			info.ch = ch;
			info.timestamp = ai_evt[i].timestamp;
			info.timestampl = ai_evt[i].timestampl;
			memcpy(&ai_data, &ai_evt[i], sizeof(ai_rule_event_t));
			if ( ai_data.type == IVCA_ET_COUNTER )
				ai_data.rule_id += 16;
			nf_meta_make_base64json(DVA_AI_DETECTION, info, &ai_data, encode_data, &ret_len, NULL, NULL);

#ifdef ONVIF_METADATA_DEBUG
			printf("\e[31m [%s][%d] %s \e[0m\n", __FUNCTION__, __LINE__, encode_data);
#endif

			nf_rec_base64json_va_metadata_handoff(E_KEY_DVA_EVENT, encode_data, ch);
			
			nf_rec_va_metadata_handoff(E_KEY_AI_EVENT, &ai_data, ch);
			memset(encode_data, 0x00, sizeof(MAX_NOTIFY_DATA_LEN));
			memset(&ai_data, 0x00, sizeof(ai_rule_event_t));
		}
	}
	else		
	{
		free(encode_data);
		return ;	
	}
	
	free(encode_data);
}

#if defined(DVR_BUILT_IN_VA)
static int nf_onvif_make_dva_msg(DVA_MSG *msg, char *data)
{
	char *topic = NULL;
	char buff[32] = {0,};
	char mac_addr[13] = {9,};
	char time_sec[32] = {0,};
	char *json_dump_data = NULL;

	if ( msg == NULL )
		return -1;

	get_macaddress(mac_addr);

	snprintf(time_sec, sizeof(time_sec) - 1, "%lld.%lld", msg->timestamp, msg->timestampl);

	if ( msg->type == DVA_INTRUSION_DETECTION )
	{
		topic = "Analytics/FieldDetector/ObjectDetected";

		json_t* _json_root = json_object();
		json_t* _json_meta = NULL;
		json_t* _json_image = NULL;
		json_t* _json_annot = NULL;
		json_t* _json_bbox = NULL;

		json_object_set_new(_json_root, "source", json_string(mac_addr));
		json_object_set_new(_json_root, "topic", json_string(topic));
		json_object_set_new(_json_root, "timestamp", json_string(time_sec));
		json_object_set_new(_json_root, "metadata", json_object());

		//metadata
		_json_meta = json_object_get(_json_root, "metadata");
		json_object_set_new(_json_meta, "annotations", json_array());

		//metadata.annotations
		_json_annot = json_object_get(_json_meta, "annotations");
		json_array_append_new(_json_annot, json_object());

		//metadata.annotations[0]
		_json_annot = json_array_get(_json_annot, 0);
		json_object_set_new(_json_annot, "class", json_string(msg->intrusion_detection.name));
		snprintf(buff, sizeof(buff) - 1, "%f", msg->intrusion_detection.confidence);
		json_object_set_new(_json_annot, "score", json_string(buff));
		json_object_set_new(_json_annot, "bbox", json_array());

		//metadata.annotations[0].bbox
		_json_bbox = json_object_get(_json_annot, "bbox");
		json_array_append_new(_json_bbox, json_real(msg->intrusion_detection.bbox.coords[0]));
		json_array_append_new(_json_bbox, json_real(msg->intrusion_detection.bbox.coords[1]));
		json_array_append_new(_json_bbox, json_real(msg->intrusion_detection.bbox.coords[2]));
		json_array_append_new(_json_bbox, json_real(msg->intrusion_detection.bbox.coords[3]));

		json_dump_data = json_dumps(_json_root, JSON_COMPACT);
		json_decref(_json_root);
	}
	else if ( msg->type == DVA_ILLEGAL_PARKING )
	{
		topic = "Analytics/FieldDetector/IllegalParking";

		json_t* _json_root = json_object();
		json_t* _json_meta = NULL;
		json_t* _json_image = NULL;
		json_t* _json_annot = NULL;
		json_t* _json_bbox = NULL;

		json_object_set_new(_json_root, "source", json_string(mac_addr));
		json_object_set_new(_json_root, "topic", json_string(topic));
		json_object_set_new(_json_root, "timestamp", json_string(time_sec));
		json_object_set_new(_json_root, "metadata", json_object());

		//metadata
		_json_meta = json_object_get(_json_root, "metadata");
		json_object_set_new(_json_meta, "annotations", json_array());

		//metadata.annotations
		_json_annot = json_object_get(_json_meta, "annotations");
		json_array_append_new(_json_annot, json_object());

		//metadata.annotations[0]
		_json_annot = json_array_get(_json_annot, 0);
		json_object_set_new(_json_annot, "class", json_string(msg->illegal_parking.name));
		snprintf(buff, sizeof(buff) - 1, "%f", msg->illegal_parking.confidence);
		json_object_set_new(_json_annot, "score", json_string(buff));
		json_object_set_new(_json_annot, "bbox", json_array());

		//metadata.annotations[0].bbox
		_json_bbox = json_object_get(_json_annot, "bbox");
		json_array_append_new(_json_bbox, json_real(msg->illegal_parking.bbox.coords[0]));
		json_array_append_new(_json_bbox, json_real(msg->illegal_parking.bbox.coords[1]));
		json_array_append_new(_json_bbox, json_real(msg->illegal_parking.bbox.coords[2]));
		json_array_append_new(_json_bbox, json_real(msg->illegal_parking.bbox.coords[3]));

		json_dump_data = json_dumps(_json_root, JSON_COMPACT);
		json_decref(_json_root);
	}
	else
	{
		return -1;
	}

	memcpy(data, json_dump_data, strlen(json_dump_data));
	if ( json_dump_data != NULL)
		free(json_dump_data);
}
#endif

void search_rec_shm_init()
{
    int shmid, page_size, shm_size, i=2;
    int semid;

    page_size = getpagesize();
    /*Semaphore init */
    semid= semget( SEARCH_REC_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    while( page_size < sizeof(SEARCH_REC_SHM)){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    //printf(">>>>>>>>>>>>>>> shm_size[%d]\n", shm_size);

    /*Create the shared memory sement using EVENT_SHM_ID */
    shmid = shmget( SEARCH_REC_SHM_ID, shm_size, 0666 | IPC_CREAT );

    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    return;
}



void event_status_shm_init()
{
    int shmid, page_size, shm_size, i=2;
    int semid;

    page_size = getpagesize();
    /*Semaphore init */
    semid= semget( EVENT_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    while( page_size < sizeof(EVENT_STATUS_SHM)){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    //printf(">>>>>>>>>>>>>>> shm_size[%d]\n", shm_size);

    /*Create the shared memory sement using EVENT_SHM_ID */
    shmid = shmget( EVENT_SHM_ID, shm_size, 0666 | IPC_CREAT );

    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    return;
}

void event_status_msgbuf_shm_init()
{
    int shmid, page_size, shm_size, i=2, ret;
    void *mem;
    int semid;

    /*Semaphore init */
    semid= semget( EVENT_MSGBUF_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    page_size = getpagesize();
    while( page_size < sizeof(EVENT_MSGBUF_SHM)){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    wait_and_lock( semid );
    /*Create the shared memory sement using EVENT_MSGBUF_SHM_ID */
    shmid = shmget(EVENT_MSGBUF_SHM_ID, shm_size, 0666 | IPC_CREAT );

    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    mem = shmat(shmid, (const void *)0, 0);
    if( mem < 0){
        signal_and_unlock( semid );
        return;
    }
    memset(mem, 0, sizeof(EVENT_MSGBUF_SHM));
    ret = shmdt(mem);
    signal_and_unlock( semid );

    if( ret < 0){
        return;
    }

    return;
}


void event_proc_shm_init()
{
    int shmid, page_size, shm_size, i=2;
    int semid;

    /*Semaphore init */
    semid= semget( EVENT_BN_PROC_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);
    semid= semget( EVENT_PULL_PROC_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    page_size = getpagesize();
    while( page_size < sizeof(struct event_proc) * MAX_SUBSCRIPTION_CNT ){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    //printf(">>>>>>>>>>>>>>> shm_size[%d]\n", shm_size);

    /*Create the shared memory sement using EVENT_BN_PROC_SHM_ID */
    shmid = shmget( EVENT_BN_PROC_SHM_ID, shm_size, 0666 | IPC_CREAT );
    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    shmid = shmget( EVENT_PULL_PROC_SHM_ID, shm_size, 0666 | IPC_CREAT );
    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    return;
}

void event_shm_init(void)
{
	int semid = 0;
	
    //create a semaphore for process synchronization
    semid= semget( SEM_KEY,1,IPC_CREAT );
    //set_sem_value(semid,0,1); //set initial value of semaphore
    semctl(semid, 0, SETVAL,1);

    sb_lock.sem_num = 0;
    sb_lock.sem_op = -1;
    sb_lock.sem_flg = 0;
    sb_unlock.sem_num = 0;
    sb_unlock.sem_op = 1;
    sb_unlock.sem_flg = 0;
	
//memset(&g_event_status, 0, sizeof(EVENT_STATUS_SHM));
	event_status_shm_init();
	event_status_msgbuf_shm_init();
	event_proc_shm_init();
	search_rec_shm_init();

    return;
}

void	onvif_event_init(void) //Search ME:
{
	gulong cb_handle = 0;

	onvif_event_mem_init();

#if defined(DVR_BUILT_IN_VA)
	cb_handle= nf_notify_connect_cb( "dva_event", _add_onvif_dva_change_event_msg_cb_func , (gpointer)NULL );
	g_message("%s VCA EVENT connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif
	cb_handle= nf_notify_connect_cb( "motion", _add_onvif_event_msg_cb_func , (gpointer)PROP_MOTION );
	g_message("%s motion connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "alarm", _add_onvif_event_msg_cb_func , (gpointer)PROP_ALARM );
	g_message("%s motion connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb("vca_event", _add_onvif_vca_change_event_msg_cb_func, (gpointer)PROP_VA);
	g_message("%s VCA connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb("ai_event", _add_onvif_vca_change_event_msg_cb_func, (gpointer)PROP_AI);
	g_message("%s VCA connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb("vca_counter", _add_onvif_vca_counter_change_event_msg_cb_func, (gpointer)PROP_VA_COUNTER);
	g_message("%s VCA COUNTER connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb("ai_counter", _add_onvif_vca_counter_change_event_msg_cb_func, (gpointer)PROP_AI_COUNTER);
	g_message("%s VCA COUNTER connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sensor", _add_onvif_emsg_all_param_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	META_RECV_MANAGER *self;
	self = (META_RECV_MANAGER *)malloc(sizeof(META_RECV_MANAGER));
	self->meta_queue = g_async_queue_new();
	meta_queue = self->meta_queue;
	self->is_run = 1;
	self->thread_id = g_thread_create((GThreadFunc)dlva_metadata_thread_func, self, TRUE, NULL);

	EVENT_RECV_MANAGER *event;
	event = (EVENT_RECV_MANAGER *)malloc(sizeof(EVENT_RECV_MANAGER));
	event->event_queue = g_async_queue_new();
	event_queue = event->event_queue;
	event->is_run = 1;
	event->thread_id = g_thread_create((GThreadFunc)ai_generic_event_thread_func, event, TRUE, NULL);
}
static void dlva_metadata_thread_func (META_RECV_MANAGER *self)
{
	VA_EVT_DATA EVT_DATA;
	void *data;
	
	if ( self == NULL )
		return FALSE;

	while ( self->is_run )
	{
		if ( g_async_queue_length(self->meta_queue) > 0)
		{
			data = g_async_queue_try_pop(self->meta_queue);
			if(data != NULL)
			{
				// metadata handoff
				memset(&EVT_DATA, 0x00, sizeof(VA_EVT_DATA));
				memcpy(&EVT_DATA, data, sizeof(VA_EVT_DATA));
				free(data);
				nf_dva_metadata_send(&EVT_DATA);
				nf_onvif_va_data_send(&EVT_DATA);
			}
		}
		else
		{
			g_usleep(1000*5);
		}
	}
	free(self);
}

static void ai_generic_event_thread_func (EVENT_RECV_MANAGER *self)
{
	VA_ONVIF_EVENT_DATA gnr_event;
	void *data;
	char *json_dump_data = NULL;
	json_t *source = NULL;
	char *encode_data = NULL;
	int encode_len = 0;
	
	if ( self == NULL )
		return FALSE;

	while ( self->is_run )
	{
		if ( g_async_queue_length(self->event_queue) > 0)
		{
			data = g_async_queue_try_pop(self->event_queue);
			if(data != NULL)
			{
				memset(&gnr_event, 0x00, sizeof(VA_ONVIF_EVENT_DATA));
				memcpy(&gnr_event, data, sizeof(VA_ONVIF_EVENT_DATA));
				free(data);
				if ( gnr_event.type == VA_TYPE_GENERIC_EVENT )
				{
					if ( gnr_event.json_root != NULL )
					{
						//source = json_object_get(gnr_event.json_root, "source");
						//json_object_del(gnr_event.json_root, "source");
						json_object_set_new(gnr_event.json_root, "source", json_integer(gnr_event.ch));
					
						json_dump_data = json_dumps(gnr_event.json_root, JSON_COMPACT);
						encode_len = base64_encode_len(strlen(json_dump_data));
						encode_data = (char *)malloc(encode_len * sizeof(char));
						
						base64_encode(json_dump_data, strlen(json_dump_data), encode_data);
						nf_rec_base64json_va_metadata_handoff(E_KEY_DVA, encode_data, gnr_event.ch);
						//printf("\e[31m [%s] \e[0m\n", json_dump_data);
						
						if ( json_dump_data != NULL)
						{
							free(json_dump_data);
							json_dump_data = NULL;
						}
						
						if ( encode_data )
						{
							free(encode_data);
							encode_data = NULL;
						}
						json_decref(gnr_event.json_root);
					}
				}
			}
		}
		else
		{
			g_usleep(1000*5);
		}
	}
	free(self);
}

static gboolean nf_onvif_va_data_send(VA_EVT_DATA *p_data)
{
	if ( p_data == NULL )
		return FALSE;

	if ( p_data->ch < 0 )
		return FALSE;

	if ( p_data->meta_count <= 0)
		return FALSE;

	ONVIF_VA_META_FRAME va_data;

	memset(&va_data, 0x00, sizeof(va_data));
	int i = 0;

	va_data.ch = p_data->ch;
	va_data.meta_count = p_data->meta_count;
	va_data.timestamp = p_data->timestamp;
	va_data.timestampl = p_data->timestampl;
	
	for ( i = 0; i < p_data->meta_count; i++)
	{
		va_data.meta_data[i].topic = ONVIF_VA_DATA;

		va_data.meta_data[i].trans_x = -1.0;
		va_data.meta_data[i].trans_y = 1.0;

		// TODO Modified
		//va_data.meta_data[i].scale_x = (double)(1.0 / (p_data->resol_width / 2.0 ));
		//va_data.meta_data[i].scale_y = (double)(1.0 / (p_data->resol_height / 2.0 ));
		va_data.meta_data[i].scale_x = 0.5;
		va_data.meta_data[i].scale_y = -0.5;

		strncpy(va_data.meta_data[i].va_class.class_type, p_data->meta_data[i].class, COMMON_SIZE - 1);

		// 'a'~'z' -> 'A'~'Z'
		if ( va_data.meta_data[i].va_class.class_type[0] >= 97 && va_data.meta_data[i].va_class.class_type[0] <= 122 )
			va_data.meta_data[i].va_class.class_type[0] -= 32;

		va_data.meta_data[i].va_class.likelihood = p_data->meta_data[i].confidence;
		va_data.meta_data[i].object_id = p_data->meta_data[i].id;
		/*
		va_data.meta_data[i].va_shape.bbox[0] = p_data->meta_data[i].bbx_position[0] * p_data->resol_width;
		va_data.meta_data[i].va_shape.bbox[1] = p_data->meta_data[i].bbx_position[1] * p_data->resol_height;
		va_data.meta_data[i].va_shape.bbox[2] = p_data->meta_data[i].bbx_position[2] * p_data->resol_width;
		va_data.meta_data[i].va_shape.bbox[3] = p_data->meta_data[i].bbx_position[3] * p_data->resol_height;
		*/
		va_data.meta_data[i].va_shape.bbox[0] = p_data->meta_data[i].bbx_position[0];
		va_data.meta_data[i].va_shape.bbox[1] = p_data->meta_data[i].bbx_position[1];
		va_data.meta_data[i].va_shape.bbox[2] = p_data->meta_data[i].bbx_position[2];
		va_data.meta_data[i].va_shape.bbox[3] = p_data->meta_data[i].bbx_position[3];
		
		va_data.meta_data[i].va_shape.center_gravity[0] = (va_data.meta_data[i].va_shape.bbox[0] + va_data.meta_data[i].va_shape.bbox[2]) / 2.0;
		va_data.meta_data[i].va_shape.center_gravity[1] = (va_data.meta_data[i].va_shape.bbox[1] + va_data.meta_data[i].va_shape.bbox[3]) / 2.0;
	}

	// onvif va spec send
	nf_rec_va_metadata_handoff(E_KEY_VA_DATA, &va_data, p_data->ch);
}

char* make_va_buf_notification_no_dest(char *send_buf, void *data, int key, int msg_cnt)
{
	char tmp_time[128] = {0, }, *p = NULL;
	char utc_time[128];
	char since_time[128];
	char buff[COMMON_SIZE] = {0,};
	char db_field[COMMON_SIZE] = {0,};
	char *tmp = NULL;
	char *str_ptr = NULL;
	int i=0;
	
	ONVIF_VA_META_FRAME *m_data;
	ivca_rule_event_t *va_event;
	ai_rule_event_t *ai_event;
	int zone_index = 0;
	int stop_time = 0, abandon_time = 0, remove_time = 0, loiter_time = 0, fall_time = 0;
	unsigned int since = 0;
	
	char vs_token[16];
	char va_token[16];

	tmp = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if ( tmp == NULL )
		return NULL;

	memset(tmp_time, 0x00, sizeof(tmp_time));
	memset(utc_time, 0x00, sizeof(utc_time));

	if ( key == E_KEY_VA_DATA )
	{
		p = send_buf;
		m_data = (ONVIF_VA_META_FRAME *)data;

		for ( i = 0; i < m_data->meta_count; i++ )
		{
			strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&m_data->timestamp));
			snprintf(utc_time, 128 - 1, "%s.%3d", tmp_time, m_data->timestampl / 1000);
			sprintf(tmp, "<tt:Frame UtcTime=\"%sZ\">", utc_time);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Object ObjectId=\"%d\">", m_data->meta_data[i].object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Appearance>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Transformation>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Translate x=\"%.1lf\" y=\"%.1lf\"/>", m_data->meta_data[i].trans_x, m_data->meta_data[i].trans_y);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Scale x=\"%lf\" y=\"%lf\"/>", m_data->meta_data[i].scale_x, m_data->meta_data[i].scale_y);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Transformation>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Shape>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:BoundingBox left=\"%lf\" top=\"%lf\" right=\"%lf\" bottom=\"%lf\"/>",
					m_data->meta_data[i].va_shape.bbox[0], m_data->meta_data[i].va_shape.bbox[1], m_data->meta_data[i].va_shape.bbox[2],
					m_data->meta_data[i].va_shape.bbox[3]);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:CenterOfGravity x=\"%lf\" y=\"%lf\"/>", 
					m_data->meta_data[i].va_shape.center_gravity[0], m_data->meta_data[i].va_shape.center_gravity[1]);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Shape>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Class>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:ClassCandidate>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Type>%s</tt:Type>", m_data->meta_data[i].va_class.class_type);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Likelihood>%lf</tt:Likelihood>", m_data->meta_data[i].va_class.likelihood);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:ClassCandidate>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Class>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Appearance>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Object>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Frame>");
			p = stpcpy(p, tmp);
		}
	}
	else if ( key == E_KEY_VA_EVENT)
	{
		va_event = (ivca_rule_event_t *)data;
		
		p = send_buf;
		
		sprintf(tmp, "<wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">");
		p = stpcpy(p, tmp);
		if ( va_event->type == IVCA_ET_DIR_POS || va_event->type == IVCA_ET_DIR_NEG )
			sprintf(tmp, "%s", "tns1:RuleEngine/LineDetector/Crossed");
		if ( va_event->type == IVCA_ET_ENTER || va_event->type == IVCA_ET_EXIT || va_event->type == IVCA_ET_INTRUSION)
			sprintf(tmp, "%s", "tns1:RuleEngine/FieldDetector/IsInside");
		if ( va_event->type == IVCA_ET_STOPPED )
			sprintf(tmp, "%s", "tns1:RuleEngine/StoppedDetector/ObjectIsStoped");
		if ( va_event->type == IVCA_ET_REMOVED )
			sprintf(tmp, "%s", "tns1:RuleEngine/RemovedDetector/ObjectIsRemoved");
		if ( va_event->type == IVCA_ET_LOITERED )
			sprintf(tmp, "%s", "tns1:RuleEngine/LoiteringDetector/ObjectIsLoitering");
		
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Topic>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Message>");
		p = stpcpy(p, tmp);

		strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&va_event->timestamp));
		//snprintf(utc_time, 128 - 1, "%s.%d", tmp_time, va_event->timestampl / 1000);
		
		sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", "Changed", utc_time);
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:Source>");
		p = stpcpy(p, tmp);

		memset(vs_token, 0x00, sizeof(vs_token));
		memset(va_token, 0x00, sizeof(va_token));

		snprintf(vs_token, 16 - 1, "vs%d", va_event->ch);

		for ( i = 0; i < IVCA_MAX_ZONES; i++ )
		{
			snprintf(buff, COMMON_SIZE - 1, "cam.vca.rule.R%d.Z%d.id", va_event->ch, i);
			if ( nf_sysdb_get_int(buff) == va_event->rule_id )
			{
				zone_index = i;
				break;
			}
		}
		
		memset(buff, 0x00, sizeof(buff));
		snprintf(buff, COMMON_SIZE - 1, "cam.vca.rule.R%d.Z%d.name", va_event->ch, zone_index);
		snprintf(va_token, 16 - 1, "va%d_%d_%s", va_event->ch, va_event->rule_id, nf_sysdb_get_str_nocopy(buff));

		sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "VideoSourceConfigurationToken", vs_token);
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "VideoAnalyticsConfigurationToken", va_token);
		p = stpcpy(p, tmp);
		
		sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "Rule", nf_sysdb_get_str_nocopy(buff));

		p = stpcpy(p, tmp);
		sprintf(tmp, "</tt:Source>");
		p = stpcpy(p, tmp);

		if ( va_event->type == IVCA_ET_LOITERED )
		{
			snprintf(db_field, 64 - 1, "cam.dvabx.rule.R%d.Z%d.time_sarlf", va_event->ch, zone_index);
			str_ptr = nf_sysdb_get_str_nocopy(db_field);
			sscanf(str_ptr, "%hu %hu %hu %hu %hu", &stop_time,&abandon_time, &remove_time, &loiter_time,&fall_time);

			since = va_event->timestamp - loiter_time;

			memset(tmp_time, 0x00, sizeof(tmp_time));
			strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&va_event->timestamp));
			
			sprintf(tmp, "<tt:Key>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%d\"/>", "ObjectId", va_event->object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Key>");
			p = stpcpy(p, tmp);

			sprintf(tmp, "<tt:Data>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "Since", tmp_time);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "TimeStamp", utc_time);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Data>");
			p = stpcpy(p, tmp);
		}

		if ( va_event->type == IVCA_ET_DIR_POS || va_event->type == IVCA_ET_DIR_NEG )
		{
			sprintf(tmp, "<tt:Data>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%d\"/>", "ObjectId", va_event->object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Data>");
			p = stpcpy(p, tmp);
		}

		if ( va_event->type == IVCA_ET_ENTER || va_event->type == IVCA_ET_EXIT || va_event->type == IVCA_ET_INTRUSION)
		{
			sprintf(tmp, "<tt:Key>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%d\"/>", "ObjectId", va_event->object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Key>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Data>");
			p = stpcpy(p, tmp);
			if ( va_event->type == IVCA_ET_ENTER || va_event->type == IVCA_ET_INTRUSION)
				sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "IsInside", "true");
			if ( va_event->type == IVCA_ET_EXIT )
				sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "IsInside", "false");
			
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Data>");
			p = stpcpy(p, tmp);
		}
		
		sprintf(tmp, "</tt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
	}
	else if ( key == E_KEY_AI_EVENT)
	{
		ai_event = (ai_rule_event_t *)data;
		
		p = send_buf;
		
		sprintf(tmp, "<wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">");
		p = stpcpy(p, tmp);
		if ( ai_event->type == IVCA_ET_DIR_POS || ai_event->type == IVCA_ET_DIR_NEG )
			sprintf(tmp, "%s", "tns1:RuleEngine/LineDetector/Crossed");
		if ( ai_event->type == IVCA_ET_ENTER || ai_event->type == IVCA_ET_EXIT || ai_event->type == IVCA_ET_INTRUSION)
			sprintf(tmp, "%s", "tns1:RuleEngine/FieldDetector/IsInside");
		if ( ai_event->type == IVCA_ET_STOPPED )
			sprintf(tmp, "%s", "tns1:RuleEngine/StoppedDetector/ObjectIsStoped");
		if ( ai_event->type == IVCA_ET_REMOVED )
			sprintf(tmp, "%s", "tns1:RuleEngine/RemovedDetector/ObjectIsRemoved");
		if ( ai_event->type == IVCA_ET_LOITERED )
			sprintf(tmp, "%s", "tns1:RuleEngine/LoiteringDetector/ObjectIsLoitering");
		
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Topic>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsnt:Message>");
		p = stpcpy(p, tmp);

		
		strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&ai_event->timestamp));
		snprintf(utc_time, 128 - 1, "%s.%3d", tmp_time, ai_event->timestampl / 1000);
		
		sprintf(tmp, "<tt:Message PropertyOperation=\"%s\" UtcTime=\"%sZ\">", "Changed", utc_time);
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:Source>");
		p = stpcpy(p, tmp);

		memset(vs_token, 0x00, sizeof(vs_token));
		memset(va_token, 0x00, sizeof(va_token));

		snprintf(vs_token, 16 - 1, "vs%d", ai_event->ch);

		for ( i = 0; i < IVCA_MAX_ZONES; i++ )
		{
			snprintf(buff, COMMON_SIZE - 1, "cam.dvabx.rule.R%d.Z%d.id", ai_event->ch, i);
			if ( nf_sysdb_get_int(buff) == ai_event->rule_id )
			{
				zone_index = i;
				break;
			}
		}
		
		memset(buff, 0x00, sizeof(buff));
		snprintf(buff, COMMON_SIZE - 1, "cam.dvabx.rule.R%d.Z%d.name", ai_event->ch, zone_index);
		snprintf(va_token, 16 - 1, "va%d_%d_%s", ai_event->ch, ai_event->rule_id, nf_sysdb_get_str_nocopy(buff));

		sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "VideoSourceConfigurationToken", vs_token);
		p = stpcpy(p, tmp);
		sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "VideoAnalyticsConfigurationToken", va_token);
		p = stpcpy(p, tmp);

		sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "Rule", nf_sysdb_get_str_nocopy(buff));

		p = stpcpy(p, tmp);
		sprintf(tmp, "</tt:Source>");
		p = stpcpy(p, tmp);

		if ( ai_event->type == IVCA_ET_LOITERED )
		{
			snprintf(db_field, 64 - 1, "cam.dvabx.rule.R%d.Z%d.time_sarlf", ai_event->ch, zone_index);
			str_ptr = nf_sysdb_get_str_nocopy(db_field);
			sscanf(str_ptr, "%hu %hu %hu %hu %hu", &stop_time,&abandon_time, &remove_time, &loiter_time,&fall_time);

			since = ai_event->timestamp - loiter_time;

			memset(tmp_time, 0x00, sizeof(tmp_time));
			strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&ai_event->timestamp));
			snprintf(since_time, 128 - 1, "%s.%3d", tmp_time, ai_event->timestampl / 1000);
			
			sprintf(tmp, "<tt:Key>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%d\"/>", "ObjectId", ai_event->object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Key>");
			p = stpcpy(p, tmp);

			sprintf(tmp, "<tt:Data>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "Since", since_time);
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "TimeStamp", utc_time);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Data>");
			p = stpcpy(p, tmp);
		}

		if ( ai_event->type == IVCA_ET_DIR_POS || ai_event->type == IVCA_ET_DIR_NEG )
		{
			sprintf(tmp, "<tt:Data>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%d\"/>", "ObjectId", ai_event->object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Data>");
			p = stpcpy(p, tmp);
		}

		if ( ai_event->type == IVCA_ET_ENTER || ai_event->type == IVCA_ET_EXIT || ai_event->type == IVCA_ET_INTRUSION)
		{
			sprintf(tmp, "<tt:Key>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%d\"/>", "ObjectId", ai_event->object_id);
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Key>");
			p = stpcpy(p, tmp);
			sprintf(tmp, "<tt:Data>");
			p = stpcpy(p, tmp);
			if ( ai_event->type == IVCA_ET_ENTER || ai_event->type == IVCA_ET_INTRUSION)
				sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "IsInside", "true");
			if ( ai_event->type == IVCA_ET_EXIT )
				sprintf(tmp, "<tt:SimpleItem Name=\"%s\" Value=\"%s\"/>", "IsInside", "false");
			
			p = stpcpy(p, tmp);
			sprintf(tmp, "</tt:Data>");
			p = stpcpy(p, tmp);
		}
		sprintf(tmp, "</tt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsnt:NotificationMessage>");
		p = stpcpy(p, tmp);
	}
	else
	{
		free(tmp);
		return NULL;	
	}
	
	free(tmp);
	
	return p;
}

gboolean nf_rec_va_metadata_handoff(int key, void *msg_data, int ch)
{
	ICODEC_HEADER       pheader;
	GTimeVal		req_timeval;
	ONVIF_VA_META_FRAME *va_data = NULL;
	ivca_rule_event_t *va_event = NULL;
	ai_rule_event_t *ai_event;
	
	// make icodec header
	memset(&req_timeval, 0x00, sizeof(GTimeVal));
	g_get_current_time(&req_timeval);
	
	pheader.codec = NF_CODEC_TYPE_METADATA;
	pheader.flags = 0;
	pheader.version = NF_CODEC_VERSION_1;
	pheader.frame_type = NF_FRAME_TYPE_METADATA;
	if ( key == E_KEY_VA_DATA )
	{
		pheader.resolution = 1;
	}
	if ( key == E_KEY_VA_EVENT || key == E_KEY_AI_EVENT )
	{
		pheader.resolution = 2;
	}
	
	pheader.frame_rate = NF_FPS_CR01;
	
	// TODO modified timestamp
	pheader.timestampl = (guchar)(req_timeval.tv_usec/1000/5);
	pheader.timestamp = (guint)req_timeval.tv_sec;
	
	pheader.gst_buffer = 0;
	// make xml message
	
	unsigned int buflen=0;
	char *send_buf = NULL;
	send_buf = (char *)malloc(MAX_NOTIFY_BUFLEN * sizeof(char));
	if ( send_buf == NULL )
		return 0;
	
	char *p=NULL;
	// make notification message
	memset(send_buf, 0x00, sizeof(char)*MAX_NOTIFY_BUFLEN);
	
	p = make_buf_metadata_head(send_buf);

	if ( key == E_KEY_VA_DATA )
	{
		va_data = (ONVIF_VA_META_FRAME *)msg_data;
		
		p = make_buf_metadata_va_head(p);		// <tt:VideoAnalytics>
		p = make_va_buf_notification_no_dest(p, va_data, key, va_data->meta_count);
		if ( p == NULL )
		{
			free(send_buf);
			return 0;
		}
		p = make_buf_metadata_va_tail(p);		// </tt:VideoAnalytics>
	}
	if ( key == E_KEY_VA_EVENT )
	{
		va_event = (ivca_rule_event_t *)msg_data;

		if ( va_event->type == IVCA_ET_STOPPED || va_event->type == IVCA_ET_ABANDONED )
		{
			free(send_buf);
			return 0;
		}
		
		p = make_buf_metadata_event_head(p);		// <tt:Event>
		p = make_va_buf_notification_no_dest(p, va_event, key, 1);
		if ( p == NULL )
		{
			free(send_buf);
			return 0;
		}
		p = make_buf_metadata_event_tail(p);			// </tt:Event>
	}
	if ( key == E_KEY_AI_EVENT )
	{
		ai_event = (ai_rule_event_t *)msg_data;

		if ( ai_event->type == IVCA_ET_STOPPED || ai_event->type == IVCA_ET_ABANDONED )
		{
			free(send_buf);
			return 0;
		}
		
		p = make_buf_metadata_event_head(p);		// <tt:Event>
		p = make_va_buf_notification_no_dest(p, ai_event, key, 1);
		if ( p == NULL )
		{
			free(send_buf);
			return 0;
		}
		p = make_buf_metadata_event_tail(p);			// </tt:Event>
	}
	
	p = make_buf_metadata_tail(p);
	buflen = (unsigned int)(p - send_buf);
	pheader.chan = ch;
	pheader.frame_size = buflen + 1;

	_nf_rec_metadata_handoff(ch, (gpointer)&pheader, send_buf );

#ifdef ONVIF_METADATA_DEBUG
	printf("\e[31m [%s] \e[0m\n", send_buf);
#endif

	free(send_buf);
}
static gboolean nf_dva_metadata_send(VA_EVT_DATA *p_data)
{
	char *data = NULL;
	char *encode_data = NULL;
	char source[4];
	char buff[64];
	char class_name[64];
	char *json_dump_data = NULL;
	int i = 0, j = 0;
	long timestamp, cur_timestamp;
	unsigned int timestampl, cur_timestampl;
	double r_timestamp = 0.0;
	guint64 process_time = 0;
	GTimeVal	req_timeval;
	double score;

	data = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if ( data == NULL )
		return 0;

	encode_data = (char *)malloc(MAX_NOTIFY_DATA_LEN * sizeof(char));
	if ( encode_data == NULL )
	{
		free(data);
		return 0;
	}
	
	memset(data, 0x00, MAX_NOTIFY_DATA_LEN);
	memset(encode_data, 0x00, MAX_NOTIFY_DATA_LEN);
	// for calculating process_time
	memset(&req_timeval, 0x00, sizeof(GTimeVal));
	g_get_current_time(&req_timeval);

	cur_timestampl = (guchar)(req_timeval.tv_usec/1000);
	cur_timestamp = (guint)req_timeval.tv_sec;

	if ( p_data == NULL)
	{
		free(data);
		free(encode_data);
		return FALSE;
	}

	// timestamp convert
	//timestamp = (guint)(dva_event[0].u64TimeStamp/1000000);
	//timestampl = (guchar)(dva_event[0].u64TimeStamp%1000000);
	r_timestamp = p_data->timestamp + (double)(p_data->timestampl / 1000000.0);
	r_timestamp = floor(r_timestamp * 1000) / 1000;
	
	memset(class_name, 0x00, sizeof(class_name));

	json_t* _json_root = json_object();
	json_t* _json_meta = NULL;
	json_t* _json_image = NULL;
	json_t* _json_annot = NULL;
	json_t* _json_bbox = NULL;
	json_t* _json_face = NULL;
	json_t* _json_groups = NULL;
	json_t* _json_face_attribute = NULL;

	snprintf(source, sizeof(source) - 1, "%d", p_data->ch);

	process_time = p_data->process_time;
	
	json_object_set_new(_json_root, "source", json_string(source));
	json_object_set_new(_json_root, "topic", json_string(p_data->topic));
	json_object_set_new(_json_root, "timestamp", json_real(r_timestamp));
	if ( process_time != 0 )
		json_object_set_new(_json_root, "process_time", json_integer(process_time));
	json_object_set_new(_json_root, "metadata", json_object());
	
	//metadata
	_json_meta = json_object_get(_json_root, "metadata");
	json_object_set_new(_json_meta, "annotations", json_array());

	for ( i = 0; i < p_data->meta_count; i++ )
	{
		memset(class_name, 0x00, sizeof(class_name));
		strncpy(class_name, p_data->meta_data[i].class,  sizeof(class_name) - 1);
		
		//metadata.annotations
		_json_annot = json_object_get(_json_meta, "annotations");
		json_array_append_new(_json_annot, json_object());
		
		//metadata.annotations[i]
		_json_annot = json_array_get(_json_annot, i);
		
		if ( p_data->meta_data[i].id >= 0 )
			json_object_set_new(_json_annot, "track_id", json_integer(p_data->meta_data[i].id));
		json_object_set_new(_json_annot, "class", json_string(class_name));
		json_object_set_new(_json_annot, "score", json_real(p_data->meta_data[i].confidence));
		json_object_set_new(_json_annot, "bbox", json_array());

		//metadata.annotations[i].bbox
		_json_bbox = json_object_get(_json_annot, "bbox");
		json_array_append_new(_json_bbox, json_real(p_data->meta_data[i].bbx_position[0]));
		json_array_append_new(_json_bbox, json_real(p_data->meta_data[i].bbx_position[1]));
		json_array_append_new(_json_bbox, json_real(p_data->meta_data[i].bbx_position[2]));
		json_array_append_new(_json_bbox, json_real(p_data->meta_data[i].bbx_position[3]));
		//printf("\e[31m i[%d] class[%d] left[%f] top[%f] right[%f] bottom[%f] \e[0m\n", i, dva_event[i].u32Class, dva_event[i].fBbox[0], dva_event[i].fBbox[1],
		//																		dva_event[i].fBbox[2], dva_event[i].fBbox[3]);

		//FIXME : current face data object "0" or "1"
		if (p_data->is_face_data)
		{
			if ( p_data->meta_data[i].face_data.info_cnt > 0)
		{
			json_object_set_new(_json_annot, "face", json_object());
			_json_face = json_object_get(_json_annot, "face");

				if ( p_data->meta_data[i].face_data.info[0].name[0] != '\0')
					json_object_set_new(_json_face, "name", json_string(p_data->meta_data[i].face_data.info[0].name));

				json_object_set_new(_json_face, "id", json_integer(p_data->meta_data[i].face_data.info[0].face_id));

				if ( p_data->meta_data[i].face_data.info[0].group_cnt > 0 )
			{
				json_object_set_new(_json_face, "groups", json_array());
				_json_groups = json_object_get(_json_face, "groups");

					for ( j = 0; j < p_data->meta_data[i].face_data.info[0].group_cnt; j++)
						json_array_append_new(_json_groups, json_string(p_data->meta_data[i].face_data.info[0].group_name[j]));
			}
			
			json_object_set_new(_json_face, "attribute", json_object());
			_json_face_attribute = json_object_get(_json_face, "attribute");

				json_object_set_new(_json_face_attribute, "age", json_integer(p_data->meta_data[i].face_data.info[0].age));
				json_object_set_new(_json_face_attribute, "gendor", json_string(p_data->meta_data[i].face_data.info[0].gender));
				json_object_set_new(_json_face_attribute, "headwear", json_string(p_data->meta_data[i].face_data.info[0].headwear));
				json_object_set_new(_json_face_attribute, "glasses", json_string(p_data->meta_data[i].face_data.info[0].glasses)); 

				json_object_set_new(_json_face, "search_score", json_real(p_data->meta_data[i].face_data.info[0].search_score));
			}
		}
		if ( p_data->is_lpr_data )
		{
			json_object_set_new(_json_annot, "lp_text", json_string(p_data->meta_data[i].lp_text));
		}
	}

	json_dump_data = json_dumps(_json_root, JSON_COMPACT);
	json_decref(_json_root);

	memcpy(data, json_dump_data, strlen(json_dump_data));
	if ( json_dump_data != NULL)
		free(json_dump_data);

#ifdef ONVIF_METADATA_DEBUG
	printf("\e[34m %s \e[0m\n", data);
#endif
	base64_encode(data, strlen(data), encode_data);
	//printf("\e[31m %s \e[0m\n", encode_data);

	// base64json data send
	nf_rec_base64json_va_metadata_handoff(E_KEY_DVA, encode_data, p_data->ch);

	if ( data )
		free(data);
	if ( encode_data )
		free(encode_data);
}
gboolean nf_rec_base64json_va_metadata_handoff(int key, char *msg_data, int ch)
{
	ICODEC_HEADER       pheader;
	GTimeVal		req_timeval;
	
	// make icodec header
	memset(&req_timeval, 0x00, sizeof(GTimeVal));
	g_get_current_time(&req_timeval);
	
	pheader.codec = NF_CODEC_TYPE_METADATA;
	pheader.flags = 0;
	pheader.version = NF_CODEC_VERSION_1;
	pheader.frame_type = NF_FRAME_TYPE_METADATA;
	pheader.resolution = 0;
	pheader.frame_rate = NF_FPS_CR01;
	
	// TODO modified timestamp
	pheader.timestampl = (guchar)(req_timeval.tv_usec/1000/5);
	pheader.timestamp = (guint)req_timeval.tv_sec;
	
	pheader.gst_buffer = 0;
	// make xml message
	
	unsigned int buflen=0;
	char *send_buf = NULL;
	send_buf = (char *)malloc(MAX_NOTIFY_BUFLEN * sizeof(char));
	if ( send_buf == NULL )
		return 0;
	char *p=NULL;
	// make notification message
	memset(send_buf, 0x00, sizeof(char)*MAX_NOTIFY_BUFLEN);
	p = make_buf_metadata_head(send_buf);
	p = make_buf_metadata_event_head(p);
	p = make_base64json_va_buf_notification_no_dest(p, msg_data, key, 1);
	if ( p == NULL )
	{
		free(send_buf);
		return 0;
	}
	p = make_buf_metadata_event_tail(p);
	p = make_buf_metadata_tail(p);
	buflen = (unsigned int)(p - send_buf);
	pheader.chan = ch;
	pheader.frame_size = buflen + 1;

	_nf_rec_metadata_handoff(ch, (gpointer)&pheader, send_buf );

	free(send_buf);
}



///////////////// MESSAGE FILTER ///////////////////
char* next_find_char(char *p, const char compare, char *p_end)
{
	if (*p == '\0'){
		return 0;
	}
	
	if ( p_end - p <= 0){
		return 0;		
	}
	
	while ( *p != compare ) {
		if (*p == '\0' || p == p_end)
			return 0;
		p++;
	}
	return p + 1;
}

char* next_find_str(char *p, const char *compare, int size, char *p_end)
{
	if (*p == '\0'){
		return 0;
	}
	
	if ( p_end - p <= 0){
		return 0;		
	}	
	while ( strncmp(p, compare, size) ) {
		if (*p == '\0' || p == p_end)
			return 0;
		p++;
	}
	return p + size;
}

char* next_match_char(char *p, const char compare)
{
	if (*p == '\0') {
		return 0;
	}
	
	while ( *p != compare ) {
		if ( *p == ' ' || *p == '\n' || *p == '\r'){
			p++;
		}
		else{
			return NULL;
		}
	}
	return p + 1;
}

char* next_match_str(char *p, const char *compare, int size)
{
	if (*p == '\0'){
		return 0;
	}
	while ( strncmp(p, compare, size) )
	{
		if ( *p == ' ' || *p == '\n' || *p == '\r'){
			p++;
		}
		else{
			return NULL;
		}
	}
	return p + size;
}

char* find_end_tag(char* p, char *p_end)
{
	int depth = 1; // current pointer
	
	if (*p == '\0'){
		return 0;
	}

	while ( *p != '\0' && depth != 0 && p < p_end) {
		if ( *p == '(') {
			depth++;
		}
		else if ( *p == ')') {
			depth--;
		}
		p++;
	}

	if ( depth != 0 ) {
		return 0;
	}

	return p + 1;
}


#define MAX_FILTER_CNT  (10)

void print_topic_ex_list(char **t, int size)
{
	int i;
	char te[MAX_FILTER_CNT][512];

	memset(te, 0, sizeof(te));
	memcpy(te, t, sizeof(te));

	if (size)
	{
		_TTY_LOG_ONVIF("******** Topic Expression List *********");
		for (i = 0; i < size; i++)
		{
			if ( te[i] && te[i][0] != '\0')
				_TTY_LOG_ONVIF("te[%d][%s]", i, te[i]);
		}
		_TTY_LOG_ONVIF("****************************************");
	}
}

char* get_flags_from_attr_comp(char *str, int *flags, char * str_end)
{
	char *p, *p_tmp, *p_end;
	int ret_flags = 0;

	p = str;

	if( str_end - str <= 0) {
		return 0;
	}

	if ( (p_tmp = next_match_char(p, '@')))
	{
		p = p_tmp;
		if ((p_tmp = next_match_str(p, "Name", 4)))
		{
			p = p_tmp;
			p = next_match_char(p, '=');
			if (!p)
				return NULL;
			if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
			{
				p = p_tmp;
			}
			else
				return NULL;
			if ((p_tmp = next_match_str(p, "VideoSourceConfigurationToken", 29)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "MotionActive", 12)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "Port", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
				ret_flags |= ONVIF_F_ALARM_OPEN;
			}
			else if ((p_tmp = next_match_str(p, "State", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
				ret_flags |= ONVIF_F_ALARM_OPEN;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "Source", 6)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}

			else
				return NULL;
		}
		else if ((p_tmp = next_match_str(p, "Value", 5)))
		{
			p = p_tmp;
			p = next_match_char(p, '=');
			if (!p)
				return NULL;
			if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
			{
				p = p_tmp;
			}
			else
				return NULL;
			if ((p_tmp = next_match_str(p, "general_vsource", 15)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "true", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_ON;
			}
			else if ((p_tmp = next_match_str(p, "false", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_OFF;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "close", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
			}
			else if ( (p_tmp = next_match_str(p, "open", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_OPEN;
			}
			else
				return NULL;

		}
		else
			return NULL;

		if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
		{
			p = p_tmp;
		}
		else
			return NULL;

		*flags = ret_flags; // return filter flags
	}
	else if ( (p_tmp = next_match_char(p, '(')))
	{
		p = p_tmp;
		p_end = find_end_tag(p, str_end);
		if (p_end == NULL )
			return 0;		
		if ( (p = get_flags_from_attr_comp( p, &ret_flags, p_end)) )
		{
			p = next_match_char(p, ')');
			if (!p){
				free(p_tmp);
				return NULL;
			}			
		}
		else{
			free(p_tmp);
			return NULL;
		}
		free(p_tmp);		

		*flags = ret_flags; // return filter flags
	}
	else if ( (p_tmp = next_match_str(p, "not", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(p, &ret_flags, str_end)) )
		{
			*flags = ~ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	if ((p_tmp = next_match_str(p, "and", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(p, &ret_flags, str_end)) )
		{
			*flags &= ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	if ((p_tmp = next_match_str(p, "or", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(p, &ret_flags, str_end)) )
		{
			*flags |= ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	return p;
}

int get_flags_from_node(char *str,  char *str_end)
{
	char *p, *p_tmp, *p_end;
	int ret_flags = 0, i;

	if( str_end - str <= 0) {
		return 0;
	}

	p = next_match_str(str, "//", 2);
	if (!p)
		return 0;	

	if ((p = next_find_str(str, "SimpleItem", 10, str_end)))
	{
		p = next_match_char(p, '[');
		if (!p)
			return 0;
		p_end = next_find_char(p, ']', str_end);
		if (!p_end)
			return 0;

		if ( (p = get_flags_from_attr_comp( p, &ret_flags, p_end)))
		{
			;
		}
		else{
//			g_assert(0);
//			free(p_tmp);
			return 0;
		}
//		g_assert(0);
//		free(p_tmp);
	}
	else
		return 0;

	return ret_flags;
}

int apply_contents_ex(MSG_FilterExpr* p_expr)
{
	int tmp_flags = 0;
	char *ex_end = 0;

	if (p_expr != NULL)
	{
		if (p_expr->ex_boolean != NULL)
		{
			if (p_expr->ex_boolean->ex_path != NULL)
			{
				if (p_expr->ex_boolean->ex_path->ex_path_str != NULL && p_expr->ex_boolean->ex_path->ex_path_str[0] != '\0')
				{
					if ( p_expr->_not )
					{
						ex_end = p_expr->ex_boolean->ex_path->ex_path_str+ p_expr->ex_boolean->ex_path->_size;
						tmp_flags = ~get_flags_from_node(p_expr->ex_boolean->ex_path->ex_path_str, ex_end);
					}
					else
					{
						ex_end = p_expr->ex_boolean->ex_path->ex_path_str+ p_expr->ex_boolean->ex_path->_size;
						tmp_flags = get_flags_from_node(p_expr->ex_boolean->ex_path->ex_path_str, ex_end);
					}
				}
			}
		}
		if ( p_expr->p_child != NULL )
		{
			tmp_flags = apply_contents_ex(p_expr->p_child);
		}
		if ( p_expr->p_next_and != NULL )
		{
			tmp_flags &= apply_contents_ex(p_expr->p_next_and);
		}
		if ( p_expr->p_next_or != NULL )
		{
			tmp_flags |= apply_contents_ex(p_expr->p_next_and);
		}
	}

	return tmp_flags;
}

void print_contents_list(MSG_FilterExpr* p_expr)
{
	if (p_expr != NULL)
	{
		if ( p_expr->_not )
			_TTY_LOG_ONVIF("(not)");
		if (p_expr->ex_boolean != NULL)
		{
			if (p_expr->ex_boolean->ex_path != NULL)
			{
				if (p_expr->ex_boolean->ex_path->ex_path_str != NULL
				        && p_expr->ex_boolean->ex_path->ex_path_str[0] != '\0')
				{
					_TTY_LOG_ONVIF("%s", p_expr->ex_boolean->ex_path->ex_path_str);
				}
			}
		}
	}
	if ( p_expr->p_child != NULL )
	{
		_TTY_LOG_ONVIF(">>>>");
		print_contents_list(p_expr->p_child);
		_TTY_LOG_ONVIF("<<<<");
	}
	if ( p_expr->p_next_and != NULL )
	{
		_TTY_LOG_ONVIF("and");
		print_contents_list(p_expr->p_next_and);
	}
	if ( p_expr->p_next_or != NULL )
	{
		_TTY_LOG_ONVIF("or");
		print_contents_list(p_expr->p_next_or);
	}
}

MSG_FilterExpr* get_content_expr(const char *str, char * str_end)
{
	char *p, *p_end, *p_tmp;
	MSG_FilterExpr *msg_filter;
	int ret = 0;

	if ( *str == '\0' )
		return 0;

	if( str_end - str <= 0) {
		return 0;
	}
	
	msg_filter = (MSG_FilterExpr *)malloc(sizeof(MSG_FilterExpr));
	if( !msg_filter ){
		return 0;
	}	
	memset(msg_filter, 0, sizeof(MSG_FilterExpr));

	p = (char *)str;
	if ( *p != '\0' )
	{
		p_tmp = next_match_str(p, "boolean", 7);
		if (p_tmp != NULL)
		{
			p = p_tmp;
			msg_filter->ex_boolean = (BoolExpr *)malloc(sizeof(BoolExpr));
			memset(msg_filter->ex_boolean, 0, sizeof(BoolExpr));
			p = next_match_char(p, '(');
			if ( p == NULL ){
				g_assert(0);
				return 0;
			}
			else
			{
				msg_filter->ex_boolean->ex_path = (PathExpr *)malloc(sizeof(PathExpr));
				memset(msg_filter->ex_boolean->ex_path, 0, sizeof(PathExpr));
				p_end = next_find_char(p, ')', str_end);
				if (p_end == NULL){
					g_assert(0);
					return 0;
				}
				else
				{
					msg_filter->ex_boolean->ex_path->_size = sizeof(char) * (p_end - p);
					msg_filter->ex_boolean->ex_path->ex_path_str = (char *)malloc( msg_filter->ex_boolean->ex_path->_size);
					memset(msg_filter->ex_boolean->ex_path->ex_path_str, 0, msg_filter->ex_boolean->ex_path->_size);
					memcpy(msg_filter->ex_boolean->ex_path->ex_path_str, p, msg_filter->ex_boolean->ex_path->_size-1);
					p = p_end;
					p_tmp = next_match_str(p, "and", 3);
					if (p_tmp != NULL)
					{
						p = p_tmp;
						msg_filter->p_next_and = get_content_expr(p, str_end);
						if (msg_filter->p_next_and == NULL){
							g_assert(0);
							return 0;
						}
						else
						{
							msg_filter->p_next_and->p_prev = msg_filter;
							return msg_filter;
						}
					}
					else
					{
						p_tmp = next_match_str(p, "or", 2);
						if (p_tmp != NULL)
						{
							p = p_tmp;
							msg_filter->p_next_or = get_content_expr(p, str_end);
							if (msg_filter->p_next_or == NULL){
								g_assert(0);
								return 0;
							}
							else
							{
								msg_filter->p_next_or->p_prev = msg_filter;
								return msg_filter;
							}
						}
						else {
							return msg_filter;
						}
					}

				}
			}
		}
		else
		{
			p_tmp = next_match_str(p, "not", 3);
			if ( p_tmp != NULL)
			{
				p = p_tmp;
				msg_filter->_not = 1;
				p = next_match_char(p, '(');
				if ( p == NULL ){
					g_assert(0);
					return 0;
				}
				else
				{
					p = next_match_str(p, "boolean", 7);
					if ( p == NULL ){
						g_assert(0);
						return 0;
					}
					else
					{
						msg_filter->ex_boolean = (BoolExpr *)malloc(sizeof(BoolExpr));
						memset(msg_filter->ex_boolean, 0, sizeof(BoolExpr));
						p = next_match_char(p, '(');
						if ( p == NULL ){
							g_assert(0);
							return 0;
						}
						else
						{
							msg_filter->ex_boolean->ex_path = (PathExpr *)malloc( sizeof(PathExpr));
							memset(msg_filter->ex_boolean->ex_path, 0, sizeof(PathExpr));
							p_end = next_find_char(p, ')', str_end);
							if (p_end == NULL){
								g_assert(0);
								return 0;
							}
							else
							{
								msg_filter->ex_boolean->ex_path->_size = sizeof(char) * (p_end - p);
								msg_filter->ex_boolean->ex_path->ex_path_str = (char *)malloc(msg_filter->ex_boolean->ex_path->_size );
								memset(msg_filter->ex_boolean->ex_path->ex_path_str, 0, msg_filter->ex_boolean->ex_path->_size);
								memcpy(msg_filter->ex_boolean->ex_path->ex_path_str, p, msg_filter->ex_boolean->ex_path->_size-1);
								p = p_end;
								p_tmp = next_match_str(p, "and", 3);
								if (p_tmp != NULL)
								{
									p = p_tmp;
									msg_filter->p_next_and = get_content_expr(p, str_end);
									if (msg_filter->p_next_and == NULL){
										g_assert(0);
										return 0;
									}
									else
									{
										msg_filter->p_next_and->p_prev = msg_filter;
										return msg_filter;
									}
								}
								else
								{
									p_tmp = next_match_str(p, "or", 2);
									if (p_tmp != NULL)
									{
										p = p_tmp;
										msg_filter->p_next_or = get_content_expr( p, str_end);
										if (msg_filter->p_next_or == NULL){
											g_assert(0);
											return 0;
										}
										else
										{
											msg_filter->p_next_or->p_prev = msg_filter;
											return msg_filter;
										}
									}
									else
										return msg_filter;
								}
							}
						}
					}
				}
			}
			else
			{
				p_tmp = next_match_char(p, '(');
				if ( p_tmp != NULL)
				{
					p = p_tmp;
					p_end = find_end_tag(p, str_end);
					if (p_end == NULL ){
						g_assert(0);
						return 0;
					}

					msg_filter->p_child = get_content_expr(p, p_end);
					if (msg_filter->p_child == NULL){{
						g_assert(0);
						return 0;
					}
					}
					else{
						msg_filter->p_child->p_parent = msg_filter;
					}

					p = p_end;
					p_tmp = next_match_str(p, "and", 3);
					if (p_tmp != NULL)
					{
						p = p_tmp;
						msg_filter->p_next_and = get_content_expr(p, str_end);
						if (msg_filter->p_next_and == NULL){
							g_assert(0);
							return 0;
						}
						else
						{
							msg_filter->p_next_and->p_prev = msg_filter;
							return msg_filter;
						}
					}
					else
					{
						p_tmp = next_match_str(p, "or", 2);
						if (p_tmp != NULL)
						{
							p = p_tmp;
							msg_filter->p_next_or = get_content_expr(p, str_end);
							if (msg_filter->p_next_or == NULL){
								g_assert(0);
								return 0;
							}
							else
							{
								msg_filter->p_next_or->p_prev = msg_filter;
								return msg_filter;
							}
						}
						else
							return msg_filter;
					}
				}
				else{
					g_assert(0);
					return 0;
				}
			}
		}
	}
	return msg_filter;
}

MSG_FilterExpr* get_msg_contents_filter(const char *str, char *str_end)
{
	/* WARNING : This function use malloc !!! use this with clear_msg_filter() */
	char *p, *p_end, tmp[512];
	MSG_FilterExpr *msg_filter;

	if( str_end - str <= 0) {
		return 0;
	}

	msg_filter = get_content_expr(str, str_end);
	if (msg_filter == NULL)
		return NULL;

	return msg_filter;
}


int get_topic_ex_filter(const char *str, char **tmp_topic_ex)
{
	char *p, *p_end, te_str[512], topic_ex[MAX_FILTER_CNT][512];
	int i, size_of_topic_ex = 0;

	memset(te_str, 0, sizeof(te_str));
	memset(topic_ex, 0, sizeof(topic_ex));
	p = strstr(str, ":TopicExpression");

	if (p)
	{
		p = strchr(p, '>');
		p++;
		if (p != NULL && *p != '\0' && *p != '<' )
		{
			p_end = strchr(p, '<');
			if (!p_end)
				return 0;
			if ( (p_end - p) < 512 )
			{
				strncpy(te_str, p, p_end - p);
			}
			else
				return 0;
		}
	}
	else
		return 0;

	p = te_str;
	for (i = 0; i < MAX_FILTER_CNT; i++)
	{
		p_end = strstr(p, "//.");
		if (p_end)
		{
			strncpy(topic_ex[i], p, p_end - p);
		}
		else
		{
			/* There is no "//." tag */
			p_end = strchr(p, '\0');
			if ( (p_end - p) < 512 )
			{
				strncpy(topic_ex[i], p, p_end - p);
				i++;
				break;
			}
			else
				return 0;
		}
		p = strchr(p, '|');
		if (!p)
		{
			i++;
			break;
		}
		else
			p++;
	}

	size_of_topic_ex = i;
	memcpy(tmp_topic_ex, topic_ex, sizeof(topic_ex));

	return size_of_topic_ex;
}

int clear_msg_filter(MSG_FilterExpr* msg_filter)
{
	if( !msg_filter ){
		return 0;
	}

	if(msg_filter->ex_boolean ){
		if(msg_filter->ex_boolean->ex_path ){
			if( msg_filter->ex_boolean->ex_path->ex_path_str ){
				free(msg_filter->ex_boolean->ex_path->ex_path_str);
				msg_filter->ex_boolean->ex_path->ex_path_str = NULL;
			}	
			free(msg_filter->ex_boolean->ex_path);
			msg_filter->ex_boolean->ex_path  = NULL;			
		}
		free(msg_filter->ex_boolean);
		msg_filter->ex_boolean = NULL;
	}
	
	if(msg_filter->p_next_and){
		clear_msg_filter(msg_filter->p_next_and);
		msg_filter->p_next_and = NULL;
	}

	if(msg_filter->p_next_or){
		clear_msg_filter(msg_filter->p_next_or);
		msg_filter->p_next_or = NULL;
	}

	if(msg_filter->p_child){
		clear_msg_filter(msg_filter->p_child);
		msg_filter->p_child = NULL;
	}
}

extern GAsyncQueue *onvif_get_meta_queue()
{
    return (GAsyncQueue *)meta_queue;
}

extern GAsyncQueue *onvif_get_event_queue()
{
	return (GAsyncQueue *)event_queue;
}


#define MAX_STACK 100

typedef struct {
    char items[MAX_STACK];
    int top;
} Stack;

void initStack(Stack *s) 
{
    s->top = -1;
}

bool isEmpty(Stack *s) 
{
    return s->top == -1;
}

void push(Stack *s, char c) 
{
    if (s->top < MAX_STACK - 1) {
        s->items[++(s->top)] = c;
    }
}

char pop(Stack *s) 
{
    if (!isEmpty(s)) {
        return s->items[(s->top)--];
    }
    return '\0';
}

bool areParenthesesBalanced(const char *str) 
{
    Stack s;
    initStack(&s);

    for (int i = 0; str[i]; i++) {
        if (str[i] == '(') {
            push(&s, str[i]);
        } else if (str[i] == ')') {
            if (isEmpty(&s)) return false;
            pop(&s);
        }
    }

    return isEmpty(&s);
}                                                                 

int isInvalidONVIFTopic(const char *topic) 
{
    if (strncmp(topic, "boolean(", 8) != 0) {
        return 1;
    }

    if (!areParenthesesBalanced(topic)) {
        return 1;
    }

    const char *simpleItem = strstr(topic, ":SimpleItem[@Name=\"");
    if (!simpleItem) {
        return 1;
    }

    const char *nameStart = simpleItem + 19; // "//tns1:SimpleItem[@Name=\"" next position
    const char *nameEnd = strchr(nameStart, '\"');
    if (!nameEnd || nameEnd == nameStart) {
        return 1;
    }

    const char *logicOp = strstr(nameEnd, " and ") ? : strstr(nameEnd, " or ");
    if (!logicOp) {
        return 1;
    }
	else {
    	const char *valuestr = strstr(logicOp, "@Value");
		if (!valuestr) {
			return 1;
		}
	}

    return 0;
}

int getContentValue(const char *topic, char *buf) 
{
    if (strncmp(topic, "boolean(", 8) != 0) {
        return 1;
    }

    if (!areParenthesesBalanced(topic)) {
        return 1;
    }

    const char *simpleItem = strstr(topic, ":SimpleItem[@Name=\"");
    if (!simpleItem) {
        return 1;
    }

    const char *nameStart = simpleItem + 19; // "//tns1:SimpleItem[@Name=\"" next position
    const char *nameEnd = strchr(nameStart, '\"');
    if (!nameEnd || nameEnd == nameStart) {
        return 1;
    }

    const char *logicOp = strstr(nameEnd, " and ") ? : strstr(nameEnd, " or ");
    if (!logicOp) {
        return 1;
    }
	else {
    	const char *valuestr = strstr(logicOp, "@Value");
		if (!valuestr) {
			return 1;
		}
		else {
			// 7 means the length of @Value="
			const char *end = strstr(valuestr + 7 + 1, "\"");
			if (!end) {
				return 1;
			}
			else {
				strncpy(buf, valuestr + 8, end - (valuestr + 8));
				strcat(buf, "\0");

			}
		}
	}

    return 0;
}

void remove_trailing_number(char *str) {
    int len = strlen(str);
    int i = len - 1;
    
    while (i >= 0 && isdigit(str[i])) {
        i--;
    }
    
    if (i >= 0) {
        str[i+1] = '\0';
    }

    else {
        str[0] = '\0';
    }
}

int extract_trailing_number(const char *str) {
    int len = strlen(str);
    int i = len - 1;
    
    while (i >= 0 && isdigit(str[i])) {
        i--;
    }
    
    i++;
    
    if (i == len) {
        return 0;
    }
    
    return atoi(&str[i]);
}

int check_topic_filter(char *te, char *topic)
{
	gchar **splited_text = NULL;
	gint count_splited_text = 0;		


	char *pos = strstr(te, "/.");
	if (pos != 0) {
		char tmp[128];
		memset(tmp, 0x00, sizeof(tmp));
		strncpy(tmp, te, pos - te);

		if (strstr(topic, tmp) != 0) {
			return 1;
		}
	}

	splited_text = g_strsplit(te, "|", MAX_MSG_FILTER_CNT);
	count_splited_text = g_strv_length(splited_text);
	int i = 0;
	for (i = 0; i < count_splited_text; ++i) {
		if (strncmp(topic, splited_text[i], strlen(splited_text[i])) == 0) {
			g_strfreev(splited_text); 	
			return 1;
		}
	}
	g_strfreev(splited_text); 	

	return 0;
}

int check_invalid_topic(char *te)
{
    if (
        check_topic_filter(te, str_tns1_tnsitx_Alarm)
        || check_topic_filter(te, str_tns1_tnsitx_Motion)
        || check_topic_filter(te, str_tns1_VideoAnalytics_Motion)
        || check_topic_filter(te, str_tns1_VideoAnalytics_Motion)
        || check_topic_filter(te, str_tns1_VideoAnalytics_Motion)
        || check_topic_filter(te, str_tns1_VideoAnalytics_Motion)
        || check_topic_filter(te, str_tns1_General_Motion)
        || check_topic_filter(te, str_tns1_tnsitx_jobState_change)
        || check_topic_filter(te, str_tns1_tnsitx_conf_change)
        || check_topic_filter(te, str_tns1_relay)
        || check_topic_filter(te, str_tns1_tnsitx_RecordingConfigTrackConfig)
        || check_topic_filter(te, str_tns1_tnsitx_data_deletion)
        || check_topic_filter(te, str_tns1_recording_and_track)
        || check_topic_filter(te, str_tns1_digital_input)
        || check_topic_filter(te, str_tns1_relay)
        || check_topic_filter(te, str_tns1_GlobalSceneChange)
        || check_topic_filter(te, str_tns1_VideoSignalLoss)
        || check_topic_filter(te, str_tns1_tnsitx_EventConfChange)
        || check_topic_filter(te, str_tns1_tnsitx_AdvancedSensor)
		|| check_topic_filter(te, str_tns1_ConfigurationChange)
		|| check_topic_filter(te, str_tns1_ProfileChange)
		|| check_topic_filter(te, str_tns1_ImageBlur)
        ) {
            return 1;
        }

	// invalid topic expression
    return 0;
}                                                                                    

int make_filter_flags(struct onvif_filters *filters, struct event_subscription *sub )
{
	char *p, *p_end, topic_ex[MAX_FILTER_CNT][512], *str_end;
	int i, ret_flags = 0, size_of_topic_ex = 0;
	MSG_FilterExpr *msg_filter;
	int ret = 0;

	if(filters == NULL || sub == NULL){
		return 0;
	}
	
	if( filters->cnt == 0 ){
		return 0;
	}

	printf("FILTER cnt[%d]\n", filters->cnt);
	
	for(i=0; i<filters->cnt; i++){
		if( filters->unit[i].type == TOPIC_EXPRESSION_TYPE){
			printf("FILTER [%d] TOPIC_EXPRESSION_TYPE [%s]\n", i, filters->unit[i].data);	
			ret = check_invalid_topic(filters->unit[i].data);
			if (ret == 0) {
				return -1;
			}

			ret = apply_topic_ex(filters->unit[i].data, sub);
			if (ret == -1) return -1;
			
		}
		else if( filters->unit[i].type == MESSAGE_CONTENTS_TYPE ){
			printf("FILTER [%d] MESSAGE_CONTENTS_TYPE [%s]\n", i, filters->unit[i].data);			

			if (isInvalidONVIFTopic(filters->unit[i].data)) {
				return -1;
			}

#if 0
			str_end = filters->unit[i].data + strlen(filters->unit[i].data);
			//str_end = filters->unit[i].data + sizeof(char)*MAX_FILTER_DATA_SIZE;
			msg_filter = get_msg_contents_filter(filters->unit[i].data, str_end);
			if ( msg_filter != NULL ) {
				ret = apply_contents_ex(msg_filter);
				//sub->mf = apply_contents_ex(msg_filter);
			}			
			clear_msg_filter(msg_filter);
			msg_filter = NULL;
#endif
			char buf[128];
			int r;
			int chnum;
			r = getContentValue(filters->unit[i].data, buf); 
			if (r == 0) {
				chnum = extract_trailing_number(buf);
			}
			else {
				printf("Could not find a number\n");
				return -1;
			}

			memset(&sub->mf, 0x01, sizeof(struct e_msg_filter)); 

			// the below code only for ODTT
			sub->mf.motion_alarm.ch[chnum]	= ONVIF_NOT_FILTERED;
		}
		else{
			printf("FILTER [%d] type[%d] is wrong !!\n", i, filters->unit[i].type);						
			return -1;
		}
		// apply Message content filter to Topic filter
	}

//	if (ret == 0) return -1;

	return 1;
}
int compare_mraw_data(unsigned char *comp1, unsigned char *comp2)
{
	int i=0;

	for(i=0;i<ONVIF_AREA;i++)
	{
		if(comp1[i] != comp2[i])
		{
			return 1;		
		}
	}
	return 0;
}


