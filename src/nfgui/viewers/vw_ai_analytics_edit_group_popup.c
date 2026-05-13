/*
 * vw_timeline_deeplearning.c
 * 	- timeline dlva viewer
 *	- dependencies :
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 11, 2018
 *
 */

#include <time.h>

#include "gui/nf_afx.h"
#include <gtk/gtk.h>

#include <glib.h> 
#include <glib-object.h>
#include <glib/gprintf.h>

#include "nf_common.h"
#include "nf_notify.h"


#include "scm.h"
#include "vsm.h"
#include "modules/ssm.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfcheckbutton.h"
#include "objects/nflistbox.h"

#include "vw_ai_analytics_edit_group_popup.h"
#include "vw_vkeyboard.h"




////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (500)
#define POPUP_SIZE_HEI          (520)


static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_group_name_obj[8];

static EDIT_GROUP_DATA_T *g_group_data;



////////////////////////////////////////////////////////////////
//
// private interfaces
//





////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_group_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
		gchar *strBuf;
		gchar *strTemp;

		gint win_x, win_y; 
		gint gap_x, gap_y; 

		nfui_nfobject_get_window_pos((NFOBJECT*)obj, &win_x, &win_y);		
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		

		gap_x += win_x;
		gap_y += win_y;

		strBuf = nfui_nflabel_get_text((NFLABEL*)obj);
		strTemp = VirtualKey_Open(g_curwnd, strBuf, gap_x, gap_y+44, 32, VKEY_NORMAL|VKEY_MULTIKEYPD);

		if (strTemp) 
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
			ifree(strTemp);
			strTemp = NULL;
		}		
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *strBuf;
		gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < 8; i++)
		{
			strBuf = nfui_nflabel_get_text((NFLABEL*)g_group_name_obj[i]);

			memset(g_group_data->name[i], 0x00, sizeof(g_group_data->name[i]));
			snprintf(g_group_data->name[i], sizeof(g_group_data->name[i]), "%s", strBuf);
		}

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{

	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}



//////////////////////////////¤º//////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_edit_group_popup_open(NFWINDOW *parent, EDIT_GROUP_DATA_T *grp_data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	guint name_tbl_w[] = {70, 360};
	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i;

	gchar strBuf[16];


	g_group_data = grp_data;


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "EDIT", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 30;
    pos_y = 64;

    tbl = (NFOBJECT*)nfui_nftable_new(2, 1, 1, 1, name_tbl_w, 40);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("NO.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("GROUP NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 0);

	pos_y += 41;

    tbl = (NFOBJECT*)nfui_nftable_new(2, 8, 1, 1, name_tbl_w, 40);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, pos_x, pos_y);

    for (i = 0; i < 8; i++)
    {
		memset(strBuf, 0x00, sizeof(strBuf));
		sprintf(strBuf, "%d", i+1);

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box(grp_data->name[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i);
		nfui_regi_post_event_callback(obj, post_group_name_label_event_handler);
		g_group_name_obj[i] = obj;	
    }

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2-174-4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2+4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	return 0;
}
