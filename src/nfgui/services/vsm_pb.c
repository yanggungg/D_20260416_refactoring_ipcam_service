/*
 * vsm_pb.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */


#include "nf_afx.h"

#include "../support/color.h"
#include "../support/util.h"

#include "cmm.h"
#include "scm.h"

#include "ix_mem.h"

#include "vsm.h"
#include "vsm_internal.h"

#define MIN_STEP_TIME		(5)
#define MAX_STEP_TIME		(60)

typedef enum _IMG_ST_E
{
	IMG_ST_INIT 		= 0,
	IMG_ST_STOP 		= 1,
	IMG_ST_RUN	 		= 2,
} IMG_ST_E;


static guint step_tid = 0;
static guint step_cnt = 0;

static guint image_status = IMG_ST_INIT;

////////////////////////////////////////////////////////////
//
// protected interfaces
//

static void _update_playstatus(VOM_PLAY_STATUS *status, VSM_T *pvsm)
{
	DIR_RATE_E dir_rate = DR_NONE;

	if ((status->is_play_stop == TRUE) || (status->play_mode == NF_PLAY_PARAM_MODE_STOP))
		dir_rate = DR_STOP;
	else if (status->play_mode == NF_PLAY_PARAM_MODE_PAUSE)
		dir_rate = DR_PAUSE;
	else if (status->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
		dir_rate = (status->play_mode << 24) | (status->play_dir << 16);
	else if (status->play_mode == NF_PLAY_PARAM_MODE_START) 
		dir_rate = (status->play_dir << 16) | (status->play_rate);	

	if (dir_rate != DR_NONE)
		pvsm->play_dir_rate = dir_rate;

	if(status->play_time.tv_sec != 0) 
		pvsm->play_time = status->play_time;
}

static void _send_playback_pos_log(VOM_PLAY_STATUS *status, VSM_T *pvsm)
{
    GTimeVal from_time;
    GTimeVal to_time;

    LOG_DATA_T log_data[128];
    gint log_cnt, next;
    gint i;

    LOGX_T *ctx;

    if (status->play_rate != NF_PLAY_PARAM_RATE_001) return;
    if (pvsm->pos_time.tv_sec == status->play_time.tv_sec) return;
    
    if (!pvsm->log_ctx) 
    {
        pvsm->log_ctx = scm_open_log_ctx();
        scm_set_log_filter_type(pvsm->log_ctx, 0xffffffff, LF_CAT_ALL, 0);
        scm_set_log_filter_type(pvsm->log_ctx, 0xffffffff, LF_CAT_POS, 1);        
    }

    from_time = status->play_time;
    from_time.tv_usec = 0;
    
    to_time = status->play_time;
    to_time.tv_usec = 0;    

    if (status->play_dir == NF_PLAY_PARAM_DIR_FORWARD) 
    {
		scm_set_log_filter_order(pvsm->log_ctx, LF_OLDEST);    
        to_time.tv_sec += 1;
    }
    else
    {
		scm_set_log_filter_order(pvsm->log_ctx, LF_LATEST);    
        from_time.tv_sec -= 1;
    }

	log_cnt = scm_get_log(pvsm->log_ctx, &from_time, &to_time, 128, log_data, 0);
	ctx = (LOGX_T*)pvsm->log_ctx;

    for (i = 0; i < log_cnt; i++)
    {
        posx_put_playback_log(&ctx->sysrec[i]);
    }
    
    pvsm->pos_time.tv_sec = status->play_time.tv_sec;
}

static void _send_image_status(DIR_RATE_E status)
{
	if (image_status == IMG_ST_INIT)
	{
		if ((status == DR_STOP) || (status == DR_PAUSE) || (status == DR_BWD_NEXT_FRAME) || (status == DR_FWD_NEXT_FRAME))
		{
			evt_send_to_local(INFY_PB_IMAGE_STATUS, 0, 0, GINT_TO_POINTER(IMG_ST_STOP));	
			image_status = IMG_ST_STOP;
		}
		else
		{
			evt_send_to_local(INFY_PB_IMAGE_STATUS, 0, 0, GINT_TO_POINTER(IMG_ST_RUN));				
			image_status = IMG_ST_RUN;
		}
	}
	else if (image_status == IMG_ST_STOP)
	{
		if ((status == DR_STOP) || (status == DR_PAUSE) || (status == DR_BWD_NEXT_FRAME) || (status == DR_FWD_NEXT_FRAME))
			return;

		evt_send_to_local(INFY_PB_IMAGE_STATUS, 0, 0, GINT_TO_POINTER(IMG_ST_RUN));				
		image_status = IMG_ST_RUN;		
	}
	else if (image_status == IMG_ST_RUN)
	{
		if ((status == DR_STOP) || (status == DR_PAUSE) || (status == DR_BWD_NEXT_FRAME) || (status == DR_FWD_NEXT_FRAME))
		{
			evt_send_to_local(INFY_PB_IMAGE_STATUS, 0, 0, GINT_TO_POINTER(IMG_ST_STOP));
			image_status = IMG_ST_STOP;
		}
	}	
}

gboolean _vsm_playback_set_playstatus(gpointer data)
{
	VSM_T *ivsm;
	VOM_PLAY_STATUS status;
	GdkRectangle area;
	gint i;

	ivsm = (VSM_T*)data;

	memset(&status, 0, sizeof(VOM_PLAY_STATUS));	
	_vom_playback_get_playstatus(&status);
	_update_playstatus(&status, ivsm);
	_send_playback_pos_log(&status, ivsm);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		_vvm_playback_set_video_status(i, status.play_status[i]);
    }
	
	_vvm_playback_set_dir_rate(ivsm->play_dir_rate);
	_vvm_playback_set_playtime(ivsm->play_time);
	_send_image_status(ivsm->play_dir_rate);
	_vsm_ack_play_cmd();
		
	return TRUE;
}

void _vsm_playback_reset_playstatus(gpointer data)
{
	VSM_T *ivsm;

	ivsm = (VSM_T*)data;
	
	memset(&ivsm->play_time, 0, sizeof(GTimeVal));
	ivsm->play_dir_rate = DR_NONE;

	memset(&ivsm->pos_time, 0, sizeof(GTimeVal));

    if (ivsm->log_ctx) scm_close_log_ctx(ivsm->log_ctx);
    ivsm->log_ctx = 0;

	image_status = IMG_ST_INIT;

	_vvm_playback_reset_playstatus();
}

gboolean _vsm_preview_set_playtime(gpointer data)
{
	VSM_T *ivsm;
	VOM_PLAY_STATUS status;

	ivsm = (VSM_T*)data;

	memset(&status, 0, sizeof(VOM_PLAY_STATUS));	
	_vom_playback_get_playstatus(&status);

	if(status.play_time.tv_sec != 0) 
		ivsm->preview_time = status.play_time;

	return TRUE;
}

void _vsm_preview_reset_playtime(gpointer data)
{
	VSM_T *ivsm;

	ivsm = (VSM_T*)data;
	memset(&ivsm->preview_time, 0, sizeof(GTimeVal));
}

gboolean _vsm_smart_set_playtime(gpointer data)
{
	VSM_T *ivsm = (VSM_T *)data;
	VOM_PLAY_STATUS status;

	memset(&status, 0, sizeof(VOM_PLAY_STATUS));
	_vom_playback_get_smart_playstatus(&status);

	if ( status.play_time.tv_sec != 0 )
		ivsm->smart_time = status.play_time;
	ivsm->smart_dir_rate = status.is_play_stop ? DR_STOP : DR_NONE;
	return TRUE;
}

void _vsm_smart_reset_playtime(gpointer data)
{
	VSM_T *ivsm = (VSM_T *)data;

	ivsm->smart_dir_rate = DR_NONE;
	memset(&ivsm->smart_time, 0, sizeof(GTimeVal));
}

static gboolean _playback_step_backward(gpointer data)
{
	gint step_time;

	step_time = MIN_STEP_TIME * (step_cnt/3);
	_vom_playback_cmd_step_backward(step_time);
	step_cnt++;

	return TRUE;
}

static gboolean _playback_step_forward(gpointer data)
{
	gint step_time;

	step_time = MIN_STEP_TIME * (step_cnt/3);
	_vom_playback_cmd_step_forward(step_time);
	step_cnt++;	

	return TRUE;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vsm_playback_change_dir_rate(PLAYBACK_DIR_E dir)
{
	if (dir == DIR_DS_BWD)
		_vom_playback_cmd_fast_backward();
	else if (dir == DIR_BWD)
		_vom_playback_cmd_backward();
	else if (dir == DIR_PAUSE)
		_vom_playback_cmd_pause();
	else if (dir == DIR_FWD)
		_vom_playback_cmd_forward();
	else if (dir == DIR_DS_FWD)
		_vom_playback_cmd_fast_forward();

	return 0;
}

void vsm_playback_step_backward()
{
	_vom_playback_cmd_step_backward(MIN_STEP_TIME);
}

void vsm_playback_step_forward()
{
	_vom_playback_cmd_step_forward(MIN_STEP_TIME);
}

void vsm_playback_run_step_backward()
{
	if (step_tid)
	{
		g_source_remove(step_tid);
		step_tid = 0;	
		step_cnt = 0;

		_vom_playback_cmd_step_backward(MIN_STEP_TIME);
		step_tid = g_timeout_add(500, _playback_step_backward, NULL);
	}
	else
	{
		_vom_playback_cmd_step_backward(MIN_STEP_TIME);
		step_tid = g_timeout_add(500, _playback_step_backward, NULL);
	}
}

void vsm_playback_stop_step_backward()
{
	if (step_tid)
	{
		g_source_remove(step_tid);
		step_tid = 0;
		step_cnt = 0;		
	}
}

void vsm_playback_run_step_forward()
{
	if (step_tid)
	{
		g_source_remove(step_tid);
		step_tid = 0;	
		step_cnt = 0;

		_vom_playback_cmd_step_forward(MIN_STEP_TIME);
		step_tid = g_timeout_add(500, _playback_step_forward, NULL);
	}
	else
	{
		_vom_playback_cmd_step_forward(MIN_STEP_TIME);
		step_tid = g_timeout_add(500, _playback_step_forward, NULL);
	}
}

void vsm_playback_stop_step_forward()
{
	if (step_tid)
	{
		g_source_remove(step_tid);
		step_tid = 0;
		step_cnt = 0;
	}
}

