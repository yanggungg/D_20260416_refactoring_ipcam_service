
#include "nf_afx.h"

//FIXME
#include "nf_api_disk.h"
#include "nf_api_raid.h"

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
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nftab.h"

#include "services/scm.h"

#include "modules/ssm.h"

#include "vw_live_disk_status.h"
#include "dtf.h"




#define DISK_STATUS_POS_X					((DISPLAY_ACTIVE_WIDTH - DISK_STATUS_SIZE_W)/2)
#define DISK_STATUS_POS_Y					((DISPLAY_ACTIVE_HEIGHT - DISK_STATUS_SIZE_H)/2)
#define DISK_STATUS_SIZE_W					(1533)
#if (var_get_supported_ext_disk)
#define DISK_STATUS_SIZE_H					(446)
#else
#define DISK_STATUS_SIZE_H					(296+(var_get_display_int_disk_count()*30))
#endif

#define DISK_COUNT							5

enum {
	ALL = 0,
	DISK_N
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_table[2][2];
static NFOBJECT *g_wbox = NULL;
static DISK_DB_T g_ddb;

static guint g_tmr_wait = 0;
static guint g_tmr_disp = 0;

static gchar* conv_smart_status_to_string(guint status, gchar *buf)
{
/*
	switch(status) {
		case 0: return "NORMAL";
		case 1: return "NORMAL";		// in the past
		case 2: return "CHECK";
		case 3: return "ERROR";			// failing now
	}
	return "-";*/

	strcpy(buf, "-");

	switch(status) {
		case 0: strcpy(buf, "NORMAL"); break;
		case 1: strcpy(buf, "NORMAL"); break;		// in the past
		case 2: strcpy(buf, "CHECK"); break;
		case 3: strcpy(buf, "ERROR"); break;			// failing now
	}
	return buf;
}

static void display_disk_status(gint disk_group, gboolean expose)
{
	DISK_RECINFO_T rec;
	DISK_CAPINFO_T cap;
	DISK_SMARTINFO_T smart;
	DISK_RAIDINFO_T raid;
	time_t aStart_t = 0;
	time_t aEnd_t = 0;

	NFOBJECT *obj;
	NFTABLE *table1, *table2;

	gchar buf[32];
	gint i, j;
	gint disk_count;

	time_t rec_s;
	time_t rec_e;
	

	table1 = (NFTABLE*)g_table[disk_group][ALL];
	table2 = (NFTABLE*)g_table[disk_group][DISK_N];

	memset(&rec, 0x00, sizeof(DISK_RECINFO_T));
	memcpy(&rec, &g_ddb.rec[disk_group], sizeof(DISK_RECINFO_T));
	
	memset(&cap, 0x00, sizeof(DISK_CAPINFO_T));
	memcpy(&cap, &g_ddb.cap[disk_group], sizeof(DISK_CAPINFO_T));
	
	memset(&smart, 0x00, sizeof(DISK_SMARTINFO_T));
	memcpy(&smart, &g_ddb.smart[disk_group], sizeof(DISK_SMARTINFO_T));
	
	memset(&raid, 0x00, sizeof(DISK_RAIDINFO_T));
	if(disk_group == INTERNAL && var_get_supported_raid())
	{
    	memcpy(&raid, &g_ddb.raid, sizeof(DISK_RAIDINFO_T));
	}

	if(disk_group == INTERNAL) disk_count = DISK_COUNT;
	else					   disk_count = DISK_COUNT;

	////////////// disk time //////////////////////////////////////////////////////////////////////
	// FIXME: start/end time api
	for(i=0; i<disk_count; i++) {
		if (rec.disk_unit[i].valid) {
			// start time
			rec_s = rec.disk_unit[i].rec_start;
			if(rec_s)
				dtf_get_local_datetime((time_t)rec_s, buf);

			obj = nfui_nftable_get_child(table2, 0, i);
			if(rec_s) nfui_nflabel_set_text((NFLABEL*)obj, buf);
			else 				nfui_nflabel_set_text((NFLABEL*)obj, "-");

			if(expose)
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);


			// end time
			rec_e = rec.disk_unit[i].rec_end;
			if(rec_e)
				dtf_get_local_datetime((time_t)rec_e, buf);

			obj = nfui_nftable_get_child(table2, 1, i);
			if(rec_e) nfui_nflabel_set_text((NFLABEL*)obj, buf);
			else 			  	nfui_nflabel_set_text((NFLABEL*)obj, "-");

			if(expose)
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		} else {
//			if(disk_i.disk_unit[i].size != 0) {
				obj = nfui_nftable_get_child(table2, 1, i);
				nfui_nflabel_set_text((NFLABEL*)obj, "-");

				if(expose)
					nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

				obj = nfui_nftable_get_child(table2, 0, i);
				nfui_nflabel_set_text((NFLABEL*)obj, "-");

				if(expose)
					nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
//			}
			continue;
		}

/*
		if(disk_t.min_time) {
			if(aStart_t == 0 || aStart_t > disk_t.min_time)
				aStart_t = disk_t.min_time;	
		}

		if(disk_t.max_time) {
			if(aEnd_t < disk_t.max_time) 
				aEnd_t = disk_t.max_time;	
		}
*/
	}

	aStart_t = rec.trec_start;
	aEnd_t = rec.trec_end;

	// all start time
	if(aStart_t) {
		dtf_get_local_datetime((time_t)aStart_t, buf);

		obj = nfui_nftable_get_child(table1, 0, 0);
		nfui_nflabel_set_text((NFLABEL*)obj, buf);

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}

	// all end time
	if(aEnd_t) {
		dtf_get_local_datetime((time_t)aEnd_t, buf);

		obj = nfui_nftable_get_child(table1, 1, 0);
		nfui_nflabel_set_text((NFLABEL*)obj, buf);

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}

	////////////// disk info //////////////////////////////////////////////////////////////////////
	// status

	
    if((raid.mode & (1 << 1)) || (raid.mode & (1 << 5))){
        for(i = 0;  i < disk_count; i++) {   
            if(cap.disk_unit[i].valid == DISK_INVALID) continue;
            
            obj = nfui_nftable_get_child(table2, 2, i);
            
            if(raid.rinfo[0].status == 0){
                nfui_nflabel_set_text((NFLABEL*)obj, "BROKEN");
            }
            else if(raid.rinfo[0].status == 1){
                nfui_nflabel_set_text((NFLABEL*)obj, "DEGRADED");
            }
            else if(raid.rinfo[0].status == 2){
                nfui_nflabel_set_text((NFLABEL*)obj, "REBUILD");
            }
            else if(raid.rinfo[0].status == 3){
                nfui_nflabel_set_text((NFLABEL*)obj, "NORMAL");
            }
            if(expose){
    			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    		}
		}
    }
    else{
    	for(i=0; i<disk_count; i++) {
    		if(cap.disk_unit[i].valid == DISK_INVALID) continue;

    		obj = nfui_nftable_get_child(table2, 2, i);

    		if(cap.disk_unit[i].use) 	nfui_nflabel_set_text((NFLABEL*)obj, "IN USE");
    		else							nfui_nflabel_set_text((NFLABEL*)obj, "NOT IN USE");

    		if(expose)
    			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    	}
    }
    
	// capacity
	for(i=0; i<disk_count; i++) {
		if(cap.disk_unit[i].valid == DISK_INVALID) continue;

		memset(buf, 0x00, sizeof(buf));
		ifn_convert_storage_size(buf, cap.disk_unit[i].size);

		obj = nfui_nftable_get_child(table2, 3, i);
		nfui_nflabel_set_text((NFLABEL*)obj, buf);

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}

	// model
	for(i=0; i<disk_count; i++) {
		if(cap.disk_unit[i].valid == DISK_INVALID) continue;

		obj = nfui_nftable_get_child(table2, 4, i);
		nfui_nflabel_set_text((NFLABEL*)obj, cap.disk_unit[i].model);	

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}

	////////////// disk smart/////////////////////////////////////////////////////////////////////
	//FIXME: smart status
	for(i=0; i<disk_count; i++) {
		if(smart.disk_unit[i].valid == DISK_INVALID) continue;

		obj = nfui_nftable_get_child(table2, 5, i);
		nfui_nflabel_set_text((NFLABEL*)obj, conv_smart_status_to_string(smart.disk_unit[i].disk_status, buf));	

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}
/*
	for(i=0; i<2; i++) {
		g_printf(">>>>>>>>>>>>>>>>>>>> smart status :");
		for(j=0; j<16; j++) 
			g_printf(" %c [%d] ", smart_i.status[i][j], smart_i.status[i][j]);
		putchar('\n');

		g_printf(">>>>>>>>>>>>>>>>>>>> smart temperature :");
		for(j=0; j<16; j++)
			g_printf(" %d", smart_i.temperature[i][j]);

		putchar('\n');
	}
*/
}

static gboolean internal_disk_status_page(NFOBJECT *page)
{
	NFOBJECT *tbl1, *tbl2;
	NFOBJECT *obj;
	guint table_w[] = {297, 297, 159, 170, 203, 193};
	gchar *topTitle[] = {"START TIME", "END TIME", "STATUS", "CAPACITY", "MODEL", "S.M.A.R.T STATUS"};
	gchar *leftTitle[] = {"ALL", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gint i, j;
	nffont_type font_idx;

	tbl1 = (NFOBJECT*)nfui_nftable_new(6, 1, 1, 1, table_w, 30);	
	nfui_nfobject_show(tbl1);
	nfui_nffixed_put((NFFIXED*)page, tbl1, 142, 24);

	if(nftool_cur_language_is_japanese())
		font_idx = NFFONT_SMALL_SEMI_1;
	else
		font_idx = NFFONT_MEDIUM_SEMI;

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(topTitle[i], nffont_get_pango_font(font_idx), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl1, obj,  i, 0);
	}

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(leftTitle[i], nffont_get_pango_font(font_idx), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_set_size(obj, 102, 30);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if(i < 1) nfui_nffixed_put((NFFIXED*)page, obj, 39, 55);
		else	  nfui_nffixed_put((NFFIXED*)page, obj, 39, (55 + 12 + (i * 30)));
	}

	tbl1 = (NFOBJECT*)nfui_nftable_new(6, 1, 1, 1, table_w, 30);	
	nfui_nftable_set_draw_outline((NFTABLE*)tbl1, TRUE);
	nfui_nfobject_show(tbl1);
	nfui_nffixed_put((NFFIXED*)page, tbl1, 142, 55);

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(font_idx), COLOR_IDX(389));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl1, obj,  i, 0);
	}

	tbl2 = (NFOBJECT*)nfui_nftable_new(6, DISK_COUNT, 1, 1, table_w, 30);	
	nfui_nftable_set_draw_outline((NFTABLE*)tbl2, TRUE);
	nfui_nfobject_show(tbl2);
	nfui_nffixed_put((NFFIXED*)page, tbl2, 142, 95);

	for(i=0; i<DISK_COUNT; i++) {
		for(j=0; j<6; j++) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(font_idx), COLOR_IDX(389));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)tbl2, obj,  j, i);
		}
	}

	g_table[INTERNAL][ALL] = tbl1;
	g_table[INTERNAL][DISK_N] = tbl2;

	return TRUE;
}

static gboolean external_disk_status_page(NFOBJECT *page)
{
	NFOBJECT *tbl1, *tbl2;
	NFOBJECT *obj;
	guint table_w[] = {297, 297, 159, 170, 203, 193};
	gchar *topTitle[] = {"START TIME", "END TIME", "STATUS", "CAPACITY", "MODEL", "S.M.A.R.T STATUS"};
	gchar *leftTitle[] = {"ALL", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gint i, j;
	nffont_type font_idx;

	tbl1 = (NFOBJECT*)nfui_nftable_new(6, 1, 1, 1, table_w, 30);	
	nfui_nfobject_show(tbl1);
	nfui_nffixed_put((NFFIXED*)page, tbl1, 142, 24);

	if(nftool_cur_language_is_japanese())
		font_idx = NFFONT_SMALL_SEMI_1;
	else
		font_idx = NFFONT_MEDIUM_SEMI;

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(topTitle[i], nffont_get_pango_font(font_idx), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl1, obj,  i, 0);
	}

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(leftTitle[i], nffont_get_pango_font(font_idx), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nfobject_set_size(obj, 102, 30);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if(i < 1) nfui_nffixed_put((NFFIXED*)page, obj, 39, 55);
		else	  nfui_nffixed_put((NFFIXED*)page, obj, 39, (55 + 12 + (i * 30)));
	}

	tbl1 = (NFOBJECT*)nfui_nftable_new(6, 1, 1, 1, table_w, 30);	
	nfui_nftable_set_draw_outline((NFTABLE*)tbl1, TRUE);
	nfui_nfobject_show(tbl1);
	nfui_nffixed_put((NFFIXED*)page, tbl1, 142, 55);

	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(font_idx), COLOR_IDX(389));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl1, obj,  i, 0);
	}

	tbl2 = (NFOBJECT*)nfui_nftable_new(6, DISK_COUNT, 1, 1, table_w, 30);	
	nfui_nftable_set_draw_outline((NFTABLE*)tbl2, TRUE);
	nfui_nfobject_show(tbl2);
	nfui_nffixed_put((NFFIXED*)page, tbl2, 142, 95);

	for(i=0; i<DISK_COUNT; i++) {
		for(j=0; j<6; j++) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(font_idx), COLOR_IDX(389));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)tbl2, obj,  j, i);
		}
	}

	g_table[EXTERNAL][ALL] = tbl1;
	g_table[EXTERNAL][DISK_N] = tbl2;

	return TRUE;
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

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
        if (g_tmr_wait) {
            g_source_remove(g_tmr_wait);
            g_tmr_wait = 0;
        }
        if (g_tmr_disp) {
            g_source_remove(g_tmr_disp);
            g_tmr_disp = 0;
        }
        nfui_page_close(PGID_LIVE_DISK_STATUS, g_curwnd);
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean disp_cb()
{
	display_disk_status(INTERNAL, TRUE);
	if (var_get_supported_ext_disk())
    	display_disk_status(EXTERNAL, FALSE);
    	
	return FALSE;
}

static gboolean show_mbox(gpointer data)
{
	NFOBJECT *mbox = (NFOBJECT*)data;

	NFUTIL_THREADS_ENTER();
	nfui_nfobject_show(mbox);
	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean post_disk_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT *wait_mbox = NULL;
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
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
        uxm_unreg_imsg_event(obj, IRET_SCM_GET_DISK_INFO);
	}
	else if (evt->type == IRET_SCM_GET_DISK_INFO)
	{
	    DISK_DB_T *ddb = ((CMM_MESSAGE_T*)data)->data;

	    memcpy(&g_ddb, ddb, sizeof(DISK_DB_T));
	    
	    if (g_wbox)
	    {
	        nftool_remove_waitbox(g_wbox);
	        g_wbox = NULL;
	    }
		disp_cb();
	}

	return FALSE;
}


static gboolean post_disk_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFOUTEVT_BUTTON_PRESS:
		    {
                if (g_tmr_wait) {
                    g_source_remove(g_tmr_wait);
                    g_tmr_wait = 0;
                }

                if (g_tmr_disp) {
                    g_source_remove(g_tmr_disp);
                    g_tmr_disp = 0;
                }
		    
			nfui_nfobject_destroy(obj);
			}
			break;

		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_EXIT) 
				{
                    if (g_tmr_wait) {
                        g_source_remove(g_tmr_wait);
                        g_tmr_wait = 0;
                    }

                    if (g_tmr_disp) {
                        g_source_remove(g_tmr_disp);
                        g_tmr_disp = 0;
                    }
				
					nfui_nfobject_destroy(obj);
					return TRUE;
				}
			}
			break;

		case GDK_DELETE:
			g_curwnd = 0;
			nfui_page_close(PGID_LIVE_DISK_STATUS, obj);
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gboolean VW_Create_DiskStatus(NFWINDOW *parent)
{
	NFOBJECT *diskWin = NULL;
	NFOBJECT *diskFixed = NULL;
	NFOBJECT *diskTab = NULL;
	NFOBJECT *tabPage[2];
	NFOBJECT *obj = NULL;

	gint i, tab_cnt;
	const gchar *strImage_h[2] =  {
				(MKB_IMG_TAB_POP_DIR_H_N_274), 
				(MKB_IMG_TAB_POP_DIR_H_S_274)
	};
	const gchar *strTitle[2] = {
				"INTERNAL DISKS",
				"EXTERNAL STORAGE"
	};
	const guint colidx[3] = {COLOR_IDX(287), COLOR_IDX(289), COLOR_IDX(288)};


	/* window */
	diskWin = (NFOBJECT*)nfui_nfwindow_new(parent, DISK_STATUS_POS_X, DISK_STATUS_POS_Y, DISK_STATUS_SIZE_W, DISK_STATUS_SIZE_H);
	g_curwnd = diskWin;
	nfui_regi_post_event_callback(diskWin, post_disk_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)diskWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)diskWin, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)diskWin, returnkey_proc);
	

	/* fixed */
	diskFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(diskFixed, DISK_STATUS_SIZE_W, DISK_STATUS_SIZE_H);
	nfui_regi_post_event_callback(diskFixed, post_disk_fixed_event_cb);
	nfui_nfobject_show(diskFixed);


	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK STATUS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 14);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)diskFixed, obj, 4, 4);


	/* tab */
    if (var_get_supported_ext_disk())   tab_cnt = 2;
    else                                tab_cnt = 1;
    
	diskTab = nfui_nftab_new(tab_cnt, strImage_h, 274, 43, NFTAB_DIR_H, strTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)diskTab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)diskTab, 10);
	nfui_nfobject_show(diskTab);
	nfui_nffixed_put((NFFIXED*)diskFixed, diskTab, 9, 50);

	for(i = 0; i < tab_cnt; i++) {
		tabPage[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tabPage[i], 1509, 269);
		nfui_nftab_regi_page((NFTAB*)diskTab, tabPage[i], i);
		nfui_nfobject_modify_bg(tabPage[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nffixed_put((NFFIXED*)diskFixed, tabPage[i], 9, 90);

		nfui_regi_pre_event_callback(tabPage[i], pre_page_event_cb);
	}


	internal_disk_status_page(tabPage[0]);
	if (var_get_supported_ext_disk())
    	external_disk_status_page(tabPage[1]);


	nfui_nfobject_show(tabPage[0]);


	/* button */
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)diskFixed, obj, 670, 377);


	nfui_nfwindow_add((NFWINDOW*)diskWin, diskFixed);
	nfui_run_main_event_handler(diskWin);
	nfui_nfobject_show(diskWin);
	nfui_make_key_hierarchy((NFWINDOW*)diskWin);
	
    uxm_reg_imsg_event(diskFixed, IRET_SCM_GET_DISK_INFO);

	nfui_page_open(PGID_LIVE_DISK_STATUS, diskWin, ssm_get_cur_id(NULL));
	
	g_wbox = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Fetching disk information.", "Please wait...");
    scm_get_disk_info(IRET_SCM_GET_DISK_INFO);
    
	return TRUE;
}

void VW_Destroy_DiskStatus()
{
	if (g_curwnd) 
	{
        if (g_tmr_wait) {
            g_source_remove(g_tmr_wait);
            g_tmr_wait = 0;
        }

        if (g_tmr_disp) {
            g_source_remove(g_tmr_disp);
            g_tmr_disp = 0;
        }
	    nfui_nfobject_destroy(g_curwnd);
    }
}
