#include <string.h>

#include "nf_afx.h"

#include "iux_afx.h"
#include "iux_msg.h"
#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "services/scm.h"
#include "services/uxm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfscrolledfixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfimage.h"

#include "viewers/vw_disk_raid_warn.h"

#include "vw_boot_info.h"
#include "vw_init_password.h"
#include "evt.h"
#include "vw.h"
#include "scm.h"
#include "nf_sysman.h"



//FIXME choissi
#include "nf_api_ipcam.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"VW_BOOTINFO"

#define BOOT_WIN_POS_X							((DISPLAY_ACTIVE_WIDTH - BOOT_WIN_SIZE_W) / 2)
#define BOOT_WIN_POS_Y							((DISPLAY_ACTIVE_HEIGHT - BOOT_WIN_SIZE_H) / 2)
#define BOOT_WIN_SIZE_W							(1512)
#define BOOT_WIN_SIZE_H							(1048)

#define BOOT_WINDOW								(g_bootWin)

#define PROG_POS_X								((DISPLAY_ACTIVE_WIDTH - 653) / 2)
#define PROG_POS_Y								900	// skshin, ((DISPLAY_ACTIVE_HEIGHT - 106) / 2)


#define BOOT_INTDISK_CNT		5
#define BOOT_EXTDISK_CNT		8


enum {
	BOOT_CAM_ICON = 0,
	BOOT_INT_DISK_ICON,
	BOOT_ODD_ICON,
	BOOT_EXT_DISK_ICON,
	BOOT_ICON_CNT
};


enum {
	DISP_IPCAM_CONNECTED_TEXT = 0,
	DISP_INT_DISK_TEXT,
	DISP_EXT_DISK_TEXT,
	DISP_ODD_TEXT,
	DISP_FILE_SYS_TEXT,
	// add..
	//
	DISP_TEXT_CNT
};

enum {
	INTERNAL_DISK_INFO,
	EXTERNAL_DISK_INFO,
	DISK_INFO_CNT
};

enum {
	PASSWORD_WIZARD = 1,
	NETWORK_WIZARD,
    All_WIZARD
};

static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_bootWin = NULL;
static NFOBJECT *ntb[4] = { 0, };
static NFOBJECT *bootFixed = NULL;
static NFOBJECT *g_wait_popup = NULL;

static guint g_ch_mask = 0;
static int g_cnt = 0 ;
static int g_web_signal = 0 ;



static gint _get_sata_port_in_raid(gint raid_id, gint disk_index, DISK_RAIDINFO_T *rai)
{
	if(!rai) return -1;
	if(rai->mode == 0)	return -1;
	if(raid_id < 0) return -1;

	return rai->rinfo[raid_id].sata_index[disk_index];
}

static guchar _get_raid_status(gpointer raidInfo)
{
    DISK_RAIDINFO_T *rai = (DISK_RAIDINFO_T*) raidInfo;
    
    if(!raidInfo) return -1;
    if(rai->mode == 0)	return -1;
    
    return rai->rinfo[0].status;
}

static gchar *getStr_raid_status(gpointer data)
{
	guchar raid_status;

    raid_status = _get_raid_status(data);

    if(raid_status != -1)
	{
        switch(raid_status) {
    		case 0: // broken
    		    return "BROKEN";

    		case 1: // degraded
    		    return "DEGRADED";

    		case 2: // rebuild
    		    return "REBUILD";

    		case 3: // normal
    		    return "NORMAL";

    		case 4: // ezBackup 
    		    return "SPARE";

    		default:
    		    return " ";
        }
	}

	return 0;
}


static void disp_ip_camera_total_count(guint ch_mask)
{
	static NFOBJECT *label = NULL;
	gint cnt = 0;
	gchar buf[64];
	gint i;

	for (i = 0; i < var_get_ch_count(); i++) {
		if (ch_mask & (1 << i)) cnt++;
	}

	memset(buf, 0x00, sizeof(buf));
	if (cnt > 1) 		 sprintf(buf, lookup_string("%d cameras are connected."), cnt);
	else if (cnt == 1) 	 sprintf(buf, "1 camera is connected.");
	else				 sprintf(buf, "No camera is connected.");

	if (!label) {
		label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(304));
		nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
		nfui_nfobject_set_size(label, 490, 36);
		nfui_nfobject_show(label);

		nfui_nffixed_put((NFFIXED*)bootFixed, label, (guint)1012, (guint)(103 + (g_cnt * 36)));
	}

	nfui_nflabel_set_text((NFLABEL*)label, buf);
	nfui_signal_emit(label, GDK_EXPOSE, FALSE);
}

static void disp_ip_camera_info(NFTABLE *table, CAM_PROFILE_T *cam)
{
	NFOBJECT *label = NULL;
	NFOBJECT *obj = NULL;
	gchar buf[64];
	gchar *strConnect[2] = {"Connected", "Not connected" };
	int ch = var_get_ch_count();
	int row, col;
	int i;

	for (i = 0; i < ch; ++i) {
		col = (i / 8) * 4;
		row = (i % 8);

		if (cam[i].connected) {
			g_ch_mask |= (1 << i);

			g_sprintf(buf, "%02d", i+1);
			//g_sprintf(buf, "%s", cam[i].model.vendor);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col, row);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			// model
			g_sprintf(buf, "%s", cam[i].model.name);
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col + 1, row);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			// connection
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strConnect[0], 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col + 2, row);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			
		}
		else {
			g_ch_mask &= ~(1 << i);

			g_sprintf(buf, "%02d", i+1);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col, row);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			// model
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col + 1, row);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			// connection
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strConnect[1], 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col + 2, row);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

		}
	}

	disp_ip_camera_total_count(g_ch_mask);
	++g_cnt;
}

static void disp_ip_camera_info_HDI(NFTABLE *table, guint connect)
{
	NFOBJECT *label = NULL;
	NFOBJECT *obj = NULL;
	gchar buf[64];
	gchar type[32];	
	gchar *strConnect[3] = {"Connected", "Not connected", "Invalid video"};
	int ch = var_get_ch_count();
	int row, col;
	int i;
	NF_NOTIFY_INFO info;

	scm_get_vloss_event_data(&info);

	g_ch_mask = connect;

	for (i = 0; i < ch; ++i) {
		col = (i / 8) * 4;
		row = (i % 8);

		if (connect & (1 << i)) {
			g_sprintf(buf, "%02d", i+1);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col, row);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			// model
			scm_get_cam_type_HDI(i, type);
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(type, 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col + 1, row);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			// connection
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strConnect[0], 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col + 2, row);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);			
		}
		else {
			g_sprintf(buf, "%02d", i+1);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)table, obj, col, row);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			if (info.d.params[2] & (1 << i))
			{
				// model
				scm_get_cam_type_HDI(i, type);				
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(type, 
						nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_show(obj);
				nfui_nftable_attach((NFTABLE*)table, obj, col + 1, row);
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

				// connection
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strConnect[2], 
						nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_show(obj);
				nfui_nftable_attach((NFTABLE*)table, obj, col + 2, row);
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
			}
			else
			{
				// model
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", 
						nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_show(obj);
				nfui_nftable_attach((NFTABLE*)table, obj, col + 1, row);
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

				// connection
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strConnect[1], 
						nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_show(obj);
				nfui_nftable_attach((NFTABLE*)table, obj, col + 2, row);
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
			}
		}
	}

	disp_ip_camera_total_count(g_ch_mask);
	++g_cnt;

}

static void disp_int_disk_info(NFTABLE *table, gpointer data, gpointer data2)
{
	DISK_CAPINFO_T *cap = (DISK_CAPINFO_T*)data;
	NFOBJECT *obj;
	gchar buf[128];
	gchar *str;
	gint i;
    int ret;

	for(i=0; i<BOOT_INTDISK_CNT; i++) {
		memset(buf, 0x00, sizeof(buf));

		// number
		g_sprintf(buf, "%02d", (i + 1));

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		
		nfui_nftable_attach((NFTABLE*)table, obj, 0, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		if(!cap->disk_unit[i].size)
			continue;

		// model
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(cap->disk_unit[i].model, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)table, obj, 1, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		// size
		ifn_convert_storage_size(buf, cap->disk_unit[i].size);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)table, obj, 2, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		// sys disk 
		if (cap->disk_unit[i].sys_disk) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYS", 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)table, obj, 3, (guint)i);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
		// raid 
//#if defined(SUPPORT_DISK_RAID)
    if(data2)
    {
		str = getStr_raid_status(data2);
    		if(str)
    		{
        		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
        		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        		nfui_nfobject_show(obj);
        		nfui_nftable_attach((NFTABLE*)table, obj, 4, (guint)i);
        		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    		}
    }
//#endif
	}

}

static void disp_odd_info(NFTABLE *table, char *odd_title)
{
	NFOBJECT *obj;
	gchar buf[128];
	gint i = 0;
	MEDIA_TYPE_E mtype;

	if (strlen(odd_title) == 0) return;

		memset(buf, 0x00, sizeof(buf));
		 
		// number 
		g_sprintf(buf, "%d", (i + 1));

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		
		nfui_nftable_attach((NFTABLE*)table, obj, 0, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		// model
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(odd_title, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)table, obj, 1, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
/*
		// size
		g_sprintf(buf, "%ld GB",(glong)0);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)table, obj, 2, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
*/

}

static void disp_ext_storage_info(NFTABLE *table, gpointer data)
{
	DISK_CAPINFO_T *cap = (DISK_CAPINFO_T*)data;
	NFOBJECT *obj;
	gchar buf[16];
	gint i;
	NFOBJECT *label = NULL;

	for(i=0; i<BOOT_EXTDISK_CNT; i++) {
		memset(buf, 0x00, sizeof(buf));

		// number
		g_sprintf(buf, "%02d", (i + 1));

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		
		nfui_nftable_attach((NFTABLE*)table, obj, 0, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		if(!cap->disk_unit[i].size)
			continue;

		// model
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(cap->disk_unit[i].model, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)table, obj, 1, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		// size
		ifn_convert_storage_size(buf, cap->disk_unit[i].size);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)table, obj, 2, (guint)i);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		// sys disk 
		if (cap->disk_unit[i].sys_disk) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYS", 
					nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(303));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			nfui_nftable_attach((NFTABLE*)table, obj, 3, (guint)i);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}

}

static gboolean load_disk_data(STRG_TYPE_E type, DISK_RECINFO_T *rec, DISK_RAIDINFO_T *rai)
{	// get disk time
	if(scm_get_disk_recinfo(type, rec) != 0)
    {
	    return FALSE;
    }

	// get raid info
    if(scm_get_disk_raidinfo(rai) != 0)
    {
   	    return FALSE;
    }

	return TRUE;
}

static int _show_cam_info()
{
	CAM_PROFILE_T	*cam;

	cam = scm_new_cam_profile();
	g_assert(cam);

	disp_ip_camera_info((NFTABLE *)ntb[0], cam);

	scm_free_cam_profile(cam);

	return 0;
}

static int _show_cam_info_HDI()
{
	guint conn_cam;
	gint ch;

	NFOBJECT *model;
	NFOBJECT *status;
	guint row, col;

	conn_cam = scm_get_cam_conn_state();
	disp_ip_camera_info_HDI((NFTABLE *)ntb[0], conn_cam);

	return 0;
}

static int _show_odd_info()
{
	int m_cnt;
	guint width[7] = {50, 225, 150, 45, 50, 175, 200 };
	NFOBJECT *label = NULL;
	gchar strBuf[64];
	gchar *dispText = "ODD: %s";
	MEDIA_TYPE_E mtype;
    gint x ;

	int has_odd = 0;
	char title[128];
	memset(title, 0x00, sizeof(title));
	if (scm_spy_odd_name(title) == 0) has_odd = 1;

	if (has_odd) {
		ntb[2] = (NFOBJECT*)nfui_nftable_new(3, 4, 0, 0, width, 30);
		nfui_nffixed_put((NFFIXED*)bootFixed, ntb[2], 545, 741);
		nfui_nfobject_show(ntb[2]);
		disp_odd_info((NFTABLE*)ntb[2], title);
		
        if (!var_get_supported_ext_disk())
            nfui_nffixed_put((NFFIXED*)bootFixed, ntb[2], 75, 741);
	}

	memset(strBuf, 0x00, sizeof(strBuf));
	if (has_odd) {
		if(strlen(title)) g_sprintf(strBuf, dispText, title);
		else			  g_sprintf(strBuf, dispText, lookup_string("Not installed"));

		label = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(304));
		nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
		nfui_nfobject_set_size(label, 490, 36);
		nfui_nfobject_show(label);

		nfui_nffixed_put((NFFIXED*)bootFixed, label, (guint)1012, (guint)(103 + (g_cnt * 36)));
		nfui_signal_emit(label, GDK_EXPOSE, FALSE);
		++g_cnt;
	}
}

static int _show_internal_disk_info()
{
	DISK_CAPINFO_T cap;
    DISK_RECINFO_T rec;
    DISK_RAIDINFO_T rai;
    
	guint width[8] = {40, 175, 130, 50, 130, 175, 130, 35 };
	NFOBJECT *label = NULL;
	gchar strBuf[64];
	gchar tmp_buf[24];

    memset(&cap, 0x00, sizeof(DISK_CAPINFO_T));
    memset(&rec, 0x00, sizeof(DISK_RECINFO_T));
    memset(&rai, 0x00, sizeof(DISK_RAIDINFO_T));

	if (scm_get_disk_capinfo(INTERNAL, &cap) == 0) {
	    load_disk_data(INTERNAL, &rec, &rai);
		ntb[1] = (NFOBJECT*)nfui_nftable_new(5, 5, 0, 0, width, 30);
		nfui_nffixed_put((NFFIXED*)bootFixed, ntb[1], 75, 508);
		nfui_nfobject_show(ntb[1]);
		disp_int_disk_info((NFTABLE*)ntb[1], (gpointer)&cap, (gpointer)&rai);
	}

	if (cap.tsize != 0) {
		ifn_convert_storage_size(tmp_buf, cap.tsize);
		g_sprintf(strBuf, "%s %s", lookup_string("Size of internal disk:"), tmp_buf);
	}
	else {
		g_sprintf(strBuf, "%s %s", lookup_string("Size of internal disk:"), lookup_string("Not installed"));
	}

	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, 
			nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(304));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
	nfui_nfobject_set_size(label, 490, 36);
	nfui_nfobject_show(label);

	nfui_nffixed_put((NFFIXED*)bootFixed, label, (guint)1012, (guint)(103 + (g_cnt * 36)));
	nfui_signal_emit(label, GDK_EXPOSE, FALSE);
	++g_cnt;

	return 0;
}

static int _show_external_disk_info()
{
	DISK_CAPINFO_T cap;
	guint width[8] = {40, 175, 130, 45, 50, 175, 130, 35 };
	gchar strBuf[64];
	char tmp_buf[24];
	NFOBJECT *label = NULL;

	if(scm_get_disk_capinfo(EXTERNAL, &cap) == 0) {
		ntb[3] = (NFOBJECT*)nfui_nftable_new(4, 8, 0, 0, width, 30);
		nfui_nffixed_put((NFFIXED*)bootFixed, ntb[3], 75, 741);
		nfui_nfobject_show(ntb[3]);

		disp_ext_storage_info((NFTABLE*)ntb[3], (gpointer)&cap);
	}

	if(cap.tsize != 0) {
		ifn_convert_storage_size(tmp_buf, cap.tsize);
		g_sprintf(strBuf, "%s %s", lookup_string("Size of external disk:"), tmp_buf);
	}
	else {
		g_sprintf(strBuf, "%s %s", lookup_string("Size of external disk:"), lookup_string("Not installed"));
	}

	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, 
			nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(304));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
	nfui_nfobject_set_size(label, 490, 36);
	nfui_nfobject_show(label);

	nfui_nffixed_put((NFFIXED*)bootFixed, label, (guint)1012, (guint)(103 + (g_cnt * 36)));
	nfui_signal_emit(label, GDK_EXPOSE, FALSE);
	++g_cnt;
}

static NFLABEL *_make_label()
{
	NFLABEL *label = 0;

	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", 
			nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(304));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
	nfui_nfobject_set_size(label, 490, 36);
	nfui_nfobject_show(label);

	nfui_nffixed_put((NFFIXED*)bootFixed, label, (guint)1012, (guint)(103 + (g_cnt * 36)));
	nfui_signal_emit(label, GDK_EXPOSE, FALSE);
	++g_cnt;

	return label;
}

static gboolean post_bootwin_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT* prog_pop = NULL;
	mb_type ret = -1;
	NFLABEL *label = 0;
	char unc_msg[1024];
	gint key = 0;
	g_web_signal = 0;

	switch(evt->type) {
	case IREQ_BOOT_ERROR:
		vw_mbox_wait(g_curwnd, "WARNING", IMBX_BOOT_DISK_ERROR);
		break;

	case INFY_NODISK_BY_WEB:
		g_web_signal = 1;
		evt_send_to_window("MESSAGE BOX", WND_CLOSE, 0, 0, 0);

		label = _make_label();
		nfui_nflabel_set_text((NFLABEL*)label, "No disk detected.");
		nfui_signal_emit(label, GDK_EXPOSE, FALSE);

		cmm_send_message(CMMPT_SCM, IRPL_BOOT_NODISK, (gint)DISK_NOSYNC, 0, 0);
		break;

	case INFY_FORMAT_BY_WEB:
		g_web_signal = 1;
		evt_send_to_window("MESSAGE BOX", WND_CLOSE, 0, 0, 0);

		if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CONFLICT, DISK_REBOOT, 0, 0);
			break;
		}

		label = _make_label();
		nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
		nfui_signal_emit(label, GDK_EXPOSE, FALSE);
		cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CONFLICT, DISK_SYNC, 0, 0);
		break;

	case IREQ_BOOT_NODISK:
		ret = nftool_mbox(g_curwnd, "WARNING", "No disk detected.\nAre you continue to boot up?",
				NFTOOL_MB_YESNO);

		if (g_web_signal != 1)
		{
			if (ret == NFTOOL_MB_YES) {
				label = _make_label();
				nfui_nflabel_set_text((NFLABEL*)label, "No disk detected.");
				nfui_signal_emit(label, GDK_EXPOSE, FALSE);

				cmm_send_message(CMMPT_SCM, IRPL_BOOT_NODISK, (gint)DISK_NOSYNC, 0, 0);
				cmm_send_message(CMMPT_WEB, IRPL_BOOT_NODISK, (gint)DISK_NOSYNC, 0, 0);
			}
			else {
				nftool_mbox(g_curwnd, "NOTICE", "This system will reboot soon because there is no disk.", NFTOOL_MB_NONE);
			}
		}
		break;

	case IREQ_BOOT_DISK_CONFLICT:
		ret = nftool_mbox(g_curwnd, "WARNING", "This system has no system disk \nor more than one system disk.\nDo you want to format all disk(s)?", 
				NFTOOL_MB_YESNO);
		if (g_web_signal != 1)
		{
			if(ret == NFTOOL_MB_YES) {

				if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
					cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CONFLICT, DISK_REBOOT, 0, 0);
					break;
				}

				label = _make_label();
				nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
				nfui_signal_emit(label, GDK_EXPOSE, FALSE);
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CONFLICT, DISK_SYNC, 0, 0);
				cmm_send_message(CMMPT_WEB, IRPL_BOOT_DISK_CONFLICT, DISK_SYNC, 0, 0);
			}
			else {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CONFLICT, DISK_REBOOT, 0, 0);
				cmm_send_message(CMMPT_WEB, IRPL_BOOT_DISK_CONFLICT, DISK_REBOOT, 0, 0);
			}
		}
		break;

	case IREQ_BOOT_DISK_ADDED:
		ret = nftool_mbox(g_curwnd, "WARNING", "Some disks have been added.\nDo you want to use it?",
				NFTOOL_MB_YESNO);


		if(ret == NFTOOL_MB_YES) {

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_ADDED, DISK_REBOOT, 0, 0);
				break;
			}

			label = _make_label();
			nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
			nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_ADDED, DISK_SYNC, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_ADDED, DISK_REBOOT, 0, 0);
		}

		break;

	case IREQ_BOOT_DISK_REMOVED:
		ret = nftool_mbox(g_curwnd, "WARNING", "Some disks have been removed.\nDo you want to use this system?",
				NFTOOL_MB_YESNO);

		if(ret == NFTOOL_MB_YES) {

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_REMOVED, DISK_REBOOT, 0, 0);
				break;
			}

			label = _make_label();
			nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
			nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_REMOVED, DISK_SYNC, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_REMOVED, DISK_REBOOT, 0, 0);
		}

		break;
		//captainnn 
	case IREQ_BOOT_SYSTEM_DISK_REMOVED:
		ret = nftool_mbox(g_curwnd, "WARNING", "System disk has been removed.\nOne disk can be formatted to System disk. \nDo you want to use this system?",
				NFTOOL_MB_YESNO);

		if(ret == NFTOOL_MB_YES) {

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK")) {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_SYSTEM_DISK_REMOVED, DISK_REBOOT, 0, 0);
				break;
			}

			label = _make_label();
			nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
			nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_SYSTEM_DISK_REMOVED, DISK_SYNC, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_SYSTEM_DISK_REMOVED, DISK_REBOOT, 0, 0);
		}

		break;

	case IREQ_BOOT_DISK_CHANGED:
		ret = nftool_mbox(g_curwnd, "WARNING", "Some disks have been removed and added.\nDo you want to use this system?",
				NFTOOL_MB_YESNO);

		if(ret == NFTOOL_MB_YES) {

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CHANGED, DISK_REBOOT, 0, 0);
				break;
			}

			label = _make_label();
			nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
			nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CHANGED, DISK_SYNC, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_CHANGED, DISK_REBOOT, 0, 0);
		}

		break;

	case IREQ_BOOT_DISK_NEED_FORMAT:
		ret = nftool_mbox(g_curwnd, "WARNING", "All disks need to be formatted.\nDo you want to format all disk(s)?",
				NFTOOL_MB_YESNO);

		if(ret == NFTOOL_MB_YES) {

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_NEED_FORMAT, DISK_REBOOT, 0, 0);
				break;
			}

			label = _make_label();
			nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
			nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_NEED_FORMAT, DISK_SYNC, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_DISK_NEED_FORMAT, DISK_REBOOT, 0, 0);
		}

		break;
	// captainnn recovery 
	case IREQ_BOOT_FORMAT_RCVR:
		DMSG(9, "");
		{
			u8 pdid;
			u8 disk_num = 0xff;
			int i;
			char sen[256];
			DISK_CAPINFO_T cap_i;
			DISK_CAPINFO_T cap_e;
			char serial[32];

			pdid = _scm_get_recovery_err_disk_pdid(NULL);

			if(pdid == 0xff){
				memset(&cap_i,0x00,sizeof(cap_i));
				scm_get_disk_capinfo(INTERNAL, &cap_i); 

				for(i = 0;i <BOOT_INTDISK_CNT; i++){
					printf("size %ld %d \n",cap_i.disk_unit[i].size,i);
					if(!cap_i.disk_unit[i].size)
						continue;
					if( cap_i.disk_unit[i].sys_disk){
						disk_num = i + 1;
						memcpy(serial,cap_i.disk_unit[i].serial,32);
						break;
					}
				}
				sprintf(sen,"Recoverying is failed\nWe recommend Internal disk%d (S/N : %s) remove.\nAnd add new disk.",disk_num,serial);
			}
			else if(pdid < 16){

				memset(&cap_i,0x00,sizeof(cap_i));
				scm_get_disk_capinfo(INTERNAL, &cap_i); 
				
				for(i = 0;i <BOOT_INTDISK_CNT; i++){
					printf("size %ld %d \n",cap_i.disk_unit[i].size,i);
					if(!cap_i.disk_unit[i].size)
						continue;
					if( pdid == i ){
						disk_num = pdid + 1;
						memcpy(serial,cap_i.disk_unit[i].serial,32);
						break;
					}
				}

				sprintf(sen,"Recoverying is failed\nWe recommend Internal disk%d (S/N : %s) remove.\nAnd add new disk.",disk_num,serial);

			}
			else{
				
				memset(&cap_e,0x00,sizeof(cap_e));
				scm_get_disk_capinfo(EXTERNAL, &cap_e); 

				for(i = 0;i <BOOT_EXTDISK_CNT; i++){
					printf("size %ld %d \n",cap_e.disk_unit[i].size,i);
					if(!cap_e.disk_unit[i].size)
						continue;
					if((pdid -16) == i ){
						disk_num = pdid + 1;
						memcpy(serial,cap_e.disk_unit[i].serial,32);
						break;
					}
				}

				sprintf(sen,"Recoverying is failed\nWe recommend External disk%d (S/N : %s) remove.\nAnd add new disk.",disk_num,serial);
			}

			ret = nftool_mbox(g_curwnd, "WARNING", sen,NFTOOL_MB_YESNO);
			if(ret == NFTOOL_MB_YES) {
				_scm_set_err_cnt(NULL,NAND_ERR_RECOVERY_ERROR, 3 -1);

				nftool_mbox_wait(g_curwnd, "NOTICE", "You can power off the system.");

				cmm_send_message(CMMPT_SCM, IRPL_BOOT_FORMAT_RCVR, DISK_NOFORMAT, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_FORMAT_RCVR, DISK_REBOOT, 0, 0);
		}

		break;
		}
		

	case IREQ_BOOT_ENFORCE_FORMAT:
		DMSG(9, "");
		ret = vw_mbox(g_curwnd, "WARNING", IMBX_ENFORCE_FORMAT, NFTOOL_MB_YESNO);

		if(ret == NFTOOL_MB_YES) {

			if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) {
				cmm_send_message(CMMPT_SCM, IRPL_BOOT_ENFORCE_FORMAT, DISK_REBOOT, 0, 0);
				break;
			}

			label = _make_label();
			nfui_nflabel_set_text((NFLABEL*)label, "Formatting...");
			nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_ENFORCE_FORMAT, DISK_FORMAT, 0, 0);
		}
		else {
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_ENFORCE_FORMAT, DISK_REBOOT, 0, 0);
		}

		break;

	case IREQ_BOOT_BROKEN_RAID:
        {
            ret = nftool_mbox(g_curwnd, "WARNING", "RAID corruption. RAID configuration is initialized.\nDo you want to continue?\n(Auto reboot after 30sec.)", NFTOOL_MB_OKCANCEL);
            cmm_send_message(CMMPT_SCM, INFY_USER_ACTION_BROKEN_RAID, 0, 0, 0);
            
            if(ret == NFTOOL_MB_OK)
            {   
                if(VW_UserPwd_Open(g_curwnd, "DELETE RAID", -1))
                {
                    if(VW_DiskRaid_Warn_Open(g_curwnd)) {			
                        cmm_send_message(CMMPT_SCM, IRPL_BOOT_BROKEN_RAID, DISK_DELRAID, 0, 0);
                        return FALSE;
	                }
                }
            }
            cmm_send_message(CMMPT_SCM, IRPL_BOOT_BROKEN_RAID, DISK_REBOOT, 0, 0);
        }
		break;

	case IREQ_BOOT_SMART_ERROR:
		{
			guint result = ((CMM_MESSAGE_T *)data)->param;
			DMSG(9, "");
			if (result == SMART_WRN)
				vw_mbox_auto_ok(g_curwnd, 30, "WARNING", IMBX_SMART_WARNING);
			else 
				vw_mbox_auto_ok(g_curwnd, 30, "WARNING", IMBX_SMART_ERROR);
			cmm_send_message(CMMPT_SCM, IRPL_BOOT_SMART_ERROR, DISK_OK, 0, 0);
		}

		break;

	case IREQ_CHANGE_ENHANCED_PASSWORD:
		{
			gboolean ret;

			ret = PasswordInit_Open(g_curwnd);
			
			if (ret) {
				cmm_send_message(CMMPT_SCM, IRPL_CHANGE_ENHANCED_PASSWORD, 0, 0, 0);
            }
			else {
				scm_reboot_system(RR_INV_PASSWD, 0);
            }
		}
		break;

	case IREQ_WIZARD_INIT:
		{
			// PASSWORD CHANGE
			guint rate = ((CMM_MESSAGE_T *)data)->param;
			gboolean ret;

			switch(rate)
			{
			case PASSWORD_WIZARD:
	 			ret = vw_init_userinfo_open(g_curwnd);
			break;
			case NETWORK_WIZARD:
				vw_wizard_init(NF_TOPWND, 15);
				ret = 1;
			break;
			case All_WIZARD:
				ret = vw_init_userinfo_open(g_curwnd);
				vw_wizard_init(NF_TOPWND, 15);
			break;
				
			default :
 				break;
			}

			if(ret)
				cmm_send_message(CMMPT_SCM, IRPL_WIZARD_INIT, 0, 0, 0);
			else
				scm_reboot_system(RR_INV_PASSWD, 0);
		}

		break;

	case INFY_RECOVERY_EXPIRED:
		vw_mbox_wait(g_curwnd, "WARNING", IMBX_RECOVERY_TIMEOUT);
		break;

	case INFY_FORMAT_RATE:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->param;

			DMSG(1, "format rate = [%d]\n", ((CMM_MESSAGE_T *)data)->param);

			if(!prog_pop) 
				prog_pop = nftool_prog_pop_open(g_curwnd, "FORMAT", TRUE, 
						(guint)PROG_POS_X, (guint)PROG_POS_Y, 0);

			nftool_prog_pop_set_rate(prog_pop, rate);
		}
		break;

	case INFY_FORMAT_CMPL:
		{
			if(prog_pop) {
				nftool_prog_pop_set_rate(prog_pop, 100);

				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;

				label = _make_label();
				nfui_nflabel_set_text((NFLABEL*)label, "Format completed.");
				nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			}

		}
		break;

	case INFY_BROKEN_RAID_START:
        {
            if (!g_wait_popup) 
                g_wait_popup = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
        }
        break;

	case INFY_BROKEN_RAID_CMPL:
        {
            if (g_wait_popup) 
            {
                nftool_remove_waitbox(g_wait_popup);
                g_wait_popup = 0;
            }
            
            nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_OK);
            scm_reboot_system(RR_NA, 1000);
        }
        break;

	case INFY_RTL_SET_RATE:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->param;

			DMSG(1, "rtl rate = [%d]\n", ((CMM_MESSAGE_T *)data)->param);

			if(!prog_pop) 
				prog_pop = nftool_prog_pop_open(g_curwnd, "RECORD TIME LIMIT", TRUE, 
						(guint)PROG_POS_X, (guint)PROG_POS_Y, 0);

			nftool_prog_pop_set_rate(prog_pop, rate);
		}
		break;

	case INFY_RTL_SET_CMPL:
		{
			if(prog_pop) {
				nftool_prog_pop_set_rate(prog_pop, 100);

				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;

				label = _make_label();
				nfui_nflabel_set_text((NFLABEL*)label, "RTL applied.");
				nfui_signal_emit(label, GDK_EXPOSE, FALSE);
			}

		}
		break;

	case INFY_RECOVERY_RATE:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->param;

			DMSG(1, "recovery rate = [%d]\n", ((CMM_MESSAGE_T *)data)->param);

            if(!prog_pop) {
                label = _make_label();
#if defined(_ZICOM_STRING_FIX)
                nfui_nflabel_set_text((NFLABEL*)label, "Checking System...");
#else
                nfui_nflabel_set_text((NFLABEL*)label, "Recovering...");
#endif
                nfui_signal_emit(label, GDK_EXPOSE, FALSE);

#if defined(_ZICOM_STRING_FIX)
                prog_pop = nftool_prog_pop_open(g_curwnd, "Checking System...", FALSE,
                        (guint)PROG_POS_X, (guint)PROG_POS_Y, 0);
#else            
				prog_pop = nftool_prog_pop_open(g_curwnd, "RECOVERY", FALSE,
                        (guint)PROG_POS_X, (guint)PROG_POS_Y, 0);
#endif
			}

			nftool_prog_pop_set_rate(prog_pop, rate);
		}
		break;

	case INFY_RECOVERY_CMPL:
		{
			if(prog_pop) {
				nftool_prog_pop_set_rate(prog_pop, 100);

				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;

                label = _make_label();
#if defined(_ZICOM_STRING_FIX)
				nfui_nflabel_set_text((NFLABEL*)label, "System checking completed.");
#else
				nfui_nflabel_set_text((NFLABEL*)label, "Recovery completed.");
#endif
                nfui_signal_emit(label, GDK_EXPOSE, FALSE);
            }
        }
        break;

	case INFY_BOOT_FORMAT_ERROR:
		{
			DMSG(1, "format error value = [%d]\n", ((CMM_MESSAGE_T *)data)->param);

			if(prog_pop) {
				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;
			}

			nftool_mbox(g_curwnd, "ERROR", "Format Error.", NFTOOL_MB_OK);
		}
		break;

	case INFY_BOOT_RECOVERY_ERROR:
		{
			DMSG(1, "recovery error value = [%d]\n", ((CMM_MESSAGE_T *)data)->param);

			if(prog_pop) {
				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;
			}

			nftool_mbox(g_curwnd, "ERROR", "Recovery Error.", NFTOOL_MB_OK);
		}
		break;

	case INFY_DISP_IP_CAMERA_INFO:
		DMSG(1, "received request for ip camera info\n");
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
		_show_cam_info_HDI();
#else
		_show_cam_info();
#endif		
		break;

	case INFY_DISP_INT_DISK_INFO:
		DMSG(1, "received request for internal disk info\n");
		_show_internal_disk_info();
		break;

	case INFY_DISP_ODD_INFO:
		DMSG(1, "received request for ODD info\n");
		_show_odd_info();
		break;

	case INFY_DISP_EXT_STORAGE_INFO:
		DMSG(1, "received request for extg info\n");
		_show_external_disk_info();
		break;

	case INFY_CFRM_UNC_ERROR_DETECTED:
		DMSG(1, "UNC ERROR DETECTED\n");
		nftool_mbox(g_curwnd, "ERROR", "System detected a bad sector on the disk,\nThe system will reboot.",
				NFTOOL_MB_NONE);
		break;

	case INFY_PND_NOTIFY:
		{
			NF_NOTIFY_INFO* info = NULL;
			NFOBJECT *model;
			NFOBJECT *status;
			guint row, col;

			if(ntb[0] == NULL) break;

			info = (NF_NOTIFY_INFO*)data;
			if(info == NULL) break;

			col = (info->d.params[1] / 8) * 4;
			row = (info->d.params[1] % 8);

			model = nfui_nftable_get_child((NFTABLE*)ntb[0], col+1, row);
			status = nfui_nftable_get_child((NFTABLE*)ntb[0], col+2, row);

			if (info->d.params[0] == PND_TYPE_VIDEO_START)
			{
				CAM_PROFILE_T prof;
				
				scm_get_cam_profile(info->d.params[1], &prof);	
			
				nfui_nflabel_set_text(model, prof.model.name);
				nfui_nflabel_set_text(status, "Connected");
				nfui_signal_emit(model, GDK_EXPOSE, FALSE);
				nfui_signal_emit(status, GDK_EXPOSE, FALSE);
				
				g_ch_mask |= (1 << info->d.params[1]);
				disp_ip_camera_total_count(g_ch_mask);

			}
			else if ((info->d.params[0] == PND_TYPE_UNPLUGGED)
					|| (info->d.params[0] == PND_TYPE_UNKNOWN)
					|| (info->d.params[0] == PND_TYPE_UNSUPPORTED)) 
			{
				nfui_nflabel_set_text(model, "");
				nfui_nflabel_set_text(status, "Not connected");
				nfui_signal_emit(model, GDK_EXPOSE, FALSE);
				nfui_signal_emit(status, GDK_EXPOSE, FALSE);

				g_ch_mask &= ~(1 << info->d.params[1]);
				disp_ip_camera_total_count(g_ch_mask);
			}
		}
		break;

	case INFY_VLOSS_NOTIFY:
		{
			NF_NOTIFY_INFO* info = NULL;
			NFOBJECT *model;
			NFOBJECT *status;
			guint row, col;
			gint ch;
			gchar type[32];

			if(ntb[0] == NULL) break;

			info = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
			if(info == NULL) break;

			for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
			{
				col = (ch / 8) * 4;
				row = (ch % 8);

				model = nfui_nftable_get_child((NFTABLE*)ntb[0], col+1, row);
				status = nfui_nftable_get_child((NFTABLE*)ntb[0], col+2, row);

				if (info->d.params[2] & (1 << ch))
				{
					scm_get_cam_type_HDI(ch, type);
					nfui_nflabel_set_text(model, type);					
					nfui_nflabel_set_text(status, "Invalid video");
					nfui_signal_emit(model, GDK_EXPOSE, FALSE);					
					nfui_signal_emit(status, GDK_EXPOSE, FALSE);
				}			
				else if (info->d.params[0] & (1 << ch))
				{
					nfui_nflabel_set_text(model, "");
					nfui_nflabel_set_text(status, "Not connected");
					nfui_signal_emit(model, GDK_EXPOSE, FALSE);					
					nfui_signal_emit(status, GDK_EXPOSE, FALSE);
				}
				else
				{
					scm_get_cam_type_HDI(ch, type);
					nfui_nflabel_set_text(model, type);					
					nfui_nflabel_set_text(status, "Connected");
					nfui_signal_emit(model, GDK_EXPOSE, FALSE);					
					nfui_signal_emit(status, GDK_EXPOSE, FALSE);
				}
			}

			g_ch_mask = info->d.params[0];
			disp_ip_camera_total_count(g_ch_mask);			
		}
		break;
		
	case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, INFY_DISP_IP_CAMERA_INFO);
			uxm_unreg_imsg_event(obj, INFY_DISP_INT_DISK_INFO);
			uxm_unreg_imsg_event(obj, INFY_DISP_ODD_INFO);
            if (!var_get_supported_ext_disk())
    			uxm_unreg_imsg_event(obj, INFY_DISP_EXT_STORAGE_INFO);
			uxm_unreg_imsg_event(obj, INFY_FORMAT_RATE);
			uxm_unreg_imsg_event(obj, INFY_FORMAT_CMPL);
			uxm_unreg_imsg_event(obj, INFY_RTL_SET_RATE);
			uxm_unreg_imsg_event(obj, INFY_RTL_SET_CMPL);
			uxm_unreg_imsg_event(obj, INFY_RECOVERY_RATE);
			uxm_unreg_imsg_event(obj, INFY_RECOVERY_CMPL);
			uxm_unreg_imsg_event(obj, INFY_BOOT_FORMAT_ERROR);
			uxm_unreg_imsg_event(obj, INFY_BOOT_RECOVERY_ERROR);
			uxm_unreg_imsg_event(obj, INFY_CFRM_UNC_ERROR_DETECTED);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_NODISK);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_DISK_CONFLICT);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_DISK_ADDED);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_DISK_REMOVED);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_DISK_CHANGED);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_DISK_NEED_FORMAT);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_SYSTEM_DISK_REMOVED);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_BROKEN_RAID);			
			uxm_unreg_imsg_event(obj, IREQ_BOOT_FORMAT_RCVR);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_ENFORCE_FORMAT);
			uxm_unreg_imsg_event(obj, IREQ_BOOT_SMART_ERROR);
			uxm_unreg_imsg_event(obj, IREQ_CHANGE_ENHANCED_PASSWORD);
			uxm_unreg_imsg_event(obj, IREQ_WIZARD_INIT);
			uxm_unreg_imsg_event(obj, INFY_RECOVERY_EXPIRED);
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
			uxm_unreg_imsg_event(obj, INFY_VLOSS_NOTIFY);
#else
			uxm_unreg_imsg_event(obj, INFY_PND_NOTIFY);			
#endif
			uxm_unreg_imsg_event(obj, INFY_BROKEN_RAID_START);
			uxm_unreg_imsg_event(obj, INFY_BROKEN_RAID_CMPL);			
			uxm_unreg_imsg_event(obj, INFY_NODISK_BY_WEB);
			uxm_unreg_imsg_event(obj, INFY_FORMAT_BY_WEB);
		}
		break;

	default:
			break;
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
			{
				GdkDrawable *drawable = NULL;
				GdkGC *gc;

				drawable = nfui_nfobject_get_window(obj);
				gc = nfui_nfobject_get_gc(obj);

				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(301));

				gdk_draw_rectangle(drawable,
						gc,
						TRUE,
						60, 440,
						900, 2);

				gdk_draw_rectangle(drawable,
						gc,
						TRUE,
						988, 50,
						2, 948);

				nfui_nfobject_gc_unref(gc);
			}
			break;

		default:
			break;
	}
	
	return FALSE;
}


void VW_BootInfo(NFWINDOW *parent, IMSG ret_msg)
{
	NFOBJECT *scrolled_fixed;
	NFOBJECT *icon = NULL;
	NFOBJECT *label = NULL;

	gint m_cnt = 0;

#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	gchar *strTitle[BOOT_ICON_CNT] = {"CAMERA", "INTERNAL DISK", "ODD", "EXTERNAL DISK"};
#else
	gchar *strTitle[BOOT_ICON_CNT] = {"IP CAMERA", "INTERNAL DISK", "ODD", "EXTERNAL DISK"};
#endif	
	gint i;
    gint x = 0;

	gchar strBuf[64];
	gchar tmp_buf[24];
	
	guint width[15] = {50, 175, 200, 45, 50, 175, 200, 45, 50, 175, 200, 45, 50, 175, 200};

	// window 
	g_bootWin = (NFOBJECT*)nfui_nfwindow_new(parent, 
			(guint)BOOT_WIN_POS_X, (guint)BOOT_WIN_POS_Y, (guint)BOOT_WIN_SIZE_W, (guint)BOOT_WIN_SIZE_H);
	g_curwnd = g_bootWin;
	nfui_nfobject_modify_bg(g_bootWin, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
	//nfui_regi_post_event_callback(g_bootWin, post_boot_win_event_cb);
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_bootWin)->main_widget), FALSE);


	// fixed
	bootFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_post_event_callback(bootFixed, post_fixed_event_cb);
	nfui_nfobject_set_size(bootFixed, BOOT_WIN_SIZE_W, BOOT_WIN_SIZE_H);
	nfui_nfobject_show(bootFixed);

    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_AUTOMATIC, NFSCROLLED_POLICY_NEVER);
    nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_POPUP_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, 320/3, 80, 320/5);
    nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_8594A6)));
    nfui_nfobject_set_size(scrolled_fixed, 886, 320);
    nfui_nfobject_show(scrolled_fixed);
    nfui_nffixed_put((NFFIXED*)bootFixed, scrolled_fixed, 65, 103);

	ntb[0] = (NFOBJECT*)nfui_nftable_new(15, 8, 0, 0, width, 30);
	nfui_nfobject_show(ntb[0]);
	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, ntb[0], 10, 10);

	// icon 
	for(i=0; i<BOOT_ICON_CNT; i++) {
		if(i == BOOT_CAM_ICON) 		icon = (NFOBJECT*)nfui_nfimage_new((IMG_BOOT_CAM_ICON));
		if(i == BOOT_INT_DISK_ICON) icon = (NFOBJECT*)nfui_nfimage_new((IMG_BOOT_INT_DISK_ICON));
		if(i == BOOT_ODD_ICON) 		icon = (NFOBJECT*)nfui_nfimage_new((IMG_BOOT_ODD_ICON));
		if(i == BOOT_EXT_DISK_ICON) icon = (NFOBJECT*)nfui_nfimage_new((IMG_BOOT_EXT_DISK_ICON));

		nfui_nfobject_use_focus(icon, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(icon);

		if(i == BOOT_CAM_ICON) 		nfui_nffixed_put((NFFIXED*)bootFixed, icon, 60, 40);
		if(i == BOOT_INT_DISK_ICON) nfui_nffixed_put((NFFIXED*)bootFixed, icon, 60, 453);
		if(i == BOOT_ODD_ICON) 		nfui_nffixed_put((NFFIXED*)bootFixed, icon, 530-x, 686);
		if(i == BOOT_EXT_DISK_ICON)	nfui_nffixed_put((NFFIXED*)bootFixed, icon, 60, 686);
		
        if (!var_get_supported_ext_disk())
        {
            if (i == BOOT_ODD_ICON)
                nfui_nffixed_put((NFFIXED*)bootFixed, icon, 60, 686);

            if (i == BOOT_EXT_DISK_ICON)
                nfui_nfobject_hide(icon);
        }

#ifndef  SUPPORTED_ODD
    if (i == BOOT_ODD_ICON)
        nfui_nfobject_hide(icon);        
#endif
	}


	// title label
	for(i=0; i<BOOT_ICON_CNT; i++) {
		label = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], 
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(302));
		nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 6);
		nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(300));
		nfui_nfobject_set_size(label, 220, 55);
		nfui_nfobject_show(label);

		if(i == BOOT_CAM_ICON)  	 nfui_nffixed_put((NFFIXED*)bootFixed, label, 115, 40);
		if(i == BOOT_INT_DISK_ICON)  nfui_nffixed_put((NFFIXED*)bootFixed, label, 115, 453);
		if(i == BOOT_ODD_ICON)  	 nfui_nffixed_put((NFFIXED*)bootFixed, label, 585-x, 686);		
		if(i == BOOT_EXT_DISK_ICON)  nfui_nffixed_put((NFFIXED*)bootFixed, label, 115, 686);
		
        if (!var_get_supported_ext_disk())
        {
            if (i == BOOT_ODD_ICON)
                nfui_nffixed_put((NFFIXED*)bootFixed, label, 115, 686);

            if (i == BOOT_EXT_DISK_ICON)
                nfui_nfobject_hide(label);
        }

#ifndef  SUPPORTED_ODD
    if (i == BOOT_ODD_ICON)
        nfui_nfobject_hide(label);        
#endif
	}

	nfui_regi_post_event_callback(g_bootWin, post_bootwin_event_cb);
	uxm_reg_imsg_event(g_bootWin, INFY_DISP_IP_CAMERA_INFO);
	uxm_reg_imsg_event(g_bootWin, INFY_DISP_INT_DISK_INFO);
	uxm_reg_imsg_event(g_bootWin, INFY_DISP_ODD_INFO);
    if (var_get_supported_ext_disk())
    	uxm_reg_imsg_event(g_bootWin, INFY_DISP_EXT_STORAGE_INFO);
	uxm_reg_imsg_event(g_bootWin, INFY_FORMAT_RATE);
	uxm_reg_imsg_event(g_bootWin, INFY_FORMAT_CMPL);
	uxm_reg_imsg_event(g_bootWin, INFY_RTL_SET_RATE);
	uxm_reg_imsg_event(g_bootWin, INFY_RTL_SET_CMPL);
	uxm_reg_imsg_event(g_bootWin, INFY_RECOVERY_RATE);
	uxm_reg_imsg_event(g_bootWin, INFY_RECOVERY_CMPL);
	uxm_reg_imsg_event(g_bootWin, INFY_BOOT_FORMAT_ERROR);
	uxm_reg_imsg_event(g_bootWin, INFY_BOOT_RECOVERY_ERROR);
	uxm_reg_imsg_event(g_bootWin, INFY_CFRM_UNC_ERROR_DETECTED);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_NODISK);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_DISK_CONFLICT);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_DISK_ADDED);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_DISK_REMOVED);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_DISK_CHANGED);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_DISK_NEED_FORMAT);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_SYSTEM_DISK_REMOVED);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_BROKEN_RAID);	
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_FORMAT_RCVR);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_ENFORCE_FORMAT);
	uxm_reg_imsg_event(g_bootWin, IREQ_BOOT_SMART_ERROR);
	uxm_reg_imsg_event(g_bootWin, IREQ_CHANGE_ENHANCED_PASSWORD);
	uxm_reg_imsg_event(g_bootWin, IREQ_WIZARD_INIT);
	uxm_reg_imsg_event(g_bootWin, INFY_RECOVERY_EXPIRED);
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	uxm_reg_imsg_event(g_bootWin, INFY_VLOSS_NOTIFY);
#else
	uxm_reg_imsg_event(g_bootWin, INFY_PND_NOTIFY);
#endif
	uxm_reg_imsg_event(g_bootWin, INFY_BROKEN_RAID_START);
	uxm_reg_imsg_event(g_bootWin, INFY_BROKEN_RAID_CMPL);
	uxm_reg_imsg_event(g_bootWin, INFY_NODISK_BY_WEB);
	uxm_reg_imsg_event(g_bootWin, INFY_FORMAT_BY_WEB);

	nfui_nfwindow_add((NFWINDOW*)g_bootWin, bootFixed);
	nfui_run_main_event_handler(g_bootWin);
	nfui_nfobject_show(g_bootWin);

	evt_send_to_local(ret_msg, 0, 0, 0);

}

void VW_Destroy_BootInfo(IMSG ret_msg)
{
	if(BOOT_WINDOW) {
		nfui_ui_lock();

		nfui_nfobject_destroy((NFOBJECT*)BOOT_WINDOW);
		BOOT_WINDOW = NULL;

		evt_send_to_local(ret_msg, 0, 0, 0);
	}
}
