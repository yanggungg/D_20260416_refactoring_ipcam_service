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
#include "objects/nfimglabel.h"
#include "objects/nfbutton.h"

#include "services/scm.h"

#include "vw_disk_main.h"
#include "vw_disk_internal_information.h"
#include "vw_disk_information.h"

#include "dtf.h"

#define DISK_COUNT		5

enum {
	ALL = 0,
	DISK_N,
};

static NFWINDOW *g_curwnd = 0;
static DISK_CAPINFO_T *disk_i;
static DISK_RECINFO_T *disk_t;
static DISK_SMARTINFO_T *smart_i;
static DISK_RAIDINFO_T *raid_i;

static NFTABLE *g_table[2];


static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		VW_DiskInfo_tab_out_handler(obj);
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


void VW_Init_DiskInternal_Info(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *allTbl;
	NFOBJECT *diskTbl;
	NFOBJECT *obj;

	GdkPixbuf *pbIcon[INT_DISP_DISK_COUNT + 1];

	gchar *strCol[] = {"START TIME", "END TIME", "STATUS", "CAPACITY", "MODEL", "S.M.A.R.T STATUS"};
	gchar *strRow[] = {"ALL", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};

	guint table_w[] = {172, 297, 297, 142, 152, 202, 192};
	gint i, j;
	nffont_type font_idx;
	
	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	/* all table */
	allTbl = (NFOBJECT*)nfui_nftable_new(7, 2, 2, 1, table_w, 40);	
	nfui_nfobject_modify_bg(allTbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(allTbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, allTbl, 27, 55);

	/* disk n table */
	diskTbl = (NFOBJECT*)nfui_nftable_new(7, DISK_COUNT, 2, 1, table_w, 40);	
	nfui_nfobject_modify_bg(diskTbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(diskTbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, diskTbl, 27, 147);

	g_table[ALL] = (NFTABLE*)allTbl;
	g_table[DISK_N] = (NFTABLE*)diskTbl;

	if(nftool_cur_language_is_japanese())
		font_idx = NFFONT_SMALL_SEMI_1;
	else
		font_idx = NFFONT_MEDIUM_SEMI;

	// col label
	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(font_idx), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)allTbl, obj,  (i + 1), 0);
	}
	
	// row label
	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 

	for(i=0; i<6; i++) {
		obj = nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(font_idx), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if(i == 0) 	 nfui_nftable_attach((NFTABLE*)allTbl, obj, 0, i + 1);
		else		 nfui_nftable_attach((NFTABLE*)diskTbl, obj, 0, i - 1);
	}

	// disk info label
	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(font_idx));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)allTbl, obj, i + 1, 1);
	}


	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		for(j=0; j<6; j++) {
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(font_idx));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);			
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)diskTbl, obj, j + 1, i);
		}
	}

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

	obj = nftool_normal_button_create_type1("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean display_internal_disk_information(gboolean expose)
{
	NFOBJECT *obj;

	gchar buf[32];

	time_t aStart_t = 0;
	time_t aEnd_t = 0;
	gint i, j;

	/* get disk data */
	disk_i = get_disk_info(INTERNAL_DISK_GRP);
	smart_i = get_smart_disk_info(INTERNAL_DISK_GRP);
	disk_t = get_disk_rec_time(INTERNAL_DISK_GRP);
    raid_i = get_disk_raid_info(INTERNAL_DISK_GRP);

	// ALL //////////////////////////////////////////////////////////////////////
	aStart_t = disk_t->trec_start;
	aEnd_t = disk_t->trec_end;

	if(aStart_t != 0 && dtf_get_local_datetime(aStart_t, buf)) {
		obj = nfui_nftable_get_child(g_table[ALL], 1, 1);
		nfui_nflabel_set_text((NFLABEL*)obj, buf);
	}

	if(aEnd_t != 0 && dtf_get_local_datetime(aEnd_t, buf)) {
		obj = nfui_nftable_get_child(g_table[ALL], 2, 1);
		nfui_nflabel_set_text((NFLABEL*)obj, buf);
	}


	// DISK N ///////////////////////////////////////////////////////////////////
	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		if(disk_i->disk_unit[i].size == 0)
			continue;

		for(j=0; j<6; j++) {

			obj = nfui_nftable_get_child(g_table[DISK_N], j + 1, i);

			if(j == 0)					// start time
			{
				if(disk_t->disk_unit[i].rec_start == 0)
					nfui_nflabel_set_text((NFLABEL*)obj, "-");
				else if(dtf_get_local_datetime((time_t)disk_t->disk_unit[i].rec_start, buf))
					nfui_nflabel_set_text((NFLABEL*)obj, buf);
			}
			else if(j == 1)					// end time
			{
				if(disk_t->disk_unit[i].rec_end == 0)
					nfui_nflabel_set_text((NFLABEL*)obj, "-");
				else if(dtf_get_local_datetime((time_t)disk_t->disk_unit[i].rec_end, buf))
					nfui_nflabel_set_text((NFLABEL*)obj, buf);
			}
			else if(j==2)				// status
			{
		        if((raid_i->mode & (1 << 1)) || (raid_i->mode & (1 << 5)))
		        {
                    if(raid_i->rinfo[0].status == 0){
                        nfui_nflabel_set_text((NFLABEL*)obj, "BROKEN");
                    }
                    else if(raid_i->rinfo[0].status == 1){
                        nfui_nflabel_set_text((NFLABEL*)obj, "DEGRADED");
                    }
                    else if(raid_i->rinfo[0].status == 2){
                        nfui_nflabel_set_text((NFLABEL*)obj, "REBUILD");
                    }
                    else if(raid_i->rinfo[0].status == 3){
                        nfui_nflabel_set_text((NFLABEL*)obj, "NORMAL");
                    }
                }
                else
                {
    				if(disk_i->disk_unit[i].use) 	nfui_nflabel_set_text((NFLABEL*)obj, "IN USE");
    				else							nfui_nflabel_set_text((NFLABEL*)obj, "NOT IN USE");
                }
			}
			else if(j==3)				// capacity
			{
				memset(buf, 0x00, sizeof(buf));
				ifn_convert_storage_size(buf, disk_i->disk_unit[i].size);
				nfui_nflabel_set_text((NFLABEL*)obj, buf);
			}
			else if(j==4)				// model
			{
				nfui_nflabel_set_text((NFLABEL*)obj, disk_i->disk_unit[i].model);	
			}
			else if(j==5)				// smart status
			{
				nfui_nflabel_set_text((NFLABEL*)obj, conv_smart_status_to_string(smart_i->disk_unit[i].disk_status, buf));	
			}
		}
	}

	if(expose) {
		nfui_signal_emit(g_table[ALL], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_table[DISK_N], GDK_EXPOSE, TRUE);
	}

	return TRUE;
}
