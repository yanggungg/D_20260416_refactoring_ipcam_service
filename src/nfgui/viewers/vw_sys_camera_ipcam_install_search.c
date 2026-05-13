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
#include "smt.h"
#include "ix_mem.h"

#include "vw.h"
#include "vw_desc.h"
#include "vw_vkeyboard.h"
#include "vw_sys_camera_ipcam_install.h"
#include "vw_sys_camera_ipcam_install_search.h"

enum {
	LIST_MODEL,
	LIST_ADDR,
	LIST_STATUS,
//	LIST_OCCUPIED,				/* TODO : changed focus*/
	LIST_ASSIGN,
	LIST_SETUP,
	LIST_PREVIEW,	
	LIST_COL_MAX
};

enum {
	SEARCH_TYPE_AUTO,
//	SEARCH_TYPE_RANGE,
	SEARCH_TYPE_SPECIFIC,
	SEARCH_TYPE_VIRTUAL,
	SEARCH_TYPE_MAX
};

#define LIST_ROW_MAX	(18)
#define PREVIEW_RATIO	(32)

#define STATUS_NORMAL_COLOR				(COLOR_IDX(231))
#define STATUS_ERROR_COLOR				(COLOR_PRG_IDX(UX_COLOR_FF0000))

#define STR_RESULT_ASSIGNED		"# OF ASSIGNED CAMERAS : %d"
#define STR_RESULT_FOUND		"# OF FOUND CAMERAS : %d"
#define STR_RESULT_NEEDED		"# OF CAMERAS NEEDED TO BE SET : %d"

static NFWINDOW *g_curwnd = 0;

static GdkPixbuf *g_setup_img[2][NFOBJECT_STATE_COUNT];
static GdkPixbuf *g_preview_img[2][NFOBJECT_STATE_COUNT];

static NFOBJECT *g_type_fixed[SEARCH_TYPE_MAX];
static NFOBJECT *g_preview_fixed;

static NFOBJECT *g_search_btn;
static NFOBJECT *g_close_btn;
static NFOBJECT *g_assign_btn;

static NFOBJECT *g_searchType_obj;
static NFOBJECT *g_ipRange_obj[2];
static NFOBJECT *g_ipHost_obj;
static NFOBJECT *g_port_obj;
static NFOBJECT *g_list_obj[LIST_ROW_MAX][LIST_COL_MAX];
static NFOBJECT *g_ethernet_obj[LIST_ROW_MAX] = {0, };
static NFOBJECT *g_page_obj;
static NFOBJECT *g_preview_text_obj;

static NFOBJECT *g_model_sort_obj;
static NFOBJECT *g_address_sort_obj;
static NFOBJECT *g_status_sort_obj;

static NFOBJECT *g_assign_obj;
static NFOBJECT *g_found_obj;
static NFOBJECT *g_needed_obj;
static NFOBJECT *g_loading_obj;

static gint g_curr_page = 0;
static gint g_total_page = 0;

static guint g_loading_timer = 0;
static guint g_setupFlicker_timer = 0;

static gint g_listobj_status[LIST_ROW_MAX] = {0, };
static gint g_retVal = 0;
static gint g_page_auto_update = 0;

static gint _draw_select_color_search_list(gint row, gboolean expose)
{
	gint i;

	if ((row < 0) || (row >= LIST_ROW_MAX)) return -1;
	if (nfui_nfobject_get_bg_color(g_list_obj[row][0], NFOBJECT_STATE_NORMAL) == COLOR_IDX(147)) return -1;
	
	for (i = 0; i < LIST_ASSIGN; i++)
	{			
		nfui_nflabel_set_pango_font((NFLABEL*)g_list_obj[row][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), STATUS_NORMAL_COLOR);
		nfui_nfobject_modify_bg(g_list_obj[row][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(147));
		if (expose) nfui_signal_emit(g_list_obj[row][i], GDK_EXPOSE, TRUE);		
	}

	return 0;
}

static gint _draw_focus_color_search_list(gint row, gboolean expose)
{
	gint i;

	if ((row < 0) || (row >= LIST_ROW_MAX)) return -1;
	if (nfui_nfobject_get_bg_color(g_list_obj[row][0], NFOBJECT_STATE_NORMAL) == COLOR_IDX(146)) return -1;
	
	for (i = 0; i < LIST_ASSIGN; i++)
	{			
		nfui_nflabel_set_pango_font((NFLABEL*)g_list_obj[row][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), STATUS_NORMAL_COLOR);
		nfui_nfobject_modify_bg(g_list_obj[row][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(146));
		if (expose) nfui_signal_emit(g_list_obj[row][i], GDK_EXPOSE, TRUE);		
	}

	return 0;
}

static gint _erase_color_search_list(gint row, gboolean expose)
{
	gint i;

	if ((row < 0) || (row >= LIST_ROW_MAX)) return -1;
	if (nfui_nfobject_get_bg_color(g_list_obj[row][0], NFOBJECT_STATE_NORMAL) == COLOR_IDX(230)) return -1;

	for (i = 0; i < LIST_ASSIGN; i++)
	{			
		if (i == LIST_STATUS)
		{
			if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_list_obj[row][i]), STR_OK) == 0)
				nfui_nflabel_set_pango_font((NFLABEL*)g_list_obj[row][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), STATUS_NORMAL_COLOR);
			else
				nfui_nflabel_set_pango_font((NFLABEL*)g_list_obj[row][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), STATUS_ERROR_COLOR);
		}
		else
			nfui_nflabel_set_pango_font((NFLABEL*)g_list_obj[row][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), STATUS_NORMAL_COLOR);

		nfui_nfobject_modify_bg(g_list_obj[row][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		if (expose) nfui_signal_emit(g_list_obj[row][i], GDK_EXPOSE, TRUE);		
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

static gint _is_possible_preview(gint status_in, gboolean *pss)
{
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
			(status_in == OPENMODE_CAM_STATE_DEV_INFO_ONVIF) ||
			(status_in == OPENMODE_CAM_STATE_VIRTUAL_CAMERA))
	{
		*pss = TRUE;
	}	
	else
	{
		*pss = FALSE;
	}

	return 0;
}

static NFOpenmodeCamInfo *_find_matched_info(NFOpenmodeDeviceList *dlist, gint f_index)
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

static gint _update_search_list_model(gint row, gchar *str)
{
	gint i;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_list_obj[row][LIST_MODEL]), str) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)g_list_obj[row][LIST_MODEL], str);
		nfui_signal_emit(g_list_obj[row][LIST_MODEL], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _update_search_list_addr(gint row, gchar *str)
{
	gint i;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_list_obj[row][LIST_ADDR]), str) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)g_list_obj[row][LIST_ADDR], str);
		nfui_signal_emit(g_list_obj[row][LIST_ADDR], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _update_search_list_ethernet(gint row, gchar *str)
{
	gint i;

	if (!g_ethernet_obj[row]) return -1;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_ethernet_obj[row]), str) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)g_ethernet_obj[row], str);
		nfui_signal_emit(g_ethernet_obj[row], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _update_search_list_status(gint row, gchar *str)
{
	gint i;

	if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_list_obj[row][LIST_STATUS]), str) != 0)
	{
		if (strcmp(str, STR_OK) == 0)
			nfui_nflabel_modify_fg((NFLABEL*)g_list_obj[row][LIST_STATUS], STATUS_NORMAL_COLOR);		
		else
			nfui_nflabel_modify_fg((NFLABEL*)g_list_obj[row][LIST_STATUS], STATUS_ERROR_COLOR);

		nfui_nflabel_set_text((NFLABEL*)g_list_obj[row][LIST_STATUS], str);
		nfui_signal_emit(g_list_obj[row][LIST_STATUS], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _change_color_assigned_channel_object()
{
	NFOpenmodeDeviceList *dlist = NULL;
	NFOpenmodeCamInfo *info = NULL;
	guint chmask = 0;

	NFOBJECT *item;
	gint i, j;
	gint color_idx;

	dlist = nf_openmode_get_ch_list();
	if (dlist == 0) return -1;

	info = dlist->head;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if ((info) && (info->state != OPENMODE_CAM_STATE_INIT)) chmask |= (1 << i);
		
		info = info->next;
	}

	for (i = 0; i < LIST_ROW_MAX; i++)
	{
		if (nfui_nfobject_is_disabled(g_list_obj[i][LIST_ASSIGN])) continue;
	
		for (j = 0; j < GUI_CHANNEL_CNT; j++)
		{
			if (chmask & (1 << j)) color_idx = 1;
			else color_idx = 0;
		
			if (((NFCOMBOBOX*)g_list_obj[i][LIST_ASSIGN])->colormap[j+1] != color_idx)
			{
				nfui_combobox_set_colormap((NFCOMBOBOX*)g_list_obj[i][LIST_ASSIGN], j+1, color_idx);
			}
		}
	}

	return 0;
}

static gint _update_search_list_assign(gint row, gint ch_idx, gboolean enable)
{
	gint combo_idx;
	gint is_changed = 0;

	if (ch_idx == -1)					combo_idx = 0;
	else if (ch_idx >= GUI_CHANNEL_CNT) combo_idx = 0;
	else								combo_idx = ch_idx+1;
	
	if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_list_obj[row][LIST_ASSIGN]) != combo_idx)
	{
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_list_obj[row][LIST_ASSIGN], combo_idx);
		is_changed = 1;
	}

	if (nfui_nfobject_is_disabled(g_list_obj[row][LIST_ASSIGN]) == enable)
	{
		if (enable) nfui_nfobject_enable(g_list_obj[row][LIST_ASSIGN]);
		else		nfui_nfobject_disable(g_list_obj[row][LIST_ASSIGN]);

		is_changed = 1;
	}

	if (is_changed) 
		nfui_signal_emit(g_list_obj[row][LIST_ASSIGN], GDK_EXPOSE, TRUE);

	return 0;
}

static gint _update_search_list_setup(gint row, gboolean enable)
{
	if (nfui_nfobject_is_disabled(g_list_obj[row][LIST_SETUP]) == enable)
	{
		if (enable) nfui_nfobject_enable(g_list_obj[row][LIST_SETUP]);
		else		nfui_nfobject_disable(g_list_obj[row][LIST_SETUP]);

		nfui_signal_emit(g_list_obj[row][LIST_SETUP], GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _update_search_list_preview(gint row, gint idx, gboolean enable)
{
	gint img_idx;

	img_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(g_list_obj[row][LIST_PREVIEW], "image index"));
	
	if ((img_idx != idx) || (nfui_nfobject_is_disabled(g_list_obj[row][LIST_PREVIEW]) == enable))
	{
		nfui_nfbutton_set_image(NF_BUTTON(g_list_obj[row][LIST_PREVIEW]), g_preview_img[idx]);
		nfui_nfobject_set_data(g_list_obj[row][LIST_PREVIEW], "image index", GINT_TO_POINTER(idx));

		if (enable) nfui_nfobject_enable(g_list_obj[row][LIST_PREVIEW]);
		else		nfui_nfobject_disable(g_list_obj[row][LIST_PREVIEW]);	

		nfui_signal_emit(g_list_obj[row][LIST_PREVIEW], GDK_EXPOSE, TRUE);		
	}

	return 0;
}

static gint _set_list_info_idx(gint row, gint info_index)
{
	nfui_nfobject_set_data(g_list_obj[row][LIST_MODEL], "info_index", GINT_TO_POINTER(info_index));
	return 0;
}

static gint _translate_search_result(gint cnt, gchar *str_in, gchar *str_out)
{
	g_sprintf(str_out, lookup_string(str_in), cnt);		
	return 0;
}

static gint _set_search_result(gchar *str_assign, gchar *str_found, gchar *str_needed)
{
	if (str_assign)
	{
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_assign_obj), str_assign) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_assign_obj, str_assign);
			nfui_signal_emit(g_assign_obj, GDK_EXPOSE, TRUE);
		}
	}

	if (str_found)
	{
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_found_obj), str_found) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_found_obj, str_found);
			nfui_signal_emit(g_found_obj, GDK_EXPOSE, TRUE);
		}
	}

	if (str_needed)
	{
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_needed_obj), str_needed) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_needed_obj, str_needed);
			nfui_signal_emit(g_needed_obj, GDK_EXPOSE, TRUE);
		}
	}
	
	return 0;
}

static gint _update_search_list(NFOpenmodeDeviceList *dlist, gint page)
{
	gint i, j;
	NFOpenmodeCamInfo *info = NULL;
	gchar strBuf[64];
	gint update_cnt = 0;
	gboolean pss_setup, pss_preview;
	gchar strEth[32];	

	memset(strBuf, 0x00, sizeof(strBuf));

	info = _find_matched_info(dlist, LIST_ROW_MAX*page);
	if (info == NULL) return -1;

	update_cnt = dlist->entry_cnt - LIST_ROW_MAX*page;

	_translate_search_result(dlist->assigned_cnt, STR_RESULT_ASSIGNED, strBuf);
	_set_search_result(strBuf, 0, 0);
	
	_translate_search_result(dlist->recognized_cnt, STR_RESULT_FOUND, strBuf);
	_set_search_result(0, strBuf, 0);
	
	_translate_search_result(dlist->setup_needed_cnt, STR_RESULT_NEEDED, strBuf);
	_set_search_result(0, 0, strBuf);

	for (i = 0; i < LIST_ROW_MAX; i++)
	{
		if (i < update_cnt)
		{
			memset(strEth, 0x00, sizeof(strEth));
			if (strcmp(info->eth_dev, "eth0") == 0) strcpy(strEth, "LAN1");
			else if (strcmp(info->eth_dev, "eth1") == 0) strcpy(strEth, "LAN2");
			else strcpy(strEth, "-");

			_update_search_list_model(i, info->model);
			_update_search_list_addr(i, info->hostname);
			_update_search_list_ethernet(i, strEth);
			_translate_status(info->state, strBuf);
			_update_search_list_status(i, strBuf);			
			_update_search_list_assign(i, info->ch, TRUE);	
			_is_possible_setup(info->state, &pss_setup);			
			_update_search_list_setup(i, pss_setup);	
			_is_possible_preview(info->state, &pss_preview);			
			_update_search_list_preview(i, 0, pss_preview);
			_set_list_info_idx(i, info->index);

			info = info->next;
		}
		else
		{
			_update_search_list_model(i, "");
			_update_search_list_addr(i, "");
			_update_search_list_ethernet(i, "");
			_update_search_list_status(i, "");
			_update_search_list_assign(i, -1, FALSE);	
			_update_search_list_setup(i, FALSE);
			_update_search_list_preview(i, 0, FALSE);			
			_set_list_info_idx(i, -1);
		}
	}

	return 0;
}

static gint _clear_search_result()
{
	gint i;
	gchar strBuf[64];

	memset(strBuf, 0x00, sizeof(strBuf));

	_translate_search_result(0, STR_RESULT_ASSIGNED, strBuf);
	_set_search_result(strBuf, 0, 0);
	
	_translate_search_result(0, STR_RESULT_FOUND, strBuf);
	_set_search_result(0, strBuf, 0);
	
	_translate_search_result(0, STR_RESULT_NEEDED, strBuf);
	_set_search_result(0, 0, strBuf);

	for (i = 0; i < LIST_ROW_MAX; i++)
	{
		_update_search_list_model(i, "");
		_update_search_list_addr(i, "");
		_update_search_list_ethernet(i, "");
		_update_search_list_status(i, "");
		_update_search_list_assign(i, -1, FALSE);	
		_update_search_list_setup(i, FALSE);		
		_set_list_info_idx(i, -1);
		_update_search_list_preview(i, 0, FALSE);
	}

	return 0;
}

static gint _update_page_info(gint cur_page, gint tot_page)
{
	gchar strBuf[10];

	if ((g_curr_page == cur_page) && (g_total_page == tot_page)) return -1;
	
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "%d / %d", cur_page+1, tot_page+1);

	nfui_nflabel_set_text((NFLABEL*)g_page_obj, strBuf);
	nfui_signal_emit(g_page_obj, GDK_EXPOSE, TRUE);

	g_curr_page = cur_page;
	g_total_page = tot_page;

	return 0;
}

static gint _stop_preview(gint on_preview_text)
{
	vsm_live_preview_stop();

	if (on_preview_text)
		nfui_nfobject_show(g_preview_text_obj);	
	else
		nfui_nfobject_hide(g_preview_text_obj);	

	nfui_nfobject_modify_bg(g_preview_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_signal_emit(g_preview_fixed, GDK_EXPOSE, TRUE);

	return 0;
}

static gint _start_preview(gint row)
{
    gint gap_x, gap_y;
	gint size_w, size_h;
	gint info_idx;
	gchar *state_str;
	
	info_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(g_list_obj[row][LIST_MODEL], "info_index"));
	state_str = nfui_nflabel_get_text((NFLABEL*)g_list_obj[row][LIST_STATUS]);
		
	if ((info_idx == -1) || (strcmp(state_str, STR_EMPTY) == 0))
	{
		_stop_preview(0);
		return -1;
	}

	if (strcmp(state_str, STR_OK) != 0)
	{
		_stop_preview(1);
		return -1;
	}
	else
	{
	    nfui_nfobject_get_offset(g_preview_fixed, &gap_x, &gap_y);
	    nfui_nfobject_get_size(g_preview_fixed, &size_w, &size_h);

		nf_openmode_set_preview(info_idx, 0);
		g_message("%s, %d, preview_index:%d", __FUNCTION__, __LINE__, info_idx);	
		
		vsm_live_preview_start(1<<0, gap_x, gap_y, size_w, size_h);

		nfui_nfobject_hide(g_preview_text_obj);
		nfui_nfobject_modify_bg(g_preview_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
		nfui_signal_emit(g_preview_fixed, GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gint _do_setup_cancel()
{
	if (g_setupFlicker_timer)
	{
		g_source_remove(g_setupFlicker_timer);
		g_setupFlicker_timer = 0;
	}

	_stop_preview(0);
	_update_page_info(0, 0);
	
	return 0;
}

static gint _do_setup_apply()
{

	return 0;
}

static gboolean _search_loading_timer(gpointer data)
{
	static gint img_idx = 0;
	
	if (img_idx >= 5) img_idx = 0;

	NFUTIL_THREADS_ENTER();

	if (img_idx == 0) 		nfui_nfimage_change_image((NFIMAGE*)g_loading_obj, IMG_LOADING_1);
	else if (img_idx == 1)	nfui_nfimage_change_image((NFIMAGE*)g_loading_obj, IMG_LOADING_2);
	else if (img_idx == 2)	nfui_nfimage_change_image((NFIMAGE*)g_loading_obj, IMG_LOADING_3);
	else if (img_idx == 3)	nfui_nfimage_change_image((NFIMAGE*)g_loading_obj, IMG_LOADING_4);
	else if (img_idx == 4)	nfui_nfimage_change_image((NFIMAGE*)g_loading_obj, IMG_LOADING_5);

	nfui_signal_emit(g_loading_obj, GDK_EXPOSE, TRUE);

	NFUTIL_THREADS_LEAVE();

	img_idx++;

	return TRUE;
}

static gboolean _flicker_setup_button_timer(gpointer data)
{
	static gint img_idx = 0;
	gchar *state_str;
	gint i;

	img_idx = img_idx % 2;

	NFUTIL_THREADS_ENTER();

	for (i = 0; i < LIST_ROW_MAX; i++)
	{
		state_str = nfui_nflabel_get_text((NFLABEL*)g_list_obj[i][LIST_STATUS]);

		if ((strcmp(state_str, STR_OK) != 0) && (nfui_nfobject_is_disabled(g_list_obj[i][LIST_SETUP]) == FALSE))
		{
			nfui_nfbutton_set_image(NF_BUTTON(g_list_obj[i][LIST_SETUP]), g_setup_img[img_idx]);	
			nfui_signal_emit(g_list_obj[i][LIST_SETUP], GDK_EXPOSE, TRUE);
		}
	}

	NFUTIL_THREADS_LEAVE();

	img_idx++;

	return TRUE;
}

static gint _set_setup_button_image()
{
	gint i, ok_cnt = 0;
	gchar *state_str;

	for (i = 0; i < LIST_ROW_MAX; i++)
	{
		state_str = nfui_nflabel_get_text((NFLABEL*)g_list_obj[i][LIST_STATUS]);

		if (strcmp(state_str, STR_OK) == 0)
		{
			nfui_nfbutton_set_image(NF_BUTTON(g_list_obj[i][LIST_SETUP]), g_setup_img[0]);
			nfui_signal_emit(g_list_obj[i][LIST_SETUP], GDK_EXPOSE, TRUE);
			ok_cnt++;
		}
	}

	if (ok_cnt != LIST_ROW_MAX)
	{
		if (g_setupFlicker_timer == 0)
			g_setupFlicker_timer = g_timeout_add(500, _flicker_setup_button_timer, 0);
	}

	return 0;
}

static gint _find_row_matched_list(NFOBJECT *obj)
{
	gint i, j;

	for(i = 0; i < LIST_ROW_MAX; i++)
	{
		for (j = 0; j < LIST_ASSIGN; j++)
		{
			if (g_list_obj[i][j] == obj) return i;
		}			
	}

	g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);

	return -1;
}

static gint _find_col_matched_list(NFOBJECT *obj, gint row)
{
	gint i;

	for(i = 0; i < LIST_ASSIGN; i++)
	{
		if (g_list_obj[row][i] == obj) return i;
	}

	g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);

	return -1;
}

static gint _is_focus_row(gint row)
{
	gint i;

	for(i = 0; i < LIST_ASSIGN; i++)
	{
		if (g_list_obj[row][i]->kfocus == NFOBJECT_FOCUS) return 0;
	}

	return -1;
}

static gint _is_select_row(gint row)
{
	gint i;

//	if (g_select_row == row) return 0;

	return -1;
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean _check_port_validity(gint port_num)
{
/*
	if (port_num >= 65536) {
		nftool_mbox(g_curwnd, "NOTICE", "The port number must be less than 65536.", NFTOOL_MB_OK);
		return FALSE;
	}
	
	if (port_num != 80 && port_num <= 1024) {
		nftool_mbox(g_curwnd, "NOTICE", "Port number must be 80 or\nlarge than 1024.", 
				NFTOOL_MB_OK);
		return FALSE;
	}
*/

	return TRUE;
}

static gint _check_possible_specific_search()
{
	gchar *host;
	gint port;

	host = nfui_nflabel_get_text((NFLABEL*)g_ipHost_obj);
	port = nfui_nflabel_get_number((NFLABEL*)g_port_obj);

	if (strlen(host) && (port != 0))
		nfui_nfobject_enable(g_search_btn);		
	else
		nfui_nfobject_disable(g_search_btn);				

	nfui_signal_emit(g_search_btn, GDK_EXPOSE, TRUE);

	return 0;
}

static gboolean post_auto_asign_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	bool is_changed = 0;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint combo_idx = 1;
		NFOpenmodeDeviceList *dlist = NULL;
		NFOpenmodeCamInfo *info = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;				
		
		dlist = nf_openmode_get_list();
		
		for (i = 0; i < dlist->entry_cnt; i++)
		{
			info = _find_matched_info(dlist, i);
			if (info == NULL) break;

			if( (combo_idx > GUI_CHANNEL_CNT) )
				break;
				
			switch (info->state)
			{
				case OPENMODE_CAM_STATE_DISCOVERED:
				case OPENMODE_CAM_STATE_DEV_INFO:
				case OPENMODE_CAM_STATE_DEV_INFO_ONVIF:
				case OPENMODE_CAM_STATE_ASSIGN_CH:
				case OPENMODE_CAM_STATE_OK:
				case OPENMODE_CAM_STATE_VIRTUAL_CAMERA:
					nf_openmode_set_channel(i, combo_idx-1);
					combo_idx++;
					break;
				default:
					nf_openmode_set_channel(i, -1);
					break;
			}
		}
	}

	return FALSE;
}

static gboolean post_search_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint type;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;				

		if (g_setupFlicker_timer)
		{
			g_source_remove(g_setupFlicker_timer);
			g_setupFlicker_timer = 0;
		}

		_stop_preview(0);

		type = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_searchType_obj);

		if (type == SEARCH_TYPE_AUTO)
		{
			_clear_search_result();
			_update_page_info(0, 0);
			nf_openmode_scan_camera();
		}
		else if (type == SEARCH_TYPE_SPECIFIC)
		{
			gchar *host;
			gint port;

			host = nfui_nflabel_get_text((NFLABEL*)g_ipHost_obj);
			port = nfui_nflabel_get_number((NFLABEL*)g_port_obj);
		
		    g_page_auto_update = 1;
			nf_openmode_add_device_manual(host, port);
		}
		else if(type == SEARCH_TYPE_VIRTUAL)
		{
		    g_page_auto_update = 1;
		    VW_virtual_camera_set_Popup(g_curwnd, 200, 200);
		}

	}

	return FALSE;
}

static gboolean post_searchType_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		gint i;

		if (index == SEARCH_TYPE_AUTO)
		{
//			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_RANGE]);
			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_SPECIFIC]);			
			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_VIRTUAL]);
			nfui_nfobject_show(g_type_fixed[SEARCH_TYPE_AUTO]);

			nfui_nfbutton_set_text((NFBUTTON*)g_search_btn, "SEARCH");
			nfui_nfobject_enable(g_search_btn);
		}
		else if (index == SEARCH_TYPE_SPECIFIC)
		{
			nfui_nflabel_set_text(g_ipHost_obj, "");
			nfui_nflabel_set_number(g_port_obj, 0);

			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_AUTO]);
//			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_RANGE]);			
            nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_VIRTUAL]);
			nfui_nfobject_show(g_type_fixed[SEARCH_TYPE_SPECIFIC]);

			nfui_nfbutton_set_text((NFBUTTON*)g_search_btn, "ADD");
			nfui_nfobject_disable(g_search_btn);
		}

		else if (index == SEARCH_TYPE_VIRTUAL)
		{
		    nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_AUTO]);
		    nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_SPECIFIC]);
		    nfui_nfobject_show(g_type_fixed[SEARCH_TYPE_VIRTUAL]);
		    
		    nfui_nfbutton_set_text((NFBUTTON*)g_search_btn, "VCAMERA ADD");
            nfui_nfobject_enable(g_search_btn);		    
		}
/*		
		else if (index == SEARCH_TYPE_RANGE)
		{
			nfui_nflabel_set_text(g_ipRange_obj[0], "");
			nfui_nflabel_set_text(g_ipRange_obj[1], "");

			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_AUTO]);
			nfui_nfobject_hide(g_type_fixed[SEARCH_TYPE_SPECIFIC]);			
			nfui_nfobject_show(g_type_fixed[SEARCH_TYPE_RANGE]);			
		}		
*/		

		for (i = 0; i < SEARCH_TYPE_MAX; i++)	
		{
			nfui_signal_emit(g_type_fixed[i], GDK_EXPOSE, TRUE);
		}

		nfui_signal_emit(g_search_btn, GDK_EXPOSE, TRUE);

		nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean post_ipHost_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *ipHost;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        mb_type ret = NFTOOL_MB_OK;

		if (kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{					
				return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}

        ipHost = nfui_nflabel_get_text((NFLABEL*)obj);
       
   		strTemp = VirtualKey_Open(g_curwnd, ipHost, x, y, 64, VKEY_NORMAL);        

		if (strTemp) 
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
			ifree(strTemp);
			strTemp = NULL;
		}

		_check_possible_specific_search();
	}
	
	return FALSE;
}

static gboolean post_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if (numRet == -1) return FALSE;

		if (_check_port_validity(numRet)) 
		{
			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}

		_check_possible_specific_search();		
	}

	return FALSE;
}

/*
static gboolean post_search_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	NFOBJECT* cur_focus;
	gint i, j;

	if (evt->type == GDK_EXPOSE)
	{
		i = _find_row_matched_list(obj);
		
		if (g_listobj_status[i] == NFOBJECT_STATE_PRELIGHT)
		{
			_draw_focus_color_search_list(i, TRUE);
		}
		else if (g_listobj_status[i] == NFOBJECT_STATE_ACTIVE)
		{
			_draw_select_color_search_list(i, TRUE);
		}		
		else
		{
			_erase_color_search_list(i, TRUE);
		}
	}
	else if (evt->type == GDK_BUTTON_PRESS || evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
		{
			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;
		}

		if (evt->type == GDK_BUTTON_PRESS || kpid == KEYPAD_ENTER)
		{	
			i = _find_row_matched_list(obj);

			if (g_listobj_status[i] != NFOBJECT_STATE_ACTIVE)
			{
				g_listobj_status[i] = NFOBJECT_STATE_ACTIVE;

				for (j = 0; j < LIST_ASSIGN; j++)
					nfui_signal_emit(g_list_obj[i][j], GDK_EXPOSE, TRUE);
			}	
			
			_start_preview(i);
		}
	}
	else if (evt->type == GDK_ENTER_NOTIFY)
	{
		i = _find_row_matched_list(obj);
		
		if (g_listobj_status[i] != NFOBJECT_STATE_ACTIVE)
		{
			g_listobj_status[i] = NFOBJECT_STATE_PRELIGHT;

			for (j = 0; j < LIST_ASSIGN; j++)
				nfui_signal_emit(g_list_obj[i][j], GDK_EXPOSE, TRUE);
		}	
	}	
	else if (evt->type == GDK_LEAVE_NOTIFY)
	{
		i = _find_row_matched_list(obj);

		if (g_listobj_status[i] != NFOBJECT_STATE_ACTIVE)
		{
			g_listobj_status[i] = NFOBJECT_STATE_NORMAL;

			for (j = 0; j < LIST_ASSIGN; j++)
				nfui_signal_emit(g_list_obj[i][j], GDK_EXPOSE, TRUE);
		}
	}	
	
	return FALSE;
}
*/

static gboolean post_sort_channel_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		gint dir;
	
		if (g_setupFlicker_timer)
		{
			g_source_remove(g_setupFlicker_timer);
			g_setupFlicker_timer = 0;
		}

		_stop_preview(0);
		_clear_search_result();
		_update_page_info(0, 0);

		dir = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

		if ((dir == 0) || (dir == 1)) 
		{
			nf_openmode_sort_by_ch(dir);
		}
		else 
		{
			NFOpenmodeDeviceList *dlist = NULL;
			NFOpenmodeCamInfo *info = NULL;
			gint i;
		
			dlist = nf_openmode_get_list();
			
			for (i = 0; i < dlist->entry_cnt; i++)
			{
				info = _find_matched_info(dlist, i);
				if (info == NULL) break;
					
				nf_openmode_set_channel(i, -1);
			}
		}		
	}

	return FALSE;
}

static gboolean post_sort_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		gint dir;
	
		if (g_setupFlicker_timer)
		{
			g_source_remove(g_setupFlicker_timer);
			g_setupFlicker_timer = 0;
		}

		_stop_preview(0);
		_clear_search_result();
		_update_page_info(0, 0);

		dir = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

		if (g_model_sort_obj == obj) nf_openmode_sort_by_model(dir);
		else if (g_address_sort_obj == obj) nf_openmode_sort_by_ip(dir);
		else if (g_status_sort_obj == obj) nf_openmode_sort_by_status(dir);
	}

	return FALSE;
}

static gboolean post_ethernet_sort_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		gint dir;
	
		if (g_setupFlicker_timer)
		{
			g_source_remove(g_setupFlicker_timer);
			g_setupFlicker_timer = 0;
		}

		_stop_preview(0);
		_clear_search_result();
		_update_page_info(0, 0);

		dir = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		nf_custommode_sort_by_lan(dir);
	}

	return FALSE;
}

static gboolean post_assign_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		gint i, ch;
		gint info_idx;

		for (i = 0; i < LIST_ROW_MAX; i++)
		{
			if (g_list_obj[i][LIST_ASSIGN] == obj) break;
		}

		if (i >= LIST_ROW_MAX) return FALSE;

		info_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(g_list_obj[i][LIST_MODEL], "info_index"));
		ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		nf_openmode_set_channel(info_idx, ch-1);
//		_change_color_assigned_channel_object();
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
		
		for (i = 0; i < LIST_ROW_MAX; i++)
		{
			if (g_list_obj[i][LIST_SETUP] == obj) break;
		}

		if (i >= LIST_ROW_MAX) return FALSE;

		info_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(g_list_obj[i][LIST_MODEL], "info_index"));

		dlist = nf_openmode_get_list();
		if (dlist == NULL) return FALSE;

		info = _find_matched_info(dlist, info_idx);
		if (info == NULL) return FALSE;

		for (i = 0; i < LIST_ROW_MAX; i++)
		{
			if (nfui_nfobject_is_disabled(g_list_obj[i][LIST_PREVIEW]) == FALSE)
			{
				_update_search_list_preview(i, 0, TRUE);
			}		
		}

		_stop_preview(0);
		nf_openmode_set_preview(info_idx, 0);
		g_message("%s, %d, preview_index:%d", __FUNCTION__, __LINE__, info_idx);			
		
		VW_Open_Cam_Ipcam_Setup_Popup(g_curwnd, info);
	}

	return FALSE;
}

static gboolean post_preview_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, img_idx;
	gint retVal;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		for (i = 0; i < LIST_ROW_MAX; i++)
		{
			if (nfui_nfobject_is_disabled(g_list_obj[i][LIST_PREVIEW]) == FALSE)
			{
				if (g_list_obj[i][LIST_PREVIEW] == obj)
				{
					img_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "image index"));
					img_idx = (img_idx+1)%2;

					_update_search_list_preview(i, img_idx, TRUE);
					
					if (img_idx) _start_preview(i);
					else 		 _stop_preview(0);	
				}
				else
				{
					_update_search_list_preview(i, 0, TRUE);
				}
			}		
		}
	}

	return FALSE;
}

static gboolean post_prev_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{	
		case GDK_BUTTON_RELEASE:
        {
			NFOpenmodeDeviceList *dlist = NULL;
		
            if (g_curr_page == 0) return FALSE;

			if (g_setupFlicker_timer)
			{
				g_source_remove(g_setupFlicker_timer);
				g_setupFlicker_timer = 0;
			}

			_stop_preview(0);
			dlist = nf_openmode_get_list();
			_update_search_list(dlist, g_curr_page-1);            
			_update_page_info(g_curr_page-1, g_total_page);
			_set_setup_button_image();
        }
		break;

		default:
		break;
	}
	
	return FALSE;
}

static gboolean post_next_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{	
		case GDK_BUTTON_RELEASE:
        {
			NFOpenmodeDeviceList *dlist = NULL;

            if (g_curr_page+1 > g_total_page) return FALSE;

			if (g_setupFlicker_timer)
			{
				g_source_remove(g_setupFlicker_timer);
				g_setupFlicker_timer = 0;
			}

			_stop_preview(0);
			dlist = nf_openmode_get_list();
			_update_search_list(dlist, g_curr_page+1);            
			_update_page_info(g_curr_page+1, g_total_page);
			_set_setup_button_image();			
        }
		break;

		default:
		break;
	}
	
	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_do_setup_cancel();	
	}
	
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_do_setup_apply();	
	}
	
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	mb_type ret;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean post_install_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, j;

	switch(evt->type)
	{
		case GDK_DELETE:
		{
			g_curwnd = 0;
			gtk_main_quit();			
		}
		break;		
		
		default:
		break;
	}

	return FALSE;
}

static gboolean post_main_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
        gc = gdk_gc_new(drawable);

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
	
		uxm_unreg_imsg_event(obj, INFY_IPCAM_INSTALL_NOTIFY);        
	}
	else if (evt->type == INFY_IPCAM_INSTALL_NOTIFY)
	{
		NFOpenmodeDeviceList *dlist = NULL;
		NF_NOTIFY_INFO *pInfo;

		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

		if (pInfo->d.params[0] == 1)
		{
			if (g_loading_timer == 0)
			{
				nfui_ui_lock();			
				g_loading_timer = g_timeout_add(300, _search_loading_timer, 0);				
			}
		} 
		else if (pInfo->d.params[0] == 0)
		{
			if (g_loading_timer)
			{		
				nfui_ui_unlock();

				g_source_remove(g_loading_timer);
				g_loading_timer = 0;

				nfui_nfimage_change_image((NFIMAGE*)g_loading_obj, IMG_LOADING_0);
				nfui_signal_emit(g_loading_obj, GDK_EXPOSE, TRUE);
			}

			_set_setup_button_image();
			_change_color_assigned_channel_object();
		}
		
		dlist = nf_openmode_get_list();

		if (dlist->entry_cnt <= 0) return FALSE;

		if (g_page_auto_update == 1)
		{
		    if (g_curr_page != ((dlist->entry_cnt-1)/LIST_ROW_MAX)) g_curr_page = ((dlist->entry_cnt-1)/LIST_ROW_MAX);
		    
    		_update_search_list(dlist, g_curr_page);
    		_update_page_info(g_curr_page, (dlist->entry_cnt-1)/LIST_ROW_MAX);		
		    g_page_auto_update = 0;
		}
		else 
		{
    		_update_search_list(dlist, g_curr_page);
    		_update_page_info(g_curr_page, (dlist->entry_cnt-1)/LIST_ROW_MAX);		
		}
	}
	
	return FALSE;
}

static gboolean post_result_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
        gint gap_x, gap_y;

		drawable = nfui_nfobject_get_window(obj);
        gc = gdk_gc_new(drawable);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);    
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	}
	
	return FALSE;
}

static gboolean post_preview_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{     
        gint gap_x, gap_y;

		if (nfui_nfobject_get_bg_color(obj, NFOBJECT_STATE_NORMAL) == COLOR_IDX(200))
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

	        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
	        nfui_nfobject_get_size(obj, &size_w, &size_h);
	        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	        nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);    
	 		nfui_nfobject_gc_unref(gc);
		}	
	}
	else if (evt->type == GDK_DELETE)
	{
		if (g_loading_timer)
		{
			nfui_ui_unlock();
		
			g_source_remove(g_loading_timer);
			g_loading_timer = 0;
		}

		if (g_setupFlicker_timer)
		{
			g_source_remove(g_setupFlicker_timer);
			g_setupFlicker_timer = 0;
		}

		vsm_live_preview_stop();
	
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	if(!nfui_nfobject_is_disabled(g_close_btn))
		return TRUE;
	else
		return FALSE;
}



////////////////////////////////////////////////////////////////////
//
//
//

gint VW_Open_IPCamInstallSearch_Page(NFOBJECT *parent)
{
	NFOBJECT *win = NULL;
	NFOBJECT *main_fixed = NULL;
	NFOBJECT *sub_fixed = NULL;
	NFOBJECT *ntb1;		
	NFOBJECT *obj = NULL;

	gint pos_x, pos_y;
	gint i, j;

	const gchar *strType[SEARCH_TYPE_MAX] = {"AUTO SCAN (LAN)", "SPECIFIC IP / HOST NAME", "VIRTUAL CAMERA"};

	guint width[10] = {0, };
	guint tot_width = 0;
	gchar strTitle[10];
	gchar strBuf[64];
	gint table_col_cnt = 0;
	gint table_col_idx = 0;

	gint assigned_fgcolor[NFOBJECT_STATE_COUNT];
	gint notassigned_fgcolor[NFOBJECT_STATE_COUNT];
	gint bgcolor[NFOBJECT_STATE_COUNT];
	
	gint img_w, img_h;

	gboolean install_mode = DAL_get_cam_install_mode();
	gboolean use_dual_lan = DAL_get_cam_install_use_dual_lan();
	

	g_setup_img[0][0] = nfui_get_image_from_file(IMG_BT_POP_SETTING_N, NULL);
	g_setup_img[0][1] = nfui_get_image_from_file(IMG_BT_POP_SETTING_O, NULL);
	g_setup_img[0][2] = nfui_get_image_from_file(IMG_BT_POP_SETTING_P, NULL);
	g_setup_img[0][3] = nfui_get_image_from_file(IMG_BT_POP_SETTING_D, NULL);	

	g_setup_img[1][0] = nfui_get_image_from_file(IMG_BT_POP_SETTING_N2, NULL);
	g_setup_img[1][1] = nfui_get_image_from_file(IMG_BT_POP_SETTING_O, NULL);
	g_setup_img[1][2] = nfui_get_image_from_file(IMG_BT_POP_SETTING_P, NULL);
	g_setup_img[1][3] = nfui_get_image_from_file(IMG_BT_POP_SETTING_D, NULL);	

	g_preview_img[0][0] = nfui_get_image_from_file(IMG_BT_POP_PLAY_N, NULL);
	g_preview_img[0][1] = nfui_get_image_from_file(IMG_BT_POP_PLAY_O, NULL);
	g_preview_img[0][2] = nfui_get_image_from_file(IMG_BT_POP_PLAY_P, NULL);
	g_preview_img[0][3] = nfui_get_image_from_file(IMG_BT_POP_PLAY_D, NULL);		

	g_preview_img[1][0] = nfui_get_image_from_file(IMG_BT_POP_PAUSE_N, NULL);
	g_preview_img[1][1] = nfui_get_image_from_file(IMG_BT_POP_PAUSE_O, NULL);
	g_preview_img[1][2] = nfui_get_image_from_file(IMG_BT_POP_PAUSE_P, NULL);
	g_preview_img[1][3] = nfui_get_image_from_file(IMG_BT_POP_PAUSE_D, NULL);		

	assigned_fgcolor[NFOBJECT_STATE_NORMAL] = COLOR_IDX(198);
	assigned_fgcolor[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(199);
	assigned_fgcolor[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(199);
	assigned_fgcolor[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);

	notassigned_fgcolor[NFOBJECT_STATE_NORMAL] = COLOR_IDX(247);
	notassigned_fgcolor[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
	notassigned_fgcolor[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
	notassigned_fgcolor[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);

	bgcolor[NFOBJECT_STATE_NORMAL] = COLOR_IDX(246);
	bgcolor[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
	bgcolor[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
	bgcolor[NFOBJECT_STATE_DISABLE] = COLOR_IDX(248);

	g_curr_page = 0;
	g_total_page = 0;
//	g_select_row = -1;
	g_retVal = 0;
	g_page_auto_update = 0;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_regi_post_event_callback(win, post_install_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA ADD/DELETE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 19);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);

	pos_x = 20;
	pos_y = 80;
	
	obj = nftool_normal_button_create_popup_type1("SEARCH", 300);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_search_button_event_handler);
	g_search_btn = obj;

	pos_x += 320;

	obj = nfui_combobox_new(strType, SEARCH_TYPE_MAX, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 400, 40);
	nfui_nfobject_show(obj);	
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_searchType_combo_event_handler);	
	g_searchType_obj = obj;

	pos_x += 420;

	obj = nftool_normal_button_create_popup_type1("IPCAM AUTO ASSIGN", 300);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_auto_asign_button_event_handler);
	g_assign_btn = obj;

	pos_x += 340;

// AUTO SCAN (LAN) - SUB FIXED
	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sub_fixed, 800, 40);
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(sub_fixed);	
	nfui_nffixed_put((NFFIXED*)main_fixed, sub_fixed, pos_x, pos_y);
	g_type_fixed[SEARCH_TYPE_AUTO] = sub_fixed;

	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sub_fixed, 800, 40);
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(sub_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, sub_fixed, pos_x, pos_y);
	g_type_fixed[SEARCH_TYPE_VIRTUAL] = sub_fixed;

// IP RANGE (LAN) - SUB FIXED
/*
	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sub_fixed, 800, 40);
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_hide(sub_fixed);	
	nfui_nffixed_put((NFFIXED*)main_fixed, sub_fixed, pos_x, pos_y);
	g_type_fixed[SEARCH_TYPE_RANGE] = sub_fixed;

	width[0] = 50;
	width[1] = 300;
	width[2] = 30;
	width[3] = 300;

	ntb1 = (NFOBJECT*)nfui_nftable_new(4, 1, 2, 0, width, 40);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(ntb1);
	nfui_nffixed_put((NFFIXED*)sub_fixed, ntb1, 0, 0);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 0, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 1, 0);
	g_ipRange_obj[0] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 2, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 3, 0);	
	g_ipRange_obj[1] = obj;
*/

// SPECIFIC IP / HOST NAME - SUB FIXED
	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sub_fixed, 800, 40);
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_hide(sub_fixed);	
	nfui_nffixed_put((NFFIXED*)main_fixed, sub_fixed, pos_x, pos_y);
	g_type_fixed[SEARCH_TYPE_SPECIFIC] = sub_fixed;

	width[0] = 230;
	width[1] = 290;
	width[2] = 150;
	width[3] = 120;

	ntb1 = (NFOBJECT*)nfui_nftable_new(4, 1, 2, 0, width, 40);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(ntb1);
	nfui_nffixed_put((NFFIXED*)sub_fixed, ntb1, 0, 0);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP / HOST NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 0, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 1, 0);
	nfui_regi_post_event_callback(obj, post_ipHost_event_handler);	
	g_ipHost_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HTTP PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 2, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, 3, 0);	
	nfui_regi_post_event_callback(obj, post_port_event_handler);	
	g_port_obj = obj;

	pos_x = 20;
	pos_y += 60;

	table_col_idx = 0;
	if (install_mode && use_dual_lan) {
		table_col_cnt = LIST_COL_MAX+1;
		width[table_col_idx++] = 250;		// LIST_MODEL
		width[table_col_idx++] = 250;		// LIST_ADDR
		width[table_col_idx++] = 180;		// LIST_ETHERNET
		width[table_col_idx++] = 320;		// LIST_STATUS
		width[table_col_idx++] = 250;		// LIST_ASSIGN
		width[table_col_idx++] = 40;		// LIST_SETUP
		width[table_col_idx++] = 40;		// LIST_PREVIEW
	}
	else {
		table_col_cnt = LIST_COL_MAX;
		width[table_col_idx++] = 250;		// LIST_MODEL
		width[table_col_idx++] = 250;		// LIST_ADDR
		width[table_col_idx++] = 400;		// LIST_STATUS
		width[table_col_idx++] = 250;		// LIST_ASSIGN
		width[table_col_idx++] = 40;		// LIST_SETUP
		width[table_col_idx++] = 40;		// LIST_PREVIEW
	}

	ntb1 = (NFOBJECT*)nfui_nftable_new(table_col_cnt-2, 1, 1, 1, width, 40);
	nfui_nftable_set_draw_outline((NFTABLE*)ntb1, TRUE);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb1);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb1, pos_x, pos_y);	

	table_col_idx = 0;

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_display_string(NF_COMBOBOX(obj), "MODEL");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "ASCENDING");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "DESCENDING");		
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, 0);
	nfui_regi_post_event_callback(obj, post_sort_combo_event_handler);	
	g_model_sort_obj = obj;

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_display_string(NF_COMBOBOX(obj), "ADDRESS");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "ASCENDING");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "DESCENDING");		
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, 0);
	nfui_regi_post_event_callback(obj, post_sort_combo_event_handler);	
	g_address_sort_obj = obj;

	if (install_mode && use_dual_lan) 
	{
		obj = nfui_combobox_new(NULL, 0, 0);
		nfui_combobox_set_display_string(NF_COMBOBOX(obj), "ETHERNET");
		nfui_combobox_append_data(NF_COMBOBOX(obj), "ASCENDING");
		nfui_combobox_append_data(NF_COMBOBOX(obj), "DESCENDING");		
		nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, 0);
		nfui_regi_post_event_callback(obj, post_ethernet_sort_combo_event_handler);	
	}

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_display_string(NF_COMBOBOX(obj), "STATUS");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "ASCENDING");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "DESCENDING");		
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, 0);
	nfui_regi_post_event_callback(obj, post_sort_combo_event_handler);	
	g_status_sort_obj = obj;

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_display_string(NF_COMBOBOX(obj), "ASSIGNED CHANNEL");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "ASCENDING");
	nfui_combobox_append_data(NF_COMBOBOX(obj), "DESCENDING");		
	nfui_combobox_append_data(NF_COMBOBOX(obj), "NOT ASSIGNED");
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, 0);
	nfui_regi_post_event_callback(obj, post_sort_channel_combo_event_handler);	

	pos_y += 41;

	ntb1 = (NFOBJECT*)nfui_nftable_new(table_col_cnt, LIST_ROW_MAX, 1, 1, width, 40);
	nfui_nftable_set_draw_outline((NFTABLE*)ntb1, TRUE);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb1);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb1, pos_x, pos_y);	

	for (i = 0; i < LIST_ROW_MAX; i++)
	{
		table_col_idx = 0;

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, FALSE);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);
//		nfui_regi_post_event_callback(obj, post_search_list_event_handler);
		g_list_obj[i][LIST_MODEL] = obj;

		nfui_nfobject_set_data(obj, "info_index", GINT_TO_POINTER(-1));

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, FALSE);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);
//		nfui_regi_post_event_callback(obj, post_search_list_event_handler);			
		g_list_obj[i][LIST_ADDR] = obj;

		if (install_mode && use_dual_lan) 
		{
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_use_focus(obj, FALSE);		
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);
	//		nfui_regi_post_event_callback(obj, post_search_list_event_handler);			
			g_ethernet_obj[i] = obj;
		}

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, FALSE);				
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);
//		nfui_regi_post_event_callback(obj, post_search_list_event_handler);	
		g_list_obj[i][LIST_STATUS] = obj;

		obj = nfui_combobox_new(NULL, 0, 0);
		nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
		nfui_combobox_set_multi_color((NFCOMBOBOX*)obj, 0, notassigned_fgcolor, bgcolor);
		nfui_combobox_set_multi_color((NFCOMBOBOX*)obj, 1, assigned_fgcolor, bgcolor);		
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
		nfui_nfobject_disable(obj);		
		nfui_nfobject_show(obj);	
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);
		nfui_regi_post_event_callback(obj, post_assign_combo_event_handler);	
		g_list_obj[i][LIST_ASSIGN] = obj;

		nfui_combobox_append_data((NFCOMBOBOX*)obj, "NOT ASSIGNED");

		for (j = 0; j < GUI_CHANNEL_CNT; j++) 
		{
			memset(strTitle, 0x00, sizeof(strTitle));
			g_sprintf(strTitle, "CAM %d", j+1);
			nfui_combobox_append_data((NFCOMBOBOX*)obj, strTitle);
		}

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), g_setup_img[0]);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
		nfui_nfobject_disable(obj);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);	
		nfui_regi_post_event_callback(obj, post_setup_button_event_handler);
		g_list_obj[i][LIST_SETUP] = obj;

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), g_preview_img[0]);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);	
		nfui_nfobject_disable(obj);		
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj, table_col_idx++, i);	
		nfui_regi_post_event_callback(obj, post_preview_button_event_handler);
		g_list_obj[i][LIST_PREVIEW] = obj;	

		nfui_nfobject_set_data(obj, "image index", GINT_TO_POINTER(0));
	}

	pos_x = 4 + (nfui_nftable_get_width((NFTABLE*)ntb1)-100)/2;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1 / 1", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 100, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+LIST_ROW_MAX*40+LIST_ROW_MAX*1+20);
    g_page_obj = obj;

	obj = nftool_normal_button_create_popup_type1("PREV.", 84);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x-134, pos_y+LIST_ROW_MAX*40+LIST_ROW_MAX*1+20);
	nfui_regi_post_event_callback(obj, post_prev_button_event_handler);	

	obj = nftool_normal_button_create_popup_type1("NEXT", 84);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+100+50, pos_y+LIST_ROW_MAX*40+LIST_ROW_MAX*1+20);
	nfui_regi_post_event_callback(obj, post_next_button_event_handler);	


// SEARCH_RESULT FIXED
	pos_x = DISPLAY_ACTIVE_WIDTH;
	pos_x -= (16*PREVIEW_RATIO + 20);

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SEARCH RESULT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	nfui_get_image_size(IMG_POPUP_TITLE_BG2, &img_w, &img_h);

	obj = nfui_nfimage_new(IMG_LOADING_0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+img_w, pos_y+8);
	g_loading_obj = obj;

	pos_y += (40+21);

	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sub_fixed, 16*PREVIEW_RATIO, 32*3+34+16+8);
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(sub_fixed);	
	nfui_nffixed_put((NFFIXED*)main_fixed, sub_fixed, pos_x, pos_y);
	nfui_regi_post_event_callback(sub_fixed, post_result_fixed_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	_translate_search_result(0, STR_RESULT_ASSIGNED, strBuf);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, 16*PREVIEW_RATIO-8, 32);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 4, 4+32*0);
	g_assign_obj = obj;

	memset(strBuf, 0x00, sizeof(strBuf));
	_translate_search_result(0, STR_RESULT_FOUND, strBuf);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, 16*PREVIEW_RATIO-8, 32);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 4, 4+32*1);
	g_found_obj = obj;

	memset(strBuf, 0x00, sizeof(strBuf));
	_translate_search_result(0, STR_RESULT_NEEDED, strBuf);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, 16*PREVIEW_RATIO-8, 32);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 4, 4+32*2);
	g_needed_obj = obj;

	obj = nftool_normal_button_create_popup_type2("SETUP ALL CAMERAS", 16*PREVIEW_RATIO-8);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 4, 4+32*3+16);
//	nfui_regi_post_event_callback(obj, post_setup_all_button_event_handler);



// PREVIEW FIXED
	pos_y += 300;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "PREVIEW");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += (40+21);

	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sub_fixed, 16*PREVIEW_RATIO, 9*PREVIEW_RATIO);
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(sub_fixed);	
	nfui_nffixed_put((NFFIXED*)main_fixed, sub_fixed, pos_x, pos_y);
	nfui_regi_post_event_callback(sub_fixed, post_preview_fixed_event_handler);
	g_preview_fixed = sub_fixed;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NEED TO BE SETUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, (16*PREVIEW_RATIO-200)/2, (9*PREVIEW_RATIO-40)/2);
    g_preview_text_obj = obj;

// cancel, apply, close button
	pos_x = DISPLAY_ACTIVE_WIDTH-12-MENU_BTN_WIDTH*3-MENU_BTN_GAP*2;
	pos_y = DISPLAY_ACTIVE_HEIGHT-10-MENU_BTN_HEIGHT;

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	pos_x += (MENU_BTN_GAP + MENU_BTN_WIDTH);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	pos_x += (MENU_BTN_GAP + MENU_BTN_WIDTH);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);
	g_close_btn = obj;

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(g_search_btn, TRUE);

	nfui_page_open(PGID_IPCAMERA_INSTALL, win, ssm_get_cur_id(NULL));
	
	nf_openmode_scan_camera();
	uxm_reg_imsg_event(main_fixed, INFY_IPCAM_INSTALL_NOTIFY);
	
	gtk_main();

	nfui_page_close(PGID_IPCAMERA_INSTALL, win);
	
	return g_retVal;
}

