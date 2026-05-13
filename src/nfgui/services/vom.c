/*
 * vom.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */
#include "support/util.h"
#include "nf_api_live.h"
#include "nf_api_play.h"

#include "vsm.h"
#include "vsm_internal.h"
#include "dtf.h"



#define DBG_LEVEL		9
#define DBG_MODULE		"VOM"


////////////////////////////////////////////////////////////
//
// protected data type 
//

typedef struct PB_INFO {
	gpointer 		*handle;		//playback handle
	NF_PLAY_PARAM 	param;			//playback param

	time_t			play_time;
	time_t			time_offset;
	guint 			stl_hold;
	guint 			stl_oper;
	PB_TYPE_E		type;
} PB_INFO;

typedef struct RES {
	guint w;
	guint h;
} RES_T;

typedef struct _VOM {
	PB_INFO		pi;

//	RES_T		lv_res;
//	RES_T		pb_res;
} VOM;


////////////////////////////////////////////////////////////
//
// private variable
//

static VOM	ivom;


static void _vom_cmd_video_play(void);
static void _prvPlayRateUp(void);
static void _prvPlayRateDown(void);
static void _print_play_param(NF_PLAY_PARAM *param, guint mask);



////////////////////////////////////////////////////////////
//
// private functions
//

static NF_DISPLAY_E _div_trans_dispmode(VSM_DIV_E dtype)
{
	NF_DISPLAY_E disp_mode = NF_DISPLAY_FULL;

	switch (dtype)
	{
		case VSM_DIV1:
			disp_mode = NF_DISPLAY_FULL;
		break;
		case VSM_DIV4:
			disp_mode = NF_DISPLAY_QUAD;
		break;
		case VSM_DIV6:
			disp_mode = NF_DISPLAY_HEXA_A;
		break;
		case VSM_DIV8:
			disp_mode = NF_DISPLAY_OCTA_A;
		break;
		case VSM_DIV9:
			disp_mode = NF_DISPLAY_NONA;
		break;
		case VSM_DIV16:
			disp_mode = NF_DISPLAY_HEXADECA;
		break;
		case VSM_DIV36:
			disp_mode = NF_DISPLAY_HEXATRICONTA;
		break;		
		default:
			disp_mode = NF_DISP_DEFAULT_MODE;
		break;
	}

	return disp_mode;
}


////////////////////////////////////////////////////////////
//
// 		init functions
//

void _vom_init()
{
	DMSG(9, "");
    scm_turnon_live_audio();
}

////////////////////////////////////////////////////////////
//
// 		live functions
//

static gint _set_lv_audio_channel(SFC_T *psfc)
{
    gint i;

    if (psfc->div != VSM_DIV1) return -1;
	if (var_get_full_scr_audio() == ONOFF_OFF) return -1;

    for (i = 0; i < 32; i++)
    {
		if (psfc->cinfo[i].win_id != -1) break;
    }

   	scm_change_live_audio(i);		

    return 0;
}

void _vom_live_start_video(SFC_T *psfc, LV_TYPE_E type)
{
    guint i;
	gchar ch_arr[32];
    gboolean covert_arr[32];
	NF_DISPLAY_E disp_mode;

	DMSG(9, "x:%d, y:%d, w:%d, h:%d", psfc->x, psfc->y, psfc->w, psfc->h);
	for(i = 0; i < 32; i++)
    {
		ch_arr[i] = psfc->cinfo[i].win_id;
        covert_arr[i] = psfc->cinfo[i].covert;
    }

	disp_mode = _div_trans_dispmode(psfc->div);

	nf_live_start(disp_mode, psfc->x, psfc->y, psfc->w, psfc->h,
					ch_arr, covert_arr, 0);				
}

void _vom_live_change_video(SFC_T *psfc)
{
    guint i;
	gchar ch_arr[32];
    gboolean covert_arr[32];
    NF_DISPLAY_E disp_mode;

	DMSG(9, "x:%d, y:%d, w:%d, h:%d", psfc->x, psfc->y, psfc->w, psfc->h);

	for(i = 0; i < 32; i++)
    {
		ch_arr[i] = psfc->cinfo[i].win_id;
        covert_arr[i] = psfc->cinfo[i].covert;
    }

	disp_mode = _div_trans_dispmode(psfc->div);

	nf_live_change(disp_mode, psfc->x, psfc->y, psfc->w, psfc->h, 
					ch_arr, covert_arr, 0);

    _set_lv_audio_channel(psfc);
}

void _vom_live_stop_video(void)
{
	DMSG(9, "");
	nf_live_stop();
}

////////////////////////////////////////////////////////////
//
// 		playback functions
//

static void _set_pb_param_common()
{
	ivom.pi.handle = NULL;
	ivom.pi.stl_hold = 0;
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_001;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_NORMAL;
	ivom.pi.param.audio_in_video_chan = -1;
}

static void _set_pb_audio_channel()
{
	gint ch;

	if(ivom.pi.param.vr_num == 1 &&
		ivom.pi.param.panorama_mode == NF_PLAY_PARAM_PANORAMA_NO &&
		ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START &&
		ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001 &&
		ivom.pi.param.direction == NF_PLAY_PARAM_DIR_FORWARD &&
		ivom.pi.param.direction == NF_PLAY_PARAM_SPEED_NORMAL)
	{
		for (ch = 0; ch < MAX_LOCAL_PLAY_CHAN; ch++)
		{
			if (ivom.pi.param.vr_index[ch] != -1) break;
		}
	
		ivom.pi.param.audio_in_video_chan = (gchar)ch;
	}
	else
		ivom.pi.param.audio_in_video_chan = -1;
}

static void _set_pb_param_sfc(SFC_T *psfc)
{
	guint ch;

    for(ch = 0; ch < MAX_LOCAL_PLAY_CHAN; ch++)
    {
        ivom.pi.param.vr_index[ch] = psfc->cinfo[ch].win_id;
        ivom.pi.param.vr_covert[ch] = psfc->cinfo[ch].covert;
    }

	switch(psfc->div)
	{
		case VSM_DIV1:
			ivom.pi.param.vr_num = 1;
		break;
		case VSM_DIV4:
			ivom.pi.param.vr_num = 4;
		break;
		case VSM_DIV9:
#if defined(GUI_16CH_SUPPORT) || defined(GUI_32CH_SUPPORT)
			ivom.pi.param.vr_num = 9;
#else
			ivom.pi.param.vr_num = 8;
#endif
		break;
		case VSM_DIV16:
			ivom.pi.param.vr_num = 16;
		break;
		case VSM_DIV36:
			ivom.pi.param.vr_num = 32;
		break;
		default:
			DMSG(1, "%s, %d, unsupported div mode", __FUNCTION__, __LINE__);
		break;
	}

    ivom.pi.param.disp_mode = _div_trans_dispmode(psfc->div);
	ivom.pi.param.win_xpos = psfc->x;
	ivom.pi.param.win_ypos = psfc->y;
	ivom.pi.param.win_width = psfc->w;
	ivom.pi.param.win_height = psfc->h;
}

static void _set_pb_param_time(GTimeVal tv)
{
	memset(&(ivom.pi.param.start_time), 0, sizeof(GTimeVal));
	memset(&(ivom.pi.param.end_time), 0, sizeof(GTimeVal));
	memset(&(ivom.pi.param.search_time), 0, sizeof(GTimeVal));
	
	ivom.pi.param.search_time = tv;
	ivom.pi.param.interval = 0;
}

static void _set_pb_param_archplay_time()
{
	memset(&(ivom.pi.param.start_time), 0, sizeof(GTimeVal));
	memset(&(ivom.pi.param.end_time), 0, sizeof(GTimeVal));
	memset(&(ivom.pi.param.search_time), 0, sizeof(GTimeVal));
	
	ivom.pi.param.search_time.tv_sec = ARCH_PLAY_TIME;
	ivom.pi.param.interval = 0;
}

static void _set_pb_param_type(PB_TYPE_E type)
{
	if (type == PB_PANO1)
		ivom.pi.param.panorama_mode = NF_PLAY_PARAM_PANORAMA_1;
	else if (type == PB_PANO2)
		ivom.pi.param.panorama_mode = NF_PLAY_PARAM_PANORAMA_2;
	else
		ivom.pi.param.panorama_mode = NF_PLAY_PARAM_PANORAMA_NO;

	ivom.pi.type = type;
}

void _vom_playback_start_video(SFC_T *psfc, GTimeVal tv, PB_TYPE_E type)
{
	DMSG(9, "");
	
    scm_turnoff_live_audio();	
    
	_set_pb_param_common();
	_set_pb_param_sfc(psfc);
	_set_pb_param_time(tv);
	_set_pb_param_type(type);
	if (type != PB_PREVIEW) _set_pb_audio_channel();

	ivom.pi.play_time = tv.tv_sec;
	ivom.pi.time_offset = 0;
	nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);
}

void _vom_playback_start_smart_video(SFC_T *psfc, GTimeVal tv_start,GTimeVal tv_end, NF_PLAY_SMART_SEARCH_MODE search_mode)
{
	_set_pb_param_common();
	_set_pb_param_sfc(psfc);
	_set_pb_param_time(tv_start);
	_set_pb_param_type(PB_NORMAL);

	//captainnn test
	ivom.pi.param.end_time = tv_end;

	ivom.pi.param.disp_mode = NF_DISPLAY_PLAYBACK_SMART;
	ivom.pi.play_time = tv_start.tv_sec;
	ivom.pi.time_offset = 0;
	
	// nf_play_smart_start((gpointer)&ivom.pi.handle, &ivom.pi.param,
	// 		search_mode, NULL);
}

void _vom_playback_pause_smart_video(NF_PLAY_SMART_SEARCH_MODE search_mode, gboolean pause)
{
	// nf_play_smart_pause(ivom.pi.handle, search_mode, pause);
}

void _vom_playback_stop_smart_video(NF_PLAY_SMART_SEARCH_MODE search_mode)
{
	// nf_play_smart_stop(ivom.pi.handle, search_mode);
}

void _vom_preserve_playback_start_video(SFC_T *psfc, GTimeVal tv, PB_TYPE_E type, guint sess_id)
{
	DMSG(9, "");
	
	nf_disk_preserve_pb_mode(1, sess_id);
    scm_turnoff_live_audio();

	_set_pb_param_common();
	_set_pb_param_sfc(psfc);
	_set_pb_param_time(tv);
	_set_pb_param_type(type);
	if (type != PB_PREVIEW) _set_pb_audio_channel();

	ivom.pi.play_time = tv.tv_sec;
	ivom.pi.time_offset = 0;
	nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);
}

void _vom_playback_start_archived_video(SFC_T *psfc, time_t offset, PB_TYPE_E type)
{
	DMSG(9, "");

    scm_turnoff_live_audio();

	_set_pb_param_common();
	_set_pb_param_sfc(psfc);
	_set_pb_param_archplay_time();
	_set_pb_param_type(type);
	_set_pb_audio_channel();

	ivom.pi.play_time = 0;
	ivom.pi.time_offset = offset;
	nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);
}

void _vom_playback_change_video(SFC_T *psfc)
{
	DMSG(9, "");

	_set_pb_param_sfc(psfc);

	//FIXME: KNOWN BUG!!
	//Current implementation: Pause->"Screen change" is Next frame. 
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_PAUSE 
			|| 	ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_NEXT_FRAME;
	}
	
	_vom_cmd_video_play();
}


void _vom_playback_stop_video(void)
{
	DMSG(9, "");
	nf_disk_preserve_pb_mode(0, 0);
	nf_play_stop(ivom.pi.handle);
    scm_turnon_live_audio();	
}	

void _vom_playback_restart_by_sfc(SFC_T *psfc)
{
    nf_play_stop(ivom.pi.handle);
    _set_pb_param_sfc(psfc);
    nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);    
}

void _vom_playback_get_playstatus(VOM_PLAY_STATUS *set_status)
{
	NF_PLAY_STATUS 	get_status;
	gint stop_cnt = 0;
	guint i = 0;
	gchar buf[30];

	memset(&get_status, 0, sizeof(NF_PLAY_STATUS));

	if(nf_play_get_status(ivom.pi.handle, &get_status, NULL )) {
	
		NFUTIL_THREADS_ENTER();

		if(get_status.play_time.tv_sec != 0)
		{
			set_status->play_time.tv_sec = get_status.play_time.tv_sec + ivom.pi.time_offset;			
			ivom.pi.play_time = get_status.play_time.tv_sec;

#if 0
			memset(buf, 0x00, sizeof(buf));				
			dtf_get_local_datetime(ivom.pi.play_time, buf);
			g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);			
#endif			
		}
		
		for(i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			set_status->play_status[i] = get_status.play_status[i];
        }

		for(i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (ivom.pi.param.vr_index[i] != -1)
			{
				if((get_status.play_status[i] == NF_PLAY_STATUS_ENDVIDEO) 
						|| (get_status.play_status[i] == NF_PLAY_STATUS_OVERLAPPED))
				{
					stop_cnt++;
				}				
			}
		}

		if(stop_cnt == ivom.pi.param.vr_num)
		{			
			set_status->is_play_stop = TRUE;
		}
		else
		{
			set_status->play_mode = ivom.pi.param.play_mode;
			set_status->play_dir = ivom.pi.param.direction;
			set_status->play_rate = ivom.pi.param.play_rate;
			set_status->speed_flag = ivom.pi.param.speed_flag;
		}

		NFUTIL_THREADS_LEAVE();
	}
	else 
		g_warning(">>>>>>> %s returns FALSE\n", "nf_play_get_status");

	return TRUE;
}

void _vom_playback_get_smart_playstatus(VOM_PLAY_STATUS *set_status)
{
	NF_PLAY_STATUS get_status;

	memset(&get_status, 0, sizeof(get_status));

	// if ( !nf_play_smart_get_status(ivom.pi.handle, &get_status, NULL) ) {
	// 	g_warning("%s returns FALSE", "nf_play_smart_get_status");
	// 	return;
	// }

	NFUTIL_THREADS_ENTER();
	if ( get_status.play_time.tv_sec ) {
		set_status->play_time.tv_sec = get_status.play_time.tv_sec +
				ivom.pi.time_offset;
	}

	set_status->play_status[0] = get_status.play_status[0];
	if ( get_status.play_status[0] == NF_PLAY_STATUS_ENDVIDEO ||
			get_status.play_status[0] == NF_PLAY_STATUS_OVERLAPPED ) {
		set_status->is_play_stop = TRUE;
	}
	NFUTIL_THREADS_LEAVE();
}

/*******************************************************************************************
						change playback mode attr.
********************************************************************************************/

static void _vom_cmd_video_play(void)
{
	gchar buf[30];

#if 0
	memset(buf, 0x00, sizeof(buf));		
	dtf_get_local_datetime(ivom.pi.play_time, buf);
	g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);	
#endif

	ivom.pi.param.search_time.tv_sec = ivom.pi.play_time;
	_set_pb_audio_channel();
	_print_play_param(&ivom.pi.param, 0xFFFF);
	nf_play_change(ivom.pi.handle, &ivom.pi.param, NULL);
}

static void _prvPlayRateUp()
{
	guchar ucPlayRate;
	
	ucPlayRate = ivom.pi.param.play_rate;

	if (ucPlayRate >= NF_PLAY_PARAM_RATE_064) return;

	ucPlayRate <<= 1;
	ivom.pi.param.play_rate = ucPlayRate;
}

static void _prvPlayRateDown()
{
	guchar ucPlayRate;

	ucPlayRate = ivom.pi.param.play_rate;

	if (ucPlayRate <= NF_PLAY_PARAM_RATE_001) return;

	ucPlayRate >>= 1;
	ivom.pi.param.play_rate = ucPlayRate;
}

void _vom_playback_cmd_step_backward(gint step_time)
{
	gchar buf[30];

#if 0
	memset(buf, 0x00, sizeof(buf));
	dtf_get_local_datetime(ivom.pi.play_time, buf);
	g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);	
#endif

	ivom.pi.param.search_time.tv_sec = ivom.pi.play_time - step_time;
	nf_play_stop(ivom.pi.handle);
	nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);
}

void _vom_playback_cmd_fast_backward(void)
{
	gint max_rate;

	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
		max_rate = NF_PLAY_PARAM_RATE_016;
	else
		max_rate = NF_PLAY_PARAM_RATE_064;		

	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START)
	{
		if(ivom.pi.param.direction == NF_PLAY_PARAM_DIR_FORWARD)
		{
			if( ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001 )
			{
				DMSG(1, "*** vom_playback_cmd_speed_down skip");
				return ;
			}
		}
		else if( ivom.pi.param.direction == NF_PLAY_PARAM_DIR_BACKWARD && ivom.pi.param.play_rate == max_rate )
		{
			DMSG(1, "*** [x64] vom_playback_cmd_speed_down skip"); 
			return ;
		}
	}
  
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_PAUSE || ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
	{	// PREV FRAME..
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_NEXT_FRAME;
		ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	}
	else if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START)
	{	// Change speed..
		if(ivom.pi.param.direction == NF_PLAY_PARAM_DIR_FORWARD)
		{
			if(ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001
				&& ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_NORMAL)
			{
				ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_SLOW;
				_prvPlayRateUp();
			}
			else if(ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_NORMAL)
			{
				_prvPlayRateDown();
			}
			else //Slow, Not RATE001
			{
				_prvPlayRateUp();
			}
        }
		else
        {
			if(ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001
				&& ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_SLOW)
			{
               ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_NORMAL;
			    _prvPlayRateUp();
			}
			else if(ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_SLOW)
			{
				_prvPlayRateDown();
			}
			else
			{
			    _prvPlayRateUp();
			}
        }
	}
	else
	{
		DMSG(1, "[WARNING] GUI_DEGUG_MESSAGE %s %d\n", __FILE__, __LINE__);
		_print_play_param(&ivom.pi.param, 0xFFFF);

		return;
	}

	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_backward(void)
{
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START && ivom.pi.param.direction == NF_PLAY_PARAM_DIR_BACKWARD &&
			ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001 && ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_NORMAL )
	{
		DMSG(1, "No change, because setting is same old state.\n");
	}
	else
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
		ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_001;
		ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
		ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_NORMAL;

		_vom_cmd_video_play();
	}

	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_pause(void)
{
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_PAUSE || ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;    //CHECK!!!!! NEXT->PLAY ??????????
	}
	else if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START)
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_PAUSE;
	}
	else
	{
		DMSG(1, "[WARNING] GUI_DEGUG_MESSAGE %s %d\n", __FILE__, __LINE__);
		_print_play_param(&ivom.pi.param, 0xFFFF);

		return;
	}

	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_forward(void)
{
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START && ivom.pi.param.direction == NF_PLAY_PARAM_DIR_FORWARD &&
			ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001 && ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_NORMAL )
	{
		DMSG(1, "No change, because setting is same old state.\n");
	}
	else
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
		ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_001;
		ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
		ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_NORMAL;

		_vom_cmd_video_play();
	}

	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_fast_forward(void)
{
	gint max_rate;

	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
		max_rate = NF_PLAY_PARAM_RATE_016;
	else
		max_rate = NF_PLAY_PARAM_RATE_064;		

	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START)
	{
		if(ivom.pi.param.direction == NF_PLAY_PARAM_DIR_BACKWARD)
		{
			if( ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001 )
			{
				DMSG(1, "*** vom_playback_cmd_speed_up skip"); 
				return ;
			}
		}
		else if( ivom.pi.param.direction == NF_PLAY_PARAM_DIR_FORWARD && ivom.pi.param.play_rate == max_rate )
		{
			DMSG(1, "[x64] *** vom_playback_cmd_speed_up skip"); 
			return ;
		}
	}

  
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_PAUSE || ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_NEXT_FRAME;
		ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	}
	else if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START)
	{
		if(ivom.pi.param.direction == NF_PLAY_PARAM_DIR_FORWARD)
        {
           if(ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001
                   && ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_SLOW)
           {
               ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_NORMAL;
			    _prvPlayRateUp();
           }
           else if(ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_SLOW)
           {
		        _prvPlayRateDown();
           }
           else
           {
			    _prvPlayRateUp();
           }
        }
		else
        {
           if(ivom.pi.param.play_rate == NF_PLAY_PARAM_RATE_001
                   && ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_NORMAL)
           {
               ivom.pi.param.speed_flag = NF_PLAY_PARAM_SPEED_SLOW;
			   _prvPlayRateUp();
           }
           else if(ivom.pi.param.speed_flag == NF_PLAY_PARAM_SPEED_NORMAL)
           {
		        _prvPlayRateDown();
           }
           else //Slow, Not RATE001
           {
			   _prvPlayRateUp();
           }
        }
	}
	else
	{
		DMSG(1, "[WARNING] GUI_DEGUG_MESSAGE %s %d\n", __FILE__, __LINE__);
		_print_play_param(&ivom.pi.param, 0xFFFF);

		return;
	}

	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_step_forward(gint step_time)
{
	gchar buf[30];

#if 0
	memset(buf, 0x00, sizeof(buf));
	dtf_get_local_datetime(ivom.pi.play_time, buf);
	g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);
#endif

	ivom.pi.param.search_time.tv_sec = ivom.pi.play_time + step_time;
	nf_play_stop(ivom.pi.handle);
	nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);
}

void _vom_playback_cmd_nextframe_backward(void)
{
	if(ivom.pi.param.play_mode != NF_PLAY_PARAM_MODE_PAUSE && ivom.pi.param.play_mode != NF_PLAY_PARAM_MODE_NEXT_FRAME)
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_PAUSE;
	}
	else
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_NEXT_FRAME;
		ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	}

	_vom_cmd_video_play();
}

void _vom_playback_cmd_nextframe_forward(void)
{
	if(ivom.pi.param.play_mode != NF_PLAY_PARAM_MODE_PAUSE && ivom.pi.param.play_mode != NF_PLAY_PARAM_MODE_NEXT_FRAME)
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_PAUSE;
	}
	else
	{
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_NEXT_FRAME;
		ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	}

	_vom_cmd_video_play();
}

void _vom_playback_set_hold(guint status)
{
	ivom.pi.stl_hold = status;
}

guint _vom_playback_get_hold(void)
{
	return ivom.pi.stl_hold;
}

void _vom_playback_cmd_force_pause(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_PAUSE;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_64(void)
{
	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_064;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_32(void)
{
	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
		return;


	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_032;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_16(void)
{
//	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
//		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_016;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_08(void)
{
//	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
//		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_008;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_04(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_004;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_02(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_002;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_backward_01(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_BACKWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_001;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_64(void)
{
	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_064;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_32(void)
{
	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_032;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_16(void)
{
//	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
//		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_016;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_08(void)
{
//	if ((ivom.pi.type == PB_ARCH_MUL) || (ivom.pi.type == PB_ARCH_AVI))
//		return;

	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_008;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_04(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_004;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_02(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_002;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}

void _vom_playback_cmd_force_forward_01(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	ivom.pi.param.direction = NF_PLAY_PARAM_DIR_FORWARD;
	ivom.pi.param.play_rate = NF_PLAY_PARAM_RATE_001;
	
	_vom_cmd_video_play();
	ivom.pi.stl_hold = 0;
}



gboolean _vom_playback_play_pause(void)
{
	if(ivom.pi.param.play_mode == NF_PLAY_PARAM_MODE_START) {
		ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_PAUSE;
		_vom_cmd_video_play();

		return TRUE;
	}

	return FALSE;
}

void _vom_playback_play_start_after_pause(void)
{
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	_vom_cmd_video_play();	
}

void _vom_playback_play_start_after_pause_time(GTimeVal playtime)
{
	gchar buf[30];

#if 0
	memset(buf, 0x00, sizeof(buf));	
	
	dtf_get_local_datetime(ivom.pi.play_time, buf);
	g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);
	
	dtf_get_local_datetime(playtime.tv_sec, buf);
	g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);	
#endif

	ivom.pi.param.search_time.tv_sec = playtime.tv_sec;
	ivom.pi.param.play_mode = NF_PLAY_PARAM_MODE_START;
	
	nf_play_stop(ivom.pi.handle);
	nf_play_start((gpointer)&ivom.pi.handle, &ivom.pi.param, NULL);
}



//<--------------- DEBUG MSG.
static void _print_play_param(NF_PLAY_PARAM *param, guint mask)
{
#if 0
	DMSG(1, "############## GUI MESSAGE BEG ##################\n");
	
	if(mask & 1)		DMSG(1, "Start Time = %ld\n", param->start_time.tv_sec);
	if(mask & (1<<1))	DMSG(1, "Search Time = %ld / %ld [%s]\n", param->search_time.tv_sec, param->search_time.tv_usec, g_time_val_to_iso8601(&(param->search_time)));
	if(mask & (1<<2))	DMSG(1, "End Time = %ld\n", param->end_time.tv_sec);
	if(mask & (1<<3))	DMSG(1, "interval = %d\n", param->interval);
	if(mask & (1<<4))	DMSG(1, "Video Object Count = %d\n", param->video_object_cnt);;
	if(mask & (1<<6))	DMSG(1, "Audio in Video Channel = %d\n", param->audio_in_video_chan);
	if(mask & (1<<8))	DMSG(1, "PlayMode = %d\n", param->play_mode);
	if(mask & (1<<9))	DMSG(1, "PlayRate = %d\n", (guchar)param->play_rate);
	if(mask & (1<<10))	DMSG(1, "Direction = %d\n", param->direction);
	if(mask & (1<<11))	DMSG(1, "SlowFlag = %d\n", param->slow_flag);
	if(mask & (1<<12))	DMSG(1, "PANO MODE = %d\n", param->panorama_mode);

	g_printf("############## GUI MESSAGE END ##################\n");
	
	g_print("\n\n");
#endif
}

