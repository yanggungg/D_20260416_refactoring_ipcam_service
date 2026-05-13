
#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/color.h"
#include "../support/util.h"
#include "../support/multi_language_support.h"

#include "../tools/nf_ui_tool.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfimage.h"
#include "../viewers/objects/nftable.h"

#include "vw_live_net_ipcam_popup.h"
#include "scm.h"


#define IPCAM_WIN_W			(400)
#define IPCAM_WIN_H			(250)

enum {
	INFO_ROW1 = 0,
	INFO_ROW2,
	INFO_ROW3,	
	INFO_ROW4,	
	INFO_ROW5,	
	INFO_ROW6,		
	INFO_ROW7,
	INFO_ROW8,	
	INFO_ROW9,	
	INFO_ROW10,	
	INFO_ROW11,	
	INFO_ROW12,	
	INFO_ROW13,	
	INFO_ROW14,	
	INFO_ROW15,	
	INFO_ROW16,	
	INFO_ROW17,	
	INFO_ROW18,	
	INFO_ROW19,	
	INFO_ROW20,		
	NUM_INFO
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *ipcam_win = NULL;
static NFOBJECT *ipcam_fixed;
static NFOBJECT *info_obj[NUM_INFO];

static gint _set_ipcam_success_info(guint ch)
{
	CameraData camdata;
	CAM_PROFILE_T profile;
	NFIPCamStatusInfo info;

    gint i, row = 0;

	gint ret_val;
	gchar strBuf[STRING_SIZE_64];

	DAL_get_camera_data(&camdata, ch);
	scm_get_cam_profile(ch, &profile);
    scm_get_ipcam_status(ch, &info);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %s", lookup_string("CAMERA NAME"), camdata.title);
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));
    nfui_nfobject_show(info_obj[row]);   	
    row += 1;

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %s", lookup_string("IP ADDRESS"), profile.conf.ipaddr);
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));
    nfui_nfobject_show(info_obj[row]);   	
    row += 1;
    
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %s", lookup_string("MODEL"), profile.model.name);
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	
    row += 1;

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %02x:%02x:%02x:%02x:%02x:%02x", lookup_string("MAC ADDRESS"),
					profile.model.mac[0], profile.model.mac[1], profile.model.mac[2],
					profile.model.mac[3], profile.model.mac[4], profile.model.mac[5]);	
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	   	
    row += 1;

    if (info.status == NF_IPCAM_STATUS_WARN_FW_VERSION)
    {
       	memset(strBuf, 0x00, sizeof(strBuf));
       	g_sprintf(strBuf, "%s : %s", lookup_string("F/W VERSION"), profile.model.swver);
    	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
    	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(399));
        nfui_nfobject_show(info_obj[row]);   	    	
        row += 1;

    	g_sprintf(strBuf, "(%s)", "INCOMPATIBLE F/W VERSION");	
    	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);                
    	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(399));
        nfui_nfobject_show(info_obj[row]);   	    	
        row += 1;    	
    }
    else
    {
       	memset(strBuf, 0x00, sizeof(strBuf));
       	g_sprintf(strBuf, "%s : %s", lookup_string("F/W VERSION"), profile.model.swver);
    	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
    	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));
        nfui_nfobject_show(info_obj[row]);   	    	 	
        row += 1;
    }
	
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %d", lookup_string("ALARM INPUT"), profile.conf.alarm_in);	
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	    	   	
    row += 1;
    
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %d", lookup_string("ALARM OUT"), profile.conf.alarm_out);	
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	    	   	
    row += 1;
    
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %d", lookup_string("VIDEO"), profile.conf.video);	
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);	
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	    	   	   	
    row += 1;

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %d", lookup_string("AUDIO"), profile.conf.audio_in);	
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);	
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	    	   	   	
    row += 1;

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%s : %d", lookup_string("AUDIO OUTPUT"), profile.conf.audio_out);	
	nfui_nflabel_set_text((NFLABEL*)info_obj[row], strBuf);	
   	nfui_nflabel_modify_fg((NFLABEL*)info_obj[row], COLOR_IDX(407));	
    nfui_nfobject_show(info_obj[row]);   	    	   	   	
    row += 1;

    for (i = row; i < NUM_INFO; i++)
        nfui_nfobject_hide(info_obj[i]);

	return row;
}

static gboolean ipcam_win_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_DELETE) {
		g_curwnd = 0;
	}

	return FALSE;
}

void VW_Live_Net_IPCam_Popup_Open(NFWINDOW *parent)
{
	gint i;
	gint pos_x, pos_y;	

	ipcam_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)0, (guint)0, (guint)IPCAM_WIN_W, (guint)IPCAM_WIN_H);
	g_curwnd = ipcam_win;
	nfui_nfobject_modify_bg(ipcam_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(405));
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)ipcam_win)->main_widget), FALSE);
	gtk_window_set_resizable(((NFWINDOW*)ipcam_win)->main_widget, TRUE);	
	nfui_regi_post_event_callback(ipcam_win, ipcam_win_event_cb);


	/* fixed */
	ipcam_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(ipcam_fixed, IPCAM_WIN_W, IPCAM_WIN_H);
	nfui_nfobject_show(ipcam_fixed);

	pos_x = 4;
	pos_y = 6;

	for (i = 0; i < NUM_INFO; i++)
	{
		info_obj[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(407));
		nfui_nflabel_set_align((NFLABEL*)info_obj[i], NFALIGN_LEFT, 8);
		nfui_nfobject_set_size(info_obj[i], IPCAM_WIN_W-pos_x-4, 24);
		nfui_nfobject_use_focus(info_obj[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(info_obj[i]);
		nfui_nffixed_put((NFFIXED*)ipcam_fixed, info_obj[i], pos_x, pos_y);

		pos_y += 24;
	}

	nfui_nfwindow_add((NFWINDOW*)ipcam_win, ipcam_fixed);
	nfui_run_main_event_handler(ipcam_win);
	nfui_nfobject_hide(ipcam_win);
}

void VW_Live_Net_IPCam_Popup_Close()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(ipcam_win);
}

void VW_Live_Net_IPCam_Popup_Show(guint ch, guint x, guint y)
{
	NFIPCamPortStatus cam_stats;
	GError *err = NULL;
    gint row_cnt = 0;

	g_return_if_fail(ipcam_win != NULL);

	nf_ipcam_get_port_status(ch, &cam_stats, &err);

//	if ((x + IPCAM_WIN_W) > DISPLAY_ACTIVE_WIDTH)
//		x = DISPLAY_ACTIVE_WIDTH - IPCAM_WIN_W;

	if (cam_stats.status == 0)
	{
		row_cnt = _set_ipcam_success_info(ch);
	}

	nfui_nfobject_set_size(ipcam_fixed, IPCAM_WIN_W, 12+24*row_cnt);
	nfui_nfobject_set_size(ipcam_win, IPCAM_WIN_W, 12+24*row_cnt);		
	nfui_nfobject_move(ipcam_win, x, y);

	if(!nfui_nfobject_is_shown(ipcam_win))
		nfui_nfobject_show(ipcam_win);		
}


void VW_Live_Net_IPCam_Popup_Hide()
{
	g_return_if_fail(ipcam_win != NULL);

	if(nfui_nfobject_is_shown(ipcam_win))
		nfui_nfobject_hide(ipcam_win);
}

void VW_Live_Net_IPCam_Popup_Is_Shown()
{
	return nfui_nfobject_is_shown(ipcam_win);
}

