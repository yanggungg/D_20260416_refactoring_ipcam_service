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
#include "vw_live_shortcut_submenu_pb.h"
#include "uxm.h"


#define MENU_SIZE_W					(289)
#if 0	// v2.0
#define MENU_SIZE_H					(316)
#endif
#define MENU_SIZE_H					(316-40)
#define MENU_FRONT_MARGIN			(8)
#define MENU_REAR_MARGIN			(14)
#define MENU_DEF_SPACE 				(MENU_FRONT_MARGIN + MENU_REAR_MARGIN)

#define MENU_BUTTON_SIZE_W			(267)
#define MENU_BUTTON_SIZE_H			(40)
#define MENU_BUTTON_DEF_SPACE		(MENU_TEXT_FRONT_MARGIN + MENU_TEXT_REAR_MARGIN) 
#define MENU_BUTTON_GAP_2			(2)

#define MENU_TEXT_FRONT_MARGIN		(15)
#define MENU_TEXT_REAR_MARGIN		(21)
#define MENU_GAP					2


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_pbWin;
static NFOBJECT *g_prt_item;

static gboolean _get_covert_info(int ch)
{
	CameraData cd[GUI_CHANNEL_CNT];
	gchar lg[10];
	guint mask = 0;
	gint i;

	ssm_get_cur_group(lg);

	for(i = 0; i < GUI_CHANNEL_CNT; i++)
		DAL_get_covert_data(&cd[i], i);
	
	if (!strcmp(lg, "ADMIN")) return cd[ch].admin;
	if (!strcmp(lg, "MANAGER")) return cd[ch].manager;
	
	return cd[ch].user;
}

static gboolean post_pb_sub_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean covert = FALSE;
    gboolean use_dl = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		guint idx;
		GTimeVal ctv;
		GTimeVal ntv;
		guint ch_mask = 0;
		SecurityData secdata;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 			 
			return FALSE;

		nfui_nfobject_hide(g_pbWin);
		VW_ShortCut_Menu_Hide();

		covert = _get_covert_info(get_menu_channel());
		if(covert) {
			nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
			return FALSE;
		}

		g_get_current_time(&ctv);

		idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "button index"));

		memset(&ntv, 0x00, sizeof(GTimeVal));
		
		if(idx == 0)	ntv.tv_sec = ctv.tv_sec - 10; 
		if(idx == 1)	ntv.tv_sec = ctv.tv_sec - 20; 
		if(idx == 2)	ntv.tv_sec = ctv.tv_sec - 30; 
		if(idx == 3)	ntv.tv_sec = ctv.tv_sec - 60; 
		if(idx == 4)	ntv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "SPECIFIC TIME", 720, 390, ctv.tv_sec, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);

		if(ntv.tv_sec != 0) {
			ch_mask |= (1 << get_menu_channel());
		
            DAL_get_use_double_login(&use_dl);

            if (use_dl && !ssm_is_admin())
            {
                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_SEARCH)) return FALSE;
            }
            else
            {
			DAL_get_security_data(&secdata);
    			if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
            }

			VW_Live_StatusBar_Hide();							

			vsm_live_stop();
			vw_playback_open(NF_TOPWND, vsm_create_livestart_obj(), OPEN_BY_LIVE_LOG);
			vsm_playback_start(ch_mask, ntv, PLAYBACK_NORMAL);
		}
	}

	return FALSE;
}

static gboolean post_pb_sub_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_pb_sub_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
	if(nfui_nfobject_is_shown(g_pbWin))
		nfui_nfobject_hide(g_pbWin);
}

void VW_create_playback_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item)
{
	NFOBJECT *pb_fixed;
	NFOBJECT *pb_tbl;
	NFOBJECT *obj;

	gchar *paramStr[5] = {"10 SEC AGO", "20 SEC AGO", "30 SEC AGO", "1 MIN AGO", "GO TO"};

	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};
	guint width = 212;
	guint i;

	g_prt_item = parent_item;

	/* window */
	g_pbWin = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 220, 220);
	g_curwnd = g_pbWin;
	nfui_nfwindow_set_title(g_curwnd, "LIVE SHORTCUT SUB PB");
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_pbWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_pbWin, GDK_MOTION_NOTIFY, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_pbWin, GDK_BUTTON_PRESS, TRUE);
	gtk_widget_add_events(((NFWINDOW*)g_pbWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(g_pbWin, post_pb_sub_win_event_cb);

	/* fixed */
	pb_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(pb_fixed, 220, 220);
	nfui_regi_post_event_callback(pb_fixed, post_pb_sub_fixed_event_handler);
	nfui_nfobject_show(pb_fixed);

	/* table */	
	pb_tbl = nfui_nftable_new(1, 5, 0, 2, &width, 40);
	nfui_nfobject_show(pb_tbl);
	nfui_nffixed_put((NFFIXED*)pb_fixed, pb_tbl, 4, 8);

	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(menu_conf.submenu_img[0], paramStr[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
		nfui_regi_post_event_callback(obj, post_pb_sub_btn_event_cb);
		nfui_nfobject_show(obj);

		nfui_nfobject_set_data(obj, "button index", GUINT_TO_POINTER(i));

		nfui_nftable_attach((NFTABLE*)pb_tbl, obj, 0, i);
	}


	nfui_nfwindow_add((NFWINDOW*)g_pbWin, pb_fixed);
	nfui_nfobject_hide(g_pbWin);
	nfui_run_main_event_handler(g_pbWin);
}

int VW_destroy_playback_submenu()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

int VW_move_pb_submenu(int x, int y)
{
	nfui_nfobject_move(g_pbWin, x, y);
}

void VW_show_pb_submenu()
{
	guint x, y;

	if(!nfui_nfobject_is_shown(g_pbWin)) {
		nfui_nfobject_show(g_pbWin);
	}
}

void VW_hide_pb_submenu()
{
	if(nfui_nfobject_is_shown(g_pbWin)) {
		nfui_nfobject_hide(g_pbWin);
	}
}

