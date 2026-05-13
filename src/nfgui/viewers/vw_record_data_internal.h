#ifndef _VW_RECORD_DATA_INTERNAL_H_
#define _VW_RECORD_DATA_INTERNAL_H_

#include "objects/nfobject.h"
#include "scm.h"

typedef struct _PARAM_MODIFY_T
{
	gboolean	size[GUI_CHANNEL_CNT][HOURS_A_DAY];
	gboolean	fps[GUI_CHANNEL_CNT][HOURS_A_DAY];
} PARAM_MODIFY_T;

typedef struct _PANIC_PARAM_MODIFY_T
{
	gboolean	size[GUI_CHANNEL_CNT];
	gboolean	fps[GUI_CHANNEL_CNT];
} PANIC_PARAM_MODIFY_T;

typedef struct _NET_PARAM_MODIFY_T
{
	gboolean	size[GUI_CHANNEL_CNT];
	gboolean	fps[GUI_CHANNEL_CNT];
} NET_PARAM_MODIFY_T;

enum {
	PARAM_AUTO_SIZE			= 0,
	PARAM_AUTO_FPS			= 1,
	PARAM_AUTO_QUAL			= 2,
	PARAM_AUTO_AUDIO		= 3,
	PARAM_AUTO_MAX
};

enum {
	PARAM_AUTO_ITS_NORMAL_SIZE		= 0,
	PARAM_AUTO_ITS_NORMAL_FPS		= 1,
	PARAM_AUTO_ITS_NORMAL_QUAL		= 2,
	PARAM_AUTO_ITS_EVENT_SIZE 		= 3,
	PARAM_AUTO_ITS_EVENT_FPS		= 4,
	PARAM_AUTO_ITS_EVENT_QUAL		= 5,
	PARAM_AUTO_ITS_AUDIO			= 6,
	PARAM_AUTO_ITS_MAX
};

enum {
	PARAM_SIZE_COL 			= 0,
	PARAM_FPS_COL			= 1,
	PARAM_QUAL_COL			= 2,
	PARAM_AUDIO_COL			= 3,
	PARAM_ALARMCH_COL		= 4,
	PARAM_MODE_COL			= 5,
	PARAM_COL_MAX
};

typedef enum {
	REC_DATA_NOT_CHANGED = 0,
	REC_PARAM_DATA_CHANGED,
	REC_SCHED_DATA_CHANGED
}ChangedDataType;

enum{
	AUTO_CONFIG_HIGH_QUAL 			= 0,
	AUTO_CONFIG_MOT 				= 1,
	AUTO_CONFIG_ALARM 				= 2,
	AUTO_CONFIG_MOT_ALARM 			= 3,
	AUTO_CONFIG_ITS_MOT 			= 4,
	AUTO_CONFIG_ITS_ALARM 			= 5,
	AUTO_CONFIG_ITS_MOT_ALARM 		= 6,
	AUTO_CONFIG_MAX
};

enum {
	SCHE_OFF_EVT = 0,
	SCHE_ON_EVT = 1,
	SCHE_CANCEL_EVT = 2,
	SCHE_EVT_NUM
};

#define IS_PTZ(ch)                      (get_ptz_channel(ch) == 0 ? 1 : 0)

#define SECOND_TIME_STRING		"%d SEC"
#define MINUTE_TIME_STRING		"%d MIN" 

#define REC_PRIORITY_MAX		2
#define REC_PRIORITY_DAYS	    "HD RECORDING DATE PRIORITY"
#define REC_PRIORITY_QUALITY	"HD IMAGE QUALITY PRIORITY"

#define REC_SCHED_MODE_MAX		2
#define REC_SCHED_MODE_DAILY	"DAILY"
#define REC_SCHED_MODE_WEEKLY	"WEEKLY"

#define REC_PRE_TIME_MAX		8
#define REC_PRE_TIME_0_SEC		"0 SEC"
#define REC_PRE_TIME_1_SEC		"1 SEC"
#define REC_PRE_TIME_2_SEC		"2 SEC"
#define REC_PRE_TIME_3_SEC		"3 SEC"
#define REC_PRE_TIME_4_SEC		"4 SEC"
#define REC_PRE_TIME_5_SEC		"5 SEC"
#define REC_PRE_TIME_10_SEC		"10 SEC"
#define REC_PRE_TIME_15_SEC		"15 SEC"

#define REC_POST_TIME_MAX		8
#define REC_POST_TIME_5_SEC		"5 SEC"
#define REC_POST_TIME_10_SEC	"10 SEC"
#define REC_POST_TIME_15_SEC	"15 SEC"
#define REC_POST_TIME_20_SEC	"20 SEC"
#define REC_POST_TIME_30_SEC	"30 SEC"
#define REC_POST_TIME_60_SEC	"60 SEC"
#define REC_POST_TIME_120_SEC	"120 SEC"
#define REC_POST_TIME_180_SEC	"180 SEC"

#define REC_PANIC_MODE_MAX		3
#define REC_PANIC_MODE_DISABLE	"DISABLE"
#define REC_PANIC_MODE_AUTO		"AUTO"
#define REC_PANIC_MODE_MANUAL	"MANUAL"

#define REC_PANIC_TIME_MAX		9
#define REC_PANIC_TIME_MANUAL	"MANUAL"
#define REC_PANIC_TIME_1_MIN	"1 MIN"
#define REC_PANIC_TIME_5_MIN	"5 MIN"
#define REC_PANIC_TIME_10_MIN	"10 MIN"
#define REC_PANIC_TIME_20_MIN	"20 MIN"
#define REC_PANIC_TIME_30_MIN	"30 MIN"
#define REC_PANIC_TIME_40_MIN	"40 MIN"
#define REC_PANIC_TIME_50_MIN	"50 MIN"
#define REC_PANIC_TIME_60_MIN	"60 MIN"

extern gchar* recPriMode[REC_PRIORITY_MAX];
extern gchar* recSchMode[REC_SCHED_MODE_MAX];
extern gchar* recPreTime[REC_PRE_TIME_MAX];
extern gchar* recPostTime[REC_POST_TIME_MAX];
extern gchar* recPanicTime[REC_PANIC_TIME_MAX];

extern gchar* size_info[CAP_RES_MAX];
extern gchar* fps_info[CAP_FPS_MAX];
extern gchar* quality_info[5];
extern gchar* audio_info[2];
extern gchar* mode_info[4];
extern gchar* alarm_ch_info[4];

extern gchar* (*continuous_info)[PARAM_COL_MAX];
extern gchar* (*motion_info)[PARAM_COL_MAX];
extern gchar* (*alarm_info)[PARAM_COL_MAX];
extern gchar* (*panic_info)[PARAM_COL_MAX];
extern gchar* (*netstream_info)[PARAM_COL_MAX];

extern gchar* (*autoMot_info)[PARAM_AUTO_MAX];
extern gchar* (*autoAlarm_info)[PARAM_AUTO_MAX];
extern gchar* (*autoMotAlarm_info)[PARAM_AUTO_MAX];

extern gchar* (*autoItsMot_info)[PARAM_AUTO_ITS_MAX];
extern gchar* (*autoItsAlarm_info)[PARAM_AUTO_ITS_MAX];
extern gchar* (*autoItsMotAlarm_info)[PARAM_AUTO_ITS_MAX];

extern gchar (*continuousArea)[24];
extern gchar (*motionArea)[24];
extern gchar (*alarmArea)[24];

void init_oper_data(void);
void remove_oper_data(void);
/*inline*/ guint get_record_mode();
/*inline*/ guint get_record_ssc();
/*inline*/ guint get_record_autoConfig();
/*inline*/ guint get_record_priMode();
/*inline*/ guint get_record_schedMode();
/*inline*/ guint get_record_pre_time();
/*inline*/ guint get_record_post_time();
/*inline*/ guint get_record_panic_time();
gboolean set_record_operation_data(RecordOperData *data);
gboolean set_question_box(RecordOperData *data);
gboolean is_changed_operation_data(RecordOperData *data);
gboolean get_record_operation_data(RecordOperData *data);


void init_continuous_data(void);
void remove_continuous_data(void);
gboolean get_record_continuous_db(guint day, guint cTime);
gboolean set_record_continuous_db(ChangedDataType cdType);
gboolean set_record_auto_continuous_db();
gboolean get_record_continuous_param_data(guint day, guint cTime);
gboolean set_record_continuous_param_info(guint day, guint sTime, guint eTime);
gboolean get_record_continuous_sche_data(guint day, guint ch, guint cTime);
gboolean set_record_continuous_sche_info(guint day, guint ch, guint cTime);
void get_continuous_tArea(gchar (*area)[24], guint day, guint s_time, guint e_time);
ChangedDataType is_changed_continuous_data();
gint set_capable_continuous_data();
gboolean get_continuous_size_data_modify(gint ch);
gboolean get_continuous_fps_data_modify(gint ch);


void init_motion_data(void);
void remove_motion_data(void);
gboolean get_record_motion_db(guint day, guint cTime);
gboolean set_record_motion_db(ChangedDataType cdType);
gboolean set_record_auto_motion_db();
gboolean get_record_motion_param_data(guint day, guint cTime);
gboolean set_record_motion_param_info(guint day, guint sTime, guint eTime);
gboolean get_record_motion_sche_data(guint day, guint ch, guint cTime);
gboolean set_record_motion_sche_info(guint day, guint ch, guint cTime);
void get_motion_tArea(gchar (*area)[24], guint day, guint s_time, guint e_time);
ChangedDataType is_changed_motion_data();
gint set_capable_motion_data();
gboolean get_motion_size_data_modify(gint ch);
gboolean get_motion_fps_data_modify(gint ch);


void init_alarm_data(void);
void remove_alarm_data(void);
gboolean get_record_alarm_db(guint day, guint cTime);
gboolean set_record_alarm_db(ChangedDataType cdType);
gboolean set_record_auto_alarm_db();
gboolean get_record_alarm_param_data(guint day, guint cTime);
gboolean set_record_alarm_param_info(guint day, guint sTime, guint eTime);
gboolean get_record_alarm_sche_data(guint day, guint ch, guint cTime);
gboolean set_record_alarm_sche_info(guint day, guint ch, guint cTime);
void get_alarm_tArea(gchar (*area)[24], guint day, guint s_time, guint e_time);
ChangedDataType is_changed_alarm_data();
gint set_capable_alarm_data();
gboolean get_alarm_size_data_modify(gint ch);
gboolean get_alarm_fps_data_modify(gint ch);


void init_panic_data(void);
void remove_panic_data(void);
gboolean get_record_panic_db();
gboolean set_record_panic_db();
gboolean set_record_panic_info();
gboolean is_changed_panic_data();
gint set_capable_panic_data();
gboolean get_panic_size_data_modify(gint ch);
gboolean get_panic_fps_data_modify(gint ch);


void init_netstream_data(void);
void remove_netstream_data(void);
void set_netstream_control_data(gint enable);
gboolean get_setup_netstream_db();
gboolean set_setup_netstream_db();
gboolean set_setup_netstream_info();
gboolean is_changed_netstream_data();
gint set_capable_netstream_data();
gboolean get_netstream_size_data_modify(gint ch);
gboolean get_netstream_fps_data_modify(gint ch);
gboolean get_netstream_control_data();


gboolean init_trans_data();
guint get_days(gchar *str);
gint translate_priMode_into_info(gchar *mode);
gint translate_schedMode_into_info(gchar *mode);
gint translate_preTime_into_info(gchar *preTime);
gint translate_postTime_into_info(gchar *postTime);
gint translate_panicTime_into_info(gchar *time);
void translate_normal_param_data_into_info_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint time);
void translate_normal_param_data_into_info_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint day, guint time);
void translate_panic_param_data_into_info(RecordPanicData *recData, gchar* (*info_table)[PARAM_COL_MAX]);
void translate_netstream_param_data_into_info(SetupNetStreamData *recData, gchar* (*info_table)[PARAM_COL_MAX]);
void translate_normal_param_info_into_data_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint sTime, guint eTime);
void translate_normal_param_info_into_data_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint days, guint sTime, guint eTime);
void translate_panic_param_info_into_data(RecordPanicData *recData, gchar* (*info_table)[PARAM_COL_MAX]);
void translate_netstream_param_info_into_data(SetupNetStreamData *recData, gchar* (*info_table)[PARAM_COL_MAX]);
void translate_auto_param_info_into_data(RecordAutoData *recData, gchar* (*info_table)[PARAM_AUTO_MAX]);
void translate_auto_param_data_into_info(RecordAutoData *recData, gchar* (*info_table)[PARAM_AUTO_MAX]);
void translate_auto_intensive_param_info_into_data(RecordAutoIntensiveData *recData, gchar* (*info_table)[PARAM_AUTO_ITS_MAX]);
void translate_auto_intensive_param_data_into_info(RecordAutoIntensiveData *recData, gchar* (*info_table)[PARAM_AUTO_ITS_MAX]);
void translate_sche_data_into_info_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint time);
void translate_sche_data_into_info_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint day, guint time);
void translate_sche_info_into_data_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint time);
void translate_sche_info_into_data_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint day, guint time);
void translate_data_size_capable(guint64 capable, gchar *buf);
void translate_data_fps_capable(guint capable, gchar *buf);
gchar* translate_info_size_capable(gchar data);
gchar* translate_info_fps_capable(gint ch, gchar data);
void get_select_timeArea(RecordParamData *data[8], gchar (*area)[24], guint day, guint s_time, guint e_time);
void refresh_info_ipcam_change(RecordParamData *recData[]);


void vw_record_check_capable_param_data();
void vw_record_set_capable_param_data();
void vw_record_refresh_param_data();
gchar* get_record_size_capable_data(gint ch);
gchar* get_record_fps_capable_data(gint ch);

gboolean init_auto_data(void);
gboolean remove_auto_data(void);
gboolean get_record_auto_db();
gboolean set_record_auto_db();
gboolean set_record_auto_info(gint config);
gboolean is_changed_auto_data();
gint set_capable_auto_data();

#endif

