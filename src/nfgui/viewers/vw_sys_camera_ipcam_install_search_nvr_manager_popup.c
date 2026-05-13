#include "nf_afx.h"
#include "nf_api_openmode.h"

#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"
#include "ssm.h"
#include "nvm.h"
#include "xmm.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nftable.h"
#include "objects/nftimelabel.h"
#include "objects/nftab.h"

#include "vw.h"
#include "vw_vkeyboard.h"


#define NM_WINDOW_H                 (650)
#define NM_WINDOW_W                 (1480)
#define NM_TBL_ROW_CNT              (10)

#define NM_TAB_CNT                  (1)

enum {
    NM_TBL_COL_HOSTNAME,
    NM_TBL_COL_ID,
    NM_TBL_COL_PW,
    NM_TBL_COL_STATUS,
    NM_TBL_COL_TIME,
    
    NM_TBL_COL_CNT
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_acc_tbl = NULL;
static NFOBJECT *g_ts_tbl = NULL;

static guint g_time_update = 0;


static gboolean _timeout_date_time_update(gpointer data)
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

static gint _trans_conn_status(XMM_CONN_STATUS status, gchar *buf)
{
    switch(status)
    {
        case XMM_CONN_LOGIN_FAIL:
            strcpy(buf, "LOGIN FAIL");
        break;

        case XMM_CONN_CONN_FAIL:
            strcpy(buf, "CONNECTION FAIL");
        break;

        case XMM_CONN_CONN_SUCC:
            strcpy(buf, "CONNECTED");
        break;

        case XMM_CONN_REQUEST:
        case XMM_CONN_REQ_SUCC:
            strcpy(buf, "LOGGING IN...");
        break;
        
        case XMM_CONN_NONE:
            strcpy(buf, "");
        break;    

        default:
            strcpy(buf, "WAITING");
        break;
    }

    return 0;
}

static gint _get_ndata_idx(gint page, gint row)
{
    return page * NM_TBL_ROW_CNT + row + 1;
}

static void _disable_login(gint row)
{
    NFOBJECT *child = NULL;   

	child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 4, row);
	
	nfui_nfobject_disable(child);
	nfui_signal_emit(child, GDK_EXPOSE, TRUE);
}

static void _enable_login(gint row, gint expose)
{
    NFOBJECT *child = NULL;   

	child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 4, row);
	
	nfui_nfobject_enable(child);
	if (expose) nfui_signal_emit(child, GDK_EXPOSE, TRUE);
}

static gint _update_label_obj(NFOBJECT *obj, gchar *text, gint expose)
{
    if (!obj) return -1;
    if (!text) return -1;

    nfui_nflabel_set_text((NFLABEL*)obj, text);
    if (expose) nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _update_device_id(NFOBJECT *obj, gint idx, gint expose)
{
    NVR_DATA_T *ndata;

    ndata = nvm_get_nvr_data(idx);
    if (!ndata) return -1;

    _update_label_obj(obj, ndata->hostname, expose);

    ifree(ndata);

    return 0;
}

static gint _update_acc_id(NFOBJECT *obj, gint idx, gint expose)
{
    NVR_DATA_T *ndata;
    XMM_SESSION_INFO_T *sinfo;

    ndata = nvm_get_nvr_data(idx);
    if (!ndata) return -1;

    sinfo = xmm_get_session_info(ndata->hostname, ndata->http_port);
    if (!sinfo) {
        ifree(ndata);
        return -1;
    }

    _update_label_obj(obj, sinfo->id, expose);

    ifree(ndata);
    ifree(sinfo);

    return 0;
}

static gint _update_acc_pw(NFOBJECT *obj, gint idx, gint expose)
{
    NVR_DATA_T *ndata;
    XMM_SESSION_INFO_T *sinfo;

    ndata = nvm_get_nvr_data(idx);
    if (!ndata) return -1;

    sinfo = xmm_get_session_info(ndata->hostname, ndata->http_port);
    if (!sinfo) {
        ifree(ndata);
        return -1;
    }

    _update_label_obj(obj, sinfo->pw, expose);

    ifree(ndata);
    ifree(sinfo);

    return 0;
}

static gint _update_acc_status(NFOBJECT *obj, gint idx, gint expose)
{
    NVR_DATA_T *ndata;
    XMM_SESSION_INFO_T *sinfo;
    gchar buf[64];

    ndata = nvm_get_nvr_data(idx);
    if (!ndata) return -1;

    sinfo = xmm_get_session_info(ndata->hostname, ndata->http_port);
    if (!sinfo) {
        ifree(ndata);
        return -1;
    }

    memset(buf, 0x00, sizeof(buf));
    _trans_conn_status(sinfo->conn, buf);
    
    _update_label_obj(obj, buf, expose);

    ifree(ndata);
    ifree(sinfo);

    return 0;
}

static gint _update_acc_login(gint row, gint idx, gint expose)
{
    NVR_DATA_T *ndata;
    XMM_SESSION_INFO_T *sinfo;
    gchar buf[64];

    ndata = nvm_get_nvr_data(idx);
    if (!ndata) return -1;

    sinfo = xmm_get_session_info(ndata->hostname, ndata->http_port);
    if (!sinfo) {
        ifree(ndata);
        return -1;
    }

    memset(buf, 0x00, sizeof(buf));
    
    switch(sinfo->conn)
    {
        case XMM_CONN_REQUEST:
        case XMM_CONN_REQ_SUCC:
            _disable_login(row);
        break;

        default:
            _enable_login(row, expose);
        break;
    }

    ifree(ndata);
    ifree(sinfo);

    return 0;
}

static gint _update_access_table(gint expose)
{
    NFOBJECT *child;
    gint i, j;
    gint d_idx = 0;

    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < NM_TBL_ROW_CNT; j++)
        {
            d_idx = _get_ndata_idx(i, j);
            
            child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 0, j);
            _update_device_id(child, d_idx, expose);

            child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 1, j);
            _update_acc_id(child, d_idx, expose);

            child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 2, j);
            _update_acc_pw(child, d_idx, expose);

            child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 3, j);
            _update_acc_status(child, d_idx, expose);
        }
    }

    return 0;
}

static gint _update_timesync_table(gint expose)
{
    NFOBJECT *child;
    gint i, j;
    gint d_idx = 0;

    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < NM_TBL_ROW_CNT; j++)
        {
            d_idx = _get_ndata_idx(i, j);
            
            child = nfui_nftable_get_child((NFTABLE*)g_ts_tbl, 0, j);
            _update_device_id(child, d_idx, expose);

            child = nfui_nftable_get_child((NFTABLE*)g_ts_tbl, 1, j);

            child = nfui_nftable_get_child((NFTABLE*)g_ts_tbl, 2, j);
        }
    }

    return 0;
}

static gboolean post_id_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NVR_DATA_T *ndata = NULL;
    NFOBJECT *child;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint i, j;
	gint d_idx;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *id;
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;

		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		id = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 16, VKEY_ALPHANUMERIC);
		if (!id) return FALSE;

		if (strlen(id) > 0)
		{
            nfui_nflabel_set_text((NFLABEL*)obj, id);
    		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            for (i = 0; i < 1; i++) {
                for (j = 0; j < NM_TBL_ROW_CNT; j++)
                {
                    child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 1, j);
                    
                    if (obj == child) 
                    {
                		d_idx = _get_ndata_idx(i, j);
                		
                        ndata = nvm_get_nvr_data(d_idx);
                        if (!ndata) return -1;

                        xmm_update_nvr_info(ndata->hostname, ndata->http_port, id, NULL);
                        ifree(ndata);
                    }
                }
            }
		}

		ifree(id);
		id = NULL;
	}

	return FALSE;
}

static gboolean post_pw_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NVR_DATA_T *ndata = NULL;
    NFOBJECT *child;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint i, j;
	gint d_idx;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *pw;
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;

		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 16, VKEY_PASSWORD);
		if (!pw) return FALSE;

		if (strlen(pw) > 0)
		{
            nfui_nflabel_set_text((NFLABEL*)obj, pw);
    		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    		
            for (i = 0; i < 1; i++) {
                for (j = 0; j < NM_TBL_ROW_CNT; j++)
                {
                    child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 2, j);
                    
                    if (obj == child) 
                    {
                		d_idx = _get_ndata_idx(i, j);
                		
                        ndata = nvm_get_nvr_data(d_idx);
                        if (!ndata) return -1;

                        xmm_update_nvr_info(ndata->hostname, ndata->http_port, NULL, pw);
                        ifree(ndata);
                    }
                }
            }
		}

		ifree(pw);
		pw = NULL;
	}

	return FALSE;
}

static gboolean post_time_sync_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *child = NULL;
    NFOBJECT *status = NULL;
    gint i;
    gint d_idx;
    
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
			
        for (i = 0; i < NM_TBL_ROW_CNT; i++) {
    		child = nfui_nftable_get_child((NFTABLE*)g_ts_tbl, 3, i);
    		if (child == obj) break;
		}

		if (i == NM_TBL_ROW_CNT) return FALSE;

		d_idx = _get_ndata_idx(0, i);
		nvm_time_sync(d_idx);	
	}

	return FALSE;
}

static gboolean post_login_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *child = NULL;
    NFOBJECT *status = NULL;
    gint i;
    gint d_idx;
    gint ret = 0;
    
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        for (i = 0; i < NM_TBL_ROW_CNT; i++) {
    		child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 4, i);
    		if (child == obj) break;
		}

		if (i == NM_TBL_ROW_CNT) return FALSE;

		d_idx = _get_ndata_idx(0, i);
		ret = nvm_connect_nvr(d_idx);
		if (ret == -1) return FALSE;

        status = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 3, i);
        _update_acc_status(status, d_idx, TRUE);
        _disable_login(i);
	}

	return FALSE;
}

static gboolean post_login_all_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *child = NULL;
    NFOBJECT *status = NULL;
    gint i;
    gint d_idx;
    gint ret = 0;
    
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        for (i = 0; i < NM_TBL_ROW_CNT; i++) 
        {
    		d_idx = _get_ndata_idx(0, i);
    		ret = nvm_connect_nvr(d_idx);
    		if (ret == -1) continue;

            status = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 3, i);
            _update_acc_status(status, d_idx, TRUE);
            _disable_login(i);
		}

		if (i == NM_TBL_ROW_CNT) return FALSE;
	}

	return FALSE;
}

static gboolean post_close_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint old_page;
    gint active;
    
	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		old_page = nfui_nftab_get_old_page((NFTAB*)obj);

    	if(cur_page == old_page) return FALSE;
		switch(cur_page) {
			case 0:	// 
				break;

			case 1:	// 
				break;

			default:
				break;
		}

	}
	return FALSE;
}

static gboolean post_acc_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *child = NULL;
    gint d_idx;
    gint i, j;
    gint params;
    
    if (evt->type == IRET_NVM_UPDATE_CONN_STATUS)
    {
        params = ((CMM_MESSAGE_T*)data)->param;
        
        for (i = 0; i < 1; i++) {
            for (j = 0; j < NM_TBL_ROW_CNT; j++)
            {
                child = nfui_nftable_get_child((NFTABLE*)g_acc_tbl, 3, j);
                
        		d_idx = _get_ndata_idx(i, j);
        		if (d_idx != params) continue;
        		
                _update_acc_status(child, d_idx, TRUE);
                _update_acc_login(j, d_idx, TRUE);
            }
        }
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, IRET_NVM_UPDATE_CONN_STATUS);
    }
    
    return FALSE;
}

static gboolean pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable;
		GdkColor line_color = UX_COLOR(392);
		GdkGC *line_gc;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		line_gc = nfui_nfobject_get_gc((NFOBJECT*)obj);
		gdk_gc_set_rgb_fg_color(line_gc, &line_color);

		gdk_gc_set_line_attributes(line_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);

		gdk_draw_rectangle(drawable,
				line_gc,
				FALSE,
				obj->x, obj->y,
				obj->width, obj->height);

		nfui_nfobject_gc_unref(line_gc);
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }
    

	return FALSE;
}

static gboolean post_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
	
		g_curwnd = 0;

		if (g_time_update) {
		    g_source_remove(g_time_update);
		    g_time_update = 0;
		}
		gtk_main_quit();
	}

	return FALSE;
}

static gint _init_time_sync_page(NFOBJECT *parent)
{
	NFOBJECT *fixed = parent;
	NFOBJECT *tbl = NULL;
	NFOBJECT *obj = NULL;
	NFOBJECT *title_tbl = NULL;

	NVR_DATA_T *ndata = NULL;
	XMM_SESSION_INFO_T *sinfo = NULL;

	gint i = 0;
    gint pos_x, pos_y;
    gint size_w, size_h;
    guint cell_width[4] = {300, 300, 300, 200};
    gchar buf[32];

    pos_x = 10;
    pos_y = 10;

	title_tbl = nfui_nftable_new(4, 1, 2, 2, cell_width, 42);
    nfui_nfobject_modify_bg(title_tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(title_tbl);
    nfui_nffixed_put((NFFIXED*)fixed, title_tbl, pos_x, pos_y);

    //hostname
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEVICE ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 0, 0);
    
    //status
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 1, 0);        
    
    //status
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LAST SYNC TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 2, 0);        

	obj = nftool_normal_button_create_type1("ALL", 200);
	nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 3, 0);        
//	nfui_regi_post_event_callback(obj, post_time_sync_event_cb);

	pos_y += 42 + 2;

	tbl = nfui_nftable_new(4, NM_TBL_ROW_CNT, 2, 2, cell_width, 42);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)fixed, tbl, pos_x, pos_y);
    g_ts_tbl = tbl;

    for (i = 0; i < NM_TBL_ROW_CNT; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 0, i);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 1, i);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 2, i);

    	obj = nftool_normal_button_create_type1("TIME SYNC", 200);
    	nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 3, i);        
    	nfui_regi_post_event_callback(obj, post_time_sync_event_cb);
    }

    _update_timesync_table(TRUE);

    return 0;
}

static gint _init_access_page(NFOBJECT *parent)
{
	NFOBJECT *fixed = parent;
	NFOBJECT *tbl = NULL;
	NFOBJECT *obj = NULL;
	NFOBJECT *title_tbl = NULL;

	NVR_DATA_T *ndata = NULL;
	XMM_SESSION_INFO_T *sinfo = NULL;

	gint i = 0;
    gint pos_x, pos_y;
    gint size_w, size_h;
    guint cell_width[5] = {250, 250, 250, 300, 150};
    gchar buf[32];

    pos_x = 10;
    pos_y = 10;

	title_tbl = nfui_nftable_new(5, 1, 2, 2, cell_width, 42);
    nfui_nfobject_modify_bg(title_tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(title_tbl);
    nfui_nffixed_put((NFFIXED*)fixed, title_tbl, pos_x, pos_y);

    //hostname
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 0, 0);
    
    //id
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 1, 0);
    
    //pw
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 2, 0);
    
    //status
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 3, 0);        

	obj = nftool_normal_button_create_type1("ALL", 150);
//	nfui_nfobject_show(obj);
    nfui_nftable_attach(title_tbl, obj, 4, 0);        
//	nfui_regi_post_event_callback(obj, post_login_all_event_cb);

	pos_y += 42 + 2;

	tbl = nfui_nftable_new(5, NM_TBL_ROW_CNT, 2, 2, cell_width, 42);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)fixed, tbl, pos_x, pos_y);
    g_acc_tbl = tbl;

    for (i = 0; i < NM_TBL_ROW_CNT; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 0, i);
        
        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 1, i);
        nfui_regi_post_event_callback(obj, post_id_event_handler);
        
        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 2, i);
        nfui_regi_post_event_callback(obj, post_pw_event_handler);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 3, i);

    	obj = nftool_normal_button_create_popup_type1("LOG IN", 150);
    	nfui_nfobject_show(obj);
        nfui_nftable_attach(tbl, obj, 4, i);        
    	nfui_regi_post_event_callback(obj, post_login_event_handler);
    }

    _update_access_table(FALSE);

    return 0;
}

gint VW_Camera_Install_NvrManager_Popup_Create(NFWINDOW *parent)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *tab = NULL;
	NFOBJECT *tab_page[NM_TAB_CNT];
	NFOBJECT *obj = NULL;
	NFOBJECT *close_btn = NULL;

	gint i = 0;
    gint pos_x, pos_y;
    gint size_w, size_h;
    gchar buf[32];
    
	gchar *strTitle[] = {
				"CONNECTION", 
				"TIME SYNC"
	};
    
    gchar *strImage_h[2] =  {
			(MKB_IMG_TAB_POP_DIR_V_N_208), 
			(MKB_IMG_TAB_POP_DIR_V_S_208)
	};
	
	guint colidx[3] = {COLOR_IDX(287), COLOR_IDX(289), COLOR_IDX(288)};

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (DISPLAY_ACTIVE_WIDTH - NM_WINDOW_W) / 2, (DISPLAY_ACTIVE_HEIGHT - NM_WINDOW_H) / 2, NM_WINDOW_W, NM_WINDOW_H);
	nfui_nfwindow_use_double_buffer(win);
	nfui_regi_post_event_callback(win, post_win_event_cb);
	g_curwnd = win;

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, NM_WINDOW_W, NM_WINDOW_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MULTIPLE RECORDER CONTROL PANEL", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, NM_WINDOW_W - 8, 36);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	pos_x = 20;
	pos_y = 60;

    tab = nfui_nftab_new(NM_TAB_CNT, strImage_h, 208, 43, NFTAB_DIR_V, strTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)tab, 10);
	nfui_nfobject_show(tab);
	nfui_nffixed_put((NFFIXED*)fixed, tab, pos_x, pos_y);
	nfui_regi_pre_event_callback(tab, pre_subtab_event_handler);

    pos_x += 208;
    
	for(i = 0; i < NM_TAB_CNT; i++) {
		tab_page[i] = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], NM_WINDOW_W - 40 -208, NM_WINDOW_H-140);
		nfui_nfobject_modify_bg(tab_page[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nftab_regi_page((NFTAB*)tab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)fixed, tab_page[i], pos_x, pos_y);

		nfui_regi_pre_event_callback(tab_page[i], pre_page_event_cb);
	}
	nfui_nfobject_show(tab_page[0]);

	_init_access_page(tab_page[0]);
//	_init_time_sync_page(tab_page[1]);

	nfui_regi_post_event_callback(tab_page[0], post_acc_page_event_handler);
	
	uxm_reg_imsg_event(tab_page[0], IRET_NVM_UPDATE_CONN_STATUS);
	uxm_monitor_on_imsg_event(tab_page[0], IRET_NVM_UPDATE_CONN_STATUS);

	obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(230));
//	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, 350, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, NM_WINDOW_H-55);
	
//	g_time_update = g_timeout_add(300, _timeout_date_time_update, obj);

	obj = nftool_normal_button_create_type1("CLOSE", 200);
	nfui_regi_post_event_callback(obj, post_close_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, NM_WINDOW_W - 20 - 200, NM_WINDOW_H-55);
	close_btn = obj;

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);
    
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(close_btn, TRUE);

	nfui_page_open(PGID_CAMERA_INSTALL_NVR_MANAGE_POPUP, g_curwnd, ssm_get_cur_id(NULL));

    gtk_main();
    
	nfui_page_close(PGID_CAMERA_INSTALL_NVR_MANAGE_POPUP, g_curwnd);

	return 0;
}
