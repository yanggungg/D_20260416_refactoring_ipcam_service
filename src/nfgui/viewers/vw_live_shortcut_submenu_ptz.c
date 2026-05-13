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
#include "vw_live_shortcut_submenu_ptz.h"
#include "vw_live_ptz_ctrl_popup.h"
#include "uxm.h"

#define MENU_TEXT_FRONT_MARGIN		(15)
#define MENU_GAP					2


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ptzWin;
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


static gboolean post_ptz_sub_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean covert = FALSE;

	if(evt->type == GDK_BUTTON_PRESS) {
		guint idx;
		GTimeVal ctv;
		GTimeVal ntv;
		guint ch_mask = 0;
		SecurityData secdata;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 			 
			return FALSE;

		nfui_nfobject_hide(g_ptzWin);
		VW_ShortCut_Menu_Hide();

		gint x,y;
        gint ch_x, ch_y, ch_w, ch_h;

        idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "button index"));

        if(idx == 0){       
            _get_size_info(&x, &y, &ch_x, &ch_y, &ch_w, &ch_h);
            VW_live_ptz_ctrl_popup_Open(g_curwnd, x, y, get_menu_channel(), ch_x, ch_y, ch_w, ch_h);
        }
        else if(idx == 1){

            VW_Live_Ptz_Main_Open(g_curwnd, get_menu_channel());
    		
        }		
	}

	return FALSE;
}

static gboolean post_ptz_sub_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_ptz_sub_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int mx, my, mw, mh;

	if(evt->type == NFOUTEVT_MOTION_NOTIFY) {
		GdkEventMotion *mevt;
		gint x, y;
		gint px, py;
		gint pedge;

		mevt = (GdkEventMotion*)evt;


		VW_ShortCut_Menu_Pos(&mx, &my);
		VW_ShortCut_Menu_Size(&mw, &mh);

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
		VW_ShortCut_Menu_Hide();
	}
	else if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
	}


	return FALSE;
}

static void hide_pb_submenu()
{
	if(nfui_nfobject_is_shown(g_ptzWin))
		nfui_nfobject_hide(g_ptzWin);
}

void VW_create_ptz_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item)
{
	NFOBJECT *ptz_fixed;
	NFOBJECT *ptz_tbl;
	NFOBJECT *obj;

	gchar *paramStr[2] = {"EASY MODE", "SETUP"};

	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};
	guint width = 212;
	guint i;

	g_prt_item = parent_item;

	/* window */
	g_ptzWin = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 220, 95);
	g_curwnd = g_ptzWin;
	nfui_nfwindow_set_title(g_curwnd, "LIVE SHORTCUT SUB PTZ");
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_ptzWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_ptzWin, GDK_MOTION_NOTIFY, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_ptzWin, GDK_BUTTON_PRESS, TRUE);
	gtk_widget_add_events(((NFWINDOW*)g_ptzWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(g_ptzWin, post_ptz_sub_win_event_cb);

	/* fixed */
	ptz_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(ptz_fixed, 220, 95);
	nfui_regi_post_event_callback(ptz_fixed, post_ptz_sub_fixed_event_handler);
	nfui_nfobject_show(ptz_fixed);

	/* table */	
	ptz_tbl = nfui_nftable_new(1, 2, 0, 2, &width, 40);
	nfui_nfobject_show(ptz_tbl);
	nfui_nffixed_put((NFFIXED*)ptz_fixed, ptz_tbl, 4, 8);

	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(menu_conf.submenu_img[0], paramStr[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
		nfui_regi_post_event_callback(obj, post_ptz_sub_btn_event_cb);
		nfui_nfobject_show(obj);

		nfui_nfobject_set_data(obj, "button index", GUINT_TO_POINTER(i));

		nfui_nftable_attach((NFTABLE*)ptz_tbl, obj, 0, i);
	}


	nfui_nfwindow_add((NFWINDOW*)g_ptzWin, ptz_fixed);
	nfui_nfobject_hide(g_ptzWin);
	nfui_run_main_event_handler(g_ptzWin);
}

int VW_destroy_ptz_submenu()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

int VW_move_ptz_submenu(int x, int y)
{
	nfui_nfobject_move(g_ptzWin, x, y);
}

void VW_show_ptz_submenu()
{
	guint x, y;

	if(!nfui_nfobject_is_shown(g_ptzWin)) {
		nfui_nfobject_show(g_ptzWin);
	}
}

void VW_hide_ptz_submenu()
{
	if(nfui_nfobject_is_shown(g_ptzWin)) {
		nfui_nfobject_hide(g_ptzWin);
	}
}

