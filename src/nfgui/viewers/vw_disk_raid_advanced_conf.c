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

#include "modules/ssm.h"

#include "vw_disk_main.h"
#include "vw_disk_raid.h"
#include "vw_disk_raid_advanced_conf.h"
#include "vw_disk_raid_available_list.h"

#define WND_SIZE_W				(1518)
#define WND_SIZE_H				(482)

#define POP_POS_X				((DISPLAY_ACTIVE_WIDTH - WND_SIZE_W)/4*2)
#define POP_POS_Y				((DISPLAY_ACTIVE_HEIGHT - WND_SIZE_H)/2)

enum {
	AD_CONF_RAID1,
	AD_CONF_RAID5,
	AD_CONF_RAID_CNT
};

enum {
	AD_CONF_ACTION_INIT,
	AD_CONF_ACTION_ADD,
	AD_CONF_ACTION_DELETE
};

typedef struct _ACTION_INFO {
	gint raid_id;
	guint disk_mask;
	gint action;
}ACTION_INFO;

typedef struct _AD_RCONF_ACTION_INFO {
	gint conf_cnt;
	ACTION_INFO ac_info[2];
}AD_RCONF_ACTION_INFO;


static DISK_RAIDINFO_T	*gRaid_i = NULL;
static DISK_RAIDINFO_T	gRaid_conf[2];
static AD_RCONF_ACTION_INFO action_i[AD_CONF_RAID_CNT];

static NFWINDOW *g_curwnd = 0;
static NFTAB	*gTab;
static NFTABLE *gR1_tbl;				// raid 1 tab table
static NFTABLE *gR5_tbl;				// raid 5 tab table
static NFBUTTON *gR1_eBtn[2];			// raid 1 edit button
static NFBUTTON *gR5_eBtn[3];			// raid 5 edit button

static GdkPixbuf *gBtnImg[4][NFOBJECT_STATE_COUNT];

static STRG_TYPE_E gStrg_type;
static gint g_rConfMode = -1;




static void init_ui_info()
{
	gint rmode;
	gint i, j;

	memset(action_i, 0x00, sizeof(AD_RCONF_ACTION_INFO) * AD_CONF_RAID_CNT);

	if(gRaid_i->mode != 0) {

		for(i=0; i<gRaid_i->raid_cnt; i++) {
			if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1 
					|| gRaid_i->rinfo[i].raid_mode == RAID1_CONF_ADD) 
				rmode = AD_CONF_RAID1;
			else if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5
					|| gRaid_i->rinfo[i].raid_mode == RAID5_CONF_ADD) 
				rmode = AD_CONF_RAID5;

			action_i[rmode].conf_cnt += 1;
			action_i[rmode].ac_info[i].raid_id = gRaid_i->rinfo[i].raid_id;
			action_i[rmode].ac_info[i].action = AD_CONF_ACTION_DELETE;

			for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {
				action_i[rmode].ac_info[i].disk_mask |= (1 << gRaid_i->rinfo[i].sata_index[j]);
			}
		}
	}
	else {
		//action_i[AD_CONF_RAID1].conf_cnt += 1;
		action_i[AD_CONF_RAID1].ac_info[0].disk_mask = 0;
		action_i[AD_CONF_RAID1].ac_info[0].raid_id = 0;
		action_i[AD_CONF_RAID1].ac_info[0].action = AD_CONF_ACTION_INIT;

		//action_i[AD_CONF_RAID5].conf_cnt += 1;
		action_i[AD_CONF_RAID5].ac_info[0].disk_mask = 0;
		action_i[AD_CONF_RAID5].ac_info[0].raid_id = 0;
		action_i[AD_CONF_RAID5].ac_info[0].action = AD_CONF_ACTION_INIT;
		//g_message(">>>>>>>>>>>>> %s [%d] >>>>>>>>>>>", __FUNCTION__, __LINE__);
	}

}

static void set_action_info(gint conf_mode, gint index, guint disk_mask, gint action)
{
	gint idx;

	if(action == AD_CONF_ACTION_ADD) {
		action_i[conf_mode].conf_cnt += 1;
		idx = gRaid_i->raid_cnt;
		action_i[conf_mode].ac_info[index].raid_id = gRaid_i->rinfo[idx-1].raid_id;
		action_i[conf_mode].ac_info[index].disk_mask |= disk_mask;
		action_i[conf_mode].ac_info[index].action = action;
g_message("%s [%d] :::::::::::::[%d] disk mask: %d idx %d raid id %d", __FUNCTION__, __LINE__, index, disk_mask, idx, action_i[conf_mode].ac_info[index].raid_id);
	}
	else if(action == AD_CONF_ACTION_DELETE) {
		action_i[conf_mode].conf_cnt -= 1;
		action_i[conf_mode].ac_info[index].raid_id = -1;
		action_i[conf_mode].ac_info[index].disk_mask &= ~disk_mask;
		action_i[conf_mode].ac_info[index].action = action;

		if(index == 0 && action_i[conf_mode].conf_cnt == 1) {
			memcpy(&action_i[conf_mode].ac_info[index], &action_i[conf_mode].ac_info[1], sizeof(ACTION_INFO));

			action_i[conf_mode].ac_info[1].raid_id = -1;
			action_i[conf_mode].ac_info[1].disk_mask = 0;
			action_i[conf_mode].ac_info[1].action = action;
		}
	}
}


static gint get_external_raidID(gint raid_type, gint disk_index)
{
	gint i, j;

	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;

	for(i=0; i<gRaid_i->raid_cnt; i++) {
		
		if(raid_type == AD_CONF_RAID1) 
		{
			if(gRaid_i->rinfo[i].raid_mode != RAID1_CONF_ADD 
					&& gRaid_i->rinfo[i].raid_mode != RAID_LEVEL_1 )
				continue;
		}
		else if(raid_type == AD_CONF_RAID5) 
		{
			if(gRaid_i->rinfo[i].raid_mode != RAID5_CONF_ADD 
					&& gRaid_i->rinfo[i].raid_mode != RAID_LEVEL_5 )
				continue;
		}

		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {
			if(gRaid_i->rinfo[i].sata_index[j] == disk_index)		
				return i;
		}
	}
	return -1;
}

static gint get_disk_index_in_raid(gint raid_id, gint disk_index)
{
	gint i;

	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;

	for(i=0; i<gRaid_i->rinfo[raid_id].member_count; i++) {
		if(gRaid_i->rinfo[raid_id].sata_index[i] == disk_index)		
			return i;
	}

	return -1;
}

static gint get_disk_index_in_raid_conf(gint raid_id, gint disk_index)
{
	gint i, j;

	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;

	for(j=0; j<gRaid_i->rinfo[raid_id].member_count; j++) {
		if(gRaid_i->rinfo[raid_id].sata_index[j] == disk_index)		
			return j;
	}
	return -1;
}

static void put_edit_button(gint raid_id, guchar raid_mode, gint disk_count, gchar *text)
{
	NFOBJECT *tab_page;
	NFOBJECT *obj;
	gint cur_page;

	cur_page =  nfui_nftab_get_cur_page(gTab);
	tab_page = nfui_nftab_get_nth_page(gTab, cur_page);

	if(raid_mode == RAID_1_MODE) {
		if(raid_id == 0) obj = (NFOBJECT*)gR1_eBtn[0];
		else if(raid_id == 1) obj = (NFOBJECT*)gR1_eBtn[1];
		else return;
	}
	else if(raid_mode == RAID_5_MODE) {
		if(disk_count == 3) obj = (NFOBJECT*)gR5_eBtn[0];
		else if(disk_count == 4) obj = (NFOBJECT*)gR5_eBtn[1];
		else if(disk_count == 5) obj = (NFOBJECT*)gR5_eBtn[2];
		else return;
	}

	nfui_nfbutton_set_text((NFBUTTON*)obj, text);
	nfui_nfobject_show(obj);
	//nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)obj, 1365, (27 + 41));
	nfui_signal_emit((NFOBJECT*)obj, GDK_EXPOSE, FALSE);
	//g_message("%s [%d]......................", __FUNCTION__, __LINE__);
}

static void clear_raid_model_name(gint conf_mode, gboolean expose)
{
	NFOBJECT *obj = NULL;
	gint i;

	if(conf_mode == AD_CONF_RAID1) {
		for(i=0; i<5; i++) {
			obj = nfui_nftable_get_child(gR1_tbl, 1, i + 1);
			nfui_nflabel_set_text((NFLABEL*)obj, "-");
		}
		if(expose) nfui_signal_emit((NFOBJECT*)gR1_tbl, GDK_EXPOSE, TRUE);
	}
	else {

		for(i=0; i<5; i++) {
			obj = nfui_nftable_get_child(gR5_tbl, 1, i + 1);
			nfui_nflabel_set_text((NFLABEL*)obj, "-");
		}
		if(expose) nfui_signal_emit((NFOBJECT*)gR5_tbl, GDK_EXPOSE, TRUE);
	}
}

static void display_raid_model_name()
{
	NFOBJECT *obj = NULL;
	gint i, j;
	gchar *ppos;
	gint r1_row = 0, r5_row = 0;

	g_message(">>>>>>>>>>>>> %s [%d] >>>>>>>>>>>", __FUNCTION__, __LINE__);
	for(i=0; i<gRaid_i->raid_cnt; i++) {
		if(gRaid_i->rinfo[i].member_count == 0) {
			continue;
		}

		if(gRaid_i->rinfo[i].raid_mode == RAID1_CONF_DELETE 
				|| gRaid_i->rinfo[i].raid_mode == RAID5_CONF_DELETE)
			continue;

	//g_message("%s [%d].........%d.............", __FUNCTION__, __LINE__, gRaid_i->rinfo[i].member_count);
		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {

	//g_message("%s [%d].........%s.............", __FUNCTION__, __LINE__, gRaid_i->rinfo[i].model[j]);
			if(strlen(gRaid_i->rinfo[i].model[j])) {

				if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1 || gRaid_i->rinfo[i].raid_mode == RAID1_CONF_ADD)  	// raid 1 conf 
					obj = nfui_nftable_get_child(gR1_tbl, 1, ++r1_row);
				else if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5 || gRaid_i->rinfo[i].raid_mode == RAID5_CONF_ADD)   // raid 5 conf
					obj = nfui_nftable_get_child(gR5_tbl, 1, ++r5_row);

				if(!obj) continue;

				ppos = strchr(gRaid_i->rinfo[i].model[j], ' '); 
				if(ppos) sprintf(ppos, "%c", '\0');

				nfui_nflabel_set_text((NFLABEL*)obj, gRaid_i->rinfo[i].model[j]);
			}
		}
	//g_message("%s [%d].........%s.............", __FUNCTION__, __LINE__, gRaid_i->rinfo[i].model[1]);
	}

	if(r1_row > 0) nfui_signal_emit((NFOBJECT*)gR1_tbl, GDK_EXPOSE, TRUE);
	if(r5_row > 0) nfui_signal_emit((NFOBJECT*)gR5_tbl, GDK_EXPOSE, TRUE);
}

static void clear_raid_status(gint conf_mode, gboolean expose)
{
	NFOBJECT *child;
	gint i;

	if(conf_mode == AD_CONF_RAID1) {
		for(i=0; i<5; i++) {
			child = nfui_nftable_get_child(gR1_tbl, 3, i + 1);
			nfui_nflabel_set_text((NFLABEL*)child, "-");
			nfui_nfobject_modify_bg((NFOBJECT*)child, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));		
		}

		if(expose) nfui_signal_emit((NFOBJECT*)gR1_tbl, GDK_EXPOSE, TRUE);
	}
	else {
		for(i=0; i<5; i++) {
			child = nfui_nftable_get_child(gR5_tbl, 3, i + 1);
			nfui_nflabel_set_text((NFLABEL*)child, "-");
			nfui_nfobject_modify_bg((NFOBJECT*)child, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));		
		}

		if(expose) nfui_signal_emit((NFOBJECT*)gR5_tbl, GDK_EXPOSE, TRUE);
	}

}

static void display_raid_status()
{
	NFOBJECT *obj = NULL;
	gint i, j;
	gchar *ppos;
	gint r1_row = 0, r5_row = 0;

	for(i=0; i<gRaid_i->raid_cnt; i++) {

		if(gRaid_i->rinfo[i].member_count == 0)
			continue;

		if(gRaid_i->rinfo[i].raid_mode == RAID1_CONF_DELETE 
				|| gRaid_i->rinfo[i].raid_mode == RAID5_CONF_DELETE)
			continue;

		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {

			if(gRaid_i->rinfo[i].raid_mode == RAID1_CONF_ADD || gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1)  	// raid 1
				obj = nfui_nftable_get_child(gR1_tbl, 3, ++r1_row);
			else if(gRaid_i->rinfo[i].raid_mode == RAID5_CONF_ADD || gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5)   // raid 5
				obj = nfui_nftable_get_child(gR5_tbl, 3, ++r5_row);

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

	if(r1_row > 0) nfui_signal_emit((NFOBJECT*)gR1_tbl, GDK_EXPOSE, TRUE);
	if(r5_row > 0) nfui_signal_emit((NFOBJECT*)gR5_tbl, GDK_EXPOSE, TRUE);
	//g_message("%s [%d]......................", __FUNCTION__, __LINE__);
}

static void clear_capacity_area(gint conf_mode, gboolean expose)
{
	if(conf_mode == AD_CONF_RAID1) {
		if(expose) nfui_signal_emit((NFOBJECT*)gR1_tbl, GDK_EXPOSE, TRUE);
	}
	else {
		if(expose) nfui_signal_emit((NFOBJECT*)gR5_tbl, GDK_EXPOSE, TRUE);
	}

}

static void draw_capacity(NFOBJECT *obj, gint raid_id, gint disk_port)
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
	if(raid_id < 0) return;
	if(disk_port < 0) return;

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

		unused_idx = get_disk_index_in_raid(raid_id, disk_port);
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

static void clear_rebuild_area(gint conf_mode, gboolean expose)
{
	if(conf_mode == AD_CONF_RAID1) {
		if(expose) nfui_signal_emit((NFOBJECT*)gR1_tbl, GDK_EXPOSE, TRUE);
	}
	else {
		if(expose) nfui_signal_emit((NFOBJECT*)gR5_tbl, GDK_EXPOSE, TRUE);
	}

}

static void draw_rebuild(NFOBJECT *obj, gint raid_id, gint disk_port)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint off_x, off_y;
	guint string_w;
	gint idx;
	gchar buf[24] = {0,};

	if(!gRaid_i) return;
	if(gRaid_i->mode == 0)	return;
	if(gRaid_i->mode & RAID_CONF_MODE)	return;
	if(disk_port < 0) return;

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

			idx = get_disk_index_in_raid(raid_id, disk_port);
			sprintf(buf, "%d%%", gRaid_i->rinfo[idx].rebuild);
		}
		else 
			strcpy(buf, "-");
	}

	string_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_SMALL_SEMI), buf, NORMAL_SPACING);
	nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, buf, (off_x + ((obj->width - string_w)/2)), off_y, obj->width, obj->height, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(1014), NFALIGN_LEFT, 0);

	g_object_unref(gc);
}

static void display_edit_button()
{
	gint i;
	gint r1_idx = 0;

	if(gRaid_i->mode != 0) {
		for(i=0; i<gRaid_i->raid_cnt; i++) {
			if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1 || gRaid_i->rinfo[i].raid_mode == RAID1_CONF_ADD) {
				nfui_nfbutton_set_text(gR1_eBtn[r1_idx], "DELETE");
				nfui_nfobject_enable((NFOBJECT*)gR1_eBtn[r1_idx]);
				nfui_signal_emit((NFOBJECT*)gR1_eBtn[r1_idx], GDK_EXPOSE, FALSE);

				r1_idx++;
			}
			else if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5 || gRaid_i->rinfo[i].raid_mode == RAID5_CONF_ADD) {
				nfui_nfbutton_set_text(gR5_eBtn[0], "DELETE");
				nfui_nfobject_enable((NFOBJECT*)gR5_eBtn[0]);
				nfui_signal_emit((NFOBJECT*)gR5_eBtn[0], GDK_EXPOSE, FALSE);
			}
		}

		// set left edit button state
		if(gRaid_i->raid_cnt == 1) { 
			if(r1_idx == 0) {
				nfui_nfbutton_set_text(gR1_eBtn[r1_idx], "ADD");
				nfui_nfobject_enable((NFOBJECT*)gR1_eBtn[r1_idx]);
				nfui_signal_emit((NFOBJECT*)gR1_eBtn[r1_idx], GDK_EXPOSE, FALSE);
			}
			else if(r1_idx == 1) {
				nfui_nfbutton_set_text(gR1_eBtn[r1_idx], "ADD");
				nfui_nfobject_enable((NFOBJECT*)gR1_eBtn[r1_idx]);
				nfui_signal_emit((NFOBJECT*)gR1_eBtn[r1_idx], GDK_EXPOSE, FALSE);

				nfui_nfbutton_set_text(gR5_eBtn[0], "ADD");
				nfui_nfobject_enable((NFOBJECT*)gR5_eBtn[0]);
				nfui_signal_emit((NFOBJECT*)gR5_eBtn[0], GDK_EXPOSE, FALSE);
			}
		}
		else if(gRaid_i->raid_cnt == 2) {
			if(r1_idx == 1) {
				nfui_nfbutton_set_text(gR1_eBtn[r1_idx], "ADD");
				nfui_nfobject_enable((NFOBJECT*)gR1_eBtn[r1_idx]);
				nfui_signal_emit((NFOBJECT*)gR1_eBtn[r1_idx], GDK_EXPOSE, FALSE);
			}
			else {
				nfui_nfbutton_set_text(gR5_eBtn[0], "ADD");
				nfui_nfobject_enable((NFOBJECT*)gR5_eBtn[0]);
				nfui_signal_emit((NFOBJECT*)gR5_eBtn[0], GDK_EXPOSE, FALSE);
			}
		}
	}
	else
	{
		nfui_nfbutton_set_text(gR1_eBtn[0], "ADD");
		nfui_nfobject_enable((NFOBJECT*)gR1_eBtn[0]);
		nfui_signal_emit((NFOBJECT*)gR1_eBtn[0], GDK_EXPOSE, FALSE);

		nfui_nfbutton_set_text(gR5_eBtn[0], "ADD");
		nfui_nfobject_enable((NFOBJECT*)gR5_eBtn[0]);
		nfui_signal_emit((NFOBJECT*)gR5_eBtn[0], GDK_EXPOSE, FALSE);
	}
	//g_message("%s [%d]......................", __FUNCTION__, __LINE__);
}

static gboolean load_raid_info(STRG_TYPE_E strg_type)
{
	gRaid_i = get_disk_raid_info(strg_type);

	if(!gRaid_i) return FALSE;

	if(gRaid_i->mode == 0 || gRaid_i->mode & RAID_CONF_MODE) {
		 memset(&gRaid_conf[strg_type], 0x00, sizeof(DISK_RAIDINFO_T));
		 scm_get_disk_advanced_raid_conf(strg_type, &gRaid_conf[strg_type]);

		 gRaid_i = &gRaid_conf[strg_type];
	}

	return TRUE;
}

static void display_raid_page()
{
	init_ui_info();

	if((gRaid_i->mode != 0)) {
		display_raid_model_name();
		display_raid_status();
	}

	display_edit_button();

//	g_message("%s [%d]......................", __FUNCTION__, __LINE__);
}

static gboolean display_int_advanced_raid_info(gpointer data)
{
	NFOBJECT *wbox = (NFOBJECT*)data;

	NFUTIL_THREADS_ENTER();
	
	load_raid_info(gStrg_type); 
	display_raid_page();

	nftool_remove_waitbox(wbox);

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean display_ext_advanced_raid_info(gpointer data)
{
	NFOBJECT *wbox = (NFOBJECT*)data;

	NFUTIL_THREADS_ENTER();
	
	load_raid_info(gStrg_type);
	display_raid_page();

	nftool_remove_waitbox(wbox);

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean post_r1_rebuild_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		NFOBJECT *child;
		static gint seq = 1;
		gint d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk order"));
		gint rid = get_external_raidID(AD_CONF_RAID1, d_index);

		g_message("%s [%d] :::::::::::::rid: %d %d", __FUNCTION__, __LINE__, rid, 0);
		if(d_index == 0 && seq != 1) seq = 1;
		if(rid != -1) {
			child = nfui_nftable_get_child(gR1_tbl, 4, seq++);
			draw_rebuild(child, rid, d_index);
		}
	}
	return FALSE;
}

static gboolean post_r5_rebuild_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		NFOBJECT *child;
		static gint seq = 1;
		gint d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk order"));
		gint rid = get_external_raidID(AD_CONF_RAID5, d_index);

		g_message("%s [%d] :::::::::::::rid: %d %d", __FUNCTION__, __LINE__, rid, 0);
		if(d_index == 0 && seq != 1) seq = 1;
		if(rid != -1) {
			child = nfui_nftable_get_child(gR5_tbl, 4, seq++);
			draw_rebuild(child, rid, d_index);
		}
	}
	return FALSE;
}

static gboolean post_r1_edit_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		guint disk_mask = 0;
		guint chk_mask = 0;
		gchar *btnStr;
		gint raid_id;
		gint index;
		gint next;
		gint ret = -1;
		gint i;


		index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "button index"));

		btnStr = nfui_nfbutton_get_text((NFBUTTON*)obj);
		if(!strcmp(btnStr, "ADD")) {

			disk_mask |= action_i[AD_CONF_RAID1].ac_info[0].disk_mask;
			disk_mask |= action_i[AD_CONF_RAID1].ac_info[1].disk_mask;
			disk_mask |= action_i[AD_CONF_RAID5].ac_info[0].disk_mask;
			disk_mask |= action_i[AD_CONF_RAID5].ac_info[1].disk_mask;

			chk_mask = VW_DiskRaid_AvailableList_Open(g_curwnd, gStrg_type, RAID_1_MODE, disk_mask);

			if(chk_mask && chk_mask != disk_mask) {

				if(scm_add_raid_disk(gStrg_type, RAID_1_MODE, chk_mask, gRaid_i) != -1) {

					set_action_info(AD_CONF_RAID1, index, chk_mask, AD_CONF_ACTION_ADD);

					nfui_nfbutton_set_text((NFBUTTON*)obj, "DELETE");
					nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

					if(action_i[AD_CONF_RAID1].conf_cnt < 2) {
						if(index == 0)	next = 1;
						else if(index == 1) next = 0;
						nfui_nfbutton_set_text(gR1_eBtn[next], "ADD");
						nfui_nfobject_enable((NFOBJECT*)gR1_eBtn[next]);
						nfui_signal_emit(gR1_eBtn[next], GDK_EXPOSE, TRUE);
					}

					display_raid_model_name();
				}
				else
					g_warning("%s : raid add or delete fail", __FUNCTION__);
			}
		}
		else if(!strcmp(btnStr, "DELETE")) {

			raid_id = action_i[AD_CONF_RAID1].ac_info[index].raid_id;
			g_message("::::::::::::::::: raid id: %d", raid_id); 
			g_message("::::::::::::::::: raid id: %d", raid_id); 
			g_message("::::::::::::::::: raid id: %d", raid_id); 
			g_message("::::::::::::::::: raid id: %d", raid_id); 

			/*if(scm_delete_raid(gStrg_type, RAID1_CONF_ADD, raid_id, gRaid_i) != -1) {

				chk_mask = action_i[AD_CONF_RAID1].ac_info[index].disk_mask;
				g_message("::::::::::::::::: conf cnt: %d", action_i[AD_CONF_RAID1].conf_cnt); 
				set_action_info(AD_CONF_RAID1, index, chk_mask, AD_CONF_ACTION_DELETE);
				g_message("::::::::::::::::: conf cnt: %d", action_i[AD_CONF_RAID1].conf_cnt); 


				if(action_i[AD_CONF_RAID1].conf_cnt == 0) {
					nfui_nfbutton_set_text((NFBUTTON*)obj, "ADD");
					nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

					nfui_nfobject_disable((NFOBJECT*)gR1_eBtn[1]);
					nfui_signal_emit((NFOBJECT*)gR1_eBtn[1], GDK_EXPOSE, TRUE);
				}

				if(action_i[AD_CONF_RAID1].conf_cnt == 1) {
					if(index == 0) {
						//nfui_nfbutton_set_text((NFBUTTON*)obj, "");
						nfui_nfobject_disable((NFOBJECT*)gR1_eBtn[1]);
						nfui_signal_emit((NFOBJECT*)gR1_eBtn[1], GDK_EXPOSE, TRUE);
					}
					else {
						nfui_nfbutton_set_text((NFBUTTON*)gR1_eBtn[1], "ADD");
						nfui_signal_emit((NFOBJECT*)gR1_eBtn[1], GDK_EXPOSE, TRUE);
					}
				}

				clear_raid_model_name(AD_CONF_RAID1, FALSE);
				clear_capacity_area(AD_CONF_RAID1, FALSE);
				clear_raid_status(AD_CONF_RAID1, TRUE);

				display_raid_model_name();
				display_raid_status();
			}
			else
				g_warning("%s : raid add or delete fail", __FUNCTION__);*/
		}
	}

	return FALSE;
}

static gboolean post_r5_edit_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		guint disk_mask = 0;
		guint chk_mask = 0;
		gchar *btnStr;
		gint raid_id;
		gint index;
		gint next;
		gint ret = -1;
		

		g_message("%s [%d] :::::::::::::disk mask: %d ", __FUNCTION__, __LINE__, disk_mask);

		index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "button index"));

		btnStr = nfui_nfbutton_get_text((NFBUTTON*)obj);
		if(!strcmp(btnStr, "ADD")) {

			disk_mask |= action_i[AD_CONF_RAID1].ac_info[0].disk_mask;
			disk_mask |= action_i[AD_CONF_RAID1].ac_info[1].disk_mask;
			disk_mask |= action_i[AD_CONF_RAID5].ac_info[0].disk_mask;
			disk_mask |= action_i[AD_CONF_RAID5].ac_info[1].disk_mask;

			chk_mask = VW_DiskRaid_AvailableList_Open(g_curwnd, gStrg_type, RAID_5_MODE, disk_mask);

			if(chk_mask && chk_mask != disk_mask) {
				if(scm_add_raid_disk(gStrg_type, RAID_5_MODE ,chk_mask, gRaid_i) != -1) {

					set_action_info(AD_CONF_RAID5, index, chk_mask, AD_CONF_ACTION_ADD);

					nfui_nfbutton_set_text((NFBUTTON*)obj, "DELETE");
					nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

					display_raid_model_name();
				}
				else
					g_warning("%s : raid add or delete fail", __FUNCTION__);

			}
		}
		else if(!strcmp(btnStr, "DELETE")) {


			raid_id = action_i[AD_CONF_RAID5].ac_info[index].raid_id;
g_message(">>>>>>>>>>>>> %s [%d] >>>>> index %d raid id %d>>>>>>", __FUNCTION__, __LINE__, index, raid_id);

			/*if(scm_delete_raid(gStrg_type, RAID5_CONF_ADD, raid_id, gRaid_i) != -1) {

				chk_mask = action_i[AD_CONF_RAID5].ac_info[index].disk_mask;
				set_action_info(AD_CONF_RAID5, index, chk_mask, AD_CONF_ACTION_DELETE);

				nfui_nfbutton_set_text((NFBUTTON*)obj, "ADD");
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

				clear_raid_model_name(AD_CONF_RAID5, FALSE);
				clear_raid_status(AD_CONF_RAID1, FALSE);
				clear_capacity_area(AD_CONF_RAID5, TRUE);
				display_raid_model_name();
			}
			else
				g_warning("%s : raid add or delete fail", __FUNCTION__);*/
		}

	}

	return FALSE;
}

static gboolean post_r1_capacity_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE)
	{
		NFOBJECT *child;
		static gint seq = 1;
		gint d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk index"));
		gint rid = get_external_raidID(AD_CONF_RAID1, d_index);

		g_message("%s [%d] :::::::::::::rid: %d %d", __FUNCTION__, __LINE__, rid, 0);
		if(d_index == 0 && seq != 1) seq = 1;
		if(rid != -1) {
			child = nfui_nftable_get_child(gR1_tbl, 2, seq++);
			draw_capacity(child, rid, d_index);
		}

	}
	return FALSE;
}

static gboolean post_r5_capacity_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE)
	{
		NFOBJECT *child;
		static gint seq = 1;
		gint d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk index"));
		gint rid = get_external_raidID(AD_CONF_RAID5, d_index);

		g_message("%s [%d] :::::::::::::rid: %d %d", __FUNCTION__, __LINE__, rid, 0);
		if(d_index == 0 && seq != 1) seq = 1;
		if(rid != -1) {
			child = nfui_nftable_get_child(gR5_tbl, 2, seq++);
			draw_capacity(child, rid, d_index);
		}
	}
	return FALSE;
}

static void	init_raid1_tabpage(NFOBJECT *page)
{
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GdkPixbuf *pbIcon[6];

	gchar *strRow[] = {"DISK", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strCol[] = {"MODEL", "CAPACITY", "STATUS", "REBUILD", "ACTION"};

	guint table_w[] = {198, 216, 478, 214, 214};
	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(210), COLOR_IDX(211), COLOR_IDX(212), COLOR_IDX(214)};
	gint i;



	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 

	// raid table
	tbl = (NFOBJECT*)nfui_nftable_new(5, 6, 2, 1, table_w, 40);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)page, tbl, 33, 27);
	gR1_tbl = (NFTABLE*)tbl;


	// col
	for(i=0; i<4; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, (i + 1), 0);
	}

	// action
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 120, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 1365, 27);


	// row
	for(i=0; i<6; i++) {
		obj = nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}

	// model
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
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
		nfui_nfobject_set_data(obj, "disk index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_r1_capacity_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}

	
	// status
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		//nfui_regi_post_event_callback(obj, pre_status_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 3, i + 1);
	}
	
	// rebuild
	for(i=0; i<5; i++) {
		obj = nfui_nffixed_new();
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_r1_rebuild_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 4, i + 1);
	}

	// add / delete button
	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(gBtnImg[0], "ADD");
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), font_color);
		nfui_nfobject_set_size(obj, 120, 81);
		nfui_nfobject_set_data(obj, "button index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_r1_edit_button_event_cb);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)page, obj, 1365, (27 + 41 + (i * 81)));
		nfui_nfobject_disable(obj);

		gR1_eBtn[i] = (NFBUTTON*)obj;
	}

	/*
	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(gBtnImg[0], "");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), font_color);
	nfui_regi_post_event_callback(obj, post_edit_button_event_cb);
	nfui_nfobject_set_size(obj, 120, 81);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 1365, (27 + 41));
	gR1_eBtn[1] = (NFBUTTON*)obj;

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(gBtnImg[1], "");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), font_color);
	nfui_regi_post_event_callback(obj, post_edit_button_event_cb);
	nfui_nfobject_set_size(obj, 120, 122);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 1365, (27 + 41));
	gR1_eBtn[2] = (NFBUTTON*)obj;
	*/
}

static void	init_raid5_tabpage(NFOBJECT *page)
{
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GdkPixbuf *pbIcon[6];

	gchar *strRow[] = {"DISK", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strCol[] = {"MODEL", "CAPACITY", "STATUS", "REBUILD", "ACTION"};

	guint table_w[] = {198, 216, 478, 214, 214};
	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(210), COLOR_IDX(211), COLOR_IDX(212), COLOR_IDX(214)};
	gint i;



	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 

	// raid table
	tbl = (NFOBJECT*)nfui_nftable_new(5, 6, 2, 1, table_w, 40);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)page, tbl, 33, 27);
	gR5_tbl = (NFTABLE*)tbl;


	// col
	for(i=0; i<4; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, (i + 1), 0);
	}
	
	// action
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 120, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 1365, 27);

	// row
	for(i=0; i<6; i++) {
		obj = nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}

	// model
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
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
		nfui_nfobject_set_data(obj, "disk index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_r5_capacity_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}

	
	// status
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		//nfui_regi_post_event_callback(obj, pre_status_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 3, i + 1);
	}
	
	// rebuild
	for(i=0; i<5; i++) {
		obj = nfui_nffixed_new();
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_r5_rebuild_event_handler);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 4, i + 1);
	}

	// add / delete button
	/*
	for(i=0; i<3; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(gBtnImg[i+1], "");
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), font_color);
		if(i == 0)nfui_nfobject_set_size(obj, 120, 122);
		else if(i == 1) nfui_nfobject_set_size(obj, 120, 163);
		else nfui_nfobject_set_size(obj, 120, 204);
		nfui_nfobject_set_data(obj, "button index", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(obj, post_r5_edit_button_event_cb);
		//nfui_regi_post_event_callback(obj, post_menu_button_event_cb);
		//nfui_nfobject_show(obj);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)page, obj, 1365, (27 + 41));

		gR5_eBtn[i] = (NFBUTTON*)obj;
	}
	*/
	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(gBtnImg[3], "ADD");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), font_color);
	nfui_nfobject_set_size(obj, 120, 204);
	nfui_nfobject_set_data(obj, "button index", GINT_TO_POINTER(0));
	nfui_regi_post_event_callback(obj, post_r5_edit_button_event_cb);
	//nfui_regi_post_event_callback(obj, post_menu_button_event_cb);
	//nfui_nfobject_show(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 1365, (27 + 41));
	nfui_nfobject_disable(obj);

	gR5_eBtn[0] = (NFBUTTON*)obj;

}

static gboolean pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable;
		GdkColor line_color = UX_COLOR(392);
		GdkGC *line_gc;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		line_gc = nfui_nfobject_get_gc((NFOBJECT*)obj);
		gdk_gc_set_rgb_fg_color(line_gc, &line_color);

		gdk_gc_set_line_attributes(line_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);

		gdk_draw_rectangle(drawable,
				line_gc,
				FALSE,
				obj->x, obj->y,
				obj->width, obj->height);

		nfui_nfobject_gc_unref(line_gc);
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
	NFOBJECT *top = NULL;
	mb_type ret = -1;
	gint cur_page;

	if(evt->type == GDK_BUTTON_RELEASE) {
		// TODO:...
		if(gRaid_i->mode & RAID_CONF_MODE) {
			ret = nftool_mbox(g_curwnd, "WARNING", "All recording data will be deleted after this configuration.\nPlease make sure if you want to do this!!", NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK) {
				cur_page = nfui_nftab_get_cur_page(gTab);

				if(cur_page == 0) g_rConfMode = RAID1_CONF;
				else if(cur_page == 1) g_rConfMode = RAID5_CONF;
			}
			else {
				scm_clear_raid_conf();
			}
		}
		else {
			if(gRaid_i->mode != 0) {
				cur_page = nfui_nftab_get_cur_page(gTab);
				if(cur_page == 0) g_rConfMode = RAID1_CONF;
				else if(cur_page == 1) g_rConfMode = RAID5_CONF;
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

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);

		scm_clear_raid_conf();
	}
	return FALSE;
}

gint VW_DiskRaid_AdvancedConf_Open(NFWINDOW *parent, STRG_TYPE_E strg_type)
{
	NFOBJECT *window = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *tab = NULL;
	NFOBJECT *tabPage[2];
	NFOBJECT *wbox = NULL;
	NFOBJECT *obj = NULL;

	const gchar *strImage_h[2] =  {
				(MKB_IMG_TAB_POP_DIR_H_N_208), 
				(MKB_IMG_TAB_POP_DIR_H_S_208)
	};
	const gchar *strTitle[] = {
				"RAID 1", 
				"RAID 5"
	};
	const guint colidx[3] = {COLOR_IDX(287), COLOR_IDX(289), COLOR_IDX(288)};
	gint i;

	

	// return val
	g_rConfMode = RAID_NONE_CONF;
	
	/* create button image */
	gBtnImg[0][0] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_N_BUTTON2, 120, IMG_N_DISK_RAID_BT02_L, IMG_N_DISK_RAID_BT02_M, IMG_N_DISK_RAID_BT02_R);
	gBtnImg[0][1] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_O_BUTTON2, 120, IMG_O_DISK_RAID_BT02_L, IMG_O_DISK_RAID_BT02_M, IMG_O_DISK_RAID_BT02_R);
	gBtnImg[0][2] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_P_BUTTON2, 120, IMG_P_DISK_RAID_BT02_L, IMG_P_DISK_RAID_BT02_M, IMG_P_DISK_RAID_BT02_R);
	gBtnImg[0][3] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_D_BUTTON2, 120, IMG_D_DISK_RAID_BT02_L, IMG_D_DISK_RAID_BT02_M, IMG_D_DISK_RAID_BT02_R);

	gBtnImg[1][0] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_N_BUTTON3, 120, IMG_N_DISK_RAID_BT03_L, IMG_N_DISK_RAID_BT03_M, IMG_N_DISK_RAID_BT03_R);
	gBtnImg[1][1] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_O_BUTTON3, 120, IMG_O_DISK_RAID_BT03_L, IMG_O_DISK_RAID_BT03_M, IMG_O_DISK_RAID_BT03_R);
	gBtnImg[1][2] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_P_BUTTON3, 120, IMG_P_DISK_RAID_BT03_L, IMG_P_DISK_RAID_BT03_M, IMG_P_DISK_RAID_BT03_R);
	gBtnImg[1][3] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_D_BUTTON3, 120, IMG_D_DISK_RAID_BT03_L, IMG_D_DISK_RAID_BT03_M, IMG_D_DISK_RAID_BT03_R);

	gBtnImg[2][0] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_N_BUTTON4, 120, IMG_N_DISK_RAID_BT04_L, IMG_N_DISK_RAID_BT04_M, IMG_N_DISK_RAID_BT04_R);
	gBtnImg[2][1] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_O_BUTTON4, 120, IMG_O_DISK_RAID_BT04_L, IMG_O_DISK_RAID_BT04_M, IMG_O_DISK_RAID_BT04_R);
	gBtnImg[2][2] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_P_BUTTON4, 120, IMG_P_DISK_RAID_BT04_L, IMG_P_DISK_RAID_BT04_M, IMG_P_DISK_RAID_BT04_R);
	gBtnImg[2][3] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_D_BUTTON4, 120, IMG_D_DISK_RAID_BT04_L, IMG_D_DISK_RAID_BT04_M, IMG_D_DISK_RAID_BT04_R);

	gBtnImg[3][0] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_N_BUTTON5, 120, IMG_N_DISK_RAID_BT05_L, IMG_N_DISK_RAID_BT05_M, IMG_N_DISK_RAID_BT05_R);
	gBtnImg[3][1] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_O_BUTTON5, 120, IMG_O_DISK_RAID_BT05_L, IMG_O_DISK_RAID_BT05_M, IMG_O_DISK_RAID_BT05_R);
	gBtnImg[3][2] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_P_BUTTON5, 120, IMG_P_DISK_RAID_BT05_L, IMG_P_DISK_RAID_BT05_M, IMG_P_DISK_RAID_BT05_R);
	gBtnImg[3][3] = nf_ui_create_image_button_no_alpha(MK_IMG_RAID_ADVANCED_D_BUTTON5, 120, IMG_D_DISK_RAID_BT05_L, IMG_D_DISK_RAID_BT05_M, IMG_D_DISK_RAID_BT05_R);



	/* window */
	window = (NFOBJECT*)nfui_nfwindow_new(parent, POP_POS_X, POP_POS_Y, WND_SIZE_W, WND_SIZE_H);
	g_curwnd = window;
	//nfui_regi_post_event_callback(window, post_disk_window_event_cb);
	//nfui_nfwindow_use_outside_evt((NFWINDOW*)window, TRUE);
	//nfui_nfwindow_set_mask((NFWINDOW*)window, GDK_BUTTON_PRESS, TRUE);
	//nfui_nfwindow_set_returnkey_proc((NFWINDOW*)window, returnkey_proc);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WND_SIZE_W, WND_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_disk_fixed_event_cb);
	nfui_nfobject_show(fixed);



	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ADVANCED RAID CONFIGURATION", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 1452, 36);
	//nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 9, 4);



	/* tab */
	tab = nfui_nftab_new(2, strImage_h, 208, 43, NFTAB_DIR_H, strTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)tab, 10);
	nfui_nfobject_show(tab);
	nfui_nffixed_put((NFFIXED*)fixed, tab, 9, 50);
	gTab = (NFTAB*)tab;

	for(i=0; i<2; i++) {
		tabPage[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tabPage[i], 1500, 309);
		nfui_nfobject_modify_bg(tabPage[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nftab_regi_page((NFTAB*)tab, tabPage[i], i);
		nfui_nffixed_put((NFFIXED*)fixed, tabPage[i], 9, 93);

		nfui_regi_pre_event_callback(tabPage[i], pre_page_event_cb);
	}
	init_raid1_tabpage(tabPage[0]);
	init_raid5_tabpage(tabPage[1]);
	nfui_nfobject_show(tabPage[0]);



	/* button */
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 564, 420);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 762, 420);

	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	// load raid info
	wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	if(strg_type == INTERNAL) g_timeout_add(300, display_int_advanced_raid_info, (gpointer)wbox);
	else g_timeout_add(300, display_ext_advanced_raid_info, (gpointer)wbox);
	gStrg_type = strg_type;

	nfui_page_open(PGID_DISK_RAID_ADVANCED_CONF, window, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DISK_RAID_ADVANCED_CONF, window);

	return g_rConfMode;
}
