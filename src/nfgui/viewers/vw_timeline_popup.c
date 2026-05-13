#include <string.h>

#include "nf_afx.h"
#include "scm.h"

#include "services/uxm.h"

#include "modules/ssm.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflabel.h"

#include "vw_internal.h"
#include "vsm.h"
#include "stm.h"
#include "dtf.h"
#include "smt.h"

#include "vw_timeline.h"
#include "vw_arch_export.h"
#include "vw_live_statusbar.h"
#include "vw_search_main.h"
#include "vw_timeline_popup.h"
#include "vw_timeline_popup_submenu_search.h"
#include "vw_timeline_popup_submenu_export.h"
#include "vw_timeline_popup_submenu_pano.h"


#define VTP_SIZE_W							(360)
#ifndef VER2
#define VTP_SIZE_H							(280 - 40)
#else
#define VTP_SIZE_H							(280)
#endif
#define VTP_XPOS_GAP						(8) // (4)

#define VTP_SUB_SIZE_W						(287)
#define VTP_SUB_SIZE_H						(104)

#define VTP_BUTTON_POS_X					(12)
#define VTP_BUTTON_POS_Y					(5)
#define VTP_BUTTON_MARGIN					(15) 
#define VTP_BUTTON_SIZE_W					(VTP_SIZE_W - VTP_BUTTON_POS_X - VTP_BUTTON_MARGIN) 
#define VTP_BUTTON_SIZE_H					(40) 
#define VTP_BUTTON_GAP_2					(2)

#define VTP_LABEL_SIZE_W					(VTP_BUTTON_SIZE_W)
#define VTP_LABEL_SIZE_H					(VTP_BUTTON_SIZE_H)


typedef struct _TIME_D TIME_D;
struct _TIME_D {
	GTimeVal s_time;
	GTimeVal e_time;
};

enum {
	START_TIME_LABEL = 0,
	END_TIME_LABEL,
	TIME_LABEL_CNT
};

typedef enum _QUERY_REASON_E {
	EXPORT		= 0,
	RESERVE		= 1,
} QUERY_REASON_E;

static BURN_INFO burn_info;
static QUERY_REASON_E g_qry_reason = 0;
static NFOBJECT *wait_mbox = NULL;
static GdkRectangle 	rect_main;
static GdkRectangle 	rect_pano;
static GdkRectangle 	rect_search;
static GdkRectangle 	rect_export;

static TIME_D g_time_t;
static TLINE_MODE_E	tl_mode = TL_LIVE;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_vtp_w;

static NFOBJECT *g_time_obj[TIME_LABEL_CNT];

static GdkPixbuf *g_menuBG[2][NFOBJECT_STATE_COUNT];
NF_ARCH_AVI_INFO query_info;

static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};



/////////////////////////////////////////////////////////////////////////
//
//

static void make_vtp_bg_images()
{
	GdkPixbuf *pbArrow[NFOBJECT_STATE_COUNT];

	// menu button bg
	g_menuBG[0][0] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_N_BUTTON, VTP_BUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[0][1] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_O_BUTTON, VTP_BUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[0][2] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_P_BUTTON, VTP_BUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[0][3] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_D_BUTTON, VTP_BUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	// arrow menu button bg
	g_menuBG[1][0] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_N_BUTTON_ARROW, VTP_BUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[1][1] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_O_BUTTON_ARROW, VTP_BUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[1][2] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_P_BUTTON_ARROW, VTP_BUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[1][3] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_D_BUTTON_ARROW, VTP_BUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);
	
	// arrow 
	pbArrow[0] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_N), NULL);
	pbArrow[1] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_O), NULL);
	pbArrow[2] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_P), NULL);
	pbArrow[3] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_D), NULL);

	gdk_pixbuf_composite(pbArrow[0], g_menuBG[1][0], (VTP_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(VTP_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbArrow[1], g_menuBG[1][1], (VTP_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(VTP_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbArrow[2], g_menuBG[1][2], (VTP_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(VTP_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbArrow[3], g_menuBG[1][3], (VTP_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(VTP_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
}


static void set_time_data(GTimeVal s_t, GTimeVal e_t)
{
	g_time_t.s_time = s_t;
	g_time_t.e_time = e_t;
}

static void set_time_text()
{
	gchar strTime[64];
	gchar tmp[32];
	gchar *p;
	gint length;
	guint dformat = 0;
	FM_DATE_E fm_date; 

	DAL_get_dateTime_format(&dformat, NULL);

	if (dformat == 0) fm_date = YYYYMMDD;
	else if (dformat == 1) fm_date = MMDDYYYY;
	else fm_date = DDMMYYYY;	

	// start time
//	dtf_get_local_datetime(g_time_t.s_time.tv_sec, tmp);
//	tmp[17] = '\0';

    memset(tmp, 0x00, sizeof(tmp));
    dtf_get_localtime_text(g_time_t.s_time.tv_sec, fm_date, H24, tmp);

	p = lookup_string("START");
	length = strlen(p);
	g_utf8_strncpy(strTime, p, g_utf8_strlen(p, -1));
	g_sprintf(&strTime[length], ": %s", tmp);

	nfui_nflabel_set_text((NFLABEL*)g_time_obj[START_TIME_LABEL], strTime);
	
	// end time
	memset(strTime, 0x00, sizeof(strTime));
//	dtf_get_local_datetime(g_time_t.e_time.tv_sec, tmp);
//	tmp[17] = '\0';

    memset(tmp, 0x00, sizeof(tmp));
    dtf_get_localtime_text(g_time_t.e_time.tv_sec, fm_date, H24, tmp);

	p = lookup_string("END");
	length = strlen(p);
	g_utf8_strncpy(strTime, p, g_utf8_strlen(p, -1));
	g_sprintf(&strTime[length], ": %s", tmp);

	nfui_nflabel_set_text((NFLABEL*)g_time_obj[END_TIME_LABEL], strTime);
}

static void update_time_info(GTimeVal s_t, GTimeVal e_t)
{
	set_time_data(s_t, e_t);
	set_time_text();
}

static void set_query_param_data(NF_ARCH_AVI_PARAM *param)
{
#ifdef GUI_4CH_SUPPORT
	guint mask = 0x000F;
#elif GUI_8CH_SUPPORT
	guint mask = 0x00FF;
#elif GUI_16CH_SUPPORT
	guint mask = 0xFFFF;
#else	
	guint mask = 0xFFFFFFFF;
#endif
	GTimeVal s_t, e_t;
	struct tm *stime_info;
	struct tm *etime_info;
	gchar buf[32];
	gchar buf2[32];

	memset(param, 0x00, sizeof(NF_ARCH_AVI_PARAM));

	memset(&s_t, 0x00, sizeof(GTimeVal));
	memset(&e_t, 0x00, sizeof(GTimeVal));
	// set time
	get_time_data(&s_t, &e_t);
	param->start_time = s_t;
	param->end_time   = e_t;

    if (g_qry_reason == EXPORT)
    {
        if (ssm_get_covert_mask() != 0)
        {
            nftool_mbox(g_curwnd, "WARNING", "Covert channel settings do not apply when archiving recorded data.", NFTOOL_MB_OK);            
        }
        
        mask &= ~(ssm_get_covert_mask());
    }

	// set mask
	param->ch_mask    = mask;
	param->audio_mask = mask;

	// set option
	param->inc_log 	  = 1;
	param->inc_text   = 1;
	param->inc_ri     = 1;
	param->inc_codec  = 1;
	param->inc_player = 1;


	// set tag name [32]
	stime_info = NFLOCALTIME(&(s_t.tv_sec));
	memset(buf, 0x00, sizeof(buf));
	strftime(buf, sizeof(buf), "%Y%m%d%H%M", stime_info); 

	etime_info = NFLOCALTIME(&(e_t.tv_sec));
	memset(buf2, 0x00, sizeof(buf2));
	strftime(buf2, sizeof(buf2), "%Y%m%d%H%M", etime_info); 

	g_sprintf(param->tag, "%s~%s", buf, buf2);
	//g_message("%s :::::::::::length %d::::::: tag name %s ", __FUNCTION__, strlen(param->tag), param->tag);

	// set user
	ssm_get_cur_id(buf);
	strcpy(param->user, buf);

	//avi_param->memo;
}

static void _reserve_data()
{
	int ret;
	ret = scm_reserve_bookmark_info();
	switch(ret) {
		case -1:			
			nftool_mbox(g_curwnd, "ERROR", "Internal error.", NFTOOL_MB_OK);
			break;

		case -11:													// List is full
			nftool_mbox(g_curwnd, "ERROR", "It's unable to reserve\nbecause the archiving list is full.", NFTOOL_MB_OK);
			break;	

		case -12: 													// queried data is already added
			nftool_mbox(g_curwnd, "ERROR", "Current queried data is\nalready added.", NFTOOL_MB_OK);
			break;	

		default:
			break;
	}


	if(ret >= 0)
		nftool_mbox_auto(g_curwnd, 2, "NOTICE", "Data is reserved successfully.");

}

static void _query_data()
{
	NF_ARCH_AVI_PARAM avi_param;

	scm_end_query();
	set_query_param_data(&avi_param);

	wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

	uxm_reg_imsg_event(g_vtp_w, INFY_QUERY_SUCCESS);
	uxm_monitor_on_imsg_event(g_vtp_w, INFY_QUERY_SUCCESS);
	uxm_reg_imsg_event(g_vtp_w, INFY_QUERY_OVER);
	uxm_monitor_on_imsg_event(g_vtp_w, INFY_QUERY_OVER);
	uxm_reg_imsg_event(g_vtp_w, INFY_QUERY_ERROR);
	uxm_monitor_on_imsg_event(g_vtp_w, INFY_QUERY_ERROR);
	uxm_reg_imsg_event(g_vtp_w, INFY_QUERY_NO_VIDEODATA);
	uxm_monitor_on_imsg_event(g_vtp_w, INFY_QUERY_NO_VIDEODATA);


	smt_set_service(SMT_ARCHIVE_QUERY);
	if (scm_start_avi_query(&avi_param) != QRY_SUCCESS) {
		if(wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}
		nftool_mbox(g_curwnd, "ERROR", "Internal error.", NFTOOL_MB_OK);
		scm_end_query();

		if (tl_mode == TL_LIVE) smt_set_service(SMT_LIVE);
		else smt_set_service(SMT_PLAYBACK);

	}
}

static gboolean post_export_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	SecurityData secdata;
	gboolean use_dl = 0;

	switch(evt->type) {
	case GDK_BUTTON_PRESS:
            DAL_get_use_double_login(&use_dl);

            if (use_dl && !ssm_is_admin())
            {
                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
            }
            else
            {
			DAL_get_security_data(&secdata);
        		if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
            }
	
			g_qry_reason = EXPORT;
			_query_data();
			break;

	default:
			break;
	}

	return FALSE;
}

static gboolean post_reserve_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	SecurityData secdata;
	gboolean use_dl = 0;

	switch(evt->type) {
		case GDK_BUTTON_PRESS:
            DAL_get_use_double_login(&use_dl);

            if (use_dl && !ssm_is_admin())
            {
                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
            }
            else
            {
    			DAL_get_security_data(&secdata);
        		if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
            }
		
			g_qry_reason = RESERVE;
			_query_data();
			break;


		case GDK_DELETE:
			break;

		default:
			break;
	}

	return FALSE;
}

static int _make_burn_info(guint16 arch_id, guchar dev_id, BURN_INFO *burn_info)
{
	burn_info->type = NF_ARCH_TYPE_AVI;
	burn_info->arch_id = arch_id;
	burn_info->dev_id = dev_id;
	return 0;
}

static int _export_data()
{
	memset(&burn_info, 0x00, sizeof(BURN_INFO));
	_make_burn_info(query_info.arch_id, 0xff, &burn_info);		// 0xff is just temporary dev_id
	VW_ArchExport_Open(g_curwnd, &burn_info);
}

static gboolean post_vtp_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	CMM_MESSAGE_T *pmsg;
	NF_ARCH_AVI_INFO *avi;
	char tibuf[128];
	char msgbuf[512];
	GTimeVal val;

	memset(&val, 0x00, sizeof(GTimeVal));
	switch (evt->type) {
	case GDK_DELETE:
		VW_Destory_Search_SubMenu();
		g_curwnd = 0;
		break;

	case GDK_BUTTON_PRESS:
		VW_Timeline_PopUp_Hide();
		break;

	case INFY_QUERY_NO_VIDEODATA:
	case INFY_QUERY_ERROR:
		if(wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}
		sprintf(msgbuf, "There was some error while query");
		nftool_mbox(g_curwnd, "ERROR", msgbuf, NFTOOL_MB_OK);
		uxm_unreg_imsg_event(obj, INFY_QUERY_ERROR);
		break;

	case INFY_QUERY_OVER:
		if(wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}

		pmsg = (CMM_MESSAGE_T *)data;
		memcpy(&query_info, pmsg->data, sizeof(NF_ARCH_AVI_INFO));
		GUINT64_TO_GTIMEVAL(query_info.time_end, val);
		dtf_get_local_datetime(val.tv_sec, tibuf);
		sprintf(msgbuf, lookup_string("Query operation is completed.\nThe end time is changed to %s,\nbecause limited size of 20GB has been exceeded."), tibuf);
		nftool_mbox(g_curwnd, "WARNING", msgbuf, NFTOOL_MB_OK);
		uxm_unreg_imsg_event(obj, INFY_QUERY_OVER);

		switch (g_qry_reason) {
    		case 0:	
    		    _export_data();
		    break;
		    
    		case 1: 
    		    _reserve_data(); 
		    break;
		}
		scm_end_query();
		if (tl_mode == TL_LIVE) smt_set_service(SMT_LIVE);
		else smt_set_service(SMT_PLAYBACK);
		break;

	case INFY_QUERY_SUCCESS:
		{
			pmsg = (CMM_MESSAGE_T *)data;
			NF_ARCH_AVI_INFO *avi_info = (NF_ARCH_AVI_INFO*)pmsg->data;
			gint ret = 0;
			guint total_size = 0;
			gint i;

			memcpy(&query_info, pmsg->data, sizeof(NF_ARCH_AVI_INFO));

			uxm_unreg_imsg_event(obj, INFY_QUERY_SUCCESS);
			for(i=0; i<GUI_CHANNEL_CNT; i++) {
				if(avi_info->channel_mask & (1 << i)) {
					total_size += avi_info->ch_size[i];	
				}
			}

			if(total_size == 0) {
				if(wait_mbox) {
					nftool_remove_waitbox((NFOBJECT*)wait_mbox);
					wait_mbox = NULL;
				}
				nftool_mbox(g_curwnd, "NOTICE", "The data size is 0.\nThere's no data for reserving.", NFTOOL_MB_OK);
				scm_end_query();

				break;
			}

			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}

			switch (g_qry_reason) {
        		case 0:	
        		    _export_data();
    		    break;
    		    
        		case 1: 
        		    _reserve_data(); 
    		    break;
			}
			scm_end_query();
			if (tl_mode == TL_LIVE) smt_set_service(SMT_LIVE);
			else smt_set_service(SMT_PLAYBACK);
		}

		break;
	}

	return FALSE;
}




static gboolean post_vtp_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
    	drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }   

	return FALSE;
}


static gboolean panorama_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{

				if (VW_IsShown_Search_SubMenu()) {
					VW_Hide_Search_SubMenu();
				}
#ifdef VER2
				if (!VW_IsShown_Pano_SubMenu()) {
					VW_Show_Pano_SubMenu();
				}
#endif
			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean search_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{


#ifdef VER2
				if (VW_IsShown_Pano_SubMenu()) {
					VW_Hide_Pano_SubMenu();
				}
#endif

				if (!VW_IsShown_Search_SubMenu()) {
					VW_Show_Search_SubMenu();
				}
			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean export_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{

#ifdef VER2
				if (VW_IsShown_Pano_SubMenu()) {
					VW_Hide_Pano_SubMenu();
				}
#endif

				if (VW_IsShown_Search_SubMenu()) {
					VW_Hide_Search_SubMenu();
				}


			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean reserve_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{

#ifdef VER2
				if (VW_IsShown_Pano_SubMenu()) {
					VW_Hide_Pano_SubMenu();
				}
#endif

				if (VW_IsShown_Search_SubMenu()) {
					VW_Hide_Search_SubMenu();
				}


			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;

}

static gboolean popup_submenu_get_position(int item_idx, int main_w, guint main_x, guint main_y, gint *mx, gint *my)
{
#define SCR_W		1920
#define SCR_H		1080

	int submenu_size[6][2] = {		// w, h
			{0, 0},
			{0, 0},
			{200, 380},				// w, h
			{360, 100},
			{200, 300},
			{0, 0}
	};

	int menu_item_height = VTP_BUTTON_SIZE_H;
	int mainmenu_w = 360;

	
	int submenu_x, submenu_y;
	int gap = 4;

	submenu_x = main_x - mainmenu_w - submenu_size[item_idx][0] - gap;
#ifdef VER2
	submenu_y = main_y + menu_item_height * item_idx + 8;
#else
	submenu_y = main_y + menu_item_height * (item_idx - 1) + 8;
#endif

	*mx = submenu_x;
	*my = submenu_y;

	return FALSE;
}


void VW_Timeline_PopUp_New(NFWINDOW *parent)
{
	NFOBJECT *vtp_f;
	NFOBJECT *obj;
	
	gchar *strLabel[2] = {"START: ", "END: "};
	gchar *strBtn[4] = {"PANORAMA2", "SEARCH", "EXPORT TO", "RESERVE DATA"};
	guint pos_y = 0;
	gint i;


	// get image
	make_vtp_bg_images();

	/* window */
	g_vtp_w = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, VTP_SIZE_W, VTP_SIZE_H);
	g_curwnd = g_vtp_w;
	nfui_regi_post_event_callback(g_vtp_w, post_vtp_window_event_cb);
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_vtp_w)->main_widget), FALSE);

//	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_vtp_w, TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)g_vtp_w, GDK_BUTTON_PRESS, TRUE);
	
//	gtk_widget_add_events(((NFWINDOW*)g_vtp_w)->main_widget, GDK_POINTER_MOTION_HINT_MASK);


	/* fixed */
	vtp_f = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(vtp_f, VTP_SIZE_W, VTP_SIZE_H);
	nfui_nfobject_modify_bg(vtp_f, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(vtp_f, post_vtp_fixed_event_cb);
	nfui_nfobject_show(vtp_f);


	/* */
	pos_y = VTP_BUTTON_POS_Y;

	for(i=0; i<6; i++) {
		if(i < 2) {				// start/end time label
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), 292);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, VTP_BUTTON_MARGIN);
			nfui_nfobject_set_size(obj, VTP_LABEL_SIZE_W, VTP_LABEL_SIZE_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			g_time_obj[i] = obj;
		}else {
#ifndef VER2
			if((i - 2) == 0)
				continue;
#endif
			if(i < 4)
				obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[1], strBtn[i - 2]);
			else
				obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[0], strBtn[i - 2]);

			switch (i) {
			case 2:
				nfui_regi_pre_event_callback(obj, panorama_button_event_handler);
				nfui_nfobject_disable(obj);
				break;
			case 3:
				nfui_nfobject_set_data(g_vtp_w, "SEARCH MENU", obj);
				nfui_regi_pre_event_callback(obj, search_button_event_handler);
				break;
			case 4:
				nfui_nfobject_set_data(g_vtp_w, "EXPORT MENU", obj);
				nfui_regi_pre_event_callback(obj, export_button_event_handler);
				break;
			case 5:
				nfui_nfobject_set_data(g_vtp_w, "RESERVE MENU", obj);
				nfui_regi_pre_event_callback(obj, reserve_button_event_handler);
				break;
			}

				
								
			nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, VTP_BUTTON_MARGIN);
			nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)g_font_color);
			nfui_nfobject_set_size(obj, VTP_BUTTON_SIZE_W, VTP_BUTTON_SIZE_H);
			nfui_nfobject_show(obj);
		}
		
		if(i != 0) 
			pos_y += (VTP_BUTTON_SIZE_H + VTP_BUTTON_GAP_2);

		if(i >= 2 && i <= 5) 
			pos_y += 5;

		nfui_nffixed_put((NFFIXED*)vtp_f, obj, VTP_BUTTON_POS_X, pos_y);

		if(i == 4) {
			nfui_regi_post_event_callback(obj, post_export_event_cb);
		}else if(i == 5) {
			nfui_regi_post_event_callback(obj, post_reserve_event_cb);
		}
	}

	nfui_nfwindow_add((NFWINDOW*)g_vtp_w, vtp_f);
	nfui_run_main_event_handler(g_vtp_w);
	nfui_nfobject_hide(g_vtp_w);

#ifdef VER2
	VW_Create_Pano_SubMenu();
#endif
	VW_Create_Search_SubMenu(g_curwnd);
//	VW_Create_Export_SubMenu();
}


void VW_Timeline_PopUp_Show(guint x, guint y, GTimeVal start_t, GTimeVal end_t, TLINE_MODE_E mode)
{
	NFOBJECT *obj = NULL;

	tl_mode = mode;
	g_return_if_fail(g_vtp_w != NULL);

	update_time_info(start_t, end_t);

	if((y + VTP_SIZE_H) > DISPLAY_ACTIVE_HEIGHT)
		y = (DISPLAY_ACTIVE_HEIGHT - VTP_SIZE_H);

	obj = (NFOBJECT*)nfui_nfobject_get_data(g_vtp_w, "SEARCH MENU");
	if(!ssm_check_access_auth(USR_AUTH_SEARCH)) nfui_nfobject_disable(obj);
	else 										nfui_nfobject_enable(obj);

	obj = (NFOBJECT*)nfui_nfobject_get_data(g_vtp_w, "EXPORT MENU");
	if(!ssm_check_access_auth(USR_AUTH_ARCHIVE)) nfui_nfobject_disable(obj);
	else 										 nfui_nfobject_enable(obj);

	if (scm_is_bookmarking()) nfui_nfobject_disable(obj);

	obj = (NFOBJECT*)nfui_nfobject_get_data(g_vtp_w, "RESERVE MENU");
	if(!ssm_check_access_auth(USR_AUTH_ARCHIVE)) nfui_nfobject_disable(obj);
	else 										 nfui_nfobject_enable(obj);

	if (scm_is_bookmarking()) nfui_nfobject_disable(obj);

	nfui_nfobject_move(g_vtp_w, (x - VTP_SIZE_W - VTP_XPOS_GAP), y);
	nfui_nfobject_show(g_vtp_w);

	{
		int i;
		int mx, my;

		for (i = 2; i < 5; ++i) {
			switch (i) {
				case 2:
#ifdef VER2
					popup_submenu_get_position(i, VTP_SIZE_W, 1724, y, &mx, &my);
					VW_Move_Pano_SubMenu(mx, my);
					VW_Get_Pano_SubMenu_Geometry(	&rect_pano.x, 
													&rect_pano.y,
													&rect_pano.width, 
													&rect_pano.height);
#endif
					break;
				case 3:
					popup_submenu_get_position(i, VTP_SIZE_W, 1724, y, &mx, &my);
					VW_Move_Search_SubMenu(mx, my);
					VW_Get_Search_SubMenu_Geometry(	&rect_search.x, 
													&rect_search.y,
													&rect_search.width, 
													&rect_search.height);
					break;

			}
		}
	}

	gdk_window_get_geometry(((NFWINDOW *)g_vtp_w)->main_widget->window, 
		&rect_main.x, &rect_main.y,
		&rect_main.width, &rect_main.height, 0);


	nfui_regi_semi_modal(g_vtp_w);
	nfui_hook_evt_in_semi_modal(GDK_BUTTON_PRESS);
	nfui_enable_semi_modal_mode(g_vtp_w);
}


int VW_Timeline_PopUp_Destroy()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

void VW_Timeline_PopUp_Hide()
{
	g_return_if_fail(g_vtp_w != NULL);

	nfui_disable_semi_modal_mode();
	nfui_nfobject_hide(g_vtp_w);

#ifdef VER2
	VW_Hide_Pano_SubMenu();
#endif
	VW_Hide_Search_SubMenu();
//	VW_Hide_Export_SubMenu();
}

void get_time_data(GTimeVal *s_t, GTimeVal *e_t)
{
	*s_t = g_time_t.s_time;
	*e_t = g_time_t.e_time;
}



