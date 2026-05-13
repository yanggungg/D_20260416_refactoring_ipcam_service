#include "nf_afx.h"

#include "nf_api_disk.h"
#include "nf_util_time.h"
#include "nf_util_sntp.h"
#include "nf_network.h"
#include "nf_record.h"
#include "nf_dspcomm_app.h"
#include "nf_api_eventlog.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_common_data.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"
#include "objects/nftimelabel.h"

#include "vw_sys_main.h"
#include "vw_date_time.h"
#include "vw_set_date_time.h"
#include "vw_user_defined_dst_popup.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "iux_msg.h"
#include "cmm.h"
#include "scm.h"
#include "ssm.h"
#include "uxm.h"
#include "smt.h"
#include "ix_mem.h"
#include "vw_sys_camera_main.h"
#include "vw_date_time_main.h"

#define DT_SETUP_TEXT_MARGIN					(20)
#define DT_SETUP_LABEL_H						(40)

#define DT_SETUP_LABEL_W						(300)
#define DT_SETUP_CELL_W							(400)


// TIME SETTING.
#define DT_TIME_SET_TITLE_X						(8)
#define DT_TIME_SET_TITLE_Y						(0)

#define DT_DATE_TIME_LABEL_X					(DT_TIME_SET_TITLE_X + DT_SETUP_TEXT_MARGIN)
#define DT_DATE_TIME_LABEL_Y					(DT_TIME_SET_TITLE_Y + DT_SETUP_LABEL_H + 21)

#define DT_DATE_FORMAT_LABEL_X					(DT_DATE_TIME_LABEL_X)
#define DT_DATE_FORMAT_LABEL_Y					(DT_DATE_TIME_LABEL_Y + DT_SETUP_LABEL_H + 2)

#define DT_TIME_FORMAT_LABEL_X					(DT_DATE_TIME_LABEL_X)
#define DT_TIME_FORMAT_LABEL_Y					(DT_DATE_FORMAT_LABEL_Y + DT_SETUP_LABEL_H + 2)


// NETWORK TIME SYNC.
#define DT_NET_TIMESYNC_TITLE_X					(DT_TIME_SET_TITLE_X)
#define DT_NET_TIMESYNC_TITLE_Y					(DT_TIME_FORMAT_LABEL_Y + DT_SETUP_LABEL_H + 60)

#define DT_TIME_SERVER_LABEL_X					(DT_DATE_TIME_LABEL_X)
#define DT_TIME_SERVER_LABEL_Y					(DT_NET_TIMESYNC_TITLE_Y + DT_SETUP_LABEL_H + 21)

#define DT_AUTO_TIME_SYNC_LABEL_X				(DT_DATE_TIME_LABEL_X)
#define DT_AUTO_TIME_SYNC_LABEL_Y				(DT_TIME_SERVER_LABEL_Y + DT_SETUP_LABEL_H + 2)

#define DT_SYNC_FREQUENCY_LABEL_X				(DT_DATE_TIME_LABEL_X)
#define DT_SYNC_FREQUENCY_LABEL_Y				(DT_AUTO_TIME_SYNC_LABEL_Y + DT_SETUP_LABEL_H + 2)

#define DT_SYNC_TIME_LABEL_X					(DT_DATE_TIME_LABEL_X)
#define DT_SYNC_TIME_LABEL_Y					(DT_SYNC_FREQUENCY_LABEL_Y + DT_SETUP_LABEL_H + 2)

#define DT_NEXT_SYNC_TIME_LABEL_X				(DT_DATE_TIME_LABEL_X)
#define DT_NEXT_SYNC_TIME_LABEL_Y				(DT_SYNC_TIME_LABEL_Y + DT_SETUP_LABEL_H + 2)

#define DT_RUN_SERVER_CHKBOX_X					(DT_DATE_TIME_LABEL_X)
#define DT_RUN_SERVER_CHKBOX_Y					(DT_NEXT_SYNC_TIME_LABEL_Y + DT_SETUP_LABEL_H + 2)

#define DT_RUN_SERVER_LABEL_W					(300)


// TIMEZONE / DST.
#define DT_TZ_DST_TITLE_X						(DT_TIME_SET_TITLE_X)
#define DT_TZ_DST_TITLE_Y						(DT_RUN_SERVER_CHKBOX_Y + DT_SETUP_LABEL_H + 60)

#define DT_TIME_ZONE_LABEL_X					(DT_DATE_TIME_LABEL_X)
#define DT_TIME_ZONE_LABEL_Y					(DT_TZ_DST_TITLE_Y + DT_SETUP_LABEL_H + 21)

#define DT_DST_LABEL_X							(DT_DATE_TIME_LABEL_X)
#define DT_DST_LABEL_Y							(DT_TIME_ZONE_LABEL_Y + DT_SETUP_LABEL_H + 2)


#define DT_WIN_SIZE_W			(600)
#define DT_WIN_SIZE_H			(200)

#define DT_POS_X				((DISPLAY_ACTIVE_WIDTH - DT_WIN_SIZE_W)/4*2)
#define DT_POS_Y				((DISPLAY_ACTIVE_HEIGHT - DT_WIN_SIZE_H)/2)

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

enum{
    LOCAL_TIME = 0,
    DB_TIME,

    NEED_TIME
};

enum {
	ALL_NTP_OFF = 0,
	SMART_NTP,
	SCHED_NTP
};

static NF_TIME_UDD nf_udd;

static NFOBJECT *lb_server;
static NFOBJECT *dt_obj;
static NFOBJECT *cancel_btn;

static DateTimeData dtdata;
static DateTimeData org_dtdata;
static guint dformat, tformat;
static guint g_time_update = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *dateFormat_obj;
static NFOBJECT *timeFormat_obj;
static NFOBJECT *timeServer_obj;
static NFOBJECT *auto_timesync_obj;
static NFOBJECT *g_sync_freq_obj;
static NFOBJECT *timesync_obj;
static NFOBJECT *g_next_time_obj;
static NFOBJECT *timeZone_obj;
static NFOBJECT *dst_obj;

static NFOBJECT *sub_win = NULL;


static FM_DATE_E prvTransDateFormat_ver2(gint db_index)
{
	FM_DATE_E ret;

	if(db_index == DF_YMD)			ret = YYYYMMDD;
	else if(db_index == DF_MDY)		ret = MMDDYYYY;
	else if(db_index == DF_DMY)		ret = DDMMYYYY;
	else	ret = MMDDYYYY;

	return ret;
}

static nftl_df_type prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if(db_index == DF_YMD)			ret = NFTL_DF_YMD;
	else if(db_index == DF_MDY)		ret = NFTL_DF_MDY;
	else if(db_index == DF_DMY)		ret = NFTL_DF_DMY;
	else	ret = NFTL_DF_HIDE;

	return ret;
}

static guint _index_to_sync_freq(gint idx)
{
    guint freq = 0;
    gint h, m, s;
    gint st;
    GTimeVal tv;

    switch(idx)
    {
        case 0:         freq = 86400;   break;
        case 1:         freq = 86400*7; break;
        case 2:         freq = 86400*30; break;
        case 3:         freq = 86400*60; break;
        case 4:         freq = 86400*180; break;
        default:        break;
    }
    
    return freq;
}

static gint _sync_freq_to_index(guint freq)
{
    gint idx = 0;

    switch(freq)
    {
        case 86400:             idx = 0; break;
        case 86400*7:       idx = 1; break;
        case 86400*30:      idx = 2; break;
        case 86400*60:      idx = 3; break;
        case 86400*180:     idx = 4; break;
        default:            break;
    }
    
    return idx;
}

static void _get_next_sync_time(gchar *buf)
{
    time_t time;
    time_t freq;
    GTimeVal tv;
    
    if (nfui_combobox_get_cur_index((NFCOMBOBOX*)auto_timesync_obj) == SCHED_NTP)
    {
        freq = _index_to_sync_freq(nfui_combobox_get_cur_index((NFCOMBOBOX*)g_sync_freq_obj));
        dtdata.sync_freq = freq;
        time = dtdata.last_sync_time + freq;
        
        dtf_get_local_date_with_dformat(time, prvTransDateFormat_ver2((gint)(dtdata.dateFormat)), buf);
        if (freq > 86400)
            g_sprintf(buf, "%s %s", buf, nfui_combobox_get_value((NFCOMBOBOX*)timesync_obj));
        else
            g_sprintf(buf, "%s %s",lookup_string("EVERY DAY"), nfui_combobox_get_value((NFCOMBOBOX*)timesync_obj));
            
        g_message("[%s:%d]NEXT_SYNC_TIME : %s\n", __FUNCTION__, __LINE__, buf);
    }
    else
    {
        g_sprintf(buf, "No settings applied.");
        
        return;
    }

    return;
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
	if (sub_win) nftool_remove_waitbox(sub_win);
	sub_win = NULL;
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



static gboolean timeout_date_time_update(gpointer data)
{
	NFTIMELABEL* ti_obj;
	GTimeVal tv;
	GTimeVal tv_temp;
	gint m, s;

	memset(&tv, 0x00, sizeof(GTimeVal));
	memset(&tv_temp, 0x00, sizeof(GTimeVal));
	ti_obj = (NFTIMELABEL*)data;

	g_get_current_time(&tv);

	NFUTIL_THREADS_ENTER();

	nfui_nftimelabel_get_datetime(ti_obj, &tv_temp);

	if(tv.tv_sec != tv_temp.tv_sec)
	{
		nfui_nftimelabel_set_datetime_expose(ti_obj, &tv);
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
	nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
	nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);

	if (g_time_update) return -1;
	g_time_update = g_timeout_add(300, timeout_date_time_update, dt_obj);
	return 0;
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
    gchar buf[32];

	GTimeVal pre_time;
	GTimeVal post_time;

    memset(buf, 0x00, sizeof(buf));
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

            if (!ssm_is_itxdebug_id())
                dtdata.last_sync_time = post_time.tv_sec;
                
            _get_next_sync_time(buf);
            nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
            nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_date_format_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    GTimeVal tv;
    
	if (evt->type ==  NFEVENT_COMBOBOX_CHANGED)
	{
        memset(buf, 0x00, sizeof(buf));
        
		if (dtdata.dateFormat != nfui_combobox_get_cur_index(dateFormat_obj))
		{
			dtdata.dateFormat = nfui_combobox_get_cur_index(dateFormat_obj);
			nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);
            nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);

    	    nfui_nftimelabel_get_datetime(dt_obj, &tv);
    	    dtdata.last_sync_time = tv.tv_sec;
            _get_next_sync_time(buf);
            nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
            nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
            /*
			if (!g_time_update)
			{
				nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
				nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
			}
            */
		}
	}

	return FALSE;
}

static gboolean post_time_format_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    GTimeVal tv;
    
	if (evt->type ==  NFEVENT_COMBOBOX_CHANGED)
	{
        memset(buf, 0x00, sizeof(buf));
        
		if (dtdata.timeFormat != nfui_combobox_get_cur_index(timeFormat_obj))
		{
			dtdata.timeFormat = nfui_combobox_get_cur_index(timeFormat_obj);
			nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);
            nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);

    	    nfui_nftimelabel_get_datetime(dt_obj, &tv);
    	    dtdata.last_sync_time = tv.tv_sec;
            _get_next_sync_time(buf);
            nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
            nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
            /*
			if (!g_time_update)
			{
				nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
				nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
			}
            */
		}
	}

	return FALSE;
}

static gboolean sync_time(gpointer data)
{
	char *server_name;
	GTimeVal tv_temp;
	int ret;
    gchar buf[32];

	NFUTIL_THREADS_ENTER();

    memset(buf, 0x00, sizeof(buf));
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

    dtdata.last_sync_time = tv_temp.tv_sec;
    _get_next_sync_time(buf);
    nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
    nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);

	nftool_remove_waitbox((NFOBJECT*)data);

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean post_sync_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *wbox = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
		g_timeout_add(300, sync_time, (gpointer)wbox);
	}

	return FALSE;
}

static gboolean post_auto_time_sync_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    GTimeVal tv;
    gint sel;
    
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        memset(buf, 0x00, sizeof(buf));

        sel = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if (sel == SCHED_NTP)
        {
			nfui_nfobject_enable(g_sync_freq_obj);
			nfui_nfobject_enable(timesync_obj);
            if (org_dtdata.last_sync_time == 0)
            {
                nfui_nftimelabel_get_datetime((NFTIMELABEL*)dt_obj, &tv);
        	    dtdata.last_sync_time = tv.tv_sec;
            }
            else
            {
                dtdata.last_sync_time = org_dtdata.last_sync_time;
            }
        }
        else
        {
			nfui_nfobject_disable(g_sync_freq_obj);
			nfui_nfobject_disable(timesync_obj);
            dtdata.last_sync_time = 0;
        }
        _get_next_sync_time(buf);
        nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
        nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
		nfui_signal_emit(g_sync_freq_obj, GDK_EXPOSE, FALSE);
		nfui_signal_emit(timesync_obj, GDK_EXPOSE, FALSE);
	}		

	return FALSE;
}

static gboolean post_sync_freq_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    GTimeVal tv;
    
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        memset(buf, 0x00, sizeof(buf));

	    nfui_nftimelabel_get_datetime(dt_obj, &tv);
        dtdata.last_sync_time = tv.tv_sec;
        _get_next_sync_time(buf);
        nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
        nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
	}		

	return FALSE;
}

static gboolean post_sync_time_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    GTimeVal tv;
    
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        memset(buf, 0x00, sizeof(buf));

	    nfui_nftimelabel_get_datetime(dt_obj, &tv);
        dtdata.last_sync_time = tv.tv_sec;
        _get_next_sync_time(buf);
        nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
        nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
	}		

	return FALSE;
}

static gboolean post_time_zone_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    GTimeVal tv;
    
	if (evt->type ==  NFEVENT_COMBOBOX_CHANGED)
	{
		if (dtdata.timeZone != nfui_combobox_get_cur_index(timeZone_obj))
		{
            memset(buf, 0x00, sizeof(buf));
            
			dtdata.timeZone = nfui_combobox_get_cur_index(timeZone_obj);
			scm_apply_timezone(dtdata.timeZone, &nf_udd);

		    nfui_nftimelabel_get_datetime(dt_obj, &tv);
            dtdata.last_sync_time = tv.tv_sec;
            _get_next_sync_time(buf);
            nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
            nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);

            /*
			if (!g_time_update)
			{
				nfui_nftimelabel_refresh_datetime((NFTIMELABEL*)dt_obj);
				nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
			}
            */
		}
	}

	return FALSE;
}

static gboolean post_user_defined_dst_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        VW_User_Defined_Dst_Popup(g_curwnd, prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat, &nf_udd, dtdata.timeZone);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];
    
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		g_memmove(&dtdata, &org_dtdata, sizeof(DateTimeData));

		nfui_combobox_set_index((NFCOMBOBOX*)dateFormat_obj, dtdata.dateFormat);
		nfui_combobox_set_index((NFCOMBOBOX*)timeFormat_obj, dtdata.timeFormat);
		nfui_nflabel_set_text((NFLABEL*)timeServer_obj, dtdata.timeServer);
		nfui_combobox_set_index((NFCOMBOBOX*)auto_timesync_obj, get_auto_time_sync_option());
		nfui_combobox_set_index((NFCOMBOBOX*)g_sync_freq_obj, _sync_freq_to_index(dtdata.sync_freq));
		nfui_combobox_set_index((NFCOMBOBOX*)timesync_obj, dtdata.sync_time);
		nfui_combobox_set_index((NFCOMBOBOX*)timeZone_obj, dtdata.timeZone);
		nfui_combobox_set_index((NFCOMBOBOX*)dst_obj, dtdata.dst);
        dtdata.last_sync_time = org_dtdata.last_sync_time;

		scm_apply_timezone(dtdata.timeZone, NULL);	
		nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);		

        memset(buf, 0x00, sizeof(buf));
        _get_next_sync_time(buf);
        nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);

        nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);
		nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(dateFormat_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(timeFormat_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(timeServer_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(auto_timesync_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sync_freq_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(timesync_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(timeZone_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(dst_obj, GDK_EXPOSE, TRUE);

		scm_restore_system_time();
		_restart_timer();
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *wait_obj;
	mb_type ret = -1;
	guint is_changed = 0;
    gchar buf[32];
    GTimeVal tv;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		dtdata.dateFormat = nfui_combobox_get_cur_index(dateFormat_obj);
		dtdata.timeFormat = nfui_combobox_get_cur_index(timeFormat_obj);
		g_stpcpy(dtdata.timeServer, nfui_nflabel_get_text((NFLABEL*)timeServer_obj));
		if(nfui_combobox_get_cur_index(auto_timesync_obj) == ALL_NTP_OFF) {
			dtdata.auto_timesync = 0;
			dtdata.time_sync_off = 1;
		} else if(nfui_combobox_get_cur_index(auto_timesync_obj) == SMART_NTP) {
			dtdata.auto_timesync = 0;
			dtdata.time_sync_off = 0;
		} else if(nfui_combobox_get_cur_index(auto_timesync_obj) == SCHED_NTP) {
			dtdata.auto_timesync = 1;
			dtdata.time_sync_off = 0;
		}
		dtdata.sync_freq = _index_to_sync_freq(nfui_combobox_get_cur_index(g_sync_freq_obj));
		dtdata.sync_time = nfui_combobox_get_cur_index(timesync_obj);
		dtdata.timeZone = nfui_combobox_get_cur_index(timeZone_obj);
		dtdata.dst = nfui_combobox_get_cur_index(dst_obj);

		if(memcmp(&org_dtdata, &dtdata, sizeof(DateTimeData)))
		{
			scm_put_log(CHANGE_SYS_TIME, 0, 0);

			g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));
			DAL_set_dateTime_data(&dtdata);
			VW_SetupSystem_set_changeflag(1);
			is_changed = 1;
		}

		if (scm_is_set_new_time()) 
			{
            memset(buf, 0x00, sizeof(buf));

			if (scm_is_past_time_set())
			{
        		ret = nftool_mbox(g_curwnd, "WARNING", "From now until the time you want to change the video will be deleted,\nhowever, the data will be backed up separately.\nAre you sure to continue?\n(* The data can be seen in Reserved Data Management)", NFTOOL_MB_OKCANCEL);		
        		if (ret == NFTOOL_MB_CANCEL) 
        		{
					scm_restore_system_time();
					_restart_timer();
        		    
                    g_get_current_time(&tv);
                    dtdata.last_sync_time = tv.tv_sec;
                    _get_next_sync_time(buf);
                    nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
                    nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);

                    g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));
                    DAL_set_dateTime_data(&dtdata);
                    
					return FALSE;
				}
			}

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK")) 
		{
				scm_restore_system_time();
				_restart_timer();

                g_get_current_time(&tv);
                dtdata.last_sync_time = tv.tv_sec;
                _get_next_sync_time(buf);
                nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);
                nfui_signal_emit(g_next_time_obj, GDK_EXPOSE, FALSE);

                g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));
                DAL_set_dateTime_data(&dtdata);

				return FALSE;
			}
				smt_set_service(SMT_TIME_CHANGE);
				scm_apply_time(IRET_SCM_CHANGE_SYSTEM_TIME);
				vw_open_datetime_waitpopup(g_curwnd);
			}
        
		scm_init_timesync();	

		if(is_changed)	nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
	}

	return FALSE;
}


static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}

		VW_DateTime_tab_out_handler();
		VW_SetupSystem_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	CMM_MESSAGE_T *pmsg;

	switch (evt->type) {
	case GDK_DELETE:
		uxm_unreg_imsg_event(obj, IRET_SCM_CHANGE_SYSTEM_TIME);
		uxm_unreg_imsg_event(obj, IRET_SCM_CHANGE_SYSTEM_TIME2);
		g_curwnd = 0;
		break;

	case IRET_SCM_CHANGE_SYSTEM_TIME:
	case IRET_SCM_CHANGE_SYSTEM_TIME2:
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
	}

    if (evt->type == IRET_SCM_CHANGE_SYSTEM_TIME2) {
        gtk_main_quit();
    }

	return FALSE;
}

int get_auto_time_sync_option()
{
	gint result = SMART_NTP;
	if(dtdata.time_sync_off == 1) {
		result = ALL_NTP_OFF;
	} else if(dtdata.time_sync_off == 0 && dtdata.auto_timesync == 0) {
		result = SMART_NTP;
	} else if(dtdata.time_sync_off == 0 && dtdata.auto_timesync == 1) {
		result = SCHED_NTP;
	}
	
	return result;
}

void VW_Init_SysDateTime_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;

	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *sync_img[NFOBJECT_STATE_COUNT];
	gchar *strTimezone[50];
	gchar buf[64];
	gint m, s;
	GTimeVal tv;

	const gchar *strDateFormat[NUM_DATE_FORMATS] = {
		"YYYY/MM/DD",
		"MM/DD/YYYY",
		"DD/MM/YYYY"
	};

	const gchar *strTimeFormat[NUM_TIME_MODES] = {
		"24 HOUR",
		"AM / PM"
	};

	const gchar *strSyncFreq[] = {
	    "EVERY DAY",
	    "7 DAYS",
	    "30 DAYS",
	    "60 DAYS",
	    "180 DAYS"
	};
	
	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strTimeSync[] = {"OFF", "SMART NTP SYNC", "SCHEDULED NTP SYNC"};
	char synctime[24][12];
	char *psynctime[24];

	guint zone_count = 0;
	guint tm_size_w, tm_size_h;		// set time image size.
	guint sy_size_w, sy_size_h;		// set time image size.
	guint ck_size_w, ck_size_h;		// check box image size.
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);

	for (i = 0; i < 24; ++i)
	{
		sprintf(synctime[i], "%02d:00", i);
		psynctime[i] = synctime[i];
	}

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
    memset(&nf_udd, 0x00, sizeof(NF_TIME_UDD));
	memset(&dtdata, 0x00, sizeof(DateTimeData));
	memset(&org_dtdata, 0x00, sizeof(DateTimeData));

	DAL_get_dateTime_data(&dtdata);
	DAL_get_dateTime_format(&dformat, &tformat);

	g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	//nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	//nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
////////////////////////////////////////////////////
//	TIME SETTING
//
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "TIME SETTING");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, DT_SETUP_TEXT_MARGIN);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_SET_TITLE_X, DT_TIME_SET_TITLE_Y);

// <---- DATE TIME
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DATE/TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DATE_TIME_LABEL_X, DT_DATE_TIME_LABEL_Y);

	dt_obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)dt_obj, COLOR_IDX(129));
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)dt_obj, COLOR_IDX(128));
	nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)dt_obj, DT_SETUP_CELL_W/*-tm_size_w*/, DT_SETUP_LABEL_H);
	nfui_nfobject_use_focus(dt_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(dt_obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, dt_obj, DT_DATE_TIME_LABEL_X+DT_SETUP_LABEL_W, DT_DATE_TIME_LABEL_Y);
	nfui_regi_post_event_callback(dt_obj, post_timelabel_event_handler);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
	nfui_nfobject_set_size(obj, (guint)tm_size_w, (guint)tm_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DATE_TIME_LABEL_X+DT_SETUP_LABEL_W+DT_SETUP_CELL_W+2 /*-tm_size_w*/, DT_DATE_TIME_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_datetime_event_handler);

// <---- DATE FORMAT
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DATE FORMAT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DATE_FORMAT_LABEL_X, DT_DATE_FORMAT_LABEL_Y);

	obj = nfui_combobox_new(strDateFormat, NUM_DATE_FORMATS, (gint)(dtdata.dateFormat));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DATE_FORMAT_LABEL_X+DT_SETUP_LABEL_W, DT_DATE_FORMAT_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_date_format_event_handler);
	dateFormat_obj = obj;

// <---- TIME FORMAT
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME FORMAT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_FORMAT_LABEL_X, DT_TIME_FORMAT_LABEL_Y);

	obj = nfui_combobox_new(strTimeFormat, NUM_TIME_MODES, (gint)(dtdata.timeFormat));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_FORMAT_LABEL_X+DT_SETUP_LABEL_W, DT_TIME_FORMAT_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_time_format_event_handler);
	timeFormat_obj = obj;

////////////////////////////////////////////////////
//	NETWORK TIME SYNC
//
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "NETWORK TIME SYNC");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, DT_SETUP_TEXT_MARGIN);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_NET_TIMESYNC_TITLE_X, DT_NET_TIMESYNC_TITLE_Y);

// <---- TIME SERVER
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_SERVER_LABEL_X, DT_TIME_SERVER_LABEL_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(dtdata.timeServer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

	timeServer_obj = obj;
	lb_server = obj;

	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_SERVER_LABEL_X+DT_SETUP_LABEL_W, DT_TIME_SERVER_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_label_event_handler);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), sync_img);
	nfui_nfobject_set_size(obj, (guint)sy_size_w, (guint)sy_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DATE_TIME_LABEL_X+DT_SETUP_LABEL_W+DT_SETUP_CELL_W+2 /*-tm_size_w*/, DT_TIME_SERVER_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_sync_event_handler);

// <---- AUTO TIME SYNC
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUTO TIME SYNC", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_AUTO_TIME_SYNC_LABEL_X, DT_AUTO_TIME_SYNC_LABEL_Y);

	obj = nfui_combobox_new(strTimeSync, 3, get_auto_time_sync_option());
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_AUTO_TIME_SYNC_LABEL_X+DT_SETUP_LABEL_W, DT_AUTO_TIME_SYNC_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_auto_time_sync_event_handler);
	auto_timesync_obj = obj;

// <---- SYNC FREQUENCY
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYNC CYCLE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_SYNC_FREQUENCY_LABEL_X, DT_SYNC_FREQUENCY_LABEL_Y);

	obj = nfui_combobox_new(strSyncFreq, 5, _sync_freq_to_index(dtdata.sync_freq));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	if(get_auto_time_sync_option() == ALL_NTP_OFF || get_auto_time_sync_option() == SMART_NTP) nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_SYNC_FREQUENCY_LABEL_X+DT_SETUP_LABEL_W, DT_SYNC_FREQUENCY_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_sync_freq_event_handler);
	g_sync_freq_obj = obj;
	
// <---- SYNC TIME
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYNC TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_SYNC_TIME_LABEL_X, DT_SYNC_TIME_LABEL_Y);

	obj = nfui_combobox_new(psynctime, 24, (gint)dtdata.sync_time);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	if(get_auto_time_sync_option() == ALL_NTP_OFF || get_auto_time_sync_option() == SMART_NTP) nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_AUTO_TIME_SYNC_LABEL_X+DT_SETUP_LABEL_W, DT_SYNC_TIME_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_sync_time_event_handler);
	timesync_obj = obj;

// <---- NEXT SYNC TIME
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SCHEDULED SYNC TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_NEXT_SYNC_TIME_LABEL_X, DT_NEXT_SYNC_TIME_LABEL_Y);

    memset(buf, 0x00, sizeof(buf));
    _get_next_sync_time(buf);
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_NEXT_SYNC_TIME_LABEL_X+DT_SETUP_LABEL_W, DT_NEXT_SYNC_TIME_LABEL_Y);
    g_next_time_obj = obj;
    
// <---- RUN AS TIME SERVER
	obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(obj, &ck_size_w, &ck_size_h);
	//nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_RUN_SERVER_CHKBOX_X, DT_RUN_SERVER_CHKBOX_Y+(DT_SETUP_LABEL_H-ck_size_h)/2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RUN AS TIME SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_RUN_SERVER_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	//nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_RUN_SERVER_CHKBOX_X+ck_size_w+4, DT_RUN_SERVER_CHKBOX_Y);

////////////////////////////////////////////////////
//	TIMEZONE / DST
//
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "TIMEZONE / DST");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, DT_SETUP_TEXT_MARGIN);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TZ_DST_TITLE_X, DT_TZ_DST_TITLE_Y);

// <---- TIME/ZONE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMEZONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_ZONE_LABEL_X, DT_TIME_ZONE_LABEL_Y);

	zone_count = nf_zoneinfo_get_count();

	for(i=0; i<zone_count; i++)
		strTimezone[i] = nf_zoneinfo_get_string((gint)i);

	obj = nfui_combobox_new(strTimezone, zone_count, (gint)(dtdata.timeZone));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_ZONE_LABEL_X+DT_SETUP_LABEL_W, DT_TIME_ZONE_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_time_zone_event_handler);
	timeZone_obj = obj;

// <---- DST
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, DT_SETUP_LABEL_W, DT_SETUP_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DST_LABEL_X, DT_DST_LABEL_Y);

	obj = nfui_combobox_new(strOffOn, 2, (gint)(dtdata.dst));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, DT_SETUP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_DST_LABEL_X+DT_SETUP_LABEL_W, DT_DST_LABEL_Y);
	dst_obj = obj;

// <---- USER DEFINED DST
	obj = nftool_normal_button_create_type3("USER DEFINED DST", 240);
	//nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DT_TIME_ZONE_LABEL_X+DT_SETUP_LABEL_W+DT_SETUP_CELL_W+2, DT_DST_LABEL_Y);
	nfui_regi_post_event_callback(obj, post_user_defined_dst_event_handler);

// <---- CANCEL, APPLY, CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	//nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	cancel_btn = obj;

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	//nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	//nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	g_time_update = 0;
	g_time_update = g_timeout_add(300, timeout_date_time_update, dt_obj);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	uxm_reg_imsg_event(parent, IRET_SCM_CHANGE_SYSTEM_TIME);
	uxm_reg_imsg_event(parent, IRET_SCM_CHANGE_SYSTEM_TIME2);
}

gboolean VW_DateTime_tab_out_handler()
{
	mb_type ret = 0;
	guint i;
    gchar buf[32];
    GTimeVal tv;

	dtdata.dateFormat = nfui_combobox_get_cur_index(dateFormat_obj);
	dtdata.timeFormat = nfui_combobox_get_cur_index(timeFormat_obj);
	g_stpcpy(dtdata.timeServer, nfui_nflabel_get_text((NFLABEL*)timeServer_obj));
	if(nfui_combobox_get_cur_index(auto_timesync_obj) == ALL_NTP_OFF) {
		dtdata.auto_timesync = 0;
		dtdata.time_sync_off = 1;
	} else if(nfui_combobox_get_cur_index(auto_timesync_obj) == SMART_NTP) {
		dtdata.auto_timesync = 0;
		dtdata.time_sync_off = 0;
	} else if(nfui_combobox_get_cur_index(auto_timesync_obj) == SCHED_NTP) {
		dtdata.auto_timesync = 1;
		dtdata.time_sync_off = 0;
	}
	dtdata.sync_freq = _index_to_sync_freq(nfui_combobox_get_cur_index(g_sync_freq_obj));
	dtdata.sync_time = nfui_combobox_get_cur_index(timesync_obj);
	dtdata.timeZone = nfui_combobox_get_cur_index(timeZone_obj);
	dtdata.dst = nfui_combobox_get_cur_index(dst_obj);

	if(memcmp(&org_dtdata, &dtdata, sizeof(DateTimeData)) || scm_is_set_new_time())
	{
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?",
								NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK)
		{
			g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));
			DAL_set_dateTime_data(&dtdata);
			VW_SetupSystem_set_changeflag(1);

			if (scm_is_set_new_time()) 
			{
                memset(buf, 0x00, sizeof(buf));
            
            	if (scm_is_past_time_set()) 
    			{
            		ret = nftool_mbox(g_curwnd, "WARNING", "From now until the time you want to change the video will be deleted,\nhowever, the data will be backed up separately.\nAre you sure to continue?\n(* The data can be seen in Reserved Data Management)", NFTOOL_MB_OKCANCEL);		
            		if (ret == NFTOOL_MB_CANCEL) 
				{
					scm_restore_system_time();
					_restart_timer();
        		    
                        g_get_current_time(&tv);
                        dtdata.last_sync_time = tv.tv_sec;
                        _get_next_sync_time(buf);
                        nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);

                        g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));
                        DAL_set_dateTime_data(&dtdata);
                        
					return FALSE;
				}
    			}

				if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK"))
				{
						scm_restore_system_time();
						_restart_timer();
        		    
                    g_get_current_time(&tv);
                    dtdata.last_sync_time = tv.tv_sec;
                    _get_next_sync_time(buf);
                    nfui_nflabel_set_text((NFLABEL*)g_next_time_obj, buf);

                    g_memmove(&org_dtdata, &dtdata, sizeof(DateTimeData));
                    DAL_set_dateTime_data(&dtdata);
                        
						return FALSE;
					}

				smt_set_service(SMT_TIME_CHANGE);
    			scm_apply_time(IRET_SCM_CHANGE_SYSTEM_TIME);		
    			vw_open_datetime_waitpopup(g_curwnd);
			}
			scm_init_timesync();
		}
		else if(ret == NFTOOL_MB_CANCEL)
		{
			g_memmove(&dtdata, &org_dtdata, sizeof(DateTimeData));

			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)dateFormat_obj, dtdata.dateFormat);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)timeFormat_obj, dtdata.timeFormat);
			nfui_nflabel_set_text((NFLABEL*)timeServer_obj, dtdata.timeServer);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)auto_timesync_obj, get_auto_time_sync_option());
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)timesync_obj, dtdata.sync_time);
			nfui_combobox_set_index((NFCOMBOBOX*)g_sync_freq_obj, dtdata.sync_freq);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)timeZone_obj, dtdata.timeZone);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)dst_obj, dtdata.dst);

			scm_apply_timezone(dtdata.timeZone, NULL);
			nfui_nftimelabel_set_mode((NFTIMELABEL*)dt_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), dtdata.timeFormat+1);
			scm_restore_system_time();
			_restart_timer();
		}
	}

	return FALSE;
}

