#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftab.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimglabel.h"

#include "vw_disk_main.h"
#include "vw_disk_raid.h"
#include "vw_disk_raid_external.h"
#include "vw_disk_raid_easy_conf.h"
#include "vw_disk_raid_advanced_conf.h"
#include "vw_disk_raid_warn.h"
#include "vw_disk_raid_conf_step2.h"


static DISK_RAIDINFO_T	*gRaid_i = NULL;		// raid information
static DISK_RAIDINFO_T	gRaid_oi;				// raid original information 

static NFWINDOW *g_curwnd = 0;
static NFTABLE *gTable;
static NFOBJECT *gModeObj;
static NFOBJECT *g_AppBtn;
static NFOBJECT *g_cancelBtn;
static NFOBJECT *g_ecBtn;
static NFOBJECT *g_acBtn;
static NFOBJECT *g_wbox = NULL;


static gint get_external_raidID(gint disk_index)
{
	gint i, j;

	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return -1;

	for(i=0; i<gRaid_i->raid_cnt; i++) {
		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {
			if(gRaid_i->rinfo[i].sata_index[j] == disk_index)		
				return i;
		}
	}
	return -1;
}

static gint get_disk_index_in_external_raid(gint raid_id, gint disk_index)
{
	gint i;

	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return -1;

	for(i=0; i<gRaid_i->rinfo[raid_id].member_count; i++) {
		if(gRaid_i->rinfo[raid_id].sata_index[i] == disk_index)		
			return i;
	}
	return -1;
}

static gint get_sata_port_in_raid(gint raid_id, gint disk_index)
{
	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return -1;
	if(raid_id < 0) return -1;

	if(disk_index >= gRaid_i->rinfo[raid_id].member_count)
		return -1;

	return gRaid_i->rinfo[raid_id].sata_index[disk_index];
}

static gboolean draw_raid_info(gboolean expose)
{
	NFOBJECT *obj;
	gint i, j, k;
	gchar *ppos;
	gint disk_port;

g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	g_printf("%s\nmode: %d\n", __FUNCTION__, gRaid_i->mode);
	for(j=0; j<5; j++) {
			g_printf("[%d]\nraid cnt: %d\nraid mode: %d\nmember cnt: %d\nmodel: %s %s %s %s %s\ncapacity: %d\nunused: %d %d %d %d %d\nstatus: %d\nrebuild: %d\nsata_index: %d %d %d %d %d\n\n", 
				j, 
				gRaid_i->raid_cnt, 
				gRaid_i->rinfo[j].raid_mode, 
				gRaid_i->rinfo[j].member_count, 
				gRaid_i->rinfo[j].model[0], 
				gRaid_i->rinfo[j].model[1], 
				gRaid_i->rinfo[j].model[2], 
				gRaid_i->rinfo[j].model[3], 
				gRaid_i->rinfo[j].model[4], 
				gRaid_i->rinfo[j].capacity, 
				gRaid_i->rinfo[j].unused[0], 
				gRaid_i->rinfo[j].unused[1], 
				gRaid_i->rinfo[j].unused[2], 
				gRaid_i->rinfo[j].unused[3], 
				gRaid_i->rinfo[j].unused[4], 
				gRaid_i->rinfo[j].status, 
				gRaid_i->rinfo[j].rebuild,
				gRaid_i->rinfo[j].sata_index[0],
				gRaid_i->rinfo[j].sata_index[1],
				gRaid_i->rinfo[j].sata_index[2],
				gRaid_i->rinfo[j].sata_index[3],
				gRaid_i->rinfo[j].sata_index[4]
				);
	}
g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);

	if(gRaid_i->mode == RAID_ALL_MODE) 	 nfui_nflabel_set_text((NFLABEL*)gModeObj, "ALL");
	else if(gRaid_i->mode == RAID_1_MODE) nfui_nflabel_set_text((NFLABEL*)gModeObj, "RAID 1");
	else if(gRaid_i->mode == RAID_5_MODE) nfui_nflabel_set_text((NFLABEL*)gModeObj, "RAID 5");
	else 								nfui_nflabel_set_text((NFLABEL*)gModeObj, "ALL");
	nfui_signal_emit(gModeObj, GDK_EXPOSE, FALSE);

	if(gRaid_i->mode == 0)	return FALSE;
	//if(gRaid_i->mode & RAID_CONF_MODE)	return FALSE;


	for(i=0; i<gRaid_i->raid_cnt; i++) {

		if(gRaid_i->rinfo[i].member_count == 0)
			continue;

		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {
			for(k=0; k<4; k++) {
				if(k == 1 || k == 3) // capacity & rebuild col
					continue;

				if(k == 0) {				// model
					if(strlen(gRaid_i->rinfo[i].model[j])) {

						disk_port = get_sata_port_in_raid(i, j);
						if(disk_port < 0)
							continue;

						obj = nfui_nftable_get_child(gTable, k+1, disk_port+1);

						ppos = strchr(gRaid_i->rinfo[i].model[j], ' '); 
						if(ppos) {
							sprintf(ppos, "%c", '\0');

							memcpy(&gRaid_oi.rinfo[i].model[j], gRaid_i->rinfo[i].model[j], sizeof(gchar)*41);
						}

						nfui_nflabel_set_text((NFLABEL*)obj, gRaid_i->rinfo[i].model[j]);
					}
					else
						nfui_nflabel_set_text((NFLABEL*)obj, "-");

				}
				else if(k == 2) { 			// status
					disk_port = get_sata_port_in_raid(i, j);
					if(disk_port < 0)
						continue;

					obj = nfui_nftable_get_child(gTable, k+1, disk_port+1);

					switch(gRaid_i->rinfo[i].status) {
						case 0: // broken
							nfui_nflabel_set_text((NFLABEL*)obj, "BROKEN");
							nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1011));
							break;

						case 1: // degraded
							nfui_nflabel_set_text((NFLABEL*)obj, "DEGRADED");
							nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1012));
							break;

						case 2: // rebuild
							nfui_nflabel_set_text((NFLABEL*)obj, "REBUILD");
							nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1009));
							break;

						case 3: // normal
							nfui_nflabel_set_text((NFLABEL*)obj, "NORMAL");
							nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1010));
							break;

						case 4: // ezBackup 
							nfui_nflabel_set_text((NFLABEL*)obj, "SPARE");
							nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1013));
							break;

						default:
							nfui_nflabel_set_text((NFLABEL*)obj, "-");
							nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
							break;
					}
				}
			}
		}
	}
		
	if(expose)
		nfui_signal_emit((NFOBJECT*)gTable, GDK_EXPOSE, TRUE);
	
	return TRUE;
}

static void redraw_raid_info()
{
	draw_raid_info(TRUE);
}

static void erase_raid_info()
{
	NFOBJECT *child;
	gint i, j;

	// erasing all conf
	nfui_nflabel_set_text((NFLABEL*)gModeObj, "ALL");
	nfui_signal_emit(gModeObj, GDK_EXPOSE, FALSE);

	for(i=0; i<4; i++) {
		for(j=0; j<5; j++) {
			child = nfui_nftable_get_child(gTable, i+1, j+1);

			if(i == 0) nfui_nflabel_set_text((NFLABEL*)child, "-");
			else if(i == 1) continue;
			else if(i == 2) nfui_nflabel_set_text((NFLABEL*)child, "-");
			else if(i == 3) continue;
		}
	}
	nfui_signal_emit((NFOBJECT*)gTable, GDK_EXPOSE, TRUE);
}

static void change_button_state(gint enable)
{
	if(enable) {
		nfui_nfobject_enable(g_AppBtn);
		nfui_nfobject_enable(g_cancelBtn);
	}else {
		nfui_nfobject_disable(g_AppBtn);
		nfui_nfobject_disable(g_cancelBtn);
	}

	nfui_signal_emit(g_AppBtn, GDK_EXPOSE, FALSE);
	nfui_signal_emit(g_cancelBtn, GDK_EXPOSE, FALSE);
}

static gboolean post_easy_conf_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		gint rmode = -1;
		gint ret = -1;

		rmode = VW_DiskRaid_EasyConf_Open(g_curwnd, EXTERNAL);

		if(rmode != -1) {
			//memset(&gRaid_oi, 0x00, sizeof(DISK_RAIDINFO_T));
			//memcpy(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T));
			//memset(gRaid_i, 0x00, sizeof(DISK_RAIDINFO_T));

			if(rmode == RAID1_CONF)
				ret = scm_get_disk_raid1_conf(EXTERNAL, gRaid_i);
			else if(rmode == RAID5_CONF)
				ret = scm_get_disk_raid5_conf(EXTERNAL, gRaid_i);
		}

		if(ret != -1) {
			redraw_raid_info();
			change_button_state(1);

			if(!nfui_nfobject_is_disabled(g_acBtn)) {
				nfui_nfobject_disable(g_acBtn);
				nfui_signal_emit(g_acBtn, GDK_EXPOSE, FALSE);
			}
		}
		//else {
		//	memcpy(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T));
		//}
	}
	return FALSE;
}

static gboolean post_advanced_conf_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		gint rmode = -1;
		gint ret = -1;

		//memcpy(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T));
		rmode = VW_DiskRaid_AdvancedConf_Open(g_curwnd, EXTERNAL);

		if(rmode != -1) {
			//memset(&gRaid_oi, 0x00, sizeof(DISK_RAIDINFO_T));
			//memcpy(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T));
			//memset(gRaid_i, 0x00, sizeof(DISK_RAIDINFO_T));

			ret = scm_get_disk_raid_advanced_conf(EXTERNAL, gRaid_i);
		}

		if(ret != -1) {
			erase_raid_info();
			redraw_raid_info();
			change_button_state(1);

			if(!nfui_nfobject_is_disabled(g_ecBtn)) {
				nfui_nfobject_disable(g_ecBtn);
				nfui_signal_emit(g_ecBtn, GDK_EXPOSE, FALSE);
			}
		}
		else {
			memcpy(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T));

			erase_raid_info();
			redraw_raid_info();
			change_button_state(1);

			if(nfui_nfobject_is_disabled(g_ecBtn)) {
				nfui_nfobject_enable(g_ecBtn);
				nfui_signal_emit(g_ecBtn, GDK_EXPOSE, FALSE);
			}
		}
	}
	return FALSE;
}

static void draw_capacity(NFOBJECT *obj, gint raid_id, gint disk_index)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint off_x, off_y;
	guint string_w;
	gint unused_idx;
	gint i;
	gchar buf[64] = {0,};


	if(!gRaid_i) return;
	if(gRaid_i->mode == 0)	return;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return;
	if(raid_id < 0) return;
	if(disk_index < 0) return;

	drawable = nfui_nfobject_get_window(obj);
	gc = gdk_gc_new(drawable);
	if(raid_id == 0) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(1001));
	else if(raid_id == 1) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(1002));
	else if(raid_id == 2) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(1003));
	else if(raid_id == 3) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(1004));
	else if(raid_id == 4) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(1005));

	if(raid_id >= 0) {
		nfui_nfobject_get_offset(obj, &off_x, &off_y);
		gdk_draw_rectangle(drawable, gc, TRUE,
				(gint)off_x,
				(gint)off_y,
				(gint)obj->width,
				(gint)obj->height);

		unused_idx = get_disk_index_in_external_raid(raid_id, disk_index);
		if(gRaid_i->rinfo[raid_id].unused[unused_idx] != 0) 
			sprintf(buf, "<# %d> USED (%dGB) / UNUSED (%dGB)", raid_id, gRaid_i->rinfo[raid_id].capacity, gRaid_i->rinfo[raid_id].unused[unused_idx]);
		else			 
			sprintf(buf, "<# %d> USED (%dGB)", raid_id, gRaid_i->rinfo[raid_id].capacity);
	}
	else
		strcpy(buf, "-");

	string_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_SMALL_SEMI), buf, NORMAL_SPACING);
	nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, buf, off_x + ((obj->width - string_w)/2), off_y, obj->width, obj->height, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(1000), NFALIGN_LEFT, 0);

	g_object_unref(gc);

}

static gboolean post_capacity_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		gint d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk order"));
		gint rid = get_external_raidID(d_index);

		if(rid != -1)
			draw_capacity(obj, rid, d_index);
	}
	return FALSE;
}

static void draw_rebuild(NFOBJECT *obj, gint raid_id)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint off_x, off_y;
	guint string_w;
	gchar buf[24] = {0,};

	if(!gRaid_i) return;
	if(gRaid_i->mode == 0)	return;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return;

	drawable = nfui_nfobject_get_window(obj);
	gc = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(1016));
	nfui_nfobject_get_offset(obj, &off_x, &off_y);

	if(raid_id < 0) {
		strcpy(buf, "-");
	}
	else {
		if(gRaid_i->rinfo[raid_id].status == 2) 
		{
			gdk_draw_rectangle(drawable, gc, TRUE,
					(gint)off_x,
					(gint)off_y,
					(gint)obj->width,
					(gint)obj->height);

			sprintf(buf, "%d%%", gRaid_i->rinfo[raid_id].rebuild);
		}
		else 
			strcpy(buf, "-");
	}

	string_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_SMALL_SEMI), buf, NORMAL_SPACING);
	nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, buf, (off_x + ((obj->width - string_w)/2)), off_y, obj->width, obj->height, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(1014), NFALIGN_LEFT, 0);

	g_object_unref(gc);
}

static gboolean post_rebuild_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		gint d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk order"));
		gint rid = get_external_raidID(d_index);

		draw_rebuild(obj, rid);
	}
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(memcmp(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T))) {
			memcpy(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T));

			erase_raid_info();
			redraw_raid_info();
			change_button_state(0);

			if(nfui_nfobject_is_disabled(g_acBtn)) {
				nfui_nfobject_enable(g_acBtn);
				nfui_signal_emit(g_acBtn, GDK_EXPOSE, FALSE);
			}

			if(nfui_nfobject_is_disabled(g_ecBtn)) {
				nfui_nfobject_enable(g_ecBtn);
				nfui_signal_emit(g_ecBtn, GDK_EXPOSE, FALSE);
			}
		}

		scm_clear_raid_conf();
	}

	return FALSE;
}

static gboolean check_status(gpointer data) 
{
	NFOBJECT *wbox = NULL;
	gint conf_status = 0;

	NFUTIL_THREADS_ENTER();
	if(conf_status == RAID_CONF_FINISH) 
	{
		wbox = (NFOBJECT*)data;
		if(wbox) nftool_remove_waitbox(wbox);

		nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_OK);
	}
	else if(conf_status == RAID_CONF_FAIL)
	{
		wbox = (NFOBJECT*)data;
		if(wbox) nftool_remove_waitbox(wbox);

		nftool_mbox_auto(g_curwnd, 3, "WARNING", "RAID configuration is fail...\nSystem will be reboot.");
	}
	else if(conf_status == RAID_CONFIGURING) 
	{
		return TRUE;
	}
	NFUTIL_THREADS_LEAVE();

	scm_reboot_system(RR_NA, 0);

	return FALSE;
}

static gboolean create_raid(gpointer data)
{
	NFOBJECT *wbox;
	gint conf_status = 0;

	conf_status = RAID_CONFIGURING; 
	g_timeout_add(1000, check_status, (gpointer)data);

	if(scm_conf_raid(EXTERNAL, gRaid_i) < 0) {
		g_warning("raid create fail");

		NFUTIL_THREADS_ENTER();
		wbox = (NFOBJECT*)data;
		nftool_remove_waitbox(wbox);

		nftool_mbox_auto(g_curwnd, 3, "WARNING", "RAID configuration is fail...\nSystem will be reboot.");

		NFUTIL_THREADS_LEAVE();
	}


	return FALSE;
}

static gboolean popup_wbox(gpointer data)
{
	static gint cnt = 0;

	if(!g_wbox) 
		g_wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	
	if(cnt++ > 3) {
		cnt = 0;
		gtk_main_quit();
		return FALSE;
	}

	return TRUE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *wbox = NULL;

		if(memcmp(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T))) {
			if(VW_DiskRaid_Warn_Open(g_curwnd)) {
				//wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
				//g_timeout_add(300, create_raid, (gpointer)wbox);
				g_timeout_add(100, popup_wbox, NULL);

				gtk_main();

				if(scm_conf_raid(EXTERNAL, gRaid_i) < 0) {
					if(g_wbox) {
						nftool_remove_waitbox(g_wbox);
						g_wbox = NULL;
					}

					nftool_mbox_auto(g_curwnd, 3, "WARNING", "RAID configuration is fail...\nSystem will be reboot.");

					scm_reboot_system(RR_NA, 0);
				}
				else {
					if(g_wbox) {
						nftool_remove_waitbox(g_wbox);
						g_wbox = NULL;
					}
					nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_OK);

					scm_reboot_system(RR_NA, 0);
				}
				
			}
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		VW_DiskRaid_tab_out_handler();
		VW_DiskSetup_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_DiskExternal_Raid_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *tbl;

	GdkPixbuf *pbIcon[6];

	nffont_type font_idx;

	gchar *strRow[] = {"DISK", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strCol[] = {"MODEL", "CAPACITY", "STATUS", "REBUILD"};
	guint table_w[] = {198, 216, 478, 214, 214};
	gint i;


	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);


	// raid mode
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RAID MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
	nfui_nfobject_set_size(obj, 198, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 25);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
	nfui_nfobject_set_size(obj, 216, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 227, 25);
	gModeObj = obj;


	// raid table
	tbl = (NFOBJECT*)nfui_nftable_new(5, 6, 2, 1, table_w, 40);	
	//nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 27, 100);
	gTable = (NFTABLE*)tbl;
	
	// col
	if(nftool_cur_language_is_japanese()) font_idx = NFFONT_SMALL_SEMI_1;
	else font_idx = NFFONT_MEDIUM_SEMI;

	for(i=0; i<4; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(font_idx), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, (i + 1), 0);
	}

	// row
	for(i=0; i<6; i++) {
		obj = nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(font_idx), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}

	// model
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(font_idx));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i + 1);
	}
	
	// capacity
	for(i=0; i<5; i++) {
		obj = nfui_nffixed_new();
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk order", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_capacity_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}
	
	
	// status
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(font_idx));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 3, i + 1);
	}
	
	// rebuild
	for(i=0; i<5; i++) {
		obj = nfui_nffixed_new();
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk order", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_rebuild_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 4, i + 1);
	}



	// button
	obj = nftool_normal_button_create_subtab_type1("EASY MODE", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_easy_conf_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 931, 360);
	g_ecBtn = obj;

	obj = nftool_normal_button_create_subtab_type1("ADVANCE MODE", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_advanced_conf_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 1139, 360);
	g_acBtn = obj;


	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	g_cancelBtn = obj;

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	g_AppBtn = obj;

	obj = nftool_normal_button_create_type1("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean display_external_raid(gboolean expose)
{
	gRaid_i = get_disk_raid_info(EXTERNAL);

	memcpy(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T));

	if(!draw_raid_info(expose))
		return FALSE;

	return TRUE;
}

gboolean check_disk_ext_raid_conf_changed()
{
	if(memcmp(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T)))
		return TRUE;
	
	return FALSE;
}

gboolean apply_disk_ext_raid_conf()
{
	if(scm_conf_raid(EXTERNAL, gRaid_i) < 0) {
		g_warning("%s : raid create fail", __FUNCTION__);

		nftool_mbox(g_curwnd, "WARNING", "RAID configuration is fail...\nSystem will be reboot.", NFTOOL_MB_OK);

		scm_reboot_system(RR_NA, 0);

		return FALSE;
	}

	return TRUE;
}
