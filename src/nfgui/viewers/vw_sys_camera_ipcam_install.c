#include "nf_afx.h"

#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/nf_ui_color.h"
#include "../support/color.h"
#include "../support/util.h"

#include "../tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "nf_api_openmode.h"

#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"

#include "vw.h"
#include "vw_sys_camera_ipcam_install.h"
#include "vw_sys_camera_ipcam_install_search.h"
#include "vw_sys_net_info_detail_popup.h"

#define PAGE_FIXED_CNT          4
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define PAGE_STR1   "You need to set up the network properties on this device for adding cameras."
#define PAGE_STR2   "If you didn't set up the network properties before, then you need to set up the network properties."

#define WARN_STR	"In this mode, the video recording will be stopped automatically.\nDo you want to continue?"
	
#define	CD_LABEL_LEFT			(28)
#define	CD_LABEL_TOP			(20)
#define	CD_LABEL_HEIGHT			(40)

#define STATUS_NORMAL_COLOR				(COLOR_IDX(231))
#define STATUS_ERROR_COLOR				(COLOR_PRG_IDX(UX_COLOR_FF0000))

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_model_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_addr_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_status_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_setup_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;
static NFOBJECT *g_add_obj;

static IPCamLoginData g_pre_info[GUI_CHANNEL_CNT];
static IPCamLoginData g_post_info[GUI_CHANNEL_CNT];


static NFOpenmodeCamInfo *_find_matched_info(NFOpenmodeDeviceList *dlist, gint ch)
{
	NFOpenmodeCamInfo *info;
	gint i;

	info = dlist->head;
	if (info == 0) return NULL;

	for (i = 0; i < dlist->entry_cnt; i++)
	{
		if (info->ch == ch) return info;		
		
		info = info->next;
	}

	return NULL;
}

static gint _is_dhcp_channel()
{
	NFOpenmodeDeviceList *dlist = NULL;
	NFOpenmodeCamInfo *info = NULL;
	
	gint i, ch;

	dlist = nf_openmode_get_ch_list();
	if (dlist == 0) return 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		info = _find_matched_info(dlist, i);

		if ((info) && (info->state != OPENMODE_CAM_STATE_INIT)) 
		{
			if (info->is_dhcp) return 1;
		}
	}

	return 0;
}

static void _print_camdata_openmode()
{
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		g_message("[%s, %d] pre id :%s, post id:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].id, g_post_info[i].id,
			(g_strcmp0(g_pre_info[i].id, g_post_info[i].id) == 0) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre pw :%s, post pw:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].password, g_post_info[i].password,
			(g_strcmp0(g_pre_info[i].password, g_post_info[i].password) == 0) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre host :%s, post host:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].hostname, g_post_info[i].hostname,
			(g_strcmp0(g_pre_info[i].hostname, g_post_info[i].hostname) == 0) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre http port :%d, post http port:%d cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].http_port, g_post_info[i].http_port,
			(g_pre_info[i].http_port == g_post_info[i].http_port) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre rtsp port :%d, post rtsp port:%d cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].rtsp_port, g_post_info[i].rtsp_port,
			(g_pre_info[i].rtsp_port == g_post_info[i].rtsp_port) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre eth :%d, post eth:%d cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].ethernet, g_post_info[i].ethernet,
			(g_pre_info[i].ethernet == g_post_info[i].ethernet) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre model :%s, post model:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].model, g_post_info[i].model,
			(g_strcmp0(g_pre_info[i].model, g_post_info[i].model) == 0) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre mac :%s, post mac:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].mac, g_post_info[i].mac,
			(g_strcmp0(g_pre_info[i].mac, g_post_info[i].mac) == 0) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre vcam :%d, post vcam:%d cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].virtual_camera, g_post_info[i].virtual_camera,
			(g_pre_info[i].virtual_camera == g_post_info[i].virtual_camera) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre vcam cnt :%d, post vcam cnt:%d cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].vcam_cnt, g_post_info[i].vcam_cnt,
			(g_pre_info[i].vcam_cnt == g_post_info[i].vcam_cnt) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre vcam rtsp addr0 :%s, post vcam rtsp addr0:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].vcam_rtsp_addr[0], g_post_info[i].vcam_rtsp_addr[0],
			(g_strcmp0(g_pre_info[i].vcam_rtsp_addr[0], g_post_info[i].vcam_rtsp_addr[0]) == 0) ? "TRUE" : "FALSE");
		g_message("[%s, %d] pre vcam rtsp addr1 :%s, post vcam rtsp addr1:%s cmp res :%s", __FUNCTION__, __LINE__, 
			g_pre_info[i].vcam_rtsp_addr[1], g_post_info[i].vcam_rtsp_addr[1],
			(g_strcmp0(g_pre_info[i].vcam_rtsp_addr[1], g_post_info[i].vcam_rtsp_addr[1]) == 0) ? "TRUE" : "FALSE");
	}
}

static gint _get_login_info()
{
	NFOpenmodeDeviceList *dlist = NULL;
	NFOpenmodeCamInfo *info = NULL;
	
	gint i, ch;
	gchar str_buf[32];

    memset(str_buf, 0x00, sizeof(str_buf));

	dlist = nf_openmode_get_ch_list();
	if (dlist == 0) return -1;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		info = _find_matched_info(dlist, i);

		if ((info) && (info->state != OPENMODE_CAM_STATE_INIT))
		{	
			strncpy(g_post_info[i].id, info->u, 31);
			strncpy(g_post_info[i].password, info->p, 31);
			strncpy(g_post_info[i].hostname, info->hostname, 63);

			g_sprintf(str_buf, "%02x%02x%02x%02x%02x%02x", 
			           info->macaddr[0], info->macaddr[1], info->macaddr[2], info->macaddr[3], info->macaddr[4], info->macaddr[5]);

			strncpy(g_post_info[i].mac, str_buf, 12);

			g_post_info[i].http_port = info->http_port;
			g_post_info[i].rtsp_port = info->rtsp_port;
            g_post_info[i].virtual_camera = info->virtual_camera;
			g_post_info[i].vcam_cnt = info->vcam_cnt;

			strncpy(g_post_info[i].vcam_rtsp_addr[0], info->vcam_rtsp_addr[0], 255);
    		strncpy(g_post_info[i].vcam_rtsp_addr[1], info->vcam_rtsp_addr[1], 255);			

            if(info->virtual_camera)
            {                               
    			strncpy(g_post_info[i].model, info->model, 63);
            }
			
			if (strcmp(info->eth_dev, "eth1") == 0) g_post_info[i].ethernet = 1;
			else g_post_info[i].ethernet = 0;
		}
		else
		{
//			strcpy(g_post_info[i].id, "");
//			strcpy(g_post_info[i].password, "");
			strcpy(g_post_info[i].hostname, "");
			g_post_info[i].http_port = 0;
		}
		
		// _print_camdata_openmode();
	}

	return 0;
}

static gint _translate_status(gint status_in, gchar *status_out)
{
	if (status_in == OPENMODE_CAM_STATE_INVALID_IP)
	{
		strcpy(status_out, STR_INVALID_IP);
	}
	else if (status_in == OPENMODE_CAM_STATE_PW_CHANGE)
	{
		strcpy(status_out, STR_NEED_CHANGE_PW);
	}
	else if (status_in == OPENMODE_CAM_STATE_CONN_FAIL)
	{
		strcpy(status_out, STR_CONNECT_FAIL);
	}
	else if (status_in == OPENMODE_CAM_STATE_LOGIN_FAIL)
	{
		strcpy(status_out, STR_LOGIN_FAIL);
	}
	else if (status_in == OPENMODE_CAM_STATE_UNSUPPORTED)
	{
		strcpy(status_out, STR_UNSUPPORTED);
	}
	else if (status_in == OPENMODE_CAM_STATE_STREAM_FAIL)
	{
		strcpy(status_out, STR_STREAM_FAIL);
	}
	else if (status_in == OPENMODE_CAM_STATE_DISCOVERED)
	{
		strcpy(status_out, STR_CONNECTING);
	}		
	else if ((status_in == OPENMODE_CAM_STATE_DEV_INFO) || 
			(status_in == OPENMODE_CAM_STATE_DEV_INFO_ONVIF) ||
			(status_in == OPENMODE_CAM_STATE_VIRTUAL_CAMERA))
	{
		strcpy(status_out, STR_OK);
	}
	else
	{
		strcpy(status_out, STR_EMPTY);
	}
		
	return 0;
}

static gint _is_possible_setup(gint status_in, gboolean *pss)
{
/*
	if (status_in == OPENMODE_CAM_STATE_INVALID_IP)
	{
		*pss = TRUE;
	}
	else if (status_in == OPENMODE_CAM_STATE_PW_CHANGE)
	{
		*pss = TRUE;
	}	
	else if (status_in == OPENMODE_CAM_STATE_LOGIN_FAIL)
	{
		*pss = TRUE;
	}		
	else if ((status_in == OPENMODE_CAM_STATE_DEV_INFO) || 
			(status_in == OPENMODE_CAM_STATE_DEV_INFO_ONVIF))
	{
		*pss = TRUE;
	}	
	else
	{
		*pss = FALSE;
	}
*/
	*pss = TRUE;
	return 0;
}

static gint _update_search_list_model(gint ch, gchar *str)
{
	gint i;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_model_obj[ch]), str) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)g_model_obj[ch], str);
		nfui_signal_emit(g_model_obj[ch], GDK_EXPOSE, TRUE);
		return 0;
	}

	return -1;
}

static gint _update_search_list_addr(gint ch, gchar *str)
{
	gint i;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_addr_obj[ch]), str) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)g_addr_obj[ch], str);
		nfui_signal_emit(g_addr_obj[ch], GDK_EXPOSE, TRUE);
		return 0;
	}

	return -1;
}

static gint _update_search_list_status(gint ch, gchar *str)
{
	gint i;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[ch]), str) != 0)
	{
		if ((strcmp(str, STR_OK) == 0) || (strcmp(str, "-") == 0))
			nfui_nflabel_modify_fg((NFLABEL*)g_status_obj[ch], STATUS_NORMAL_COLOR);
		else
			nfui_nflabel_modify_fg((NFLABEL*)g_status_obj[ch], STATUS_ERROR_COLOR);
	
		nfui_nflabel_set_text((NFLABEL*)g_status_obj[ch], str);
		nfui_signal_emit(g_status_obj[ch], GDK_EXPOSE, TRUE);
		return 0;
	}

	return -1;
}

static gint _update_search_list_setup(gint ch, gboolean enable)
{
	if (nfui_nfobject_is_disabled(g_setup_obj[ch]) == enable)
	{
		if (enable) nfui_nfobject_enable(g_setup_obj[ch]);
		else		nfui_nfobject_disable(g_setup_obj[ch]);

		return 0;
	}

	return -1;
}

static gint _set_setup_button_image()
{
	gint i;
	gchar *state_str;
	GdkPixbuf *normal_img[4];	
	GdkPixbuf *error_img[4];	
	
	normal_img[0] = nfui_get_image_from_file(IMG_BT_SETTING_N, NULL);
	normal_img[1] = nfui_get_image_from_file(IMG_BT_SETTING_O, NULL);
	normal_img[2] = nfui_get_image_from_file(IMG_BT_SETTING_P, NULL);
	normal_img[3] = nfui_get_image_from_file(IMG_BT_SETTING_D, NULL);	

	error_img[0] = nfui_get_image_from_file(IMG_BT_SETTING_N2, NULL);
	error_img[1] = nfui_get_image_from_file(IMG_BT_SETTING_O, NULL);
	error_img[2] = nfui_get_image_from_file(IMG_BT_SETTING_P, NULL);
	error_img[3] = nfui_get_image_from_file(IMG_BT_SETTING_D, NULL);	
	
	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		state_str = nfui_nflabel_get_text((NFLABEL*)g_status_obj[i]);

		if ((strcmp(state_str, STR_OK) == 0) || (strcmp(state_str, "-") == 0))
			nfui_nfbutton_set_image(NF_BUTTON(g_setup_obj[i]), normal_img);
		else
			nfui_nfbutton_set_image(NF_BUTTON(g_setup_obj[i]), error_img);
	}

	return 0;
}

static gint _update_camera_info()
{
	NFOpenmodeDeviceList *dlist = 0;
	NFOpenmodeCamInfo *info = NULL;
	gint i;
	gchar strBuf[64];
	gint retVal = 0;
	gboolean pss_setup;

	dlist = nf_openmode_get_ch_list();
	if (dlist == 0) return -1;

	memset(strBuf, 0x00, sizeof(strBuf));

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		info = _find_matched_info(dlist, i);
	
		if ((info) && (info->state != OPENMODE_CAM_STATE_INIT))
		{
			_update_search_list_model(i, info->model);
			_update_search_list_addr(i, info->hostname);
			_translate_status(info->state, strBuf);		
			_update_search_list_status(i, strBuf);
			_is_possible_setup(info->state, &pss_setup);			
			_update_search_list_setup(i, pss_setup);
		}
		else
		{
			_update_search_list_model(i, "-");
			_update_search_list_addr(i, "-");		
			_update_search_list_status(i, "-");
			_update_search_list_setup(i, FALSE);		
		}
	}

	_set_setup_button_image();

	return 0;
}

static gboolean post_add_camera_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;
		mb_type ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = nftool_mbox(g_curwnd, "WARNING", WARN_STR, NFTOOL_MB_OKCANCEL);

		if (ret == NFTOOL_MB_CANCEL) return FALSE;

		scm_enter_openmode_install();
		VW_Open_IPCamInstallSearch_Page(g_curwnd);	
	}

	return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_add_camera2_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;
		mb_type ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = nftool_mbox(g_curwnd, "WARNING", WARN_STR, NFTOOL_MB_OKCANCEL);

		if (ret == NFTOOL_MB_CANCEL) return FALSE;

		scm_enter_openmode_install();
		VW_Open_IPCamInstallSearch_Page_ver2(g_curwnd);	
	}

	return FALSE;
}

static gboolean post_setup_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint i, info_idx;
		NFOpenmodeDeviceList *dlist = NULL;
		NFOpenmodeCamInfo *info = NULL;
		mb_type ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (g_setup_obj[i] == obj) break;
		}

		if (i >= GUI_CHANNEL_CNT) return FALSE;

		ret = nftool_mbox(g_curwnd, "WARNING", WARN_STR, NFTOOL_MB_OKCANCEL);

		if (ret == NFTOOL_MB_CANCEL) return FALSE;

		dlist = nf_openmode_get_ch_list();
		if (dlist == 0) return FALSE;

		info = _find_matched_info(dlist, i);

		scm_enter_openmode_install();
	
		g_message("%s, %d, preview_index:%d", __FUNCTION__, __LINE__, info->index);	
		info = nf_openmode_get_dlist_info_by_chlist_index(info->index); 	
		nf_openmode_set_preview(info->index, 0);
		VW_Open_Cam_Ipcam_Setup_Popup(g_curwnd, info);
	}

	return FALSE;
}

static gboolean post_go_network_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		SystemSetupNetwork_Open(g_curwnd, NULL, 0);
		vsm_live_stop();		
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		nf_openmode_cancel();
		g_memmove(g_post_info, g_pre_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);		
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_get_login_info();

		if (memcmp(g_post_info, g_pre_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT) == 0)
		{
			return FALSE;
		}
		
		if (_is_dhcp_channel())
			nftool_mbox(g_curwnd, "WARNING", "In case of camera that set as DHCP,\nthe camera may be disconnected by IP change.", NFTOOL_MB_OK);	
		
		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		
		for (i = 0; i < GUI_CHANNEL_CNT; i++)
			DAL_set_ipcam_login_data(g_post_info[i], i);		

		nf_openmode_apply();
		syscam_set_changeflag(1);
		
		g_memmove(g_pre_info, g_post_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		IPCamInstall_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) 
	{

	}
	else if (evt->type == INFY_IPCAM_INSTALL_NOTIFY)
	{
		gint i;
	
		_update_camera_info();		

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
//			nfui_signal_emit(g_setup_obj[i], GDK_EXPOSE, TRUE);			
		}
	}
	else if (evt->type == INFY_SHOW_OLD_VER_CAM_INSTALL)
	{
	    nfui_nfobject_show(g_add_obj);
	    nfui_signal_emit(g_add_obj, GDK_EXPOSE, TRUE);
	}
	else if (evt->type == GDK_DELETE)
	{
		scm_leave_openmode_install();
		uxm_unreg_imsg_event(obj, INFY_IPCAM_INSTALL_NOTIFY);	
		uxm_unreg_imsg_event(obj, INFY_SHOW_OLD_VER_CAM_INSTALL);	
	}

	return FALSE;
}


static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}



void init_IPCamInstall_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *ntb1;	
	NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];
    
	gint pos_x, pos_y;
	gint i;
	guint width[5] = {0, };
	gchar *strTitle[4] = {"CHANNEL", "MODEL", "ADDRESS", "STATUS"};
	gchar strBuf[STRING_SIZE_CAMTITLE];

	GdkPixbuf *stg_img[4];	
	gint img_w, img_h;
	
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


	stg_img[0] = nfui_get_image_from_file(IMG_BT_SETTING_N, NULL);
	stg_img[1] = nfui_get_image_from_file(IMG_BT_SETTING_O, NULL);
	stg_img[2] = nfui_get_image_from_file(IMG_BT_SETTING_P, NULL);
	stg_img[3] = nfui_get_image_from_file(IMG_BT_SETTING_D, NULL);	
	
	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

	g_curwnd = nfui_nfobject_get_top(parent);

	nf_openmode_init_detection_list();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	pos_x = CD_LABEL_LEFT;
	pos_y = CD_LABEL_TOP;	

	if (ivsc.vendor_code == 28 || ivsc.vendor_code == 128 || ivsc.vendor_code == 228 || ivsc.vendor_code == 43) 
	{
		obj = nftool_normal_button_create_type3("CAMERA ADD/DELETE", 300);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_add_camera_button_event_handler);
		g_add_obj = obj;
	}
	else
	{
		obj = nftool_normal_button_create_type3("CAMERA ADD/DELETE", 300);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	//	nfui_nfobject_show(obj);
	//	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 300 + 20, pos_y);
		nfui_regi_post_event_callback(obj, post_add_camera_button_event_handler);
		g_add_obj = obj;

		memset(strBuf, 0x00, sizeof(strBuf));
	//	g_sprintf(strBuf, "%s2", lookup_string("CAMERA ADD/DELETE"));

		obj = nftool_normal_button_create_type3("CAMERA ADD/DELETE", 300);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(obj);
	//	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 300 + 20, pos_y);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_add_camera2_button_event_handler);
	}

	width[0] = 200;
	width[1] = 300;
	width[2] = 300;
	width[3] = 300;
	width[4] = 40;

	pos_y += (CD_LABEL_HEIGHT+10);	

	ntb1 = (NFOBJECT*)nfui_nftable_new(5, 1, 2, 1, width, CD_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(ntb1);
	nfui_nffixed_put((NFFIXED*)content_fixed, ntb1, pos_x, pos_y);
	
	for (i = 0; i < 4; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));		
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, i, 0);
	}

	pos_y += CD_LABEL_HEIGHT + 1;
	
    size_w = 0;
    for (i = 0; i < 5; i++) {
        size_w += width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, pos_x, pos_y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(5, ROW_CNT_PER_PAGE, 2, 1, width, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		var_get_camtitle(strBuf, i);
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 1, row_num);
		g_model_obj[i] = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 2, row_num);
		g_addr_obj[i] = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 3, row_num);
		g_status_obj[i] = obj;

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), stg_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
		nfui_nfobject_disable(obj);		
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 4, row_num);
		nfui_regi_post_event_callback(obj, post_setup_button_event_handler);
		g_setup_obj[i] = obj;

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

	pos_y = MENU_V_INNER_H;	
	pos_y -= (32+32+10+40);	

	obj = nfui_nflabel_new_with_pango_font("[!]", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 40, 32);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));		
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR1, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 1000, 32);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));		
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+40, pos_y);

	pos_y += 32;	

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR2, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 1000, 32);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));		
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+40, pos_y);

	pos_y += (32+10);

	obj = nftool_normal_button_create_type3("GO TO NETWORK SETUP", 300);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_go_network_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);
	nfui_regi_post_event_callback(parent, post_page_event_handler);

	memset(g_pre_info, 0x00, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);
	memset(g_post_info, 0x00, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		DAL_get_ipcam_login_data(&g_pre_info[i], i);
	}

	g_memmove(g_post_info, g_pre_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);

	_update_camera_info();

	uxm_reg_imsg_event(content_fixed, INFY_IPCAM_INSTALL_NOTIFY);	
	uxm_monitor_on_imsg_event(content_fixed, INFY_IPCAM_INSTALL_NOTIFY);		

	uxm_reg_imsg_event(content_fixed, INFY_SHOW_OLD_VER_CAM_INSTALL);
}

gboolean IPCamInstall_tab_in_handler()
{

	return FALSE;
}

gboolean IPCamInstall_tab_out_handler()
{
	mb_type ret;
	gint i;

	_get_login_info();

	if (memcmp(g_post_info, g_pre_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT) == 0)
	{
		nf_openmode_set_preview(-1, 1);
		return FALSE;
	}

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
					NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
		if (_is_dhcp_channel())
			nftool_mbox(g_curwnd, "WARNING", "In case of camera that set as DHCP,\nthe camera may be disconnected by IP change.", NFTOOL_MB_OK);	

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
			DAL_set_ipcam_login_data(g_post_info[i], i);

		nf_openmode_apply();
		syscam_set_changeflag(1);
		
		g_memmove(g_pre_info, g_post_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);
	}
	else if (ret == NFTOOL_MB_CANCEL)
	{
		nf_openmode_cancel();
		g_memmove(g_post_info, g_pre_info, sizeof(IPCamLoginData)*GUI_CHANNEL_CNT);		
	}
	
	nf_openmode_set_preview(-1, 1);	
	
	return FALSE;
}
