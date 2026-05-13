#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nftimelabel.h"
#include "objects/nfcombobox.h"


#include "vw_vkeyboard.h"
#include "vw_set_date_time.h"
#include "vw_wizard_init.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"
#include "smt.h"


#define HELP_STR ""

#define MAX_MARGIM_SIZE			(guint)12

#define PI_WND_SIZE_WID			(guint)(610 + 200)
#define PI_WND_SIZE_HEI			(guint)(520 + 200)

#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		    (guint)(12)
#define PI_FIXED_POS_Y		    (guint)(56)
#define PI_FIXED_SIZE_WID	    (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	    (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH					(162)
#define MENU_BTN_HEIGHT					(44)
#define MENU_BTN_GAP					(4)

#define MENU_V_BTN_R_START_X			(PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X					(MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X					(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y					(PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)


#define	IPS_LABEL_HEIGHT			(40)
#define	IPS_LABEL_ROW_SPACE			(2)
#define CATEGORY_LABEL_LEFT         (4)
#define CATEGORY_CONTENT_GAP        (60)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(250)
#define	SUBJECT_LABEL_MARGIN		(0)

#define SUBJECT_LABEL_WIDTH1        (190)
#define DT_SETUP_LABEL_W			(300)
#define DT_SETUP_CELL_W				(440)

#define DT_WIN_SIZE_W			    (600)
#define DT_WIN_SIZE_H			    (200)

#define DT_POS_X				    ((DISPLAY_ACTIVE_WIDTH - DT_WIN_SIZE_W)/4*2)
#define DT_POS_Y			    	((DISPLAY_ACTIVE_HEIGHT - DT_WIN_SIZE_H)/2)


enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS,
};

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,

	NUM_DATE_FORMATS,
};

enum {
	TIME_MODE_24H = 0,
	TIME_MODE_12H,

	NUM_TIME_MODES,
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static DATATIME_DATA_T dtdata;
static DATATIME_DATA_T org_dtdata;
static guint dformat, tformat;
static guint g_time_update = 0;

static WIZARD_USERDATA_T *g_wizard_data;
static NFOBJECT *dateFormat_obj;
static NFOBJECT *timeFormat_obj;
static NFOBJECT *timeServer_obj;
static NFOBJECT *auto_timesync_obj;
static NFOBJECT *timesync_obj;
static NFOBJECT *timeZone_obj;
static NFOBJECT *dst_obj;
static NFOBJECT *lb_server;
static NFOBJECT *dt_obj;

static NFOBJECT *sub_win = NULL;

static mb_type g_popup_ret = 0;


static gint _exit_proc()
{
	nfui_nfobject_destroy(g_curwnd);	
	_wizard_finish();

	return 0;
}

static gint _next_step_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_next_step(1);

	return 0;
}

static gint _prev_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_prev_step(1);

    return 0;
}

static gint _prvLoadDataFromObjects()
{
	dtdata.dateFormat = nfui_combobox_get_cur_index(dateFormat_obj);
	dtdata.timeFormat = nfui_combobox_get_cur_index(timeFormat_obj);
	dtdata.timeZone = nfui_combobox_get_cur_index(timeZone_obj);
	dtdata.dst = nfui_combobox_get_cur_index(dst_obj);

	return 0;
}

static gint _load_DataTime_data()
{
    DAL_get_dateTime_format(&dtdata.dateFormat, &dtdata.timeFormat);
    DAL_get_tz_data(&dtdata.timeZone);
    dtdata.dst = DAL_get_dst();

}

static gint _save_DataTime_data()
{
    DAL_set_wizard_datetime_data(&dtdata);
    
    return 0;
}

static nftl_df_type _prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if(db_index == DF_YMD)			ret = NFTL_DF_YMD;
	else if(db_index == DF_MDY)		ret = NFTL_DF_MDY;
	else if(db_index == DF_DMY)		ret = NFTL_DF_DMY;
	else	ret = NFTL_DF_HIDE;

	return ret;
}

static gboolean _timeout_date_time_update(gpointer data)
{
	NFTIMELABEL* ti_obj;
	GTimeVal tv;
	GTimeVal tv_temp;

	memset(&tv, 0x00, sizeof(GTimeVal));
	memset(&tv_temp, 0x00, sizeof(GTimeVal));
	ti_obj = (NFTIMELABEL*)data;

	g_get_current_time(&tv);

	NFUTIL_THREADS_ENTER();

	nfui_nftimelabel_get_datetime(ti_obj, &tv_temp);

	if(tv.tv_sec != tv_temp.tv_sec)
	{
		nfui_nftimelabel_set_datetime(ti_obj, &tv);
		nfui_signal_emit((NFOBJECT*)ti_obj, GDK_EXPOSE, TRUE);
	}
	NFUTIL_THREADS_LEAVE();

	return TRUE;
}

static int _stop_timer()
{
	if (!g_time_update) return -1;
	g_source_remove(g_time_update);
	g_time_update = 0;
	return 0;
}

static int _restart_timer()
{
	if (g_time_update) return -1;
	g_time_update = g_timeout_add(300, _timeout_date_time_update, dt_obj);
	return 0;
}

static gboolean _sync_time(gpointer data)
{
	char *server_name;
	GTimeVal tv_temp;
	int ret;

	NFUTIL_THREADS_ENTER();

	memset(&tv_temp, 0x00, sizeof(GTimeVal));

	server_name = nfui_nflabel_get_text((NFLABEL*)lb_server);
	ret = scm_get_ntp_time(server_name, &tv_temp);
	if (ret < 0) {
		nftool_remove_waitbox((NFOBJECT*)data);
		vw_mbox(g_curwnd, "NOTICE", IMBX_FAIL_NTP, NFTOOL_MB_OK);
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	if (tv_temp.tv_sec != 0) {	
		_stop_timer();
		nfui_nftimelabel_set_datetime(dt_obj, &tv_temp);
		nfui_signal_emit((NFOBJECT*)dt_obj, GDK_EXPOSE, TRUE);		
		scm_set_system_time(&tv_temp);	
	}		

	nftool_remove_waitbox((NFOBJECT*)data);

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean post_dt_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int result;
	int rate;
	NFOBJECT *top = NULL;

	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	
	switch(evt->type) 
	{
		case GDK_EXPOSE:
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
        }
        break;
		
		default: break;
	}

	

	return FALSE;
}
	
static gboolean post_wait_popup_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean subwin_returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

static void vw_open_datetime_waitpopup(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	
	win = (NFOBJECT*)nfui_nfwindow_new(parent, DT_POS_X, DT_POS_Y, DT_WIN_SIZE_W, DT_WIN_SIZE_H);
	nfui_regi_post_event_callback(win, post_wait_popup_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, subwin_returnkey_proc);
	sub_win = win;

// fixed 
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DT_WIN_SIZE_W, DT_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_dt_fixed_event_cb);
	nfui_nfobject_show(fixed);

// title	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WAIT", 
									nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, DT_WIN_SIZE_W - 20, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);


//WAIT MESSAGE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("The system time is changing.",
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, DT_WIN_SIZE_W - 20, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, 90);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_MESSAGEBOX, win, nfui_get_last_user());

	gtk_main();	

	nfui_page_close(PGID_MESSAGEBOX, win);	
}

static void vw_hide_datetime_waitpopup()
{
	if (sub_win) 
		nfui_nfobject_hide(sub_win);
}

static void vw_destroy_datetime_waitpopup()
{
	if (sub_win) nfui_nfobject_destroy(sub_win);
	sub_win = NULL;
}

static gboolean post_timelabel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		_stop_timer();
	}
	return FALSE;
}

static gboolean post_datetime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	NFOBJECT *top;
	int x, y;

	GTimeVal pre_time;
	GTimeVal post_time;

	memset(&pre_time, 0x00, sizeof(GTimeVal));
	memset(&post_time, 0x00, sizeof(GTimeVal));
	
	if (evt->type == GDK_BUTTON_RELEASE) 
	{	
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);
	
		x += top->x + obj->width + 4;
		y += top->y;

		
		nfui_nftimelabel_get_datetime(dt_obj, &pre_time);		
		
		post_time.tv_sec = VW_Set_DateTime_Open(g_curwnd, "DATE/TIME", x, y, pre_time.tv_sec, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);

		if (post_time.tv_sec != 0)
		{	
			_stop_timer();
			nfui_nftimelabel_set_datetime(dt_obj, &post_time);
			nfui_signal_emit((NFOBJECT*)dt_obj, GDK_EXPOSE, TRUE);		
			scm_set_system_time(&post_time);	
		}		
	}		

	return FALSE;
}

static gboolean post_date_format_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type ==  NFEVENT_COMBOBOX_CHANGED) 
	{
		if (dtdata.dateFormat != nfui_combobox_get_cur_index(dateFormat_obj))
		{
			dtdata.dateFormat = nfui_combobox_get_cur_index(dateFormat_obj);
			nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, _prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);

			if (!g_time_update)
			{
				nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
				nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static gboolean post_time_format_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type ==  NFEVENT_COMBOBOX_CHANGED) 
	{
		if (dtdata.timeFormat != nfui_combobox_get_cur_index(timeFormat_obj))
		{
			dtdata.timeFormat = nfui_combobox_get_cur_index(timeFormat_obj);	
			nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, _prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);

			if (!g_time_update)
			{
				nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
				nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static gboolean post_sync_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *wbox = NULL;

	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
		g_timeout_add(300, _sync_time, (gpointer)wbox);
	}		

	return FALSE;
}

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += (obj->width)/2 + top->x;
		y += obj->height + top->y;
		
		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), 
								  x, y, 
								  STRING_SIZE_32, 
								  VKEY_NORMAL);
		
		if(strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);

			ifree(strTemp);
			strTemp = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
	}

	return FALSE;
}

static gboolean post_time_zone_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type ==  NFEVENT_COMBOBOX_CHANGED) 
	{
		if (dtdata.timeZone != nfui_combobox_get_cur_index(timeZone_obj))
		{
			dtdata.timeZone = nfui_combobox_get_cur_index(timeZone_obj);
			scm_apply_timezone(dtdata.timeZone, NULL);

			if (!g_time_update)
			{
				nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
				nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	CMM_MESSAGE_T *pmsg;

	switch (evt->type) 
	{
    	case GDK_DELETE:
    	{    	
    		uxm_unreg_imsg_event(obj, IRET_SCM_CHANGE_SYSTEM_TIME);
    		g_curwnd = 0;	
        }
    	break;
    		
    	case IRET_SCM_CHANGE_SYSTEM_TIME:
    	{
    		pmsg = (CMM_MESSAGE_T *)data;

    		vw_hide_datetime_waitpopup();
    		_restart_timer();

    		switch (pmsg->param) 
    		{
    		case 0:
    			nftool_mbox(g_curwnd, "INFORMATION", "The system time is changed successfully.", NFTOOL_MB_OK);
    			break;
    		case ER_VFS_STOPPING:
    			nftool_mbox(g_curwnd, "ERROR", "It's unable to stop the file system.", NFTOOL_MB_OK);
    			break;
    		case ER_DATA_DELETING:
    			nftool_mbox(g_curwnd, "ERROR", "It's unable to delete the data due to reserved one.", NFTOOL_MB_OK);
    			break;
    		default:
    			nftool_mbox(g_curwnd, "ERROR", "TBD", NFTOOL_MB_OK);
    			break;
    		}

    		smt_return_to_previous();
    		vw_destroy_datetime_waitpopup();
            gtk_main_quit();
    	}
    	break;
    	
    	default:
    	break;
	}

    return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
		if(evt->type == GDK_DELETE)
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
	
		return FALSE;

}

static gboolean post_exitbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		
		_exit_proc();
	}

	return FALSE;
}

static gboolean post_previousbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		 _prev_proc();
	}

	return FALSE;
}
	
static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		_prvLoadDataFromObjects();
		
		if(memcmp(&org_dtdata, &dtdata, sizeof(DATATIME_DATA_T)))
		{	
			scm_put_log(CHANGE_SYS_TIME, 0, 0);

			g_memmove(&org_dtdata, &dtdata, sizeof(DATATIME_DATA_T));
			g_memmove(&g_wizard_data->dtData, &dtdata, sizeof(DATATIME_DATA_T));
			_save_DataTime_data();
			scm_init_timesync();
		}
		
		if (scm_is_set_new_time()) {
			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK")) 
			{
				scm_restore_system_time();
				_restart_timer();
				return FALSE;
			}

			if (scm_is_past_time_set()) 
			{			
				ret = nftool_mbox(g_curwnd, "WARNING", "From now until the time you want to change the video will be deleted,\nhowever, the data will be backed up separately.\nAre you sure to continue?\n(* The data can be seen in Reserved Data Management)", NFTOOL_MB_ERASECANCEL);		

				if (ret == NFTOOL_MB_CANCEL) {
					scm_restore_system_time();
					_restart_timer();
					return FALSE;
				}
			}
			smt_set_service(SMT_TIME_CHANGE);
			scm_apply_time(IRET_SCM_CHANGE_SYSTEM_TIME);
//			vw_open_datetime_waitpopup(g_curwnd);
			sub_win = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "The system time is changing.", "");
			gtk_main();
		}

		_next_step_proc();
	}

	return FALSE;
}


gint vw_wizard_datetime_change_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;	
	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *sync_img[NFOBJECT_STATE_COUNT];
	gchar *strTimezone[50];

	const gchar *strDateFormat[NUM_DATE_FORMATS] = {
		"YYYY/MM/DD",
		"MM/DD/YYYY",
		"DD/MM/YYYY"
	};

	const gchar *strTimeFormat[NUM_TIME_MODES] = {
		"24 HOUR",
		"AM / PM"
	};
	
	const gchar *strOffOn[] = {"OFF", "ON"};
	
	NFOBJECT *btns[PIB_BUTTONS];
	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};
	
    guint zone_count = 0;
	guint tm_size_w, tm_size_h;		// set time image size.
	guint sy_size_w, sy_size_h;		// set time image size.
	guint ck_size_w, ck_size_h;		// check box image size.
	gint pos_x,pos_y,size_w,size_h;
    gint i, cnt;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;

// <---- IMAGE LOAD
	datetime_img[0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	datetime_img[1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);	
	datetime_img[2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);	
	datetime_img[3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);	

	nfui_get_pixbuf_size(datetime_img[0], &tm_size_w, &tm_size_h);

	sync_img[0] = nfui_get_image_from_file((IMG_N_SYNC_TIME), NULL);
	sync_img[1] = nfui_get_image_from_file((IMG_O_SYNC_TIME), NULL);
	sync_img[2] = nfui_get_image_from_file((IMG_P_SYNC_TIME), NULL);
	sync_img[3] = nfui_get_image_from_file((IMG_D_SYNC_TIME), NULL);

	nfui_get_pixbuf_size(sync_img[0], &sy_size_w, &sy_size_h);

// <---- DB LOAD
	memset(&dtdata, 0x00, sizeof(DATATIME_DATA_T));
	memset(&org_dtdata, 0x00, sizeof(DATATIME_DATA_T));

    _load_DataTime_data();
	//g_memmove(&org_dtdata, &g_wizard_data->dtData, sizeof(DATATIME_DATA_T));
	//g_memmove(&dtdata, &g_wizard_data->dtData, sizeof(DATATIME_DATA_T));

	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = (guint)4;
	pos_y = (guint)4;

////////////////////////////////////////////////////
//	TIMEZONE / DST
//	
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, "TIMEZONE / DST");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

// <---- TIME/ZONE
    pos_x = SUBJECT_LABEL_LEFT;
    pos_y += CATEGORY_CONTENT_GAP;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMEZONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	zone_count = nf_zoneinfo_get_count();
	
	for(i=0; i<zone_count; i++)
		strTimezone[i] = nf_zoneinfo_get_string((gint)i);

	obj = nfui_combobox_new(strTimezone, zone_count, (gint)(dtdata.timeZone));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_time_zone_event_handler);	
	timeZone_obj = obj;

// <---- DST
    pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	obj = nfui_combobox_new(strOffOn, 2, (gint)(dtdata.dst));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	dst_obj = obj;

	pos_x = CATEGORY_LABEL_LEFT;
    pos_y += IPS_LABEL_HEIGHT + 40;
    
////////////////////////////////////////////////////
//	TIME SETTING
//	
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, "TIME SETTING");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);


#if 0
// <---- TIME SERVER
	pos_x = SUBJECT_LABEL_LEFT;
    pos_y += CATEGORY_CONTENT_GAP;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(dtdata.timeServer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

	timeServer_obj = obj;
	lb_server = obj;

	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_label_event_handler);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), sync_img);
	nfui_nfobject_set_size(obj, (guint)sy_size_w, (guint)sy_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE + DT_SETUP_CELL_W + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_sync_event_handler);
#endif
// <---- DATE / TIME
    pos_y += CATEGORY_CONTENT_GAP;
    pos_x = SUBJECT_LABEL_LEFT;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DATE/TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(129));
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(128));
	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, _prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_timelabel_event_handler);
    dt_obj = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
	nfui_nfobject_set_size(obj, (guint)sy_size_w, (guint)sy_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE + DT_SETUP_CELL_W + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_datetime_event_handler);

// <---- DATE FORMAT
    pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DATE FORMAT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	obj = nfui_combobox_new(strDateFormat, NUM_DATE_FORMATS, (gint)(dtdata.dateFormat));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_date_format_event_handler);
	dateFormat_obj = obj;

// <---- TIME FORMAT
    pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME FORMAT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	obj = nfui_combobox_new(strTimeFormat, NUM_TIME_MODES, (gint)(dtdata.timeFormat));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	nfui_regi_post_event_callback(obj, post_time_format_event_handler);
	timeFormat_obj = obj;

    pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
    
	for( i=0; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	g_time_update = 0;
	g_time_update = g_timeout_add(300, _timeout_date_time_update, dt_obj);

	uxm_reg_imsg_event(main_fixed, IRET_SCM_CHANGE_SYSTEM_TIME);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	gtk_main();

	return 0;

}

