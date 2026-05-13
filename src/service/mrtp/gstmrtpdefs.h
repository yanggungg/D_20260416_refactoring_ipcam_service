/*
 * ITX Security Co.,Ltd.
 *  System software section
 *
 *  2010-02-21 jykim	created
 *  2010-07-07 jykim	revision
 */
#ifndef __GOBJ_MRTP_DEFS_H__
#define __GOBJ_MRTP_DEFS_H__



// #include <gst/gst.h>
#include <gobjbuddybuffer.h>
#include <nf_codec_header.h>


#define MRTPSRC_VERSION		"V1.0.0"


#define MRTPSRC_DBG_MSG		(1)
#define MRTPSRC_ANSI_COLOR	(0)
#define MRTPSRC_FULL_DBG	(0)


#define MINOR	0
#define MAJOR	1
#define IMPACT	2
#define WARN	3
#define ERROR	4

static const gchar* LOG_VERB_STR[5] = {
#if MRTPSRC_ANSI_COLOR
	"\033[1;49;30m" "[mrtpsrc] \033[0m",
	"\033[0;49;37m" "[mrtpsrc] \033[0m",
	"\033[1;49;34m" "[mrtpsrc] \033[0m",
	"\033[1;49;33m" "[mrtpsrc] WARNING | \033[0m",
	"\033[1;49;31m" "[mrtpsrc] ERROR | \033[0m"
#else
	"[mrtpsrc] CAMGR | ",
	"[mrtpsrc] CAMGR | ",
	"[mrtpsrc] CAMGR | ",
	"[mrtpsrc] CAMGR | WARNING - ",
	"[mrtpsrc] CAMGR | ERROR - "
#endif
};
#ifndef _WIN32
#if MRTPSRC_DBG_MSG
#define MRTPSRC_DBG(l,format,arg...) printf("%s", LOG_VERB_STR[l]);printf(format,##arg);printf("\033[0m\n")
#else
#define MRTPSRC_DBG(l,format,arg...) while(0)printf(format,##arg)
#endif
#else
#if MRTPSRC_DBG_MSG
#define MRTPSRC_DBG(l,format,...) printf("%s", LOG_VERB_STR[l]);printf(format,__VA_ARGS__)
#else
#define MRTPSRC_DBG(l,format,...) while(0)printf(format,__VA_ARGS__)
#endif
#endif

#define MRTPSRC_ALWAYS		1
#define MRTPSRC_NEVER		0

const static gchar* MRTPSRC_STREAM_STR[] = {
	"1ST STREAM",
	"2ND STREAM",
#if MRTPSRC_SUPPORT_3RD_STREAM
	"3RD STREAM",
#endif
	"AUDIO STREAM",
	"META STREAM"
};



/*
 *
 * Receive Ring Buffer Physical Structure
 *  rbs: ring buffer start
 *  rbe: ring buffer end
 *  cp : consume point
 *  lp : load point
 *
 *
 * ------------>|<----- A ----->|<B>|<---------------- C ----
 *              |               |   |
 * +--------------------------------------------------------+
 * |            |               |   |                       |
 * +--------------------------------------------------------+
 * |            |                   |                       |
 * rbs          cp                  lp                      rbe
 *
 * A: buf_used
 * B: MRTPSRC_RING_GAP
 * C: buf_remain
 */
#define MRTPSRC_RING_GAP		(4)		// cp should not be the same with lp. And multiple-4 value may prevent bus error
#define MRTPSRC_MAX_CH 			(32)	// Maximum channel number, each channel may have 1st~3rd stream
#define MRTPSRC_MAX_CB_LEN		(256)	// RTSP address buffer length
#define MRTPSRC_NBUF_SZ			(1536)	// 1024 + 512

/* Ring buffer size
 *  audio stream:  64 KBytes
 *  3rd stream: 2*128 KBytes
 *  2nd stream: 4*128 KBytes
 *  1st stream: 16*128 KBytes
 */
#define MRTPSRC_RCV_BUF_SZ		(0x30000)   //192kb

#define MRTPSRC_SUPPORT_3RD_STREAM	(0)

//#define MRTPSRC_USER_AGENT_STR		"NetworkVideoRecorder"
#define MRTPSRC_USER_AGENT_STR		"IPX-NVR"
#define MRTPSRC_USER_AGENT_AMB_STR	"iIPCAM"

#define MRTPSRC_ADD_SPSPPS
//#undef MRTPSRC_ADD_SPSPPS
#define MRTPSRC_FRAME_SEQ_POS (0x1f)
#define MRTPSRC_NAL_UNIT_MASK (0x1f)

#define MRTPSRC_ENLARGE_SKB			(1)
#define MRTPSRC_ABORTIVE_DISCONNECT (0)
#define MRTPSRC_NO_LOCAL_CRC_CHECK	(0)
#define MRTPSRC_SET_PRIORITY        (1)
#define MRTPSRC_CTRL_FRAME			(1)

#define MRTPSRC_AUDIO_RING_SZ		(0x6000)
#define MRTPSRC_AUDIO_HO_SZ			(8320)
#define MRTPSRC_AUDIO_RING_PRINT	(0)
#define MRTPSRC_AUDIO_MAX_SZ		(32000)

#define MRTPSRC_PRINT_MRAW		(0)
#define MRTPSRC_PRINT_MRAW_ABS	(0)
#define MRTPSRC_USE_STATISTICS	(1)
#define MRTPSRC_EVENTLOG_PUT	(0)
#define MRTPSRC_RECORD_HOFF		(1)
#define MRTPSRC_LIVE_PAD_PUSH	(1)	//live queue push 
#define MRTPSRC_AUDIO_PAD_PUSH	(1)
#define MRTPSRC_CAM_STAT_DUMP   (1)

/********************** FIXME. modify here !!!!!!!!!! **********************/
#define MRTPSRC_VLOSS_CONNECTIVITY (15000)	// if $(MRTPSRC_VLOSS_CONNECTIVITY) loop catches no packet then vloss
#define MRTPSRC_VLOSS_CONNECTIVITY_TIME	(15)	// if $(MRTPSRC_VLOSS_CONNECTIVITY) loop catches no packet then vloss
//#define MRTPSRC_TOLERANT_TS_SECS	(5)	// $(MRTPSRC_TOLERANT_TS_SECS) seconds time difference is tolerable
#define MRTPSRC_TOLERANT_TS_SECS	(30)	// $(MRTPSRC_TOLERANT_TS_SECS) seconds difference is tolerable
#define MRTPSRC_MIN_IDR_ALIGN		(0x4000)	// 16 kbytes
/***************************************************************************/

#define MRTPSRC_SUPPORT_SMART_CAMERA

#define MRTPSRC_PRINT_IP(x) (((x)&0x000000FF)),(((x)&0x0000FF00)>>8),(((x)&0x00FF0000)>>16),(((x)&0xFF000000)>>24)




enum MRTPSRC_SYS_ENUM__
{
	/* Return Codes */
	RTN_FAIL = 0,
	RTN_OK,

	/* Error No */
	ERR_NO_ERROR = 0,

	ERR_OPEN_INVALID_CH_NUM = 1,
	ERR_OPEN_INVALID_STREAM,
	ERR_OPEN_NULL_LOCATION,
	ERR_OPEN_INVALID_PORT_NUM,
	ERR_OPEN_INVALID_RTP_METHOD,
	ERR_OPEN_CH_DUP = 6,
	ERR_OPEN_INVALID_LOCATION,
	ERR_OPEN_CONN_FAIL,
	ERR_OPEN_OPTION_FAIL,
	ERR_OPEN_NO_AUDIO,
	ERR_OPEN_NO_AUDIO_PT = 11,
	ERR_OPEN_NO_AUDIO_CTRL,
	ERR_OPEN_NO_VIDEO,
	ERR_OPEN_NO_VIDEO_PT,
	ERR_OPEN_NO_VIDEO_CTRL,
	ERR_OPEN_NO_VIDEO_RTPMAP = 16,
	ERR_OPEN_NO_VIDEO_FMTP,
	ERR_OPEN_ENC_H264,
	ERR_OPEN_NO_VIDEO_SPSPPS,
	ERR_OPEN_DESCRIBE_FAIL,
	ERR_OPEN_SESSION_FAIL = 21,
	ERR_OPEN_RES_TEMP_UNAVAILABLE,

	ERR_PLAY_PLAY_REQ_FAIL,

	ERR_RTP_UNDEFINED_TYPE,
	ERR_RTP_UNHANDLED_TYPE,
	ERR_RTP_MEM_FAIL = 26,

	ERR_TRDN_INTERNAL,
	ERR_TRDN_INVALID_CH,
	ERR_TRDN_INVALID_STREAM,
	ERR_TRDN_N_PLAYING_CH,
	ERR_TRDN_FAIL = 31,

	/* Stream States */
	STATE_INITIAL = 0,
	STATE_TCP_DISCONNECTED,
	STATE_TCP_CONNECTED,
	STATE_SESSION_DESCRIBED,
	STATE_SESSION_OBTAINED,
	STATE_TCPPLAY_REQUESTED,
	STATE_STREAM_READY,
	STATE_PLAYING,
	STATE_I_ONLY,
	STATE_P_REQUESTED,
	STATE_DROP_P_1,
	STATE_DROP_P_2,
	STATE_DROP_P_5,
	STATE_DROP_P_ALL,
	STATE_SHADOW_STREAM,
	STATE_RECONN_REQ,
	STATE_REBOOT_REQ,
	STATE_PAUSED,
	STATE_TEARED_DOWN,

	/* Frame types */
	FTYPE_N_IDR = 0,
	FTYPE_IDR,
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
	FTYPE_VA_META=7,
	FTYPE_VA_EVENT=8,
	FTYPE_VA_SKIP=9,
#endif
	FTYPE_AUDIO = 10,
	FTYPE_NULL = 0xff,

	/* RTP type/NAL type */
	RTP_UNDEFINED_T0 = 0,
	RTP_SINGLE_NAL_N_IDR = 1,
	RTP_CSD_A,
	RTP_CSD_B,
	RTP_CSD_C,
	RTP_SINGLE_NAL_IDR = 5,
	RTP_SEI,
	RTP_SPS,
	RTP_PPS,
	RTP_AUD,
	RTP_EO_SEQ = 10,
	RTP_EO_STREAM,
	RTP_FILTER_DATA,
	RTP_SPS_EXT,
	/* 14-23 reserved fields */
	RTP_STAP_A = 24,
	RTP_STAP_B,
	RTP_MTAP_16,
	RTP_MTAP_24,
	RTP_FU_A,
	RTP_FU_B,
	RTP_UNDEFINED_T30,
	RTP_UNDEFINED_T31,

	/* Audio type */
	MRTP_AUDIO_ULAW = 0,
	MRTP_AUDIO_ALAW,

	/* Stream */
	STREAM_1ST = 0,
	STREAM_2ND,
#if MRTPSRC_SUPPORT_3RD_STREAM
	STREAM_3RD,
#endif
	STREAM_AUDIO,
	STREAM_META,
	STREAM_MAX,

	/* FPS */
	FPS_UNDEFINED = 0,
	FPS_300 = 1<< 0,
	FPS_150 = 1<< 1,
	FPS_100 = 1<< 2,
	FPS_75  = 1<< 3,
	FPS_60  = 1<< 4,
	FPS_50  = 1<< 5,
	FPS_43  = 1<< 6,
	FPS_38  = 1<< 7,
	FPS_33  = 1<< 8,
	FPS_30  = 1<< 9,
	FPS_27  = 1<< 10,
	FPS_25  = 1<< 11,
	FPS_20  = 1<< 12,
	FPS_10  = 1<< 13,
	FPS_AUDIO = 1<< 14,

	MODEL_AMB_A2 = 0,
	MODEL_TI_365,
	MODEL_AMB_D1,
	MODEL_ONVIF_GENERAL,
	MODEL_ONVIF_SUPPORT,
	MODEL_ONVIF_GRUNDIG,
	MODEL_NVS,
	MODEL_MAX,
	MODEL_ONTHEFLY_CHEAT = -1,

	RTP_OVER_RTSP_TCP = 0,	// set default
	RTP_UNICAST_UDP,
	RTP_MULTICAST_UDP,

	RTP_PT_NONE = 0,
	RTP_PT_H264 = 1,
	RTP_PT_H265 = 2,

	// H265 Nal Unit Type
	NAL_TRAIL_N = 0,
	NAL_TRAIL_R = 1,
	NAL_TSA_N = 2,
	NAL_TSA_R = 3,
	NAL_STSA_N = 4,
	NAL_STSA_R = 5,
	NAL_RADL_N = 6,
	NAL_RADL_R = 7,
	NAL_RASL_N = 8,
	NAL_RASL_R = 9,
	NAL_BLA_W_LP = 16,
	NAL_BLA_W_RADL = 17,
	NAL_BLA_N_LP = 18,
	NAL_IDR_W_RADL = 19,
	NAL_IDR_N_LP = 20,
	NAL_CRA_NUT = 21,
	NAL_VPS = 32,
	NAL_SPS = 33,
	NAL_PPS = 34,
	NAL_AUD = 35,
	NAL_EOS_NUT = 36,
	NAL_EOB_NUT = 37,
	NAL_FD_NUT = 38,
	NAL_SEI_PREFIX = 39,
	NAL_SEI_SUFFIX = 40,
	NAL_FUS = 49,
};

typedef struct _MOTION_INFO_T_ MRTPSRC_MRAW_T;
struct _MOTION_INFO_T_
{
	gint ch;
	gint stream_num;
	gint width;
	gint height;
	guint timestamp;
	guint timestampl;
	
	guchar mraw[512];

};

struct _FRAME_INFO_T_
{
	guint frame_seq;
	guint frame_type;
	guint len;

	guint alarm_flag;
	guint motion_flag;
	guint mblock_size;
	MRTPSRC_MRAW_T motion_raw;

	ICODEC_HEADER icodec_h;
	GobjBuddyBuffer* frame;

	struct _FRAME_INFO_T_* next;
};
typedef struct _FRAME_INFO_T_* MRTPSRC_FRAME_T;

struct _RCV_BUFFER_T_
{
	guchar *lp;		// load pointer
	guchar *cp;		// consume pointer
	guchar *rbs;	// rcv buffer start addr
	guchar *rbe;	// rcv buffer end addr

	gint buf_remain;
	gint buf_used;

	//GstBuffer* buf;
	guchar* buf;
};
typedef struct _RCV_BUFFER_T_ MRTPSRC_RBUF_T;

typedef struct __IPX_AUDIO_DATA_T_ MRTPSRC_AUDIO_T;
struct __IPX_AUDIO_DATA_T_
{
	gint start;
	gint len;
	guchar data[MRTPSRC_AUDIO_MAX_SZ];
};

struct __LIVE_STATISTICS_T_
{
	GTimeVal base_time;
	//struct timespec base_time;
	guint clear_interval;

	guint frame_count[32][4];
	guint i_cnt[32][3];
	guint p_cnt[32][3];

	guint frame_bytes[32][4];
	guint i_bytes[32][3];
	guint p_bytes[32][3];

	guint frame_len_average[32][4];
	guint i_avg[32][3];
	guint p_avg[32][3];

	guint i_max_len[32][3];
	guint p_max_len[32][3];

	guint frame_interval_min[32][4];
	guint interval_20_cnt[32][3];
	guint interval_10_cnt[32][3];

	guint frame_interval_max[32][4];
	guint interval_40_cnt[32][3];
	guint interval_70_cnt[32][3];
	guint interval_100_cnt[32][3];
	guint interval_130_cnt[32][3];

	guint frame_last_sec[32][4];
	guint frame_last_msec[32][4];
};
typedef struct __LIVE_STATISTICS_T_ MRTPSRC_LIVE_STAT_T;

struct __STATISTICS_T_
{
	guint live_ch_cnt;

	guint64 recv_cnt;
	guint64 recv_miss;
	guint64 recv_bytes;

	guint frame_cnt;
	guint i_cnt;
	guint p_cnt;
	guint audio_cnt;

	guint handoff_cnt;
	guint drop_cnt;

	guint ts_sys_cnt;		// system time change event
	guint ts_err_cnt;		// camera timestamp error
	guint ts_compen_cnt;	// timestamp compenstaion

	guint reconn_server_close[32];
	guint reconn_sock_corrupt[32];
	guint reconn_ring_full[32];
	guint reconn_wrong_magic[4][32];
	guint reconn_mem_fail[32];
	guint reconn_cmem_fail[32];
	guint reconn_vlen_fail[32];
	guint reconn_systime[32];
	guint reconn_timestamp[32];
	guint reconn_audio_send[32];
	guint reboot_resume_fail[32];
	guint reboot_disconn[32];
	guint reboot_hoff_mem[32];
	guint reboot_hoff_cmem[32];

	guint reconn_cnt[64];
	guint64 cbc_read[64];
	guint64 cbc_read_miss[64];
	guint cbc_sys_time_chg[64];
	guint cbc_ts_err[64];
	guint cbc_ts_com[64];
	guint cbc_frame_cnt[64];

	guint frame_drop_ssrc[4][32];
	guint frame_drop_seqnum[4][32];
};
typedef struct __STATISTICS_T_ MRTPSRC_STREAM_STAT_T;

struct _STREAM_INFO_T_
{
	gint rtsp_sock;					// rtsp socket descriptor
	guint req_seq;					// rtsp sequence number
	guint state;						// stream state

	gint ch_num;						// stream number
	gint stream_num;					// session number
	gint buf_sz;						// ring buffer size

	guint ip_addr;					// camera ip address
	guint rtsp_port;
	gchar rtsp_addr[MRTPSRC_MAX_CB_LEN];		// camera rtsp address

	gchar username[64];
	gchar password[64];
	gchar realm[512];
	gchar nonce[512];

	/* for udp conn */
	guchar rtp_protocol;
	gint udp_sock[2];
	guint client_port[2];
	guint server_port[2];
	gchar dest_addr[MRTPSRC_MAX_CB_LEN];

	/* grundig only */
	gint audio_out_sock;
	guint audio_out_client_port;
	guint audio_out_server_port;
	guint16 audio_out_seq;

	/* sdp info */
	guint en; 						// Encoding Name
	guint donl;
	guint pt;
	guint sec_dur;
	guint sps_len;
	guint pps_len;
	guint sei_len;
	guint vps_len;

	guint resolution;
	guint fps;

	guint ts_last_sendsec;
	guint ts_last_rcvd;				// last received timestamp
	guint ts_last_second;
	guint ts_last_5_mili;
	guint ts_last_remainder;
	guint ts_last_diff;
	guint ts_first_second;
	guint ts_first_5_mili;
	guint frame_num;
	guint model;
	guint gop_seq;
	guint gop;
	guint gop_len;

	guint ring_full_cnt;

	/* session info */
	gchar rtsp_cb[MRTPSRC_MAX_CB_LEN];	// content-base
	gchar ctrl[128];
	gchar sps[128];
	gchar pps[128];
	gchar vps[128];
	gchar sei[128];
	gchar session[128];
	gchar rtsp_validation[16];

	/* rtcp info */
	guint16 seq_group;
	guint16 seq_last;
	guint sr_ssrc;
	guint ts_last_sr;

	/* security */
	guchar seed_video;
	guchar seed_audio;
	guchar seed_sps;
	guchar seed_pps;
	guchar seed_vps;
	guchar pbUserKey[16];

	/* audio info */
	guint audio_type;
	guint audio_rate;

	guchar itlv_rtp_ch;
	guchar itlv_rtcp_ch;
	guint16 itlv_align;

	MRTPSRC_RBUF_T rbuf;
	MRTPSRC_AUDIO_T abuf;
	MRTPSRC_FRAME_T frame_head;
	MRTPSRC_FRAME_T frame_tail;

#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
	guchar check_sei;
	MRTPSRC_FRAME_T sei_meta_1st;
	MRTPSRC_FRAME_T sei_meta_2nd;
	MRTPSRC_FRAME_T sei_event_1st;
	MRTPSRC_FRAME_T sei_event_2nd;
#endif
	
	guint sps_added;
	guchar is_gortsplib;
};
typedef struct _STREAM_INFO_T_ MRTPSRC_STREAM_T;

struct _CHANNEL_INFO_T_
{
	guint state;

	gint ch_num;
	gint model_code;

	gint active_video_cnt;
	gint is_audio_activated;
	gint is_metadata_on;
	gint is_metadata_enable;
	gint shadow_flag;

	guchar dev_mac[8];

	gint audio_qlen;
	guchar audio_qbuf[1024*128];

	MRTPSRC_STREAM_T *stream_1;
	MRTPSRC_STREAM_T *stream_2;
#if MRTPSRC_SUPPORT_3RD_STREAM
	MRTPSRC_STREAM_T *stream_3;
#endif
	MRTPSRC_STREAM_T *stream_a;

	MRTPSRC_STREAM_T *stream_m;
};
typedef struct _CHANNEL_INFO_T_ MRTPSRC_CHANNEL_T;

struct RTSP_INTERLEAVED_FRAME_T
{
	guchar magic;
	guchar channel;
	guint16 len;
};
typedef struct RTSP_INTERLEAVED_FRAME_T MRTPSRC_RTSP_IF_T;

/* only static 12 bytes of RTP header */
struct RTP_HEADER_T
{
	guint cc:4;
	guint x:1;
	guint p:1;
	guint v:2;

	guint pt:7;
	guint m:1;

	guint seq:16;

	guint timestamp;
	guint ssrc;
};
typedef struct RTP_HEADER_T MRTPSRC_RTP_HEADER_T;

struct RTP_HEADER_X_T
{
	guint16 prof;
	guint16 xlen;
};
typedef struct RTP_HEADER_X_T MRTPSRC_RTP_HEADER_X_T;

struct RTCP_HEADER_T
{
	guint cc:5;
	guint p:1;
	guint v:2;

	guint pt:8;

	guint len:16;

	guint ssrc;
	guint ts_msw;
	guint ts_lsw;
	guint ts_rtp;
	guint sender_pkt_cnt;
	guint sender_octet_cnt;
};
typedef struct RTCP_HEADER_T MRTPSRC_RTCP_HEADER_T;

typedef struct __IPX_AUDIO_RING_T_ MRTPSRC_AUDIO_RING_T;
struct __IPX_AUDIO_RING_T_
{
	gint lp;
	gint cp;
	gint len;
	guint timestamp;
	guint timestampl;
	guchar data[MRTPSRC_AUDIO_RING_SZ];
};

typedef struct _REC_HANDOFF_QUEUE_* MRTPSRC_RECORD_HO_Q;
struct _REC_HANDOFF_QUEUE_
{
	GobjBuddyBuffer *frame;
	struct _REC_HANDOFF_QUEUE_ *next;
};



extern void mrtpsrc_closesync_lock(void);
extern void mrtpsrc_closesync_unlock(void);
extern void mrtpsrc_frame_q_lock(void);
extern void mrtpsrc_frame_q_unlock(void);
extern guint mrtpsrc_get_errno(void);
extern void mrtpsrc_rtsp_client_initialize(void);
extern void mrtpsrc_rtsp_client_finalize(void);
extern guint mrtpsrc_pause_stream(guint, guint);
extern guint mrtpsrc_resume_stream(guint, guint);
extern guint mrtpsrc_close_stream(guint, guint);
extern guint mrtpsrc_open_stream(guint, guint, guint, guint, gchar*, gchar*, gchar*, guint, guint, guint, guchar, guint);
extern MRTPSRC_STREAM_STAT_T *mrtpsrc_get_stat_instance(void);
extern MRTPSRC_STREAM_T* mrtpsrc_get_stream(guint, guint);
extern MRTPSRC_CHANNEL_T* mrtpsrc_get_channel(guint);

extern gint mrtpsrc_parse_video(MRTPSRC_STREAM_T*);
extern gint mrtpsrc_parse_audio(MRTPSRC_STREAM_T*);
extern gint mrtpsrc_parse_metadata(MRTPSRC_STREAM_T*);
extern void mrtpsrc_ho_control_frame(MRTPSRC_STREAM_T*, int);
extern GTimeVal mrtpsrc_get_captured_time(void);
extern MRTPSRC_AUDIO_RING_T *mrtpsrc_get_audio_ring(void);
extern void mrtpsrc_tx_audio_lock(void);
extern void mrtpsrc_tx_audio_unlock(void);

extern void mrtpsrc_rtsp_client_md5(char*, char*);
extern void mrtpsrc_ho_enqueue(GobjBuddyBuffer *frame);

extern void* mrtpsrc_alloc_heap(size_t msz, const char* filename, const char* funcname, const int line);
extern void mrtpsrc_free_heap(const char* filename, const char* funcname, const int line, void* mem);
extern GobjBuddyBuffer* mrtpsrc_alloc_cmem(size_t msz, void* pch, const char* filename, const char* funcname, const int line);
extern void mrtpsrc_free_cmem(const char* filename, const char* funcname, const int line, GobjBuddyBuffer* buf);
extern void mrtpsrc_gst_buffer_free_cmem(const char* filename, const char* funcname, const int line, GobjBuddyBuffer* buf);
extern void mrtpsrc_check_ring_ptr(void* ptr, int dch, const char* filename, const char* funcname, const int line);
extern int mrtpsrc_get_rtsp_msg_len(guchar* buf, gint maxlen);

#endif // __GOBJ_MRTP_DEFS_H__


