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

#include "modules/ssm.h"

#include "vw_disk_raid_conf_step2.h"


#define WND_SIZE_W				(570)
#define WND_SIZE_H				(350)

#define POP_POS_X				((DISPLAY_ACTIVE_WIDTH - WND_SIZE_W)/4*2)
#define POP_POS_Y				((DISPLAY_ACTIVE_HEIGHT - WND_SIZE_H)/2)


static NFWINDOW *g_curwnd = 0;



static gboolean post_disk_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable = NULL;
		GdkGC *gc = NULL;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfutil_draw_image(drawable, gc, MK_IMG_DISK_RAID_WARN_BG, (gint)obj->x, (gint)obj->y, (gint)obj->width, (gint)obj->height, NFALIGN_LEFT, 0);
	
		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
		nfui_page_close(PGID_DISK_RAID_CONF_STEP2, (NFOBJECT*)g_curwnd);
	}

	return FALSE;
}

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
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


void VW_DiskRaid_ConfStep2_Open(NFWINDOW *parent)
{
	NFOBJECT *window = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj = NULL;

	GdkPixbuf *btn_img[NFOBJECT_STATE_COUNT];

	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(210), COLOR_IDX(211), COLOR_IDX(213), COLOR_IDX(214)};
	gint btn_w, btn_h;
	gint i;

	gchar *msg = "SELECT A SOURCE DISK TO USE BETWEEN THE TWO\nDISKS. CLICK ONE OF THE BELOW DISKS.";
	gchar buf[24];



	/* window */
	window = (NFOBJECT*)nfui_nfwindow_new(parent, POP_POS_X, POP_POS_Y, WND_SIZE_W, WND_SIZE_H);
	g_curwnd = (NFWINDOW*)window;


	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WND_SIZE_W, WND_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_disk_fixed_event_cb);
	nfui_nfobject_show(fixed);


	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONFIGURATION STEP2", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 552, 36);
	//nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 9, 4);


	/* message */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(msg, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
	nfui_nfobject_set_size(obj, 518, 52);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 26, 64);

	
	/* button */
	btn_img[0] = nfui_get_image_from_file((IMG_DISK_RAID_BT_N), NULL);
	btn_img[1] = nfui_get_image_from_file((IMG_DISK_RAID_BT_O), NULL);
	btn_img[2] = nfui_get_image_from_file((IMG_DISK_RAID_BT_P), NULL);
	btn_img[3] = nfui_get_image_from_file((IMG_DISK_RAID_BT_D), NULL);

	memset(buf, 0x00, sizeof(buf));
	for(i=0; i<4; i++) {
		if(i == 0) sprintf(buf, "DISK #%d\n(MASTER)", (i + 1));
		else sprintf(buf, "DISK #%d\n(NORMAL)", (i + 1));

		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_img, buf);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)font_color);
		nfui_nfobject_show(obj);
		//nfui_regi_post_event_callback(obj, post_ao_all_btn_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 40 + (i * 124), 142);			
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

	nfui_page_open(PGID_DISK_RAID_CONF_STEP2, window, ssm_get_cur_id(NULL));
}
