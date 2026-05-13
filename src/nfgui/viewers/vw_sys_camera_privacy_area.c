
#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_motion.h"
#include "vw_motion_sensor_conf.h"
#include "vw_motion_sensor_area.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_image.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nftile.h"
#include "objects/cw_slider.h"
#include "objects/nftab.h"
#include "scm.h"
#include "vsm.h"

#include "nf_ipcam_defs.h"
#include "vw_sys_camera_privacy.h"
#include "vw_sys_camera_privacy_area.h"
#include "vw_internal.h"


#define VIDEO_AREA_SIZE_W			(1920/10*6)
#define VIDEO_AREA_SIZE_H			(1080/10*6)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ch_obj;
static NFOBJECT *g_video_back_fixed;
static NFOBJECT *g_area_tile_obj;
static NFOBJECT *g_all_radio_obj;
static NFOBJECT *g_all_label_obj;
static NFOBJECT *g_area_radio_obj[MAX_PRIVACY_CNT];
static NFOBJECT *g_area_label_obj[MAX_PRIVACY_CNT];

static PrivacyData *g_org_privacy_data = 0;
static PrivacyData g_privacy_data[GUI_CHANNEL_CNT];
static NFIPCamPrivacyMaskProfile g_profile;

static gint g_radio_idx = 0;
static gint g_video_size_w = 0;

static void _set_video_width(gint ch)
{
	gint ratio_w, ratio_h;
	gint tmp = 0;

	scm_ipcam_get_main_stream_ratio(ch, &ratio_w, &ratio_h);
	g_message("[%s, %d], ratio_w : %d, ratio_h : %d", __FUNCTION__, __LINE__, ratio_w, ratio_h);
	
	if (scm_get_ipcam_corridor_mode(ch) != 0)
	{
		tmp = ratio_w;
		ratio_w = ratio_h;
		ratio_h = tmp;
		
		g_video_size_w = VIDEO_AREA_SIZE_H * ratio_w / ratio_h;
	}
	else
	{
		g_video_size_w = VIDEO_AREA_SIZE_H * ratio_w / ratio_h;
	}
	// g_message("[%s, %d], g_video_size_w : %d", __FUNCTION__, __LINE__, g_video_size_w);
}

static void show_preview(gint ch)
{
	guint ch_mask = 0;
	gint x, y;

	if (ch < 0) return;

	nfui_nfobject_get_offset(g_area_tile_obj, &x, &y);
	ch_mask |= (1 << ch);
    	
	if (scm_get_ipcam_corridor_mode(ch) != 0)
        vsm_live_preview_start(ch_mask, (guint)x, (guint)y, g_video_size_w, VIDEO_AREA_SIZE_H);	
    else
        vsm_live_preview_start(ch_mask, (guint)x+((VIDEO_AREA_SIZE_W-g_video_size_w)/2), (guint)y, g_video_size_w, VIDEO_AREA_SIZE_H);	
}

static void stop_preview()
{
    if (nfui_nfobject_is_shown(g_area_tile_obj))
    {
    	GdkDrawable *drawable = nfui_nfobject_get_window((NFOBJECT*)g_area_tile_obj);
    	GdkGC *gc = nfui_nfobject_get_gc((NFOBJECT*)g_area_tile_obj);
    	GdkColor color = UX_COLOR(COLOR_PRG_IDX(UX_COLOR_808080));
    	gint off_x, off_y;

    	gdk_gc_set_rgb_fg_color(gc, &color);
    	nfui_nfobject_get_offset(g_area_tile_obj, &off_x, &off_y);
    	gdk_draw_rectangle(drawable, gc, TRUE, off_x, off_y, g_video_size_w, VIDEO_AREA_SIZE_H);
    	nfui_nfobject_gc_unref(gc);
    }

	vsm_live_preview_stop();
}

static void _trans_db_to_val()
{
    NFIPCamPrivacyMaskProfile profile;
    gint ch, area;
    gint corr_mode;
    gint db_sx, db_sy, db_ex, db_ey;
    gint max_area_cnt = DAL_get_privacy_max_cnt();

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        memset(&profile, 0x00, sizeof(profile));
        
        if (nf_ipcam_get_privacy_mask_profile(ch, &profile) != IPCAM_SETUP_RTN_DONE)
        {
            profile.is_support = 0;
            continue;
        }        

        if ((profile.block_width == 0) && (profile.block_height == 0))
        {
            profile.is_support = 0;
            continue;
        }
        
        corr_mode = scm_get_ipcam_corridor_mode(ch);
        if (!corr_mode) continue;
        
        for (area = 0; area < max_area_cnt; area++)
        {
            db_sx = g_privacy_data[ch].area[area].sx;
            db_sy = g_privacy_data[ch].area[area].sy;
            db_ex = g_privacy_data[ch].area[area].ex;
            db_ey = g_privacy_data[ch].area[area].ey;
            
            if (db_sx == -1 && db_sy == -1 && db_ex == -1 && db_ey == -1) continue;
            
            if (corr_mode == 1)
            {
                g_privacy_data[ch].area[area].sx = (profile.block_width - 1) - db_ey;
                g_privacy_data[ch].area[area].sy = db_sx;
                g_privacy_data[ch].area[area].ex = (profile.block_width - 1) - db_sy;
                g_privacy_data[ch].area[area].ey = db_ex;
            }
            else if (corr_mode == 2)
            {
                g_privacy_data[ch].area[area].sx = db_sy;
                g_privacy_data[ch].area[area].sy = (profile.block_height - 1) - db_ex;
                g_privacy_data[ch].area[area].ex = db_ey;
                g_privacy_data[ch].area[area].ey = (profile.block_height - 1) - db_sx;
            }
        }
    }
}

static void _trans_val_to_db()
{
    NFIPCamPrivacyMaskProfile profile;
    gint ch, area;
    gint corr_mode;
    gint cur_sx, cur_sy, cur_ex, cur_ey;
    gint max_area_cnt = DAL_get_privacy_max_cnt();
    
    g_memmove(g_org_privacy_data, g_privacy_data, sizeof(PrivacyData) * GUI_CHANNEL_CNT);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        memset(&profile, 0x00, sizeof(profile));
        
        if (nf_ipcam_get_privacy_mask_profile(ch, &profile) != IPCAM_SETUP_RTN_DONE)
        {
            profile.is_support = 0;
            continue;
        }        

        if ((profile.block_width == 0) && (profile.block_height == 0))
        {
            profile.is_support = 0;
            continue;
        }
        
        corr_mode = scm_get_ipcam_corridor_mode(ch);
        if (!corr_mode) continue;
        
        for (area = 0; area < max_area_cnt; area++)
        {
            cur_sx = g_privacy_data[ch].area[area].sx;
            cur_sy = g_privacy_data[ch].area[area].sy;
            cur_ex = g_privacy_data[ch].area[area].ex;
            cur_ey = g_privacy_data[ch].area[area].ey;
            
            if (cur_sx == -1 && cur_sy == -1 && cur_ex == -1 && cur_ey == -1) continue;
            
            if (corr_mode == 1)
            {
                g_org_privacy_data[ch].area[area].sx = cur_sy;
                g_org_privacy_data[ch].area[area].sy = (profile.block_width -1) - cur_ex;
                g_org_privacy_data[ch].area[area].ex = cur_ey;
                g_org_privacy_data[ch].area[area].ey = (profile.block_width - 1) - cur_sx;
            }
            else if (corr_mode == 2)
            {
                g_org_privacy_data[ch].area[area].sx = (profile.block_height - 1) - cur_ey;
                g_org_privacy_data[ch].area[area].sy = cur_sx;
                g_org_privacy_data[ch].area[area].ex = (profile.block_height - 1) - cur_sy;
                g_org_privacy_data[ch].area[area].ey = cur_ex;
            }
        }
    }
}

static gint _redraw_tile_privacy_mask(guint idx_mask, gint draw)
{
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_obj));
    gint idx, select;
    gint i, j;
    
/* 
    for (idx = 0; idx < g_profile.area_num; idx++)
    {
        if (idx_mask & (1 << idx))
        {
            g_message("%s, %d, ch:%d", __FUNCTION__, __LINE__, ch);
            g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, idx);
            g_message("%s, %d, sx:%d, sy:%d, ex:%d, ey:%d", __FUNCTION__, __LINE__, 
                        g_privacy_data[ch].area[idx].sx, g_privacy_data[ch].area[idx].sy, 
                        g_privacy_data[ch].area[idx].ex, g_privacy_data[ch].area[idx].ey);
        }
    }
 */    
    for (i = 0; i < g_profile.block_width; i++)
    {
    	for (j = 0; j < g_profile.block_height; j++)
    	{
            select = 0;
    	
            for (idx = 0; idx < g_profile.area_num; idx++)
            {
                if (idx_mask & (1 << idx))
                {
                    if ((i >= g_privacy_data[ch].area[idx].sx) && (i <= g_privacy_data[ch].area[idx].ex)
                        && (j >= g_privacy_data[ch].area[idx].sy) && (j <= g_privacy_data[ch].area[idx].ey))
                    {
                        select = 1;
                    }
                }            
            }
    	
            if (select)
            {
    			nfui_tile_set_select_state(NF_TILE(g_area_tile_obj), j, i, j, i);
            }
    		else
    		{
    			nfui_tile_set_conv_select_state(NF_TILE(g_area_tile_obj), j, i, j, i);
            }
    	} 
    }

//    if (draw)
//        nfui_tile_draw_area(NF_TILE(g_area_tile_obj), 0, 0, g_profile.block_height-1, g_profile.block_width-1);

    return 0;
}

static gint _change_channel_privacy_objs(gint ch)
{
    gint i;
    gint max_area_cnt;
    gint tmp;

    memset(&g_profile, 0x00, sizeof(NFIPCamPrivacyMaskProfile));

    if (nf_ipcam_get_privacy_mask_profile(ch, &g_profile) != IPCAM_SETUP_RTN_DONE)
    {
        g_profile.is_support = 0;
    }        

    if ((g_profile.block_width == 0) && (g_profile.block_height == 0))
    {
        g_profile.is_support = 0;
    }

    // g_message("%s, %d, is_support:%d", __FUNCTION__, __LINE__, g_profile.is_support);
    // g_message("%s, %d, block_width:%d, block_height:%d", __FUNCTION__, __LINE__, g_profile.block_width, g_profile.block_height);

    max_area_cnt = DAL_get_privacy_max_cnt();

    if (scm_get_ipcam_corridor_mode(ch) != 0) {
        nfui_nfobject_set_size(g_area_tile_obj, g_video_size_w, VIDEO_AREA_SIZE_H);
        nfui_nffixed_put((NFFIXED*)g_video_back_fixed, g_area_tile_obj, (VIDEO_AREA_SIZE_W-g_video_size_w)/2, 0);
    }
    else {
        nfui_nfobject_set_size(g_area_tile_obj, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
        nfui_nffixed_put((NFFIXED*)g_video_back_fixed, g_area_tile_obj, 0, 0);
    }
    nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);

    if (g_profile.is_support)
    {
        nfui_tile_reset_area_size((NFTILE*)g_area_tile_obj, g_profile.block_height, g_profile.block_width);

        nfui_nfobject_enable(g_all_radio_obj);
        nfui_nfobject_enable(g_all_label_obj);

        // g_message("%s, %d, g_profile.area_num:%d", __FUNCTION__, __LINE__, g_profile.area_num);

        nfui_radio_button_set_toggled(NF_BUTTON(g_all_radio_obj), TRUE);        

        for (i = 0; i < max_area_cnt; i++)
        {
            if (i < g_profile.area_num)
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
    }
    else
    {
        nfui_tile_reset_area_size((NFTILE*)g_area_tile_obj, 1, 1);

        nfui_nfobject_disable(g_all_radio_obj);
        nfui_nfobject_disable(g_all_label_obj);        

        for (i = 0; i < max_area_cnt; i++)
        {
            nfui_nfobject_disable(g_area_radio_obj[i]);
            nfui_nfobject_disable(g_area_label_obj[i]);
        }
    }

    g_radio_idx = 0;

    return 0;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_TILE_INIT)
	{
        gint i;
        guint mask = 0;
        
        for (i = 0; i < g_profile.area_num; i++)
        {
            mask |= (1 << i);
        }   

        _redraw_tile_privacy_mask(mask, 1);
        nfui_tile_draw_area(NF_TILE(obj), 0, 0, g_profile.block_height-1, g_profile.block_width-1);
	}
	if (evt->type == NFEVENT_TILE_START_SELECT)
	{
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_obj));	

        if (g_radio_idx == 0) return FALSE;

        g_privacy_data[ch].area[g_radio_idx-1].sx = -1;
        g_privacy_data[ch].area[g_radio_idx-1].sy = -1;
        g_privacy_data[ch].area[g_radio_idx-1].ex = -1;
        g_privacy_data[ch].area[g_radio_idx-1].ey = -1;       
            
        _redraw_tile_privacy_mask(1<<(g_radio_idx-1), 1);
        nfui_tile_draw_area(NF_TILE(obj), 0, 0, g_profile.block_height-1, g_profile.block_width-1);        
	}
	if (evt->type == NFEVENT_TILE_MOVE_SELECT)
	{
  
	}
	else if (evt->type == NFEVENT_TILE_END_SELECT)
	{
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_obj));	
        gint sx, sy, ex, ey;

        if (g_radio_idx == 0) return FALSE;

        nfui_tile_get_selectArea(NF_TILE(obj), &sy, &sx, &ey, &ex);

//        g_message("%s, %d, sx:%d, sy:%d, ex:%d, ey:%d", __FUNCTION__, __LINE__, sx, sy, ex, ey);
	
        g_privacy_data[ch].area[g_radio_idx-1].sx = sx;
        g_privacy_data[ch].area[g_radio_idx-1].sy = sy;
        g_privacy_data[ch].area[g_radio_idx-1].ex = ex;
        g_privacy_data[ch].area[g_radio_idx-1].ey = ey;     
	}
	
	return FALSE;
}


static gboolean post_ch_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
    	gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
        gint i, j;
        guint mask = 0;
        gint max_area_cnt;

        // focus del....
        for (i = 0; i < g_profile.block_width; i++)
        {
        	for (j = 0; j < g_profile.block_height; j++)
        	{
    			nfui_tile_set_select_state(NF_TILE(g_area_tile_obj), j, i, j, i);
    			nfui_tile_set_conv_select_state(NF_TILE(g_area_tile_obj), j, i, j, i);
        	}
        }

        _set_video_width(ch);
    	_change_channel_privacy_objs(ch);
    	show_preview(ch);

        if (g_radio_idx == 0)
        {
            for (i = 0; i < g_profile.area_num; i++)
            {
                mask |= (1 << i);           
            }
          
            _redraw_tile_privacy_mask(mask, 1);
        }
        else
        {               
            _redraw_tile_privacy_mask(1<<(g_radio_idx-1), 1);
        }  

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_all_radio_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_all_label_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_video_back_fixed, GDK_EXPOSE, TRUE);

        max_area_cnt = DAL_get_privacy_max_cnt();

        for (i = 0; i < max_area_cnt; i++)
        {
            nfui_signal_emit(g_area_radio_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_area_label_obj[i], GDK_EXPOSE, TRUE);
        }        
	}
	else if((evt->type == INFY_CAMDB_CHANGE_NOTIFY) || (evt->type == INFY_USRDB_CHANGE_NOTIFY))
	{
		gchar strCh[STRING_SIZE_CAMTITLE+8];
		gint i, j;
    	gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		nfui_combobox_remove_all((NFCOMBOBOX*)obj);

		for (i = 0; i<GUI_CHANNEL_CNT; i++) 
		{
			memset(strCh, 0, (STRING_SIZE_CAMTITLE+8));
			j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);
			var_get_camtitle(&strCh[j], (guint)i);
			nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
		}

		nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), ch);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == GDK_DELETE)  
	{
		uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
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
        for (i = 0; i < g_profile.block_width; i++)
        {
        	for (j = 0; j < g_profile.block_height; j++)
        	{
    			nfui_tile_set_select_state(NF_TILE(g_area_tile_obj), j, i, j, i);
    			nfui_tile_set_conv_select_state(NF_TILE(g_area_tile_obj), j, i, j, i);
        	}
        }
        
        if (idx == 0)
        {
            for (i = 0; i < g_profile.area_num; i++)
            {
                mask |= (1 << i);
            }

            _redraw_tile_privacy_mask(mask, 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);
        }
        else
        {               
            _redraw_tile_privacy_mask(1<<(idx-1), 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), TRUE);
        }

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
        g_radio_idx = idx;

//        g_message("%s, %d, g_radio_idx:%d", __FUNCTION__, __LINE__, g_radio_idx);
	}
	
	return FALSE;
}

static gboolean post_delete_area_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_obj));
        gint i;
        guint mask = 0;

        if (g_radio_idx == 0)
        {
            for (i = 0; i < g_profile.area_num; i++)
            {
                g_privacy_data[ch].area[i].sx = -1;
                g_privacy_data[ch].area[i].sy = -1;
                g_privacy_data[ch].area[i].ex = -1;
                g_privacy_data[ch].area[i].ey = -1;

                mask |= (1 << i);
            }            

            _redraw_tile_privacy_mask(mask, 1);
        }
        else
        {
            g_privacy_data[ch].area[g_radio_idx-1].sx = -1;
            g_privacy_data[ch].area[g_radio_idx-1].sy = -1;
            g_privacy_data[ch].area[g_radio_idx-1].ex = -1;
            g_privacy_data[ch].area[g_radio_idx-1].ey = -1;       
                
            _redraw_tile_privacy_mask(1<<(g_radio_idx-1), 1);
        }

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
	}
	
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
	    gint i;
        guint mask = 0;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        _trans_val_to_db();
        if (_is_changed_data_PrivacyMask() == 0) return FALSE;

        _restore_data_PrivacyMask();
        memset(g_privacy_data, 0x00, sizeof(g_privacy_data));
        g_memmove(g_privacy_data, g_org_privacy_data, sizeof(PrivacyData) * GUI_CHANNEL_CNT);

        if (g_radio_idx == 0)
        {
            for (i = 0; i < g_profile.area_num; i++)
            {
                mask |= (1 << i);
            }

            _redraw_tile_privacy_mask(mask, 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);
        }
        else
        {               
            _redraw_tile_privacy_mask(1<<(g_radio_idx-1), 1);
            nfui_tile_sensitive(NF_TILE(g_area_tile_obj), TRUE);            
        }

        nfui_signal_emit(g_area_tile_obj, GDK_EXPOSE, TRUE);
        
 	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        _trans_val_to_db();
        if (_is_changed_data_PrivacyMask() == 0) return FALSE;

        _save_data_PrivacyMask();
		syscam_set_changeflag(1);		
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
    	mb_type ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        _trans_val_to_db();
        if (_is_changed_data_PrivacyMask() == 1)
        {
            ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

            if (ret == NFTOOL_MB_OK)
			{
                _save_data_PrivacyMask();            
				syscam_set_changeflag(1);
			}		
            else
            {
                _restore_data_PrivacyMask();
                memset(g_privacy_data, 0x00, sizeof(g_privacy_data));
                g_memmove(g_privacy_data, g_org_privacy_data, sizeof(PrivacyData) * GUI_CHANNEL_CNT);

            }
        }
    
		SystemSetupCam_Destroy(obj);	
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

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		break;

		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default:
		break;
	}

	return FALSE;
}

void init_PrivacyMask_Area_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *video_back_fixed;
    NFOBJECT *group_fixed;
	NFOBJECT *obj;

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GdkColor tile_scolor[4] = {UX_COLOR(734), UX_COLOR(734), UX_COLOR(734), UX_COLOR(734)};
	gint label_color[NFOBJECT_STATE_COUNT];
	gchar strCh[STRING_SIZE_CAMTITLE+8];
	gchar strBuf[16];
	GSList *slist = NULL;
	
	gint size_w, size_h;
	guint pos_x, pos_y;
	gint max_area_cnt;
	guint i = 0, j = 0;
    guint mask;

	radio_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_RADIO_OFF), NULL);

	label_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(968);
	label_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(968);
	label_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(968);
	label_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(972);

    g_radio_idx = 0;
	
	g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);
	nfui_regi_post_event_callback(parent, post_page_event_handler);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	pos_x = 27;
	pos_y = 40;

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 290, 40); 
	nfui_nfobject_show(obj); 
	nfui_regi_post_event_callback(obj, post_ch_combo_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    g_ch_obj = obj;

	for (i = 0; i<GUI_CHANNEL_CNT; i++)
	{
		memset(strCh, 0, sizeof(strCh));
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);
		var_get_camtitle(&strCh[j], (guint)i);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
	}

	uxm_reg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);

	// preview label
	pos_y = 120;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 160, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	// motion area
	pos_y += 40;

	video_back_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(video_back_fixed, NFOBJECT_STATE_NORMAL, VIDEO_BG_COLOR);
	nfui_nfobject_set_size(video_back_fixed, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
	nfui_nfobject_show(video_back_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, video_back_fixed, pos_x, pos_y);
	g_video_back_fixed = video_back_fixed;

	obj = nfui_tile_new(1, 1);
	nfui_tile_set_fill(NF_TILE(obj), TRUE);
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_NORMAL, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000))); 
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_FOCUS, &UX_COLOR(733)); 
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_SELECT, tile_scolor);
	nfui_tile_set_drawable_outline(NF_TILE(obj), FALSE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
	nfui_regi_post_event_callback(obj, post_tile_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)video_back_fixed, obj, 0, 0);
	g_area_tile_obj = obj;

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

    max_area_cnt = DAL_get_privacy_max_cnt();

    for (i = 0; i < max_area_cnt; i++)
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
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 815);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

	obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

    _set_video_width(0);
	_change_channel_privacy_objs(0);
}

void PrivacyMask_Area_Show_Preview()
{
    gint ch = nfui_combobox_get_cur_index(g_ch_obj);
	show_preview(ch);
}

void PrivacyMask_Area_Stop_Preview()
{
	stop_preview();
}

gint _set_data_PrivacyMask_Area(PrivacyData *data)
{
    g_org_privacy_data = data;
    
    memset(g_privacy_data, 0x00, sizeof(g_privacy_data));
    g_memmove(g_privacy_data, g_org_privacy_data, sizeof(PrivacyData) * GUI_CHANNEL_CNT);
    
    _trans_db_to_val();
/*
    g_message("%s, %d, sx:%d, sy:%d, ex:%d, ey:%d", __FUNCTION__, __LINE__, 
                g_privacy_data[0].area[0].sx, g_privacy_data[0].area[0].sy, 
                g_privacy_data[0].area[0].ex, g_privacy_data[0].area[0].ey);
*/

    return 0;
}

void PrivacyMask_Area_tab_in_handler()
{
    gint ch = nfui_combobox_get_cur_index(g_ch_obj);
	show_preview(ch);
}


void PrivacyMask_Area_tab_out_handler()
{
	mb_type ret;
    gint i;
    guint mask = 0;

    stop_preview();
        
    _trans_val_to_db();
    if (_is_changed_data_PrivacyMask() == 1)
    {
        ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

        if (ret == NFTOOL_MB_OK)
        {
            _save_data_PrivacyMask();
			syscam_set_changeflag(1);		
        }
        else
        {
            _restore_data_PrivacyMask();
            memset(g_privacy_data, 0x00, sizeof(g_privacy_data));
            g_memmove(g_privacy_data, g_org_privacy_data, sizeof(PrivacyData) * GUI_CHANNEL_CNT);

            if (g_radio_idx == 0)
            {
                for (i = 0; i < g_profile.area_num; i++)
                {
                    mask |= (1 << i);
                }

                _redraw_tile_privacy_mask(mask, 1);
                nfui_tile_sensitive(NF_TILE(g_area_tile_obj), FALSE);
            }
            else
            {               
                _redraw_tile_privacy_mask(1<<(g_radio_idx-1), 0);
                nfui_tile_sensitive(NF_TILE(g_area_tile_obj), TRUE);            
            }
        }
    }
}
