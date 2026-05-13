
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nftable.h"

#include "vsm.h"
#include "vw_live_div_popup.h"
#include "ssm.h"

#define DIV_WIN_W			(230)
#define DIV_WIN_H			(88)
#define DIV_WIN_Y			(DISPLAY_ACTIVE_HEIGHT - 108 - DIV_WIN_H - 1)


#define DIV_BTN_X			(9)
#define DIV_BTN_Y			(9)
#define DIV_BTN_GAP			(1)



static NFWINDOW *g_curwnd = 0;


static gboolean change_live_display_by_user_def(gint valid_num)
{
    gint i;
    gchar win[VSM_WIN_MAX];

    memset(win, 0xff, sizeof(win));

	// change div
	vsm_change_div_by_user(valid_num);

	return TRUE;
}

static gboolean _post_div_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

			//	if(kpid == KEYPAD_EXIT)
			//	{
			//		nfui_nfobject_destroy(obj);
			//	}
			}
			break;

		case NFOUTEVT_BUTTON_PRESS: 
			{
				nfui_nfobject_destroy(obj);
			}
			break;

		case GDK_DELETE: 
			{
				g_curwnd = 0;
				gtk_main_quit();
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean _post_div_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
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

static gboolean _post_div6_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV6);
		var_set_active_layout(-1);
	}
	return FALSE;
}

static gboolean _post_div8_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV8);
		var_set_active_layout(-1);
	}
	return FALSE;
}

static gboolean _post_user_1_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		change_live_display_by_user_def(0);
	}

	return FALSE;
}

static gboolean _post_user_2_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		change_live_display_by_user_def(1);
	}

	return FALSE;
}

static gboolean _post_user_3_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		change_live_display_by_user_def(2);
	}

	return FALSE;
}

static gboolean _post_user_4_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		change_live_display_by_user_def(3);
	}

	return FALSE;
}

static gboolean _post_seq_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vsm_live_cmd_sequence();
	}
	return FALSE;
}

gboolean VW_Create_Div_Popup_Create(NFWINDOW *parent, guint pos_x)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj = NULL;

	GdkPixbuf *div6_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *div8_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *seq_img[NFOBJECT_STATE_COUNT];

	gint i;
	guint size_w, size_h;

	div6_img[0] = nfui_get_image_from_file((IMG_N_POPUP_DIV_6), NULL);
	div6_img[1] = nfui_get_image_from_file((IMG_O_POPUP_DIV_6), NULL);
	div6_img[2] = nfui_get_image_from_file((IMG_P_POPUP_DIV_6), NULL);
	div6_img[3] = nfui_get_image_from_file((IMG_D_POPUP_DIV_6), NULL);

	div8_img[0] = nfui_get_image_from_file((IMG_N_POPUP_DIV_8), NULL);
	div8_img[1] = nfui_get_image_from_file((IMG_O_POPUP_DIV_8), NULL);
	div8_img[2] = nfui_get_image_from_file((IMG_P_POPUP_DIV_8), NULL);
	div8_img[3] = nfui_get_image_from_file((IMG_D_POPUP_DIV_8), NULL);

	seq_img[0] = nfui_get_image_from_file((IMG_N_POPUP_DIV_SEQ), NULL);
	seq_img[1] = nfui_get_image_from_file((IMG_O_POPUP_DIV_SEQ), NULL);
	seq_img[2] = nfui_get_image_from_file((IMG_P_POPUP_DIV_SEQ), NULL);
	seq_img[3] = nfui_get_image_from_file((IMG_D_POPUP_DIV_SEQ), NULL);

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, pos_x, DIV_WIN_Y, DIV_WIN_W, DIV_WIN_H);
	g_curwnd = win;
	nfui_regi_post_event_callback(win, _post_div_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DIV_WIN_W, DIV_WIN_H);
	nfui_regi_post_event_callback(fixed, _post_div_fixed_event_cb);
	nfui_nfobject_show(fixed);

	nfui_get_image_size(IMG_N_POPUP_DIV_6, &size_w, &size_h);
	pos_x = DIV_BTN_X;

	for(i = 0; i < 3; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		if(i == 0) 		nfui_nfbutton_set_image(NF_BUTTON(obj), div6_img);
		else if(i == 1) nfui_nfbutton_set_image(NF_BUTTON(obj), div8_img);
		else if(i == 2) nfui_nfbutton_set_image(NF_BUTTON(obj), seq_img);
		nfui_nfobject_set_size(obj, size_w, size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, DIV_BTN_Y);

		if(i == 0)
		{
			nfui_nfobject_set_tooltip(obj, "6 SPLIT SCREEN");				
			nfui_regi_post_event_callback(obj, _post_div6_event_cb);
		}
		else if(i == 1) 
		{
			nfui_nfobject_set_tooltip(obj, "8 SPLIT SCREEN");
			nfui_regi_post_event_callback(obj, _post_div8_event_cb);
		}
		else if(i == 2) 
		{
			nfui_nfobject_set_tooltip(obj, "SEQUENCE");
			nfui_regi_post_event_callback(obj, _post_seq_event_cb);
		}

		pos_x += (size_w + 1);
	}

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_LIVE_DIV_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_LIVE_DIV_POPUP, win);
}

void VW_Destroy_Div_Popup()
{
	if (g_curwnd) nfui_nfobject_destroy(g_curwnd);
}
