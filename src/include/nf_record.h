#ifndef __NF_RECORD_H__
#define __NF_RECORD_H__

#include "nf_object.h"
#include "nf_common.h"
#include "nf_codec_header.h"

#if defined(_ANF_1648)||defined(_ATM_1624)
	#include "nf_util_device.h"
	#include "anf_rip.h"
#endif  /*_ANF_1648*/

typedef struct _NfRecord 		NfRecord;

// 	hand off
typedef int (*NF_RECORD_HANDOFF_FUNC) ( gpointer data ); 

#define NF_DSPCOMM_GOP_CNT		(30*3)
#define NF_DSPCOMM_STATE_CNT	(8)
#define NF_DSPCOMM_NUM_CH		(64)

#define NF_RECORD_MAX_CH		(32)
#define NF_RECORD_MAX_GOP_CNT	(2)
#define NF_RECORD_MAX_GOP_LEN	(30)

typedef struct _NF_DSPCOMM_STATE_T
{
	gulong	tv_sec;	
	
	gulong	i_cnt;
	gulong	i_kbyte;
	gulong	p_cnt;
	gulong	p_kbyte;
			
}NF_DSPCOMM_STATE; 

typedef struct _NF_RECORD_STAT_T
{
	gint				gop_idx[NF_DSPCOMM_NUM_CH];
	ICODEC_HEADER		*gop_arr[NF_DSPCOMM_NUM_CH];

	NF_DSPCOMM_STATE	tot_state[NF_DSPCOMM_NUM_CH];	

}NF_RECORD_STAT;

typedef struct _NF_RECORD_CH_T {	
	guchar		channel_id;		// ch #

	guchar		flags;
	guchar		quality;
	guchar		fps;
	
	guchar		is_progressive;
	guchar		codec;	
	guchar		resolution;	
	guchar		reserved;
		
	gushort		width;
	gushort		height;
	
} NF_RECORD_CH;

/*
	NF_RECORD_CH_INFO Flags �ʵ� ����
	Bit[0]: network flag	
	Bit[1]: prerecord flag
	Bit[2~7]: record_reason		
*/

typedef struct _NF_RECORD_MAN_T {	
	guchar		change;
                		
	guchar		vloss;
	guchar		alarm;
	guchar		motion;
								
	guchar		va_event;
	guchar		user_event;
								
	guchar		network;
	guchar		pre_record;
	guchar		record_reason;
	guchar 		covert;

// from sysdb
	guchar		resolution;
	guchar		fps;
	guchar		quality;
	guchar		audio;		
	guint		pre_rec_sec;	
	
	guint		audio_ch;	
	glong		post_rec_timer;
		
} NF_RECORD_MAN;

typedef struct _NF_RECORD_SST_T {	
	guint			push_frame_count;
	guint			put_frame_count;
	guint			err_frame_count;
	guint			err_frame_ts;
			
	gint			stream_id;		// sst stream id ( default : -1);	

	gboolean		is_put_iframe;
	gboolean		need_to_close;
	guint			skip_pframe_cnt;
	
	ICODEC_HEADER	last_header;
	ICODEC_HEADER	last_iframe_header;	

	ICODEC_HEADER	last_close_iframe_header;
		
	guint			frame_idx[2];
	ICODEC_HEADER	*frame_arr[2][32];
	
} NF_RECORD_SST;


typedef struct _NF_RECORD_NOTIFY_DATA_T {	
	guint 		cb_change_flag;
	
	guint64		cb_rise_alarm;	
	guint 		cb_rise_vloss;
	guint 		cb_rise_motion;
	
	guint 		cb_rise_va_event;
	guint 		cb_rise_user_event;
	
	guint64		cb_curr_alarm;
	guint 		cb_curr_vloss;
	guint 		cb_curr_motion;
		
	guint 		cb_curr_va_event;
	guint 		cb_curr_user_event;

	guint 		cb_rise_pos_event;
	guint 		cb_curr_pos_event;

	guint		cb_curr_manual_event;
	guint		cb_rise_manual_event;

	guint		cb_rise_vca_event;

	guint 		cb_rise_dva_event;
	guint 		cb_curr_dva_event;

} NF_RECORD_NOTIFY_DATA;
			
typedef enum _NF_RECORD_REQ_E
{
	NF_RECORD_REQ_NONE 			= 0,
	NF_RECORD_REQ_ON			= 1,
	NF_RECORD_REQ_REQUEST		= 2,
}NF_RECORD_REQ_E;
	
typedef enum _NF_RECORD_INTENSIVE_E
{
	NF_RECORD_INTENSIVE_NONE	= 0,
	NF_RECORD_INTENSIVE_MOTION	= 1,
	NF_RECORD_INTENSIVE_ALARM	= 2,
	NF_RECORD_INTENSIVE_BOTH	= 3,
}NF_RECORD_INTENSIVE_E;

typedef enum _NF_RECORD_SCHEDULE_E
{
	NF_RECORD_SCHEDULE_DAILY	= 0,
	NF_RECORD_SCHEDULE_WEEKLY	= 1,

	NF_RECORD_SCHEDULE_ANYDAY	= 7,
	NF_RECORD_SCHEDULE_HOLIDAY	= 8	
}NF_RECORD_SCHEDULE_E;

typedef enum _NF_RECORD_END_FLAG_E
{
	NF_RECORD_END_FLAG_NONE = 0,
	NF_RECORD_END_FLAG_FLUSH = 1,
	NF_RECORD_END_FLAG_END = 2
} NF_RECORD_END_FLAG_E;

typedef enum _NF_RECORD_FLAG_E
{
	NF_RECORD_FLAG_NONE 		= 0x0,	
	NF_RECORD_FLAG_TIMER 		= 0x1,	
	NF_RECORD_FLAG_MOTION 		= 0x2,	
	NF_RECORD_FLAG_TIMER_MOTION = NF_RECORD_FLAG_TIMER|NF_RECORD_FLAG_MOTION
} NF_RECORD_FLAG_E;

typedef enum _NF_RECORD_CTRL_FLAG_E
{
	RECORD_CTRL_FLAG_RP00 = 0,
	RECORD_CTRL_FLAG_RP01 = 1,
	RECORD_CTRL_FLAG_RP10 = 2,
	RECORD_CTRL_FLAG_RP11 = 3
} NF_RECORD_CTRL_FLAG_E;

typedef enum _NF_RECORD_REC_MODE_E
{
	NF_RECORD_REC_MODE_AUTO = 0,
	NF_RECORD_REC_MODE_MANUAL = 1,
	NF_RECORD_REC_MODE_NR

} NF_RECORD_REC_MODE_E;

typedef enum _NF_RECORD_AUTO_CONFIG_E
{
	NF_RECORD_AUTO_CONFIG_HIGH_QUAL     = 0,
	NF_RECORD_AUTO_CONFIG_MOT           = 1,
	NF_RECORD_AUTO_CONFIG_ALARM         = 2,
	NF_RECORD_AUTO_CONFIG_MOT_ALARM     = 3,
	NF_RECORD_AUTO_CONFIG_ITS_MOT       = 4,
	NF_RECORD_AUTO_CONFIG_ITS_ALARM     = 5,
	NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM = 6,
	NF_RECORD_AUTO_CONFIG_NR            = 7
} NF_RECORD_AUTO_CONFIG_E;


/* type macro */
#define NF_TYPE_RECORD				(nf_record_get_type())

#define NF_IS_RECORD(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_RECORD))
#define NF_IS_RECORD_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_RECORD))

#define NF_RECORD_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_RECORD, NfRecordClass))
#define NF_RECORD(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_RECORD, NfRecord))
#define NF_RECORD_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_RECORD, NfRecordClass))

#define NF_RECORD_CAST(obj)			((NfRecord*)(obj))
#define NF_RECORD_CLASS_CAST(klass)	((NfRecordClass*)(klass))

//typedef struct _NfRecord 		NfRecord;
typedef struct _NfRecordClass 	NfRecordClass;

/**
 * NfRecord:
 *
 * NfDVR notify class
 */
struct _NfRecord {
	NfObject 	 	object;
	
	/*< public >*/
	gint			init_done;
	
	GAsyncQueue		*queue;		
#ifdef ANF_MODEL	
	GAsyncQueue     *queue_v4l2;    /*ANF_MODEL*/
#endif
	GThread			*thread;	

	gint			thread_run;
	gint			thread_status;
											
	/*< public >*/ /* with LOCK */
			
	/*< private >*/	
	gint		record_man_start;
	gint		record_off;				// NONE, REQ, ON;
	gint		refresh_table;			// NONE, REQ;
	gint		manual_rec;				// NONE, REQ, ON;			
	gint		req_network;
	
	gint		record_mode;			// simple/advanced 
	gint		record_auto_mode;
	gint 		intensive_mode;			// NONE, INTENSIVE(M,A,M+A)
	gint		sched_mode;				// DAILY, WEEKLY	

	gint		pre_rec_time;
	gint		post_rec_time;
	gint		panic_time;
		
	GTimeVal	current_time;	
	gint		current_week;
	gint		current_hour;
	gint		current_date;
	
	gboolean	is_dst;
	gboolean	is_enable_stream_control;
	gchar		netstream_fps[32];
	gchar		netstream_quality[32];
	
	glong		panic_rec_timer;
	
	gint		log_stream_id;		// �޷¿� sst stream id ( default : -1);
																											
	NF_RECORD_HANDOFF_FUNC	handoff_func;
	long long               handoff_ch_mask;
	
	NF_RECORD_NOTIFY_DATA	notify_data;
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		gchar					linked_cam[48][32];
	#else
		gchar					linked_cam[NUM_ALARM][32];
	#endif
	gchar					linked_pos[NUM_ACTIVE_CH][32];

//	gchar					linked_dva[NUM_ACTIVE_CH][16];
		
	NF_RECORD_CH		ch[NUM_TOTAL_CHANNEL];		// ���� ����;	
	NF_RECORD_MAN		man[NUM_TOTAL_CHANNEL];		// for 1st stream
	
	NF_RECORD_SST		sst[2][NUM_TOTAL_CHANNEL];
		
	NF_RECORD_STAT		stat;
		
};

struct _NfRecordClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

typedef struct _NF_RECORD_CFG_T{
	gchar   fps[NUM_CHANNEL];   /* NF_FPS_E  */
	gchar   res[NUM_CHANNEL];   /* NF_RESOLUTION_E, don't care NTSC/PAL */
}__attribute__ ((aligned(4)))NF_RECORD_CFG;


gboolean 
nf_record_init(int wait);


/**
	@brief				ä���� ���ڵ� ���� ��ȸ
	@param[in]	ch_num	���� ä�� 
	@param[out] ch_info	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_get_channel_info(gint ch_num, NF_RECORD_CH *ch_info );

/**
	@brief				ä���� ���ڵ� watchdog
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_watchdog_is_ok();

/**
	@brief				
	@param[in]	ch_num	���� ä��
	@param[in]	frame	���� ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_put_frame(gint ch_num, gpointer frame );

/**
	@brief				rec_audio handoff ���
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_record_register_handoff(long long ch_mask, NF_RECORD_HANDOFF_FUNC handoff_func, long long mobile_mask );


/**
	@brief				�д� ���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_start( GError **error );
/**
	@brief				�д� ���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_stop(  GError **error );
/**
	@brief				�д� ���ڵ� status
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on panic recording,  %FALSE on normal recording
*/
gboolean nf_panic_record_is_set();
/**
	@brief				�д� ���ڵ� toggle
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_toggle(  GError **error );

/**
	@brief				���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_start( GError **error );

/**
	@brief				���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_stop(  GError **error ); 

gboolean nf_record_is_rec_off(void);

gboolean nf_record_is_alarm_rec(gint ch);

glong nf_record_get_panic_end_time(void);

gboolean nf_record_alarm_by_manual_event(gint ch, gboolean event_on);

typedef struct _NET_STREAM_COUNT_SIZE_T
{
	gulong	i_tot_cnt[NF_DSPCOMM_NUM_CH];
	gulong	i_tot_kbyte[NF_DSPCOMM_NUM_CH];
	gulong	p_tot_cnt[NF_DSPCOMM_NUM_CH];
	gulong	p_tot_kbyte[NF_DSPCOMM_NUM_CH];

	guint	i_avg_size[NF_DSPCOMM_NUM_CH];
	guint	p_avg_size[NF_DSPCOMM_NUM_CH];

	guint	sec_p_count[NF_DSPCOMM_NUM_CH];
} NET_STREAM_COUNT_SIZE_T;


typedef struct _NF_RECORD_STREAM_STAT_T
{
	gulong	tv_sec;	
	
	gulong	i_cnt[NF_DSPCOMM_NUM_CH];
	gulong	i_kbyte[NF_DSPCOMM_NUM_CH];
	gulong	p_cnt[NF_DSPCOMM_NUM_CH];
	gulong	p_kbyte[NF_DSPCOMM_NUM_CH];
			
} NF_RECORD_STREAM_STAT;

void nf_record_get_stream_stat( gint type,  NF_RECORD_STREAM_STAT *stat ); // 0:curr 1:diff

// 2012-11-04 ���� 1:01:52 choissi for s1 hw test
void nf_record_send_stream_stat_to_s1();	


typedef enum _NF_RECORD_CALC_MODE_E
{
	NF_RECORD_CALC_CURRENT = 0,
	NF_RECORD_CALC_AUTO_CONTINUOUS,
	NF_RECORD_CALC_AUTO_MOTION,
	NF_RECORD_CALC_AUTO_ALARM,
	NF_RECORD_CALC_AUTO_MOTION_ALARM,
	NF_RECORD_CALC_AUTO_ITS_MOTION,
	NF_RECORD_CALC_AUTO_ITS_ALARM,
	NF_RECORD_CALC_AUTO_ITS_MOTION_ALARM,
	NF_RECORD_CALC_MANUAL_CONTINUOUS,
	NF_RECORD_CALC_MANUAL_MOTION,
	NF_RECORD_CALC_MANUAL_ALARM,
	NF_RECORD_CALC_PANIC,
	NF_RECORD_CALC_MAX
} NF_RECORD_CALC_MODE_E;

typedef struct _NF_RECORD_CALC_PARAM_T
{
	guint	mode;

	guint	motion_occur_pcnt;
	guint	alarm_occur_pcnt;
} NF_RECORD_CALC_PARAM_T;

typedef struct _NF_RECORD_CALC_RESULT_T
{
	gdouble	ch_gb_per_day[NUM_CHANNEL];
	gdouble	gb_per_day;

	gdouble	hdd_full_gb;
	gdouble	hdd_remain_gb;

	gdouble	day_full;
	gdouble	day_remain;

	gdouble day_full_range;
} NF_RECORD_CALC_RESULT_T;

typedef struct _NF_RECORD_MP4_LIST{
	int num;
	char mp4_name[10][256];
} NF_RECORD_MP4_LIST;

/*
	@brief				���ڵ� �뷮���
	@param[in]  param	���(current/continuous/motion/alarm ��)
	@param[out]	result	�����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_calculate( NF_RECORD_CALC_PARAM_T param, NF_RECORD_CALC_RESULT_T *result );

#endif
