
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"
#include "ssm.h"


#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"

#include "vw_channel_mask_ctrl.h"
#include "vw.h"

static NFWINDOW *g_curwnd = 0;

static guint64 g_ch_mask = 0;
static guint64 g_ch_omask = 0;
static gboolean g_ret = FALSE;



static gboolean init_mask(guint64 *mask)
{
	if(*mask & CAMERA_MASK_TYPE) 		 g_ch_omask = (*mask & ~CAMERA_MASK_TYPE);
	else if(*mask & ALARM_OUT_MASK_TYPE) g_ch_omask = (*mask & ~ALARM_OUT_MASK_TYPE);
	else 
		return FALSE;

	g_ch_mask = g_ch_omask;

	return TRUE;
}


static gboolean post_chk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint ch = 0;

		ch = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "channel"));

		if(nfui_check_button_get_active((NFCHECKBUTTON*)obj)) 
			g_ch_mask |= (1LL << ch);
		else 
			g_ch_mask &= ~(1LL << ch);
	}

	return FALSE;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		g_ret = TRUE;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
		
		g_ret = FALSE;
		g_ch_mask = g_ch_omask;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
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
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_EXIT) {
			g_ret = FALSE;
			g_ch_mask = g_ch_omask;
		}
	}
	else if (evt->type == GDK_DELETE) {
	
		g_curwnd = 0;
		gtk_main_quit();

	}

	return FALSE;
}

gboolean VW_ChannelMask_Ctrl(NFWINDOW *parent, gchar *title, gint x, gint y, guint64 *mask)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj = NULL;
	NFOBJECT *ok_btn = NULL;

	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];

	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(414), COLOR_IDX(415), COLOR_IDX(416), COLOR_IDX(418)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(417), COLOR_IDX(431), COLOR_IDX(432), COLOR_IDX(418)};

	gint i = 0;
	gchar buf[4];

    gint win_width, win_height;

    win_width = 383;
    win_height = 112;    

    if(*mask & CAMERA_MASK_TYPE) 
    {
#if defined(GUI_16CH_SUPPORT)
        win_height += 34 * 2;
#elif defined(GUI_32CH_SUPPORT)
        win_height += 34 * 4;
#else
        win_height += 34;
#endif
    }

    if(*mask & ALARM_OUT_MASK_TYPE)     //include camera mask
    {
        win_height += 34 * ((ALARM_OUT_COUNT / 8) + (ALARM_OUT_COUNT % 8 ? 1 : 0));
    } 

	// init data
	if(!init_mask(mask))
		return FALSE;

	if (y + win_height > DISPLAY_ACTIVE_HEIGHT) y = DISPLAY_ACTIVE_HEIGHT - win_height;

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, win_width, win_height);
	g_curwnd = win;
	nfui_regi_post_event_callback(win, post_win_event_cb);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win_width, win_height);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	/* ch */
	chk_inact_img[0] = nfui_get_image_from_file((IMG_N_CH_SQ_INACTIVE), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_O_CH_SQ_INACTIVE), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_P_CH_SQ_INACTIVE), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_D_CH_SQ_INACTIVE), NULL);

	chk_act_img[0] = nfui_get_image_from_file((IMG_N_CH_SQ_ACTIVE), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_O_CH_SQ_ACTIVE), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_P_CH_SQ_ACTIVE), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_D_CH_SQ_ACTIVE), NULL);

	i = 0;
	while(1) {
		if(*mask & ALARM_OUT_MASK_TYPE) 
		{
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
			g_sprintf(buf, "A%d", i + 1);
#else
#if defined(_IPX_1648P4E) || defined(_IPX_0824P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			if(i >= GUI_CHANNEL_CNT+NVR_RELAY_OUT) g_sprintf(buf, "A%d", (i - GUI_CHANNEL_CNT - NVR_RELAY_OUT) + 1);
			else if(i >= GUI_CHANNEL_CNT) g_sprintf(buf, "R%d", (i - GUI_CHANNEL_CNT) + 1);
#else
			if(i >= GUI_CHANNEL_CNT) g_sprintf(buf, "A%d", (i - GUI_CHANNEL_CNT) + 1);
#endif
			else					 g_sprintf(buf, "%d", i + 1);
#endif			
		}
		else 
		{
			g_sprintf(buf, "%d", i + 1);
		}

		if(*mask & (1LL << i)) 
		{
			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		}
		else				
		{
			obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		}
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_DEF);		
		nfui_check_button_set_inactive_image(NF_CHECKBUTTON(obj), chk_inact_img);
		nfui_check_button_set_active_image(NF_CHECKBUTTON(obj), chk_act_img);	
		nfui_check_set_text(NF_CHECKBUTTON(obj), buf);
		nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
		nfui_check_set_active_pango_font(NF_CHECKBUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);		
		nfui_nfobject_set_size(obj, 32, 32);
		nfui_regi_post_event_callback(obj, post_chk_event_cb);
		nfui_nfobject_show(obj);

		nfui_nfobject_set_data(obj, "channel", GINT_TO_POINTER(i));

		if(*mask & CAMERA_MASK_TYPE) 
		{
			nfui_nffixed_put((NFFIXED*)fixed, obj, 53 + (34 * (i % 8)), 60 + (34 * (i / 8)));
			if(GUI_CHANNEL_CNT <= ++i) 
				break;
		}
		else if(*mask & ALARM_OUT_MASK_TYPE) 
		{
			nfui_nffixed_put((NFFIXED*)fixed, obj, 53 + (34 * (i % 8)), 60 + (34 * (i / 8)));

			if((CAM_ALARM_OUT + DVR_ALARM_OUT) <= ++i) 
				break;
		}
	}

	/* select/deselect */
	obj = nftool_normal_button_create_popup_type2("OK", 150);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfbutton_set_spacing((NFBUTTON*)obj, NORMAL_SPACING);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 37, win_height-49);
	ok_btn = obj;

	obj = nftool_normal_button_create_popup_type2("CANCEL", 150);
	nfui_nfbutton_set_spacing((NFBUTTON*)obj, NORMAL_SPACING);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 189, win_height-49);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(ok_btn, TRUE);
	
	nfui_page_open(PGID_CH_MASK_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CH_MASK_POPUP, win);
	
	if(g_ret)
		*mask = g_ch_mask;

	return g_ret;
}
