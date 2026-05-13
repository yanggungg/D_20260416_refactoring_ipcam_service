/*
 * vw_text_box_open.c
 *	- dependencies :
 *
 *
 * Written by Suelbee lee. <suelbeelee@itxsecurity.com>
 * Copyright (c) ITX security, July 5, 2013
 *
 */

#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfwindow.h"
#include "objects/nflistbox.h"

#include "iux_msg.h"
#include "uxm.h"

#include "vw.h"
#include "vw_internal.h"
#include "vw_text_box_open.h"


static NFWINDOW *g_curwnd = 0;

static gint g_label_cnt;
static NFOBJECT *g_label_obj[36];

static NFOBJECT *g_list_obj;

static TEXT_BOX_INFO *g_box_info;
static TEXT_BOX_CONTROL *g_box_control;

static gint g_box_opt;


static gint _set_string_label(gint i, gchar *str, gint expose)
{
    if (i >= g_label_cnt) return -1;
    if (!strcmp(nfui_nflabel_get_text((NFLABEL*)g_label_obj[i]), str)) return -1;

    nfui_nflabel_set_text((NFLABEL*)g_label_obj[i], str);
    if (expose) nfui_signal_emit(g_label_obj[i], GDK_EXPOSE, TRUE);
	return 0;
}

static gint _insert_string_label(gchar *str, gint expose)
{
	gchar *pStr;
    gint i;

    for (i = 0; i < g_label_cnt; i++)
    {
        pStr = nfui_nflabel_get_text((NFLABEL*)g_label_obj[i]);
        if (strlen(pStr) == 0) break;
    }

    nfui_nflabel_set_text((NFLABEL*)g_label_obj[i], str);
    if (expose) nfui_signal_emit(g_label_obj[i], GDK_EXPOSE, TRUE);
    return 0;
}

static gint _insert_line_feed_string_label(gchar *str, gint expose)
{
	gchar *p;
	gchar *pStr;
	gchar *insert_str;
    gint i;

    for (i = 0; i < g_label_cnt; i++)
    {
        pStr = nfui_nflabel_get_text((NFLABEL*)g_label_obj[i]);
        if (strlen(pStr) == 0) break;
    }

	p = str;

	while(1)
	{
		pStr = strchr(p,'\n');

		if(pStr == NULL)
		{
			insert_str = g_strdup(p);
			if (!insert_str) break;
			nfui_nflabel_set_text((NFLABEL*)g_label_obj[i], insert_str);
            if (expose) nfui_signal_emit(g_label_obj[i], GDK_EXPOSE, TRUE);
			g_free(insert_str);
			break;
		}
		else
		{
			insert_str = g_strndup(p, (pStr-p));
			if (!insert_str) break;
			nfui_nflabel_set_text((NFLABEL*)g_label_obj[i], insert_str);
            if (expose) nfui_signal_emit(g_label_obj[i], GDK_EXPOSE, TRUE);
			g_free(insert_str);
			p += (pStr-p+1);

			i++;
		}

		if (i >= g_label_cnt) break;
	}

	return 0;
}

static gint _clear_label(gint expose)
{
    gint i;

    for (i = 0; i < g_label_cnt; i++)
    {
        nfui_nflabel_set_text((NFLABEL*)g_label_obj[i], "");
        if (expose) nfui_signal_emit(g_label_obj[i], GDK_EXPOSE, TRUE);
    }

    return 0;
}

static gint _set_string_listbox(gint i, gchar *str, gint expose)
{
    nfui_listbox_modify_text_by_index((NFLISTBOX*)g_list_obj, &str, i);
    if (expose) nfui_signal_emit(g_list_obj, GDK_EXPOSE, TRUE);
	return 0;
}

static gint _insert_string_listbox(gchar *str, gint expose)
{
	nfui_listbox_set_text((NFLISTBOX*)g_list_obj, &str);
    if (expose) nfui_signal_emit(g_list_obj, GDK_EXPOSE, TRUE);
    return 0;
}

static gint _insert_line_feed_string_listbox(gchar *str, gint expose)
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
			nfui_listbox_set_text((NFLISTBOX*)g_list_obj, &insert_str);
			g_free(insert_str);
			break;
		}
		else
		{
			insert_str = g_strndup(p, (pStr-p));
			if (!insert_str) break;
			nfui_listbox_set_text((NFLISTBOX*)g_list_obj, &insert_str);
			g_free(insert_str);
			p += (pStr-p+1);
		}
	}

    if (expose) nfui_signal_emit(g_list_obj, GDK_EXPOSE, TRUE);

	return 0;
}

static gint _clear_listbox(gint expose)
{
    nfui_listbox_delete_all((NFLISTBOX*)g_list_obj);
    if (expose) nfui_signal_emit(g_list_obj, GDK_EXPOSE, TRUE);
    return 0;
}

static gint _add_line(gchar *strBuf, gint expose)
{
    gint bg_width = g_box_info->win_w;

    if (!strBuf) return -1;

	if (g_box_opt & (1 << TB_OPT_AUTO_LF))
	{
    	gchar tmp[4096];

    	memset(tmp, 0x00, sizeof(tmp));
	    nfutil_get_line_feed_string(strBuf, bg_width, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), tmp, sizeof(tmp));

        if (g_box_info->type == TB_TYPE_LIST)
        {
            _insert_line_feed_string_listbox(tmp, expose);
        }
        else
        {
            _insert_line_feed_string_label(tmp, expose);
        }
    }
    else
    {
        if (g_box_info->type == TB_TYPE_LIST)
        {
            _insert_string_listbox(strBuf, expose);
        }
        else
        {
            _insert_string_label(strBuf, expose);
        }
    }

    return 0;
}

static gint _modify_line(gint line, gchar *strBuf, gint expose)
{
    if (!strBuf) return -1;

    if (g_box_info->type == TB_TYPE_LIST)
    {
        _set_string_listbox(line, strBuf, expose);
    }
    else
    {
        _set_string_label(line, strBuf, expose);
    }

    return 0;
}

static _clear_box(gint expose)
{
    if (g_box_info->type == TB_TYPE_LIST)
    {
        _clear_listbox(expose);
    }
    else
    {
        _clear_label(expose);
    }

    return 0;
}

static gboolean post_okay_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}
	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
    else if ((g_box_control) && (g_box_control->add != IMSG_NONE) && (evt->type == g_box_control->add))
	{
        gchar *strBuf = (gchar*)(((CMM_MESSAGE_T *)data)->data);

        _add_line(strBuf, 1);
	}
    else if ((g_box_control) && (g_box_control->modify != IMSG_NONE) && (evt->type == g_box_control->modify))
	{
	    gint line = ((CMM_MESSAGE_T *)data)->param;
        gchar *strBuf = (gchar*)(((CMM_MESSAGE_T *)data)->data);

        _modify_line(line, strBuf, 1);
	}
    else if ((g_box_control) && (g_box_control->clear != IMSG_NONE) && (evt->type == g_box_control->clear))
	{
        _clear_box(1);
	}
	else if (evt->type == GDK_DELETE)
	{
        if (g_box_control)
        {
            if (g_box_control->add != IMSG_NONE) uxm_unreg_imsg_event(obj, g_box_control->add);
            if (g_box_control->modify != IMSG_NONE) uxm_unreg_imsg_event(obj, g_box_control->modify);
            if (g_box_control->clear != IMSG_NONE) uxm_unreg_imsg_event(obj, g_box_control->clear);
        }

    	nfui_nfobject_get_size(obj, &size_w, &size_h);
    	nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

    	g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}


gint VW_TextBox_Open(NFOBJECT *parent, gchar *title, TEXT_BOX_INFO *box_info, TEXT_BOX_CONTROL *box_control, guint opt)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *label_obj;
	NFOBJECT *list_obj;
	NFOBJECT *win;
	guint width;

	guint li_size_w, li_size_h;
	gint i, lc_size;
	gint label_cnt;

    g_box_info = box_info;
    g_box_control = box_control;
    g_box_opt = opt;


	g_curwnd = nfui_nfobject_get_top(parent);

	win = (NFOBJECT*)nfui_nfwindow_new(parent, box_info->win_x, box_info->win_y, box_info->win_w, box_info->win_h);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));

	// <----- fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, box_info->win_w, box_info->win_h);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);


	// <----- title
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, box_info->win_w-40, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	lc_size = box_info->win_w - 20;

    if (box_info->type == TB_TYPE_LIST)
    {
    	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);
        lc_size -= li_size_w;

    	list_obj = nfui_listbox_new(1, &lc_size, 40);
    	nfui_listbox_set_skin_type(NF_LISTBOX(list_obj),NFLISTBOX_TYPE_POPUP_1);
    	nfui_listbox_set_pango_font(NF_LISTBOX(list_obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nfobject_set_size(list_obj, box_info->win_w-20, box_info->win_h-56-70);
    	nfui_nfobject_modify_bg(list_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    	nfui_nfobject_use_focus(list_obj, NFOBJECT_FOCUS_ON);
    	nfui_nfobject_show(list_obj);
    	nfui_nffixed_put((NFFIXED*)fixed, list_obj, 10, 56);
        g_list_obj = obj;
    }
    else
    {
        label_cnt = (box_info->win_h-130)/40;
        label_cnt = (label_cnt > 36) ? 36 : label_cnt;

        for (i = 0; i < label_cnt; i++)
        {
        	label_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        	nfui_nflabel_set_skin_type((NFLABEL*)label_obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        	nfui_nflabel_set_align((NFLABEL*)label_obj, NFALIGN_LEFT, 10);
        	nfui_nfobject_use_focus(label_obj, NFOBJECT_FOCUS_OFF);
        	nfui_nfobject_set_size(label_obj, lc_size, 40);
        	nfui_nfobject_show(label_obj);
        	nfui_nffixed_put((NFFIXED*)fixed, label_obj, 10, 56 + i*41);
        	g_label_obj[i] = label_obj;
        }

        g_label_cnt = label_cnt;
    }

    _add_line(box_info->line, 0);

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_okay_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (box_info->win_w-174)/2, box_info->win_h-56);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

    if (box_control)
    {
        if (box_control->add != IMSG_NONE) uxm_reg_imsg_event(fixed, box_control->add);
        if (box_control->modify != IMSG_NONE) uxm_reg_imsg_event(fixed, box_control->modify);
        if (box_control->clear != IMSG_NONE) uxm_reg_imsg_event(fixed, box_control->clear);
    }

	nfui_page_open(PGID_TEXT_BOX, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_TEXT_BOX, win);

	return 0;
}

