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

static RecordPanicData *panic_data = NULL;
gchar* (*panic_info)[PARAM_COL_MAX] = NULL;

static PANIC_PARAM_MODIFY_T data_modify = {FALSE,};

////////////////////////////////////////////////////////////
//
// private functions
//

static void _get_record_panic_param_data()
{
	translate_panic_param_data_into_info(panic_data, panic_info);
}


static void _set_record_panic_param_info()
{
	translate_panic_param_info_into_data(panic_data, panic_info);
}

static void _set_record_capable_size_data()
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

		db_info = panic_data->sizeParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info)
			{
				is_matched = TRUE;
			}
		}

		if (is_matched)
		{
			panic_data->sizeParam[ch] = db_info;		
			data_modify.size[ch] = FALSE;
		}
		else
		{
			panic_data->sizeParam[ch] = buf[0];
			data_modify.size[ch] = TRUE;
		}
	}
}

static void _set_record_capable_fps_data()
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

		db_info = panic_data->fpsParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info)
			{
				is_matched = TRUE;
			}
		}

		if (is_matched)
		{
			panic_data->fpsParam[ch] = db_info;
			data_modify.fps[ch] = FALSE;
		}
		else
		{
			panic_data->fpsParam[ch] = buf[0];
			data_modify.fps[ch] = TRUE;
		}
		
#if defined(_PTZ_2STREAM_FPS_LIMIT)
        if (IS_PTZ(ch))
        {
            size = translate_info_size_capable(panic_data->sizeParam[ch]);
            fps = translate_info_fps_capable(ch, panic_data->fpsParam[ch]);

            if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
            {
                if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0) 
                    || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
                {
        			panic_data->fpsParam[ch] = buf[2];
        			data_modify.fps[ch] = TRUE;
                }
    		}
        }
#endif	
	}
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

void init_panic_data(void)
{
	gint i;
	
	// allocation
	panic_data = imalloc(sizeof(RecordPanicData));	
	if(!panic_data) {
		g_warning("%s : record data allocation error", __FUNCTION__);

		return FALSE;
	}

	
	panic_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_COL_MAX));	
	if(!panic_info) {
		g_warning("%s : record info allocation error", __FUNCTION__);

		return FALSE;
	}


	if(!DAL_get_recordPanic_data(panic_data)) {
		g_warning("%s : rec panic data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_record_panic_param_data();

	return TRUE;
}

void remove_panic_data(void)
{
	gint i;

	if(panic_data) {
		ifree(panic_data);
		panic_data = NULL;
	}

	if(panic_info) {
		ifree(panic_info);
		panic_info = NULL;
	}
}

gboolean get_record_panic_db()
{
	if(!DAL_get_recordPanic_data(panic_data)) {
		g_warning("%s : DAL_get_recordPanic_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	_get_record_panic_param_data();

	return TRUE;
}
 
gboolean set_record_panic_db()
{
	if(!DAL_set_recordPanic_data(panic_data)) {
		g_warning("%s : DAL_set_recordPanic_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

 	scm_put_log(CHANGE_PANIC_PARAM, 0, 0);

	set_changed_record_data();

	return TRUE;
}

gboolean set_record_panic_info()
{
	_set_record_panic_param_info();

	return TRUE;
}

gboolean is_changed_panic_data()
{
	RecordPanicData pre_panic_data;
	gboolean ret;

	if(!DAL_get_recordPanic_data(&pre_panic_data)) {
		g_warning("%s : DAL_get_recordPanic_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	if(!memcmp(panic_data, &pre_panic_data, sizeof(RecordPanicData)))
		ret = FALSE;
	else 
		ret = TRUE;

	return ret;
}

gint set_capable_panic_data()
{
	gint ret_val = 0;

	_set_record_capable_size_data();
	_set_record_capable_fps_data();

	if(is_changed_panic_data()) {
	
		if(!set_record_panic_db())
			g_error("%s : set record panic error", __FUNCTION__);

		ret_val = 1;
	}
	
	_get_record_panic_param_data();

	return ret_val;
}

gboolean get_panic_size_data_modify(gint ch)
{
	return data_modify.size[ch];
}

gboolean get_panic_fps_data_modify(gint ch)
{
	return data_modify.fps[ch];
}


