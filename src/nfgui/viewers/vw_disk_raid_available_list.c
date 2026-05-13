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
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"

#include "services/scm.h"
#include "modules/ssm.h"

#include "vw_disk_raid.h"
#include "vw_disk_raid_available_list.h"


//#define WND_SIZE_W				(570)
#define WND_SIZE_W				(672)
//#define WND_SIZE_H				(350)
#define WND_SIZE_H				(400)

#define POP_POS_X				((DISPLAY_ACTIVE_WIDTH - WND_SIZE_W)/4*2)
#define POP_POS_Y				((DISPLAY_ACTIVE_HEIGHT - WND_SIZE_H)/2)


static NFWINDOW *g_curwnd = 0;
//static DISK_CAPINFO_T *disk_i;
static SATA_INFO_T sata_i;
static guint g_dmsk = 0;			// disk mask value
static guchar g_rmode = 0;



static gboolean post_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint chk_index;

		chk_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "chk index"));
		if(nfui_check_button_get_active((NFCHECKBUTTON*)obj)) 
			g_dmsk |= (1 << chk_index);
		else
			g_dmsk &= ~(1 << chk_index);
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
	
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		NFOBJECT *top = NULL;
		gint i;
		gint cnt = 0;

		for(i=0; i<5; i++) {
			if(g_dmsk & (1 << i)) 
				cnt++;
		}

		if(g_rmode == RAID_1_MODE) {
			if(cnt <= 1 || cnt > 2) {
				nftool_mbox(g_curwnd, "WARNING", "The number of disks is not enough or\ntoo much to create a RAID1.\nYou need to select 2 disks for RAID1.", NFTOOL_MB_OK);
				return FALSE;
			}
		}else if(g_rmode == RAID_5_MODE) {
			if(cnt <= 1 || cnt < 3) {
				nftool_mbox(g_curwnd, "WARNING", "You selected less than 3 disks.\nTo configure RAID5, at least 3 disks are requried.", NFTOOL_MB_OK);
				return FALSE;
			}
		}

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

		g_dmsk = 0;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}
	return FALSE;
}


guint VW_DiskRaid_AvailableList_Open(NFWINDOW *parent, STRG_TYPE_E strg_type, guchar raid_mode, guint disk_mask)
{
	NFOBJECT *window = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *tbl = NULL;
	NFOBJECT *fixed_temp = NULL;
	NFOBJECT *obj = NULL;

	GdkPixbuf *pbIcon[6];
	
	gchar *strRow[] = {"DISK", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strCol[] = {"MODEL", "CAPACITY", "REBUILD"};

	guint table_w[] = {172, 202, 152, 100};
	guint chk_w, chk_h;
	gint i, j;
	gchar buf[32];



	g_dmsk = 0;
	g_rmode = raid_mode;

	// disk info
	//disk_i = get_disk_info(strg_type);
	memset(&sata_i, 0x00, sizeof(SATA_INFO_T));
	scm_get_sata_info(strg_type, &sata_i);

	/* window */
	window = (NFOBJECT*)nfui_nfwindow_new(parent, POP_POS_X, POP_POS_Y, WND_SIZE_W, WND_SIZE_H);
	g_curwnd = (NFWINDOW*)window;


	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WND_SIZE_W, WND_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_disk_fixed_event_cb);
	nfui_nfobject_show(fixed);


	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AVAILABLE DISK LIST", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 654, 36);
	//nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 9, 4);


	/* table */
	tbl = (NFOBJECT*)nfui_nftable_new(4, 6, 2, 1, table_w, 40);	
	//nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, 20, 64);
	
	
	// row
	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}

	// col
	for(i=0; i<3; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, (i + 1), 0);
	}
	
	// model
	for(i=0; i<5; i++) {
		//if(disk_i->disk_unit[i].size == 0)
		if(sata_i.capacity[i] == 0)
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(sata_i.model_name[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			//obj = (NFOBJECT*)nfui_nflabel_new_text_box(disk_i->disk_unit[i].model, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i + 1);
	}
	
	// capacity
	for(i=0; i<5; i++) {
		//if(disk_i->disk_unit[i].size != 0) {
		if(sata_i.capacity[i] != 0) {
			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "%d GB", sata_i.capacity[i]);
			//ifn_convert_storage_size(buf, disk_i->disk_unit[i].size);
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(buf,  nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		}
		else 
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("-",  nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}
	
	// rebuild
	for(i=0; i<5; i++) {
		obj = nfui_nffixed_new();
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 3, i + 1);
	}

	for(i=0; i<5; i++) {
		fixed_temp = nfui_nftable_get_child((NFTABLE*)tbl, 3, i + 1);

		if(disk_mask & (1 << i))
			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		else
			obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
		nfui_check_get_size(NF_CHECKBUTTON(obj), &chk_w, &chk_h);
		nfui_nfobject_set_data(obj, "chk index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_chk_event_handler);
		nfui_nfobject_show(obj);
		if(disk_mask & (1 << i)) nfui_nfobject_disable(obj);
		//if(disk_i->disk_unit[i].size == 0) nfui_nfobject_disable(obj);
		if(sata_i.capacity[i] == 0) nfui_nfobject_disable(obj);
		nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (100-chk_w)/2, (40-chk_h)/2);
	}
		

	/* button */
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 141, 338);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 339, 338);


	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_DISK_RAID_AVAILABLE_LIST, window, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DISK_RAID_AVAILABLE_LIST, (NFOBJECT*)window);

	return g_dmsk;
}
