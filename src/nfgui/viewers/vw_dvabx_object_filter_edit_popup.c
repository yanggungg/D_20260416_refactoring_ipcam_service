
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vkeyboard.h"

#include "vw_dvabx_object_filter_edit_popup.h"
#include "ix_mem.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_filterObj;
static NFOBJECT *g_addObj;
static NFOBJECT *g_lboxObj;

static gchar *g_exception_string;
static gchar *g_filter_string;
static gint g_buff_size;


static gboolean post_filter_input_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
		gchar *filter = NULL;
		guint x, y;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);
		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		filter = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(g_filterObj), x, y, 32, VKEY_ALPHANUMERIC|VKEY_MULTIKEYPD);

		if (filter) 
		{
			nfui_nflabel_set_text((NFLABEL*)g_filterObj, filter);
			nfui_signal_emit(g_filterObj, GDK_EXPOSE, TRUE);
			ifree(filter);
		}
	}

	return FALSE;
}

static gboolean post_add_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *tmp_string = 0, *list_text;
		gint i, cnt;

		gchar *filter = NULL;

		gchar tmp_exception_string[64];
		gchar *exception_token;
		gchar *pbuf = NULL;
		gchar *pnext = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		filter = nfui_nflabel_get_text(g_filterObj);
		if (strlen(filter)) 
		{
			tmp_string = imalloc(g_buff_size);
			if (!tmp_string) return FALSE;

			memset(tmp_exception_string, 0x00, sizeof(tmp_exception_string));
			snprintf(tmp_exception_string, sizeof(tmp_exception_string)-1, "%s", g_exception_string);

			exception_token = strtok_r(tmp_exception_string, ",", &pnext);
			pbuf = pnext;

			while (exception_token != 0)
			{
				if (!strcmp(filter, exception_token)) {
					nftool_mbox(g_curwnd, "NOTICE", "The same filter already exists.", NFTOOL_MB_OK);
					ifree(tmp_string);
					return FALSE;
				}
				exception_token = strtok_r(pbuf, ",", &pnext);
				pbuf = pnext;
			}

			if (strstr(g_exception_string, filter)) {
				nftool_mbox(g_curwnd, "NOTICE", "The same filter already exists.", NFTOOL_MB_OK);
				ifree(tmp_string);
				return FALSE;
			}

			cnt = nfui_listbox_get_box_count(NF_LISTBOX(g_lboxObj));

			for (i = 0; i < cnt; i++)
			{
				list_text = nfui_listbox_get_text_of_list(NF_LISTBOX(g_lboxObj), i, 0);
				if (!strcmp(filter, list_text)) {
					nftool_mbox(g_curwnd, "NOTICE", "The same filter already exists.", NFTOOL_MB_OK);
					ifree(tmp_string);
					return FALSE;
				}				
			}

			for (i = 0; i < cnt; i++)
			{
				list_text = nfui_listbox_get_text_of_list(NF_LISTBOX(g_lboxObj), i, 0);
				if (i == 0) snprintf(tmp_string, g_buff_size, "%s", list_text);
				else snprintf(tmp_string+strlen(tmp_string), g_buff_size-strlen(tmp_string)-1, ",%s", list_text);
			}

			if (strlen(tmp_string)+strlen(filter)+2 < g_buff_size)
			{
				nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &filter);
				nfui_signal_emit(g_lboxObj, GDK_EXPOSE, TRUE);

				nfui_nflabel_set_text((NFLABEL*)g_filterObj, "");
				nfui_signal_emit(g_filterObj, GDK_EXPOSE, TRUE);
			}
			else
			{
				nftool_mbox(g_curwnd, "NOTICE", "You can not add a filter because the length of the search name that can be retrieved is exceeded.\nYou can add a new filter by removing unused filters from the list.", NFTOOL_MB_OK);
			}

			ifree(tmp_string);	
		}
	}
	return FALSE;
}

static gboolean post_delete_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint f_idx;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		f_idx = nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj));
		if (f_idx >= 0) 
		{
			nfui_listbox_delete(NF_LISTBOX(g_lboxObj), (guint)f_idx);
			nfui_signal_emit(g_lboxObj, GDK_EXPOSE, TRUE);
		}

	}
	return FALSE;
}

static gboolean post_close_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		gchar *text;
		gint i, cnt;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(g_filter_string, 0x00, g_buff_size);

		cnt = nfui_listbox_get_box_count(NF_LISTBOX(g_lboxObj));

		for (i = 0; i < cnt; i++)
		{
			text = nfui_listbox_get_text_of_list(NF_LISTBOX(g_lboxObj), i, 0);
			if (i == 0) snprintf(g_filter_string, g_buff_size, "%s", text);
			else snprintf(g_filter_string+strlen(g_filter_string), g_buff_size-strlen(g_filter_string)-1, ",%s", text);
		}

		top = nfui_nfobject_get_top(obj);
		if (top) nfui_nfobject_destroy(top);
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
	if (evt->type == GDK_DELETE) 
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

void vw_dvabx_object_filter_edit_popup_open(NFWINDOW *parent, gint win_x, gint win_y, gchar *exception, gchar *filter, gint buff_size)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *lbox;
	NFOBJECT *obj;
	NFOBJECT *delete_btn;

	gchar *token_ptr;

	gint scl_w, scl_h;
	guint pos_x, pos_y;
	guint col_w = 428;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;


	g_exception_string = exception;
	g_filter_string = filter;
	g_buff_size = buff_size;


	win = (NFOBJECT*)nfui_nfwindow_new(parent, win_x, win_y, 592, 582);
	nfui_nfwindow_set_title(win, "MANUAL OBJECT FILTER EDIT");
	nfui_regi_post_event_callback(win, post_win_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 592, 598);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EDIT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);
	
	pos_x = 26;
	pos_y = 64;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OBJECT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	pos_x += (96 + 10);

	g_filterObj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_filterObj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(g_filterObj, 338, 40);
	nfui_nflabel_set_align((NFLABEL*)g_filterObj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(g_filterObj);
	nfui_regi_post_event_callback(g_filterObj, post_filter_input_event_cb);
	nfui_nffixed_put((NFFIXED*)fixed, g_filterObj, pos_x, pos_y);

	pos_x += (338 + 10);

	g_addObj = nftool_normal_button_create_type1("ADD", 80);
	nfui_regi_post_event_callback(g_addObj, post_add_button_event_handler);
	nfui_nfobject_show(g_addObj);
	nfui_nffixed_put((NFFIXED*)fixed, g_addObj, pos_x, pos_y);

	pos_x = 26;
	pos_y += (40 + 13);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	pos_x += (10 + 96);

	nfui_get_image_size(IMG_N_SCROLL_UP, &scl_w, &scl_h);
	col_w -= scl_w;

	g_lboxObj = nfui_listbox_new(1, &col_w, 40);
	nfui_listbox_set_skin_type(NF_LISTBOX(g_lboxObj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(g_lboxObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_listbox_support_multi_lang(NF_LISTBOX(g_lboxObj), FALSE);
	nfui_nfobject_set_size(g_lboxObj, 428, 370);
	nfui_nfobject_use_focus(g_lboxObj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(g_lboxObj);
	nfui_nffixed_put((NFFIXED*)fixed, g_lboxObj, pos_x, pos_y);

	token_ptr = strtok_r(filter, ",", &pnext);
	pbuf = pnext;

	while (token_ptr != 0)
	{
		g_message("%s, %d, token:%s", __FUNCTION__, __LINE__, token_ptr);
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &token_ptr);
		token_ptr = strtok_r(pbuf, ",", &pnext);
		pbuf = pnext;
	}

	pos_x = 126;
	pos_y += (370 + 34);

	obj = nftool_normal_button_create_type1("DELETE", 174);
	nfui_regi_post_event_callback(obj, post_delete_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	delete_btn = obj;

	pos_x += (174 + 6);

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_close_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(delete_btn, TRUE);

	nfui_page_open(PGID_DVABX_OBJECT_FILTER_EDIT_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DVABX_OBJECT_FILTER_EDIT_POPUP, win);
}

void vw_dvabx_object_filter_add_popup_open(NFWINDOW *parent, gint win_x, gint win_y, gchar *exception, gchar *filter, gint buff_size)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *lbox;
	NFOBJECT *obj;
	NFOBJECT *delete_btn;

	gchar *token_ptr;

	gint scl_w, scl_h;
	guint pos_x, pos_y;
	guint col_w = 428;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;


	g_exception_string = exception;
	g_filter_string = filter;
	g_buff_size = buff_size;


	win = (NFOBJECT*)nfui_nfwindow_new(parent, win_x, win_y, 592, 582);
	nfui_nfwindow_set_title(win, "MANUAL OBJECT FILTER EDIT");
	nfui_regi_post_event_callback(win, post_win_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 592, 598);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ADD", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);
	
	pos_x = 26;
	pos_y = 64;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	pos_x += (96 + 10);

	g_filterObj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_filterObj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(g_filterObj, 338, 40);
	nfui_nflabel_set_align((NFLABEL*)g_filterObj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(g_filterObj);
	nfui_regi_post_event_callback(g_filterObj, post_filter_input_event_cb);
	nfui_nffixed_put((NFFIXED*)fixed, g_filterObj, pos_x, pos_y);

	pos_x += (338 + 10);

	g_addObj = nftool_normal_button_create_type1("ADD", 80);
	nfui_regi_post_event_callback(g_addObj, post_add_button_event_handler);
	nfui_nfobject_show(g_addObj);
	nfui_nffixed_put((NFFIXED*)fixed, g_addObj, pos_x, pos_y);

	pos_x = 26;
	pos_y += (40 + 13);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	pos_x += (10 + 96);

	nfui_get_image_size(IMG_N_SCROLL_UP, &scl_w, &scl_h);
	col_w -= scl_w;

	g_lboxObj = nfui_listbox_new(1, &col_w, 40);
	nfui_listbox_set_skin_type(NF_LISTBOX(g_lboxObj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(g_lboxObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_listbox_support_multi_lang(NF_LISTBOX(g_lboxObj), FALSE);
	nfui_nfobject_set_size(g_lboxObj, 428, 370);
	nfui_nfobject_use_focus(g_lboxObj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(g_lboxObj);
	nfui_nffixed_put((NFFIXED*)fixed, g_lboxObj, pos_x, pos_y);

	token_ptr = strtok_r(filter, ",", &pnext);
	pbuf = pnext;

	while (token_ptr != 0)
	{
		g_message("%s, %d, token:%s", __FUNCTION__, __LINE__, token_ptr);
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &token_ptr);
		token_ptr = strtok_r(pbuf, ",", &pnext);
		pbuf = pnext;
	}

	pos_x = 126;
	pos_y += (370 + 34);

	obj = nftool_normal_button_create_type1("DELETE", 174);
	nfui_regi_post_event_callback(obj, post_delete_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	delete_btn = obj;

	pos_x += (174 + 6);

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_close_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(delete_btn, TRUE);

	nfui_page_open(PGID_DVABX_OBJECT_FILTER_EDIT_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DVABX_OBJECT_FILTER_EDIT_POPUP, win);
}
