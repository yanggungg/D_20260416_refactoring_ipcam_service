#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"


#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"


#include "vw_evt_type_data.h"
#include "ssm.h"


#define W_SIZE_W		(293)//(183)
#define W_SIZE_H(c)		((28 * c) + 26)


static NFWINDOW *g_curwnd = 0;
static gint g_rv = -1;



static gboolean post_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);
		g_rv = index;
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
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
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFOUTEVT_BUTTON_PRESS) 
	{
		nfui_nfobject_destroy(obj);
	}
	else if(evt->type == GDK_DELETE) 
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_EvtTypeData_Ctrl(NFWINDOW *parent, gchar **param, gint count, guint init, gint x, gint y)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj = NULL;

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	gint size_w, size_h;
	gint i;


	// return val
	g_rv = init;

	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);


	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, W_SIZE_W, W_SIZE_H(count));
	g_curwnd = win;
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_post_event_callback(win, post_window_event_cb);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, W_SIZE_W, W_SIZE_H(count));
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);



	for (i = 0; i < count; i++) {
		// radio button
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_radio_event_handler);

		if (i == init)
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

		if (i == 0) slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		else 		nfui_radio_button_add_group(NF_BUTTON(obj), slist);

		nfui_nffixed_put((NFFIXED*)fixed, obj, 30, (8 + (i * (size_h + 4))));


		// label
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(param[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, W_SIZE_W-72, 32);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)fixed, obj, 60, (4 + (i * (size_h + 4))));
	}

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_EVT_TYPE_DATA_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_EVT_TYPE_DATA_POPUP, win);
	
	return g_rv;
}


