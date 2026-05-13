//#define __USE_GNU
#include "nf_afx.h"

#include "nf_ptz.h"
#include "nf_api_live.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"

#include "uxm.h"
#include "scm.h"
#include "ssm.h"
#include "smt.h"
#include "iux_msg.h"

#include "vw_system_fw_up.h"
#include "vw_progress_fwupdate_128MB.h"
#include "vw_vendor_code.h"

//#include <io.h>
#include <dirent.h>

#define FILE_STRING_LENGTH					(256)
#define FILE_COUNT							(10)
#define FILE_SIZE							(3) //3MB

#define ITX_FORCE_DB_INITIALIZE             "itx_force_db_init.txt"

#define FW_UP_WIN_SIZE_W					(652)
#define FW_UP_WIN_SIZE_H					(448)

#define FW_UP_WIN_SIZE_X					((DISPLAY_ACTIVE_WIDTH - FW_UP_WIN_SIZE_W)/2)
#define FW_UP_WIN_SIZE_Y					((DISPLAY_ACTIVE_HEIGHT - FW_UP_WIN_SIZE_H)/2)

#define FW_UP_TITLE_H						(36)

#define FW_UP_LABEL_H						(40)

// <---- DEVICE
#define FW_UP_DEV_LABEL_X					(26)
#define FW_UP_DEV_LABEL_Y					(64)
#define FW_UP_DEV_LABEL_W					(156)

#define FW_UP_DEV_CELL_X					(FW_UP_DEV_LABEL_X + FW_UP_DEV_LABEL_W + 10)
#define FW_UP_DEV_CELL_Y					(FW_UP_DEV_LABEL_Y)
#define FW_UP_DEV_CELL_W					(427)

// <---- FILE NAME
#define FW_UP_FILE_LIST_NUM					(6)
#define FW_UP_FILE_LIST_H					(41)

#define FW_UP_FILE_LABEL_X					(FW_UP_DEV_LABEL_X)
#define FW_UP_FILE_LABEL_Y					(FW_UP_DEV_LABEL_Y + FW_UP_LABEL_H + 3)
#define FW_UP_FILE_LABEL_W					(FW_UP_DEV_LABEL_W)

#define FW_UP_FILE_CELL_X					(FW_UP_DEV_CELL_X)
#define FW_UP_FILE_CELL_Y					(FW_UP_FILE_LABEL_Y)
#define FW_UP_FILE_CELL_W					(FW_UP_DEV_CELL_W)
#define FW_UP_FILE_CELL_H					(FW_UP_FILE_LIST_NUM*FW_UP_FILE_LIST_H)

// <---- UPGRADE, CLOSE BUTTON
#define FW_UP_LOAD_BTN_X					(146)
#define FW_UP_LOAD_BTN_Y					(FW_UP_WIN_SIZE_H - 74)
#define FW_UP_LOAD_BTN_W					(174)

#define FW_UP_CLOSE_BTN_X					(FW_UP_LOAD_BTN_X + FW_UP_LOAD_BTN_W + 6)
#define FW_UP_CLOSE_BTN_Y					(FW_UP_LOAD_BTN_Y)
#define FW_UP_CLOSE_BTN_W					(174)



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *dev_obj = NULL;
static NFOBJECT *fw_list_obj = NULL;
static NFOBJECT *upgrade_obj = NULL;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *fu_win;
static NFOBJECT *prog_pop = NULL;

static gchar (*g_file)[FILE_STRING_LENGTH];
static guint g_fileCnt = 0;
static gint g_media_cnt;
static MEDIA_INFO_T	*g_media_info;

#define CHEAT_CODE_SIZE 	4
//static int show_all = 0;
static int cheatkey_cnt = 0;
static int designkey_cnt = 0;
static int fac_init_cnt = 0;

static guint cheat_code[CHEAT_CODE_SIZE] = {
	KEYPAD_CH01,
	KEYPAD_CH02,
	KEYPAD_CH03,
	KEYPAD_CH04,
};

static guint design_code[CHEAT_CODE_SIZE] = {
	KEYPAD_CH02,
	KEYPAD_CH05,
	KEYPAD_CH08,
	RMC_NUM_0,
};

static guint fac_init_code[CHEAT_CODE_SIZE] = {
	KEYPAD_CH04,
	KEYPAD_CH03,
	KEYPAD_CH02,
	KEYPAD_CH01,
};

typedef enum _FWFILTER_E {
	FWFILTER_NORMAL		= 0,
	FWFILTER_ALL		= 1
} FWFILTER_E;

static char *remove_ext(char* mystr) {
    char *retstr;
    char *lastdot;
    if (mystr == NULL)
         return NULL;
    if ((retstr = malloc (strlen (mystr) + 1)) == NULL)
        return NULL;
    strcpy (retstr, mystr);
    lastdot = strrchr (retstr, '.');
    if (lastdot != NULL)
        *lastdot = '\0';
    return retstr;
}

static void _create_waitbox(void)
{
	wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
}

static void _remove_waitbox(void)
{
	if(wait_pop)
	{
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}
}

static void AudioPopup_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(obj);
	
	nfui_nfobject_hide(topwin);

	nftool_destroy_setup_window(topwin);
}

static void init_file(gchar (*file)[FILE_STRING_LENGTH])
{
	guint i;

	if(g_file) g_file = NULL;
	if(g_fileCnt) g_fileCnt = 0;

	if(file != NULL) {
		g_file = file;

		for(i=0; i<FILE_COUNT; i++) {
			//g_message("%s ::::::::::::::::::::::::: %p %d::::::::::::", __FUNCTION__, g_address[i], strlen(g_address[i]));
			if(strlen(g_file[i]))
				g_fileCnt++;
		}
	}
}

static gint add_file(gchar *file) 
{
	gint i;

	if(file) {
		//g_message("%s ::::::::::::::::::addrCnt %d  address : %s ", __FUNCTION__, g_addrCnt, address);
		if(g_fileCnt < FILE_COUNT) {
			for(i=0; i<=g_fileCnt; i++) {
				if(!strcmp(g_file[i], file)) 
					//return EMAIL_ADD_DUPLICATION;
					return 2;
			}

			g_stpcpy(g_file[g_fileCnt], file);
			g_fileCnt += 1;
			
			//return EMAIL_ADD_COMPLETE;
			return 1;
		}
	}
	//return EMAIL_ADD_FULL;
	return 0;
}

static void _update_dev_list(void)
{
	guint i, cnt = 0;

	if (g_media_info)
	{
		scm_free_media_list(g_media_info);
		nfui_combobox_remove_all(dev_obj);
	}

	g_media_cnt = 0;	
	g_media_info = scm_new_media_list(&g_media_cnt);

	for (i = 0; i < g_media_cnt; i++)
	{
		if (scm_get_media_type(g_media_info[i].id) == MTYPE_USB)
		{
//			if (strcmp(g_media_info[i].title, "XTICK") == 0) {
				nfui_combobox_append_data(dev_obj, g_media_info[i].title);
				cnt++;

				g_message("%s, %d, i:%d, id:%d, name:%s", __FUNCTION__, __LINE__, i, g_media_info[i].id, g_media_info[i].title);
//			}
		}
	}

	if (cnt == 0)
	{
		nfui_combobox_append_data(dev_obj, "NONE");
		nfui_nfobject_disable(fw_list_obj);				
		nfui_nfobject_disable(upgrade_obj);
	}
	else
	{
		nfui_nfobject_enable(fw_list_obj);				
		nfui_nfobject_enable(upgrade_obj);
	}

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(fw_list_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(upgrade_obj, GDK_EXPOSE, TRUE);

	nfui_make_key_hierarchy((NFWINDOW*)fu_win);

}

static void _update_fw_file_list(FWFILTER_E filter)
{
	guint i;
	gchar *dev = NULL;

	gchar **fw_list;
	gint fw_cnt;

	char *tmp_testmail;

	if (nfui_nfobject_is_shown(fw_list_obj) == FALSE)	return;

	tmp_testmail = nf_sysdb_get_str_nocopy("net.email.testmail");

	if (strcmp(tmp_testmail, "choissi@debug.com") == 0)
	{
        filter = FWFILTER_ALL;
	}

	dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

	for (i = 0; i < g_media_cnt; i++)
	{
		if(!strcmp(dev, g_media_info[i].title))
			break;
	}

	if (i == g_media_cnt)
	{
		g_warning("%s, %d, g_media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, g_media_cnt);
		return;
	}
	/*
	if (filter == FWFILTER_NORMAL)
		fw_list = scm_new_fw_list(g_media_info[i].id, 
				FF_FNAME_EXT | FF_MODEL | FF_BUYERCODE, FO_DEFAULT, &fw_cnt);
	else 
		fw_list = scm_new_fw_list(g_media_info[i].id, 
				FF_FNAME_EXT, FO_DEFAULT, &fw_cnt);
	*/
	fw_list = scm_new_audio_list(g_media_info[i].id, &fw_cnt);

	nfui_listbox_delete_all(NF_LISTBOX(fw_list_obj));

	if ((fw_cnt > 0) && (fw_list != NULL))
	{			
		for(i = 0; i < fw_cnt; i++)	
		{
			nfui_listbox_set_text(NF_LISTBOX(fw_list_obj), &(fw_list[i]));
		}

		scm_free_fw_list(fw_list);
	}
		
	nfui_signal_emit(fw_list_obj, GDK_EXPOSE, TRUE);
}


static void _update_info_dev_changed(void)
{
	_update_dev_list();
	_update_fw_file_list(FWFILTER_NORMAL);
}

static int _check_file(void)
{
	gchar dir[128];
	gchar path[256];
	gchar *file = NULL;
	gchar *dev = NULL;
	guint i, cnt = 0;

	if (g_media_info)
	{
		scm_free_media_list(g_media_info);
	}

	g_media_cnt = 0;	
	g_media_info = scm_new_media_list(&g_media_cnt);

	for (i = 0; i < g_media_cnt; i++)
	{
		if (scm_get_media_type(g_media_info[i].id) == MTYPE_USB)
		{
    		memset(dir, 0x00, sizeof(dir));
  		
    		if(scm_get_mounted_path(g_media_info[i].id, dir, 128) < 0) {
    			g_warning("%s :::: RETURN -1" , __FUNCTION__);
    			return 0;
    		}

            memset(path, 0x00, sizeof(path));
    		g_sprintf(path, "%s/%s", dir, ITX_FORCE_DB_INITIALIZE);
    		g_message("%s :::: full path : %s", __FUNCTION__, path);
    			
		    if(ifn_is_file_exist(path))
		    {
		        return 1;
		    }
		}
	}

    return 0;
}

static gint _get_mounted_path(gchar *mnt_path, int path_len)
{
	gchar *dev = NULL;
	guint i;

    if (nfui_nfobject_is_shown(fw_list_obj) == FALSE) return -1;

	dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

	for (i = 0; i < g_media_cnt; i++)
	{
		if(!strcmp(dev, g_media_info[i].title))
			break;
	}

	if (i == g_media_cnt)
	{
		g_warning("%s, %d, media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, g_media_cnt);
		return -1;
	}

	scm_get_mounted_path(g_media_info[i].id, mnt_path, path_len);

    return 0;
}

static gint _get_focused_file(gchar *file_name)
{
    if (nfui_nfobject_is_shown(fw_list_obj) == FALSE) return -1;

    strcpy(file_name, nfui_listbox_get_focus_text((NFLISTBOX*)fw_list_obj, 0));
    return 0;
}

static gint _check_valid_fwfile(gchar *mnt_path, gchar *filename)
{
    char full_name[256];
    gint ret = 0;

    memset(full_name, 0x00, sizeof(full_name));
    g_sprintf(full_name, "%s/%s", mnt_path, filename);

    g_message("%s, %d, fwfile:%s", __FUNCTION__, __LINE__, full_name);
    ret = scm_prepare_upgrade_validate_check(full_name, IRET_SCM_PREPARE_FWUP_VALIDATE);
    return ret;
}

static gint _backup_update_info(gchar *mnt_path)
{
    gint ret = 0;

    g_message("%s, %d, path:%s", __FUNCTION__, __LINE__, mnt_path);
	ret = scm_prepare_upgrade_data_backup("ramdisk", IRET_SCM_PREPARE_FWUP_DATABACKUP);
	return ret;
}

static gboolean post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		_update_fw_file_list(FWFILTER_NORMAL);
	}

	return FALSE;
}

static gboolean post_uploadbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == 3)			// mouse right button
			return FALSE;

		gint file_idx = 0;
		gint pos_x, pos_y, width;
		gboolean ret;

		file_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)fw_list_obj);

		if(file_idx < 0)
		{
			nftool_mbox(g_curwnd, "ERROR", "Please select a file", NFTOOL_MB_OK);
			return FALSE;
		}

		g_printf("SELECTED FILE INDEX:%d\n", file_idx);

		ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to upload an audio file?", NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK)
		{
			gchar strBuf[256];
			char mount_path[128];
			char file_name[128];
			char full_name[256];
			char dest_name[256];
			FILE    *fp_sour;    
   			FILE    *fp_dest;    
   			char     buff[1024];
			char     cmd[1024];
			char     *file_name_ext;
			struct   stat  file_info;
   			size_t   n_size;     
			gint ret;

            memset(mount_path, 0x00, sizeof(mount_path));
			ret = _get_mounted_path(mount_path, sizeof(mount_path));
			if (ret == -1) return FALSE;

            memset(file_name, 0x00, sizeof(file_name));
			ret = _get_focused_file(file_name);
			if (ret == -1) return FALSE;

			g_message("%s, %d, mount_path:%s", __FUNCTION__, __LINE__, mount_path);
			g_message("%s, %d, file_name:%s", __FUNCTION__, __LINE__, file_name);

			memset(full_name, 0x00, sizeof(full_name));
			g_sprintf(full_name, "%s/%s", mount_path, file_name);
			g_sprintf(dest_name, "/opt/%s", file_name);

			//add_file(file_name);

			if (!access(dest_name, F_OK))
			{
				g_message("file(w) already exist");
				goto EXIT;
			}

			if ((fp_sour  = fopen(full_name, "r")) == NULL)
			{
				g_message("file(r) open error");
				//goto EXIT;
				return FALSE;
			}
			else
			{
				stat(full_name, &file_info);
				if(file_info.st_size > 3145728) //3MB
				{
					g_message("file upload fail(file size: %d)", file_info.st_size);
					snprintf(strBuf, sizeof(strBuf)-1, lookup_string("The uploadable audio file size is %dMB."), FILE_SIZE);
					nftool_mbox_auto(g_curwnd, 1, "NOTICE", strBuf);
					fclose(fp_sour);
					return FALSE;
				}
			}

			if ((fp_dest  = fopen(dest_name, "w")) == NULL)
			{
				g_message("file(w) open error");
				//goto EXIT;
				fclose(fp_sour);
				return FALSE;
			}

			_create_waitbox();

			while( 0 < (n_size = fread( buff, 1, 1024, fp_sour)))
			{
				fwrite( buff, 1, n_size, fp_dest);
			}

			file_name_ext = remove_ext(dest_name);

			g_message("%s, %d, file_name:%s, file_name_ext:%s", __FUNCTION__, __LINE__, dest_name, file_name_ext);
			sprintf(cmd, "/usr/local/bin/sox \"%s\" -b 8 -c 1 -r 8k -t ul \"%s.raw\"", dest_name, file_name_ext);
			system(cmd);
			sprintf(cmd, "mv \"%s.raw\" \"%s\"", file_name_ext, dest_name);
			system(cmd);

			fclose(fp_sour);
			fclose(fp_dest);
			add_file(file_name);
			_remove_waitbox();
EXIT:
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "File is uploaded.");
			//g_curwnd = 0;
			//gtk_main_quit();
			AudioPopup_Destroy(obj);
		}
		else if(ret == NFTOOL_MB_CANCEL)
		{
			return FALSE;
		}

	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nftool_destroy_setup_window(topwin);
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	int result;

	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(event->type) {
		case GDK_EXPOSE:
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_BUTTON_RELEASE:
		break;					

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		
			uxm_unreg_imsg_event(obj, INFY_MEDIA_STATUS_CHANGED);	
			uxm_unreg_imsg_event(obj, INFY_FWUP_RATE);
			uxm_unreg_imsg_event(obj, IRET_SCM_UPGRADE_FW);
			uxm_unreg_imsg_event(obj, IRET_SCM_PREPARE_FWUP_VALIDATE);
			uxm_unreg_imsg_event(obj, IRET_SCM_PREPARE_FWUP_DATABACKUP);
			
			if (g_media_info)
			{
				scm_free_media_list(g_media_info);
				g_media_info = 0;
			}

		}
		break;

		case INFY_MEDIA_STATUS_CHANGED:
			_update_info_dev_changed();
		break;

		case IRET_SCM_UPGRADE_FW:
		{		
			if(prog_pop) {
				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;
				result = ((CMM_MESSAGE_T *)data)->param;

				if (result == 0)
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Complete.\nThe system will be reboot soon.", NFTOOL_MB_OK);
				else
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Fail.\nThe system will be reboot soon.", NFTOOL_MB_OK);

				scm_reboot_system(RR_FWUP, 0);
			}
		}
		break;

		case INFY_FWUP_RATE:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->param;
		
			if (prog_pop)
			{
				g_message("firmware upgrade rate = [%d]\n", ((CMM_MESSAGE_T *)data)->param);
				nftool_prog_pop_set_rate(prog_pop, rate);
			}
		}
		break;

		case IRET_SCM_PREPARE_FWUP_VALIDATE:
        {
			char mount_path[128];
			gint ret;

            ret = ((CMM_MESSAGE_T *)data)->param;
            if (ret != 0) return FALSE;

            memset(mount_path, 0x00, sizeof(mount_path));
			ret = _get_mounted_path(mount_path, sizeof(mount_path));
			if (ret == -1) {
                evt_send_to_local(IRET_SCM_PREPARE_FWUP_DATABACKUP, -1, 0, 0);
			    return FALSE;
            }

            _backup_update_info(mount_path);
        }
        break;

		case IRET_SCM_PREPARE_FWUP_DATABACKUP:
        {
			char mount_path[128];
			char file_name[128];
			char full_name[256];
			gint ret;

            ret = ((CMM_MESSAGE_T *)data)->param;
            if (ret != 0) return FALSE;

            memset(mount_path, 0x00, sizeof(mount_path));
			ret = _get_mounted_path(mount_path, sizeof(mount_path));
			if (ret == -1) {
    			evt_send_to_local(IRET_SCM_PREPARE_FWUP_CMPL, -1, 0, 0);
			    return FALSE;
            }

            memset(file_name, 0x00, sizeof(file_name));
			ret = _get_focused_file(file_name);
			if (ret == -1) {
    			evt_send_to_local(IRET_SCM_PREPARE_FWUP_CMPL, -1, 0, 0);
			    return FALSE;
            }

			g_message("%s, %d, mount_path:%s", __FUNCTION__, __LINE__, mount_path);
			g_message("%s, %d, file_name:%s", __FUNCTION__, __LINE__, file_name);

			memset(full_name, 0x00, sizeof(full_name));
			g_sprintf(full_name, "%s/%s", mount_path, file_name);
			scm_upgrade_fw(full_name, IRET_SCM_PREPARE_FWUP_CMPL);
        }
        break;

		default:
		break;
	}

	return FALSE;
}

static int _update_design()
{
	gchar *dev = NULL;
	char *file_name = NULL;

	char mount_path[128] = { 0, };
	char full_name[256];
	guint i;
	int vendor = 0;

	if (nfui_nfobject_is_shown(fw_list_obj) == FALSE)	return;

	vendor = VW_VendorCode_Open(g_curwnd);	
	printf("VENDOR CODE = %d\n", vendor);
	if (vendor <= 0) return;

	dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));
	for (i = 0; i < g_media_cnt; i++) {
		if(!strcmp(dev, g_media_info[i].title))
			break;
	}
	if (i == g_media_cnt)	return;

	scm_get_mounted_path(g_media_info[i].id, mount_path, 128);
	scm_update_design(mount_path, vendor, IRET_SCM_UPDATE_DESIGN);

	wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	return 0;
}

static gboolean _enable_fac_init(void)
{	
	if(DAL_get_fac_init_run() == FALSE)
	{
		DAL_set_fac_init_run(TRUE);
		DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);		
	}
	
	nftool_mbox(g_curwnd, "NOTICE", "Default value wizard is enabled.\nSystem will be reboot.", NFTOOL_MB_OK);
	
	scm_reboot_system(RR_NA, 0);

	return 0;
}

static gboolean _run_force_db_init()
{
    mb_type ret;


    if (_check_file()) {
    	ret = nftool_mbox(g_curwnd, "WARNING", "All configurations are initialized.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

        if (ret == NFTOOL_MB_OK){
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_cam.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_disp.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_audio.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_usr.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_net.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_sys.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_disk.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_rec.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_alarm.conf",1,3);
            proxy_system("/bin/rm -f /NFDVR/data/nf_sysdb_act.conf",1,3);
        }
    }
    
    return 0;
}

static gboolean pre_mainWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	switch(evt->type) {
	case NFEVENT_KEYPAD_PRESS:
	case NFEVENT_REMOCON_PRESS:
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

//		if (show_all == 1) return FALSE;

		if (cheat_code[cheatkey_cnt] == kpid) cheatkey_cnt++;
		else cheatkey_cnt = 0;

		if (design_code[designkey_cnt] == kpid) designkey_cnt++;
		else designkey_cnt = 0;

		if (fac_init_code[fac_init_cnt] == kpid) fac_init_cnt++;
		else fac_init_cnt = 0;
		
		if(cheatkey_cnt == CHEAT_CODE_SIZE) {
//			show_all = 1;
			cheatkey_cnt = 0;
			_update_fw_file_list(FWFILTER_ALL);
		}
		
		if(designkey_cnt == CHEAT_CODE_SIZE) {
			designkey_cnt = 0;
			_update_design();
		}

		if(fac_init_cnt == CHEAT_CODE_SIZE) {
			fac_init_cnt = 0;
			if(DAL_get_fac_init_func())
				_enable_fac_init();
		}
		
		break;
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

void VW_System_AudioUpload_Open(NFWINDOW *parent, gchar (*file)[FILE_STRING_LENGTH])
{
	NFOBJECT *main_fixed;
	NFOBJECT *obj;

	guint lc_size[] = {FW_UP_FILE_CELL_W, };
	guint li_size_w, li_size_h;
	guint i;
	gchar strBuf[128];

	init_file(file);
	g_message("%s => %s", __FUNCTION__, g_file[0]);

// <---- WINDOW
	fu_win = (NFOBJECT*)nfui_nfwindow_new(parent, FW_UP_WIN_SIZE_X, FW_UP_WIN_SIZE_Y, FW_UP_WIN_SIZE_W, FW_UP_WIN_SIZE_H);
	g_curwnd = fu_win;
	nfui_nfobject_modify_bg(fu_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(fu_win, post_main_win_event_handler);

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, FW_UP_WIN_SIZE_W, FW_UP_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LOAD", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 500, FW_UP_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 76, 4);

// <---- DEVICE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USB", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, FW_UP_DEV_LABEL_W, FW_UP_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_DEV_LABEL_X, FW_UP_DEV_LABEL_Y);

	dev_obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(dev_obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(dev_obj, FW_UP_DEV_CELL_W, FW_UP_LABEL_H);
	nfui_nfobject_show(dev_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, dev_obj, FW_UP_DEV_CELL_X, FW_UP_DEV_CELL_Y);
	nfui_regi_post_event_callback(dev_obj, post_device_event_handler);

// <---- DATA FILE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, FW_UP_FILE_LABEL_W, FW_UP_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_FILE_LABEL_X, FW_UP_FILE_LABEL_Y);
	
	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;
	
	fw_list_obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(fw_list_obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(fw_list_obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(fw_list_obj), FALSE);
	nfui_nfobject_set_size(fw_list_obj, FW_UP_FILE_CELL_W, FW_UP_FILE_CELL_H);
	nfui_nfobject_modify_bg(fw_list_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(fw_list_obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(fw_list_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, fw_list_obj, FW_UP_FILE_CELL_X, FW_UP_FILE_CELL_Y);


// <---- UPGRADE BUTTON
	upgrade_obj = nftool_normal_button_create_type1("UPLOAD", FW_UP_LOAD_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(upgrade_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(upgrade_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, upgrade_obj, FW_UP_LOAD_BTN_X, FW_UP_LOAD_BTN_Y);
	nfui_regi_post_event_callback(upgrade_obj, post_uploadbutton_event_handler);

// <---- CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", FW_UP_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_CLOSE_BTN_X, FW_UP_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	
	nfui_regi_pre_event_callback(fu_win, pre_mainWin_event_handler);
	nfui_nfwindow_add((NFWINDOW*)fu_win, main_fixed);
	nfui_run_main_event_handler(fu_win);
	nfui_nfobject_show(fu_win);

    _run_force_db_init();
	_update_info_dev_changed();
	
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);	
	uxm_reg_imsg_event(main_fixed, INFY_FWUP_RATE);
	uxm_reg_imsg_event(main_fixed, IRET_SCM_UPGRADE_FW);
	uxm_reg_imsg_event(main_fixed, IRET_SCM_PREPARE_FWUP_VALIDATE);
	uxm_reg_imsg_event(main_fixed, IRET_SCM_PREPARE_FWUP_DATABACKUP);

	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)fu_win);
	nfui_set_key_focus(upgrade_obj, TRUE);

	nfui_page_close(PGID_POPUPWND, fu_win);
	nfui_page_open(PGID_SYS_FWUP, fu_win, NULL);


	gtk_main();

	nfui_page_close(PGID_SYS_FWUP, fu_win);
}




