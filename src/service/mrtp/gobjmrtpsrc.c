/*
 * Copyright/Licensing information
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <netdb.h>
#include <arpa/inet.h>

#include "gobjmrtpsrc.h"
#include "gstmrtpdefs.h"


#define LIVE_STATISTICS			(1)
#define  LIVE_PRINT_SIZE		(1)
#define  LIVE_PRINT_INTERVAL	(1)
#define  LIVE_PRINT_2ND			(1)
#define  LIVE_PRINT_3RD			(0)
#define  LIVE_PRINT_AUDIO		(0)
#define  LIVE_PRINT_16CH		(0)

#define BUFFERING_5MSEC			(0)
#define BUFFERING_DYNAMIC		(1)
#define BUFFERING_RELATIVE		(0)
#define MAX_WAIT_COUNT			(20)
#define PRINT_REMAIN			(1)
#define DROP_P_FRAME_ENABLE 	(0)
#define PRINT_P_DROP			(1)
#define STATS_AVERAGE 			(0)
#if STATS_AVERAGE
#define AVERAGE_DURATION_SECS	(10)
static int avg_fcnt[AVERAGE_DURATION_SECS];
static int avg_index;
#endif

#define	MRTP_BUFFERING_DEFAULT_TIME	(40) 

static gint live_audio_ch = 0xff;
static guint last_alarm_flag = 0;
static guint i_only_mask = 0;
static gint skip_cnt[MRTPSRC_MAX_CH][STREAM_MAX];

GTimeVal now_time;
static GTimeVal prev_time;
static GTimeVal last_alarm[MRTPSRC_MAX_CH];
static guint call_cnt = 0;
static guint push_cnt = 0;
static guint frame_cnt = 0;
static guint remain_frame[MRTPSRC_MAX_CH][STREAM_MAX];

#if LIVE_STATISTICS
static guint live_skip[MRTPSRC_MAX_CH];
static guint live_buffering[MRTPSRC_MAX_CH];
static MRTPSRC_LIVE_STAT_T live_stat;
static void MRTPSRC_LIVE_STAT(int, int, MRTPSRC_FRAME_T);
static void MRTPSRC_LIVE_DBG(void);
#endif

/* Filter signals and args */
enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_0,

	ENG_CH_NUM,
	ENG_STREAM_NUM,
	ENG_PORT,
	ENG_IPADDR,
	ENG_FPS,
	ENG_RESOLUTION,
	ENG_LOCATION,
	ENG_RTSPADDR,
	ENG_USERNAME,
	ENG_PASSWORD,
	ENG_MODEL,
	ENG_IONLY_CH,
	ENG_MOTION_CALLBACK,
	ENG_CMD_CALLBACK,

	CMD_OPEN_PLAY,
	CMD_TEARDOWN,
	CMD_PAUSE,
	CMD_RESUME,

	GET_STATE,

	PROP_SILENT
};

static GobjMrtpSrc* this_mrtpsrc = NULL;

extern GobjListBuffer *gobj_list_buffer_new (void);
extern void gobj_list_buffer_push (GobjListBuffer *list_buffer, GObject *child_buffer);

static void gst_mrtp_src_finalize (GObject *object);
static void gst_mrtp_src_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec * pspec);
static void gst_mrtp_src_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec * pspec);
#ifndef NMZ_STANDLONE_MODE
static gboolean gst_mrtp_src_start (void *basesrc);
static gboolean gst_mrtp_src_stop (void *basesrc);
// static int gst_mrtp_src_create (void *basesrc, guint64 offset, guint length, GobjBuddyBuffer **buffer);
#endif


static guint get_ipaddr_from_domainname(gchar*);
static void set_max_fps(GObject*);
static void iframe_only_req(guint);

/* Error logs */
const char* _SYS_ERR_STR[] = {
	"No error",

	"Invalid channel number",
	"Invalid stream number",
	"IP address 0",
	"Invalid port number",
	"Channel duplicated",
	"Invalid IP address",
	"TCP connection failed",
	"OPTION command failed",
	"No audio session",
	"No audio payload type",
	"No audio control",
	"No video session",
	"No video payload type",
	"No video control",
	"No video rtp map",
	"No video fmtp",
	"Video doesn't encoded h.264 type",
	"No video spspps",
	"DESCRIBE command failed",
	"SESSION command failed",
	"RESOURCE failed",

	"PLAY command failed",

	"Undefined RTP type",
	"Unhandled RTP type",
	"Buddy buffer error",

	"Teardown from internal request",
	"Invalid teardown channel",
	"Invalid teardown stream",
	"Teardown request channel not playing",
	"Teardown command failed"
};

static int create_init = 0;

////////////////////////////////////////////////////////////////////////////
G_DEFINE_TYPE (GobjMrtpSrc, gobj_mrtp_src, G_TYPE_OBJECT)

GobjMrtpSrc *
gobj_mrtp_src_new (void)
{
	if (this_mrtpsrc == NULL) {
		this_mrtpsrc = g_object_new(GOBJ_TYPE_MRTP_SRC, NULL);
	}

    return this_mrtpsrc;
}

/* object finalizer */
static void
gobj_mrtp_src_finalize (GObject *object)
{	
	G_OBJECT_CLASS (gobj_mrtp_src_parent_class)->finalize (object);
}

/* class initializer */
static void
gobj_mrtp_src_class_init (GobjMrtpSrcClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = gst_mrtp_src_set_property;
	gobject_class->get_property = gst_mrtp_src_get_property;


	g_object_class_install_property (gobject_class, PROP_SILENT,
		g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
		TRUE, (GParamFlags)G_PARAM_READWRITE));

	/* Set param */
	g_object_class_install_property (gobject_class, ENG_CH_NUM,
			g_param_spec_int ("channel", "Channel", "Channel number", 
				0, MRTPSRC_MAX_CH, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_STREAM_NUM,
			g_param_spec_int ("stream", "Stream", "Stream number", 
				0, STREAM_AUDIO, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_PORT,
			g_param_spec_int ("port", "Port", "RTSP Port number", 
				0, MAX_2BYTES, RTSP_PORT, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_IPADDR,
			g_param_spec_uint ("ipaddr", "IPAddr", "NVT IP address", 
				0, MAX_4BYTES, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_FPS,
			g_param_spec_uint ("fps", "FPS", "Frames per second", 
				0, MAX_4BYTES, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_RESOLUTION,
			g_param_spec_uint ("resolution", "Resolution", "Video Resolution", 
				0, MAX_4BYTES, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_MODEL,
			g_param_spec_uint ("model", "ModelCode", "IP Camera model code", 
				0, MAX_4BYTES, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_IONLY_CH,
			g_param_spec_uint ("ionlych", "I-only-channel", "I frame only channel mask",
				0, MAX_4BYTES, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_LOCATION,
			g_param_spec_string ("location", "Location", "Domain name", 
				NULL, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_RTSPADDR,
			g_param_spec_string ("rtspaddr", "RTSPAddress", "RTSP address", 
				NULL, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_USERNAME,
			g_param_spec_string ("username", "RTSP user name", "RTSP user name", 
				NULL, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, ENG_PASSWORD,
			g_param_spec_string ("password", "RTSP password", "RTSP password", 
				NULL, (GParamFlags)G_PARAM_READWRITE));

	/* Command param */
	g_object_class_install_property (gobject_class, CMD_OPEN_PLAY,
			g_param_spec_int ("open-play", "Open and Play", "Open and Play",
				0, 1, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, CMD_TEARDOWN,
			g_param_spec_int ("teardown", "Teardown", "Stop channel",
				0, 1, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, CMD_PAUSE,
			g_param_spec_int ("pause", "Pause", "Stop channel without resource cleaning",
				0, 1, 0, (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, CMD_RESUME,
			g_param_spec_int ("resume", "Resume", "Resume paused channel",
				0, 1, 0, (GParamFlags)G_PARAM_READWRITE));

	gobject_class->finalize = gobj_mrtp_src_finalize;
	g_print("gobj_mrtp_src_class_init()\n");
}

/* object initializer */
static void
gobj_mrtp_src_init (GobjMrtpSrc *self)
{
	printf("MRTP VERSION : [%s]", MRTPSRC_VERSION);
	
	self->ch_num = 0;
	self->stream_num = 0;
	self->port = RTSP_PORT;
	self->ipaddr = 0;
	self->location = NULL;
	self->rtspaddr = NULL;
	self->model = 0;
	self->ionlych = 0;
	self->silent = TRUE;
	self->max_fps = FPS_25;
	self->rtp_method = RTP_OVER_RTSP_TCP;

	self->cmd_callback = NULL;
	self->motion_callback = NULL;
	self->alarm_callback = NULL;
	self->handoff_callback = NULL;
	self->handoff_callback_streamer = NULL;

	self->cmd_user_data = NULL;
	self->motion_user_data = NULL;
	self->handoff_user_data = NULL;

	memset(last_alarm, 0x00, sizeof(GTimeVal)*MRTPSRC_MAX_CH);
	memset(skip_cnt, 0x00, sizeof(skip_cnt));
#if LIVE_STATISTICS
	memset(live_buffering, 0x00, sizeof(guint)*MRTPSRC_MAX_CH);
	memset(live_skip, 0x00, sizeof(guint)*MRTPSRC_MAX_CH);
	memset(&live_stat, 0x00, sizeof(MRTPSRC_LIVE_STAT_T));
#endif
#if STATS_AVERAGE
	memset(avg_fcnt, 0x00, sizeof(int) * AVERAGE_DURATION_SECS);
	avg_index = 0;
#endif

	mrtpsrc_rtsp_client_initialize();

	g_print("gobj_mrtp_src_init()\n");
}
///////////////////////////////////////////////////////////////////////////////
static void gst_mrtp_src_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
#ifndef NMZ_STANDLONE_MODE
	gboolean vali;
	gint rtn = 0;
	GobjMrtpSrc *filter = GOBJ_MRTP_SRC (object);

	switch (prop_id) {
		case PROP_SILENT:
			filter->silent = g_value_get_boolean (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | silent(%d)", filter->silent);
			break;
		case ENG_CH_NUM:
			filter->ch_num = g_value_get_int (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | ch_num(%d)", filter->ch_num);
			break;
		case ENG_STREAM_NUM:
			filter->stream_num = g_value_get_int (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | stream_num(%d)", filter->stream_num);
			break;
		case ENG_PORT:
			filter->port = g_value_get_int (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | port(%d)", filter->port);
			break;
		case ENG_IPADDR:
			filter->ipaddr = g_value_get_uint (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | ipaddr(%08x)", filter->ipaddr);
			break;
		case ENG_FPS:
			filter->fps = g_value_get_uint (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | fps(%u)", filter->fps);
			break;
		case ENG_RESOLUTION:
			filter->resolution = g_value_get_uint (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | resolution(%u)", filter->resolution);
			break;
		case ENG_LOCATION:
			filter->location = g_strdup((gchar*) g_value_get_string (value));
			filter->ipaddr = get_ipaddr_from_domainname(filter->location);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, (void*)filter->location);
			filter->location = NULL;
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | location(%s)", filter->location);
			break;
		case ENG_RTSPADDR:
			filter->rtspaddr = g_strdup((gchar*) g_value_get_string (value));
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | rtsp(%s)", filter->rtspaddr);
			break;
		case ENG_USERNAME:
			filter->username = g_strdup((gchar*) g_value_get_string (value));
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | rtsp(%s)", filter->username);
			break;
		case ENG_PASSWORD:
			filter->password = g_strdup((gchar*) g_value_get_string (value));
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | rtsp(%s)", filter->password);
			break;
		case ENG_MODEL:
			filter->model = g_value_get_uint (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | model(%d)", filter->model);
			break;
		case ENG_IONLY_CH:
			filter->ionlych = g_value_get_uint (value);
			//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | ionlych(%08x)", filter->ionlych);
			if (filter->ionlych != i_only_mask)
			{
				iframe_only_req(filter->ionlych);
			}
			break;
		case CMD_OPEN_PLAY:
			vali = g_value_get_int (value);

			if (!vali)
			{
#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MAJOR, "gst_mrtp_src_set_property | CH(%d,%s) trigger is not TRUE",
						filter->ch_num, MRTPSRC_STREAM_STR[filter->stream_num]);
#endif
				goto init_infos;
			}

			rtn = mrtpsrc_open_stream(filter->ch_num, filter->stream_num,
						filter->ipaddr, filter->port,
						filter->rtspaddr, filter->username, filter->password,
						filter->fps, filter->resolution, filter->model, filter->rtp_method, filter->codec_type);

			if (filter->cmd_callback != NULL)
			{
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | Open command callback run");
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | op(0) ch(%d) stream(%d) en(%u)", filter->ch_num, filter->stream_num, mrtpsrc_get_errno());
				filter->cmd_callback(0, filter->ch_num, filter->stream_num, mrtpsrc_get_errno(),
						filter->cmd_user_data);
			}

			if (rtn == RTN_FAIL)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | Failed to open(%s)", _SYS_ERR_STR[mrtpsrc_get_errno()]);
				goto init_infos;
			}

			if (filter->location != NULL)
			{
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, filter->location);
				filter->location = NULL;
			}
			if (filter->rtspaddr != NULL)
			{
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, filter->rtspaddr);
				filter->rtspaddr = NULL;
			}

			set_max_fps(object);

			break;

			/* informations initialized */
		case CMD_TEARDOWN:
			vali = g_value_get_int (value);
			if (!vali)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | CH(%d,%s) trigger is not TRUE",
						filter->ch_num, MRTPSRC_STREAM_STR[filter->stream_num]);
				goto init_infos;
			}
			rtn = mrtpsrc_close_stream(filter->ch_num, filter->stream_num);

			if (filter->cmd_callback != NULL)
			{
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | Close command callback run");
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | op(1) ch(%d) stream(%d) en(%u)", filter->ch_num, filter->stream_num, mrtpsrc_get_errno());
				filter->cmd_callback(1, filter->ch_num, filter->stream_num, mrtpsrc_get_errno(),
						filter->cmd_user_data);
			}

			if (rtn == RTN_FAIL)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | Failed to close(%s)", _SYS_ERR_STR[mrtpsrc_get_errno()]);
			}
			set_max_fps(object);

init_infos:
			//filter->ch_num = 0;
			//filter->stream_num = 0;
			//filter->ipaddr = 0;
			//filter->port = RTSP_PORT;
			if (filter->location != NULL)
			{
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, filter->location);
				filter->location = NULL;
			}
			if (filter->rtspaddr != NULL)
			{
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, filter->rtspaddr);
				filter->rtspaddr = NULL;
			}
			break;
		case CMD_PAUSE:
			vali = g_value_get_int (value);
			if (!vali)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | CH(%d,%s) trigger is not TRUE",
						filter->ch_num, MRTPSRC_STREAM_STR[filter->stream_num]);
				break;
			}
			rtn = mrtpsrc_pause_stream(filter->ch_num, filter->stream_num);
			if (filter->cmd_callback != NULL)
			{
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | Pause command callback run");
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | op(2) ch(%d) stream(%d) en(%u)", filter->ch_num, filter->stream_num, mrtpsrc_get_errno());
				filter->cmd_callback(2, filter->ch_num, filter->stream_num, mrtpsrc_get_errno(),
						filter->cmd_user_data);
			}
			if (rtn == RTN_FAIL)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | Failed to pause(%s)", _SYS_ERR_STR[mrtpsrc_get_errno()]);
			}
			set_max_fps(object);

			break;
		case CMD_RESUME:
			vali = g_value_get_int (value);
			if (!vali)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | CH(%d,%s) trigger is not TRUE",
						filter->ch_num, MRTPSRC_STREAM_STR[filter->stream_num]);
				break;
			}
			rtn = mrtpsrc_resume_stream(filter->ch_num, filter->stream_num);
			if (filter->cmd_callback != NULL)
			{
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | Resume command callback run");
				//MRTPSRC_DBG(MINOR, "gst_mrtp_src_set_property | op(3) ch(%d) stream(%d) en(%u)", filter->ch_num, filter->stream_num, mrtpsrc_get_errno());
				filter->cmd_callback(3, filter->ch_num, filter->stream_num, mrtpsrc_get_errno(),
						filter->cmd_user_data);
			}
			if (rtn == RTN_FAIL)
			{
				MRTPSRC_DBG(WARN, "gst_mrtp_src_set_property | Failed to resume(%s)", _SYS_ERR_STR[mrtpsrc_get_errno()]);
			}
			set_max_fps(object);

			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
#endif
}

static void gst_mrtp_src_get_property (GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec)
{
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *filter = GOBJ_MRTP_SRC (object);

	switch (prop_id) {
		case PROP_SILENT:
			g_value_set_boolean (value, filter->silent);
			break;
		case ENG_CH_NUM:
		case ENG_PORT:
		case ENG_IPADDR:
		case ENG_FPS:
		case ENG_RESOLUTION:
		case ENG_RTSPADDR:
		case ENG_USERNAME:
		case ENG_PASSWORD:
		case ENG_MODEL:
		case ENG_LOCATION:
		case CMD_OPEN_PLAY:
		case CMD_TEARDOWN:
			break;
		case ENG_IONLY_CH:
			g_value_set_uint (value, i_only_mask);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
#endif
}

#ifndef NMZ_STANDLONE_MODE
static gboolean gst_mrtp_src_start (void *basesrc)
{
	//GobjMrtpSrc *src = GOBJ_MRTP_SRC (basesrc);
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "gst_mrtp_src_start | start");
#endif
	return TRUE;
}

static gboolean gst_mrtp_src_stop (void *basesrc)
{
	//GobjMrtpSrc *src = GOBJ_MRTP_SRC (basesrc);
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "gst_mrtp_src_stop | stop");
#endif
	return TRUE;
}
#endif

static void set_max_fps(GObject *object)
{
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *src;
#endif
	gint i = 0;
	gint j = 0;
	gint max = FPS_25;
	MRTPSRC_STREAM_T* ch = NULL;


#ifndef NMZ_STANDLONE_MODE
	src = GOBJ_MRTP_SRC (object);
#endif

	for (i = 0; i < MRTPSRC_MAX_CH; i++)
	{
		for (j = STREAM_1ST; j < STREAM_AUDIO; j++)
		{
			ch = mrtpsrc_get_stream(i, j);

			if (ch->state == STATE_PLAYING || ch->state == STATE_STREAM_READY)
			{
				if (max > ch->fps)
					max = ch->fps;
			}
		}
	}
#ifndef NMZ_STANDLONE_MODE
	src->max_fps = max;
#endif
}

static void iframe_only_req(guint ch_mask)
{
	int i = 0;
	MRTPSRC_STREAM_T *lst = NULL;

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | channel mask (%d)", __FUNCTION__, ch_mask);
#endif

	for (i = 0; i < 64; i++)
	{
		lst = mrtpsrc_get_stream(i%32, i/32);
		if (ch_mask & (1<<i))
		{
			if (lst->state >= STATE_STREAM_READY && lst->state <= STATE_DROP_P_ALL)
			{
#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MINOR, "%s | ch(%d) stream(%d) requests i-frame only",
						__FUNCTION__, i%32, i/32);
#endif
				lst->state = STATE_I_ONLY;
			}
			else
			{
				ch_mask &= ~(1<<i);
			}
		}
		else
		{
			if (lst->state == STATE_I_ONLY)
			{
#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MINOR, "%s | ch(%d) stream(%d) requests all frames",
						__FUNCTION__, i%32, i/32);
#endif
				lst->state = STATE_P_REQUESTED;
			}
		}
	}

	i_only_mask = ch_mask;
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | i_only_mask(%08x)", __FUNCTION__, i_only_mask);
#endif
}

static void display_sched_attr(int policy, struct sched_param *param)
{
	printf("[mrtpsrc create]    policy=%s, priority=%d\n",
			(policy == SCHED_FIFO)  ? "SCHED_FIFO" :
			(policy == SCHED_RR)    ? "SCHED_RR" :
			(policy == SCHED_OTHER) ? "SCHED_OTHER" :
			"???",
			param->sched_priority);
}

int gst_mrtp_src_create (GobjMrtpSrc *src, GobjListBuffer **buffer)
{
	GobjListBuffer *list_buf = NULL;
	gint i, j;
#if MRTPSRC_PRINT_MRAW_ABS
	gint k;
#endif
	gint fcnt = 0;
	guint now_alarm_flag = 0;
	//int wait_count = 0;
	MRTPSRC_FRAME_T probe = NULL;
	MRTPSRC_STREAM_T* lst = NULL;
	MRTPSRC_CHANNEL_T* lch = NULL;

#if 1
	{
		int policy, s;
		struct sched_param param;
		pthread_t thread;

		thread = pthread_self();

		policy = SCHED_FIFO;
		//policy = SCHED_RR;
		param.sched_priority = sched_get_priority_max(policy);
		pthread_setschedparam (thread, policy, &param);

		if(create_init == 0)
		{
			s = pthread_getschedparam(thread, &policy, &param);
			if (s == 0)
			{
				display_sched_attr(policy, &param);
			}
			create_init = 1;
		}
	}
#endif


	list_buf = gobj_list_buffer_new();
	now_alarm_flag = last_alarm_flag;
	while (MRTPSRC_ALWAYS)
	{
		unsigned int now_sec;
		unsigned int now_5msec;

		g_get_current_time(&now_time);
		now_sec = now_time.tv_sec;
		now_5msec = now_time.tv_usec / 1000 / 5;

#if LIVE_STATISTICS
		if (live_stat.clear_interval == 0)
		{
			live_stat.clear_interval = 29;
		}
		if (live_stat.base_time.tv_sec + live_stat.clear_interval < now_time.tv_sec)
		{
			MRTPSRC_LIVE_DBG();

			for (i=0; i<32; i++)
			{
				lch = mrtpsrc_get_channel(i);
#if 0
				if (live_stat.frame_count[i][0] != 0 &&
					live_stat.frame_count[i][0] < 800 &&
					live_stat.frame_count[i][0] > 700 &&
					lch->model_code <= MODEL_AMB_D1)
				{
					if (live_buffering[i] >= 0 && live_buffering[i] < 30)
					{
						live_buffering[i] = 30;
					}
				}
				else if (lch->model_code == MODEL_AMB_A2 || lch->model_code == MODEL_AMB_D1)
				{
					live_buffering[i] = 40;
				}
				else
				{
					live_buffering[i] = 0;
				}
#else
				if (lch->model_code == MODEL_AMB_A2 || lch->model_code == MODEL_AMB_D1)
				{
					//live_buffering[i] = 40;
				}
				else
				{
					//live_buffering[i] = 40;			//20172010
					//live_buffering[i] = 400;
				}
#endif
			}

			memset(&live_stat, 0x00, sizeof(MRTPSRC_LIVE_STAT_T));
			live_stat.base_time = now_time;
		}
#endif

		MRTPSRC_STREAM_T *jst = NULL;

		for (i = MRTPSRC_MAX_CH - 1; i >= 0; i--)
		{
			int diff = 0;
			lch = mrtpsrc_get_channel(i);

			if (lch->state < STATE_PLAYING || lch->state > STATE_SHADOW_STREAM)
			{
				continue;
			}
	
			/* Video frames */
			for (j=STREAM_AUDIO-1; j>=STREAM_1ST; j--)
			{
				lst = mrtpsrc_get_stream(i, j);
				diff = now_sec - lst->ts_last_sendsec;

				if (lst->state < STATE_PLAYING || lst->state > STATE_SHADOW_STREAM)
				{
					continue;
				}

				if (lst->ts_last_sendsec != 0 && diff > MRTPSRC_TOLERANT_TS_SECS && lst->state != STATE_SHADOW_STREAM)
				{
					printf("%s | Frame Audit(CH:%d,%s) now(%u) last(%u)\n",
							__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
					//lch->state = STATE_REBOOT_REQ;
					if(lch->model_code == MODEL_ONTHEFLY_CHEAT)
					{
						lch->state = STATE_RECONN_REQ;
					}
					else
					{
						lch->state = STATE_REBOOT_REQ;
					}
					//end_frame_emit[i][0] = 0;
					//end_frame_emit[i][1] = 0;
					break;
				}
#if 0	// Frame Audit enable
				/* Frame audit */
				if (lst->ts_last_second != 0)
				{
					static char end_frame_emit[MRTPSRC_MAX_CH][STREAM_MAX] = { { 0, }, };
					int diff = now_sec - lst->ts_last_second;
					//if (diff > MRTPSRC_TOLERANT_TS_SECS)
					if (diff > MRTPSRC_TOLERANT_TS_SECS /*&& lst->rtp_protocol == RTP_OVER_RTSP_TCP*/)
					{
						MRTPSRC_DBG(ERROR, "%s | Frame Audit(CH:%d,%s) now(%u) last(%u)",
								__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
						//lch->state = STATE_REBOOT_REQ;
						lch->state = STATE_REBOOT_REQ;
						end_frame_emit[i][0] = 0;
						end_frame_emit[i][1] = 0;
						break;
					}
					if (diff > 5 && end_frame_emit[i][j] == 0)
					{
						MRTPSRC_DBG(WARN, "%s | Suspected Frame Audit(CH:%d,%s) now(%u) last(%u)",
								__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
						mrtpsrc_ho_control_frame(lst, 1);
						end_frame_emit[i][j] = 1;
						break;
					}
					if (diff < 5 && end_frame_emit[i][j] == 1)
					{
						MRTPSRC_DBG(WARN, "%s | Recovered Frame Audit(CH:%d,%s) now(%u) last(%u)",
								__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
						mrtpsrc_ho_control_frame(lst, 0);
						end_frame_emit[i][j] = 0;
					}
				}
#endif

				if(lch->model_code == MODEL_ONTHEFLY_CHEAT)
				{
					lst->ts_last_sendsec = now_sec;
					continue;
				}

				if (lst->frame_head == NULL)
				{
					continue;
				}

				mrtpsrc_frame_q_lock();
				if (lst->state >= STATE_PLAYING && lst->state <= STATE_SHADOW_STREAM)
				{
					probe = lst->frame_head;
#if BUFFERING_5MSEC
					//if (lst->model == MODEL_AMB_A2 || lst->model == MODEL_AMB_D1)
					{
						int time_diff;

						time_diff = (now_sec - probe->icodec_h.timestamp)*200 +
									(now_5msec - probe->icodec_h.timestampl);

						if (time_diff < BUFFERING_5MSEC)
						{
							mrtpsrc_frame_q_unlock();
							continue;
						}
					}
#elif BUFFERING_DYNAMIC
					{
						int time_diff;

						time_diff = (now_sec - probe->icodec_h.timestamp)*200 +
									(now_5msec - probe->icodec_h.timestampl);

						if (time_diff < live_buffering[i])
						{
							mrtpsrc_frame_q_unlock();
							continue;
						}
					}
#elif BUFFERING_RELATIVE
#endif
#if !LIVE_STATISTICS
					{
						int time_diff;

						time_diff = (now_sec - live_stat.frame_last_sec[i][j])*1000 +
									(now_time.tv_nsec/1000/1000 - live_stat.frame_last_msec[i][j]);
						if (time_diff < 3)
						{
							mrtpsrc_frame_q_unlock();
							continue;
						}
					}
#endif
					lst->frame_head = lst->frame_head->next;
					mrtpsrc_frame_q_unlock();

					lst->ts_last_sendsec = now_sec;
					if (j == STREAM_1ST)
					{
						if (probe->alarm_flag != 0)
						{
							last_alarm[i] = now_time;
							now_alarm_flag |= (1<<i);
						}
						else
						{
							if (now_time.tv_sec - last_alarm[i].tv_sec > 1)
							{
								now_alarm_flag &= ~(1<<i);
							}
						}
					}
#if 0
					if (probe->alarm_flag != 0 && src->alarm_callback != NULL)
					{
						src->alarm_callback(lst->ch_num, src->alarm_user_data);
					}
#endif
					if (probe->motion_raw.width != 0 && probe->motion_raw.height != 0 && 
								src->motion_callback != NULL)
					{
						if (lst->stream_num == STREAM_1ST)
						{
#if MRTPSRC_PRINT_MRAW_ABS
							printf("===================================\n");
							for (k = 0; k < (probe->motion_raw.width * probe->motion_raw.height); k++)
							{
								if (k!=0 && (k%probe->motion_raw.width == 0)) {printf("\n");}
								printf("%02d ", probe->motion_raw.mraw[k]);
							}
							printf("\n-----------------------------------\n");
#endif
							probe->motion_raw.timestamp = probe->icodec_h.timestamp;
							probe->motion_raw.timestampl = probe->icodec_h.timestampl;
							src->motion_callback((GobjMrtpSrcInfoMotion*)&probe->motion_raw, src->motion_user_data);
						}
					}
					else if (probe->motion_raw.width == 0 && probe->motion_raw.height == 0)
					{
						if (lch->model_code == MODEL_AMB_A2 || lch->model_code == MODEL_AMB_D1 || lch->model_code == MODEL_TI_365)
						{
						}
						else if (lst->stream_num == STREAM_1ST)
						{
							probe->motion_raw.width = 0;
							probe->motion_raw.height = probe->motion_flag;
							probe->motion_raw.timestamp = probe->icodec_h.timestamp;
							probe->motion_raw.timestampl = probe->icodec_h.timestampl;
							src->motion_callback((GobjMrtpSrcInfoMotion*)&probe->motion_raw, src->motion_user_data);
						}
					}

#if LIVE_STATISTICS
					MRTPSRC_LIVE_STAT(i, j, probe);
#endif
#if MRTPSRC_LIVE_PAD_PUSH
					{

						ICODEC_HEADER *icodec = NULL;
						icodec =(ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(probe->frame);
						if(!(icodec->chan < 32 || icodec->chan >= 0))
						{
							printf("[%s:%d] !(icodec->chan < 32 || icodec->chan >= 0) - chan(%d) \n", __FUNCTION__, __LINE__, icodec->chan);
						}

						gobj_list_buffer_push(list_buf, probe->frame);
						*buffer = list_buf;
						memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
						probe = NULL;
						frame_cnt++;
						fcnt++;
					}
#else
					{
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
						memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
						probe = NULL;
					}
#endif
				}
				else
				{
					mrtpsrc_frame_q_unlock();
				}

			}
			/* Video frames end */










#if 1
			/* Audio frames */
			lst = mrtpsrc_get_stream(i, STREAM_AUDIO);

			if (lst->state != STATE_PLAYING)
			{
				continue;
			}

			mrtpsrc_frame_q_lock();
			if (lst->frame_head == NULL)
			{
				mrtpsrc_frame_q_unlock();
				continue;
			}

			probe = lst->frame_head;
			lst->frame_head = lst->frame_head->next;
			mrtpsrc_frame_q_unlock();
#if 0
if (i == 0)
{
	guint a, b;

	a = ((ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(probe->frame))->timestamp;
	b = ((ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(probe->frame))->timestampl;
	printf("AUDIO [%u %u]\n", a, b);
}
#endif
			if (probe != NULL)
			{
#if MRTPSRC_AUDIO_PAD_PUSH
				if (src != NULL && src->handoff_callback_streamer != NULL)
				{
					void *tmp_gst_ret = NULL;
					tmp_gst_ret = g_object_ref(probe->frame);
					if(tmp_gst_ret == NULL) {
						printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
						gst_buffer_debug(probe->frame);
					}
					else
					{
						src->handoff_callback_streamer(probe->frame);
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					}
				}

				if (live_audio_ch == i && GOBJ_IS_BUDDY_BUFFER(probe->frame))
				{
					gobj_list_buffer_push(list_buf, probe->frame);
					*buffer = list_buf;
					memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
					probe = NULL;
					frame_cnt++;
					fcnt++;
				}
				else
				{
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
					probe = NULL;
				}

#else
				{
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
					probe = NULL;
				}
#endif
			}
			/* Audio frames end */
#endif





		}








		if (last_alarm_flag != now_alarm_flag)
		{
			last_alarm_flag = now_alarm_flag;
			if (src->alarm_callback != NULL)
			{
				src->alarm_callback(now_alarm_flag, src->alarm_user_data);
			}
		}




		if (now_time.tv_sec != prev_time.tv_sec)
		{
			if (src != NULL && src->cmd_callback != NULL)
			{
				gint stch = 0;
				for (stch =0; stch<MRTPSRC_MAX_CH; stch++)
				{
					src->cmd_callback(30, stch, 0, live_stat.frame_last_sec[stch][0], src->cmd_user_data);
				}
			}
			prev_time = now_time;
		}



		if (fcnt != 0) { break; }
		g_usleep(3*1000);
	}

	/* count remained frames */
	//if (MRTPSRC_ALWAYS)
	//if (PRINT_REMAIN)
	if (MRTPSRC_NEVER)
	{
		memset(remain_frame, 0x00, sizeof(remain_frame));
		mrtpsrc_frame_q_lock();
		for (i = 0; i < MRTPSRC_MAX_CH; i++)
		{
			for (j = 0; j < STREAM_MAX; j++)
			{
				lst = mrtpsrc_get_stream(i,j);

				probe = lst->frame_head;
				while (probe != NULL)
				{
					remain_frame[i][j]++;
					probe = probe->next;
				}
			}
		}

		lst = NULL;
		probe = NULL;
		mrtpsrc_frame_q_unlock();
	}
	/* count remained frames end */

	//if (MRTPSRC_ALWAYS)
	if (MRTPSRC_NEVER)
	{
		//g_get_current_time(&now_time); 

		if (now_time.tv_sec != prev_time.tv_sec)
		{
			//if (call_cnt != 30)
			{
#if 0
#if STATS_AVERAGE
				int total = 0;

				avg_fcnt[avg_index] = frame_cnt;
				avg_index = (avg_index+1) % AVERAGE_DURATION_SECS;
				for (i = 0; i < AVERAGE_DURATION_SECS; i++)
				{
					total += avg_fcnt[i];
				}

				MRTPSRC_DBG(MAJOR, "[%u] CALL CNT(%u) FRAME(%u) AVG_%d_SECS(%d,%d)", now_time.tv_sec, call_cnt, frame_cnt, AVERAGE_DURATION_SECS, total/AVERAGE_DURATION_SECS, total%AVERAGE_DURATION_SECS);
#else
				MRTPSRC_DBG(MAJOR, "[%u] CALL CNT(%u) PAD_PUSH(%u) FRAME(%u)", now_time.tv_sec, call_cnt, push_cnt, frame_cnt);
#endif
				printf("\n");
#endif
				//printf("\n\nPAD PUSH COUNT %d\n\n", push_cnt);
#if PRINT_REMAIN
				for (i = 0; i < MRTPSRC_MAX_CH; i++)
				{
					printf("     CH %02d     ", i);
					for (j = 0; j < STREAM_MAX; j++)
					{
						lst = mrtpsrc_get_stream(i,j);
						if (lst->state < STATE_STREAM_READY || lst->state > STATE_PAUSED)
							continue;
						printf("%s %u\t", MRTPSRC_STREAM_STR[j], remain_frame[i][j]);
					}
					printf("\n");
				}

				printf("\n");
#endif
			}
			call_cnt = 0;
			frame_cnt = 0;
			push_cnt = 0;
		}
		call_cnt++;

		prev_time = now_time;
	}

	if (MRTPSRC_NEVER)
	{
		if (now_time.tv_sec != prev_time.tv_sec)
		{
			printf("Buffering [%u,%u,%u,%u,%u,%u,%u,%u  %u,%u,%u,%u,%u,%u,%u,%u]\n",
					live_buffering[ 0],  live_buffering[ 1],
					live_buffering[ 2],  live_buffering[ 3],
					live_buffering[ 4],  live_buffering[ 5],
					live_buffering[ 6],  live_buffering[ 7],
					live_buffering[ 8],  live_buffering[ 9],
					live_buffering[10],  live_buffering[11],
					live_buffering[12],  live_buffering[13],
					live_buffering[14],  live_buffering[15]);

			if (src != NULL && src->cmd_callback != NULL)
			{
				gint stch = 0;
				for (stch =0; stch<MRTPSRC_MAX_CH; stch++)
				{
					src->cmd_callback(30, stch, 0, live_stat.frame_last_sec[stch][0], src->cmd_user_data);
				}
			}
		}
		prev_time = now_time;
	}

#if DROP_P_FRAME_ENABLE
	for(i = 0; i < MRTPSRC_MAX_CH; i++)
	{
		for (j = 0; j <= STREAM_2ND; j++)
		{
			lst = mrtpsrc_get_stream(i, j);

			if (lst->state >= STATE_DROP_P_1 && lst->state <= STATE_DROP_P_ALL)
				continue;

			if (remain_frame[i][j] > 30)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(WARN, "[create] %u queue entry, flush all gop p-frames(%d, %s)",
						remain_frame[i][j], i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_ALL;
			}
			else if (remain_frame[i][j] > 15)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(WARN, "[create] 15 queue entries exceeded, drops 5 pframes(%d, %s)",
						i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_5;
			}
			else if (remain_frame[i][j] > 10)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(IMPACT, "[create] 10 queue entries exceeded, drops 2 pframes(%d, %s)",
						i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_2;
			}
			else if (remain_frame[i][j] > 5)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(IMPACT, "[create] 5 queue entries exceeded, drops 1 pframe(%d, %s)",
						i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_1;
			}
		}
	}
#endif

	push_cnt++;
	return 0;
}

static guint get_ipaddr_from_domainname(gchar* domain_name)
{
	struct addrinfo hints, *res, *p;
	gint status;
	gchar ipstr[INET6_ADDRSTRLEN];

	if (domain_name == NULL)
	{
		MRTPSRC_DBG(WARN, "get_ipaddr_from_domainname | domain name is NULL");
		return RTN_FAIL;
	}

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(domain_name, NULL, &hints, &res)) != 0)
	{
		MRTPSRC_DBG(WARN, "get_ipaddr_from_domainname | getaddrinfo fail");
		return RTN_FAIL;
	}

	for(p = res;p != NULL; p = p->ai_next)
	{
		void *addr;
		char *ipver;

		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		} else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("  %s: %s\n", ipver, ipstr);
	}

	freeaddrinfo(res); // free the linked list

	return inet_addr(ipstr);

}

static void MRTPSRC_LIVE_STAT(int ch_num, int stream_num, MRTPSRC_FRAME_T probe)
{
	int i = ch_num;
	int j = stream_num;

	live_stat.frame_count[i][j]++;
	live_stat.frame_bytes[i][j] += probe->len;
	if (probe->frame_type == FTYPE_IDR)
	{
		live_stat.i_cnt[i][j]++;
		live_stat.i_bytes[i][j] += probe->len;
		if (probe->len > live_stat.i_max_len[i][j])
		{
			live_stat.i_max_len[i][j] = probe->len;
		}
	}
	else if (probe->frame_type == FTYPE_N_IDR)
	{
		live_stat.p_cnt[i][j]++;
		live_stat.p_bytes[i][j] += probe->len;
		if (probe->len > live_stat.p_max_len[i][j])
		{
			live_stat.p_max_len[i][j] = probe->len;
		}
	}
	live_stat.frame_len_average[i][j] =
			live_stat.frame_bytes[i][j]/live_stat.frame_count[i][j];
	if (live_stat.i_cnt[i][j] != 0)
	{
		live_stat.i_avg[i][j] = live_stat.i_bytes[i][j]/live_stat.i_cnt[i][j];
	}
	if (live_stat.p_cnt[i][j] != 0)
	{
		live_stat.p_avg[i][j] = live_stat.p_bytes[i][j]/live_stat.p_cnt[i][j];
	}

	if (live_stat.frame_last_sec[i][j] != 0)
	{
		gint interval_sec;
		gint interval_msec;
		gint interval;

		interval_sec = now_time.tv_sec - live_stat.frame_last_sec[i][j];
		interval_msec = (now_time.tv_usec/1000) - live_stat.frame_last_msec[i][j];
		interval = (interval_sec*1000) + interval_msec;

		if (interval == 0)
		{
			interval = 1;
		}
		if (live_stat.frame_interval_min[i][j] == 0 ||
			interval < live_stat.frame_interval_min[i][j])
		{
			live_stat.frame_interval_min[i][j] = interval;
		}
		if (interval > live_stat.frame_interval_max[i][j])
		{
			live_stat.frame_interval_max[i][j] = interval;
		}
		if (interval < 10)
		{
			live_stat.interval_10_cnt[i][j]++;
			live_stat.interval_20_cnt[i][j]++;
		}
		else if (interval < 20)
		{
			live_stat.interval_20_cnt[i][j]++;
		}
		if (interval > 130)
		{
			live_stat.interval_130_cnt[i][j]++;
			live_stat.interval_100_cnt[i][j]++;
			live_stat.interval_70_cnt[i][j]++;
			live_stat.interval_40_cnt[i][j]++;
		}
		else if (interval > 100)
		{
			live_stat.interval_100_cnt[i][j]++;
			live_stat.interval_70_cnt[i][j]++;
			live_stat.interval_40_cnt[i][j]++;
		}
		else if (interval > 70)
		{
			live_stat.interval_70_cnt[i][j]++;
			live_stat.interval_40_cnt[i][j]++;
		}
		else if (interval > 40)
		{
			live_stat.interval_40_cnt[i][j]++;
		}
	}

	live_stat.frame_last_sec[i][j] = now_time.tv_sec;
	live_stat.frame_last_msec[i][j] = now_time.tv_usec/1000;
}

static void MRTPSRC_LIVE_DBG(void)
{
	char *tbuf;
	char *sbuf;
	char *ibuf;
	char *buf;


	tbuf = (char*) mrtpsrc_alloc_heap(512, __FILE__, __FUNCTION__, __LINE__);
	if (tbuf == NULL)
	{
		MRTPSRC_DBG(ERROR, "MRTPSRC_LIVE_DBG | tbuf alloc fail");
		return;
	}

	sbuf = (char*) mrtpsrc_alloc_heap(4096, __FILE__, __FUNCTION__, __LINE__);
	if (sbuf == NULL)
	{
		MRTPSRC_DBG(ERROR, "MRTPSRC_LIVE_DBG | sbuf alloc fail");
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, tbuf);
		return;
	}

	ibuf = (char*) mrtpsrc_alloc_heap(4096, __FILE__, __FUNCTION__, __LINE__);
	if (ibuf == NULL)
	{
		MRTPSRC_DBG(ERROR, "MRTPSRC_LIVE_DBG | ibuf alloc fail");
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, tbuf);
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sbuf);
		return;
	}

	buf  = (char*) mrtpsrc_alloc_heap(8704, __FILE__, __FUNCTION__, __LINE__);
	if (buf == NULL)
	{
		MRTPSRC_DBG(ERROR, "MRTPSRC_LIVE_DBG | buf alloc fail");
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, tbuf);
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sbuf);
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, ibuf);
		return;
	}
	
	g_snprintf(tbuf, 512, "====== MRTPSRC LIVE INFO [%ld.%06lu ~ %ld.%06lu] ======\n",
			live_stat.base_time.tv_sec, live_stat.base_time.tv_usec,
			now_time.tv_sec, now_time.tv_usec);

	g_snprintf(sbuf, 4096,
	"  Frame count\n"
	"    total         [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"    total         [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"    total         [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_AUDIO
	"    total         [3]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	"  Frame kbytes\n"
	"    total         [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"    total         [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"    total         [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_AUDIO
	"    total         [3]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	"  Frame average bytes\n"
	"    total         [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"    total         [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"    total         [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    i             [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    p             [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_AUDIO
	"    total         [3]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	"  I-Frame max bytes\n"
	"                  [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"                  [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"                  [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	"  P-Frame max bytes\n"
	"                  [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"                  [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"                  [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	,
	live_stat.frame_count[0][0],  live_stat.frame_count[1][0],
	live_stat.frame_count[2][0],  live_stat.frame_count[3][0],
	live_stat.frame_count[4][0],  live_stat.frame_count[5][0],
	live_stat.frame_count[6][0],  live_stat.frame_count[7][0],
#if LIVE_PRINT_16CH
	live_stat.frame_count[8][0],  live_stat.frame_count[9][0],
	live_stat.frame_count[10][0], live_stat.frame_count[11][0],
	live_stat.frame_count[12][0], live_stat.frame_count[13][0],
	live_stat.frame_count[14][0], live_stat.frame_count[15][0],
#endif
	live_stat.i_cnt[0][0],  live_stat.i_cnt[1][0],
	live_stat.i_cnt[2][0],  live_stat.i_cnt[3][0],
	live_stat.i_cnt[4][0],  live_stat.i_cnt[5][0],
	live_stat.i_cnt[6][0],  live_stat.i_cnt[7][0],
#if LIVE_PRINT_16CH
	live_stat.i_cnt[8][0],  live_stat.i_cnt[9][0],
	live_stat.i_cnt[10][0], live_stat.i_cnt[11][0],
	live_stat.i_cnt[12][0], live_stat.i_cnt[13][0],
	live_stat.i_cnt[14][0], live_stat.i_cnt[15][0],
#endif
	live_stat.p_cnt[0][0],  live_stat.p_cnt[1][0],
	live_stat.p_cnt[2][0],  live_stat.p_cnt[3][0],
	live_stat.p_cnt[4][0],  live_stat.p_cnt[5][0],
	live_stat.p_cnt[6][0],  live_stat.p_cnt[7][0],
#if LIVE_PRINT_16CH
	live_stat.p_cnt[8][0],  live_stat.p_cnt[9][0],
	live_stat.p_cnt[10][0], live_stat.p_cnt[11][0],
	live_stat.p_cnt[12][0], live_stat.p_cnt[13][0],
	live_stat.p_cnt[14][0], live_stat.p_cnt[15][0],
#endif
#if LIVE_PRINT_2ND
	live_stat.frame_count[0][1],  live_stat.frame_count[1][1],
	live_stat.frame_count[2][1],  live_stat.frame_count[3][1],
	live_stat.frame_count[4][1],  live_stat.frame_count[5][1],
	live_stat.frame_count[6][1],  live_stat.frame_count[7][1],
#if LIVE_PRINT_16CH
	live_stat.frame_count[8][1],  live_stat.frame_count[9][1],
	live_stat.frame_count[10][1], live_stat.frame_count[11][1],
	live_stat.frame_count[12][1], live_stat.frame_count[13][1],
	live_stat.frame_count[14][1], live_stat.frame_count[15][1],
#endif
	live_stat.i_cnt[0][1],  live_stat.i_cnt[1][1],
	live_stat.i_cnt[2][1],  live_stat.i_cnt[3][1],
	live_stat.i_cnt[4][1],  live_stat.i_cnt[5][1],
	live_stat.i_cnt[6][1],  live_stat.i_cnt[7][1],
#if LIVE_PRINT_16CH
	live_stat.i_cnt[8][1],  live_stat.i_cnt[9][1],
	live_stat.i_cnt[10][1], live_stat.i_cnt[11][1],
	live_stat.i_cnt[12][1], live_stat.i_cnt[13][1],
	live_stat.i_cnt[14][1], live_stat.i_cnt[15][1],
#endif
	live_stat.p_cnt[0][1],  live_stat.p_cnt[1][1],
	live_stat.p_cnt[2][1],  live_stat.p_cnt[3][1],
	live_stat.p_cnt[4][1],  live_stat.p_cnt[5][1],
	live_stat.p_cnt[6][1],  live_stat.p_cnt[7][1],
#if LIVE_PRINT_16CH
	live_stat.p_cnt[8][1],  live_stat.p_cnt[9][1],
	live_stat.p_cnt[10][1], live_stat.p_cnt[11][1],
	live_stat.p_cnt[12][1], live_stat.p_cnt[13][1],
	live_stat.p_cnt[14][1], live_stat.p_cnt[15][1],
#endif
#endif
#if LIVE_PRINT_3RD
	live_stat.frame_count[0][2],  live_stat.frame_count[1][2],
	live_stat.frame_count[2][2],  live_stat.frame_count[3][2],
	live_stat.frame_count[4][2],  live_stat.frame_count[5][2],
	live_stat.frame_count[6][2],  live_stat.frame_count[7][2],
#if LIVE_PRINT_16CH
	live_stat.frame_count[8][2],  live_stat.frame_count[9][2],
	live_stat.frame_count[10][2], live_stat.frame_count[11][2],
	live_stat.frame_count[12][2], live_stat.frame_count[13][2],
	live_stat.frame_count[14][2], live_stat.frame_count[15][2],
#endif
	live_stat.i_cnt[0][2],  live_stat.i_cnt[1][2],
	live_stat.i_cnt[2][2],  live_stat.i_cnt[3][2],
	live_stat.i_cnt[4][2],  live_stat.i_cnt[5][2],
	live_stat.i_cnt[6][2],  live_stat.i_cnt[7][2],
#if LIVE_PRINT_16CH
	live_stat.i_cnt[8][2],  live_stat.i_cnt[9][2],
	live_stat.i_cnt[10][2], live_stat.i_cnt[11][2],
	live_stat.i_cnt[12][2], live_stat.i_cnt[13][2],
	live_stat.i_cnt[14][2], live_stat.i_cnt[15][2],
#endif
	live_stat.p_cnt[0][2],  live_stat.p_cnt[1][2],
	live_stat.p_cnt[2][2],  live_stat.p_cnt[3][2],
	live_stat.p_cnt[4][2],  live_stat.p_cnt[5][2],
	live_stat.p_cnt[6][2],  live_stat.p_cnt[7][2],
#if LIVE_PRINT_16CH
	live_stat.p_cnt[8][2],  live_stat.p_cnt[9][2],
	live_stat.p_cnt[10][2], live_stat.p_cnt[11][2],
	live_stat.p_cnt[12][2], live_stat.p_cnt[13][2],
	live_stat.p_cnt[14][2], live_stat.p_cnt[15][2],
#endif
#endif
#if LIVE_PRINT_AUDIO
	live_stat.frame_count[0][3],  live_stat.frame_count[1][3],
	live_stat.frame_count[2][3],  live_stat.frame_count[3][3],
	live_stat.frame_count[4][3],  live_stat.frame_count[5][3],
	live_stat.frame_count[6][3],  live_stat.frame_count[7][3],
#if LIVE_PRINT_16CH
	live_stat.frame_count[8][3],  live_stat.frame_count[9][3],
	live_stat.frame_count[10][3], live_stat.frame_count[11][3],
	live_stat.frame_count[12][3], live_stat.frame_count[13][3],
	live_stat.frame_count[14][3], live_stat.frame_count[15][3],
#endif
#endif
	live_stat.frame_bytes[0][0] /1024,live_stat.frame_bytes[1][0] /1024,
	live_stat.frame_bytes[2][0] /1024,live_stat.frame_bytes[3][0] /1024,
	live_stat.frame_bytes[4][0] /1024,live_stat.frame_bytes[5][0] /1024,
	live_stat.frame_bytes[6][0] /1024,live_stat.frame_bytes[7][0] /1024,
#if LIVE_PRINT_16CH
	live_stat.frame_bytes[8][0] /1024,live_stat.frame_bytes[9][0] /1024,
	live_stat.frame_bytes[10][0]/1024,live_stat.frame_bytes[11][0]/1024,
	live_stat.frame_bytes[12][0]/1024,live_stat.frame_bytes[13][0]/1024,
	live_stat.frame_bytes[14][0]/1024,live_stat.frame_bytes[15][0]/1024,
#endif
	live_stat.i_bytes[0][0] /1024,live_stat.i_bytes[1][0] /1024,
	live_stat.i_bytes[2][0] /1024,live_stat.i_bytes[3][0] /1024,
	live_stat.i_bytes[4][0] /1024,live_stat.i_bytes[5][0] /1024,
	live_stat.i_bytes[6][0] /1024,live_stat.i_bytes[7][0] /1024,
#if LIVE_PRINT_16CH
	live_stat.i_bytes[8][0] /1024,live_stat.i_bytes[9][0] /1024,
	live_stat.i_bytes[10][0]/1024,live_stat.i_bytes[11][0]/1024,
	live_stat.i_bytes[12][0]/1024,live_stat.i_bytes[13][0]/1024,
	live_stat.i_bytes[14][0]/1024,live_stat.i_bytes[15][0]/1024,
#endif
	live_stat.p_bytes[0][0] /1024,live_stat.p_bytes[1][0] /1024,
	live_stat.p_bytes[2][0] /1024,live_stat.p_bytes[3][0] /1024,
	live_stat.p_bytes[4][0] /1024,live_stat.p_bytes[5][0] /1024,
	live_stat.p_bytes[6][0] /1024,live_stat.p_bytes[7][0] /1024,
#if LIVE_PRINT_16CH
	live_stat.p_bytes[8][0] /1024,live_stat.p_bytes[9][0] /1024,
	live_stat.p_bytes[10][0]/1024,live_stat.p_bytes[11][0]/1024,
	live_stat.p_bytes[12][0]/1024,live_stat.p_bytes[13][0]/1024,
	live_stat.p_bytes[14][0]/1024,live_stat.p_bytes[15][0]/1024,
#endif
#if LIVE_PRINT_2ND
	live_stat.frame_bytes[0][1] /1024,live_stat.frame_bytes[1][1] /1024,
	live_stat.frame_bytes[2][1] /1024,live_stat.frame_bytes[3][1] /1024,
	live_stat.frame_bytes[4][1] /1024,live_stat.frame_bytes[5][1] /1024,
	live_stat.frame_bytes[6][1] /1024,live_stat.frame_bytes[7][1] /1024,
#if LIVE_PRINT_16CH
	live_stat.frame_bytes[8][1] /1024,live_stat.frame_bytes[9][1] /1024,
	live_stat.frame_bytes[10][1]/1024,live_stat.frame_bytes[11][1]/1024,
	live_stat.frame_bytes[12][1]/1024,live_stat.frame_bytes[13][1]/1024,
	live_stat.frame_bytes[14][1]/1024,live_stat.frame_bytes[15][1]/1024,
#endif
	live_stat.i_bytes[0][1] /1024,live_stat.i_bytes[1][1] /1024,
	live_stat.i_bytes[2][1] /1024,live_stat.i_bytes[3][1] /1024,
	live_stat.i_bytes[4][1] /1024,live_stat.i_bytes[5][1] /1024,
	live_stat.i_bytes[6][1] /1024,live_stat.i_bytes[7][1] /1024,
#if LIVE_PRINT_16CH
	live_stat.i_bytes[8][1] /1024,live_stat.i_bytes[9][1] /1024,
	live_stat.i_bytes[10][1]/1024,live_stat.i_bytes[11][1]/1024,
	live_stat.i_bytes[12][1]/1024,live_stat.i_bytes[13][1]/1024,
	live_stat.i_bytes[14][1]/1024,live_stat.i_bytes[15][1]/1024,
#endif
	live_stat.p_bytes[0][1] /1024,live_stat.p_bytes[1][1] /1024,
	live_stat.p_bytes[2][1] /1024,live_stat.p_bytes[3][1] /1024,
	live_stat.p_bytes[4][1] /1024,live_stat.p_bytes[5][1] /1024,
	live_stat.p_bytes[6][1] /1024,live_stat.p_bytes[7][1] /1024,
#if LIVE_PRINT_16CH
	live_stat.p_bytes[8][1] /1024,live_stat.p_bytes[9][1] /1024,
	live_stat.p_bytes[10][1]/1024,live_stat.p_bytes[11][1]/1024,
	live_stat.p_bytes[12][1]/1024,live_stat.p_bytes[13][1]/1024,
	live_stat.p_bytes[14][1]/1024,live_stat.p_bytes[15][1]/1024,
#endif
#endif
#if LIVE_PRINT_3RD
	live_stat.frame_bytes[0][2] /1024,live_stat.frame_bytes[1][2] /1024,
	live_stat.frame_bytes[2][2] /1024,live_stat.frame_bytes[3][2] /1024,
	live_stat.frame_bytes[4][2] /1024,live_stat.frame_bytes[5][2] /1024,
	live_stat.frame_bytes[6][2] /1024,live_stat.frame_bytes[7][2] /1024,
#if LIVE_PRINT_16CH
	live_stat.frame_bytes[8][2] /1024,live_stat.frame_bytes[9][2] /1024,
	live_stat.frame_bytes[10][2]/1024,live_stat.frame_bytes[11][2]/1024,
	live_stat.frame_bytes[12][2]/1024,live_stat.frame_bytes[13][2]/1024,
	live_stat.frame_bytes[14][2]/1024,live_stat.frame_bytes[15][2]/1024,
#endif
	live_stat.i_bytes[0][2] /1024,live_stat.i_bytes[1][2] /1024,
	live_stat.i_bytes[2][2] /1024,live_stat.i_bytes[3][2] /1024,
	live_stat.i_bytes[4][2] /1024,live_stat.i_bytes[5][2] /1024,
	live_stat.i_bytes[6][2] /1024,live_stat.i_bytes[7][2] /1024,
#if LIVE_PRINT_16CH
	live_stat.i_bytes[8][2] /1024,live_stat.i_bytes[9][2] /1024,
	live_stat.i_bytes[10][2]/1024,live_stat.i_bytes[11][2]/1024,
	live_stat.i_bytes[12][2]/1024,live_stat.i_bytes[13][2]/1024,
	live_stat.i_bytes[14][2]/1024,live_stat.i_bytes[15][2]/1024,
#endif
	live_stat.p_bytes[0][2] /1024,live_stat.p_bytes[1][2] /1024,
	live_stat.p_bytes[2][2] /1024,live_stat.p_bytes[3][2] /1024,
	live_stat.p_bytes[4][2] /1024,live_stat.p_bytes[5][2] /1024,
	live_stat.p_bytes[6][2] /1024,live_stat.p_bytes[7][2] /1024,
#if LIVE_PRINT_16CH
	live_stat.p_bytes[8][2] /1024,live_stat.p_bytes[9][2] /1024,
	live_stat.p_bytes[10][2]/1024,live_stat.p_bytes[11][2]/1024,
	live_stat.p_bytes[12][2]/1024,live_stat.p_bytes[13][2]/1024,
	live_stat.p_bytes[14][2]/1024,live_stat.p_bytes[15][2]/1024,
#endif
#endif
#if LIVE_PRINT_AUDIO
	live_stat.frame_bytes[0][3] /1024,live_stat.frame_bytes[1][3] /1024,
	live_stat.frame_bytes[2][3] /1024,live_stat.frame_bytes[3][3] /1024,
	live_stat.frame_bytes[4][3] /1024,live_stat.frame_bytes[5][3] /1024,
	live_stat.frame_bytes[6][3] /1024,live_stat.frame_bytes[7][3] /1024,
#if LIVE_PRINT_16CH
	live_stat.frame_bytes[8][3] /1024,live_stat.frame_bytes[9][3] /1024,
	live_stat.frame_bytes[10][3]/1024,live_stat.frame_bytes[11][3]/1024,
	live_stat.frame_bytes[12][3]/1024,live_stat.frame_bytes[13][3]/1024,
	live_stat.frame_bytes[14][3]/1024,live_stat.frame_bytes[15][3]/1024,
#endif
#endif
	live_stat.frame_len_average[0][0],  live_stat.frame_len_average[1][0],
	live_stat.frame_len_average[2][0],  live_stat.frame_len_average[3][0],
	live_stat.frame_len_average[4][0],  live_stat.frame_len_average[5][0],
	live_stat.frame_len_average[6][0],  live_stat.frame_len_average[7][0],
#if LIVE_PRINT_16CH
	live_stat.frame_len_average[8][0],  live_stat.frame_len_average[9][0],
	live_stat.frame_len_average[10][0], live_stat.frame_len_average[11][0],
	live_stat.frame_len_average[12][0], live_stat.frame_len_average[13][0],
	live_stat.frame_len_average[14][0], live_stat.frame_len_average[15][0],
#endif
	live_stat.i_avg[0][0],  live_stat.i_avg[1][0],
	live_stat.i_avg[2][0],  live_stat.i_avg[3][0],
	live_stat.i_avg[4][0],  live_stat.i_avg[5][0],
	live_stat.i_avg[6][0],  live_stat.i_avg[7][0],
#if LIVE_PRINT_16CH
	live_stat.i_avg[8][0],  live_stat.i_avg[9][0],
	live_stat.i_avg[10][0], live_stat.i_avg[11][0],
	live_stat.i_avg[12][0], live_stat.i_avg[13][0],
	live_stat.i_avg[14][0], live_stat.i_avg[15][0],
#endif
	live_stat.p_avg[0][0],  live_stat.p_avg[1][0],
	live_stat.p_avg[2][0],  live_stat.p_avg[3][0],
	live_stat.p_avg[4][0],  live_stat.p_avg[5][0],
	live_stat.p_avg[6][0],  live_stat.p_avg[7][0],
#if LIVE_PRINT_16CH
	live_stat.p_avg[8][0],  live_stat.p_avg[9][0],
	live_stat.p_avg[10][0], live_stat.p_avg[11][0],
	live_stat.p_avg[12][0], live_stat.p_avg[13][0],
	live_stat.p_avg[14][0], live_stat.p_avg[15][0],
#endif
#if LIVE_PRINT_2ND
	live_stat.frame_len_average[0][1],  live_stat.frame_len_average[1][1],
	live_stat.frame_len_average[2][1],  live_stat.frame_len_average[3][1],
	live_stat.frame_len_average[4][1],  live_stat.frame_len_average[5][1],
	live_stat.frame_len_average[6][1],  live_stat.frame_len_average[7][1],
#if LIVE_PRINT_16CH
	live_stat.frame_len_average[8][1],  live_stat.frame_len_average[9][1],
	live_stat.frame_len_average[10][1], live_stat.frame_len_average[11][1],
	live_stat.frame_len_average[12][1], live_stat.frame_len_average[13][1],
	live_stat.frame_len_average[14][1], live_stat.frame_len_average[15][1],
#endif
	live_stat.i_avg[0][1],  live_stat.i_avg[1][1],
	live_stat.i_avg[2][1],  live_stat.i_avg[3][1],
	live_stat.i_avg[4][1],  live_stat.i_avg[5][1],
	live_stat.i_avg[6][1],  live_stat.i_avg[7][1],
#if LIVE_PRINT_16CH
	live_stat.i_avg[8][1],  live_stat.i_avg[9][1],
	live_stat.i_avg[10][1], live_stat.i_avg[11][1],
	live_stat.i_avg[12][1], live_stat.i_avg[13][1],
	live_stat.i_avg[14][1], live_stat.i_avg[15][1],
#endif
	live_stat.p_avg[0][1],  live_stat.p_avg[1][1],
	live_stat.p_avg[2][1],  live_stat.p_avg[3][1],
	live_stat.p_avg[4][1],  live_stat.p_avg[5][1],
	live_stat.p_avg[6][1],  live_stat.p_avg[7][1],
#if LIVE_PRINT_16CH
	live_stat.p_avg[8][1],  live_stat.p_avg[9][1],
	live_stat.p_avg[10][1], live_stat.p_avg[11][1],
	live_stat.p_avg[12][1], live_stat.p_avg[13][1],
	live_stat.p_avg[14][1], live_stat.p_avg[15][1],
#endif
#endif
#if LIVE_PRINT_3RD
	live_stat.frame_len_average[0][2],  live_stat.frame_len_average[1][2],
	live_stat.frame_len_average[2][2],  live_stat.frame_len_average[3][2],
	live_stat.frame_len_average[4][2],  live_stat.frame_len_average[5][2],
	live_stat.frame_len_average[6][2],  live_stat.frame_len_average[7][2],
#if LIVE_PRINT_16CH
	live_stat.frame_len_average[8][2],  live_stat.frame_len_average[9][2],
	live_stat.frame_len_average[10][2], live_stat.frame_len_average[11][2],
	live_stat.frame_len_average[12][2], live_stat.frame_len_average[13][2],
	live_stat.frame_len_average[14][2], live_stat.frame_len_average[15][2],
#endif
	live_stat.i_avg[0][2],  live_stat.i_avg[1][2],
	live_stat.i_avg[2][2],  live_stat.i_avg[3][2],
	live_stat.i_avg[4][2],  live_stat.i_avg[5][2],
	live_stat.i_avg[6][2],  live_stat.i_avg[7][2],
#if LIVE_PRINT_16CH
	live_stat.i_avg[8][2],  live_stat.i_avg[9][2],
	live_stat.i_avg[10][2], live_stat.i_avg[11][2],
	live_stat.i_avg[12][2], live_stat.i_avg[13][2],
	live_stat.i_avg[14][2], live_stat.i_avg[15][2],
#endif
	live_stat.p_avg[0][2],  live_stat.p_avg[1][2],
	live_stat.p_avg[2][2],  live_stat.p_avg[3][2],
	live_stat.p_avg[4][2],  live_stat.p_avg[5][2],
	live_stat.p_avg[6][2],  live_stat.p_avg[7][2],
#if LIVE_PRINT_16CH
	live_stat.p_avg[8][2],  live_stat.p_avg[9][2],
	live_stat.p_avg[10][2], live_stat.p_avg[11][2],
	live_stat.p_avg[12][2], live_stat.p_avg[13][2],
	live_stat.p_avg[14][2], live_stat.p_avg[15][2],
#endif
#endif
#if LIVE_PRINT_AUDIO
	live_stat.frame_len_average[0][3],  live_stat.frame_len_average[1][3],
	live_stat.frame_len_average[2][3],  live_stat.frame_len_average[3][3],
	live_stat.frame_len_average[4][3],  live_stat.frame_len_average[5][3],
	live_stat.frame_len_average[6][3],  live_stat.frame_len_average[7][3],
#if LIVE_PRINT_16CH
	live_stat.frame_len_average[8][3],  live_stat.frame_len_average[9][3],
	live_stat.frame_len_average[10][3], live_stat.frame_len_average[11][3],
	live_stat.frame_len_average[12][3], live_stat.frame_len_average[13][3],
	live_stat.frame_len_average[14][3], live_stat.frame_len_average[15][3],
#endif
#endif
	live_stat.i_max_len[0][0],  live_stat.i_max_len[1][0],
	live_stat.i_max_len[2][0],  live_stat.i_max_len[3][0],
	live_stat.i_max_len[4][0],  live_stat.i_max_len[5][0],
	live_stat.i_max_len[6][0],  live_stat.i_max_len[7][0],
#if LIVE_PRINT_16CH
	live_stat.i_max_len[8][0],  live_stat.i_max_len[9][0],
	live_stat.i_max_len[10][0], live_stat.i_max_len[11][0],
	live_stat.i_max_len[12][0], live_stat.i_max_len[13][0],
	live_stat.i_max_len[14][0], live_stat.i_max_len[15][0],
#endif
#if LIVE_PRINT_2ND
	live_stat.i_max_len[0][1],  live_stat.i_max_len[1][1],
	live_stat.i_max_len[2][1],  live_stat.i_max_len[3][1],
	live_stat.i_max_len[4][1],  live_stat.i_max_len[5][1],
	live_stat.i_max_len[6][1],  live_stat.i_max_len[7][1],
#if LIVE_PRINT_16CH
	live_stat.i_max_len[8][1],  live_stat.i_max_len[9][1],
	live_stat.i_max_len[10][1], live_stat.i_max_len[11][1],
	live_stat.i_max_len[12][1], live_stat.i_max_len[13][1],
	live_stat.i_max_len[14][1], live_stat.i_max_len[15][1],
#endif
#endif
#if LIVE_PRINT_3RD
	live_stat.i_max_len[0][2],  live_stat.i_max_len[1][2],
	live_stat.i_max_len[2][2],  live_stat.i_max_len[3][2],
	live_stat.i_max_len[4][2],  live_stat.i_max_len[5][2],
	live_stat.i_max_len[6][2],  live_stat.i_max_len[7][2],
#if LIVE_PRINT_16CH
	live_stat.i_max_len[8][2],  live_stat.i_max_len[9][2],
	live_stat.i_max_len[10][2], live_stat.i_max_len[11][2],
	live_stat.i_max_len[12][2], live_stat.i_max_len[13][2],
	live_stat.i_max_len[14][2], live_stat.i_max_len[15][2],
#endif
#endif
	live_stat.p_max_len[0][0],  live_stat.p_max_len[1][0],
	live_stat.p_max_len[2][0],  live_stat.p_max_len[3][0],
	live_stat.p_max_len[4][0],  live_stat.p_max_len[5][0],
	live_stat.p_max_len[6][0],  live_stat.p_max_len[7][0]
#if LIVE_PRINT_16CH
	,
	live_stat.p_max_len[8][0],  live_stat.p_max_len[9][0],
	live_stat.p_max_len[10][0], live_stat.p_max_len[11][0],
	live_stat.p_max_len[12][0], live_stat.p_max_len[13][0],
	live_stat.p_max_len[14][0], live_stat.p_max_len[15][0]
#endif
#if LIVE_PRINT_2ND
	,
	live_stat.p_max_len[0][1],  live_stat.p_max_len[1][1],
	live_stat.p_max_len[2][1],  live_stat.p_max_len[3][1],
	live_stat.p_max_len[4][1],  live_stat.p_max_len[5][1],
	live_stat.p_max_len[6][1],  live_stat.p_max_len[7][1]
#if LIVE_PRINT_16CH
	,
	live_stat.p_max_len[8][1],  live_stat.p_max_len[9][1],
	live_stat.p_max_len[10][1], live_stat.p_max_len[11][1],
	live_stat.p_max_len[12][1], live_stat.p_max_len[13][1],
	live_stat.p_max_len[14][1], live_stat.p_max_len[15][1]
#endif
#endif
#if LIVE_PRINT_3RD
	,
	live_stat.p_max_len[0][2],  live_stat.p_max_len[1][2],
	live_stat.p_max_len[2][2],  live_stat.p_max_len[3][2],
	live_stat.p_max_len[4][2],  live_stat.p_max_len[5][2],
	live_stat.p_max_len[6][2],  live_stat.p_max_len[7][2]
#if LIVE_PRINT_16CH
	,
	live_stat.p_max_len[8][2],  live_stat.p_max_len[9][2],
	live_stat.p_max_len[10][2], live_stat.p_max_len[11][2],
	live_stat.p_max_len[12][2], live_stat.p_max_len[13][2],
	live_stat.p_max_len[14][2], live_stat.p_max_len[15][2]
#endif
#endif
	);

	g_snprintf(ibuf, 4096,
	"  Short interval\n"
	"    min[0] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    20 msec under [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    10 msec under [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"    min[1] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    20 msec under [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    10 msec under [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"    min[2] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    20 msec under [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    10 msec under [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_AUDIO
	"    min[3] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	"  Long interval\n"
	"    max[0] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    40 msec over  [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    70 msec over  [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    100 msec over [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    130 msec over [0]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#if LIVE_PRINT_2ND
	"    max[1] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_2ND
	"    40 msec over  [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    70 msec over  [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    100 msec over [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    130 msec over [1]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_3RD
	"    max[2] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    40 msec over  [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    70 msec over  [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    100 msec over [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
	"    130 msec over [2]   %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
#if LIVE_PRINT_AUDIO
	"    max[3] (msec)       %u,%u,%u,%u, %u,%u,%u,%u\n"
#if LIVE_PRINT_16CH
	"                        %u,%u,%u,%u, %u,%u,%u,%u\n"
#endif
#endif
	,
	live_stat.frame_interval_min [0][0], live_stat.frame_interval_min [1][0],
	live_stat.frame_interval_min [2][0], live_stat.frame_interval_min [3][0],
	live_stat.frame_interval_min [4][0], live_stat.frame_interval_min [5][0],
	live_stat.frame_interval_min [6][0], live_stat.frame_interval_min [7][0],
#if LIVE_PRINT_16CH
	live_stat.frame_interval_min [8][0], live_stat.frame_interval_min [9][0],
	live_stat.frame_interval_min[10][0], live_stat.frame_interval_min[11][0],
	live_stat.frame_interval_min[12][0], live_stat.frame_interval_min[13][0],
	live_stat.frame_interval_min[14][0], live_stat.frame_interval_min[15][0],
#endif
	live_stat.interval_20_cnt [0][0], live_stat.interval_20_cnt [1][0],
	live_stat.interval_20_cnt [2][0], live_stat.interval_20_cnt [3][0],
	live_stat.interval_20_cnt [4][0], live_stat.interval_20_cnt [5][0],
	live_stat.interval_20_cnt [6][0], live_stat.interval_20_cnt [7][0],
#if LIVE_PRINT_16CH
	live_stat.interval_20_cnt [8][0], live_stat.interval_20_cnt [9][0],
	live_stat.interval_20_cnt[10][0], live_stat.interval_20_cnt[11][0],
	live_stat.interval_20_cnt[12][0], live_stat.interval_20_cnt[13][0],
	live_stat.interval_20_cnt[14][0], live_stat.interval_20_cnt[15][0],
#endif
	live_stat.interval_10_cnt [0][0], live_stat.interval_10_cnt [1][0],
	live_stat.interval_10_cnt [2][0], live_stat.interval_10_cnt [3][0],
	live_stat.interval_10_cnt [4][0], live_stat.interval_10_cnt [5][0],
	live_stat.interval_10_cnt [6][0], live_stat.interval_10_cnt [7][0],
#if LIVE_PRINT_16CH
	live_stat.interval_10_cnt [8][0], live_stat.interval_10_cnt [9][0],
	live_stat.interval_10_cnt[10][0], live_stat.interval_10_cnt[11][0],
	live_stat.interval_10_cnt[12][0], live_stat.interval_10_cnt[13][0],
	live_stat.interval_10_cnt[14][0], live_stat.interval_10_cnt[15][0],
#endif
#if LIVE_PRINT_2ND
	live_stat.frame_interval_min [0][1], live_stat.frame_interval_min [1][1],
	live_stat.frame_interval_min [2][1], live_stat.frame_interval_min [3][1],
	live_stat.frame_interval_min [4][1], live_stat.frame_interval_min [5][1],
	live_stat.frame_interval_min [6][1], live_stat.frame_interval_min [7][1],
#if LIVE_PRINT_16CH
	live_stat.frame_interval_min [8][1], live_stat.frame_interval_min [9][1],
	live_stat.frame_interval_min[10][1], live_stat.frame_interval_min[11][1],
	live_stat.frame_interval_min[12][1], live_stat.frame_interval_min[13][1],
	live_stat.frame_interval_min[14][1], live_stat.frame_interval_min[15][1],
#endif
	live_stat.interval_20_cnt [0][1], live_stat.interval_20_cnt [1][1],
	live_stat.interval_20_cnt [2][1], live_stat.interval_20_cnt [3][1],
	live_stat.interval_20_cnt [4][1], live_stat.interval_20_cnt [5][1],
	live_stat.interval_20_cnt [6][1], live_stat.interval_20_cnt [7][1],
#if LIVE_PRINT_16CH
	live_stat.interval_20_cnt [8][1], live_stat.interval_20_cnt [9][1],
	live_stat.interval_20_cnt[10][1], live_stat.interval_20_cnt[11][1],
	live_stat.interval_20_cnt[12][1], live_stat.interval_20_cnt[13][1],
	live_stat.interval_20_cnt[14][1], live_stat.interval_20_cnt[15][1],
#endif
	live_stat.interval_10_cnt [0][1], live_stat.interval_10_cnt [1][1],
	live_stat.interval_10_cnt [2][1], live_stat.interval_10_cnt [3][1],
	live_stat.interval_10_cnt [4][1], live_stat.interval_10_cnt [5][1],
	live_stat.interval_10_cnt [6][1], live_stat.interval_10_cnt [7][1],
#if LIVE_PRINT_16CH
	live_stat.interval_10_cnt [8][1], live_stat.interval_10_cnt [9][1],
	live_stat.interval_10_cnt[10][1], live_stat.interval_10_cnt[11][1],
	live_stat.interval_10_cnt[12][1], live_stat.interval_10_cnt[13][1],
	live_stat.interval_10_cnt[14][1], live_stat.interval_10_cnt[15][1],
#endif
#endif
#if LIVE_PRINT_3RD
	live_stat.frame_interval_min [0][2], live_stat.frame_interval_min [1][2],
	live_stat.frame_interval_min [2][2], live_stat.frame_interval_min [3][2],
	live_stat.frame_interval_min [4][2], live_stat.frame_interval_min [5][2],
	live_stat.frame_interval_min [6][2], live_stat.frame_interval_min [7][2],
#if LIVE_PRINT_16CH
	live_stat.frame_interval_min [8][2], live_stat.frame_interval_min [9][2],
	live_stat.frame_interval_min[10][2], live_stat.frame_interval_min[11][2],
	live_stat.frame_interval_min[12][2], live_stat.frame_interval_min[13][2],
	live_stat.frame_interval_min[14][2], live_stat.frame_interval_min[15][2],
#endif
	live_stat.interval_20_cnt [0][2], live_stat.interval_20_cnt [1][2],
	live_stat.interval_20_cnt [2][2], live_stat.interval_20_cnt [3][2],
	live_stat.interval_20_cnt [4][2], live_stat.interval_20_cnt [5][2],
	live_stat.interval_20_cnt [6][2], live_stat.interval_20_cnt [7][2],
#if LIVE_PRINT_16CH
	live_stat.interval_20_cnt [8][2], live_stat.interval_20_cnt [9][2],
	live_stat.interval_20_cnt[10][2], live_stat.interval_20_cnt[11][2],
	live_stat.interval_20_cnt[12][2], live_stat.interval_20_cnt[13][2],
	live_stat.interval_20_cnt[14][2], live_stat.interval_20_cnt[15][2],
#endif
	live_stat.interval_10_cnt [0][2], live_stat.interval_10_cnt [1][2],
	live_stat.interval_10_cnt [2][2], live_stat.interval_10_cnt [3][2],
	live_stat.interval_10_cnt [4][2], live_stat.interval_10_cnt [5][2],
	live_stat.interval_10_cnt [6][2], live_stat.interval_10_cnt [7][2],
#if LIVE_PRINT_16CH
	live_stat.interval_10_cnt [8][2], live_stat.interval_10_cnt [9][2],
	live_stat.interval_10_cnt[10][2], live_stat.interval_10_cnt[11][2],
	live_stat.interval_10_cnt[12][2], live_stat.interval_10_cnt[13][2],
	live_stat.interval_10_cnt[14][2], live_stat.interval_10_cnt[15][2],
#endif
#endif
#if LIVE_PRINT_AUDIO
	live_stat.frame_interval_min [0][3], live_stat.frame_interval_min [1][3],
	live_stat.frame_interval_min [2][3], live_stat.frame_interval_min [3][3],
	live_stat.frame_interval_min [4][3], live_stat.frame_interval_min [5][3],
	live_stat.frame_interval_min [6][3], live_stat.frame_interval_min [7][3],
#if LIVE_PRINT_16CH
	live_stat.frame_interval_min [8][3], live_stat.frame_interval_min [9][3],
	live_stat.frame_interval_min[10][3], live_stat.frame_interval_min[11][3],
	live_stat.frame_interval_min[12][3], live_stat.frame_interval_min[13][3],
	live_stat.frame_interval_min[14][3], live_stat.frame_interval_min[15][3],
#endif
#endif
	live_stat.frame_interval_max [0][0], live_stat.frame_interval_max [1][0],
	live_stat.frame_interval_max [2][0], live_stat.frame_interval_max [3][0],
	live_stat.frame_interval_max [4][0], live_stat.frame_interval_max [5][0],
	live_stat.frame_interval_max [6][0], live_stat.frame_interval_max [7][0],
#if LIVE_PRINT_16CH
	live_stat.frame_interval_max [8][0], live_stat.frame_interval_max [9][0],
	live_stat.frame_interval_max[10][0], live_stat.frame_interval_max[11][0],
	live_stat.frame_interval_max[12][0], live_stat.frame_interval_max[13][0],
	live_stat.frame_interval_max[14][0], live_stat.frame_interval_max[15][0],
#endif
	live_stat.interval_40_cnt [0][0], live_stat.interval_40_cnt [1][0],
	live_stat.interval_40_cnt [2][0], live_stat.interval_40_cnt [3][0],
	live_stat.interval_40_cnt [4][0], live_stat.interval_40_cnt [5][0],
	live_stat.interval_40_cnt [6][0], live_stat.interval_40_cnt [7][0],
#if LIVE_PRINT_16CH
	live_stat.interval_40_cnt [8][0], live_stat.interval_40_cnt [9][0],
	live_stat.interval_40_cnt[10][0], live_stat.interval_40_cnt[11][0],
	live_stat.interval_40_cnt[12][0], live_stat.interval_40_cnt[13][0],
	live_stat.interval_40_cnt[14][0], live_stat.interval_40_cnt[15][0],
#endif
	live_stat.interval_70_cnt [0][0], live_stat.interval_70_cnt [1][0],
	live_stat.interval_70_cnt [2][0], live_stat.interval_70_cnt [3][0],
	live_stat.interval_70_cnt [4][0], live_stat.interval_70_cnt [5][0],
	live_stat.interval_70_cnt [6][0], live_stat.interval_70_cnt [7][0],
#if LIVE_PRINT_16CH
	live_stat.interval_70_cnt [8][0], live_stat.interval_70_cnt [9][0],
	live_stat.interval_70_cnt[10][0], live_stat.interval_70_cnt[11][0],
	live_stat.interval_70_cnt[12][0], live_stat.interval_70_cnt[13][0],
	live_stat.interval_70_cnt[14][0], live_stat.interval_70_cnt[15][0],
#endif
	live_stat.interval_100_cnt [0][0], live_stat.interval_100_cnt [1][0],
	live_stat.interval_100_cnt [2][0], live_stat.interval_100_cnt [3][0],
	live_stat.interval_100_cnt [4][0], live_stat.interval_100_cnt [5][0],
	live_stat.interval_100_cnt [6][0], live_stat.interval_100_cnt [7][0],
#if LIVE_PRINT_16CH
	live_stat.interval_100_cnt [8][0], live_stat.interval_100_cnt [9][0],
	live_stat.interval_100_cnt[10][0], live_stat.interval_100_cnt[11][0],
	live_stat.interval_100_cnt[12][0], live_stat.interval_100_cnt[13][0],
	live_stat.interval_100_cnt[14][0], live_stat.interval_100_cnt[15][0],
#endif
	live_stat.interval_130_cnt [0][0], live_stat.interval_130_cnt [1][0],
	live_stat.interval_130_cnt [2][0], live_stat.interval_130_cnt [3][0],
	live_stat.interval_130_cnt [4][0], live_stat.interval_130_cnt [5][0],
	live_stat.interval_130_cnt [6][0], live_stat.interval_130_cnt [7][0]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_130_cnt [8][0], live_stat.interval_130_cnt [9][0],
	live_stat.interval_130_cnt[10][0], live_stat.interval_130_cnt[11][0],
	live_stat.interval_130_cnt[12][0], live_stat.interval_130_cnt[13][0],
	live_stat.interval_130_cnt[14][0], live_stat.interval_130_cnt[15][0]
#endif
#if LIVE_PRINT_2ND
	,
	live_stat.frame_interval_max [0][1], live_stat.frame_interval_max [1][1],
	live_stat.frame_interval_max [2][1], live_stat.frame_interval_max [3][1],
	live_stat.frame_interval_max [4][1], live_stat.frame_interval_max [5][1],
	live_stat.frame_interval_max [6][1], live_stat.frame_interval_max [7][1]
#if LIVE_PRINT_16CH
	,
	live_stat.frame_interval_max [8][1], live_stat.frame_interval_max [9][1],
	live_stat.frame_interval_max[10][1], live_stat.frame_interval_max[11][1],
	live_stat.frame_interval_max[12][1], live_stat.frame_interval_max[13][1],
	live_stat.frame_interval_max[14][1], live_stat.frame_interval_max[15][1]
#endif
	,
	live_stat.interval_40_cnt [0][1], live_stat.interval_40_cnt [1][1],
	live_stat.interval_40_cnt [2][1], live_stat.interval_40_cnt [3][1],
	live_stat.interval_40_cnt [4][1], live_stat.interval_40_cnt [5][1],
	live_stat.interval_40_cnt [6][1], live_stat.interval_40_cnt [7][1]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_40_cnt [8][1], live_stat.interval_40_cnt [9][1],
	live_stat.interval_40_cnt[10][1], live_stat.interval_40_cnt[11][1],
	live_stat.interval_40_cnt[12][1], live_stat.interval_40_cnt[13][1],
	live_stat.interval_40_cnt[14][1], live_stat.interval_40_cnt[15][1]
#endif
	,
	live_stat.interval_70_cnt [0][1], live_stat.interval_70_cnt [1][1],
	live_stat.interval_70_cnt [2][1], live_stat.interval_70_cnt [3][1],
	live_stat.interval_70_cnt [4][1], live_stat.interval_70_cnt [5][1],
	live_stat.interval_70_cnt [6][1], live_stat.interval_70_cnt [7][1]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_70_cnt [8][1], live_stat.interval_70_cnt [9][1],
	live_stat.interval_70_cnt[10][1], live_stat.interval_70_cnt[11][1],
	live_stat.interval_70_cnt[12][1], live_stat.interval_70_cnt[13][1],
	live_stat.interval_70_cnt[14][1], live_stat.interval_70_cnt[15][1]
#endif
	,
	live_stat.interval_100_cnt [0][1], live_stat.interval_100_cnt [1][1],
	live_stat.interval_100_cnt [2][1], live_stat.interval_100_cnt [3][1],
	live_stat.interval_100_cnt [4][1], live_stat.interval_100_cnt [5][1],
	live_stat.interval_100_cnt [6][1], live_stat.interval_100_cnt [7][1]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_100_cnt [8][1], live_stat.interval_100_cnt [9][1],
	live_stat.interval_100_cnt[10][1], live_stat.interval_100_cnt[11][1],
	live_stat.interval_100_cnt[12][1], live_stat.interval_100_cnt[13][1],
	live_stat.interval_100_cnt[14][1], live_stat.interval_100_cnt[15][1]
#endif
	,
	live_stat.interval_130_cnt [0][1], live_stat.interval_130_cnt [1][1],
	live_stat.interval_130_cnt [2][1], live_stat.interval_130_cnt [3][1],
	live_stat.interval_130_cnt [4][1], live_stat.interval_130_cnt [5][1],
	live_stat.interval_130_cnt [6][1], live_stat.interval_130_cnt [7][1]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_130_cnt [8][1], live_stat.interval_130_cnt [9][1],
	live_stat.interval_130_cnt[10][1], live_stat.interval_130_cnt[11][1],
	live_stat.interval_130_cnt[12][1], live_stat.interval_130_cnt[13][1],
	live_stat.interval_130_cnt[14][1], live_stat.interval_130_cnt[15][1]
#endif
#endif
#if LIVE_PRINT_3RD
	,
	live_stat.frame_interval_max [0][2], live_stat.frame_interval_max [1][2],
	live_stat.frame_interval_max [2][2], live_stat.frame_interval_max [3][2],
	live_stat.frame_interval_max [4][2], live_stat.frame_interval_max [5][2],
	live_stat.frame_interval_max [6][2], live_stat.frame_interval_max [7][2]
#if LIVE_PRINT_16CH
	,
	live_stat.frame_interval_max [8][2], live_stat.frame_interval_max [9][2],
	live_stat.frame_interval_max[10][2], live_stat.frame_interval_max[11][2],
	live_stat.frame_interval_max[12][2], live_stat.frame_interval_max[13][2],
	live_stat.frame_interval_max[14][2], live_stat.frame_interval_max[15][2]
#endif
	,
	live_stat.interval_40_cnt [0][2], live_stat.interval_40_cnt [1][2],
	live_stat.interval_40_cnt [2][2], live_stat.interval_40_cnt [3][2],
	live_stat.interval_40_cnt [4][2], live_stat.interval_40_cnt [5][2],
	live_stat.interval_40_cnt [6][2], live_stat.interval_40_cnt [7][2]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_40_cnt [8][2], live_stat.interval_40_cnt [9][2],
	live_stat.interval_40_cnt[10][2], live_stat.interval_40_cnt[11][2],
	live_stat.interval_40_cnt[12][2], live_stat.interval_40_cnt[13][2],
	live_stat.interval_40_cnt[14][2], live_stat.interval_40_cnt[15][2]
#endif
	,
	live_stat.interval_70_cnt [0][2], live_stat.interval_70_cnt [1][2],
	live_stat.interval_70_cnt [2][2], live_stat.interval_70_cnt [3][2],
	live_stat.interval_70_cnt [4][2], live_stat.interval_70_cnt [5][2],
	live_stat.interval_70_cnt [6][2], live_stat.interval_70_cnt [7][2]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_70_cnt [8][2], live_stat.interval_70_cnt [9][2],
	live_stat.interval_70_cnt[10][2], live_stat.interval_70_cnt[11][2],
	live_stat.interval_70_cnt[12][2], live_stat.interval_70_cnt[13][2],
	live_stat.interval_70_cnt[14][2], live_stat.interval_70_cnt[15][2]
#endif
	,
	live_stat.interval_100_cnt [0][2], live_stat.interval_100_cnt [1][2],
	live_stat.interval_100_cnt [2][2], live_stat.interval_100_cnt [3][2],
	live_stat.interval_100_cnt [4][2], live_stat.interval_100_cnt [5][2],
	live_stat.interval_100_cnt [6][2], live_stat.interval_100_cnt [7][2]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_100_cnt [8][2], live_stat.interval_100_cnt [9][2],
	live_stat.interval_100_cnt[10][2], live_stat.interval_100_cnt[11][2],
	live_stat.interval_100_cnt[12][2], live_stat.interval_100_cnt[13][2],
	live_stat.interval_100_cnt[14][2], live_stat.interval_100_cnt[15][2]
#endif
	,
	live_stat.interval_130_cnt [0][2], live_stat.interval_130_cnt [1][2],
	live_stat.interval_130_cnt [2][2], live_stat.interval_130_cnt [3][2],
	live_stat.interval_130_cnt [4][2], live_stat.interval_130_cnt [5][2],
	live_stat.interval_130_cnt [6][2], live_stat.interval_130_cnt [7][2]
#if LIVE_PRINT_16CH
	,
	live_stat.interval_130_cnt [8][2], live_stat.interval_130_cnt [9][2],
	live_stat.interval_130_cnt[10][2], live_stat.interval_130_cnt[11][2],
	live_stat.interval_130_cnt[12][2], live_stat.interval_130_cnt[13][2],
	live_stat.interval_130_cnt[14][2], live_stat.interval_130_cnt[15][2]
#endif
#endif
#if LIVE_PRINT_AUDIO
	,
	live_stat.frame_interval_max [0][3], live_stat.frame_interval_max [1][3],
	live_stat.frame_interval_max [2][3], live_stat.frame_interval_max [3][3],
	live_stat.frame_interval_max [4][3], live_stat.frame_interval_max [5][3],
	live_stat.frame_interval_max [6][3], live_stat.frame_interval_max [7][3]
#if LIVE_PRINT_16CH
	,
	live_stat.frame_interval_max [8][3], live_stat.frame_interval_max [9][3],
	live_stat.frame_interval_max[10][3], live_stat.frame_interval_max[11][3],
	live_stat.frame_interval_max[12][3], live_stat.frame_interval_max[13][3],
	live_stat.frame_interval_max[14][3], live_stat.frame_interval_max[15][3]
#endif
#endif
	);

	g_snprintf(buf, 8704, "%s"
#if LIVE_PRINT_SIZE
			"%s"
#endif
#if LIVE_PRINT_SIZE
			"%s"
#endif
			"=======================================================================\n",
			tbuf
#if LIVE_PRINT_SIZE
			,
			sbuf
#endif
#if LIVE_PRINT_INTERVAL
			,ibuf
#endif
			);

	//printf("%s", buf);

#if MRTPSRC_CAM_STAT_DUMP
	{
		FILE *log_fp = NULL;
		log_fp = fopen("/tmp/webra-info/cam_live_stat.txt", "w");
		if (log_fp != NULL)
		{
			fprintf(log_fp, "%s%s%s=======================================================================\n", tbuf, sbuf, ibuf);
			fclose(log_fp);
		}
	}
#endif
	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, tbuf);
	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sbuf);
	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, ibuf);
	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, buf);
}

///////////////////////////////////////////////////////////////////////////////
void gst_mrtp_src_set_cmd_callback(GobjMrtpSrc* src, GobjMrtpSrcFxnCbCmd callback, gpointer user_data)
{
	src->cmd_callback = callback;
    src->cmd_user_data = user_data;
}

void gst_mrtp_src_set_motion_callback(GobjMrtpSrc* src, GobjMrtpSrcFxnCbMotion callback, gpointer user_data)
{
	src->motion_callback = callback;
    src->motion_user_data = user_data;
}

void gst_mrtp_src_set_alarm_callback(GobjMrtpSrc* src, GobjMrtpSrcFxnCbAlarm callback, gpointer user_data)
{
	src->alarm_callback = callback;
	src->alarm_user_data = user_data;
}

void gst_mrtp_src_set_handoff_callback(GobjMrtpSrc* src, GobjMrtpSrcFxnCbHandoff callback, gpointer user_data)
{
	src->handoff_callback = callback;
    src->handoff_user_data = user_data;
}

void gst_mrtp_src_set_handoff_callback_streamer(GobjMrtpSrc* src, GobjMrtpSrcFxnCbHandoffStreamer callback)
{
	src->handoff_callback_streamer = callback;
}

void gst_mrtp_src_set_header_x_callback(GobjMrtpSrc* src, GobjMrtpSrcFxnCbHeaderX callback, gpointer user_data)
{
	src->header_x_callback = callback;
	src->header_x_user_data = user_data;
}

void gst_mrtp_src_set_onvif_meta_callback(GobjMrtpSrc* src, GobjMrtpSrcFxnCbOnvifMeta callback, gpointer user_data)
{
	src->onvif_meta_callback = callback;
	src->onvif_meta_user_data = user_data;
}

int gst_mrtp_src_send_audio(GobjMrtpSrc* src, GobjMrtpSrcAudioT *audio, gpointer user_data)
{
	guint8* p = NULL;
	gint rtn = 0;
	MRTPSRC_STREAM_T *lst = mrtpsrc_get_stream(audio->ch_num, 0);
	gint copy_len1, copy_len2, remain_buf;
	gint len = 0;

	g_return_val_if_fail(audio != NULL, 0);
	//g_return_val_if_fail(lst->model == MODEL_AMB_A2 || lst->model == MODEL_AMB_D1, 0);

	/* This sends START/END immediately, DATA enqueue */
	if (audio->type == AUDIO_START || audio->type == AUDIO_END)
	{
		if(lst->model != MODEL_ONVIF_GRUNDIG)
		{
			guint8* p1 = NULL;
			guint16* p2 = NULL;
			guint8 buf[16];
			if (lst->state != STATE_PLAYING)
			{
#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MAJOR, "%s | No where to send", __FUNCTION__);
#endif
				lst->abuf.len = 0;
				lst->abuf.start = 0;
				return RTN_OK;
			}

			memset(buf, 0x00, 16);

			/* RTSP header */
			p1 = buf;
			*p1++ = 0x24;
			*p1++ = 0x1;
			p2 = (guint16*) p1;
			*p2++ = htons(2);	// Audio header len + Audio data

			/* Audio header */
			p1 = (guchar*) p2;
			*p1++ = 0xf;
			if (audio->type == AUDIO_START)
			{
				*p1++ = 0x7;
			}
			else if (audio->type == AUDIO_END)
			{
				*p1++ = 0xE;
				/* Flush remained buffer */
				mrtpsrc_tx_audio_lock();
				lst->abuf.len = 0;
				lst->abuf.start = 0;
				mrtpsrc_tx_audio_unlock();
			}

			switch(lst->rtp_protocol)
			{
			case RTP_UNICAST_UDP:
				len = send(lst->udp_sock[1], buf + 4, 2, MSG_DONTWAIT);
				break;

			case RTP_MULTICAST_UDP:
				len = send(lst->udp_sock[1], buf + 4, 2, MSG_DONTWAIT);
				break;

			case RTP_OVER_RTSP_TCP:
				len = send(lst->rtsp_sock, buf, 6, MSG_DONTWAIT);
				break;
			}

			if(len < 0)
			{
				MRTPSRC_DBG(ERROR, "%s | Audio Control send fail(errno%d)", __FUNCTION__, errno);
				return 0;
			}

			/*
			if (send(lst->rtsp_sock, buf, 6, MSG_DONTWAIT) < 0)
			{
				MRTPSRC_DBG(ERROR, "%s | Audio Control send fail(errno%d)", __FUNCTION__, errno);
				return 0;
			}*/


			return 6;
		}
	}


	g_return_val_if_fail(audio->buf != NULL, 0);

	{
		void *tmp_gst_ret = NULL;
		tmp_gst_ret = g_object_ref(audio->buf);
		if(tmp_gst_ret == NULL) {
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(audio->buf);
		}
		else
		{
			mrtpsrc_tx_audio_lock();

			len = gobj_buddy_buffer_buf_get_size(audio->buf);
			remain_buf = MRTPSRC_AUDIO_MAX_SZ - lst->abuf.len;
			if (remain_buf < len)
			{
				MRTPSRC_DBG(WARN, "%s | Audio Tx Queue using %d/%d. Drop(%d)", __FUNCTION__, MRTPSRC_AUDIO_MAX_SZ, lst->abuf.len, len - remain_buf);
				copy_len1 = remain_buf;
				copy_len2 = 0;
				rtn = remain_buf;
			}
			else
			{
				copy_len1 = len;
				copy_len2 = 0;
				rtn = len;
			}
			p = gobj_buddy_buffer_buf_get_addr(audio->buf);
			memcpy(&lst->abuf.data[lst->abuf.len], p, copy_len1);
			lst->abuf.len += copy_len1;
			if (copy_len2 != 0)
			{
				memcpy(&lst->abuf.data[0], p, copy_len2);
				lst->abuf.len += copy_len2;
			}
			mrtpsrc_tx_audio_unlock();
			g_object_unref(audio->buf);
		}
	}

	return rtn;
}

int gst_mrtp_src_open_ch(GobjMrtpSrc* src, GobjMrtpSrcChannel *info, gpointer user_data)
{
	gint rtn = 0;
	gint rtspc_errno = 0;
	gint i = 0;
	MRTPSRC_CHANNEL_T* lch = mrtpsrc_get_channel(info->ch_num);

	g_return_val_if_fail(src != NULL, ERR_OPEN_CONN_FAIL);
	g_return_val_if_fail(info != NULL, ERR_OPEN_CONN_FAIL);
	g_return_val_if_fail(info->video_cnt != 0, ERR_OPEN_CONN_FAIL);

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | start", __FUNCTION__);
#endif

	/* init metadata onoff */
	lch->is_metadata_on = FALSE;
	lch->is_metadata_enable = info->metadata_on;

	for (i = 0; i < info->video_cnt; i++)
	{
		rtn = mrtpsrc_open_stream(info->ch_num, i,
						  info->video[i].ip_addr, info->video[i].rtsp_port, info->video[i].rtsp_addr,
						  info->username, info->password, 30, info->video[i].resolution, info->model_code, info->rtp_method, info->video[i].codec_type);

		if (rtn != RTN_OK)
		{
			rtspc_errno = mrtpsrc_get_errno();
			mrtpsrc_close_stream(info->ch_num, STREAM_1ST);
			mrtpsrc_close_stream(info->ch_num, STREAM_2ND);
#if MRTPSRC_SUPPORT_3RD_STREAM
			mrtpsrc_close_stream(info->ch_num, STREAM_3RD);
#endif
			mrtpsrc_close_stream(info->ch_num, STREAM_AUDIO);
			mrtpsrc_close_stream(info->ch_num, STREAM_META);
			goto open_ch_failed;
		}

		rtspc_errno = mrtpsrc_get_errno();
	}

	if (rtspc_errno == ERR_NO_ERROR && info->audio_cnt != 0)
	{
		rtn = mrtpsrc_open_stream(info->ch_num, STREAM_AUDIO,
						info->audio.ip_addr, info->audio.rtsp_port, info->audio.rtsp_addr,
						info->username, info->password, 0, info->audio.resolution, info->model_code, info->rtp_method, info->video[i].codec_type);
		if (rtn != RTN_OK)
		{
			//rtspc_errno = mrtpsrc_get_errno();
			//mrtpsrc_close_stream(info->ch_num, STREAM_1ST);
			//mrtpsrc_close_stream(info->ch_num, STREAM_2ND);
#if MRTPSRC_SUPPORT_3RD_STREAM
			//mrtpsrc_close_stream(info->ch_num, STREAM_3RD);
#endif
			//mrtpsrc_close_stream(info->ch_num, STREAM_AUDIO);
			//mrtpsrc_close_stream(info->ch_num, STREAM_META);
			//goto open_ch_failed;

			//No audio descriptions, but it's OK. keep going...
			MRTPSRC_DBG(WARN, "%s CH(%d) Onvif audio profile enabled..but No audio description.. (audio off)", __FUNCTION__, info->ch_num);
			mrtpsrc_close_stream(info->ch_num, STREAM_AUDIO);
			info->audio_cnt = 0; 
		}
	}

	/* parameter->metadata_on && metadata control found */
	//if (rtspc_errno == ERR_NO_ERROR && info->metadata_on && lch->is_metadata_on &&
	//		info->model_code >= MODEL_ONVIF_GENERAL && info->model_code != MODEL_NVS)
	if (rtspc_errno == ERR_NO_ERROR && info->metadata_on && lch->is_metadata_on && info->model_code != MODEL_NVS)
	{
		rtn = mrtpsrc_open_stream(info->ch_num, STREAM_META,
				info->video[0].ip_addr, info->video[0].rtsp_port, info->video[0].rtsp_addr,
										info->username, info->password, 30, info->video[0].resolution, info->model_code, info->rtp_method, info->video[i].codec_type);
		if (rtn != RTN_OK)
		{
			rtspc_errno = mrtpsrc_get_errno();
			mrtpsrc_close_stream(info->ch_num, STREAM_1ST);
			mrtpsrc_close_stream(info->ch_num, STREAM_2ND);
#if MRTPSRC_SUPPORT_3RD_STREAM
			mrtpsrc_close_stream(info->ch_num, STREAM_3RD);
#endif
			mrtpsrc_close_stream(info->ch_num, STREAM_AUDIO);
			mrtpsrc_close_stream(info->ch_num, STREAM_META);

			goto open_ch_failed;
		}
	}

open_ch_failed:
	if (src->cmd_callback != NULL)
	{
		if(info->model_code == MODEL_ONTHEFLY_CHEAT)
		{
			src->cmd_callback(20, info->ch_num, 0, rtspc_errno, src->cmd_user_data);
		}
		else
		{
			src->cmd_callback(0, info->ch_num, 0, rtspc_errno, src->cmd_user_data);
		}
	}

	if (rtspc_errno == ERR_NO_ERROR)
	{
		MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(info->ch_num);

		lch->state = STATE_STREAM_READY;
		lch->model_code = info->model_code;
		lch->active_video_cnt = info->video_cnt;
		lch->is_audio_activated = info->audio_cnt;
		if (lch->active_video_cnt == 1)
		{
			lch->stream_2->state = STATE_SHADOW_STREAM;
		}
	}

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | end(%s)", __FUNCTION__, _SYS_ERR_STR[rtspc_errno]);
#endif

	//live_buffering time setting 
	live_buffering[info->ch_num] = MRTP_BUFFERING_DEFAULT_TIME;

	return rtspc_errno;
}

int gst_mrtp_src_update_live_time(GobjMrtpSrc* src, guint ch_num, guint p_time, gpointer user_data)
{
	int err_no = 1;
	live_buffering[ch_num] = p_time;
	//printf("\e[33m [%s][%d] p_time(%d) \e[0m\n", __func__, __LINE__, p_time);
	//err_no is keep alive scenario 
	return err_no;
}

int gst_mrtp_src_close_all(GobjMrtpSrc* src, gpointer user_data)
{
	gint i;
	MRTPSRC_CHANNEL_T *lch = NULL;
	mrtpsrc_closesync_lock();

	for (i=0; i<MRTPSRC_MAX_CH; i++)
	{
		lch = mrtpsrc_get_channel(i);
		lch->state = STATE_TEARED_DOWN;
		lch->is_metadata_on = FALSE;
	}
	for (i=0; i<MRTPSRC_MAX_CH; i++)
	{
		mrtpsrc_close_stream(i, STREAM_META);
		mrtpsrc_close_stream(i, STREAM_AUDIO);
#if MRTPSRC_SUPPORT_3RD_STREAM
		mrtpsrc_close_stream(i, STREAM_3RD);
#endif
		mrtpsrc_close_stream(i, STREAM_2ND);
		mrtpsrc_close_stream(i, STREAM_1ST);
	}
	for (i=0; i<MRTPSRC_MAX_CH; i++)
	{
		if (src->cmd_callback != NULL)
		{
			src->cmd_callback(1, i, 0, ERR_NO_ERROR, src->cmd_user_data);
		}
	}
	mrtpsrc_closesync_unlock();

#if 0
	if (src->cmd_callback != NULL)
	{
		for (i=0; i<16; i++)
		{
			src->cmd_callback(1, i, 0, ERR_NO_ERROR, src->cmd_user_data);
		}
	}
#endif
	return RTN_OK;
}

int gst_mrtp_src_close_ch(GobjMrtpSrc* src, gint ch_num, gpointer user_data)
{
	MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(ch_num);

	mrtpsrc_closesync_lock();
	lch->state = STATE_TEARED_DOWN;
	lch->is_metadata_on = FALSE;
	mrtpsrc_close_stream(ch_num, STREAM_META);
	mrtpsrc_close_stream(ch_num, STREAM_AUDIO);
#if MRTPSRC_SUPPORT_3RD_STREAM
	mrtpsrc_close_stream(ch_num, STREAM_3RD);
#endif
	mrtpsrc_close_stream(ch_num, STREAM_2ND);
	mrtpsrc_close_stream(ch_num, STREAM_1ST);
	if (src->cmd_callback != NULL && lch->model_code != MODEL_ONTHEFLY_CHEAT)
	{
		src->cmd_callback(1, ch_num, 0, ERR_NO_ERROR, src->cmd_user_data);
	}
	mrtpsrc_closesync_unlock();

	return RTN_OK;
}

void gst_mrtp_src_set_live_audio_ch(gint ch)
{
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | live audio channel(%d)", __FUNCTION__, ch);
#endif
	live_audio_ch = ch;
}

void gst_mrtp_src_set_dev_mac(gint ch, gchar* mac)
{
	MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(ch);
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | CH(%d) mac(%02x:%02x:%02x:%02x:%02x:%02x)",
			__FUNCTION__, ch,
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);
#endif
	memcpy(lch->dev_mac, mac, 6);
}

void gst_mrtp_src_get_recv_rate(gchar *buf)
{
	MRTPSRC_STREAM_STAT_T *s = mrtpsrc_get_stat_instance();
	GTimeVal cal_time;
	guint64 total_bytes;
	guint64 bytes_diff;
	guint64 time_diff;

	static GTimeVal init_time;
	static gint initialized = 0;
	static guint64 init_bytes = 0;
	static guint64 last_rtn = 0;

	if (initialized != 1)
	{
		init_bytes = s->recv_bytes;
		g_get_current_time(&init_time);
		initialized = 1;
		last_rtn = 0;
		g_snprintf(buf, 32, "(%llu Kbits)", last_rtn);
		return;
	}

	g_get_current_time(&cal_time);
	total_bytes = s->recv_bytes;
	time_diff = cal_time.tv_sec - init_time.tv_sec;

	if (time_diff < 3)
	{
		g_snprintf(buf, 32, "(%llu Kbits)", last_rtn);
		return;
	}

	bytes_diff = total_bytes - init_bytes;
	bytes_diff = bytes_diff/time_diff;
	bytes_diff = bytes_diff*8;
	bytes_diff = bytes_diff/1024;
	g_snprintf(buf, 32, "(%llu Kbits)", bytes_diff);

	init_time = cal_time;
	init_bytes = total_bytes;
	last_rtn = bytes_diff;
}

#ifndef NMZ_STANDLONE_MODE
GobjMrtpSrc* gst_mrtp_src_get_object(void)
{
	while (this_mrtpsrc == NULL)
	{
		usleep(1000);
	}
	
	return this_mrtpsrc;
}
#endif

void gst_mrtp_src_resolution_changed(MRTPSRC_STREAM_T *lst, guint old_resol, guint new_resol)
{
	guint m_resol;

	MRTPSRC_DBG(MAJOR, "%s | resolution changed(%d, %s from %08x to %08x", __FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], old_resol, new_resol);
	m_resol = (old_resol<<8) | (new_resol);
#ifndef NMZ_STANDLONE_MODE
	if (this_mrtpsrc->cmd_callback != NULL)
	{
		this_mrtpsrc->cmd_callback(10, lst->ch_num, lst->stream_num, m_resol, this_mrtpsrc->cmd_user_data);
	}
#endif
}

#if 0
int gst_mrtp_src_create (GobjMrtpSrc *src, GobjListBuffer **buffer)
{
	GobjListBuffer *list_buf = NULL;
	gint i, j;
#if MRTPSRC_PRINT_MRAW_ABS
	gint k;
#endif
	gint fcnt = 0;
	guint now_alarm_flag = 0;
	//int wait_count = 0;
	MRTPSRC_FRAME_T probe = NULL;
	MRTPSRC_STREAM_T* lst = NULL;
	MRTPSRC_CHANNEL_T* lch = NULL;

#if 1
	{
		int policy, s;
		struct sched_param param;
		pthread_t thread;

		thread = pthread_self();

		policy = SCHED_FIFO;
		//policy = SCHED_RR;
		param.sched_priority = sched_get_priority_max(policy);
		pthread_setschedparam (thread, policy, &param);

		if(create_init == 0)
		{
			s = pthread_getschedparam(thread, &policy, &param);
			if (s == 0)
			{
				display_sched_attr(policy, &param);
			}
			create_init = 1;
		}
	}
#endif


	list_buf = gobj_list_buffer_new();
	now_alarm_flag = last_alarm_flag;
	while (MRTPSRC_ALWAYS)
	{
		unsigned int now_sec;
		unsigned int now_5msec;

		g_get_current_time(&now_time);
		now_sec = now_time.tv_sec;
		now_5msec = now_time.tv_usec / 1000 / 5;

#if LIVE_STATISTICS
		if (live_stat.clear_interval == 0)
		{
			live_stat.clear_interval = 29;
		}
		if (live_stat.base_time.tv_sec + live_stat.clear_interval < now_time.tv_sec)
		{
			MRTPSRC_LIVE_DBG();

			for (i=0; i<16; i++)
			{
				lch = mrtpsrc_get_channel(i);
#if 0
				if (live_stat.frame_count[i][0] != 0 &&
					live_stat.frame_count[i][0] < 800 &&
					live_stat.frame_count[i][0] > 700 &&
					lch->model_code <= MODEL_AMB_D1)
				{
					if (live_buffering[i] >= 0 && live_buffering[i] < 30)
					{
						live_buffering[i] = 30;
					}
				}
				else if (lch->model_code == MODEL_AMB_A2 || lch->model_code == MODEL_AMB_D1)
				{
					live_buffering[i] = 40;
				}
				else
				{
					live_buffering[i] = 0;
				}
#else
				if (lch->model_code == MODEL_AMB_A2 || lch->model_code == MODEL_AMB_D1)
				{
					//live_buffering[i] = 40;
				}
				else
				{
					//live_buffering[i] = 40;			//20172010
					//live_buffering[i] = 400;
				}
#endif
			}

			memset(&live_stat, 0x00, sizeof(MRTPSRC_LIVE_STAT_T));
			live_stat.base_time = now_time;
		}
#endif

		MRTPSRC_STREAM_T *jst = NULL;

		for (i = MRTPSRC_MAX_CH - 1; i >= 0; i--)
		{
			int diff = 0;
			lch = mrtpsrc_get_channel(i);

			if (lch->state < STATE_PLAYING || lch->state > STATE_SHADOW_STREAM)
			{
				continue;
			}
	
			/* Video frames */
			for (j=STREAM_AUDIO-1; j>=STREAM_1ST; j--)
			{
				lst = mrtpsrc_get_stream(i, j);
				diff = now_sec - lst->ts_last_sendsec;

				if (lst->state < STATE_PLAYING || lst->state > STATE_SHADOW_STREAM)
				{
					continue;
				}

				if (lst->ts_last_sendsec != 0 && diff > MRTPSRC_TOLERANT_TS_SECS && lst->state != STATE_SHADOW_STREAM)
				{
					printf("%s | Frame Audit(CH:%d,%s) now(%u) last(%u)\n",
							__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
					//lch->state = STATE_REBOOT_REQ;
					if(lch->model_code == MODEL_ONTHEFLY_CHEAT)
					{
						lch->state = STATE_RECONN_REQ;
					}
					else
					{
						lch->state = STATE_REBOOT_REQ;
					}
					//end_frame_emit[i][0] = 0;
					//end_frame_emit[i][1] = 0;
					break;
				}
#if 0	// Frame Audit enable
				/* Frame audit */
				if (lst->ts_last_second != 0)
				{
					static char end_frame_emit[MRTPSRC_MAX_CH][STREAM_MAX] = { { 0, }, };
					int diff = now_sec - lst->ts_last_second;
					//if (diff > MRTPSRC_TOLERANT_TS_SECS)
					if (diff > MRTPSRC_TOLERANT_TS_SECS /*&& lst->rtp_protocol == RTP_OVER_RTSP_TCP*/)
					{
						MRTPSRC_DBG(ERROR, "%s | Frame Audit(CH:%d,%s) now(%u) last(%u)",
								__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
						//lch->state = STATE_REBOOT_REQ;
						lch->state = STATE_REBOOT_REQ;
						end_frame_emit[i][0] = 0;
						end_frame_emit[i][1] = 0;
						break;
					}
					if (diff > 5 && end_frame_emit[i][j] == 0)
					{
						MRTPSRC_DBG(WARN, "%s | Suspected Frame Audit(CH:%d,%s) now(%u) last(%u)",
								__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
						mrtpsrc_ho_control_frame(lst, 1);
						end_frame_emit[i][j] = 1;
						break;
					}
					if (diff < 5 && end_frame_emit[i][j] == 1)
					{
						MRTPSRC_DBG(WARN, "%s | Recovered Frame Audit(CH:%d,%s) now(%u) last(%u)",
								__FUNCTION__, i, MRTPSRC_STREAM_STR[j], now_sec, lst->ts_last_second);
						mrtpsrc_ho_control_frame(lst, 0);
						end_frame_emit[i][j] = 0;
					}
				}
#endif

				if(lch->model_code == MODEL_ONTHEFLY_CHEAT)
				{
					lst->ts_last_sendsec = now_sec;
					continue;
				}

				if (lst->frame_head == NULL)
				{
					continue;
				}

				mrtpsrc_frame_q_lock();
				if (lst->state >= STATE_PLAYING && lst->state <= STATE_SHADOW_STREAM)
				{
					probe = lst->frame_head;
#if BUFFERING_5MSEC
					//if (lst->model == MODEL_AMB_A2 || lst->model == MODEL_AMB_D1)
					{
						int time_diff;

						time_diff = (now_sec - probe->icodec_h.timestamp)*200 +
									(now_5msec - probe->icodec_h.timestampl);

						if (time_diff < BUFFERING_5MSEC)
						{
							mrtpsrc_frame_q_unlock();
							continue;
						}
					}
#elif BUFFERING_DYNAMIC
					{
						int time_diff;

						time_diff = (now_sec - probe->icodec_h.timestamp)*200 +
									(now_5msec - probe->icodec_h.timestampl);

						if (time_diff < live_buffering[i])
						{
							mrtpsrc_frame_q_unlock();
							continue;
						}
					}
#elif BUFFERING_RELATIVE
#endif
#if !LIVE_STATISTICS
					{
						int time_diff;

						time_diff = (now_sec - live_stat.frame_last_sec[i][j])*1000 +
									(now_time.tv_nsec/1000/1000 - live_stat.frame_last_msec[i][j]);
						if (time_diff < 3)
						{
							mrtpsrc_frame_q_unlock();
							continue;
						}
					}
#endif
					lst->frame_head = lst->frame_head->next;
					mrtpsrc_frame_q_unlock();

					lst->ts_last_sendsec = now_sec;
					if (j == STREAM_1ST)
					{
						if (probe->alarm_flag != 0)
						{
							last_alarm[i] = now_time;
							now_alarm_flag |= (1<<i);
						}
						else
						{
							if (now_time.tv_sec - last_alarm[i].tv_sec > 1)
							{
								now_alarm_flag &= ~(1<<i);
							}
						}
					}
#if 0
					if (probe->alarm_flag != 0 && src->alarm_callback != NULL)
					{
						src->alarm_callback(lst->ch_num, src->alarm_user_data);
					}
#endif
					if (probe->motion_raw.width != 0 && probe->motion_raw.height != 0 && 
								src->motion_callback != NULL)
					{
						if (lst->stream_num == STREAM_1ST)
						{
#if MRTPSRC_PRINT_MRAW_ABS
							printf("===================================\n");
							for (k = 0; k < (probe->motion_raw.width * probe->motion_raw.height); k++)
							{
								if (k!=0 && (k%probe->motion_raw.width == 0)) {printf("\n");}
								printf("%02d ", probe->motion_raw.mraw[k]);
							}
							printf("\n-----------------------------------\n");
#endif
							probe->motion_raw.timestamp = probe->icodec_h.timestamp;
							probe->motion_raw.timestampl = probe->icodec_h.timestampl;
							src->motion_callback((GobjMrtpSrcInfoMotion*)&probe->motion_raw, src->motion_user_data);
						}
					}
					else if (probe->motion_raw.width == 0 && probe->motion_raw.height == 0)
					{
						if (lch->model_code == MODEL_AMB_A2 || lch->model_code == MODEL_AMB_D1 || lch->model_code == MODEL_TI_365)
						{
						}
						else if (lst->stream_num == STREAM_1ST)
						{
							probe->motion_raw.width = 0;
							probe->motion_raw.height = probe->motion_flag;
							probe->motion_raw.timestamp = probe->icodec_h.timestamp;
							probe->motion_raw.timestampl = probe->icodec_h.timestampl;
							src->motion_callback((GobjMrtpSrcInfoMotion*)&probe->motion_raw, src->motion_user_data);
						}
					}

#if LIVE_STATISTICS
					MRTPSRC_LIVE_STAT(i, j, probe);
#endif
#if MRTPSRC_LIVE_PAD_PUSH
					{

						ICODEC_HEADER *icodec = NULL;
						icodec =(ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(probe->frame);
						if(!(icodec->chan < 32 || icodec->chan >= 0))
						{
							printf("[%s:%d] !(icodec->chan < 32 || icodec->chan >= 0) - chan(%d) \n", __FUNCTION__, __LINE__, icodec->chan);
						}

						gobj_list_buffer_push(list_buf, probe->frame);
						*buffer = list_buf;
						memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
						probe = NULL;
						frame_cnt++;
						fcnt++;
					}
#else
					{
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
						memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
						probe = NULL;
					}
#endif
				}
				else
				{
					mrtpsrc_frame_q_unlock();
				}

			}
			/* Video frames end */










#if 1
			/* Audio frames */
			lst = mrtpsrc_get_stream(i, STREAM_AUDIO);

			if (lst->state != STATE_PLAYING)
			{
				continue;
			}

			mrtpsrc_frame_q_lock();
			if (lst->frame_head == NULL)
			{
				mrtpsrc_frame_q_unlock();
				continue;
			}

			probe = lst->frame_head;
			lst->frame_head = lst->frame_head->next;
			mrtpsrc_frame_q_unlock();
#if 0
if (i == 0)
{
	guint a, b;

	a = ((ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(probe->frame))->timestamp;
	b = ((ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(probe->frame))->timestampl;
	printf("AUDIO [%u %u]\n", a, b);
}
#endif
			if (probe != NULL)
			{
#if MRTPSRC_AUDIO_PAD_PUSH
				if (src != NULL && src->handoff_callback_streamer != NULL)
				{
					void *tmp_gst_ret = NULL;
					tmp_gst_ret = g_object_ref(probe->frame);
					if(tmp_gst_ret == NULL) {
						printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
						gst_buffer_debug(probe->frame);
					}
					else
					{
						src->handoff_callback_streamer(probe->frame);
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					}
				}

				if (live_audio_ch == i && GOBJ_IS_BUDDY_BUFFER(probe->frame))
				{
					gobj_list_buffer_push(list_buf, probe->frame);
					*buffer = list_buf;
					memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
					probe = NULL;
					frame_cnt++;
					fcnt++;
				}
				else
				{
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
					probe = NULL;
				}

#else
				{
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
					probe = NULL;
				}
#endif
			}
			/* Audio frames end */
#endif





		}








		if (last_alarm_flag != now_alarm_flag)
		{
			last_alarm_flag = now_alarm_flag;
			if (src->alarm_callback != NULL)
			{
				src->alarm_callback(now_alarm_flag, src->alarm_user_data);
			}
		}




		if (now_time.tv_sec != prev_time.tv_sec)
		{
			if (src != NULL && src->cmd_callback != NULL)
			{
				gint stch = 0;
				for (stch =0; stch<MRTPSRC_MAX_CH; stch++)
				{
					src->cmd_callback(30, stch, 0, live_stat.frame_last_sec[stch][0], src->cmd_user_data);
				}
			}
			prev_time = now_time;
		}



		if (fcnt != 0) { break; }
		g_usleep(3*1000);
	}

	/* count remained frames */
	//if (MRTPSRC_ALWAYS)
	//if (PRINT_REMAIN)
	if (MRTPSRC_NEVER)
	{
		memset(remain_frame, 0x00, sizeof(remain_frame));
		mrtpsrc_frame_q_lock();
		for (i = 0; i < MRTPSRC_MAX_CH; i++)
		{
			for (j = 0; j < STREAM_MAX; j++)
			{
				lst = mrtpsrc_get_stream(i,j);

				probe = lst->frame_head;
				while (probe != NULL)
				{
					remain_frame[i][j]++;
					probe = probe->next;
				}
			}
		}

		lst = NULL;
		probe = NULL;
		mrtpsrc_frame_q_unlock();
	}
	/* count remained frames end */

	//if (MRTPSRC_ALWAYS)
	if (MRTPSRC_NEVER)
	{
		//g_get_current_time(&now_time); 

		if (now_time.tv_sec != prev_time.tv_sec)
		{
			//if (call_cnt != 30)
			{
#if 0
#if STATS_AVERAGE
				int total = 0;

				avg_fcnt[avg_index] = frame_cnt;
				avg_index = (avg_index+1) % AVERAGE_DURATION_SECS;
				for (i = 0; i < AVERAGE_DURATION_SECS; i++)
				{
					total += avg_fcnt[i];
				}

				MRTPSRC_DBG(MAJOR, "[%u] CALL CNT(%u) FRAME(%u) AVG_%d_SECS(%d,%d)", now_time.tv_sec, call_cnt, frame_cnt, AVERAGE_DURATION_SECS, total/AVERAGE_DURATION_SECS, total%AVERAGE_DURATION_SECS);
#else
				MRTPSRC_DBG(MAJOR, "[%u] CALL CNT(%u) PAD_PUSH(%u) FRAME(%u)", now_time.tv_sec, call_cnt, push_cnt, frame_cnt);
#endif
				printf("\n");
#endif
				//printf("\n\nPAD PUSH COUNT %d\n\n", push_cnt);
#if PRINT_REMAIN
				for (i = 0; i < MRTPSRC_MAX_CH; i++)
				{
					printf("     CH %02d     ", i);
					for (j = 0; j < STREAM_MAX; j++)
					{
						lst = mrtpsrc_get_stream(i,j);
						if (lst->state < STATE_STREAM_READY || lst->state > STATE_PAUSED)
							continue;
						printf("%s %u\t", MRTPSRC_STREAM_STR[j], remain_frame[i][j]);
					}
					printf("\n");
				}

				printf("\n");
#endif
			}
			call_cnt = 0;
			frame_cnt = 0;
			push_cnt = 0;
		}
		call_cnt++;

		prev_time = now_time;
	}

	if (MRTPSRC_NEVER)
	{
		if (now_time.tv_sec != prev_time.tv_sec)
		{
			printf("Buffering [%u,%u,%u,%u,%u,%u,%u,%u  %u,%u,%u,%u,%u,%u,%u,%u]\n",
					live_buffering[ 0],  live_buffering[ 1],
					live_buffering[ 2],  live_buffering[ 3],
					live_buffering[ 4],  live_buffering[ 5],
					live_buffering[ 6],  live_buffering[ 7],
					live_buffering[ 8],  live_buffering[ 9],
					live_buffering[10],  live_buffering[11],
					live_buffering[12],  live_buffering[13],
					live_buffering[14],  live_buffering[15]);

			if (src != NULL && src->cmd_callback != NULL)
			{
				gint stch = 0;
				for (stch =0; stch<MRTPSRC_MAX_CH; stch++)
				{
					src->cmd_callback(30, stch, 0, live_stat.frame_last_sec[stch][0], src->cmd_user_data);
				}
			}
		}
		prev_time = now_time;
	}

#if DROP_P_FRAME_ENABLE
	for(i = 0; i < MRTPSRC_MAX_CH; i++)
	{
		for (j = 0; j <= STREAM_2ND; j++)
		{
			lst = mrtpsrc_get_stream(i, j);

			if (lst->state >= STATE_DROP_P_1 && lst->state <= STATE_DROP_P_ALL)
				continue;

			if (remain_frame[i][j] > 30)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(WARN, "[create] %u queue entry, flush all gop p-frames(%d, %s)",
						remain_frame[i][j], i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_ALL;
			}
			else if (remain_frame[i][j] > 15)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(WARN, "[create] 15 queue entries exceeded, drops 5 pframes(%d, %s)",
						i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_5;
			}
			else if (remain_frame[i][j] > 10)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(IMPACT, "[create] 10 queue entries exceeded, drops 2 pframes(%d, %s)",
						i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_2;
			}
			else if (remain_frame[i][j] > 5)
			{
#if PRINT_P_DROP
				MRTPSRC_DBG(IMPACT, "[create] 5 queue entries exceeded, drops 1 pframe(%d, %s)",
						i, MRTPSRC_STREAM_STR[j]);
#endif
				lst->state = STATE_DROP_P_1;
			}
		}
	}
#endif

	push_cnt++;
	return 0;
}
#endif
