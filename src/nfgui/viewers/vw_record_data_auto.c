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

static RecordAutoData *autoMot_data = NULL;
static RecordAutoData *autoAlarm_data = NULL;
static RecordAutoData *autoMotAlarm_data = NULL;

static RecordAutoIntensiveData *autoItsMot_data = NULL;
static RecordAutoIntensiveData *autoItsAlarm_data = NULL;
static RecordAutoIntensiveData *autoItsMotAlarm_data = NULL;

gchar* (*autoMot_info)[PARAM_AUTO_MAX] = NULL;
gchar* (*autoAlarm_info)[PARAM_AUTO_MAX] = NULL;
gchar* (*autoMotAlarm_info)[PARAM_AUTO_MAX] = NULL;

gchar* (*autoItsMot_info)[PARAM_AUTO_ITS_MAX] = NULL;
gchar* (*autoItsAlarm_info)[PARAM_AUTO_ITS_MAX] = NULL;
gchar* (*autoItsMotAlarm_info)[PARAM_AUTO_ITS_MAX] = NULL;




////////////////////////////////////////////////////////////
//
// private functions
//

static void _get_auto_param_data(RecordAutoData *recData, gchar* (*info_table)[PARAM_AUTO_MAX])
{
	translate_auto_param_data_into_info(recData, info_table);
}

static void _set_auto_param_info(RecordAutoData *recData, gchar* (*info_table)[PARAM_AUTO_MAX])
{
	translate_auto_param_info_into_data(recData, info_table);
}

static void _get_auto_its_param_data(RecordAutoIntensiveData *recData, gchar* (*info_table)[PARAM_AUTO_ITS_MAX])
{
	translate_auto_intensive_param_data_into_info(recData, info_table);
}

static void _set_auto_its_param_info(RecordAutoIntensiveData *recData, gchar* (*info_table)[PARAM_AUTO_ITS_MAX])
{
	translate_auto_intensive_param_info_into_data(recData, info_table);
}

static void _set_record_auto_capable_size_data(RecordAutoData *recData)
{
	gchar *buf;	
	gint buf_len;
	gchar db_info;

	gint ch, cnt; 
	gboolean is_matched;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		is_matched = FALSE;
		
		buf = get_record_size_capable_data(ch);
		buf_len = strlen(buf);

		db_info = recData->sizeParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info)
			{
				is_matched = TRUE;
			}
		}

		if (is_matched)
			recData->sizeParam[ch] = db_info;		
		else
			recData->sizeParam[ch] = buf[0];
	}
}

static void _set_record_auto_capable_fps_data(RecordAutoData *recData)
{
	gchar *buf;	
	gint buf_len;
	gchar db_info;

	gint ch, cnt; 
	gboolean is_matched;

    gchar *size;
    gchar *fps;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		is_matched = FALSE;
		
		buf = get_record_fps_capable_data(ch);
		buf_len = strlen(buf);

		db_info = recData->fpsParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info)
			{
				is_matched = TRUE;
			}
		}

		if (is_matched)
			recData->fpsParam[ch] = db_info;
		else
			recData->fpsParam[ch] = buf[0];

#if defined(_PTZ_2STREAM_FPS_LIMIT)
        if (IS_PTZ(ch))
        {
            size = translate_info_size_capable(recData->sizeParam[ch]);
            fps = translate_info_fps_capable(ch, recData->fpsParam[ch]);

            if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
            {
                if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0) 
                    || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)                    
                    || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
    			recData->fpsParam[ch] = buf[2];
    		}
		}
#endif			
	}
}

static void _set_record_auto_its_capable_size_data(RecordAutoIntensiveData *recData)
{
	gchar *buf;	
	gint buf_len;
	gchar db_info_n, db_info_e;

	gint ch, cnt; 
	gboolean is_matched_n, is_matched_e;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		is_matched_n = FALSE;
		is_matched_e = FALSE;
		
		buf = get_record_size_capable_data(ch);
		buf_len = strlen(buf);

		db_info_n = recData->normal_sizeParam[ch];
		db_info_e = recData->event_sizeParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info_n)
			{
				is_matched_n = TRUE;
			}

			if (buf[cnt] == db_info_e)
			{
				is_matched_e = TRUE;
			}
		}

		if (is_matched_n)
			recData->normal_sizeParam[ch] = db_info_n;		
		else
			recData->normal_sizeParam[ch] = buf[buf_len-1];

		if (is_matched_e)
			recData->event_sizeParam[ch] = db_info_e;
		else
			recData->event_sizeParam[ch] = buf[0];
	}
}

static void _set_record_auto_its_capable_fps_data(RecordAutoIntensiveData *recData)
{
	gchar *buf;	
	gint buf_len;
	gchar db_info_n, db_info_e;

	gint ch, cnt; 
	gboolean is_matched_n, is_matched_e;

    gchar *size;
    gchar *fps;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		is_matched_n = FALSE;
		is_matched_e = FALSE;
		
		buf = get_record_fps_capable_data(ch);
		buf_len = strlen(buf);

		db_info_n = recData->normal_fpsParam[ch];
		db_info_e = recData->event_fpsParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info_n)
			{
				is_matched_n = TRUE;
			}

			if (buf[cnt] == db_info_e)
			{
				is_matched_e = TRUE;
			}
		}

		if (is_matched_n)
			recData->normal_fpsParam[ch] = db_info_n;	
		else
			recData->normal_fpsParam[ch] = buf[0];

#if defined(_PTZ_2STREAM_FPS_LIMIT)
        if (IS_PTZ(ch))
        {
            size = translate_info_size_capable(recData->normal_sizeParam[ch]);
            fps = translate_info_fps_capable(ch, recData->normal_fpsParam[ch]);

            if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
            {
                if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0) 
                    || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
    			recData->normal_fpsParam[ch] = buf[2];
    		}
        }
#endif

		if (is_matched_e)
			recData->event_fpsParam[ch] = db_info_e;	
		else
			recData->event_fpsParam[ch] = buf[0];

#if defined(_PTZ_2STREAM_FPS_LIMIT)
        if (IS_PTZ(ch))
        {
            size = translate_info_size_capable(recData->event_sizeParam[ch]);
            fps = translate_info_fps_capable(ch, recData->event_fpsParam[ch]);

            if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
            {
                if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0) 
                    || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
    			recData->event_fpsParam[ch] = buf[2];
    		}
        }
#endif			
	}
}




static gboolean _init_auto_motion_data(void)
{
	// allocation
	autoMot_data = imalloc(sizeof(RecordAutoData));	
	if(!autoMot_data) {
		g_warning("%s : record auto motion data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	autoMot_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX));	
	if(!autoMot_info) {
		g_warning("%s : record auto motion info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_recordAuto_Motion_data(autoMot_data)) {
		g_warning("%s : rec auto motion data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_auto_param_data(autoMot_data, autoMot_info);

	return TRUE;
}

static gboolean _init_auto_alarm_data(void)
{
	// allocation
	autoAlarm_data = imalloc(sizeof(RecordAutoData));	
	if(!autoAlarm_data) {
		g_warning("%s : record auto alarm data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	autoAlarm_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX));	
	if(!autoAlarm_info) {
		g_warning("%s : record auto alarm info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_recordAuto_Alarm_data(autoAlarm_data)) {
		g_warning("%s : rec auto alarm data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_auto_param_data(autoAlarm_data, autoAlarm_info);

	return TRUE;
}

static gboolean _init_auto_motAlarm_data(void)
{
	// allocation
	autoMotAlarm_data = imalloc(sizeof(RecordAutoData));	
	if(!autoMotAlarm_data) {
		g_warning("%s : record auto motion_alarm data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	autoMotAlarm_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX));	
	if(!autoMotAlarm_info) {
		g_warning("%s : record auto motion_alarm info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_recordAuto_MotionAlarm_data(autoMotAlarm_data)) {
		g_warning("%s : rec auto motion_alarm data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_auto_param_data(autoMotAlarm_data, autoMotAlarm_info);

	return TRUE;
}

static gboolean _init_auto_intensive_motion_data(void)
{
	// allocation
	autoItsMot_data = imalloc(sizeof(RecordAutoIntensiveData));	
	if(!autoItsMot_data) {
		g_warning("%s : record auto intensive motion data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	autoItsMot_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_ITS_MAX));	
	if(!autoItsMot_info) {
		g_warning("%s : record auto intensive motion info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_recordAuto_intensive_Motion_data(autoItsMot_data)) {
		g_warning("%s : rec auto intensive motion data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_auto_its_param_data(autoItsMot_data, autoItsMot_info);

	return TRUE;
}

static gboolean _init_auto_intensive_alarm_data(void)
{
	// allocation
	autoItsAlarm_data = imalloc(sizeof(RecordAutoIntensiveData));	
	if(!autoItsAlarm_data) {
		g_warning("%s : record auto intensive alarm data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	autoItsAlarm_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_ITS_MAX));	
	if(!autoItsAlarm_info) {
		g_warning("%s : record auto intensive alarm info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_recordAuto_intensive_Alarm_data(autoItsAlarm_data)) {
		g_warning("%s : rec auto intensive alarm data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_auto_its_param_data(autoItsAlarm_data, autoItsAlarm_info);

	return TRUE;
}

static gboolean _init_auto_intensive_motAlarm_data(void)
{
	// allocation
	autoItsMotAlarm_data = imalloc(sizeof(RecordAutoIntensiveData));	
	if(!autoItsMotAlarm_data) {
		g_warning("%s : record auto intensive motion_alarm data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	autoItsMotAlarm_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_ITS_MAX));	
	if(!autoItsMotAlarm_info) {
		g_warning("%s : record auto intensive motion_alarm info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_recordAuto_intensive_MotionAlarm_data(autoItsMotAlarm_data)) {
		g_warning("%s : rec auto intensive motion_alarm data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_auto_its_param_data(autoItsMotAlarm_data, autoItsMotAlarm_info);

	return TRUE;
}


static gboolean _remove_auto_motion_data(void)
{
	if(autoMot_data) {
		ifree(autoMot_data);
		autoMot_data = NULL;
	}

	if(autoMot_info) {
		ifree(autoMot_info);
		autoMot_info = NULL;
	}

	return TRUE;
}

static gboolean _remove_auto_alarm_data(void)
{
	if(autoAlarm_data) {
		ifree(autoAlarm_data);
		autoAlarm_data = NULL;
	}

	if(autoAlarm_info) {
		ifree(autoAlarm_info);
		autoAlarm_info = NULL;
	}

	return TRUE;
}

static gboolean _remove_auto_motAlarm_data(void)
{
	if(autoMotAlarm_data) {
		ifree(autoMotAlarm_data);
		autoMotAlarm_data = NULL;
	}

	if(autoMotAlarm_info) {
		ifree(autoMotAlarm_info);
		autoMotAlarm_info = NULL;
	}

	return TRUE;
}

static gboolean _remove_auto_its_motion_data(void)
{
	if(autoItsMot_data) {
		ifree(autoItsMot_data);
		autoItsMot_data = NULL;
	}

	if(autoItsMot_info) {
		ifree(autoItsMot_info);
		autoItsMot_info = NULL;
	}

	return TRUE;
}

static gboolean _remove_auto_its_alarm_data(void)
{
	if(autoItsAlarm_data) {
		ifree(autoItsAlarm_data);
		autoItsAlarm_data = NULL;
	}

	if(autoItsAlarm_info) {
		ifree(autoItsAlarm_info);
		autoItsAlarm_info = NULL;
	}

	return TRUE;
}

static gboolean _remove_auto_its_motAlarm_data(void)
{
	if(autoItsMotAlarm_data) {
		ifree(autoItsMotAlarm_data);
		autoItsMotAlarm_data = NULL;
	}

	if(autoItsMotAlarm_info) {
		ifree(autoItsMotAlarm_info);
		autoItsMotAlarm_info = NULL;
	}

	return TRUE;
}

static gboolean _set_auto_motion_capable_data()
{
	_set_record_auto_capable_size_data(autoMot_data);
	_set_record_auto_capable_fps_data(autoMot_data);
	
	return TRUE;
}

static gboolean _set_auto_alarm_capable_data()
{
	_set_record_auto_capable_size_data(autoAlarm_data);
	_set_record_auto_capable_fps_data(autoAlarm_data);

	return TRUE;
}

static gboolean _set_auto_motAlarm_capable_data()
{
	_set_record_auto_capable_size_data(autoMotAlarm_data);
	_set_record_auto_capable_fps_data(autoMotAlarm_data);

	return TRUE;
}

static gboolean _set_auto_its_motion_capable_data()
{
	_set_record_auto_its_capable_size_data(autoItsMot_data);
	_set_record_auto_its_capable_fps_data(autoItsMot_data);

	return TRUE;
}

static gboolean _set_auto_its_alarm_capable_data()
{
	_set_record_auto_its_capable_size_data(autoItsAlarm_data);
	_set_record_auto_its_capable_fps_data(autoItsAlarm_data);
	
	return TRUE;
}

static gboolean _set_auto_its_motAlarm_capable_data()
{
	_set_record_auto_its_capable_size_data(autoItsMotAlarm_data);
	_set_record_auto_its_capable_fps_data(autoItsMotAlarm_data);
	
	return TRUE;
}

static gint _is_changed_auto_motion_data()
{
	RecordAutoData pre_data;
	gboolean ret;

	if(!DAL_get_recordAuto_Motion_data(&pre_data)) {
		g_warning("%s : DAL_get_recordAuto_Motion_data returns FALSE", __FUNCTION__);
		return 0;
	}

	if(memcmp(autoMot_data, &pre_data, sizeof(RecordAutoData)))
		return 1;
	else 
		return 0;
}

static gint _is_changed_auto_alarm_data()
{
	RecordAutoData pre_data;
	gboolean ret;

	if(!DAL_get_recordAuto_Alarm_data(&pre_data)) {
		g_warning("%s : DAL_get_recordAuto_Alarm_data returns FALSE", __FUNCTION__);
		return 0;
	}

	if(memcmp(autoAlarm_data, &pre_data, sizeof(RecordAutoData)))
		return 1;
	else 
		return 0;
}

static gint _is_changed_auto_motAlarm_data()
{
	RecordAutoData pre_data;
	gboolean ret;

	if(!DAL_get_recordAuto_MotionAlarm_data(&pre_data)) {
		g_warning("%s : DAL_get_recordAuto_MotionAlarm_data returns FALSE", __FUNCTION__);
		return 0;
	}

	if(memcmp(autoMotAlarm_data, &pre_data, sizeof(RecordAutoData)))
		return 1;
	else 
		return 0;
}

static gint _is_changed_auto_its_motion_data()
{
	RecordAutoIntensiveData pre_data;
	gboolean ret;

	if(!DAL_get_recordAuto_intensive_Motion_data(&pre_data)) {
		g_warning("%s : DAL_get_recordAuto_intensive_Motion_data returns FALSE", __FUNCTION__);
		return 0;
	}

	if(memcmp(autoItsMot_data, &pre_data, sizeof(RecordAutoIntensiveData)))
		return 1;
	else 
		return 0;
}

static gint _is_changed_auto_its_alarm_data()
{
	RecordAutoIntensiveData pre_data;
	gboolean ret;

	if(!DAL_get_recordAuto_intensive_Alarm_data(&pre_data)) {
		g_warning("%s : DAL_get_recordAuto_intensive_Alarm_data returns FALSE", __FUNCTION__);
		return 0;
	}

	if(memcmp(autoItsAlarm_data, &pre_data, sizeof(RecordAutoIntensiveData)))
		return 1;
	else 
		return 0;
}

static gint _is_changed_auto_its_motAlarm_data()
{
	RecordAutoIntensiveData pre_data;

	if(!DAL_get_recordAuto_intensive_MotionAlarm_data(&pre_data)) {
		g_warning("%s : DAL_get_recordAuto_intensive_MotionAlarm_data returns FALSE", __FUNCTION__);
		return 0;
	}

	if(memcmp(autoItsMotAlarm_data, &pre_data, sizeof(RecordAutoIntensiveData)))
		return 1;
	else 
		return 0;
}

static gboolean _get_auto_motion_db()
{
	if(!DAL_get_recordAuto_Motion_data(autoMot_data)) {
		g_warning("%s : DAL_get_recordAuto_Motion_data returns FALSE", __FUNCTION__);
		return FALSE;
	}

	_get_auto_param_data(autoMot_data, autoMot_info);
	return TRUE;
}

static gboolean _get_auto_alarm_db()
{
	if(!DAL_get_recordAuto_Alarm_data(autoAlarm_data)) {
		g_warning("%s : DAL_get_recordAuto_Alarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}

	_get_auto_param_data(autoAlarm_data, autoAlarm_info);
	return TRUE;
}

static gboolean _get_auto_motAlarm_db()
{
	if(!DAL_get_recordAuto_MotionAlarm_data(autoMotAlarm_data)) {
		g_warning("%s : DAL_get_recordAuto_MotionAlarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}

	_get_auto_param_data(autoMotAlarm_data, autoMotAlarm_info);
	return TRUE;
}

static gboolean _get_auto_its_motion_db()
{
	if(!DAL_get_recordAuto_intensive_Motion_data(autoItsMot_data)) {
		g_warning("%s : DAL_get_recordAuto_intensive_Motion_data returns FALSE", __FUNCTION__);
		return FALSE;
	}

	_get_auto_its_param_data(autoItsMot_data, autoItsMot_info);
	return TRUE;
}

static gboolean _get_auto_its_alarm_db()
{
	if(!DAL_get_recordAuto_intensive_Alarm_data(autoItsAlarm_data)) {
		g_warning("%s : DAL_get_recordAuto_intensive_Alarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}

	_get_auto_its_param_data(autoItsAlarm_data, autoItsAlarm_info);
	return TRUE;
}

static gboolean _get_auto_its_motAlarm_db()
{
	if(!DAL_get_recordAuto_intensive_MotionAlarm_data(autoItsMotAlarm_data)) {
		g_warning("%s : DAL_get_recordAuto_intensive_MotionAlarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}

	_get_auto_its_param_data(autoItsMotAlarm_data, autoItsMotAlarm_info);
	return TRUE;
}

static gboolean _set_auto_motion_db()
{
	if(!DAL_set_recordAuto_Motion_data(autoMot_data)) {
		g_warning("%s : DAL_set_recordAuto_Motion_data returns FALSE", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean _set_auto_alarm_db()
{
	if(!DAL_set_recordAuto_Alarm_data(autoAlarm_data)) {
		g_warning("%s : DAL_set_recordAuto_Alarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean _set_auto_motAlarm_db()
{
	if(!DAL_set_recordAuto_MotionAlarm_data(autoMotAlarm_data)) {
		g_warning("%s : DAL_set_recordAuto_MotionAlarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean _set_auto_its_motion_db()
{
	if(!DAL_set_recordAuto_intensive_Motion_data(autoItsMot_data)) {
		g_warning("%s : DAL_set_recordAuto_intensive_Motion_data returns FALSE", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean _set_auto_its_alarm_db()
{
	if(!DAL_set_recordAuto_intensive_Alarm_data(autoItsAlarm_data)) {
		g_warning("%s : DAL_set_recordAuto_intensive_Alarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean _set_auto_its_motAlarm_db()
{
	if(!DAL_set_recordAuto_intensive_MotionAlarm_data(autoItsMotAlarm_data)) {
		g_warning("%s : DAL_set_recordAuto_intensive_MotionAlarm_data returns FALSE", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

static gint _set_capable_auto_motion_data()
{
	_set_auto_motion_capable_data();

	if (_is_changed_auto_motion_data())
	{
		_set_auto_motion_db();
		_get_auto_param_data(autoMot_data, autoMot_info);
		set_changed_record_data();		
		return 1;
	}
	
	return 0;
}

static gint _set_capable_auto_alarm_data()
{
	_set_auto_alarm_capable_data();

	if (_is_changed_auto_alarm_data())
	{
		_set_auto_alarm_db();
		_get_auto_param_data(autoAlarm_data, autoAlarm_info);
		set_changed_record_data();		
		return 1;
	}
	
	return 0;
}

static gint _set_capable_auto_motAlarm_data()
{
	_set_auto_motAlarm_capable_data();
	
	if (_is_changed_auto_motAlarm_data())
	{
		_set_auto_motAlarm_db();
		_get_auto_param_data(autoMotAlarm_data, autoMotAlarm_info);
		set_changed_record_data();		
		return 1;
	}
	
	return 0;
}

static gint _set_capable_auto_its_motion_data()
{
	_set_auto_its_motion_capable_data();

	if (_is_changed_auto_its_motion_data())
	{
		_set_auto_its_motion_db();
		_get_auto_its_param_data(autoItsMot_data, autoItsMot_info);
		set_changed_record_data();		
		return 1;
	}
	
	return 0;
}

static gint _set_capable_auto_its_alarm_data()
{
	_set_auto_its_alarm_capable_data();

	if (_is_changed_auto_its_alarm_data())
	{
		_set_auto_its_alarm_db();
		_get_auto_its_param_data(autoItsAlarm_data, autoItsAlarm_info);
		set_changed_record_data();		
		return 1;
	}
	
	return 0;
}

static gint _set_capable_auto_its_motAlarm_data()
{
	_set_auto_its_motAlarm_capable_data();

	if (_is_changed_auto_its_motAlarm_data())
	{
		_set_auto_its_motAlarm_db();
		_get_auto_its_param_data(autoItsMotAlarm_data, autoItsMotAlarm_info);
		set_changed_record_data();		
		return 1;
	}

	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

gboolean init_auto_data(void)
{
	_init_auto_motion_data();
	_init_auto_alarm_data();
	_init_auto_motAlarm_data();
	
	_init_auto_intensive_motion_data();
	_init_auto_intensive_alarm_data();
	_init_auto_intensive_motAlarm_data();
	
	return TRUE;
}

gboolean remove_auto_data(void)
{
	_remove_auto_motion_data();
	_remove_auto_alarm_data();
	_remove_auto_motAlarm_data();

	_remove_auto_its_motion_data();
	_remove_auto_its_alarm_data();
	_remove_auto_its_motAlarm_data();

	return TRUE;
}

gboolean get_record_auto_db()
{
	_get_auto_motion_db();
	_get_auto_alarm_db();
	_get_auto_motAlarm_db();

	_get_auto_its_motion_db();
	_get_auto_its_alarm_db();
	_get_auto_its_motAlarm_db();
	
	return TRUE;
}

gboolean set_record_auto_db()
{
	_set_auto_motion_db();
	_set_auto_alarm_db();
	_set_auto_motAlarm_db();	

	_set_auto_its_motion_db();
	_set_auto_its_alarm_db();
	_set_auto_its_motAlarm_db();	

	//TODO : scm_put_log

	set_changed_record_data();

	return TRUE;
}

gboolean set_record_auto_info(gint config)
{
	if (config == AUTO_CONFIG_MOT)					
		_set_auto_param_info(autoMot_data, autoMot_info);
	else if (config == AUTO_CONFIG_ALARM)			
		_set_auto_param_info(autoAlarm_data, autoAlarm_info);
	else if (config == AUTO_CONFIG_MOT_ALARM)		
		_set_auto_param_info(autoMotAlarm_data, autoMotAlarm_info);
	else if (config == AUTO_CONFIG_ITS_MOT)			
		_set_auto_its_param_info(autoItsMot_data, autoItsMot_info);
	else if (config == AUTO_CONFIG_ITS_ALARM)		
		_set_auto_its_param_info(autoItsAlarm_data, autoItsAlarm_info);
	else if (config == AUTO_CONFIG_ITS_MOT_ALARM)	
		_set_auto_its_param_info(autoItsMotAlarm_data, autoItsMotAlarm_info);

	return TRUE;
}

gboolean is_changed_auto_data()
{
	gint check = 0;

	check |= _is_changed_auto_motion_data();
	check |= _is_changed_auto_alarm_data();
	check |= _is_changed_auto_motAlarm_data();

	check |= _is_changed_auto_its_motion_data();
	check |= _is_changed_auto_its_alarm_data();
	check |= _is_changed_auto_its_motAlarm_data();

	if (check)	return TRUE;
	else		return FALSE;
}

gint set_capable_auto_data()
{
	gint check = 0;

	check |= _set_capable_auto_motion_data();
	check |= _set_capable_auto_alarm_data();
	check |= _set_capable_auto_motAlarm_data();

	check |= _set_capable_auto_its_motion_data();
	check |= _set_capable_auto_its_alarm_data();
	check |= _set_capable_auto_its_motAlarm_data();

	return check;
}

