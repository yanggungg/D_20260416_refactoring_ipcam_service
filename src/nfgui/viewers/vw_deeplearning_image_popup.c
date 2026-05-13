#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nflistbox.h"
#include "objects/nftable.h"

#include "vw_deeplearning_image_popup.h"
#include "ix_mem.h"
#include "scm.h"
#include "vw_snapshot.h"

#define	WIN_HEIGHT	(635)
#define WIN_WIDTH	(660)
#define	GAP			(10)

static NFWINDOW *g_curwnd = 0;

static DOBJECT_INFO_T g_dobj_info;
static GdkPixbuf *g_deep_pixbuf = 0;

static gint g_ret_val = 0;

static NFOBJECT *g_cap_obj;
static NFOBJECT *g_tit_obj;
static NFOBJECT *g_des_obj;

static gint _draw_frame(NFOBJECT *obj)
{
	GdkGC *gc;	
	GdkDrawable *drawable;

	float top_x, top_y;
	float bottom_x, bottom_y;

	GdkPoint draw_pt[4] = {0, };
	GdkPoint text_pt;
	gint gap_x, gap_y;
	gint text_w, text_h;

	gchar strBuf[128];
	gchar strTemp[64];

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);

	if (g_deep_pixbuf) {
		nfutil_draw_pixbuf(drawable, gc, g_deep_pixbuf, GAP, GAP + 40, -1, -1, NFALIGN_CENTER, 0);
	}

	nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

	top_x = MAX(g_dobj_info.coords[0], 0);
	top_y = MAX(g_dobj_info.coords[1], 0);
	bottom_x = MIN(g_dobj_info.coords[2], 1);
	bottom_y = MIN(g_dobj_info.coords[3], 1);

	draw_pt[0].x = gap_x + (gint)(top_x*640);
	draw_pt[0].y = gap_y + (gint)(top_y*360);
	draw_pt[1].x = gap_x + (gint)(bottom_x*640); 
	draw_pt[1].y = gap_y + (gint)(top_y*360);        
	draw_pt[2].x = gap_x + (gint)(bottom_x*640);
	draw_pt[2].y = gap_y + (gint)(bottom_y*360);           
	draw_pt[3].x = gap_x + (gint)(top_x*640);
	draw_pt[3].y = gap_y + (gint)(bottom_y*360);

	gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)));
	gdk_draw_polygon(drawable, gc, 0, draw_pt, 4);

	memset(strTemp, 0x00, sizeof(strTemp));
	dtf_get_local_time(g_dobj_info.ftime, strTemp);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_snprintf(strBuf, 128, " CH%d  (%s) ", g_dobj_info.ch+1, strTemp);

	text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_4), strBuf, NORMAL_SPACING);
	text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_4), strBuf, NORMAL_SPACING);

	text_pt.x = draw_pt[0].x;
	text_pt.y = draw_pt[0].y-text_h;
	text_pt.x = MIN(text_pt.x, gap_x+640-text_w);
	text_pt.x = MAX(gap_x+2, text_pt.x);
	text_pt.y = MIN(text_pt.y, gap_y+360-text_h);
	text_pt.y = MAX(gap_y+2, text_pt.y);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)));
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)), drawable, gc, 
		strBuf, text_pt.x, text_pt.y, text_w, text_h, 
		nffont_get_pango_font(NFFONT_MINI_NORMAL_4), &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_202020)), NFALIGN_LEFT, 0);

	nfui_nfobject_gc_unref(gc);

	return 0;
}

static gboolean post_frame_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;	
	GdkDrawable *drawable;

	if(evt->type == GDK_EXPOSE) 
	{
		_draw_frame(obj);
	}
	else if (evt->type == GDK_BUTTON_PRESS || evt->type == GDK_BUTTON_RELEASE)
	{
		_draw_frame(obj);

		if (g_dobj_info.act_playbtn)
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);
			nfutil_draw_image(drawable, gc, "ai_pop_play_p.png", (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
			nfui_nfobject_gc_unref(gc);		

			if (evt->type == GDK_BUTTON_RELEASE)
			{
				NFOBJECT *topwin;

				g_ret_val = 1;
				topwin = nfui_nfobject_get_top(obj);
				nfui_nfobject_destroy(topwin);				
			}
		}
	}
	else if (evt->type == GDK_ENTER_NOTIFY)
	{
		_draw_frame(obj);

		if (g_dobj_info.act_playbtn)
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);
			nfutil_draw_image(drawable, gc, "ai_pop_play_o.png", (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
			nfui_nfobject_gc_unref(gc);
		}
	}
	else if (evt->type == GDK_LEAVE_NOTIFY)
	{
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == GDK_DELETE)
    {
		g_object_unref(g_deep_pixbuf);
    }
	
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (!g_curwnd) return 0;

		nfui_nfobject_destroy((NFOBJECT*)g_curwnd);	
		return 0;
	}
	
	return FALSE;
}

static gboolean post_add_face_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_PRESS)
	{
		int ch;
		ch = g_dobj_info.ch;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		vw_quick_add_face_popup_open(g_curwnd, ch, (void *)g_deep_pixbuf, 0, FALSE);
	}
	
	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;	
	GdkDrawable *drawable;
	GdkPixbuf *pbuf;

	gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }
	
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_DELETE)
	{
	    nfui_page_close(PGID_POPUPWND, g_curwnd);
		gtk_main_quit();
		g_curwnd = 0;
	}
    else if(evt->type == NFOUTEVT_BUTTON_PRESS)
    {
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_EXIT)
		{
			topwin = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(topwin);	
		}
	}

	return FALSE;
}

gboolean vw_deeplearning_image_popup_open(NFWINDOW *parent, gint pos_x, gint pos_y, DOBJECT_INFO_T *dobj_info, GdkPixbuf *dpixbuf) 
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	NFOBJECT *tbl;

	gchar strBuf[128];
	gchar strTemp[64];
	guint list_w[] = {WIN_WIDTH-GAP*2};
	guint table_w[] = {WIN_WIDTH-GAP*2};
	gchar *token_ptr;
	guint i;

	gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

	g_ret_val = 0;

	memcpy(&g_dobj_info, dobj_info, sizeof(DOBJECT_INFO_T));
	g_deep_pixbuf = gdk_pixbuf_scale_simple(dpixbuf, 640, 360, 0);
	if (!g_deep_pixbuf) return 0;

	main_wnd = (NFOBJECT*)nfui_nfwindow_new(parent, pos_x, pos_y, WIN_WIDTH, WIN_HEIGHT);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)main_wnd, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)main_wnd, GDK_MOTION_NOTIFY, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)main_wnd, GDK_BUTTON_PRESS, TRUE);
    gtk_widget_add_events(((NFWINDOW*)main_wnd)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size((NFOBJECT*)main_fixed, WIN_WIDTH, WIN_HEIGHT);
	nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	memset(strTemp, 0x00, sizeof(strTemp));
	memset(strBuf, 0x00, sizeof(strBuf));
	dtf_get_local_datetime(dobj_info->ftime, strTemp);
	g_sprintf(strBuf, "CH%d)  %s", dobj_info->ch+1, strTemp);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 280, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, GAP, GAP);
	nfui_nflabel_set_text((NFLABEL*)obj, strBuf);

	obj = nfui_nfbutton_new();
	nfui_nfobject_set_size(obj, 640, 360);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, GAP, GAP + 40);
	nfui_regi_post_event_callback(obj, post_frame_button_event_handler);

	tbl = (NFOBJECT*)nfui_nftable_new(1, 2, 1, 1, table_w, 32);
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, GAP, GAP + 40 + 362);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(387));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);
	g_cap_obj = obj;

	memset(strBuf, 0x00, sizeof(strBuf));
	if (dobj_info->caption[0] != 0x00)	memcpy(strBuf, dobj_info->caption, sizeof(strBuf));
	else	memcpy(strBuf, dobj_info->rule_name, sizeof(strBuf));
	nfui_nflabel_set_text((NFLABEL*)g_cap_obj, strBuf);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(387));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 1);
	g_tit_obj = obj;

	memset(strBuf, 0x00, sizeof(strBuf));
	if (dobj_info->title[0] != 0x00)	memcpy(strBuf, dobj_info->title, sizeof(strBuf));
	else if (dobj_info->caption[0] == 0x00)	memcpy(strBuf, dobj_info->class_name, sizeof(strBuf));
	for (i=0; i<strlen(strBuf); i++)
	{
		if (strBuf[i] == '\n')
		{
			strBuf[i] ='\0';
			break;
		}
	}
	nfui_nflabel_set_text((NFLABEL*)g_tit_obj, strBuf);

	fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);
    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);

	obj = nfui_listbox_new(1, list_w, 32);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), 0, NFALIGN_LEFT);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_support_tooltip(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, WIN_WIDTH-GAP*2, 98);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, GAP, GAP + 40 + 362 + (32*2)+2);
	g_des_obj = obj;

	nfui_listbox_delete_all(NF_LISTBOX(g_des_obj));
	token_ptr = strtok_r(dobj_info->description, "\n", &pnext);
	pbuf = pnext;

	while (token_ptr != 0)
	{
		nfui_listbox_set_text(NF_LISTBOX(g_des_obj), &token_ptr);
		token_ptr = strtok_r(pbuf, "\n", &pnext);
		pbuf = pnext;
	}

	obj = nftool_normal_button_create_popup_type2("CLOSE", 180);
    nfui_nfobject_show(obj);    
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, main_wnd->width-GAP-180, main_wnd->height-50);
    nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	obj = nftool_normal_button_create_popup_type2("ADD FACE", 180);
    nfui_nfobject_show(obj);    
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, GAP, main_wnd->height-50);
    nfui_regi_post_event_callback(obj, post_add_face_button_event_handler);
	if (!ivsc.dfunc.support_face) nfui_nfobject_hide(obj);

    nfui_nfobject_show(main_fixed);
    nfui_run_main_event_handler(g_curwnd);
    nfui_nfobject_show(main_wnd);

	nfui_make_key_hierarchy(main_wnd);
    nfui_page_open(PGID_POPUPWND, g_curwnd, nfui_get_last_user());

    nfui_nfobject_show(g_curwnd);

	gtk_main();

	return g_ret_val;
}

gboolean vw_deeplearning_image_popup_close()
{
	if (!g_curwnd) return 0;

	nfui_nfobject_destroy((NFOBJECT*)g_curwnd);	
	return 0;
}
