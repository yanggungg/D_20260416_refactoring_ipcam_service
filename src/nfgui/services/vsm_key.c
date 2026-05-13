/*
 * vsm_key.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Aug 9, 2011
 *
 */

#include "nf_afx.h"

#include "../support/color.h"
#include "../support/util.h"
#include "modules/ssm.h"

#include "cmm.h"
#include "scm.h"

#include "ix_mem.h"

#include "vsm.h"
#include "vsm_internal.h"

static SHUTTLEID g_stl_last_id = SHUTTLE_CENTER;
static gint zig_ipcam_hori_step = 1;

////////////////////////////////////////////////////////////
//
// protected interfaces
//

static gboolean _check_active_sfc()
{

	return TRUE;	
}

static gboolean _change_channel(guint key_id)
{
	switch (key_id)
	{
		case KEYPAD_CH01:	vsm_change_sfc_by_ch(0);	break;		
		case KEYPAD_CH02:	vsm_change_sfc_by_ch(1);	break;		
		case KEYPAD_CH03:	vsm_change_sfc_by_ch(2);	break;		
		case KEYPAD_CH04:	vsm_change_sfc_by_ch(3);	break;		
		case KEYPAD_CH05:	vsm_change_sfc_by_ch(4);	break;		
		case KEYPAD_CH06:	vsm_change_sfc_by_ch(5);	break;		
		case KEYPAD_CH07:	vsm_change_sfc_by_ch(6);	break;		
		case KEYPAD_CH08:	vsm_change_sfc_by_ch(7);	break;		
		case KEYPAD_CH09:	vsm_change_sfc_by_ch(8);	break;		
		case KEYPAD_CH10:	vsm_change_sfc_by_ch(9);	break;		
		case KEYPAD_CH11:	vsm_change_sfc_by_ch(10);	break;		
		case KEYPAD_CH12:	vsm_change_sfc_by_ch(11);	break;		
		case KEYPAD_CH13:	vsm_change_sfc_by_ch(12);	break;		
		case KEYPAD_CH14:	vsm_change_sfc_by_ch(13);	break;		
		case KEYPAD_CH15:	vsm_change_sfc_by_ch(14);	break;		
		case KEYPAD_CH16:	vsm_change_sfc_by_ch(15);	break;
		case KEYPAD_CH17:	vsm_change_sfc_by_ch(16);	break;		
		case KEYPAD_CH18:	vsm_change_sfc_by_ch(17);	break;		
		case KEYPAD_CH19:	vsm_change_sfc_by_ch(18);	break;		
		case KEYPAD_CH20:	vsm_change_sfc_by_ch(19);	break;		
		case KEYPAD_CH21:	vsm_change_sfc_by_ch(20);	break;		
		case KEYPAD_CH22:	vsm_change_sfc_by_ch(21);	break;		
		case KEYPAD_CH23:	vsm_change_sfc_by_ch(22);	break;		
		case KEYPAD_CH24:	vsm_change_sfc_by_ch(23);	break;		
		case KEYPAD_CH25:	vsm_change_sfc_by_ch(24);	break;		
		case KEYPAD_CH26:	vsm_change_sfc_by_ch(25);	break;		
		case KEYPAD_CH27:	vsm_change_sfc_by_ch(26);	break;		
		case KEYPAD_CH28:	vsm_change_sfc_by_ch(27);	break;		
		case KEYPAD_CH29:	vsm_change_sfc_by_ch(28);	break;		
		case KEYPAD_CH30:	vsm_change_sfc_by_ch(29);	break;		
		case KEYPAD_CH31:	vsm_change_sfc_by_ch(30);	break;		
		case KEYPAD_CH32:	vsm_change_sfc_by_ch(31);	break;		
		default:	break;		
	}

	return TRUE;
}

static gboolean 
_get_item_next_sfc(gchar pre_ch, VSM_DIV_E pre_div, gchar *post_ch, VSM_DIV_E *post_div)
{
	gchar tmp;
	
	switch (pre_div)
	{
		case VSM_DIV1:
		{
			*post_ch = 0;
			*post_div = DEFAULT_DIV_MODE;
		}
		break;
		case VSM_DIV4:
		{
			if ((pre_ch >= 0) && (pre_ch < 4))
			{
				*post_ch = 4;
				*post_div = VSM_DIV4;
			}
			else if ((pre_ch >= 4) && (pre_ch < 8))
			{
				*post_ch = 8;
				*post_div = VSM_DIV4;
			}
			else if ((pre_ch >= 8) && (pre_ch < 12))
			{
				*post_ch = 12;
				*post_div = VSM_DIV4;
			}
			else
			{
				*post_ch = 0;
				*post_div = VSM_DIV1;
			}
		
			if ((pre_div == DEFAULT_DIV_MODE) || (*post_ch + 4 > GUI_CHANNEL_CNT))
            {
				*post_ch = 0;
				*post_div = VSM_DIV1;
            }		
		}
		break;
		case VSM_DIV6:
		case VSM_DIV8:		
		{
			*post_ch = 0;
			*post_div = VSM_DIV4;
		}	
		break;
		case VSM_DIV9:
		{
			if (pre_ch == 0)
			{
				*post_ch = 7;
				*post_div = VSM_DIV9;
			}
			else
			{
				*post_ch = 0;
				*post_div = VSM_DIV4;
			}
		
			if (pre_div == DEFAULT_DIV_MODE)
            {
				*post_ch = 0;
				*post_div = VSM_DIV4;
            }		
		}		
		break;
		case VSM_DIV16:
		{
			*post_ch = 0;
			*post_div = VSM_DIV9;
		}		
		break;
	}
	
	return FALSE;	
}

static gboolean _change_div()
{
	VSM_DIV_E pre_div, post_div;
	gchar first_ch, next_ch, i;
    gchar win_id[VSM_WIN_MAX];
#if defined(GUI_8CH_SUPPORT)
	gint div_cnt[] = {1, 4, 0, 0, 8, 0};
#else
	gint div_cnt[] = {1, 4, 0, 0, 9, 16};
#endif

	memset(win_id, 0xff, sizeof(win_id));
	pre_div = vsm_get_div();
	first_ch = _vsm_get_ch(VSM_WIN_ID1);	
	_get_item_next_sfc(first_ch, pre_div, &next_ch, &post_div);

	for (i = 0; i < div_cnt[post_div]; i++)
	{
		win_id[next_ch] = i;
		next_ch++;
	}

	vsm_change_sfc_by_array(win_id, post_div);

	return TRUE;	
}







////////////////////////////////////////////////////////////
//
// public interfaces
//

void vsm_keypad_event(KEYPAD_KID key_id)
{
	switch (key_id)
	{
		case KEYPAD_CH01:	case KEYPAD_CH02:	case KEYPAD_CH03:	case KEYPAD_CH04:
		case KEYPAD_CH05:	case KEYPAD_CH06:	case KEYPAD_CH07:	case KEYPAD_CH08:
		case KEYPAD_CH09:	case KEYPAD_CH10:	case KEYPAD_CH11:	case KEYPAD_CH12:
		case KEYPAD_CH13:	case KEYPAD_CH14:	case KEYPAD_CH15:	case KEYPAD_CH16:
		case KEYPAD_CH17:	case KEYPAD_CH18:	case KEYPAD_CH19:	case KEYPAD_CH20:
		case KEYPAD_CH21:	case KEYPAD_CH22:	case KEYPAD_CH23:	case KEYPAD_CH24:
		case KEYPAD_CH25:	case KEYPAD_CH26:	case KEYPAD_CH27:	case KEYPAD_CH28:
		case KEYPAD_CH29:	case KEYPAD_CH30:	case KEYPAD_CH31:	case KEYPAD_CH32:
			if (key_id >= KEYPAD_CH17) key_id -= 27;
			vsm_change_sfc_cstlayout_by_ch(key_id);
		    zig_ipcam_hori_step = 1;
		break;
		case KEYPAD_DISP:
			vsm_change_sfc_cstlayout_by_cdiv();
		    zig_ipcam_hori_step = 1;			
		break;
/*
		case KEYPAD_DISP1:
			vsm_change_sfc_cstlayout_next(VSM_DIV1);
		break;	
		case KEYPAD_DISP4:
			vsm_change_sfc_cstlayout_next(VSM_DIV4);			
		break;			
		case KEYPAD_DISP9:
			vsm_change_sfc_cstlayout_next(VSM_DIV9);			
		break;			
		case KEYPAD_DISP16:
			vsm_change_sfc_cstlayout_next(VSM_DIV16);
		break;	
*/
		case KEYPAD_REW:
		break;		
		case KEYPAD_BACKWARD:
		break;		
		case KEYPAD_PAUSE:
		break;		
		case KEYPAD_FORWARD:
		break;		
		case KEYPAD_FF:
		break;		
		case KEYPAD_HOLD:
			if(g_stl_last_id != SHUTTLE_CENTER)
				_vom_playback_set_hold(1);
		break;		
		case KEYPAD_FREEZE:
		break;		
		default:
		break;		
	}
}

void vsm_jog_event(JOGID jog_id)
{
	switch (jog_id)
	{
		case JOG_CW:
			_vom_playback_cmd_nextframe_forward();
		break;
		case JOG_CCW:
			_vom_playback_cmd_nextframe_backward();
		break;
		default:
		break;		
	}
}

void vsm_shuttle_init()
{
	g_stl_last_id = SHUTTLE_CENTER;
	}

void vsm_shuttle_event(SHUTTLEID shuttle_id)
	{
	g_stl_last_id = shuttle_id;

	if(shuttle_id != SHUTTLE_CENTER && _vom_playback_get_hold())
			return;

	switch (shuttle_id)
	{
		case SHUTTLE_CENTER:
			{
			if (_vom_playback_get_hold())
				_vom_playback_set_hold(0);
			else
				_vom_playback_cmd_force_pause();
			}
		break;		
		case SHUTTLE_CW_1:
			_vom_playback_cmd_force_forward_01();
		break;
		case SHUTTLE_CW_2:
			_vom_playback_cmd_force_forward_02();
		break;
		case SHUTTLE_CW_3:
			_vom_playback_cmd_force_forward_04();
		break;
		case SHUTTLE_CW_4:
			_vom_playback_cmd_force_forward_08();		
		break;
		case SHUTTLE_CW_5:
			_vom_playback_cmd_force_forward_16();		
		break;
		case SHUTTLE_CW_6:
			_vom_playback_cmd_force_forward_32();		
		break;
		case SHUTTLE_CW_7:
			_vom_playback_cmd_force_forward_64();		
		break;
		case SHUTTLE_CCW_1:
			_vom_playback_cmd_force_backward_01();				
		break;
		case SHUTTLE_CCW_2:
			_vom_playback_cmd_force_backward_02();				
		break;
		case SHUTTLE_CCW_3:
			_vom_playback_cmd_force_backward_04();
		break;
		case SHUTTLE_CCW_4:
			_vom_playback_cmd_force_backward_08();						
		break;
		case SHUTTLE_CCW_5:
			_vom_playback_cmd_force_backward_16();						
		break;
		case SHUTTLE_CCW_6:
			_vom_playback_cmd_force_backward_32();						
		break;
		case SHUTTLE_CCW_7:
			_vom_playback_cmd_force_backward_64();						
		break;
		case SHUTTLE_CCW_8:
			_vom_playback_cmd_force_backward_64();						
		break;
		default:
		break;
	}
}

void vsm_ipcam_zig_event_press(KEYPAD_KID key_id)
{
    gint i;

    g_message("%s, %d", __FUNCTION__, __LINE__);

	switch (key_id)
	{
    	case RMC_SNAPSHOT:
	        scm_req_ipcam_mf_info();
		break;	
    	case KEYPAD_FREEZE:
    	    scm_start_ipcam_scan();
		break;		
		case KEYPAD_LOCK:
    	    scm_sync_ipcam_time_info();		
		break;		
		case RMC_AUDIO:
            nftool_mbox_auto(NF_TOPWND, 10, "NOTICE", "Please wait...");
    	    scm_work_ipcam_done();	
		break;		
		case RMC_ZOOMOUT:
            nf_ipcam_zig_zoom_wide();
		break;		
		case RMC_ZOOMIN:
            nf_ipcam_zig_zoom_tele();		
		break;		
		case RMC_NEAR:
            nf_ipcam_zig_focus_near();
    	break;		
		case RMC_FAR:
            nf_ipcam_zig_focus_far();
		break;	
		case RMC_PRESET:
            nf_ipcam_zig_origin();
		break;		
		case RMC_AFOCUS:
            nf_ipcam_zig_onepush();
		break;		
		case KEYPAD_ZOOM:
            nf_ipcam_zig_iris_open();
		break;		
		case KEYPAD_PTZ:
            nf_ipcam_zig_iris_close();
		break;		
        case KEYPAD_UP:
            if(vsm_get_div() == VSM_DIV1) 
            {
                i = (gint)vsm_get_focused_channel();
                nf_ipcam_zig_up(i);

                g_message("%s, %d, ch:%d", __FUNCTION__, __LINE__, i);
            }
        break;
        case KEYPAD_DOWN:
            if(vsm_get_div() == VSM_DIV1) 
            {
                i = (gint)vsm_get_focused_channel();
                nf_ipcam_zig_down(i);
            }        
        break;
        case KEYPAD_LEFT:
            if(vsm_get_div() == VSM_DIV1) 
            {
                i = (gint)vsm_get_focused_channel();
                nf_ipcam_zig_left(i, zig_ipcam_hori_step);
                zig_ipcam_hori_step = 1;
            }        
        break;
        case KEYPAD_RIGHT:
            if(vsm_get_div() == VSM_DIV1) 
            {
                i = (gint)vsm_get_focused_channel();
                nf_ipcam_zig_right(i, zig_ipcam_hori_step);
                zig_ipcam_hori_step = 1;                
            }        
        break;	
		case KEYPAD_CH01:	
		    zig_ipcam_hori_step = 1;
        break;	
		case KEYPAD_CH02:	
		    zig_ipcam_hori_step = 2;
        break;	
		case KEYPAD_CH03:	
		    zig_ipcam_hori_step = 3;
        break;	
		case KEYPAD_CH04:
		    zig_ipcam_hori_step = 4;
        break;			
		case KEYPAD_CH05:	
		    zig_ipcam_hori_step = 5;
        break;	
		case KEYPAD_CH06:	
		    zig_ipcam_hori_step = 6;
        break;	
		case KEYPAD_CH07:	
		    zig_ipcam_hori_step = 7;
        break;	
		case KEYPAD_CH08:
		    zig_ipcam_hori_step = 8;
        break;			
		case KEYPAD_CH09:	
		    zig_ipcam_hori_step = 9;
        break;	    
		case KEYPAD_DISP:
			_change_div();
            zig_ipcam_hori_step = 1;			
		break;		        
		default:
		break;		
	}
}

void vsm_ipcam_zig_event_release(KEYPAD_KID key_id)
{
    g_message("%s, %d", __FUNCTION__, __LINE__);

}

