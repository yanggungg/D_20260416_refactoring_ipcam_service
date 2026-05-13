#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbargraph.h"
#include "objects/nfpiechart.h"

#include "vw_deeplearning_statistic_popup.h"
#include "ix_mem.h"
#include "scm.h"
#include "iva_cntr.h"


static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_ch_all_obj;
static NFOBJECT *g_ch_obj[GUI_CHANNEL_CNT];

static NFOBJECT *g_bargraph_obj;

static NFOBJECT *g_piechart_obj;
static NFOBJECT *g_piechart_day[7];
static NFOBJECT *g_piechart_val[7];

static IVACR_DATA_T g_ivcacr_data;


static guint _get_check_chmask()
{
	guint check_mask = 0;
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
        if (!nfui_nfobject_is_disabled(g_ch_obj[i])) {
	        if (nfui_check_button_get_active((NFCHECKBUTTON*)g_ch_obj[i])) check_mask |= 1 << i;
        }
	}

	return check_mask;
}

static gint _set_all_chk_btn(NFOBJECT *all, NFOBJECT *unit[], gboolean expose)
{
	gboolean state = TRUE;
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
        if (!nfui_nfobject_is_disabled(unit[i])) {
	        if (!nfui_check_button_get_active((NFCHECKBUTTON*)unit[i])) state = FALSE;
        }
	}

	nfui_check_button_set_active((NFCHECKBUTTON*)all, state);
    return 0;
}

static gint _update_va_statistic_value(gint expose)
{
	struct tm ttm;
	gchar strBuf[64];

	gchar *strDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	guint total_val = 0;

	gint i;

	for (i = 0; i < 24; i++)
	{
		localtime_r(&g_ivcacr_data.hour[i].ttime, &ttm);

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "%d", ttm.tm_hour);
		nfui_nfbargraph_set_bar_data_text((NFBARGRAPH*)g_bargraph_obj, strBuf, i);
		nfui_nfbargraph_set_bar_value((NFBARGRAPH*)g_bargraph_obj, g_ivcacr_data.hour[i].cnt, i);
	}

	for (i = 0; i < 7; i++) {
		total_val += g_ivcacr_data.day[i].cnt;
	}

	//g_message("%s, %d, week_total_val:%d", __FUNCTION__, __LINE__, total_val);
	nfui_nfpiechart_set_max_value((NFPIECHART*)g_piechart_obj, total_val);

	for (i = 0; i < 7; i++)
	{
		localtime_r(&g_ivcacr_data.day[i].ttime, &ttm);

		nfui_nfpiechart_set_chart_value((NFPIECHART*)g_piechart_obj, i, (gdouble)g_ivcacr_data.day[i].cnt);

		if (g_ivcacr_data.day[i].cnt == 0) {
			memset(strBuf, 0x00, sizeof(strBuf));
			g_sprintf(strBuf, "%d.%d(%s)", ttm.tm_mon+1, ttm.tm_mday, lookup_string(strDay[ttm.tm_wday]));
			nfui_nflabel_set_text((NFLABEL*)g_piechart_day[i], strBuf);
			nfui_nflabel_set_text((NFLABEL*)g_piechart_val[i], "0(0%)");
		}
		else {
			memset(strBuf, 0x00, sizeof(strBuf));
			g_sprintf(strBuf, "%d.%d(%s)", ttm.tm_mon+1, ttm.tm_mday, lookup_string(strDay[ttm.tm_wday]));
			nfui_nflabel_set_text((NFLABEL*)g_piechart_day[i], strBuf);

			memset(strBuf, 0x00, sizeof(strBuf));
			g_sprintf(strBuf, "%d(%.2f%%)", g_ivcacr_data.day[i].cnt, ((float)g_ivcacr_data.day[i].cnt/(float)total_val)*100);
			nfui_nflabel_set_text((NFLABEL*)g_piechart_val[i], strBuf);
		}
	}	

	if (expose) 
	{
		nfui_signal_emit(g_bargraph_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_piechart_obj, GDK_EXPOSE, TRUE);

		for (i = 0; i < 7; i++) {
			nfui_signal_emit(g_piechart_day[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_piechart_val[i], GDK_EXPOSE, TRUE);			
		}
	}
	return 0;
}

static gboolean post_channel_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
		IVACR_T *icr;
        gboolean state;
        gint i;
		guint chmask = 0;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (nfui_nfobject_is_disabled(g_ch_obj[i])) continue;

            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ch_obj[i]), state);
            nfui_signal_emit(g_ch_obj[i], GDK_EXPOSE, TRUE);
        }

		chmask = _get_check_chmask();

		icr = iva_cntr_create();
		iva_cntr_set_filter_chmask(icr, chmask);
		iva_cntr_get_counter_data(icr, &g_ivcacr_data);
		iva_cntr_destroy(icr);

		_update_va_statistic_value(1);
    }

    return FALSE;
}

static gboolean post_channel_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
		IVACR_T *icr;
		guint chmask = 0;

        _set_all_chk_btn(g_ch_all_obj, g_ch_obj, TRUE);

		chmask = _get_check_chmask();

		icr = iva_cntr_create();
		iva_cntr_set_filter_chmask(icr, chmask);
		iva_cntr_get_counter_data(icr, &g_ivcacr_data);
		iva_cntr_destroy(icr);

		_update_va_statistic_value(1);
    }

    return FALSE;
}

static gboolean post_wnd_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch (event->type) 
	{
		case NFOUTEVT_BUTTON_PRESS:
		{
			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_POPUPWND, obj);	
		}
		break;
		case GDK_DELETE:
		{
			nfui_page_close(PGID_POPUPWND, obj);
			g_curwnd = 0;
		}
		break;
	}

	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;	
	GdkDrawable *drawable;
	GdkPixbuf *pbuf;

	gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(677)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*0+6, 10, 10);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(678)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*1+6, 10, 10);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(681)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*2+6, 10, 10);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(682)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*3+6, 10, 10);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(684)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*4+6, 10, 10);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(670)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*5+6, 10, 10);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(671)));
		gdk_draw_rectangle(drawable, gc, TRUE, 1060-16, 220+48*6+6, 10, 10);

        nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }
	
	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	nfui_nfobject_hide(top);
	return FALSE;
}

static gboolean post_refresh_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE) 
	{
		IVACR_T *icr;
		guint chmask = 0;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		chmask = _get_check_chmask();

		icr = iva_cntr_create();
		iva_cntr_set_filter_chmask(icr, chmask);
		iva_cntr_get_counter_data(icr, &g_ivcacr_data);
		iva_cntr_destroy(icr);

		_update_va_statistic_value(1);
    }

    return FALSE;
}

static gboolean post_reset_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE) 
	{
		mb_type ret;

		IVACR_T *icr;
		guint chmask = 0;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = nftool_mbox(g_curwnd, "CONFIRM", "All accumulated event data in the graph will be initialized.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
		if (ret == NFTOOL_MB_CANCEL) return FALSE;

		iva_cntr_reset();

		chmask = _get_check_chmask();

		icr = iva_cntr_create();
		iva_cntr_set_filter_chmask(icr, chmask);
		iva_cntr_get_counter_data(icr, &g_ivcacr_data);
		iva_cntr_destroy(icr);

		_update_va_statistic_value(1);
    }

    return FALSE;
}

static gboolean post_close_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

    if (evt->type == GDK_BUTTON_RELEASE) 
	{
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(top);
        nfui_page_close(PGID_POPUPWND, top);
    }

    return FALSE;
}


gint vw_deeplearning_statistic_popup_open(NFWINDOW *parent) 
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	gint pos_x, pos_y;
	gint i;

	gchar strBuf[32];

	memset(&g_ivcacr_data, 0x00, sizeof(IVACR_DATA_T));

	main_wnd = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-1224)/2, (1080-653)/2, 1224, 653);
	nfui_nfwindow_set_title(main_wnd, "TIMELINE_DEEPLEARNING_STATISTIC");
	nfui_nfwindow_set_mask((NFWINDOW*)main_wnd, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)main_wnd, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)main_wnd, returnkey_proc);
	nfui_nfobject_modify_bg(main_wnd, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(main_wnd, post_wnd_event_cb);
	g_curwnd = main_wnd;

	main_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size((NFOBJECT*)main_fixed, 1224, 653);
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI STATISTICS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 600, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 19);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);

	pos_x = 50;
	pos_y = 61;

	obj = nfui_checkbutton_new(TRUE);
	nfui_check_button_set_skin_type((NFCHECKBUTTON*)obj, NFCHECK_TYPE_POPUP_SMALL);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(34-18)/2);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_channel_all_event_handler);
	g_ch_all_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(340));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_set_size(obj, 100, 34);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+32, pos_y);
	nfui_nfobject_show(obj);

	pos_y += 34;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		obj = nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type((NFCHECKBUTTON*)obj, NFCHECK_TYPE_POPUP_SMALL);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(34-18)/2);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_channel_group_event_handler);
		g_ch_obj[i] = obj;

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "CH%d", i+1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(340));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_set_size(obj, 100, 34);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+32, pos_y);
		nfui_nfobject_show(obj);

		pos_x += 140;

		if (i == 7) {
			pos_x = 50;
			pos_y += 34;
		}
	}

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 240, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 50, 210);

	obj = nfui_nfbargraph_new(683, 300);
	nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH*)obj, 16);
	nfui_nfbargraph_set_chart_color((NFBARGRAPH*)obj, COLOR_IDX(689));
	nfui_nfbargraph_set_line_color((NFBARGRAPH*)obj, COLOR_IDX(689));
	nfui_nfbargraph_set_barbg_color((NFBARGRAPH*)obj, COLOR_IDX(690));
	nfui_nfbargraph_set_bar_focus_color((NFBARGRAPH*)obj, COLOR_IDX(691));
	nfui_nfbargraph_set_pango_font((NFBARGRAPH*)obj, nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
	nfui_nfbargraph_set_chart_line_gap_draw((NFBARGRAPH*)obj, 0, TRUE, TRUE);
	nfui_nfbargraph_set_chart_bg_color((NFBARGRAPH*)obj, COLOR_IDX(200));
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 250);
	nfui_nfobject_show(obj);
	//nfui_regi_post_event_callback(obj, post_nfbargraph_event_handler);
	g_bargraph_obj = obj;

	for (i = 0; i < 24; i++)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "%d", i+1);
		nfui_nfbargraph_set_bar_data_text(obj, strBuf, i);
		nfui_nfbargraph_set_bar_value(obj, 0, i);
	}

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WEEK", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 240, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 743, 210);

    obj = nfui_nfpiechart_new(240, 7, 100);
    nfui_nfpiechart_set_arcbg_color((NFPIECHART*)obj, COLOR_IDX(686));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 0, COLOR_IDX(677));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 1, COLOR_IDX(678));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 2, COLOR_IDX(681));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 3, COLOR_IDX(682));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 4, COLOR_IDX(684));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 5, COLOR_IDX(670));
	nfui_nfpiechart_set_chart_color((NFPIECHART*)obj, 6, COLOR_IDX(671));
    nfui_nfpiechart_set_pango_font((NFPIECHART*)obj, nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(262));
	nfui_nfobject_set_size(obj, 240, 240);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 743, 270);
    //nfui_regi_post_event_callback(obj, post_pichart_event_handler);
	g_piechart_obj = obj;

	pos_x = 1060;
	pos_y = 220;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SUNDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[0] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[0] = obj;

	pos_y += 26;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MONDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[1] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[1] = obj;

	pos_y += 26;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TUESDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[2] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[2] = obj;

	pos_y += 26;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WEDNESDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[3] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[3] = obj;

	pos_y += 26;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("THURSDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[4] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[4] = obj;

	pos_y += 26;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FRIDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[5] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[5] = obj;

	pos_y += 26;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SATURDAY", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_day[6] = obj;

	pos_y += 22;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0(0%)", nffont_get_pango_font(NFFONT_MINI_SEMI_3), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 160, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_piechart_val[6] = obj;

	obj = nftool_normal_button_create_type1("REFRESH", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 40, main_fixed->height-62);
	nfui_regi_post_event_callback(obj, post_refresh_button_event_cb);

	obj = nftool_normal_button_create_type1("RESET", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 236, main_fixed->height-62);
	nfui_regi_post_event_callback(obj, post_reset_button_event_cb);

	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, main_fixed->width-230, main_fixed->height-62);
	nfui_regi_post_event_callback(obj, post_close_button_event_cb);

    nfui_nfobject_show(main_fixed);
    nfui_run_main_event_handler(main_wnd);

	nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);
    nfui_page_open(PGID_POPUPWND, g_curwnd, nfui_get_last_user());
    nfui_nfobject_hide(main_wnd);

	nfui_make_key_hierarchy(main_wnd);

	return 0;
}

gint vw_deeplearning_statistic_popup_show()
{
	IVACR_T *icr;
	guint chmask = 0;

	if (!g_curwnd) return;
	if (nfui_nfobject_is_shown((NFOBJECT*)g_curwnd)) return;

	chmask = _get_check_chmask();

	icr = iva_cntr_create();
	iva_cntr_set_filter_chmask(icr, chmask);
	iva_cntr_get_counter_data(icr, &g_ivcacr_data);
	iva_cntr_destroy(icr);

	_update_va_statistic_value(0);

	nfui_nfobject_show((NFOBJECT*)g_curwnd);
	nfui_page_open(PGID_POPUPWND, (NFOBJECT*)g_curwnd, ssm_get_cur_id(NULL));

	return 0;
}

gint vw_deeplearning_statistic_popup_hide()
{
	if(!g_curwnd) return;

	if(nfui_nfobject_is_shown((NFOBJECT*)g_curwnd))
	{
		nfui_nfobject_hide((NFOBJECT*)g_curwnd);
		nfui_page_close(PGID_POPUPWND, (NFOBJECT*)g_curwnd);	
	}

	return 0;
}
