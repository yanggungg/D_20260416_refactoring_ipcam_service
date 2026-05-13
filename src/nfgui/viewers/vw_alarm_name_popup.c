#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"

#include "vw_alarm_name_popup.h"
#include "ssm.h"



#define ALRM_NAME_POS_X						((DISPLAY_ACTIVE_WIDTH - ALRM_NAME_SIZE_W)/2)
#define ALRM_NAME_POS_Y						((DISPLAY_ACTIVE_HEIGHT - ALRM_NAME_SIZE_H)/2)
#define ALRM_NAME_SIZE_W					(534)
#define ALRM_NAME_SIZE_H					(250)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_anWin = NULL;
static NFOBJECT *g_anLb = NULL;
static NFOBJECT *g_btn = NULL;

static guint g_tid = 0;


static gboolean auto_hide(gpointer data)
{
	NFUTIL_THREADS_ENTER();
	nfui_nfobject_hide(g_anWin);
	nfui_page_close(PGID_OSD_POPUP, g_anWin);
	NFUTIL_THREADS_LEAVE();
	
	return FALSE;
}

static void destroy_cb(gpointer data)
{
	if(g_tid != 0) 	g_tid = 0;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		nfui_nfobject_hide(g_anWin);
		nfui_page_close(PGID_OSD_POPUP, g_anWin);
	}

	return FALSE;
}

static gboolean post_an_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable = NULL;
		GdkGC *gc = NULL;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfutil_draw_image(drawable, gc, IMG_POPUP_OSD_BG, (gint)obj->x, (gint)obj->y, (gint)obj->width, (gint)obj->height, NFALIGN_LEFT, 0);
	
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean post_an_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFOUTEVT_BUTTON_PRESS) {
		guint sec = 0;

		sec = DAL_get_evtNoti_oPop_duration();
		if(sec > 300) {
			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_OSD_POPUP, g_anWin);
		}
	}
	else if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    nfui_nfobject_hide(g_anWin);
	return FALSE;
}

void VW_Create_OSD_Popup(NFWINDOW *parent)
{
	NFOBJECT *anFixed;
	NFOBJECT *obj;
	GdkPixbuf *bgImg;
	GdkPixbuf *pbBG;

	/* window */
	g_anWin = (NFOBJECT*)nfui_nfwindow_new(parent, ALRM_NAME_POS_X, ALRM_NAME_POS_Y, ALRM_NAME_SIZE_W, ALRM_NAME_SIZE_H);
	g_curwnd = g_anWin;
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_anWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_anWin, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_anWin, returnkey_proc);
	//nfui_nfwindow_set_modal((NFWINDOW*)g_anWin, FALSE);
	nfui_regi_post_event_callback(g_anWin, post_an_window_event_cb);


	/* fixed */
	anFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(anFixed, ALRM_NAME_SIZE_W, ALRM_NAME_SIZE_H);
	nfui_regi_post_event_callback(anFixed, post_an_fixed_event_cb);
	nfui_nfobject_show(anFixed);

	bgImg = nfui_get_image_from_file(IMG_POPUP_OSD_BG, NULL);

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, ALRM_NAME_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, 4, ALRM_NAME_SIZE_H-132, ALRM_NAME_SIZE_W-12, 32, pbBG, 0, 0);

	g_anLb = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_pango_font((NFIMAGE*)g_anLb, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfobject_set_size(g_anLb, ALRM_NAME_SIZE_W-12, 32);
	nfui_nfobject_use_focus(g_anLb, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_anLb);
	nfui_nffixed_put((NFFIXED*)anFixed, g_anLb, 4, ALRM_NAME_SIZE_H-132);

#if 0
/* */
	g_anLb = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(315));
	nfui_nfobject_set_size(g_anLb, ALRM_NAME_SIZE_W-12, 32);
	nfui_nflabel_set_align((NFLABEL*)g_anLb, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(g_anLb, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(g_anLb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(g_anLb);
	nfui_nffixed_put((NFFIXED*)anFixed, g_anLb, 4, ALRM_NAME_SIZE_H-132);
#endif

	obj = nftool_normal_button_create_type1("OK", 182);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)anFixed, obj, 176, ALRM_NAME_SIZE_H-62);
	g_btn = obj;


	nfui_nfwindow_add((NFWINDOW*)g_anWin, anFixed);
	nfui_run_main_event_handler(g_anWin);
	nfui_nfobject_hide(g_anWin);
}

int VW_Destroy_OSD_Popup()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

gboolean VW_Show_OSD_Popup(gchar *str)
{
	guint sec; 

	if(!g_anWin) return FALSE;
	if(!g_anLb) return FALSE;


	// start timer
	if(g_tid) 
		g_source_remove(g_tid);

	sec = DAL_get_evtNoti_oPop_duration();
	if(sec <= 300) 
		g_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, (sec * 1000), auto_hide, NULL, destroy_cb);			

	nfui_nfimage_set_text((NFLABEL*)g_anLb, str);
	nfui_signal_emit(g_anLb, GDK_EXPOSE, FALSE);

	// show
	if(!nfui_nfobject_is_shown(g_anWin)) {
		nfui_nfobject_show(g_anWin);

		nfui_make_key_hierarchy((NFWINDOW*)g_anWin);
		nfui_set_key_focus(g_btn, TRUE);

		nfui_page_open(PGID_OSD_POPUP, g_anWin, ssm_get_cur_id(NULL));
	}

	return TRUE;
}

gboolean VW_Hide_OSD_Popup()
{
	if(!g_anWin) return FALSE;
	if(!g_anLb) return FALSE;

	if(nfui_nfobject_is_shown(g_anWin)) {
		nfui_nfobject_hide(g_anWin);
		nfui_page_close(PGID_OSD_POPUP, g_anWin);
	}

	if(g_tid) 
		g_source_remove(g_tid);

	return TRUE;
}

gint VW_OSD_Popup_is_untilkey()
{
	if(!g_anWin) return -1;
	if(!g_anLb) return -1;
	if(nfui_nfobject_is_shown(g_anWin) == FALSE) return -1;

	if (nfui_nfobject_is_shown(g_anWin) && (g_tid == 0)) return 0;

	return -1;
}


