
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "ix_mem.h"

#include "modules/ssm.h"

#include "uxm.h"
#include "scm.h"
#include "smt.h"
#include "iux_msg.h"
/*
#include "vw_system_self_start.h"
#include "vw_system_self_check.h"
*/
#include "vw_system_self_internal.h"
#include "vw_sys_main.h"

#define SSS_WIN_SIZE_X					(610)
#define SSS_WIN_SIZE_Y					(320)
#define SSS_WIN_SIZE_W					(700)
#define SSS_WIN_SIZE_H					(430)

#define SSS_TITLE_H					(36)
#define SSS_TITLE_X					(4)
#define SSS_TITLE_Y					(4)

#define SSS_CELL_W					(50)

#define SSS_LABEL_H					(40)
#define SSS_LABEL_W					(400)
#define SSS_LABEL_TEXT_MARGIN				(20)

#define SSS_TABLE_ROW					(SSS_OBJ_MAX)
#define SSS_TABLE_COL					(2)

#define SSS_TABLE_ROW_SPACE				(2)
#define SSS_TABLE_COL_SPACE				(1)

#define SSS_TABLE_W					(SSS_LABEL_W + SSS_CELL_W + SSS_TABLE_ROW_SPACE)

#define SSS_TABLE_X					(SSS_TITLE_X + SSS_LABEL_TEXT_MARGIN + 35)
#define SSS_TABLE_Y					((SSS_TITLE_H + SSS_LABEL_H + 36 + (SSS_TABLE_ROW_SPACE * 2 ))

// <---- START CHECK, CLOSE BUTTON
#define SSS_START_BTN_X					(180)
#define SSS_START_BTN_Y					(SSS_WIN_SIZE_H - 75)
#define SSS_START_BTN_W					(174)

#define SSS_CLOSE_BTN_X					(SSS_START_BTN_X + SSS_START_BTN_W + 4)
#define SSS_CLOSE_BTN_Y					(SSS_START_BTN_Y)
#define SSS_CLOSE_BTN_W					(174)


enum {
	SSS_POWER = 0,
	SSS_NETWORK,
	SSS_HDD,
	SSS_PORT,
	SSS_OBJ_MAX
};

static NFWINDOW *g_curwnd = 0;
static NFWINDOW *g_parent = 0;
static NFOBJECT *check_allObj;
static NFOBJECT *check_obj[SSS_OBJ_MAX];


static gboolean post_startCheck_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;
	guint opt = 0;
	gint chk_cnt = 0;
	int i;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0 ; i < SSS_OBJ_MAX ; i++)
		{
			if (nfui_check_button_get_active((NFCHECKBUTTON*)check_obj[i]))
			{
				if (i == SSS_POWER)
				{
					opt |= (1 << MSK_POWER);
				}
				else if (i == SSS_NETWORK)
				{
					opt |= (1 << MSK_NET);
				}
				else if (i == SSS_HDD)
				{
					opt |= (1 << MSK_HDD);
				}
				else   // i == SSS_PORT
				{
					opt |= (1 << MSK_PORT);
				}
					chk_cnt++;
			}
		}

		if (chk_cnt == 0)
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please choose check list.", NFTOOL_MB_OK);
		}
		else
		{
			topwin = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(topwin);
			VW_System_Self_Check_Open(g_parent, opt);
		}
	}

	return FALSE;
}

static gboolean post_check_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	gboolean state;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for(i = 0; i < SSS_OBJ_MAX; i++)
		{
		
			if (!((i == SSS_POWER) && (DAL_get_cam_install_mode() == 1)))	// 1 == INSTALL MODE
			{
				nfui_check_button_set_active((NFCHECKBUTTON*)check_obj[i], state);
			}	
		}
	}

	return FALSE;
}

static gboolean post_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	gint chk_cnt = 0;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		for (i = 0 ; i < SSS_OBJ_MAX ; i++) 
		{
			if (nfui_check_button_get_active((NFCHECKBUTTON*)check_obj[i]))
				chk_cnt += 1;
		}

		if(chk_cnt == SSS_OBJ_MAX)
			nfui_check_button_set_active((NFCHECKBUTTON*)check_allObj, TRUE);
		else
			nfui_check_button_set_active((NFCHECKBUTTON*)check_allObj, FALSE);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}
static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
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

gboolean VW_System_Self_Start_Open(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *tbl;
	NFOBJECT *fixedTemp;

	guint i;
	gint title_size;
	gint size_w, size_h;

	gchar *strLabel[] = {"CAMERA POWER CONSUMPTION",
			"CAMERA NETWORK",
			"HDD",
			"SERVICE PORT"};	

	guint table_w[] = {SSS_CELL_W, SSS_LABEL_W};

	title_size = SSS_WIN_SIZE_W -20;

    g_parent = parent;
    
// <---- window
	win = (NFOBJECT*)nfui_nfwindow_new(parent, SSS_WIN_SIZE_X, SSS_WIN_SIZE_Y, SSS_WIN_SIZE_W, SSS_WIN_SIZE_H);
	nfui_nfwindow_set_title((NFWINDOW*)win, "SELF-DIAGNOSIS START");
	g_curwnd = win;
	nfui_regi_post_event_callback(win, post_main_win_event_handler);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));

// <---- fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, SSS_WIN_SIZE_W, SSS_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(fixed);

// <---- title
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SELF-DIAGNOSIS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, title_size, SSS_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSS_TITLE_X, SSS_TITLE_Y);

// <---- check all
	obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
	nfui_check_get_size(obj, &size_w, &size_h);
	nfui_regi_post_event_callback(obj, post_check_all_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSS_TABLE_X - 30), (SSS_TITLE_H + 30));
	nfui_nfobject_show(obj);
	check_allObj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHECK OPTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SSS_LABEL_W, SSS_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSS_TABLE_X + SSS_CELL_W + SSS_TABLE_COL_SPACE - 30), (SSS_TITLE_H + 23));

// <---- TABEL

 	tbl = (NFOBJECT*)nfui_nftable_new(SSS_TABLE_COL, SSS_TABLE_ROW, SSS_TABLE_COL_SPACE, SSS_TABLE_ROW_SPACE, table_w, SSS_LABEL_H);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, SSS_TABLE_X, (SSS_TITLE_H + SSS_LABEL_H + 40));

// <---- CHECK BUTTON / LABEL
	for (i = 0; i < SSS_TABLE_ROW; i++)
	{
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_show(fixedTemp);
		nfui_nftable_attach((NFTABLE*)tbl, fixedTemp, 0, i);
				
		obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_nfobject_get_size(obj, &size_w, &size_h);

		if ((i == SSS_POWER) && (DAL_get_cam_install_mode() == 1))	// 1 == INSTALL MODE
		{
			nfui_check_button_set_active((NFCHECKBUTTON*)obj, FALSE);
			nfui_nfobject_disable(obj);
		}
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (table_w[0]-size_w)/2, (table_w[0]-size_h)/2);
		nfui_regi_post_event_callback(obj, post_check_event_handler);
		check_obj[i] = obj;


		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i);
	}

// <---- START BUTTON
	obj = nftool_normal_button_create_type1("START CHECK", SSS_START_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_startCheck_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSS_START_BTN_X, SSS_START_BTN_Y);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", SSS_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSS_CLOSE_BTN_X, SSS_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_SELF_START_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_SELF_START_POPUP, win);
}

void VW_System_Self_Start_Close()
{
    if (!g_curwnd) return;
    
    evt_send_to_window("SELF-DIAGNOSIS START", WND_CLOSE, 0, 0, 0);
}

