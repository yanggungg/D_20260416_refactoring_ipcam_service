/*
 * vw_detect_newfw.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 14, 2012
 *
 */

#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/color.h"
#include "../support/util.h"
#include "../support/multi_language_support.h"

#include "../tools/nf_ui_tool.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfimage.h"
#include "../viewers/objects/nftable.h"
#include "../viewers/objects/nfcheckbutton.h"
#include "objects/nflistbox.h"

#include "nf_util_fw.h"
#include "support/nf_ui_page_manager.h"
#include "vw_detect_newcamfw.h"

enum {
	LIST_CHANNEL,
	LIST_MODEL,
	LIST_CVER,
	LIST_NVER,
	LIST_UPIMP,
	LIST_DINFO,
	LIST_COL_MAX
};

static NFWINDOW *g_curwnd = 0;
static DETECT_VERINFO_T *g_verInfo;
static NFOBJECT *g_list_obj[GUI_CHANNEL_CNT][LIST_COL_MAX];
static guint g_smask = 0;

static gint _insert_line_feed_string(NFOBJECT *obj, gchar *str)
{
	gchar *p;
	gchar *pStr;
	gchar *insert_str;

	p = str;

	while(1)
	{
		pStr = strchr(p,'\n');

		if(pStr == NULL)
		{
			insert_str = g_strdup(p);
			if (!insert_str) break;
			nfui_listbox_set_text((NFLISTBOX*)obj, &insert_str);
			g_free(insert_str);
			break;
		}
		else
		{
			insert_str = g_strndup(p, (pStr-p));
			if (!insert_str) break;			
			nfui_listbox_set_text((NFLISTBOX*)obj, &insert_str);
			g_free(insert_str);
			p += (pStr-p+1);
		}
	}

	return 0;
}

static gint _init_camfw_info(DETECT_VERINFO_T *verInfo, CAM_PROFILE_T *profile)
{
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (verInfo[i].need)
		{	
			// CHANNEL CHECK BOX
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_list_obj[i][LIST_CHANNEL], TRUE);
			nfui_nfobject_enable(g_list_obj[i][LIST_CHANNEL]);

			// MODEL STRING
			if (verInfo[i].model) nfui_nflabel_set_text((NFLABEL*)g_list_obj[i][LIST_MODEL], verInfo[i].model);
			
			// CURRENT SWVER STRING
			nfui_nflabel_set_text((NFLABEL*)g_list_obj[i][LIST_CVER], profile[i].model.swver);			

			// NEW SWVER STRING
			if (verInfo[i].new_fwver) nfui_nflabel_set_text((NFLABEL*)g_list_obj[i][LIST_NVER], verInfo[i].new_fwver);

			// UPDATE IMPORTANCE
			if (verInfo[i].importance) nfui_nflabel_set_text((NFLABEL*)g_list_obj[i][LIST_UPIMP], verInfo[i].importance);

			// DETAIL INFO
			nfui_nfobject_enable(g_list_obj[i][LIST_DINFO]);			
		}
	}

	return 0;
}

static gboolean _post_detail_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	gint i;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (g_list_obj[i][LIST_DINFO] == obj) break;
		}

		if (i >= GUI_CHANNEL_CNT) return FALSE;

		vw_detect_newfw_popup_detail_info_open(g_curwnd, &g_verInfo[i]);
	}
	
	return FALSE;
}

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean _post_ok_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	gint i;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (nfui_check_button_get_active((NFCHECKBUTTON*)g_list_obj[i][LIST_CHANNEL]))
			{
				g_smask |= (1 << i);
			}
		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean _post_cancel_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_smask = 0;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

guint vw_detect_newcamfw_popup_open(NFWINDOW *parent, DETECT_VERINFO_T *verInfo, CAM_PROFILE_T *profile)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed;
	NFOBJECT *tmp_fixed;	
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gint win_size_w, win_size_h;
	guint table_w[] = {180, 260, 260, 260, 260, 220};

	gint idx = 0, row_cnt = 0;
	gint pos_x, pos_y;	
	gchar strBuf[64]; 
	SysInfoData sys_info;

	gchar inBuf[4096];
	gchar outBuf[4096];

	guint li_size_w, li_size_h;
	gint lc_size;
	gint i, size_w, size_h;

	g_smask = 0;
	g_verInfo = verInfo;

	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);

	win_size_w = table_w[0]+table_w[1]+table_w[2]+table_w[3]+table_w[4]+table_w[5]+5+24;
	win_size_h = 160 + 41*(GUI_CHANNEL_CNT+1);

	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-win_size_w)/2, (1080-win_size_h)/2, (guint)win_size_w, (guint)win_size_h);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_main_wnd_event_handler);	
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win_size_w, win_size_h);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);

// <----------TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FW UPDATE (IPCAM)", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, win_size_w-8, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	pos_x = 12;
	pos_y = 70;

// <----------create TABLE
	tbl = (NFOBJECT*)nfui_nftable_new(6, GUI_CHANNEL_CNT+1, 1, 1, table_w, 40);	
 	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, pos_x, pos_y);

	pos_y = 70 + 41*(GUI_CHANNEL_CNT+1);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MODEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CURRENT FW VERSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 2, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NEW FW VERSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 3, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("UPDATE IMPORTANCE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 4, 0);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));		
		nfui_nfobject_show(tmp_fixed);
		nfui_nftable_attach((NFTABLE*)tbl, tmp_fixed, 0, i+1);

		obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size(obj, &size_w, &size_h);
		nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 10, (40-size_h)/2);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
		g_list_obj[i][LIST_CHANNEL] = obj;
		
		g_sprintf(strBuf, "CAM %d", i+1);
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, FALSE);	
		nfui_nfobject_set_size(obj, table_w[0]-54, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 54, 0);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, FALSE);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, LIST_MODEL, i+1);
		g_list_obj[i][LIST_MODEL] = obj;
		
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, FALSE);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, LIST_CVER, i+1);
		g_list_obj[i][LIST_CVER] = obj;
		
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, FALSE);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, LIST_NVER, i+1);
		g_list_obj[i][LIST_NVER] = obj;
		
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_use_focus(obj, FALSE);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, LIST_UPIMP, i+1);
		g_list_obj[i][LIST_UPIMP] = obj;
		
		obj = nftool_normal_button_create_type3("DETAIL INFO", table_w[5]);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, LIST_DINFO, i+1);
		nfui_regi_post_event_callback(obj, _post_detail_btn_event_handler);
		g_list_obj[i][LIST_DINFO] = obj;
	}

	pos_y = win_size_h - 60;

// <------ OK, CANCEL BTN
	obj = nftool_normal_button_create_type1("UPGRADE", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win_size_w/2-179, pos_y);
	nfui_regi_post_event_callback(obj, _post_ok_btn_event_handler);
 
	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win_size_w/2+5, pos_y);
	nfui_regi_post_event_callback(obj, _post_cancel_btn_event_handler);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy(win);
	nfui_set_key_focus(obj, TRUE);

	_init_camfw_info(verInfo, profile);

	nfui_page_open(PGID_DETECT_NEWFW_POPUP, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_DETECT_NEWFW_POPUP, win);	

	return g_smask;
}

