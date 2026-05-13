
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftab.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"

#include "vw_disk_main.h"
#include "vw_disk_smart_info.h"
#include "ssm.h"


#define DSI_WIN_POS_X				((DISPLAY_ACTIVE_WIDTH - DSI_WIN_SIZE_W)/2)
#define DSI_WIN_POS_Y				((DISPLAY_ACTIVE_HEIGHT - DSI_WIN_SIZE_H)/2)

#if (var_get_supported_ext_disk)
#define DSI_WIN_SIZE_W				(1550)
#else
#define DSI_WIN_SIZE_W				(364+86+(220*var_get_display_int_disk_count()))
#endif

#define DSI_WIN_SIZE_H				(630)


static NFWINDOW *g_curwnd = 0;
static DISK_CAPINFO_T *disk_i;
static DISK_SMARTINFO_T *gSmart_i;


static gchar* conv_temperature_to_string(guint temperature, gchar *buf)
{
//	gchar buf[8];

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, lookup_string("%d (celsius)"), temperature);
	return buf;
}

static gchar* conv_uint_to_string(guint val, gchar *buf)
{
//	gchar buf[32];

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, "%d", val);
	return buf;
}

static gchar* conv_uint64_to_string(guint64 val, gchar *buf)
{
//	gchar buf[32];

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, "%llu", val);
	return buf;
}

static gchar* conv_powerontime_to_string(guint val, gchar *buf)
{
//	gchar buf[32];

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, lookup_string("%d (h)"), val);
	return buf;
}

static void	display_disks_info(gint grp, NFTABLE *table)
{
	NFOBJECT *obj;
	guint d_status;
	gint disp_cnt = 0;
	gint i, j;
	gchar buf[128];

	memset(buf, 0x00, sizeof(buf));

	if(grp == INTERNAL_DISK_GRP) disp_cnt = INT_DISP_DISK_COUNT;
	else 						 disp_cnt = EXT_DISP_DISK_COUNT;

	for(i=0; i<disp_cnt; i++)
	{
		if(gSmart_i->disk_unit[i].valid == DISK_INVALID)
			continue;

		for(j=0; j<11; j++)
		{
			obj = nfui_nftable_get_child(table, i + 1, j);

			if(gSmart_i->disk_unit[i].disk_status > 1) 		{
				if(j == 0)		d_status = gSmart_i->disk_unit[i].disk_status;
				else if(j == 2)	d_status = gSmart_i->disk_unit[i].raw_read_error_rate.status;
				else if(j == 3)	d_status = gSmart_i->disk_unit[i].seek_error_rate.status;
				else if(j == 4)	d_status = gSmart_i->disk_unit[i].reallocated_sector_ct.status;
				else if(j == 5)	d_status = gSmart_i->disk_unit[i].spin_up_time.status;

				if(d_status == 3) 		nfui_nflabel_set_pango_font((NFLABEL*)obj, NULL, COLOR_IDX(190));				// error
				else if(d_status == 2) 	nfui_nflabel_set_pango_font((NFLABEL*)obj, NULL, COLOR_IDX(191));				// check
			}
			nfui_nflabel_set_pango_font((NFLABEL*)obj, NULL, COLOR_IDX(389));											// normal


			if(j == 0) 		nfui_nflabel_set_text((NFLABEL*)obj, conv_smart_status_to_string(gSmart_i->disk_unit[i].disk_status, buf));					// smart status
			else if(j == 1) nfui_nflabel_set_text((NFLABEL*)obj, conv_temperature_to_string(gSmart_i->disk_unit[i].temperature_celsius, buf));			// hdd temperature
			else if(j == 2) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint_to_string(gSmart_i->disk_unit[i].raw_read_error_rate.value, buf));			// read error rate
			else if(j == 3) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint_to_string(gSmart_i->disk_unit[i].seek_error_rate.value, buf));				// seek error rate
			else if(j == 4) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint64_to_string(gSmart_i->disk_unit[i].reallocated_sector_ct.raw, buf));			// reallocated sector val
			else if(j == 5) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint64_to_string(gSmart_i->disk_unit[i].spin_up_time.raw, buf));					// spin up time
			else if(j == 6) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint_to_string(gSmart_i->disk_unit[i].spin_retry_cnt, buf));						// spin retry cnt
			else if(j == 7) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint_to_string(gSmart_i->disk_unit[i].start_stop_cnt, buf));						// start/stop cnt
			else if(j == 8) nfui_nflabel_set_text((NFLABEL*)obj, conv_uint_to_string(gSmart_i->disk_unit[i].power_cycle_cnt, buf));						// power cycle count
			else if(j == 9) nfui_nflabel_set_text((NFLABEL*)obj, conv_powerontime_to_string(gSmart_i->disk_unit[i].power_on_hours, buf));				// power-on hours
			else if(j == 10) nfui_nflabel_set_text((NFLABEL*)obj, disk_i->disk_unit[i].serial);						// serial number
		}
	}
}

static void internal_disk_smart_info(NFOBJECT *page)
{
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gchar *strCol[] = {"DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strRow[] = {"S.M.A.R.T STATUS",
					"HDD TEMPERATURE",
					"READ ERROR RATE",
					"SEEK ERROR RATE",
					"BAD SECTOR COUNT",
					"SPIN UP TIME",
					"SPIN RETRY COUNT",
					"START/STOP COUNT",
					"POWER CYCLE COUNT",
					"DISK POWER-ON HOURS",
					"SERIAL NUMBER"};

	guint table_w[] = {364, 220, 220, 220, 220, 220};
	gint i, j;



	tbl = (NFOBJECT*)nfui_nftable_new(6, 11, 2, 1, table_w, 30);
	nfui_nfobject_show(tbl);
	nfui_nftable_set_draw_outline((NFTABLE *)tbl, TRUE);
	nfui_nffixed_put((NFFIXED*)page, tbl, 10, 75);



	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_set_size(obj, 220, 30);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)page, obj, (374 + (i * 220)), 41);
	}


	for(i=0; i<11; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRow[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, i);
	}

	// disk
	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		for(j=0; j<11; j++) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(389));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)tbl, obj,  i + 1, j);
		}
	}

	display_disks_info(INTERNAL_DISK_GRP, (NFTABLE*)tbl);
}

static void external_storage_smart_info(NFOBJECT *page)
{
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gchar *strCol[] = {"DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strRow[] = {"S.M.A.R.T STATUS",
					"HDD TEMPERATURE",
					"READ ERROR RATE",
					"SEEK ERROR RATE",
					"BAD SECTOR COUNT",
					"SPIN UP TIME",
					"SPIN RETRY COUNT",
					"START/STOP COUNT",
					"POWER CYCLE COUNT",
					"DISK POWER-ON HOURS",
					"SERIAL NUMBER"};

	guint table_w[] = {364, 220, 220, 220, 220, 220};
	gint i, j;



	tbl = (NFOBJECT*)nfui_nftable_new(6, 11, 2, 1, table_w, 30);
	nfui_nfobject_show(tbl);
	nfui_nftable_set_draw_outline((NFTABLE *)tbl, TRUE);
	nfui_nffixed_put((NFFIXED*)page, tbl, 10, 75);



	for(i=0; i<EXT_DISP_DISK_COUNT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_set_size(obj, 220, 30);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)page, obj, (374 + (i * 220)), 41);
	}


	for(i=0; i<11; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRow[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, i);
	}

	// disk
	for(i=0; i<EXT_DISP_DISK_COUNT; i++) {
		for(j=0; j<11; j++) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(389));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)tbl, obj,  i + 1, j);
		}
	}

	display_disks_info(EXTERNAL_DISK_GRP, (NFTABLE*)tbl);
}

static gboolean post_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean pre_tab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case NFEVENT_TAB_BEFORE_CHANGE:
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_dsi_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
	    g_curwnd = 0;
		nfui_page_close(PGID_DISK_SMART_POPUP, obj);
	}

	return FALSE;
}

static gboolean post_dsi_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

void VW_DiskSmartInfo_Open(NFWINDOW *parent, gint group)
{
	NFOBJECT *dsi_win;
	NFOBJECT *dsi_fixed;
	NFOBJECT *dsi_tab;
	NFOBJECT *tabPage;
	NFOBJECT *obj;

	const gchar *strImage_h[2] =  {
				(MKB_IMG_TAB_POP_DIR_H_N_274),
				(MKB_IMG_TAB_POP_DIR_H_S_274)
	};
	const gchar *strTitle[2] = {
				"INTERNAL DISKS",
				"EXTERNAL STORAGE"
	};

	const guint colidx[3] = {COLOR_IDX(287), COLOR_IDX(289), COLOR_IDX(288)};
	gint i;


	// smart info
	disk_i = get_disk_info(group);
	gSmart_i = get_smart_disk_info(group);


	/* window */
	dsi_win = (NFOBJECT*)nfui_nfwindow_new(parent, DSI_WIN_POS_X, DSI_WIN_POS_Y, DSI_WIN_SIZE_W, DSI_WIN_SIZE_H);
	nfui_regi_post_event_callback(dsi_win, post_dsi_win_event_cb);
	g_curwnd = dsi_win;


	/* fixed */
	dsi_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(dsi_fixed, DSI_WIN_SIZE_W, DSI_WIN_SIZE_H);
	nfui_regi_post_event_callback(dsi_fixed, post_dsi_fixed_event_cb);
	nfui_nfobject_show(dsi_fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("S.M.A.R.T INFORMATION", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 14);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)dsi_fixed, obj, 4, 4);

	/* tab */
	dsi_tab = nfui_nftab_new(1, strImage_h, 274, 43, NFTAB_DIR_H, &strTitle[group], colidx);
	nfui_nftab_set_pango_font((NFTAB*)dsi_tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)dsi_tab, 10);
	nfui_regi_pre_event_callback(dsi_tab, pre_tab_event_handler);
	nfui_nfobject_show(dsi_tab);
	nfui_nffixed_put((NFFIXED*)dsi_fixed, dsi_tab, 14, 50);


	tabPage = nfui_nffixed_new();
	nfui_nftab_regi_page((NFTAB*)dsi_tab, tabPage, 0);
	nfui_nfobject_set_size(tabPage, 1512, 442);
	nfui_nfobject_modify_bg(tabPage, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nffixed_put((NFFIXED*)dsi_fixed, tabPage, 14, 90);

	nfui_regi_post_event_callback(tabPage, post_page_event_cb);

	if(group == INTERNAL_DISK_GRP) internal_disk_smart_info(tabPage);
	else 						   external_storage_smart_info(tabPage);
	nfui_nfobject_show(tabPage);



	// button
	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)dsi_fixed, obj, 1336, 558);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


	nfui_nfwindow_add((NFWINDOW*)dsi_win, dsi_fixed);
	nfui_run_main_event_handler(dsi_win);
	nfui_nfobject_show(dsi_win);
	nfui_make_key_hierarchy((NFWINDOW*)dsi_win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_DISK_SMART_POPUP, dsi_win, ssm_get_cur_id(NULL));
}
