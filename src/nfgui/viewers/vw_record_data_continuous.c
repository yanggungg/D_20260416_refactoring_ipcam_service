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


static RecordParamData *continuous_data[REC_WEEKLY_PARAM] = {NULL,};

gchar* (*continuous_info)[PARAM_COL_MAX] = NULL;
gchar (*continuousArea)[24];

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
				db_info = continuous_data[day]->sizeParam[ch][time];

				for (cnt = 0; cnt < buf_len; cnt++)
				{				
					if (buf[cnt] == db_info)
					{
						is_matched = TRUE;
					}
				}

				if (is_matched)
				{
					continuous_data[day]->sizeParam[ch][time] = db_info;		// reset info					
					data_modify[day].size[ch][time] = FALSE;
				}
				else
				{
					continuous_data[day]->sizeParam[ch][time] = buf[0];
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
				db_info = continuous_data[day]->fpsParam[ch][time];

				for (cnt = 0; cnt < buf_len; cnt++)
				{
					if (buf[cnt] == db_info)
					{
						is_matched = TRUE;
					}
				}

				if (is_matched)
				{
					continuous_data[day]->fpsParam[ch][time] = db_info;		// reset info
					data_modify[day].fps[ch][time] = FALSE;
				}
				else
				{
					continuous_data[day]->fpsParam[ch][time] = buf[0];
					data_modify[day].fps[ch][time] = TRUE;
				}

#if defined(_PTZ_2STREAM_FPS_LIMIT)
                if (IS_PTZ(ch))
                {
                    size = translate_info_size_capable(continuous_data[day]->sizeParam[ch][time]);
                    fps = translate_info_fps_capable(ch, continuous_data[day]->fpsParam[ch][time]);

                    if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
                    {
                        if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0) 
                            || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                            || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)
                            || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
                        {
                			continuous_data[day]->fpsParam[ch][time] = buf[2];
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

void init_continuous_data(void)
{
	guint i;

	// allocation
	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		continuous_data[i] = imalloc(sizeof(RecordParamData));

		if(!continuous_data[i]) {
			g_warning("%s : record data allocation error", __FUNCTION__);

			return FALSE;
		}
	}

	continuous_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_COL_MAX));	
	if(!continuous_info) {
		g_warning("%s : record info allocation error", __FUNCTION__);

		return FALSE;
	}

	continuousArea = g_malloc0(sizeof(gchar) * (REC_WEEKLY_PARAM * 24));
	if(!continuousArea) {
		g_warning("%s : record tile area allocation error", __FUNCTION__);

		return FALSE;
	}
	memset(continuousArea, -1, sizeof(gchar) * (REC_WEEKLY_PARAM * 24));

	// get data
	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_get_recordContParam_data(continuous_data[i],i)) {
			g_warning("%s : rec param data returns FALSE", __FUNCTION__);

			return FALSE;
		}
	}
	// translate data into info
	translate_normal_param_data_into_info_weekly(continuous_data, continuous_info, REC_SUN_PARAM, 0);
	translate_normal_param_data_into_info_daily(continuous_data[REC_DAILY_PARAM], continuous_info, 0);
	latest_day = REC_DAILY_PARAM;
	latest_time = 0;

	return TRUE;
}

void remove_continuous_data(void)
{
	gint i;

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		ifree(continuous_data[i]);
		continuous_data[i] = NULL;
	}

	if(continuous_info) {
		ifree(continuous_info);
		continuous_info = NULL;
	}

	if(continuousArea) {
		g_free(continuousArea);
		continuousArea = NULL;
	}	
}

gboolean get_record_continuous_db(guint day, guint cTime)
{
	guint i;

	g_return_val_if_fail(continuous_info != NULL, FALSE);
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_get_recordContParam_data(continuous_data[i],i)) {
			g_warning("%s : rec param data returns FALSE", __FUNCTION__);
			return FALSE;
		}
	}

	if(day == REC_DAILY_PARAM)
		translate_normal_param_data_into_info_daily(continuous_data[REC_DAILY_PARAM], continuous_info, cTime);
	else
		translate_normal_param_data_into_info_weekly(continuous_data, continuous_info, day, cTime);			
	

	return TRUE;
}
 
gboolean set_record_continuous_db(ChangedDataType cdType)
{
	guint i;

	g_return_val_if_fail(continuous_data != NULL, FALSE);

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_set_recordContParam_data(continuous_data[i], i)) {
			g_warning("%s : DAL_set_recordContParam_data returns FALSE", __FUNCTION__);

			return FALSE;
		}
	}

	if(cdType == REC_PARAM_DATA_CHANGED)
		scm_put_log(CHANGE_CONT_PARAM, 0, 0);
	else if(cdType == REC_SCHED_DATA_CHANGED)
		scm_put_log(CHANGE_CONT_SCHED, 0, 0);
	
	set_changed_record_data();

	return TRUE;
}

gboolean set_record_auto_continuous_db()
{
	guint i;

	g_return_val_if_fail(continuous_data != NULL, FALSE);

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_set_recordContParam_data(continuous_data[i], i)) {
			g_warning("%s : DAL_set_recordContParam_data returns FALSE", __FUNCTION__);

			return FALSE;
		}
	}

	set_changed_record_data();

	return TRUE;
}



gboolean get_record_continuous_param_data(guint day, guint cTime)
{
	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	latest_day = day;
	latest_time = cTime;

	if(day == REC_DAILY_PARAM)
		translate_normal_param_data_into_info_daily(continuous_data[REC_DAILY_PARAM], continuous_info, cTime);
	else
		translate_normal_param_data_into_info_weekly(continuous_data, continuous_info, day, cTime);			
	
	return TRUE;
}


gboolean set_record_continuous_param_info(guint day, guint sTime, guint eTime)
{

	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	if(day == REC_DAILY_PARAM)
		translate_normal_param_info_into_data_daily(continuous_data[REC_DAILY_PARAM], continuous_info, sTime, eTime);
	else
		translate_normal_param_info_into_data_weekly(continuous_data, continuous_info, day, sTime, eTime);

	return TRUE;
}


gboolean get_record_continuous_sche_data(guint day, guint ch, guint cTime)
{

	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	if(day == REC_DAILY_PARAM) {
		translate_sche_data_into_info_daily(continuous_data[REC_DAILY_PARAM], continuous_info, ch, cTime);
	}else {
		translate_sche_data_into_info_weekly(continuous_data, continuous_info, ch, day, cTime);
	}

	return TRUE;
}

gboolean set_record_continuous_sche_info(guint day, guint ch, guint cTime)
{

	//g_return_val_if_fail(REC_DAILY_PARAM >= day, FALSE);	

	if(day == REC_DAILY_PARAM) {
		translate_sche_info_into_data_daily(continuous_data[REC_DAILY_PARAM], continuous_info, ch, cTime);
	}else {
		translate_sche_info_into_data_weekly(continuous_data, continuous_info, ch, day, cTime);
	}

	return TRUE;
}


void get_continuous_tArea(gchar (*area)[24], guint day, guint s_time, guint e_time)
{
	get_select_timeArea(continuous_data, area, day, s_time, e_time);
}


ChangedDataType is_changed_continuous_data()
{
	RecordParamData pre_continuous_data[REC_WEEKLY_PARAM];
	ChangedDataType cdType = REC_DATA_NOT_CHANGED;
	guint i;
	

	for(i=0; i<REC_WEEKLY_PARAM; i++) {
		if(!DAL_get_recordContParam_data(&pre_continuous_data[i],i)) {

			g_warning("%s : DAL_get_recordContParam_data returns FALSE", __FUNCTION__);	

			return cdType;
		}
	}

	if(get_record_schedMode()) {	// weekly
		for(i=0; i<REC_DAILY_PARAM; i++) {
			if(memcmp(continuous_data[i]->sizeParam[0], &pre_continuous_data[i].sizeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY)*4)) {

				cdType = REC_PARAM_DATA_CHANGED;
				break;
			
			}else if(memcmp(continuous_data[i]->modeParam[0], &pre_continuous_data[i].modeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY))) {

				cdType = REC_SCHED_DATA_CHANGED;
				break;

			}
		}
		
		if(memcmp(continuous_data[REC_USR_HOL_PARAM]->sizeParam[0], &pre_continuous_data[REC_USR_HOL_PARAM].sizeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY)*4)) {

			cdType = REC_PARAM_DATA_CHANGED;
		}
		else if(memcmp(continuous_data[REC_USR_HOL_PARAM]->modeParam[0], &pre_continuous_data[REC_USR_HOL_PARAM].modeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY))) {

			cdType = REC_SCHED_DATA_CHANGED;
		}

	}else {							// daily
		if(memcmp(continuous_data[REC_DAILY_PARAM]->sizeParam[0], &pre_continuous_data[REC_DAILY_PARAM].sizeParam[0], ((CAMERA_CHANNEL * HOURS_A_DAY) * 4))) {

			cdType = REC_PARAM_DATA_CHANGED;
			
		}else if(memcmp(continuous_data[REC_DAILY_PARAM]->modeParam[0], &pre_continuous_data[REC_DAILY_PARAM].modeParam[0], (CAMERA_CHANNEL * HOURS_A_DAY))) { 

			cdType = REC_SCHED_DATA_CHANGED;
		}
	}

	g_message("%s, %d, %d", __FUNCTION__, __LINE__, cdType);

	return cdType;
}

gint set_capable_continuous_data()
{
	gint ret_val = 0;

	_set_record_capable_size_data();
	_set_record_capable_fps_data();

	if(is_changed_continuous_data()) {
	
		if(!set_record_continuous_db(REC_PARAM_DATA_CHANGED))
			g_error("%s : set record continuous error", __FUNCTION__);

		ret_val = 1;
	}

	if(latest_day == REC_DAILY_PARAM)
		translate_normal_param_data_into_info_daily(continuous_data[REC_DAILY_PARAM], continuous_info, latest_time);
	else
		translate_normal_param_data_into_info_weekly(continuous_data, continuous_info, latest_day, latest_time);

	return ret_val;
}

gboolean get_continuous_size_data_modify(gint ch)
{
	return data_modify[latest_day].size[ch][latest_time];
}

gboolean get_continuous_fps_data_modify(gint ch)
{
	return data_modify[latest_day].fps[ch][latest_time];
}

