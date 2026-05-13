
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
#include "viewers/objects/nfcombobox.h"

#include "vw_vkeyboard.h"

#include "vw_dvabx_group_filter_edit_popup.h"
#include "ix_mem.h"
#include "nf_api_dlva.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_filterObj;
static NFOBJECT *g_addObj;
static NFOBJECT *g_lboxObj;

static gint *g_devgroup_id;
static gint *g_register_id;
static size_t g_register_len;


static gboolean post_add_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint i, idx;
		gchar *filter;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_filterObj);

		for (i = 0; i < g_register_len; i++)
		{
			if (g_devgroup_id[idx] == g_register_id[i]) return FALSE;
		}

		g_register_id[g_register_len] = g_devgroup_id[idx];
		g_register_len++;

		filter = nfui_combobox_get_value((NFCOMBOBOX*)g_filterObj);

		nfui_listbox_set_text_single_column(NF_LISTBOX(g_lboxObj), filter);
		nfui_signal_emit(g_lboxObj, GDK_EXPOSE, TRUE);
	}
	return FALSE;
}

static gboolean post_delete_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint group_id[MAX_AIBOX_DB_GROUP_SIZE];
		gint i, idx;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		idx = nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj));
		if (idx < 0) return FALSE;

		g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, idx);

		memset(group_id, 0x00, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
		memcpy(group_id, g_register_id, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
		group_id[idx] = 0;

		memset(g_register_id, 0x00, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
		g_register_len = 0;

		for (i = 0; i < MAX_AIBOX_DB_GROUP_SIZE; i++)
		{
			if (group_id[i]) g_register_id[g_register_len++] = group_id[i];
		}

		nfui_listbox_delete(NF_LISTBOX(g_lboxObj), idx);
		nfui_signal_emit(g_lboxObj, GDK_EXPOSE, TRUE);
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

void vw_dvabx_group_filter_edit_popup_open(NFWINDOW *parent, gchar (*devgroup_name)[255], gint *devgroup_id, gint *register_id, size_t register_len)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *lbox;
	NFOBJECT *obj;
	NFOBJECT *delete_btn;

	gint scl_w, scl_h;
	guint pos_x, pos_y;
	guint col_w = 428;
	gint i, j;


	//g_outlist_buff = out_list;
	g_devgroup_id = devgroup_id;
	g_register_id = register_id;
	g_register_len = register_len;


	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-592)/2, (1080-582)/2, 592, 582);
	nfui_nfwindow_set_title(win, "GROUP FILTER EDIT");
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

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	pos_x += (96 + 10);

    g_filterObj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(g_filterObj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_align(NF_COMBOBOX(g_filterObj), NFALIGN_LEFT, 20);
	nfui_nfobject_support_multi_lang(g_filterObj, FALSE);
    nfui_nfobject_set_size(g_filterObj, 338, 40);
    nfui_nfobject_show(g_filterObj);
    nfui_nffixed_put((NFFIXED*)fixed, g_filterObj, pos_x, pos_y);

	for (i = 0; i < MAX_AIBOX_DB_GROUP_SIZE; i++)
	{
		if (strlen(devgroup_name[i])) nfui_combobox_append_data((NFCOMBOBOX*)g_filterObj, devgroup_name[i]);
		else break;			
	}

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

	for (i = 0; i < register_len; i++)
	{
		for (j = 0; j < MAX_AIBOX_DB_GROUP_SIZE; j++)
		{
			if (register_id[i] == devgroup_id[j]) {
				nfui_listbox_set_text_single_column(NF_LISTBOX(g_lboxObj), devgroup_name[j]);
			}
		}
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

	nfui_page_open(PGID_POPUPWND, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_POPUPWND, win);
}
