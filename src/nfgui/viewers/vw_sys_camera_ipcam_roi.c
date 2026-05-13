#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"
#include "objects/nftile.h"

#include "nf_ipcam_defs.h"
#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"

#include "vw.h"
#include "vw_internal.h"
#include "vw_sys_camera_ipcam_setup_main.h"
#include "vw_sys_camera_ipcam_roi_level_popup.h"


#define STR_NOTICE_VBR "Current bit rate is set to VBR+.\nWhen the bit rate is set to VBR+, ROI function does not operate."


#define ROI_LB_START_POS_X              (20)
#define ROI_LB_START_POS_Y              (20)

#define VIDEO_AREA_SIZE_W			    (1920/10*5)
#define VIDEO_AREA_SIZE_H			    (1080/10*5)


static ROIData g_roi_data[GUI_CHANNEL_CNT] = {0,};
static ROIData g_org_roi_data[GUI_CHANNEL_CNT] = {0,};
static NFIPCamROIAreaProfile g_profile[GUI_CHANNEL_CNT] = {0,};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_back_fixed;
static NFOBJECT *g_option_fixed;
static NFOBJECT *g_mode_label_obj;
static NFOBJECT *g_quality_label_obj;
static NFOBJECT *g_mode_obj;
static NFOBJECT *g_quality_obj;
static NFOBJECT *g_area_tile_obj;
static NFOBJECT *g_all_radio_obj;
static NFOBJECT *g_all_label_obj;
static NFOBJECT *g_area_radio_obj[MAX_ROI_AREA_CNT];
static NFOBJECT *g_area_label_obj[MAX_ROI_AREA_CNT];
static NFOBJECT *g_area_del_obj;
static NFOBJECT *g_notice_vbr_label;

static gint g_radio_idx = 0;
static gint g_ch;
static gint g_max_area_cnt;
static gint g_video_size_w = 0;



static void _set_video_width(gint ch)
{
	gint ratio_w, ratio_h;
	gint tmp = 0;

	scm_ipcam_get_main_stream_ratio(ch, &ratio_w, &ratio_h);
	// g_message("[%s, %d], ratio_w : %d, ratio_h : %d", __FUNCTION__, __LINE__, ratio_w, ratio_h);
	
	if (scm_get_ipcam_corridor_mode(ch) != 0)
	{
		tmp = ratio_w;
		ratio_w = ratio_h;
		ratio_h = tmp;
		
		g_video_size_w = VIDEO_AREA_SIZE_H * ratio_w / ratio_h;
	}
	else
	{
		g_video_size_w = VIDEO_AREA_SIZE_W;
	}
	// g_message("[%s, %d], g_video_size_w : %d", __FUNCTION__, __LINE__, g_video_size_w);
}

static void _trans_db_to_val(gint ch)
{
    NFIPCamROIAreaProfile profile;
    gint area;
    gint corr_mode;
    gint db_tx, db_ty, db_bx, db_by;

    memset(&profile, 0x00, sizeof(NFIPCamROIAreaProfile));
    if (nf_ipcam_get_roi_area_profile(ch, &profile) != IPCAM_SETUP_RTN_DONE) {
        return;
    }
    
    if (!profile.isSupport) return;

    corr_mode = scm_get_ipcam_corridor_mode(ch);
    if (!corr_mode) return;
    
    for (area = 0; area < profile.area_num; area++)
    {
        db_tx = g_roi_data[ch].area[area].tx;
        db_ty = g_roi_data[ch].area[area].ty;
        db_bx = g_roi_data[ch].area[area].bx;
        db_by = g_roi_data[ch].area[area].by;
        
        if (db_tx == 0 && db_ty == 0 && db_bx == 0 && db_by == 0) continue;
        
        // g_message("%s, %d, before tx:%d, ty:%d, bx:%d, by:%d", __FUNCTION__, __LINE__, g_roi_data[ch].area[area].tx, g_roi_data[ch].area[area].ty, g_roi_data[ch].area[area].bx, g_roi_data[ch].area[area].by);
        
        if (corr_mode == 1)
        {
            g_roi_data[ch].area[area].tx = profile.block_width - db_by;
            g_roi_data[ch].area[area].ty = db_tx;
            g_roi_data[ch].area[area].bx = profile.block_width - db_ty;
            g_roi_data[ch].area[area].by = db_bx;
        }
        else if (corr_mode == 2)
        {
            g_roi_data[ch].area[area].tx = db_ty;
            g_roi_data[ch].area[area].ty = profile.block_height - db_bx;
            g_roi_data[ch].area[area].bx = db_by;
            g_roi_data[ch].area[area].by = profile.block_height - db_tx;
        }
        
        // g_message("%s, %d, after tx:%d, ty:%d, bx:%d, by:%d", __FUNCTION__, __LINE__, g_roi_data[ch].area[area].tx, g_roi_data[ch].area[area].ty, g_roi_data[ch].area[area].bx, g_roi_data[ch].area[area].by);
    }
}

static void _trans_val_to_db(ROIData *roi_db)
{
    gint ch, area;
    gint corr_mode;
    gint cur_tx, cur_ty, cur_bx, cur_by;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        corr_mode = scm_get_ipcam_corridor_mode(ch);
        if (!corr_mode) continue;
        
        for (area = 0; area < g_profile[ch].area_num; area++)
        {            
            cur_tx = g_roi_data[ch].area[area].tx;
            cur_ty = g_roi_data[ch].area[area].ty;
            cur_bx = g_roi_data[ch].area[area].bx;
            cur_by = g_roi_data[ch].area[area].by;
            
            if (cur_tx == 0 && cur_ty == 0 && cur_bx == 0 && cur_by == 0) continue;

            if (corr_mode == 1)
            {
                roi_db[ch].area[area].tx = cur_ty;
                roi_db[ch].area[area].ty = g_profile[ch].block_width - cur_bx;
                roi_db[ch].area[area].bx = cur_by;
                roi_db[ch].area[area].by = g_profile[ch].block_width  - cur_tx;
            }
            else if (corr_mode == 2)
            {
                roi_db[ch].area[area].tx = g_profile[ch].block_height  - cur_by;
                roi_db[ch].area[area].ty = cur_tx;
                roi_db[ch].area[area].bx = g_profile[ch].block_height  - cur_ty;
                roi_db[ch].area[area].by = cur_bx;
            }
        }
    }
}

static gint _init_roi_data()
{
    gint i;
    
    memset(g_roi_data, 0x00, sizeof(g_roi_data));
    memset(g_org_roi_data, 0x00, sizeof(g_org_roi_data));

    for (i = 0; i < GUI_CHANNEL_CNT; i++)   {
        DAL_get_roi_data(&g_roi_data[i], i);
        _trans_db_to_val(i);
    }

    g_memmove(g_org_roi_data, g_roi_data, sizeof(ROIData) * GUI_CHANNEL_CNT);
    return 0;
}

static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch,
        MENU_V_SUBTAB_FIXED_X+MENU_V_IPCAMSET_SUBTAB_PAGE_X+MENU_V_IPCAMSET_SUBTAB_INNER_X+ROI_LB_START_POS_X,
        MENU_V_SUBTAB_FIXED_Y+MENU_V_IPCAMSET_SUBTAB_PAGE_Y+ROI_LB_START_POS_Y + 180,
        VIDEO_AREA_SIZE_W,
        VIDEO_AREA_SIZE_H);

    return 0;
}

static void _stop_preview()
{
    if (nfui_nfobject_is_shown(g_area_tile_obj))
    {
    	GdkDrawable *drawable = nfui_nfobject_get_window((NFOBJECT*)g_area_tile_obj);
    	GdkGC *gc = nfui_nfobject_get_gc((NFOBJECT*)g_area_tile_obj);
    	GdkColor color = UX_COLOR(COLOR_PRG_IDX(UX_COLOR_808080));
    	gint off_x, off_y;

    	gdk_gc_set_rgb_fg_color(gc, &color);
    	nfui_nfobject_get_offset(g_area_tile_obj, &off_x, &off_y);
    	gdk_draw_rectangle(drawable, gc, TRUE, off_x, off_y, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
    	nfui_nfobject_gc_unref(gc);
    }

	vsm_live_preview_stop();
}

static gint _clear_tile_roi_mask(guint idx_mask, gint draw)
{
    gint ch = g_ch;
    gint idx, select;
    gint i, j;
    guint color_n;

    for (i = 0; i < g_profile[ch].block_height; i++)
    {
    	for (j = 0; j < g_profile[ch].block_width; j++)
    	{
            color_n = SELECT_STATE_COLOR_1;
            if (draw) nfui_tile_draw_color(NF_TILE(g_area_tile_obj), (guint)color_n, i, j, i, j);
			else nfui_tile_no_draw_color(NF_TILE(g_area_tile_obj), (guint)color_n, i, j, i, j);
    	}
    }

    return 0;
}

static gint _redraw_tile_roi_mask(guint idx_mask, gint draw)
{
    gint ch = g_ch;
    gint idx, select;
    gint i, j;
    guint color_n;

    if (!nfui_nfobject_is_shown(g_area_tile_obj)) {
        draw = 0;
    }

    for (i = 0; i < g_profile[ch].block_height; i++)
    {
    	for (j = 0; j < g_profile[ch].block_width; j++)
    	{
            color_n = SELECT_STATE_COLOR_1;

            for (idx = g_profile[ch].area_num-1; idx >= 0; idx--)
            {
                if (idx_mask & (1 << idx))
                {
                    if ((i >= g_roi_data[ch].area[idx].ty) && (i < g_roi_data[ch].area[idx].by)
                        && (j >= g_roi_data[ch].area[idx].tx) && (j < g_roi_data[ch].area[idx].bx))
                    {
                        if (g_roi_data[ch].area[idx].level == 1) {
                            color_n = SELECT_STATE_COLOR_4;
                        }
                        else {
                            color_n = SELECT_STATE_COLOR_3;
                        }
                        break;
                    }
                }
            }

            if (draw) nfui_tile_draw_color(NF_TILE(g_area_tile_obj), (guint)color_n, i, j, i, j);
			else nfui_tile_no_draw_color(NF_TILE(g_area_tile_obj), (guint)color_n, i, j, i, j);
    	}
    }

//    if (draw)
//        nfui_tile_draw_area(NF_TILE(g_area_tile_obj), 0, 0, g_profile[ch].block_height-1, g_profile[ch].block_width-1);

    return 0;
}

static gint _get_roi_profile()
{
    gint i;
    
    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(&g_profile[i], 0x00, sizeof(NFIPCamROIAreaProfile));
        if (nf_ipcam_get_roi_area_profile(i, &g_profile[i]) != IPCAM_SETUP_RTN_DONE) {
            g_profile[i].isSupport = 0;
        }

        if ((g_profile[i].block_width == 0) || (g_profile[i].block_height == 0)) {
            g_profile[i].block_width = 1;
            g_profile[i].block_height = 1;
        }
    
        // g_message("%s, %d, ch : %d, is_support:%d, isOption:%d", __FUNCTION__, __LINE__, i, g_profile[i].isSupport, g_profile[i].isOption);
        // g_message("%s, %d, block_width:%d, block_height:%d", __FUNCTION__, __LINE__, g_profile[i].block_width, g_profile[i].block_height);
    }

    return 0;
}

static gint _delete_focus_roi_tile()
{
    gint i, j;

    for (i = 0; i < g_profile[g_ch].block_width; i++)
    {
        for (j = 0; j < g_profile[g_ch].block_height; j++)
        {
            nfui_tile_set_select_state(NF_TILE(g_area_tile_obj), j, i);
            nfui_tile_set_conv_select_state(NF_TILE(g_area_tile_obj), j, i);
        }
    }

    return 0;
}

static guint _trans_mode_text_to_val()
{
    guint val = 0;
    gchar *text;

    text = nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj);

    if (strcmp(text, "OFF") == 0) {
        val = (1 << 0);
    }
    else if (strcmp(text, "AUTO") == 0) {
        val = (1 << 1);
    }
    else {
        val = (1 << 2);
    }

    return val;
}

static void _trans_mode_val_to_text(gint ch, gchar *text)
{
    if (g_roi_data[ch].mode & (1 << 0)) {
        strcpy(text, "OFF");
    }
    else if (g_roi_data[ch].mode & (1 << 1)) {
        strcpy(text, "AUTO");
    }
    else {
        strcpy(text, "MANUAL");
    }
}

static gint _init_mode_objs()
{    
    nfui_nfobject_hide(g_quality_label_obj);
    nfui_nfobject_hide(g_quality_obj);
    
    if (g_profile[g_ch].isOption)
    {
        nfui_combobox_remove_all((NFCOMBOBOX*)g_mode_obj);
        
        if (g_profile[g_ch].isOption & (1 << 0)) {
            nfui_combobox_append_data((NFCOMBOBOX*)g_mode_obj, "OFF");
        }
        if (g_profile[g_ch].isOption & (1 << 1)) {
            nfui_combobox_append_data((NFCOMBOBOX*)g_mode_obj, "AUTO");
            nfui_nfobject_show(g_quality_label_obj);
            nfui_nfobject_show(g_quality_obj);
        }
        if (g_profile[g_ch].isOption & (1 << 2)) {
            nfui_combobox_append_data((NFCOMBOBOX*)g_mode_obj, "MANUAL");
        }
    }
    
    return 0;
}

static gint _change_channel_roi_objs(gint ch)
{
    gchar text[16];

    memset(text, 0x00, sizeof(text));
    _trans_mode_val_to_text(ch, text);
    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_mode_obj, text);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_quality_obj, g_roi_data[ch].quality);
    return 0;
}

static gint _change_disable_roi_objs()
{
    gint i;

    nfui_nfobject_disable(g_mode_obj);
    nfui_nfobject_disable(g_quality_obj);
    nfui_nfobject_disable(g_all_radio_obj);
    nfui_nfobject_disable(g_all_label_obj);
    nfui_nfobject_disable(g_area_del_obj);

    for (i = 0; i < g_max_area_cnt; i++)
    {
        nfui_nfobject_disable(g_area_radio_obj[i]);
        nfui_nfobject_disable(g_area_label_obj[i]);
    }

    nfui_nfobject_hide(g_notice_vbr_label);

    g_radio_idx = 0;
    return 0;
}

static gint _change_mode_roi_objs(gint ch)
{
    gint i;
    StreamData streamdata;
    
    if (g_profile[ch].isSupport == 0) 
    {
        nfui_nfobject_set_size(g_area_tile_obj, g_video_size_w, VIDEO_AREA_SIZE_H);
        nfui_nffixed_put((NFFIXED*)g_video_back_fixed, g_area_tile_obj, 0, 0);

        nfui_tile_reset_area_size((NFTILE*)g_area_tile_obj, 1, 1);
        nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);
        
        return -1;
    }

    memset(&streamdata, 0x00, sizeof(StreamData));
    DAL_get_stream_data(&streamdata, ch);

    if ((strcmp(streamdata.control[0], "IDNR") == 0) || (strcmp(streamdata.control[1], "IDNR") == 0)) {
        nfui_nfobject_show(g_notice_vbr_label);
    }
    else {
        nfui_nfobject_hide(g_notice_vbr_label);
    }

    nfui_nfobject_set_size(g_area_tile_obj, g_video_size_w, VIDEO_AREA_SIZE_H);
    nfui_nffixed_put((NFFIXED*)g_video_back_fixed, g_area_tile_obj, (VIDEO_AREA_SIZE_W-g_video_size_w)/2, 0);
    nfui_tile_reset_area_size((NFTILE*)g_area_tile_obj, g_profile[ch].block_height, g_profile[ch].block_width);
    nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);

    if (g_profile[ch].isOption)
    {
        nfui_nfobject_enable(g_mode_obj);
        nfui_nfobject_show(g_mode_label_obj);
        nfui_nfobject_show(g_mode_obj);

        if (strcmp(nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj), "OFF") == 0) {
            return 0;
        }

        if (strcmp(nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj), "AUTO") == 0) {
            nfui_nfobject_enable(g_quality_obj);
            return 0;
        }
    }
    else
    {
        nfui_nfobject_hide(g_mode_label_obj);
        nfui_nfobject_hide(g_quality_label_obj);
        nfui_nfobject_hide(g_mode_obj);
        nfui_nfobject_hide(g_quality_obj);
    }

    nfui_nfobject_enable(g_all_radio_obj);
    nfui_nfobject_enable(g_all_label_obj);
    nfui_nfobject_enable(g_area_del_obj);    

    for (i = 0; i < g_max_area_cnt; i++)
    {
        if (i < g_profile[ch].area_num)
        {
            nfui_nfobject_enable(g_area_radio_obj[i]);
            nfui_nfobject_enable(g_area_label_obj[i]);
        }
        else
        {
            nfui_nfobject_disable(g_area_radio_obj[i]);
            nfui_nfobject_disable(g_area_label_obj[i]);
        }
    }

    _redraw_tile_roi_mask(0xffff, 0);
    nfui_radio_button_set_toggled(NF_BUTTON(g_all_radio_obj), TRUE);
    return 0;
}

static gint _expose_roi_objs()
{
    gint i;

    if (g_radio_idx == 0) {
        _redraw_tile_roi_mask(0xffff, 1);
    }
    else {
        _redraw_tile_roi_mask(1<<(g_radio_idx-1), 1);
    }

    nfui_signal_emit(g_option_fixed, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_all_radio_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_all_label_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_video_back_fixed, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_area_del_obj, GDK_EXPOSE, TRUE);

    for (i = 0; i < g_max_area_cnt; i++)
    {
        nfui_signal_emit(g_area_radio_obj[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_area_label_obj[i], GDK_EXPOSE, TRUE);
    }

    nfui_signal_emit(g_notice_vbr_label->parent, GDK_EXPOSE, TRUE);

    nfui_make_key_hierarchy(nfui_nfobject_get_top(g_option_fixed));
    return 0;
}

static gboolean post_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint idx;

    switch(evt->type)
    {
        case NFEVENT_COMBOBOX_CHANGED:
        {
//            g_roi_data[g_ch].mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_mode_obj);
            g_roi_data[g_ch].mode = _trans_mode_text_to_val();
            _delete_focus_roi_tile();
            _change_disable_roi_objs();
            _change_mode_roi_objs(g_ch);
            _expose_roi_objs();
        }
        break;

        default:
        break;
    }

    return FALSE;
}

static gboolean post_backqual_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case NFEVENT_COMBOBOX_CHANGED:
        {
            g_roi_data[g_ch].quality = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_quality_obj);
        }
        break;

        default:
        break;
    }

    return FALSE;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_TILE_INIT)
	{
        gint i;
        guint mask = 0;

        for (i = 0; i < g_profile[g_ch].area_num; i++)
        {
            mask |= (1 << i);
        }

        _redraw_tile_roi_mask(mask, 0);
        //nfui_tile_draw_area(NF_TILE(obj), 0, 0, g_profile[g_ch].block_height-1, g_profile[g_ch].block_width-1);
	}
	else if (evt->type == NFEVENT_TILE_START_SELECT)
	{
        gint ch = g_ch;

        //if (g_radio_idx == 0) return FALSE;

        NF_TILE(obj)->select_n = SELECT_STATE_COLOR_1;
        _clear_tile_roi_mask(1<<(g_radio_idx-1), 1);
        nfui_tile_draw_area(NF_TILE(obj), 0, 0, g_profile[g_ch].block_height-1, g_profile[g_ch].block_width-1);
	}
	else if (evt->type == NFEVENT_TILE_MOVE_SELECT)
	{
	}
	else if (evt->type == NFEVENT_TILE_END_SELECT)
	{
        gint ch = g_ch;
        gint tx, ty, bx, by;
		gint x, y;
		gint mx, my;
		int off_x, off_y;
	    guint mode;
    	guint s_row = 0, s_col = 0;
    	guint e_row = 0, e_col = 0;
    	guint i, j;
    	guint color_n = 0;

        if (g_radio_idx == 0) return FALSE;

		nfui_tile_get_selectArea(NF_TILE(obj), &s_row, &s_col, &e_row, &e_col);
//        g_message("%s, %d, tx:%d, ty:%d, bx:%d, by:%d", __FUNCTION__, __LINE__, tx, ty, bx, by);

		x = (50 * (e_col + 1));
		y = (41 * (e_row + 1));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
		x += off_x;
		y += off_y;

		gdk_display_get_pointer(gdk_display_get_default(), NULL, &mx, &my, NULL);

		mode = VW_ROI_Level_Select_Popup(g_curwnd, mx, my);

		if (mode == ROI_CANCEL) return FALSE;

        if (mode == ROI_NO_INTEREST)
        {
            color_n = SELECT_STATE_COLOR_3;
            g_roi_data[ch].area[g_radio_idx-1].level = 0;
            g_roi_data[ch].area[g_radio_idx-1].tx = s_col;
            g_roi_data[ch].area[g_radio_idx-1].ty = s_row;
            g_roi_data[ch].area[g_radio_idx-1].bx = e_col+1;
            g_roi_data[ch].area[g_radio_idx-1].by = e_row+1;
        }
	    else if (mode == ROI_INTEREST)
	    {
            color_n = SELECT_STATE_COLOR_4;
            g_roi_data[ch].area[g_radio_idx-1].level = 1;
            g_roi_data[ch].area[g_radio_idx-1].tx = s_col;
            g_roi_data[ch].area[g_radio_idx-1].ty = s_row;
            g_roi_data[ch].area[g_radio_idx-1].bx = e_col+1;
            g_roi_data[ch].area[g_radio_idx-1].by = e_row+1;
	    }

        _redraw_tile_roi_mask(1<<(g_radio_idx-1), 1);
	}

	return FALSE;
}

static gboolean post_delete_area_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        gint i;

        if (g_radio_idx == 0)
        {
            for (i = 0; i < g_profile[g_ch].area_num; i++)
            {
                g_roi_data[g_ch].area[i].tx = 0;
                g_roi_data[g_ch].area[i].ty = 0;
                g_roi_data[g_ch].area[i].bx = 0;
                g_roi_data[g_ch].area[i].by = 0;
            }
            _redraw_tile_roi_mask(0xffff, 1);
        }
        else
        {
            g_roi_data[g_ch].area[g_radio_idx-1].tx = 0;
            g_roi_data[g_ch].area[g_radio_idx-1].ty = 0;
            g_roi_data[g_ch].area[g_radio_idx-1].bx = 0;
            g_roi_data[g_ch].area[g_radio_idx-1].by = 0;

            _redraw_tile_roi_mask(1<<(g_radio_idx-1), 1);
        }

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_group_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
        gint idx = nfui_radio_button_get_index((NFBUTTON*)obj);
        gint i, j;
        guint mask = 0;

        // focus del....
        for (i = 0; i < g_profile[g_ch].block_width; i++)
        {
        	for (j = 0; j < g_profile[g_ch].block_height; j++)
        	{
    			nfui_tile_set_select_state(NF_TILE(g_area_tile_obj), j, i);
    			nfui_tile_set_conv_select_state(NF_TILE(g_area_tile_obj), j, i);
        	}
        }

        if (idx == 0)
        {
            for (i = 0; i < g_profile[g_ch].area_num; i++)
            {
                mask |= (1 << i);
            }

            _redraw_tile_roi_mask(mask, 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);
        }
        else
        {
            _redraw_tile_roi_mask(1<<(idx-1), 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), TRUE);
        }

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
        g_radio_idx = idx;
	}

	return FALSE;
}

static gboolean pre_group_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
			gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

static gboolean post_legend_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(803));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+2, gap_y+2,
                    22, 22);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(801));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+2, gap_y+26,
                    22, 22);

            g_object_unref(gc);
        }
        break;


        case GDK_DELETE:
        break;

        default :
        break;
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    ROIData roi_db[GUI_CHANNEL_CNT];
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if(memcmp(g_org_roi_data, g_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT))
        {
            g_memmove(g_org_roi_data, g_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT);
            
            memset(roi_db, 0x00, sizeof(roi_db));
            g_memmove(roi_db, g_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT);
            
            _trans_val_to_db(roi_db);
            DAL_set_roi_data_all(roi_db, GUI_CHANNEL_CNT);

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            syscam_set_changeflag(1);
        }
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
	    gint i;
        guint mask = 0;

		if (evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (memcmp(g_roi_data, g_org_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT))
		{
            g_memmove(g_roi_data, g_org_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT);
        }

        _delete_focus_roi_tile();
        _change_disable_roi_objs();
        _change_mode_roi_objs(g_ch);
        _expose_roi_objs();

        if (g_radio_idx == 0)
        {
            for (i = 0; i < g_profile[g_ch].area_num; i++)
            {
                mask |= (1 << i);
            }

            _redraw_tile_roi_mask(mask, 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);
        }
        else
        {
            _redraw_tile_roi_mask(1<<(g_radio_idx-1), 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), TRUE);
        }

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        IPCamROI_tab_out_handler();
        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}

void VW_Init_IPCamROI_Page(NFOBJECT *parent, gint ch)
{
    NFOBJECT *content_fixed;
	NFOBJECT *video_back_fixed;
    NFOBJECT *group_fixed;
    NFOBJECT *option_fixed;
    NFOBJECT *legend_fixed;
    NFOBJECT *tmp_fixed;
    NFOBJECT *obj;
    GSList *slist;
    guint pos_y, pos_x;
    gint size_w, size_h;
	gint label_color[NFOBJECT_STATE_COUNT];
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    gchar *strMode[] = {"OFF", "AUTO", "MANUAL"};
    gchar *strQuality[] = {"LOW", "MID", "HIGH"};
	GdkColor tile_scolor[4] = {UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)), UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)), UX_COLOR(801), UX_COLOR(803)};
    gchar strBuf[16];
	gint i;


	radio_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_RADIO_OFF), NULL);

	label_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(968);
	label_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(968);
	label_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(968);
	label_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(972);


    g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);

    g_max_area_cnt = DAL_get_roi_area_max_cnt();
    _get_roi_profile();
    _init_roi_data();

    g_radio_idx = 0;
    g_ch = ch;

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_W, MENU_V_IPCAMSET_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_X, MENU_V_IPCAMSET_SUBTAB_INNER_Y);
    nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);

    pos_x = ROI_LB_START_POS_X;
    pos_y = ROI_LB_START_POS_Y;

    option_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(option_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(option_fixed, 620, 82);
    nfui_nfobject_show(option_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, option_fixed, pos_x, pos_y);
    g_option_fixed = option_fixed;

//===> ROI mode

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ROI MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)option_fixed, obj, 0, 0);
    nfui_nfobject_show(obj);
    g_mode_label_obj = obj;

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)option_fixed, obj, 400, 0);
	nfui_regi_post_event_callback(obj, post_mode_event_handler);
	g_mode_obj = obj;

//===> background quality

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BACKGROUND IMAGE QUALITY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)option_fixed, obj, 0, 42);
    nfui_nfobject_show(obj);
    g_quality_label_obj = obj;

	obj = nfui_combobox_new(strQuality, 3, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)option_fixed, obj, 400, 42);
	nfui_regi_post_event_callback(obj, post_backqual_event_handler);
	g_quality_obj = obj;

//===> legend
	pos_y = 120;

	legend_fixed = nfui_nffixed_new();
    nfui_nfobject_modify_bg(legend_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(legend_fixed, 250, 50);
    nfui_nfobject_show(legend_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, legend_fixed, (pos_x + VIDEO_AREA_SIZE_W - 250), pos_y);
    nfui_regi_post_event_callback(legend_fixed, post_legend_fixed_event_handler);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INTEREST REGION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 250, 22);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)legend_fixed, obj, 26, 2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NO-INTEREST REGION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 250, 22);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)legend_fixed, obj, 26, 26);

//===> preview
	pos_y = 140;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 160, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

//=== video area
	pos_y += 40;

	video_back_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(video_back_fixed, NFOBJECT_STATE_NORMAL, VIDEO_BG_COLOR);
	nfui_nfobject_set_size(video_back_fixed, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
	nfui_nfobject_show(video_back_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, video_back_fixed, pos_x, pos_y);
	g_video_back_fixed = video_back_fixed;

	obj = nfui_tile_new(1, 1);
	nfui_tile_set_color(NF_TILE(obj), NFTILE_STATE_FOCUS, &UX_COLOR(802));
	nfui_tile_set_color(NF_TILE(obj), NFTILE_STATE_SELECT, tile_scolor);
    nfui_tile_sensitive(NF_TILE(obj), FALSE);
	nfui_tile_set_drawable_outline(NF_TILE(obj), FALSE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
	nfui_regi_post_event_callback(obj, post_tile_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)video_back_fixed, obj, 0, 0);
	g_area_tile_obj = obj;

//===> area button

    pos_x += VIDEO_AREA_SIZE_W+40;

	group_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(group_fixed, MENU_V_SUBTAB_INNER_W-VIDEO_AREA_SIZE_W-80, VIDEO_AREA_SIZE_H);
	nfui_nfobject_show(group_fixed);
	nfui_nfobject_modify_bg(group_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
	nfui_nffixed_put((NFFIXED*)content_fixed, group_fixed, pos_x, pos_y);
	nfui_regi_pre_event_callback(group_fixed, pre_group_fixed_event_handler);

    pos_x = 10;
    pos_y = 10;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y+(40-size_h)/2);
	nfui_regi_post_event_callback(obj, post_group_radio_event_handler);
	g_all_radio_obj = obj;

	slist = nfui_radio_button_get_group(NF_BUTTON(g_all_radio_obj));

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL VIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_fg_color((NFLABEL*)obj, label_color);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 200, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+30, pos_y);
	g_all_label_obj = obj;

    pos_y += 40;

    for (i = 0; i < g_max_area_cnt; i++)
    {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y+(40-size_h)/2);
    	nfui_regi_post_event_callback(obj, post_group_radio_event_handler);
		g_area_radio_obj[i] = obj;

        g_sprintf(strBuf, g_area_str[0], i+1);

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    	nfui_nflabel_set_fg_color((NFLABEL*)obj, label_color);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(186));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_set_size(obj, 200, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+30, pos_y);
		g_area_label_obj[i] = obj;

        pos_y += 40;
    }


	obj = nftool_normal_button_create_type3("DELETE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, post_delete_area_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ROI_LB_START_POS_X, 180 + VIDEO_AREA_SIZE_H + 20);
    g_area_del_obj = obj;

    tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(tmp_fixed, 1200, 80);
	nfui_nfobject_show(tmp_fixed);
	nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nffixed_put((NFFIXED*)content_fixed, tmp_fixed, ROI_LB_START_POS_X+220, 180 + VIDEO_AREA_SIZE_H + 20);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_NOTICE_VBR, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 1200, 80);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 0, 0);
    g_notice_vbr_label = obj;

    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R3_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R2_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R1_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    _init_mode_objs();
    _set_video_width(ch);
    _change_channel_roi_objs(ch);
    _change_disable_roi_objs();
    _change_mode_roi_objs(ch);
}

gint IPCamROI_update_channel(gint ch)
{
    _delete_focus_roi_tile();
    
	g_ch = ch;
    _init_mode_objs();
    _set_video_width(ch);
    _change_channel_roi_objs(ch);
    _change_disable_roi_objs();
    _change_mode_roi_objs(ch);
    _expose_roi_objs();
    return 0;
}

gint IPCamROI_start_preview(gint ch)
{
    _start_preview(ch);
}

gint IPCamROI_stop_preview()
{
    _stop_preview();
}

gboolean IPCamROI_tab_in_handler()
{
    _start_preview(g_ch);
    return FALSE;
}

gboolean IPCamROI_tab_out_handler()
{
    ROIData roi_db[GUI_CHANNEL_CNT];
    mb_type ret;

    _stop_preview();

    if (!memcmp(g_org_roi_data, g_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT))
        return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if(ret == NFTOOL_MB_OK)
    {
        g_memmove(g_org_roi_data, g_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT);
        
        memset(roi_db, 0x00, sizeof(roi_db));
        g_memmove(roi_db, g_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT);
        
        _trans_val_to_db(roi_db);
        DAL_set_roi_data_all(roi_db, GUI_CHANNEL_CNT);

        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        syscam_set_changeflag(1);
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(g_roi_data, g_org_roi_data, sizeof(ROIData)*GUI_CHANNEL_CNT);
        
        _delete_focus_roi_tile();
        _change_disable_roi_objs();
        _change_mode_roi_objs(g_ch);
    }

    return FALSE;
}
