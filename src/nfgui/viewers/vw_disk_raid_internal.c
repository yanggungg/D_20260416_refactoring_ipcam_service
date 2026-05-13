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

#include "vw_disk_main.h"
#include "vw_disk_raid.h"
#include "vw_disk_raid_internal.h"
#include "vw_disk_raid_easy_conf.h"
#include "vw_disk_raid_advanced_conf.h"
#include "vw_disk_raid_warn.h"
#include "vw_disk_raid_conf_step2.h"


#define DETAIL_WND_POS_X            (27)
#define DETAIL_WND_POS_Y            (420)
#define DETAIL_WND_SIZE_W           (198 + 216 + 478 + 214 + 214 + (2*4))
#define DETAIL_WND_SIZE_H           (420)

#define IMG_STORAGE_POS_START_X     (380+DETAIL_WND_POS_X)
#define IMG_STORAGE_POS_Y           (788) 
#define IMG_BROKEN_DISK_POS_Y       (IMG_STORAGE_POS_Y + 65)

#define LABEL_SIZE_SMALL_THIN       (30)

#define LABEL_DISKNUM_SIZE_W        (91)


static DISK_RAIDINFO_T	*gRaid_i = NULL;		// raid information
static DISK_RAIDINFO_T	gRaid_oi;				// raid original information 

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_content_fixed;
static NFOBJECT *g_detail_page = 0;
static NFTABLE *gTable;
static NFOBJECT *gModeObj;
static NFOBJECT *g_AppBtn;
static NFOBJECT *g_cancelBtn;
static NFOBJECT *g_crBtn;
static NFOBJECT *g_drBtn;
static NFOBJECT *g_wbox = NULL;
static NFOBJECT *g_detailBtn;
static NFIMAGE *g_r1_img[2];

static int cheat_mode = -1;
static int key_count = 0;

enum {
	CHEAT_SHOWFWUP       = 0,
	CHEAT_MODE
};

#define CHEAT_SHOWFWUP_SIZE        (6)

static guint cheat_code[CHEAT_MODE][6] = {
    { KEYPAD_LEFT, KEYPAD_LEFT, KEYPAD_RIGHT, KEYPAD_RIGHT, KEYPAD_LEFT, KEYPAD_RIGHT },
};


static gint _init_raid_status()
{
    gRaid_i = get_disk_raid_info(INTERNAL);

    return 0;
}

static gboolean _popup_wbox(gpointer data)
{
	static gint cnt = 0;

	if(!g_wbox) 
		g_wbox = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
	
	if(cnt++ > 3) {
		cnt = 0;
		gtk_main_quit();
		return FALSE;
	}

	return TRUE;
}

static gint _get_cheat_val(KEYPAD_KID kpid)
{
    gint i;

    if (key_count == 0)
    {
        for (i = 0; i < CHEAT_MODE; i++)
        {
            if (cheat_code[i][key_count] == kpid)
            {
                cheat_mode = i;
                key_count++;
                break;
            }
        }
    }
    else
    {   
        if (cheat_mode == CHEAT_SHOWFWUP)
        {
            if (cheat_code[cheat_mode][key_count] == kpid)
            {
                key_count++;
            }
            else
            {
			    cheat_mode = -1;
                key_count = 0;
            }
        }
        else
        {
		    cheat_mode = -1;
            key_count = 0;
        }
    }

    g_message("%s, %d, cheat_mode:%d, key_count:%d", __FUNCTION__, __LINE__, cheat_mode, key_count);

    if ((cheat_mode == CHEAT_SHOWFWUP) && (key_count == CHEAT_SHOWFWUP_SIZE)) 
    {
	    cheat_mode = -1;
        key_count = 0;    
        return CHEAT_SHOWFWUP;
    }

    return -1;
}

static gint _get_disk_index_in_internal_raid(gint raid_id, gint disk_index)
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

static gint _get_sata_port_in_raid(gint raid_id, gint disk_index)
{
	if(!gRaid_i) return -1;
	if(gRaid_i->mode == 0)	return -1;

	return gRaid_i->rinfo[raid_id].sata_index[disk_index];
}

static guchar _get_disk_raid_mode()
{
    return gRaid_i->mode;
}

static guchar _get_disk_raid_status()
{
    return gRaid_i->rinfo[0].status;
}

static guchar _get_disk_raid_member_count()
{
    return gRaid_i->rinfo[0].member_count;
}

static gboolean _set_text_raid_mode()
{
	if(_get_disk_raid_mode() == RAID_ALL_MODE) 	 nfui_nflabel_set_text((NFLABEL*)gModeObj, "ALL");
	else if(_get_disk_raid_mode() == RAID_1_MODE) nfui_nflabel_set_text((NFLABEL*)gModeObj, "RAID 1");
	else if(_get_disk_raid_mode() == RAID_5_MODE) nfui_nflabel_set_text((NFLABEL*)gModeObj, "RAID 5");
	else nfui_nflabel_set_text((NFLABEL*)gModeObj, "ALL");

	return TRUE;
}

static guchar _get_disk_raid_broken_member()
{
    DISK_RAIDINFO_T *tRaid = gRaid_i;
    guchar broken;
    guchar member_cnt;
    gint status;
    gint i;


    memset(&broken, 0x00, sizeof(broken));
    status = _get_disk_raid_status();
    member_cnt = _get_disk_raid_member_count();
    
    if(status > 1) return broken;

    for(i = 0; i < member_cnt; i++)
    {
        if(tRaid->rinfo[0].sata_index[i] == -1){
            broken |= (1 << i);
        }
    }

    return broken;
}

static gboolean _table_model(gint raid_id, gint disk_index)
{
	NFOBJECT *obj;
	gint disk_port;
	gchar *ppos;

	disk_port = _get_sata_port_in_raid(raid_id, disk_index);
	if(disk_port < 0)
		return FALSE;
	
	obj = nfui_nftable_get_child(gTable, 1, disk_port+1);

	if(strlen(gRaid_i->rinfo[raid_id].model[disk_index])) 
	{
		ppos = strchr(gRaid_i->rinfo[raid_id].model[disk_index], ' '); 
		if(ppos) 
		{
			sprintf(ppos, "%c", '\0');	
			memcpy(&gRaid_oi.rinfo[raid_id].model[disk_index], gRaid_i->rinfo[raid_id].model[disk_index], sizeof(gchar)*41);
		}
	
		nfui_nflabel_set_text((NFLABEL*)obj, gRaid_i->rinfo[raid_id].model[disk_index]);
	}
	else
		nfui_nflabel_set_text((NFLABEL*)obj, "-");
	
	return TRUE;
}

static gboolean _table_capacity(gint raid_id, gint disk_index)
{
	NFOBJECT *obj;
	gint disk_port;
	gint d_index;
	gint unused_idx;
	gchar buf[64] = {0,};

	disk_port = _get_sata_port_in_raid(raid_id, disk_index);
	if(disk_port < 0)
	    return FALSE;
	
	if(disk_port < 0) {			
		obj = nfui_nftable_get_child(gTable, 2, disk_index+1);
		if(obj){
			nfui_nflabel_set_text((NFLABEL*)obj, "-");
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
			return TRUE;
			
		} else {			
			return FALSE;
		}
	}
	
	obj = nfui_nftable_get_child(gTable, 2, disk_port+1);
	
	d_index = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "disk order"));
	
	if(raid_id != -1)
	{			


#if 0
		unused_idx = _get_disk_index_in_internal_raid(raid_id, d_index);
		if(gRaid_i->rinfo[raid_id].unused[unused_idx] != 0) 
			sprintf(buf, "<# %d> USED (%dGB) / UNUSED (%dGB)", raid_id, 
						gRaid_i->rinfo[raid_id].capacity, 
						gRaid_i->rinfo[raid_id].unused[raid_id]);
		else			 			
			sprintf(buf, "<# %d> USED (%dGB)", raid_id, 
						gRaid_i->rinfo[raid_id].capacity);
#else	// FIXME
			
			// RAID10 case
			if( gRaid_i->rinfo[raid_id].raid_mode == RAID_LEVEL_1 ) {				
				sprintf(buf, "<# %d> USED (%dGB)", d_index/2, 
							gRaid_i->rinfo[raid_id].capacity);
				raid_id	 = d_index/2;
			} else {
				sprintf(buf, "<# %d> USED (%dGB)", raid_id, 
							gRaid_i->rinfo[raid_id].capacity);	
			}
#endif 		
		if(raid_id == 0) 
		{
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1001));
		}
		else if(raid_id == 1)
		{
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1002));
		}
		else if(raid_id == 2)
		{
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1003));
		}
		else if(raid_id == 3)
		{
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1004));
		}
		else if(raid_id == 4)
		{
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1005));
		}
	}
	else
		strcpy(buf, "-");
		
	nfui_nflabel_set_text((NFLABEL*)obj, buf);

	return TRUE;
}

static gboolean _table_status(gint raid_id, gint disk_index)
{
	NFOBJECT *obj;
	gint disk_port;
	
	disk_port = _get_sata_port_in_raid(raid_id, disk_index);
	if(disk_port < 0)
	    return FALSE;
	g_message("%s(%d) : disk_port[%d] raid_id[%d] disk_idx[%d]", __FUNCTION__, __LINE__, 
				disk_port, raid_id, disk_index);
				
	if(disk_port < 0) {	
		obj = nfui_nftable_get_child(gTable, 3, disk_index+1);
		if(obj){
			g_message("%s(%d) : obj[0x%x]", __FUNCTION__, __LINE__, obj);
			nfui_nflabel_set_text((NFLABEL*)obj, "-");			
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
			return TRUE;
			
		} else {
			g_message("%s(%d) : obj[0x%x]", __FUNCTION__, __LINE__ ,obj);
			return FALSE;
		}
	}
g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	obj = nfui_nftable_get_child(gTable, 3, disk_port+1);
g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	switch(gRaid_i->rinfo[raid_id].status) {
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
	g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	if(gRaid_i->rinfo[raid_id].status < 0 || gRaid_i->rinfo[raid_id].status > 4)
	{
		nfui_nfobject_enable(g_crBtn);
		nfui_nfobject_disable(g_drBtn);
		nfui_nfobject_disable(g_detailBtn);
	}
	else
	{
		nfui_nfobject_disable(g_crBtn);
		nfui_nfobject_enable(g_drBtn);
		nfui_nfobject_enable(g_detailBtn);	
	}
	nfui_signal_emit(gModeObj, GDK_EXPOSE, FALSE);
	g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	return TRUE;
}

static gboolean _table_rebuild(gint raid_id, gint disk_index)
{
	NFOBJECT *obj;
	gint disk_port;
	gchar buf[24] = {0,};

g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	if(!gRaid_i) return FALSE;
	if(gRaid_i->mode == 0)	return FALSE;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return;

	disk_port = _get_sata_port_in_raid(raid_id, disk_index);			
	if(disk_port < 0)
	    return FALSE;
	        
	if(disk_port < 0) {			
		obj = nfui_nftable_get_child(gTable, 4, disk_index+1);
		if(obj){
			nfui_nflabel_set_text((NFLABEL*)obj, "-");
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
			return TRUE;
			
		} else {			
			return FALSE;
		}
	}
	obj = nfui_nftable_get_child(gTable, 4, disk_port+1);

	if(raid_id < 0) {
		strcpy(buf, "-");
	}
	else {
		if(gRaid_i->rinfo[raid_id].status == 2) 
		{
			sprintf(buf, "%d%%", gRaid_i->rinfo[raid_id].rebuild);
		}
		else 
			strcpy(buf, "-");
		
		nfui_nflabel_set_text((NFLABEL*)obj, buf);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1016));
	}
g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	return TRUE;
}

static gboolean _draw_raid_info(gboolean expose)
{
	NFOBJECT *obj;
	gint i, j, k;
	gint disk_port;
	gchar buf[64] = {0,};

g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	g_printf("%s\nmode: %d\n", __FUNCTION__, gRaid_i->mode);
	for(j=0; j<2; j++) {
		g_printf("[%d]\n"
	            "raid cnt: %d\n"
	            "raid mode: %d\n"
	            "member cnt: %d\n"
				"model: [%s]\n"
		               "[%s]\n"
        	           "[%s]\n"
        		       "[%s]\n"
        	  	       "[%s]\n"
				"capacity: %d\n"
				"status: %d\n"
				"rebuild: %d\n"
				"sata_index: %d %d %d %d %d\n"
				"sata_capacity: %d, %d, %d, %d, %d\n\n", 
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
				gRaid_i->rinfo[j].status, 
				gRaid_i->rinfo[j].rebuild,
				gRaid_i->rinfo[j].sata_index[0],
				gRaid_i->rinfo[j].sata_index[1],
				gRaid_i->rinfo[j].sata_index[2],
				gRaid_i->rinfo[j].sata_index[3],
			gRaid_i->rinfo[j].sata_index[4],
			gRaid_i->rinfo[j].sata_capacity[0],
			gRaid_i->rinfo[j].sata_capacity[1],
			gRaid_i->rinfo[j].sata_capacity[2],
			gRaid_i->rinfo[j].sata_capacity[3],
			gRaid_i->rinfo[j].sata_capacity[4]
				);
	}

	_set_text_raid_mode();

	if(gRaid_i->mode == 0)	return FALSE;

	for(i=0; i<gRaid_i->raid_cnt; i++) {

		if(gRaid_i->rinfo[i].member_count == 0)
			continue;

		for(j=0; j<gRaid_i->rinfo[i].member_count; j++) 
		{
			for(k=0; k<4; k++) 
			{
				if(k == 0)_table_model(i, j);
				else if(k == 1) _table_capacity(i, j); 				
				else if(k == 2) _table_status(i, j);
				else if(k == 3) _table_rebuild(i, j);
			}
		}
	}
	
    g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
		
	if(expose)
	{
		nfui_signal_emit((NFOBJECT*)gTable, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_crBtn, GDK_EXPOSE, FALSE);
		nfui_signal_emit(g_drBtn, GDK_EXPOSE, FALSE);
		nfui_signal_emit(g_detailBtn, GDK_EXPOSE, FALSE);
	}
	g_message(":::::::::::::::::::::: %s [%d]", __FUNCTION__, __LINE__);
	return TRUE;
}

static gboolean _redraw_raid_info(gpointer data)
{
    NFOBJECT *wait_box = (NFOBJECT*)data;

	scm_get_disk_raidinfo(gRaid_i);
    nftool_remove_waitbox(wait_box);
    
	_draw_raid_info(TRUE);
	
	if(memcmp(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T))) 
		memcpy(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T));

    return FALSE;
}

static gboolean _post_refresh_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) 
    {  
        NFOBJECT *wait_box;
        
        wait_box = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
        g_timeout_add(300, _redraw_raid_info, (gpointer)wait_box);
	}

	return FALSE;
}

static gboolean _post_detail_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) 
    {
        if(!nfui_nfobject_is_shown(g_detail_page)){
            nfui_nfobject_show(g_detail_page);
            nfui_signal_emit(g_detail_page, GDK_EXPOSE, TRUE);
        }
	}

	return FALSE;
}


static gboolean _post_create_raid_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {

	    if(evt->type == MOUSE_RIGTH_BUTTON) return FALSE;
	    
		gint rmode = -1;
		gint ret = -1;

		rmode = VW_DiskRaid_EasyConf_Open(g_curwnd, INTERNAL);

		if(rmode != -1) {

			if(VW_DiskRaid_Warn_Open(g_curwnd)) 
			{				
				g_timeout_add(100, _popup_wbox, NULL);
				
				gtk_main();
				if(rmode == RAID1_CONF)
					scm_create_raid1(INTERNAL, IRET_SCM_DISK_CREATE_RAID);
				else if(rmode == RAID5_CONF)
					scm_create_raid5(INTERNAL, IRET_SCM_DISK_CREATE_RAID);
			}
			else
			{				
				if(memcmp(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T))) 
					memcpy(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T));
			}

		}

	}
	return FALSE;
}

static gboolean _post_delete_raid_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {

		if(VW_DiskRaid_Warn_Open(g_curwnd)) {	
			
			g_timeout_add(100, _popup_wbox, NULL);
			
			gtk_main();
			scm_delete_raid(IRET_SCM_DISK_DELETE_RAID);			
		}
		else
		{				
			if(memcmp(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T))) 
				memcpy(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T));
		}
	}
	
	return FALSE;
}

static gboolean _post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
   	gint val;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		VW_DiskRaid_tab_out_handler();
		VW_DiskSetup_Destroy(obj);
	}
	else if ((evt->type == NFEVENT_KEYPAD_PRESS) || (evt->type == NFEVENT_REMOCON_PRESS))
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

        	val = _get_cheat_val(kpid);

		if (val == CHEAT_SHOWFWUP) 
		{
			g_usleep(10*1000);	
			VW_Jmfwup_Popup_Open(g_curwnd);
		}
	}


	return FALSE;
}

static gboolean _post_raid1_broken_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable;
        GdkGC *gc;
        GdkPixbuf *brk_img;
        guchar member_cnt;
        guchar broken;
        gint i, img_cnt;
        gint size_w, size_h;
        gint pos_x;


        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_get_pixbuf_size(((NFIMAGE*)obj)->pixbuf, &size_w, &size_h);
        
        brk_img = nfui_get_image_from_file(IMG_DISK_BROKEN_RAID_1, NULL);

        member_cnt = _get_disk_raid_member_count();
        broken = _get_disk_raid_broken_member();

        if (member_cnt == 4)
        {
            pos_x = IMG_STORAGE_POS_START_X + ((DETAIL_WND_SIZE_W - (size_w * 2 + 60)) / 2) + 21;
        }
        else
        {
            pos_x = IMG_STORAGE_POS_START_X + ((DETAIL_WND_SIZE_W - size_w) / 2) + 21;
        }

        if (g_r1_img[0] == obj)
        {
            if((broken & (1 << 0)))
            {   
                nfutil_draw_pixbuf(drawable, gc, brk_img, pos_x, IMG_BROKEN_DISK_POS_Y, -1, -1, NFALIGN_LEFT, 0);
            }
            if((broken & (1 << 1)))
            {
                pos_x += 106;
                nfutil_draw_pixbuf(drawable, gc, brk_img, pos_x, IMG_BROKEN_DISK_POS_Y, -1, -1, NFALIGN_LEFT, 0);
            }
        }
        else
        {
            if((broken & (1 << 2)))
            {
                pos_x += 268;
                nfutil_draw_pixbuf(drawable, gc, brk_img, pos_x, IMG_BROKEN_DISK_POS_Y, -1, -1, NFALIGN_LEFT, 0);
            }
            if((broken & (1 << 3)))
            {
                pos_x += 374;
                nfutil_draw_pixbuf(drawable, gc, brk_img, pos_x, IMG_BROKEN_DISK_POS_Y, -1, -1, NFALIGN_LEFT, 0);
            }
        }

        nfui_nfobject_gc_unref(gc);
    }
    return FALSE;
}

static gboolean _post_raid5_broken_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable;
        GdkGC *gc;
        GdkPixbuf *brk_img;
        guchar member_cnt;
        guchar broken;
        gint i;
        gint size_w, size_h;
        gint pos_x;


        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_get_pixbuf_size(((NFIMAGE*)obj)->pixbuf, &size_w, &size_h);
        
        brk_img = nfui_get_image_from_file(IMG_DISK_BROKEN_RAID_5, NULL);

        member_cnt = _get_disk_raid_member_count();
        broken = _get_disk_raid_broken_member();

        pos_x = IMG_STORAGE_POS_START_X + ((DETAIL_WND_SIZE_W - size_w) / 2) + 21;

        for( i = 0; i < member_cnt; i++)
        {
            if((broken & (1 << i)))
            {   
                nfutil_draw_pixbuf(drawable, gc, brk_img, pos_x, IMG_BROKEN_DISK_POS_Y, -1, -1, NFALIGN_LEFT, 0);
            }
            pos_x += 105;
        }

        nfui_nfobject_gc_unref(gc);
    }
    return FALSE;
}

static gboolean _post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;

	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		uxm_unreg_imsg_event(obj, IRET_SCM_DISK_DELETE_RAID);
		uxm_unreg_imsg_event(obj, IRET_SCM_DISK_CREATE_RAID);
	}
	else if(evt->type == IRET_SCM_DISK_DELETE_RAID)
    {
		if(g_wbox) 
	    {
			nftool_remove_waitbox(g_wbox);
			g_wbox = NULL;
				}

					nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_OK);
		scm_reboot_system(RR_NA, 0);
				}
	else if(evt->type == IRET_SCM_DISK_CREATE_RAID)
	{
		gint ret = ((CMM_MESSAGE_T *)data)->param;

		if(g_wbox) 
		{
			nftool_remove_waitbox(g_wbox);
			g_wbox = NULL;
        }

		if(ret < 0) 
        {
			nftool_mbox_auto(g_curwnd, 3, "WARNING", "RAID configuration is fail...\nSystem will be reboot."); 
		}
		else 
	    {
			nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_OK);			
		}
	

		scm_reboot_system(RR_NA, 0);		
	}

	return FALSE;
}

static gboolean _pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(gRaid_i)
	{
        nfui_nfobject_disable(g_crBtn);
        nfui_signal_emit(g_crBtn, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gint _make_raid_1_image(NFOBJECT *parent, gint member_cnt)
{
    NFOBJECT *obj;
    NFOBJECT *fixed;
    NFOBJECT *img[2];
    GdkPixbuf *imgg;
    NFOBJECT *disk_num[4];
    NFOBJECT *raid_num[2];
    gchar *strDisk[] = {"DISK1", "DISK2", "DISK3", "DISK4"};
    gchar *strStorage[] = {"#0", "#1"};
    gint i = 0;
    gint size_w;
    gint storage_cnt;


    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_show(fixed);

    for (i = 0; i < 2; i++)
    {
    	img[i] = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID1_3);
    	nfui_regi_post_event_callback(img[i], _post_raid1_broken_event_cb);
    	nfui_nfobject_show(img[i]);
    	g_r1_img[i] = img[i];
	}

    for(i = 0; i < 2; i++){
    	raid_num[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strStorage[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1020));
    	nfui_nfobject_set_size(raid_num[i], 22, 22);
    	nfui_nfobject_modify_bg(raid_num[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    	nfui_nfobject_use_focus(raid_num[i], NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(raid_num[i]);
	}
    
    for(i = 0; i < 4; i++){
    	disk_num[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDisk[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
    	nfui_nfobject_set_size(disk_num[i], 104, 22);
    	nfui_nfobject_modify_bg(disk_num[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    	nfui_nflabel_set_align(disk_num[i], NFALIGN_CENTER, 0);
    	nfui_nfobject_use_focus(disk_num[i], NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(disk_num[i]);
	}

    if(member_cnt < 4){
        size_w = 208;
        nfui_nfobject_set_size(fixed, size_w, 166);
        nfui_nffixed_put((NFFIXED*)fixed, img[0], 0, 0);
        nfui_nffixed_put((NFFIXED*)fixed, raid_num[0], (208 - 22)/2, 0);
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[0], 0, 144);
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[1], 105, 144);
    }
    else{
        size_w = 208 * 2 + 60;
        nfui_nfobject_set_size(fixed, size_w, 166);
        nfui_nffixed_put((NFFIXED*)fixed, img[0], 0, 0);
        nfui_nffixed_put((NFFIXED*)fixed, img[1], 268, 0);
        nfui_nffixed_put((NFFIXED*)fixed, raid_num[0], (208 - 22)/2, 0);
        nfui_nffixed_put((NFFIXED*)fixed, raid_num[1], size_w - ((208+22)/2), 0);
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[0], 0, 144);
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[1], 105, 144);
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[2], 270, 144);
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[3], 375, 144);
    }

    nfui_nffixed_put((NFFIXED*)parent, fixed, (DETAIL_WND_SIZE_W-(fixed->width))/2, 228);

    return 0;
}

static gint _make_raid_5_image(NFOBJECT *parent, gint member_cnt)
{
    NFOBJECT *obj;
    NFOBJECT *fixed;
    NFOBJECT *img;
    NFOBJECT *disk_num[5];
    NFOBJECT *raid_num;
    gchar *strDisk[] = {"DISK1", "DISK2", "DISK3", "DISK4", "DISK5"};
    gchar strStorage[3] = "#0";
    gint i = 0;
    gint size_w, pos_x;


    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_show(fixed);

	if(member_cnt == 3){
    	size_w = 312;
	    img = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID5_3);
    	nfui_nfobject_show(img);
	}
	else if(member_cnt == 4){
	    size_w = 416;
	    img = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID5_4);
    	nfui_nfobject_show(img);
	}
	else{
	    size_w = 520;
	    img = (NFOBJECT*)nfui_nfimage_new(IMG_DISK_RAID5_5);
    	nfui_nfobject_show(img);
	}
	nfui_regi_post_event_callback(img, _post_raid5_broken_event_cb);

	raid_num = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strStorage, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1020));
	nfui_nfobject_set_size(raid_num, 22, 22);
	nfui_nfobject_modify_bg(raid_num, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(raid_num, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(raid_num);
    
    for(i = 0; i < 5; i++){
    	disk_num[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDisk[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(1019));
    	nfui_nfobject_set_size(disk_num[i], 104, 22);
    	nfui_nflabel_set_align(disk_num[i], NFALIGN_CENTER, 0);
    	nfui_nfobject_modify_bg(disk_num[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    	nfui_nfobject_use_focus(disk_num[i], NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(disk_num[i]);
	}
       
    nfui_nfobject_set_size(fixed, size_w, 166);
    nfui_nffixed_put((NFFIXED*)fixed, img, 0, 0);
    nfui_nffixed_put((NFFIXED*)fixed, raid_num, (size_w - 22)/2, 0);
    
    pos_x = 0;
    for(i = 0; i < member_cnt; i++){
        nfui_nffixed_put((NFFIXED*)fixed, disk_num[i], pos_x, 144);
        pos_x += 105;
    }

    nfui_nffixed_put((NFFIXED*)parent, fixed, (DETAIL_WND_SIZE_W-(fixed->width))/2, 228);

    return 0;
}

static gint _make_raid_disk_image(NFOBJECT *parent)
{
    guchar member_cnt;
    guchar broken;


    member_cnt = _get_disk_raid_member_count();

    if(_get_disk_raid_mode() == RAID_1_MODE){
        _make_raid_1_image(parent, member_cnt);
    }
    else if(_get_disk_raid_mode() == RAID_5_MODE){
        _make_raid_5_image(parent, member_cnt);
    }

    return 0;
}

static gint _init_detail_page()
{
    NFOBJECT *main_fixed;
    NFOBJECT *status_label[6];
    NFOBJECT *obj;
    gint i;
    gint label_cnt;
    gchar *strDesc[] = {"<STATUS DESCRIPTION AND ACTIONS>",
                        "NORMAL : Normal status.",
                        "DEGRADED : The disk is defective and should be urgently replaced.", 
                        "REBUILD : The RAID volume is currently recovering. Recovery speeds can be affected by recording/playback activity.",
                        "BROKEN : The number of defected disks has exceeded the RAID system's damage tolerance.",
                        "<CURRENT STATUS>"};

    main_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(main_fixed, DETAIL_WND_SIZE_W, DETAIL_WND_SIZE_H);
    nfui_nffixed_put((NFFIXED*)g_content_fixed, main_fixed, DETAIL_WND_POS_X, DETAIL_WND_POS_Y);
    g_detail_page = main_fixed;

    label_cnt = 6;
    for(i = 0; i < label_cnt; i++)
    {
        status_label[i] = nfui_nflabel_new_with_pango_font(strDesc[i], nffont_get_pango_font(NFFONT_SMALL_THIN), COLOR_IDX(116));
        nfui_nfobject_set_size(status_label[i], DETAIL_WND_SIZE_W, 30);
        nfui_nflabel_set_align(status_label[i], NFALIGN_LEFT, 10);
        nfui_nfobject_modify_bg(status_label[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(status_label[i], FALSE);
        if(i == 5){
            nfui_nffixed_put((NFFIXED*)main_fixed, status_label[i], 0, (18+(32*i)));
        }
        else{
            nfui_nffixed_put((NFFIXED*)main_fixed, status_label[i], 0, (3+(32*i)));
        }
        nfui_nfobject_show(status_label[i]);
    }
    
    _make_raid_disk_image(main_fixed);

    return 0;
}

void VW_Init_DiskInternal_Raid_Page(NFOBJECT *parent)
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
	nfui_regi_pre_event_callback(g_curwnd, _pre_page_event_handler);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
	g_content_fixed = content_fixed;


	// raid mode
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RAID MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
	nfui_nfobject_set_size(obj, 198, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 25);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
	nfui_nfobject_set_size(obj, 216, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 227, 25);
	gModeObj = obj;


	// raid table
	tbl = (NFOBJECT*)nfui_nftable_new(5, 6, 2, 1, table_w, 40);	
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
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(font_idx));
 		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk order", GINT_TO_POINTER(i));

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}
	
	// status
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(font_idx));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 3, i + 1);
	}
	
	// rebuild
	for(i=0; i<5; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(font_idx));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "disk order", GINT_TO_POINTER(i));

		nfui_nftable_attach((NFTABLE*)tbl, obj, 4, i + 1);
	}

	// button
	obj = nftool_normal_button_create_subtab_type1("REFRESH", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, _post_refresh_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 360);

	obj = nftool_normal_button_create_subtab_type1("DETAIL", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_regi_post_event_callback(obj, _post_detail_btn_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 236, 360);
	g_detailBtn = obj;

	obj = nftool_normal_button_create_subtab_type1("CREATE RAID", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, _post_create_raid_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 950, 360);
	g_crBtn = obj;

	obj = nftool_normal_button_create_subtab_type1("DELETE RAID", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, _post_delete_raid_event_handler);
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 1158, 360);
	g_drBtn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	g_cancelBtn = obj;

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	g_AppBtn = obj;

	obj = nftool_normal_button_create_type1("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, _post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, _post_page_event_handler);

	uxm_reg_imsg_event(parent, IRET_SCM_DISK_DELETE_RAID);	
	uxm_reg_imsg_event(parent, IRET_SCM_DISK_CREATE_RAID);

}

gboolean display_internal_raid(gboolean expose)
{
    if(!gRaid_i){
    	gRaid_i = get_disk_raid_info(INTERNAL);
	}

	memcpy(&gRaid_oi, gRaid_i, sizeof(DISK_RAIDINFO_T));

	if(!_draw_raid_info(expose))
		return FALSE;

    _init_detail_page();
    
	return TRUE;
}

gboolean check_disk_int_raid_conf_changed()
{
	if(memcmp(gRaid_i, &gRaid_oi, sizeof(DISK_RAIDINFO_T)))
		return TRUE;

	return FALSE;
}

gboolean apply_disk_int_raid_conf()
{
	if(scm_conf_raid(INTERNAL, gRaid_i) < 0) {
		g_warning("%s : raid create fail", __FUNCTION__);

		nftool_mbox(g_curwnd, "WARNING", "RAID configuration is fail...\nSystem will be reboot.", NFTOOL_MB_OK);

		scm_reboot_system(RR_NA, 0);

		return FALSE;
	}

	return TRUE;
}
