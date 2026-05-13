#include <string.h>

#include "nf_afx.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "scm.h"
#include "ix_mem.h"


////////////////////////////////////////////////////////////
//
// private variable
//

static RecordParamData *alarm_data[REC_WEEKLY_PARAM] = {NULL,};
gchar* (*alarm_info)[PARAM_COL_MAX] = NULL;
gchar (*alarmArea)[24];

static PARAM_MODIFY_T data_modify[REC_WEEKLY_PARAM] = {FALSE,};
static guint latest_day, latest_time;


////////////////////////////////////////////////////////////
//
// private functions
//

static void _set_record_capable_size_data()
{
	gchar *buf;	
	gint buf_len;
	gchar db_info;

	gint day, time, ch, cnt; 
	gboolean is_matched;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{	
		buf = get_record_size_capable_data(ch);
		buf_len = strlen(buf);

		for (day = 0; day < REC_WEEKLY_PARAM; day++) 
		{
			for (time = 0; time < HOURS_A_DAY; time++) 
			{
				is_matched = FALSE;
				db_info = alarm_data[day]->sizeParam[ch][time];

				for (cnt = 0; cnt < buf_len; cnt++)
				{
					if (buf[cnt] == db_info)
					{
						is_matched = TRUE;
					}
				}

				if (is_matched)
				{
					alarm_data[day]->sizeParam[ch][time] = db_info;		// reset info				
					data_modify[day].size[ch][time] = FALSE;
				}
				else
				{
					alarm_data[day]->sizeParam[ch][time] = buf[0];
					data_modify[day].size[ch][time] = TRUE;
				}
			}
		}
	}
}

static void _set_record_capable_fps_data()
{
	gchar *buf;	
	gint buf_len;
	gchar db_info;

	gint day, time, ch, cnt; 
	gboolean is_matched;

    gchar *size;
    gchar *fps;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{	
		buf = get_record_fps_capable_data(ch);
		buf_len = strlen(buf);

		for (day = 0; day < REC_WEEKLY_PARAM; day++) 
		{
			for (time = 0; time < HOURS_A_DAY; time++) 
			{
				is_matched = FALSE;
				db_info = alarm_data[day]->fpsParam[ch][time];

				for (cnt = 0; cnt < buf_len; cnt++)
				{
					if (buf[cnt] == db_info)
					{
						is_matched = TRUE;
					}
				}

				if (is_matched)
				{
					alarm_data[day]->fpsParam[ch][time] = db_info;		// reset info				
					data_modify[day].fps[ch][time] = FALSE;
				}
				else
				{
					alarm_data[day]->fpsParam[ch][time] = buf[0];
					data_modify[day].fps[ch][time] = TRUE;
				}

#if defined(_PTZ_2STREAM_FPS_LIMIT)
                if (IS_PTZ(ch))
                {        
                    size = translate_info_size_capable(alarm_data[day]->sizeParam[ch][time]);
                    fps = translate_info_fps_capable(ch, alarm_data[day]->fpsParam[ch][time]);

                    if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
                    {
                        if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0)  
                            || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                            || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)
                            || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
                        {                            
                			alarm_data[day]->fpsParam[ch][time] = buf[2];
        					data_modify[day].fps[ch][time] = TRUE;
    					}
            		}
            	}
#endif									
			}
		}
	}
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

void init_alarm_data(void)
{
	guint i;

	// allocation
	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		alarm_data[i] = imalloc(sizeof(RecordParamData));
		if(!alarm_data[i]) {
			g_warning("%s : record data allocation error", __FUNCTION__);
			
			return FALSE;
		}
	}

	alarm_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_COL_MAX));	
	if(!alarm_info) {
		g_warning("%s : record info allocation error", __FUNCTION__);

		return FALSE;
	}

	alarmArea = g_malloc0(sizeof(gchar) * (REC_WEEKLY_PARAM * 24));
	if(!alarmArea) {
		g_warning("%s : record tile area allocation error", __FUNCTION__);

		return FALSE;
	}
	memset(alarmArea, -1, sizeof(gchar) * (REC_WEEKLY_PARAM * 24));

	// get data
	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_get_recordAlarmParam_data(alarm_data[i],i)) {
			g_warning("%s : rec param data returns FALSE", __FUNCTION__);

			return FALSE;
		}
	}

	// translate data into info
	translate_normal_param_data_into_info_weekly(alarm_data, alarm_info, REC_SUN_PARAM, 0);			
	translate_normal_param_data_into_info_daily(alarm_data[REC_DAILY_PARAM], alarm_info, 0);

	latest_day = REC_DAILY_PARAM;
	latest_time = 0;

	return TRUE;
}

void remove_alarm_data(void)
{
	gint i;

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		ifree(alarm_data[i]);
		alarm_data[i] = NULL;
	}


	if(alarm_info) {
		ifree(alarm_info);
		alarm_info = NULL;
	}

	if(alarmArea) {
		g_free(alarmArea);
		alarmArea = NULL;
	}	
}

gboolean get_record_alarm_db(guint day, guint cTime)
{
	guint i;

	g_return_val_if_fail(alarm_info != NULL, FALSE);
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_get_recordAlarmParam_data(alarm_data[i],i)) {
			g_warning("%s : rec param data returns FALSE", __FUNCTION__);
			return FALSE;
		}
	}

	if(day == REC_DAILY_PARAM)
		translate_normal_param_data_into_info_daily(alarm_data[REC_DAILY_PARAM], alarm_info, cTime);
	else
		translate_normal_param_data_into_info_weekly(alarm_data, alarm_info, day, cTime);			

	return TRUE;
}


gboolean set_record_alarm_db(ChangedDataType cdType)
{
	guint i;

	g_return_val_if_fail(alarm_info != NULL, FALSE);

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_set_recordAlarmParam_data(alarm_data[i], i)) {
			g_warning("%s : DAL_set_recordAlarmParam_data returns FALSE", __FUNCTION__);

			return FALSE;
		}
	}

	if(cdType == REC_PARAM_DATA_CHANGED)
		scm_put_log(CHANGE_ALARM_PARAM, 0, 0);
	else if(cdType == REC_SCHED_DATA_CHANGED)
		scm_put_log(CHANGE_ALARM_SCHED, 0, 0);
	
	set_changed_record_data();

	return TRUE;
}

gboolean set_record_auto_alarm_db()
{
	guint i;

	g_return_val_if_fail(alarm_info != NULL, FALSE);

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_set_recordAlarmParam_data(alarm_data[i], i)) {
			g_warning("%s : DAL_set_recordAlarmParam_data returns FALSE", __FUNCTION__);

			return FALSE;
		}
	}

	set_changed_record_data();

	return TRUE;
}

gboolean get_record_alarm_param_data(guint day, guint cTime)
{
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	latest_day = day;
	latest_time = cTime;

	if(day == REC_DAILY_PARAM)
		translate_normal_param_data_into_info_daily(alarm_data[REC_DAILY_PARAM], alarm_info, cTime);
	else
		translate_normal_param_data_into_info_weekly(alarm_data, alarm_info, day, cTime);			

	return TRUE;
}

gboolean set_record_alarm_param_info(guint day, guint sTime, guint eTime)
{
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	if(day == REC_DAILY_PARAM)
		translate_normal_param_info_into_data_daily(alarm_data[REC_DAILY_PARAM], alarm_info, sTime, eTime);
	else
		translate_normal_param_info_into_data_weekly(alarm_data, alarm_info, day, sTime, eTime);

	return TRUE;
}

gboolean get_record_alarm_sche_data(guint day, guint ch, guint cTime)
{
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	if(day == REC_DAILY_PARAM) 
		translate_sche_data_into_info_daily(alarm_data[REC_DAILY_PARAM], alarm_info, ch, cTime);
	else 
		translate_sche_data_into_info_weekly(alarm_data, alarm_info, ch, day, cTime);

	return TRUE;
}

gboolean set_record_alarm_sche_info(guint day, guint ch, guint cTime)
{
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	if(day == REC_DAILY_PARAM) 
		translate_sche_info_into_data_daily(alarm_data[REC_DAILY_PARAM], alarm_info, ch, cTime);
	else 
		translate_sche_info_into_data_weekly(alarm_data, alarm_info, ch, day, cTime);

	return TRUE;
}


void get_alarm_tArea(gchar (*area)[24], guint day, guint s_time, guint e_time)
{
	get_select_timeArea(alarm_data, area, day, s_time, e_time);
}


ChangedDataType is_changed_alarm_data()
{
	RecordParamData pre_alarm_data[REC_WEEKLY_PARAM];
	ChangedDataType cdType = REC_DATA_NOT_CHANGED;
	guint i;


	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_get_recordAlarmParam_data(&pre_alarm_data[i],i)) {

			g_warning("%s : DAL_get_recordAlarmParam_data returns FALSE", __FUNCTION__);	

			return cdType;
		}
	}

	if(get_record_schedMode()) {	// weekly
		for(i=0; i<REC_DAILY_PARAM; i++) {
			if(memcmp(alarm_data[i]->sizeParam[0], &pre_alarm_data[i].sizeParam[0], ((CAMERA_CHANNEL * HOURS_A_DAY) * 4))) {
				cdType = REC_PARAM_DATA_CHANGED;
				break;

			}else if(memcmp(alarm_data[i]->modeParam[0], &pre_alarm_data[i].modeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY))) {

				cdType = REC_SCHED_DATA_CHANGED;
				break;

			}
		}

		if(memcmp(alarm_data[REC_USR_HOL_PARAM]->sizeParam[0], &pre_alarm_data[REC_USR_HOL_PARAM].sizeParam[0], ((CAMERA_CHANNEL * HOURS_A_DAY) * 4))) {
			cdType = REC_PARAM_DATA_CHANGED;

		}else if(memcmp(alarm_data[REC_USR_HOL_PARAM]->modeParam[0], &pre_alarm_data[REC_USR_HOL_PARAM].modeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY))) {
			cdType = REC_SCHED_DATA_CHANGED;
		}

	}else {							// daily	
		if(memcmp(alarm_data[REC_DAILY_PARAM]->sizeParam[0], &pre_alarm_data[REC_DAILY_PARAM].sizeParam[0], ((CAMERA_CHANNEL * HOURS_A_DAY) * 4))) {
			cdType = REC_PARAM_DATA_CHANGED;
		}else if(memcmp(alarm_data[REC_DAILY_PARAM]->modeParam[0], &pre_alarm_data[REC_DAILY_PARAM].modeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY))) { 
			cdType = REC_SCHED_DATA_CHANGED;
		}
	}
	
	return cdType;
}

gint set_capable_alarm_data()
{
	gint ret_val = 0;

	_set_record_capable_size_data();
	_set_record_capable_fps_data();

	if(is_changed_alarm_data()) {
	
		if(!set_record_alarm_db(REC_PARAM_DATA_CHANGED))
			g_error("%s : set record continuous error", __FUNCTION__);

		ret_val = 1;
	}

	if(latest_day == REC_DAILY_PARAM)
		translate_normal_param_data_into_info_daily(alarm_data[REC_DAILY_PARAM], alarm_info, latest_time);
	else
		translate_normal_param_data_into_info_weekly(alarm_data, alarm_info, latest_day, latest_time);	

	return ret_val;
}

gboolean get_alarm_size_data_modify(gint ch)
{
	return data_modify[latest_day].size[ch][latest_time];
}

gboolean get_alarm_fps_data_modify(gint ch)
{
	return data_modify[latest_day].fps[ch][latest_time];
}

