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

static SetupNetStreamData *netstream_data = NULL;
gchar* (*netstream_info)[PARAM_COL_MAX] = NULL;

static NET_PARAM_MODIFY_T data_modify = {FALSE,};


////////////////////////////////////////////////////////////
//
// private functions
//

static void get_record_netstream_param_data()
{
	translate_netstream_param_data_into_info(netstream_data, netstream_info);
}


static void set_record_netstream_param_info()
{
	translate_netstream_param_info_into_data(netstream_data, netstream_info);
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

		db_info = netstream_data->sizeParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info)
			{
				is_matched = TRUE;
			}
		}

		if (is_matched)
		{
			netstream_data->sizeParam[ch] = db_info;		
			data_modify.size[ch] = FALSE;
		}
		else
		{
			netstream_data->sizeParam[ch] = buf[0];
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

		db_info = netstream_data->fpsParam[ch];

		for (cnt = 0; cnt < buf_len; cnt++)
		{
			if (buf[cnt] == db_info)
			{
				is_matched = TRUE;
			}
		}

		if (is_matched)
		{
			netstream_data->fpsParam[ch] = db_info;	
			data_modify.fps[ch] = FALSE;
		}
		else
		{
			netstream_data->fpsParam[ch] = buf[0];
			data_modify.fps[ch] = TRUE;
		}

#if defined(_PTZ_2STREAM_FPS_LIMIT)
        if (IS_PTZ(ch))
        {
            size = translate_info_size_capable(netstream_data->sizeParam[ch]);
            fps = translate_info_fps_capable(ch, netstream_data->fpsParam[ch]);

            if (strcmp(size, size_info[CAP_RES_640x352]) == 0)
            {
                if ((strcmp(fps, fps_info[CAP_FPS_30]) == 0) 
                    || (strcmp(fps, fps_info[CAP_FPS_15]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_25]) == 0)
                    || (strcmp(fps, fps_info[CAP_FPS_12]) == 0))
                {
        			netstream_data->fpsParam[ch] = buf[2];
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

void init_netstream_data(void)
{
	gint i;
	
	// allocation
	netstream_data = imalloc(sizeof(SetupNetStreamData));	
	if(!netstream_data) {
		g_warning("%s : netwotk live data allocation error", __FUNCTION__);

		return FALSE;
	}
	
	netstream_info = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_COL_MAX));	
	if(!netstream_info) {
		g_warning("%s : netwotk live info allocation error", __FUNCTION__);

		return FALSE;
	}

	if(!DAL_get_setupNetStream_data(netstream_data)) {
		g_warning("%s : netwotk live data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	get_record_netstream_param_data();

	return TRUE;
}

void remove_netstream_data(void)
{
	gint i;

	if(netstream_data) {
		ifree(netstream_data);
		netstream_data = NULL;
	}

	if(netstream_info) {
		ifree(netstream_info);
		netstream_info = NULL;
	}
}

gboolean get_setup_netstream_db()
{
	if(!DAL_get_setupNetStream_data(netstream_data)) {
		g_warning("%s : DAL_get_networkLive_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	get_record_netstream_param_data();

	return TRUE;
}


gboolean set_setup_netstream_db()
{
	if(!DAL_set_setupNetStream_data(netstream_data)) {
		g_warning("%s : DAL_set_setupNetStream_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	scm_put_log(CHANGE_DUAL_PARAM, 0, 0);

	set_changed_record_data();

	return TRUE;
}

gboolean set_setup_netstream_info()
{
	set_record_netstream_param_info();

	return TRUE;
}

gboolean is_changed_netstream_data()
{
	SetupNetStreamData pre_netlive_data;
	gboolean ret;

	if(!DAL_get_setupNetStream_data(&pre_netlive_data)) {
		g_warning("%s : DAL_get_networkLive_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	if(!memcmp(netstream_data, &pre_netlive_data, sizeof(SetupNetStreamData)))
		ret = FALSE;
	else 
		ret = TRUE;

	return ret;
}

gint set_capable_netstream_data()
{
	gint ret_val = 0;

	_set_record_capable_size_data();
	_set_record_capable_fps_data();

	if(is_changed_netstream_data()) {
	
		if(!set_setup_netstream_db())
			g_error("%s : set record netstream error", __FUNCTION__);

		ret_val = 1;
	}
	
	get_record_netstream_param_data();

	return ret_val;
}

gboolean get_netstream_size_data_modify(gint ch)
{
	return data_modify.size[ch];
}

gboolean get_netstream_fps_data_modify(gint ch)
{
	return data_modify.fps[ch];
}

gboolean get_netstream_control_data()
{
	if(netstream_data->enableStreamControl)
		return TRUE;

	return FALSE;
}

void set_netstream_control_data(gint enable)
{
	netstream_data->enableStreamControl = enable;
}

