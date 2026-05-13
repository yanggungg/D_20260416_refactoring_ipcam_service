#include "nf_common.h"
#include "nf_debug.h"
#include "nf_watchdog.h"
#include "nf_sysdb.h"
#include "nf_action.h"
#include "nf_notify.h"
#include "nf_record.h"
#include "nf_sysman.h"

#include "nf_common_util.h"
#include "nf_util_device.h"
#include "nf_util_time.h"
#include "nf_util_mail.h"
#include "nf_util_jpeg.h"

#include "nf_util_ftp.h"
#include "nf_util_netif.h"

#include "nf_network.h"
#include "nf_api_eventlog.h"

#include "ivca_def.h"
#include "nf_meta_data.h"
#include "nf_pos.h"
#include "nf_sysman.h"
#include "nf_api_dva_eventlog.h"
#include "nf_api_dlva.h"

#include "nf_api_param_fwver.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "action"

#define DEBUG_JBSHELL_ACTION
#ifdef DEBUG_JBSHELL_ACTION
#include "jbshell.h"
#endif

#if defined(ENABLE_SENSOR_IPCAM)
#include "nf_api_ipcam.h"
#endif

#if defined(ENABLE_ACTION_PTZ_PRESET)
#include "nf_ptz.h"
#endif

#if defined(ENABLE_ACTION_MOBILE)
#include "nf_util_sms.h"
#endif

#if defined(ENABLE_ACTION_PUSH)
#include "nf_util_push.h"
#endif
//#define DEBUG_ACTION_SNAPSHOT
//#define EVENTALARMDEBUG
//#define DEBUG_ACTION_LOG
//#define DEBUG_ACTION_KEYLED
//#define ENABLE_IMMEDIATELY_SEND_BOOTING_EMAIL
//#define DEBUG_DVA_OBJ_CNT
//#define DEBUG_VCA_PUSH
//#define DEBUG_AI
//#define DEBUG_BUZZ_TRANS
//#define DEBUG_AI_FR_LPR
//#define DEBUG_EMAIL_ARM
//#define DEBUG_AI_DATA
#define DEBUG_EVENT_OCCUR 0
//#define DEBUG_AI_KEEP_ALIVE 1

#define NF_ACTOIN_ENABLE_SNAPSHOT_CHECK
#define NF_ACTOIN_ENABLE_VIDEO_CHECK
#define ENABLE_ACTION_FTP_SEND


//#define PPP g_message("PUSH - %s - %d", __FUNCTION__, __LINE__);

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	guint _nf_action_num_nvr_alarm = 0;
	guint _nf_action_num_alarm = 0;
#endif

typedef enum _DEBUG_ACTION_IDX_E
{
	DEBUG_ACTION_IDX_CB 			= 0,
	DEBUG_ACTION_IDX_CB_SIMPLE 		= 1,
	DEBUG_ACTION_IDX_SYSDB 			= 2,
	DEBUG_ACTION_IDX_RELAY			= 3,

	DEBUG_ACTION_IDX_RELAY_RELOAD	= 4,
	DEBUG_ACTION_IDX_RELAY_INIT		= 5,
	DEBUG_ACTION_IDX_RELAY_OUT		= 6,
	DEBUG_ACTION_IDX_BUZZER 		= 7,

	DEBUG_ACTION_IDX_BUZZER_RELOAD 	= 8,
	DEBUG_ACTION_IDX_BUZZER_INIT 	= 9,
	DEBUG_ACTION_IDX_BUZZER_OUT		= 10,
	DEBUG_ACTION_IDX_EMAIL_RELOAD	= 11,

	DEBUG_ACTION_IDX_EMAIL_INIT		= 12,
	DEBUG_ACTION_IDX_EMAIL_RESET	= 13,
	DEBUG_ACTION_IDX_EMAIL_SEND		= 14,
	DEBUG_ACTION_IDX_EMAIL_ACTION	= 15,

	DEBUG_ACTION_IDX_HDD_EVENT		= 16,
	DEBUG_ACTION_IDX_SETUP_CHG		= 17,
	DEBUG_ACTION_IDX_BOOTING		= 18,
	DEBUG_ACTION_IDX_FTP_RELOAD		= 19,

	DEBUG_ACTION_IDX_FTP_INIT		= 20,
	DEBUG_ACTION_IDX_FTP_RESET		= 21,
	DEBUG_ACTION_IDX_FTP_SEND		= 22,
	DEBUG_ACTION_IDX_FTP_ACTION		= 23,

	DEBUG_ACTION_IDX_PUSH_INIT		= 24,
	DEBUG_ACTION_IDX_PUSH_RESET		= 25,
	DEBUG_ACTION_IDX_PUSH_SEND		= 26,
	DEBUG_ACTION_IDX_PUSH_ACTION	= 27,

	DEBUG_ACTION_IDX_NR
}DEBUG_ACTION_IDX_E;

static const char *_DEBUG_ACTION_str[32] =
{
	"ACTION_IDX_CB",
	"ACTION_IDX_CB_SIMPLE",
	"ACTION_IDX_SYSDB",
	"ACTION_IDX_RELAY",

	"ACTION_IDX_RELAY_RELOAD",
	"ACTION_IDX_RELAY_INIT",
	"ACTION_IDX_RELAY_OUT",
	"ACTION_IDX_BUZZER",

	"ACTION_IDX_BUZZER_RELOAD",
	"ACTION_IDX_BUZZER_INIT",
	"ACTION_IDX_BUZZER_OUT",
	"ACTION_IDX_EMAIL_RELOAD",

	"ACTION_IDX_EMAIL_INIT",
	"ACTION_IDX_EMAIL_RESET",
	"ACTION_IDX_EMAIL_SEND",
	"ACTION_IDX_EMAIL_ACTION",

	"ACTION_IDX_IDX_HDD_EVENT",
	"ACTION_IDX_IDX_SETUP_CHG",
	"ACTION_IDX_IDX_BOOTING",
	"ACTION_IDX_FTP_RELOAD",

	"ACTION_IDX_FTP_INIT",
	"ACTION_IDX_FTP_RESET",
	"ACTION_IDX_FTP_SEND",
	"ACTION_IDX_FTP_ACTION",

	"ACTION_IDX_PUSH_INIT",
	"ACTION_IDX_PUSH_RESET",
	"ACTION_IDX_PUSH_SEND",
	"ACTION_IDX_PUSH_ACTION",

	"ACTION_IDX_NR"
};

static gint _DEBUG_ACTION_log[32] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

/* Object signals and args */
enum
{
	LAST_SIGNAL
};
static 
gchar _vcalog_str[12][64] = {
	"VA etc",
	"VA Positive direction",
	"VA Negative direction",
	"VA Enter",
	"VA Exit",
	"VA Stopped",
	"VA Abandoned",
	"VA Removed",
	"VA Loitering",
	"VA Fall",
	"VA Counter",
	"VA Camera tamper",
};

/** IMSI AI BOX **/
#if defined(VIDECON)
static 
gchar _ailog_str[17][64] = {
	"AI etc",
	"AI Positive direction",
	"AI Negative direction",
	"AI Enter",
	"AI Exit",
	"AI Stopped",
	"AI Abandoned",
	"AI Removed",
	"AI Loitering",
	"AI Fall",
	"AI Counter",
	"AI Camera tamper",
	"AI Color",
	"AI Size",
	"AI Class",
	"AI Speed",
	"AI Intrusion",
};
#else
static 
gchar _ailog_str[17][64] = {
	"AI etc",
	"AI Positive direction",
	"AI Negative direction",
	"AI Enter",
	"AI Exit",
	"AI Stopped",
	"AI Abandoned",
	"AI Removed",
	"AI Loitering",
	"AI Fall",
	"AI Counter",
	"AI Camera tamper",
	"AI Color",
	"AI Size",
	"AI Class",
	"AI Speed",
	"AI Intrusion",
};

#endif

typedef enum _NF_ACTION_IDX_DLVA_EVENT_TYPE
{
	NF_ACTION_IDX_NONE,
	NF_ACTION_IDX_HUMAN_VEHICLE_DETECTOR,
	NF_ACTION_IDX_GENERIC_EVENT,
} NF_ACTION_IDX_DLVA_EVENT_TYPE;

static const char *_ACTION_STR_TABLE_DLVA_EVENT_TYPE[] = {
	"NONE",
	"HUMAN/VEHICLE DETECTOR",
#if defined(VIDECON)
	"DL RULE",
#else
	"DL RULE",
#endif
};

static const char *_ACTION_STR_TABLE_TYPE0[EMAIL_EVENT_TYPE0_NR] = {
	"Alarm monitoring disarmed.",
	"Alarm monitoring armed."
};

static const char *_ACTION_STR_TABLE_TYPE1[NF_ACTION_LANG_ID_MAX][LANG_ID_NR] = {
	// English
	{	"ALARM             :",					// 0
		"MOTION            :",					// 1
		"VLOSS             :",					// 2
		"PANIC RECORD",							// 3
		"REC ALARM         :",					// 4
		"REC MOTION        :",					// 5
		"TEXT_IN           :",					// 6
#if defined(ENABLE_EVENT_TAMPER)
		"TAMPER          :",
#endif
#if defined(SUPPORT_VCA_CAMERA)
		"Intelligent Video Watch ",
#endif
		"SETUP_CHANGE      :",					// 7
		"CH",									// 8
		"NVR Event E-Mail",						// 9
		"Time",									// 10
		"System ID",							// 11
		"Count Occurrence",						// 12
		"Occurred..",						// 13
		"POS             :",					// 14
		"Intrusion detection ",
		"illegal parking ",
		"License plate ",
#if defined(VIDECON)
		"DL Object Counter ",
		"DL EVENT", 	/** IMSI AI BOX **/
#else
		"DL Object Counter ",
		"AI ANALYTICS ",					/** IMSI AI BOX **/
		"AI FACE RECOGNITION ",				/** IMSI AI FR LPR **/
		"AI LICENSE PLATE RECOGNITION ",
		"AI BOX RULE ",
#endif	
	},

	// korean
	{	"�˶�              :",					// 0
		"�����Ӱ���        :",					// 1
		"���� �ս�       :",					// 2
		"��� ��ȭ",							// 3
		"�˶� ��ȭ         :",					// 4
		"�����Ӱ��� ��ȭ   :",					// 5
		"TEXT_IN           :",					// 6
#if defined(ENABLE_EVENT_TAMPER)
		"TAMPER          :",
#endif
#if defined(SUPPORT_VCA_CAMERA)
		"�����м�����",
#endif
		"���� ����         :",					// 7
		"ä��",									// 8
		"NVR Event E-Mail",						// 9
		"�ð�",									// 10
		"System ID",							// 11
		"ȸ �߻�",						        // 12
		"�߻�",								    // 13
		"POS             :",					// 14
		"ħ�� Ž��",
		"�ҹ� ����",
		"���� ��ȣ��",
#if defined(VIDECON)
		"DL Object Counter ",
		"DL EVENT", 	/** IMSI AI BOX **/
#else
		"DL Object Counter ",
		"AI ANALYTICS ", 	/** IMSI AI BOX **/
		"AI FACE RECOGNITION ",				/** IMSI AI FR LPR **/
		"AI LICENSE PLATE RECOGNITION ",
		"AI GENERIC ",
#endif	
	},
};

static const char *_ACTION_STR_TABLE_TYPE2[NF_ACTION_LANG_ID_MAX][EMAIL_EVENT_TYPE2_NR][EMAIL_EVENT_TYPE2_INDEX_MAX] = {
	// English
	{	{"HDD_EVENT(OVERWRITE)", "HDD_EVENT(SMART)", "HDD_EVENT(NO DISK)",
			"HDD_EVENT(WFAIL)",  "HDD_EVENT(DISK EXHAUSTED)", "HDD_EVENT(DISK FULL)",
			"HDD_EVENT(SMART_REQ)" },
	{"BOOTING", "LOG ON FAIL             :", "FAN FAIL",
#if defined(ENABLE_POE_CHECK)
		"TEMPERATURE FAIL", "POE FAIL", "System2 Reserved",
#else
		"TEMPERATURE FAIL", "System1 Reserved", "System2 Reserved",
#endif
		"System3 Reserved" },
	{"NET_ETHERNET TROUBLE", "NET REMOTE LOG ON FAIL  :", "NET DDNS UPDATE FAIL",
		"Net1 Reserved", 
	#if defined(VIDECON)
		"NET DL PLUS CONNECTION FAIL ", 
	#else
		"NET AI BOX CONNECTION FAIL ", 
	#endif
		"Net3 Reserved",
		"Net4 Reserved" },
	{"Not Use", "Not Use", "Not Use", "Not Use", "Not Use", "Not Use", "Not Use"}
	},

	// Korean
	{	{"��ũ ����� ���� �̺�Ʈ", "��ũ SMART �̺�Ʈ", "��ũ ���� �̺�Ʈ",
			"��ũ ������� �̺�Ʈ", "��ũ �������� �̺�Ʈ", "��ũ ������ �̺�Ʈ",
			"��ũ SMART���� �̺�Ʈ" },
	{"����", "�α��� ����             :", "�� ����",
#if defined(ENABLE_POE_CHECK)
		"�µ� �̻�", "POE �̻�", "System2 Reserved",
#else
		"�µ� �̻�", "System1 Reserved", "System2 Reserved",
#endif
		"System3 Reserved" },
	{"��Ʈ��ũ �̻�", "��Ʈ��ũ �α��� ����   :", "DDNS ������Ʈ ����",
		"Net1 Reserved", 
	#if defined(VIDECON)
		"NET DL PLUS CONNECTION FAIL ", 
	#else
		"NET AI BOX CONNECTION FAIL ", 
	#endif 
		"Net3 Reserved",
		"Net4 Reserved"
	},
	{"Not Use", "Not Use", "Not Use", "Not Use", "Not Use", "Not Use", "Not Use"},
	}

};

static const char *_ACTION_STR_TABLE_TYPE3[NF_ACTION_LANG_ID_MAX][EMAIL_EVENT_TYPE3_NR][EMAIL_EVENT_TYPE3_INDEX_MAX] = {
	{
#if defined(SUPPORT_VCA_CAMERA)
		{"Crossed Positive Direction", "Crossed Negative Direction", "Entered", "Exited", "Stopped", "ABANDONED",
			"Removed", "Loiter", "FALL", "COUNTER", "TAMPER", "COLOR",
			"SIZE", "CLASS", "SPEED", "Intrusion"
		}
#endif
	},
	{
#if defined(SUPPORT_VCA_CAMERA)
		{"������ ������", "������ ������", "����", "����", "����", "������",
			"����", "��ȸ", "�Ѿ���", "�̵��ڰ��", "ī�޶� ����ȭ", "color",
			"size", "class", "speed", "ħ��"
		}
#endif
	}
};
/** DVA Event **/
/*EVENT�� PERSON���� �ö���µ� ������ �Ŵ� HUMAN���� ������ �ϤѤ�... */
static const char *_ACTION_STR_TABLE3_TYPE4[2] = {"bike", "������"};
static const char *_ACTION_STR_TABLE2_TYPE4[NF_ACTION_LANG_ID_MAX][NF_ACTION_DVA_DESC_NR] = {
	{
		"human", "vehicle", "animal", "illegal parking"
	},
	{
		"���", "����", "����", "�ҹ� ����"
	}
};
static const char *_ACTION_STR_TABLE_TYPE4[NF_ACTION_LANG_ID_MAX][EMAIL_EVENT_TYPE4_NR][EMAIL_EVENT_TYPE4_INDEX_MAX] = {
	{
		{
			"person", "bicycle", "bike", "bus", "car", "bird",
			"cat", "dog", "cow", "horse"
		},
		{	
			"bicycle", "bike","bus", "car"
		}
	},
	{
		{
			"���", "������", "�������", "����", "�ڵ���", "��",
			"������", "��", "��", "��"
		},
		{
		 	"������", "�������", "����", "�ڵ���"
		}
	}
};

/** IMSI AI BOX **/
static const char *_ACTION_STR_TABLE_TYPE5[NF_ACTION_LANG_ID_MAX][EMAIL_EVENT_TYPE5_NR][EMAIL_EVENT_TYPE5_INDEX_MAX] = {
	{
		{"Crossed Positive Direction", "Crossed Negative Direction", "Entered", "Exited", "Stopped", "ABANDONED",
			"Removed", "Loiter", "FALL", "COUNTER", "TAMPER", "COLOR",
			"SIZE", "CLASS", "SPEED", "Intrusion"
		}
	},
	{
		{"������ ������", "������ ������", "����", "����", "����", "������",
			"����", "��ȸ", "�Ѿ���", "�̵��ڰ��", "ī�޶� ����ȭ", "color",
			"size", "class", "speed", "ħ��"
		}
	}
};

#if defined(ENABLE_ACTION_MOBILE)
static const char *_ACTION_STR_TABLE_TYPE1_MOBILE[NF_ACTION_LANG_ID_MAX][LANG_ID_NR] = {
	// English
	{	"ALARM : ",								// 0
		"MOTION : ",							// 1
		"VLOSS : ",								// 2
		"PANIC RECORD",							// 3
		"REC ALARM : ",							// 4
		"REC MOTION : ",						// 5
		"TEXT_IN : ",							// 6
#if defined(ENABLE_EVENT_TAMPER)
		"TAMPER : ",
#endif
#if defined(SUPPORT_VCA_CAMERA)
		"VCA :",
#endif
		"SETUP_CHANGE",							// 7
		"CH",									// 8
		"NVR Event E-Mail",						// 9
		"Time",									// 10
		"System ID",							// 11
		"Count Occurrence",						// 12
		"Occured."								// 13
	},
	// korean
	{	"�˶� :",					// 0
		"�����Ӱ��� :",					// 1
		"���� �ս� :",					// 2
		"��� ��ȭ",							// 3
		"�˶� ��ȭ :",					// 4
		"�����Ӱ��� ��ȭ :",					// 5
		"TEXT_IN :",					// 6
#if defined(ENABLE_EVENT_TAMPER)
		"TAMPER :",
#endif
#if defined(SUPPORT_VCA_CAMERA)
		"�����м� ���� : ",
#endif
		"���� ���� :",					// 7
		"ä��",									// 8
		"NVR Event E-Mail",						// 9
		"�ð�",									// 10
		"System ID",							// 11
		"ȸ �߻�",						        // 12
		"�߻�"								    // 13
	},

};
#endif

// added by chcha 2015.4.23.
#if defined(ENABLE_ACTION_PUSH)
static const char *_ACTION_STR_TABLE_TYPE1_PUSH[LANG_PUSH_ID_NR] = {
  // Event Log 문서�?기초�??�여 "log" + LogID + ParameterID ?�식?�로 ?�의?
  "log1401", // "ALARM",                // 0
  "log1501", // "MOTION",              // 1
		"log1600", // "VLOSS",                // 2
		"log2304", // "PANIC RECORD",              // 3
		"log2301", // "REC ALARM : ",              // 4
		"log2302", // "REC MOTION : ",            // 5
		"log1342", // "TEXT_IN : ",              // 6
#if defined(ENABLE_EVENT_TAMPER)
  "log1800", // "TAMPER", // 7
#endif
  "log27", // "POS"
  "log3100", // "DVA_INTRUSION_DETECTION"
  "log3101", // "DVA_ILLEGAL_PARKING"
  "log3103", // "DVA_OBJECT_COUNTER"
#if defined(SUPPORT_VCA_CAMERA)
  "log3000", // "VCA", //8
#endif
};

// added by chcha 2015.4.23.
static const char *_ACTION_STR_TABLE_TYPE2_PUSH[EMAIL_EVENT_TYPE2_NR][EMAIL_EVENT_TYPE2_INDEX_MAX] = {
  {
    "log2200", //"HDD_EVENT(OVER WRITE)",
    "log1900", //"HDD_EVENT(SMART)",
    "log2000", // "HDD_EVENT(NO DISK)",
    "log2003", // "HDD_EVENT(WFAIL)",
    "log2006", // "HDD_EVENT(DISK EXHAUSTED)",
    "log2100", // "HDD_EVENT(DISK FULL)",
    "log1900", // "HDD_EVENT(SMART_REQ)"
  }, {
    "log0000", // "BOOTING",
    "log2518", // "LOG ON FAIL             :",
    "log2507", // "FAN FAIL",
#if defined(ENABLE_POE_CHECK)
    "log2511", // "TEMPERATURE FAIL",
    "log2524", // "POE FAIL",
    "log9999", // "System2 Reserved",
#else
    "log2511", // "TEMPERATURE FAIL",
    "log9999", // "System1 Reserved",
    "log9999", // "System2 Reserved",
#endif
    "log9999", // "System3 Reserved"
  }, {
    "log2803", // "NET_ETHERNET TROUBLE",
    "log2820", // "NET REMOTE LOG ON FAIL  :",
    "log2816", // "NET DDNS UPDATE FAIL",
    "log9999", // "Net1 Reserved", "Net2 Reserved",
    "log2825", // "Net3 Reserved",
    "log9999", // "Net4 Reserved"
  }, {
    "log9999", // "Not Use"
    "log9999", // "Not Use"
    "log9999", // "Not Use"
    "log9999", // "Not Use"
    "log9999", // "Not Use"
    "log9999", // "Not Use"
    "log9999", // "Not Use"
  }
};
static const char *_ACTION_STR_TABLE_TYPE3_PUSH[EMAIL_EVENT_TYPE3_NR] = {
#if defined(SUPPORT_VCA_CAMERA)
  "log3000", // "VCA", //8
#endif
};

static const char *_ACTION_STR_TABLE_TYPE5_PUSH[EMAIL_EVENT_TYPE5_NR] = {
#if defined(SUPPORT_VCA_CAMERA)
  "log3104", // "AI BOX"
#endif
};

#endif // ENABLE_ACTION_PUSH

/** 091222 by pakkhman **/
#if defined (ENABLE_MOUSE_UNTILKEY_STOP)
static volatile guint _mouse_key_in_check=0;
#endif
static GStaticMutex _nf_pos_data_email_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex _nf_pos_data_ftp_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex _nf_pos_data_push_mutex = G_STATIC_MUTEX_INIT;


static void nf_action_class_init (NfActionClass * klass);
static void nf_action_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_action_set_property (GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec);
static void nf_action_get_property (GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec);

static void nf_action_dispose (GObject * object);
static void nf_action_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfAction	*_nf_action = NULL;
static guint64 _relay_webra_onoff = 0;

static void _action_thread_func (NfAction *self);
static void _action_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);	// for sensor, motion, vloss

#if defined(ENABLE_ARI_PANIC)
static void _action_ari_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#endif
static void _action_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_time_change_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

static void _action_sysdb_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_disk_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_disk_usage_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

static void _action_record_cb_func( NF_NOTIFY_INFO *pinfo, gpointer data );
static void _action_net_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_system_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#if defined(ENABLE_FAN_FAIL_CHECK)
static void _action_sysfan_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_sys_temperature_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#endif
static void _action_dva_cb_func( NF_NOTIFY_INFO *pinfo, gpointer data );

#if defined(USE_DEV_GENNUM)
static void _action_std_type_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#endif
#ifdef	SUPPORT_VCA_CAMERA
static void _action_vca_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#endif	/* SUPPORT_VCA_CAMERA */
static void _action_pos_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_ai_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_ai_fr_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_ai_lpr_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _action_ai_generic_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

static void _relay_init(void);
static void _nf_action_alarm_in_init_from_reload(void);
static guint64 _nf_action_reload_alarm_in(void);
static void _relay_init_from_reload(void);
static void _relay_on(gint ch);
static void _relay_off(gint ch);
static void _relay_on_off(guint val);
#if defined(ENABLE_ARI_PANIC)
static void _set_relay_force_off(void);
#endif

static void _set_relay_enable(void);
#if defined(ENABLE_SENSOR_IPCAM)
static void _nf_action_relay_on_ipcam(gint ch);
static void _nf_action_relay_off_ipcam(gint ch);
#endif

static void _buzzer_init(void);
static void _buzzer_onoff(gint flag);
//static void _all_io_reset(void);

static void _relay_action(void);
static void _buzzer_action(void);
#if defined(ENABLE_EMAIL_DUAL_SERVER)
	static gint _email_send(time_t send_time, NF_MAIL_SEND_CONTENT *cont);
	static gboolean nf_action_request_snapshot_sigle_send(NF_MAIL_SEND_CONTENT *cont, EMAIL_SNAPSHOT_DATA *email_snapshot, gint ch);
#else
	static gint _email_send(time_t send_time);
#endif

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	static gboolean nf_action_request_snapshot(EMAIL_SNAPSHOT_DATA *email_snapshot, gint ch);
	static gboolean _email_snapshot_event_check(int  div_count);
#endif

static void	_email_action(void);
static void _email_init(void);
static void _email_reset(glong curr_time);

static void	_ftp_action(void);
static void _ftp_init(void);
static void _ftp_reset(glong curr_time);

#if defined(ENABLE_ACTION_PTZ_PRESET)
static void _preset_action(void);
static void _nf_action_preset_set(gint ch, guint num_preset, guint num_event);
#endif

static void _nf_action_motion_interval_chk(void);
#if defined(ENABLE_EVENT_TAMPER)
static void _nf_action_tamper_interval_chk(void);
#endif
static void _nf_action_load_sensor_data(void);
static void _nf_action_load_motion_data(void);
static void _nf_action_init_motion_status(void);
static void _nf_action_load_vloss_data(void);
static void _nf_action_load_system_disk_data(void);
static void _nf_action_load_system_record_data(void);
static void _nf_action_load_system_sys_data(void);
static void _nf_action_load_system_net_data(void);
static void _nf_action_load_system_poe_data(void);
#ifdef	SUPPORT_VCA_CAMERA
static void _nf_action_load_vca_data(void);
static guint _nf_event_vca_get_evt_mask(guint evt_type);
static void _nf_action_vca_status_check(void);
static guint _nf_event_vca_get_evt_idx(guint evt_type);
#endif	/* SUPPORT_VCA_CAMERA */
static void _nf_action_load_buzzer_data(void);
static void _nf_action_load_relay_data(void);
static void _nf_action_load_email_data(void);
static void _nf_action_load_ftp_data(void);
#if defined(ENABLE_EVENT_TAMPER)
static void _nf_action_load_tamper_data(void);
#endif
static void _nf_action_load_dva_data(void);

#if defined(ENABLE_ACTION_MOBILE)
static void _nf_action_mobile_init(void);
static void _nf_action_load_mobile_data(void);
static void _nf_action_mobile_action(void);
static void _nf_action_mobile_reset(glong curr_time);
static int _nf_action_mobile_is_act(int index_type1, int index_type2, int index_type3, int prop_id, int type);
static void _nf_action_mobile_event_check(MOBILE_DATA *m_data, EMAIL_EVENT_DATA *e_send_data, MOBILE_STATE *m_state);
static gint _nf_action_mobile_send(time_t send_time);
#endif

#if defined(ENABLE_ACTION_PUSH)
static void _nf_action_push_init(void);
static void _nf_action_load_push_data(void);
static void _nf_action_push_action(void);
static void _nf_action_push_reset(glong curr_time);
static int _nf_action_push_is_act(int index_type1, int index_type2, int prop_id, int type);
static void _nf_action_push_event_check(PUSH_DATA *m_data, EMAIL_EVENT_DATA *e_send_data, PUSH_STATE *m_state);
//static gint _nf_action_push_send(time_t send_time);
#endif

static void _nf_action_load_pos_data(void);
static void _pos_reset(int force);

//#define DEBUG_EMAIL_TEST

/** IMSI AI BOX **/
static guint _nf_event_ai_get_evt_idx(guint evt_type);
static void _nf_action_load_ai_data(void);


	GType
nf_action_get_type (void)
{
	static GType nf_action_type = 0;

	if (G_UNLIKELY (nf_action_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfActionClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_action_class_init,
			NULL,
			NULL,
			sizeof (NfAction),
			0,
			(GInstanceInitFunc) nf_action_instance_init,
			NULL
		};

		nf_action_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfAction", &object_info, 0);
	}

	return nf_action_type;
}

	static void
nf_action_class_init (NfActionClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_action_set_property;
	gobject_class->get_property = nf_action_get_property;

	gobject_class->dispose = nf_action_dispose;
	gobject_class->finalize = nf_action_finalize;

}

	static void
nf_action_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfAction *self = NF_ACTION (instance);

	self->init_done = FALSE;

	// queue ?�성
	self->queue = g_async_queue_new();

	self->thread_run = TRUE;
	self->thread = g_thread_create(	(GThreadFunc)_action_thread_func,
			self, FALSE, NULL);
	self->is_booting = TRUE;

	self->email_send_data = g_malloc0( sizeof(EMAIL_EVENT_DATA));
	g_assert(self->email_send_data);

	self->ftp_send_data = g_malloc0( sizeof(EMAIL_EVENT_DATA));
	g_assert(self->ftp_send_data);

#if defined(ENABLE_ACTION_MOBILE)
	self->mobile_send_data = g_malloc0( sizeof(EMAIL_EVENT_DATA));
	g_assert(self->mobile_send_data);
#endif

#if defined(ENABLE_ACTION_PUSH)
	self->push_send_data = g_malloc0( sizeof(EMAIL_EVENT_DATA));
	g_assert(self->push_send_data);
	
	self->push_pos_text_data = g_malloc0( sizeof(POS_TEXT_DATA) * NUM_ACTIVE_CH);
	g_assert(self->push_pos_text_data);
#endif
	self->cb_ai_meta_data = g_malloc0( sizeof(ai_rule_event_t) * NUM_ACTIVE_CH );
	g_assert(self->cb_ai_meta_data);
	
	self->ai_meta_data = g_malloc0( sizeof(ai_rule_event_t) * NUM_ACTIVE_CH );
	g_assert(self->ai_meta_data);

	/** IMSI AI GENERIC EVENT **/
	self->cb_ai_generic_data = g_malloc0( sizeof(ai_generic_event_t) * NUM_ACTIVE_CH );
	g_assert(self->cb_ai_generic_data);

	self->ai_generic_data = g_malloc0( sizeof(ai_generic_event_t) * NUM_ACTIVE_CH );
	g_assert(self->ai_generic_data);

	self->cb_ai_fr_data = g_malloc0( sizeof(ai_fr_event_t) * NUM_ACTIVE_CH );
	g_assert(self->cb_ai_fr_data);
	
	self->cb_ai_lpr_data = g_malloc0( sizeof(ai_lpr_event_t) * NUM_ACTIVE_CH );
	g_assert(self->cb_ai_lpr_data);
	
	self->ai_fr_data = g_malloc0( sizeof(ai_fr_event_t) * NUM_ACTIVE_CH );
	g_assert(self->ai_fr_data);
	
	self->ai_lpr_data = g_malloc0( sizeof(ai_lpr_event_t) * NUM_ACTIVE_CH );
	g_assert(self->ai_lpr_data);

	self->email_pos_text_data = g_malloc0( sizeof(POS_TEXT_DATA) * NUM_ACTIVE_CH );
	g_assert(self->ai_lpr_data);
	
	self->ftp_pos_text_data = g_malloc0( sizeof(POS_TEXT_DATA) * NUM_ACTIVE_CH );
	g_assert(self->ai_lpr_data);

	
	printf("\033[0;35m %s action instance END \033[0;39m\n", __FUNCTION__);

}

/* dispose is called when the object has to release all links
 * to other objects */
	static void
nf_action_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
	static void
nf_action_finalize (GObject * object)
{
	parent_class->finalize (object);

	/*	2013-01-13 ?�후 11:49:11 choissi FIXME
		g_free(self->email_state);
		g_free(self->email_send_data);
		g_free(self->email_snapshot);

		g_free(self->ftp_state);
		g_free(self->ftp_send_data);
		g_free(self->ftp_snapshot);
		*/

}


	static void
nf_action_set_property (GObject * object, guint prop_id,
		const GValue * value, GParamSpec *pspec)
{
	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

	static void
nf_action_get_property (GObject * object, guint prop_id,
		GValue * value, GParamSpec *pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

/**
  @brief				action call back function
  @return gboolean
  */

	static void
_action_thread_func (NfAction *self)
{
	time_t 		tick;
	struct tm	curr_tm;
	u_int smart=0;
	gint boot_sec = 0;
	gint boot_min = 0;
	gint db_tmp = 0;

	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_action == NULL ) g_usleep(10*1000);

	self->init_done = 1;

	sleep(20);	// wait for nf_util_mail init

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_ACTION, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
#endif

	// for smart check
	smart = nf_notify_get_param0("disk_smart");
	smart |= nf_notify_get_param0("disk_smart_reqchk");
	if((smart == 0x1) || (smart == 0x2) || (smart == 0x4))
	{
		_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART);
		_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART);
	}

	while(self->thread_run)
	{
		tick=time(NULL);
		nf_datetime_localtime(&tick, _nf_action->is_dst, &curr_tm);

		NF_OBJECT_LOCK( _nf_action );
		_nf_action->curr_day = curr_tm.tm_wday;
		_nf_action->curr_hour = curr_tm.tm_hour;
		NF_OBJECT_UNLOCK( _nf_action );

		db_tmp = _nf_action->sysdb_reload;

		if(self->is_booting)
		{
			boot_sec = curr_tm.tm_sec;
			boot_min = curr_tm.tm_min;
			
			if(nf_get_nf_mail_send_value() == 0)  // DHCP network config check
				self->is_booting=FALSE;
			
			nf_notify_fire_params("sys_booting", 0x1, 0, 0, 0);
		}
		else
		{
			if((boot_sec == curr_tm.tm_sec) && (boot_min < curr_tm.tm_min))
			{
				NF_OBJECT_LOCK( _nf_action );
				_nf_action->cb_curr_system &= ~(1<<NF_ACTION_SYSTEM_EVENT_BOOTING);
				NF_OBJECT_UNLOCK( _nf_action );
			}
		}

		if(db_tmp)
		{
			gboolean is_buzzer_test=FALSE;
			gint i=0;
			long long  relay_test_bit=0;

			_buzzer_init();

			_nf_action_load_relay_data();
			_nf_action_load_email_data();

#ifdef ENABLE_ACTION_FTP_SEND
			_nf_action_load_ftp_data();
#endif
			_nf_action_load_buzzer_data();
			_nf_action_load_sensor_data();
			_nf_action_load_motion_data();
			_nf_action_load_vloss_data();
			_nf_action_load_system_disk_data();
			_nf_action_load_system_record_data();
			_nf_action_load_system_sys_data();
			_nf_action_load_system_net_data();
#ifdef	SUPPORT_VCA_CAMERA
			_nf_action_load_vca_data();

#endif	/* SUPPORT_VCA_CAMERA */

			/** IMSI AI BOX **/
			_nf_action_load_ai_data();

			_relay_init_from_reload();
#if defined(ENABLE_EVENT_TAMPER)
			_nf_action_load_tamper_data();
#endif
#if defined(ENABLE_ACTION_MOBILE)
			_nf_action_load_mobile_data();
#endif
#if defined(ENABLE_ACTION_PUSH)
			_nf_action_load_push_data();
#endif
			_nf_action_load_pos_data();
			_nf_action_load_dva_data();

			is_buzzer_test=_nf_action->buzzer_state.is_manual_test;
			NF_OBJECT_LOCK( _nf_action );
			memset( &_nf_action->buzzer_state, 0x00, sizeof(BUZZER_STATE) );
			if(is_buzzer_test)
				_nf_action->buzzer_state.is_manual_test=TRUE;
			NF_OBJECT_UNLOCK( _nf_action );

			for(i=0; i<NUM_RELAY; i++)
			{
				if(_nf_action->relay_state[i].is_manual_test)
					relay_test_bit |= (0x1ULL<<i);
			}

			NF_OBJECT_LOCK( _nf_action );
			memset( _nf_action->relay_state, 0x00, sizeof(RELAY_STATE)*NUM_RELAY );
			for(i=0; i<NUM_RELAY; i++)
			{
				if(relay_test_bit & (0x1ULL<<i))
					_nf_action->relay_state[i].is_manual_test = TRUE;
			}
			NF_OBJECT_UNLOCK( _nf_action );

			_email_reset(0);

#ifdef ENABLE_ACTION_FTP_SEND
			_ftp_reset(0);
#endif
#if defined(ENABLE_ACTION_PUSH)
			_nf_action_push_reset(0);
#endif
			_pos_reset(1);
		}
		NF_OBJECT_LOCK( _nf_action );
		{
#if defined(ENABLE_SENSOR_IPCAM)
			_nf_action->rise_alarm = _nf_action->cb_rise_alarm_dvr;
			_nf_action->rise_alarm =(_nf_action->rise_alarm<< NUM_ALARM_IPCAM) | _nf_action->cb_rise_alarm_ipcam;
			memcpy(_nf_action->alarm_timestamp, _nf_action->cb_alarm_timestamp, (sizeof(guint) * (NUM_ALARM_DVR+NUM_ALARM_IPCAM)));
#else
			_nf_action->rise_alarm   = _nf_action->cb_rise_alarm;
			memcpy(_nf_action->alarm_timestamp, _nf_action->cb_alarm_timestamp, (sizeof(guint) * NUM_ALARM));
#endif
			_nf_action->rise_vloss   = _nf_action->cb_rise_vloss;
			memcpy(_nf_action->vloss_timestamp, _nf_action->cb_vloss_timestamp, (sizeof(int)*NUM_ACTIVE_CH));
			
			_nf_action->rise_motion  = _nf_action->cb_rise_motion;
			memcpy(_nf_action->motion_timestamp, _nf_action->cb_motion_timestamp, (sizeof(int)*NUM_ACTIVE_CH));

			_nf_action->rise_hdd	 = _nf_action->cb_rise_hdd;
			_nf_action->hdd_timestamp = _nf_action->cb_hdd_timestamp;
			
			_nf_action->rise_sysdb	 = _nf_action->cb_rise_sysdb;

			_nf_action->rise_rec_panic	= _nf_action->cb_rise_rec_panic;
			_nf_action->rec_panic_timestamp = _nf_action->cb_rec_panic_timestamp;

			_nf_action->rise_rec_alarm	= _nf_action->cb_rise_rec_alarm;
			_nf_action->rise_rec_motion	= _nf_action->cb_rise_rec_motion;

			_nf_action->rise_net		= _nf_action->cb_rise_net;
			_nf_action->net_timestamp = _nf_action->cb_net_timestamp;
			
			_nf_action->rise_system		= _nf_action->cb_rise_system;
			_nf_action->system_timestamp = _nf_action->cb_system_timestamp;
#if defined(ENABLE_EVENT_TAMPER)
			_nf_action->rise_tamper	= _nf_action->cb_rise_tamper;
			memcpy(_nf_action->tamper_timestamp, _nf_action->cb_tamper_timestamp, (sizeof(int)*NUM_ACTIVE_CH));
#endif
#ifdef	SUPPORT_VCA_CAMERA
			memcpy(_nf_action->rise_vca, _nf_action->cb_rise_vca, sizeof(_nf_action->cb_rise_vca));
			memcpy(&_nf_action->vca_timestamp, &_nf_action->cb_vca_timestamp, (sizeof(int)*NUM_ACTIVE_CH*NF_ACTION_VCA_NR));
#endif	/* SUPPORT_VCA_CAMERA */

			/** IMSI AI BOX **/
			memcpy(_nf_action->rise_ai, _nf_action->cb_rise_ai, sizeof(_nf_action->cb_rise_ai));
			memcpy(_nf_action->ai_timestamp, _nf_action->cb_ai_timestamp, (sizeof(int)*NUM_ACTIVE_CH*NF_ACTION_AI_NR));
			memcpy(_nf_action->ai_meta_data, _nf_action->cb_ai_meta_data, ( sizeof(ai_rule_event_t) * NUM_ACTIVE_CH ));

			/** IMSI AI FR LPR **/
			memcpy(_nf_action->ai_fr_data, _nf_action->cb_ai_fr_data, sizeof(ai_fr_event_t) * NUM_ACTIVE_CH);
			memcpy(_nf_action->ai_lpr_data, _nf_action->cb_ai_lpr_data, sizeof(ai_lpr_event_t) * NUM_ACTIVE_CH);
			/** IMSI AI GENERIC EVENT **/
			memcpy(_nf_action->ai_generic_data, _nf_action->cb_ai_generic_data, ( sizeof(ai_generic_event_t) * NUM_ACTIVE_CH ));
			/** AI KEEP ALIVE **/
			//memcpy(_nf_action->ai_keep_alive_state, _nf_action->cb_ai_keep_alive_state, ( sizeof(guint) * NUM_ACTIVE_CH ));

			_nf_action->rise_pos   = _nf_action->cb_rise_pos;
			_nf_action->rise_dva_idz = _nf_action->cb_rise_dva_idz;
			_nf_action->rise_dva_ipz = _nf_action->cb_rise_dva_ipz;
			/*DVA OBJ CNT*/
			_nf_action->rise_dva_obj_cnt = _nf_action->cb_rise_dva_obj_cnt;

			memcpy(&_nf_action->meta_dva_idz, &_nf_action->cb_meta_dva_idz, sizeof(DVA_META));
			memcpy(&_nf_action->meta_dva_ipz, &_nf_action->cb_meta_dva_ipz, sizeof(DVA_META));
			/*DVA OBJ CNT*/
			memcpy(&_nf_action->meta_dva_obj_cnt, &_nf_action->cb_meta_dva_obj_cnt, sizeof(DVA_META));
			/** DVA Event **/
			memcpy(&_nf_action->rise_dva_event, &_nf_action->cb_rise_dva_event, (sizeof(int)*NUM_ACTIVE_CH*EMAIL_EVENT_TYPE4_NR));
			
#if defined(ENABLE_SENSOR_IPCAM)
			_nf_action->curr_alarm = _nf_action->cb_curr_alarm_dvr;
			_nf_action->curr_alarm = (_nf_action->curr_alarm << NUM_ALARM_IPCAM) | _nf_action->cb_curr_alarm_ipcam;
#else
			_nf_action->curr_alarm   = _nf_action->cb_curr_alarm;
#endif

			_nf_action->curr_vloss   = _nf_action->cb_curr_vloss;
			_nf_action->curr_motion  = _nf_action->cb_curr_motion;

			_nf_action->curr_hdd	 = _nf_action->cb_curr_hdd;

			_nf_action->curr_rec_panic	= _nf_action->cb_curr_rec_panic;
			_nf_action->curr_rec_alarm	= _nf_action->cb_curr_rec_alarm;
			_nf_action->curr_rec_motion	= _nf_action->cb_curr_rec_motion;

			_nf_action->curr_net	= _nf_action->cb_curr_net;
			_nf_action->curr_system	= _nf_action->cb_curr_system;
#if defined(ENABLE_EVENT_TAMPER)
			_nf_action->curr_tamper	= _nf_action->cb_curr_tamper;
#endif
#ifdef	SUPPORT_VCA_CAMERA
			memcpy(_nf_action->curr_vca, _nf_action->cb_curr_vca, sizeof(_nf_action->cb_curr_vca));
#endif	/* SUPPORT_VCA_CAMERA */

			/** IMSI AI BOX **/
			memcpy(_nf_action->curr_ai, _nf_action->cb_curr_ai, sizeof(_nf_action->cb_curr_ai));
			
			_nf_action->curr_pos   = _nf_action->cb_curr_pos;
			_nf_action->curr_dva_idz = _nf_action->cb_curr_dva_idz;
			_nf_action->curr_dva_ipz = _nf_action->cb_curr_dva_ipz;
		/*DVA OBJ CNT*/
			_nf_action->curr_dva_obj_cnt = _nf_action->cb_curr_dva_obj_cnt;
		/** DVA Event **/
			memcpy(&_nf_action->curr_dva_event, &_nf_action->cb_curr_dva_event, (sizeof(int)*NUM_ACTIVE_CH*EMAIL_EVENT_TYPE4_NR));

			if(db_tmp)
			{
#ifdef	SUPPORT_VCA_CAMERA
				memcpy(_nf_action->curr_vca, _nf_action->cb_curr_vca, sizeof(_nf_action->cb_curr_vca));
#endif	/* SUPPORT_VCA_CAMERA */

#if defined(ENABLE_SENSOR_IPCAM)
				// Patch For N/C <-> N/O buzzer Beep Bug
				_nf_action->cb_curr_alarm_dvr = 0;
				_nf_action->cb_curr_alarm_ipcam = 0;
#else
				_nf_action->cb_curr_alarm = 0;
				_nf_action->curr_alarm = 0;

				_nf_action->rise_alarm   |= _nf_action->cb_curr_alarm;
#endif
				_nf_action->rise_vloss   |= _nf_action->cb_curr_vloss;
				_nf_action->rise_motion  |= _nf_action->cb_curr_motion;
				_nf_action->rise_hdd	 |= _nf_action->cb_curr_hdd;

				_nf_action->rise_rec_panic	|= _nf_action->cb_curr_rec_panic;
				_nf_action->rise_rec_alarm	|= _nf_action->cb_curr_rec_alarm;
				_nf_action->rise_rec_motion	|= _nf_action->cb_curr_rec_motion;
#if defined(ENABLE_EVENT_TAMPER)
				_nf_action->rise_tamper	= _nf_action->cb_rise_tamper;
#endif
				_nf_action->rise_pos   |= _nf_action->cb_curr_pos;
				_nf_action->rise_dva_idz   |= _nf_action->cb_curr_dva_idz;
				_nf_action->rise_dva_ipz   |= _nf_action->cb_curr_dva_ipz;
				/*DVA OBJ CNT*/
				_nf_action->rise_dva_obj_cnt   |= _nf_action->cb_curr_dva_obj_cnt;
			}

#if defined(ENABLE_SENSOR_IPCAM)
			_nf_action->cb_rise_alarm_dvr = 0;
			_nf_action->cb_rise_alarm_ipcam = 0;
#else
			_nf_action->cb_rise_alarm = 0;
#endif
			_nf_action->cb_rise_vloss = 0;
			_nf_action->cb_rise_motion = 0;
			_nf_action->cb_rise_hdd = 0;
			_nf_action->cb_rise_sysdb = 0;

			_nf_action->cb_rise_rec_panic = 0;
			_nf_action->cb_rise_rec_alarm = 0;
			_nf_action->cb_rise_rec_motion = 0;

			_nf_action->cb_rise_net			= 0;
			_nf_action->cb_rise_system		= 0;
#if defined(ENABLE_EVENT_TAMPER)
			_nf_action->cb_rise_tamper	= 0;
#endif
#ifdef	SUPPORT_VCA_CAMERA
			memset(_nf_action->cb_rise_vca, 0x0, sizeof(_nf_action->cb_rise_vca));
#endif	/* SUPPORT_VCA_CAMERA */
			memset(_nf_action->cb_rise_ai, 0x0, sizeof(_nf_action->cb_rise_ai));
			/** IMSI AI FR LPR **/
			//memset(_nf_action->cb_ai_fr_data, 0x0, sizeof(ai_fr_event_t) * NUM_ACTIVE_CH);
			//memset(_nf_action->cb_ai_lpr_data, 0x0, sizeof(ai_lpr_event_t) * NUM_ACTIVE_CH);
			
			_nf_action->cb_rise_pos = 0;
			_nf_action->cb_rise_dva_idz = 0;
			_nf_action->cb_rise_dva_ipz = 0;
			_nf_action->cb_rise_dva_obj_cnt = 0;

			memset(&_nf_action->cb_meta_dva_idz, 0x0, sizeof(DVA_META));
			memset(&_nf_action->cb_meta_dva_ipz, 0x0, sizeof(DVA_META));
			/*DVA OBJ CNT*/
			memset(&_nf_action->cb_meta_dva_obj_cnt, 0x0, sizeof(DVA_META));
			/** DVA Event **/
			memset(&_nf_action->cb_rise_dva_event, 0x00,(sizeof(int)*NUM_ACTIVE_CH*EMAIL_EVENT_TYPE4_NR));
			memset(&_nf_action->cb_curr_dva_event, 0x00,(sizeof(int)*NUM_ACTIVE_CH*EMAIL_EVENT_TYPE4_NR));
		}
		NF_OBJECT_UNLOCK( _nf_action );

		_nf_action_motion_interval_chk();
#if defined(ENABLE_EVENT_TAMPER)
		_nf_action_tamper_interval_chk();
#endif
#ifdef	SUPPORT_VCA_CAMERA
		_nf_action_vca_status_check();
#endif	/* SUPPORT_VCA_CAMERA */

		/** ACTION START **/
		_relay_action();
		_buzzer_action();
#if defined(ENABLE_ACTION_PTZ_PRESET)
		_preset_action();
#endif
#if defined(ENABLE_ACTION_MOBILE)
		_nf_action_mobile_action();
#endif
#if defined(ENABLE_ACTION_PUSH)
		if( nf_sysdb_get_bool("sys.sequrinet.factory_enabled") ) {
			_nf_action_push_action();
		}
#endif
		
		_email_action();

#ifdef ENABLE_ACTION_FTP_SEND
		_ftp_action();
#endif
		/** ACTION END **/
		_pos_reset(0);
#ifdef ENABLE_WATCHDOG
		nf_watchdog_kick( NF_WATCHDOG_MEMBER_ACTION );
#endif
		if(db_tmp)
		{
			guint64 alarm_in=0;

			_nf_action_alarm_in_init_from_reload();		// Patch For N/C <-> N/O buzzer Beep Bug
			alarm_in=_nf_action_reload_alarm_in();

			NF_OBJECT_LOCK( _nf_action );
#if defined(ENABLE_SENSOR_IPCAM)
			_nf_action->cb_curr_alarm_dvr = (alarm_in >> NUM_ALARM_IPCAM);
			_nf_action->cb_rise_alarm_dvr = (alarm_in >> NUM_ALARM_IPCAM);
			
			_nf_action->cb_curr_alarm_ipcam = (alarm_in & ALARM_IPCAM_MASK);
			_nf_action->cb_rise_alarm_ipcam = (alarm_in & ALARM_IPCAM_MASK);
#else
			_nf_action->cb_curr_alarmr=alarm_in;
			_nf_action->cb_rise_alarmr=alarm_in;
#endif
			NF_OBJECT_UNLOCK( _nf_action );

			_nf_action->sysdb_reload = FALSE;
		}

		g_usleep( 1000*100 );
	}

	g_message("%s end", __FUNCTION__);
}

	static void
_nf_action_motion_interval_chk(void)
{
	int active_ch=0;
	GTimeVal        curr_timeval;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	NF_OBJECT_LOCK( _nf_action );

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		MOTION_DATA *mdata = &_nf_action->motion_data[active_ch];
		MOTION_STATE *mstate = &_nf_action->motion_state[active_ch];

#if 0
		g_message("%s Ch[%d] Ignore Interval[%d]", __FUNCTION__, active_ch, mdata->ignore_interval);
#endif

		if(mstate->is_motion == FALSE)
		{
			if(_nf_action->curr_motion & (1<<active_ch) &&
					_nf_action->rise_motion & (1<<active_ch))
			{
				mstate->is_motion = TRUE;
				mstate->is_lasting = TRUE;
				mstate->ignore_sec = curr_timeval.tv_sec + mdata->ignore_interval;

				//	_nf_action->rise_motion |= (1<<active_ch);
#if 0
				g_message("%s Motion Occur!!!! CH[%d]", __FUNCTION__, active_ch);
#endif
			}
		}
		else
		{
			
			if(mstate->ignore_sec < curr_timeval.tv_sec)
			{
				if(mstate->is_lasting)
				{
					if( !((_nf_action->curr_motion >> active_ch) & 0x1) )
					{
						mstate->is_motion = FALSE;
						mstate->is_lasting = FALSE;
#if 0
						g_message("%s is_lasting False!!!!! curr[0x%08x] [0x%08x]",
								__FUNCTION__, _nf_action->curr_motion, (_nf_action->curr_motion >> active_ch) );
#endif
					}
#if 0
					else
						g_message("%s is_lasting Continue!!!!!", __FUNCTION__);
#endif
				}
				else
					mstate->is_motion = FALSE;
			}
			else
			{
				/** For Transparent **/
				/*
				if(_nf_action->curr_motion & (1<<active_ch) &&
						_nf_action->rise_motion & (1<<active_ch))
				{
					_nf_action->curr_motion &= ~(1<<active_ch);
					_nf_action->cb_curr_motion &= ~(1<<active_ch);
				}
				*/
				_nf_action->rise_motion &= ~(1<<active_ch);
				
				if(mstate->is_lasting == FALSE)
					_nf_action->curr_motion &= ~(1<<active_ch);
					
				if( !((_nf_action->curr_motion >> active_ch) & 0x1) )
				{
					mstate->is_lasting = FALSE;
#if 0
					g_message("%s is_lasting False!!!!! curr[0x%08x] [0x%08x]",
							__FUNCTION__, _nf_action->curr_motion, (_nf_action->curr_motion >> active_ch) );
#endif
				}
			}
		}
	}

	NF_OBJECT_UNLOCK( _nf_action );
}

#if defined(ENABLE_EVENT_TAMPER)
static void _nf_action_tamper_interval_chk(void)
{
	int active_ch=0;
	GTimeVal        curr_timeval;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	NF_OBJECT_LOCK( _nf_action );

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		TAMPER_DATA *tdata = &_nf_action->tamper_data[active_ch];
		TAMPER_STATE *tstate = &_nf_action->tamper_state[active_ch];

#if 0
		g_message("%s Ch[%d] Ignore Interval[%d]", __FUNCTION__, active_ch, tdata->ignore_interval);
#endif

		if(tstate->is_tamper == FALSE)
		{
			if(_nf_action->curr_tamper & (1<<active_ch) &&
					_nf_action->rise_tamper & (1<<active_ch))
			{
				tstate->is_tamper = TRUE;
				tstate->is_lasting = TRUE;
				tstate->ignore_sec = curr_timeval.tv_sec + tdata->ignore_interval;

				//  _nf_action->rise_tamper |= (1<<active_ch);
#if 0
				g_message("%s Tamper Occur!!!! CH[%d]", __FUNCTION__, active_ch);
#endif
			}
		}
		else
		{
			if(tstate->ignore_sec < curr_timeval.tv_sec)
			{
				if(tstate->is_lasting)
				{
					if( !((_nf_action->curr_tamper >> active_ch) & 0x1) )
					{
						tstate->is_tamper = FALSE;
						tstate->is_lasting = FALSE;
#if 0
						g_message("%s is_lasting False!!!!! curr[0x%08x] [0x%08x]",
								__FUNCTION__, _nf_action->curr_tamper, (_nf_action->curr_tamper >> active_ch) );
#endif
					}
#if 0
					else
						g_message("%s is_lasting Continue!!!!!", __FUNCTION__);
#endif
				}
				else
					tstate->is_tamper = FALSE;
			}
			else
			{
				/** For Transparent **/
				if(_nf_action->curr_tamper & (1<<active_ch) &&
						_nf_action->rise_tamper & (1<<active_ch))
				{
					_nf_action->curr_tamper &= ~(1<<active_ch);
					_nf_action->cb_curr_tamper &= ~(1<<active_ch);
				}

				_nf_action->rise_tamper &= ~(1<<active_ch);

				if( !((_nf_action->curr_tamper >> active_ch) & 0x1) )
				{
					tstate->is_lasting = FALSE;
#if 0
					g_message("%s is_lasting False!!!!! curr[0x%08x] [0x%08x]",
							__FUNCTION__, _nf_action->curr_tamper, (_nf_action->curr_tamper >> active_ch) );
#endif
				}
			}
		}
	}

	NF_OBJECT_UNLOCK( _nf_action );
}
#endif

#ifdef	SUPPORT_VCA_CAMERA
/**
 * @remark  Ths VCA does not have state information (like high/low, on/off)
 *  or 'falling' events. Therefore we have to generate pseudo 'falling' events
 *  by checking the dwell time of each zone to clear _nf_action->cb_curr_vca[]
 *  bits for the 'TRANSPARENT' mode. This function will do it.
 *  Currently, we fix the dwell time to 5 seconds for all zones. So, the
 *  transparent mode is equivalent to the latched mode with 5 seconds.
 *  (Actually, there is no 'TRANSPARENT concept' in the VCA events.)
 */
static void _nf_action_vca_status_check(void)
{
#if 0		// 20131113 Blocked By pakkhman
#define	VCA_DWELL_TIME		5
	GTimeVal curr_timeval;
	int ch, type, elem;
	guint *curr, *rise;
	VCA_STATE *vstate;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	NF_OBJECT_LOCK(_nf_action);

	curr = _nf_action->curr_vca[0];
	rise = _nf_action->rise_vca[0];
	vstate = _nf_action->vca_state[0][0];
	for (ch = 0; ch < NUM_ACTIVE_CH; ch++) {
		for (type = 0; type < 2; type++, curr++, rise++) {
			for (elem = 0; elem < VCA_MAX_ELEMS; elem++, vstate++) {
				if ( *curr & (1 << elem) ) {
					if ( *rise & (1 << elem) )
						vstate->dwell_out_sec = curr_timeval.tv_sec +
							VCA_DWELL_TIME;
					else if ( vstate->dwell_out_sec < curr_timeval.tv_sec )
						*curr &= ~(1 << elem);
				}
			}
		}
	}

	NF_OBJECT_UNLOCK(_nf_action);
#endif
}
#endif	/* SUPPORT_VCA_CAMERA */

/**
  @brief				action 초기??  @return	gboolean	%TRUE on success, %FALSE if an error occurred
  */
	gboolean
nf_action_init(int wait)
{
	gulong cb_handle = 0;
	gboolean ret = TRUE;
	gint fd=0;

	g_return_val_if_fail (_nf_action == NULL, FALSE);

	printf("\033[0;35m %s Enter \033[0;39m\n", __FUNCTION__);
	//printf("\033[0;35m %s sizeof _nf_action %d \033[0;39m\n", __FUNCTION__,sizeof(struct _NfAction));

	_nf_action = g_object_new ( NF_TYPE_ACTION , NULL);

	nf_debug_category_add( "action", _DEBUG_ACTION_str, _DEBUG_ACTION_log, DEBUG_ACTION_IDX_NR);

	_nf_action->vendor=nf_sysman_get_vendor_from_db();

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A) {
			#if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
				_nf_action_num_nvr_alarm = 16;
			#elif defined(_IPX_0824M4) || defined(_IPX_0824M4E) || defined(_IPX_0824P4E)
				_nf_action_num_nvr_alarm = 8;
			#elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
				_nf_action_num_nvr_alarm = 16;			
			#else
				_nf_action_num_nvr_alarm = 4;
			#endif
		} else {
			#if defined(_IPX_1648M4)|| defined(_IPX_0824M4)	|| defined(_IPX_1648M4E) || defined(_IPX_0824M4E)
				_nf_action_num_nvr_alarm = 2;
			#elif defined(_IPX_0824P4E)
				_nf_action_num_nvr_alarm = 8;			
			#elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)	 || defined(_IPX_1648M4E)
				_nf_action_num_nvr_alarm = 16; 
			#endif
		}

		_nf_action_num_alarm = _nf_action_num_nvr_alarm + NUM_ALARM_IPCAM;

		_nf_action->cb_alarm_timestamp = g_malloc0( sizeof(guint) * _nf_action_num_alarm );
		g_assert(_nf_action->cb_alarm_timestamp);
		_nf_action->alarm_timestamp = g_malloc0( sizeof(guint) * _nf_action_num_alarm );
		g_assert(_nf_action->alarm_timestamp);

		printf("[%s] num_nvr_alarm = 0x%x num_alarm = 0x%x\n",
				__FUNCTION__, _nf_action_num_nvr_alarm, _nf_action_num_alarm);
	#else
		_nf_action->cb_alarm_timestamp = g_malloc0( sizeof(guint) * NUM_ALARM );
		g_assert(_nf_action->cb_alarm_timestamp);
		_nf_action->alarm_timestamp = g_malloc0( sizeof(guint) * NUM_ALARM );
		g_assert(_nf_action->alarm_timestamp);
	#endif

	_email_init();

#ifdef ENABLE_ACTION_FTP_SEND
	_ftp_init();
#endif

	_pos_reset(1);
#if defined(ENABLE_ACTION_MOBILE)
	_nf_action_mobile_init();
#endif
#if defined(ENABLE_ACTION_PUSH)
	_nf_action_push_init();
#endif

	_nf_action_load_relay_data();
	_nf_action_load_email_data();
#ifdef ENABLE_ACTION_FTP_SEND
	_nf_action_load_ftp_data();
#endif
	_nf_action_load_buzzer_data();
	_nf_action_load_sensor_data();
	_nf_action_load_motion_data();
	_nf_action_init_motion_status();
	_nf_action_load_vloss_data();
	_nf_action_load_system_disk_data();
	_nf_action_load_system_record_data();
	_nf_action_load_system_sys_data();
	_nf_action_load_system_net_data();
#if defined(ENABLE_EVENT_TAMPER)
	_nf_action_load_tamper_data();
#endif
#ifdef	SUPPORT_VCA_CAMERA
	_nf_action_load_vca_data();
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	_nf_action_load_ai_data();
	
#if defined(ENABLE_ACTION_MOBILE)
	_nf_action_load_mobile_data();
#endif
#if defined(ENABLE_ACTION_PUSH)
	_nf_action_load_push_data();
#endif
	_nf_action_load_pos_data();
	_nf_action_load_dva_data();

	fd = nf_dev_open_board_pp();
	if(fd < 0)
	{
		g_warning("%s nf_dev_open_board_pp Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
	}else{
		g_message("%s  nf_dev_open_board_pp fd[%d]", __FUNCTION__,  fd);
		_buzzer_init();
	}

	fd = nf_dev_open_relay();
	if(fd < 0)
	{
		g_warning("%s nf_dev_open_relay Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
	}else{
		g_message("%s  nf_dev_open_relay fd[%d]", __FUNCTION__,  fd);
		//_relay_init();
		_relay_init_from_reload();
	}

	cb_handle= nf_notify_connect_cb( "sensor", _action_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_ARI_PANIC)
	cb_handle= nf_notify_connect_cb( "sensor", _action_ari_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

	cb_handle= nf_notify_connect_cb( "motion", _action_cb_func , (gpointer)PROP_MOTION );
	g_message("%s motion connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "vloss", _action_cb_func , (gpointer)PROP_VLOSS);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_EVENT_TAMPER)
	cb_handle= nf_notify_connect_cb( "tamper", _action_cb_func , (gpointer)PROP_TAMPER);
	g_message("%s tamper connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

	cb_handle= nf_notify_connect_cb( "sysdb_change", _action_sysdb_change_cb_func , (gpointer)PROP_SETUP_CHG);
	g_message("%s sysdb change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	// disk event
	cb_handle= nf_notify_connect_cb( "disk_overwr", _action_disk_cb_func, (gpointer)PROP_DISK_OVERWR);
	g_message("%s disk overwr connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_smart", _action_disk_cb_func, (gpointer)PROP_DISK_SMART);
	g_message("%s disk nodisk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_nodisk", _action_disk_cb_func, (gpointer)PROP_DISK_NODISK);
	g_message("%s disk nodisk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_write_fail", _action_disk_cb_func, (gpointer)PROP_DISK_WRFAIL);
	g_message("%s disk exhaust connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_exhaust", _action_disk_cb_func, (gpointer)PROP_DISK_EXHAUST);
	g_message("%s disk exhaust connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_smart_reqchk", _action_disk_cb_func, (gpointer)PROP_DISK_SMART_REQCHK);
	g_message("%s disk smart_reqchk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_full", _action_disk_cb_func, (gpointer)PROP_DISK_FULL);
	g_message("%s disk disk_full connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_usage", _action_disk_usage_cb_func, NULL);
	g_message("%s disk usage connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	// system event
	cb_handle= nf_notify_connect_cb( "sys_booting", _action_system_cb_func , (gpointer)PROP_SYS_BOOTING);
	g_message("%s system sys_booting connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "dvr_login_fail", _action_system_cb_func , (gpointer)PROP_SYS_LOGON_FAIL);
	g_message("%s system dvr_login_fail connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_fan", _action_system_cb_func , (gpointer)PROP_SYS_FAN);
	g_message("%s system sys_fan connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_temperature", _action_system_cb_func , (gpointer)PROP_SYS_TEMPERATURE);
	g_message("%s system sys_temperature connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_POE_CHECK)
	cb_handle= nf_notify_connect_cb( "sys_poe_status", _action_system_cb_func , (gpointer)PROP_SYS_POE);
	g_message("%s system sys_poe_status connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_poe_status_hub", _action_system_cb_func , (gpointer)PROP_SYS_POE_HUB);
	g_message("%s system sys_poe_status_hub connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

	// network event
	cb_handle= nf_notify_connect_cb( "net_login_fail", _action_net_cb_func ,(gpointer)PROP_NET_LOGIN_FAIL);
	g_message("%s net net_login_fail connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "net_ddns_status", _action_net_cb_func ,(gpointer)PROP_NET_DDNS_FAIL);
	g_message("%s net net_ddns_status connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "net_wan_status", _action_net_cb_func ,(gpointer)PROP_NET_TROUBLE);
	g_message("%s net net_wan_status connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "set_ip_conflict", _action_net_cb_func ,(gpointer)PROP_NET_SET_IP_CONFLICT);
	g_message("%s net net_ip_conflict connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "cam_ip_conflict", _action_net_cb_func ,(gpointer)PROP_NET_CAM_IP_CONFLICT);
	g_message("%s net net_ip_conflict connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb("ai_keep_alive", _action_net_cb_func, (gpointer)PROP_NET_TROUBLE_AI);
	g_message("%s net ai_keep_alive event connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);
	
	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _action_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "time_change", _action_time_change_reload_cb_func, NULL);
	g_message("%s reload time_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);


	cb_handle= nf_notify_connect_cb( "analog_rec", _action_record_cb_func , (gpointer)0);
	g_message("%s analog_rec connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "dva_event", _action_dva_cb_func , (gpointer)0);
	g_message("%s dva_event connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_FAN_FAIL_CHECK)
	cb_handle= nf_notify_connect_cb( "sys_fan", _action_sysfan_cb_func , NULL);
	g_message("%s sys_fan connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

	_nf_action->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );

#ifdef DEBUG_ACTION_KEYLED
	fd = nf_dev_open_keypad();
	nf_dev_keypad_dev_enable();
	g_message("%s  nf_device_open_keypad fd[%d]", __FUNCTION__,  fd);
#endif

#if defined(USE_DEV_GENNUM)
	cb_handle= nf_notify_connect_cb( "std_type", _action_std_type_cb_func , (gpointer)0);
	g_message("%s std_type connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

#ifdef	SUPPORT_VCA_CAMERA
	cb_handle= nf_notify_connect_cb("vca_event", _action_vca_cb_func, (gpointer)PROP_VCA);
	g_message("%s VCA connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);
#endif	/* SUPPORT_VCA_CAMERA */

	cb_handle= nf_notify_connect_cb( "pos_text_event", _action_pos_cb_func , NULL);
	g_message("%s POS connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
	
	/** IMSI AI BOX **/
	cb_handle= nf_notify_connect_cb("ai_event", _action_ai_cb_func, (gpointer)PROP_AI);
	g_message("%s AI connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	
	cb_handle= nf_notify_connect_cb("ai_fr_event", _action_ai_fr_cb_func, (gpointer)PROP_AI_FR);
	g_message("%s AI connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);
	
	cb_handle= nf_notify_connect_cb("ai_lpr_event", _action_ai_lpr_cb_func, (gpointer)PROP_AI_LPR);
	g_message("%s AI connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb("ai_generic_event", _action_ai_generic_cb_func, (gpointer)PROP_AI);
	g_message("%s AI generic event connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	if( wait )
	{
		while( _nf_action->init_done != 1)
			g_usleep(10*1000);
	}

	return ret;
}

#if defined(ENABLE_ARI_PANIC)
/**
  @brief				ari panic callback function (sensor)
  */
static void _action_ari_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint prop_id = (guint)data;
	guint new_val=0, i=0;

	g_return_if_fail(pinfo != NULL);

	g_assert( prop_id < LAST_PROP );

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB]
			|| _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB_SIMPLE] )
		g_message("%s prop_id[%d] [%ld.%06ld] type[%d] [0x%02x]", __FUNCTION__,
				prop_id,
				pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
				pinfo->type, pinfo->d.params[3] );
#endif
	new_val	= pinfo->d.params[3];

#if 0
	g_message("%s new_val [0x%08x]\n", __FUNCTION__, new_val);
#endif

	if(prop_id == PROP_SENSOR){

		guint ari_val=0, panic_val=0;
/*
#if defined(ENABLE_SENSOR_IPCAM)
		ari_val = ( new_val & 0x40000 );
#else
		ari_val = ( new_val & 0x10000 );
#endif*/
		ari_val = ( new_val & 0x1 );

		if(_nf_action->vendor == VENDOR_VIDECON || _nf_action->vendor == VENDOR_CBC)
		{
			EMAIL_DATA *e_data = &_nf_action->email_data;
			if(e_data->al_switch_port == ARI_PANIC_LOCATION)
				g_message("%s Videcon ARI Disarm Enabled!!", __FUNCTION__);
			else
			{
				if(ari_val && !nf_sysman_qcmode_is_enable()) {
					_relay_webra_onoff = 0;
					_set_relay_force_off();
					for(i=0; i<NUM_RELAY; i++)
					{
						RELAY_STATE	*rstate = &_nf_action->relay_state[i];
						rstate->onoff = OFF;
						rstate->dwell_in_sec = 0;
						rstate->dwell_out_sec = 0;
					}
				}
				else{
					nf_dev_relay_ari_off();
				}
			}
		}
		else
		{
			if(ari_val && !nf_sysman_qcmode_is_enable() ) {
				_relay_webra_onoff = 0;
				_set_relay_force_off();
				for(i=0; i<NUM_RELAY; i++)
				{
					RELAY_STATE	*rstate = &_nf_action->relay_state[i];
					rstate->onoff = OFF;
					rstate->dwell_in_sec = 0;
					rstate->dwell_out_sec = 0;
				}
			}
			else{
				nf_dev_relay_ari_off();
			}
		}
/*
#if defined(ENABLE_SENSOR_IPCAM)
		panic_val = ( new_val & 0x80000 );
#else
		panic_val = ( new_val & 0x20000 );
#endif*/
		panic_val = ( new_val & 0x2 );

		if(panic_val && !nf_sysman_qcmode_is_enable()) {
			g_message("%s panic toggle", __FUNCTION__);
			nf_panic_record_toggle(NULL);
		}
	}else{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);
		return;
	}
}
#endif

static void _set_relay_enable(void)
{
	nf_dev_relay_enable();
}

#if defined(ENABLE_ARI_PANIC)
static void _set_relay_force_off()
{
	nf_dev_relay_ari_on();
}
#endif

	static void
_action_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_action != NULL);

	_nf_action->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );

	if(pinfo->d.params[0] == NF_SYSDB_CATE_ACT
			|| pinfo->d.params[0] == NF_SYSDB_CATE_DISK
		#if defined(ENABLE_ACTION_PTZ_PRESET)
			|| pinfo->d.params[0] == NF_SYSDB_CATE_CAM
		#endif
			|| pinfo->d.params[0] == NF_SYSDB_CATE_ALARM ){	// 2010-05-04 ?�후 5:16:51

		_nf_action->sysdb_reload = 1;
	}

	/** 20111114 **/
	if(pinfo->d.params[0] == NF_SYSDB_CATE_ACT)
	{
		g_message("%s Init Motion Status!!!!!!!!!", __FUNCTION__);
		_nf_action_init_motion_status();
	}
}

	static void
_action_time_change_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_action != NULL);

	_nf_action->sysdb_reload = 1;
}

	static void
_action_sysdb_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_action != NULL);

	NF_OBJECT_LOCK( _nf_action );
	++_nf_action->cb_rise_sysdb;
	NF_OBJECT_UNLOCK( _nf_action );

}

	static void
_action_disk_usage_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint used=0, total=0;
	guint exhaust_threshold=0, exhaust_threshold_curr=0;
	static gboolean is_rise=FALSE;

	g_return_if_fail(pinfo->d.params[1] != 0);

	used=pinfo->d.params[0];
	total=pinfo->d.params[1];

	exhaust_threshold=_nf_action->disk_ddata.exhaust_threshold;
	exhaust_threshold_curr=(guint)(100 * ((float)used/(float)total));

#if 0
	g_message("%s threshold[%d] threshold_curr[%d] used[%d] total[%d]",
			__FUNCTION__, exhaust_threshold, exhaust_threshold_curr, used, total);
#endif
	if(exhaust_threshold_curr >= exhaust_threshold)
	{
		if(!is_rise)
		{

			// BPM-708
			guint is_overwrite = nf_sysdb_get_uint("disk.write_mode");

			g_warning("%s is_overwrite[%d]",__FUNCTION__, is_overwrite);
			if( is_overwrite == 1 )
			{
				g_warning("%s is_overwrite[%d] disk_exhaust notify fire skip!!",__FUNCTION__, is_overwrite);
			}else{
				nf_notify_fire_params("disk_exhaust", 1, 0, 0, 0);
			}

			is_rise=TRUE;
		}
	}
	else
	{
		nf_notify_fire_params("disk_exhaust", 0, 0, 0, 0);
		is_rise=FALSE;
	}
}

	static void
_action_disk_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint prop_id = (guint)data;

	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_action != NULL);
	g_assert( prop_id < LAST_PROP );

	NF_OBJECT_LOCK( _nf_action );

	if(prop_id == PROP_DISK_OVERWR)
	{
		if(pinfo->d.params[0] == 1)
		{
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_OVER);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_OVER);
		}else{
			_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_OVER);
		}
	}
	else if(prop_id == PROP_DISK_SMART)
	{
		if(pinfo->d.params[0] == 1          // smart
				|| pinfo->d.params[0] == 2      // mirror fail
				|| pinfo->d.params[0] == 4 )    // boot disk fail
		{
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART);
		}else{
			_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_SMART);
		}
	}
	else if(prop_id == PROP_DISK_NODISK)
	{
		if(pinfo->d.params[0] == 1)
		{
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_NODISK);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_NODISK);
		}else{
			_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_NODISK);
		}
	}
	else if(prop_id == PROP_DISK_WRFAIL)
	{
		_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_WRFAIL);
		//		_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_WRFAIL);
	}
	else if(prop_id == PROP_DISK_FULL)
	{
		if(pinfo->d.params[0] == 1)
		{
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_FULL);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_FULL);

		}else{
			_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_FULL);
		}
	}
	else if(prop_id == PROP_DISK_EXHAUST)
	{
		if(pinfo->d.params[0] == 1)
		{
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_EXHU);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_EXHU);

		}else{
			_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_EXHU);
		}
	}
	else if(prop_id == PROP_DISK_SMART_REQCHK)
	{
		if(pinfo->d.params[0] == 1)
		{
#if 0
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART_REQCHK);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART_REQCHK);
#else
			_nf_action->cb_rise_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART);
			_nf_action->cb_curr_hdd |= (1 << NF_ACTION_HDD_EVENT_SMART);
#endif
		}else{
			#if 0
				_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_SMART_REQCHK);
			#else
				_nf_action->cb_curr_hdd &= ~(1 << NF_ACTION_HDD_EVENT_SMART);
			#endif
		}
	}
	else
	{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);;
		NF_OBJECT_UNLOCK( _nf_action );
		return ;
	}
	if(_nf_action->cb_rise_hdd)
		_nf_action->cb_hdd_timestamp = time(NULL);
		
	NF_OBJECT_UNLOCK( _nf_action );
}
#define AI_KEEP_ALIVE_TROUBLE_FLAG	1
#define AI_KEEP_ALIVE_CLEAR_FLAG	2
	static void
_action_net_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static int ddns_fail_curr_cnt = 0;
	guint prop_id = (guint)data;
	static int cam_ip_conflict_mask = 0;

	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_action != NULL);
	g_assert( prop_id < LAST_PROP );

	NF_OBJECT_LOCK( _nf_action );

	if(prop_id == PROP_NET_TROUBLE)
	{
		// if 0 --> init status , if 1 --> normal , if < 0 --> abnormal
		if((gint)pinfo->d.params[0] == 1)
		{
			_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE);
			_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE);
		}else{
			_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_TROUBLE);
		}
	}
	else if(prop_id == PROP_NET_LOGIN_FAIL)
	{
		if(pinfo->d.params[0] == 0x1 )
		{
			_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_LOGON_FAIL);
			_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_LOGON_FAIL);
		}else{
			_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_LOGON_FAIL);
		}
	}
	else if(prop_id == PROP_NET_DDNS_FAIL)
	{
		// if 0 --> init status , if 1 --> normal , if < 0 --> abnormal
		if(((gint)pinfo->d.params[0] == 1))
		{
			g_message("%s Reason --> [%d], %d", __FUNCTION__, pinfo->d.params[1], ddns_fail_curr_cnt);

			ddns_fail_curr_cnt++;
			if(ddns_fail_curr_cnt >= nf_sysdb_get_int("act.sys.net.ddns_fail.cnt"))
			{
				_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_DDNS_FAIL);
				_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_DDNS_FAIL);
				ddns_fail_curr_cnt=0;
			}
		}else{
			_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_DDNS_FAIL);
			ddns_fail_curr_cnt=0;
		}
	}
	else if(prop_id == PROP_NET_SET_IP_CONFLICT || prop_id == PROP_NET_CAM_IP_CONFLICT)
	{
		if(prop_id == PROP_NET_CAM_IP_CONFLICT)
		{
			if(pinfo->d.params[0] == 0x1)
				cam_ip_conflict_mask |= (1 << pinfo->d.params[1]);
			else
				cam_ip_conflict_mask &= ~(1 << pinfo->d.params[1]);
		}

		if(pinfo->d.params[0] == 0x1 || cam_ip_conflict_mask)
		{
			_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_IP_CONFLICT);
			_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_IP_CONFLICT);
		}else{
			_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_IP_CONFLICT);
		}
	}
	else if(prop_id == PROP_NET_TROUBLE_AI)
	{
		#if 1
			int n_ch = 0, n_prev_status = 0, n_curr_status = 0, n_tmp_keep_alive_status = 0;
			int n_loop_ch = 0, n_tmp_val = 0;
			int trouble_flag = 0;

			n_ch = pinfo->d.params[0];
			n_prev_status = pinfo->d.params[1];
			n_curr_status = pinfo->d.params[2];

			
			#if DEBUG_AI_KEEP_ALIVE
				printf("\033[0;36m %s DEBUG_AI_KEEP_ALIVE CH[%d] PREV PARAM[%d] CURR PARAM[%d]\033[0;39m\n", __FUNCTION__, n_ch, n_prev_status, n_curr_status);
			#endif
			
			for(n_loop_ch = 0; n_loop_ch < NUM_ACTIVE_CH; n_loop_ch++)
			{
				#if DEBUG_AI_KEEP_ALIVE
					//printf("\033[0;34m %s DEBUG_AI_KEEP_ALIVE CH[%d] PREV STATE[%d] \033[0;39m\n", __FUNCTION__, n_loop_ch, _nf_action->ai_keep_alive_state[n_loop_ch]);
				#endif
				if(n_loop_ch == n_ch)
					continue;
				if((NF_AIBOX_CONN_FAILED <= _nf_action->ai_keep_alive_state[n_loop_ch]) && (NF_AIBOX_STREAM_CONN_FAILED >= _nf_action->ai_keep_alive_state[n_loop_ch]))
				{
					#if DEBUG_AI_KEEP_ALIVE
						printf("\033[0;35m %s DEBUG_AI_KEEP_ALIVE CH[%d] TROUBLE \033[0;39m\n", __FUNCTION__, n_loop_ch);
					#endif
					trouble_flag = 1;				
				}
				#if DEBUG_AI_KEEP_ALIVE
					//printf("\033[0;35m %s DEBUG_AI_KEEP_ALIVE CH[%d] CURR STATE[%d] \033[0;39m\n", __FUNCTION__, n_loop_ch, _nf_action->ai_keep_alive_state[n_loop_ch]);
				#endif
			}
			
			if(((NF_AIBOX_CONFIG_OFF <= n_prev_status) && (NF_AIBOX_CONN_SUCCESS >= n_prev_status)) \
			&& ((NF_AIBOX_CONN_FAILED <= n_curr_status) && (NF_AIBOX_STREAM_CONN_FAILED >= n_curr_status)))
			{
				if(trouble_flag == 0)
				{
					_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
					_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
					#if DEBUG_AI_KEEP_ALIVE
						printf("\033[0;37m %s DEBUG_AI_KEEP_ALIVE RISE \033[0;39m\n", __FUNCTION__);
					#endif
					{
						GTimeVal tv;
						GValue ret_value = {0,};
						char tmp_mac[128];
						char tmp_text[200];
						
						gettimeofday(&tv, NULL);
							
						if(nf_sysdb_get_key1("cam.ai_box.A%d.mac", n_ch, &ret_value, NULL))
						{
							strncpy(tmp_mac, g_value_get_string(&ret_value), sizeof(tmp_mac) - 1);
							tmp_mac[sizeof(tmp_mac) - 1] = 0;
							g_value_unset(&ret_value);
							snprintf(tmp_text, sizeof(tmp_text), "%d,%s", n_ch, tmp_mac);
						}
						else{
							snprintf(tmp_text, sizeof(tmp_text), "%d", n_ch);
						}
						
						nf_eventlog_put_param(&tv, LT_NETWORK_EVENT, 1, LP2_NETWORK_EVENT_AIBOX_DISCONNECTED, tmp_text);
					}
				}
				else
				{
					#if DEBUG_AI_KEEP_ALIVE
						printf("\033[0;36m %s DEBUG_AI_KEEP_ALIVE RISE SKIP\033[0;39m\n", __FUNCTION__);
					#endif
				}
			}
			else if(((NF_AIBOX_CONFIG_OFF <= n_curr_status) && (NF_AIBOX_CONN_SUCCESS >= n_curr_status)) \
			&& ((NF_AIBOX_CONN_FAILED <= n_prev_status) && (NF_AIBOX_STREAM_CONN_FAILED >= n_prev_status)))
			{
				if(trouble_flag == 0)
				{
					#if DEBUG_AI_KEEP_ALIVE
						printf("\033[0;35m %s DEBUG_AI_KEEP_ALIVE CLEAR \033[0;39m\n", __FUNCTION__);
					#endif
					_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
				}
				else
				{
					#if DEBUG_AI_KEEP_ALIVE
						printf("\033[0;34m %s DEBUG_AI_KEEP_ALIVE CLEAR SKIP\033[0;39m\n", __FUNCTION__);
					#endif
				}
			}
			_nf_action->ai_keep_alive_state[n_ch] = n_curr_status;
		#elif 0
			int n_ch = 0, n_prev_status = 0, n_curr_status = 0, n_tmp_keep_alive_status = 0;
			int n_loop_ch = 0, n_tmp_val = 0;
			int trouble_flag = 0;
			unsigned int curr_rise_time = 0;

			static unsigned int prev_rise_time = 0;
			
			n_ch = pinfo->d.params[0];
			n_prev_status = pinfo->d.params[1];
			n_curr_status = pinfo->d.params[2];

			if(prev_rise_time == 0)
			{
				prev_rise_time = time(NULL);
				prev_rise_time--;
			}
				
			curr_rise_time = time(NULL);
			
			#if DEBUG_AI_KEEP_ALIVE
				printf("\033[0;36m %s DEBUG_AI_KEEP_ALIVE CH[%d] PREV PARAM[%d] time[%d] CURR PARAM[%d] time[%d]\033[0;39m\n", __FUNCTION__, n_ch, n_prev_status, prev_rise_time, n_curr_status, curr_rise_time);
			#endif

			for(n_loop_ch = 0; n_loop_ch < NUM_ACTIVE_CH; n_loop_ch++)
			{
				#if DEBUG_AI_KEEP_ALIVE
					//printf("\033[0;34m %s DEBUG_AI_KEEP_ALIVE CH[%d] PREV STATE[%d] \033[0;39m\n", __FUNCTION__, n_loop_ch, _nf_action->ai_keep_alive_state[n_loop_ch]);
				#endif				
				_nf_action->ai_keep_alive_state[n_loop_ch] = nf_api_get_aibox_connection_status(n_loop_ch);
				if((NF_AIBOX_CONN_FAILED <= _nf_action->ai_keep_alive_state[n_loop_ch]) && (NF_AIBOX_STREAM_CONN_FAILED >= _nf_action->ai_keep_alive_state[n_loop_ch]))
				{
					trouble_flag = 1;				
				}
				#if DEBUG_AI_KEEP_ALIVE
					//printf("\033[0;35m %s DEBUG_AI_KEEP_ALIVE CH[%d] CURR STATE[%d] \033[0;39m\n", __FUNCTION__, n_loop_ch, _nf_action->ai_keep_alive_state[n_loop_ch]);
				#endif
			}

			if(curr_rise_time - prev_rise_time > 3)
			{
				if(((NF_AIBOX_CONFIG_OFF <= n_prev_status) && (NF_AIBOX_CONN_SUCCESS >= n_prev_status)) \
				&& ((NF_AIBOX_CONN_FAILED <= n_curr_status) && (NF_AIBOX_STREAM_CONN_FAILED >= n_curr_status)))
				{
					_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
					_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
					#if DEBUG_AI_KEEP_ALIVE
						printf("\033[0;37m %s DEBUG_AI_KEEP_ALIVE RISE \033[0;39m\n", __FUNCTION__);
					#endif
					{
						GTimeVal tv;
						GValue ret_value = {0,};
						char tmp_mac[128];
						char tmp_text[200];
						
						gettimeofday(&tv, NULL);
							
						if(nf_sysdb_get_key1("cam.ai_box.A%d.mac", n_ch, &ret_value, NULL))
						{
							strncpy(tmp_mac, g_value_get_string(&ret_value), sizeof(tmp_mac) - 1);
							tmp_mac[sizeof(tmp_mac) - 1] = 0;
							g_value_unset(&ret_value);
							snprintf(tmp_text, sizeof(tmp_text), "%d,%s", n_ch, tmp_mac);
						}
						else{
							snprintf(tmp_text, sizeof(tmp_text), "%d", n_ch);
						}
						
						nf_eventlog_put_param(&tv, LT_NETWORK_EVENT, 1, LP2_NETWORK_EVENT_AIBOX_DISCONNECTED, tmp_text);
					}
				}
				else if(((NF_AIBOX_CONFIG_OFF <= n_curr_status) && (NF_AIBOX_CONN_SUCCESS >= n_curr_status)) \
				&& ((NF_AIBOX_CONN_FAILED <= n_prev_status) && (NF_AIBOX_STREAM_CONN_FAILED >= n_prev_status)))
				{
					if(trouble_flag == 0)
					{
						#if DEBUG_AI_KEEP_ALIVE
							printf("\033[0;37m %s DEBUG_AI_KEEP_ALIVE CLEAR \033[0;39m\n", __FUNCTION__);
						#endif
						_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
					}
				}
				prev_rise_time = curr_rise_time;

			}	
			else
			{
				#if DEBUG_AI_KEEP_ALIVE
					printf("\033[0;37m %s DEBUG_AI_KEEP_ALIVE SKIP \033[0;39m\n", __FUNCTION__);
				#endif
			}
		#elif 0
			unsigned int un_connectedChMask = 0, un_streamChMask = 0, un_riseChMask = 0;
			unsigned int un_troubleChMask = 0;
			int n_ch = 0;

			un_riseChMask = pinfo->d.params[0];
			un_connectedChMask = pinfo->d.params[1];
			un_streamChMask = pinfo->d.params[2];
			
			for(n_ch = 0; n_ch < NUM_ACTIVE_CH; n_ch++)
			{
				if( (un_connectedChMask >> n_ch) & 0x1 )
					_nf_action->ai_keep_alive_state[n_ch] |= ( 0x1 << 1 );
				else
					_nf_action->ai_keep_alive_state[n_ch] &= ~( 0x1 << 1 );

				if( (un_streamChMask >> n_ch) & 0x1 )
					_nf_action->ai_keep_alive_state[n_ch] |= ( 0x1 << 2 );
				else
					_nf_action->ai_keep_alive_state[n_ch] &= ~( 0x1 << 2 );
					

				if(_nf_action->ai_keep_alive_state[n_ch])
				{
					un_troubleChMask |= (1 << n_ch);
				}
				#if DEBUG_AI_KEEP_ALIVE
					printf("\033[0;34m %s DEBUG_AI_KEEP_ALIVE CH[%d] STATE[%d] un_troubleChMask[0x%x]\033[0;39m\n", __FUNCTION__, n_ch, _nf_action->ai_keep_alive_state[n_ch], un_troubleChMask);
				#endif
			}
			
			if(un_troubleChMask)
			{
				_nf_action->cb_rise_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
				_nf_action->cb_curr_net |= (1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
				{
					GTimeVal tv;
					int k;
					
					GValue ret_value = {0,};
					char tmp_mac[128];
					char tmp_text[200];
					
					gettimeofday(&tv, NULL);

					for(k=0;k<NUM_ACTIVE_CH;k++){
						if(un_troubleChMask & (1<<k)){
							
							if(nf_sysdb_get_key1("cam.ai_box.A%d.mac", k, &ret_value, NULL))
							{
								strncpy(tmp_mac, g_value_get_string(&ret_value), sizeof(tmp_mac) - 1);
								tmp_mac[sizeof(tmp_mac) - 1] = 0;
								g_value_unset(&ret_value);
								snprintf(tmp_text, sizeof(tmp_text), "%d,%s", k, tmp_mac);
							}
							else{
								snprintf(tmp_text, sizeof(tmp_text), "%d", k);
							}
							
							nf_eventlog_put_param(&tv, LT_NETWORK_EVENT, 1, LP2_NETWORK_EVENT_AIBOX_DISCONNECTED, tmp_text);
						}
					}
				}
			}
			else
			{
				_nf_action->cb_curr_net &= ~(1 << NF_ACTION_NET_EVENT_TROUBLE_AI_BOX);
			}
			
			#if DEBUG_AI_KEEP_ALIVE
				printf("\033[0;34m %s DEBUG_AI_KEEP_ALIVE riseMask[0x%x] conMask[0x%x] streamMask[0x%x] un_troubleChMask[0x%x]\033[0;39m\n", __FUNCTION__,\
					un_riseChMask, un_connectedChMask, un_streamChMask, un_troubleChMask);
			#endif
		#endif
	}
	else
	{
		NF_OBJECT_UNLOCK( _nf_action );
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);;
		return ;
	}
	if(_nf_action->cb_rise_net)
		_nf_action->cb_net_timestamp = time(NULL);
	NF_OBJECT_UNLOCK( _nf_action );
}

	static void
_action_system_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint prop_id = (guint)data;

	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_action != NULL);
	g_assert( prop_id < LAST_PROP );

	NF_OBJECT_LOCK( _nf_action );

	if(prop_id == PROP_SYS_BOOTING)
	{
		if(pinfo->d.params[0] == 0x1)
		{
			_nf_action->cb_rise_system |= (1 << NF_ACTION_SYSTEM_EVENT_BOOTING);
			_nf_action->cb_curr_system |= (1 << NF_ACTION_SYSTEM_EVENT_BOOTING);
		}else{
			_nf_action->cb_curr_system &= ~(1 << NF_ACTION_SYSTEM_EVENT_BOOTING);
		}
	}
	else if(prop_id == PROP_SYS_LOGON_FAIL)
	{
		if((gint)pinfo->d.params[0] == 0x1)
		{
			_nf_action->cb_rise_system |= (1 << NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL);
			_nf_action->cb_curr_system |= (1 << NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL);
		}else{
			_nf_action->cb_curr_system &= ~(1 << NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL);
		}
	}
#if defined(ENABLE_FAN_FAIL_CHECK)
	else if(prop_id == PROP_SYS_FAN)
	{
		if((pinfo->d.params[0] & (1 << 0)) ||				// cpu
				(pinfo->d.params[0] & (1 << 1)) || 			// sys1
				(pinfo->d.params[0] & (1 << 2))) 			// sys2
		{
			_nf_action->cb_rise_system |= (1 << NF_ACTION_SYSTEM_EVENT_FAN_FAIL);
			_nf_action->cb_curr_system |= (1 << NF_ACTION_SYSTEM_EVENT_FAN_FAIL);
		}else{
			_nf_action->cb_curr_system &= ~(1 << NF_ACTION_SYSTEM_EVENT_FAN_FAIL);
		}
	}
	else if(prop_id == PROP_SYS_TEMPERATURE)
	{
		if((pinfo->d.params[0] & (1 << 0)) ||
				(pinfo->d.params[0] & (1 << 1)))
		{
			_nf_action->cb_rise_system |= (1 << NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL);
			_nf_action->cb_curr_system |= (1 << NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL);
		}else{
			_nf_action->cb_curr_system &= ~(1 << NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL);
		}
	}
#endif
#if defined(ENABLE_POE_CHECK)
	else if((prop_id == PROP_SYS_POE) || (prop_id == PROP_SYS_POE_HUB))
	{
		guint poe_warn_mask=0, poe_power_cut_mask=0, poe_power_total_usage=0;

		poe_warn_mask=pinfo->d.params[1];
		poe_power_cut_mask=pinfo->d.params[2];
		poe_power_total_usage=pinfo->d.params[3];

		if(pinfo->d.params[0] != 0)
		{
			_nf_action->cb_rise_system |= (1 << NF_ACTION_SYSTEM_EVENT_POE);
			_nf_action->cb_curr_system |= (1 << NF_ACTION_SYSTEM_EVENT_POE);

			g_message("%s %s warn_mask[0x%08x] power_cut_mask[0x%08x] poe_power_usage[%d]",
					__FUNCTION__, (prop_id == PROP_SYS_POE) ? "PROP_SYS_POE" : "PROP_SYS_POE_HUB",
					poe_warn_mask, poe_power_cut_mask, poe_power_total_usage);
		}else{
			_nf_action->cb_curr_system &= ~(1 << NF_ACTION_SYSTEM_EVENT_POE);
		}
	}
#endif
	else
	{
		NF_OBJECT_UNLOCK( _nf_action );
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);;
		return ;
	}
	if( _nf_action->cb_rise_system )
		_nf_action->cb_system_timestamp = time(NULL);
	NF_OBJECT_UNLOCK( _nf_action );
}

	static void
_action_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	gint ch=0;

	g_return_if_fail(pinfo != NULL);

	guint prop_id = (guint)data;
	guint new_val, m_new, m_old,ipcam_sensor;
	guint *curr_val, *rise_val;
	gint i;

	gint div_count = _nf_action->email_state.div_count;

	g_assert( prop_id < LAST_PROP );

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB]
			|| _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB_SIMPLE] )
		g_message("%s prop_id[%d] [%ld.%06ld] type[%d] [0x%02x]", __FUNCTION__,
				prop_id,
				pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
				pinfo->type, pinfo->d.params[0] );
#endif

	new_val	= pinfo->d.params[0];
	ipcam_sensor = pinfo->d.params[1];
	if( (old_val[prop_id] == new_val) && (prop_id == PROP_SENSOR) && (old_val[PROP_SENSOR_IPCAM] == ipcam_sensor))
	{
#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
			g_message("%s  same value, skip old[0x%08x] new[0x%08x]",
					__FUNCTION__, old_val[prop_id] , new_val);
#endif
		return;
	}

	if(prop_id == PROP_MOTION ){
		curr_val = &_nf_action->cb_curr_motion;
		rise_val = &_nf_action->cb_rise_motion;
		//printf("\033[0;31m %s [DEBUG_BUZZ_TRANS] MOTION OCCUR \033[0;39m\n", __FUNCTION__);
		ch=NUM_ACTIVE_CH;
	}else if(prop_id == PROP_SENSOR){
#if defined(ENABLE_SENSOR_IPCAM)
		curr_val = &_nf_action->cb_curr_alarm_dvr;
		rise_val = &_nf_action->cb_rise_alarm_dvr;
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			ch = _nf_action_num_alarm;
		#else
			ch = NUM_ALARM_DVR;
		#endif
#else
		curr_val = &_nf_action->cb_curr_alarm;
		rise_val = &_nf_action->cb_rise_alarm;
		ch=NUM_ACTIVE_CH;
#endif

	}else if(prop_id == PROP_VLOSS ){
		curr_val = &_nf_action->cb_curr_vloss;
		rise_val = &_nf_action->cb_rise_vloss;

		ch=NUM_ACTIVE_CH;
#if defined(ENABLE_EVENT_TAMPER)
	}else if(prop_id == PROP_TAMPER ){
		curr_val = &_nf_action->cb_curr_tamper;
		rise_val = &_nf_action->cb_rise_tamper;

		ch=NUM_ACTIVE_CH;
#endif
	}else{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);
		return;
	}

	NF_OBJECT_LOCK( _nf_action );
	for(i=0; i<ch; i++)
	{
		m_new = (new_val >> i) & 1;
		m_old = (old_val[prop_id] >> i) & 1;

		if( m_new != m_old )
		{
#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
				g_message("%s prop_id[%d] ch[%d] [%d]-->[%d] [%d]",  __FUNCTION__,
								prop_id, i,  m_old, m_new, div_count);
#endif
			if(m_new)
			{
				*rise_val |= (1 << i);
				*curr_val |= (1 << i);
				#if defined (DEBUG_BUZZ_TRANS)
					if(prop_id == PROP_MOTION){
						printf("\033[0;31m %s [DEBUG_BUZZ_TRANS] RISE \033[0;39m\n", __FUNCTION__);
						printf("\033[0;31m %s [DEBUG_BUZZ_TRANS] curr_val %d addr %x \033[0;39m\n", __FUNCTION__,*curr_val,curr_val);
					}
				#endif
				if(prop_id == PROP_SENSOR){
					_nf_action->cb_alarm_timestamp[i + NUM_ALARM_IPCAM] = time(NULL);
					#if defined (DEBUG_TIMESTAMP)
						printf("\033[0;36m %s DEBUG_TIMESTAMP PROP_SENSOR\033[0;39m\n", __FUNCTION__);
					#endif
				}
				else if(prop_id == PROP_VLOSS ){
					_nf_action->cb_vloss_timestamp[i] = time(NULL);
					#if defined (DEBUG_TIMESTAMP)
						printf("\033[0;36m %s DEBUG_TIMESTAMP PROP_VLOSS\033[0;39m\n", __FUNCTION__);
					#endif
				}
				#if defined(ENABLE_EVENT_TAMPER)
				else if(prop_id == PROP_TAMPER ){
					_nf_action->cb_tamper_timestamp[i] = time(NULL);
					#if defined (DEBUG_TIMESTAMP)
						printf("\033[0;36m %s DEBUG_TIMESTAMP PROP_TAMPER\033[0;39m\n", __FUNCTION__);
					#endif
				}
				#endif
				else if(prop_id == PROP_MOTION ){
					_nf_action->cb_motion_timestamp[i] = time(NULL);
					#if defined (DEBUG_TIMESTAMP)
						printf("\033[0;36m %s DEBUG_TIMESTAMP PROP_motion\033[0;39m\n", __FUNCTION__);
					#endif
				}
				
#if 0 // send push immediately - chcha
				if(prop_id == PROP_SENSOR)
		          _nf_action->push_send_data->type1[i].rise[prop_id][div_count]++;
				else if(prop_id == PROP_MOTION)
		          _nf_action->push_send_data->type1[i].rise[prop_id][div_count]++;
				else if(prop_id == PROP_VLOSS)
		          _nf_action->push_send_data->type1[i].rise[prop_id][div_count]++;

		        _nf_action->push_state.send_time = 0;
#endif
			}else{
				*curr_val &= ~(1 << i);
				#if defined (DEBUG_BUZZ_TRANS)
					if(prop_id == PROP_MOTION){
						printf("\033[0;31m %s [DEBUG_BUZZ_TRANS] RISE ELSE \033[0;39m\n", __FUNCTION__);
						printf("\033[0;31m %s [DEBUG_BUZZ_TRANS] curr_val %d addr %x \033[0;39m\n", __FUNCTION__,*curr_val,curr_val);
					}
				#endif
			}
		}
	}

	if(prop_id == PROP_SENSOR){
		
		curr_val = &_nf_action->cb_curr_alarm_ipcam;
		rise_val = &_nf_action->cb_rise_alarm_ipcam;
		
		ch = NUM_ALARM_IPCAM;

		for(i=0; i<ch; i++)
		{
			m_new = (ipcam_sensor >> i) & 1;
			m_old = (old_val[PROP_SENSOR_IPCAM] >> i) & 1;

			if( m_new != m_old )
			{
				if(m_new)
				{
					*rise_val |= (1 << i);
					*curr_val |= (1 << i);
					
					_nf_action->cb_alarm_timestamp[i] = time(NULL);
				}else{
					*curr_val &= ~(1 << i);
				}
			}
		}
	}
	
	NF_OBJECT_UNLOCK( _nf_action );

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
		g_message("%s curr[0x%08x] rise[0x%08x]",  __FUNCTION__, *curr_val, *rise_val);
#endif
	old_val[prop_id] = new_val;

	if(prop_id == PROP_SENSOR){
		old_val[PROP_SENSOR_IPCAM] = ipcam_sensor;
	}

	//_nf_action->email_send_data_store(*curr_val);
}

#if defined(ENABLE_FAN_FAIL_CHECK)
static void _action_sysfan_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint fan_fail_mask=0, fan_rpm[3]={0, };

	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}

	fan_fail_mask=pinfo->d.params[0];
	fan_rpm[0]=pinfo->d.params[1];
	fan_rpm[1]=pinfo->d.params[2];
	fan_rpm[2]=pinfo->d.params[3];
#if 0
	g_message("%s mask[0x%08x] cpu_fan_rpm[%d] sys_fan1_rpm[%d] sys_fan2_rpm[%d]",
			__FUNCTION__, fan_fail_mask, fan_rpm[0], fan_rpm[1], fan_rpm[2]);
#endif
}

static void _action_sys_temperature_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint fan_temp_fail_mask=0;
	gint temper_cpu=0, temper_sys=0;

	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}

	fan_temp_fail_mask=pinfo->d.params[0];
	temper_cpu=(gint)pinfo->d.params[1];
	temper_sys=(gint)pinfo->d.params[2];
	g_message("%s mask[0x%08x] cpu_temperature[%d] sys_temperature[%d]",
			__FUNCTION__, fan_temp_fail_mask, temper_cpu, temper_sys);
}
#endif

	static void
_action_record_cb_func( NF_NOTIFY_INFO *pinfo, gpointer data )
{

	gint i;
	gchar *new_val;
	static gchar old_val[NUM_ACTIVE_CH+1] = {0,};

	gint div_count = _nf_action->email_state.div_count;
#if 0
	g_return_val_if_fail(pinfo != NULL, 0);
#else
	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}
#endif
	g_assert( pinfo->type == NF_NOTIFY_CHMAP);

	new_val = pinfo->c.chmap;
	if( !memcmp(old_val, new_val, NUM_ACTIVE_CH) )
	{
		return;
	}

	NF_OBJECT_LOCK( _nf_action );

	_nf_action->cb_curr_rec_panic = 0;
	_nf_action->cb_curr_rec_alarm = 0;
	_nf_action->cb_curr_rec_motion = 0;

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		guint ch_mask = (guint)(1 << i);
		guint ch_mask_rise = (new_val[i] != old_val[i] ) ? ch_mask : 0;

		if( new_val[i] == NF_RECORD_REASON_CHAR_MANUAL ) {
			_nf_action->cb_curr_rec_panic |= ch_mask;

			if( ch_mask_rise )
			{
				_nf_action->cb_rise_rec_panic |= ch_mask_rise;
				_nf_action->cb_rec_panic_timestamp = time(NULL);
			}

		} else if( new_val[i] == NF_RECORD_REASON_CHAR_ALARM ) {
			_nf_action->cb_curr_rec_alarm |= ch_mask;

			if( ch_mask_rise )
				_nf_action->cb_rise_rec_alarm |= ch_mask_rise;

		} else if( new_val[i] == NF_RECORD_REASON_CHAR_MOTION ) {
			_nf_action->cb_curr_rec_motion |= ch_mask;

			if( ch_mask_rise )
				_nf_action->cb_rise_rec_motion |= ch_mask_rise;
		}
	}

	NF_OBJECT_UNLOCK( _nf_action );

	memcpy( old_val, new_val, NUM_ACTIVE_CH );
}
/** DVA Event **/
static void
_action_set_dva_event(guchar ch,guchar dva_type,char *name)
{
	int event;
#if 0
	printf("\033[0;31m %s >>>>> [NAME] : %s <<<<< \033[0;39m\n", __FUNCTION__,name);
#endif
	#if defined(DEBUG_DVA_OBJ_CNT)
	printf("\033[0;33m %s TYPE[%d] NAME[%s]\033[0;39m\n", __FUNCTION__,dva_type,name);
	#endif
	if (dva_type == DVA_INTRUSION_DETECTION)
	{
		for(event = 0; event < NF_ACTION_DVA_IDZ_NR; event++)
		{
			if(strcmp(_ACTION_STR_TABLE_TYPE4[0][EMAIL_EVENT_TYPE4_DVA_IDZ][event], name) == 0)
			{
				_nf_action->cb_rise_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_IDZ] |= (1<<event);
				_nf_action->cb_curr_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_IDZ] |= (1<<event);
			}
		}
	}
	else if (dva_type == DVA_ILLEGAL_PARKING)
	{
		for(event = 0; event < NF_ACTION_DVA_IPZ_NR; event++)
		{
			if(strcmp(_ACTION_STR_TABLE_TYPE4[0][EMAIL_EVENT_TYPE4_DVA_IPZ][event], name) == 0)
			{
				_nf_action->cb_rise_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_IPZ] |= (1<<event);
				_nf_action->cb_curr_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_IPZ] |= (1<<event);
			}
		}
	}
	else if (dva_type == DVA_COUNTER)
	{
		/*DVA OBJ CNT*/
		for(event = 0; event < NF_ACTION_DVA_IDZ_NR; event++)
		{
			#if 0
			if(event == 0)
			{
				if(strcmp(_ACTION_STR_TABLE_TYPE4[0][EMAIL_EVENT_TYPE4_DVA_IDZ][event], name) == 0)
				{
					_nf_action->cb_rise_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT] |= (1<<event);
					_nf_action->cb_curr_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT] |= (1<<event);
				}
			}
			else if(strcmp(_ACTION_STR_TABLE2_TYPE4[0][event], name) == 0)
			{
				_nf_action->cb_rise_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT] |= (1<<event);
				_nf_action->cb_curr_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT] |= (1<<event);
			}
			#endif
			if(strcmp(_ACTION_STR_TABLE_TYPE4[0][EMAIL_EVENT_TYPE4_DVA_IDZ][event], name) == 0)
			{
				_nf_action->cb_rise_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT] |= (1<<event);
				_nf_action->cb_curr_dva_event[ch][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT] |= (1<<event);
			}
		}
	}
#if 0
	printf("\033[0;31m %s >>>>> curr[0x%08x] rise[0x%08x] <<<<< \033[0;39m\n", __FUNCTION__,_nf_action->cb_curr_dva_event[ch][DVA_INTRUSION_DETECTION],_nf_action->cb_rise_dva_event[ch][DVA_INTRUSION_DETECTION]);
	printf("\033[0;31m %s >>>>> curr[0x%08x] rise[0x%08x] <<<<< \033[0;39m\n", __FUNCTION__,_nf_action->cb_curr_dva_event[ch][DVA_ILLEGAL_PARKING],_nf_action->cb_rise_dva_event[ch][DVA_ILLEGAL_PARKING]);
#endif
}
	static void
_action_dva_cb_func( NF_NOTIFY_INFO *pinfo, gpointer data )
{
	guchar ch = 0;
	guchar dva_type = 0;
	DVA_MSG msg;
	
	if(!pinfo)
	{
		g_warning("%s Notify Info is NULL!!", __FUNCTION__);
		return ;
	}
	else
		printf("%s event ch %d !!\n", __FUNCTION__, ch);

	memcpy(&msg, pinfo->p.ptr, sizeof(DVA_MSG));	
	ch = msg.ch;
	dva_type = msg.type;

	NF_OBJECT_LOCK( _nf_action );
	
	if (dva_type == DVA_INTRUSION_DETECTION)
	{
		_nf_action->cb_rise_dva_idz |= (1 << ch);
		_nf_action->cb_meta_dva_idz.timestamp = msg.timestamp;
		_nf_action->cb_meta_dva_idz.timestampl = msg.timestampl;
		memcpy(&_nf_action->cb_meta_dva_idz.intrusion_detection, &msg.intrusion_detection, sizeof(DVA_MSG_IDZ));
		_action_set_dva_event(msg.ch,dva_type, msg.intrusion_detection.name);
	}
	else if (dva_type == DVA_ILLEGAL_PARKING)
	{
		_nf_action->cb_rise_dva_ipz |= (1 << ch);
		_nf_action->cb_meta_dva_ipz.timestamp = msg.timestamp;
		_nf_action->cb_meta_dva_ipz.timestampl = msg.timestampl;
		memcpy(&_nf_action->cb_meta_dva_ipz.illegal_parking, &msg.illegal_parking, sizeof(DVA_MSG_IPZ));
		_action_set_dva_event(msg.ch,dva_type, msg.illegal_parking.name);
	}
	else if (dva_type == DVA_COUNTER)
	{
		/*DVA OBJ CNT*/
		_nf_action->cb_rise_dva_obj_cnt |= (1 << ch);
		_nf_action->cb_meta_dva_obj_cnt.timestamp = msg.timestamp;
		_nf_action->cb_meta_dva_obj_cnt.timestampl = msg.timestampl;
		memcpy(&_nf_action->cb_meta_dva_obj_cnt.counter, &msg.counter, sizeof(DVA_MSG_COUNTER));
		_action_set_dva_event(msg.ch,dva_type, msg.counter.name);
	}
	NF_OBJECT_UNLOCK( _nf_action );

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
		g_message("%s curr[0x%08x] rise[0x%08x]",  __FUNCTION__, *curr_val, *rise_val);
#endif
}

#if defined(USE_DEV_GENNUM)
static void _action_std_type_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
#if 0
	gint ch=0;
	guchar tmp[NUM_ACTIVE_CH]={0, };
	g_message("%s param --> 0x%08x 0x%08x", __FUNCTION__, pinfo->d.params[0], pinfo->d.params[1]);

	tmp[0]=(guchar)pinfo->d.params[1];
	tmp[1]=(guchar)(pinfo->d.params[1] >> 8);
	tmp[2]=(guchar)(pinfo->d.params[1] >> 16);
	tmp[3]=(guchar)(pinfo->d.params[1] >> 24);

	g_message("%s 0x%02x 0x%02x 0x%02x 0x%02x", __FUNCTION__, tmp[0], tmp[1], tmp[2], tmp[3]);
	for(ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		if(pinfo->d.params[0] & (1<<ch))
			g_message("%s CH[%d] --> 0x%02x", __FUNCTION__, ch, tmp[ch]);
	}
#endif
}
#endif

/* DOUBLE KNOCK */
//#define DEBUG_DOBULE_KNOCK
int nf_aciton_get_double_knock_ch(int ch)
{
	guint mask_sensor_dvr = 0, mask_lcamera = 0, mask_sensor_ipcam = 0;
	guint sensor_num=0;
	gchar str_cmdBuff[128] = {0, };
	#if defined(DEBUG_DOBULE_KNOCK)
		//printf("\033[0;34m %s START \033[0;39m\n", __FUNCTION__);
	#endif
	
	#if 0
	if(nf_sysdb_get_uint("act.event.double_knock") == 0)
		return 1;
	#else
		sprintf(str_cmdBuff, "act.vca.V%d.doubleknock", ch);
		if(nf_sysdb_get_bool(str_cmdBuff) == FALSE)
		{
			#if defined(DEBUG_DOBULE_KNOCK)
				//printf("\033[0;31m %s CH %d doubleknock off\033[0;39m\n", __FUNCTION__, ch+1);
			#endif
			return 1;
		}
		else
		{
			#if defined(DEBUG_DOBULE_KNOCK)
				//printf("\033[0;35m %s CH %d doubleknock on\033[0;39m\n", __FUNCTION__, ch+1);
			#endif
		}
	#endif
		
	mask_sensor_dvr = nf_notify_get_param0("sensor");
	mask_sensor_ipcam = nf_notify_get_param1("sensor");
	
	#if defined(DEBUG_DOBULE_KNOCK)
		//printf("\033[0;34m %s mask_sensor 0x%x \033[0;39m\n", __FUNCTION__, mask_sensor);
	#endif
	
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (sensor_num = 0; sensor_num < _nf_action_num_alarm; sensor_num++)
	#else
	for(sensor_num=0; sensor_num<NUM_ALARM; sensor_num++)
	#endif
	{
		if(sensor_num < NUM_ALARM_IPCAM){
	
			if((mask_sensor_ipcam >> sensor_num)&0x1)
			{
				mask_lcamera = _nf_action->sensor_data[sensor_num].mask_lcamera;
				#if defined(DEBUG_DOBULE_KNOCK)
					//printf("\033[0;33m %s SENSOR [%d] mask_lcamera[0x%x] \033[0;39m\n", __FUNCTION__,sensor_num,mask_lcamera);
				#endif
				if((mask_lcamera >> ch) & 0x1)
				{
					#if defined(DEBUG_DOBULE_KNOCK)
						printf("\033[0;37m %s CH %d EVENT UNLOCK S[0x%x] L[0x%x]\033[0;39m\n", __FUNCTION__, ch+1,mask_sensor_ipcam ,mask_lcamera);
					#endif
					return 1;
				}
			}
		}
		else{
			if((mask_sensor_dvr >> (sensor_num-NUM_ALARM_IPCAM))&0x1)
		{
			mask_lcamera = _nf_action->sensor_data[sensor_num].mask_lcamera;
			#if defined(DEBUG_DOBULE_KNOCK)
				//printf("\033[0;33m %s SENSOR [%d] mask_lcamera[0x%x] \033[0;39m\n", __FUNCTION__,sensor_num,mask_lcamera);
			#endif
			if((mask_lcamera >> ch) & 0x1)
			{
				#if defined(DEBUG_DOBULE_KNOCK)
						printf("\033[0;37m %s CH %d EVENT UNLOCK S[0x%x] L[0x%x]\033[0;39m\n", __FUNCTION__, ch+1,mask_sensor_dvr ,mask_lcamera);
				#endif
				return 1;
		}
	}
	}
	}
	#if defined(DEBUG_DOBULE_KNOCK)
		printf("\033[0;37m %s CH %d EVENT LOCK S[0x%x] L[0x%x]\033[0;39m\n", __FUNCTION__, ch+1,mask_sensor ,mask_lcamera);
	#endif
	return 0;
}

/** IMSI AI BOX **/
static void _action_ai_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	ai_rule_event_t *pevt;
	gint type;
	int* p;
	char text[256];

	int i;

	g_return_if_fail(pinfo);
	g_return_if_fail(_nf_action);

	p = pinfo->p.ptr;

	pevt = p+2;
	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if(!get_ai_enable(p[0]))
		return;


	if(p[1] >= 1)
{
//		printf("%s event cnt %d ch %d!!\n", __FUNCTION__, p[1], pevt->ch);
}
	else
	{
		_nf_action->cb_rise_ai[pevt->ch] &= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] &= pevt->type;
	}

	for(i=0;i<p[1];i++){
		//		pevt = pevt + i;
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);
		//FIXME how to handle events independent of zones? (ex. tamper)

		if (_nf_event_ai_get_evt_idx(pevt->type) < 0) {
			printf("\033[0;33m %s [DEBUG_AI] ch %d _nf_event_ai_get_evt_idx(pevt->type) = %d \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type));
			return;
		}
		type = (pevt->type & IVCA_ET_COUNTER) != 0;

		NF_OBJECT_LOCK(_nf_action);
#if 0
		_nf_action->cb_rise_vca[pevt->ch][type] |= 1 << pevt->rule_id;
		_nf_action->cb_curr_vca[pevt->ch][type] |= 1 << pevt->rule_id;
#else
		_nf_action->cb_rise_ai[pevt->ch] |= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] |= pevt->type;

		memcpy(&_nf_action->cb_ai_meta_data[pevt->ch], pevt, sizeof(ai_rule_event_t));
		#if defined(DEBUG_AI_DATA)
			printf("\033[0;36m %s DEBUG_AI_DATA CH %d CLASS %s Topic %s\033[0;39m\n", __FUNCTION__, pevt->ch, _nf_action->cb_ai_meta_data[pevt->ch].object_class, _nf_action->cb_ai_meta_data[pevt->ch].topic);
		#endif

#if 0
		g_message("%s Line[%d] cb_curr_vca [0x%08x] type[0x%08x]",
				__FUNCTION__, __LINE__, _nf_action->cb_curr_vca[pevt->ch], pevt->type);
		printf("%s ch %d type %d object_id %d rule_id %d x %d y %d h %d  w %d\n", __FUNCTION__,
				pevt->ch, pevt->type,pevt->object_id,pevt->rule_id,pevt->rc.x,pevt->rc.y,pevt->rc.h,pevt->rc.w);
#endif

#endif
		NF_OBJECT_UNLOCK(_nf_action);

/*
		printf("_action_ai_cb_func log put !!!!\n");
		printf("type %d object_id %d rule_id %d\n ",
					pevt->type,pevt->object_id,pevt->rule_id);
					*/

		{
			GTimeVal tv;

			gettimeofday(&tv, NULL);
			pevt->timestamp = tv.tv_sec;
			memcpy(text,pevt,sizeof(ai_rule_event_t));
			nf_eventlog_put_param(&tv, LT_DVA, pevt->ch, DVA_AI_DETECTION, text);
			NF_OBJECT_LOCK(_nf_action);

			_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;
			NF_OBJECT_UNLOCK(_nf_action);
			
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] _nf_event_ai_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}
		pevt++;
	}


}

static void _action_ai_fr_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	ai_fr_event_t *pevt;
	gint type;
	int* p;
	char text[256];

	int i;

	g_return_if_fail(pinfo);
	g_return_if_fail(_nf_action);

	p = pinfo->p.ptr;

	pevt = p+2;

	if(!get_ai_enable(p[0]))
		return;


	if(p[1] >= 1)
		printf("%s event cnt %d !!\n", __FUNCTION__, p[1]);
	else
	{
		_nf_action->cb_rise_ai[pevt->ch] &= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] &= pevt->type;
	}

	for(i=0;i<p[1];i++){
		//		pevt = pevt + i;
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		//g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);
		//FIXME how to handle events independent of zones? (ex. tamper)

		type = (pevt->type & IVCA_ET_COUNTER) != 0;

		NF_OBJECT_LOCK(_nf_action);
#if 0
		_nf_action->cb_rise_vca[pevt->ch][type] |= 1 << pevt->rule_id;
		_nf_action->cb_curr_vca[pevt->ch][type] |= 1 << pevt->rule_id;
#else
		_nf_action->cb_rise_ai[pevt->ch] |= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] |= pevt->type;
		memcpy((_nf_action->cb_ai_fr_data)+(pevt->ch),pevt,sizeof(ai_fr_event_t));
		
		#if defined(DEBUG_AI_FR_LPR)
			//printf("\033[0;36m %s IMSI [CH %d] %d %s\033[0;39m\n", __FUNCTION__, pevt->ch,_nf_action->cb_ai_fr_data[pevt->ch].info_cnt,_nf_action->cb_ai_fr_data[pevt->ch].info[0].name);
		#endif
		

#if 0
		g_message("%s Line[%d] cb_curr_vca [0x%08x] type[0x%08x]",
				__FUNCTION__, __LINE__, _nf_action->cb_curr_vca[pevt->ch], pevt->type);
		printf("%s ch %d type %d object_id %d rule_id %d x %d y %d h %d  w %d\n", __FUNCTION__,
				pevt->ch, pevt->type,pevt->object_id,pevt->rule_id,pevt->rc.x,pevt->rc.y,pevt->rc.h,pevt->rc.w);
#endif

#endif
		NF_OBJECT_UNLOCK(_nf_action);

/*
		printf("_action_ai_cb_func log put !!!!\n");
		printf("type %d object_id %d rule_id %d\n ",
					pevt->type,pevt->object_id,pevt->rule_id);
					*/
		
		if(1){
			GTimeVal tv;
			ai_fr_log_t log_data;

			gettimeofday(&tv, NULL);
			log_data.timestamp = tv.tv_sec;
			log_data.timestampl = tv.tv_usec;
			log_data.type = pevt->type;
			log_data.object_id = pevt->object_id;
			log_data.rule_id = pevt->rule_id;
			log_data.ch = pevt->ch;
			memcpy(log_data.bbx_position, pevt->bbx_position , sizeof(double)*4);
			log_data.process_time = pevt->process_time;
			log_data.confidence = pevt->confidence;
			if(pevt->info_cnt){
				log_data.face_id = pevt->info[0].face_id;
				log_data.search_score = pevt->info[0].search_score;
				memcpy(log_data.group_name, pevt->info[0].group_name, 32);
				memcpy(log_data.name, pevt->info[0].name , 32);
				log_data.age = pevt->info[0].age;
				memcpy(log_data.gender, pevt->info[0].gender , 32);
			}
			
			memcpy(text,&(log_data),sizeof(ai_fr_log_t));
			nf_eventlog_put_param(&tv, LT_DVA, pevt->ch, DVA_AI_FR_DETECTION, text);
			NF_OBJECT_LOCK(_nf_action);
			_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
			NF_OBJECT_UNLOCK(_nf_action);
			
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] _nf_event_ai_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}
		
		if(0){
			GTimeVal tv;

			gettimeofday(&tv, NULL);
			pevt->timestamp = tv.tv_sec;
			memcpy(text,pevt,sizeof(ai_fr_event_t));
			nf_eventlog_put_param(&tv, LT_DVA, pevt->ch, DVA_AI_DETECTION, text);
			NF_OBJECT_LOCK(_nf_action);
			_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
			NF_OBJECT_UNLOCK(_nf_action);
			
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] _nf_event_ai_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}
		if(0){
			sprintf(text,"Face Detect Name : %s Group : %s",pevt->info[0].name, pevt->info[0].group_name[0]);
		
			nf_pos_eventlog_put_text(pevt->ch, text, "", ""); 
	
		//	NF_OBJECT_LOCK(_nf_action);
		//	_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
		//	NF_OBJECT_UNLOCK(_nf_action);
			

		}
		pevt++;
	}


}

static void _action_ai_lpr_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	ai_lpr_event_t *pevt;
	gint type;
	int* p;
	char text[256];

	int i;

	g_return_if_fail(pinfo);
	g_return_if_fail(_nf_action);

	p = pinfo->p.ptr;

	pevt = p+2;
	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if(!get_ai_enable(p[0]))
		return;


	if(p[1] >= 1)
		printf("%s event cnt %d !!\n", __FUNCTION__, p[1]);
	else
	{
		_nf_action->cb_rise_ai[pevt->ch] &= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] &= pevt->type;
	}

	for(i=0;i<p[1];i++){
		//		pevt = pevt + i;
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		//g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);
		//FIXME how to handle events independent of zones? (ex. tamper)

		type = (pevt->type & IVCA_ET_COUNTER) != 0;

		NF_OBJECT_LOCK(_nf_action);
#if 0
		_nf_action->cb_rise_vca[pevt->ch][type] |= 1 << pevt->rule_id;
		_nf_action->cb_curr_vca[pevt->ch][type] |= 1 << pevt->rule_id;
#else
		_nf_action->cb_rise_ai[pevt->ch] |= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] |= pevt->type;
		memcpy((_nf_action->cb_ai_lpr_data)+(pevt->ch),pevt,sizeof(ai_lpr_event_t));
		#if defined(DEBUG_AI_FR_LPR)
			printf("\033[0;36m %s 2.IMSI LPR %s \033[0;39m\n", __FUNCTION__,_nf_action->cb_ai_lpr_data[pevt->ch].lp_text);
		#endif
#if 0
		g_message("%s Line[%d] cb_curr_vca [0x%08x] type[0x%08x]",
				__FUNCTION__, __LINE__, _nf_action->cb_curr_vca[pevt->ch], pevt->type);
		printf("%s ch %d type %d object_id %d rule_id %d x %d y %d h %d  w %d\n", __FUNCTION__,
				pevt->ch, pevt->type,pevt->object_id,pevt->rule_id,pevt->rc.x,pevt->rc.y,pevt->rc.h,pevt->rc.w);
#endif
#endif
		NF_OBJECT_UNLOCK(_nf_action);

/*
		printf("_action_ai_cb_func log put !!!!\n");
		printf("type %d object_id %d rule_id %d\n ",
					pevt->type,pevt->object_id,pevt->rule_id);
					*/
		if(1){
			GTimeVal tv;
			ai_lpr_log_t log_data;

			gettimeofday(&tv, NULL);
			log_data.timestamp = tv.tv_sec;
			log_data.timestampl = tv.tv_usec;
			log_data.type = pevt->type;
			log_data.object_id = pevt->object_id;
			log_data.rule_id = pevt->rule_id;
			log_data.ch = pevt->ch;
			memcpy(log_data.bbx_position, pevt->bbx_position , sizeof(double)*4);
			log_data.process_time = pevt->process_time;
			log_data.confidence = pevt->confidence;
			memcpy(log_data.lp_text, pevt->lp_text, 64);
			log_data.group_mask = pevt->grp_mask;
			
			memcpy(text,&(log_data),sizeof(ai_lpr_log_t));
			nf_eventlog_put_param(&tv, LT_DVA, pevt->ch, DVA_AI_LPR_DETECTION, text);
			NF_OBJECT_LOCK(_nf_action);
			_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
			NF_OBJECT_UNLOCK(_nf_action);
			
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] _nf_event_ai_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}

		if(0){
			GTimeVal tv;

			gettimeofday(&tv, NULL);
			pevt->timestamp = tv.tv_sec;
			memcpy(text,pevt,sizeof(ai_rule_event_t));
			nf_eventlog_put_param(&tv, LT_DVA, pevt->ch, DVA_AI_DETECTION, text);
			NF_OBJECT_LOCK(_nf_action);
			_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
			NF_OBJECT_UNLOCK(_nf_action);
			
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] _nf_event_ai_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}
		if(0){
			//sprintf(text,"LPR Number : %s",pevt->info[0].number);
		
			nf_pos_eventlog_put_text(pevt->ch, text, "", ""); 
	
		//	NF_OBJECT_LOCK(_nf_action);
		//	_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
		//	NF_OBJECT_UNLOCK(_nf_action);
			

		}
		pevt++;
	}


}

static void _action_ai_generic_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	ai_generic_event_t *pevt;
	ai_generic_log_t g_evt_log;
	gint type;
	int* p;
	char text[256];

	int i,k;

	g_return_if_fail(pinfo);
	g_return_if_fail(_nf_action);

	p = pinfo->p.ptr;

	pevt = p+2;
	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if(!get_ai_enable(p[0]))
		return;


	if(p[1] >= 1)
	{
	//	printf("%s event cnt %d ch %d !!!!\n", __FUNCTION__, p[1], pevt->ch);
	}
	else
	{
		_nf_action->cb_rise_ai[pevt->ch] &= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] &= pevt->type;
	}
	

	for(i=0;i<p[1];i++){
		//		pevt = pevt + i;
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);

		NF_OBJECT_LOCK(_nf_action);
		
		_nf_action->cb_rise_ai[pevt->ch] |= pevt->type;
		_nf_action->cb_curr_ai[pevt->ch] |= pevt->type;
		memcpy(&_nf_action->cb_ai_generic_data[pevt->ch], pevt, sizeof(ai_generic_event_t));
		#if defined(DEBUG_AI_DATA)
			printf("\033[0;36m %s DEBUG_AI_DATA CAPTION %s\033[0;39m\n", __FUNCTION__, _nf_action->cb_ai_generic_data[pevt->ch].caption);
		#endif
#if 0
		g_message("%s Line[%d] cb_curr_vca [0x%08x] type[0x%08x]",
				__FUNCTION__, __LINE__, _nf_action->cb_curr_vca[pevt->ch], pevt->type);
		printf("%s ch %d type %d object_id %d rule_id %d x %d y %d h %d  w %d\n", __FUNCTION__,
				pevt->ch, pevt->type,pevt->object_id,pevt->rule_id,pevt->rc.x,pevt->rc.y,pevt->rc.h,pevt->rc.w);
#endif

		NF_OBJECT_UNLOCK(_nf_action);

		{
			GTimeVal tv;

			gettimeofday(&tv, NULL);
			pevt->timestamp = tv.tv_sec;

			g_evt_log.type = pevt->type;
			g_evt_log.ch = pevt->ch;
			memcpy(g_evt_log.event_area,pevt->event_area, sizeof(ai_point_t)*2);
			g_evt_log.timestamp = pevt->timestamp;
			g_evt_log.timestampl = pevt->timestampl;

			
			memcpy(g_evt_log.caption,pevt->caption, sizeof(g_evt_log.caption));
			memcpy(g_evt_log.title,pevt->title, sizeof(g_evt_log.title));
			memcpy(g_evt_log.description,pevt->description, sizeof(g_evt_log.description));
		
			memcpy(text,&g_evt_log,sizeof(ai_generic_log_t));
			nf_eventlog_put_param(&tv, LT_DVA, g_evt_log.ch, DVA_AI_GENERIC, text);
			NF_OBJECT_LOCK(_nf_action);
			_nf_action->cb_ai_timestamp[pevt->ch][_nf_event_ai_get_evt_idx(pevt->type)] = tv.tv_sec;;
			NF_OBJECT_UNLOCK(_nf_action);
			
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] _nf_event_ai_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_ai_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}
		pevt++;
	}


}


/** IMSI AI BOX **/
static guint
_nf_event_ai_get_evt_mask(guint evt_type)
{
	guint mask=0;
#if 0
	g_message("%s Line[%d] evt_type[0x%08x]", __FUNCTION__, __LINE__, evt_type);
#endif

	if(evt_type & IVCA_ET_DIR_POS)
		mask |= (1 << NF_ACTION_AI_DIR_POS);
	if(evt_type & IVCA_ET_DIR_NEG)
		mask |= (1 << NF_ACTION_AI_DIR_NEG);
	if(evt_type & IVCA_ET_ENTER)
		mask |= (1 << NF_ACTION_AI_ENTER);
	if(evt_type & IVCA_ET_EXIT)
		mask |= (1 << NF_ACTION_AI_EXIT);
	if(evt_type & IVCA_ET_STOPPED)
		mask |= (1 << NF_ACTION_AI_STOPPED);
	if(evt_type & IVCA_ET_ABANDONED)
		mask |= (1 << NF_ACTION_AI_ABANDONED);
	if(evt_type & IVCA_ET_REMOVED)
		mask |= (1 << NF_ACTION_AI_REMOVED);
	if(evt_type & IVCA_ET_LOITERED)
		mask |= (1 << NF_ACTION_AI_LOITERED);
	if(evt_type & IVCA_ET_FALL)
		mask |= (1 << NF_ACTION_AI_FALL);
	if(evt_type & IVCA_ET_COUNTER)
		mask |= (1 << NF_ACTION_AI_COUNTER);
	if(evt_type & IVCA_ET_TAMPER)
		mask |= (1 << NF_ACTION_AI_TAMPER);
	if(evt_type & IVCA_ET_COLOR)
		mask |= (1 << NF_ACTION_AI_COLOR);
	if(evt_type & IVCA_ET_SIZE)
		mask |= (1 << NF_ACTION_AI_SIZE);
	if(evt_type & IVCA_ET_CLASS)
		mask |= (1 << NF_ACTION_AI_CLASS);
	if(evt_type & IVCA_ET_SPEED)
		mask |= (1 << NF_ACTION_AI_SPEED);
	if(evt_type & IVCA_ET_INTRUSION)
		mask |= (1 << NF_ACTION_AI_INTRUSION);
	if(evt_type & IVCA_ET_FR)
		mask |= (1 << NF_ACTION_AI_FR);
	if(evt_type & IVCA_ET_LPR)
		mask |= (1 << NF_ACTION_AI_LPR);
	if(evt_type & IVCA_ET_GENERIC)
		mask |= (1 << NF_ACTION_AI_GENERIC);	

#if 0
	g_message("%s Line[%d] mask[0x%08x]", __FUNCTION__, __LINE__, mask);
#endif

	return mask;
}

/** IMSI AI BOX **/
static guint
_nf_event_ai_get_evt_idx(guint evt_type)
{
#if 0
		g_message("%s Line[%d] evt_type[0x%08x]", __FUNCTION__, __LINE__, evt_type);
#endif
	
	if(evt_type & IVCA_ET_DIR_POS)
		return NF_ACTION_AI_DIR_POS;
	if(evt_type & IVCA_ET_DIR_NEG)
		return NF_ACTION_AI_DIR_NEG;
	if(evt_type & IVCA_ET_ENTER)
		return NF_ACTION_AI_ENTER;
	if(evt_type & IVCA_ET_EXIT)
		return NF_ACTION_AI_EXIT;
	if(evt_type & IVCA_ET_STOPPED)
		return NF_ACTION_AI_STOPPED;
	if(evt_type & IVCA_ET_ABANDONED)
		return NF_ACTION_AI_ABANDONED;
	if(evt_type & IVCA_ET_REMOVED)
		return NF_ACTION_AI_REMOVED;
	if(evt_type & IVCA_ET_LOITERED)
		return NF_ACTION_AI_LOITERED;
	if(evt_type & IVCA_ET_FALL)
		return NF_ACTION_AI_FALL;
	if(evt_type & IVCA_ET_COUNTER)
		return NF_ACTION_AI_COUNTER;
	if(evt_type & IVCA_ET_TAMPER)
		return NF_ACTION_AI_TAMPER;
	if(evt_type & IVCA_ET_COLOR)
		return NF_ACTION_AI_COLOR;
	if(evt_type & IVCA_ET_SIZE)
		return NF_ACTION_AI_SIZE;
	if(evt_type & IVCA_ET_CLASS)
		return NF_ACTION_AI_CLASS;
	if(evt_type & IVCA_ET_SPEED)
		return NF_ACTION_AI_SPEED;	
	if(evt_type & IVCA_ET_INTRUSION)
		return NF_ACTION_AI_INTRUSION;	
	if(evt_type & IVCA_ET_FR)
		return NF_ACTION_AI_FR;	
	if(evt_type & IVCA_ET_LPR)
		return NF_ACTION_AI_LPR;
	if(evt_type & IVCA_ET_GENERIC)
		return NF_ACTION_AI_GENERIC;		
#if 0
		g_message("%s Line[%d] mask[0x%08x]", __FUNCTION__, __LINE__, mask);
#endif
	return -1;
}

#ifdef	SUPPORT_VCA_CAMERA
static void _action_vca_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	ivca_rule_event_t *pevt;
	gint type;
	int* p;
	char text[256];

	int i;

	g_return_if_fail(pinfo);
	g_return_if_fail(_nf_action);

	p = pinfo->p.ptr;

	pevt = p+2;
	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if(!get_vca_enable(p[0]))
		return;


	if(p[1] >= 1)
		printf("%s event cnt %d !!\n", __FUNCTION__, p[1]);
	else
	{
		_nf_action->cb_rise_vca[pevt->ch] &= pevt->type;
		_nf_action->cb_curr_vca[pevt->ch] &= pevt->type;
	}

	for(i=0;i<p[1];i++){
		//		pevt = pevt + i;
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);
		//FIXME how to handle events independent of zones? (ex. tamper)

		type = (pevt->type & IVCA_ET_COUNTER) != 0;

		NF_OBJECT_LOCK(_nf_action);
#if 0
		_nf_action->cb_rise_vca[pevt->ch][type] |= 1 << pevt->rule_id;
		_nf_action->cb_curr_vca[pevt->ch][type] |= 1 << pevt->rule_id;
#else
		_nf_action->cb_rise_vca[pevt->ch] |= pevt->type;
		_nf_action->cb_curr_vca[pevt->ch] |= pevt->type;

#if 0
		g_message("%s Line[%d] cb_curr_vca [0x%08x] type[0x%08x]",
				__FUNCTION__, __LINE__, _nf_action->cb_curr_vca[pevt->ch], pevt->type);
		printf("%s ch %d type %d object_id %d rule_id %d x %d y %d h %d  w %d\n", __FUNCTION__,
				pevt->ch, pevt->type,pevt->object_id,pevt->rule_id,pevt->rc.x,pevt->rc.y,pevt->rc.h,pevt->rc.w);
#endif

#endif
		NF_OBJECT_UNLOCK(_nf_action);


		//printf("_action_vca_cb_func log put !!!!\n");
		//printf("type %d object_id %d rule_id %d x %d y %d h %d  w %d\n ",
		//			pevt->type,pevt->object_id,pevt->rule_id,pevt->rc.x,pevt->rc.y,pevt->rc.h,pevt->rc.w);

		{
			GTimeVal tv;

			gettimeofday(&tv, NULL);
			pevt->timestamp = tv.tv_sec;
			memcpy(text,pevt,sizeof(ivca_rule_event_t));
			nf_eventlog_put_param(&tv, LT_VCA, pevt->ch, 0, text);
			NF_OBJECT_LOCK(_nf_action);
			_nf_action->cb_vca_timestamp[pevt->ch][_nf_event_vca_get_evt_idx(pevt->type)] = tv.tv_sec;;
			NF_OBJECT_UNLOCK(_nf_action);
			#if defined(DEBUG_VCA_PUSH)
			printf("\033[0;36m %s [%d]_nf_event_vca_get_evt_idx(pevt->type) = %d time [%d] \033[0;39m\n", __FUNCTION__,pevt->ch,_nf_event_vca_get_evt_idx(pevt->type),tv.tv_sec);
			#endif

		}
		pevt++;
	}
}
#endif	/* SUPPORT_VCA_CAMERA */

static void _action_pos_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NF_POS_TEXT_EVENT *pos_text;
	POS_TEXT_DATA *e_pos_text_data=_nf_action->email_pos_text_data;
	POS_TEXT_DATA *f_pos_text_data=_nf_action->ftp_pos_text_data;
	POS_TEXT_DATA *p_pos_text_data=_nf_action->push_pos_text_data;
	gint pos_data_position=0;

	g_message("%s called", __FUNCTION__);
	g_return_if_fail(pinfo);
	g_return_if_fail(_nf_action);

	pos_text = pinfo->p.ptr;

	if (pos_text->ch >= NUM_ACTIVE_CH)
		return;

	/** E-mail **/
	g_static_mutex_lock (&_nf_pos_data_email_mutex);
	pos_data_position = e_pos_text_data[pos_text->ch].data_position;
	if(pos_data_position < NF_ACTION_POS_TEXT_DATA_MAX_NUM)
	{
		e_pos_text_data[pos_text->ch].timestamp[pos_data_position] = pos_text->timestamp;
		strncpy(e_pos_text_data[pos_text->ch].data[pos_data_position], pos_text->data, 128);
		e_pos_text_data[pos_text->ch].data_position++;
	}
	g_static_mutex_unlock (&_nf_pos_data_email_mutex);

	/** FTP **/
	g_static_mutex_lock (&_nf_pos_data_ftp_mutex);
	pos_data_position = f_pos_text_data[pos_text->ch].data_position;
	if(pos_data_position < NF_ACTION_POS_TEXT_DATA_MAX_NUM)
	{
		f_pos_text_data[pos_text->ch].timestamp[pos_data_position] = pos_text->timestamp;
		strncpy(f_pos_text_data[pos_text->ch].data[pos_data_position], pos_text->data, 128);
		f_pos_text_data[pos_text->ch].data_position++;
	}
	g_static_mutex_unlock (&_nf_pos_data_ftp_mutex);

	/** PUSH **/
	g_static_mutex_lock (&_nf_pos_data_push_mutex);
	pos_data_position = p_pos_text_data[pos_text->ch].data_position;
	if(pos_data_position < NF_ACTION_POS_TEXT_DATA_MAX_NUM)
	{
		p_pos_text_data[pos_text->ch].timestamp[pos_data_position] = pos_text->timestamp;
		strncpy(p_pos_text_data[pos_text->ch].data[pos_data_position], pos_text->data, 128);
		p_pos_text_data[pos_text->ch].data_position++;
		printf("\033[0;36m %s IMSI ch %d pos %d\033[0;39m\n", __FUNCTION__, pos_text->ch, p_pos_text_data[pos_text->ch].data_position);
	}
	g_static_mutex_unlock (&_nf_pos_data_push_mutex);
	
	NF_OBJECT_LOCK(_nf_action);

	_nf_action->cb_rise_pos |= 1<<pos_text->ch;
	_nf_action->cb_curr_pos |= 1<<pos_text->ch;

	NF_OBJECT_UNLOCK(_nf_action);
}

static void
_pos_reset(int force)
{
	static int pos_event_reset_cnt[NUM_ACTIVE_CH] = {0,};
	POS_TEXT_DATA		*e_pos_text_data = _nf_action->email_pos_text_data;
	POS_TEXT_DATA		*f_pos_text_data = _nf_action->ftp_pos_text_data;
	POS_TEXT_DATA		*p_pos_text_data = _nf_action->push_pos_text_data;
	int i;

	if(force)
	{
		g_message("%s called All DATA", __FUNCTION__ );

		g_static_mutex_lock (&_nf_pos_data_email_mutex);
		for(i=0; i<NUM_ACTIVE_CH; i++)
		{
			memset(&e_pos_text_data[i], 0, sizeof(POS_TEXT_DATA));
		}
		g_static_mutex_unlock (&_nf_pos_data_email_mutex);

		g_static_mutex_lock (&_nf_pos_data_ftp_mutex);
		for(i=0; i<NUM_ACTIVE_CH; i++)
		{
			memset(&f_pos_text_data[i], 0, sizeof(POS_TEXT_DATA));
		}
		g_static_mutex_unlock (&_nf_pos_data_ftp_mutex);

		g_static_mutex_lock (&_nf_pos_data_push_mutex);
		for(i=0; i<NUM_ACTIVE_CH; i++)
		{
			memset(&p_pos_text_data[i], 0, sizeof(POS_TEXT_DATA));
		}
		g_static_mutex_unlock (&_nf_pos_data_push_mutex);
		
		_nf_action->pos_reset_flag = 0;
	}
	else
	{
		if(_nf_action->pos_reset_flag & NF_ACTION_POS_DATA_RESET_EMAIL)
		{
			g_message("%s called Email DATA", __FUNCTION__ );

			g_static_mutex_lock (&_nf_pos_data_email_mutex);
			for(i=0; i<NUM_ACTIVE_CH; i++)
			{
				memset(&e_pos_text_data[i], 0, sizeof(POS_TEXT_DATA));
			}
			g_static_mutex_unlock (&_nf_pos_data_email_mutex);
			_nf_action->pos_reset_flag &= ~(NF_ACTION_POS_DATA_RESET_EMAIL);
		}

		if(_nf_action->pos_reset_flag & NF_ACTION_POS_DATA_RESET_FTP)
		{
			g_message("%s called FTP DATA", __FUNCTION__ );

			g_static_mutex_lock (&_nf_pos_data_ftp_mutex);
			for(i=0; i<NUM_ACTIVE_CH; i++)
			{
				memset(&f_pos_text_data[i], 0, sizeof(POS_TEXT_DATA));
			}
			g_static_mutex_unlock (&_nf_pos_data_ftp_mutex);
			_nf_action->pos_reset_flag &= ~(NF_ACTION_POS_DATA_RESET_FTP);
		}

		if(_nf_action->pos_reset_flag & NF_ACTION_POS_DATA_RESET_PUSH)
		{
			g_message("%s called Push DATA", __FUNCTION__ );

			g_static_mutex_lock (&_nf_pos_data_push_mutex);
			for(i=0; i<NUM_ACTIVE_CH; i++)
			{
				memset(&p_pos_text_data[i], 0, sizeof(POS_TEXT_DATA));
			}
			g_static_mutex_unlock (&_nf_pos_data_push_mutex);
			_nf_action->pos_reset_flag &= ~(NF_ACTION_POS_DATA_RESET_PUSH);
		}
	}

	NF_OBJECT_LOCK( _nf_action );
	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		if((pos_event_reset_cnt[i] > 10) || force)
		{
			if(_nf_action->curr_pos & (1<<i))
			{
				_nf_action->curr_pos &= ~(1<<i);
				_nf_action->cb_curr_pos &= ~(1<<i);
			}

			_nf_action->rise_pos &= ~(1<<i);
			pos_event_reset_cnt[i] = 0;
		}
		else
			pos_event_reset_cnt[i]++;
	}
	NF_OBJECT_UNLOCK( _nf_action );
}

static GStaticMutex _nf_relay_mutex = G_STATIC_MUTEX_INIT;
static guint		_relay_onoff_bit = 0;

/*	N/O is 0 and N/C is 1 in device driver */
	static void
_relay_init(void)
{
	gint 	relay_ch;
#if defined(ENABLE_SENSOR_IPCAM)
	gint dvr_ch=0;
#endif
	guint	op_type_bit=0;
	gint	dwell_time = 0;
	guint	act_bit = 0;

	g_message("%s called!!", __FUNCTION__);

	_set_relay_enable();

	for(relay_ch = 0; relay_ch < NUM_RELAY; relay_ch++){
#if 0
		if(_nf_action->relay_data[relay_ch].op_type == ON){	// N/C
			op_type_bit |= (1 <<relay_ch);
		}
#endif
#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY_INIT] )
			g_message("%s relay_data[%d].op_type[%d]", __FUNCTION__, relay_ch,
					_nf_action->relay_data[relay_ch].op_type);
#endif
	}

	//Do same with device driver	default is N/O
	nf_dev_relay_set_type(0);
	//g_message("%s relay_set_type[0x%04x]", __FUNCTION__, op_type_bit);

	nf_dev_relay_init(0);		//switch all off.. this is default

	//default is all off
	for(relay_ch = 0; relay_ch < NUM_RELAY; relay_ch++){
#if defined(ENABLE_SENSOR_IPCAM)
		if(relay_ch < NUM_RELAY_IPCAM)
		{
			/**
			  To Do
			  IPCAM Relay Off!!
			 **/
			;
		}
		else
		{
			dvr_ch=(relay_ch - NUM_RELAY_IPCAM);
			nf_dev_relay_off(dvr_ch);
		}
#else
		nf_dev_relay_off(relay_ch);
#endif
	}

}

	static void
_nf_action_alarm_in_init_from_reload(void)
{
	extern void nf_event_reload_alarm_in(void);

	nf_event_reload_alarm_in();
}

	static guint64
_nf_action_reload_alarm_in(void)
{
	guint64 alarm_in_val_curr=0;
#if defined(ENABLE_SENSOR_IPCAM)
	guint alarm_in_val_dvr=0, alarm_in_val_ipcam=0;
	guint alarm_in_val_old=0;
#endif

	nf_dev_sensor_get_curr_value(&alarm_in_val_dvr);
#if defined(ENABLE_SENSOR_IPCAM)
	alarm_in_val_ipcam = nf_notify_get_param1("sensor");
	alarm_in_val_curr = (alarm_in_val_dvr << NUM_ALARM_IPCAM) | (alarm_in_val_ipcam & ALARM_IPCAM_MASK);
	g_message("%s sensor sysdb reload patch. old[0x%08x] curr[0x%08x] dvr[0x%08x] ipcam[0x%08x]",
			__FUNCTION__, alarm_in_val_old, alarm_in_val_curr, alarm_in_val_dvr, alarm_in_val_ipcam);
#else
	g_message("%s sensor sysdb reload patch. alarm_in_val_dvr[0x%08x]", __FUNCTION__, alarm_in_val_dvr);
	alarm_in_val_curr=alarm_in_val_dvr;
#endif

	return alarm_in_val_curr;
}

	static void
_relay_init_from_reload(void)
{
	gint 	relay_ch=0;
#if defined(ENABLE_SENSOR_IPCAM)
	gint dvr_ch=0;
#endif
	guint	op_type_bit=0;
	gint	dwell_time=0;
	guint opbit_dvr=0, opbit_ipcam=0;

	g_message("%s called!!", __FUNCTION__);

	//OPERATION TYPE
	for(relay_ch = 0; relay_ch < NUM_RELAY; relay_ch++){
		if(relay_ch < NUM_RELAY_IPCAM){
		if(_nf_action->relay_data[relay_ch].op_type == NF_ACTION_TYPE_NCLOSE){	// N/C
				opbit_ipcam |= (1 <<relay_ch);
		}
		else{
				opbit_ipcam &= ~(1 <<relay_ch);
			}
		}
		else{
			if(_nf_action->relay_data[relay_ch].op_type == NF_ACTION_TYPE_NCLOSE){	// N/C
				opbit_dvr |= (1 << (relay_ch-NUM_RELAY_IPCAM));
			}
			else{
				opbit_dvr &= ~(1 << (relay_ch-NUM_RELAY_IPCAM));
			}
		}
#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY_RELOAD] )
			g_message("%s relay_data[%d].op_type[%d]", __FUNCTION__, relay_ch,
					_nf_action->relay_data[relay_ch].op_type);
#endif
	}
#if defined(ENABLE_SENSOR_IPCAM)
	//opbit_dvr=(op_type_bit >> NUM_RELAY_IPCAM);
	//opbit_ipcam=(op_type_bit & RELAY_IPCAM_MASK);
	nf_dev_relay_set_type(opbit_dvr);
	/**
	  To Do
	  SET IPCAM OP TYPE!!
	 **/
#else
	nf_dev_relay_set_type(op_type_bit);
#endif

	_relay_webra_onoff = 0;

	for(relay_ch = 0; relay_ch < NUM_RELAY; relay_ch++){
#if defined(ENABLE_SENSOR_IPCAM)
		if(relay_ch < NUM_RELAY_IPCAM)
		{
			/**
			  To Do
			  IPCAM Relay Off!!
			 **/
			_nf_action_relay_off_ipcam(relay_ch);
		}
		else
		{
			dvr_ch=(relay_ch - NUM_RELAY_IPCAM);
			nf_dev_relay_off(dvr_ch);
		}
#else
		nf_dev_relay_off(relay_ch);
#endif
	}
}

#if defined(ENABLE_SENSOR_IPCAM)
	static void
_nf_action_relay_on_ipcam(gint ch)
{
	nf_dev_relay_ch_on_ipcam(ch);
}

	static void
_nf_action_relay_off_ipcam(gint ch)
{
	nf_dev_relay_ch_off_ipcam(ch);
}
#endif

	static void
_relay_on(gint ch)
{
	nf_dev_relay_ch_on(ch);
#ifdef DEBUG_ACTION_KEYLED
	nf_dev_keypad_led_on(ch);
#endif
}

	static void
_relay_off(gint ch)
{
	nf_dev_relay_off(ch);
#ifdef DEBUG_ACTION_KEYLED
	nf_dev_keypad_led_off(ch);
#endif
}

#if 0
	static void
_relay_on_off(guint val)
{
	nf_dev_relay_set_unset(val);
}
#endif

	static void
_buzzer_init(void)
{
	nf_dev_buzzer_off();
}

	static void
_buzzer_onoff(gint flag)
{
	if(flag){
		nf_dev_buzzer_on();
	}
	else{
		nf_dev_buzzer_off();
	}
}

	static guint
_get_sysdb_get_mask(const gchar *property, guint prop_idx, guint mask_cnt, gboolean mode)
{
	//_get_sysdb_get_mask("act.sensor.S%d.arout", (guint)sensor_num, num_relay, 1);
	gchar *tmp_val=NULL;
	gchar tmp_key[256];
	gint i=0;
	guint ret_mask=0;

	g_return_val_if_fail( property != NULL, 0);
#if 0
	g_return_val_if_fail( prop_idx < mask_cnt, 0);
#else
	g_return_val_if_fail( mask_cnt <= NUM_RELAY, 0);
#endif
	g_return_val_if_fail( mode == 1 || mode == 0, 0);

#ifdef DEBUG_ACTION_LOG
	g_message("%s prop[%-32s] idx[%02d] mask_cnt[%02d]", __FUNCTION__,
			property, prop_idx, mask_cnt);
#endif

	if(mode){
		sprintf(tmp_key, property , prop_idx);
	}
	else{
		sprintf(tmp_key, property);
	}

	tmp_val = nf_sysdb_get_str(tmp_key);

	g_return_val_if_fail( tmp_val != NULL , 0);

	for(i=0;i<(gint)mask_cnt;i++)
	{
		if( tmp_val[i] - '0' )
		{
			ret_mask |= 0x1ULL<<i;
		}
	}

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_SYSDB] )
		g_message("%s prop[%-32s] idx[%02d] val[%-16s] mask[0x%08x]", __FUNCTION__,
				tmp_key, prop_idx, tmp_val, ret_mask);
#endif

	g_free( tmp_val);
	return ret_mask;
}

	static guint64
_get_sysdb_get_mask64(const gchar *property, guint prop_idx, guint mask_cnt, gboolean mode)
{
	//_get_sysdb_get_mask("act.sensor.S%d.arout", (guint)sensor_num, num_relay, 1);
	gchar *tmp_val=NULL;
	gchar tmp_key[256];
	gint i=0;
	guint64 ret_mask=0;

	g_return_val_if_fail( property != NULL, 0);
#if 0
	g_return_val_if_fail( prop_idx < mask_cnt, 0);
#else
	//g_return_val_if_fail( mask_cnt <= NUM_RELAY, 0);
	g_return_val_if_fail( mask_cnt <= NUM_ALARM, 0);
#endif
	g_return_val_if_fail( mode == 1 || mode == 0, 0);

#ifdef DEBUG_ACTION_LOG
	g_message("%s prop[%-32s] idx[%02d] mask_cnt[%02d]", __FUNCTION__,
			property, prop_idx, mask_cnt);
#endif

	if(mode){
		sprintf(tmp_key, property , prop_idx);
	}
	else{
		sprintf(tmp_key, property);
	}

	tmp_val = nf_sysdb_get_str(tmp_key);

	g_return_val_if_fail( tmp_val != NULL , 0);

	for(i=0;i<(gint)mask_cnt;i++)
	{
		if( tmp_val[i] - '0' )
		{
			ret_mask |= 0x1ULL<<i;
		}
	}	

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_SYSDB] )
		g_message("%s prop[%-32s] idx[%02d] val[%-16s] mask[%"G_GUINT64_FORMAT"]", __FUNCTION__,
				tmp_key, prop_idx, tmp_val, ret_mask);
#endif

	g_free( tmp_val);
	return ret_mask;
}

	static void
_nf_action_load_buzzer_data(void)
{
	gchar tmp_key[256];

	g_message("%s called", __FUNCTION__);

	if (nf_sysman_qcmode_is_enable() == 1) {
		_nf_action->buzzer_data.buzzer_act = FALSE;
	} else {
		_nf_action->buzzer_data.buzzer_act = TRUE;
	}

	sprintf(tmp_key, "act.event.buzzer.duration");
	_nf_action->buzzer_data.dwell_time = nf_sysdb_get_int(tmp_key);
}

	static void
_nf_action_load_email_data(void)
{
	gchar tmp_key[256]={0, };
	gint addr=0, i;
	EMAIL_DATA *edata=&_nf_action->email_data;

	g_message("%s called", __FUNCTION__);

	// Force True
	_nf_action->email_data.email_act = TRUE;

	sprintf(tmp_key, "act.event.email.freq");
	edata->frequency = nf_sysdb_get_uint(tmp_key);

	for(addr=0; addr<NF_ACTION_EMAIL_MAX_ADDRESS; addr++)
	{
		gchar *s=NULL;

		sprintf(tmp_key, "act.event.A%d.address", addr);
		s=nf_sysdb_get_str_nocopy(tmp_key);

		strncpy(edata->address[addr], s, NF_ACTION_EMAIL_MAX_LENGTH);

#if defined(ENABLE_EMAIL_DUAL_SERVER)
		sprintf(tmp_key, "act.event.A%d.email_serv", addr);
		edata->email_serv[addr] = nf_sysdb_get_uint(tmp_key);
#endif
	}

	sprintf(tmp_key, "act.event.email.jpeg");
	edata->snapshot_onoff = nf_sysdb_get_bool(tmp_key);
	#if 0
	sprintf(tmp_key, "act.event.email.sched.from");
	edata->sched_from = nf_sysdb_get_uint(tmp_key);

	sprintf(tmp_key, "act.event.email.sched.to");
	edata->sched_to = nf_sysdb_get_uint(tmp_key);

	for (i = 0; i < 7; i++)
	{
		sprintf(tmp_key, "act.event.email.sched.wday%d", i);
		edata->sched_wday[i] = nf_sysdb_get_uint(tmp_key);
	}
	#else
		for (i = 0; i < 10; i++)
		{
			sprintf(tmp_key, "act.event.email.S%d.wday", i);
			edata->sched_wday[i] = nf_sysdb_get_uint(tmp_key);

			sprintf(tmp_key, "act.event.email.S%d.from_h", i);
			edata->sched_from_h[i] = nf_sysdb_get_uint(tmp_key);

			sprintf(tmp_key, "act.event.email.S%d.from_m", i);
			edata->sched_from_m[i] = nf_sysdb_get_uint(tmp_key);

			sprintf(tmp_key, "act.event.email.S%d.from_s", i);
			edata->sched_from_s[i] = nf_sysdb_get_uint(tmp_key);

			sprintf(tmp_key, "act.event.email.S%d.to_h", i);
			edata->sched_to_h[i] = nf_sysdb_get_uint(tmp_key);

			sprintf(tmp_key, "act.event.email.S%d.to_m", i);
			edata->sched_to_m[i] = nf_sysdb_get_uint(tmp_key);

			sprintf(tmp_key, "act.event.email.S%d.to_s", i);
			edata->sched_to_s[i] = nf_sysdb_get_uint(tmp_key);
#if 0
			g_message("%s line%d sced%d wday[0x%08x] from_h[%d] form_m[%d] from_s[%d] to_h[%d] to_m[%d] to_s[%d]",
					__FUNCTION__, __LINE__, i, edata->sched_wday[i], edata->sched_from_h[i], edata->sched_from_m[i],
					edata->sched_from_s[i], edata->sched_to_h[i], edata->sched_to_m[i], edata->sched_to_s[i]);
#endif
		}
	#endif

	if(_nf_action->vendor == VENDOR_VIDECON || _nf_action->vendor == VENDOR_CBC)
	{
		sprintf(tmp_key, "act.event.email.al_switch");
		edata->al_switch = nf_sysdb_get_uint(tmp_key);

		sprintf(tmp_key, "act.event.email.al_switch_port");
		edata->al_switch_port = nf_sysdb_get_uint(tmp_key);
	}

	edata->vlink_onoff = nf_sysdb_get_bool("act.event.email.vlink");
#if 0
	sprintf(tmp_key, "act.event.A%d.address", addr);
	edata->snapshot_ch = nf_sysdb_get_uint(tmp_key);
#endif
}

#ifdef ENABLE_ACTION_FTP_SEND
	static void
_nf_action_load_ftp_data(void)
{
	gchar tmp_key[256]={0, };
	gchar *tmp_value;
	gint addr=0;

	g_message("%s called", __FUNCTION__);

	//memset( &_nf_action->ftp_data, 0x00, sizeof(FTP_DATA));

	// Force True
	_nf_action->ftp_data.ftp_act = TRUE;
	_nf_action->ftp_data.frequency = nf_sysdb_get_uint("act.event.ftp.freq");

	_nf_action->ftp_data.snapshot_onoff = nf_sysdb_get_bool("act.event.ftp.jpeg");
	_nf_action->ftp_data.snapshot_ch = 0xff; // ch auto select;

	_nf_action->ftp_data.video_onoff = nf_sysdb_get_bool("act.event.ftp.video");
	_nf_action->ftp_data.video_ch = 0xff; // ch auto select;

	_nf_action->ftp_data.trans_mode = nf_sysdb_get_uint("act.event.ftp.trans_mode");
	_nf_action->ftp_data.dir_mode = nf_sysdb_get_uint("act.event.ftp.dir_mode");
	_nf_action->ftp_data.fname_mode = nf_sysdb_get_uint("act.event.ftp.fname_mode");
	_nf_action->ftp_data.webra_link = nf_sysdb_get_uint("act.event.ftp.weblink");

	tmp_value = nf_sysdb_get_str_nocopy( "act.event.ftp.dir_path");
	//g_return_if_fail( tmp_value != NULL);
	if( tmp_value)
		strncpy( _nf_action->ftp_data.dir_path, tmp_value,	sizeof(_nf_action->ftp_data.dir_path)-1 );

	tmp_value = nf_sysdb_get_str_nocopy( "act.event.ftp.fname_prefix");
	//g_return_if_fail( tmp_value != NULL);
	if( tmp_value)
		strncpy( _nf_action->ftp_data.fname_prefix, tmp_value,	sizeof(_nf_action->ftp_data.fname_prefix)-1 );

	/*
	   <item key="act.event.ftp.host" type="STRING" min="0" max="32" val="" />
	   <item key="act.event.ftp.port" type="UINT" min="1" max="65535" val="21" />
	   <item key="act.event.ftp.username" type="STRING" min="0" max="64" val="" />
	   <item key="act.event.ftp.passwd" type="STRING" min="0" max="16" val="" />

	   <item key="act.event.ftp.freq" type="UINT" min="0" max="9999" val="1" />
	   <item key="act.event.ftp.jpeg" type="BOOL" min="0" max="1" val="0" />

	   <item key="act.event.ftp.weblink" type="UINT" min="0" max="1" val="0" />

	   <item key="act.event.ftp.dir_mode" type="UINT" min="0" max="1" val="0" />
	   <item key="act.event.ftp.dir_path" type="STRING" min="0" max="32" val="" />

	   <item key="act.event.ftp.fname_mode" type="UINT" min="0" max="1" val="0" />
	   <item key="act.event.ftp.fname_prefix" type="STRING" min="0" max="32" val="" />
	   */

}
#endif

#if defined(ENABLE_ACTION_MOBILE)
	static void
_nf_action_load_mobile_data(void)
{
	gchar tmp_key[256]={0, };
	gint addr=0;
	MOBILE_DATA *mdata=&_nf_action->mobile_data;

	g_message("%s called", __FUNCTION__);

	// Force True
	mdata->mobile_act = TRUE;

	sprintf(tmp_key, "act.event.email.freq");
	mdata->frequency = nf_sysdb_get_uint(tmp_key);

#if 0
	for(addr=0; addr<NF_ACTION_EMAIL_MAX_ADDRESS; addr++)
	{
		gchar *s=NULL;

		sprintf(tmp_key, "act.event.A%d.address", addr);
		s=nf_sysdb_get_str_nocopy(tmp_key);

		strncpy(edata->address[addr], s, NF_ACTION_EMAIL_MAX_LENGTH);
	}

	sprintf(tmp_key, "act.event.email.jpeg");
	edata->snapshot_onoff = nf_sysdb_get_bool(tmp_key);

#if 0
	sprintf(tmp_key, "act.event.A%d.address", addr);
	edata->snapshot_ch = nf_sysdb_get_uint(tmp_key);
#endif
#endif

}
#endif


#if defined(ENABLE_ACTION_PUSH)
	static void
_nf_action_load_push_data(void)
{
	gchar tmp_key[256]={0, };
	gint addr=0;
	PUSH_DATA *mdata=&_nf_action->push_data;

	g_message("%s called", __FUNCTION__);

	// Force True
	mdata->push_act = TRUE;

	sprintf(tmp_key, "act.event.mobilepush.freq");
	mdata->frequency = nf_sysdb_get_uint(tmp_key);

	if(_nf_action->vendor == VENDOR_VIDECON || _nf_action->vendor == VENDOR_CBC)
	{
		printf("\033[0;36m %s !!!!!!!!!!!!!!\033[0;39m\n", __FUNCTION__);
		sprintf(tmp_key, "act.event.email.al_switch");
		mdata->al_switch = nf_sysdb_get_uint(tmp_key);
		sprintf(tmp_key, "act.event.email.al_switch_port");
		mdata->al_switch_port = nf_sysdb_get_uint(tmp_key);
	}
	printf("\033[0;36m %s sw %d port %d\033[0;39m\n", __FUNCTION__, mdata->al_switch, mdata->al_switch_port);

}
#endif

	static void
_nf_action_load_relay_data(void)
{
	gchar tmp_key[256];
	gint relay_ch=0, day=0;
	char *s=NULL;

	g_message("%s called", __FUNCTION__);

	for (relay_ch = 0; relay_ch < NUM_RELAY; ++relay_ch) {
		sprintf(tmp_key, "act.arout.R%d.relay_type", relay_ch);
		_nf_action->relay_data[relay_ch].relay_type = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.arout.R%d.op_type", relay_ch);
		_nf_action->relay_data[relay_ch].op_type = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.arout.R%d.duration", relay_ch);
		_nf_action->relay_data[relay_ch].dwell_time = nf_sysdb_get_int(tmp_key);

		sprintf(tmp_key, "act.arout.R%d.sched", relay_ch);
		s=nf_sysdb_get_str_nocopy(tmp_key);
		memcpy(&_nf_action->relay_sched_data.sched_data[relay_ch], s,
				sizeof(_nf_action->relay_sched_data.sched_data[relay_ch]));
#if 0
		if(relay_ch < NUM_RELAY_IPCAM)
			_nf_action->relay_data[relay_ch].is_dvr = FALSE;
		else
			_nf_action->relay_data[relay_ch].is_dvr = TRUE;
#endif
	}
}

static void
_nf_action_load_sensor_data(void)
{
	gchar tmp_key[256]={0, };
	gint sensor_num=0, active_ch_cam=0;

	g_message("%s called", __FUNCTION__);

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		for (sensor_num = 0; sensor_num < _nf_action_num_alarm; sensor_num++)
	#else
		for(sensor_num=0; sensor_num<NUM_ALARM; sensor_num++)
	#endif
	{
		#if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_1648M4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			gint num_relay=0;
			
			num_relay=NUM_RELAY_IPCAM+NUM_RELAY_DVR;   //32
		#endif

		_nf_action->sensor_data[sensor_num].mask_arout =
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			#if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_1648M4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
				_get_sysdb_get_mask("act.sensor.S%d.arout", (guint)sensor_num, num_relay, 1);
			#else
				_get_sysdb_get_mask("act.sensor.S%d.arout", (guint)sensor_num, _nf_action_num_alarm, 1);
			#endif
		#else
			//_get_sysdb_get_mask64("act.sensor.S%d.arout", (guint)sensor_num, NUM_ALARM, 1);
			_get_sysdb_get_mask64("act.sensor.S%d.arout", (guint)sensor_num, num_relay, 1);
		#endif

		_nf_action->sensor_data[sensor_num].mask_lcamera =
			_get_sysdb_get_mask("act.sensor.S%d.lcamera", (guint)sensor_num, NUM_ACTIVE_CH, 1);

#if defined(ENABLE_ACTION_PTZ_PRESET)
		sprintf(tmp_key, "act.sensor.S%d.preset", sensor_num);
		_nf_action->sensor_data[sensor_num].preset_act = nf_sysdb_get_bool(tmp_key);

		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.sensor.S%d.cam.C%d.preset_num", sensor_num, active_ch_cam);
			_nf_action->sensor_data[sensor_num].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
#endif

		sprintf(tmp_key, "act.sensor.S%d.buzzer", sensor_num);
		_nf_action->sensor_data[sensor_num].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.sensor.S%d.email", sensor_num);
		_nf_action->sensor_data[sensor_num].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.sensor.S%d.ftp", sensor_num);
		_nf_action->sensor_data[sensor_num].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
		sprintf(tmp_key, "act.sensor.S%d.mobile", sensor_num);
		_nf_action->sensor_data[sensor_num].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif
#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.sensor.S%d.mobilepush", sensor_num);
		_nf_action->sensor_data[sensor_num].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	}
}

	static void
_nf_action_load_motion_data(void)
{
	gchar tmp_key[256]={0, };
	gint active_ch=0, active_ch_cam=0;

	g_message("%s called", __FUNCTION__);

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		sprintf(tmp_key, "act.motion.M%d.interval", active_ch);
		_nf_action->motion_data[active_ch].ignore_interval = nf_sysdb_get_int(tmp_key);

		_nf_action->motion_data[active_ch].mask_arout =
			_get_sysdb_get_mask64("act.motion.M%d.arout", (guint)active_ch, NUM_RELAY, 1);

#if defined(ENABLE_ACTION_PTZ_PRESET)
		sprintf(tmp_key, "act.motion.M%d.preset", active_ch);
		_nf_action->motion_data[active_ch].preset_act = nf_sysdb_get_bool(tmp_key);

		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.motion.M%d.cam.C%d.preset_num", active_ch, active_ch_cam);
			_nf_action->motion_data[active_ch].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
#endif

		sprintf(tmp_key, "act.motion.M%d.buzzer", active_ch);
		_nf_action->motion_data[active_ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.motion.M%d.email", active_ch);
		_nf_action->motion_data[active_ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.motion.M%d.ftp", active_ch);
		_nf_action->motion_data[active_ch].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
		sprintf(tmp_key, "act.motion.M%d.mobile", active_ch);
		_nf_action->motion_data[active_ch].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif
#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.motion.M%d.mobilepush", active_ch);
		_nf_action->motion_data[active_ch].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	}
}

#if defined(ENABLE_EVENT_TAMPER)
	static void
_nf_action_load_tamper_data(void)
{
	gchar tmp_key[256];
	gint active_ch=0;

	g_message("%s called", __FUNCTION__);

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		sprintf(tmp_key, "act.tamper.T%d.interval", active_ch);
		_nf_action->tamper_data[active_ch].ignore_interval = nf_sysdb_get_int(tmp_key);

		_nf_action->tamper_data[active_ch].mask_arout =
			_get_sysdb_get_mask64("act.tamper.T%d.arout", (guint)active_ch, NUM_RELAY, 1);

		sprintf(tmp_key, "act.tamper.T%d.buzzer", active_ch);
		_nf_action->tamper_data[active_ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.tamper.T%d.email", active_ch);
		_nf_action->tamper_data[active_ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.tamper.T%d.ftp", active_ch);
		_nf_action->tamper_data[active_ch].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
		sprintf(tmp_key, "act.tamper.T%d.mobile", active_ch);
		_nf_action->tamper_data[active_ch].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif
#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.tamper.T%d.mobilepush", active_ch);
		_nf_action->tamper_data[active_ch].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	}
}
#endif

	static void
_nf_action_init_motion_status(void)
{
	gint num_ch=0;

	NF_OBJECT_LOCK( _nf_action );
	for(num_ch=0; num_ch<NUM_ACTIVE_CH; num_ch++)
	{
		MOTION_STATE *mstate = &_nf_action->motion_state[num_ch];

		memset(mstate, 0x0, sizeof(MOTION_STATE));
	}
	NF_OBJECT_UNLOCK( _nf_action );
}

	static void
_nf_action_load_vloss_data(void)
{
	gchar tmp_key[256]={0, };
	gint active_ch=0, active_ch_cam=0;

	g_message("%s called", __FUNCTION__);

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		_nf_action->vloss_data[active_ch].mask_arout =
			_get_sysdb_get_mask64("act.vloss.V%d.arout", (guint)active_ch, NUM_RELAY, 1);

#if defined(ENABLE_ACTION_PTZ_PRESET)
		sprintf(tmp_key, "act.vloss.V%d.preset", active_ch);
		_nf_action->vloss_data[active_ch].preset_act = nf_sysdb_get_bool(tmp_key);

		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.vloss.V%d.cam.C%d.preset_num", active_ch, active_ch_cam);
			_nf_action->vloss_data[active_ch].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
#endif

		sprintf(tmp_key, "act.vloss.V%d.buzzer", active_ch);
		_nf_action->vloss_data[active_ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.vloss.V%d.email", active_ch);
		_nf_action->vloss_data[active_ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.vloss.V%d.ftp", active_ch);
		_nf_action->vloss_data[active_ch].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
		sprintf(tmp_key, "act.vloss.V%d.mobile", active_ch);
		_nf_action->vloss_data[active_ch].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif
#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.vloss.V%d.mobilepush", active_ch);
		_nf_action->vloss_data[active_ch].push_act = nf_sysdb_get_bool(tmp_key);
#endif
	}
}

	static void
_nf_action_load_system_disk_data(void)
{
	gchar tmp_key[256];

	g_message("%s called", __FUNCTION__);

	// disk over
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_OVER].mask_arout =
		_get_sysdb_get_mask64("act.sys.disk.over.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.disk.over.buzzer");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_OVER].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.over.email");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_OVER].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.over.ftp");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_OVER].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.disk.over.mobile");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_OVER].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.disk.over.mobilepush");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_OVER].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// disk full
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_FULL].mask_arout =
		_get_sysdb_get_mask64("act.sys.disk.full.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.disk.full.buzzer");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_FULL].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.full.email");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_FULL].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.full.ftp");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_FULL].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.disk.full.mobile");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_FULL].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.disk.full.mobilepush");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_FULL].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// disk exhausted
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_EXHU].mask_arout =
		_get_sysdb_get_mask64("act.sys.disk.exhau.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.disk.exhau.buzzer");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_EXHU].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.exhau.email");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_EXHU].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.exhau.ftp");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_EXHU].ftp_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.exhau.threshold");
	_nf_action->disk_ddata.exhaust_threshold = nf_sysdb_get_uint(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.disk.exhau.mobile");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_EXHU].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.disk.exhau.mobilepush");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_EXHU].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// disk smart
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_SMART].mask_arout =
		_get_sysdb_get_mask64("act.sys.disk_smart.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.disk.smart.buzzer");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_SMART].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.smart.email");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_SMART].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.smart.ftp");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_SMART].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.disk.smart.mobile");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_SMART].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.disk.smart.mobilepush");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_SMART].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// disk nodisk
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_NODISK].mask_arout =
		_get_sysdb_get_mask64("act.sys.disk.nodisk.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.disk.nodisk.buzzer");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_NODISK].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.nodisk.email");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_NODISK].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.disk.nodisk.ftp");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_NODISK].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.disk.nodisk.mobile");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_NODISK].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.disk.nodisk.mobilepush");
	_nf_action->disk_data[NF_ACTION_HDD_EVENT_NODISK].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	sprintf(tmp_key, "act.sys.disk.nodisk.cnt");
	_nf_action->disk_ddata.nodisk_cnt = nf_sysdb_get_int(tmp_key);
}

	static void
_nf_action_load_system_record_data(void)
{
	gchar tmp_key[256];

	g_message("%s called", __FUNCTION__);

	sprintf(tmp_key, "act.sys.rec.pstart.arout");
	_nf_action->rec_data[NF_ACTION_REC_EVENT_PANIC].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.rec.pstart.buzzer");
	_nf_action->rec_data[NF_ACTION_REC_EVENT_PANIC].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.rec.pstart.email");
	_nf_action->rec_data[NF_ACTION_REC_EVENT_PANIC].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.rec.pstart.ftp");
	_nf_action->rec_data[NF_ACTION_REC_EVENT_PANIC].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.rec.pstart.mobile");
	_nf_action->rec_data[NF_ACTION_REC_EVENT_PANIC].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.rec.pstart.mobilepush");
	_nf_action->rec_data[NF_ACTION_REC_EVENT_PANIC].push_act = nf_sysdb_get_bool(tmp_key);
#endif
}

	static void
_nf_action_load_system_sys_data(void)
{
	gchar tmp_key[256]={0, };

	/** Booting Event **/
	sprintf(tmp_key, "act.sys.sys.booting.arout");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_BOOTING].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.sys.booting.buzzer");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_BOOTING].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.booting.email");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_BOOTING].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.booting.ftp");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_BOOTING].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.sys.booting.mobile");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_BOOTING].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.sys.booting.mobilepush");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_BOOTING].push_act = nf_sysdb_get_bool(tmp_key);
#endif
	/** Login Event **/
	sprintf(tmp_key, "act.sys.sys.logon_fail.arout");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.sys.logon_fail.buzzer");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.logon_fail.email");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.logon_fail.ftp");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.sys.logon_fail.mobile");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.sys.logon_fail.mobilepush");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	/** Fan Fail Event **/
	sprintf(tmp_key, "act.sys.sys.fan_fail.arout");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_FAN_FAIL].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.sys.fan_fail.buzzer");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_FAN_FAIL].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.fan_fail.email");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_FAN_FAIL].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.fan_fail.ftp");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_FAN_FAIL].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.sys.fan_fail.mobile");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_FAN_FAIL].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.sys.fan_fail.mobilepush");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_FAN_FAIL].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	/** Temperature Fail Event **/
	sprintf(tmp_key, "act.sys.sys.temper_fail.arout");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.sys.temper_fail.buzzer");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.temper_fail.email");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.temper_fail.ftp");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.sys.temper_fail.mobile");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.sys.temper_fail.mobilepush");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL].push_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_POE_CHECK)
	/** POE Fail Event **/
	sprintf(tmp_key, "act.sys.sys.poe_fail.arout");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_POE].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.sys.poe_fail.buzzer");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_POE].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.poe_fail.email");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_POE].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.sys.poe_fail.ftp");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_POE].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.sys.poe_fail.mobile");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_POE].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.sys.poe_fail.mobilepush");
	_nf_action->system_data[NF_ACTION_SYSTEM_EVENT_POE].push_act = nf_sysdb_get_bool(tmp_key);
#endif
#endif
}

	static void
_nf_action_load_system_net_data(void)
{
	gchar tmp_key[256];

	g_message("%s called", __FUNCTION__);

	// net trouble
	sprintf(tmp_key, "act.sys.net.eth_trouble.arout");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE].mask_arout =
		_get_sysdb_get_mask64(tmp_key, 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.net.eth_trouble.buzzer");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.eth_trouble.email");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.eth_trouble.ftp");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.net.eth_trouble.mobile");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.net.eth_trouble.mobilepush");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// net remote log on fail
	_nf_action->net_data[NF_ACTION_NET_EVENT_LOGON_FAIL].mask_arout =
		_get_sysdb_get_mask64("act.sys.net.rfail.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.net.rfail.buzzer");
	_nf_action->net_data[NF_ACTION_NET_EVENT_LOGON_FAIL].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.rfail.email");
	_nf_action->net_data[NF_ACTION_NET_EVENT_LOGON_FAIL].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.rfail.ftp");
	_nf_action->net_data[NF_ACTION_NET_EVENT_LOGON_FAIL].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.net.rfail.mobile");
	_nf_action->net_data[NF_ACTION_NET_EVENT_LOGON_FAIL].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.net.rfail.mobilepush");
	_nf_action->net_data[NF_ACTION_NET_EVENT_LOGON_FAIL].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// net ddns updata fail
	_nf_action->net_data[NF_ACTION_NET_EVENT_DDNS_FAIL].mask_arout =
		_get_sysdb_get_mask64("act.sys.net.ddns_fail.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.net.ddns_fail.buzzer");
	_nf_action->net_data[NF_ACTION_NET_EVENT_DDNS_FAIL].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.ddns_fail.email");
	_nf_action->net_data[NF_ACTION_NET_EVENT_DDNS_FAIL].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.ddns_fail.ftp");
	_nf_action->net_data[NF_ACTION_NET_EVENT_DDNS_FAIL].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.net.ddns_fail.mobile");
	_nf_action->net_data[NF_ACTION_NET_EVENT_DDNS_FAIL].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.net.ddns_fail.mobilepush");
	_nf_action->net_data[NF_ACTION_NET_EVENT_DDNS_FAIL].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	// net ip conflict
	_nf_action->net_data[NF_ACTION_NET_EVENT_IP_CONFLICT].mask_arout =
		_get_sysdb_get_mask64("act.sys.net.dhcp_fail.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.net.dhcp_fail.buzzer");
	_nf_action->net_data[NF_ACTION_NET_EVENT_IP_CONFLICT].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.dhcp_fail.email");
	_nf_action->net_data[NF_ACTION_NET_EVENT_IP_CONFLICT].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.dhcp_fail.ftp");
	_nf_action->net_data[NF_ACTION_NET_EVENT_IP_CONFLICT].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.net.dhcp_fail.mobile");
	_nf_action->net_data[NF_ACTION_NET_EVENT_IP_CONFLICT].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.net.dhcp_fail.mobilepush");
	_nf_action->net_data[NF_ACTION_NET_EVENT_IP_CONFLICT].push_act = nf_sysdb_get_bool(tmp_key);
#endif

	/** AI KEEP ALIVE **/
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE_AI_BOX].mask_arout =
		_get_sysdb_get_mask64("act.sys.net.aibox_trouble.arout", 0, NUM_RELAY, 0);

	sprintf(tmp_key, "act.sys.net.aibox_trouble.buzzer");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE_AI_BOX].buzzer_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.aibox_trouble.email");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE_AI_BOX].email_act = nf_sysdb_get_bool(tmp_key);

	sprintf(tmp_key, "act.sys.net.aibox_trouble.ftp");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE_AI_BOX].ftp_act = nf_sysdb_get_bool(tmp_key);

#if defined(ENABLE_ACTION_MOBILE)
	sprintf(tmp_key, "act.sys.net.aibox_trouble.mobile");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE_AI_BOX].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif

#if defined(ENABLE_ACTION_PUSH)
	sprintf(tmp_key, "act.sys.net.aibox_trouble.mobilepush");
	_nf_action->net_data[NF_ACTION_NET_EVENT_TROUBLE_AI_BOX].push_act = nf_sysdb_get_bool(tmp_key);
#endif

}


#if 0
	static void
_nf_action_load_system_poe_data(void)
{
	gchar tmp_key[256];

	g_message("%s called", __FUNCTION__);


}
#endif

/** IMSI AI BOX **/
static void _nf_action_load_ai_data(void)
{
	gchar tmp_key[256]={0, };
	gint ch=0;
	gint active_ch_cam=0;
	g_message("%s called", __FUNCTION__);

	memset(_nf_action->ai_data, 0, sizeof(_nf_action->ai_data));

	for (ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.dvabx.D%d.cam.C%d.preset_num", ch, active_ch_cam);
			_nf_action->ai_data[ch].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
		_nf_action->ai_data[ch].mask_arout =
			_get_sysdb_get_mask64("act.dvabx.D%d.arout", ch, NUM_RELAY, 1);

		sprintf(tmp_key, "act.dvabx.D%d.buzzer", ch);
		_nf_action->ai_data[ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.dvabx.D%d.email", ch);
		_nf_action->ai_data[ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.dvabx.D%d.ftp", ch);
		_nf_action->ai_data[ch].ftp_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.dvabx.D%d.mobile", ch);
		_nf_action->ai_data[ch].mobile_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.dvabx.D%d.preset", ch);
		_nf_action->ai_data[ch].preset_act = nf_sysdb_get_bool(tmp_key);
		
		#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.dvabx.D%d.mobilepush", ch);
		_nf_action->ai_data[ch].push_act = nf_sysdb_get_bool(tmp_key);
		#endif

	}
}

#ifdef	SUPPORT_VCA_CAMERA
static void _nf_action_load_vca_data(void)
{
	gchar tmp_key[256]={0, };
	gint ch=0;
	gint active_ch_cam=0;
	g_message("%s called", __FUNCTION__);

	memset(_nf_action->vca_data, 0, sizeof(_nf_action->vca_data));

	for (ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.vca.V%d.cam.C%d.preset_num", ch, active_ch_cam);
			_nf_action->vca_data[ch].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
		_nf_action->vca_data[ch].mask_arout =
			_get_sysdb_get_mask64("act.vca.V%d.arout", ch, NUM_RELAY, 1);

		sprintf(tmp_key, "act.vca.V%d.buzzer", ch);
		_nf_action->vca_data[ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.vca.V%d.email", ch);
		_nf_action->vca_data[ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.vca.V%d.ftp", ch);
		_nf_action->vca_data[ch].ftp_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.vca.V%d.mobile", ch);
		_nf_action->vca_data[ch].mobile_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.vca.V%d.preset", ch);
		_nf_action->vca_data[ch].preset_act = nf_sysdb_get_bool(tmp_key);
		
		#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.vca.V%d.mobilepush", ch);
		_nf_action->vca_data[ch].push_act = nf_sysdb_get_bool(tmp_key);
		#endif

	}
}

	static void
_nf_action_load_pos_data(void)
{
	gchar tmp_key[256]={0, };
	gint active_ch=0, active_ch_cam=0;

	g_message("%s called", __FUNCTION__);

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		_nf_action->pos_data[active_ch].mask_arout =
			_get_sysdb_get_mask64("act.pos.P%d.arout", (guint)active_ch, NUM_RELAY, 1);

#if defined(ENABLE_ACTION_PTZ_PRESET)
		sprintf(tmp_key, "act.pos.P%d.preset", active_ch);
		_nf_action->pos_data[active_ch].preset_act = nf_sysdb_get_bool(tmp_key);

		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.pos.P%d.cam.C%d.preset_num", active_ch, active_ch_cam);
			_nf_action->pos_data[active_ch].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
#endif

		sprintf(tmp_key, "act.pos.P%d.buzzer", active_ch);
		_nf_action->pos_data[active_ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.pos.P%d.email", active_ch);
		_nf_action->pos_data[active_ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.pos.P%d.ftp", active_ch);
		_nf_action->pos_data[active_ch].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
		sprintf(tmp_key, "act.pos.P%d.mobile", active_ch);
		_nf_action->pos_data[active_ch].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif
#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.pos.P%d.mobilepush", active_ch);
		_nf_action->pos_data[active_ch].push_act = nf_sysdb_get_bool(tmp_key);
#endif
	}
}

	static void
_nf_action_load_dva_data(void)
{
	gchar tmp_key[256]={0, };
	gint active_ch=0, active_ch_cam=0;

	g_message("%s called", __FUNCTION__);

#if 0
	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
#if defined(ENABLE_ACTION_PUSH)
		_nf_action->dva_data[active_ch].push_act = 1;
#endif
	}
#else

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		_nf_action->dva_data[active_ch].mask_arout =
			_get_sysdb_get_mask64("act.dva.D%d.arout", (guint)active_ch, NUM_RELAY, 1);

#if defined(ENABLE_ACTION_PTZ_PRESET)
		sprintf(tmp_key, "act.dva.D%d.preset", active_ch);
		_nf_action->dva_data[active_ch].preset_act = nf_sysdb_get_bool(tmp_key);

		for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
		{
			sprintf(tmp_key, "act.dva.D%d.cam.C%d.preset_num", active_ch, active_ch_cam);
			_nf_action->dva_data[active_ch].preset_num[active_ch_cam] = nf_sysdb_get_uint(tmp_key);
		}
#endif

		sprintf(tmp_key, "act.dva.D%d.buzzer", active_ch);
		_nf_action->dva_data[active_ch].buzzer_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.dva.D%d.email", active_ch);
		_nf_action->dva_data[active_ch].email_act = nf_sysdb_get_bool(tmp_key);

		sprintf(tmp_key, "act.dva.D%d.ftp", active_ch);
		_nf_action->dva_data[active_ch].ftp_act = nf_sysdb_get_bool(tmp_key);
#if defined(ENABLE_ACTION_MOBILE)
		sprintf(tmp_key, "act.dva.D%d.mobile", active_ch);
		_nf_action->dva_data[active_ch].mobile_act = nf_sysdb_get_bool(tmp_key);
#endif
#if defined(ENABLE_ACTION_PUSH)
		sprintf(tmp_key, "act.dva.D%d.mobilepush", active_ch);
		_nf_action->dva_data[active_ch].push_act = nf_sysdb_get_bool(tmp_key);
#endif
	}

#endif
}

	static guint
_nf_event_vca_get_evt_mask(guint evt_type)
{
	guint mask=0;
#if 0
	g_message("%s Line[%d] evt_type[0x%08x]", __FUNCTION__, __LINE__, evt_type);
#endif

	if(evt_type & IVCA_ET_DIR_POS)
		mask |= (1 << NF_ACTION_VCA_DIR_POS);
	if(evt_type & IVCA_ET_DIR_NEG)
		mask |= (1 << NF_ACTION_VCA_DIR_NEG);
	if(evt_type & IVCA_ET_ENTER)
		mask |= (1 << NF_ACTION_VCA_ENTER);
	if(evt_type & IVCA_ET_EXIT)
		mask |= (1 << NF_ACTION_VCA_EXIT);
	if(evt_type & IVCA_ET_STOPPED)
		mask |= (1 << NF_ACTION_VCA_STOPPED);
	if(evt_type & IVCA_ET_ABANDONED)
		mask |= (1 << NF_ACTION_VCA_ABANDONED);
	if(evt_type & IVCA_ET_REMOVED)
		mask |= (1 << NF_ACTION_VCA_REMOVED);
	if(evt_type & IVCA_ET_LOITERED)
		mask |= (1 << NF_ACTION_VCA_LOITERED);
	if(evt_type & IVCA_ET_FALL)
		mask |= (1 << NF_ACTION_VCA_FALL);
	if(evt_type & IVCA_ET_COUNTER)
		mask |= (1 << NF_ACTION_VCA_COUNTER);
	if(evt_type & IVCA_ET_TAMPER)
		mask |= (1 << NF_ACTION_VCA_TAMPER);
	if(evt_type & IVCA_ET_COLOR)
		mask |= (1 << NF_ACTION_VCA_COLOR);
	if(evt_type & IVCA_ET_SIZE)
		mask |= (1 << NF_ACTION_VCA_SIZE);
	if(evt_type & IVCA_ET_CLASS)
		mask |= (1 << NF_ACTION_VCA_CLASS);
	if(evt_type & IVCA_ET_SPEED)
		mask |= (1 << NF_ACTION_VCA_SPEED);

#if 0
	g_message("%s Line[%d] mask[0x%08x]", __FUNCTION__, __LINE__, mask);
#endif

	return mask;
}
static guint
_nf_event_vca_get_evt_idx(guint evt_type)
{
#if 0
		g_message("%s Line[%d] evt_type[0x%08x]", __FUNCTION__, __LINE__, evt_type);
#endif
	
	if(evt_type & IVCA_ET_DIR_POS)
		return NF_ACTION_VCA_DIR_POS;
	if(evt_type & IVCA_ET_DIR_NEG)
		return NF_ACTION_VCA_DIR_NEG;
	if(evt_type & IVCA_ET_ENTER)
		return NF_ACTION_VCA_ENTER;
	if(evt_type & IVCA_ET_EXIT)
		return NF_ACTION_VCA_EXIT;
	if(evt_type & IVCA_ET_STOPPED)
		return NF_ACTION_VCA_STOPPED;
	if(evt_type & IVCA_ET_ABANDONED)
		return NF_ACTION_VCA_ABANDONED;
	if(evt_type & IVCA_ET_REMOVED)
		return NF_ACTION_VCA_REMOVED;
	if(evt_type & IVCA_ET_LOITERED)
		return NF_ACTION_VCA_LOITERED;
	if(evt_type & IVCA_ET_FALL)
		return NF_ACTION_VCA_FALL;
	if(evt_type & IVCA_ET_COUNTER)
		return NF_ACTION_VCA_COUNTER;
	if(evt_type & IVCA_ET_TAMPER)
		return NF_ACTION_VCA_TAMPER;
	if(evt_type & IVCA_ET_COLOR)
		return NF_ACTION_VCA_COLOR;
	if(evt_type & IVCA_ET_SIZE)
		return NF_ACTION_VCA_SIZE;
	if(evt_type & IVCA_ET_CLASS)
		return NF_ACTION_VCA_CLASS;
	if(evt_type & IVCA_ET_SPEED)
		return NF_ACTION_VCA_SPEED;	
#if 0
		g_message("%s Line[%d] mask[0x%08x]", __FUNCTION__, __LINE__, mask);
#endif
	return -1;
}

#endif	/* SUPPORT_VCA_CAMERA */

/* thread ?�서 ?�제 기능 ?�수 */
	static void
_relay_action(void)
{
	gint 				i=0, active_ch=0, num_evt=0, curr_hour=0, curr_day=0, num_alarm=0, num_relay=0;
	guint 				rise_dvr=0, keyin_check=0, keypad_keyin_check=0, rc_keyin_check=0;
	guint64 				is_rise=0, is_on=0;
	GTimeVal			curr_timeval;
	RELAY_SCHED_DATA 	*sched_data=&_nf_action->relay_sched_data;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

#ifdef USE_DEV_KEYPAD
	nf_dev_keypad_input_cnt( &keypad_keyin_check);
#endif

#ifdef USE_DEV_REMOCON
	nf_dev_remocon_input_cnt( &rc_keyin_check);         /** add by pakkhman 091009 **/
#endif

#if defined (ENABLE_MOUSE_UNTILKEY_STOP)
	keyin_check = (keypad_keyin_check + rc_keyin_check + _mouse_key_in_check);          /** add by pakkhman 091222 **/
#else
	keyin_check = (keypad_keyin_check + rc_keyin_check);
#endif

	if(!_nf_action->is_action_ctrl)
		keyin_check=0;

	/** Get Time Schedule **/
	curr_day=_nf_action->curr_day;
	curr_hour=_nf_action->curr_hour;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (num_alarm = 0; num_alarm < _nf_action_num_alarm; num_alarm++)
	#else
	for(num_alarm=0; num_alarm<NUM_ALARM; num_alarm++)
	#endif
	{
		ALARM_SENSOR_DATA	*sdata = &_nf_action->sensor_data[num_alarm];

		if(_nf_action->curr_alarm & (0x1ULL<<num_alarm))
		{
			is_on |= sdata->mask_arout;
		}

		if(_nf_action->rise_alarm & (0x1ULL<<num_alarm))
		{
			is_rise |= sdata->mask_arout;
		}
	}

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		MOTION_DATA			*mdata = &_nf_action->motion_data[active_ch];
		VLOSS_DATA			*vdata = &_nf_action->vloss_data[active_ch];
#if defined(ENABLE_EVENT_TAMPER)
		TAMPER_DATA			*tdata = &_nf_action->tamper_data[active_ch];
#endif
		POS_DATA			*pdata = &_nf_action->pos_data[active_ch];

		if(_nf_action->curr_motion & (1<<active_ch))
			is_on |= mdata->mask_arout;

		if(_nf_action->rise_motion & (1<<active_ch))
			is_rise |= mdata->mask_arout;

		if(_nf_action->curr_vloss & (1<<active_ch))
			is_on |= vdata->mask_arout;

		if(_nf_action->rise_vloss & (1<<active_ch))
		{
			is_rise |= vdata->mask_arout;
#if 0
			g_message("%s Vloss CH[%d] Rise Vloss[0x%08x] mask[0x%08x] is_rise[0x%08x]",
					__FUNCTION__, active_ch, _nf_action->rise_vloss, vdata->mask_arout, is_rise);
#endif
		}

#if defined(ENABLE_EVENT_TAMPER)
		if(_nf_action->curr_tamper & (1<<active_ch))
			is_on |= tdata->mask_arout;

		if(_nf_action->rise_tamper & (1<<active_ch))
			is_rise |= tdata->mask_arout;
#endif

		if(_nf_action->curr_pos & (1<<active_ch))
			is_on |= pdata->mask_arout;

		if(_nf_action->rise_pos & (1<<active_ch))
			is_rise |= pdata->mask_arout;
	}

	/** HDD Event Check **/
	for(num_evt=0; num_evt<NF_ACTION_HDD_EVENT_NR; num_evt++)
	{
		DISK_DATA			*ddata = &_nf_action->disk_data[num_evt];

		if(_nf_action->curr_hdd & (1<<num_evt))
			is_on |= ddata->mask_arout;

		if(_nf_action->rise_hdd & (1<<num_evt))
			is_rise |= ddata->mask_arout;
	}

	/** Record Event Check **/
	for(num_evt=0; num_evt<NF_ACTION_REC_EVENT_NR; num_evt++)
	{
		REC_DATA			*rdata = &_nf_action->rec_data[num_evt];

		if(_nf_action->curr_rec_panic)
			is_on |= rdata->mask_arout;

		if(_nf_action->rise_rec_panic)
		{
			is_rise |= rdata->mask_arout;
#if 0
			g_message("%s Rec_Panic Evt[%d] Rise_Rec_Panic[0x%08x] mask[0x%08x] is_rise[0x%08x]",
					__FUNCTION__, num_evt, _nf_action->rise_rec_panic, rdata->mask_arout, is_rise);
#endif
		}
	}

	/** System Event Check **/
	for(num_evt=0; num_evt<NF_ACTION_SYSTEM_EVENT_NR; num_evt++)
	{
		SYSTEM_DATA		*sysdata = &_nf_action->system_data[num_evt];

		if(_nf_action->curr_system & (1<<num_evt))
			is_on |= sysdata->mask_arout;

		if(_nf_action->rise_system & (1<<num_evt))
			is_rise |= sysdata->mask_arout;
	}

	for(num_evt=0; num_evt<NF_ACTION_NET_EVENT_NR; num_evt++)
	{
		NET_DATA			*ndata = &_nf_action->net_data[num_evt];

		if(_nf_action->curr_net & (1<<num_evt))
			is_on |= ndata->mask_arout;

		if(_nf_action->rise_net & (1<<num_evt))
			is_rise |= ndata->mask_arout;
	}

#ifdef	SUPPORT_VCA_CAMERA
	/** VCA Event Check **/
	for (active_ch = 0; active_ch < NUM_ACTIVE_CH; active_ch++)
	{
		VCA_DATA *vcadata = &_nf_action->vca_data[active_ch];

		if ( _nf_action->curr_vca[active_ch] )
			is_on |= vcadata->mask_arout;
		if ( _nf_action->rise_vca[active_ch] )
			is_rise |= vcadata->mask_arout;
	}
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	
	for (active_ch = 0; active_ch < NUM_ACTIVE_CH; active_ch++)
	{
		AI_DATA *aidata = &_nf_action->ai_data[active_ch];

		if ( _nf_action->curr_ai[active_ch] )
		{
			is_on |= aidata->mask_arout;
			#if defined DEBUG_AI
			//printf("\033[0;33m %s [DEBUG_AI] ch[%d] is_on\033[0;39m\n", __FUNCTION__,active_ch);
			#endif
		}
		if ( _nf_action->rise_ai[active_ch] )
		{
			is_rise |= aidata->mask_arout;
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] ch[%d] is_rise\033[0;39m\n", __FUNCTION__,active_ch);
			#endif
		}
	}
	
	/** DVA Event Check **/
	/*DVA OBJ CNT*/
	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		DVA_DATA *dvadata = &_nf_action->dva_data[active_ch];
		int mask = (guint)(1<<active_ch);
		if((_nf_action->curr_dva_idz & mask) || (_nf_action->curr_dva_ipz & mask) || (_nf_action->curr_dva_obj_cnt & mask))
			is_on |= dvadata->mask_arout;
		if((_nf_action->rise_dva_idz & mask) || (_nf_action->rise_dva_ipz & mask) || (_nf_action->rise_dva_obj_cnt & mask))
			is_rise |= dvadata->mask_arout;
	}
	
	for(i=0; i<NUM_RELAY; i++)
	{
		RELAY_STATE	*rstate = &_nf_action->relay_state[i];
		RELAY_DATA	*rdata = &_nf_action->relay_data[i];
		gchar curr_event=0;

		sched_data->sched_ptr=sched_data->sched_data[i];
		curr_event=sched_data->sched_ptr[(NF_ACTION_HOUR_A_DAY*curr_day)+curr_hour];

		if(curr_event == NF_ACTION_RELAY_CHAR_EVENT)
			sched_data->sched_mode=NF_ACTION_RELAY_CHAR_EVENT;		// '2' -> 50
		else if(curr_event == NF_ACTION_RELAY_CHAR_ON)
			sched_data->sched_mode=NF_ACTION_RELAY_CHAR_ON;			// '1' -> 49
		else
			sched_data->sched_mode=NF_ACTION_RELAY_CHAR_OFF;		// '0' -> 48

#if 0		// for test
		sched_data->sched_mode=NF_ACTION_RELAY_CHAR_EVENT;
#endif

		if(sched_data->sched_mode != NF_ACTION_RELAY_CHAR_OFF)
		{
			if(sched_data->sched_mode == NF_ACTION_RELAY_CHAR_EVENT)
			{
#if 0
#ifdef DEBUG_ACTION_LOG
				if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY] & (1 << i) )
					g_message("%s on[%d] rise[%d] motion[%04x][%04x][%04x] alarm[%04x][%04x][%04x] vloss[%04x][%04x][%04x]",
							__FUNCTION__, is_on, is_rise,
							rdata_mask->mask_motion[i], _nf_action->curr_motion, _nf_action->rise_motion,
							rdata_mask->mask_alarm[i],  _nf_action->curr_alarm,  _nf_action->rise_alarm,
							rdata_mask->mask_vloss[i],  _nf_action->curr_vloss,  _nf_action->rise_vloss );
#endif
#endif

				if(!(_relay_webra_onoff & (0x1ULL<<i)))
				{
					if( rdata->dwell_time == NF_ACTION_MODE_TRANSPARENT ){
						if( is_on & (0x1ULL<<i) ) {
							rstate->onoff = ON;
						}else{
							rstate->onoff = OFF;
						}
					}else{
						if( is_rise & (0x1ULL<<i) ) {
							rstate->onoff = ON;
							rstate->dwell_in_sec = curr_timeval.tv_sec;
							rstate->dwell_out_sec = curr_timeval.tv_sec
								+ ((rdata->dwell_time == NF_ACTION_UNTIL_KEY ) ? (60*60*24*31) : rdata->dwell_time);

							rstate->keyin_check = keyin_check;

#ifdef DEBUG_ACTION_LOG
							if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY] & (0x1ULL << i) )
								g_message("ch[%d] rise %ld %ld", i, curr_timeval.tv_sec, rstate->dwell_out_sec);
#endif
						} else {

#ifdef DEBUG_ACTION_LOG
							if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY] & (0x1ULL << i) )
								g_message("ch[%d] latch %ld %ld", i, curr_timeval.tv_sec, rstate->dwell_out_sec);
#endif

							if(rstate->dwell_out_sec > curr_timeval.tv_sec ){
								if( rdata->dwell_time == NF_ACTION_UNTIL_KEY )
								{
									if( rstate->keyin_check != keyin_check)
									{
										g_message("%s until_key off [%d][%d]", __FUNCTION__, rstate->keyin_check, keyin_check);
										rstate->onoff = OFF;
										rstate->dwell_in_sec = 0;
										rstate->dwell_out_sec = 0;
#if defined (ENABLE_MOUSE_UNTILKEY_STOP)        /** 091222 by pakkhman **/
										_mouse_key_in_check=FALSE;
#endif
									}
								}else{
									rstate->onoff = ON;
								}
							}else{
								rstate->onoff = OFF;
								rstate->dwell_in_sec = 0;
								rstate->dwell_out_sec = 0;
							}
						} // is_rise
					} // rdata->dwell_time == NF_ACTION_MODE_TRANSPARENT
				}
			} // if(sched_data->sched_mode == NF_ACTION_RELAY_CHAR_EVENT)
			else{
				rstate->onoff = ON;
			}	// end NF_ACTION_RELAY_CHAR_EVENT
		}
		else{
			rstate->onoff = OFF;
		}	// end of NF_ACTION_RELAY_CHAR_OFF

		// for relay test
		if(rstate->is_manual_test)
			rstate->onoff = ON;

		/*	here	*/
		// sensor type....(N/C ? N/O)
		if(	rstate->onoff ){
			if( rstate->state == OFF )
			{
#if defined(ENABLE_SENSOR_IPCAM)
				if(i < NUM_RELAY_IPCAM)
				{
					g_message("%s IPCAM CH%d ON!!!", __FUNCTION__, i);
					_nf_action_relay_on_ipcam(i);
				}
				else
				{
					gint ch=0;

					ch= i-NUM_RELAY_IPCAM;
					g_message("%s NVR Relay CH%d On!!", __FUNCTION__, ch);
					_relay_on(ch);
				}
#else
				g_message("%s Relay [%d] On!!", __FUNCTION__, i);
				_relay_on(i);
#endif

				rstate->state = ON;

#ifdef DEBUG_ACTION_LOG
				if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY_OUT] )
					g_message("%s ch[%d] on[%d] rise[%d] %s!!!!", __FUNCTION__,
							i, is_on, is_rise, rstate->onoff  ? "ON":"OFF" );
#endif
			}
		}else{
			if( rstate->state )
			{
#if defined(ENABLE_SENSOR_IPCAM)
				if(i < NUM_RELAY_IPCAM)
				{
					g_message("%s IPCAM CH%d OFF!!!", __FUNCTION__, i);
					_nf_action_relay_off_ipcam(i);
				}
				else
				{
					gint ch=0;

					ch=i-NUM_RELAY_IPCAM;
					g_message("%s NVR Relay CH%d Off!!", __FUNCTION__, ch);
					_relay_off(ch);
				}
#else
				g_message("%s Relay [%d] Off!!", __FUNCTION__, i);
				_relay_off(i);
#endif

				rstate->state = OFF;

#ifdef DEBUG_ACTION_LOG
				if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_RELAY_OUT] )
					g_message("%s ch[%d] on[%d] rise[%d] %s!!!!", __FUNCTION__,
							i, is_on, is_rise, rstate->onoff  ? "ON":"OFF" );
#endif
			}
		}
	}	// end for
}

	static void
_buzzer_action(void)
{
	gint			i=0, active_ch=0, num_evt=0, num_alarm=0;
	guint64		is_on =0, is_rise =0;
	GTimeVal        curr_timeval;
	BUZZER_DATA		*bdata = &_nf_action -> buzzer_data;
	BUZZER_STATE	*bstate = &_nf_action -> buzzer_state;
	guint 			keyin_check=0, keypad_keyin_check=0, rc_keyin_check=0;
	guint			force_buzzer_on=0;

#ifdef USE_DEV_KEYPAD
	nf_dev_keypad_input_cnt( &keypad_keyin_check);
#endif

#ifdef USE_DEV_REMOCON
	nf_dev_remocon_input_cnt( &rc_keyin_check);         /** add by pakkhman 091009 **/
#endif

#if defined (ENABLE_MOUSE_UNTILKEY_STOP)
	keyin_check = (keypad_keyin_check + rc_keyin_check + _mouse_key_in_check);          /** add by pakkhman 091222 **/
#else
	keyin_check = (keypad_keyin_check + rc_keyin_check);
#endif

	if(!_nf_action->is_action_ctrl)
		keyin_check=0;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	if(bdata -> buzzer_act){

		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		for (num_alarm = 0; num_alarm < _nf_action_num_alarm; num_alarm++)
		#else
		for(num_alarm=0; num_alarm<NUM_ALARM; num_alarm++)
		#endif
		{
			ALARM_SENSOR_DATA	*sdata = &_nf_action->sensor_data[num_alarm];

			if(_nf_action->curr_alarm & (0x1ULL<<num_alarm))
			{
				#if defined (DEBUG_BUZZ_TRANS)
				//printf("\033[0;36m %s [DEBUG_BUZZ_TRANS] curr_alarm \033[0;39m\n", __FUNCTION__);
				#endif
				if(sdata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_alarm & (0x1ULL<<num_alarm))
			{
				#if defined (DEBUG_BUZZ_TRANS)
				//printf("\033[0;36m %s [DEBUG_BUZZ_TRANS] rise_alarm \033[0;39m\n", __FUNCTION__);
				#endif
				if(sdata->buzzer_act)
					is_rise |= TRUE;
			}
		}

		for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
		{
			MOTION_DATA			*mdata = &_nf_action->motion_data[active_ch];
			VLOSS_DATA			*vdata = &_nf_action->vloss_data[active_ch];
#if defined(ENABLE_EVENT_TAMPER)
			TAMPER_DATA			*tdata = &_nf_action->tamper_data[active_ch];
#endif
			POS_DATA			*pdata = &_nf_action->pos_data[active_ch];

			if(_nf_action->curr_motion & (1<<active_ch))
			{
				#if defined (DEBUG_BUZZ_TRANS)
				printf("\033[0;36m %s [DEBUG_BUZZ_TRANS] curr_alarm \033[0;39m\n", __FUNCTION__);
				#endif
				if(mdata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_motion & (1<<active_ch))
			{
				#if defined (DEBUG_BUZZ_TRANS)
				printf("\033[0;36m %s [DEBUG_BUZZ_TRANS] rise_alarm \033[0;39m\n", __FUNCTION__);
				#endif
				if(mdata->buzzer_act)
					is_rise |= TRUE;
			}

			if(_nf_action->curr_vloss & (1<<active_ch))
			{
				if(vdata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_vloss & (1<<active_ch))
			{
				if(vdata->buzzer_act)
					is_rise |= TRUE;
			}

#if defined(ENABLE_EVENT_TAMPER)
			if(_nf_action->curr_tamper & (1<<active_ch))
			{
				if(tdata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_tamper & (1<<active_ch))
			{
				if(tdata->buzzer_act)
					is_rise |= TRUE;
			}
#endif
			if(_nf_action->curr_pos & (1<<active_ch))
			{
				if(pdata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_pos & (1<<active_ch))
			{
				if(pdata->buzzer_act)
					is_rise |= TRUE;
			}
		}

		/** HDD Event Check **/
		for(num_evt=0; num_evt<NF_ACTION_HDD_EVENT_NR; num_evt++)
		{
			DISK_DATA		*ddata = &_nf_action->disk_data[num_evt];

			if(_nf_action->curr_hdd & (1<<num_evt))
			{
				if(ddata->buzzer_act)
				{
					is_on |= TRUE;

#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
					if((num_evt == NF_ACTION_HDD_EVENT_SMART) ||
							(num_evt == NF_ACTION_HDD_EVENT_NODISK))
						bstate->event[NF_ACTION_EVENT_DISK] |= (1<<num_evt);
#endif
				}
			}
#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
			else
				bstate->event[NF_ACTION_EVENT_DISK] &= ~(1<<num_evt);
#endif

			if(_nf_action->rise_hdd & (1<<num_evt))
			{
				if(ddata->buzzer_act)
				{
#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
					if((num_evt != NF_ACTION_HDD_EVENT_SMART) ||
							(num_evt != NF_ACTION_HDD_EVENT_NODISK))
						is_rise |= TRUE;
#else
					is_rise |= TRUE;
#endif
				}
			}
		}

		/** Record Event Check **/
		for(num_evt=0; num_evt<NF_ACTION_REC_EVENT_NR; num_evt++)
		{
			REC_DATA		*rdata = &_nf_action->rec_data[num_evt];

			if(_nf_action->curr_rec_panic)
			{
				if(rdata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_rec_panic)
			{
				if(rdata->buzzer_act)
					is_rise |= TRUE;
			}
		}

		/** System Event Check **/
		for(num_evt=0; num_evt<NF_ACTION_SYSTEM_EVENT_NR; num_evt++)
		{
			SYSTEM_DATA		*sysdata = &_nf_action->system_data[num_evt];

			if(_nf_action->curr_system & (1<<num_evt))
			{
				if(sysdata->buzzer_act)
				{
					is_on |= TRUE;

#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
					if((num_evt == NF_ACTION_SYSTEM_EVENT_FAN_FAIL) ||
							(num_evt == NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL))
						bstate->event[NF_ACTION_EVENT_SYSTEM] |= (1<<num_evt);
#endif
				}
			}
#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
			else
				bstate->event[NF_ACTION_EVENT_SYSTEM] &= ~(1<<num_evt);
#endif

			if(_nf_action->rise_system & (1<<num_evt))
			{
				if(sysdata->buzzer_act)
				{
#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
					if((num_evt != NF_ACTION_SYSTEM_EVENT_FAN_FAIL) ||
							(num_evt != NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL))
						is_rise |= TRUE;
#else
					is_rise |= TRUE;
#endif
				}
			}
		}

		for(num_evt=0; num_evt<NF_ACTION_NET_EVENT_NR; num_evt++)
		{
			NET_DATA		*ndata = &_nf_action->net_data[num_evt];

			if(_nf_action->curr_net & (1<<num_evt))
			{
				if(ndata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_net & (1<<num_evt))
			{
				if(ndata->buzzer_act)
					is_rise |= TRUE;
			}
		}

#ifdef	SUPPORT_VCA_CAMERA
		/** VCA Event Check **/
		for (active_ch = 0; active_ch < NUM_ACTIVE_CH; active_ch++)
		{
			VCA_DATA *vcadata = &_nf_action->vca_data[active_ch];

			if(_nf_action->curr_vca[active_ch])
			{
				if(vcadata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_vca[active_ch])
			{
				if(vcadata->buzzer_act)
					is_rise |= TRUE;
			}
		}
#endif	/* SUPPORT_VCA_CAMERA */

		/** IMSI AI BOX **/
		for (active_ch = 0; active_ch < NUM_ACTIVE_CH; active_ch++)
		{
			AI_DATA *aidata = &_nf_action->ai_data[active_ch];

			if(_nf_action->curr_ai[active_ch])
			{
				#if defined DEBUG_AI
				//printf("\033[0;33m %s [DEBUG_AI] ch[%d] is_on\033[0;39m\n", __FUNCTION__,active_ch);
				#endif
				if(aidata->buzzer_act)
					is_on |= TRUE;
			}

			if(_nf_action->rise_ai[active_ch])
			{
				#if defined DEBUG_AI
				printf("\033[0;33m %s [DEBUG_AI] ch[%d] is_rise\033[0;39m\n", __FUNCTION__,active_ch);
				#endif
				if(aidata->buzzer_act)
					is_rise |= TRUE;
			}
		}

		/** DVA Event Check **/
		/*DVA OBJ CNT*/
		for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
		{
			DVA_DATA *dvadata = &_nf_action->dva_data[active_ch];
			int mask = (guint)(1<<active_ch);
			if((_nf_action->curr_dva_idz & mask) || (_nf_action->curr_dva_ipz & mask) || (_nf_action->curr_dva_obj_cnt & mask))
			{
				if(dvadata->buzzer_act)
					is_on |= TRUE;
			}
			if((_nf_action->rise_dva_idz & mask) || (_nf_action->rise_dva_ipz & mask) || (_nf_action->rise_dva_obj_cnt & mask))
			{
				if(dvadata->buzzer_act)
					is_rise |= TRUE;
			}

		}
		
#ifdef DEBUG_ACTION_LOG
#if 0
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_BUZZER] )
			g_message("%s on[%d] rise[%d] motion[%04x][%04x][%04x] alarm[%04x][%04x][%04x] vloss[%04x][%04x][%04x]",
					__FUNCTION__, is_on, is_rise,
					mdata->buzzer_act, _nf_action->curr_motion, _nf_action->rise_motion,
					rdata->buzzer_act, _nf_action->curr_alarm,  _nf_action->rise_alarm,
					vdata->buzzer_act, _nf_action->curr_vloss,  _nf_action->rise_vloss );
#endif
#endif

#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
		for(num_evt=0; num_evt<NF_ACTION_EVENT_NR; num_evt++)
			force_buzzer_on |= bstate->event[num_evt];
#endif

		if(bdata->dwell_time == NF_ACTION_MODE_TRANSPARENT){
		#if defined (DEBUG_BUZZ_TRANS)
		printf("\033[0;33m %s [DEBUG_BUZZ_TRANS] NF_ACTION_MODE_TRANSPARENT \033[0;39m\n", __FUNCTION__);
		#endif
			if(is_on || (bstate->is_manual_test == TRUE)){
			#if defined (DEBUG_BUZZ_TRANS)
			printf("\033[0;33m %s [DEBUG_BUZZ_TRANS] NF_ACTION_MODE_TRANSPARENT IS ON\033[0;39m\n", __FUNCTION__);
			#endif
				bstate->onoff = ON;
			}
			else{
			#if defined (DEBUG_BUZZ_TRANS)
			printf("\033[0;33m %s [DEBUG_BUZZ_TRANS] NF_ACTION_MODE_TRANSPARENT IS OFF\033[0;39m\n", __FUNCTION__);
			#endif
				bstate->onoff = OFF;
			}
		}else{
			if(is_rise){
				bstate->onoff = ON;
				bstate->dwell_in_sec = curr_timeval.tv_sec;
				bstate->dwell_out_sec = curr_timeval.tv_sec
					+ ((bdata->dwell_time == NF_ACTION_UNTIL_KEY ) ? (60*60*24*31) : bdata->dwell_time);
				bstate->keyin_check = keyin_check;
#if 0
				g_message("Rise Buzzer!! in_sec[%ld] out_sec[%ld] Duratioin[%ld]",
						bstate->dwell_in_sec, bstate->dwell_out_sec, bstate->dwell_out_sec - bstate->dwell_in_sec);
#endif
			}else{

				if(bstate->dwell_out_sec > curr_timeval.tv_sec){
					if( bdata->dwell_time == NF_ACTION_UNTIL_KEY )
					{
						if( bstate->keyin_check != keyin_check)
						{
							g_message("%s until_key off [%d][%d]", __FUNCTION__, bstate->keyin_check, keyin_check);

							bstate->onoff = OFF;
							bstate->dwell_in_sec = 0;
							bstate->dwell_out_sec = 0;
#if defined (ENABLE_MOUSE_UNTILKEY_STOP)        /** 091222 by pakkhman **/
							_mouse_key_in_check=FALSE;
#endif
						}

					}else{
						bstate->onoff = ON;
					}
				}
				else{
					bstate->onoff = OFF;
					bstate->dwell_in_sec = 0;
					bstate->dwell_out_sec = 0;
				}
			} // is_rise
		}

#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
		if(force_buzzer_on)
		{
			bstate->onoff = ON;
			bstate->state = OFF;	// if status is off, buzzer is always on!!
		}
#endif
	}	// if(bdata -> buzzer_act)
	else{
		bstate->onoff = OFF;
	}

	if (bstate->is_manual_test == 1) {
		bstate->onoff = ON;
	}

	if(bstate->onoff){
		#if defined (DEBUG_BUZZ_TRANS)
			printf("\033[0;33m %s [DEBUG_BUZZ_TRANS] ON state %d \033[0;39m\n", __FUNCTION__, bstate->state);
		#endif
		if( bstate->state == OFF )
		{
			_buzzer_onoff(ON);
			bstate->state = ON;
#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_BUZZER_OUT] )
				g_message("%s on[%d] rise[%d] %s!!!!", __FUNCTION__,
						is_on, is_rise, bstate->onoff  ? "ON":"OFF" );
#endif
		}

	}else{
		#if defined (DEBUG_BUZZ_TRANS)
			printf("\033[0;33m %s [DEBUG_BUZZ_TRANS] OFF state %d \033[0;39m\n", __FUNCTION__, bstate->state);
		#endif
		if( bstate->state)
		{
			_buzzer_onoff(OFF);
			bstate->state = OFF;
#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_BUZZER_OUT] )
				g_message("%s on[%d] rise[%d] %s!!!!", __FUNCTION__,
						is_on, is_rise, bstate->onoff  ? "ON":"OFF" );
#endif
		}
	}
}

#if defined(ENABLE_ACTION_PTZ_PRESET)
#if 0
typedef struct _NF_PTZ_CMD_T {
	gint            ch;
	NF_PTZ_CMD_E    cmd;
	gint            params[4];
	gint            reserved[2];
} NF_PTZ_CMD;
#endif
static void _preset_action(void)
{
	gint		active_ch=0, active_ch_cam=0, num_alarm=0;
	guint		preset_num=0;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (num_alarm = 0; num_alarm < _nf_action_num_alarm; num_alarm++)
	#else
	for(num_alarm=0; num_alarm<NUM_ALARM; num_alarm++)
	#endif
	{
		ALARM_SENSOR_DATA   *sdata = &_nf_action->sensor_data[num_alarm];

		if(_nf_action->rise_alarm & (0x1ULL<<num_alarm))
		{
			//			g_message("%s LINE[%d] Alarm preset_act[%d]", __FUNCTION__, __LINE__, sdata->preset_act);

			if(sdata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=sdata->preset_num[active_ch_cam];

					//					g_message("%s LINE[%d] ALARM CH[%d] preset_num[%d]", __FUNCTION__, __LINE__, active_ch_cam, preset_num);
					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_ALARM);
				}
			}
		}
	}
#ifdef	SUPPORT_VCA_CAMERA
	/** VCA Event Check **/
	for (active_ch = 0; active_ch < NUM_ACTIVE_CH; active_ch++)
	{
		VCA_DATA *vcadata = &_nf_action->vca_data[active_ch];
	
		if(_nf_action->rise_vca[active_ch])
		{
			if(vcadata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=vcadata->preset_num[active_ch_cam];
					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_VCA);
				}
			}
		}
	}
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	for (active_ch = 0; active_ch < NUM_ACTIVE_CH; active_ch++)
	{
		AI_DATA *aidata = &_nf_action->ai_data[active_ch];
	
		if(_nf_action->rise_ai[active_ch])
		{
			#if defined DEBUG_AI
			printf("\033[0;33m %s [DEBUG_AI] ch[%d] rise_ai\033[0;39m\n", __FUNCTION__,active_ch);
			#endif
			if(aidata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=aidata->preset_num[active_ch_cam];
					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_AI);
				}
			}
		}
	}

	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		MOTION_DATA         *mdata = &_nf_action->motion_data[active_ch];
		VLOSS_DATA          *vdata = &_nf_action->vloss_data[active_ch];

		POS_DATA			*pdata = &_nf_action->pos_data[active_ch];
		DVA_DATA			*dvadata = &_nf_action->dva_data[active_ch];
		if(_nf_action->rise_motion & (1<<active_ch))
		{
			if(mdata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=mdata->preset_num[active_ch_cam];

					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_MOTION);
				}
			}
		}		// End if motion

		if(_nf_action->rise_vloss & (1<<active_ch))
		{
			if(vdata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=vdata->preset_num[active_ch_cam];

					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_VLOSS);
				}
			}
		}		// End if Vloss

		if(_nf_action->rise_pos & (1<<active_ch))
		{
			if(pdata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=pdata->preset_num[active_ch_cam];

					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_POS);
				}
			}
		}		// End if POS
		
		if(_nf_action->rise_dva_idz& (1<<active_ch))
		{
			if(dvadata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=dvadata->preset_num[active_ch_cam];

					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_DVA);
				}
			}
		}// End if DVA_IDZ
		if(_nf_action->rise_dva_ipz& (1<<active_ch))
		{
			if(dvadata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=dvadata->preset_num[active_ch_cam];

					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_DVA);
				}
			}
		}// End if DVA_IPZ
		if(_nf_action->rise_dva_obj_cnt& (1<<active_ch))
		{
			if(dvadata->preset_act)
			{
				for(active_ch_cam=0; active_ch_cam<NUM_ACTIVE_CH; active_ch_cam++)
				{
					preset_num=dvadata->preset_num[active_ch_cam];

					if((preset_num != 0) && (preset_num <= 255))
						_nf_action_preset_set(active_ch_cam, preset_num, NF_ACTION_PTZ_PRESET_DVA);
				}
			}
		}// End if DVA_OBJ_CNT
		
	}	// End For
}

/**
  @brief                  Set PTZ Preset
  @param[in]  ch		   	Channel
  @param[in]  num_preset	Preset Num
  @param[in]  num_event   For Debugging
  @return     void
  */
static void _nf_action_preset_set(gint ch, guint num_preset, guint num_event)
{
	NF_PTZ_CMD	ptz_cmd;
	gchar str_evt[32]={0, };
	ptz_cmd.ch=ch;
	ptz_cmd.cmd=NF_PTZ_CMD_GOTO_PRESET;
	ptz_cmd.params[0]=(gint)num_preset;

	if(num_event == NF_ACTION_PTZ_PRESET_ALARM)
		strcpy(str_evt, "ALARM");
	else if(num_event == NF_ACTION_PTZ_PRESET_MOTION)
		strcpy(str_evt, "MOTION");
	else if(num_event == NF_ACTION_PTZ_PRESET_VLOSS)
		strcpy(str_evt, "VLOSS");
	else if(num_event == NF_ACTION_PTZ_PRESET_POS)
		strcpy(str_evt, "POS");
	else if(num_event == NF_ACTION_PTZ_PRESET_DVA)
		strcpy(str_evt, "DVA");
	else if(num_event, NF_ACTION_PTZ_PRESET_VCA)
		strcpy(str_evt, "VCA");
	else if(num_event, NF_ACTION_PTZ_PRESET_AI) /** IMSI AI BOX **/
		strcpy(str_evt, "AI");

	g_message("%s Event[%s] CH[%d] Preset_Num[%d]", __FUNCTION__, str_evt, ch, num_preset);
	nf_ptz_cmd(&ptz_cmd);
}
#endif

static void _email_init(void)
{
#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_INIT] )
		g_message("%s called", __FUNCTION__ );
#endif
	_email_reset(0);
}

	static void
_email_reset(glong curr_time)
{
	EMAIL_DATA		*e_data = &_nf_action->email_data;
	EMAIL_STATE		*e_state = &_nf_action->email_state;

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_RESET] )
		g_message("%s called", __FUNCTION__ );
#endif

#if 0
	g_assert( NF_ACTION_EMAIL_DATA_MAX_SIZE >= LAST_PROP );
#endif

	memset(&_nf_action->email_state, 0, sizeof(_nf_action->email_state));
	memset(_nf_action->email_send_data, 0, sizeof(EMAIL_EVENT_DATA));

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	if( _nf_action->email_snapshot.data )
	{
		g_free(_nf_action->email_snapshot.data);
		_nf_action->email_snapshot.data = NULL;
	}
	memset(&_nf_action->email_snapshot, 0, sizeof(_nf_action->email_snapshot));
#endif

	if( curr_time == 0)
	{
		GTimeVal        curr_timeval;
		gettimeofday((struct timeval *)&curr_timeval, NULL);
		curr_time = curr_timeval.tv_sec;
		e_state->start_time = curr_time;
		e_state->send_time = 5 + curr_time;
		e_state->div_frequency = 1;
		return ;
	}else{
		e_state->start_time = curr_time;
	}

	if ( e_data->frequency == 0 )
	{
		e_state->send_time = 5 + curr_time;
		e_state->div_frequency = 1;
	}
	else
	{
#if 1
		e_state->send_time = (glong)(e_data->frequency * 60) + curr_time;
		e_state->div_frequency = (e_state->send_time - e_state->start_time) / 60;
#else	// for test
		e_state->send_time = (glong)curr_time+5;
		e_state->div_frequency = 1;
#endif
	}

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_RESET] )
		g_message("%s start[%ld] freq[%d] send[%ld] div_freq[%ld]", __FUNCTION__,
				curr_time, e_data->frequency, e_state->send_time, e_state->div_frequency );
#endif

}

static gint _email_get_lang_id(void)
{
	const char *db_lang = nf_sysdb_get_str_nocopy("disp.osd.lang");

	g_return_val_if_fail( db_lang != NULL, 0);

	if(	strcasecmp( db_lang, "Korean") == 0 )
		return 1;
	else
		return 0;
}

#if defined(ENABLE_EMAIL_DUAL_SERVER)
static gint
_email_send_by_user(time_t send_time)
{
	NF_MAIL_SEND_CONTENT cont[2];

	gint user_count=0, send_usr_cnt_serv1=0, send_usr_cnt_serv2=0, addr_cnt=0, svr_no=0, to_cnt=0;
	gchar user_buff[256]={0, }, *tmp_buff = NULL;
	guint email_serv=0;
	guint ret=0;

	EMAIL_EVENT_DATA	*e_send_data = _nf_action->email_send_data;

	memset(&cont, 0x0, sizeof(cont));

	to_cnt = (gint)nf_sysdb_get_uint("usr.UCNT");

	for ( user_count = 0; user_count < to_cnt; ++user_count )
	{
		snprintf( user_buff, sizeof(user_buff)-1 , "usr.U%d.email_notify", user_count );
		if ( !nf_sysdb_get_bool(user_buff) )
		{
			continue;
		}
		snprintf( user_buff, sizeof(user_buff)-1, "usr.U%d.email", user_count );
		tmp_buff = nf_sysdb_get_str_nocopy(user_buff);
		if ( tmp_buff == NULL )
		{
			g_warning("%s user email error", __FUNCTION__);
			continue;
		}

		if (nf_mail_send_check_email(tmp_buff) == 0 )
		{
			g_warning("%s nf_mail_send_check_email(tmp_buff) error", __FUNCTION__);
			continue;
		}

		snprintf( user_buff, sizeof(user_buff)-1 , "usr.U%d.email_serv", user_count );
		email_serv = nf_sysdb_get_uint(user_buff);

		if( email_serv == 0 )
		{
			snprintf( cont[0].to[send_usr_cnt_serv1], sizeof(cont[0].to[send_usr_cnt_serv1]), "%s", tmp_buff);
			send_usr_cnt_serv1++;
		}
		else if( email_serv == 1  )
		{
			snprintf( cont[1].to[send_usr_cnt_serv2], sizeof(cont[1].to[send_usr_cnt_serv2]), "%s", tmp_buff);
			send_usr_cnt_serv2++;
		}
	}

	sprintf(user_buff, "act.event.email.serv");
	email_serv = nf_sysdb_get_uint(user_buff);

	for(addr_cnt=0; addr_cnt<NF_ACTION_EMAIL_MAX_ADDRESS; addr_cnt++)
	{
		snprintf( user_buff, sizeof(user_buff)-1, "act.event.A%d.address", addr_cnt );
		tmp_buff = nf_sysdb_get_str_nocopy(user_buff);

		if(!nf_mail_send_check_email(tmp_buff))
			continue;

		if(  email_serv == 0 )
		{
			snprintf( cont[0].to[send_usr_cnt_serv1], sizeof(cont[0].to[send_usr_cnt_serv1]), "%s", tmp_buff);
			send_usr_cnt_serv1++;
		}
		else if( email_serv == 1 )
		{
			snprintf( cont[1].to[send_usr_cnt_serv2], sizeof(cont[1].to[send_usr_cnt_serv2]), "%s", tmp_buff);
			send_usr_cnt_serv2++;
		}
	}

	if( send_usr_cnt_serv1 == 0 )
	{
		#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_SEND] )
			g_message("%s send_usr_cnt_serv1[%d]", __FUNCTION__ , send_usr_cnt_serv1 );
		#endif
	}
	else
	{
		cont[0].to_cnt = send_usr_cnt_serv1;
		cont[0].email_serv = NF_ACTION_MAIL_SERVER_FIRST;

		if(_email_send(send_time, &cont[0]))
			ret = (ret | 0x1) & 0x3;
	}

	if( send_usr_cnt_serv2 == 0 )
	{
		#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_SEND] )
				g_message("%s send_usr_cnt_serv1[%d]", __FUNCTION__ , send_usr_cnt_serv2 );
		#endif
	}
	else
	{
		cont[1].to_cnt = send_usr_cnt_serv2;
		cont[1].email_serv = NF_ACTION_MAIL_SERVER_SECNOD;

		if(_email_send(send_time, &cont[1]))
			ret = (ret | 0x2) & 0x3;
	}

	if(ret&0x3)
		return TRUE;
	else
		return FALSE;
}
#endif

#if defined(DEBUG_ACTION_SNAPSHOT)
static int _is_dva_active_on(int ch, char *alg_type)
{
	gchar sysdb_key[32] = {0};
	
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.%s.act", ch, alg_type);
	
	return nf_sysdb_get_bool(sysdb_key);
}
#endif
static int _make_send_string(char * _str, int _length, int _type)
{
		char tmp_str[128] = {0,};
		int first_flag = 0;
		
		if(_str == NULL || _length < 0)
		{
			printf("%s CHECK PARAM length[%d]\n", __FUNCTION__, _length);
			return 0;
		}
		
		memset(_str, 0x0, _length);
		if(_type == 0)
		{
			int n_ch = 0;
			strcpy(_str, " ( CH : ");
			for(n_ch = 0; n_ch < NUM_ACTIVE_CH; n_ch++)
			{
				if((NF_AIBOX_CONN_FAILED <= _nf_action->ai_keep_alive_state[n_ch]) && (NF_AIBOX_STREAM_CONN_FAILED >= _nf_action->ai_keep_alive_state[n_ch]))
				{
					memset(tmp_str, 0x0, sizeof(tmp_str) / sizeof(char));
					sprintf(tmp_str,"%d",n_ch+1);
					if(first_flag)
						strcat(_str,", ");
					else
						first_flag = 1;
					strcat(_str,tmp_str);	
				}
			}
			if(first_flag)
				strcat(_str," )");
			else
				strcpy(_str, "( NONE )");
		}
		printf("\033[0;36m %s %s\033[0;39m\n", __FUNCTION__, _str);
		return 1;
}

static void _replace_ai_send_string(char *str_des, char *str_src, int size)
{
	int idx = 0;	
	if(str_src[0] == '\n')
	{
		memcpy(str_src, str_src+1, size-1);
		size--;
	}
	
	for(idx = 0; idx < size; idx++)
	{
		if(str_src[idx] == '\n')
		{
			if(idx == size - 1) 
				str_des[idx] = '\0';
			else
				str_des[idx] = ',';
		}
		else
			str_des[idx] = str_src[idx];		
	}
}
static void _make_ai_send_string(gchar **_pos, gchar *_buffer, void* _evt_data ,gint _evt_type)
{
		if(_evt_type == NF_ACTION_AI_GENERIC)
		{
			ai_generic_event_t *data = (ai_generic_event_t*)_evt_data;
			int evt_idx = 0;
			for( evt_idx = 0; evt_idx < EMAIL_EVENT_TYPE5_GENERIC_EVT_INDAX_MAX; evt_idx++ )
			{
				#if defined(DEBUG_AI_DATA)
					//printf("\033[0;37m %s DEBUG_AI_DATA %s\033[0;39m\n", __FUNCTION__, data[evt_idx].generic_type);
				#endif
				if( strlen(data[evt_idx].caption) != 0 )
				{
					printf("\033[0;31m %s DEBUG_AI_DATA WRITE [%d]%s!\033[0;39m\n", __FUNCTION__, evt_idx, data[evt_idx].caption);
					(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "     %d. CAPTION( %s )", evt_idx+1, data[evt_idx].caption);				
					(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "-TITLE( %s )", data[evt_idx].title );
					(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "-DES( %s )  ", data[evt_idx].description );
				}
			}
		}
                else if(_evt_type == NF_ACTION_AI_FR)
		{
			ai_fr_event_t *data = (ai_fr_event_t*)_evt_data;
			int infoIdx = 0;
			for(infoIdx = 0; infoIdx < data->info_cnt; infoIdx++)
			{
				if(strlen(data->info[infoIdx].name) != 0)
				{
					(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "%d. NAME (%s) ",
											infoIdx+1 ,data->info[infoIdx].name);
				}
				
				if(data->info[infoIdx].group_cnt > 0)
				{
					int groupIdx = 0;
					for(groupIdx = 0; groupIdx < data->info[infoIdx].group_cnt; groupIdx++)
					{
						if(strlen(data->info[infoIdx].group_name[groupIdx]) != 0)
						{
							if(groupIdx == 0)
							{
								(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "GROUP (%s)",
											data->info[infoIdx].group_name[groupIdx]);
							}
							else
							{
								(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), ", (%s)",
									data->info[infoIdx].group_name[groupIdx]);
							}
						}
					}
				}
				(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "   %c   ",
									'|');
			}
			
		}
		else if(_evt_type == NF_ACTION_AI_LPR)
		{
			ai_lpr_event_t *data = (ai_lpr_event_t*)_evt_data;
			#if 0
			int infoIdx = 0;
			for(infoIdx = 0; infoIdx < data->info_cnt; infoIdx++)
			{
				if(strlen(data->info[infoIdx].number) != 0)
				{
					(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "%d. NUMBER (%s) ",
											infoIdx+1 ,data->info[infoIdx].number);
				}
				
				if(strlen(data->info[infoIdx].country) != 0)
				{
					(*_pos) s+= snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "COUNTRY (%s) ",
											infoIdx+1 ,data->info[infoIdx].country);
				}
				
				(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "   %c   ",
									'|');
			}
			#else
			if(strlen(data->lp_text) != 0)
			{
				(*_pos) += snprintf((*_pos), sizeof(_buffer)-((guint)(*_pos)-(guint)_buffer), "NUMBER (%s) ",
										 data->lp_text);
			}
			#endif
		}
}

static void
_save_ai_generic_evt(EMAIL_EVENT_DATA *e_send_data, int ch)
{
	int evt_idx = 0;
	int replace_flag = 0;
	for(evt_idx = 0; evt_idx < EMAIL_EVENT_TYPE5_GENERIC_EVT_INDAX_MAX; evt_idx++)
	{
		#if defined(DEBUG_AI_DATA)
			printf("\033[0;36m %s DEBUG_AI_DATA %d. DES:%s SRC:%s\033[0;39m\n", __FUNCTION__, evt_idx, e_send_data->type5.st_generic_evt[ch][evt_idx].caption, _nf_action->ai_generic_data[ch].caption);
		#endif
		if(NULL != strstr(e_send_data->type5.st_generic_evt[ch][evt_idx].caption, _nf_action->ai_generic_data[ch].caption))
			replace_flag = 1;
		if(strlen(e_send_data->type5.st_generic_evt[ch][evt_idx].caption) == 0 || replace_flag)
		{
			#if defined(DEBUG_AI_DATA)
				printf("\033[0;34m %s DEBUG_AI_DATA %d. %s COPY\033[0;39m\n", __FUNCTION__, evt_idx,  replace_flag == 1 ? "REPLACE" : "NEW");
			#endif
			strcpy( e_send_data->type5.st_generic_evt[ch][evt_idx].caption, _nf_action->ai_generic_data[ch].caption );			

			if( strlen( _nf_action->ai_generic_data[ch].title ) == 0 )
				strcpy( e_send_data->type5.st_generic_evt[ch][evt_idx].title , "NONE");
			else
				_replace_ai_send_string(e_send_data->type5.st_generic_evt[ch][evt_idx].title , _nf_action->ai_generic_data[ch].title, strlen(_nf_action->ai_generic_data[ch].title));
							
			if( strlen( _nf_action->ai_generic_data[ch].title ) == 0 )
				strcpy( e_send_data->type5.st_generic_evt[ch][evt_idx].description, "NONE");
			else
				_replace_ai_send_string(e_send_data->type5.st_generic_evt[ch][evt_idx].description , _nf_action->ai_generic_data[ch].description, strlen(_nf_action->ai_generic_data[ch].description));

			e_send_data->type5.st_generic_evt[ch][evt_idx].timestamp = _nf_action->ai_generic_data[ch].timestamp;
			e_send_data->type5.st_generic_evt[ch][evt_idx].timestampl = _nf_action->ai_generic_data[ch].timestampl;
			break;
		}
	}
}


#if defined(ENABLE_EMAIL_DUAL_SERVER)
static gint
_email_send(time_t send_time, NF_MAIL_SEND_CONTENT *cont_msg)
#else
static gint
_email_send(time_t send_time)
#endif
{
	NF_MAIL_SEND_CONTENT cont;

	gint ch_num=0, event_num=0, min_num=0, event_cate=0;
	time_t e_time = send_time;
	struct tm st_buff;
	struct tm *st = &st_buff;
	gchar time_buff[256];

	gchar buffer[NF_MAIL_SEND_MAX_CONTENTS]={0, }, user_buff[256]={0, };
	gchar *pos=buffer, *tmp_buff = NULL;
	gchar alarm_db_key[NF_MAIL_SEND_MAX_ALARM_NAME_DB_KEY] = {0,};
	gchar *alarm_name=NULL;
	gint user_count=0, send_usr_cnt=0, addr_cnt=0, cnt=0;
	gboolean sndflag = FALSE, is_dst=FALSE;
	gboolean booting = FALSE;
	gint lang_id = 0, num_alarm=0;
	gint ret=0, title_print=0;
	gboolean is_videcon=FALSE;
	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		gboolean is_single_send=FALSE;
		char *pos_single_send = NULL, time_str[50]={0, };
		time_t time_utc;
		struct tm *tm_ptr;
	#endif
	#if defined(DEBUG_ACTION_SNAPSHOT)
	gint dlvaFlag = 0;
	#endif
	EMAIL_SNAPSHOT_DATA	*email_snapshot = &_nf_action->email_snapshot;
	EMAIL_EVENT_DATA	*e_send_data = _nf_action->email_send_data;

	POS_TEXT_DATA		*p_text_data = _nf_action->email_pos_text_data;

	#if 1
		lang_id = _email_get_lang_id();

		if(lang_id >= NF_ACTION_LANG_ID_MAX)
		{
			g_warning("%s Email LangID is False", __FUNCTION__);
			return FALSE;
		}
	#endif

	memset(buffer, 0, sizeof(buffer));
	tmp_buff = nf_sysdb_get_str_nocopy("sys.info.vendor");

	if(strncmp(NF_ACTION_VENDOR_VIDECON, tmp_buff, strlen(NF_ACTION_VENDOR_VIDECON)) == 0)
		is_videcon=TRUE;
	else
		is_videcon=FALSE;

#if defined(ENABLE_EMAIL_DUAL_SERVER)
	if(time(&time_utc) == -1)
		g_warning("%s time error!!", __FUNCTION__);
	tm_ptr = gmtime(&time_utc);
	sprintf(time_str, "%d-%d-%d %d:%d:%d", tm_ptr->tm_year+1900,
			tm_ptr->tm_mon+1,
			tm_ptr->tm_mday,
			tm_ptr->tm_hour,
			tm_ptr->tm_min,
			tm_ptr->tm_sec);

	tmp_buff = nf_sysdb_get_str_nocopy("sys.info.vendor");

	if(strncmp(NF_ACTION_VENDOR_VIDECON, tmp_buff, strlen(NF_ACTION_VENDOR_VIDECON)) == 0)
		is_videcon=TRUE;
	else
		is_videcon=FALSE;

	memcpy(&cont, cont_msg, sizeof(NF_MAIL_SEND_CONTENT));

	if(cont.email_serv == NF_ACTION_MAIL_SERVER_FIRST)
	{
		is_single_send=(gboolean)nf_sysdb_get_uint("net.email.individual");
	}
	else if(cont.email_serv == NF_ACTION_MAIL_SERVER_SECNOD)
	{
		is_single_send=(gboolean)nf_sysdb_get_uint("net.email_2nd.individual");
	}

#else
	memset(&cont, 0, sizeof(cont));

	cont.to_cnt = (gint)nf_sysdb_get_uint("usr.UCNT");
	for ( user_count = 0; user_count < cont.to_cnt; ++user_count )
	{
		snprintf( user_buff, sizeof(user_buff)-1 , "usr.U%d.email_notify", user_count );
		if ( !nf_sysdb_get_bool(user_buff) )
		{
			continue;
		}
		snprintf( user_buff, sizeof(user_buff)-1, "usr.U%d.email", user_count );
		tmp_buff = nf_sysdb_get_str_nocopy(user_buff);
		if ( tmp_buff == NULL )
		{
			g_warning("%s user email error", __FUNCTION__);
			continue;
		}

		if (nf_mail_send_check_email(tmp_buff) == 0 )
		{
			g_warning("%s nf_mail_send_check_email(tmp_buff) error", __FUNCTION__);
			continue;
		}

		snprintf( cont.to[send_usr_cnt], sizeof(cont.to[user_count]), "%s", tmp_buff);

			++send_usr_cnt;		// User Count�� ������� E-
	}

	for(addr_cnt=0; addr_cnt<NF_ACTION_EMAIL_MAX_ADDRESS; addr_cnt++)
	{
		snprintf( user_buff, sizeof(user_buff)-1, "act.event.A%d.address", addr_cnt );
		tmp_buff = nf_sysdb_get_str_nocopy(user_buff);

		if(!nf_mail_send_check_email(tmp_buff))
			continue;

		snprintf( cont.to[send_usr_cnt], sizeof(cont.to[send_usr_cnt]), "%s", tmp_buff);

		++send_usr_cnt;
	}

	if( send_usr_cnt == 0 )
	{
		#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_SEND] )
				g_message("%s send_usr_cnt[%d]", __FUNCTION__ , send_usr_cnt );
		#endif
		return 0;
	}
	cont.to_cnt = send_usr_cnt;
#endif

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s !!\n",
			_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_TITLE] );	//DVR Event E-Mail

	is_dst = _nf_action->is_dst;
	if( is_dst == 0 && nf_datetime_is_dst( e_time ) )
		e_time -= 3600;

	{
		char date_str[128], time_str[128];

		guint datefmt = nf_sysdb_get_uint("sys.date.dateform");
		guint timefmt = nf_sysdb_get_uint("sys.date.timeform");

		localtime_r(&e_time, st);

		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(date_str, sizeof(date_str)-1, "%04d/%02d/%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d %s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		snprintf(time_buff, sizeof(time_buff)-1, "%s %s", date_str, time_str);
	}

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s : %.24s\n",
			_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_TIME], time_buff);

	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		if ( is_single_send )
		{
			if(is_videcon)
			{
				pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s_GMT : %.24s\n",
						_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_TIME], time_str);
			}
		}
	#endif

	tmp_buff = nf_sysdb_get_str_nocopy("sys.info.sysid");
	if (tmp_buff == NULL)
	{
		g_warning("%s sys.info.sysid error", __FUNCTION__);
		return -1;
	}

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s : %s\n\n\n",
			_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_SYSTEM_ID], tmp_buff);
	snprintf( cont.subject, sizeof(cont.subject), "%s (%s)",
			_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_TITLE], tmp_buff );

	tmp_buff = nf_sysdb_get_str_nocopy("net.email.from");
	if (tmp_buff == NULL )
	{
		g_warning("%s net.email.from error", __FUNCTION__);
		return -1;
	}

	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		pos_single_send = pos;
	#endif

	snprintf( cont.from, sizeof(cont.from), "%s", tmp_buff);
	for(event_num=0; event_num<EMAIL_EVENT_TYPE0_NR; event_num++)
	{
		gboolean is_title_print=FALSE;
		gboolean is_send=FALSE;
		gboolean is_first=TRUE;
		#if defined(ENABLE_EMAIL_DUAL_SERVER)
		if ( is_single_send ){
				pos = pos_single_send;
		}
		#endif
		
		if(e_send_data->type0.rise[event_num][63])
		{
			#if defined(DEBUG_EMAIL_ARM)
				printf("\033[0;34m %s e_send_data->type0.rise[%d][63] = [%d]\033[0;39m\n", __FUNCTION__,event_num,e_send_data->type0.rise[event_num][63]);
			#endif
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
			if(is_videcon)
			{
				if ( is_single_send  )
				{

					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",
							_ACTION_STR_TABLE_TYPE0[event_num]);
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");

					strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

					// patch
					nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
					nf_mail_send_request( &cont, NULL);

					pos = pos_single_send;
					sndflag = TRUE;

				}
				else
					goto nf_action_email_default_str_type0;
			}
			else
			{
				nf_action_email_default_str_type0:
			#endif


				if(is_first)
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",
							_ACTION_STR_TABLE_TYPE0[event_num]);

					is_first=FALSE;
				}
				else
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), ",%s ",
							_ACTION_STR_TABLE_TYPE0[event_num]);
				sndflag = TRUE;
				is_send = TRUE;
			}
	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		}
	#endif
		if(is_send)
		{
			pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");

	#if defined(ENABLE_EMAIL_DUAL_SERVER)
			if ( is_single_send ){
				strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

				// patch
				nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
				nf_mail_send_request( &cont, NULL);
			}
	#endif
		}
	}
	
	// Type4 Event
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		gboolean is_title_print=FALSE;
		gboolean is_send=FALSE;
		gboolean is_first=TRUE;
		gboolean is_animal_first = TRUE;
		char is_first_obj_cnt = 7;

		for( event_num = 0; event_num < EMAIL_EVENT_TYPE4_NR; event_num++ )
		{
			gint event_max=0;
			gboolean is_event_print=FALSE;
			gint lang_id_event=0;
			if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
			{
				event_max=NF_ACTION_DVA_IDZ_NR;
				lang_id_event=LANG_ID_DVA_IDZ;
			}
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
			{
				event_max=NF_ACTION_DVA_IPZ_NR;
				lang_id_event=LAND_ID_DVA_IPZ;
			}
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT)
			{
				event_max=NF_ACTION_DVA_IDZ_NR;
				lang_id_event=LANG_ID_DVA_OBJ_CNT;
			}
			
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				if ( is_single_send ){
					pos = pos_single_send;
				}
			#endif

			for( event_cate=0; event_cate<event_max; event_cate++)
			{
				gint erase_length = 0;
				if (e_send_data->type4.rise[ch_num][event_num][event_cate][63])
				{
					#if defined(DEBUG_ACTION_SNAPSHOT)
					dlvaFlag = 1;
					if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (0 == _is_dva_active_on(ch_num, "idz")))
					{
						printf("\033[0;33m %s [DEBUG_ACTION_SNAPSHOT] dva idz down!! \033[0;39m\n", __FUNCTION__);
					}
					if((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (0 == _is_dva_active_on(ch_num, "ipz")))
					{
						printf("\033[0;33m %s [DEBUG_ACTION_SNAPSHOT] dva ipz down!! \033[0;39m\n", __FUNCTION__);
					}
					#endif
					#if 0
						g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
					#if defined(ENABLE_EMAIL_DUAL_SERVER)
						if(is_videcon)
						{
							if ( is_single_send )
							{
								erase_length = snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
								pos += erase_length;

								erase_length = snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
										_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
								pos += erase_length;

								if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
								{
									if(!is_animal_first)
									{
										//pos -= erase_length;
										continue;
									}
									else
										is_animal_first = FALSE;
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
								}
								else if(((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate == NF_ACTION_DVA_IDZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IDZ_BICYCLE))
							 	 	 || ((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (event_cate == NF_ACTION_DVA_IPZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IPZ_BICYCLE)))
								{
									if(lang_id == 0)
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","bike");
									else if(lang_id == 1)
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","������");
								}
								/*DVA OBJ CNT*/
								else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
								{
									if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_ANIMAL) & 0x01))
									{
										pos -= erase_length;
										#if defined(DEBUG_DVA_OBJ_CNT)
										printf("\033[0;35m %s ANMIMAL CONTINUE \033[0;39m\n", __FUNCTION__);
										#endif
										continue;
									}
									else
										is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_ANIMAL));
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
								}
								else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate >= NF_ACTION_DVA_IDZ_BICYCLE && event_cate <= NF_ACTION_DVA_IDZ_CAR))
								{
									if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_VEHICLE) & 0x01))
									{
										pos -= erase_length;
										#if defined(DEBUG_DVA_OBJ_CNT)
										printf("\033[0;35m %s VEHICLE CONTINUE \033[0;39m\n", __FUNCTION__);
										#endif
										continue;
									}
									else
										is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_VEHICLE));
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_VEHICLE]);
								}
								else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate == NF_ACTION_DVA_IDZ_HUMAN))
								{
									if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_HUMAN) & 0x01))
									{
										pos -= erase_length;
										#if defined(DEBUG_DVA_OBJ_CNT)
										printf("\033[0;35m %s HUMAN CONTINUE \033[0;39m\n", __FUNCTION__);
										#endif
										continue;
									}
									else
										is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_HUMAN));
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE_TYPE4[lang_id][EMAIL_EVENT_TYPE4_DVA_IDZ][NF_ACTION_DVA_IDZ_HUMAN]);
								}
								else
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",_ACTION_STR_TABLE_TYPE4[lang_id][event_num][event_cate]);
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");

								strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

								// patch
								nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
								nf_mail_send_request( &cont, NULL);
								pos = pos_single_send;
							}
							else
								goto nf_action_email_default_str_type4;
						}
						else
						{
							nf_action_email_default_str_type4:
					#endif

						if(!is_title_print)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
									_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
							is_title_print=TRUE;
						}

						if(!is_event_print)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
									_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
							is_event_print=TRUE;
						}

						if(is_first)
						{
							if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
							{
								if(!is_animal_first)
									continue;
								else
									is_animal_first = FALSE;
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
							}
							else if(((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate == NF_ACTION_DVA_IDZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IDZ_BICYCLE))
							 	 || ((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (event_cate == NF_ACTION_DVA_IPZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IPZ_BICYCLE)))
							{
								if(lang_id == 0)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","bike");
								else if(lang_id == 1)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","������");
							}
							else
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",_ACTION_STR_TABLE_TYPE4[lang_id][event_num][event_cate]);

							is_first=FALSE;
						}
						else
						{
							if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
							{
								if(!is_animal_first)
									continue;
								else
									is_animal_first = FALSE;
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
							}
							else if(((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate == NF_ACTION_DVA_IDZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IDZ_BICYCLE))
							 	 || ((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (event_cate == NF_ACTION_DVA_IPZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IPZ_BICYCLE)))
							{
								if(lang_id == 0)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","bike");
								else if(lang_id == 1)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","������");
							}
							/*DVA OBJ CNT*/
							else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
							{
								if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_ANIMAL) & 0x01))
									continue;
								else
									is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_ANIMAL));
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
							}
							else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate >= NF_ACTION_DVA_IDZ_BICYCLE && event_cate <= NF_ACTION_DVA_IDZ_CAR))
							{
								if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_VEHICLE) & 0x01))
									continue;
								else
									is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_VEHICLE));
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_VEHICLE]);
							}
							else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate == NF_ACTION_DVA_IDZ_HUMAN))
							{
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE_TYPE4[lang_id][EMAIL_EVENT_TYPE4_DVA_IDZ][NF_ACTION_DVA_IDZ_HUMAN]);
							}
							else
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",_ACTION_STR_TABLE_TYPE4[lang_id][event_num][event_cate]);
						}
						sndflag = TRUE;
						is_send = TRUE;
					}
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				}
			#endif
			}

			if(is_send)
			{
				pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");

			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				if ( is_single_send ){
					strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

					// patch
					nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
					nf_mail_send_request( &cont, NULL);
				}
			#endif
			}
		}
	}
	// 	Type5 Event
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			gboolean is_title_print=FALSE;
			gboolean is_send=FALSE;
			gboolean is_first=TRUE;
	
			for( event_num = 0; event_num < EMAIL_EVENT_TYPE5_NR; event_num++ )
			{
				gint event_max=0;
				gboolean is_event_print=FALSE;
				gint lang_id_event=0;
	
				if(event_num == EMAIL_EVENT_TYPE5_AI)
				{
					event_max=NF_ACTION_AI_NR;
				}
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						pos = pos_single_send;
					}
			#endif
				
				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					#if defined(VIDECON)
					int idx_ai_evt_type = 0;
					#endif
				
					if(event_num == EMAIL_EVENT_TYPE5_AI)
					{
						if(event_cate == NF_ACTION_AI_GENERIC)
						{
							lang_id_event=LANG_ID_AI_GENERIC;
							#if defined(VIDECON)
								idx_ai_evt_type = NF_ACTION_IDX_GENERIC_EVENT;
							#endif
						}
                                                else if(event_cate == NF_ACTION_AI_FR)
							lang_id_event=LANG_ID_AI_FR;
						else if(event_cate == NF_ACTION_AI_LPR)
							lang_id_event=LANG_ID_AI_LPR;
						else
						{
							lang_id_event=LANG_ID_AI;
							#if defined(VIDECON)
								idx_ai_evt_type = NF_ACTION_IDX_HUMAN_VEHICLE_DETECTOR;
							#endif
						}
					}
					
					if (e_send_data->type5.rise[ch_num][event_num][event_cate][63])
					{
					#if defined (DEBUG_AI)
							printf("\033[0;35m %s Line[%d] [DEBUG_AI] ch : %d	event_num : %d event_cate : %d \033[0;39m\n", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
							//g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
					#if defined(ENABLE_EMAIL_DUAL_SERVER)
							if(is_videcon)
							{
								if ( is_single_send )
								{
	
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
									#if defined(VIDECON)				
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s)    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_AI], _ACTION_STR_TABLE_DLVA_EVENT_TYPE[idx_ai_evt_type]);
									#else
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
									#endif
                                                                        if((event_num == EMAIL_EVENT_TYPE5_AI) && (event_cate == NF_ACTION_AI_GENERIC))
									{									
										if(event_cate == NF_ACTION_AI_GENERIC)
										{																			
											_make_ai_send_string(&pos, buffer, (void *)e_send_data->type5.st_generic_evt[ch_num],NF_ACTION_AI_GENERIC);
										}
									}
									else if((event_num == EMAIL_EVENT_TYPE5_AI) && (event_cate == NF_ACTION_AI_FR || event_cate == NF_ACTION_AI_LPR))
									{
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
										if(event_cate == NF_ACTION_AI_FR)
										_make_ai_send_string(&pos, buffer, (void *)&_nf_action->ai_fr_data[ch_num],NF_ACTION_AI_FR);
										else if(event_cate == NF_ACTION_AI_LPR)
											_make_ai_send_string(&pos, buffer, (void *)&_nf_action->ai_lpr_data[ch_num],NF_ACTION_AI_LPR);
									}
									else
									{
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s) ",
											_ACTION_STR_TABLE_TYPE5[lang_id][event_num][event_cate], e_send_data->type5.text[ch_num][event_num][event_cate]);
									}
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
	
									strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );
	
									// patch
									nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
									nf_mail_send_request( &cont, NULL);
	
									pos = pos_single_send;
									sndflag = TRUE;
	
								}
								else
									goto nf_action_email_default_str_type5;
							}
							else
							{
								nf_action_email_default_str_type5:
					#endif
	
							if(!is_title_print)
							{
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
								is_title_print=TRUE;
							}
                                                        if((event_num == EMAIL_EVENT_TYPE5_AI) && (event_cate == NF_ACTION_AI_GENERIC))
							{
								if(is_event_print)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
									
								#if defined(VIDECON)				
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s)    :   ",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_AI], _ACTION_STR_TABLE_DLVA_EVENT_TYPE[idx_ai_evt_type]);
								#else
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
										_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
								#endif	
								if(event_cate == NF_ACTION_AI_GENERIC)
									_make_ai_send_string(&pos, buffer, (void *)e_send_data->type5.st_generic_evt[ch_num], NF_ACTION_AI_GENERIC);
							}
							else if((event_num == EMAIL_EVENT_TYPE5_AI) && (event_cate == NF_ACTION_AI_FR || event_cate == NF_ACTION_AI_LPR))
							{
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);	
								if(event_cate == NF_ACTION_AI_FR)
									_make_ai_send_string(&pos, buffer, (void *)&_nf_action->ai_fr_data[ch_num],NF_ACTION_AI_FR);
								else if(event_cate == NF_ACTION_AI_LPR)
									_make_ai_send_string(&pos, buffer, (void *)&_nf_action->ai_lpr_data[ch_num],NF_ACTION_AI_LPR);
							}
							else
							{
								if(!is_event_print)
								{
									#if defined(VIDECON)				
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s)    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_AI], _ACTION_STR_TABLE_DLVA_EVENT_TYPE[idx_ai_evt_type]);
									#else
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
									#endif	
									is_event_print=TRUE;
								}
		
								if(is_first)
								{
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s) ",
											_ACTION_STR_TABLE_TYPE5[lang_id][event_num][event_cate], e_send_data->type5.text[ch_num][event_num][event_cate]);
		
									is_first=FALSE;
								}
								else
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), ",%s(%s) ",
											_ACTION_STR_TABLE_TYPE5[lang_id][event_num][event_cate], e_send_data->type5.text[ch_num][event_num][event_cate]);
							}
							sndflag = TRUE;
							is_send = TRUE;
						}
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
					}
			#endif
				}
	
				if(is_send)
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );
	
						// patch
						nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
						nf_mail_send_request( &cont, NULL);
					}
			#endif
				}
			}
		}
	// 	Type3 Event
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		gboolean is_title_print=FALSE;
		gboolean is_send=FALSE;
		gboolean is_first=TRUE;

		for( event_num = 0; event_num < EMAIL_EVENT_TYPE3_NR; event_num++ )
		{
			gint event_max=0;
			gboolean is_event_print=FALSE;
			gint lang_id_event=0;

			#if defined(SUPPORT_VCA_CAMERA)
				if(event_num == EMAIL_EVENT_TYPE3_VCA)
				{
					event_max=NF_ACTION_VCA_NR;
					lang_id_event=LANG_ID_VCA;
				}
			#endif
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				if ( is_single_send ){
					pos = pos_single_send;
				}
			#endif
			
			for( event_cate=0; event_cate<event_max; event_cate++)
			{
				
				if (e_send_data->type3.rise[ch_num][event_num][event_cate][63])
				{
					#if 1
						printf("\033[0;35m %s Line[%d] ch : %d  event_num : %d event_cate : %d \033[0;39m\n", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
						g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
					#if defined(ENABLE_EMAIL_DUAL_SERVER)
						if(is_videcon)
						{
							if ( is_single_send  )
							{

								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
										_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",
										_ACTION_STR_TABLE_TYPE3[lang_id][event_num][event_cate]);
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");

								strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

								// patch
								nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
								nf_mail_send_request( &cont, NULL);

								pos = pos_single_send;

							}
							else
								goto nf_action_email_default_str_type3;
						}
						else
						{
							nf_action_email_default_str_type3:
					#endif

						if(!is_title_print)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
									_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
							is_title_print=TRUE;
						}

						if(!is_event_print)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
									_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
							is_event_print=TRUE;
						}

						if(is_first)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",
									_ACTION_STR_TABLE_TYPE3[lang_id][event_num][event_cate]);

							is_first=FALSE;
						}
						else
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), ",%s ",
									_ACTION_STR_TABLE_TYPE3[lang_id][event_num][event_cate]);
						sndflag = TRUE;
						is_send = TRUE;
					}
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				}
			#endif
			}

			if(is_send)
			{
				pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");

			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				if ( is_single_send ){
					strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

					// patch
					nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
					nf_mail_send_request( &cont, NULL);
				}
			#endif
			}
		}
	}

	// 	Type2 Event
	for( event_num = EMAIL_EVENT_TYPE2_HDD; event_num < EMAIL_EVENT_TYPE2_NR; event_num++ )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++)
		{
			if (e_send_data->type2.rise[event_num][event_cate][63])
			{
				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						pos = pos_single_send;
					}
				#endif

				if(((event_num == EMAIL_EVENT_TYPE2_NET) && (event_cate == NF_ACTION_NET_EVENT_LOGON_FAIL)) ||
						((event_num == EMAIL_EVENT_TYPE2_SYSTEM) && (event_cate == NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL)))
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s	 %d	 %s\n",  
							_ACTION_STR_TABLE_TYPE2[lang_id][event_num][event_cate],
							e_send_data->type2.rise[event_num][event_cate][63],
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );
				}
				else
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %s",
							_ACTION_STR_TABLE_TYPE2[lang_id][event_num][event_cate],
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_OCCUR] );
					#if 0
					if( event_cate == NF_ACTION_NET_EVENT_TROUBLE_AI_BOX )
					{
						char str_riseCh[128] = {0,};
						_make_send_string(str_riseCh, 128, 0);
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s", str_riseCh);
					}
					#endif
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%c", '\n');
				}

				sndflag = TRUE;

				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

						nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
						nf_mail_send_request( &cont, NULL);
					}
				#endif
			}
		}
	}

	if (e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[NF_ACTION_REC_EVENT_PANIC][63])
	{
#if defined(ENABLE_EMAIL_DUAL_SERVER)
		if ( is_single_send ){
			pos = pos_single_send;
		}
#endif
		printf("check_chck point 1 \n", __LINE__); 				
		pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s	%s\n\n",
				_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_REC_PANIC],
				_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_OCCUR] );
		#if defined(ENABLE_EMAIL_DUAL_SERVER)
			if ( is_single_send ){
				strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

				nf_mail_send_request( &cont, NULL);
			}
			else
		sndflag = TRUE;
		#else
			sndflag = TRUE;
		#endif
	}

	#if defined(ENABLE_ACTION_REC_STOP)
		if (e_send_data->type1[EMAIL_EVENT_TYPE1_REC_STOP].rise[NF_ACTION_REC_STOP_EVENT_STOP][63])
		{
			#if defined(ENABLE_EMAIL_DUAL_SERVER)
				if ( is_single_send ){
					pos = pos_single_send;
				}
			#endif

			pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s	%s\n\n",
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_REC_STOP],
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_OCCUR] );

#if defined(ENABLE_EMAIL_DUAL_SERVER)
		if ( is_single_send ){
			strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );

			nf_mail_send_request( &cont, NULL);
		}
				else
					sndflag = TRUE;
			#else
				sndflag = TRUE;
#endif
	}
	#endif

	// Type1 Event in linked camera
	for( ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++ )
	{
		title_print=FALSE;

		for( event_num = EMAIL_EVENT_TYPE1_SENSOR; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			if((event_num != EMAIL_EVENT_TYPE1_SENSOR) && (event_num != EMAIL_EVENT_TYPE1_POS))
				continue;

		#if defined(ENABLE_SENSOR_IPCAM)
			if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
			{
				gchar tmp_key[64]={0, }, *str=NULL;

				/**
				  When IPCAM+DVR, This is DVR String
				 **/
				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				if (ch_num < _nf_action_num_nvr_alarm)
				#else
				if(ch_num < NUM_ALARM_DVR)
				#endif
				{
					gint index=0;

					index=NUM_ALARM_IPCAM+ch_num;

					gchar str_lcamera[64] = {0, };
					gint cnt_ch=0;
					guint mask_lcamera = _nf_action->sensor_data[index].mask_lcamera;

					#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					if (index < _nf_action_num_alarm)
					#else
					if(index < NUM_ALARM)
					#endif
					{
						if (e_send_data->type1[event_num].rise[index][63])
						{
						#if defined(ENABLE_EMAIL_DUAL_SERVER)
							if ( is_single_send ){
								pos = pos_single_send;
							}
						#endif

						#if defined(ENABLE_EMAIL_DUAL_SERVER)
							if(is_videcon)
							{
								if ( is_single_send  )
								{
									#if 0
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s CH%d (NVR)\n",
												_ACTION_STR_TABLE_TYPE1[lang_id][event_num], ch_num+1);
									#else		// For Videcon
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "< %s %2d >\n",
														_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], ch_num+1);

										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s (NVR)\n",
													_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
													e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[index][63],
													_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
									#endif
								}
								else
									goto nf_action_email_default_str_type1;
							}
							else
							{
								nf_action_email_default_str_type1:
						#endif
								if(is_videcon)
								{
									if(title_print == FALSE)
									{
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "< %s %2d >\n",
													_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], ch_num+1);
										title_print = TRUE;
									}

									#if 0
										g_message("%s line%d Alarm NVR chnum %d index %d rise %d", 
												__FUNCTION__, __LINE__, ch_num, index,
												e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[index][63]);
									#endif
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s (NVR)\n",
												_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
												e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[index][63],
												_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
									title_print=FALSE;
								}
								else
								{
									sprintf(tmp_key, "alarm.sensor.S%d.name", index);
									str = nf_sysdb_get_str_nocopy(tmp_key);

									for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
									{
										if((mask_lcamera >> cnt_ch) & 0x1)
											sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
									}

									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%-16.16s : %3d %s - %s\n",
											str, e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[index][63],
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR], str_lcamera);
								}
						#if defined(ENABLE_EMAIL_DUAL_SERVER)
							}
						#endif

							sndflag = TRUE;

						#if defined(ENABLE_EMAIL_DUAL_SERVER)
							if ( is_single_send ){
								strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );
								nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
								nf_mail_send_request( &cont, NULL);
							}

						#endif
						}
					}
				}
			}
		#endif

			if (e_send_data->type1[event_num].rise[ch_num][63])
			{
				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						pos = pos_single_send;
					}
				#endif

				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if(is_videcon)
					{
						if ( is_single_send  )
						{
							// title print
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "< %s %2d >\n",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], ch_num+1);

							if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
							{
								#if 0
									gchar tmp_key[64]={0, }, *str=NULL, str_lcamera[64] = {0, };
									gint cnt_ch=0;
									guint mask_lcamera = _nf_action->sensor_data[ch_num].mask_lcamera;

									sprintf(tmp_key, "alarm.sensor.S%d.name", ch_num);
									str = nf_sysdb_get_str_nocopy(tmp_key);

									for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
									{
										if((mask_lcamera >> cnt_ch) & 0x1)
											sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
									}

									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  
													"%-16.16s : %s\n", str, str_lcamera);
								#else
									#if defined(ENABLE_SENSOR_IPCAM)
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s (IPCAM)\n",
													_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
													e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
													_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
									#else
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s\n",
													_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
													e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
													_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
									#endif
								#endif
							}
							else if(event_num == EMAIL_EVENT_TYPE1_POS)
							{
								gushort pos_data_cnt = 0;
								gushort pos_data_max_cnt = e_send_data->type1[event_num].rise[ch_num][63];
								GTimeVal	tv_pos;
								memset(&tv_pos, 0x00, sizeof(tv_pos));

								if(pos_data_max_cnt > NF_ACTION_POS_TEXT_DATA_MAX_NUM)
									pos_data_max_cnt = NF_ACTION_POS_TEXT_DATA_MAX_NUM;

								#if 1
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s\n",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_POS],
											e_send_data->type1[event_num].rise[ch_num][63],
											
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );
								#else
									gchar str_lcamera[64] = {0, };
									gint cnt_ch=0;
									guint mask_lcamera = _nf_action->pos_data[ch_num].mask_lcamera;

									for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
									{
										if((mask_lcamera >> cnt_ch) & 0x1)
											sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
									}
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %s\n",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_POS], str_lcamera);
								#endif

								for(pos_data_cnt=0; pos_data_cnt<pos_data_max_cnt; pos_data_cnt++)
								{
									if((NF_MAIL_SEND_MAX_CONTENTS - strlen(buffer)) > NF_ACTION_POS_TEXT_MAX_LENGTH)
									{
										gchar str_time[128]={0, };
										time_t ltime;
										struct tm st_buff;
										struct tm *st = &st_buff;

										g_static_mutex_lock (&_nf_pos_data_email_mutex);
										GUINT64_TO_GTIMEVAL(p_text_data[ch_num].timestamp[pos_data_cnt], tv_pos);
										#if 0
											pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), " - POS %d %d %s\n",
													ch_num+1, tv_pos.tv_sec, p_text_data[ch_num].data[pos_data_cnt]);
										#else
											ltime=tv_pos.tv_sec;
											localtime_r(&ltime, st);
											pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),
														" - POS %d %04d%02d%02d_%02d:%02d:%02d %s\n",
														ch_num+1, st->tm_year+1900, st->tm_mon+1, st->tm_mday,
														st->tm_hour, st->tm_min, st->tm_sec,
														p_text_data[ch_num].data[pos_data_cnt]);
										#endif
										g_static_mutex_unlock (&_nf_pos_data_email_mutex);

									}
								}
							}
							else
							{
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s CH%d\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][event_num], ch_num+1);
							}
						}
						else
							goto nf_action_email_default_str_type2;
					}
					else
					{
						nf_action_email_default_str_type2:
				#endif
					if(is_videcon)
					{
						if(title_print == FALSE)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "< %s %2d >\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], ch_num+1);
							title_print = TRUE;
						}
					}

					if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
					{
						/**
						  When IPCAM+DVR, This is IPCAM String
						  When Only DVR, this is DVR String
						 **/
						#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
						if (ch_num < _nf_action_num_alarm)
						#else
						if(ch_num < NUM_ALARM)
						#endif
						{
						#if 0
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s\n",
									_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
									e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
									_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
						#else
							if(is_videcon)
							{
								#if defined(ENABLE_SENSOR_IPCAM)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s (IPCAM)\n",
											_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
											e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
								#else
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s %3d %s\n",
											_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
											e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR]);
								#endif
							}
							else
							{
								gchar tmp_key[64]={0, }, *str=NULL, str_lcamera[64] = {0, };
								gint cnt_ch=0;
								guint mask_lcamera = _nf_action->sensor_data[ch_num].mask_lcamera;

								sprintf(tmp_key, "alarm.sensor.S%d.name", ch_num);
								str = nf_sysdb_get_str_nocopy(tmp_key);

								for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
								{
									if((mask_lcamera >> cnt_ch) & 0x1)
										sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
								}

								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%-16.16s : %3d %s - %s\n",
											str, e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR], str_lcamera);
							}
						#endif
						}
					}
					else if(event_num == EMAIL_EVENT_TYPE1_POS)
					{
						gushort pos_data_cnt = 0;
						gushort pos_data_max_cnt = e_send_data->type1[event_num].rise[ch_num][63];
						GTimeVal	tv_pos;
						memset(&tv_pos, 0x00, sizeof(tv_pos));

						if(pos_data_max_cnt > NF_ACTION_POS_TEXT_DATA_MAX_NUM)
							pos_data_max_cnt = NF_ACTION_POS_TEXT_DATA_MAX_NUM;

						#if 0
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s\n",
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_POS],
								e_send_data->type1[event_num].rise[ch_num][63],
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );
						#else
							if(is_videcon)
							{
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_POS],
										e_send_data->type1[event_num].rise[ch_num][63],
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );
							}
							else
							{
								gchar str_lcamera[64] = {0, };
								gint cnt_ch=0;
								guint mask_lcamera = _nf_action->pos_data[ch_num].mask_lcamera;

								for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
								{
									if((mask_lcamera >> cnt_ch) & 0x1)
										sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
								}
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s - %s\n",
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_POS],
										e_send_data->type1[event_num].rise[ch_num][63],
										_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR], str_lcamera );
							}
						#endif

						for(pos_data_cnt=0; pos_data_cnt<pos_data_max_cnt; pos_data_cnt++)
						{
							if((NF_MAIL_SEND_MAX_CONTENTS - strlen(buffer)) > NF_ACTION_POS_TEXT_MAX_LENGTH)
							{
								gchar str_time[128]={0, };
								time_t ltime;
								struct tm st_buff;
								struct tm *st = &st_buff;

								g_static_mutex_lock (&_nf_pos_data_email_mutex);
								GUINT64_TO_GTIMEVAL(p_text_data[ch_num].timestamp[pos_data_cnt], tv_pos);
								#if 0
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), " - POS %d %d %s\n",
										ch_num+1, tv_pos.tv_sec, p_text_data[ch_num].data[pos_data_cnt]);
								#else
									ltime=tv_pos.tv_sec;
									localtime_r(&ltime, st);
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),
												" - POS %d %04d%02d%02d_%02d:%02d:%02d %s\n",
												ch_num+1, st->tm_year+1900, st->tm_mon+1, st->tm_mday,
												st->tm_hour, st->tm_min, st->tm_sec,
												p_text_data[ch_num].data[pos_data_cnt]);
								#endif
								g_static_mutex_unlock (&_nf_pos_data_email_mutex);

							}
						}
					}
				#if defined(ENABLE_EMAIL_DUAL_SERVER)
				}
				#endif

				sndflag = TRUE;

				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						gint ch_snap=0, cnt_ch=0;
						guint mask_lcamera=0;

						if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
						{
							mask_lcamera = _nf_action->sensor_data[ch_num].mask_lcamera;
							for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
							{
								if((mask_lcamera >> cnt_ch) & 0x1)
								{
									ch_snap=cnt_ch;
									break;
								}
							}
						}
						else if(event_num == EMAIL_EVENT_TYPE1_POS)
						{
							mask_lcamera = _nf_action->pos_data[ch_num].mask_lcamera;
							for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
							{
								if((mask_lcamera >> cnt_ch) & 0x1)
								{
									ch_snap=cnt_ch;
									break;
								}
							}
						}
					else
							ch_snap=ch_num;

						strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );
						nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_snap);
						nf_mail_send_request( &cont, NULL);
					}
				#endif
			}
		}
	}

	// Type1 Event
	for( ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++ )
	{
		title_print=FALSE;

		for( event_num = EMAIL_EVENT_TYPE1_SENSOR; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			if((event_num == EMAIL_EVENT_TYPE1_REC_PANIC) || (event_num == EMAIL_EVENT_TYPE1_SENSOR)
					|| (event_num == EMAIL_EVENT_TYPE1_POS))
				continue;

			if (e_send_data->type1[event_num].rise[ch_num][63])
			{
				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						pos = pos_single_send;
					}
				#endif

				if(title_print == FALSE)
					{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "< %s %2d >\n",
									_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], ch_num+1);
					title_print = TRUE;
				}

						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s\n",
								_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
								e_send_data->type1[event_num].rise[ch_num][63],
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );

				sndflag = TRUE;

				#if defined(ENABLE_EMAIL_DUAL_SERVER)
					if ( is_single_send ){
						strncpy( cont.contents, buffer, sizeof(cont.contents)-1 );
						nf_action_request_snapshot_sigle_send(&cont, email_snapshot, ch_num);
						nf_mail_send_request( &cont, NULL);
					}
				#endif
			}
		}
	}

	#if 0		// for debug
		if(sndflag)
		g_message("%s sndflag[%d]\nbuffer[%s]", __FUNCTION__, sndflag, buffer);
	#endif

	if( sndflag == FALSE )
	{
		#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_SEND] )
				g_message("%s sndflag [%d]", __FUNCTION__ , sndflag );
		#endif
		return 0;
	}

	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		if ( is_single_send )
			return 1;
	#endif

	cont.is_dvr_event = 1;
	strncat( cont.contents, buffer, sizeof(cont.contents)-1 );

	#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
		if( email_snapshot->data && email_snapshot->data_size >0)
		{
			time_t ltime;
			struct tm st_buff;
			struct tm *st = &st_buff;

			ltime =(time_t)(email_snapshot->capture_time.tv_sec);
			localtime_r(&ltime, st);

			snprintf( cont.image_name,  sizeof(cont.image_name) - 1,
					"ch%02d_%04d%02d%02d_%02d%02d%02d_%06ld.jpg",
					email_snapshot->ch+1,
					st->tm_year+1900, st->tm_mon+1, st->tm_mday,
					st->tm_hour, st->tm_min, st->tm_sec,
					email_snapshot->capture_time.tv_usec);

			cont.image_size = email_snapshot->data_size;
			cont.image_data = email_snapshot->data;
		}
		#if defined(DEBUG_ACTION_SNAPSHOT)
		else
		{
			if(dlvaFlag == 1)
				printf("\033[0;33m %s  [DEBUG_ACTION_SNAPSHOT] dlva snapshot x \033[0;39m\n", __FUNCTION__);	
		}
		#endif 
	#endif

	nf_mail_send_request( &cont, NULL);

	#if 0
		GTimeVal tv;
		gettimeofday((struct timeval *)&tv, NULL);

		for(cnt=0; cnt<cont.to_cnt; cnt++)
			nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_EMAIL_SENT, cont.to[cnt]);
	#endif

	return 1;
}

#if defined(ENABLE_EMAIL_DUAL_SERVER)
static gboolean nf_action_request_snapshot_sigle_send(NF_MAIL_SEND_CONTENT *cont, EMAIL_SNAPSHOT_DATA *email_snapshot, gint ch)
{
	guint mask_vloss=0;

	memset(cont->image_name, 0x0, NF_MAIL_SEND_MAX_IMAGE_NAME);
	cont->image_size=0;
	cont->image_data=NULL;

	mask_vloss = nf_notify_get_param0("vloss");
	if(((mask_vloss >> ch) & 0x1) == 0)		// not vloss
	{
	nf_action_request_snapshot(email_snapshot, ch);

	if( email_snapshot->data && email_snapshot->data_size >0)
	{
		time_t ltime;
		struct tm st_buff;
		struct tm *st = &st_buff;

		ltime =(time_t)(email_snapshot->capture_time.tv_sec);
		localtime_r(&ltime, st);

		snprintf( cont->image_name,  sizeof(cont->image_name) - 1,
				"ch%02d_%04d%02d%02d_%02d%02d%02d_%06ld.jpg",
				email_snapshot->ch+1,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday,
				st->tm_hour, st->tm_min, st->tm_sec,
				email_snapshot->capture_time.tv_usec);

		cont->image_size = email_snapshot->data_size;
		cont->image_data = email_snapshot->data;
	}

	return TRUE;
}
	else
	{
		#if defined(DEBUG_ACTION_SNAPSHOT)
		printf("\033[0;33m %s  [DEBUG_ACTION_SNAPSHOT] VLOSS  \033[0;39m\n", __FUNCTION__);
		#endif
		return FALSE;
	}
}
#endif

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
static gboolean nf_action_request_snapshot(EMAIL_SNAPSHOT_DATA *email_snapshot, gint ch)
{
	EMAIL_DATA		*e_data = &_nf_action->email_data;
	EMAIL_STATE		*e_state = &_nf_action->email_state;
	gint 			div_count = e_state->div_count;

	gint ch_num=0, event_num=0, min_num=0, event_cate=0;

	if((email_snapshot->data_size != 0) && (email_snapshot->data != NULL))
	{
		g_free(email_snapshot->data);
		email_snapshot->data=NULL;
	}
	memset(email_snapshot, 0x0, sizeof(EMAIL_SNAPSHOT_DATA));

	e_data->snapshot_ch=ch;		// renew snapshot ch
	if( e_data->snapshot_onoff && (e_data->snapshot_ch != 0xff))
	{
		NF_JPEG_MAN_SNAPSHOT *snapshot = NULL;

		if( !email_snapshot->is_complete &&
				_email_snapshot_event_check( div_count ) )
		{
			gint retry_cnt=30;

			// Get Snapshot
			do{
				nf_jpeg_man_request_snapshot( e_data->snapshot_ch,NF_SECOND_SIZE, NF_JPEC_REQUEST_TIME );
				if( nf_jpeg_man_check_snapshot( e_data->snapshot_ch,NF_SECOND_SIZE, NF_JPEC_JPEG_CHECK_TIME ) )
				{
					if( !nf_jpeg_man_get_snapshot( e_data->snapshot_ch,NF_SECOND_SIZE, &snapshot ) )
						goto snap_get_failed;

					if( snapshot->data_size <= 0 )
					{
						g_warning("%s snapshot data size failed[%d]",
								__FUNCTION__, snapshot->data_size);

						goto snap_proc_failed;
					}

					email_snapshot->data = g_malloc(snapshot->data_size);
					if( email_snapshot->data == NULL )
					{
						g_warning("%s snapshot data malloc failed[%d]",
								__FUNCTION__, snapshot->data_size);
						goto snap_proc_failed;
					}

					memcpy( email_snapshot->data, snapshot->data, snapshot->data_size);

					email_snapshot->data_size = snapshot->data_size;
					email_snapshot->ch = e_data->snapshot_ch;
					email_snapshot->is_complete = 1;
					email_snapshot->capture_time = snapshot->ctime;

					snap_proc_failed:
						nf_jpeg_man_free_snapshot(snapshot);

					break;
				}

				g_usleep(41000);
			}while( --retry_cnt >0  );

			if(retry_cnt == 0)
				g_warning("email.. get jpeg fail!! ch%d", e_data->snapshot_ch);

			snap_get_failed:
				snapshot = NULL;
		}
	}

	return TRUE;
}
#endif

static int
_email_is_act(int index_type1, int index_type2, int index_type3, int prop_id, int type)
{
	ALARM_SENSOR_DATA	*sdata	= NULL;
	MOTION_DATA			*mdata	= NULL;
	VLOSS_DATA			*vdata	= NULL;
	REC_DATA			*rpdata	= NULL;
	TEXTIN_DATA			*tdata	= NULL;

	DISK_DATA			*ddata	= NULL;
	SYSTEM_DATA			*stdata	= NULL;
	NET_DATA			*ndata	= NULL;
#if defined(ENABLE_EVENT_TAMPER)
	TAMPER_DATA			*tadata	= NULL;
#endif
#if defined(SUPPORT_VCA_CAMERA)
	VCA_DATA *vcadata=NULL;
#endif
/** IMSI AI BOX **/
	AI_DATA *aidata=NULL;
/** DVA Event **/
	DVA_DATA *dvadata = NULL;
	POS_DATA *pdata=NULL;

	if(type == 0)
	{
		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR ) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				g_return_val_if_fail( index_type1 < _nf_action_num_alarm, 0);
			#else
				g_return_val_if_fail( index_type1 < NUM_ALARM, 0);
			#endif
		} else {
			g_return_val_if_fail( index_type1 < NUM_ACTIVE_CH, 0);
		}

		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE1_NR, 0);

		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR )
		{
			sdata = &_nf_action->sensor_data[index_type1];
			return sdata->email_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_MOTION )
		{
			mdata = &_nf_action->motion_data[index_type1];
			return mdata->email_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_VLOSS )
		{
			vdata = &_nf_action->vloss_data[index_type1];
			return vdata->email_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_PANIC )
		{
			rpdata = &_nf_action->rec_data[index_type1];
			return rpdata->email_act ? 1: 0;
		}
#if 0
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_ALARM )
			return radata->email_act ? 1: 0;
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_MOTION )
			return rmdata->email_act ? 1: 0;
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_TEXT_IN )
		{
			tdata = &_nf_action->textin_data[index_type1];
			return tdata->email_act ? 1: 0;
		}
#if defined(ENABLE_EVENT_TAMPER)
		else if ( prop_id == EMAIL_EVENT_TYPE1_TAMPER )
		{
			tadata = &_nf_action->tamper_data[index_type1];
			return tadata->email_act ? 1: 0;
		}
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_POS )
		{
			pdata = &_nf_action->pos_data[index_type1];
			return pdata->email_act ? 1: 0;
		}
	}
	else if(type == 1)
	{
		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE2_NR, 0);

		if ( prop_id == EMAIL_EVENT_TYPE2_HDD )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_HDD_EVENT_NR, 0);

			ddata = &_nf_action->disk_data[index_type2];

			return ddata->email_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_SYSTEM )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_SYSTEM_EVENT_NR, 0);

			stdata = &_nf_action->system_data[index_type2];

			return stdata->email_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_NET )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_NET_EVENT_NR, 0);

			ndata = &_nf_action->net_data[index_type2];
			return ndata->email_act ? 1: 0;
		}
	}
	else if(type == 2)
	{
#if defined(SUPPORT_VCA_CAMERA)
		if ( prop_id == EMAIL_EVENT_TYPE3_VCA )
		{
			vcadata = &_nf_action->vca_data[index_type3];

			return vcadata->email_act ? 1: 0;
		}
#endif
	}
	else if(type == 3) /** DVA Event **/
	{
		if ( prop_id == EMAIL_EVENT_TYPE4_DVA_IDZ || prop_id == EMAIL_EVENT_TYPE4_DVA_IPZ || prop_id == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT)
		{
			dvadata = &_nf_action->dva_data[index_type3];
			return dvadata->email_act ? 1: 0;
		}
	}
	else if(type == 5)
	{
		/** IMSI AI BOX **/
		if ( prop_id == EMAIL_EVENT_TYPE5_AI )
		{
			aidata = &_nf_action->ai_data[index_type3];

			return aidata->email_act ? 1: 0;
		}
	}

	return 0;
}

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
static gboolean
_email_snapshot_event_check(int  div_count)
{
	gint ch_num=0, mask_num=0, min_num=0;
	gint event_num=0, event_cate=0;
	guint event_sum=0;

	if( _nf_action->email_snapshot.event_cnt >0 )
		return 1;

	// email event type1
	for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
	{
		gint ch_max=0;

		if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				ch_max = _nf_action_num_alarm;
			#else
				ch_max=NUM_ALARM;
			#endif
		} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
			ch_max=NF_ACTION_REC_EVENT_NR;
		} else {
			ch_max=NUM_ACTIVE_CH;
		}

		for( ch_num=0; ch_num < ch_max; ++ch_num )
		{
			if( _email_is_act(ch_num, 0, 0, event_num, 0)
					&& _nf_action->email_send_data->type1[event_num].rise[ch_num][div_count] )
			{
				g_get_current_time( &_nf_action->email_snapshot.detect_time);
				_nf_action->email_snapshot.event_cnt = 1;
				return 1;
			}
		}
	}

	// email event type2
	for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++ )
		{
			if( _email_is_act(0, event_cate, 0, event_num, 1)
					&& _nf_action->email_send_data->type2.rise[event_num][event_cate][div_count] )
			{
				g_get_current_time( &_nf_action->email_snapshot.detect_time);
				_nf_action->email_snapshot.event_cnt = 1;
				return 1;
			}
		}
	}

	// email event type3
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE3_NR; ++event_num )
		{
			gint event_max=0;

			#if defined(SUPPORT_VCA_CAMERA)
				if(event_num == EMAIL_EVENT_TYPE3_VCA)
					event_max=NF_ACTION_VCA_NR;
			#endif

			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _email_is_act(0, 0, ch_num, event_num, 2)
						&& _nf_action->email_send_data->type3.rise[event_num][event_cate][div_count] )
				{
					g_get_current_time( &_nf_action->email_snapshot.detect_time);
					_nf_action->email_snapshot.event_cnt = 1;
					return 1;
				}
			}
		}
	}
        /** DVA Event **/
	// email event type4
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE4_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
				event_max=NF_ACTION_DVA_IDZ_NR;
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
				event_max=NF_ACTION_DVA_IPZ_NR;

			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _email_is_act(0, 0, ch_num, event_num, 3)
						&& _nf_action->email_send_data->type4.rise[event_num][event_cate][div_count] )
				{
					g_get_current_time( &_nf_action->email_snapshot.detect_time);
					_nf_action->email_snapshot.event_cnt = 1;
					return 1;
				}
			}
		}
	}
	// email event type5
	/** IMSI AI BOX **/
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE5_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE5_AI)
				event_max=NF_ACTION_AI_NR;
					
			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _email_is_act(0, 0, ch_num, event_num, 5)
						&& _nf_action->email_send_data->type5.rise[event_num][event_cate][div_count] )
				{
					g_get_current_time( &_nf_action->email_snapshot.detect_time);
					_nf_action->email_snapshot.event_cnt = 1;
					return 1;
				}
			}
		}
	}
	return 0;
}
#endif

static gboolean
_email_act_switch_check(EMAIL_DATA *e_data, EMAIL_EVENT_DATA *e_send_data, gint div_count)
{
	gint num_ch=0, num_alarm=0, num_port=0, num_evt=0, num_evt_type=0;
	guint mask=0, alarm_curr=0;
	static guint mask_log=0;
	gboolean is_logput=FALSE, is_log_arm=0, is_log_disarm=0;

	if(e_data->al_switch)
	{
		GTimeVal tv;
		gettimeofday((struct timeval *)&tv, NULL);

		#if 0
			if(_nf_action->sysdb_reload)
				mask_log=0;
		#endif

		num_port=e_data->al_switch_port;

		mask = (guint)(1 << num_port);

		alarm_curr = nf_notify_get_param0("sensor");
		//alarm_curr = (alarm_curr >> 16);			// For NVR
		if(alarm_curr & mask)
		{
		
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			for(num_alarm=0; num_alarm < _nf_action_num_alarm; num_alarm++)
				e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[num_alarm][div_count]=0;
		#else
			for(num_alarm=0; num_alarm < NUM_ALARM; num_alarm++)
				e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[num_alarm][div_count]=0;
		#endif

			for(num_ch=0; num_ch < NUM_ACTIVE_CH; num_ch++)
				e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].rise[num_ch][div_count]=0;
				
			for(num_ch=0; num_ch < NUM_ACTIVE_CH; num_ch++)
				e_send_data->type1[EMAIL_EVENT_TYPE1_POS].rise[num_ch][div_count]=0;

			//BASIC VA
			for(num_ch=0; num_ch < NUM_ACTIVE_CH; num_ch++)
			{
				for(num_evt = 0; num_evt < EMAIL_EVENT_TYPE3_NR; num_evt++)
				{
					for(num_evt_type = 0; num_evt_type < EMAIL_EVENT_TYPE3_INDEX_MAX; num_evt_type++)
						e_send_data->type3.rise[num_ch][num_evt][num_evt_type][div_count]=0;
				}
			}
			
			//DLVA
			for(num_ch=0; num_ch < NUM_ACTIVE_CH; num_ch++)
			{
				for(num_evt = 0; num_evt < EMAIL_EVENT_TYPE4_NR; num_evt++)
				{
					for(num_evt_type = 0; num_evt_type < EMAIL_EVENT_TYPE4_INDEX_MAX; num_evt_type++)
						e_send_data->type4.rise[num_ch][num_evt][num_evt_type][div_count]=0;
				}
			}

			//AI BOX
			for(num_ch=0; num_ch < NUM_ACTIVE_CH; num_ch++)
			{
				for(num_evt = 0; num_evt < EMAIL_EVENT_TYPE5_NR; num_evt++)
				{
					for(num_evt_type = 0; num_evt_type < EMAIL_EVENT_TYPE5_INDEX_MAX; num_evt_type++)
						e_send_data->type5.rise[num_ch][num_evt][num_evt_type][div_count]=0;
				}
			}
			
			if((mask_log & 0x1) == 0)
			{
				mask_log |= (1 << 0);		// log arm
				mask_log &= ~(1 << 1);		// log disarm
				e_send_data->type0.rise[EMAIL_EVENT_TYPE0_DISARM][div_count]++;
				#if defined(DEBUG_EMAIL_ARM)
				printf("\033[0;34m %s DISARM\033[0;39m\n", __FUNCTION__);
				#endif
				is_logput=TRUE; is_log_arm=FALSE; is_log_disarm=TRUE;
			}
		}
		else
		{
			if(((mask_log >> 1) & 0x1) == 0)
			{
				mask_log &= ~(1 << 0);		// log arm
				mask_log |= (1 << 1);		// log disarm
				#if defined(DEBUG_EMAIL_ARM)
					printf("\033[0;34m %s ARM\033[0;39m\n", __FUNCTION__);
				#endif
				e_send_data->type0.rise[EMAIL_EVENT_TYPE0_ARM][div_count]++;
				is_logput=TRUE; is_log_arm=TRUE; is_log_disarm=FALSE;
			}
		}

		if(is_logput)
		{
			if(is_log_arm)
			{
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_ARM, NULL);
				g_message("%s line%d logput arm", __FUNCTION__, __LINE__);
			}

			if(is_log_disarm)
			{
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_DISARM, NULL);
				g_message("%s line%d logput Disarm", __FUNCTION__, __LINE__);
			}
		}
	}
	else
	{
		mask_log=0;
		return FALSE;
	}

	return TRUE;
}

static gboolean
_push_act_switch_check(PUSH_DATA *e_data, EMAIL_EVENT_DATA *e_send_data, RECENT_EVENT_DATA *recent_event)
{
	gint num_ch=0, num_alarm=0, num_port=0, num_evt=0, num_evt_type=0;
	guint mask=0, alarm_curr=0;
	static guint mask_log=0;
	gboolean is_logput=FALSE, is_log_arm=0, is_log_disarm=0, ret = FALSE;
	
	//printf("\033[0;36m %s al_switch %d \033[0;39m\n", __FUNCTION__, e_data->al_switch);
	if(e_data->al_switch)
	{
		GTimeVal tv;
		gettimeofday((struct timeval *)&tv, NULL);

		num_port=e_data->al_switch_port;

		mask = (guint)(1 << num_port);

		alarm_curr = nf_notify_get_param0("sensor");
		//printf("\033[0;36m %s alarm_curr %x mask %x\033[0;39m\n", __FUNCTION__, alarm_curr, mask);
		//printf("\033[0;36m %s event_num %d event_type %d\033[0;39m\n", __FUNCTION__, recent_event->event_num , recent_event->event_type);
		if(alarm_curr & mask)
		{		
			if((recent_event->event_num == EMAIL_EVENT_TYPE1_SENSOR && recent_event->event_type == 1) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE1_MOTION && recent_event->event_type == 1) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE1_POS && recent_event->event_type == 1) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE1_DVA_OBJ_CNT && recent_event->event_type == 1) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE1_DVA_IDZ && recent_event->event_type == 1) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE1_DVA_IPZ && recent_event->event_type == 1) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE3_VCA && recent_event->event_type == 3) \
				||(recent_event->event_num == EMAIL_EVENT_TYPE5_AI && recent_event->event_type == 5))
			{
				ret = TRUE;
			}
			if((mask_log & 0x1) == 0)
			{
				mask_log |= (1 << 0);		// log arm
				mask_log &= ~(1 << 1);		// log disarm
				#if defined(DEBUG_EMAIL_ARM)
				printf("\033[0;34m %s DISARM\033[0;39m\n", __FUNCTION__);
				#endif
				is_logput=TRUE; is_log_arm=FALSE; is_log_disarm=TRUE;
			}
		}
		else
		{
			if(((mask_log >> 1) & 0x1) == 0)
			{
				mask_log &= ~(1 << 0);		// log arm
				mask_log |= (1 << 1);		// log disarm
				#if defined(DEBUG_EMAIL_ARM)
					printf("\033[0;34m %s ARM\033[0;39m\n", __FUNCTION__);
				#endif
				is_logput=TRUE; is_log_arm=TRUE; is_log_disarm=FALSE;
			}
		}
		#if 0
		if(is_logput)
		{
			if(is_log_arm)
			{
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_ARM, NULL);
				g_message("%s line%d logput arm", __FUNCTION__, __LINE__);
			}

			if(is_log_disarm)
			{
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_DISARM, NULL);
				g_message("%s line%d logput Disarm", __FUNCTION__, __LINE__);
			}
		}
		#endif
	}
	else
	{
		mask_log=0;
	}

	return ret;
}

static gboolean
_email_act_sched_check(EMAIL_DATA *e_data)
{
#if 0
	time_t curr_time;
	struct tm *div_time;
	gint sched_to = 0;

	curr_time = time(NULL);
	div_time = localtime(&curr_time);

	sched_to = e_data->sched_to;
	if (e_data->sched_to == 0)
		sched_to = 24;

	if (e_data->sched_wday[div_time->tm_wday] == 1)
	{
		if (e_data->sched_from == e_data->sched_to)
			goto sched_check_ok;
		else
		{
			if (e_data->sched_from > sched_to)
			{
				if ((sched_to <= div_time->tm_hour)
				 && (e_data->sched_from > div_time->tm_hour))
					goto sched_check_fail;
				else
					goto sched_check_ok;
			}
			else
			{
				if ((e_data->sched_from <= div_time->tm_hour)
				 && (sched_to > div_time->tm_hour))
					goto sched_check_ok;
				else
					goto sched_check_fail;
			}
		}
	}
	else
	{
sched_check_fail:
		return FALSE;
	}
sched_check_ok:
		return TRUE;
#else
	time_t curr_time;
	struct tm *div_time;
	gint cnt_sched=0, day=0;
	guint wday=0, curr_hour=0, curr_min=0, curr_sec=0;

	curr_time = time(NULL);
	div_time = localtime(&curr_time);

	#if 0
		g_message("%s line%d wday %d tm_hour %d tm_min %d",
					__FUNCTION__, __LINE__, div_time->tm_wday, div_time->tm_hour, div_time->tm_min);
	#endif

	for(day=0; day<7; day++)
	{
		if(div_time->tm_wday == day)
		{
			wday=(1 << day);
			break;
		}
	}

	curr_hour=div_time->tm_hour;
	curr_min=div_time->tm_min;
	curr_sec=div_time->tm_sec;

	for(cnt_sched=0; cnt_sched<10; cnt_sched++)
	{
		#if 0
			g_message("%s line%d sched%d wday 0x%08x sched_wday 0x%08x   -> 0x%08x", __FUNCTION__, __LINE__, cnt_sched,
						wday, e_data->sched_wday[cnt_sched], wday & e_data->sched_wday[cnt_sched]);
		#endif
		if(wday & e_data->sched_wday[cnt_sched])
		{
			#if 0
				if(cnt_sched == 0)
				{
					g_message("%s line%d cnt_sched %d curr_hour %d curr_min %d from_h %d from_m %d to_h %d to_m %d",
								__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min,
								e_data->sched_from_h[cnt_sched], e_data->sched_from_m[cnt_sched],
								e_data->sched_to_h[cnt_sched], e_data->sched_to_m[cnt_sched]);
				}
			#endif

			if((e_data->sched_from_h[cnt_sched] == e_data->sched_to_h[cnt_sched])
						&& (e_data->sched_from_m[cnt_sched] == e_data->sched_to_m[cnt_sched]))
				goto sched_check_ok_cont0;
			else if((e_data->sched_from_h[cnt_sched] <= curr_hour) && (e_data->sched_to_h[cnt_sched] >= curr_hour))
			{
				if(e_data->sched_from_h[cnt_sched] == e_data->sched_to_h[cnt_sched])
				{
					if(e_data->sched_from_m[cnt_sched] == e_data->sched_to_m[cnt_sched])
					{
						goto sched_check_ok_cont1;
					}
					else if(e_data->sched_from_m[cnt_sched] < e_data->sched_to_m[cnt_sched])
					{
						if((e_data->sched_from_m[cnt_sched] <= curr_min) &&
								(e_data->sched_to_m[cnt_sched] >= curr_min))
						{
							if(e_data->sched_to_m[cnt_sched] == curr_min)		// if this condition, compare second
							{
								if(curr_sec == 0)
									goto sched_check_ok_cont2;
							}
							else
								goto sched_check_ok_cont2;
						}
					}
				}
				else if(e_data->sched_from_h[cnt_sched] < e_data->sched_to_h[cnt_sched])
				{
					if((curr_hour >= e_data->sched_from_h[cnt_sched]) &&
						(curr_hour < e_data->sched_to_h[cnt_sched]))
					{
						if(curr_hour == e_data->sched_from_h[cnt_sched])
						{
							if(e_data->sched_from_m[cnt_sched] <= curr_min)
								goto sched_check_ok_cont3;
						}
						else if(curr_hour > e_data->sched_from_h[cnt_sched])
							goto sched_check_ok_cont4;
					}
					else if(curr_hour == e_data->sched_to_h[cnt_sched])
					{
						if(e_data->sched_to_m[cnt_sched] >= curr_min)
						{
							if(e_data->sched_to_m[cnt_sched] == curr_min)		// if this condition, compare second
							{
								if(curr_sec == 0)
									goto sched_check_ok_cont5;
							}
							else
								goto sched_check_ok_cont5;
						}
					}
				}
			}
		}
		#if 0
			g_message("%s line%d wday 0x%08x tm_wday %d tm_hour %d tm_min %d sched_wday 0x%08x"
							" from_h %d from_m %d from_s %d to_h %d to_m %d to_s %d",
							__FUNCTION__, __LINE__, wday,  div_time->tm_wday, div_time->tm_hour, div_time->tm_min,
							e_data->sched_wday[cnt_sched], e_data->sched_from_h[cnt_sched], e_data->sched_from_m[cnt_sched],
							e_data->sched_from_s[cnt_sched], e_data->sched_to_h[cnt_sched], e_data->sched_to_m[cnt_sched],
							e_data->sched_to_s[cnt_sched]);
		#endif
	}

sched_check_fail:
	#if 0
		g_message("%s line%d sched_check_fail hour %d min %d sec %d", __FUNCTION__, __LINE__, curr_hour, curr_min, curr_sec);
	#endif
	return FALSE;

sched_check_ok_cont0:
	#if 0
		g_message("%s line%d sched%d condition0 Ok!! hour %d min %d sec %d",
						__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min, curr_sec);
	#endif
	return TRUE;

sched_check_ok_cont1:
	#if 0
		g_message("%s line%d sched%d condition1 Ok!! hour %d min %d sec %d",
						__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min, curr_sec);
	#endif
	return TRUE;

sched_check_ok_cont2:
	#if 0
		g_message("%s line%d sched%d condition2 Ok!! hour %d min %d sec %d",
						__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min, curr_sec);
	#endif
	return TRUE;

sched_check_ok_cont3:
	#if 0
		g_message("%s line%d sched%d condition3 Ok!! hour %d min %d sec %d",
						__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min, curr_sec);
	#endif
	return TRUE;

sched_check_ok_cont4:
	#if 0
		g_message("%s line%d sched%d condition4 Ok!! hour %d min %d sec %d",
						__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min, curr_sec);
	#endif
	return TRUE;

sched_check_ok_cont5:
	#if 0
		g_message("%s line%d sched%d condition5 Ok!! hour %d min %d sec %d",
						__FUNCTION__, __LINE__, cnt_sched, curr_hour, curr_min, curr_sec);
	#endif
	return TRUE;
#endif
	}

static void
_email_event_check(EMAIL_DATA *e_data, EMAIL_EVENT_DATA *e_send_data, EMAIL_STATE *e_state)
{
	gint 	div_count = e_state->div_count;
	gint	i=0, event_cate=0;
	guint 	mask_snapshot_ch=0, mask_motion=0, mask_lcamera_alarm=0, mask_rec_panic=0, mask_pos=0;
	gint64 	mask=0;
	ALARM_SENSOR_DATA *sdata=NULL;
	MOTION_DATA *mdata=NULL;
	REC_DATA *rdata=NULL;

	#if defined(ENABLE_EVENT_TAMPER)
		TAMPER_DATA *tdata=NULL;
		guint mask_tamper=0;
	#endif
	#if defined(SUPPORT_VCA_CAMERA)
		VCA_DATA *vcadata=NULL;
		guint mask_vca=0;
	#endif
   	/** DVA Event **/
	DVA_DATA *dvadata=NULL;
	guint mask_dva=0;
	
	/** IMSI AI BOX **/
	AI_DATA *aidata=NULL;
	guint mask_ai=0;
		
	POS_DATA *pdata=NULL;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);

		if( _nf_action->curr_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].active[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->curr_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].active[i][div_count]++;
		if( _nf_action->curr_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].active[i][div_count]++;
		#if 0
			if( _nf_action->curr_rec_alarm & mask )
				e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_ALARM][div_count]++;

			if( _nf_action->curr_rec_motion & mask )
				e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_MOTION][div_count]++;
		#endif
		#if defined(ENABLE_EVENT_TAMPER)
			if( _nf_action->curr_tamper & mask )
				e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].active[i][div_count]++;
		#endif

		#if defined(SUPPORT_VCA_CAMERA)
			if(_nf_action->curr_vca[i])
			{
				guint mask=0, evt_max=0;

				mask=_nf_event_vca_get_evt_mask(_nf_action->rise_vca[i]);
				for(evt_max=0; evt_max<16; evt_max++)
				{
					if(mask & (1 << evt_max))
						e_send_data->type3.rise[i][EMAIL_EVENT_TYPE3_VCA][evt_max][div_count]++;
				}
			}
		#endif
        /** DVA Event **/
		if( _nf_action->curr_dva_idz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->curr_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IDZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.active[i][EMAIL_EVENT_TYPE4_DVA_IDZ][evt_max][div_count]++;
			}
		}
		if( _nf_action->curr_dva_ipz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->curr_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IPZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IPZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.active[i][EMAIL_EVENT_TYPE4_DVA_IPZ][evt_max][div_count]++;
			}
		}
		/*DVA OBJ CNT*/
		if( _nf_action->curr_dva_obj_cnt & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->curr_dva_event[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
				{
					e_send_data->type4.active[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT][evt_max][div_count]++;
					#if defined(EVENTALARMDEBUG)
					printf("\033[0;31m %s CURR EVENT %d \033[0;39m\n", __FUNCTION__, evt_cnt);
					#endif
				}
			}
		}
		/** DVA Event **/
		/** IMSI AI BOX **/
		if(_nf_action->curr_ai[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_ai_get_evt_mask(_nf_action->rise_ai[i]);
			for(evt_max=0; evt_max<NF_ACTION_AI_NR; evt_max++)
			{
				if(mask & (1 << evt_max))
				{
					e_send_data->type5.rise[i][EMAIL_EVENT_TYPE5_AI][evt_max][div_count]++;
					
					#if defined (DEBUG_AI)
					printf("\033[0;33m %s [DEBUG_AI] curr_ai[%d] EVENT[%d] div_count[%d]\033[0;39m\n", __FUNCTION__,i,evt_max,div_count);
					#endif
				}
			}
		}
		if( _nf_action->curr_pos & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_POS].active[i][div_count]++;
	}

	// 2010-08-02 ?�후 1:55:05 choissi
	if( _nf_action->curr_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].active[0][div_count]++;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);

		if( _nf_action->rise_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->rise_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].rise[i][div_count]++;
		if( _nf_action->rise_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].rise[i][div_count]++;
		#if defined(ENABLE_EVENT_TAMPER)
			if( _nf_action->rise_tamper & mask )
				e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].rise[i][div_count]++;
		#endif
		#if defined(SUPPORT_VCA_CAMERA)
		if(_nf_action->rise_vca[i])
			{
				guint mask=0, evt_cnt=0;

				mask=_nf_event_vca_get_evt_mask(_nf_action->rise_vca[i]);
				
				
				for(evt_cnt=0; evt_cnt<16; evt_cnt++)
				{
					if(mask & (1 << evt_cnt))
					{
						#if defined(EVENTALARMDEBUG)
						printf("\033[0;31m %s RISE EVENT %d \033[0;39m\n", __FUNCTION__, evt_cnt);
						#endif
						e_send_data->type3.rise[i][EMAIL_EVENT_TYPE3_VCA][evt_cnt][div_count]++;
					}
				}
			}
		#endif
		/** DVA Event **/
		if( _nf_action->rise_dva_idz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->rise_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IDZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.rise[i][EMAIL_EVENT_TYPE4_DVA_IDZ][evt_max][div_count]++;
			}
		}
		
		if( _nf_action->rise_dva_ipz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->rise_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IPZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IPZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.rise[i][EMAIL_EVENT_TYPE4_DVA_IPZ][evt_max][div_count]++;
			}
		}
		/*DVA OBJ CNT*/
		if( _nf_action->rise_dva_obj_cnt & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->rise_dva_event[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
				{
					e_send_data->type4.rise[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT][evt_max][div_count]++;
					#if defined(DEBUG_DVA_OBJ_CNT)
					printf("\033[0;35m %s RISE DVA_OBJ_CNT \033[0;39m\n", __FUNCTION__);
					#endif
				}
			}
		}
		/** IMSI AI BOX **/
		if(_nf_action->rise_ai[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_ai_get_evt_mask(_nf_action->rise_ai[i]);
				
			for(evt_max=0; evt_max<NF_ACTION_AI_NR; evt_max++)
			{
				if(mask & (1 << evt_max))
				{
					e_send_data->type5.rise[i][EMAIL_EVENT_TYPE5_AI][evt_max][div_count]++;
					
					if(evt_max >= NF_ACTION_AI_DIR_POS && evt_max <= NF_ACTION_AI_INTRUSION)
					{
						if( NULL == strstr(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max], _nf_action->ai_meta_data[i].object_class) )
						{
							if( 0 != strlen(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max]) )
							{
								strcat(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max],", ");
							}
							strcat(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max], _nf_action->ai_meta_data[i].object_class);
							#if defined(DEBUG_AI_DATA)
								printf("\033[0;36m %s DEBUG_AI_DATA %s\033[0;39m\n", __FUNCTION__, e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max]);
							#endif
						}
					}
					else if(evt_max == NF_ACTION_AI_GENERIC)
					{
						_save_ai_generic_evt(e_send_data, i);
					}
					#if defined (DEBUG_AI)
					printf("\033[0;33m %s [DEBUG_AI] rise_ai[%d] EVENT[%d] div_count[%d]\033[0;39m\n", __FUNCTION__,i,evt_max,div_count);
					#endif
				}
			}
		}
		if( _nf_action->rise_pos & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_POS].rise[i][div_count]++;
	}

	/** Panic Record **/
	if( _nf_action->rise_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[0][div_count]++;

	// Disk Event
	for(i=NF_ACTION_HDD_EVENT_OVER; i<NF_ACTION_HDD_EVENT_NR; i++)
	{
		if(_nf_action->rise_hdd & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_HDD][i][div_count]++;
	}

	// System Event
	for(i=NF_ACTION_SYSTEM_EVENT_BOOTING; i<NF_ACTION_SYSTEM_EVENT_NR; i++)
	{
		if(_nf_action->rise_system & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_SYSTEM][i][div_count]++;
	}

	// Net Event
	for(i=NF_ACTION_NET_EVENT_TROUBLE; i<NF_ACTION_NET_EVENT_NR; i++)
	{
		if(_nf_action->rise_net & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_NET][i][div_count]++;
	}

	// Set Snapshot CH
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; i++)
	#else
	for(i=0; i<NUM_ALARM; i++)
	#endif
	{
		sdata = &_nf_action->sensor_data[i];

		if((_nf_action->rise_alarm >> i) & 0x1)
		{
			if(sdata->email_act)
				mask_lcamera_alarm |= sdata->mask_lcamera;
		}
	}

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		mdata = &_nf_action->motion_data[i];

		if((_nf_action->rise_motion >> i) & 0x1)
		{
			if(mdata->email_act)
				mask_motion |= (1 << i);
		}
	}

	for(i=0; i<NF_ACTION_REC_EVENT_NR; i++)
	{
		rdata = &_nf_action->rec_data[i];

		if((_nf_action->rise_rec_panic >> i) & 0x1)
		{
			if(rdata->email_act)
				mask_rec_panic |= (1 << i);
		}
	}

	#if defined(ENABLE_EVENT_TAMPER)
		for(i=0; i<NUM_ACTIVE_CH; ++i)
		{
			tdata = &_nf_action->tamper_data[i];

			if((_nf_action->rise_tamper >> i) & 0x1)
			{
				if(tdata->email_act)
					mask_tamper |= (1 << i);
			}
		}
	#endif

	#if defined(SUPPORT_VCA_CAMERA)
		for(i=0; i<NUM_ACTIVE_CH; ++i)
		{
			vcadata = &_nf_action->vca_data[i];

			if(_nf_action->rise_vca[i])
			{
				if(vcadata->email_act)
					mask_vca |= (1 << i);
			}
		}
	#endif
    /** DVA Event **/
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);
		
		dvadata = &_nf_action->dva_data[i];

		if( _nf_action->rise_dva_idz & mask || _nf_action->rise_dva_ipz & mask || _nf_action->rise_dva_obj_cnt & mask)
		{
			if(dvadata->email_act)
				mask_dva |= (1 << i);
		}
	}
	/** IMSI AI BOX **/
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		aidata = &_nf_action->ai_data[i];

		if(_nf_action->rise_ai[i])
		{
			if(aidata->email_act)
				mask_ai |= (1 << i);
		}
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		pdata = &_nf_action->pos_data[i];

		if((_nf_action->rise_pos >> i) & 0x1)
		{
			if(pdata->email_act)
				mask_pos |= (1 << i);
		}
	}

	#if defined(ENABLE_EVENT_TAMPER)
		mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_tamper | mask_dva | mask_ai);
	#elif defined(SUPPORT_VCA_CAMERA)
		mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_vca | mask_dva | mask_ai);
	#elif defined(ENABLE_EVENT_TAMPER) && defined(SUPPORT_VCA_CAMERA)
		mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_tamper | mask_vca | mask_dva | mask_ai);
	#else
		mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_dva | mask_ai);
	#endif

	if(_nf_action->vendor == VENDOR_VIDECON || _nf_action->vendor == VENDOR_CBC )
	{
		if(_email_act_switch_check(e_data, e_send_data, div_count))
		{
		#if defined(ENABLE_EVENT_TAMPER)
				mask_snapshot_ch = (mask_rec_panic | mask_pos | mask_tamper);
		#else
				mask_snapshot_ch = (mask_rec_panic | mask_pos);
		#endif
		}
	}

	mask_snapshot_ch &= ~((gint)nf_notify_get_param0("vloss"));

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		if((mask_snapshot_ch >> i) & 0x1)
		{
			e_data->snapshot_ch = i;
			break;
		}
		else
			e_data->snapshot_ch = 0xff;
	}
}

static void
_email_action(void)
{
	GTimeVal        curr_timeval;

	EMAIL_DATA		*e_data = &_nf_action->email_data;
	EMAIL_STATE		*e_state = &_nf_action->email_state;
	EMAIL_EVENT_DATA	*e_send_data = _nf_action->email_send_data;
	gint			ret=0, i=0;
	gint 			div_count = e_state->div_count;

	gint ch_num=0, event_num=0, min_num=0, event_cate=0;
	guint event_sum=0;

	if(e_data->email_act == 0)
		return;

	gettimeofday((struct timeval *)&curr_timeval, NULL);
#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_ACTION] )
	{
		gint x=0, y=0, z=0;
		static glong check_time = 0;

		if( check_time == curr_timeval.tv_sec ){
			goto out;
		}else{
			check_time = curr_timeval.tv_sec;

			g_message("%s send_time[%ld] curr[%ld]",
					__FUNCTION__, e_state->send_time,  curr_timeval.tv_sec);

		}

		for(x=0;x<NUM_ACTIVE_CH;++x)
		{
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_ACTION] & (1<<x) )
			{
				for(y=0;y<e_state->div_frequency;++y)
				{
					g_print("ch[%2d] freq[%2d] %c rise", x, y,
							(y == e_state->div_count) ? '*' : ' ');
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].rise[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.rise[z][i][y] );
					}

					g_print("  active");
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].active[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.active[z][i][y] );
					}

					g_print("\n");
				}
			} // if
		} // for x
	}
out:

#endif

	if (!_email_act_sched_check(e_data))
		return;

	_email_event_check(e_data, e_send_data, e_state);

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	if( e_data->snapshot_onoff && (e_data->snapshot_ch != 0xff))
	{
		EMAIL_SNAPSHOT_DATA *email_snapshot = &_nf_action->email_snapshot;
		NF_JPEG_MAN_SNAPSHOT *snapshot = NULL;

		if( !email_snapshot->is_complete &&
				_email_snapshot_event_check( div_count ) )
		{
			gint retry_cnt=30;

			// Get Snapshot
			do{
				nf_jpeg_man_request_snapshot( e_data->snapshot_ch,NF_SECOND_SIZE, NF_JPEC_REQUEST_TIME );
				if( nf_jpeg_man_check_snapshot( e_data->snapshot_ch, NF_SECOND_SIZE, NF_JPEC_JPEG_CHECK_TIME ) )
				{
					if( !nf_jpeg_man_get_snapshot( e_data->snapshot_ch,NF_SECOND_SIZE, &snapshot ) )
						goto snap_get_failed;

					if( snapshot->data_size <= 0 )
					{
						g_warning("%s snapshot data size failed[%d]",
								__FUNCTION__, snapshot->data_size);

						goto snap_proc_failed;
					}

					email_snapshot->data = g_malloc(snapshot->data_size);
					if( email_snapshot->data == NULL )
					{
						g_warning("%s snapshot data malloc failed[%d]",
								__FUNCTION__, snapshot->data_size);
						goto snap_proc_failed;
					}

					memcpy( email_snapshot->data, snapshot->data, snapshot->data_size);

					email_snapshot->data_size = snapshot->data_size;
					email_snapshot->ch = e_data->snapshot_ch;
					email_snapshot->is_complete = 1;
					email_snapshot->capture_time = snapshot->ctime;

snap_proc_failed:
					#if defined(DEBUG_ACTION_SNAPSHOT)
					printf("\033[0;33m %s [DEBUG_ACTION_SNAPSHOT] snap_proc_failed \033[0;39m\n", __FUNCTION__);
					#endif
					nf_jpeg_man_free_snapshot(snapshot);

					break;
				}

				g_usleep(33000);
			}while( --retry_cnt >0  );

			if(retry_cnt == 0)
				g_warning("email.. get jpeg fail!!");

snap_get_failed:
			#if defined(DEBUG_ACTION_SNAPSHOT)
			printf("\033[0;33m %s [DEBUG_ACTION_SNAPSHOT] snap_get_failed \033[0;39m\n", __FUNCTION__);
			#endif
			snapshot = NULL;
		}
	}
#endif

	if(e_state->send_time <= curr_timeval.tv_sec)
	{
		#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_EMAIL_ACTION] )
				g_message("%s email_send", __FUNCTION__);
		#endif
		for( event_num=0; event_num < EMAIL_EVENT_TYPE0_NR; ++event_num )
		{
			event_sum=0;
			for( min_num=0; min_num <= e_state->div_frequency; ++min_num)
				event_sum += e_send_data->type0.rise[event_num][min_num];
			e_send_data->type0.rise[event_num][63] = (gushort)event_sum;
		}
		// email event type1
		for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			gint ch_max=0;

			if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					ch_max = _nf_action_num_alarm;
				#else
					ch_max=NUM_ALARM;
				#endif
			} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
				ch_max=NF_ACTION_REC_EVENT_NR;
			} else {
				ch_max=NUM_ACTIVE_CH;
			}

			for( ch_num=0; ch_num < ch_max; ++ch_num )
			{
				if( _email_is_act(ch_num, 0, 0, event_num, 0) )
				{
					event_sum=0;
					for( min_num=0; min_num <= e_state->div_frequency; ++min_num)
						event_sum += e_send_data->type1[event_num].rise[ch_num][min_num];
					e_send_data->type1[event_num].rise[ch_num][63] = (gushort)event_sum;
				}
			}
		}

		// email event type2
		for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE2_HDD)
				event_max=NF_ACTION_HDD_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
				event_max=NF_ACTION_SYSTEM_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_NET)
				event_max=NF_ACTION_NET_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
				event_max=1;

			for( event_cate=0; event_cate<event_max; event_cate++ )
			{
				if( _email_is_act(0, event_cate, 0, event_num, 1) )
				{
					event_sum = 0;
					for( min_num=0; min_num < e_state->div_frequency; ++min_num)
						event_sum += e_send_data->type2.rise[event_num][event_cate][min_num];


					e_send_data->type2.rise[event_num][event_cate][63] = (gushort)event_sum;
				}
			}
		}

		// email event type3
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE3_NR; event_num++ )
			{
				gint event_max=0;

				#if defined(SUPPORT_VCA_CAMERA)
					if(event_num == EMAIL_EVENT_TYPE3_VCA)
						event_max=NF_ACTION_VCA_NR;
				#endif

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _email_is_act(0, 0, ch_num, event_num, 2) )
					{
						event_sum = 0;
						for( min_num=0; min_num < e_state->div_frequency; ++min_num)
							event_sum += e_send_data->type3.rise[ch_num][event_num][event_cate][min_num];
						#if defined(EVENTALARMDEBUG)
						//#if 0
						if(event_sum > 0)
								printf("\033[0;32m %s im event_sum %d \033[0;39m\n", __FUNCTION__,event_sum);
						#endif
						e_send_data->type3.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}
		/** DVA Event **/
		// email event type4
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE4_NR; event_num++ )
			{
				gint event_max=0;
				if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
					event_max=NF_ACTION_DVA_IDZ_NR;
				else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
					event_max=NF_ACTION_DVA_IPZ_NR;
				else if(event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) /*DVA OBJ CNT*/
					event_max=NF_ACTION_DVA_IDZ_NR;

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _email_is_act(0, 0, ch_num, event_num, 3) )
					{
						event_sum = 0;
						for( min_num=0; min_num < e_state->div_frequency; ++min_num)
							event_sum += e_send_data->type4.rise[ch_num][event_num][event_cate][min_num];

						e_send_data->type4.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}
		// email event type5
		/** IMSI AI BOX **/
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE5_NR; event_num++ )
			{
				gint event_max=0;

				if(event_num == EMAIL_EVENT_TYPE5_AI)
					event_max=NF_ACTION_AI_NR;

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _email_is_act(0, 0, ch_num, event_num, 5) )
					{
						event_sum = 0;
						for( min_num=0; min_num < e_state->div_frequency; ++min_num)
							event_sum += e_send_data->type5.rise[ch_num][event_num][event_cate][min_num];
						#if defined(EVENTALARMDEBUG)
						//#if 0
						if(event_sum > 0)
								printf("\033[0;32m %s im event_sum %d \033[0;39m\n", __FUNCTION__,event_sum);
						#endif
						#if defined (DEBUG_AI)
						if(event_sum > 0)
							printf("\033[0;32m %s [DEBUG_AI] im event_sum %d \033[0;39m\n", __FUNCTION__,event_sum);
						#endif
						e_send_data->type5.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}

		// e-mail send
		#if defined(ENABLE_EMAIL_DUAL_SERVER)
			ret = _email_send_by_user(curr_timeval.tv_sec/*e_state->send_time*/);
		#else
			ret = _email_send(curr_timeval.tv_sec/*e_state->send_time*/);
		#endif
		if (ret == -1)
		{
			g_warning("%s _email_send() ret[%d]", __FUNCTION__, ret);
		}

		// time resetting and snapshot free
		if (ret == 1)
		{
			_email_reset(curr_timeval.tv_sec);
			_nf_action->pos_reset_flag |= NF_ACTION_POS_DATA_RESET_EMAIL;
		}

	}
	else
	{
		if(curr_timeval.tv_sec >= e_state->start_time)
		{
			e_state->div_count = (curr_timeval.tv_sec - e_state->start_time) / 60;

			// g_assert;
			if( e_state->div_count >= NF_ACTION_EMAIL_MAX_INTERVAL)
			{
				g_warning("%s div_count overflow[%d]", __FUNCTION__, e_state->div_count);
				e_state->div_count = NF_ACTION_EMAIL_MAX_INTERVAL - 1;
			}
		}
	}
}




/******************************************************************************************
  FTP
 ******************************************************************************************/
#ifdef ENABLE_ACTION_FTP_SEND

static void _ftp_init(void)
{
#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_INIT] )
		g_message("%s called", __FUNCTION__ );
#endif
	_ftp_reset(0);
}

static void
_ftp_reset(glong curr_time)
{
	FTP_DATA		*e_data = &_nf_action->ftp_data;
	EMAIL_STATE		*e_state = &_nf_action->ftp_state;

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_RESET] )
		g_message("%s called", __FUNCTION__ );
#endif

	memset(&_nf_action->ftp_state, 0, sizeof(_nf_action->ftp_state));
	memset(_nf_action->ftp_send_data, 0, sizeof(EMAIL_EVENT_DATA));

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	if( _nf_action->ftp_snapshot.data )
	{
		g_free(_nf_action->ftp_snapshot.data);
		_nf_action->ftp_snapshot.data = NULL;
	}
	memset(&_nf_action->ftp_snapshot, 0, sizeof(_nf_action->ftp_snapshot));
#endif

#if defined(NF_ACTOIN_ENABLE_VIDEO_CHECK)
	memset(&_nf_action->ftp_video, 0, sizeof(_nf_action->ftp_video));
#endif

	if( curr_time == 0)
	{
		GTimeVal        curr_timeval;
		gettimeofday((struct timeval *)&curr_timeval, NULL);

		curr_time = curr_timeval.tv_sec;
		e_state->start_time = curr_time;
		e_state->send_time = 5 + curr_time;
		e_state->div_frequency = 1;
		return ;
	}else{
		e_state->start_time = curr_time;
	}

	if ( e_data->frequency == 0 )
	{
		e_state->send_time = 5 + curr_time;
		e_state->div_frequency = 1;
	}
	else
	{
#if 1
		e_state->send_time = (glong)(e_data->frequency * 60) + curr_time;
		e_state->div_frequency = (e_state->send_time - e_state->start_time) / 60;
#else	// for test
		e_state->send_time = (glong)curr_time+5;
		e_state->div_frequency = 1;
#endif
	}

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_RESET] )
		g_message("%s start[%ld] freq[%d] send[%ld] div_freq[%ld]", __FUNCTION__,
				curr_time, e_data->frequency, e_state->send_time, e_state->div_frequency );
#endif

}

static gint
_ftp_send(time_t send_time)
{
	NF_NET_SEND_CONTENT cont;

	gint ch_num=0, event_num=0, min_num=0, event_cate=0;
	time_t e_time = send_time;
	time_t e_time_for_weblink = send_time;

	struct tm st_buff;
	struct tm *st = &st_buff;
	gchar time_buff[256];

	// for ftp
	char webra_link_pb_time[32];
	int	 webra_link_ch_cnt = 0;
	int  webra_link_ch = -1;
	char ftp_date[32], ftp_time[32];

	gchar buffer[NF_NET_SEND_MAX_CONTENTS]={0, }, user_buff[256]={0, };
	gchar *pos=buffer, *tmp_buff = NULL;
	gint user_count=0, send_usr_cnt=0, addr_cnt=0, cnt=0;
	gboolean sndflag = FALSE, is_dst=FALSE;
	gboolean booting = FALSE;
	gint lang_id = 0, num_alarm=0;
	gint ret=0, title_print=0;

	EMAIL_SNAPSHOT_DATA	*ftp_snapshot = &_nf_action->ftp_snapshot;
	FTP_VIDEO_DATA		*ftp_video = &_nf_action->ftp_video;

	EMAIL_EVENT_DATA	*e_send_data = _nf_action->ftp_send_data;

	POS_TEXT_DATA		*p_text_data = _nf_action->ftp_pos_text_data;
	// for ftp
	FTP_DATA			*ftp_data = &_nf_action->ftp_data;

	guint64 video_start_time;
	guint64 video_end_time;

	memset(&cont, 0, sizeof(cont));
	memset(buffer, 0, sizeof(buffer));

#if 1
	lang_id = _email_get_lang_id();

	if(lang_id >= NF_ACTION_LANG_ID_MAX)
	{
		g_warning("%s Email LangID is False", __FUNCTION__);
		return FALSE;
	}
#endif

	is_dst = _nf_action->is_dst;
	if( is_dst == 0 && nf_datetime_is_dst( e_time ) )
		e_time -= 3600;

	{
		char date_str[128], time_str[128];

		guint datefmt = nf_sysdb_get_uint("sys.date.dateform");
		guint timefmt = nf_sysdb_get_uint("sys.date.timeform");

		localtime_r(&e_time, st);

		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(date_str, sizeof(date_str)-1, "%04d/%02d/%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d %s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		snprintf(time_buff, sizeof(time_buff)-1, "%s %s", date_str, time_str);

	}

	{
		char date_str[128], time_str[128];

		guint datefmt = nf_sysdb_get_uint("sys.date.dateform");
		guint timefmt = nf_sysdb_get_uint("sys.date.timeform");

		localtime_r(&e_time, st);

		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(date_str, sizeof(date_str)-1, "%04d/%02d/%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d %s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		snprintf(time_buff, sizeof(time_buff)-1, "%s %s", date_str, time_str);

		// for ftp time
		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(ftp_date, sizeof(ftp_date)-1, "%04d%02d%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(ftp_date, sizeof(ftp_date)-1, "%02d%02d%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(ftp_date, sizeof(ftp_date)-1, "%02d%02d%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(ftp_time, sizeof(ftp_time)-1, "%02d%02d%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(ftp_time, sizeof(ftp_time)-1, "%02d%02d%02d%s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		// -5 sec
		e_time_for_weblink -= 5;
		snprintf(webra_link_pb_time, sizeof(webra_link_pb_time)-1,"%ld", e_time_for_weblink);
		// gmtime_r(&e_time_for_weblink, st);
		// snprintf(webra_link_pb_time, sizeof(webra_link_pb_time)-1,
		// 		"%04d%02d%02d%02d%02d%02d",
		// 		st->tm_year+1900, st->tm_mon + 1, st->tm_mday,
		// 		st->tm_hour, st->tm_min, st->tm_sec	);
	}

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s : %.24s\n",
			_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_TIME], time_buff);

	tmp_buff = nf_sysdb_get_str_nocopy("sys.info.sysid");
	if (tmp_buff == NULL)
	{
		g_warning("%s sys.info.sysid error", __FUNCTION__);
		return -1;
	}

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s : %s\n\n\n",
			_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_SYSTEM_ID], tmp_buff);

	/*

	   <item key="act.ftp_noti.dir_mode"            type="UINT"      min="0" max="3" val="0" />
	   Val = 0 -> MANUAL => ?�용?��? 직접?�력 (?�act.ftp_noti.dir_path") DB ?�용
	   Val = 1 -> MANUAL_DATE => ?�용?��? 직접?�력 (?�act.ftp_noti.dir_path")  ?�에 ?�짜가 추�???	   Val = 2 -> SYSTEM ID => ?�스???�임???�용?�여 ?�성
	   Val = 3 -> SYSTEM ID_DATE => ?�스??ID ?�에 ?�짜가 추�???
	   <item key="act.ftp_noti.fname_mode"                  type="UINT"      min="0" max="3" val="0" />
	   Val = 0 -> MANUAL => ?�용?��? 직접?�력 ("act.ftp_noti.fname_prefix") DB ?�용
	   Val = 1 -> MANUAL_DATE_TIME => ?�용?��? 직접?�력 ("act.ftp_noti.fname_prefix")  ?�에 ?�짜?� ?�간??추�???	   Val = 2 -> SYSTEM ID => ?�스???�임???�용?�여 ?�성
	   Val = 3 -> SYSTEM ID_DATE_TIME => ?�스??ID ?�에 ?�짜?� ?�간??추�???
	   <item key="act.ftp_noti.fname_prefix"        type="STRING"   min="0" max="256" val="" />
	   -	"act.ftp_noti.fname_mode" ??모드가 MANUAL ???�닐??(SYSTEM ID/SYSTEM ID_DATE_TIME) ?�용

*/

	if( ftp_data->dir_mode == 0  || ftp_data->dir_mode == 1) // manual
		snprintf( cont.ftp_dir, sizeof(cont.ftp_dir), "%s",  ftp_data->dir_path);
	else
		snprintf( cont.ftp_dir, sizeof(cont.ftp_dir), "%s",  tmp_buff);		// system id

	if( ftp_data->dir_mode == 1  || ftp_data->dir_mode == 3) {
		strcat( cont.ftp_dir, "_" );
		strcat( cont.ftp_dir, ftp_date );
	}

	if( ftp_data->fname_mode == 0  || ftp_data->fname_mode == 1)
		snprintf( cont.subject, sizeof(cont.subject), "%s",  ftp_data->fname_prefix);
	else
		snprintf( cont.subject, sizeof(cont.subject), "%s",  tmp_buff);		// system id

	if( ftp_data->fname_mode == 1  || ftp_data->fname_mode == 3) {
		strcat( cont.subject, "_" );
		strcat( cont.subject, ftp_date );
		strcat( cont.subject, "_" );
		strcat( cont.subject, ftp_time );
	}

	// for null subject
	if( !cont.subject[0] )
		strcat( cont.subject, "default");
	/** IMSI AI BOX **/
	//  Type5 Event
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			gboolean is_title_print=FALSE;
			gboolean is_send=FALSE;
			gboolean is_first=TRUE;
	
			for( event_num = 0; event_num < EMAIL_EVENT_TYPE5_NR; event_num++ )
			{
				gint event_max=0;
				gboolean is_event_print=FALSE;
				gint lang_id_event=0;
	
				if(event_num == EMAIL_EVENT_TYPE5_AI)
				{
					event_max=NF_ACTION_AI_NR;
				}
	
				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					#if defined(VIDECON)
						int idx_ai_evt_type = 0;
					#endif
					if(event_num == EMAIL_EVENT_TYPE5_AI)
					{
                                                if(event_cate == NF_ACTION_AI_GENERIC)
						{
							lang_id_event=LANG_ID_AI_GENERIC;
							#if defined(VIDECON)
								idx_ai_evt_type = NF_ACTION_IDX_GENERIC_EVENT;
							#endif
						}
						else if(event_cate == NF_ACTION_AI_FR)
							lang_id_event=LANG_ID_AI_FR;
						else if(event_cate == NF_ACTION_AI_LPR)
							lang_id_event=LANG_ID_AI_LPR;
						else
						{
							lang_id_event=LANG_ID_AI;
							#if defined(VIDECON)
								idx_ai_evt_type = NF_ACTION_IDX_HUMAN_VEHICLE_DETECTOR;
							#endif
						}
					}
					
					if (e_send_data->type5.rise[ch_num][event_num][event_cate][63])
					{
					#if 0
							g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
						if(!is_title_print)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
									_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
							is_title_print=TRUE;
						}
                                                if((event_num == EMAIL_EVENT_TYPE5_AI) && (event_cate == NF_ACTION_AI_GENERIC))
						{
							if(is_event_print)
									pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
									
							#if defined(VIDECON)
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s)    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_AI], _ACTION_STR_TABLE_DLVA_EVENT_TYPE[idx_ai_evt_type]);
							#else
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
											_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
							#endif
							if(event_cate == NF_ACTION_AI_GENERIC)
								_make_ai_send_string(&pos, buffer, (void *)e_send_data->type5.st_generic_evt[ch_num], NF_ACTION_AI_GENERIC);
							
						}
						else if((event_num == EMAIL_EVENT_TYPE5_AI) && (event_cate == NF_ACTION_AI_FR || event_cate == NF_ACTION_AI_LPR))
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
										_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
							if(event_cate == NF_ACTION_AI_FR)
								_make_ai_send_string(&pos, buffer, (void *)&_nf_action->ai_fr_data[ch_num],NF_ACTION_AI_FR);
							else if(event_cate == NF_ACTION_AI_LPR)
								_make_ai_send_string(&pos, buffer, (void *)&_nf_action->ai_lpr_data[ch_num],NF_ACTION_AI_LPR);
						}
						else
						{
	
							if(!is_event_print)
							{
									#if defined(VIDECON)
										pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s)    :   ",
													_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_AI], _ACTION_STR_TABLE_DLVA_EVENT_TYPE[idx_ai_evt_type]);
									#else
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
										_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
									#endif
								is_event_print=TRUE;
							}
		
							if(is_first)
							{
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s) ",
										_ACTION_STR_TABLE_TYPE5[lang_id][event_num][event_cate], e_send_data->type5.text[ch_num][event_num][event_cate]);
		
								is_first=FALSE;
							}
							else
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), ",%s(%s) ",
										_ACTION_STR_TABLE_TYPE5[lang_id][event_num][event_cate], e_send_data->type5.text[ch_num][event_num][event_cate]);
						}
	
						sndflag = TRUE;
						is_send = TRUE;
					}
				}
	
				if(is_send)
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
			}
		}
    /** DVA Event **/
	//  Type4 Event
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		gboolean is_title_print=FALSE;
		gboolean is_send=FALSE;
		gboolean is_first=TRUE;
		char is_first_obj_cnt = 7;
		
		for( event_num = 0; event_num < EMAIL_EVENT_TYPE4_NR; event_num++ )
		{
			gint event_max=0;
			gboolean is_event_print=FALSE;
			gint lang_id_event=0;
			gboolean is_animal_first = TRUE;
			
			if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
			{
				event_max=NF_ACTION_DVA_IDZ_NR;
				lang_id_event=LANG_ID_DVA_IDZ;
			}
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
			{
				event_max=NF_ACTION_DVA_IPZ_NR;
				lang_id_event=LAND_ID_DVA_IPZ;
			}
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT)
			{
				event_max=NF_ACTION_DVA_IDZ_NR;
				lang_id_event=LANG_ID_DVA_OBJ_CNT;
			}


			for( event_cate=0; event_cate<event_max; event_cate++)
			{
				if (e_send_data->type4.rise[ch_num][event_num][event_cate][63])
				{
					#if 0
						g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
					if(!is_title_print)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
						is_title_print=TRUE;
					}

					if(!is_event_print)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
								_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
						is_event_print=TRUE;
					}

					if(is_first)
					{
						if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
						{
							if(!is_animal_first)
								continue;
							else
								is_animal_first = FALSE;
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
						}
						else if(((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate == NF_ACTION_DVA_IDZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IDZ_BICYCLE))
							 || ((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (event_cate == NF_ACTION_DVA_IPZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IPZ_BICYCLE)))
						{
							if(lang_id == 0)
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","bike");
							else if(lang_id == 1)
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","������");
						}
						else
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",_ACTION_STR_TABLE_TYPE4[lang_id][event_num][event_cate]);

						is_first=FALSE;
					}
					else
					{
						if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
						{
							if(!is_animal_first)
								continue;
							else
								is_animal_first = FALSE;
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
						}
						else if(((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate == NF_ACTION_DVA_IDZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IDZ_BICYCLE))
							 || ((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (event_cate == NF_ACTION_DVA_IPZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IPZ_BICYCLE)))
						{
							if(lang_id == 0)
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","bike");
							else if(lang_id == 1)
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ","������");
						}
						/*DVA OBJ CNT*/
						else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
						{
							if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_ANIMAL) & 0x01))
								continue;
							else
								is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_ANIMAL));
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL]);
						}
						else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate >= NF_ACTION_DVA_IDZ_BICYCLE && event_cate <= NF_ACTION_DVA_IDZ_CAR))
						{
							if(!((is_first_obj_cnt >> NF_ACTION_DVA_DESC_VEHICLE) & 0x01))
								continue;
							else
								is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_VEHICLE));
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_VEHICLE]);
						}
						else if((event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT) && (event_cate == NF_ACTION_DVA_IDZ_HUMAN))
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s count, ", _ACTION_STR_TABLE_TYPE4[lang_id][EMAIL_EVENT_TYPE4_DVA_IDZ][NF_ACTION_DVA_IDZ_HUMAN]);
						}
						else
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",_ACTION_STR_TABLE_TYPE4[lang_id][event_num][event_cate]);

					}
					sndflag = TRUE;
					is_send = TRUE;
				}
			}

			if(is_send)
				pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
		}
	}


	//  Type3 Event
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		gboolean is_title_print=FALSE;
		gboolean is_send=FALSE;
		gboolean is_first=TRUE;

		for( event_num = 0; event_num < EMAIL_EVENT_TYPE3_NR; event_num++ )
		{
			gint event_max=0;
			gboolean is_event_print=FALSE;
			gint lang_id_event=0;

			#if defined(SUPPORT_VCA_CAMERA)
				if(event_num == EMAIL_EVENT_TYPE3_VCA)
				{
					event_max=NF_ACTION_VCA_NR;
					lang_id_event=LANG_ID_VCA;
				}
			#endif

			for( event_cate=0; event_cate<event_max; event_cate++)
			{
				if (e_send_data->type3.rise[ch_num][event_num][event_cate][63])
				{
					#if 0
						g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
					if(!is_title_print)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "< %s %d >\n",
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], (ch_num + 1));
						is_title_print=TRUE;
					}

					if(!is_event_print)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s    :   ",
								_ACTION_STR_TABLE_TYPE1[lang_id][lang_id_event]);
						is_event_print=TRUE;
					}

					if(is_first)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s ",
								_ACTION_STR_TABLE_TYPE3[lang_id][event_num][event_cate]);

						is_first=FALSE;
					}
					else
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), ",%s ",
								_ACTION_STR_TABLE_TYPE3[lang_id][event_num][event_cate]);

					sndflag = TRUE;
					is_send = TRUE;
				}
			}

			if(is_send)
				pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "\n");
		}
	}

	// 	Type2 Event
	for( event_num = EMAIL_EVENT_TYPE2_HDD; event_num < EMAIL_EVENT_TYPE2_NR; event_num++ )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++)
		{
			if (e_send_data->type2.rise[event_num][event_cate][63])
			{
				if(((event_num == EMAIL_EVENT_TYPE2_NET) && (event_cate == NF_ACTION_NET_EVENT_LOGON_FAIL)) ||
						((event_num == EMAIL_EVENT_TYPE2_SYSTEM) && (event_cate == NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL)))
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s	%d	%s\n",
							_ACTION_STR_TABLE_TYPE2[lang_id][event_num][event_cate],
							e_send_data->type2.rise[event_num][event_cate][63],
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );
				}
				else
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %s",
							_ACTION_STR_TABLE_TYPE2[lang_id][event_num][event_cate],
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_OCCUR] );
					#if 0
					if( event_cate == NF_ACTION_NET_EVENT_TROUBLE_AI_BOX )
					{		
						char str_riseCh[128] = {0,};
						_make_send_string(str_riseCh, 128, 0);
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s", str_riseCh);
					}
					#endif
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%c", '\n');
				}
				sndflag = TRUE;
			}
		}
	}

	if (e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[NF_ACTION_REC_EVENT_PANIC][63])
	{
		pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s	%s\n\n",
				_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_REC_PANIC],
				_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_OCCUR] );
		sndflag = TRUE;
	}

	// Type1 Event
	for( ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++ )
	{
		title_print=FALSE;

		for( event_num = EMAIL_EVENT_TYPE1_SENSOR; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			if (event_num == EMAIL_EVENT_TYPE1_REC_PANIC)
				continue;
#if defined(ENABLE_SENSOR_IPCAM)
			if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
			{
				/**
				  When IPCAM+DVR, This is DVR String
				 **/
				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				if (ch_num < _nf_action_num_nvr_alarm)
				#else
				if(ch_num < NUM_ALARM_DVR)
				#endif
				{
					gint index=0;

					index=NUM_ALARM_IPCAM+ch_num;

					#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					if (index < _nf_action_num_alarm)
					#else
					if(index < NUM_ALARM)
					#endif
					{
						if (e_send_data->type1[event_num].rise[index][63])
						{
							gchar tmp_key[64]={0, }, *str=NULL, str_lcamera[64] = {0, };
							gint cnt_ch=0;
							guint mask_lcamera = _nf_action->sensor_data[index].mask_lcamera;

							sprintf(tmp_key, "alarm.sensor.S%d.name", index);
							str = nf_sysdb_get_str_nocopy(tmp_key);

							for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
							{
								if((mask_lcamera >> cnt_ch) & 0x1)
									sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
							}

							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%-16.16s : %3d %s - %s\n",
									str, e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[index][63],
									_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR], str_lcamera);

							sndflag = TRUE;
						}
					}
				}
			}
#endif

			if (e_send_data->type1[event_num].rise[ch_num][63])
			{
				if(title_print == FALSE)
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "< %s %2d >\n",
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_CH], ch_num+1);
					title_print = TRUE;
				}

				if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
				{
					/**
					  When IPCAM+DVR, This is IPCAM String
					  When Only DVR, this is DVR String
					 **/
					#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					if (ch_num < _nf_action_num_alarm)
					#else
					if(ch_num < NUM_ALARM)
					#endif
					{
						gchar tmp_key[64]={0, }, *str=NULL, str_lcamera[64] = {0, };
						gint cnt_ch=0;
						guint mask_lcamera = _nf_action->sensor_data[ch_num].mask_lcamera;

						sprintf(tmp_key, "alarm.sensor.S%d.name", ch_num);
						str = nf_sysdb_get_str_nocopy(tmp_key);

						for(cnt_ch=0; cnt_ch<NUM_ACTIVE_CH; cnt_ch++)
						{
							if((mask_lcamera >> cnt_ch) & 0x1)
								sprintf(str_lcamera,  "%s CH%d ", str_lcamera, cnt_ch+1);
						}

						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%-16.16s : %3d %s - %s\n",
								str, e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[ch_num][63],
								_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR], str_lcamera);
					}
				}
				else if(event_num == EMAIL_EVENT_TYPE1_POS)
				{
					gushort pos_data_cnt = 0;
					gushort pos_data_max_cnt = e_send_data->type1[event_num].rise[ch_num][63];
					GTimeVal	tv_pos;
					memset(&tv_pos, 0x00, sizeof(tv_pos));

					if(pos_data_max_cnt > NF_ACTION_POS_TEXT_DATA_MAX_NUM)
						pos_data_max_cnt = NF_ACTION_POS_TEXT_DATA_MAX_NUM;

					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s\n",
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_POS],
							e_send_data->type1[event_num].rise[ch_num][63],
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );

					for(pos_data_cnt=0; pos_data_cnt<pos_data_max_cnt; pos_data_cnt++)
					{
						if((NF_MAIL_SEND_MAX_CONTENTS - strlen(buffer)) > NF_ACTION_POS_TEXT_MAX_LENGTH)
						{
							gchar str_time[128]={0, };
							time_t ltime;
							struct tm st_buff;
							struct tm *st = &st_buff;

							g_static_mutex_lock (&_nf_pos_data_ftp_mutex);
							GUINT64_TO_GTIMEVAL(p_text_data[ch_num].timestamp[pos_data_cnt], tv_pos);
							#if 0
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), " - POS %d %d %s\n",
									ch_num+1, tv_pos.tv_sec, p_text_data[ch_num].data[pos_data_cnt]);
							#else
								ltime=tv_pos.tv_sec;
								localtime_r(&ltime, st);
								pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),
											" - POS %d %04d%02d%02d_%02d:%02d:%02d %s\n",
											ch_num+1, st->tm_year+1900, st->tm_mon+1, st->tm_mday,
											st->tm_hour, st->tm_min, st->tm_sec,
											p_text_data[ch_num].data[pos_data_cnt]);
							#endif
							g_static_mutex_unlock (&_nf_pos_data_ftp_mutex);

						}
					}
				}
				else
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s %3d %s\n",
							_ACTION_STR_TABLE_TYPE1[lang_id][event_num],
							e_send_data->type1[event_num].rise[ch_num][63],
							_ACTION_STR_TABLE_TYPE1[lang_id][LANG_ID_COUNT_OCCUR] );
				}

				sndflag = TRUE;
			}
		}
	}

#if 0		// for debug
	g_message("%s sndflag[%d]\nbuffer[%s]", __FUNCTION__, sndflag, buffer);
#endif

	if( sndflag == FALSE )
	{
#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_SEND] )
			g_message("%s sndflag [%d]", __FUNCTION__ , sndflag );
#endif
		return 0;
	}

#if 1
	if( ftp_data->webra_link ) {

		gboolean is_ddnson = nf_sysdb_get_bool("net.proto.ddnson");
		guint web_port = nf_sysdb_get_uint("net.proto.webport");
		guint is_https = (guint)nf_sysdb_get_bool("net.proto.httpson");
		char host_addr[256];

		if( is_ddnson ){
			ddns_get_hostaddr(host_addr, sizeof(host_addr));
		}else{
			struct sockaddr_in ipaddr;
			ipaddr.sin_addr.s_addr = htonl( nf_sysdb_get_uint(NET_IPADDR) );
			snprintf( host_addr, sizeof(host_addr), "%s", inet_ntoa(ipaddr.sin_addr));
		}

		pos += snprintf(pos, sizeof(buffer)-(pos-buffer),
				"\n\n\n%s://%s:%d/#/live?playback=%s",
				(is_https ? "https" : "http"), host_addr, web_port, webra_link_pb_time);

		snprintf(cont.webra_link, sizeof(cont.webra_link) -1 ,
				"[InternetShortcut]\r\nURL=%s://%s:%d/#/live?playback=%s\r\n",
				(is_https ? "https" : "http"), host_addr, web_port, webra_link_pb_time);
	}
#endif

	cont.is_dvr_event = 1;
	strncat( cont.contents, buffer, sizeof(cont.contents)-1 );


#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	if( ftp_snapshot->data && ftp_snapshot->data_size >0)
	{

		snprintf( cont.image_name,  sizeof(cont.image_name) - 1, "%s_ch%02d.jpg",
				cont.subject, ftp_snapshot->ch+1 );

		cont.image_size = ftp_snapshot->data_size;
		cont.image_data = ftp_snapshot->data;
	}
#endif

#if defined(NF_ACTOIN_ENABLE_VIDEO_CHECK)
	video_start_time = GTIMEVAL_TO_GUINT64(ftp_video->start_time);
	video_end_time = GTIMEVAL_TO_GUINT64(ftp_video->end_time);

	g_message("%s - %d, start:%llu, end:%llu", __FUNCTION__, __LINE__, video_start_time, video_end_time);

	if( (video_start_time != 0 ) && (video_end_time != 0) )
	{
		snprintf( cont.video_name,  sizeof(cont.video_name) - 1, "%s_ch%02d.mp4",
				cont.subject, ftp_video->ch+1);

		cont.video_ch = ftp_video->ch + 32; // captainnn 
		cont.video_start_time = video_start_time;
		cont.video_end_time = video_end_time;
	}
#endif

	g_message("%s net_send_req[%d]", __FUNCTION__, ret );
	ret = nf_net_send_request( &cont, NULL);
	g_message("%s net_send_req[%d]", __FUNCTION__, ret );

	return 1;
}

	static int
_ftp_is_act(int index_type1, int index_type2, int index_type3, int prop_id, int type)
{
	ALARM_SENSOR_DATA	*sdata	= NULL;
	MOTION_DATA			*mdata	= NULL;
	VLOSS_DATA			*vdata	= NULL;
	REC_DATA			*rpdata	= NULL;
	TEXTIN_DATA			*tdata	= NULL;

	DISK_DATA			*ddata	= NULL;
	SYSTEM_DATA			*stdata	= NULL;
	NET_DATA			*ndata	= NULL;
#if defined(ENABLE_EVENT_TAMPER)
	TAMPER_DATA			*tadata	= NULL;
#endif
#if defined(SUPPORT_VCA_CAMERA)
	VCA_DATA *vcadata=NULL;
#endif
        /** DVA Event **/
	DVA_DATA *dvadata = NULL;
	POS_DATA *pdata=NULL;

	/** IMSI AI BOX **/
	AI_DATA *aidata=NULL;
	
	if(type == 0)
	{
		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR ) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				g_return_val_if_fail( index_type1 < _nf_action_num_alarm, 0);
			#else
				g_return_val_if_fail( index_type1 < NUM_ALARM, 0);
			#endif
		} else {
			g_return_val_if_fail( index_type1 < NUM_ACTIVE_CH, 0);
		}

		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE1_NR, 0);

		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR )
		{
			sdata = &_nf_action->sensor_data[index_type1];
			return sdata->ftp_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_MOTION )
		{
			mdata = &_nf_action->motion_data[index_type1];
			return mdata->ftp_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_VLOSS )
		{
			vdata = &_nf_action->vloss_data[index_type1];
			return vdata->ftp_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_PANIC )
		{
			rpdata = &_nf_action->rec_data[index_type1];
			return rpdata->ftp_act ? 1: 0;
		}
#if 0
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_ALARM )
			return radata->ftp_act ? 1: 0;
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_MOTION )
			return rmdata->ftp_act ? 1: 0;
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_TEXT_IN )
		{
			tdata = &_nf_action->textin_data[index_type1];
			return tdata->ftp_act ? 1: 0;
		}
#if defined(ENABLE_EVENT_TAMPER)
		else if ( prop_id == EMAIL_EVENT_TYPE1_TAMPER )
		{
			tadata = &_nf_action->tamper_data[index_type1];
			return tadata->ftp_act ? 1: 0;
		}
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_POS )
		{
			pdata = &_nf_action->pos_data[index_type1];
			return pdata->ftp_act ? 1: 0;
		}
	}
	else if(type == 1)
	{
		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE2_NR, 0);

		if ( prop_id == EMAIL_EVENT_TYPE2_HDD )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_HDD_EVENT_NR, 0);

			ddata = &_nf_action->disk_data[index_type2];

			return ddata->ftp_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_SYSTEM )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_SYSTEM_EVENT_NR, 0);

			stdata = &_nf_action->system_data[index_type2];

			return stdata->ftp_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_NET )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_NET_EVENT_NR, 0);

			ndata = &_nf_action->net_data[index_type2];
			return ndata->ftp_act ? 1: 0;
		}
	}
	else if(type == 2)
	{
#if defined(SUPPORT_VCA_CAMERA)
		if ( prop_id == EMAIL_EVENT_TYPE3_VCA )
		{
			vcadata = &_nf_action->vca_data[index_type3];

			return vcadata->ftp_act ? 1: 0;
		}
#endif
	}
	else if(type == 3)/** DVA Event **/
	{
		if ( prop_id == EMAIL_EVENT_TYPE4_DVA_IDZ || prop_id == EMAIL_EVENT_TYPE4_DVA_IPZ || prop_id == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT)
		{
			dvadata = &_nf_action->dva_data[index_type3];
			return dvadata->ftp_act ? 1: 0;
		}
	}
	else if(type == 5)
	{
		/** IMSI AI BOX **/
		if ( prop_id == EMAIL_EVENT_TYPE5_AI )
		{
			aidata = &_nf_action->ai_data[index_type3];

			return aidata->ftp_act ? 1: 0;
		}
	}

	return 0;
}

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
static gboolean
_ftp_snapshot_event_check(int  div_count)
{
	gint ch_num=0, mask_num=0, min_num=0;
	gint event_num=0, event_cate=0;
	guint event_sum=0;

	if( _nf_action->ftp_snapshot.event_cnt >0 )
		return 1;

	// ftp event type1
	for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
	{
		gint ch_max=0;

		if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				ch_max = _nf_action_num_alarm;
			#else
				ch_max=NUM_ALARM;
			#endif
		} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
			ch_max=NF_ACTION_REC_EVENT_NR;
		} else {
			ch_max=NUM_ACTIVE_CH;
		}

		for( ch_num=0; ch_num < ch_max; ++ch_num )
		{
			if( _ftp_is_act(ch_num, 0, 0, event_num, 0)
					&& _nf_action->ftp_send_data->type1[event_num].rise[ch_num][div_count] )
			{
				g_get_current_time( &_nf_action->ftp_snapshot.detect_time);
				_nf_action->ftp_snapshot.event_cnt = 1;
				return 1;
			}
		}
	}

	// ftp event type2
	for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++
				&& _nf_action->ftp_send_data->type2.rise[event_num][event_cate][div_count] )
		{
			if( _ftp_is_act(0, event_cate, 0, event_num, 1) )
			{
				g_get_current_time( &_nf_action->ftp_snapshot.detect_time);
				_nf_action->ftp_snapshot.event_cnt = 1;
				return 1;
			}
		}
	}

	// ftp event type3
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE3_NR; ++event_num )
		{
			gint event_max=0;

#if defined(SUPPORT_VCA_CAMERA)
			if(event_num == EMAIL_EVENT_TYPE3_VCA)
				event_max=NF_ACTION_VCA_NR;
#endif

			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _ftp_is_act(0, 0, ch_num, event_num, 2) )
				{
					g_get_current_time( &_nf_action->email_snapshot.detect_time);
					_nf_action->email_snapshot.event_cnt = 1;
					return 1;
				}
			}
		}
	}

	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE4_NR; ++event_num )
		{
			gint event_max=0;


			if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
				event_max=NF_ACTION_DVA_IDZ_NR;
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
				event_max=NF_ACTION_DVA_IPZ_NR;

			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _ftp_is_act(0, 0, ch_num, event_num, 3) )
				{
					g_get_current_time( &_nf_action->email_snapshot.detect_time);
					_nf_action->email_snapshot.event_cnt = 1;
					return 1;
				}
			}
		}
	}
	
	// ftp event type5
	/** IMSI AI BOX **/
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE5_NR; ++event_num )
		{
			gint event_max=0;
	

			if(event_num == EMAIL_EVENT_TYPE5_AI)
				event_max=NF_ACTION_AI_NR;
	
			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _ftp_is_act(0, 0, ch_num, event_num, 5) )
				{
					g_get_current_time( &_nf_action->email_snapshot.detect_time);
					_nf_action->email_snapshot.event_cnt = 1;
					return 1;
				}
			}
		}
	}
	
	return 0;
}
#endif

#if defined(NF_ACTOIN_ENABLE_VIDEO_CHECK)
	static gboolean
_ftp_video_event_check(int  div_count)
{
	gint ch_num=0, mask_num=0, min_num=0;
	gint event_num=0, event_cate=0;
	guint event_sum=0;

	if( _nf_action->ftp_video.event_cnt >0 )
		return 1;

	// ftp event type1
	for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
	{
		gint ch_max=0;

		if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				ch_max = _nf_action_num_alarm;
			#else
				ch_max=NUM_ALARM;
			#endif
		} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
			ch_max=NF_ACTION_REC_EVENT_NR;
		} else {
			ch_max=NUM_ACTIVE_CH;
		}

		for( ch_num=0; ch_num < ch_max; ++ch_num )
		{
			if( _ftp_is_act(ch_num, 0, 0, event_num, 0)
					&& _nf_action->ftp_send_data->type1[event_num].rise[ch_num][div_count] )
			{
				_nf_action->ftp_video.event_cnt = 1;
				return 1;
			}
		}
	}

	// ftp event type2
	for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++
				&& _nf_action->ftp_send_data->type2.rise[event_num][event_cate][div_count] )
		{
			if( _ftp_is_act(0, event_cate, 0, event_num, 1) )
			{
				_nf_action->ftp_video.event_cnt = 1;
				return 1;
			}
		}
	}

	// ftp event type3
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE3_NR; ++event_num )
		{
			gint event_max=0;

#if defined(SUPPORT_VCA_CAMERA)
			if(event_num == EMAIL_EVENT_TYPE3_VCA)
				event_max=NF_ACTION_VCA_NR;
#endif

			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _ftp_is_act(0, 0, ch_num, event_num, 2) )
				{
					_nf_action->ftp_video.event_cnt = 1;
				}
			}
		}
	}

	// ftp event type4
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE4_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
				event_max=NF_ACTION_DVA_IDZ_NR;
			else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
				event_max=NF_ACTION_DVA_IPZ_NR;
			
			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _ftp_is_act(0, 0, ch_num, event_num, 3) )
				{
					_nf_action->ftp_video.event_cnt = 1;
				}
			}
		}
	}
	
	/** IMSI AI BOX **/
	// ftp event type5
	for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
	{
		for( event_num=0; event_num < EMAIL_EVENT_TYPE5_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE5_AI)
				event_max=NF_ACTION_AI_NR;

			for( event_cate=0; event_cate<event_max; ++event_cate)
			{
				if( _ftp_is_act(0, 0, ch_num, event_num, 5) )
				{
					_nf_action->ftp_video.event_cnt = 1;
				}
			}
		}
	}
	return 0;
}
#endif

	static void
_ftp_event_check(FTP_DATA *e_data, EMAIL_EVENT_DATA *e_send_data, EMAIL_STATE *e_state)
{

	gint 	div_count = e_state->div_count;
	gint	i=0;
	guint 	mask_snapshot_ch=0, mask_motion=0, mask_lcamera_alarm=0, mask_rec_panic=0, mask_pos=0;
	gint64 	mask=0;
	ALARM_SENSOR_DATA *sdata=NULL;
	MOTION_DATA *mdata=NULL;
	REC_DATA *rdata=NULL;

#if defined(ENABLE_EVENT_TAMPER)
	TAMPER_DATA *tdata=NULL;
	guint mask_tamper=0;
#endif
#if defined(SUPPORT_VCA_CAMERA)
	VCA_DATA *vcadata=NULL;
	guint mask_vca=0;
#endif
	DVA_DATA *dvadata=NULL;
	guint mask_dva=0;

	AI_DATA *aidata=NULL;
	guint mask_ai=0;
	
	POS_DATA *pdata=NULL;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);

		if( _nf_action->curr_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].active[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->curr_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].active[i][div_count]++;
		if( _nf_action->curr_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].active[i][div_count]++;
#if 0
		if( _nf_action->curr_rec_alarm & mask )
			e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_ALARM][div_count]++;

		if( _nf_action->curr_rec_motion & mask )
			e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_MOTION][div_count]++;
#endif
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->curr_tamper & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].active[i][div_count]++;
#endif

#if defined(SUPPORT_VCA_CAMERA)
		if(_nf_action->curr_vca[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_vca_get_evt_mask(_nf_action->curr_vca[i]);

			for(evt_max=0; evt_max<16; evt_max++)
			{
				if(mask & (1 << evt_max))
					e_send_data->type3.active[i][EMAIL_EVENT_TYPE3_VCA][evt_max][div_count]++;
			}
		}
#endif
                /** DVA Event **/
		if( _nf_action->curr_dva_idz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->curr_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IDZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.active[i][EMAIL_EVENT_TYPE4_DVA_IDZ][evt_max][div_count]++;
			}
		}
		if( _nf_action->curr_dva_ipz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->curr_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IPZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IPZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.active[i][EMAIL_EVENT_TYPE4_DVA_IPZ][evt_max][div_count]++;
			}
		}
		/*DVA OBJ CNT*/
		if( _nf_action->curr_dva_obj_cnt & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->curr_dva_event[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
				{
					e_send_data->type4.active[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT][evt_max][div_count]++;
					#if defined(DEBUG_DVA_OBJ_CNT)
					printf("\033[0;35m %s CURR \033[0;39m\n", __FUNCTION__);
					#endif
				}
			}
		}
		
		/** IMSI AI BOX **/
		if(_nf_action->curr_ai[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_ai_get_evt_mask(_nf_action->curr_vca[i]);

			for(evt_max=0; evt_max<NF_ACTION_AI_NR; evt_max++)
			{
				if(mask & (1 << evt_max))
					e_send_data->type5.active[i][EMAIL_EVENT_TYPE5_AI][evt_max][div_count]++;
			}
		}
		
		if( _nf_action->curr_pos & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_POS].active[i][div_count]++;
	}

	// 2010-08-02 ?�후 1:55:05 choissi
	if( _nf_action->curr_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].active[0][div_count]++;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);

		if( _nf_action->rise_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->rise_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].rise[i][div_count]++;
		if( _nf_action->rise_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].rise[i][div_count]++;
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->rise_tamper & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].rise[i][div_count]++;
#endif

#if defined(SUPPORT_VCA_CAMERA)
		if(_nf_action->curr_vca[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_vca_get_evt_mask(_nf_action->rise_vca[i]);

			for(evt_max=0; evt_max<16; evt_max++)
			{
				if(mask & (1 << evt_max))
					e_send_data->type3.rise[i][EMAIL_EVENT_TYPE3_VCA][evt_max][div_count]++;
			}
		}
#endif
        /** DVA Event **/
		if( _nf_action->rise_dva_idz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->rise_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IDZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.rise[i][EMAIL_EVENT_TYPE4_DVA_IDZ][evt_max][div_count]++;
			}
		}
		if( _nf_action->rise_dva_ipz & mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->rise_dva_event[i][EMAIL_EVENT_TYPE4_DVA_IPZ];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IPZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
					e_send_data->type4.rise[i][EMAIL_EVENT_TYPE4_DVA_IPZ][evt_max][div_count]++;
			}
		}
		/*DVA OBJ CNT*/
		if( _nf_action->rise_dva_obj_cnt& mask )
		{
			guint mask=0, evt_max=0;
			mask = _nf_action->rise_dva_event[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT];
			for(evt_max=0; evt_max < NF_ACTION_DVA_IDZ_NR; evt_max++ )
			{
				if(mask & (1 << evt_max))
				{
					#if defined(DEBUG_DVA_OBJ_CNT)
					printf("\033[0;35m %s RISE \033[0;39m\n", __FUNCTION__);
					#endif
					e_send_data->type4.rise[i][EMAIL_EVENT_TYPE4_DVA_OBJ_CNT][evt_max][div_count]++;
				}
			}
		}
		/** IMSI AI BOX **/
		if(_nf_action->rise_ai[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_ai_get_evt_mask(_nf_action->rise_ai[i]);

			for(evt_max=0; evt_max<NF_ACTION_AI_NR; evt_max++)
			{
				if(mask & (1 << evt_max))
				{
					e_send_data->type5.rise[i][EMAIL_EVENT_TYPE5_AI][evt_max][div_count]++;
					if(evt_max >= NF_ACTION_AI_DIR_POS && evt_max <= NF_ACTION_AI_INTRUSION)
					{
						if( NULL == strstr(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max], _nf_action->ai_meta_data[i].object_class) )
						{
							if( 0 != strlen(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max]) )
							{
								strcat(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max],", ");
							}
							strcat(e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max], _nf_action->ai_meta_data[i].object_class);
							#if defined(DEBUG_AI_DATA)
								printf("\033[0;37m %s DEBUG_AI_DATA %s\033[0;39m\n", __FUNCTION__, e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max]);
							#endif
						}
					}
					else if(evt_max == NF_ACTION_AI_GENERIC)
					{			
						_save_ai_generic_evt(e_send_data, i);
						#if defined(DEBUG_AI_DATA)
							printf("\033[0;37m %s DEBUG_AI_DATA %s\033[0;39m\n", __FUNCTION__, e_send_data->type5.text[i][EMAIL_EVENT_TYPE5_AI][evt_max]);
						#endif
					}
				}
			}
		}
		if( _nf_action->rise_pos & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_POS].rise[i][div_count]++;
	}

	/** Panic Record **/
	if( _nf_action->rise_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[0][div_count]++;

	// Disk Event
	for(i=NF_ACTION_HDD_EVENT_OVER; i<NF_ACTION_HDD_EVENT_NR; i++)
	{
		if(_nf_action->rise_hdd & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_HDD][i][div_count]++;
	}

	// System Event
	for(i=NF_ACTION_SYSTEM_EVENT_BOOTING; i<NF_ACTION_SYSTEM_EVENT_NR; i++)
	{
		if(_nf_action->rise_system & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_SYSTEM][i][div_count]++;
	}

	// Net Event
	for(i=NF_ACTION_NET_EVENT_TROUBLE; i<NF_ACTION_NET_EVENT_NR; i++)
	{
		if(_nf_action->rise_net & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_NET][i][div_count]++;
	}

	// Set Snapshot CH
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; i++)
	#else
	for(i=0; i<NUM_ALARM; i++)
	#endif
	{
		sdata = &_nf_action->sensor_data[i];

		if((_nf_action->rise_alarm >> i) & 0x1)
		{
			if(sdata->ftp_act)
				mask_lcamera_alarm |= sdata->mask_lcamera;
		}
	}

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		mdata = &_nf_action->motion_data[i];

		if((_nf_action->rise_motion >> i) & 0x1)
		{
			if(mdata->ftp_act)
				mask_motion |= (1 << i);
		}
	}

	for(i=0; i<NF_ACTION_REC_EVENT_NR; i++)
	{
		rdata = &_nf_action->rec_data[i];

		if((_nf_action->rise_rec_panic >> i) & 0x1)
		{
			if(rdata->ftp_act)
				mask_rec_panic |= (1 << i);
		}
	}

#if defined(ENABLE_EVENT_TAMPER)
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		tdata = &_nf_action->tamper_data[i];

		if((_nf_action->rise_tamper >> i) & 0x1)
		{
			if(tdata->ftp_act)
				mask_tamper |= (1 << i);
		}
	}
#endif

#if defined(SUPPORT_VCA_CAMERA)
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		vcadata = &_nf_action->vca_data[i];

		if(_nf_action->rise_vca[i])
		{
			if(vcadata->ftp_act)
				mask_vca |= (1 << i);
		}
	}
#endif
	/** DVA Event **/
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);
		
		dvadata = &_nf_action->dva_data[i];
		
		if( _nf_action->rise_dva_idz & mask || _nf_action->rise_dva_ipz & mask || _nf_action->rise_dva_obj_cnt & mask)
		{		
			if(dvadata->ftp_act)
				mask_dva |= (1 << i);
		}
	}
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		pdata = &_nf_action->pos_data[i];

		if((_nf_action->rise_pos >> i) & 0x1)
		{
			if(pdata->ftp_act)
				mask_pos |= (1 << i);
		}
	}
	/** IMSI AI BOX **/
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		aidata = &_nf_action->ai_data[i];

		if(_nf_action->rise_ai[i])
		{
			if(aidata->ftp_act)
				mask_ai |= (1 << i);
		}
	}
#if defined(ENABLE_EVENT_TAMPER)
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_tamper | mask_dva | mask_ai);
#elif defined(SUPPORT_VCA_CAMERA)
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_vca | mask_dva | mask_ai);
#elif defined(ENABLE_EVENT_TAMPER) && defined(SUPPORT_VCA_CAMERA)
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_tamper | mask_vca | mask_dva | mask_ai);
#else
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_pos | mask_dva| mask_ai);
#endif
	mask_snapshot_ch &= ~((gint)nf_notify_get_param0("vloss"));

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		if((mask_snapshot_ch >> i) & 0x1)
		{
			e_data->snapshot_ch = i;
			break;
		}
		else
			e_data->snapshot_ch = 0xff;
	}

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		if((mask_snapshot_ch >> i) & 0x1)
		{
			e_data->video_ch = i;
			break;
		}
		else
			e_data->video_ch = 0xff;
	}

}

static void
_ftp_action(void)
{
	GTimeVal        curr_timeval;

	FTP_DATA			*f_data = &_nf_action->ftp_data;
	EMAIL_STATE			*e_state = &_nf_action->ftp_state;
	EMAIL_EVENT_DATA	*e_send_data = _nf_action->ftp_send_data;
	gint			ret=0, i=0;
	gint 			div_count = e_state->div_count;

	gint ch_num=0, event_num=0, min_num=0, event_cate=0;
	guint event_sum=0;


	if(f_data->ftp_act == 0)
		return;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_ACTION] )
	{
		gint x=0, y=0, z=0;
		static glong check_time = 0;

		if( check_time == curr_timeval.tv_sec ){
			goto out;
		}else{
			check_time = curr_timeval.tv_sec;

			g_message("%s send_time[%ld] curr[%ld]",
					__FUNCTION__, e_state->send_time,  curr_timeval.tv_sec);

		}

		for(x=0;x<NUM_ACTIVE_CH;++x)
		{
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_ACTION] & (1<<x) )
			{
				for(y=0;y<e_state->div_frequency;++y)
				{
					g_print("ch[%2d] freq[%2d] %c rise", x, y,
							(y == e_state->div_count) ? '*' : ' ');
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].rise[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.rise[z][i][y] );
					}

					g_print("  active");
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].active[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.active[z][i][y] );
					}

					g_print("\n");
				}
			} // if
		} // for x
	}
out:

#endif

	_ftp_event_check(f_data, e_send_data, e_state);

#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	if( f_data->snapshot_onoff && (f_data->snapshot_ch != 0xff))
	{
		EMAIL_SNAPSHOT_DATA *ftp_snapshot = &_nf_action->ftp_snapshot;
		NF_JPEG_MAN_SNAPSHOT *snapshot = NULL;

		if( !ftp_snapshot->is_complete &&
				_ftp_snapshot_event_check( div_count ) )
		{
			gint retry_cnt=30;

			// Get Snapshot
			do{
				nf_jpeg_man_request_snapshot( f_data->snapshot_ch,NF_SECOND_SIZE, NF_JPEC_REQUEST_TIME );
				if( nf_jpeg_man_check_snapshot( f_data->snapshot_ch,NF_SECOND_SIZE, NF_JPEC_JPEG_CHECK_TIME ) )
				{
					if( !nf_jpeg_man_get_snapshot( f_data->snapshot_ch,NF_SECOND_SIZE, &snapshot ) )
						goto snap_get_failed;

					if( snapshot->data_size <= 0 )
					{
						g_warning("%s snapshot data size failed[%d]",
								__FUNCTION__, snapshot->data_size);

						goto snap_proc_failed;
					}

					ftp_snapshot->data = g_malloc(snapshot->data_size);
					if( ftp_snapshot->data == NULL )
					{
						g_warning("%s snapshot data malloc failed[%d]",
								__FUNCTION__, snapshot->data_size);
						goto snap_proc_failed;
					}

					memcpy( ftp_snapshot->data, snapshot->data, snapshot->data_size);

					ftp_snapshot->data_size = snapshot->data_size;
					ftp_snapshot->ch = f_data->snapshot_ch;
					ftp_snapshot->is_complete = 1;
					ftp_snapshot->capture_time = snapshot->ctime;

snap_proc_failed:
					nf_jpeg_man_free_snapshot(snapshot);

					break;
				}

				g_usleep(33000);
			}while( --retry_cnt >0  );

			if(retry_cnt == 0)
				g_warning("ftp.. get jpeg fail!!");

snap_get_failed:
			snapshot = NULL;
		}
	}
#endif

#if defined(NF_ACTOIN_ENABLE_VIDEO_CHECK)
	if( f_data->video_onoff && (f_data->video_ch != 0xff))
	{
		FTP_VIDEO_DATA *ftp_video = &_nf_action->ftp_video;

		if( !ftp_video->is_complete &&
				_ftp_video_event_check( div_count ) )
		{
			GTimeVal  		start_time;
			GTimeVal 		end_time;

			gettimeofday(&(ftp_video->end_time), NULL);

			ftp_video->start_time.tv_sec = ftp_video->end_time.tv_sec - 10;
			ftp_video->start_time.tv_usec = ftp_video->end_time.tv_usec;
			ftp_video->is_complete = 1;
			ftp_video->ch = f_data->video_ch;
		}
	}
#endif

	if(e_state->send_time <= curr_timeval.tv_sec)
	{
		#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_ACTION] )
				g_message("%s ftp_send", __FUNCTION__);
		#endif

		// ftp event type1
		for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			gint ch_max=0;

			if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					ch_max = _nf_action_num_alarm;
				#else
					ch_max=NUM_ALARM;
				#endif
			} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
				ch_max=NF_ACTION_REC_EVENT_NR;
			} else {
				ch_max=NUM_ACTIVE_CH;
			}

			for( ch_num=0; ch_num < ch_max; ++ch_num )
			{
				if( _ftp_is_act(ch_num, 0, 0, event_num, 0) )
				{
					event_sum=0;
					for( min_num=0; min_num <= e_state->div_frequency; ++min_num)
						event_sum += e_send_data->type1[event_num].rise[ch_num][min_num];
					e_send_data->type1[event_num].rise[ch_num][63] = (gushort)event_sum;
				}
			}
		}

		// ftp event type2
		for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE2_HDD)
				event_max=NF_ACTION_HDD_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
				event_max=NF_ACTION_SYSTEM_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_NET)
				event_max=NF_ACTION_NET_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
				event_max=1;

			for( event_cate=0; event_cate<event_max; event_cate++ )
			{
				if( _ftp_is_act(0, event_cate, 0, event_num, 1) )
				{
					event_sum = 0;
					for( min_num=0; min_num < e_state->div_frequency; ++min_num)
						event_sum += e_send_data->type2.rise[event_num][event_cate][min_num];

					e_send_data->type2.rise[event_num][event_cate][63] = (gushort)event_sum;
				}
			}
		}

		// email event type3
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE3_NR; event_num++ )
			{
				gint event_max=0;

				#if defined(SUPPORT_VCA_CAMERA)
					if(event_num == EMAIL_EVENT_TYPE3_VCA)
						event_max=NF_ACTION_VCA_NR;
				#endif

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _ftp_is_act(0, 0, ch_num, event_num, 2) )
					{
						event_sum = 0;
						for( min_num=0; min_num < e_state->div_frequency; ++min_num)
							event_sum += e_send_data->type3.rise[ch_num][event_num][event_cate][min_num];

						e_send_data->type3.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}
		/** DVA Event **/
		// ftp event type4
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE4_NR; event_num++ )
			{
				gint event_max=0;

				if(event_num == EMAIL_EVENT_TYPE4_DVA_IDZ)
					event_max=NF_ACTION_DVA_IDZ_NR;
				else if(event_num == EMAIL_EVENT_TYPE4_DVA_IPZ)
					event_max=NF_ACTION_DVA_IPZ_NR;
				else if(event_num == EMAIL_EVENT_TYPE4_DVA_OBJ_CNT)
					event_max=NF_ACTION_DVA_IDZ_NR;

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _ftp_is_act(0, 0, ch_num, event_num, 3) )
					{
						event_sum = 0;
						for( min_num=0; min_num < e_state->div_frequency; ++min_num)
							event_sum += e_send_data->type4.rise[ch_num][event_num][event_cate][min_num];

						e_send_data->type4.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}
	/** DVA Event **/
	// email event type5
	/** IMSI AI BOX **/
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE5_NR; event_num++ )
			{
				gint event_max=0;

				if(event_num == EMAIL_EVENT_TYPE5_AI)
					event_max=NF_ACTION_AI_NR;

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _ftp_is_act(0, 0, ch_num, event_num, 5) )
					{
						event_sum = 0;
						for( min_num=0; min_num < e_state->div_frequency; ++min_num)
							event_sum += e_send_data->type5.rise[ch_num][event_num][event_cate][min_num];

						e_send_data->type5.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}

		// ftp send
		ret = _ftp_send(curr_timeval.tv_sec/*e_state->send_time*/);
		if (ret == -1)
		{
			g_warning("%s _ftp_send() ret[%d]", __FUNCTION__, ret);
		}

		// time resetting and snapshot free
		if (ret == 1)
		{
			_ftp_reset(curr_timeval.tv_sec);
			_nf_action->pos_reset_flag |= NF_ACTION_POS_DATA_RESET_FTP;
		}
	}
	else
	{
		if(curr_timeval.tv_sec >= e_state->start_time)
		{
			e_state->div_count = (curr_timeval.tv_sec - e_state->start_time) / 60;

			// g_assert;
			if( e_state->div_count >= NF_ACTION_EMAIL_MAX_INTERVAL)
			{
			   g_warning("%s div_count overflow[%d]", __FUNCTION__, e_state->div_count);
			   e_state->div_count = NF_ACTION_EMAIL_MAX_INTERVAL - 1;
			}
		}
	}
}

/******************************************************************************************
  FTP END
 ******************************************************************************************/

#endif

#if defined(ENABLE_ACTION_MOBILE)

static void _nf_action_mobile_init(void)
{
#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_INIT] )
		g_message("%s called", __FUNCTION__ );
#endif
	_nf_action_mobile_reset(0);
}

	static void
_nf_action_mobile_reset(glong curr_time)
{
	MOBILE_DATA		*m_data = &_nf_action->mobile_data;
	MOBILE_STATE	*m_state = &_nf_action->mobile_state;

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_RESET] )
		g_message("%s called", __FUNCTION__ );
#endif

	memset(&_nf_action->mobile_state, 0, sizeof(_nf_action->mobile_state));
	memset(_nf_action->mobile_send_data, 0, sizeof(EMAIL_EVENT_DATA));

#if 0
#if defined(NF_ACTOIN_ENABLE_SNAPSHOT_CHECK)
	if( _nf_action->ftp_snapshot.data )
	{
		g_free(_nf_action->ftp_snapshot.data);
		_nf_action->ftp_snapshot.data = NULL;
	}
	memset(&_nf_action->ftp_snapshot, 0, sizeof(_nf_action->ftp_snapshot));
#endif
#endif

	if( curr_time == 0)
	{
		GTimeVal        curr_timeval;
		gettimeofday((struct timeval *)&curr_timeval, NULL);

		m_state->start_time = curr_timeval.tv_sec;
		curr_time = curr_timeval.tv_sec;
	}else{
		m_state->start_time = curr_time;
	}

	if ( m_data->frequency == 0 )
	{
		m_state->send_time = 5 + curr_time;
		m_state->div_frequency = 1;
	}
	else
	{
#if 1
		m_state->send_time = (glong)(m_data->frequency * 60) + curr_time;
		m_state->div_frequency = (m_state->send_time - m_state->start_time) / 60;
#else   // for test
		m_state->send_time = (glong)curr_time+30;
		m_state->div_frequency = 1;
#endif
	}

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_RESET] )
		g_message("%s start[%ld] freq[%d] send[%ld] div_freq[%ld]", __FUNCTION__,
				curr_time, m_data->frequency, m_state->send_time, m_state->div_frequency );
#endif

}

	static gint
_nf_action_mobile_send(time_t send_time)
{
	gint ch_num=0, ch_num_active=0, event_num=0, min_num=0, event_cate=0;
	time_t e_time = send_time;
	time_t e_time_for_weblink = send_time;

	struct tm st_buff;
	struct tm *st = &st_buff;
	gchar time_buff[256];
	gboolean is_send=FALSE;

	gchar buffer[NF_NET_SEND_MAX_CONTENTS]={0, }, user_buff[256]={0, };
	gchar *pos=buffer, *next_pos, *tmp_buff = NULL;
	gint user_count=0, send_usr_cnt=0, addr_cnt=0, cnt=0;
	gboolean sndflag = FALSE, is_dst=FALSE;
	gboolean booting = FALSE;
	gint lang_id = 0, num_alarm=0;
	gint ret=0, title_print=0;
	gint is_first=TRUE;

	EMAIL_EVENT_DATA    *e_send_data = _nf_action->mobile_send_data;

	MOBILE_DATA         *mobile_data = &_nf_action->mobile_data;

	NF_SMS_SEND_DATA cont;

	memset(&cont, 0, sizeof(cont));
	memset(buffer, 0, sizeof(buffer));

#if 1
	lang_id = _email_get_lang_id();

	if(lang_id >= NF_ACTION_LANG_ID_MAX)
	{
		g_warning("%s Email LangID is False", __FUNCTION__);
		return FALSE;
	}
#endif

	cont.receiver_cnt = (gint)nf_sysdb_get_uint("usr.UCNT");
	for ( user_count = 0; user_count < cont.receiver_cnt; ++user_count )
	{
		snprintf( user_buff, sizeof(user_buff)-1 , "usr.U%d.phone_notify", user_count );
		if ( !nf_sysdb_get_bool(user_buff) )
		{
			continue;
		}
		snprintf( user_buff, sizeof(user_buff)-1, "usr.U%d.phone", user_count );
		tmp_buff = nf_sysdb_get_str_nocopy(user_buff);
		if ( tmp_buff == NULL )
		{
			g_warning("%s user mobile error", __FUNCTION__);
			continue;
		}

#if 0
		if (nf_mail_send_check_email(tmp_buff) == 0 )
		{
			g_warning("%s nf_mail_send_check_email(tmp_buff) error", __FUNCTION__);
			continue;
		}
#else
		// To Do
		// Phone Number Check!!
#endif
		snprintf( cont.receiver[send_usr_cnt], sizeof(cont.receiver[user_count]), "%s", tmp_buff);
		++send_usr_cnt;     // User Count
	}

	is_dst = _nf_action->is_dst;
	if( is_dst == 0 && nf_datetime_is_dst( e_time ) )
		e_time -= 3600;

	{
		char date_str[128], time_str[128];

		guint datefmt = nf_sysdb_get_uint("sys.date.dateform");
		guint timefmt = nf_sysdb_get_uint("sys.date.timeform");

		localtime_r(&e_time, st);

		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(date_str, sizeof(date_str)-1, "%04d/%02d/%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d %s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		snprintf(time_buff, sizeof(time_buff)-1, "%s %s", date_str, time_str);

	}

	{
		char date_str[128], time_str[128];

		guint datefmt = nf_sysdb_get_uint("sys.date.dateform");
		guint timefmt = nf_sysdb_get_uint("sys.date.timeform");

		localtime_r(&e_time, st);

		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(date_str, sizeof(date_str)-1, "%04d/%02d/%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d %s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		snprintf(time_buff, sizeof(time_buff)-1, "%s %s", date_str, time_str);
	}

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s : %.24s\n",
			_ACTION_STR_TABLE_TYPE1_MOBILE[lang_id][LANG_ID_TIME], time_buff);

	tmp_buff = nf_sysdb_get_str_nocopy("sys.info.sysid");
	if (tmp_buff == NULL)
	{
		g_warning("%s sys.info.sysid error", __FUNCTION__);
		return -1;
	}

	pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s : %s\n\n",
			_ACTION_STR_TABLE_TYPE1_MOBILE[lang_id][LANG_ID_SYSTEM_ID], tmp_buff);


	//  Type3 Event
	for( event_num = 0; event_num < EMAIL_EVENT_TYPE3_NR; event_num++ )
	{
		gint event_max=0, event_id=0;
		gboolean is_title_print=FALSE;

		is_first=TRUE;
		is_send=FALSE;

		#if defined(SUPPORT_VCA_CAMERA)
			if(event_num == EMAIL_EVENT_TYPE3_VCA)
			{
				event_max=NF_ACTION_VCA_NR;
				event_id=LANG_ID_VCA;
			}
		#endif

		for( event_cate=0; event_cate<event_max; event_cate++)
		{
			is_title_print=FALSE;
			is_first=TRUE;
			is_send=FALSE;
			for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
			{
				if (e_send_data->type3.rise[ch_num][event_num][event_cate][63])
				{
					#if 0
						g_message("%s Line[%d] %d %d %d", __FUNCTION__, __LINE__, ch_num, event_num, event_cate);
					#endif
					if(!is_title_print)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s(%s) : ",
								_ACTION_STR_TABLE_TYPE1[lang_id][event_id],
								_ACTION_STR_TABLE_TYPE3[lang_id][event_num][event_cate]);
						is_title_print=TRUE;
					}

					if(is_first)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%d", ch_num+1);
						is_first=FALSE;
					}
					else
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), ", %d", ch_num+1);

					sndflag = TRUE;
					is_send = TRUE;
				}
			}

			if(is_send)
				pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  " ch\n|");
		}
	}

	//  Type2 Event
	for( event_num = EMAIL_EVENT_TYPE2_HDD; event_num < EMAIL_EVENT_TYPE2_NR; event_num++ )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++)
		{
			if (e_send_data->type2.rise[event_num][event_cate][63])
			{
				if(((event_num == EMAIL_EVENT_TYPE2_NET) && (event_cate == NF_ACTION_NET_EVENT_LOGON_FAIL)) ||
						((event_num == EMAIL_EVENT_TYPE2_SYSTEM) && (event_cate == NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL)))
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s\n|",
							_ACTION_STR_TABLE_TYPE2[lang_id][event_num][event_cate]);
				}
				else
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s\n|",
							_ACTION_STR_TABLE_TYPE2[lang_id][event_num][event_cate]);
				}
				sndflag = TRUE;
			}
		}
	}

	if (e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[NF_ACTION_REC_EVENT_PANIC][63])
	{
		pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer), "%s\n|",
				_ACTION_STR_TABLE_TYPE1_MOBILE[lang_id][LANG_ID_REC_PANIC]);
		sndflag = TRUE;
	}

	// Type1 Event
	for( event_num = EMAIL_EVENT_TYPE1_SENSOR; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
	{
		title_print=FALSE;

		// For IPCAM Sensor
#if defined(ENABLE_SENSOR_IPCAM)
		is_send=FALSE;
		if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
		{
			/**
			  When IPCAM+DVR, This is DVR String
			 **/
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			for (ch_num = 0; ch_num < _nf_action_num_nvr_alarm; ch_num++)
			#else
			for( ch_num=0; ch_num<NUM_ALARM_DVR; ch_num++ )
			#endif
			{
				gint index=0;

				index=NUM_ALARM_IPCAM+ch_num;

				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				if (index < _nf_action_num_alarm)
				#else
				if(index < NUM_ALARM)
				#endif
				{
					if (e_send_data->type1[event_num].rise[index][63])
					{
						if(title_print == FALSE)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s",
									_ACTION_STR_TABLE_TYPE1_MOBILE[lang_id][event_num]);
							title_print = TRUE;
						}

						if(is_first)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%d", ch_num+1);
							is_first=FALSE;
						}
						else
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  ", %d", ch_num+1);

						sndflag = TRUE;
						is_send=TRUE;
					}
				}

				if(is_send)
				{
					// for last check
					#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					if ((ch_num + 1) == _nf_action_num_nvr_alarm)
					#else
					if((ch_num + 1) == NUM_ALARM_DVR)
					#endif
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  " ch (NVR)\n|");
				}
			}
		}
#endif

		is_first=TRUE;
		title_print = FALSE;
		is_send=FALSE;
		for( ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++ )
		{
			if (e_send_data->type1[event_num].rise[ch_num][63])
			{
				if(title_print == FALSE)
				{
					pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%s",
							_ACTION_STR_TABLE_TYPE1_MOBILE[lang_id][event_num]);
					title_print = TRUE;
				}

				if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
				{
					/**
					  When IPCAM+DVR, This is IPCAM String
					  When Only DVR, this is DVR String
					 **/
					#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					if (ch_num < _nf_action_num_alarm)
					#else
					if(ch_num < NUM_ALARM)
					#endif
					{
						if(is_first)
						{
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%d", ch_num+1);
							is_first=FALSE;
						}
						else
							pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  ", %d", ch_num+1);
					}

					#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					if ((ch_num + 1) == _nf_action_num_alarm)
					#else
					if((ch_num + 1) == NUM_ALARM)
					#endif
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  " ch\n|");
				}
				else
				{
					if(is_first)
					{
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  "%d", ch_num+1);
						is_first=FALSE;
					}
					else
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  ", %d", ch_num+1);
				}

				sndflag = TRUE;
				is_send=TRUE;
			}

			if(is_send)
			{
				if((ch_num + 1) == NUM_ACTIVE_CH)
				{
					if((event_num == EMAIL_EVENT_TYPE1_VLOSS) || (event_num == EMAIL_EVENT_TYPE1_MOTION)
							|| (event_num == EMAIL_EVENT_TYPE1_SENSOR))
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  " ch\n|");
					else
						pos += snprintf(pos, sizeof(buffer)-((guint)pos-(guint)buffer),  " \n|");
				}
			}
		}
	}

#if 0       // for debug
	g_message("%s sndflag[%d]\nbuffer[%s]", __FUNCTION__, sndflag, buffer);
#endif

#define DEBUG_FORCE_SMS_SENDx

#ifdef DEBUG_FORCE_SMS_SEND
	sndflag = 1;
#endif

	if( sndflag == FALSE )
	{
#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_SEND] )
			g_message("%s sndflag [%d]", __FUNCTION__ , sndflag );
#endif
		return 0;
	}

#if 0
	cont.is_dvr_event = 1;
#endif

	next_pos = pos = buffer;
	while( pos = strstr(next_pos, "|") )
	{
		*pos = 0x00;
		strncpy( cont.text, next_pos, sizeof(cont.text)-1 );

		ret = nf_sms_push_event(NULL, &cont);
		g_message("%s mobile_send_req[%d] msg[%s]", __FUNCTION__, ret , cont.text);

		next_pos = ++pos;
		if( *next_pos == 0x00) break;
	}

	return 1;
}

	static int
_nf_action_mobile_is_act(int index_type1, int index_type2, int index_type3, int prop_id, int type)
{
	ALARM_SENSOR_DATA   *sdata  = NULL;
	MOTION_DATA         *mdata  = NULL;
	VLOSS_DATA          *vdata  = NULL;
	REC_DATA            *rpdata = NULL;
	TEXTIN_DATA         *tdata  = NULL;

	DISK_DATA           *ddata  = NULL;
	SYSTEM_DATA         *stdata = NULL;
	NET_DATA            *ndata  = NULL;
#if defined(ENABLE_EVENT_TAMPER)
	TAMPER_DATA         *tadata = NULL;
#endif
#if defined(SUPPORT_VCA_CAMERA)
	VCA_DATA *vcadata=NULL;
#endif

	if(type == 0)
	{
		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR ) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				g_return_val_if_fail( index_type1 < _nf_action_num_alarm, 0);
			#else
				g_return_val_if_fail( index_type1 < NUM_ALARM, 0);
			#endif
		} else {
			g_return_val_if_fail( index_type1 < NUM_ACTIVE_CH, 0);
		}

		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE1_NR, 0);

		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR )
		{
			sdata = &_nf_action->sensor_data[index_type1];
			return sdata->mobile_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_MOTION )
		{
			mdata = &_nf_action->motion_data[index_type1];
			return mdata->mobile_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_VLOSS )
		{
			vdata = &_nf_action->vloss_data[index_type1];
			return vdata->mobile_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_PANIC )
		{
			rpdata = &_nf_action->rec_data[index_type1];
			return rpdata->mobile_act ? 1: 0;
		}
#if 0
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_ALARM )
			return radata->mobile_act ? 1: 0;
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_MOTION )
			return rmdata->mobile_act ? 1: 0;
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_TEXT_IN )
		{
			tdata = &_nf_action->textin_data[index_type1];
			return tdata->mobile_act ? 1: 0;
		}
#if defined(ENABLE_EVENT_TAMPER)
		else if ( prop_id == EMAIL_EVENT_TYPE1_TAMPER )
		{
			tadata = &_nf_action->tamper_data[index_type1];
			return tadata->mobile_act ? 1: 0;
		}
#endif
	}
	else if(type == 1)
	{
		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE2_NR, 0);

		if ( prop_id == EMAIL_EVENT_TYPE2_HDD )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_HDD_EVENT_NR, 0);

			ddata = &_nf_action->disk_data[index_type2];

			return ddata->mobile_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_SYSTEM )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_SYSTEM_EVENT_NR, 0);

			stdata = &_nf_action->system_data[index_type2];

			return stdata->mobile_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_NET )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_NET_EVENT_NR, 0);

			ndata = &_nf_action->net_data[index_type2];
			return ndata->mobile_act ? 1: 0;
		}
	}
	else
	{
#if defined(SUPPORT_VCA_CAMERA)
		if ( prop_id == EMAIL_EVENT_TYPE3_VCA )
		{
			vcadata = &_nf_action->vca_data[index_type3];

			return vcadata->mobile_act ? 1: 0;
		}
#endif
	}

	return 0;
}

	static void
_nf_action_mobile_event_check(MOBILE_DATA *m_data, EMAIL_EVENT_DATA *e_send_data, MOBILE_STATE *m_state)
{
	gint    div_count = m_state->div_count;
	gint    i=0;
	guint   mask_snapshot_ch=0, mask_motion=0, mask_lcamera_alarm=0, mask_rec_panic=0;
	gint64 	mask=0;
	ALARM_SENSOR_DATA *sdata=NULL;
	MOTION_DATA *mdata=NULL;
	REC_DATA *rdata=NULL;
#if defined(ENABLE_EVENT_TAMPER)
	TAMPER_DATA *tdata=NULL;
	guint mask_tamper=0;
#endif
#if defined(SUPPORT_VCA_CAMERA)
	VCA_DATA *vcadata=NULL;
	guint mask_vca=0;
#endif


	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);

		if( _nf_action->curr_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].active[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->curr_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].active[i][div_count]++;
		if( _nf_action->curr_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].active[i][div_count]++;
#if 0
		if( _nf_action->curr_rec_alarm & mask )
			e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_ALARM][div_count]++;

		if( _nf_action->curr_rec_motion & mask )
			e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_MOTION][div_count]++;
#endif
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->curr_tamper & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].active[i][div_count]++;
#endif
#if defined(SUPPORT_VCA_CAMERA)
		if(_nf_action->curr_vca[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_vca_get_evt_mask(_nf_action->curr_vca[i]);

			for(evt_max=0; evt_max<16; evt_max++)
			{
				if(mask & (1 << evt_max))
					e_send_data->type3.active[i][EMAIL_EVENT_TYPE3_VCA][evt_max][div_count]++;
			}
		}
#endif
	}

	// 2010-08-02 ###1:55:05 choissi
	if( _nf_action->curr_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].active[0][div_count]++;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);
		if( _nf_action->rise_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->rise_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].rise[i][div_count]++;
		if( _nf_action->rise_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].rise[i][div_count]++;
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->rise_tamper & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].rise[i][div_count]++;
#endif
#if defined(SUPPORT_VCA_CAMERA)
		if(_nf_action->rise_vca[i])
		{
			guint mask=0, evt_max=0;

			mask=_nf_event_vca_get_evt_mask(_nf_action->rise_vca[i]);

			for(evt_max=0; evt_max<16; evt_max++)
			{
				if(mask & (1 << evt_max))
					e_send_data->type3.rise[i][EMAIL_EVENT_TYPE3_VCA][evt_max][div_count]++;
			}
		}
#endif
	}

	/** Panic Record **/
	if( _nf_action->rise_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[0][div_count]++;

	// Disk Event
	for(i=NF_ACTION_HDD_EVENT_OVER; i<NF_ACTION_HDD_EVENT_NR; i++)
	{
		if(_nf_action->rise_hdd & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_HDD][i][div_count]++;
	}

	// System Event
	for(i=NF_ACTION_SYSTEM_EVENT_BOOTING; i<NF_ACTION_SYSTEM_EVENT_NR; i++)
	{
		if(_nf_action->rise_system & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_SYSTEM][i][div_count]++;
	}

	// Net Event
	for(i=NF_ACTION_NET_EVENT_TROUBLE; i<NF_ACTION_NET_EVENT_NR; i++)
	{
		if(_nf_action->rise_net & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_NET][i][div_count]++;
	}

	// Set Snapshot CH
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; i++)
	#else
	for(i=0; i<NUM_ALARM; i++)
	#endif
	{
		sdata = &_nf_action->sensor_data[i];

		if((_nf_action->rise_alarm >> i) & 0x1)
		{
			if(sdata->mobile_act)
				mask_lcamera_alarm |= sdata->mask_lcamera;
		}
	}

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		mdata = &_nf_action->motion_data[i];

		if((_nf_action->rise_motion >> i) & 0x1)
		{
			if(mdata->mobile_act)
				mask_motion |= (1 << i);
		}
	}

	for(i=0; i<NF_ACTION_REC_EVENT_NR; i++)
	{
		rdata = &_nf_action->rec_data[i];

		if((_nf_action->rise_rec_panic >> i) & 0x1)
		{
			if(rdata->mobile_act)
				mask_rec_panic |= (1 << i);
		}
	}
#if defined(ENABLE_EVENT_TAMPER)
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_tamper);
#elif defined(SUPPORT_VCA_CAMERA)
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_vca);
#elif defined(ENABLE_EVENT_TAMPER) && defined(SUPPORT_VCA_CAMERA)
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic | mask_tamper | mask_vca);
#else
	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic);
#endif
	mask_snapshot_ch &= ~((gint)nf_notify_get_param0("vloss"));

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		if((mask_snapshot_ch >> i) & 0x1)
		{
			m_data->snapshot_ch = i;
			break;
		}
		else
			m_data->snapshot_ch = 0xff;
	}
}

	static void
_nf_action_mobile_action(void)
{
	GTimeVal        curr_timeval;

	MOBILE_DATA            *m_data = &_nf_action->mobile_data;
	MOBILE_STATE         *m_state = &_nf_action->mobile_state;
	EMAIL_EVENT_DATA    *e_send_data = _nf_action->mobile_send_data;
	gint            ret=0, i=0;
	gint            div_count = m_state->div_count;

	if(m_data->mobile_act == 0)
		return;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_ACTION] )
	{
		gint x=0, y=0, z=0;
		static glong check_time = 0;

		if( check_time == curr_timeval.tv_sec ){
			goto out;
		}else{
			check_time = curr_timeval.tv_sec;

			g_message("%s send_time[%ld] curr[%ld]",
					__FUNCTION__, e_state->send_time,  curr_timeval.tv_sec);

		}

		for(x=0;x<NUM_ACTIVE_CH;++x)
		{
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_ACTION] & (1<<x) )
			{
				for(y=0;y<e_state->div_frequency;++y)
				{
					g_print("ch[%2d] freq[%2d] %c rise", x, y,
							(y == e_state->div_count) ? '*' : ' ');
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].rise[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.rise[z][i][y] );
					}

					g_print("  active");
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].active[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.active[z][i][y] );
					}

					g_print("\n");
				}
			} // if
		} // for x
	}
out:

#endif

	_nf_action_mobile_event_check(m_data, e_send_data, m_state);

	if(m_state->send_time <= curr_timeval.tv_sec)
	{
		int ch_num=0, event_num=0, min_num=0, event_cate=0;
		guint event_sum=0;

#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_FTP_ACTION] )
			g_message("%s mobile_send", __FUNCTION__);
#endif

		// mobile event type1
		for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			gint ch_max=0;

			if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					ch_max = _nf_action_num_alarm;
				#else
					ch_max=NUM_ALARM;
				#endif
			} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
				ch_max=NF_ACTION_REC_EVENT_NR;
			} else {
				ch_max=NUM_ACTIVE_CH;
			}

			for( ch_num=0; ch_num < ch_max; ++ch_num )
			{
				if( _nf_action_mobile_is_act(ch_num, 0, 0, event_num, 0) )
				{
					event_sum=0;
					for( min_num=0; min_num <= m_state->div_frequency; ++min_num)
						event_sum += e_send_data->type1[event_num].rise[ch_num][min_num];
					e_send_data->type1[event_num].rise[ch_num][63] = (gushort)event_sum;
				}
			}
		}

		// mobile event type2
		for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE2_HDD)
				event_max=NF_ACTION_HDD_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
				event_max=NF_ACTION_SYSTEM_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_NET)
				event_max=NF_ACTION_NET_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
				event_max=1;

			for( event_cate=0; event_cate<event_max; event_cate++ )
			{
				if( _nf_action_mobile_is_act(0, event_cate, 0, event_num, 1) )
				{
					event_sum = 0;
					for( min_num=0; min_num < m_state->div_frequency; ++min_num)
						event_sum += e_send_data->type2.rise[event_num][event_cate][min_num];

					e_send_data->type2.rise[event_num][event_cate][63] = (gushort)event_sum;
				}
			}
		}

		// mobile event type3
		for(ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++)
		{
			for( event_num=0; event_num < EMAIL_EVENT_TYPE3_NR; event_num++ )
			{
				gint event_max=0;

#if defined(SUPPORT_VCA_CAMERA)
				if(event_num == EMAIL_EVENT_TYPE3_VCA)
					event_max=NF_ACTION_VCA_NR;
#endif

				for( event_cate=0; event_cate<event_max; event_cate++)
				{
					if( _nf_action_mobile_is_act(0, 0, ch_num, event_num, 2) )
					{
						event_sum = 0;
						for( min_num=0; min_num < m_state->div_frequency; ++min_num)
							event_sum += e_send_data->type3.rise[ch_num][event_num][event_cate][min_num];

						e_send_data->type3.rise[ch_num][event_num][event_cate][63] = (gushort)event_sum;
					}
				}
			}
		}

		// mobile send
		ret = _nf_action_mobile_send(curr_timeval.tv_sec/*m_state->send_time*/);
		if (ret == -1)
		{
			g_warning("%s _mobile_send() ret[%d]", __FUNCTION__, ret);
		}

		// time resetting and snapshot free
		_nf_action_mobile_reset(curr_timeval.tv_sec);
	}
	else
	{
		if(curr_timeval.tv_sec >= m_state->start_time)
		{
			m_state->div_count = (curr_timeval.tv_sec - m_state->start_time) / 60;

			// g_assert;
			if( m_state->div_count >= NF_ACTION_EMAIL_MAX_INTERVAL)
			{
				g_warning("%s div_count overflow[%d]", __FUNCTION__, m_state->div_count);
				m_state->div_count = NF_ACTION_EMAIL_MAX_INTERVAL - 1;
			}
		}
	}
}

/*******************************************************************************************
  DVA ETC
********************************************************************************************/
#if 1
static int _set_dva_group_name(char *name)
{
	int event_idx = 0;
	gboolean is_detected = FALSE;
	#if defined(DEBUG_DVA_OBJ_CNT)
	printf("\033[0;35m %s START \033[0;39m\n", __FUNCTION__);
	#endif
	for(event_idx = 0; event_idx < NF_ACTION_DVA_IDZ_NR; event_idx++)
	{
		#if defined(DEBUG_DVA_OBJ_CNT)
		printf("\033[0;35m %s event_idx[%d] \033[0;39m\n", __FUNCTION__, event_idx);
		#endif
		if(strcmp(name,_ACTION_STR_TABLE_TYPE4[0][EMAIL_EVENT_TYPE4_DVA_IDZ][event_idx]) == 0)
		{
			is_detected = TRUE;
			#if defined(DEBUG_DVA_OBJ_CNT)
			printf("\033[0;35m %s is_detected \033[0;39m\n", __FUNCTION__);
			#endif
			break;
		}
	}
	if(is_detected)
	{
		if((event_idx >= NF_ACTION_DVA_IDZ_BICYCLE) && (event_idx <= NF_ACTION_DVA_IDZ_CAR))
			strcpy(name,_ACTION_STR_TABLE2_TYPE4[0][NF_ACTION_DVA_DESC_VEHICLE]);
		else if((event_idx >= NF_ACTION_DVA_IDZ_BIRD) && (event_idx <= NF_ACTION_DVA_IDZ_HORSE))
			strcpy(name,_ACTION_STR_TABLE2_TYPE4[0][NF_ACTION_DVA_DESC_ANIMAL]);
		else if( event_idx == NF_ACTION_DVA_IDZ_HUMAN )
		{ ; }
	}
}
static int _set_dva_name(int event_num ,int event_cate, char* name, int name_size, int* is_first)
{
	//gint lang_id = _email_get_lang_id();
	gint lang_id = 0;
	gint is_first_obj_cnt = 0;
	if(is_first != NULL)
		is_first_obj_cnt = *is_first;
	#if defined(DEBUG_DVA_OBJ_CNT)
	printf("\033[0;34m %s event_num[%d] event_cate[%d]\033[0;39m\n", __FUNCTION__,event_num,event_cate);
	#endif
	if((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE))
	{		
		strncpy(name ,_ACTION_STR_TABLE2_TYPE4[lang_id][NF_ACTION_DVA_DESC_ANIMAL],name_size);
		#if defined(DEBUG_DVA_OBJ_CNT)
		printf("\033[0;34m %s [%s]\033[0;39m\n", __FUNCTION__,name);
		#endif
	}
	else if(((event_num == EMAIL_EVENT_TYPE4_DVA_IDZ) && (event_cate == NF_ACTION_DVA_IDZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IDZ_BICYCLE))
			|| ((event_num == EMAIL_EVENT_TYPE4_DVA_IPZ) && (event_cate == NF_ACTION_DVA_IPZ_MOTORBIKE || event_cate == NF_ACTION_DVA_IPZ_BICYCLE)))
	{
		strncpy(name ,_ACTION_STR_TABLE3_TYPE4[lang_id],name_size);
		#if defined(DEBUG_DVA_OBJ_CNT)
		printf("\033[0;34m %s MOTOR [%s]\033[0;39m\n", __FUNCTION__,name);
		#endif
	}
	else
	{
		strncpy(name ,_ACTION_STR_TABLE_TYPE4[lang_id][event_num][event_cate],name_size);
		#if defined(DEBUG_DVA_OBJ_CNT)
		printf("\033[0;34m %s ELSE [%s]\033[0;39m\n", __FUNCTION__,name);
		#endif
	}
	if(is_first != NULL)
	{
		if(event_cate >= NF_ACTION_DVA_IDZ_BIRD && event_cate <= NF_ACTION_DVA_IDZ_HORSE)
			is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_ANIMAL));
		else if(event_cate >= NF_ACTION_DVA_IDZ_BICYCLE && event_cate <= NF_ACTION_DVA_IDZ_CAR)
			is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_VEHICLE));
		else if(event_cate == NF_ACTION_DVA_IDZ_HUMAN)
			is_first_obj_cnt = is_first_obj_cnt & (~(0x01 << NF_ACTION_DVA_DESC_HUMAN));
		(*is_first) = is_first_obj_cnt;
	}
}

#else
#endif

/******************************************************************************************
  MOBILE END
 ******************************************************************************************/
#endif


#if defined(ENABLE_ACTION_PUSH)

static void _nf_action_push_init(void)
{
#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_PUSH_INIT] )
		g_message("%s called", __FUNCTION__ );
#endif

	_nf_action_push_reset(0);
}

	static void
_nf_action_push_reset(glong curr_time)
{
	PUSH_DATA		*m_data = &_nf_action->push_data;
	PUSH_STATE	*m_state = &_nf_action->push_state;

#ifdef DEBUG_ACTION_LOG
  if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_PUSH_RESET] )
		g_message("%s called", __FUNCTION__ );
#endif

	memset(&_nf_action->push_state, 0, sizeof(_nf_action->push_state));
	memset(_nf_action->push_send_data, 0, sizeof(EMAIL_EVENT_DATA));

	if( curr_time == 0)
	{
		GTimeVal        curr_timeval;
		gettimeofday((struct timeval *)&curr_timeval, NULL);

		m_state->start_time = curr_timeval.tv_sec;
		curr_time = curr_timeval.tv_sec;
		m_state->send_time = 0;
		return;
	}else{
		m_state->start_time = curr_time;
	}

	if ( m_data->frequency == 0 )
	{
		m_state->send_time = 5 + curr_time;
		m_state->div_frequency = 1;
	}
	else
	{
#if 1
		m_state->send_time = (glong)(m_data->frequency * 60) + curr_time;
		m_state->div_frequency = (m_state->send_time - m_state->start_time) / 60;
#else   // for test
		m_state->send_time = (glong)curr_time+30;
		m_state->div_frequency = 1;
#endif
	}

#ifdef DEBUG_ACTION_LOG
  if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_PUSH_RESET] )
		g_message("%s start[%ld] freq[%d] send[%ld] div_freq[%ld]", __FUNCTION__,
				curr_time, m_data->frequency, m_state->send_time, m_state->div_frequency );
#endif
}

static gint _push_make_content(
    const char *str,
    char *mac_buff,
    gint param1, gint param2,
    char *buff, guint buff_size)
{
	char *result;

	time_t     tm_time;
	struct tm *st_time;
	char       time_str[256];
  GValue ret_value = {0,};
  gchar      ddns_addr[1024] = {0,};
  guint      webport, rtspport;

	time( &tm_time);
	st_time = localtime( &tm_time);
	strftime( time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", st_time);

  memset(ddns_addr, 0, sizeof(ddns_addr));

  // *TODO* This Function is nfgui functions. need to be modified (chcha)
  // DDNS information
  if(DAL_is_ddns_on() == TRUE)
  {
    scm_get_dvr_addr_str(ddns_addr, 1024);
	}

  if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
  {
    webport = g_value_get_uint(&ret_value);
    g_value_unset(&ret_value);
}

  if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
{
    rtspport = g_value_get_uint(&ret_value);
    g_value_unset(&ret_value);
  }
  // DDNS end

	json_t *json = json_object();
	json_error_t json_error;

	json_object_set_new(json, "macAddress", json_string(mac_buff));

	json_t *alert_json = json_object();

	json_object_set_new(alert_json, "loc-key", json_string(str));

	json_t *args = json_array();

  if( param1 >= 0 ) {
    json_array_append_new(args, json_integer(param1));

    if( param2 >= 0 ) {
      json_array_append_new(args, json_integer(param2));
    }
  }

	json_object_set(alert_json, "loc-args", args);

	json_object_set_new(json, "alert", alert_json);

	json_t *message_json = json_object();
	json_object_set_new(message_json, "mac", json_string(mac_buff));
	json_object_set_new(message_json, "type", json_string(str));

	json_object_set_new(message_json, "params", args);

  json_object_set_new(message_json, "url", json_string(ddns_addr));
  json_object_set_new(message_json, "httpport", json_integer(webport));
  json_object_set_new(message_json, "rtspport", json_integer(rtspport));
	json_object_set_new(message_json, "unixtimestamp", json_integer(tm_time));
	json_object_set_new(message_json, "datestr", json_string(time_str));

	json_object_set_new(json, "message", message_json);

#ifdef DEBUG_ACTION_LOG
  result = json_dumps(json, JSON_INDENT(2) |  JSON_ENCODE_ANY);
  g_print("--------------------------------\n");
  g_print("PUSH MESSAGE : \n %s \n", result);
  g_print("--------------------------------\n");
  free(result);
#endif

	result = json_dumps(json, 0);

	if( strlen(result) > buff_size ) {
		free(result);
		json_decref(json);
		return -1;
	}

	strcpy(buff, result);
	free(result);
	json_decref(json);

	return 0;
}
//#define DEBUG_ACTION_LOG
static gint _push_make_content2(
    const char *str,
    char *mac_buff,
    gint param1, gint param2, gchar *param2s,
    char *buff, guint buff_size,
    guint64 timestamp, guint64 timestampl, json_t *metadata_json)
{
	char *result;

	time_t     tm_time;
	struct tm *st_time;
	char       time_str[256];
	GValue ret_value = {0,};
	gchar      ddns_addr[1024] = {0,};
	guint      webport, rtspport;

	tm_time = (time_t)timestamp;
	if (tm_time == 0)
		time( &tm_time);
		
	st_time = localtime( &tm_time);
	strftime( time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", st_time);

	memset(ddns_addr, 0, sizeof(ddns_addr));

	// *TODO* This Function is nfgui functions. need to be modified (chcha)
	// DDNS information
	if(DAL_is_ddns_on() == TRUE)
	{
		scm_get_dvr_addr_str(ddns_addr, 1024);
	}

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		webport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		rtspport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	// DDNS end

	json_t *json = json_object();
	json_error_t json_error;

	json_object_set_new(json, "macAddress", json_string(mac_buff));

	json_t *alert_json = json_object();

	json_object_set_new(alert_json, "loc-key", json_string(str));

	json_t *args = json_array();

	if( param1 >= 0 ) {

		if((strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_SENSOR]) == 0)
		|| (strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_MOTION]) == 0)
		|| (strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_VLOSS]) == 0))
		{
			char c;
			sprintf(&c,"%d",(param1+1));
			printf("\033[0;34m %s >>>>> %c <<<<< \033[0;39m\n", __FUNCTION__,c);
			json_array_append_new(args, json_string(&c));
		}
		else
			json_array_append_new(args, json_integer(param1));

		if( param2s ) {
			json_array_append_new(args, json_string(param2s));
		}
		else if( param2 >= 0 ) {
			json_array_append_new(args, json_integer(param2));
		}
	}

	json_object_set(alert_json, "loc-args", args);

	json_object_set_new(json, "alert", alert_json);

	json_t *message_json = json_object();
	json_object_set_new(message_json, "mac", json_string(mac_buff));
	json_object_set_new(message_json, "type", json_string(str));

	json_object_set_new(message_json, "params", args);

	json_object_set_new(message_json, "url", json_string(ddns_addr));
	json_object_set_new(message_json, "httpport", json_integer(webport));
	json_object_set_new(message_json, "rtspport", json_integer(rtspport));
	json_object_set_new(message_json, "unixtimestamp", json_integer(timestamp));
	json_object_set_new(message_json, "unixtimestampl", json_integer(timestampl));
	json_object_set_new(message_json, "datestr", json_string(time_str));
	
	if (metadata_json)
		json_object_set_new(message_json, "metadata", metadata_json);

	json_object_set_new(json, "message", message_json);

#ifdef DEBUG_ACTION_LOG
	result = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY);
	g_print("--------------------------------\n");
	g_print("PUSH MESSAGE : \n %s \n", result);
	g_print("--------------------------------\n");
	free(result);
#endif

	result = json_dumps(json, 0);

	if( strlen(result) > buff_size ) {
		free(result);
		json_decref(json);
		return -1;
	}

	strcpy(buff, result);
	free(result);
	json_decref(json);

	return 0;
}
static gint _push_make_content2_sysid(
    const char *str,
    char *mac_buff,
    gint param1, gint param2, gchar *param2s,
    char *buff, guint buff_size,
    guint64 timestamp, guint64 timestampl, json_t *metadata_json)
{
	char *result;

	time_t     tm_time;
	struct tm *st_time;
	char       time_str[256];
	GValue ret_value = {0,};
	gchar      ddns_addr[1024] = {0,};
	guint      webport, rtspport;
	gchar *pSysId = NULL;

	tm_time = (time_t)timestamp;
	if (tm_time == 0)
		time( &tm_time);
		
	st_time = localtime( &tm_time);
	strftime( time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", st_time);

	memset(ddns_addr, 0, sizeof(ddns_addr));

	// *TODO* This Function is nfgui functions. need to be modified (chcha)
	// DDNS information
	if(DAL_is_ddns_on() == TRUE)
	{
		scm_get_dvr_addr_str(ddns_addr, 1024);
	}

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		webport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		rtspport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	// DDNS end

	json_t *json = json_object();
	json_error_t json_error;

	json_object_set_new(json, "macAddress", json_string(mac_buff));

	json_t *alert_json = json_object();

	json_object_set_new(alert_json, "loc-key", json_string(str));

	json_t *args = json_array();
	
	if( param1 >= 0 ) {
			
		if((strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_SENSOR]) == 0)
		|| (strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_MOTION]) == 0)
		|| (strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_VLOSS]) == 0))
		{
			char c;
			sprintf(&c,"%d",(param1+1));
			json_array_append_new(args, json_string(&c));
		}
		else
			json_array_append_new(args, json_integer(param1));

		pSysId = nf_sysdb_get_str_nocopy("sys.info.sysid");
		if (pSysId == NULL)
			json_array_append_new(args, json_string("NONE"));
		else
			json_array_append_new(args, json_string(pSysId));

		if( param2s ) {
			json_array_append_new(args, json_string(param2s));
		}
		else if( param2 >= 0 ) {
			json_array_append_new(args, json_integer(param2));
		}
	}
	else if(strcmp(str,_ACTION_STR_TABLE_TYPE2_PUSH[EMAIL_EVENT_TYPE2_NET][NF_ACTION_NET_EVENT_TROUBLE_AI_BOX]) == 0)
	{
		pSysId = nf_sysdb_get_str_nocopy("sys.info.sysid");
		if (pSysId == NULL)
			json_array_append_new(args, json_string("NONE"));
		else
			json_array_append_new(args, json_string(pSysId));

		if( param2s ) {
			json_array_append_new(args, json_string(param2s));
		}
		else if( param2 >= 0 ) {
			json_array_append_new(args, json_integer(param2));
		}
	}

	json_object_set(alert_json, "loc-args", args);

	json_object_set_new(json, "alert", alert_json);

	json_t *message_json = json_object();
	json_object_set_new(message_json, "mac", json_string(mac_buff));
	json_object_set_new(message_json, "type", json_string(str));

	json_object_set_new(message_json, "params", args);

	json_object_set_new(message_json, "url", json_string(ddns_addr));
	json_object_set_new(message_json, "httpport", json_integer(webport));
	json_object_set_new(message_json, "rtspport", json_integer(rtspport));
	json_object_set_new(message_json, "unixtimestamp", json_integer(timestamp));
	json_object_set_new(message_json, "unixtimestampl", json_integer(timestampl));
	json_object_set_new(message_json, "datestr", json_string(time_str));
	
	if (metadata_json)
		json_object_set_new(message_json, "metadata", metadata_json);

	json_object_set_new(json, "message", message_json);

#ifdef DEBUG_ACTION_LOG
	result = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY);
	g_print("--------------------------------\n");
	g_print("PUSH MESSAGE : \n %s \n", result);
	g_print("--------------------------------\n");
	free(result);
#endif

	result = json_dumps(json, 0);

	if( strlen(result) > buff_size ) {
		free(result);
		json_decref(json);
		return -1;
	}

	strcpy(buff, result);
	free(result);
	json_decref(json);

	return 0;
}

#define PUSH_MSG_LIMIT_SIZE 512
/**
 *	prarm1 : cam channel, param2 : reseved
 *	param2s : catption
 *	msg1s : title, msg2s : description
**/
static gint _push_make_content_generic_evt(
    const char *str,
    char *mac_buff,
    gint param1, gint param2, gchar *param2s ,gchar *msg1s, gchar *msg2s,
    char *buff, guint buff_size,
    guint64 timestamp, guint64 timestampl, json_t *metadata_json)
{
	char *result;
	time_t     tm_time;
	struct tm *st_time;
	char       time_str[256];
	GValue ret_value = {0,};
	gchar      ddns_addr[1024] = {0,};
	guint      webport, rtspport;
	gchar *pSysId = NULL;
	
	tm_time = (time_t)timestamp;
	if (tm_time == 0)
		time( &tm_time);
		
	st_time = localtime( &tm_time);
	strftime( time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", st_time);

	memset(ddns_addr, 0, sizeof(ddns_addr));

	// *TODO* This Function is nfgui functions. need to be modified (chcha)
	// DDNS information
	if(DAL_is_ddns_on() == TRUE)
	{
		scm_get_dvr_addr_str(ddns_addr, 1024);
	}

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		webport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		rtspport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	// DDNS end

	json_t *json = json_object();
	json_error_t json_error;

	json_object_set_new(json, "macAddress", json_string(mac_buff));

	json_t *alert_json = json_object();

	json_object_set_new(alert_json, "loc-key", json_string(str));

	json_t *args = json_array();
	
	if( param1 >= 0 ) {
		
		json_array_append_new(args, json_integer(param1));

		pSysId = nf_sysdb_get_str_nocopy("sys.info.sysid");
		if (pSysId == NULL)
			json_array_append_new(args, json_string("NONE"));
		else
			json_array_append_new(args, json_string(pSysId));

		if( param2s ) {
			json_array_append_new(args, json_string(param2s));
		}
		else if( param2 >= 0 ) {
			json_array_append_new(args, json_integer(param2));
		}
	}

	json_object_set(alert_json, "loc-args", args);

	json_object_set_new(json, "alert", alert_json);

	json_t *message_json = json_object();
	json_object_set_new(message_json, "mac", json_string(mac_buff));
	json_object_set_new(message_json, "type", json_string(str));

	json_object_set_new(message_json, "params", args);
	json_object_set_new(message_json, "title", json_string(msg1s));
	json_object_set_new(message_json, "description", json_string(msg2s));
	
	json_object_set_new(message_json, "url", json_string(ddns_addr));
	json_object_set_new(message_json, "httpport", json_integer(webport));
	json_object_set_new(message_json, "rtspport", json_integer(rtspport));
	json_object_set_new(message_json, "unixtimestamp", json_integer(timestamp));
	json_object_set_new(message_json, "unixtimestampl", json_integer(timestampl));
	json_object_set_new(message_json, "datestr", json_string(time_str));
	
	if (metadata_json)
		json_object_set_new(message_json, "metadata", metadata_json);

	/*
	** If the message size exceeds 512 bytes, no push is sent
	** The code below is necessary to not fail to send the push message.
	*/
	{
		char arrKey[64] = {0, };
		char *msg_result;
		int ret = 0;
		int idx  = 0;
		for(idx = 0; idx < 3; idx++)
		{
			msg_result = json_dumps(message_json, 0);
			printf("\033[0;36m %s MSG SIZE %d\033[0;39m\n", __FUNCTION__, strlen(msg_result));
			if( strlen(msg_result) > PUSH_MSG_LIMIT_SIZE )
			{
				if( idx == 0 )
					strcpy(arrKey, "description");
				else if( idx == 1 )
					strcpy(arrKey, "title");
				else if( idx == 2 )
				{
					printf("%s Failed to Resize msg\n", __FUNCTION__);
					ret = -1;
					goto FAIL_TO_RESIZE_MSG;
				}
				
				if(json_object_del(message_json, arrKey))
				{
					printf("%s Failed to delete %s key\n", __FUNCTION__, arrKey);
					ret = -1;
					goto FAIL_TO_RESIZE_MSG;
				}
				else
					printf("%s Delete %s key\n", __FUNCTION__, arrKey);
					
				json_object_set_new(message_json, arrKey, json_string("NONE"));
			}
			
			FAIL_TO_RESIZE_MSG:
			{
				free(msg_result);
				if( ret == -1 )
					return ret;
			}
		}
	}
	
	json_object_set_new(json, "message", message_json);

#ifdef DEBUG_ACTION_LOG
	result = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY);
	g_print("--------------------------------\n");
	g_print("PUSH MESSAGE : \n %s \n", result);
	g_print("--------------------------------\n");
	free(result);
#endif

	result = json_dumps(json, 0);
	if( strlen(result) > buff_size ) {
		free(result);
		json_decref(json);
		return -1;
	}
	
	strcpy(buff, result);
	free(result);
	json_decref(json);

	return 0;
}


static gint _push_make_content3(
    const char *str,
    char *mac_buff,
    gint param1, gint param2, gchar *param2s, gchar *param3s,
    char *buff, guint buff_size,
    guint64 timestamp, guint64 timestampl, json_t *metadata_json)
{
	char *result;

	time_t     tm_time;
	struct tm *st_time;
	char       time_str[256];
	GValue ret_value = {0,};
	gchar      ddns_addr[1024] = {0,};
	guint      webport, rtspport;

	tm_time = (time_t)timestamp;
	if (tm_time == 0)
		time( &tm_time);
		
	st_time = localtime( &tm_time);
	strftime( time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", st_time);

	memset(ddns_addr, 0, sizeof(ddns_addr));

	// *TODO* This Function is nfgui functions. need to be modified (chcha)
	// DDNS information
	if(DAL_is_ddns_on() == TRUE)
	{
		scm_get_dvr_addr_str(ddns_addr, 1024);
	}

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		webport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		rtspport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	// DDNS end

	json_t *json = json_object();
	json_error_t json_error;

	json_object_set_new(json, "macAddress", json_string(mac_buff));

	json_t *alert_json = json_object();
	json_object_set_new(alert_json, "loc-key", json_string(str));

	json_t *args = json_array();

	if( param1 >= 0 ) {

		if((strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_SENSOR]) == 0)
		|| (strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_MOTION]) == 0)
		|| (strcmp(str,_ACTION_STR_TABLE_TYPE1_PUSH[EMAIL_EVENT_TYPE1_VLOSS]) == 0))
		{
			char c;
			sprintf(&c,"%d",(param1+1));
			printf("\033[0;34m %s >>>>> %c <<<<< \033[0;39m\n", __FUNCTION__,c);
			json_array_append_new(args, json_string(&c));
		}
		else
			json_array_append_new(args, json_integer(param1));

		if( param2s ) {
			json_array_append_new(args, json_string(param2s));
		}
		else if( param2 >= 0 ) {
			json_array_append_new(args, json_integer(param2));
		}

		if(param3s)
		{
			json_array_append_new(args, json_string(param3s));
		}
	}

	json_object_set(alert_json, "loc-args", args);

	json_object_set_new(json, "alert", alert_json);

	json_t *message_json = json_object();
	json_object_set_new(message_json, "mac", json_string(mac_buff));
	json_object_set_new(message_json, "type", json_string(str));

	json_object_set_new(message_json, "params", args);

	json_object_set_new(message_json, "url", json_string(ddns_addr));
	json_object_set_new(message_json, "httpport", json_integer(webport));
	json_object_set_new(message_json, "rtspport", json_integer(rtspport));
	json_object_set_new(message_json, "unixtimestamp", json_integer(timestamp));
	json_object_set_new(message_json, "unixtimestampl", json_integer(timestampl));
	json_object_set_new(message_json, "datestr", json_string(time_str));
	
	if (metadata_json)
		json_object_set_new(message_json, "metadata", metadata_json);

	json_object_set_new(json, "message", message_json);

#ifdef DEBUG_ACTION_LOG
	result = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY);
	g_print("--------------------------------\n");
	g_print("PUSH MESSAGE : \n %s \n", result);
	g_print("--------------------------------\n");
	free(result);
#endif

	result = json_dumps(json, 0);

	if( strlen(result) > buff_size ) {
		free(result);
		json_decref(json);
		return -1;
	}

	strcpy(buff, result);
	free(result);
	json_decref(json);

	return 0;
}

#if 0
static gint _nf_action_push_send(time_t send_time)
{
	gint ch_num=0, ch_num_active=0, event_num=0, min_num=0, event_cate=0;
	time_t e_time = send_time;
	time_t e_time_for_weblink = send_time;

	struct tm st_buff;
	struct tm *st = &st_buff;
	gchar time_buff[256];
	gboolean is_send=FALSE;

	gchar buffer[NF_NET_SEND_MAX_CONTENTS]={0, }, user_buff[256]={0, };
	gchar *pos=buffer, *next_pos, *tmp_buff = NULL;
	gint user_count=0, send_usr_cnt=0, addr_cnt=0, cnt=0;
	gboolean sndflag = FALSE, is_dst=FALSE;
	gboolean booting = FALSE;
	gint lang_id = 0, num_alarm=0;
	gint ret=0, title_print=0;
	gint is_first=TRUE;
	gchar *mac_buff = NULL;

	char temp[128];
	gchar ddns_buff[2048];
  gint  http_port;
  gint  rtsp_port;

	EMAIL_EVENT_DATA    *e_send_data = _nf_action->push_send_data;
	PUSH_DATA         *push_data = &_nf_action->push_data;

	//memset(&cont, 0, sizeof(cont));
	memset(buffer, 0, sizeof(buffer));

	is_dst = _nf_action->is_dst;
	if( is_dst == 0 && nf_datetime_is_dst( e_time ) )
		e_time -= 3600;

	{
		char date_str[128], time_str[128];

		guint datefmt = nf_sysdb_get_uint("sys.date.dateform");
		guint timefmt = nf_sysdb_get_uint("sys.date.timeform");

		localtime_r(&e_time, st);

		if (datefmt == NF_DATE_YYYYMMDD)
			snprintf(date_str, sizeof(date_str)-1, "%04d/%02d/%02d", st->tm_year+1900, st->tm_mon + 1, st->tm_mday);
		else if (datefmt == NF_DATE_MMDDYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mon + 1, st->tm_mday, st->tm_year+1900);
		else if (datefmt == NF_DATE_DDMMYYYY)
			snprintf(date_str, sizeof(date_str)-1, "%02d/%02d/%04d", st->tm_mday, st->tm_mon + 1, st->tm_year+1900);

		if (timefmt == NF_TIME_24H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
		else if (timefmt == NF_TIME_12H)
			snprintf(time_str, sizeof(time_str)-1, "%02d:%02d:%02d %s",
					((st->tm_hour != 0) && (st->tm_hour != 12) ? (st->tm_hour % 12) : 12),
					st->tm_min, st->tm_sec, ((st->tm_hour >= 12) ? "PM" : "AM"));

		snprintf(time_buff, sizeof(time_buff)-1, "%s %s", date_str, time_str);

	}

	tmp_buff = nf_sysdb_get_str_nocopy("sys.info.sysid");
	if (tmp_buff == NULL)
	{
		g_warning("%s sys.info.sysid error", __FUNCTION__);
		return -1;
	}

	mac_buff = nf_sysdb_get_str_nocopy("sys.info.mac");
	if (mac_buff == NULL)
	{
		g_warning("%s sys.info.mac error", __FUNCTION__);
		return -1;
	}

	snprintf(ddns_buff, sizeof(ddns_buff), "%s.%s", nf_sysdb_get_str_nocopy("sys.info.mac"), nf_sysdb_get_str_nocopy("net.proto.ddnssvr"));

	//  Type2 Event
	for( event_num = EMAIL_EVENT_TYPE2_HDD; event_num < EMAIL_EVENT_TYPE2_NR; event_num++ )
	{
		gint event_max=0;

		if(event_num == EMAIL_EVENT_TYPE2_HDD)
			event_max=NF_ACTION_HDD_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
			event_max=NF_ACTION_SYSTEM_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_NET)
			event_max=NF_ACTION_NET_EVENT_NR;
		else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
			event_max=1;

		for( event_cate=0; event_cate<event_max; event_cate++)
		{
			if (e_send_data->type2.rise[event_num][event_cate][63])
			{
				NF_UTIL_PUSH_CONTENT cont;
				memset(&cont, 0x0, sizeof(NF_UTIL_PUSH_CONTENT));

			_push_make_content(_ACTION_STR_TABLE_TYPE2_PUSH[event_num][event_cate], mac_buff , -1 , -1 , cont.contents , sizeof(cont.contents));
				g_message("%s PUSH MESSAGE : TYPE2 evt_num: %d", __FUNCTION__, event_num);
				nf_util_push_request(&cont, NULL);
			}
		}
	}

	if (e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[NF_ACTION_REC_EVENT_PANIC][63])
	{
		NF_UTIL_PUSH_CONTENT cont;
		memset(&cont, 0x0, sizeof(NF_UTIL_PUSH_CONTENT));

		_push_make_content(_ACTION_STR_TABLE_TYPE1_PUSH[LANG_PUSH_ID_REC_PANIC], mac_buff, -1 , -1 , cont.contents , sizeof(cont.contents));
		g_message("%s PUSH MESSAGE : TYPE1 PANIC", __FUNCTION__);
		nf_util_push_request(&cont, NULL);
	}

	// Type1 Event
  for( event_num = EMAIL_EVENT_TYPE1_SENSOR; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
	{
		title_print=FALSE;

		if( event_num == EMAIL_EVENT_TYPE1_REC_PANIC )
			continue;

		// For IPCAM Sensor
#if defined(ENABLE_SENSOR_IPCAM)
		if(event_num == EMAIL_EVENT_TYPE1_SENSOR)
		{
			/**
			  When IPCAM+DVR, This is DVR String
			 **/
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			for (ch_num = 0; ch_num < _nf_action_num_nvr_alarm; ch_num++)
			#else
			for( ch_num=0; ch_num<NUM_ALARM_DVR; ch_num++ )
			#endif
			{
				gint index=0;

				index=NUM_ALARM_IPCAM+ch_num;

				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				if (index < _nf_action_num_alarm)
				#else
				if(index < NUM_ALARM)
				#endif
				{
					if (e_send_data->type1[event_num].rise[index][63])
					{
						NF_UTIL_PUSH_CONTENT cont;
						memset(&cont, 0x0, sizeof(NF_UTIL_PUSH_CONTENT));

				if( _push_make_content(_ACTION_STR_TABLE_TYPE1_PUSH[0], mac_buff, ch_num, 0, cont.contents, sizeof(cont.contents)) == 0 ) {
							g_message("%s PUSH MESSAGE : TYPE1 SENSOR : CH%d", __FUNCTION__, ch_num);
							nf_util_push_request(&cont, NULL);
				}
				else{
							g_message("%s PUSH MESSAGE FAIL %d", __FUNCTION__, __LINE__);
						}

						sndflag = TRUE;
					}
				}
			}
		}
#endif

		is_first=TRUE;
		title_print = FALSE;
		is_send=FALSE;
		for( ch_num=0; ch_num<NUM_ACTIVE_CH; ch_num++ )
		{
			if (e_send_data->type1[event_num].rise[ch_num][63])
			{
					NF_UTIL_PUSH_CONTENT cont;
					memset(&cont, 0x0, sizeof(NF_UTIL_PUSH_CONTENT));

			if( _push_make_content(_ACTION_STR_TABLE_TYPE1_PUSH[event_num], mac_buff, ch_num, 0, cont.contents, sizeof(cont.contents)) == 0 ) {
				g_message("%s PUSH MESSAGE : TYPE1 %d : CH%d", __FUNCTION__,  event_num, ch_num);
						nf_util_push_request(&cont, NULL);
					}
			else{
						g_message("%s PUSH MESSAGE FAIL %d", __FUNCTION__, __LINE__);
					}

				// push send
			}
		}
	}

#ifdef DEBUG_ACTION_LOG
	g_message("%s sndflag[%d]\nbuffer[%s]", __FUNCTION__, sndflag, buffer);
#endif

	return 1;
}
#else
static gint _nf_action_push_send(RECENT_EVENT_DATA *recent_event, time_t send_time)
{
	gint event_num=0, min_num=0, event_cate=0;
	gint param1=0;
	gchar param2s[128] = {0};
	gchar param3s[128] = {0};
	
	gchar *mac_buff = NULL;
	gchar *loc_key = NULL;
	
	json_t *metadata_json = NULL;
	json_t *bbox = NULL;

	EMAIL_EVENT_DATA  *e_send_data = _nf_action->push_send_data;
	PUSH_DATA         *push_data = &_nf_action->push_data;
	PUSH_STATE        *m_state = &_nf_action->push_state;

	POS_TEXT_DATA		*p_text_data = _nf_action->push_pos_text_data;
	
	mac_buff = nf_sysdb_get_str_nocopy("sys.info.mac");
	if (mac_buff == NULL)
	{
		g_warning("%s sys.info.mac error", __FUNCTION__);
		return -1;
	}
	
	NF_UTIL_PUSH_CONTENT cont;
	memset(&cont, 0x0, sizeof(NF_UTIL_PUSH_CONTENT));
	if(recent_event->event_type == 5)
	{
		int ailog_idx = 0;
		loc_key = _ACTION_STR_TABLE_TYPE5_PUSH[recent_event->event_num];
		param1 = (recent_event->ch);
		#if defined(DEBUG_AI)
		printf("\033[0;36m %s [DEBUG_AI] recent_event->event_type == 5 \033[0;39m\n", __FUNCTION__);
		#endif
		if (recent_event->event_num == EMAIL_EVENT_TYPE5_AI)
		{
			#if defined(DEBUG_AI)
			printf("\033[0;36m %s [DEBUG_AI] recent_event->event_num == EMAIL_EVENT_TYPE5_AI \033[0;39m\n", __FUNCTION__);
			#endif
			param1 += 1;
			if(recent_event->event_num_sub == NF_ACTION_AI_GENERIC)
			{
				int nIdx = 0;
				for(nIdx = 0; nIdx < EMAIL_EVENT_TYPE5_GENERIC_EVT_INDAX_MAX; nIdx++)
				{
					if(strlen(e_send_data->type5.st_generic_evt[recent_event->ch][nIdx].caption) == 0)
						return;
					else
					{
						ai_generic_event_t tmp_data;
						memcpy(&tmp_data, &e_send_data->type5.st_generic_evt[recent_event->ch][nIdx], sizeof(ai_generic_event_t));
						if(_push_make_content_generic_evt( "log3107", mac_buff , param1, 0, \
								tmp_data.caption, tmp_data.title, tmp_data.description , \
								cont.contents , sizeof(cont.contents), \
								tmp_data.timestamp, tmp_data.timestamp*5, NULL ) == 0)
						{
							nf_util_push_request(&cont, NULL);
						}
					}
				}
			}	
			else if(recent_event->event_num_sub == NF_ACTION_AI_FR || recent_event->event_num_sub == NF_ACTION_AI_LPR)
			{
				gchar *tmp_buff = NULL;


			if(recent_event->event_num_sub <= NF_ACTION_AI_SPEED && recent_event->event_num_sub >= NF_ACTION_AI_TAMPER)
				ailog_idx = 0;
			else
				ailog_idx = recent_event->event_num_sub + 1;

				
				tmp_buff = nf_sysdb_get_str_nocopy("sys.info.sysid");
				
				memset(param2s, 0x00, sizeof(param2s));
				if (tmp_buff == NULL)
					strcpy(param2s,"NONE");
				else
					strcpy(param2s,tmp_buff);

				if(recent_event->event_num_sub == NF_ACTION_AI_FR)
				{
					int i = 0;
					loc_key = "log3105";

					printf("\033[0;36m %s IMSI %s!!\033[0;39m\n", __FUNCTION__,loc_key);
					for(i = 0; i < _nf_action->ai_fr_data[recent_event->ch].info_cnt; i++)
					{
						memset(param3s, 0x00, sizeof(param3s));
						if(strlen(_nf_action->ai_fr_data[recent_event->ch].info[i].name) == 0)
							strcpy(param3s,"UNKNOWN NAME");
						else
							strcpy(param3s,_nf_action->ai_fr_data[recent_event->ch].info[i].name);
						if(_push_make_content3(loc_key, mac_buff , param1, 0, param2s, param3s, cont.contents , sizeof(cont.contents), 
								recent_event->timestamp, 0, NULL) == 0)
						{
							nf_util_push_request(&cont, NULL);
						}
					}
				}
				else if( recent_event->event_num_sub == NF_ACTION_AI_LPR )
				{
					loc_key = "log3106";

					memset(param3s, 0x00, sizeof(param3s));
					strcpy(param3s,_nf_action->ai_lpr_data[recent_event->ch].lp_text);
					if(_push_make_content3(loc_key, mac_buff , param1, 0, param2s, param3s, cont.contents , sizeof(cont.contents), 
							recent_event->timestamp, 0, NULL) == 0)
					{
						nf_util_push_request(&cont, NULL);
					}
				}
				
			}
			else
			{
			        if(recent_event->event_num_sub <= NF_ACTION_AI_SPEED && recent_event->event_num_sub >= NF_ACTION_AI_TAMPER)
				     ailog_idx = 0;
			        else
				     ailog_idx = recent_event->event_num_sub + 1;
				memset(param2s, 0x00, sizeof(param2s));
				strcpy(param2s,_ailog_str[ailog_idx]);
				#if defined(DEBUG_VCA_PUSH)
				printf("\033[0;36m %s param2s[%s] \033[0;39m\n", __FUNCTION__,param2s);
				#endif
				if(_push_make_content2(loc_key, mac_buff , param1, 0, param2s, cont.contents , sizeof(cont.contents), 
								recent_event->timestamp, 0, NULL) == 0)
				{
					nf_util_push_request(&cont, NULL);
				}
			}
		}
	}
	else if(recent_event->event_type == 3)
	{
		int vcalog_idx = 0;
		loc_key = _ACTION_STR_TABLE_TYPE3_PUSH[recent_event->event_num];
		param1 = (recent_event->ch);
		#if defined(DEBUG_VCA_PUSH)
		printf("\033[0;36m %s recent_event->event_type == 3 \033[0;39m\n", __FUNCTION__);
		#endif
		if (recent_event->event_num == EMAIL_EVENT_TYPE3_VCA)
		{
			#if defined(DEBUG_VCA_PUSH)
			printf("\033[0;36m %s recent_event->event_num == EMAIL_EVENT_TYPE3_VCA \033[0;39m\n", __FUNCTION__);
			#endif
			param1 += 1;
			if(recent_event->event_num_sub > NF_ACTION_VCA_TAMPER)
				vcalog_idx = 0;
			else
				vcalog_idx = recent_event->event_num_sub + 1;
			memset(param2s, 0x00, sizeof(param2s));
			strcpy(param2s,_vcalog_str[vcalog_idx]);
			#if defined(DEBUG_VCA_PUSH)
			printf("\033[0;36m %s param2s[%s] \033[0;39m\n", __FUNCTION__,param2s);
			#endif
			if(_push_make_content2(loc_key, mac_buff , param1, 0, param2s, cont.contents , sizeof(cont.contents), 
							recent_event->timestamp, 0, NULL) == 0)
			{
				nf_util_push_request(&cont, NULL);
			}
		}
	}
	else if (recent_event->event_type == 2)
	{	
		loc_key = _ACTION_STR_TABLE_TYPE2_PUSH[recent_event->event_num][recent_event->event_num_sub];

		if((recent_event->event_num == EMAIL_EVENT_TYPE2_NET) && (recent_event->event_num_sub == NF_ACTION_NET_EVENT_TROUBLE_AI_BOX))
		{
			int n_ch = 0;
			char tmp_str[128] = {0,};
			int first_flag = 0;
			#if 0
			_make_send_string(param2s, 128, 0);
			#endif
			_push_make_content2_sysid(loc_key, mac_buff, -1, -1, NULL, cont.contents, sizeof(cont.contents), 
							recent_event->timestamp, NULL, metadata_json);
		}
		else
		{
		_push_make_content2(loc_key, mac_buff , -1 , -1 , NULL, cont.contents , sizeof(cont.contents), 
							recent_event->timestamp, 0, NULL);
		}
		
		g_message("%s PUSH MESSAGE : TYPE2 evt_num: %d", __FUNCTION__, recent_event->event_num);
	}
	else if (recent_event->event_type == 1)
	{
		loc_key = _ACTION_STR_TABLE_TYPE1_PUSH[recent_event->event_num];
		param1 = (recent_event->ch);
		#if defined(DEBUG_DVA_OBJ_CNT)
		printf("\033[0;35m %s type 1 [%d]\033[0;39m\n", __FUNCTION__,recent_event->event_num);
		#endif		
		if (recent_event->event_num == EMAIL_EVENT_TYPE1_DVA_IDZ)
		{
			DVA_MSG_IDZ *idz = &_nf_action->meta_dva_idz.intrusion_detection;

			param1 += 1;
			dvatext_translate_to_uxitem(idz->name, param2s, sizeof(param2s));
			
			bbox = json_array();
			json_array_append_new(bbox, json_real(idz->bbox.coords[0]));
			json_array_append_new(bbox, json_real(idz->bbox.coords[1]));
			json_array_append_new(bbox, json_real(idz->bbox.coords[2]));
			json_array_append_new(bbox, json_real(idz->bbox.coords[3]));

			metadata_json = json_object();
			json_object_set_new(metadata_json, "class", json_string(param2s));
			json_object_set_new(metadata_json, "confidence", json_real(idz->confidence));
			json_object_set_new(metadata_json, "bbox", bbox);
		}
		else if (recent_event->event_num == EMAIL_EVENT_TYPE1_DVA_IPZ)
		{
			DVA_MSG_IPZ *ipz = &_nf_action->meta_dva_ipz.illegal_parking;

			param1 += 1;
			dvatext_translate_to_uxitem(ipz->name, param2s, sizeof(param2s));
			
			bbox = json_array();
			json_array_append_new(bbox, json_real(ipz->bbox.coords[0]));
			json_array_append_new(bbox, json_real(ipz->bbox.coords[1]));
			json_array_append_new(bbox, json_real(ipz->bbox.coords[2]));
			json_array_append_new(bbox, json_real(ipz->bbox.coords[3]));

			metadata_json = json_object();
			json_object_set_new(metadata_json, "class", json_string(param2s));
			json_object_set_new(metadata_json, "confidence", json_real(ipz->confidence));
			json_object_set_new(metadata_json, "bbox", bbox);
		}
		else if (recent_event->event_num == EMAIL_EVENT_TYPE1_DVA_OBJ_CNT)
		{
			DVA_MSG_COUNTER *obj_cnt = &_nf_action->meta_dva_obj_cnt.counter;
			gchar obj_name[64] = {0,};
			
			param1 += 1;
			
			#if defined(DEBUG_DVA_OBJ_CNT)
			printf("\033[0;35m %s RISE \033[0;39m\n", __FUNCTION__);
			#endif
			
			#if 0
			strcpy(obj_name,obj_cnt->name);
			_set_dva_group_name(obj_name);
			#endif
			
			strcpy(param2s,obj_cnt->name);
			#if defined(DEBUG_DVA_OBJ_CNT)
			printf("\033[0;35m %s SEND NAME [%s] \033[0;39m\n", __FUNCTION__,param2s);
			#endif
			_set_dva_group_name(param2s);
			
			#if defined(DEBUG_DVA_OBJ_CNT)
			printf("\033[0;35m %s SEND NAME [%s] \033[0;39m\n", __FUNCTION__,param2s);
			#endif
			

			bbox = json_array();
			json_array_append_new(bbox, json_real(obj_cnt->bbox.coords[0]));
			json_array_append_new(bbox, json_real(obj_cnt->bbox.coords[1]));
			json_array_append_new(bbox, json_real(obj_cnt->bbox.coords[2]));
			json_array_append_new(bbox, json_real(obj_cnt->bbox.coords[3]));

			metadata_json = json_object();
			json_object_set_new(metadata_json, "class", json_string(param2s));
			json_object_set_new(metadata_json, "confidence", json_real(obj_cnt->confidence));
			json_object_set_new(metadata_json, "bbox", bbox);
			
		}
		else if(recent_event->event_num == EMAIL_EVENT_TYPE1_SENSOR)
		{
			int is_ipacam = 1,ch_num;
			int ch_param = -1;
			guint64 mask=0;
			#if 0
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			for (ch_num = 0; ch_num < _nf_action_num_alarm; ++ch_num)
			#else
			for(ch_num=0; ch_num<NUM_ALARM; ++ch_num)
			#endif
			{
				//index = NUM_ALARM_IPCAM + ch_num;
				mask = (0x1ULL<<ch_num);
				if( _nf_action->rise_alarm & mask ) {
					printf("\033[0;31m %s ALARM DETECTED\033[0;39m\n", __FUNCTION__);
					if(NUM_ALARM_IPCAM <= ch_num)
					{
						printf("\033[0;31m %s ALARM NVR\033[0;39m\n", __FUNCTION__);
						memset(param2s, 0x00, sizeof(param2s));
						strcpy(param2s,"NVR");
						ch_param = ch_num - NUM_ALARM_IPCAM;
					}
					else
				{
						printf("\033[0;31m %s ALARM IP CAM\033[0;39m\n", __FUNCTION__);
						memset(param2s, 0x00, sizeof(param2s));
						strcpy(param2s,"CAM");
						ch_param = ch_num;
					}
					
					if(_push_make_content2(loc_key, mac_buff, ch_param, 0, param2s, cont.contents, sizeof(cont.contents), 
							recent_event->timestamp, recent_event->timestampl*5, metadata_json) == 0)
						{
						printf("\033[0;31m %s ALARM SEND\033[0;39m\n", __FUNCTION__);
							nf_util_push_request(&cont, NULL);
						}
					}
				
			}
			#else
			if(NUM_ALARM_IPCAM <= recent_event->ch)
			{
				printf("\033[0;31m %s ALARM NVR\033[0;39m\n", __FUNCTION__);
				memset(param2s, 0x00, sizeof(param2s));
				strcpy(param2s,"NVR");
				ch_param = (recent_event->ch) - NUM_ALARM_IPCAM;
			}
			else
			{
				printf("\033[0;31m %s ALARM IP CAM\033[0;39m\n", __FUNCTION__);
				memset(param2s, 0x00, sizeof(param2s));
				strcpy(param2s,"CAM");
				ch_param = (recent_event->ch);
			}
			if(_push_make_content2(loc_key, mac_buff, ch_param, 0, param2s, cont.contents, sizeof(cont.contents),
				recent_event->timestamp, recent_event->timestampl*5, metadata_json) == 0)
			{
				nf_util_push_request(&cont, NULL);
			}
			#endif
		}
                else if(recent_event->event_num == EMAIL_EVENT_TYPE1_POS)
		{
			gushort pos_data_max_cnt = p_text_data[param1].data_position;
			gushort pos_data_cnt = 0;
			guint64 timestamp;
			printf("\033[0;31m %s POS SEND %d ch %d\033[0;39m\n", __FUNCTION__, pos_data_max_cnt, param1);
			for(pos_data_cnt=0; pos_data_cnt<pos_data_max_cnt; pos_data_cnt++)
			{
				g_static_mutex_lock (&_nf_pos_data_push_mutex);
				strncpy(param2s, p_text_data[param1].data[pos_data_cnt], 128);
				timestamp =  p_text_data[param1].timestamp[pos_data_cnt];
				g_static_mutex_unlock (&_nf_pos_data_push_mutex);
				
				if(_push_make_content2_sysid(loc_key, mac_buff, param1+1, 0, param2s, cont.contents, sizeof(cont.contents), 
							timestamp, NULL, metadata_json) == 0)
				{				
					nf_util_push_request(&cont, NULL);
				}
				
			}
			
		}
		if(recent_event->event_num != EMAIL_EVENT_TYPE1_SENSOR && recent_event->event_num != EMAIL_EVENT_TYPE1_POS)
		{
		_push_make_content2(loc_key, mac_buff, param1, 0, param2s, cont.contents, sizeof(cont.contents), 
							recent_event->timestamp, recent_event->timestampl*5, metadata_json);
		}
		
		g_message("%s PUSH MESSAGE : TYPE1 %d : CH%d", __FUNCTION__,  recent_event->event_num, recent_event->ch);
	}
	
	if((recent_event->event_num != EMAIL_EVENT_TYPE1_SENSOR && recent_event->event_num != EMAIL_EVENT_TYPE3_VCA \
	&& recent_event->event_num != EMAIL_EVENT_TYPE1_POS) || (recent_event->event_num == EMAIL_EVENT_TYPE2_HDD && recent_event->event_type == 2))
	{
	nf_util_push_request(&cont, NULL);
	}
	return 1;
}
#endif

	static int
_nf_action_push_is_act(int index_type1, int index_type2, int prop_id, int type)
{
	ALARM_SENSOR_DATA   *sdata  = NULL;
	MOTION_DATA         *mdata  = NULL;
	VLOSS_DATA          *vdata  = NULL;
	REC_DATA            *rpdata = NULL;
	TEXTIN_DATA         *tdata  = NULL;
	POS_DATA			*posdata = NULL;
	DISK_DATA           *ddata  = NULL;
	SYSTEM_DATA         *stdata = NULL;
	NET_DATA            *ndata  = NULL;
#if defined(ENABLE_EVENT_TAMPER)
	TAMPER_DATA         *tadata = NULL;
#endif
#if defined(SUPPORT_VCA_CAMERA)
	VCA_DATA *vcadata=NULL;
#endif

	DVA_DATA			*dvadata = NULL;
	AI_DATA				*aidata = NULL;

	if(type == 0)
	{
		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR ) {
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				g_return_val_if_fail( index_type1 < _nf_action_num_alarm, 0);
			#else
				g_return_val_if_fail( index_type1 < NUM_ALARM, 0);
			#endif
		} else {
			g_return_val_if_fail( index_type1 < NUM_ACTIVE_CH, 0);
		}

		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE1_NR, 0);

		if( prop_id == EMAIL_EVENT_TYPE1_SENSOR )
		{
			sdata = &_nf_action->sensor_data[index_type1];
			return sdata->push_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_MOTION )
		{
			mdata = &_nf_action->motion_data[index_type1];
			return mdata->push_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_VLOSS )
		{
			vdata = &_nf_action->vloss_data[index_type1];
			return vdata->push_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_PANIC )
		{
			rpdata = &_nf_action->rec_data[index_type1];
			return rpdata->push_act ? 1: 0;
		}
#if 0
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_ALARM )
			return radata->push_act ? 1: 0;
		else if ( prop_id == EMAIL_EVENT_TYPE1_REC_MOTION )
			return rmdata->push_act ? 1: 0;
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_TEXT_IN )
		{
			tdata = &_nf_action->textin_data[index_type1];
			return tdata->push_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE1_POS )
		{
			posdata = &_nf_action->pos_data[index_type1];
			return posdata->push_act ? 1: 0;
		}
#if defined(ENABLE_EVENT_TAMPER)
		else if ( prop_id == EMAIL_EVENT_TYPE1_TAMPER )
		{
			tadata = &_nf_action->tamper_data[index_type1];
			return tadata->push_act ? 1: 0;
		}
#endif
		else if ( prop_id == EMAIL_EVENT_TYPE1_DVA_IDZ || prop_id == EMAIL_EVENT_TYPE1_DVA_IPZ || prop_id == EMAIL_EVENT_TYPE1_DVA_OBJ_CNT)
		{
			#if defined(DEBUG_DVA_OBJ_CNT)
			printf("\033[0;35m %s [%d] \033[0;39m\n", __FUNCTION__,prop_id);
			#endif
			dvadata = &_nf_action->dva_data[index_type1];
			return dvadata->push_act ? 1: 0;
		}
	}
	else if(type == 1)
	{
		g_return_val_if_fail( prop_id < EMAIL_EVENT_TYPE2_NR, 0);

		if ( prop_id == EMAIL_EVENT_TYPE2_HDD )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_HDD_EVENT_NR, 0);

			ddata = &_nf_action->disk_data[index_type2];

			return ddata->push_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_SYSTEM )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_SYSTEM_EVENT_NR, 0);

			stdata = &_nf_action->system_data[index_type2];

			return stdata->push_act ? 1: 0;
		}
		else if ( prop_id == EMAIL_EVENT_TYPE2_NET )
		{
			g_return_val_if_fail( index_type2 < NF_ACTION_NET_EVENT_NR, 0);

			ndata = &_nf_action->net_data[index_type2];
			return ndata->push_act ? 1: 0;
		}
	}
	else if(type == 2)
	{
		if ( prop_id == EMAIL_EVENT_TYPE3_VCA )
		{
			vcadata = &_nf_action->vca_data[index_type1];
			#if defined(DEBUG_VCA_PUSH)
			printf("\033[0;36m %s EMAIL_EVENT_TYPE3_VCA[%d] \033[0;39m\n", __FUNCTION__,vcadata->push_act);
			#endif
			return vcadata->push_act ? 1: 0;
		}
	}
	else if (type == 5)
	{
		if ( prop_id == EMAIL_EVENT_TYPE5_AI ) /** IMSI AI BOX **/
		{
			aidata = &_nf_action->ai_data[index_type1];
			#if defined(DEBUG_AI)
			printf("\033[0;36m %s [DEBUG_AI] EMAIL_EVENT_TYPE5_AI[%d] \033[0;39m\n", __FUNCTION__,aidata->push_act);
			#endif
			return aidata->push_act ? 1: 0;
		}
	}
	return 0;
}

	static void
_nf_action_push_event_check(PUSH_DATA *m_data, EMAIL_EVENT_DATA *e_send_data, PUSH_STATE *m_state)
{
	gint    div_count = m_state->div_count;
	gint    i=0;
	guint   mask_snapshot_ch=0, mask_motion=0, mask_lcamera_alarm=0, mask_rec_panic=0;
	gint64 	mask=0;
	ALARM_SENSOR_DATA *sdata=NULL;
	MOTION_DATA *mdata=NULL;
	REC_DATA *rdata=NULL;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);

		if( _nf_action->curr_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].active[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->curr_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].active[i][div_count]++;
		if( _nf_action->curr_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].active[i][div_count]++;
#if 0
		if( _nf_action->curr_rec_alarm & mask )
			e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_ALARM][div_count]++;

		if( _nf_action->curr_rec_motion & mask )
			e_send_data->type1[i].active[EMAIL_EVENT_TYPE1_REC_MOTION][div_count]++;
#endif
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->curr_tamper & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].active[i][div_count]++;
#endif
	}

	// 2010-08-02 ###1:55:05 choissi
	if( _nf_action->curr_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].active[0][div_count]++;

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);
		if( _nf_action->rise_alarm & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_SENSOR].rise[i][div_count]++;
	}

	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->rise_motion & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_MOTION].rise[i][div_count]++;
		if( _nf_action->rise_vloss & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_VLOSS].rise[i][div_count]++;
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->rise_tamper & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_TAMPER].rise[i][div_count]++;
#endif
	}

	// DVA Event
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);

		if( _nf_action->rise_dva_idz & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_DVA_IDZ].rise[i][div_count]++;
		if( _nf_action->rise_dva_ipz & mask )
			e_send_data->type1[EMAIL_EVENT_TYPE1_DVA_IPZ].rise[i][div_count]++;
	}

	/** Panic Record **/
	if( _nf_action->rise_rec_panic )
		e_send_data->type1[EMAIL_EVENT_TYPE1_REC_PANIC].rise[0][div_count]++;

	// Disk Event
	for(i=NF_ACTION_HDD_EVENT_OVER; i<NF_ACTION_HDD_EVENT_NR; i++)
	{
		if(_nf_action->rise_hdd & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_HDD][i][div_count]++;
	}

	// System Event
	for(i=NF_ACTION_SYSTEM_EVENT_BOOTING; i<NF_ACTION_SYSTEM_EVENT_NR; i++)
	{
		if(_nf_action->rise_system & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_SYSTEM][i][div_count]++;
	}

	// Net Event
	for(i=NF_ACTION_NET_EVENT_TROUBLE; i<NF_ACTION_NET_EVENT_NR; i++)
	{
		if(_nf_action->rise_net & (1<<i))
			e_send_data->type2.rise[EMAIL_EVENT_TYPE2_NET][i][div_count]++;
	}

	// Set Snapshot CH
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; i++)
	#else
	for(i=0; i<NUM_ALARM; i++)
	#endif
	{
		sdata = &_nf_action->sensor_data[i];

		if((_nf_action->rise_alarm >> i) & 0x1)
		{
			if(sdata->push_act)
				mask_lcamera_alarm |= sdata->mask_lcamera;
		}
	}

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		mdata = &_nf_action->motion_data[i];

		if((_nf_action->rise_motion >> i) & 0x1)
		{
			if(mdata->push_act)
				mask_motion |= (1 << i);
		}
	}

	for(i=0; i<NF_ACTION_REC_EVENT_NR; i++)
	{
		rdata = &_nf_action->rec_data[i];

		if((_nf_action->rise_rec_panic >> i) & 0x1)
		{
			if(rdata->push_act)
				mask_rec_panic |= (1 << i);
		}
	}

	mask_snapshot_ch = (mask_motion | mask_lcamera_alarm | mask_rec_panic);
	mask_snapshot_ch &= ~((gint)nf_notify_get_param0("vloss"));

	for(i=0; i<NUM_ACTIVE_CH; i++)
	{
		if((mask_snapshot_ch >> i) & 0x1)
		{
			m_data->snapshot_ch = i;
			break;
		}
		else
			m_data->snapshot_ch = 0xff;
	}
}

	static gboolean
_nf_action_push_get_recent_event(PUSH_DATA *m_data, EMAIL_EVENT_DATA *e_send_data, PUSH_STATE *m_state, RECENT_EVENT_DATA *recent_event)
{
	gint    div_count = m_state->div_count;
	gint    i=0, ret=0;
	guint64   mask=0, mask_snapshot_ch=0, mask_motion=0, mask_lcamera_alarm=0, mask_rec_panic=0;
	ALARM_SENSOR_DATA *sdata=NULL;
	MOTION_DATA *mdata=NULL;
	REC_DATA *rdata=NULL;

	if (!recent_event)
		return FALSE;
		
	memset(recent_event, 0, sizeof(RECENT_EVENT_DATA));
	
	/** IMSI AI BOX **/
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);
		
		if(_nf_action->rise_ai[i])
		{
			guint mask=0, evt_cnt=0;

			mask=_nf_event_ai_get_evt_mask(_nf_action->rise_ai[i]);
				
			for(evt_cnt=0; evt_cnt<NF_ACTION_AI_NR; evt_cnt++)
			{
				if(mask & (1 << evt_cnt))
				{
					if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE5_AI, 5) ) {
						recent_event->event_type = 5;
						recent_event->event_num = EMAIL_EVENT_TYPE5_AI;
						recent_event->timestamp = _nf_action->ai_timestamp[i][evt_cnt];
						recent_event->ch = i;
						recent_event->event_num_sub = evt_cnt;
						
						if(recent_event->event_num_sub == NF_ACTION_AI_GENERIC)
							_save_ai_generic_evt(e_send_data, i);
							
						#if defined(DEBUG_AI)
						printf("\033[0;35m %s [DEBUG_AI] EMAIL_EVENT_TYPE5_AI \033[0;39m\n", __FUNCTION__);
						#endif
						return TRUE;
					}
				}
			}
		}
	}
	
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		mask = (0x1ULL<<i);
		if( _nf_action->rise_alarm & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_SENSOR, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_SENSOR;
				recent_event->timestamp = _nf_action->alarm_timestamp[i];
				recent_event->ch = i;
				return TRUE;
			}
		}
	}
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);
		if( _nf_action->rise_motion & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_MOTION, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_MOTION;
				recent_event->timestamp = _nf_action->motion_timestamp[i];
				recent_event->ch = i;
				return TRUE;
			}
		}
			
		if( _nf_action->rise_vloss & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_VLOSS, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_VLOSS;
				recent_event->timestamp = _nf_action->vloss_timestamp[i];
				recent_event->ch = i;
				return TRUE;
			}
		}
		if( _nf_action->rise_pos & mask ) {			
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_POS, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_POS;
				recent_event->timestamp = time(NULL);
				recent_event->ch = i;
				printf("\033[0;31m %s POS SEND\033[0;39m\n", __FUNCTION__);
				return TRUE;
			}
		}
			
#if defined(ENABLE_EVENT_TAMPER)
		if( _nf_action->rise_tamper & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_TAMPER, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_TAMPER;
				recent_event->timestamp = _nf_action->tamper_timestamp[i];
				recent_event->ch = i;
				return TRUE;
			}
		}
#endif
	}

	// DVA Event
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);
		if( _nf_action->rise_dva_obj_cnt & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_DVA_OBJ_CNT, 0) ) {
				#if defined(DEBUG_DVA_OBJ_CNT)
				printf("\033[0;35m %s rise_dva_obj_cnt \033[0;39m\n", __FUNCTION__);
				#endif
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_DVA_OBJ_CNT;
				recent_event->timestamp = _nf_action->meta_dva_obj_cnt.timestamp;
				recent_event->timestampl = _nf_action->meta_dva_obj_cnt.timestampl;
				recent_event->ch = i;
				return TRUE;
			}
		}

		if( _nf_action->rise_dva_idz & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_DVA_IDZ, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_DVA_IDZ;
				recent_event->timestamp = _nf_action->meta_dva_idz.timestamp;
				recent_event->timestampl = _nf_action->meta_dva_idz.timestampl;
				recent_event->ch = i;
				return TRUE;
			}
		}
			
			
		if( _nf_action->rise_dva_ipz & mask ) {
			if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE1_DVA_IPZ, 0) ) {
				recent_event->event_type = 1;
				recent_event->event_num = EMAIL_EVENT_TYPE1_DVA_IPZ;
				recent_event->timestamp = _nf_action->meta_dva_ipz.timestamp;
				recent_event->timestampl = _nf_action->meta_dva_ipz.timestampl;
				recent_event->ch = i;
				return TRUE;
			}
		}
	}
	/** Panic Record **/
	if( _nf_action->rise_rec_panic ) {
		if( _nf_action_push_is_act(0, 0, EMAIL_EVENT_TYPE1_REC_PANIC, 0) ) {
			recent_event->event_type = 1;
			recent_event->event_num = EMAIL_EVENT_TYPE1_REC_PANIC;
			recent_event->timestamp = _nf_action->rec_panic_timestamp;
			return TRUE;
		}
	}

	// Disk Event
	for(i=NF_ACTION_HDD_EVENT_OVER; i<NF_ACTION_HDD_EVENT_NR; i++)
	{
		if(_nf_action->rise_hdd & (1<<i)) {
			if( _nf_action_push_is_act(0, i, EMAIL_EVENT_TYPE2_HDD, 1) ) {
				recent_event->event_type = 2;
				recent_event->event_num = EMAIL_EVENT_TYPE2_HDD;
				recent_event->event_num_sub = i;
				recent_event->timestamp = _nf_action->hdd_timestamp;
				return TRUE;
			}
		}
	}

	// System Event
	for(i=NF_ACTION_SYSTEM_EVENT_BOOTING; i<NF_ACTION_SYSTEM_EVENT_NR; i++)
	{
		if(_nf_action->rise_system & (1<<i)) {
			if( _nf_action_push_is_act(0, i, EMAIL_EVENT_TYPE2_SYSTEM, 1) ) {
				recent_event->event_type = 2;
				recent_event->event_num = EMAIL_EVENT_TYPE2_SYSTEM;
				recent_event->event_num_sub = i;
				recent_event->timestamp = _nf_action->system_timestamp;
				return TRUE;
			}
		}
	}

	// Net Event
	for(i=NF_ACTION_NET_EVENT_TROUBLE; i<NF_ACTION_NET_EVENT_NR; i++)
	{
		if(_nf_action->rise_net & (1<<i)) {
			if( _nf_action_push_is_act(0, i, EMAIL_EVENT_TYPE2_NET, 1) ) {
				recent_event->event_type = 2;
				recent_event->event_num = EMAIL_EVENT_TYPE2_NET;
				recent_event->event_num_sub = i;
				recent_event->timestamp = _nf_action->net_timestamp;
				return TRUE;
			}
		}
	}
	/*VCA*/
	#if defined(SUPPORT_VCA_CAMERA)
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		mask = (guint)(1<<i);
		#if defined(DEBUG_VCA_PUSH)
		//printf("\033[0;36m %s _nf_action->rise_vca[%d] = %d\033[0;39m\n", __FUNCTION__,i,_nf_action->rise_vca[i]);
		#endif
		if(_nf_action->rise_vca[i])
		{
			guint mask=0, evt_cnt=0;

			mask=_nf_event_vca_get_evt_mask(_nf_action->rise_vca[i]);
				
			for(evt_cnt=0; evt_cnt<16; evt_cnt++)
			{
				if(mask & (1 << evt_cnt))
				{
					if( _nf_action_push_is_act(i, 0, EMAIL_EVENT_TYPE3_VCA, 2) ) {
						recent_event->event_type = 3;
						recent_event->event_num = EMAIL_EVENT_TYPE3_VCA;
						recent_event->timestamp = _nf_action->cb_vca_timestamp[i][evt_cnt];
						recent_event->ch = i;
						recent_event->event_num_sub = evt_cnt;
						#if defined(DEBUG_VCA_PUSH)
						printf("\033[0;35m %s EMAIL_EVENT_TYPE3_VCA \033[0;39m\n", __FUNCTION__);
						#endif
						return TRUE;
					}
				}
			}
		}
	}
	#endif	
	
	return FALSE;
}

#if 0
	static void
_nf_action_push_action(void)
{
	GTimeVal        curr_timeval;

	PUSH_DATA            *m_data = &_nf_action->push_data;
	PUSH_STATE         *m_state = &_nf_action->push_state;
	EMAIL_EVENT_DATA    *e_send_data = _nf_action->push_send_data;
	gint            ret=0, i=0;
	gint            div_count = m_state->div_count;

	if(m_data->push_act == 0)
		return;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

#ifdef DEBUG_ACTION_LOG
  if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_PUSH_ACTION] )
	{
		gint x=0, y=0, z=0;
		static glong check_time = 0;

		if( check_time == curr_timeval.tv_sec ){
			goto out;
		}else{
			check_time = curr_timeval.tv_sec;

			g_message("%s send_time[%ld] curr[%ld]",
          __FUNCTION__, m_state->send_time,  curr_timeval.tv_sec);

		}

		for(x=0;x<NUM_ACTIVE_CH;++x)
		{
      if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_PUSH_ACTION] & (1<<x) )
			{
        for(y=0;y<m_state->div_frequency;++y)
				{
					g_print("ch[%2d] freq[%2d] %c rise", x, y,
              (y == m_state->div_count) ? '*' : ' ');
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].rise[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.rise[z][i][y] );
					}

					g_print("  active");
					for(z=0; z<EMAIL_EVENT_TYPE1_NR; ++z)
					{
						g_print(" %2d", e_send_data->type1[x].active[z][y] );
					}
					for(z=0; z<EMAIL_EVENT_TYPE2_NR; ++z)
					{
						for(i=0; i<5; i++)
							g_print(" %2d", e_send_data->type2.active[z][i][y] );
					}

					g_print("\n");
				}
			} // if
		} // for x
	}
out:

#endif

	_nf_action_push_event_check(m_data, e_send_data, m_state);

	if(m_state->send_time <= curr_timeval.tv_sec)
	{
		int ch_num=0, event_num=0, min_num=0, event_cate=0;
		guint event_sum=0;

#ifdef DEBUG_ACTION_LOG
    if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_PUSH_ACTION] )
			g_message("%s push_send", __FUNCTION__);
#endif

		// push event type1
		for( event_num=0; event_num < EMAIL_EVENT_TYPE1_NR; ++event_num )
		{
			gint ch_max=0;

			if(event_num == EMAIL_EVENT_TYPE1_SENSOR) {
				#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
					ch_max = _nf_action_num_alarm;
				#else
					ch_max=NUM_ALARM;
				#endif
			} else if(event_num == EMAIL_EVENT_TYPE1_REC_PANIC) {
				ch_max=NF_ACTION_REC_EVENT_NR;
			} else {
				ch_max=NUM_ACTIVE_CH;
			}

			for( ch_num=0; ch_num < ch_max; ++ch_num )
			{
				if( _nf_action_push_is_act(ch_num, 0, event_num, 0) )
				{
					event_sum=0;
					for( min_num=0; min_num <= m_state->div_frequency; ++min_num)
						event_sum += e_send_data->type1[event_num].rise[ch_num][min_num];
					e_send_data->type1[event_num].rise[ch_num][63] = (gushort)event_sum;
				}
			}
		}

		// push event type2
		for( event_num=0; event_num < EMAIL_EVENT_TYPE2_NR; ++event_num )
		{
			gint event_max=0;

			if(event_num == EMAIL_EVENT_TYPE2_HDD)
				event_max=NF_ACTION_HDD_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SYSTEM)
				event_max=NF_ACTION_SYSTEM_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_NET)
				event_max=NF_ACTION_NET_EVENT_NR;
			else if(event_num == EMAIL_EVENT_TYPE2_SETUP_CHG)
				event_max=1;

			for( event_cate=0; event_cate<event_max; event_cate++ )
			{
				if( _nf_action_push_is_act(0, event_cate, event_num, 1) )
				{
					event_sum = 0;
					for( min_num=0; min_num < m_state->div_frequency; ++min_num)
						event_sum += e_send_data->type2.rise[event_num][event_cate][min_num];

					e_send_data->type2.rise[event_num][event_cate][63] = (gushort)event_sum;
				}
			}
		}

		// push send

		ret = _nf_action_push_send(m_state->send_time);
		if (ret == -1)
		{
			g_warning("%s _push_send() ret[%d]", __FUNCTION__, ret);
		}

		// time resetting and snapshot free
		_nf_action_push_reset(curr_timeval.tv_sec);
	}
	else
	{
		if(curr_timeval.tv_sec >= m_state->start_time)
		{
			m_state->div_count = (curr_timeval.tv_sec - m_state->start_time) / 60;

			// g_assert;
			if( m_state->div_count >= NF_ACTION_EMAIL_MAX_INTERVAL)
			{
				g_warning("%s div_count overflow[%d]", __FUNCTION__, m_state->div_count);
				m_state->div_count = NF_ACTION_EMAIL_MAX_INTERVAL - 1;
			}
		}
	}
}

#else
	static void
_nf_action_push_action(void)
{
	GTimeVal        curr_timeval;

	PUSH_DATA            *m_data = &_nf_action->push_data;
	PUSH_STATE         *m_state = &_nf_action->push_state;
	EMAIL_EVENT_DATA    *e_send_data = _nf_action->push_send_data;
	RECENT_EVENT_DATA	recent_event;
	gint            ret=0, i=0;
	gboolean is_over_ignr_itvl = 0;
	guint	ignr_itvl_sec = 0;

	if(m_data->push_act == 0)
		return;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	ignr_itvl_sec = m_data->frequency * 60;
	if (m_data->frequency == 0) //immediately
		ignr_itvl_sec = 5;
		
	is_over_ignr_itvl = curr_timeval.tv_sec >= (m_state->send_time + ignr_itvl_sec);
	if (!is_over_ignr_itvl)
		return;
	
	if (_nf_action_push_get_recent_event(m_data, e_send_data, m_state, &recent_event))
	{
		//g_printf("is_over_ignr_itvl[%d] event_num[%d] timestamp[%lld] send_time[%ld] cur_tv_sec[%ld]\n", 
		//	is_over_ignr_itvl, recent_event.event_num, recent_event.timestamp, m_state->send_time, curr_timeval.tv_sec);

		if(_nf_action->vendor == VENDOR_VIDECON || _nf_action->vendor == VENDOR_CBC )
		{
			if(_push_act_switch_check(m_data, e_send_data, &recent_event) == TRUE)
			{
				return;
			}
		}
		#if defined(DEBUG_DVA_OBJ_CNT)
		printf("\033[0;35m %s 1 \033[0;39m\n", __FUNCTION__);
		#endif	
		if(recent_event.event_type > 0
		&& recent_event.timestamp > m_state->send_time
		&& recent_event.timestamp <= curr_timeval.tv_sec)
		{
			#if defined(DEBUG_DVA_OBJ_CNT)
			printf("\033[0;35m %s 2 \033[0;39m\n", __FUNCTION__);
			#endif	
			ret = _nf_action_push_send(&recent_event,m_state->send_time);
			if (ret == -1)
				g_warning("%s _push_send() ret[%d]", __FUNCTION__, ret);
				
			m_state->send_time = curr_timeval.tv_sec;
			//_nf_action_push_reset(curr_timeval.tv_sec);
		}
	}
}
#endif

/******************************************************************************************
  PUSH END
 ******************************************************************************************/
#endif


#if defined (ENABLE_MOUSE_UNTILKEY_STOP)        /** 091222 by pakkhman **/
void nf_action_mouse_untilkey_stop(void)
{
	++_mouse_key_in_check;
}
#endif

/******************************************************************************************
  IPX
 ******************************************************************************************/
void nf_action_relay_webra_onoff(gint relay_num)
{
	RELAY_STATE	*rstate = &_nf_action->relay_state[relay_num];

	if(rstate->onoff)
	{
		_relay_webra_onoff &= ~(0x1ULL<<relay_num);
		rstate->onoff = OFF;
		rstate->dwell_in_sec = 0;
		rstate->dwell_out_sec = 0;
	}
	else
	{
		_relay_webra_onoff |= (0x1ULL<<relay_num);
		rstate->onoff = ON;
	}
}

gboolean nf_action_relay_test(gboolean is_test_on, gint relay_num, gboolean type)
{
	guint64 relay_type=0;

	if(relay_num >= NUM_RELAY)
	{
		g_warning("%s Relay Num is wrong. Relay num[%d] Relay Max Num[%d]\n",
				__FUNCTION__, relay_num, NUM_RELAY);
		return FALSE;
	}

#if 0
	g_message("%s called.. is_test_on[%d] relay_num[%d] type[%d]\n", __FUNCTION__, is_test_on, relay_num, type);
#endif

	if(!nf_dev_relay_get_type(&relay_type))
		return FALSE;

	if(type == NF_ACTION_TYPE_NCLOSE)
		relay_type |= (0x1ULL<< relay_num);
	else
		relay_type &= ~(0x1ULL<< relay_num);

	if(!nf_dev_relay_set_type(relay_type))
		return FALSE;

	g_message("%s is_test_on: %d", __FUNCTION__, is_test_on);
	g_message("%s relay_num: %d", __FUNCTION__, relay_num);
	g_message("%s type: %d", __FUNCTION__, type);

	NF_OBJECT_LOCK( _nf_action );

	if(is_test_on)
	{
		_nf_action->relay_state[relay_num].is_manual_test=TRUE;
	}else{
		_nf_action->relay_state[relay_num].is_manual_test=FALSE;
	}

	NF_OBJECT_UNLOCK( _nf_action );

	return TRUE;
}

void nf_action_buzzer_test(gboolean is_test_on)
{
#if 0
	g_message("%s called.. is_test_on[%d]\n", __FUNCTION__, is_test_on);
#endif
	NF_OBJECT_LOCK( _nf_action );
	if(is_test_on)
		_nf_action->buzzer_state.is_manual_test=TRUE;
	else
		_nf_action->buzzer_state.is_manual_test=FALSE;
	NF_OBJECT_UNLOCK( _nf_action );
}

/**
  For Group Authority
 **/
void nf_action_set_action_ctrl(gboolean is_action_ctrl)
{
	NF_OBJECT_LOCK( _nf_action );

	_nf_action->is_action_ctrl=is_action_ctrl;

	NF_OBJECT_UNLOCK( _nf_action );
}

static char nf_action_jbshell_cmd_help[] = "nf_actoin";
static int nf_action_jbshell_cmd(int argc, char **argv)
{
	gint active_ch=0;
#if 0
	for(active_ch=0; active_ch<NUM_ACTIVE_CH; active_ch++)
	{
		ALARM_SENSOR_DATA   *sdata = &_nf_action->sensor_data[active_ch];
		MOTION_DATA         *mdata = &_nf_action->motion_data[active_ch];
		VLOSS_DATA          *vdata = &_nf_action->vloss_data[active_ch];

		sdata->mask_arout=0xffff;
		sdata->buzzer_act=TRUE;
	}
#endif
	if(strcmp(argv[1], "b") == 0)
	{
		//		_nf_action->cb_rise_alarm= 0x1;
		_nf_action->sensor_data[0].buzzer_act=1;
		_nf_action->buzzer_data.dwell_time=5;

		g_message("%s  _nf_action->sensor_data[0].buzzer_act --> [%d]", __FUNCTION__, _nf_action->sensor_data[0].buzzer_act);
	}
	else if(strcmp(argv[1], "relay") == 0)
	{
		ALARM_SENSOR_DATA   *sdata = &_nf_action->sensor_data[0];
		MOTION_DATA         *mdata = &_nf_action->motion_data[1];
		VLOSS_DATA          *vdata = &_nf_action->vloss_data[0];

		_nf_action->relay_sched_data.sched_mode=NF_ACTION_RELAY_CHAR_EVENT;
		_nf_action->cb_rise_alarm=0xffff;
		_nf_action->cb_rise_motion=0xffff;
		sdata->mask_arout=0x1;
		mdata->mask_arout=0x2;

		_nf_action->relay_data[0].dwell_time=5;
		_nf_action->relay_data[1].dwell_time=7;


		g_message("%s sensor [0x%08x] motion [0x%08x]\n", __FUNCTION__, sdata->mask_arout, mdata->mask_arout);
	}
	else
		g_message("Unknown Command!!\n");

	return 0;
}
__commandlist(nf_action_jbshell_cmd, "nf_action", nf_action_jbshell_cmd_help, nf_action_jbshell_cmd_help);

/*
   ?�메??보내�?   typedef	struct _SnapShotMail_t
   {
   Uint32	uTime;
   char	sTag[32];
   char	sMemo[256];
   char	sAddress[128];
   }	SnapShotMail_t;
   */
/*
   ?�바?�스 ?�일

// for event/action process

mknod /dev/sensor c 242 0
mknod /dev/relay c 243 0

// for ui input
mknod /dev/jog c 245 0
mknod /dev/keypad c 244 0
mknod /dev/shuttle c 246 0
mknod /dev/remocon c 247 0

alarm in/out 관??모듈?� ?�기??dwell ?�???�안 relay out
email send

event?� action ??관�?
event?�서 driver�?부??poll???�서 action?�로 ?�달?��???
action쪽에??지?�시�?ex:email send)??발생?????�기 ?�문???�을 분리?�다.

while(1)
{
action_polling_delay(DELAY_TIME);
g_async_queue;

relay_action();
buzzer_action();
popup_action();
}

case SendEmailStatistic:
case InformVLToACT:
case InformMSToACT:
case InformASToACT:
case InformUSToACT:
case StopActionThread:	// email ?�계 ?�보 보내???�레?��? stop ?�다.

email?� ?�트?�크???�라??지?�이 발생 ?��?�?보낼 ??별도???�레?��? 만들?�서 보낸?? -> 메일 발송???�레??& queue;
�?��?�한, ?�버?�로??UI?�게 ?�업???�울 api?�공 받기 (vpopup)
email getsnapshot
*/
