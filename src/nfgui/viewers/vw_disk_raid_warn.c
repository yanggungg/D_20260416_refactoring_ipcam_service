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
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"

#include "modules/ssm.h"

#include "vw_disk_raid_warn.h"


#define WND_SIZE_W				(650)
#define WND_SIZE_H				(350)

#define POP_POS_X				((DISPLAY_ACTIVE_WIDTH - WND_SIZE_W)/4*2)
#define POP_POS_Y				((DISPLAY_ACTIVE_HEIGHT - WND_SIZE_H)/2)


static NFWINDOW *g_curwnd = 0;

static gboolean g_ret = FALSE;



static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		g_curwnd = 0;

		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_disk_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;
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
	else if(evt->type == GDK_DELETE) {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	}
	
	return FALSE;
}

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;

	if(evt->type == GDK_BUTTON_RELEASE) {
		g_ret = TRUE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}
	return FALSE;
}

static gboolean post_cancel_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}
	return FALSE;
}

gboolean VW_DiskRaid_Warn_Open(NFWINDOW *parent)
{
	NFOBJECT *window = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj = NULL;

	gint btn_w, btn_h;
	gint i;

	gchar *msg = "ALL CONFIGURATION YOU HAVE MADE WILL RESULT\nIN THE FOLLOWING ACTIONS AFTER REBOOTING.";
	gchar *lmsg[] = {"ALL DISKS WILL BE FORMATTED.",
					 "SEVERAL DISKS WILL BE FORMATED.",
					 "SYNCHRONIZATION WILL BE APPLIED."};

	
	g_ret = FALSE;

	/* window */
	window = (NFOBJECT*)nfui_nfwindow_new(parent, POP_POS_X, POP_POS_Y, WND_SIZE_W, WND_SIZE_H);
	nfui_regi_post_event_callback(window, post_window_event_cb);
	g_curwnd = (NFWINDOW*)window;


	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WND_SIZE_W, WND_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_disk_fixed_event_cb);
	nfui_nfobject_show(fixed);


	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WARNING LIST", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 552, 36);
	//nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 9, 4);


	/* message */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(msg, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
	nfui_nfobject_set_size(obj, WND_SIZE_W-20, 52);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, 64);


	/* list */
	for(i=0; i<3; i++) {
		obj = nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
		nfui_check_get_size(obj, &btn_w, &btn_h);
		nfui_check_button_sensitive(NF_CHECKBUTTON(obj), FALSE);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 103, ((i * 42) + 142));
	}

	for(i=0; i<3; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lmsg[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 6);
		nfui_nfobject_set_size(obj, (503 - btn_w), 27);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, (103 + btn_w + 4), ((i * 42) + 142));
	}

	/* button */
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 90, 288);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 288, 288);


	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_DISK_RAID_WARN, window, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DISK_RAID_WARN, window);

	return g_ret;
}


