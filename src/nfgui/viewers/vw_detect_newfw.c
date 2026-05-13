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
#include "objects/nflistbox.h"

#include "nf_util_fw.h"
#include "support/nf_ui_page_manager.h"
#include "vw_detect_newfw.h"


#define NEWFW_WIN_W			(924)

static NFWINDOW *g_curwnd = 0;
static gint retVal = 0;
static DETECT_VERINFO_T *g_verInfo;

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

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	        retVal = 1;

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

	        retVal = 0;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean _post_detail_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		vw_detect_newfw_popup_detail_info_open(g_curwnd, g_verInfo);
	}
	
	return FALSE;
}

gint vw_detect_newfw_popup_open(NFWINDOW *parent, DETECT_VERINFO_T *verInfo)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gint win_size_h;
	guint table_w[] = {300, 600};

	gint idx = 0, row_cnt = 0;
	gint pos_x, pos_y;	
	gchar strBuf[64]; 
	SysInfoData sys_info;

	gchar inBuf[4096];
	gchar outBuf[4096];

	guint li_size_w, li_size_h;
	gint lc_size;

	retVal = 0;
	g_verInfo = verInfo;

	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);

	row_cnt = 2; 	// for "CURRENT FW VERSION, LAST FW UPDATE DATE"

	if (strlen(verInfo->model)) row_cnt++;
	if (strlen(verInfo->new_fwver)) row_cnt++;
	if (strlen(verInfo->importance)) row_cnt++;

	win_size_h = 250 + 41*row_cnt;

	if (strlen(verInfo->infor_general)) win_size_h += (160+1);

	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-NEWFW_WIN_W)/2, (1080-win_size_h)/2, (guint)NEWFW_WIN_W, (guint)win_size_h);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_main_wnd_event_handler);	
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, NEWFW_WIN_W, win_size_h);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);

// <----------TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FW UPDATE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, NEWFW_WIN_W-8, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	pos_x = 12;
	pos_y = 70;

// <----------create TABLE
	tbl = (NFOBJECT*)nfui_nftable_new(2, row_cnt, 1, 1, table_w, 40);	
 	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, pos_x, pos_y);

	pos_y = 70 + 41*row_cnt;

// <---------MODEL
	if (strlen(verInfo->model))
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MODEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, idx);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(verInfo->model, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  1, idx++);
	}

// <---------CURRENT F/W VERSION
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CURRENT FW VERSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  0, idx);

	memset(strBuf, 0x00, sizeof(strBuf));
	var_get_fake_fwver(strBuf, 64);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, idx++);

// <---------NEW/W VERSION
	if (strlen(verInfo->new_fwver))
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NEW FW VERSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, idx);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(verInfo->new_fwver, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  1, idx++);
	}

// <---------LAST FW UPDATE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LAST FW UPDATE DATE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  0, idx);

	memset(strBuf, 0x00, sizeof(strBuf));
	if (sys_info.fwupTime) dtf_get_local_datetime(sys_info.fwupTime, strBuf);
	else sprintf(strBuf, "N/A");

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, idx++);

// <---------UPDATE IMPORTANCE
	if (strlen(verInfo->importance))
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("UPDATE IMPORTANCE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, idx);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(verInfo->importance, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  1, idx++);
	}

// <---------GENERAL
	if (strlen(verInfo->infor_general))
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("UPDATE INFORMATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 300, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);		

		nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
		lc_size = table_w[1] - li_size_w;

		obj = nfui_listbox_new(1, &lc_size, 40);
		nfui_listbox_set_skin_type(NF_LISTBOX(obj),NFLISTBOX_TYPE_POPUP_1);
		nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
		nfui_listbox_set_drawing_outline(NF_LISTBOX(obj), FALSE);
		nfui_listbox_set_use_infocus_box(NF_LISTBOX(obj), FALSE);
		nfui_nfobject_set_size(obj, table_w[1], 160);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+301, pos_y);

		memset(inBuf, 0x00, sizeof(inBuf));
		memset(outBuf, 0x00, sizeof(outBuf));

		strcpy(inBuf, verInfo->infor_general);
		nfutil_get_line_feed_string_delimiter(inBuf, lc_size-10, outBuf, sizeof(outBuf), '|');
		_insert_line_feed_string(obj, outBuf);

		pos_y += 161;
	}

	pos_y += 10;

// <------ DETAIL INFO

    if (strlen(verInfo->infor_general) || strlen(verInfo->infor_fix) || strlen(verInfo->infor_func) || strlen(verInfo->reference_link))
    {
    	obj = nftool_normal_button_create_type3("DETAIL INFO", 220);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, NEWFW_WIN_W-16-12-220, pos_y);
    	nfui_regi_post_event_callback(obj, _post_detail_btn_event_handler);
    }

	pos_y = win_size_h - 60;

// <------ OK, CANCEL BTN
	obj = nftool_normal_button_create_type1("UPGRADE", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, NEWFW_WIN_W/2-179, pos_y);
	nfui_regi_post_event_callback(obj, _post_ok_btn_event_handler);
 
	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, NEWFW_WIN_W/2+5, pos_y);
	nfui_regi_post_event_callback(obj, _post_cancel_btn_event_handler);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy(win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_DETECT_NEWFW_POPUP, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_DETECT_NEWFW_POPUP, win);	

	return retVal;
}
