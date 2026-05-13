#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

#include "services/vsm.h"

#include "modules/ssm.h"

#include "vw_live_shortcut_menu.h"
#include "vw_zoom_pip.h"
#include "vw_timeline.h"
#include "vw_set_date_time.h"
#include "vw_live_statusbar.h"
#include "vw_live_snapshot.h"
#include "vw_playback_main.h"
#include "vw_live_shortcut_submenu_fisheye.h"
#include "uxm.h"

#include "nf_api_live.h"


#define MENU_TEXT_FRONT_MARGIN		(15)
#define MENU_GAP					2


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_fisheyeWin;
static NFOBJECT *g_prt_item;


static gboolean _get_size_info(gint *pos_x, gint *pos_y, gint *ch_x, gint *ch_y, gint *ch_w, gint *ch_h)
{
    gint x,y,w,h;
    gint win_h = get_win_height();
    gint win_w = get_win_width();

    vsm_get_current_win_id_size(&x, &y, &w, &h);

    *ch_x = x;
    *ch_y = y;
    *ch_w = w;
    *ch_h = h;

    if(w == DISPLAY_ACTIVE_WIDTH && h == DISPLAY_ACTIVE_HEIGHT){
        *pos_x = DISPLAY_ACTIVE_WIDTH - win_w;
        *pos_y = DISPLAY_ACTIVE_HEIGHT - win_h;

        return FALSE;
    }

    x = x+w+7;
    
    if(y+win_h > DISPLAY_ACTIVE_HEIGHT) y = y+h-win_h;
    if(x+win_w > DISPLAY_ACTIVE_WIDTH) x = x-w-win_w-14;
    if(x < 0) x = DISPLAY_ACTIVE_WIDTH - win_w;
    if(y < 0) y = 0;

    *pos_x = x;
    *pos_y = y;   

    return FALSE;
}

static gboolean post_fisheye_sub_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NF_FISHEYE_VIDEO_PARAM param;
	guint idx, view_type = NF_FISHEYE_VIEW_SIGLE;
	gint i, ch;

	if (evt->type == GDK_BUTTON_PRESS) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_hide(g_fisheyeWin);

		if (vsm_get_vmode() == VMODE_LV) {
			VW_ShortCut_Menu_Hide();
			ch = get_menu_channel();
		}
		else {
			vw_playback_shortcut_menu_hide();
			ch = get_menu_channel_pb();
		}

		if (nf_live_fisheye_get_enable() != ch)
		{
			nf_live_fisheye_set_enable(ch);
			feye_set_dewarping_channel(ch);
		}

        idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "button index"));
        if (idx == 0) view_type = NF_FISHEYE_VIEW_SIGLE;
        else if (idx == 1) view_type = NF_FISHEYE_VIEW_QUAD;
        else if (idx == 2) view_type = NF_FISHEYE_VIEW_PANORAMA;

		memset(&param, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));
		nf_live_fisheye_get_video_param(ch, &param);

		if (param.view_type != view_type)
		{
			param.view_type = view_type;
			nf_live_fisheye_set_video_param(ch, &param);
			feye_set_video_param_data(ch, &param);					
		}
	}

	return FALSE;
}

static gboolean post_fisheye_sub_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_fisheye_sub_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int mx, my, mw, mh;

	if(evt->type == NFOUTEVT_MOTION_NOTIFY) {
		GdkEventMotion *mevt;
		gint x, y;
		gint px, py;
		gint pedge;

		mevt = (GdkEventMotion*)evt;

		if (vsm_get_vmode() == VMODE_LV) {
			VW_ShortCut_Menu_Pos(&mx, &my);
			VW_ShortCut_Menu_Size(&mw, &mh);			
		}
		else {
			VW_ShortCut_Menu_Pos_pb(&mx, &my);
			VW_ShortCut_Menu_Size_pb(&mw, &mh);			
		}

		x = (gint)mevt->x_root - mx;
		y = (gint)mevt->y_root - my;

		px = (gint)mevt->x_root - g_prt_item->x;
		py = (gint)mevt->y_root - g_prt_item->y;
	
		if (px < obj->x) { 	// show on right side	
			if((x < g_prt_item->x || x > (mw + obj->width))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);

					return TRUE;
				}
			}

			if((y < g_prt_item->y || y > (g_prt_item->y + g_prt_item->height))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);

					return TRUE;
				}
			}
		}
		else {
			pedge = obj->x + obj->width + MENU_GAP + mw;

			if((gint)mevt->x_root < obj->x || (gint)mevt->x_root > pedge) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);

					return TRUE;
				}
			}

			if((y < g_prt_item->y || y > (g_prt_item->y + g_prt_item->height))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);

					return TRUE;
				}
			}
		}

		return TRUE;
	}
	else if(evt->type == NFOUTEVT_BUTTON_PRESS) {
		nfui_nfobject_hide(obj);

		if (vsm_get_vmode() == VMODE_LV) VW_ShortCut_Menu_Hide();
		else vw_playback_shortcut_menu_hide();
	}
	else if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
	}


	return FALSE;
}

void VW_create_fisheye_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item)
{
	NFOBJECT *fisheye_fixed;
	NFOBJECT *fisheye_tbl;
	NFOBJECT *obj;

	gchar *paramStr[3] = {"SINGLE VIEW", "QUAD VIEW", "PANORAMA VIEW"};

	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};
	guint width = 312;
	guint i;

	if (!ivsc.dfunc.support_fisheye) return;

	g_prt_item = parent_item;

	/* window */
//	g_fisheyeWin = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 320, 135);
	g_fisheyeWin = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 320, 55);
	nfui_nfwindow_set_title((NFWINDOW*)g_fisheyeWin, "LIVE SHORTCUT SUB FISHEYE");
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_fisheyeWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_fisheyeWin, GDK_MOTION_NOTIFY, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_fisheyeWin, GDK_BUTTON_PRESS, TRUE);
	gtk_widget_add_events(((NFWINDOW*)g_fisheyeWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(g_fisheyeWin, post_fisheye_sub_win_event_cb);
	g_curwnd = g_fisheyeWin;

	/* fixed */
	fisheye_fixed = (NFOBJECT*)nfui_nffixed_new();
//	nfui_nfobject_set_size(fisheye_fixed, 320, 135);
	nfui_nfobject_set_size(fisheye_fixed, 320, 55);
	nfui_regi_post_event_callback(fisheye_fixed, post_fisheye_sub_fixed_event_handler);
	nfui_nfobject_show(fisheye_fixed);

	/* table */	
	fisheye_tbl = nfui_nftable_new(1, 3, 0, 2, &width, 40);
	nfui_nfobject_show(fisheye_tbl);
	nfui_nffixed_put((NFFIXED*)fisheye_fixed, fisheye_tbl, 4, 8);

	for(i=0; i<1; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(menu_conf.submenu_img[2], paramStr[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "button index", GUINT_TO_POINTER(i));
		nfui_nftable_attach((NFTABLE*)fisheye_tbl, obj, 0, i);
		nfui_regi_post_event_callback(obj, post_fisheye_sub_btn_event_cb);
	}

	nfui_nfwindow_add((NFWINDOW*)g_fisheyeWin, fisheye_fixed);
	nfui_nfobject_hide(g_fisheyeWin);
	nfui_run_main_event_handler(g_fisheyeWin);
}

int VW_destroy_fisheye_submenu()
{
	if (!g_curwnd) return 0;
	if (!ivsc.dfunc.support_fisheye) return 0;

	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

int VW_move_fisheye_submenu(int x, int y)
{
	if (!ivsc.dfunc.support_fisheye) return 0;

	nfui_nfobject_move(g_fisheyeWin, x, y);
	return 0;
}

void VW_show_fisheye_submenu(NFOBJECT *parent_item)
{
	guint x, y;

	if (!ivsc.dfunc.support_fisheye) return;

	if(!nfui_nfobject_is_shown(g_fisheyeWin)) {
		g_message("%s, %d", __FUNCTION__, __LINE__);
		g_prt_item = parent_item;
		nfui_nfobject_show(g_fisheyeWin);
	}
}


