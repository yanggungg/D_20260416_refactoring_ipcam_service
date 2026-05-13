#include <string.h>

#include "nf_afx.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_codec_data_translate.h"

#include "scm.h"


////////////////////////////////////////////////////////////
//
// private variable
//

gchar* recPriMode[REC_PRIORITY_MAX] = 
				{REC_PRIORITY_DAYS, 
					REC_PRIORITY_QUALITY};

gchar* recSchMode[REC_SCHED_MODE_MAX] = 
				{REC_SCHED_MODE_DAILY, 
					REC_SCHED_MODE_WEEKLY};

gchar* recPreTime[REC_PRE_TIME_MAX] = 
				{REC_PRE_TIME_0_SEC, 
					REC_PRE_TIME_1_SEC, 
					REC_PRE_TIME_2_SEC, 
					REC_PRE_TIME_3_SEC, 
					REC_PRE_TIME_4_SEC, 
					REC_PRE_TIME_5_SEC,
					REC_PRE_TIME_10_SEC,
					REC_PRE_TIME_15_SEC};

gchar* recPostTime[REC_POST_TIME_MAX] = 
				{REC_POST_TIME_5_SEC, 
					REC_POST_TIME_10_SEC, 
					REC_POST_TIME_15_SEC, 
					REC_POST_TIME_20_SEC, 
					REC_POST_TIME_30_SEC, 
					REC_POST_TIME_60_SEC, 
					REC_POST_TIME_120_SEC, 
					REC_POST_TIME_180_SEC};

gchar* recPanicTime[REC_PANIC_TIME_MAX] = 
				{REC_PANIC_TIME_MANUAL,
					REC_PANIC_TIME_1_MIN, 
					REC_PANIC_TIME_5_MIN, 
					REC_PANIC_TIME_10_MIN, 
					REC_PANIC_TIME_20_MIN, 
					REC_PANIC_TIME_30_MIN, 
					REC_PANIC_TIME_40_MIN, 
					REC_PANIC_TIME_50_MIN, 
					REC_PANIC_TIME_60_MIN};

gchar* size_info[CAP_RES_MAX];
gchar* fps_info[CAP_FPS_MAX];
gchar* quality_info[5];

gchar* audio_info[2] = 	{"OFF","ON"};
gchar* mode_info[4] = 	{"0", "1", "2", "3"};
gchar* alarm_ch_info[4]= {"1","2","3","4"};

static guint cam_signal_mask = 0;


////////////////////////////////////////////////////////////
//
// private functions
//

static gchar* get_size_info(gchar data)
{
	guint main_cap, main_cur;
	guint sec_cap, sec_cur;
	guint result;
	guint vloss_val;

	g_return_val_if_fail(data, NULL);

	return vw_codec_get_resol_info(data);
}

static gchar* get_fps_info(gint ch, gchar data)
{
    gint is_ntsc;

	g_return_val_if_fail(data, NULL);

	if (cam_signal_mask & (1 << ch))
	{
        is_ntsc = 0;
	}
	else
	{
        is_ntsc = 1;
	}

	return vw_codec_get_fps_info(data, is_ntsc);
}

static gchar* get_qual_info(gchar data)
{
	g_return_val_if_fail(data, NULL);

	return vw_codec_get_quality_info(data);
}

static gchar* get_audio_info(gchar data)
{
	g_return_val_if_fail(data, NULL);

	switch(data) {
		case '0':
			return audio_info[0];
		case '1':
			return audio_info[1];
	}

	return NULL;
}

static gchar* get_mode_info(gchar data)
{
	g_return_val_if_fail(data, NULL);

	switch(data) {
		case '0':
			return mode_info[0];
		case '1':
			return mode_info[1];
		case '2':
			return mode_info[2];
		case '3':
			return mode_info[3];		
	}

	return NULL;
}

static gchar get_size_data(gchar *info)
{
	return vw_codec_get_resol_data(info);
}

static gchar get_fps_data( gchar *info)
{
	return vw_codec_get_fps_data(info);
}

static gchar get_qual_data(gchar *info)
{	
	return vw_codec_get_quality_data(info);
}

static gchar get_audio_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, "OFF")) return '0';
	else if(!g_ascii_strcasecmp(info, "ON")) return '1';

	return '\0';
}

static gchar get_mode_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, "0")) return '0';
	else if(!g_ascii_strcasecmp(info, "1")) return '1';
	else if(!g_ascii_strcasecmp(info, "2")) return '2';
	else if(!g_ascii_strcasecmp(info, "3")) return '3';

	return '\0';
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

gboolean init_trans_data()
{
    gint i;

	cam_signal_mask = scm_get_cam_signal();

    for (i = 0; i < CAP_RES_MAX; i++)
    {
        size_info[i] = g_resol_info[i];
    }

    for (i = 0; i < CAP_FPS_MAX; i++)
    {
        fps_info[i] = g_fps_info[i];
    }

    for (i = 0; i < 5; i++)
    {
        quality_info[i] = g_quality_info[i];
    }
	
	return TRUE;
}

guint get_days(gchar *str)
{
	gchar *days[8] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "HOL"};

	g_return_val_if_fail(str != NULL, REC_DAILY_PARAM);

	if(!g_ascii_strcasecmp(days[0], str)) return REC_SUN_PARAM;
	else if(!g_ascii_strcasecmp(days[1], str)) return REC_MON_PARAM;
	else if(!g_ascii_strcasecmp(days[2], str)) return REC_TUE_PARAM;
	else if(!g_ascii_strcasecmp(days[3], str)) return REC_WED_PARAM;
	else if(!g_ascii_strcasecmp(days[4], str)) return REC_THU_PARAM;
	else if(!g_ascii_strcasecmp(days[5], str)) return REC_FRI_PARAM;
	else if(!g_ascii_strcasecmp(days[6], str)) return REC_SAT_PARAM;
	else if(!g_ascii_strcasecmp(days[7], str)) return REC_USR_HOL_PARAM;

	return REC_DAILY_PARAM;
}

gint translate_priMode_into_info(gchar *mode)
{
	g_return_val_if_fail(mode != NULL, -1);

	if(!g_ascii_strcasecmp(mode, REC_PRIORITY_DAYS))
		return 0;	
	else if(!g_ascii_strcasecmp(mode, REC_PRIORITY_QUALITY))
		return 1;	
	
	return -1;
}

gint translate_schedMode_into_info(gchar *mode)
{
	g_return_val_if_fail(mode != NULL, -1);

	if(!g_ascii_strcasecmp(mode, REC_SCHED_MODE_DAILY))
		return 0;	
	else if(!g_ascii_strcasecmp(mode, REC_SCHED_MODE_WEEKLY))
		return 1;	
	
	return -1;
}

gint translate_preTime_into_info(gchar *preTime)
{
	g_return_val_if_fail(preTime != NULL, 0);

	if(preTime) {
		if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_0_SEC ))
			return 0;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_1_SEC ))
			return 1;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_2_SEC))
			return 2;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_3_SEC))
			return 3;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_4_SEC))
			return 4;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_5_SEC))
			return 5;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_10_SEC))
			return 10;
		else if(!g_ascii_strcasecmp(preTime, REC_PRE_TIME_15_SEC))
			return 15;
	}

	return -1;
}


gint translate_postTime_into_info(gchar *postTime)
{
	g_return_val_if_fail(postTime != NULL, 0);

	if(postTime) {
		if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_5_SEC ))
			return 5;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_10_SEC))
			return 10;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_15_SEC))
			return 15;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_120_SEC))
			return 120;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_180_SEC))
			return 180;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_20_SEC))
			return 20;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_30_SEC))
			return 30;
		else if(!g_ascii_strcasecmp(postTime, REC_POST_TIME_60_SEC))
			return 60;
	}

	return -1;
}

gint translate_panicTime_into_info(gchar *time)
{
	g_return_val_if_fail(time != NULL, 0);

	if(time) {
		if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_MANUAL ))
			return 0;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_1_MIN ))
			return 1*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_5_MIN ))
			return 5*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_10_MIN))
			return 10*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_20_MIN))
			return 20*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_30_MIN))
			return 30*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_40_MIN))
			return 40*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_50_MIN))
			return 50*60;
		else if(!g_ascii_strcasecmp(time, REC_PANIC_TIME_60_MIN))
			return 60*60;
	}

	return -1;
}

void translate_normal_param_data_into_info_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint time)
{
	guint i;
	
	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

//    g_message("%s, %d", __FUNCTION__, __LINE__);
    
	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_SIZE_COL] = get_size_info(recData->sizeParam[i][time]);
		info_table[i][PARAM_FPS_COL] = get_fps_info(i, recData->fpsParam[i][time]);
		info_table[i][PARAM_QUAL_COL] = get_qual_info(recData->qualParam[i][time]);
		info_table[i][PARAM_AUDIO_COL] = get_audio_info(recData->audioParam[i][time]);
		info_table[i][PARAM_MODE_COL] = get_mode_info(recData->modeParam[i][time]); 
		info_table[i][PARAM_ALARMCH_COL] = NULL;
	}
}


void translate_normal_param_data_into_info_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint day, guint time)
{
	guint i;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

//    g_message("%s, %d", __FUNCTION__, __LINE__);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_SIZE_COL] = get_size_info(recData[day]->sizeParam[i][time]);
		info_table[i][PARAM_FPS_COL] = get_fps_info(i, recData[day]->fpsParam[i][time]);
		info_table[i][PARAM_QUAL_COL] = get_qual_info(recData[day]->qualParam[i][time]);
		info_table[i][PARAM_AUDIO_COL] = get_audio_info(recData[day]->audioParam[i][time]);
		info_table[i][PARAM_MODE_COL] = get_mode_info(recData[day]->modeParam[i][time]);
		info_table[i][PARAM_ALARMCH_COL] = NULL;
	}
}

#if 0
void translate_alarm_param_data_into_info_daily(AlarmRecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint time)
{
	guint i;
	
	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_SIZE_COL] = get_size_info(recData->sizeParam[i][time]);
		info_table[i][PARAM_FPS_COL] = get_fps_info(recData->fpsParam[i][time]);
		info_table[i][PARAM_QUAL_COL] = get_qual_info(recData->qualParam[i][time]);
		info_table[i][PARAM_AUDIO_COL] = get_audio_info(recData->audioParam[i][time]);
//		info_table[i][PARAM_ALARMCH_COL] = get_alarmch_info(recData->alarmChParam[i][time]);
		info_table[i][PARAM_MODE_COL] = get_mode_info(recData->modeParam[i][time]); 
	}
}


void translate_alarm_param_data_into_info_weekly(AlarmRecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint day, guint time)
{
	guint i;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_SIZE_COL] = get_size_info(recData[day]->sizeParam[i][time]);
		info_table[i][PARAM_FPS_COL] = get_fps_info(recData[day]->fpsParam[i][time]);
		info_table[i][PARAM_QUAL_COL] = get_qual_info(recData[day]->qualParam[i][time]);
		info_table[i][PARAM_AUDIO_COL] = get_audio_info(recData[day]->audioParam[i][time]);
		info_table[i][PARAM_ALARMCH_COL] = get_audio_info(recData[day]->alarmChParam[i][time]);
		info_table[i][PARAM_MODE_COL] = get_mode_info(recData[day]->modeParam[i][time]);
	}
}
#endif

void translate_panic_param_data_into_info(RecordPanicData *recData, gchar* (*info_table)[PARAM_COL_MAX])
{
	guint i;
	
	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

    g_message("%s, %d", __FUNCTION__, __LINE__);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_SIZE_COL] = get_size_info(recData->sizeParam[i]);
		info_table[i][PARAM_FPS_COL] = get_fps_info(i, recData->fpsParam[i]);
		info_table[i][PARAM_QUAL_COL] = get_qual_info(recData->qualParam[i]);
		info_table[i][PARAM_AUDIO_COL] = get_audio_info(recData->audioParam[i]);
		info_table[i][PARAM_ALARMCH_COL] = NULL;
		info_table[i][PARAM_MODE_COL] = NULL;
	}
}

void translate_netstream_param_data_into_info(SetupNetStreamData *recData, gchar* (*info_table)[PARAM_COL_MAX])
{
	guint i;
	
	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

    g_message("%s, %d", __FUNCTION__, __LINE__);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_SIZE_COL] = get_size_info(recData->sizeParam[i]);
		info_table[i][PARAM_FPS_COL] = get_fps_info(i, recData->fpsParam[i]);
		info_table[i][PARAM_QUAL_COL] = get_qual_info(recData->qualParam[i]);
		info_table[i][PARAM_AUDIO_COL] = NULL;		
		info_table[i][PARAM_ALARMCH_COL] = NULL;
		info_table[i][PARAM_MODE_COL] = NULL;		
	}
}

void translate_normal_param_info_into_data_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint sTime, guint eTime)
{
	guint i, j;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		for(j=sTime; j<=eTime; j++) {
			recData->sizeParam[i][j] = get_size_data(info_table[i][PARAM_SIZE_COL]);
			recData->fpsParam[i][j] = get_fps_data(info_table[i][PARAM_FPS_COL]);
			recData->qualParam[i][j] = get_qual_data(info_table[i][PARAM_QUAL_COL]);
			recData->audioParam[i][j] = get_audio_data(info_table[i][PARAM_AUDIO_COL]);
		}
	}
}

void translate_normal_param_info_into_data_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint days, guint sTime, guint eTime)
{
	guint i, j;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		for(j=sTime; j<=eTime; j++) {
			recData[days]->sizeParam[i][j] = get_size_data(info_table[i][PARAM_SIZE_COL]);
			recData[days]->fpsParam[i][j] = get_fps_data(info_table[i][PARAM_FPS_COL]);
			recData[days]->qualParam[i][j] = get_qual_data(info_table[i][PARAM_QUAL_COL]);
			recData[days]->audioParam[i][j] = get_audio_data(info_table[i][PARAM_AUDIO_COL]);
		}
	}
}

#if 0
void translate_alarm_param_info_into_data_daily(AlarmRecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint sTime, guint eTime)
{
	guint i, j;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		for(j=sTime; j<=eTime; j++) {
			recData->sizeParam[i][j] = get_size_data(info_table[i][PARAM_SIZE_COL]);
			recData->fpsParam[i][j] = get_fps_data(info_table[i][PARAM_FPS_COL]);
			recData->qualParam[i][j] = get_qual_data(info_table[i][PARAM_QUAL_COL]);
			recData->audioParam[i][j] = get_audio_data(info_table[i][PARAM_AUDIO_COL]);
			recData->alarmChParam[i][j] = get_audio_data(info_table[i][PARAM_ALARMCH_COL]);
		}
	}
}

void translate_alarm_param_info_into_data_weekly(AlarmRecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint days, guint sTime, guint eTime)
{
	guint i, j;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		for(j=sTime; j<=eTime; j++) {
			recData[days]->sizeParam[i][j] = get_size_data(info_table[i][PARAM_SIZE_COL]);
			recData[days]->fpsParam[i][j] = get_fps_data(info_table[i][PARAM_FPS_COL]);
			recData[days]->qualParam[i][j] = get_qual_data(info_table[i][PARAM_QUAL_COL]);
			recData[days]->audioParam[i][j] = get_audio_data(info_table[i][PARAM_AUDIO_COL]);
			recData[days]->alarmChParam[i][j] = get_audio_data(info_table[i][PARAM_ALARMCH_COL]);
		}
	}
}
#endif

void translate_panic_param_info_into_data(RecordPanicData *recData, gchar* (*info_table)[PARAM_COL_MAX])
{
	guint i;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		recData->sizeParam[i] = get_size_data(info_table[i][PARAM_SIZE_COL]);
		recData->fpsParam[i] = get_fps_data(info_table[i][PARAM_FPS_COL]);
		recData->qualParam[i] = get_qual_data(info_table[i][PARAM_QUAL_COL]);
		recData->audioParam[i] = get_audio_data(info_table[i][PARAM_AUDIO_COL]);
	}
}

void translate_netstream_param_info_into_data(SetupNetStreamData *recData, gchar* (*info_table)[PARAM_COL_MAX])
{
	guint i;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		recData->sizeParam[i] = get_size_data(info_table[i][PARAM_SIZE_COL]);
		recData->fpsParam[i] = get_fps_data(info_table[i][PARAM_FPS_COL]);
		recData->qualParam[i] = get_qual_data(info_table[i][PARAM_QUAL_COL]);
	}
}

void translate_auto_param_info_into_data(RecordAutoData *recData, gchar* (*info_table)[PARAM_AUTO_MAX])
{
	guint i;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		recData->sizeParam[i] = get_size_data(info_table[i][PARAM_AUTO_SIZE]);
		recData->fpsParam[i] = get_fps_data(info_table[i][PARAM_AUTO_FPS]);
		recData->qualParam[i] = get_qual_data(info_table[i][PARAM_AUTO_QUAL]);
		recData->audioParam[i] = get_audio_data(info_table[i][PARAM_AUTO_AUDIO]);
	}
}

void translate_auto_param_data_into_info(RecordAutoData *recData, gchar* (*info_table)[PARAM_AUTO_MAX])
{
	guint i;
	
	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

    g_message("%s, %d", __FUNCTION__, __LINE__);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_AUTO_SIZE] = get_size_info(recData->sizeParam[i]);
		info_table[i][PARAM_AUTO_FPS] = get_fps_info(i, recData->fpsParam[i]);
		info_table[i][PARAM_AUTO_QUAL] = get_qual_info(recData->qualParam[i]);
		info_table[i][PARAM_AUTO_AUDIO] = get_audio_info(recData->audioParam[i]);
	}
}

void translate_auto_intensive_param_info_into_data(RecordAutoIntensiveData *recData, gchar* (*info_table)[PARAM_AUTO_ITS_MAX])
{
	guint i;

	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		recData->normal_sizeParam[i] = get_size_data(info_table[i][PARAM_AUTO_ITS_NORMAL_SIZE]);
		recData->normal_fpsParam[i] = get_fps_data(info_table[i][PARAM_AUTO_ITS_NORMAL_FPS]);
		recData->normal_qualParam[i] = get_qual_data(info_table[i][PARAM_AUTO_ITS_NORMAL_QUAL]);
		recData->event_sizeParam[i] = get_size_data(info_table[i][PARAM_AUTO_ITS_EVENT_SIZE]);
		recData->event_fpsParam[i] = get_fps_data(info_table[i][PARAM_AUTO_ITS_EVENT_FPS]);
		recData->event_qualParam[i] = get_qual_data(info_table[i][PARAM_AUTO_ITS_EVENT_QUAL]);
		recData->audioParam[i] = get_audio_data(info_table[i][PARAM_AUTO_ITS_AUDIO]);
	}
}

void translate_auto_intensive_param_data_into_info(RecordAutoIntensiveData *recData, gchar* (*info_table)[PARAM_AUTO_ITS_MAX])
{
	guint i;
	
	g_return_if_fail(recData != NULL);
	g_return_if_fail(info_table != NULL);

    g_message("%s, %d", __FUNCTION__, __LINE__);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		info_table[i][PARAM_AUTO_ITS_NORMAL_SIZE] = get_size_info(recData->normal_sizeParam[i]);
		info_table[i][PARAM_AUTO_ITS_NORMAL_FPS] = get_fps_info(i, recData->normal_fpsParam[i]);
		info_table[i][PARAM_AUTO_ITS_NORMAL_QUAL] = get_qual_info(recData->normal_qualParam[i]);
		info_table[i][PARAM_AUTO_ITS_EVENT_SIZE] = get_size_info(recData->event_sizeParam[i]);
		info_table[i][PARAM_AUTO_ITS_EVENT_FPS] = get_fps_info(i, recData->event_fpsParam[i]);
		info_table[i][PARAM_AUTO_ITS_EVENT_QUAL] = get_qual_info(recData->event_qualParam[i]);
		info_table[i][PARAM_AUTO_ITS_AUDIO] = get_audio_info(recData->audioParam[i]);
	}
}

void translate_sche_data_into_info_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint time)
{
	info_table[ch][PARAM_MODE_COL] = get_mode_info(recData->modeParam[ch][time]);
}

void translate_sche_data_into_info_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint day, guint time)
{
	info_table[ch][PARAM_MODE_COL] = get_mode_info(recData[day]->modeParam[ch][time]);
}

void translate_sche_info_into_data_daily(RecordParamData *recData, gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint time)
{
	recData->modeParam[ch][time] = get_mode_data(info_table[ch][PARAM_MODE_COL]);
}

void translate_sche_info_into_data_weekly(RecordParamData *recData[], gchar* (*info_table)[PARAM_COL_MAX], guint ch, guint day, guint time)
{
	recData[day]->modeParam[ch][time] = get_mode_data(info_table[ch][PARAM_MODE_COL]);
}

void translate_data_size_capable(guint64 capable, gchar *buf)
{
    vw_codec_translate_capable_data_resol(capable, buf);
}

void translate_data_fps_capable(guint capable, gchar *buf)
{
    vw_codec_translate_capable_data_fps(capable, buf);
}

gchar* translate_info_size_capable(gchar data)
{
	return get_size_info(data);
}

gchar* translate_info_fps_capable(gint ch, gchar data)
{
	return get_fps_info(ch, data);
}

void get_select_timeArea(RecordParamData *data[8], gchar (*area)[24], guint day, guint s_time, guint e_time)
{
	guint i, j;
	gint color_n = -1;
	gchar* info[24][GUI_CHANNEL_CNT][PARAM_COL_MAX];
	guint used_color = 0;
	gint max = 0;

	g_return_if_fail(data != NULL);	
	g_return_if_fail(area != NULL);	

	
	if(day == REC_DAILY_PARAM) {
		for(i=0; i<24; i++) 
			translate_normal_param_data_into_info_daily(data[day], info[i], i);
	}else {
		for(i=0; i<24; i++) {
			translate_normal_param_data_into_info_weekly(data, info[i], day, i);
		}
	}

	for(i=0; i<24; i++) {
		for(j=0; j<GUI_CHANNEL_CNT; j++) {
			info[i][j][PARAM_ALARMCH_COL] = NULL;
			info[i][j][PARAM_MODE_COL] = NULL;			
		}
	}


	if(area[day][0] < 0) {						
		color_n = 0;

		for(i=0; i<24; i++) {
			if(area[day][i] < 0) {
				if(color_n == 1) 
					++color_n;

				area[day][i] = color_n++;

				for(j=i+1; j<24; j++) {
					if(!memcmp(info[i], info[j], sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_COL_MAX))) {
						if(area[day][j] != area[day][i]) 
							area[day][j] = area[day][i];
					}
				}
			}
		}
	}else {
		for(i=0; i<24; i++) {
			if(s_time <= i && i <= e_time) 
				continue;

			used_color |= (1 << area[day][i]);

			if(!memcmp(info[s_time], info[i], sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_COL_MAX))) {
				color_n = area[day][i];
				break;
			}
		}

		if(color_n >= 0) {
			for(i=s_time; i<=e_time; i++) 
				area[day][i] = color_n;
		}else {
			for(i=0; i<25; i++) {
				if(i == 1)
					continue;

				if(!(used_color & (1 << i))) {
					color_n = i;
					break;
				}
			}

			for(i=s_time; i<=e_time; i++) 
				area[day][i] = color_n;
		}
	}

	for(i=0; i<24; i++) {
		printf(" %d ", area[day][i]);
	}
	putchar('\n');
}
