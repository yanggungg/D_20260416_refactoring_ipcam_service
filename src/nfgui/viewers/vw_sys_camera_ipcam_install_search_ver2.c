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
#include "objects/nftab.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "nf_api_openmode.h"

#include "iux_afx.h"
#include "scm.h"
#include "vsm.h"
#include "smt.h"
#include "nvm.h"
#include "ocam.h"
#include "xmm.h"
#include "ipcam_thumbnail.h"

#include "vw.h"
#include "vw_desc.h"
#include "vw_vkeyboard.h"
#include "vw_sys_camera_ipcam_install.h"
#include "vw_sys_camera_ipcam_install_search_ver2.h"
#include "vw_nvm_nvr_login_popup.h"
#include "vw_sys_camera_ipcam_install_search_mode_popup.h"
#include "vw_sys_camera_ipcam_install_search_nvr_manager_popup.h"
#include "vw_sys_camera_ipcam_install_search_filter_menu.h"
#include "vw_sys_camera_ipcam_install_search_filter_popup.h"
#include "vw_sys_camera_ipcam_install_search_option_popup.h"
#include "vw_camera_install_recorder_data_save.h"

#define DBG_LEVEL		0
#define DBG_MODULE		"CAMERA INSTALL"

#define NORMAL_OUTLINE_COLOR		200
#define FOCUS_OUTLINE_COLOR			146
#define SELECT_OUTLINE_COLOR		147

#define FONT_COLOR_RED              (402)
#define FONT_COLOR_GREEN            (400)
#define FONT_COLOR_YELLOW           (433)

#define CIS_CAM_FIXED_W                 ((CIS_CARD_DEF_WIDTH * 4) + (15 * 5) + 16)
#define CIS_NVR_FIXED_W                 (CIS_CAM_FIXED_W)

#define CIS_CAM_PAGE_CNT                (10)
#define CIS_CAM_CARD_CNT_PER_PAGE       (16)
#define CIS_CAM_INFO_BUF_SIZE           (CIS_CAM_PAGE_CNT * CIS_CAM_CARD_CNT_PER_PAGE)

#define CIS_NVR_PAGE_CNT                (2)
#define CIS_NVR_CARD_CNT_PER_PAGE       (16)
#define CIS_NVR_INFO_BUF_SIZE           (CIS_NVR_PAGE_CNT * CIS_NVR_CARD_CNT_PER_PAGE)

#define CIS_CARD_DEF_WIDTH              (190)
#define CIS_CARD_DEF_HEIGHT             (168)
#define CIS_CARD_CONTENT_W              (CIS_CARD_DEF_WIDTH - 3)
#define CIS_CARD_LB_H                   (21)
#define CIS_CARD_THUMB_H                (CIS_CARD_DEF_HEIGHT - (CIS_CARD_LB_H*3) - 3)

#define LOCAL_IDX       (0)

typedef enum {
    OUTLINE_TYPE_FULL = 0,
    OUTLINE_TYPE_DOT,
}OUTLINE_TYPE_E;

enum {
    SCAN_INIT = 0,
    SCAN_RUNNING,
    SCAN_PAUSE
};

typedef struct _CARD_DATA_T CARD_DATA_T;
struct _CARD_DATA_T
{
    CAMERA_INFO_T cam_info;
    gint thumb_idx;
    gint is_selected;
}; 

typedef struct _CIS_MOVE_CARD{
    gint is_support;
    gint pressed;
    gint moved;
    NFOBJECT *from_card;
    NFOBJECT *to_card;
    gint from_x;
    gint from_y;
    gint to_x;
    gint to_y;
}CIS_MOVE_CARD;

static CIS_MOVE_CARD g_move_card;

static CARD_DATA_T g_cam_card[CIS_CAM_INFO_BUF_SIZE];
static CARD_DATA_T g_nvr_card[32];
static CAM_THUMBNAIL_T g_thumbnail[CIS_CAM_INFO_BUF_SIZE];
static CAMERA_INFO_T *g_org_caminfo = NULL;

static IPCamInstOptData g_opt_data;

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_wait_box = NULL;
static NFOBJECT *g_cam_card_obj[CIS_CAM_CARD_CNT_PER_PAGE];
static NFOBJECT *g_cam_btn_login[CIS_CAM_CARD_CNT_PER_PAGE];
static NFOBJECT *g_nvr_card_obj[CIS_CAM_CARD_CNT_PER_PAGE];
static NFOBJECT *g_nvr_btn_login[CIS_CAM_CARD_CNT_PER_PAGE];
static NFOBJECT *g_cam_page[CIS_CAM_PAGE_CNT];
static NFOBJECT *g_cam_page_num[CIS_CAM_PAGE_CNT];
static NFOBJECT *g_nvr_page[CIS_NVR_PAGE_CNT];
static NFOBJECT *g_nvr_page_num[CIS_NVR_PAGE_CNT];
static NFOBJECT *g_cam_arrow[2];
static NFOBJECT *g_nvr_arrow[2];
static NFOBJECT *g_nvr_list = NULL;
static NFOBJECT *g_nvr_reload = NULL;
static NFOBJECT *g_nvr_multi = NULL;
static NFOBJECT *g_nvr_setting = NULL;
static NFOBJECT *g_assigned_cam_cnt_obj = NULL;
static NFOBJECT *g_found_cam_cnt_obj = NULL;
static NFOBJECT *g_cam_filter = NULL;

static guint g_update_tmr = 0;
static guint g_ch_change_tmr = 0;
static guint g_ch_assign_tmr = 0;
static guint g_nvr_page_move_tmr = 0;
static gint g_cur_nvr_idx = 0;
static gint g_move_ch_mask = 0;
static gint g_assign_ch_mask = 0;
static gint g_from_ch = -1;
static gint g_assign_cam = -1;
static gint g_is_scanning = SCAN_INIT;
static gint g_multi_mode = 0;
static gint g_mode_select = 1;
static gint g_conn_try_cnt = 0;


/////////////////////////////////////////////////
// private function
//

static void _print_cam_info(CAMERA_INFO_T *data)
{
    DMSG(1, "addr : %s", data->url);
    DMSG(1, "port : %d", data->port);
    DMSG(1, "model : %s", data->model);
    DMSG(1, "state : %d", data->state);
    DMSG(1, "ch : %d", data->ch);
}

static void _show_waitbox(gchar *strcont1, gchar *strcont2)
{
    if (g_wait_box) return FALSE;
    
	g_wait_box = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", strcont1, strcont2);
}

static void _hide_waitbox()
{
    if (g_wait_box) {
        nftool_remove_waitbox(g_wait_box);
        g_wait_box = NULL;
    }
}

static void _init_db()
{
    memset(&g_opt_data, 0x00, sizeof(IPCamInstOptData));

    DAL_get_ipcam_install_opt_data(&g_opt_data);
}

static gint _get_cur_nvr()
{
    return g_cur_nvr_idx;
}

static void _set_cur_nvr(gint idx)
{
    g_cur_nvr_idx = idx;
}

static void _init_local_ocam()
{
    gint i;

    if (!g_org_caminfo) return;

    for (i = 0; i < nvm_get_active_ch_cnt(LOCAL_IDX); i++)
    {
        ocam_add_cam(&g_org_caminfo[i]);
        ocam_reg_cam(&g_org_caminfo[i]);
    }
}

static gint _set_filter_btn_image(gint expose)
{
	GdkPixbuf* filter[NFOBJECT_STATE_COUNT];

    if (ocam_check_filter()) {
    	filter[0] = nfui_get_image_from_file("bt_pop_filter_i.png", NULL);
    }
    else {
    	filter[0] = nfui_get_image_from_file(IMG_BT_POP_FILTER_N, NULL);
    }
    
	filter[1] = nfui_get_image_from_file(IMG_BT_POP_FILTER_O, NULL);
	filter[2] = nfui_get_image_from_file(IMG_BT_POP_FILTER_P, NULL);
	filter[3] = nfui_get_image_from_file(IMG_BT_POP_FILTER_D, NULL);

	nfui_nfbutton_set_image((NFBUTTON*)g_cam_filter, filter);
	if (expose) nfui_signal_emit(g_cam_filter, GDK_EXPOSE, TRUE);

	return 0;
}

static void _scan_camera(gint expose)
{
    ocam_clear_filter();
    _set_filter_btn_image(expose);
    
    nf_openmode_scan_camera();
}

static gint _translate_status(gint status_in, gchar *status_out, guint *font_color)
{
	if (status_in == OPENMODE_CAM_STATE_INVALID_IP)
	{
		strcpy(status_out, STR_INVALID_IP);
		*font_color = FONT_COLOR_RED;
	}
	else if (status_in == OPENMODE_CAM_STATE_PW_CHANGE)
	{
		strcpy(status_out, STR_NEED_CHANGE_PW);
		*font_color = FONT_COLOR_RED;
	}
	else if (status_in == OPENMODE_CAM_STATE_CONN_FAIL)
	{
		strcpy(status_out, STR_CONNECT_FAIL);
		*font_color = FONT_COLOR_RED;
	}
	else if (status_in == OPENMODE_CAM_STATE_LOGIN_FAIL)
	{
		strcpy(status_out, STR_LOGIN_FAIL);
		*font_color = FONT_COLOR_RED;
	}
	else if (status_in == OPENMODE_CAM_STATE_UNSUPPORTED)
	{
		strcpy(status_out, STR_UNSUPPORTED);
		*font_color = FONT_COLOR_RED;
	}
	else if (status_in == OPENMODE_CAM_STATE_STREAM_FAIL)
	{
		strcpy(status_out, STR_STREAM_FAIL);
		*font_color = FONT_COLOR_RED;
	}
	else if (status_in == OPENMODE_CAM_STATE_DISCOVERED)
	{
		strcpy(status_out, STR_CONNECTING);
		*font_color = FONT_COLOR_RED;
	}		
	else if ((status_in == OPENMODE_CAM_STATE_DEV_INFO) || 
			(status_in == OPENMODE_CAM_STATE_DEV_INFO_ONVIF) ||
			(status_in == OPENMODE_CAM_STATE_VIRTUAL_CAMERA))
	{
		strcpy(status_out, STR_NO_IMAGE);
		*font_color = FONT_COLOR_GREEN;
	}
	else
	{
		strcpy(status_out, STR_EMPTY);
	}
		
	return 0;
}

static gboolean _get_already_assigned()
{
    return FALSE;
}

static gint _get_info_idx_by_cam_card(NFOBJECT *card)
{
    gboolean toggled = FALSE;
    gint cur_page;
    gint i, info_idx, card_idx;
            
    for (i = 0; i < CIS_CAM_PAGE_CNT; i++) {
        toggled = nfui_radio_button_get_toggled(g_cam_page[i]);
        if (toggled) break;
    }

    if (i == CIS_CAM_PAGE_CNT) return -1;
    cur_page = i;

    for (i = 0; i < CIS_CAM_CARD_CNT_PER_PAGE; i++) {
        if (g_cam_card_obj[i] == card) break;
    }
    
    if (i == CIS_CAM_CARD_CNT_PER_PAGE) return -1;
    card_idx = i;
    
    info_idx = (cur_page * CIS_CAM_CARD_CNT_PER_PAGE) + card_idx;

//    g_message("###yanggungg : %s, %d, info_idx : %d", __FUNCTION__, __LINE__, info_idx);

    return info_idx;
}

static gint _get_info_idx_by_nvr_card(NFOBJECT *card)
{
    gboolean toggled = FALSE;
    gint cur_page;
    gint i, info_idx, card_idx;
            
    for (i = 0; i < CIS_NVR_PAGE_CNT; i++) {
        toggled = nfui_radio_button_get_toggled(g_nvr_page[i]);
        if (toggled) break;
    }

    if (i == CIS_NVR_PAGE_CNT) return -1;
    cur_page = i;

    for (i = 0; i < CIS_NVR_CARD_CNT_PER_PAGE; i++) {
        if (g_nvr_card_obj[i] == card) break;
    }
    
    if (i == CIS_NVR_CARD_CNT_PER_PAGE) return -1;
    card_idx = i;
    
    info_idx = (cur_page * CIS_NVR_CARD_CNT_PER_PAGE) + card_idx;

//    g_message("###yanggungg : %s, %d, info_idx : %d", __FUNCTION__, __LINE__, info_idx);

    return info_idx;
}

static gint _draw_card_out_line(NFOBJECT *obj, gint color, OUTLINE_TYPE_E type)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;
    gint8 dash_style[] = {5, 5};

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

    if (type == OUTLINE_TYPE_FULL)
    {
    	gdk_gc_set_line_attributes(gc, 3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);
	}
	else 
	{
    	gdk_gc_set_line_attributes(gc, 3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(NORMAL_OUTLINE_COLOR));
    	gdk_draw_rectangle(drawable, gc, FALSE, gap_x-5, gap_y-5, CIS_CARD_DEF_WIDTH + 10, CIS_CARD_DEF_HEIGHT + 10);
    	
    	gdk_gc_set_line_attributes(gc, 3, GDK_LINE_ON_OFF_DASH, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
        gdk_gc_set_dashes (gc, 5, dash_style, G_N_ELEMENTS(dash_style));
	}

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));
	gdk_draw_rectangle(drawable, gc, FALSE, gap_x-5, gap_y-5, CIS_CARD_DEF_WIDTH + 10, CIS_CARD_DEF_HEIGHT + 10);
	
	nfui_nfobject_gc_unref(gc);

	return 0;
}

static NFOpenmodeCamInfo *_find_matched_info_by_idx(NFOpenmodeDeviceList *dlist, gint f_index)
{
	NFOpenmodeCamInfo *info;
	gint i;

	if (f_index >= dlist->entry_cnt) return NULL;

	info = dlist->head;

	for (i = 0; i < dlist->entry_cnt; i++)
	{
		if (info->index == f_index) return info;		
		
		info = info->next;
	}

	return NULL;
}

static gint _get_thumbnail_idx(CAMERA_INFO_T *cam_info)
{
    gint i;

    for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++)
    {
        if ((strcmp(g_thumbnail[i].hostname, cam_info->url) == 0) && (g_thumbnail[i].port == cam_info->port)) return i;
    }

    if (i == CIS_CAM_INFO_BUF_SIZE) return -1;
    
    return 0;
}

static gint _get_cam_selected_camera_cnt()
{
    gint i;
    gint cnt = 0;
    
    for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++) {
        if (g_cam_card[i].is_selected) cnt++;
    }

    return cnt;
}

static gint _get_cam_enable_page_cnt()
{
	NFOpenmodeDeviceList *dlist = NULL;
	gint page_cnt = 0;
	gint remainder = 0;
	gint i;

    for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++)
    {
        if (strlen(g_cam_card[i].cam_info.url) <= 0) break;
    }
    
    page_cnt = (i) / CIS_CAM_CARD_CNT_PER_PAGE;
    remainder = (i) % CIS_CAM_CARD_CNT_PER_PAGE;

    if (remainder) page_cnt++;
    if (page_cnt > CIS_CAM_PAGE_CNT) page_cnt = CIS_CAM_PAGE_CNT;

	return page_cnt;
}

static gint _get_nvr_enable_page_cnt()
{
	gint page_cnt = 0;
	gint remainder = 0;

    page_cnt = nvm_get_active_ch_cnt(_get_cur_nvr()) / CIS_NVR_CARD_CNT_PER_PAGE;
    remainder = nvm_get_active_ch_cnt(_get_cur_nvr()) % CIS_NVR_CARD_CNT_PER_PAGE;

    if (remainder) page_cnt++;

	return page_cnt;
}

static gint _get_cam_cur_page()
{
    gboolean toggled;
    gint i = 0;

	for (i = 0; i < CIS_CAM_PAGE_CNT; i++) {
	    if (nfui_nfobject_is_disabled(g_cam_page[i])) continue;
	    
	    toggled = nfui_radio_button_get_toggled((NFBUTTON*)g_cam_page[i]);
	    if (toggled) break;
    }

    if (i == CIS_CAM_PAGE_CNT) return 0;

    return i;
}

static gint _get_cam_last_page()
{
    return _get_cam_enable_page_cnt() - 1;
}

static gint _update_nvr_card_data(gint idx)
{
    CAMERA_INFO_T *cam_info = NULL;
    gint i;
    
    memset(g_nvr_card, 0x00, sizeof(CARD_DATA_T) * 32);

    for (i = 0; i < nvm_get_active_ch_cnt(idx); i++)
    {
        cam_info = nvm_get_cam_info(idx, i);
        if (!cam_info) continue;
        
        g_memmove(&g_nvr_card[i].cam_info, cam_info, sizeof(CAMERA_INFO_T));
        g_nvr_card[i].thumb_idx = _get_thumbnail_idx(&g_nvr_card[i].cam_info);
        
        ifree(cam_info);
    }

    return 0;
}

static gint _reg_to_ocam(gint idx)
{
    CAMERA_INFO_T *cam_info = NULL;
    gint i;
    
    memset(g_nvr_card, 0x00, sizeof(CARD_DATA_T) * 32);

    for (i = 0; i < nvm_get_active_ch_cnt(idx); i++)
    {
        cam_info = nvm_get_cam_info(idx, i);
        if (!cam_info) continue;

        ocam_reg_cam(cam_info);
        
        ifree(cam_info);
    }

    return 0;
}

static gint _get_nvr_page_by_ch(gint ch)
{
    gint page = 0;

    page = ch / CIS_NVR_CARD_CNT_PER_PAGE;

    return page;
}

static gint _get_nvr_cur_page()
{
    gboolean toggled;
    gint i = 0;

	for (i = 0; i < CIS_NVR_PAGE_CNT; i++) {
	    if (nfui_nfobject_is_disabled(g_nvr_page[i])) continue;
	    
	    toggled = nfui_radio_button_get_toggled((NFBUTTON*)g_nvr_page[i]);
	    if (toggled) break;
    }

    if (i == CIS_NVR_PAGE_CNT) return 0;

    return i;
}

static gint _get_nvr_empty_ch_cnt()
{
    gint ch;
    gint cnt = 0;

    for (ch = 0; ch < nvm_get_active_ch_cnt(_get_cur_nvr()); ch++)
    {
        if (nvm_check_empty_ch(_get_cur_nvr(), ch)) cnt++;
    }

    return cnt;
}

static gint _get_nvr_selected_camera_cnt()
{
    gint cnt = 0;
    gint i;

    for (i = 0; i < nvm_get_active_ch_cnt(_get_cur_nvr()); i++)
    {
        if (g_nvr_card[i].is_selected) cnt++;
    }

    return cnt;
}

static gint _get_inside_nvr_card_idx(gint mx, gint my)
{
    gint off_x, off_y;
    gint i;

    if (mx < 0 && my < 0) return 0;

    for (i = 0; i < CIS_NVR_CARD_CNT_PER_PAGE; i++)
    {
        nfui_nfobject_get_offset(g_nvr_card_obj[i], &off_x, &off_y);
        
        if ((mx >= off_x) && (mx <= (off_x + g_nvr_card_obj[i]->width)) &&
            (my >= off_y) && (my <= (off_y + g_nvr_card_obj[i]->height)))
        {
            return i;
        }
    }

    if (i == CIS_NVR_CARD_CNT_PER_PAGE) return -1;
}

static void _set_active_reload_obj(gint cur_idx, gint expose)
{
    if (cur_idx == 0) {
        nfui_nfobject_disable(g_nvr_reload);
    }
    else {
        nfui_nfobject_enable(g_nvr_reload);
    }

    if (expose)
        nfui_signal_emit(g_nvr_reload, GDK_EXPOSE, TRUE);
}

static void _set_page_by_num(NFOBJECT *obj[], gint page_num, gint page_tot)
{
    gint i;
    
	nfui_radio_button_set_toggled((NFBUTTON*)obj[page_num], TRUE);

	for (i = 0; i < page_tot; i++)
	    nfui_signal_emit(obj[i], GDK_EXPOSE, TRUE);
}

static gint _translate_search_result(gchar *str_out)
{
    NFOpenmodeDeviceList *dlist = NULL;
	NFOpenmodeCamInfo *info = NULL;
    gint found = 0, nvr_cnt = 0;
    gint i;

    dlist = nf_openmode_get_list();
    
	if (!dlist) {
	    g_sprintf(str_out, "%s : %d / %s : %d", lookup_string("CONNECTED"), 0, lookup_string("DISCOVERED"), 0);	
	    return FALSE;
    }
    	
    info = _find_matched_info_by_idx(dlist, 0);

	for (i = 0; i < dlist->entry_cnt; i++) {
	    if(info->dev_cate == OPENMODE_DEV_NVR) {
	        nvr_cnt++;
        }
        
	    info = info->next;
	}

	found = dlist->entry_cnt - nvr_cnt;

	g_sprintf(str_out, "%s : %d / %s : %d", lookup_string("CONNECTED"), dlist->recognized_cnt, lookup_string("DISCOVERED"), found);		
	return 0;
}

static gint _set_search_result(gchar *str_assign, gchar *str_found, gint expose)
{
	if (str_assign)
	{
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_assigned_cam_cnt_obj), str_assign) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_assigned_cam_cnt_obj, str_assign);
			if (expose) nfui_signal_emit(g_assigned_cam_cnt_obj, GDK_EXPOSE, TRUE);
		}
	}

	if (str_found)
	{
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_found_cam_cnt_obj), str_found) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_found_cam_cnt_obj, str_found);
			if (expose) nfui_signal_emit(g_found_cam_cnt_obj, GDK_EXPOSE, TRUE);
		}
	}
	
	return 0;
}

static gint _check_changed()
{
    CAMERA_INFO_T *cam_info = NULL;
    gint i;
    gint ret = 0;

    if (!g_org_caminfo) return 0;

    cam_info = nvm_get_cam_info_all(_get_cur_nvr());

    if (!cam_info) return 0;

    for (i = 0; i < nvm_get_active_ch_cnt(_get_cur_nvr()); i++)
    {
        g_message("%s, %d, org_caminfo[%d]  url : %s, model : %s", __FUNCTION__, __LINE__, i,  g_org_caminfo[i].url, g_org_caminfo[i].model);
        g_message("%s, %d, cur_caminfo[%d]  url : %s, model : %s", __FUNCTION__, __LINE__, i, cam_info[i].url, cam_info[i].model);
    }
    
    if (!memcmp(g_org_caminfo, cam_info, sizeof(CAMERA_INFO_T) * nvm_get_active_ch_cnt(_get_cur_nvr()))) ret = 0;
    else ret = 1;

    ifree(cam_info);

    return ret;
}

static gint _unassign_camera(gint ch)
{
    gint i;
    gint last_ch = 0;

    if (ch == -1) return -1;

    nvm_unassign_cam(_get_cur_nvr(), ch);

    for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++)
    {
        if ((strcmp(g_cam_card[i].cam_info.url, g_nvr_card[ch].cam_info.url) == 0) && 
            (g_cam_card[i].cam_info.port == g_nvr_card[ch].cam_info.port))
        {
            ocam_unassign(g_cam_card[i].cam_info.url);
            g_cam_card[i].cam_info.ch = -1;
            break;
        }
    }
    last_ch = ch;

    return last_ch;
}

static gint _unassign_all_camera()
{
    gint ch = 0, i;
    gint last_ch = 0;

    for (ch = 0; ch < nvm_get_active_ch_cnt(_get_cur_nvr()); ch++)
    {
        nvm_unassign_cam(_get_cur_nvr(), ch);

        for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++)
        {
            if ((strcmp(g_cam_card[i].cam_info.url, g_nvr_card[ch].cam_info.url) == 0) &&
                (g_cam_card[i].cam_info.port == g_nvr_card[ch].cam_info.port))
            {
                ocam_unassign(g_cam_card[i].cam_info.url);
                g_cam_card[i].cam_info.ch = -1;
                break;
            }
        }
        last_ch = ch;
    }

    return last_ch;
}

static gint _unassign_selected_camera()
{
    gint ch = 0, i;
    gint last_ch = 0;

    for (ch = 0; ch < nvm_get_active_ch_cnt(_get_cur_nvr()); ch++)
    {
        if (g_nvr_card[ch].is_selected) {
            nvm_unassign_cam(_get_cur_nvr(), ch);

            for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++)
            {
                if ((strcmp(g_cam_card[i].cam_info.url, g_nvr_card[ch].cam_info.url) == 0) && 
                    (g_cam_card[i].cam_info.port == g_nvr_card[ch].cam_info.port))
                {
                    ocam_unassign(g_cam_card[i].cam_info.url);
                    g_cam_card[i].cam_info.ch = -1;
                    break;
                }
            }
            last_ch = ch;
        }
    }

    return last_ch;
}

static gint _assign_camera(gint info_idx)
{
    gint last_ch = 0;
    gint ch;

    for (ch = 0; ch < nvm_get_active_ch_cnt(_get_cur_nvr()); ch++)
    {
        if (nvm_check_empty_ch(_get_cur_nvr(), ch)) 
        {
            ocam_assign(g_cam_card[info_idx].cam_info.url);
            nvm_assign_cam(_get_cur_nvr(), &g_cam_card[info_idx].cam_info, ch);
            g_cam_card[info_idx].cam_info.ch = ch;
            g_cam_card[info_idx].is_selected = 0;
        	last_ch = ch;
            break;
        }
    }
    
    if (ch == nvm_get_active_ch_cnt(_get_cur_nvr())) return -1;

    return last_ch;	    
}

static gint _assign_selected_camera()
{
    gint info_idx;
    gint last_ch = 0;
    gint ch;

    for (info_idx = 0; info_idx < CIS_CAM_INFO_BUF_SIZE; info_idx++) 
    {
        if (g_cam_card[info_idx].is_selected) 
        {
            for (ch = 0; ch < nvm_get_active_ch_cnt(_get_cur_nvr()); ch++)
            {
                if (nvm_check_empty_ch(_get_cur_nvr(), ch)) 
                {
                    ocam_assign(g_cam_card[info_idx].cam_info.url);
                    nvm_assign_cam(_get_cur_nvr(), &g_cam_card[info_idx].cam_info, ch);
                    g_cam_card[info_idx].cam_info.ch = ch;
                    g_cam_card[info_idx].is_selected = 0;
                	last_ch = ch;
                    break;
                }
            }
            if (ch == nvm_get_active_ch_cnt(_get_cur_nvr())) break;
        }
    }

    return last_ch;	    
}

static gint _assign_auto()
{
    gint info_idx;
    gint last_ch = 0;
    gint ch;

    for (info_idx = 0; info_idx < CIS_CAM_INFO_BUF_SIZE; info_idx++) 
    {
        if (g_cam_card[info_idx].cam_info.ch == -1) 
        {
            for (ch = 0; ch < nvm_get_active_ch_cnt(_get_cur_nvr()); ch++)
            {
                if (nvm_check_empty_ch(_get_cur_nvr(), ch)) 
                {
                    ocam_assign(g_cam_card[info_idx].cam_info.url);
                    nvm_assign_cam(_get_cur_nvr(), &g_cam_card[info_idx].cam_info, ch);
                    g_cam_card[info_idx].cam_info.ch = ch;
                	last_ch = ch;
                    break;
                }
            }
            if (ch == nvm_get_active_ch_cnt(_get_cur_nvr())) break;
        }
    }

    return last_ch;	    
}

static void _crop_text(gchar *src, gchar *desc)
{
    gchar tmp[128] = {0,};
    gint str_w, str_len;
    gint i = 0;

    str_len = strlen(src);
    str_w = nfutil_string_width(0, NULL, nffont_get_pango_font(NFFONT_MINI_SEMI_5), src, NORMAL_SPACING);

    if (str_w < CIS_CARD_CONTENT_W) {
        strcpy(desc, src);
        return;
    }

    strcpy(tmp, src);
    while (1)
    {
        str_w = nfutil_string_width(0, NULL, nffont_get_pango_font(NFFONT_MINI_SEMI_5), tmp, NORMAL_SPACING);
        if (str_w > CIS_CARD_CONTENT_W) {
            tmp[--str_len] = '\0';
            continue;
        }
        else {
            tmp[--str_len] = '.';
            tmp[--str_len] = '.';
            tmp[--str_len] = '.';
            strcpy(desc, tmp);
            str_w = nfutil_string_width(0, NULL, nffont_get_pango_font(NFFONT_MINI_SEMI_5), desc, NORMAL_SPACING);
            break;
        }
    };
}

static gint _update_camera_model(gint card_idx, gchar *model, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;
	gchar buf[128];

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_cam_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_cam_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_cam_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(435));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+2, CIS_CARD_CONTENT_W, CIS_CARD_LB_H);

	if (strlen(model) > 0)
	{
	    _crop_text(model, buf);
        nfutil_draw_text_with_pango_eng(NULL, NULL, &UX_COLOR(435), drawable, gc, buf, gap_x+2, gap_y+4, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
                                    nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(FONT_COLOR_YELLOW), NFALIGN_LEFT, 0);
	}
	else
	{
        nfutil_draw_text_with_pango_eng(NULL, NULL, &UX_COLOR(435), drawable, gc, "", gap_x+2, gap_y+2, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
                                    nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(295), NFALIGN_LEFT, 0);
	}
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_camera_url(gint card_idx, gchar *url, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_cam_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_cam_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_cam_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(435));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+23, CIS_CARD_CONTENT_W, CIS_CARD_LB_H);
	
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(435), drawable, gc, url, gap_x+2, gap_y+25, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(295), NFALIGN_LEFT, 0);
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_camera_client(gint card_idx, gint assign_cnt, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;
    gchar buf[64];

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_cam_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_cam_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_cam_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(435));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+44, CIS_CARD_CONTENT_W, CIS_CARD_LB_H);
	
    if (!g_multi_mode) return 0;

    memset(buf, 0x00, sizeof(buf));
    if (assign_cnt) g_sprintf(buf, "+%d", assign_cnt);

	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(435), drawable, gc, buf, gap_x+2, gap_y+46, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(295), NFALIGN_LEFT, 0);
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_camera_status(gint card_idx, gint status, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint bg_color, font_color;
	gint gap_x, gap_y;
    gchar buf[64];

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_cam_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_cam_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_cam_card_obj[card_idx], &gap_x, &gap_y);

#if 1
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y + 65, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H);
	
    memset(buf, 0x00, sizeof(buf));
    _translate_status(status, buf, &font_color);
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(436), drawable, gc, buf, gap_x+2, gap_y + 65 + ((CIS_CARD_THUMB_H-20)/2), CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
#else
    if (status != OPENMODE_CAM_STATE_LOGIN_FAIL)
    {
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
    	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y + 65, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H);
    	
        memset(buf, 0x00, sizeof(buf));
        _translate_status(status, buf, &font_color);
    	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(436), drawable, gc, buf, gap_x+2, gap_y + 65 + ((CIS_CARD_THUMB_H-20)/2), CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
    		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
    		
        nfui_nfobject_hide(g_cam_btn_login[card_idx]);
        nfui_signal_emit(g_cam_card_obj[card_idx], GDK_EXPOSE, TRUE);
	}
	else
	{
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
    	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y + 65, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H);
    	
        nfui_nfobject_show(g_cam_btn_login[card_idx]);
        nfui_signal_emit(g_cam_btn_login[card_idx], GDK_EXPOSE, TRUE);
	}
#endif
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_camera_select_line(gint card_idx, gint is_selected, gboolean expose)
{
    if (!expose) return 0;
    
    if (is_selected) {
        _draw_card_out_line(g_cam_card_obj[card_idx], SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    }
    else {
        _draw_card_out_line(g_cam_card_obj[card_idx], NORMAL_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    }

    return 0;
}

static gint _update_camera_frame_line(gint card_idx, gint assigned_ch, gboolean expose)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint gap_x, gap_y;

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_cam_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_cam_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_cam_card_obj[card_idx], &gap_x, &gap_y);

	gdk_gc_set_line_attributes(gc,
			3,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_BEVEL);
    
    if (assigned_ch != -1)
    {
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(400));    //378
    	gdk_draw_rectangle(drawable, gc, FALSE, gap_x, gap_y, CIS_CARD_DEF_WIDTH, CIS_CARD_DEF_HEIGHT);	
    }
    else
    {
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(434));
    	gdk_draw_rectangle(drawable, gc, FALSE, gap_x, gap_y, CIS_CARD_DEF_WIDTH, CIS_CARD_DEF_HEIGHT);	
    }

	nfui_nfobject_gc_unref(gc);

	return 0;
}

static gint _update_camera_thumbnail(gint card_idx, gint thumb_idx, gboolean expose)
{
	GdkDrawable *drawable;
	GdkGC *gc;
    GInputStream *stream = NULL;
    GdkPixbuf *pixbuf;
	gint gap_x, gap_y;

    if (!expose) return 0;
    if (thumb_idx == -1) return 0;
    
    drawable = nfui_nfobject_get_window(g_cam_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_cam_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_cam_card_obj[card_idx], &gap_x, &gap_y);

    stream = g_memory_input_stream_new_from_data(g_thumbnail[thumb_idx].jpec_data, g_thumbnail[thumb_idx].jpec_size, NULL);
    pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H, FALSE, NULL, NULL);
    
    if (!pixbuf) {
        g_object_unref(stream);
    	nfui_nfobject_gc_unref(gc);
    	return -1;
    }
    
    gdk_draw_pixbuf(drawable, gc, pixbuf, 0, 0, gap_x + 2, gap_y + 65, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

    g_object_unref(stream);
    g_object_unref(pixbuf);
	nfui_nfobject_gc_unref(gc);

	return 0;
}

static gint _update_dev_camera_model(gint card_idx, gchar *model, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;
	gchar buf[128];

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_nvr_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_nvr_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_nvr_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(435));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+2, CIS_CARD_CONTENT_W, CIS_CARD_LB_H);

	if (strlen(model) > 0)
	{
	    _crop_text(model, buf);
    	nfutil_draw_text_with_pango_eng(NULL, NULL, &UX_COLOR(435), drawable, gc, buf, gap_x+2, gap_y+4, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
    		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(FONT_COLOR_YELLOW), NFALIGN_LEFT, 0);
	}
	else
	{
    	nfutil_draw_text_with_pango_eng(NULL, NULL, &UX_COLOR(435), drawable, gc, "", gap_x+2, gap_y+2, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
    		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(295), NFALIGN_LEFT, 0);
	}
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_dev_camera_url(gint card_idx, gchar *url, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_nvr_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_nvr_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_nvr_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(435));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+23, CIS_CARD_CONTENT_W, CIS_CARD_LB_H);
	
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(435), drawable, gc, url, gap_x+2, gap_y+25, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(295), NFALIGN_LEFT, 0);
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_dev_camera_client(gint card_idx, gchar *client, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_nvr_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_nvr_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_nvr_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(435));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+44, CIS_CARD_CONTENT_W, CIS_CARD_LB_H);
	
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(435), drawable, gc, client, gap_x+2, gap_y+46, CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(295), NFALIGN_LEFT, 0);
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_dev_camera_status(gint card_idx, gint status, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint bg_color, font_color;
	gint gap_x, gap_y;
    gchar buf[64];

    if (!expose) return 0;
    
    drawable = nfui_nfobject_get_window(g_nvr_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_nvr_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_nvr_card_obj[card_idx], &gap_x, &gap_y);

#if 1
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
    	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y + 65, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H);
    	
        memset(buf, 0x00, sizeof(buf));
        _translate_status(status, buf, &font_color);
    	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(436), drawable, gc, buf, gap_x+2, gap_y + 65 + ((CIS_CARD_THUMB_H-20)/2), CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
    		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
#else
    if (status != OPENMODE_CAM_STATE_LOGIN_FAIL)
    {
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
    	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y + 65, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H);
    	
        memset(buf, 0x00, sizeof(buf));
        _translate_status(status, buf, &font_color);
    	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(436), drawable, gc, buf, gap_x+2, gap_y + 65 + ((CIS_CARD_THUMB_H-20)/2), CIS_CARD_CONTENT_W, CIS_CARD_LB_H, 
    		nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
    		
        nfui_nfobject_hide(g_nvr_btn_login[card_idx]);
        nfui_signal_emit(g_nvr_card_obj[card_idx], GDK_EXPOSE, TRUE);
	}
	else
	{
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
    	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y + 65, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H);
    	
        nfui_nfobject_show(g_nvr_btn_login[card_idx]);
        nfui_signal_emit(g_nvr_btn_login[card_idx], GDK_EXPOSE, TRUE);
	}
#endif
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_dev_camera_select_line(gint card_idx, gint is_selected, gboolean expose)
{
    if (!expose) return 0;
    
    if (is_selected) {
        _draw_card_out_line(g_nvr_card_obj[card_idx], SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    }
    else {
        _draw_card_out_line(g_nvr_card_obj[card_idx], NORMAL_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    }

    return 0;
}

static gint _update_dev_camera_thumbnail(gint card_idx, gint thumb_idx, gboolean expose)
{
	GdkDrawable *drawable;
	GdkGC *gc;
    GInputStream *stream = NULL;
    GdkPixbuf *pixbuf;
	gint gap_x, gap_y;

    if (!expose) return 0;
    if (thumb_idx == -1) return 0;
    
    drawable = nfui_nfobject_get_window(g_nvr_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_nvr_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_nvr_card_obj[card_idx], &gap_x, &gap_y);

    stream = g_memory_input_stream_new_from_data(g_thumbnail[thumb_idx].jpec_data, g_thumbnail[thumb_idx].jpec_size, NULL);
    pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, CIS_CARD_CONTENT_W, CIS_CARD_THUMB_H, FALSE, NULL, NULL);
    
    if (!pixbuf) {
        g_object_unref(stream);
    	nfui_nfobject_gc_unref(gc);
    	return -1;
    }
    
    gdk_draw_pixbuf(drawable, gc, pixbuf, 0, 0, gap_x + 2, gap_y + 65, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

    g_object_unref(stream);
    g_object_unref(pixbuf);
	nfui_nfobject_gc_unref(gc);

	return 0;
}

static gint _update_dev_camera_empty(gint card_idx, gint ch, gboolean expose)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;
	gchar buf[8];

    if (!expose) return 0;

    memset(buf, 0x00, sizeof(buf));
    if (ch < nvm_get_active_ch_cnt(_get_cur_nvr()))
        g_sprintf(buf, "%d", ch+1);
    
    drawable = nfui_nfobject_get_window(g_nvr_card_obj[card_idx]);
    gc = nfui_nfobject_get_gc(g_nvr_card_obj[card_idx]);

    nfui_nfobject_get_offset((NFOBJECT*)g_nvr_card_obj[card_idx], &gap_x, &gap_y);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(436));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+2, CIS_CARD_CONTENT_W, CIS_CARD_DEF_HEIGHT - 3);
	
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(436), drawable, gc, buf, gap_x+2, gap_y+2, CIS_CARD_CONTENT_W, CIS_CARD_DEF_HEIGHT - 3, 
		nffont_get_pango_font(NFFONT_XLARGE_NORMAL_3), &UX_COLOR(295), NFALIGN_CENTER, 0);
    
    nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _update_cam_card(gint card_idx, CARD_DATA_T *card_data, gboolean expose)
{
    gint ret = 0;
    
    if (strlen(card_data->cam_info.url) > 0)
    {
        _update_camera_model(card_idx, card_data->cam_info.model, expose);
        _update_camera_url(card_idx, card_data->cam_info.url, expose);
        _update_camera_client(card_idx, nvm_get_assigned_cam_cnt_by_url(card_data->cam_info.url), expose);
        if (card_data->thumb_idx != -1) {
            ret = _update_camera_thumbnail(card_idx, card_data->thumb_idx, expose);
            if (ret == -1) {
                _update_camera_status(card_idx, card_data->cam_info.state, expose);
            } 
        }
        else {
            _update_camera_status(card_idx, card_data->cam_info.state, expose);
        }
//        _update_camera_frame_line(card_idx, card_data->cam_info.ch, expose);
        _update_camera_select_line(card_idx, card_data->is_selected, expose);
    //    _update_camera_frame_line(card_idx, cam_data->temp_assigned_ch, expose);
    }
    else
    {
        _update_camera_model(card_idx, "", expose);
        _update_camera_url(card_idx, "", expose);
        _update_camera_client(card_idx, 0, expose);
        _update_camera_status(card_idx, 0, expose);
        _update_camera_select_line(card_idx, 0, expose);
        _update_camera_frame_line(card_idx, -1, expose);
    //    _update_camera_frame_line(card_idx, cam_data->temp_assigned_ch, expose);
    }

    return 0;
}

static gint _update_nvr_card(gint card_idx, gint ch, CARD_DATA_T *card_data, gboolean expose)
{
    gint ret = 0;
    
    if (strlen(card_data->cam_info.url) > 0)
    {
        _update_dev_camera_model(card_idx, card_data->cam_info.model, expose);
        _update_dev_camera_url(card_idx, card_data->cam_info.url, expose);
        _update_dev_camera_client(card_idx, "", expose);
        if (card_data->thumb_idx != -1) {
            ret = _update_dev_camera_thumbnail(card_idx, card_data->thumb_idx, expose);
            if (ret == -1) {
                _update_dev_camera_status(card_idx, card_data->cam_info.state, expose);
            }
        }
        else {
            _update_dev_camera_status(card_idx, card_data->cam_info.state, expose);
        }
        _update_dev_camera_select_line(card_idx, card_data->is_selected, expose);
    }
    else
    {
        _update_dev_camera_empty(card_idx, ch, expose);
        /*
        _update_dev_camera_model(card_idx, "", expose);
        _update_dev_camera_url(card_idx, "", expose);
        _update_dev_camera_client(card_idx, "", expose);
        _update_dev_camera_status(card_idx, 0, expose);
        */
        _update_dev_camera_select_line(card_idx, 0, expose);
    }

    return 0;
}

static gint _update_cam_page(gint page_idx, gboolean expose)
{
    gint i, info_idx;
    gchar buf[64];

    info_idx = page_idx * CIS_CAM_CARD_CNT_PER_PAGE;
//    g_message("###NEW CIS : %s, %d, page_idx : %d, info_idx : %d", __FUNCTION__, __LINE__, page_idx, info_idx);
    
    for (i = 0; i < CIS_CAM_CARD_CNT_PER_PAGE; i++)
    {
        _update_cam_card(i, &g_cam_card[info_idx], expose);
        info_idx++;
    }

    return 0;
}

static gint _update_nvr_page(gint page_idx, gboolean expose)
{
    gint i, info_idx;
    gint cur_nvr;

    info_idx = page_idx * CIS_NVR_CARD_CNT_PER_PAGE;
//    g_message("###NEW CIS : %s, %d, page_idx : %d, info_idx : %d", __FUNCTION__, __LINE__, page_idx, info_idx);
    
    for (i = 0; i < CIS_NVR_CARD_CNT_PER_PAGE; i++)
    {
//        if (info_idx >= nvm_get_active_ch_cnt(_get_cur_nvr())) break;
        
        _update_nvr_card(i, info_idx, &g_nvr_card[info_idx], expose);
        info_idx++;
    }

    return 0;
}

static gint _enable_cam_page(gboolean expose)
{
	gint page_cnt = _get_cam_enable_page_cnt();
	gint i;

	for (i = 0; i  < page_cnt; i++)
	{
	    nfui_nfobject_enable(g_cam_page[i]);
	    nfui_nfobject_enable(g_cam_page_num[i]);
	    if (expose) nfui_signal_emit(g_cam_page[i], GDK_EXPOSE, TRUE);
	    if (expose) nfui_signal_emit(g_cam_page_num[i], GDK_EXPOSE, TRUE);
	}

	if (page_cnt == CIS_CAM_PAGE_CNT) return page_cnt;

	for (i = page_cnt; i < CIS_CAM_PAGE_CNT; i++)
	{
	    nfui_nfobject_disable(g_cam_page[i]);
	    nfui_nfobject_disable(g_cam_page_num[i]);
	    if (expose) nfui_signal_emit(g_cam_page[i], GDK_EXPOSE, TRUE);
	    if (expose) nfui_signal_emit(g_cam_page_num[i], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _enable_nvr_page(gboolean expose)
{
	gint page_cnt = _get_nvr_enable_page_cnt();
	gint i;

	for (i = 0; i  < page_cnt; i++)
	{
	    nfui_nfobject_enable(g_nvr_page[i]);
	    nfui_nfobject_enable(g_nvr_page_num[i]);
	    if (expose) nfui_signal_emit(g_nvr_page[i], GDK_EXPOSE, TRUE);
	    if (expose) nfui_signal_emit(g_nvr_page_num[i], GDK_EXPOSE, TRUE);
	}

	if (page_cnt == CIS_NVR_PAGE_CNT) return page_cnt;

	for (i = page_cnt; i < CIS_NVR_PAGE_CNT; i++)
	{
	    nfui_nfobject_disable(g_nvr_page[i]);
	    nfui_nfobject_disable(g_nvr_page_num[i]);
	    if (expose) nfui_signal_emit(g_nvr_page[i], GDK_EXPOSE, TRUE);
	    if (expose) nfui_signal_emit(g_nvr_page_num[i], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static void _change_nvr(gint idx)
{
//    _set_active_reload_obj(idx, 1);
    _update_nvr_card_data(idx);
    _enable_nvr_page(TRUE);
	_set_page_by_num(g_nvr_page, 0, CIS_NVR_PAGE_CNT);

    _update_nvr_page(_get_nvr_cur_page(), TRUE);
                    	
	if (g_org_caminfo) {
	    ifree(g_org_caminfo);
	    g_org_caminfo = NULL;
	}
	g_org_caminfo = nvm_get_cam_info_all(idx);
}

static gint _set_cam_card()
{
    CAMERA_INFO_T tmp[1024];
    int cnt = 0;
    gint i;

    ocam_get_caminfo(OCAM_B_ALL, tmp, &cnt, 1024);

    memset(g_cam_card, 0x00, sizeof(g_cam_card));

    for (i = 0; i < cnt; i++)
    {
		memcpy(&g_cam_card[i], &tmp[i], sizeof(CAMERA_INFO_T));
        g_cam_card[i].thumb_idx = _get_thumbnail_idx(&g_cam_card[i].cam_info);
        _print_cam_info(&g_cam_card[i].cam_info);
    }

    return 0;
}

static void _make_camera_data()
{
    NFOpenmodeDeviceList *dlist;
	NFOpenmodeCamInfo *info = NULL;
	CAMERA_INFO_T cam_info;
	gchar filename[256];
	gint i, j = 0;

	dlist = nf_openmode_get_list();
    info = _find_matched_info_by_idx(dlist, 0);
    
	for (i = 0; i < dlist->entry_cnt; i++) {
	    if ((info->dev_cate != OPENMODE_DEV_CAM) || (info->state == OPENMODE_CAM_STATE_INIT) || (strlen(info->hostname) <= 0)) {
	        info = info->next;
	        continue;
        }

        memset(&cam_info, 0x00, sizeof(CAMERA_INFO_T));
        
        strcpy(cam_info.id, info->u);
        strcpy(cam_info.pw, info->p);
        strcpy(cam_info.model, info->model);
        strcpy(cam_info.url, info->hostname);
        strcpy(cam_info.hostname, info->hostname);
        cam_info.port = info->http_port;
        cam_info.state = info->state;
        cam_info.ch = info->ch;
        cam_info.virtual_camera = info->virtual_camera;
        strcpy(cam_info.vcam_rtsp_addr[0], info->vcam_rtsp_addr[0]);
        strcpy(cam_info.vcam_rtsp_addr[1], info->vcam_rtsp_addr[1]);

		ocam_add_cam(&cam_info); 

	    info = info->next;
	}

    _set_cam_card();
}

static gint _make_nvr_list(gint expose)
{
    XMM_CONN_STATUS status;
    gchar url[256];
    gchar *sysid = NULL;
    gchar buf[512];
    gint i;
    gint http_port;
    
    nfui_combobox_remove_all((NFCOMBOBOX*)g_nvr_list);

    for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {
        memset(buf, 0x00, sizeof(buf));
        
        http_port = nvm_get_nvr_conn_info(i, url, sizeof(url));
        if (strlen(url) == 0) continue;

#if 0
        sysid = nvm_get_sysid(i);
        if (sysid) {
            g_sprintf(buf, "%s - %s", url, sysid);
            ifree(sysid);
            sysid = NULL;
        }
        else {
            g_sprintf(buf, "%s", url);
        }
#else
        if (i == 0) {
            g_sprintf(buf, "%s - (%d/%d)", lookup_string("LOCAL"), nvm_get_assigned_cam_cnt(i), nvm_get_active_ch_cnt(i));
        }
        else {
            status = xmm_get_conn_status(url, http_port);
            g_message("[%s, %d] url : %s, status : %d", __FUNCTION__, __LINE__, url, status);
            
            if (status == XMM_CONN_LOGIN_FAIL)
                g_sprintf(buf, "%s - %s", url, lookup_string("LOGIN FAIL"));
            else if (status == XMM_CONN_CONN_FAIL)
                g_sprintf(buf, "%s - %s", url, lookup_string("CONNECTION FAIL"));
            else
                g_sprintf(buf, "%s - (%d/%d)", url, nvm_get_assigned_cam_cnt(i), nvm_get_active_ch_cnt(i));
        }
#endif
        nfui_combobox_append_data((NFCOMBOBOX*)g_nvr_list, buf);

        if (i == 0 && !g_multi_mode) {
            memset(buf, 0x00, sizeof(buf));
            g_sprintf(buf, "Find connectable recorders");
            nfui_combobox_append_data((NFCOMBOBOX*)g_nvr_list, buf);
            break;
        }
    }

    if (expose) nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
    
	return 0;
}

static void _update_nvr_list_cam_cnt(gint expose)
{
    _make_nvr_list(expose);
}

static void _make_nvr_data()
{
    NFOpenmodeDeviceList *dlist;
	NFOpenmodeCamInfo *info = NULL;
    NVR_DATA_T nvr_data;
	gint i;

	dlist = nf_openmode_get_list();
	if (!dlist) return 0;
	
    info = _find_matched_info_by_idx(dlist, 0);

	for (i = 0; i < dlist->entry_cnt; i++) {
	    if(info->dev_cate == OPENMODE_DEV_NVR) {
        	memset(&nvr_data, 0x00, sizeof(NVR_DATA_T));
            strcpy(nvr_data.hostname, info->hostname);
            nvr_data.http_port = info->http_port;

            nvm_check_supported_nvm(i, info->hostname, info->http_port);
        }
        
	    info = info->next;
	}

	nvm_update_oneself_cam_info();

    return;
}

static void _refresh_cam_info()
{
    _make_camera_data();
}

static gboolean _proc_update_page(gpointer data)
{
	gint i;

	_enable_cam_page(TRUE);
	_enable_nvr_page(TRUE);
	
	_set_page_by_num(g_nvr_page, 0, CIS_NVR_PAGE_CNT);
	_set_page_by_num(g_cam_page, 0, CIS_CAM_PAGE_CNT);

	_update_cam_page(0, TRUE);
	_update_nvr_page(0, TRUE);

    nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
    
    return FALSE;
}

static gboolean _proc_redraw(gpointer data)
{
    gint page_idx;

	_update_nvr_page(_get_nvr_cur_page(), TRUE);
	_update_cam_page(_get_cam_cur_page(), TRUE);
	
    return FALSE;
}

static gboolean _proc_ch_assign(gpointer data)
{
	if (g_assign_ch_mask == 0)
	{
        g_assign_ch_mask = 1;
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, NF_CURSOR_FLEUR);
	}

	g_assign_cam = GPOINTER_TO_INT(data);

    return FALSE;
}

static gboolean _proc_ch_change(gpointer data)
{
	if (g_move_ch_mask == 0)
	{
        g_move_ch_mask = 1;
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, NF_CURSOR_FLEUR);
	}

	g_from_ch = GPOINTER_TO_INT(data);

    return FALSE;
}

static gboolean _proc_nvr_page_move(gpointer data)
{
    gint idx;
    gint i;

    idx = GPOINTER_TO_INT(data);
    _set_page_by_num(g_nvr_page, idx, CIS_NVR_PAGE_CNT);
    _update_nvr_page(idx, TRUE);

    return TRUE;
}

static gint _finalize_ch_assign(gint to_ch)
{
    CARD_DATA_T tmp;

    if (g_assign_cam == -1) return 0;
    
    if (g_ch_assign_tmr) {
        g_source_remove(g_ch_assign_tmr);
        g_ch_assign_tmr = 0;
    }
    
    g_assign_ch_mask = 0;

    if (to_ch == -1) return 0;
    
    if (to_ch >= nvm_get_active_ch_cnt(_get_cur_nvr())) {
        g_assign_cam = -1;
        return 0;
    }

    ocam_assign(g_cam_card[g_assign_cam].cam_info.url);
    nvm_assign_cam(_get_cur_nvr(), &g_cam_card[g_assign_cam].cam_info, to_ch);
    g_cam_card[g_assign_cam].cam_info.ch = to_ch;
    g_cam_card[g_assign_cam].is_selected = 0;
    
    _update_nvr_card_data(_get_cur_nvr());
    _update_cam_page(_get_cam_cur_page(), TRUE);
    _update_nvr_page(_get_nvr_cur_page(), TRUE);
    g_assign_cam = -1;

	_update_nvr_list_cam_cnt(TRUE);
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
	nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _finalize_ch_change(gint to_ch)
{
    CARD_DATA_T tmp;

    if (g_from_ch == -1) return 0;
    
    if (g_ch_change_tmr) {
        g_source_remove(g_ch_change_tmr);
        g_ch_change_tmr = 0;
    }
    
    g_move_ch_mask = 0;

    if ((to_ch >= nvm_get_active_ch_cnt(_get_cur_nvr())) || (to_ch == -1)) {
        g_from_ch = -1;
        return 0;
    }

    if (g_from_ch != to_ch) {
        memset(&tmp, 0x00, sizeof(CARD_DATA_T));
        
        g_memmove(&tmp, &g_nvr_card[g_from_ch], sizeof(CARD_DATA_T));
        g_memmove(&g_nvr_card[g_from_ch], &g_nvr_card[to_ch], sizeof(CARD_DATA_T));
        g_memmove(&g_nvr_card[to_ch], &tmp, sizeof(CARD_DATA_T));

        nvm_swap_ch(_get_cur_nvr(), g_from_ch, to_ch);
        _update_nvr_page(_get_nvr_cur_page(), TRUE);
        g_from_ch = -1;
    }

    return 0;
}

static gint _set_ready_move_card(NFOBJECT *obj, gint mx, gint my)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    gint off_x, off_y;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset(obj, &off_x, &off_y);

    g_move_card.from_card = obj;
	g_move_card.from_x = mx;
	g_move_card.from_y = my;
    g_move_card.to_card = NULL;
	g_move_card.to_x = -1;
	g_move_card.to_y = -1;

	return 0;
}

static gint _set_moving_card(NFOBJECT *obj, gint mx, gint my)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);
	
    if (g_move_card.moved == 0)
    {
        if ((abs(mx - g_move_card.from_x) < 5) && (abs(my - g_move_card.from_y) < 5)) return -1;

        g_move_card.moved = 1;
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, NF_CURSOR_FLEUR);
    }

    // for debounce
    if ((abs(g_move_card.to_x-mx) < 10) && (abs(g_move_card.to_y-my) < 10)) return 0;
            	
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_394E4A)));
    gdk_draw_rectangle(drawable, gc, TRUE, abs(mx - g_move_card.from_x)+2, abs(my - g_move_card.from_y)+2, obj->width, obj->height);
    
	nfui_nfobject_gc_unref(gc);

    return 0;
}

static gint _set_complete_move_card(NFOBJECT *obj, gint mx, gint my)
{
    nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, NF_CURSOR_ARROW);

    g_move_card.moved = 0;
    g_move_card.pressed = 0;
    g_move_card.from_card = NULL;
    g_move_card.to_card = NULL;
    g_move_card.from_x = -1;
    g_move_card.from_y = -1;
    g_move_card.to_x = -1;
    g_move_card.to_y = -1;

    return 0;
}

static gint _set_account_with_master_acc()
{
    NVR_DATA_T *ndata;
    XMM_SESSION_INFO_T *sinfo;
    gchar m_id[64], m_pw[64];
    gchar sid[128];
    gint i;
    gint res;

    memset(m_id, 0x00, sizeof(m_id));
    memset(m_pw, 0x00, sizeof(m_pw));

    ndata = nvm_get_nvr_data(0);
    sinfo = xmm_get_session_info(ndata->hostname, ndata->http_port);
    strcpy(m_id, sinfo->id);
    strcpy(m_pw, sinfo->pw);
    
    ifree(ndata);
    ifree(sinfo);

    for (i = 1; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {
        ndata = nvm_get_nvr_data(i);
        if (strlen(ndata->hostname) > 0) {
            if (!xmm_get_session_id(ndata->hostname, ndata->http_port, sid, 128)) {
                xmm_update_nvr_info(ndata->hostname, ndata->http_port, m_id, m_pw);
            }
        }
        
        ifree(ndata);
    }
    
    return 0;
}

static gint _try_to_connect()
{
    NVR_DATA_T *ndata;
    gint i;
    gint call_cnt = 0;

    for (i = 1; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {        
        if (nvm_connect_nvr(i) != -1)
            call_cnt++;
    }

    g_message("[%s, %d] call_cnt : %d", __FUNCTION__, __LINE__, call_cnt);
    
    return call_cnt;
}

static gboolean _complete_conn_nvr(gpointer data)
{
    _hide_waitbox();
    g_multi_mode = 1;
    _make_nvr_list(TRUE);

    nfui_nfobject_enable(g_nvr_multi);
    nfui_signal_emit(g_nvr_multi, GDK_EXPOSE, TRUE);

    return FALSE;
}

static gboolean post_all_unassign_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    gint selected_cnt = 0;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
        
        _unassign_all_camera();
        _update_nvr_card_data(_get_cur_nvr());
        _update_nvr_page(_get_nvr_cur_page(), TRUE);        
        _update_cam_page(_get_cam_cur_page(), TRUE);

        _update_nvr_list_cam_cnt(TRUE);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
        nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_unassign_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    gint selected_cnt = 0;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do

        selected_cnt = _get_nvr_selected_camera_cnt();
        
        if (selected_cnt == 0) {
            nftool_mbox(g_curwnd, "ERROR", "Select one or more cameras to be removed.", NFTOOL_MB_OK);
            return FALSE;
        }
        _unassign_selected_camera();
        _update_nvr_card_data(_get_cur_nvr());
        _update_nvr_page(_get_nvr_cur_page(), TRUE);        
        _update_cam_page(_get_cam_cur_page(), TRUE);

        _update_nvr_list_cam_cnt(TRUE);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
        nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_auto_assign_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    gint i;
    gint last_ch = 0;
    gint empty_cnt = 0;
    gint page;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do

        empty_cnt = _get_nvr_empty_ch_cnt();

        if (empty_cnt <= 0) {
//            nftool_mbox(g_curwnd, "ERROR", "It can't assign camera because there is not empty channel.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        last_ch = _assign_auto();
        
        page = _get_nvr_page_by_ch(last_ch);

        _set_page_by_num(g_nvr_page, page, CIS_NVR_PAGE_CNT);
        _update_nvr_card_data(_get_cur_nvr());
        _update_nvr_page(page, TRUE);
        _update_cam_page(_get_cam_cur_page(), TRUE);

        _update_nvr_list_cam_cnt(TRUE);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
        nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_assign_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    gint selected_cnt = 0;
    gint i;
    gint last_ch = 0;
    gint empty_cnt = 0;
    gint page;
    gchar buf[512];
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do

        selected_cnt = _get_cam_selected_camera_cnt();
        empty_cnt = _get_nvr_empty_ch_cnt();

        if (selected_cnt == 0) {
            nftool_mbox(g_curwnd, "ERROR", "Select one or more cameras to be assigned.", NFTOOL_MB_OK);
            return FALSE;
        }

        if (empty_cnt <= 0) {
            nftool_mbox(g_curwnd, "ERROR", "Cameras are assigned to all channels.", NFTOOL_MB_OK);
            return FALSE;
        }

        if (selected_cnt > empty_cnt) {
            memset(buf, 0x00, sizeof(buf));
            g_sprintf(buf, lookup_string("The number of selected cameras(%d) is more than the number of no video channels(%d).\nOnly the number of no video channels is assigned."), selected_cnt, empty_cnt);
            ret = nftool_mbox(g_curwnd, "WARNING", buf, NFTOOL_MB_OKCANCEL);
            if (ret == NFTOOL_MB_CANCEL) return FALSE;
        }
        
        last_ch = _assign_selected_camera();
        
        page = _get_nvr_page_by_ch(last_ch);

        _set_page_by_num(g_nvr_page, page, CIS_NVR_PAGE_CNT);
        _update_nvr_card_data(_get_cur_nvr());
        _update_nvr_page(page, TRUE);
        _update_cam_page(_get_cam_cur_page(), TRUE);

        _update_nvr_list_cam_cnt(TRUE);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
        nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_option_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
        VW_Camera_Install_Option_Popup_Open(g_curwnd, &g_opt_data);
    }
    
    return FALSE;
}

static gboolean post_camera_login_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
    }
    else if (evt->type == GDK_ENTER_NOTIFY)
    {
        _draw_card_out_line(obj->parent, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    }
    
    return FALSE;
}

static gboolean post_cam_add_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint x, y;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;

        x = 20 + CIS_CAM_FIXED_W + 2;
        y = 60 + 10 + 40 + 10 + 40 + 2;
        
        VW_Manual_Add_Camera_Popup_show(x, y);
    }
    
    return FALSE;
}

static gboolean post_cam_order_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;

        if (VW_Camera_Order_Popup_show() == 0) return FALSE;

        _set_cam_card();
        _update_cam_page(_get_cam_cur_page(), TRUE);
    }
    
    return FALSE;
}

static gboolean post_cam_filter_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;

        if (VW_Camera_Filter_Menu_show() == 0) return FALSE;

        _set_filter_btn_image(TRUE);
        _set_cam_card();
        _update_cam_page(_get_cam_cur_page(), TRUE);
    }
    
    return FALSE;
}

static gboolean post_cam_rescan_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do

        if (_get_cur_nvr() != 0) {
            nftool_mbox(g_curwnd, "ERROR", "Cameras cannot be searched during remote control.", NFTOOL_MB_OK);
            return FALSE;
        }

        _scan_camera(TRUE);
    }
    
    return FALSE;
}

static gboolean post_nvr_camera_login_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint info_idx;
    
    if (evt->type == GDK_BUTTON_PRESS)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
//        g_move_ch_mask = 1;
//        g_ch_change_tmr = g_timeout_add(400, _proc_ch_change, GINT_TO_POINTER(info_idx));
    }
    else if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
	    if (g_ch_change_tmr && !g_move_ch_mask) {
	        g_source_remove(g_ch_change_tmr);
	        g_ch_change_tmr = 0;
	    }
	    
        if (g_move_ch_mask) {
            _finalize_ch_change(_get_info_idx_by_nvr_card(obj->parent));
            return FALSE;
        }
    }
    else if (evt->type == GDK_ENTER_NOTIFY)
    {
        info_idx = _get_info_idx_by_nvr_card(obj->parent);

        if (g_move_ch_mask) {
            _draw_card_out_line(obj->parent, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_DOT);            
        }
        else
        {
            if (g_nvr_card[info_idx].is_selected) {
    		    _draw_card_out_line(obj->parent, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
		    else {
    		    _draw_card_out_line(obj->parent, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
	    }
	}
    else if (evt->type == GDK_LEAVE_NOTIFY)
    {
        info_idx = _get_info_idx_by_nvr_card(obj->parent);
        
        if (g_nvr_card[info_idx].is_selected) {
		    _draw_card_out_line(obj->parent, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
	    }
	    else {
		    _draw_card_out_line(obj->parent, NORMAL_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
	    }
	}
    
    return FALSE;
}

static gboolean post_nvr_all_select_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
    }
    
    return FALSE;
}

static gboolean post_nvr_save_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
		NFOBJECT *top;
		gchar title[64] = {"RECORDER DATA EXPORT"};

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_Camera_Install_Recorder_Data_Save_Open(g_curwnd, title, _get_cur_nvr());
	}
    
    return FALSE;
}

static gboolean post_nvr_load_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
    }
    
    return FALSE;
}

static gboolean post_nvr_restore_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do

        if (!_check_changed()) return FALSE;

        if (nvm_set_cam_info_all(_get_cur_nvr(), g_org_caminfo) == -1) return FALSE;
        
        _update_nvr_card_data(_get_cur_nvr());
    	_set_page_by_num(g_nvr_page, 0, CIS_NVR_PAGE_CNT);
        
        _update_nvr_page(_get_nvr_cur_page(), TRUE);
    }
    
    return FALSE;
}

static gboolean post_nvr_multi_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do

        if (_get_cur_nvr() != 0) {
            nftool_mbox(g_curwnd, "ERROR", "Multiple control is impossible during remote control.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        VW_Camera_Install_NvrManager_Popup_Create(g_curwnd);
        _make_nvr_list(TRUE);
    }
    
    return FALSE;
}

static gboolean post_nvr_reload_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
        nvm_reload_nvr_data(_get_cur_nvr());
    }
    
    return FALSE;
}

static gboolean post_nvr_setting_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;
        //to do
    }
    
    return FALSE;
}

static gboolean post_nvr_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    gint new_idx, pre_idx;
    gint res = 0, msg_ret = 0;
    gint i;
    
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        pre_idx = _get_cur_nvr();
        new_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if (pre_idx == new_idx) return FALSE;
        
        if (g_multi_mode) 
        {
            if (_check_changed()) {
                ret = nftool_mbox(g_curwnd, "WARNING", "Setting is changed.\nDo you want to apply the changes?", NFTOOL_MB_OKCANCEL);
                if (ret == NFTOOL_MB_OK) {
                    res = nvm_finalize_remote_cam_install_with_apply(pre_idx);
                    /*
                    if (res == -1 && pre_idx == 0) {
                        nftool_mbox(g_curwnd, "ERROR", "Failed to apply the settings.", NFTOOL_MB_OK);
                        return FALSE;
                    }
                    */
                }
                else {
                    nvm_finalize_remote_cam_install(pre_idx);
                }
            }
            else
            {
                nvm_finalize_remote_cam_install(pre_idx);
            }

            if (new_idx != 0)    
            {
                nvm_start_remote_cam_install(new_idx);       // If remote access is successful, the response is INFY_OPENMODE_READY_INSTALL_BY_NVM.
            }
            else
            {            
                _set_cur_nvr(LOCAL_IDX);
                _change_nvr(LOCAL_IDX);
            }
        }
        else
        {
            _show_waitbox("Searching for a recorder...", "Please wait...");
            _set_account_with_master_acc();
            g_conn_try_cnt = _try_to_connect();

            g_timeout_add(4000, _complete_conn_nvr, NULL);
        }
    }
    else if (evt->type == IRET_NVM_START_CAMERA_INSTALL)
    {
        msg_ret = ((CMM_MESSAGE_T*)data)->param;

        g_message("IRET_NVM_START_CAMERA_INSTALL : %d", msg_ret);

        if (msg_ret) {
            _show_waitbox("Connecting to Recorder...", "Please wait...");
        }
        else {
            res = VW_NVM_Nvr_Login_Popup_Open(g_curwnd, nfui_combobox_get_cur_index((NFCOMBOBOX*)obj));
            
            if (!res) {
                _set_cur_nvr(LOCAL_IDX);
                _change_nvr(LOCAL_IDX);
                
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, LOCAL_IDX);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                return FALSE;
            }
            else {
                nvm_start_remote_cam_install(nfui_combobox_get_cur_index((NFCOMBOBOX*)obj));
            }
        }
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, IRET_NVM_START_CAMERA_INSTALL);
    }
    
    return FALSE;
}

static gboolean post_arrow_cam_page_move_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean toggled = FALSE;
    gint i;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;

        for (i = 0; i < CIS_CAM_PAGE_CNT; i++) {
            toggled = nfui_radio_button_get_toggled((NFBUTTON*)g_cam_page[i]);
            if (toggled) break;
        }

        if (i == CIS_CAM_PAGE_CNT) return FALSE;

        if (obj == g_cam_arrow[0]) {        //LEFT
            if (i == 0) return FALSE;
            
            _set_page_by_num(g_cam_page, i-1, CIS_CAM_PAGE_CNT);            
            _update_cam_page(i-1, TRUE);
        }
        else {                              //RIGHT
            if (i == (_get_cam_enable_page_cnt() - 1)) return FALSE;
            
            _set_page_by_num(g_cam_page, i+1, CIS_CAM_PAGE_CNT);            
            _update_cam_page(i+1, TRUE);
        }

    }
    
    return FALSE;
}

static gboolean post_arrow_nvr_page_move_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean toggled = FALSE;
    gint idx;
    gint i;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button != MOUSE_LEFT_BUTTON) return FALSE;

        for (i = 0; i < CIS_NVR_PAGE_CNT; i++) {
            toggled = nfui_radio_button_get_toggled((NFBUTTON*)g_nvr_page[i]);
            if (toggled) break;
        }

        if (i == CIS_NVR_PAGE_CNT) return FALSE;

        if (obj == g_nvr_arrow[0]) {        //LEFT
            if (i == 0) return FALSE;
            
        	_set_page_by_num(g_nvr_page, i-1, CIS_NVR_PAGE_CNT);
            _update_nvr_page(i-1, TRUE);
        }
        else {                              //RIGHT
            if (i == (_get_nvr_enable_page_cnt() - 1)) return FALSE;
            
        	_set_page_by_num(g_nvr_page, i+1, CIS_NVR_PAGE_CNT);
            _update_nvr_page(i+1, TRUE);
        }

    }
	else if (evt->type == GDK_ENTER_NOTIFY)
	{
	    if (g_move_ch_mask)
	    {
            for (i = 0; i < CIS_NVR_PAGE_CNT; i++) {
                toggled = nfui_radio_button_get_toggled((NFBUTTON*)g_nvr_page[i]);
                if (toggled) break;
            }
            if (i == CIS_NVR_PAGE_CNT) return FALSE;

            if (obj == g_nvr_arrow[0]) {        //LEFT
                if (i == 0) return FALSE;

                idx = i - 1;
            }
            else {                              //RIGHT
                if (i == (_get_nvr_enable_page_cnt() - 1)) return FALSE;

                idx = i + 1;
            }

	        g_nvr_page_move_tmr = g_timeout_add(500, _proc_nvr_page_move, GINT_TO_POINTER(idx));
	    }
	}
	else if (evt->type == GDK_LEAVE_NOTIFY)
	{
	    if (g_nvr_page_move_tmr) {
	        g_source_remove(g_nvr_page_move_tmr);
	        g_nvr_page_move_tmr = 0;
	    }
	}
    
    return FALSE;
}

static gboolean post_radio_cam_page_move_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
		gint idx = 0;

		idx = nfui_radio_button_get_index((NFBUTTON*)obj);
		
        _update_cam_page(idx, TRUE);
	}

	return FALSE;
}

static gboolean post_radio_nvr_page_move_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint idx = 0;
	
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
		idx = nfui_radio_button_get_index((NFBUTTON*)obj);
		
        _update_nvr_page(idx, TRUE);
	}
	else if (evt->type == GDK_ENTER_NOTIFY)
	{
	    if (g_move_ch_mask)
	    {
    		idx = nfui_radio_button_get_index((NFBUTTON*)obj);
	        g_nvr_page_move_tmr = g_timeout_add(500, _proc_nvr_page_move, GINT_TO_POINTER(idx));
	    }
	}
	else if (evt->type == GDK_LEAVE_NOTIFY)
	{
	    if (g_nvr_page_move_tmr) {
	        g_source_remove(g_nvr_page_move_tmr);
	        g_nvr_page_move_tmr = 0;
	    }
	}

	return FALSE;
}

static gboolean post_camera_card_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    mb_type ret;
    gint i;
    gint last_ch = 0;
    gint empty_cnt = 0;
    gint page;
    gint info_idx;
    gint res = -1;
    
	switch (event->type) 
	{    
		case GDK_2BUTTON_PRESS:
		{
            if (event->button.button != MOUSE_LEFT_BUTTON) return FALSE;
            
            info_idx = _get_info_idx_by_cam_card(obj);
            if (info_idx == -1) return FALSE;
            
            empty_cnt = _get_nvr_empty_ch_cnt();

            if (empty_cnt <= 0) {
                nftool_mbox(g_curwnd, "ERROR", "It can't assign camera because there is not empty channel.", NFTOOL_MB_OK);
                return FALSE;
            }

            res = _assign_camera(info_idx);
            if (res == -1) return FALSE;
        
        	page = _get_nvr_page_by_ch(last_ch);

        	_set_page_by_num(g_nvr_page, page, CIS_NVR_PAGE_CNT);
            _update_nvr_card_data(_get_cur_nvr());
        	_update_nvr_page(page, TRUE);
        	_update_cam_page(_get_cam_cur_page(), TRUE);
        	
        	_update_nvr_list_cam_cnt(TRUE);
        	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
        	nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
		}
		break;
		
		case GDK_BUTTON_PRESS:
		{
            if (event->button.button == MOUSE_LEFT_BUTTON)
            {
                info_idx = _get_info_idx_by_cam_card(obj);
                if (info_idx == -1) return FALSE;
                
                if (g_cam_card[info_idx].is_selected) {
                    g_cam_card[info_idx].is_selected = 0;
        		    _draw_card_out_line(obj, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    		    }
    		    else {
    		        g_cam_card[info_idx].is_selected = 1;
        		    _draw_card_out_line(obj, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    		    }

    		    if (g_ch_assign_tmr) {
    		        g_source_remove(g_ch_assign_tmr);
    		        g_ch_assign_tmr = 0;
    		    }
    		    
    		    g_ch_assign_tmr = g_timeout_add(50, _proc_ch_assign, GINT_TO_POINTER(info_idx));
		    }
		    else if (event->button.button == MOUSE_RIGTH_BUTTON)
		    {
		    }
		}
		break;

		case GDK_BUTTON_RELEASE:
		{
    		guint x, y;
    		
            if (event->button.button == MOUSE_LEFT_BUTTON)
            {
    		    if (g_ch_assign_tmr && !g_assign_ch_mask) {
    		        g_source_remove(g_ch_assign_tmr);
    		        g_ch_assign_tmr = 0;
    		    }

    		    if (g_assign_ch_mask) _finalize_ch_assign(-1);
		    }
		    else
		    {
    			nfui_nfobject_get_window_pos(obj, &x, &y);

    			x += ((GdkEventButton*)event)->x;
    			y += ((GdkEventButton*)event)->y;
    			
		        info_idx = _get_info_idx_by_cam_card(obj);
                VW_Show_Install_Search_Shortcut_Menu(&g_cam_card[info_idx].cam_info, x, y);
		    }
		}
		break;

		case GDK_ENTER_NOTIFY:
		{
            info_idx = _get_info_idx_by_cam_card(obj);
            if (info_idx == -1) return FALSE;
            
            if (g_cam_card[info_idx].is_selected) {
    		    _draw_card_out_line(obj, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
		    else {
    		    _draw_card_out_line(obj, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
		}
		break;

		case GDK_LEAVE_NOTIFY:
		{
            info_idx = _get_info_idx_by_cam_card(obj);
            if (info_idx == -1) return FALSE;
            
            if (g_cam_card[info_idx].is_selected) {
    		    _draw_card_out_line(obj, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
		    else {
    		    _draw_card_out_line(obj, NORMAL_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }

		    if (g_ch_assign_tmr && !g_assign_ch_mask) {
		        g_source_remove(g_ch_assign_tmr);
		        g_ch_assign_tmr = 0;
		    }
		}
		break;

        case GDK_DELETE:
        {
        }
        break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_nvr_card_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    gint info_idx;
    mb_type ret;
    gint selected_cnt = 0;

	switch (event->type) 
	{
		case GDK_2BUTTON_PRESS:
		{
            if (event->button.button != MOUSE_LEFT_BUTTON) return FALSE;
            
            info_idx = _get_info_idx_by_nvr_card(obj);
            if (info_idx == -1) return FALSE;

            _unassign_camera(info_idx);
            _update_nvr_card_data(_get_cur_nvr());
        	_update_nvr_page(_get_nvr_cur_page(), TRUE);        
        	_update_cam_page(_get_cam_cur_page(), TRUE);

        	_update_nvr_list_cam_cnt(TRUE);
        	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
        	nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
		}
		break;
		
		case GDK_BUTTON_PRESS:
		{
            if (event->button.button != MOUSE_LEFT_BUTTON) return FALSE;

            if (event->button.button == MOUSE_LEFT_BUTTON)
            {
                info_idx = _get_info_idx_by_nvr_card(obj);
                if (info_idx == -1) return FALSE;
                
                if (g_nvr_card[info_idx].is_selected) {
                    g_nvr_card[info_idx].is_selected = 0;
        		    _draw_card_out_line(obj, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    		    }
    		    else {
    		        g_nvr_card[info_idx].is_selected = 1;
        		    _draw_card_out_line(obj, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    		    }

    		    if (g_ch_change_tmr) {
    		        g_source_remove(g_ch_change_tmr);
    		        g_ch_change_tmr = 0;
    		    }
    		    
    		    g_ch_change_tmr = g_timeout_add(400, _proc_ch_change, GINT_TO_POINTER(info_idx));
//    		    _set_ready_move_card(obj, (gint)bevent->x, (gint)bevent->y);
		    }
		}
		break;

		case GDK_MOTION_NOTIFY:
		{
            GdkEventMotion *mevent;
            gint off_x, off_y;

            mevent = (GdkEventMotion*)event;
		}
        break;

        case GDK_BUTTON_RELEASE:
        {
    		guint x, y;
    		
            if (event->button.button == MOUSE_LEFT_BUTTON)
            {
    		    if (g_ch_change_tmr && !g_move_ch_mask) {
    		        g_source_remove(g_ch_change_tmr);
    		        g_ch_change_tmr = 0;
    		    }

    		    if (g_move_ch_mask) _finalize_ch_change(-1);
		    }
		    else
		    {
    			nfui_nfobject_get_window_pos(obj, &x, &y);

    			x += ((GdkEventButton*)event)->x;
    			y += ((GdkEventButton*)event)->y;

    			if (x > (DISPLAY_ACTIVE_WIDTH - 250)) x -= 250;
    			
		        info_idx = _get_info_idx_by_nvr_card(obj);
                VW_Show_Install_Search_Shortcut_Menu(&g_nvr_card[info_idx].cam_info, x, y);
		    }
        }
        break;
        
		case GDK_ENTER_NOTIFY:
		{
            info_idx = _get_info_idx_by_nvr_card(obj);
            if (info_idx == -1) return FALSE;

		    if (g_ch_change_tmr && !g_move_ch_mask) {
		        g_source_remove(g_ch_change_tmr);
		        g_ch_change_tmr = 0;
		    }

		    if (g_ch_assign_tmr && !g_assign_ch_mask) {
		        g_source_remove(g_ch_assign_tmr);
		        g_ch_assign_tmr = 0;
		    }
            
            if (g_move_ch_mask || g_assign_ch_mask) {
                _draw_card_out_line(obj, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_DOT);            
            }
            else
            {
                if (g_nvr_card[info_idx].is_selected) {
        		    _draw_card_out_line(obj, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    		    }
    		    else {
        		    _draw_card_out_line(obj, FOCUS_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
    		    }
		    }
		}
		break;

		case GDK_LEAVE_NOTIFY:
		{
            info_idx = _get_info_idx_by_nvr_card(obj);
            if (info_idx == -1) return FALSE;

		    if (g_ch_change_tmr && !g_move_ch_mask) {
		        g_source_remove(g_ch_change_tmr);
		        g_ch_change_tmr = 0;
		    }

		    if (g_ch_assign_tmr && !g_assign_ch_mask) {
		        g_source_remove(g_ch_assign_tmr);
		        g_ch_assign_tmr = 0;
		    }
            
            if (g_nvr_card[info_idx].is_selected) {
    		    _draw_card_out_line(obj, SELECT_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
		    else {
    		    _draw_card_out_line(obj, NORMAL_OUTLINE_COLOR, OUTLINE_TYPE_FULL);
		    }
		}
		break;

        case GDK_DELETE:
        {
        }
        break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	gint ret;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (_get_cur_nvr() == 0)
		{
		    if (!_check_changed()) return FALSE;
		    
    		ret = nvm_apply_cam_settings(_get_cur_nvr());
    		
    		if (ret != 0) {
                nftool_mbox(g_curwnd, "ERROR", "Failed to apply the changes.", NFTOOL_MB_OK);
                return FALSE;
    		}
    		else {
            	if (g_org_caminfo) {
            	    ifree(g_org_caminfo);
            	    g_org_caminfo = NULL;
            	}
            	g_org_caminfo = nvm_get_cam_info_all(_get_cur_nvr());
    		    
                nftool_mbox(g_curwnd, "NOTICE", "Successfully apply the changes.", NFTOOL_MB_OK);
    		}
		}
		else
		{
    		 nvm_apply_cam_settings(_get_cur_nvr());
            _show_waitbox("", "Please wait...");
		}
	}
	else if (event->type == IRET_NVM_APPLY_CAMERA_SETTING)
	{
        ret = ((CMM_MESSAGE_T *)data)->param;

        g_message("IRET_NVM_APPLY_CAMERA_SETTING : %d", ret);

        _hide_waitbox();
        
		if (!ret) {
            nftool_mbox(g_curwnd, "ERROR", "Failed to apply the changes.", NFTOOL_MB_OK);
            return FALSE;
		}
		else {
        	if (g_org_caminfo) {
        	    ifree(g_org_caminfo);
        	    g_org_caminfo = NULL;
        	}
        	g_org_caminfo = nvm_get_cam_info_all(_get_cur_nvr());
		    
            nftool_mbox(g_curwnd, "NOTICE", "Successfully apply the changes.", NFTOOL_MB_OK);
		}
	}
	else if (event->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, IRET_NVM_APPLY_CAMERA_SETTING);
	}
	
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	mb_type ret;
	gint res;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (_check_changed()) {
            ret = nftool_mbox(g_curwnd, "WARNING", "Setting is changed.\nDo you want to apply the changes?", NFTOOL_MB_OKCANCEL);
            if (ret == NFTOOL_MB_OK) {
                res = nvm_finalize_remote_cam_install_with_apply(_get_cur_nvr());
                /*
                if (res == -1 && _get_cur_nvr() == 0) {
                    nftool_mbox(g_curwnd, "ERROR", "Failed to apply the settings.", NFTOOL_MB_OK);
                    return FALSE;
                }
                */
            }
            else {
                nvm_finalize_remote_cam_install(_get_cur_nvr());
            }
        }
        else
        {
            nvm_finalize_remote_cam_install(_get_cur_nvr());
        }
        
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) 
	{
        g_timeout_add(50, _proc_redraw, NULL);
	}
	else if (evt->type == GDK_DELETE)
	{	
		uxm_unreg_imsg_event(obj, INFY_IPCAM_INSTALL_NOTIFY); 
		uxm_unreg_imsg_event(obj, INFY_NVM_DATA_IS_CHANGED); 
		uxm_unreg_imsg_event(obj, INFY_OPENMODE_READY_INSTALL_BY_NVM); 
		uxm_unreg_imsg_event(obj, INFY_OPENMODE_FAIL_INSTALL_BY_NVM); 
		uxm_unreg_imsg_event(obj, IRET_NVM_APPLY_AND_FINALIZE);
		uxm_unreg_imsg_event(obj, IRET_NVM_UPDATE_CONN_STATUS);
 		uxm_unreg_imsg_event(obj, INFY_GET_IPCAM_THUMBNAIL);
	}
	else if (evt->type == INFY_IPCAM_INSTALL_NOTIFY)
	{
		NFOpenmodeDeviceList *dlist = NULL;
		NF_NOTIFY_INFO *pInfo;
		gchar strBuf[64];

		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
#if 1
		if (pInfo->d.params[0] == 1)
		{
		    g_is_scanning = SCAN_RUNNING;
		    _show_waitbox("Searching for a camera...", "Please wait...");
		} 
		else if (pInfo->d.params[0] == 0)
		{
		    _hide_waitbox();
		}
		
		dlist = nf_openmode_get_list();

		if (!dlist) return FALSE;
		if (dlist->entry_cnt <= 0) return FALSE;

        if (pInfo->d.params[0] == 0 && g_is_scanning == SCAN_RUNNING)
        {
            if (g_update_tmr) {
                g_source_remove(g_update_tmr);
                g_update_tmr = 0;
            }

            _translate_search_result(strBuf);
            _set_search_result(strBuf, 0, TRUE);

            _make_camera_data();
            _make_nvr_data();
            _update_nvr_card_data(0);
            
            if (g_org_caminfo) {
                ifree(g_org_caminfo);
                g_org_caminfo = NULL;
            }
            g_org_caminfo = nvm_get_cam_info_all(0);

            g_update_tmr = g_timeout_add(100, _proc_update_page, NULL);

            g_is_scanning = SCAN_PAUSE;

            if (g_multi_mode & g_mode_select) 
            {
                _make_nvr_list(TRUE);
                g_mode_select = 0;
            }
            else {
                _make_nvr_list(TRUE);
            }
        }
        else if (pInfo->d.params[0] == 0 && g_is_scanning == SCAN_PAUSE)
        {
            gint page_cnt = _get_cam_enable_page_cnt();
            gint i;
        	
            _refresh_cam_info();
            _enable_cam_page(TRUE);
            
            if (pInfo->d.params[1] == 1) _set_page_by_num(g_cam_page, _get_cam_last_page(), CIS_CAM_PAGE_CNT);
            else _set_page_by_num(g_cam_page, _get_cam_cur_page(), CIS_CAM_PAGE_CNT);
            
            _update_cam_page(_get_cam_cur_page(), TRUE);

            for (i = 0; i < (page_cnt * CIS_CAM_CARD_CNT_PER_PAGE); i++)
            {
                if (nvm_update_cam_info(_get_cur_nvr(), &g_cam_card[i].cam_info) == -1) continue;
            }
            
            _update_nvr_card_data(_get_cur_nvr());
            
            _update_nvr_page(_get_nvr_cur_page(), TRUE);
		}
#else
        if (pInfo->d.params[0] == 1 && g_is_scanning == 1)
        {
        }
        else if (pInfo->d.params[0] == 1 && g_is_scanning == 0)
        {
        }
        else if (pInfo->d.params[0] == 0 && g_is_scanning == 1)
        {
        }
        else if (pInfo->d.params[0] == 0 && g_is_scanning == 0)
        {
        }

#endif
	}
	else if (evt->type == INFY_NVM_DATA_IS_CHANGED)
	{
	    gint idx;
	    gint i;

	    idx  = ((CMM_MESSAGE_T *)data)->param;
		g_message("###INFY_NVM_DATA_IS_CHANGED  : %s, %d idx : %d", __FUNCTION__, __LINE__, idx);

        _update_nvr_card_data(idx);
        _enable_nvr_page(TRUE);
        _set_page_by_num(g_nvr_page, 0, CIS_NVR_PAGE_CNT);    	    
        _update_nvr_page(_get_nvr_cur_page(), TRUE);        
        _make_nvr_list(TRUE);
	}
	else if (evt->type == INFY_OPENMODE_READY_INSTALL_BY_NVM)
	{
	    gint res = ((CMM_MESSAGE_T *)data)->param;
	    gint i;
	    
		g_message("###INFY_OPENMODE_READY_INSTALL_BY_NVM  : %s, %d, res : %d", __FUNCTION__, __LINE__, res);

		_hide_waitbox();

		if (res)
		{
		    _set_cur_nvr(nfui_combobox_get_cur_index((NFCOMBOBOX*)g_nvr_list));
		    _change_nvr(_get_cur_nvr());
		    
            _make_nvr_list(TRUE);
            
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, _get_cur_nvr());
            nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
		}
		else
		{
		    _set_cur_nvr(LOCAL_IDX);
		    _change_nvr(LOCAL_IDX);
            
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, LOCAL_IDX);
            nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);
		}
	}
	else if (evt->type == INFY_OPENMODE_FAIL_INSTALL_BY_NVM)
	{
	    gint idx = ((CMM_MESSAGE_T *)data)->param;
	    gint i;
	    
		g_message("###INFY_OPENMODE_FAIL_INSTALL_BY_NVM  : %s, %d, idx : %d", __FUNCTION__, __LINE__, idx);

		_hide_waitbox();
		
	    _set_cur_nvr(LOCAL_IDX);
	    _change_nvr(LOCAL_IDX);
        
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_nvr_list, LOCAL_IDX);
        nfui_signal_emit(g_nvr_list, GDK_EXPOSE, TRUE);

        nftool_mbox_auto(g_curwnd, 2, "ERROR", "Connection failure.");
	}
	else if (evt->type == IRET_NVM_APPLY_AND_FINALIZE)
	{
	}    
	else if (evt->type == IRET_NVM_UPDATE_CONN_STATUS)
	{
	    NVR_DATA_T *ndata = NULL;
        CAMERA_INFO_T tmp[1024];
        XMM_CONN_STATUS cstate;
	    gint idx = ((CMM_MESSAGE_T *)data)->param;
	    gint i, cnt = 0;

        g_message("###yanggungg : %s, %d, idx : %d", __FUNCTION__, __LINE__, idx);
        
        ndata = nvm_get_nvr_data(idx);
        cstate = xmm_get_conn_status(ndata->hostname, ndata->http_port);
        g_message("###yanggungg : %s, %d, cstate : %d", __FUNCTION__, __LINE__, cstate);
        
        if (cstate != XMM_CONN_CONN_SUCC) {
            ifree(ndata);
            ndata = NULL;
            return FALSE;
        }

        for (i = 0; i < nvm_get_active_ch_cnt(idx); i++)
        {
            if (strlen(ndata->cam_info[i].url) > 0) {
                g_message("###yanggungg : %s, %d, url : %s", __FUNCTION__, __LINE__, ndata->cam_info[i].url);
                ocam_assign(ndata->cam_info[i].url);
            }
        }

        _set_cam_card();
        _update_cam_page(_get_cam_cur_page(), TRUE);

	    ifree(ndata);
	    ndata = NULL;
	}
	else if (evt->type == INFY_GET_IPCAM_THUMBNAIL)
	{
	    CAM_THUMBNAIL_T *tdata = ((CMM_MESSAGE_T *)data)->data;
	    gint idx = ((CMM_MESSAGE_T *)data)->param;
	    gint i;
	    gint ret = 0;

//	    g_message("###yanggungg : %s, %d INFY_GET_IPCAM_THUMBNAIL START", __FUNCTION__, __LINE__);

	    for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++) 
	    {
	        if (strlen(g_thumbnail[i].hostname) > 0) 
	        {
	            if (strcmp(g_thumbnail[i].hostname, tdata->hostname) == 0)
	            {
    	            if (g_thumbnail[i].jpec_data) {
    	                ifree(g_thumbnail[i].jpec_data);
                    }
    	            
    	            g_memmove(&g_thumbnail[i], tdata, sizeof(CAM_THUMBNAIL_T));
    	            g_thumbnail[i].jpec_data = (gchar *)imalloc(g_thumbnail[i].jpec_size);
    	            g_memmove(g_thumbnail[i].jpec_data, tdata->jpec_data, g_thumbnail[i].jpec_size);
    	            break;
	            }
	        }
	        else 
	        {
	            g_memmove(&g_thumbnail[i], tdata, sizeof(CAM_THUMBNAIL_T));
	            g_thumbnail[i].jpec_data = (gchar *)imalloc(sizeof(gchar) * g_thumbnail[i].jpec_size);
	            g_memmove(g_thumbnail[i].jpec_data, tdata->jpec_data, g_thumbnail[i].jpec_size);
	            break;
	        }
	    }
	    
        ifree(tdata->jpec_data);
        ifree(tdata);

	    if (i == CIS_CAM_INFO_BUF_SIZE) return FALSE;
	    if (g_is_scanning != SCAN_PAUSE) return FALSE;

	    //update cam, nvr card image
	    {
            gint card_idx, info_idx;

            info_idx = _get_cam_cur_page() * CIS_CAM_CARD_CNT_PER_PAGE;
            
            for (card_idx = 0; card_idx < CIS_CAM_CARD_CNT_PER_PAGE; card_idx++)
            {
                if (strlen(g_cam_card[info_idx].cam_info.url) > 0)
                {
                    if (g_cam_card[info_idx].thumb_idx == -1) g_cam_card[info_idx].thumb_idx = _get_thumbnail_idx(&g_cam_card[info_idx].cam_info);
                    
                    if (g_cam_card[info_idx].thumb_idx != -1) {
                        ret = _update_camera_thumbnail(card_idx, g_cam_card[info_idx].thumb_idx, TRUE);
                        if (ret == -1) {
                            _update_camera_status(card_idx, g_cam_card[info_idx].cam_info.state, TRUE);
                        }
                    }
                    else {
                        _update_camera_status(card_idx, g_cam_card[info_idx].cam_info.state, TRUE);
                    }
                }
                else
                {
                    _update_camera_status(card_idx, 0, TRUE);
                }
                
                info_idx++;
            }
        }
        {
            gint card_idx, info_idx;

            info_idx = _get_nvr_cur_page() * CIS_NVR_CARD_CNT_PER_PAGE;
            
            for (card_idx = 0; card_idx < CIS_NVR_CARD_CNT_PER_PAGE; card_idx++)
            {
                if (strlen(g_nvr_card[info_idx].cam_info.url) > 0)
                {
                    if (g_nvr_card[info_idx].thumb_idx == -1) g_nvr_card[info_idx].thumb_idx = _get_thumbnail_idx(&g_nvr_card[info_idx].cam_info);
                    
                    if (g_nvr_card[info_idx].thumb_idx != -1) {
                        ret = _update_dev_camera_thumbnail(card_idx, g_nvr_card[info_idx].thumb_idx, TRUE);
                        if (ret == -1) {
                            _update_dev_camera_status(card_idx, g_nvr_card[info_idx].cam_info.state, TRUE);
                        }
                    }
                    else {
                        _update_dev_camera_status(card_idx, g_nvr_card[info_idx].cam_info.state, TRUE);
                    }
                }
                else
                {
//                    _update_dev_camera_select_line(card_idx, 0, TRUE);
                    _update_dev_camera_empty(card_idx, info_idx, TRUE);
                }
                
                info_idx++;
            }
        }	
    }

	return FALSE;
}

static gboolean post_window_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, j;

	switch(evt->type)
	{
		case GDK_DELETE:
		{
        	if (g_org_caminfo) {
        	    ifree(g_org_caminfo);
        	    g_org_caminfo = NULL;
        	}
        	
    	    for (i = 0; i < CIS_CAM_INFO_BUF_SIZE; i++) 
    	    {
                if (g_thumbnail[i].jpec_data) {
                    ifree(g_thumbnail[i].jpec_data);
                }
    	    }

        	iis_thumb_manager_stop();
        	
			g_curwnd = 0;
			gtk_main_quit();
		}
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

static gboolean pre_card_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) 
	{
		GdkDrawable *drawable;
		GdkGC *gc;
        gint off_x, off_y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc((NFOBJECT*)obj);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		gdk_gc_set_line_attributes(gc,
				3,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);
    
    	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(434));
    	gdk_draw_rectangle(drawable, gc, FALSE, off_x, off_y, CIS_CARD_DEF_WIDTH, CIS_CARD_DEF_HEIGHT);	

		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) 
	{
		GdkDrawable *drawable;
		GdkColor line_color = UX_COLOR(392);
		GdkGC *line_gc;
        gint off_x, off_y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		line_gc = nfui_nfobject_get_gc((NFOBJECT*)obj);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		gdk_gc_set_rgb_fg_color(line_gc, &line_color);

		gdk_gc_set_line_attributes(line_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);

		gdk_draw_rectangle(drawable,
				line_gc,
				FALSE,
				off_x, off_y,
				obj->width, obj->height);

		nfui_nfobject_gc_unref(line_gc);
	}

	return FALSE;
}

static gboolean pre_sub_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
    gint off_x, off_y;

	if (evt->type == GDK_EXPOSE) 
	{
		GdkDrawable *drawable;
		GdkColor line_color = UX_COLOR(392);
		GdkGC *line_gc;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		line_gc = nfui_nfobject_get_gc((NFOBJECT*)obj);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
		
		gdk_gc_set_rgb_fg_color(line_gc, &line_color);

		gdk_gc_set_line_attributes(line_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);

		gdk_draw_rectangle(drawable,
				line_gc,
				FALSE,
				off_x, off_y,
				obj->width, obj->height);

		nfui_nfobject_gc_unref(line_gc);
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc((NFOBJECT*)obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);    
		nfutil_draw_image(drawable, gc, POPUP_MENU_LINE, 2, 44, -1, -1, NFALIGN_LEFT, 0);        
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}	
	
	return FALSE;
}

static void init_cam_page(NFOBJECT *parent)
{
    NFOBJECT *fixed = parent;
    NFOBJECT *card = NULL;
    NFOBJECT *obj = NULL;
    gint pos_x, pos_y;
    gint i;

    pos_x = 15;
    pos_y = 15;
    
    for (i = 0; i < CIS_CAM_CARD_CNT_PER_PAGE; i++)
    {
        card = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(card, CIS_CARD_DEF_WIDTH, CIS_CARD_DEF_HEIGHT);
        nfui_nfobject_use_focus(card, NFOBJECT_FOCUS_ON);
        nfui_nfobject_show(card);    
        nfui_nffixed_put((NFFIXED*)fixed, card, pos_x, pos_y);
        nfui_regi_pre_event_callback(card, pre_card_event_handler);
        nfui_regi_post_event_callback(card, post_camera_card_event_handler);
        g_cam_card_obj[i] = card;

        obj = nftool_normal_button_create_popup_type2("LOG IN", 120);
        nfui_nffixed_put((NFFIXED*)card, obj, (176-120) / 2, 65 + ((CIS_CARD_THUMB_H-30)/2));
        nfui_regi_post_event_callback(obj, post_camera_login_event_handler);
        nfui_nfobject_hide(obj);    
        g_cam_btn_login[i] = obj;

        pos_x += CIS_CARD_DEF_WIDTH + 15;

        if (i % 4 == 3)	{
            pos_x = 15;
            pos_y += (CIS_CARD_DEF_HEIGHT + 15); 
        }
    }	

    return;
}

static void init_nvr_page(NFOBJECT *parent, int page_num)
{
    NFOBJECT *fixed = parent;
    NFOBJECT *card = NULL;
    NFOBJECT *obj = NULL;
    gint pos_x, pos_y;
    gint size_w, size_h;
    gint i;

    pos_x = 15;
    pos_y = 15;
    
	for (i = 0; i < CIS_NVR_CARD_CNT_PER_PAGE; i++)
	{
		card = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(card, CIS_CARD_DEF_WIDTH, CIS_CARD_DEF_HEIGHT);
		nfui_nfobject_use_focus(card, NFOBJECT_FOCUS_ON);
        nfui_nfobject_show(card);    
		nfui_nffixed_put((NFFIXED*)fixed, card, pos_x, pos_y);
		nfui_regi_pre_event_callback(card, pre_card_event_handler);
		nfui_regi_post_event_callback(card, post_nvr_card_event_handler);
		g_nvr_card_obj[i] = card;

    	obj = nftool_normal_button_create_popup_type2("LOG IN", 120);
    	nfui_nffixed_put((NFFIXED*)card, obj, (176-120) / 2, 65 + ((CIS_CARD_THUMB_H-30)/2));
    	nfui_regi_post_event_callback(obj, post_nvr_camera_login_event_handler);
        nfui_nfobject_hide(obj);    
		g_nvr_btn_login[i] = obj;

        pos_x += CIS_CARD_DEF_WIDTH + 15;
        
    	if (i % 4 == 3)	{
            pos_x = 15;
    		pos_y += (CIS_CARD_DEF_HEIGHT + 15); 
		}
	}	

    return;
}

////////////////////////////////////////////////////////////////////
//
//
//

gint VW_Open_IPCamInstallSearch_Page_ver2(NFOBJECT *parent)
{
	NFOBJECT *win = NULL;
	NFOBJECT *main_fixed = NULL;
	NFOBJECT *cam_fixed = NULL;
	NFOBJECT *nvr_fixed = NULL;
	NFOBJECT *video_fixed = NULL;
    NFOBJECT *cam_page;
    NFOBJECT *notice_board;
    NFOBJECT *nvr_page;
	NFOBJECT *obj = NULL;

	gint pos_x, pos_y;
	gint addcam_x, addcam_y;
	gint filter_x, filter_y;
	gint order_x, order_y;
	gint i;
	gint size_w, size_h, size_w2, size_h2;
	GSList *slist = NULL;
	gint mode;
	gchar buf[64];

	gchar strpage_num[10][3] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *left_arrow[NFOBJECT_STATE_COUNT];
    GdkPixbuf *right_arrow[NFOBJECT_STATE_COUNT];

	GdkPixbuf* rescan_cam[NFOBJECT_STATE_COUNT];
	GdkPixbuf* add_cam[NFOBJECT_STATE_COUNT];
	GdkPixbuf* filter[NFOBJECT_STATE_COUNT];
	GdkPixbuf* order[NFOBJECT_STATE_COUNT];
	GdkPixbuf* option[NFOBJECT_STATE_COUNT];
    
	GdkPixbuf *ch_move_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *ch_move_down[NFOBJECT_STATE_COUNT];
    GdkPixbuf *ch_move_right[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ch_move_left[NFOBJECT_STATE_COUNT];


    _set_cur_nvr(LOCAL_IDX);
    g_is_scanning = SCAN_INIT;
    g_mode_select = 1;
    g_multi_mode = 0;
    
    if (g_org_caminfo) {
        ifree(g_org_caminfo);
        g_org_caminfo = NULL;
    }
    g_org_caminfo = NULL;
    
	memset(g_cam_card, 0x00, sizeof(g_cam_card));
	memset(g_nvr_card, 0x00, sizeof(g_nvr_card));
	memset(g_thumbnail, 0x00, sizeof(g_thumbnail));

    ocam_init();
	_init_db();


	ch_move_up[0] = nfui_get_image_from_file(IMG_N_PTZ_UP_LIVE, NULL);
	ch_move_up[1] = nfui_get_image_from_file(IMG_O_PTZ_UP_LIVE, NULL);
	ch_move_up[2] = nfui_get_image_from_file(IMG_P_PTZ_UP_LIVE, NULL);
	ch_move_up[3] = nfui_get_image_from_file(IMG_D_PTZ_UP_LIVE, NULL);	

	ch_move_down[0] = nfui_get_image_from_file(IMG_N_PTZ_DOWN_LIVE, NULL);
	ch_move_down[1] = nfui_get_image_from_file(IMG_O_PTZ_DOWN_LIVE, NULL);
	ch_move_down[2] = nfui_get_image_from_file(IMG_P_PTZ_DOWN_LIVE, NULL);
	ch_move_down[3] = nfui_get_image_from_file(IMG_D_PTZ_DOWN_LIVE, NULL);	

    ch_move_right[0] = nfui_get_image_from_file((IMG_N_PTZ_RIGHT_LIVE), NULL);
    ch_move_right[1] = nfui_get_image_from_file((IMG_O_PTZ_RIGHT_LIVE), NULL);
    ch_move_right[2] = nfui_get_image_from_file((IMG_P_PTZ_RIGHT_LIVE), NULL);
    ch_move_right[3] = nfui_get_image_from_file((IMG_D_PTZ_RIGHT_LIVE), NULL);

    ch_move_left[0] = nfui_get_image_from_file((IMG_N_PTZ_LEFT_LIVE), NULL);
    ch_move_left[1] = nfui_get_image_from_file((IMG_O_PTZ_LEFT_LIVE), NULL);
    ch_move_left[2] = nfui_get_image_from_file((IMG_P_PTZ_LEFT_LIVE), NULL);
    ch_move_left[3] = nfui_get_image_from_file((IMG_D_PTZ_LEFT_LIVE), NULL);

	rescan_cam[0] = nfui_get_image_from_file(("bt_pop_sync_n.png"), NULL);
	rescan_cam[1] = nfui_get_image_from_file(("bt_pop_sync_o.png"), NULL);
	rescan_cam[2] = nfui_get_image_from_file(("bt_pop_sync_p.png"), NULL);
	rescan_cam[3] = nfui_get_image_from_file(("bt_pop_sync_d.png"), NULL);

	add_cam[0] = nfui_get_image_from_file((IMG_BT_POP_ADDCAM_N), NULL);
	add_cam[1] = nfui_get_image_from_file((IMG_BT_POP_ADDCAM_O), NULL);
	add_cam[2] = nfui_get_image_from_file((IMG_BT_POP_ADDCAM_P), NULL);
	add_cam[3] = nfui_get_image_from_file((IMG_BT_POP_ADDCAM_D), NULL);

	filter[0] = nfui_get_image_from_file(IMG_BT_POP_FILTER_N, NULL);
	filter[1] = nfui_get_image_from_file(IMG_BT_POP_FILTER_O, NULL);
	filter[2] = nfui_get_image_from_file(IMG_BT_POP_FILTER_P, NULL);
	filter[3] = nfui_get_image_from_file(IMG_BT_POP_FILTER_D, NULL);	

	order[0] = nfui_get_image_from_file(IMG_BT_POP_ORDER_N, NULL);
	order[1] = nfui_get_image_from_file(IMG_BT_POP_ORDER_O, NULL);
	order[2] = nfui_get_image_from_file(IMG_BT_POP_ORDER_P, NULL);
	order[3] = nfui_get_image_from_file(IMG_BT_POP_ORDER_D, NULL);	

	option[0] = nfui_get_image_from_file(IMG_BT_POP_SETTING_N, NULL);
	option[1] = nfui_get_image_from_file(IMG_BT_POP_SETTING_O, NULL);
	option[2] = nfui_get_image_from_file(IMG_BT_POP_SETTING_P, NULL);
	option[3] = nfui_get_image_from_file(IMG_BT_POP_SETTING_D, NULL);	

    left_arrow[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_LEFT), NULL);
    left_arrow[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_LEFT), NULL);
    left_arrow[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_LEFT), NULL);
    left_arrow[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_LEFT), NULL);

    right_arrow[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_RIGHT), NULL);
    right_arrow[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_RIGHT), NULL);
    right_arrow[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_RIGHT), NULL);
    right_arrow[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_RIGHT), NULL);
    
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_nfwindow_use_double_buffer((NFWINDOW*)win);
	nfui_regi_post_event_callback(win, post_window_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA ADD/DELETE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 19);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);


	
//////////////////////////////////////////////
// CAMERA
//////////////////////////////////////////////

	pos_x = 20;
	pos_y = 60;
	
	cam_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(cam_fixed, CIS_CAM_FIXED_W, DISPLAY_ACTIVE_HEIGHT - 60 - 60);
	nfui_regi_pre_event_callback(cam_fixed, pre_sub_fixed_event_handler);
	nfui_nfobject_show(cam_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, cam_fixed, pos_x, pos_y);

// TITLE
    pos_x = 8;
    pos_y = 10;
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "NETWORK CAMERA");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);

// RESCAN

    nfui_get_pixbuf_size(rescan_cam[0], &size_w, &size_h);
//    pos_x = cam_fixed->width - 8 - (size_w*5+20);
    pos_x = cam_fixed->width - 8 - (size_w*4+15);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), rescan_cam);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_cam_rescan_event_handler);

// ADD CAMERA
//    pos_x = cam_fixed->width - 8 - (size_w*4+15);
    pos_x = cam_fixed->width - 8 - (size_w*3+10);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), add_cam);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_cam_add_event_handler);
    addcam_x = pos_x;
    addcam_y = pos_y;
    
// FILTER
//    pos_x = cam_fixed->width - 8 - (size_w*3+10);
    pos_x = cam_fixed->width - 8 - (size_w*2+5);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), filter);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_cam_filter_event_handler);
	g_cam_filter = obj;
	filter_x = pos_x;
	filter_y = pos_y;
    
// ORDER
//    pos_x = cam_fixed->width - 8 - (size_w*2+5);
    pos_x = cam_fixed->width - 8 - (size_w*1);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), order);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_cam_order_event_handler);
	order_x = pos_x;
	order_y = pos_y;

// OPTION
    pos_x = cam_fixed->width - 8 - (size_w*1);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), option);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_option_event_handler);

// COUNTER TEXT
/*
    pos_x = cam_fixed->width - 8 - 350;
    pos_y = cam_fixed->height - 30 - 2;
    
	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(231));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 350, 30);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	g_assigned_cam_cnt_obj = obj;
*/

// CAM PAGE
    pos_x = 8;
    pos_y = 10 + 40 + 10 + 40 + 2;

	cam_page = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(cam_page, (CIS_CARD_DEF_WIDTH * 4) + (15 * 5), (CIS_CARD_DEF_HEIGHT * 4) + (15 * 5));
	nfui_nfobject_modify_bg(cam_page, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nffixed_put((NFFIXED*)cam_fixed, cam_page, pos_x, pos_y);
	nfui_regi_pre_event_callback(cam_page, pre_page_event_handler);
	nfui_nfobject_show(cam_page);

	init_cam_page(cam_page);

// COUNTER TEXT
    pos_x = cam_fixed->width - 8 - 350;
    pos_y = cam_page->y - 30 - 2;
    
	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(231));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 350, 30);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	g_assigned_cam_cnt_obj = obj;


// MOVE BUTTON	
	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);
	nfui_get_pixbuf_size(left_arrow[0], &size_w2, &size_h2);
	
    pos_x = (cam_fixed->width - ((size_w2 * 2) + (size_w * CIS_CAM_PAGE_CNT) + 110)) / 2;
    pos_y = cam_page->y + cam_page->height + 20 - ((size_h2-size_h)/2);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), left_arrow);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w2, (guint)size_h2);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_arrow_cam_page_move_event_handler);
	g_cam_arrow[0] = obj;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    pos_x = (cam_fixed->width - (size_w * CIS_CAM_PAGE_CNT + 90)) / 2;
    pos_y = cam_page->y + cam_page->height + 20;

	for (i = 0; i < CIS_CAM_PAGE_CNT; i++)
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_radio_cam_page_move_event_handler);
        g_cam_page[i] = obj;
        
		if(i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_radio_button_set_toggled((NFBUTTON*)obj, TRUE);
		} else {
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
			nfui_nfobject_disable(obj);
		}

		pos_y += size_h + 2;

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strpage_num[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, size_w, 30);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
		g_cam_page_num[i] = obj;
        
		if(i == 0) {
			nfui_nfobject_enable(obj);
		} else {
			nfui_nfobject_disable(obj);
		}		

		pos_x += size_w + 10;
        pos_y = cam_page->y + cam_page->height + 20;
	}
	
	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);
	nfui_get_pixbuf_size(right_arrow[0], &size_w2, &size_h2);
	
    pos_y = cam_page->y + cam_page->height + 20 - ((size_h2-size_h)/2);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), right_arrow);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w2, (guint)size_h2);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cam_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_arrow_cam_page_move_event_handler);
	g_cam_arrow[1] = obj;


	
//////////////////////////////////////////////
// NVR
//////////////////////////////////////////////

	pos_x = DISPLAY_ACTIVE_WIDTH - CIS_NVR_FIXED_W - 20;
	pos_y = 60;
	
	nvr_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(nvr_fixed, CIS_NVR_FIXED_W, DISPLAY_ACTIVE_HEIGHT - 60 - 60);
	nfui_regi_pre_event_callback(nvr_fixed, pre_sub_fixed_event_handler);
	nfui_nfobject_show(nvr_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, nvr_fixed, pos_x, pos_y);

// NVR LIST
	pos_x = 8;
	pos_y = 8;
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RECORDER CONTROL");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	
    nfui_get_pixbuf_size(option[0], &size_w, &size_h);
    pos_x += size_w + 5;

// MULTI		
	pos_x = nvr_fixed->width - 6 - 100;
	pos_y += 40 + 5;
	
	obj = nftool_normal_button_create_type2("MULTI", 100);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_multi_event_handler);
	g_nvr_multi = obj;

	pos_x = 8;
//	pos_y += 40 + 5;

    obj = nfui_nflabel_new_with_pango_font("RECORDER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 150, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);

    pos_x += 150 + 5;
    
	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_list_event_handler);
	g_nvr_list = obj;

	uxm_reg_imsg_event(obj, IRET_NVM_START_CAMERA_INSTALL);

    pos_x +=  450 + 5;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), rescan_cam);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
	nfui_nfobject_show(obj);
//	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_reload_event_handler);
	g_nvr_reload = obj;

    nfui_get_pixbuf_size(rescan_cam[0], &size_w, &size_h);
	pos_x += size_w + 5;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), option);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
//	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_setting_event_handler);
	g_nvr_setting = obj;

// RESTORE		
	pos_x = nvr_fixed->width - 6 - 100;
	
	obj = nftool_normal_button_create_type2("RESTORE", 100);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_restore_event_handler);

// NVR PAGE
    pos_x = 8;
    pos_y = 10 + 40 + 10 + 40 + 2;

	nvr_page = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(nvr_page, (CIS_CARD_DEF_WIDTH * 4) + (15 * 5), (CIS_CARD_DEF_HEIGHT * 4) + (15 * 5));
	nfui_nfobject_modify_bg(nvr_page, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nffixed_put((NFFIXED*)nvr_fixed, nvr_page, pos_x, pos_y);
	nfui_regi_pre_event_callback(nvr_page, pre_page_event_handler);
	nfui_nfobject_show(nvr_page);

	init_nvr_page(nvr_page, i);


//PAGE MOVE BUTTON

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);
	nfui_get_pixbuf_size(left_arrow[0], &size_w2, &size_h2);
	
    pos_x = (nvr_fixed->width - ((size_w2 * 2) + (size_w * CIS_NVR_PAGE_CNT) + 30)) / 2;
    pos_y = nvr_page->y + nvr_page->height + 20 - ((size_h2-size_h)/2);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), left_arrow);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w2, (guint)size_h2);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_arrow_nvr_page_move_event_handler);
	g_nvr_arrow[0] = obj;

    pos_x = (nvr_fixed->width - (size_w * CIS_NVR_PAGE_CNT + 10)) / 2;
    pos_y = nvr_page->y + nvr_page->height + 20;

	for (i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_radio_nvr_page_move_event_handler);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
        g_nvr_page[i] = obj;

		if(i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_radio_button_set_toggled((NFBUTTON*)obj, TRUE);
		} else {
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

		pos_y += size_h + 2;

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strpage_num[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, size_w, 30);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
		g_nvr_page_num[i] = obj;

		pos_x += size_w + 10;
        pos_y = nvr_page->y + nvr_page->height + 20;
	}
		
	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);
	nfui_get_pixbuf_size(right_arrow[0], &size_w2, &size_h2);
	
    pos_y = nvr_page->y + nvr_page->height + 20 - ((size_h2-size_h)/2);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), right_arrow);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w2, (guint)size_h2);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_arrow_nvr_page_move_event_handler);
	g_nvr_arrow[1] = obj;
    
// EXPORT	
	pos_x = 6;
	pos_y = nvr_fixed->height - 46;

	obj = nftool_normal_button_create_type2("EXPORT", 150);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_save_event_handler);
	
// LOAD	
	pos_x = 6 + 150 + 5;
	pos_y = nvr_fixed->height - 46;
	
	obj = nftool_normal_button_create_type2("LOAD", 150);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)nvr_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_nvr_load_event_handler);

// AUTO ASSIGN
	pos_x = (main_fixed->width - 120) / 2;
	pos_y = main_fixed->height / 2 - 130;

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, "%s >>", lookup_string("AUTO"));
	
	obj = nftool_normal_button_create_type2(buf, 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_auto_assign_event_handler);

// ASSIGN
	pos_x = (main_fixed->width - 120) / 2;
	pos_y = main_fixed->height / 2 - 80;
	
	obj = nftool_normal_button_create_type2(">>", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_assign_event_handler);

// UNASSIGN
	pos_x = (main_fixed->width - 120) / 2;
	pos_y = main_fixed->height / 2 + 40;

	obj = nftool_normal_button_create_type2("<<", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_unassign_event_handler);

// UNASSIGN ALL
	pos_x = (main_fixed->width - 120) / 2;
	pos_y = main_fixed->height / 2 + 90;

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, "<< %s", lookup_string("ALL"));

	obj = nftool_normal_button_create_type2(buf, 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_all_unassign_event_handler);

//CLOSE
	pos_x = DISPLAY_ACTIVE_WIDTH - 20 - MENU_BTN_WIDTH;
	pos_y = DISPLAY_ACTIVE_HEIGHT - 10 - MENU_BTN_HEIGHT;

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	pos_x = DISPLAY_ACTIVE_WIDTH - 20 - ((MENU_BTN_WIDTH * 2) + 5);

	obj = nftool_normal_button_create_type2("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	uxm_reg_imsg_event(obj, IRET_NVM_APPLY_CAMERA_SETTING);

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	uxm_reg_imsg_event(main_fixed, INFY_IPCAM_INSTALL_NOTIFY);
	uxm_reg_imsg_event(main_fixed, INFY_NVM_DATA_IS_CHANGED);
	uxm_reg_imsg_event(main_fixed, INFY_OPENMODE_READY_INSTALL_BY_NVM);
	uxm_reg_imsg_event(main_fixed, INFY_OPENMODE_FAIL_INSTALL_BY_NVM);
	uxm_reg_imsg_event(main_fixed, IRET_NVM_APPLY_AND_FINALIZE);
	uxm_reg_imsg_event(main_fixed, IRET_NVM_UPDATE_CONN_STATUS);
	uxm_reg_imsg_event(main_fixed, INFY_GET_IPCAM_THUMBNAIL);

    VW_Create_Manual_Add_Camera_Popup_Open(g_curwnd, cam_fixed->x + addcam_x, cam_fixed->y + addcam_y + 40 + 2);
    VW_Create_Camera_Filter_Menu_Open(g_curwnd, cam_fixed->x + filter_x, cam_fixed->y + filter_y + 40 + 2);
    VW_Create_Camera_Order_Menu_Open(g_curwnd, cam_fixed->x + order_x, cam_fixed->y + order_y + 40 + 2);
    VW_Create_Install_Search_Shortcut_Menu_Open(g_curwnd);
 
    _make_nvr_list(FALSE);
    _enable_nvr_page(FALSE);
    nfui_radio_button_set_toggled((NFBUTTON*)g_nvr_page[0], TRUE);
    _update_nvr_card_data(0);
    _update_nvr_page(0, FALSE);
    g_org_caminfo = nvm_get_cam_info_all(0);
    _init_local_ocam();
    
	if (g_opt_data.auto_scan) {
	    _scan_camera(FALSE);
    }
    
    iis_thumb_manager_start();

	nfui_page_open(PGID_IPCAMERA_INSTALL, win, ssm_get_cur_id(NULL));
	
	gtk_main();

	nfui_page_close(PGID_IPCAMERA_INSTALL, win);
	
	return 0;
}

int VW_IPCamInstallSearch_Page_ver2_mevt_cb(GdkEvent *event)
{
    gint card_idx = -1;
    
    if (!g_curwnd) return 0;
    
    if (event->type == GDK_BUTTON_RELEASE)
    {
        GdkEventButton *bevent;
        gint off_x, off_y;
        
        if (event->button.button != MOUSE_LEFT_BUTTON) return 0;
        if (!g_move_ch_mask && !g_assign_ch_mask) return 0;
        
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, NF_CURSOR_ARROW);

        bevent = (GdkEventButton *)event;

        card_idx = _get_inside_nvr_card_idx(bevent->x, bevent->y);
        if (card_idx == -1 || card_idx >= nvm_get_active_ch_cnt(_get_cur_nvr())) 
        {
    	    if (g_move_ch_mask) {
                if (g_ch_change_tmr) {
                    g_source_remove(g_ch_change_tmr);
                    g_ch_change_tmr = 0;
                }
                g_move_ch_mask = 0;
                g_from_ch = -1;
    	    }
    	    
    	    if (g_assign_ch_mask) {
                if (g_ch_assign_tmr) {
                    g_source_remove(g_ch_assign_tmr);
                    g_ch_assign_tmr = 0;
                }
                g_assign_ch_mask = 0;
                g_assign_cam = -1;
    	    }
    	    
            return 0;
        }
        
	    if (g_move_ch_mask) _finalize_ch_change(_get_info_idx_by_nvr_card(g_nvr_card_obj[card_idx]));
	    if (g_assign_ch_mask) _finalize_ch_assign(_get_info_idx_by_nvr_card(g_nvr_card_obj[card_idx]));
    }

    return 0;
}
