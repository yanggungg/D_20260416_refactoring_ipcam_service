
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "vw_desc.h"
#include "vw_copy_sched.h"


enum {
	ALL = 0,
	SUN,
	MON,
	TUE,
	WED,
	THU,
	FRI,
	SAT,
	HOLI,
	DAY_COUNT
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_chk[DAY_COUNT];

static guint g_day_mask = 0;



static /*inline*/ void set_day(guint day) 
{
	if(day == 7){
		g_day_mask |= (1 << (day+1) );
	}else{
		g_day_mask |= (1 << day);
	}
}


static /*inline*/ void unset_day(guint day) 
{
	if(day == 7){
		g_day_mask &= ~(1 << (day+1) );
	}else{
		g_day_mask &= ~(1 << (day) );
	}
}

static gboolean post_chk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gboolean act = FALSE;
		gint i;

		if(obj == g_chk[ALL]) {
			act = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

			for(i=SUN; i<DAY_COUNT; i++) {
				if(nfui_nfobject_is_disabled(g_chk[i]))
					continue;

				if(act) nfui_check_button_set_active((NFCHECKBUTTON*)g_chk[i], TRUE);
				else    nfui_check_button_set_active((NFCHECKBUTTON*)g_chk[i], FALSE);
			}

			for(i=SUN; i<DAY_COUNT; i++)  {
				if(nfui_nfobject_is_disabled(g_chk[i]))
					continue;

				nfui_signal_emit(g_chk[i], GDK_EXPOSE, FALSE);
			}

		}else {
			if(nfui_check_button_get_active((NFCHECKBUTTON*)g_chk[ALL])) {
				nfui_check_button_set_active((NFCHECKBUTTON*)g_chk[ALL], FALSE);
				nfui_signal_emit(g_chk[ALL], GDK_EXPOSE, FALSE);
			}
		}
	}

	return FALSE;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		for(i=SUN; i<DAY_COUNT; i++) {
			if(nfui_check_button_get_active((NFCHECKBUTTON*)g_chk[i])) {
				set_day(i - 1);
			}else {
				unset_day(i - 1);
			}
		}

		top = nfui_nfobject_get_top(obj);
		if(top)
			nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		top = nfui_nfobject_get_top(obj);
		if(top)
			nfui_nfobject_destroy(top);
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
	else if(evt->type == GDK_DELETE) 
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		g_curwnd = 0;		
		
		gtk_main_quit();
	}

	return FALSE;
}

guint VW_Copy_Sched(NFWINDOW *parent, guint day)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;

	gint i, j;
	
	gchar *strDay[] = {"ALL", 
					"SUNDAY", 
					"MONDAY",
					"TUESDAY", 
					"WEDNESDAY", 
					"THURSDAY", 
					"FRIDAY", 
					"SATURDAY",
					"HOLIDAY"
					}; 
	// init data
	g_day_mask = 0;
	set_day(day);


	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 960, 400, 413, 513);
	g_curwnd = win;

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 413, 513);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COPY SCHEDULE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	for(i=ALL, j=0; i<DAY_COUNT; i++) {

		

		if(i == (day + 1)) g_chk[i] = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		else			   g_chk[i] = (NFOBJECT*)nfui_checkbutton_new(FALSE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(g_chk[i]), NFCHECK_TYPE_NORMAL);
		nfui_nfobject_show(g_chk[i]);
		nfui_regi_post_event_callback(g_chk[i], post_chk_event_cb);

		if(i == (day + 1)) 
			nfui_check_button_sensitive(NF_CHECKBUTTON(g_chk[i]), FALSE);

		nfui_nffixed_put((NFFIXED*)fixed, g_chk[i], 26, 53 + (j * 42));

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(294));
		nfui_nfobject_set_size(obj, 200, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if (!ivsc.dfunc.support_usrdef_holiday){
			if(i == HOLI){
				nfui_nfobject_hide(g_chk[i]);
				nfui_nfobject_hide(obj);
			}
		}
		nfui_nffixed_put((NFFIXED*)fixed, obj, 60, 51 + (j * 42));
		j+=1;
	}

	/* button */
	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 26, 438);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 206, 438);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_COPY_SCHED_POPUP, win, ssm_get_cur_id(NULL));
	nfui_set_key_focus(g_chk[0], TRUE);
	gtk_main();

	nfui_page_close(PGID_COPY_SCHED_POPUP, win);

	return g_day_mask;
}

