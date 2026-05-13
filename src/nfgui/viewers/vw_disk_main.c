#include "nf_afx.h"

#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfwindow.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

#include "vw_disk_main.h"
#include "vw_disk_information.h"
#include "vw_disk_internal_information.h"
#include "vw_disk_operation.h"
#include "vw_disk_smart.h"

#include "vsm.h"
#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		0
#define DBG_MODULE		"LOCAL_VW"


static NFWINDOW *g_curwnd = 0;

static DISK_CAPINFO_T 			gDisk_i[2];
static DISK_SMARTINFO_T 		gSmart_i[2];
static DISK_RECINFO_T			gDisk_t[2];
static DISK_RAIDINFO_T			gRaid_i[2];

static gboolean g_diskChanged = FALSE;

static NFOBJECT *g_wBox = NULL;



// FIXME: api
static gboolean _load_disk_data()
{
	// get disk info
	//scm_get_disk_capinfo(INTERNAL, &gDisk_i[0]);
	//scm_get_disk_capinfo(EXTERNAL, &gDisk_i[1]);


	// get disk time
	//scm_get_disk_recinfo(INTERNAL, &gDisk_t[0]);
	//scm_get_disk_recinfo(EXTERNAL, &gDisk_t[1]);

	// get raid info
	//memset(gRaid_i, 0x00, sizeof(gRaid_i));
	//scm_get_disk_raidinfo(gRaid_i);

	// get smart info
	scm_get_disk_smartinfo(INTERNAL, &gSmart_i[0]);
	scm_get_disk_smartinfo(EXTERNAL, &gSmart_i[1]);

	return TRUE;
}

static gboolean _load_disk_data_ver2(DISK_DB_T *ddb)
{
	// get disk info
    g_memmove(&gDisk_i[0], &ddb->cap[0], sizeof(DISK_CAPINFO_T));
    g_memmove(&gDisk_i[1], &ddb->cap[1], sizeof(DISK_CAPINFO_T));

	// get disk time
    g_memmove(&gDisk_t[0], &(ddb->rec[0]), sizeof(DISK_RECINFO_T));
    g_memmove(&gDisk_t[1], &(ddb->rec[1]), sizeof(DISK_RECINFO_T));

	// get raid info
	memset(gRaid_i, 0x00, sizeof(gRaid_i));
    g_memmove(&gRaid_i, &(ddb->raid), sizeof(DISK_RAIDINFO_T));

	// get smart info
    g_memmove(&gSmart_i[0], &(ddb->smart[0]), sizeof(DISK_SMARTINFO_T));
    g_memmove(&gSmart_i[1], &(ddb->smart[1]), sizeof(DISK_SMARTINFO_T));

	return TRUE;
}

static void _disp_disk_data(guint page)
{
	if(page == mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION])
	{
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION] != -1) display_disk_inforamtion(TRUE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP] != -1) display_disk_smart(FALSE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION] != -1) display_disk_raid(FALSE);
	}
	else if(page == mcf.sys_sub7.menu_pos[SYS_SUB7_OPERATION])
	{
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION] != -1) display_disk_inforamtion(FALSE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP] != -1) display_disk_smart(FALSE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION] != -1) display_disk_raid(FALSE);
	}
	else if(page == mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION])
	{
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION] != -1) display_disk_inforamtion(FALSE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP] != -1) display_disk_smart(FALSE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION] != -1) display_disk_raid(TRUE);
	}
	else if(page == mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP])
	{
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION] != -1) display_disk_inforamtion(FALSE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP] != -1) display_disk_smart(TRUE);
		if (mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION] != -1) display_disk_raid(FALSE);
	}
}

static gboolean _display_page(gpointer data)
{
	NFUTIL_THREADS_ENTER();

	_disp_disk_data(GPOINTER_TO_UINT(data));

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean _show_mbox()
{
	g_wBox = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Fetching disk information.", "Please wait...");

	return FALSE;
}

static gboolean post_diskInfo_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
	    //if (!g_wBox)
    		//g_timeout_add(200, _display_page, GUINT_TO_POINTER(0));
	}

	return FALSE;
}

static gboolean post_diskOper_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
	    //if (!g_wBox)
    		//g_timeout_add(200, _display_page, GUINT_TO_POINTER(1));
	}

	return FALSE;
}

static gboolean post_diskConf_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
	    //if (!g_wBox)
    		//g_timeout_add(200, _display_page, GUINT_TO_POINTER(2));
	}

	return FALSE;
}

static gboolean post_diskSmart_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
	    //if (!g_wBox)
    		//g_timeout_add(200, _display_page, GUINT_TO_POINTER(3));
	}

	return FALSE;
}

static void _init_disk_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION];
    if (pos != -1) {
        DMSG(1, "VW_Init_DiskInfo_Page");

    	VW_Init_DiskInfo_Page(((NFTAB*)nftab)->page[pos]);
    	nfui_regi_post_event_callback(((NFTAB*)nftab)->page[pos], post_diskInfo_page_event_handler);
    }

    pos = mcf.sys_sub7.menu_pos[SYS_SUB7_OPERATION];
    if (pos != -1) {
        DMSG(1, "VW_Init_DiskOper_Page");

    	VW_Init_DiskOper_Page(((NFTAB*)nftab)->page[pos]);
    	nfui_regi_post_event_callback(((NFTAB*)nftab)->page[pos], post_diskOper_page_event_handler);
    }

    pos = mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION];
    if (pos != -1) {
        DMSG(1, "VW_Init_DiskRaid_Page");

    	VW_Init_DiskRaid_Page(((NFTAB*)nftab)->page[pos]);
    	nfui_regi_post_event_callback(((NFTAB*)nftab)->page[pos], post_diskConf_page_event_handler);
    }

    pos = mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP];
    if (pos != -1) {
        DMSG(1, "VW_Init_DiskSmart_Page");

    	VW_Init_DiskSmart_Page(((NFTAB*)nftab)->page[pos]);
    	nfui_regi_post_event_callback(((NFTAB*)nftab)->page[pos], post_diskSmart_page_event_handler);
    }
}

static void _in_handler_disk_main(gint page)
{
    if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION])
    {
        DMSG(1, "");
    }
    else if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_OPERATION])
    {
        DMSG(1, "");
    }
    else if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION])
    {
        DMSG(1, "");
    }
    else if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP])
    {
        DMSG(1, "");
    }
}

static void _out_handler_disk_main(gint page)
{
    if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION])
    {
        DMSG(1, "VW_DiskInfo_tab_out_handler");

        VW_DiskInfo_tab_out_handler();
    }
    else if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_OPERATION])
    {
        DMSG(1, "VW_DiskOper_tab_out_handler");

        VW_DiskOper_tab_out_handler();
    }
    else if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION])
    {
        DMSG(1, "VW_DiskRaid_tab_out_handler");

		VW_DiskRaid_tab_out_handler();
    }
    else if (page == mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP])
    {
        DMSG(1, "VW_DiskSmart_tab_out_handler");

        VW_DiskSmart_tab_out_handler();
    }
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	NFOBJECT *nftab = NULL;
	gint cur_page;

	nftab = nftool_get_nftab_from_setup_window(top);
	if(!nftab)	return FALSE;

	cur_page = nfui_nftab_get_cur_page((NFTAB*)nftab);

	if(cur_page < 0 || cur_page >= ((NFTAB*)nftab)->pages)
		return FALSE;

    _out_handler_disk_main(cur_page);

	return TRUE;
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		if(g_wBox)
			g_wBox = NULL;

		if(disk_data_is_changed())
			DAL_save_setup_db(NFSETUP_WINDOW_DISK);

		vsm_live_start();

		g_curwnd = 0;

		// SKSHIN
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch(evt->type)
	{
		case NFEVENT_TAB_BEFORE_CHANGE:
			cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
			new_page = nfui_nftab_get_new_page((NFTAB*)obj);

			if(cur_page == new_page)	return FALSE;

            _out_handler_disk_main(cur_page);
            _in_handler_disk_main(new_page);
		break;

    	case IRET_SCM_GET_DISK_INFO:
    	{
    	    CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T*)data;
    	    DISK_DB_T *ddb = (DISK_DB_T*)pmsg->data;

    	    _load_disk_data_ver2(ddb);

    	    if (g_wBox)
    	    {
            	nftool_remove_waitbox(g_wBox);
            	g_wBox = NULL;
    	    }

			cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
    	    //g_timeout_add(100, _display_page, GINT_TO_POINTER(cur_page));
            nfui_nfobject_timeout_add(obj, 100, _display_page, GINT_TO_POINTER(cur_page));
        }
        break;

        case GDK_DELETE:
            uxm_unreg_imsg_event(obj, IRET_SCM_GET_DISK_INFO);
            break;

		default:
		break;
	}

	return FALSE;
}

void disk_data_changed(gboolean change)
{
	g_diskChanged = change;
}

gboolean disk_data_is_changed()
{
	return g_diskChanged;
}

void VW_DiskSetup_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page)
{
	NFOBJECT *disk_wnd, *nftab;

    DMSG(1, "VW_DiskSetup_Open");

	vsm_live_stop();
	disk_data_changed(FALSE);

	disk_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_DISK, page);
	g_curwnd = disk_wnd;

	nfui_nfwindow_set_title(disk_wnd, "SYSTEM SETUP - DISK");
	nftab = nftool_get_nftab_from_setup_window(disk_wnd);
	nfui_nftab_set_cur_page(nftab, page);

    _init_disk_main(nftab, page);

	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
	nfui_regi_pre_event_callback(disk_wnd, pre_main_wnd_event_handler);
	nfui_nfwindow_set_returnkey_proc(disk_wnd, returnkey_proc);

	nfui_nfobject_show(disk_wnd);
	nfui_make_key_hierarchy(disk_wnd);

	nfui_set_key_focus(nftab, TRUE);

    uxm_reg_imsg_event(nftab, IRET_SCM_GET_DISK_INFO);

	_show_mbox();
    scm_get_disk_info(IRET_SCM_GET_DISK_INFO);

	gtk_main();

    DMSG(1, "VW_DiskSetup_Open EXIT");
}

void VW_DiskSetup_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(obj);
	nfui_nfobject_hide(topwin);
	if(topwin)
		nftool_destroy_setup_window(topwin);
}

DISK_CAPINFO_T* get_disk_info(gint group)
{
	return &gDisk_i[group];
}

DISK_SMARTINFO_T* get_smart_disk_info(gint group)
{
	return &gSmart_i[group];
}

DISK_RECINFO_T* get_disk_rec_time(gint group)
{
	return &gDisk_t[group];
}

DISK_RAIDINFO_T* get_disk_raid_info(STRG_TYPE_E type)
{
	return &gRaid_i[type];
}

gboolean update_smart_disk_info()
{
	return _load_disk_data();
}

gchar* conv_smart_status_to_string(guint status, gchar *buf)
{
/*	switch(status) {
		case 0: return "NORMAL";
		case 1: return "NORMAL";	// in the past
		case 2: return "CHECK";
		case 3: return "ERROR";		// failing now
	}
	return "-";*/

	strcpy(buf, "-");

	switch(status) {
		case 0: strcpy(buf, "NORMAL"); break;
		case 1: strcpy(buf, "NORMAL"); break;
		case 2: strcpy(buf, "CHECK"); break;
		case 3: strcpy(buf, "ERROR"); break;
	}
//	return "-";

	return buf;
}

gint get_disk_count(gint group)
{
    return gDisk_i[group].tdisk_count;
}
