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
#include "objects/nfimage.h"

#include "modules/ssm.h"
#include "services/scm.h"

#include "vw_disk_main.h"
#include "vw_disk_raid.h"
#include "vw_disk_raid_easy_conf.h"

#define WND_SIZE_W				(962)
#define WND_SIZE_H				(822)

#define POP_POS_X				((DISPLAY_ACTIVE_WIDTH - WND_SIZE_W)/4*2)
#define POP_POS_Y				((DISPLAY_ACTIVE_HEIGHT - WND_SIZE_H)/2)

#define ENABLE_RAID5_DISK_CNT   (3)
#define ENABLE_RAID1_DISK_CNT   (2)

enum {
	EASY_CONF_RAID1,
	EASY_CONF_RAID5
};

static DISK_RAIDINFO_T	*gRaid_i = NULL;
static DISK_RAIDINFO_T	gRaid_conf[2];
static gint g_rConfMode;
static gint g_disk_cnt;
static gint g_enable_R1;
static gint g_enable_R5;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_okBtn;
static NFTAB *g_subtab;

static NFTABLE *gRaid1_tbl;				// raid 1 tab
static NFIMAGE *gRaid1_img0;
static NFIMAGE *gRaid1_img1;
static NFLABEL *gRaid1_lb0;
static NFLABEL *gRaid1_lb1;
static NFLABEL *gRaid1_ins0;
static NFLABEL *gRaid1_ins1;
static NFLABEL *gRaid1_ins2;

static NFTABLE *gRaid5_tbl;				// raid 5 tab
static NFIMAGE *gRaid5_img0;
static NFLABEL *gRaid5_lb0;				// #r0 label
static NFLABEL *gRaid5_lb1;				// disk number label 1~5
static NFLABEL *gRaid5_lb2;
static NFLABEL *gRaid5_lb3;
static NFLABEL *gRaid5_lb4;
static NFLABEL *gRaid5_lb5;
static NFLABEL *gRaid5_ins0;
static NFLABEL *gRaid5_ins1;
static NFLABEL *gRaid5_ins2;
static NFOBJECT *g_wbox = NULL;


static gint _get_enable_config_raid5()
{
    DISK_CAPINFO_T *tcap = NULL;
    gint i;

    tcap = get_disk_info(INTERNAL);
    if (!tcap) return 0;
    
    if (g_disk_cnt < ENABLE_RAID5_DISK_CNT) return 0;   //not enough minimum disk cnt

    for (i = 0; i < g_disk_cnt; i++)
    {
        g_message("%s, %d, index : %d, tcap->disk_unit[i].valid : %d", __FUNCTION__, __LINE__, i, tcap->disk_unit[i].valid);
        if (tcap->disk_unit[i].valid == DISK_INVALID)
        {
            g_message("%s, %d, index : %d", __FUNCTION__, __LINE__, i);
            return 0;
        }
    }
    
    return 1;
}

static gint _get_enable_config_raid1()
{
    DISK_CAPINFO_T *tcap;
    gint i;
    
    tcap = get_disk_info(INTERNAL);
    if (!tcap) return 0;
    
    if (g_disk_cnt < ENABLE_RAID1_DISK_CNT) return 0;   //not enough minimum disk cnt

    for (i = 0; i < g_disk_cnt; i++)
    {
        g_message("%s, %d, index : %d, tcap->disk_unit[i].valid : %d", __FUNCTION__, __LINE__, i, tcap->disk_unit[i].valid);
        if (tcap->disk_unit[i].valid == DISK_INVALID)
        {
            g_message("%s, %d, index : %d", __FUNCTION__, __LINE__, i);
            return 0;
        }
    }
    
    return 1;
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

static gint get_external_raidID(gint raid_type, gint disk_index)
{
	gint i, j;

	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;

	for(i=0; i<gRaid_i->raid_cnt; i++) {
		if(raid_type == EASY_CONF_RAID1) {
			if(gRaid_i->rinfo[i].raid_mode != RAID_LEVEL_1)
				continue;
		}
		else {
			if(gRaid_i->rinfo[i].raid_mode != RAID_LEVEL_5)
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

static gint get_sata_port_in_raid(gint raid_id, gint disk_index)
{
	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;
	if(raid_id < 0) return -1;

	if(disk_index >= gRaid_i->rinfo[raid_id].member_count)
		return -1;

	return gRaid_i->rinfo[raid_id].sata_index[disk_index];
}

static gboolean load_raid_info(STRG_TYPE_E strg_type)
{
    if (!g_enable_R1 && !g_enable_R5) return FALSE;
    
	gRaid_i = get_disk_raid_info(strg_type);

	if(!gRaid_i) return FALSE;
	if(gRaid_i->mode == 0)	{
		 memset(&gRaid_conf[strg_type], 0x00, sizeof(DISK_RAIDINFO_T));
		 if(!scm_get_disk_easy_raid_conf(strg_type, &gRaid_conf[strg_type])) {
			 gRaid_i = &gRaid_conf[strg_type];
			 return TRUE;
		 }

		return FALSE;
	}

	return TRUE;
}

static void display_raid_instruction()
{
	gchar *strInst_R1[6] = {"Sorry!!",
                    		"You don't have enough number of disks to create a RAID1.",
                    		"Please insert more disks.", 
                    		"What is RAID1?",
                    		"RAID1 is the replication of data on to seperate hard disks in real time",
                    		"to ensure continuous availibility."};
                    		
    gchar *strInst_R5[6] = {"Sorry!!",
                            "You don't have enough number of disks to create a RAID5.",
                            "Please insert more disks.", 
		                    "What is RAID5?",
		                    "Parity blocks are used to protect data. When in RAID5 mode",
	                        "parity block will be spread all over the different hard drives."};

    gchar *strFail[2] = {   "Disk not properly installed.",
                            "Please install disks sequentially from disk slot 1."};


	if (g_disk_cnt < ENABLE_RAID1_DISK_CNT)
	{
		nfui_nflabel_set_text(gRaid1_ins0, strInst_R1[0]);
        nfui_nflabel_set_text(gRaid1_ins1, strInst_R1[1]);
		nfui_nflabel_set_text(gRaid1_ins2, strInst_R1[2]);
		nfui_signal_emit(gRaid1_ins0, GDK_EXPOSE, FALSE);
		nfui_signal_emit(gRaid1_ins1, GDK_EXPOSE, FALSE);
		nfui_signal_emit(gRaid1_ins2, GDK_EXPOSE, FALSE);
	}
	else if (!g_enable_R1)
	{
		nfui_nflabel_set_text(gRaid1_ins0, strInst_R1[0]);
        nfui_nflabel_set_text(gRaid1_ins1, strFail[0]);
		nfui_nflabel_set_text(gRaid1_ins2, strFail[1]);
		nfui_signal_emit(gRaid1_ins0, GDK_EXPOSE, FALSE);
		nfui_signal_emit(gRaid1_ins1, GDK_EXPOSE, FALSE);
		nfui_signal_emit(gRaid1_ins2, GDK_EXPOSE, FALSE);
	}
	else if(gRaid_i->mode & RAID_1_MODE) {					// raid 1
		nfui_nflabel_set_text(gRaid1_ins0, strInst_R1[3]);
        nfui_nflabel_set_text(gRaid1_ins1, strInst_R1[4]);
		nfui_nflabel_set_text(gRaid1_ins2, strInst_R1[5]);
		nfui_signal_emit(gRaid1_ins0, GDK_EXPOSE, FALSE);
		nfui_signal_emit(gRaid1_ins1, GDK_EXPOSE, FALSE);
		nfui_signal_emit(gRaid1_ins2, GDK_EXPOSE, FALSE);
	}

	if (g_disk_cnt < ENABLE_RAID5_DISK_CNT)
	{
		nfui_nflabel_set_text(gRaid5_ins0, strInst_R5[0]);
        nfui_nflabel_set_text(gRaid5_ins1, strInst_R5[1]);
		nfui_nflabel_set_text(gRaid5_ins2, strInst_R5[2]);
	}
	else if (!g_enable_R5)
	{
		nfui_nflabel_set_text(gRaid5_ins0, strInst_R1[0]);
        nfui_nflabel_set_text(gRaid5_ins1, strFail[0]);
		nfui_nflabel_set_text(gRaid5_ins2, strFail[1]);
	}
	else if(gRaid_i->mode & RAID_5_MODE)	
	{					// raid 5
		nfui_nflabel_set_text(gRaid5_ins0, strInst_R5[3]);
        nfui_nflabel_set_text(gRaid5_ins1, strInst_R5[4]);
		nfui_nflabel_set_text(gRaid5_ins2, strInst_R5[5]);
	}

}

static void display_disk_image()
{
	NFOBJECT *tab_page;
	gint i;
	gint r1_cnt = 0, r5_cnt = 0;
	gint r5_dcnt = 0;

	guint r5_imgPosX[3] = {320, 269 ,237};
	guint r5_lbNumPosX[3] = {462, 466, 488};
	guint r5_lb3PosX[3] = {320, 423, 526};  
	guint r5_lb4PosX[4] = {269, 372, 475, 578};
	guint r5_lb5PosX[5] = {237, 340, 443, 546, 649};

	
    if (!gRaid_i) return;
    
	for(i=0; i<gRaid_i->raid_cnt; i++) {
		if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1) {  	// raid 1
			r1_cnt += 1;
		}
		else if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5) {   // raid 5
			r5_cnt += 1;
			r5_dcnt = gRaid_i->rinfo[i].member_count;
		}
	}


	// raid 1
	if (g_enable_R1)
	{
	if(r1_cnt != 0 && r1_cnt <= 2) {
		nfui_nfimage_change_image(gRaid1_img0, IMG_DISK_RAID1_3);
		nfui_signal_emit((NFOBJECT*)gRaid1_img0, GDK_EXPOSE, FALSE);
		nfui_signal_emit((NFOBJECT*)gRaid1_lb0, GDK_EXPOSE, FALSE);

		if(r1_cnt == 2) {
			nfui_nfimage_change_image(gRaid1_img1, IMG_DISK_RAID1_3);
			nfui_signal_emit((NFOBJECT*)gRaid1_img1, GDK_EXPOSE, FALSE);
			nfui_signal_emit((NFOBJECT*)gRaid1_lb1, GDK_EXPOSE, FALSE);
		}
	}
	
}
	// raid 5
	if(r5_cnt != 0) {
		tab_page = nfui_nftab_get_nth_page(g_subtab, 1);
		nfui_nfobject_hide(gRaid5_lb0);
		nfui_nfobject_hide(gRaid5_lb1);
		nfui_nfobject_hide(gRaid5_lb2);
		nfui_nfobject_hide(gRaid5_lb3);
		nfui_nfobject_hide(gRaid5_lb4);
		nfui_nfobject_hide(gRaid5_lb5);

		if(r5_dcnt == 3) {
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_img0, r5_imgPosX[0], 451);
			nfui_nfimage_change_image(gRaid5_img0, IMG_DISK_RAID5_3);
			nfui_signal_emit((NFOBJECT*)gRaid5_img0, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb0);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb0, r5_lbNumPosX[0], 454);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb0, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb1);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb1, r5_lb3PosX[0], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb1, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb2);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb2, r5_lb3PosX[1], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb2, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb3);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb3, r5_lb3PosX[2], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb3, GDK_EXPOSE, FALSE);
		}
		else if(r5_dcnt == 4) {
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_img0, r5_imgPosX[1], 451);
			nfui_nfimage_change_image(gRaid5_img0, IMG_DISK_RAID5_4);
			nfui_signal_emit((NFOBJECT*)gRaid5_img0, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb0);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb0, r5_lbNumPosX[1], 454);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb0, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb1);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb1, r5_lb4PosX[0], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb1, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb2);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb2, r5_lb4PosX[1], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb2, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb3);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb3, r5_lb4PosX[2], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb3, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb4);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb4, r5_lb4PosX[3], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb4, GDK_EXPOSE, FALSE);
		}
		else if(r5_dcnt == 5) {
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_img0, r5_imgPosX[2], 451);
			nfui_nfimage_change_image(gRaid5_img0, IMG_DISK_RAID5_5);
			nfui_signal_emit((NFOBJECT*)gRaid5_img0, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb0);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb0, r5_lbNumPosX[2], 454);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb0, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb1);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb1, r5_lb5PosX[0], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb1, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb2);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb2, r5_lb5PosX[1], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb2, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb3);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb3, r5_lb5PosX[2], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb3, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb4);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb4, r5_lb5PosX[3], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb4, GDK_EXPOSE, FALSE);

			nfui_nfobject_show(gRaid5_lb5);
			nfui_nffixed_move((NFFIXED*)tab_page, (NFOBJECT*)gRaid5_lb5, r5_lb5PosX[4], 597);
			nfui_signal_emit((NFOBJECT*)gRaid5_lb5, GDK_EXPOSE, FALSE);
		}
	}
	else {
		nfui_nfobject_show(gRaid5_lb0);
		nfui_signal_emit((NFOBJECT*)gRaid5_lb0, GDK_EXPOSE, FALSE);

		nfui_nfobject_show(gRaid5_lb1);
		nfui_signal_emit((NFOBJECT*)gRaid5_lb1, GDK_EXPOSE, FALSE);

		nfui_nfobject_show(gRaid5_lb2);
		nfui_signal_emit((NFOBJECT*)gRaid5_lb2, GDK_EXPOSE, FALSE);

		nfui_nfobject_show(gRaid5_lb3);
		nfui_signal_emit((NFOBJECT*)gRaid5_lb3, GDK_EXPOSE, FALSE);

		nfui_nfobject_show(gRaid5_lb4);
		nfui_signal_emit((NFOBJECT*)gRaid5_lb4, GDK_EXPOSE, FALSE);

		nfui_nfobject_show(gRaid5_lb5);
		nfui_signal_emit((NFOBJECT*)gRaid5_lb5, GDK_EXPOSE, FALSE);
	}

}

static void display_disk_model_name()
{
	NFOBJECT *obj = NULL;
	gint i, j;
	gchar *ppos;
	gint disk_port;

    if (!gRaid_i) return;

	for(i=0; i<gRaid_i->raid_cnt; i++) {
		if(gRaid_i->rinfo[i].member_count == 0)
			continue;

		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) {


			if(strlen(gRaid_i->rinfo[i].model[j])) {

				disk_port = get_sata_port_in_raid(i, j);

				if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1)  	// raid 1
					obj = nfui_nftable_get_child(gRaid1_tbl, 1, disk_port+1);
				else if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5)   // raid 5
					obj = nfui_nftable_get_child(gRaid5_tbl, 1, disk_port+1);

				if(!obj) continue;

				ppos = strchr(gRaid_i->rinfo[i].model[j], ' '); 
				if(ppos) sprintf(ppos, "%c", '\0');

				nfui_nflabel_set_text((NFLABEL*)obj, gRaid_i->rinfo[i].model[j]);
			}
		}
	}
}

static void display_disk_capacity()
{
	NFOBJECT *obj = NULL;
	gint i, j;
	gint disk_port;
	gint d_index;
	gint unused_idx;
	gchar buf[64] = {0,};

    if (!gRaid_i) return;
    
	for(i=0; i<gRaid_i->raid_cnt; i++) {
		if(gRaid_i->rinfo[i].member_count == 0)
			continue;

		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) 
		{
			disk_port = get_sata_port_in_raid(i, j);

			if(disk_port < 0)
				return FALSE;
			
			if(i != -1)			
			{
				if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_1)  	// raid 1
					obj = nfui_nftable_get_child(gRaid1_tbl, 2, disk_port+1);
				else if(gRaid_i->rinfo[i].raid_mode == RAID_LEVEL_5)   // raid 5
					obj = nfui_nftable_get_child(gRaid5_tbl, 2, disk_port+1);

				d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk order"));
				unused_idx = get_disk_index_in_raid(i, d_index);
				
				g_message("%s(%d) : raid_id[%d] d_index[%d] unused_idx[%d]  unused_val[%d]", __FUNCTION__, __LINE__,
							i, d_index, unused_idx, gRaid_i->rinfo[i].unused[unused_idx]);
#if 0				
				if(gRaid_i->rinfo[i].unused[unused_idx] != 0) 
					sprintf(buf, "<# %d> USED (%dGB) / UNUSED (%dGB)", i, gRaid_i->rinfo[i].capacity, gRaid_i->rinfo[i].unused[unused_idx]);
				else			 
#endif					
					sprintf(buf, "<# %d> USED (%dGB)", i, gRaid_i->rinfo[i].capacity);
								
				if(i == 0) 
				{
					nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1001));
				}
				else if(i == 1)
				{
					nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1002));
				}
				else if(i == 2)
				{
					nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1003));
				}
				else if(i == 3)
				{
					nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1004));
				}
				else if(i == 4)
				{
					nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1005));
				}
			}
			else
				strcpy(buf, "-");

			nfui_nflabel_set_text((NFLABEL*)obj, buf);
		}
	}
}

static void display_raid_page()
{
	display_raid_instruction(); // instruction
	display_disk_image(); // disk image
	display_disk_model_name(); // model name
	display_disk_capacity(); //capacity

	nfui_signal_emit((NFOBJECT*)gRaid1_tbl, GDK_EXPOSE, TRUE);
	
	if (!gRaid_i) return;
	// set ok button state
	if(gRaid_i->mode & RAID_CONF_MODE) {
		if(gRaid_i->raid_cnt) {
			nfui_nfobject_enable(g_okBtn);
			nfui_signal_emit(g_okBtn, GDK_EXPOSE, FALSE);
		}
	}
	else {
		if(gRaid_i->raid_cnt) {
			nfui_nfobject_disable(g_okBtn);
			nfui_signal_emit(g_okBtn, GDK_EXPOSE, FALSE);
		}
	}

}

static gboolean display_internal_raid_info(gpointer data)
{
	NFOBJECT *wbox = (NFOBJECT*)data;

	NFUTIL_THREADS_ENTER();
	
	load_raid_info(INTERNAL); 
	
		display_raid_page();

	nftool_remove_waitbox(wbox);

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean display_external_raid_info(gpointer data)
{
	NFOBJECT *wbox = (NFOBJECT*)data;

	NFUTIL_THREADS_ENTER();
	
	load_raid_info(EXTERNAL);
	
		display_raid_page();

	nftool_remove_waitbox(wbox);

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static void	init_raid1_tabpage(NFOBJECT *page)
{
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GdkPixbuf *pbIcon[6];

	gchar *strRow[] = {"DISK", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strCol[] = {"MODEL", "CAPACITY"};

	guint table_w[] = {198, 216, 478};
	gint i;


	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 


	tbl = (NFOBJECT*)nfui_nftable_new(3, 6, 2, 1, table_w, 40);	
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)page, tbl, 24, 37);
	gRaid1_tbl = (NFTABLE*)tbl;

	// row title
	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}
	
	// col title
	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, (i + 1), 0);
	}

	// model
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i + 1);
	}
	
	// capacity
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk index", GINT_TO_POINTER(i));

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}


	// instruction
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("<INSTRUCTION>", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1017));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 24, 312);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(1018));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 40, 350);
	gRaid1_ins0 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(1018));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 40, 375);
	gRaid1_ins1 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(1018));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 40, 400);
	gRaid1_ins2 = (NFLABEL*)obj;


	// disk status
	obj = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID1_1);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 237, 451);
	gRaid1_img0 = (NFIMAGE*)obj;

	obj = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID1_1);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 498, 451);
	gRaid1_img1 = (NFIMAGE*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("#0", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1020));
	nfui_nfobject_set_size(obj, 22, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 331, 454);
	gRaid1_lb0 = (NFLABEL*)obj;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("#1", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1020));
	nfui_nfobject_set_size(obj, 22, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 592, 454);
	gRaid1_lb1 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK1", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 237, 597);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK2", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 340, 597);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK3", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 498, 597);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK4", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 602, 597);

}

static void	init_raid5_tabpage(NFOBJECT *page)
{
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GdkPixbuf *pbIcon[6];

	gchar *strRow[] = {"DISK", "DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strCol[] = {"MODEL", "CAPACITY"};

	guint table_w[] = {198, 216, 478};
	gint i;


	pbIcon[0] = nfui_get_image_from_file(IMG_HEAD_STORAGE_ICON, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[5] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 


	tbl = (NFOBJECT*)nfui_nftable_new(3, 6, 2, 1, table_w, 40);	
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)page, tbl, 24, 37);
	gRaid5_tbl = (NFTABLE*)tbl;

	// row title
	for(i=0; i<6; i++) {
		obj = (NFOBJECT*)nfui_nfimglabel_new(pbIcon[i], strRow[i]);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}
	
	// col title
	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, (i + 1), 0);
	}

	// model
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i + 1);
	}
	
	// capacity
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk index", GINT_TO_POINTER(i));

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}


	// instruction
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("<INSTRUCTION>", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1017));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 896, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 24, 312);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(1018));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 40, 350);
	gRaid5_ins0 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(1018));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 40, 375);
	gRaid5_ins1 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(1018));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 880, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 40, 400);
	gRaid5_ins2 = (NFLABEL*)obj;


	// disk status
	obj = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID5_EMPTY);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 237, 451);
	gRaid5_img0 = (NFIMAGE*)obj;


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("#0", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1020));
	nfui_nfobject_set_size(obj, 22, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 488, 454);
	gRaid5_lb0 = (NFLABEL*)obj;
	

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK1", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 237, 597);
	gRaid5_lb1 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK2", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 340, 597);
	gRaid5_lb2 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK3", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 443, 597);
	gRaid5_lb3 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK4", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 546, 597);
	gRaid5_lb4 = (NFLABEL*)obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK5", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
	nfui_nfobject_set_size(obj, 104, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 649, 597);
	gRaid5_lb5 = (NFLABEL*)obj;


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

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

    	if(cur_page == new_page) return FALSE;

		switch(cur_page) {
			case 0:	// raid 5 page
    		    if(g_enable_R5)
    		    {
    		        nfui_nfobject_enable(g_okBtn);
    		        nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);
		        }
		        else
    		    {
    		        nfui_nfobject_disable(g_okBtn);
    		        nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);
		        }
				break;

			case 1:	// raid 1 page
			    if(g_enable_R1)
    		    {
		            nfui_nfobject_enable(g_okBtn);
    		        nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);
		        }
		        else
		        {
    		        nfui_nfobject_disable(g_okBtn);
		            nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);
	            }
				break;

			default:
				break;
		}
	}
	return FALSE;
}

static gboolean post_disk_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
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
	else if(evt->type == IRET_SCM_DISK_CREATE_RAID)
	{
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
		
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}
	else if(evt->type == GDK_DELETE) {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		uxm_unreg_imsg_event(obj, IRET_SCM_DISK_CREATE_RAID);
        
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

		ret = nftool_mbox(g_curwnd, "WARNING", "All recording data will be deleted after this configuration.\nPlease make sure if you want to do this!!", NFTOOL_MB_OKCANCEL);

		if (ret == NFTOOL_MB_OK) {
			cur_page = nfui_nftab_get_cur_page(g_subtab);

			if(cur_page == 0) g_rConfMode = RAID1_CONF;
			else if(cur_page == 1) g_rConfMode = RAID5_CONF;

			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
		}
	}

	return FALSE;
}

static gboolean post_close_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

gint VW_DiskRaid_EasyConf_Open(NFWINDOW *parent, STRG_TYPE_E strg_type)
{
	NFOBJECT *window = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *tab = NULL;
	NFOBJECT *tabPage[2];
	NFOBJECT *obj = NULL;
	NFOBJECT *wbox = NULL;

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

	g_disk_cnt = get_disk_count(INTERNAL_DISK_GRP);
	g_enable_R1 = _get_enable_config_raid1();
	g_enable_R5 = _get_enable_config_raid5();


	/* window */
	window = (NFOBJECT*)nfui_nfwindow_new(parent, POP_POS_X, POP_POS_Y, WND_SIZE_W, WND_SIZE_H);
	g_curwnd = (NFWINDOW*)window;
	//nfui_regi_post_event_callback(window, post_easy_conf_window_event_cb);
	//nfui_nfwindow_use_outside_evt((NFWINDOW*)window, TRUE);
	//nfui_nfwindow_set_mask((NFWINDOW*)window, GDK_BUTTON_PRESS, TRUE);
	//nfui_nfwindow_set_returnkey_proc((NFWINDOW*)window, returnkey_proc);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WND_SIZE_W, WND_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_disk_fixed_event_cb);
	nfui_nfobject_show(fixed);



	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RAID CONFIGURATION (CREATE RAID)", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 944, 36);
	//nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 9, 4);



	/* tab */
	tab = (NFOBJECT*)nfui_nftab_new(2, strImage_h, 208, 43, NFTAB_DIR_H, strTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)tab, 10);
	nfui_regi_pre_event_callback(tab, pre_subtab_event_handler);
	nfui_nfobject_show(tab);
	nfui_nffixed_put((NFFIXED*)fixed, tab, 9, 50);
	g_subtab = (NFTAB*)tab;

	for(i=0; i<2; i++) {
		tabPage[i] = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(tabPage[i], 944, 652);
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
	nfui_nffixed_put((NFFIXED*)fixed, obj, 286, 760);
	if(!g_enable_R1)
        nfui_nfobject_disable(obj);
	g_okBtn = obj;

	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_regi_post_event_callback(obj, post_close_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 484, 760);

	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	uxm_reg_imsg_event(fixed, IRET_SCM_DISK_CREATE_RAID);

	// load raid info
	wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	if(strg_type == INTERNAL) g_timeout_add(300, display_internal_raid_info, (gpointer)wbox);
	else g_timeout_add(300, display_external_raid_info, (gpointer)wbox);


	nfui_page_open(PGID_DISK_RAID_EASY_CONF, window, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DISK_RAID_EASY_CONF, window);

	return g_rConfMode;
}
