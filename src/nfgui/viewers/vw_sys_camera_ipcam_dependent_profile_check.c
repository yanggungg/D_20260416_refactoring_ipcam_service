#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"
#include "nf_api_ipcam.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_ipcam_internal.h"

static gboolean g_supported_base_shutter_speed[GUI_CHANNEL_CNT] = {0,};  //  base shutter speed supported check 

static gint _dependent_check_base_shutter_speed(guint64 *dependent, guint64 *visible, guint64 *enable, guint64 base_category);
static gint _dependent_check_max_shutter_speed(guint64 *dependent, guint64 *visible, guint64 *enable, guint64 max_category);
static gint _dependent_check_antiflicker(guint64 *dependent, guint64 *visible, guint64 *enable, guint64 antiflicker_category);
static gint _dependent_check_dc_iris(guint64 *dependent,  guint64 *visible, guint64 *enable, guint64 dc_iris_category);

static gint _check_base_shutter_speed_support(gint ch);

DEPENDENT_CHECK_PROFILE_T g_ipcam_data_temp[GUI_CHANNEL_CNT];

// :::::::::::::::::::::::::: set ipcam data ::::::::::::::::::::::::::::

#if 0
#endif

static gint _set_ipcam_data(gint ch)
{
    DAL_set_IPCamSetup_data(g_ipcamData[ch], ch);
    DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, ch);
    return 0;
}

static void _set_antiflicker_db(gint ch, NFIPCamOption_onvif *antiflicker, gint antiflicker_cnt)
{
    gint i;

    for( i = 0; i < antiflicker_cnt; i++)
    {
        if(antiflicker[i].selected && (g_ipcamData[ch].antiflicker != antiflicker[i].value)) // callback �� selected add plz
        {
            g_ipcamData[ch].antiflicker = antiflicker[i].value;
            _set_ipcam_data(ch);

            break;
        }
    }
}

static void _set_dc_iris_db(gint ch, NFIPCamOption_onvif *dc_iris, gint dc_iris_cnt)
{
    gint i;

    for( i = 0; i < dc_iris_cnt; i++)
    {   
        if(dc_iris[i].selected && (g_ipcamData[ch].iris_control != dc_iris[i].value))  // callbak �� selected  add  plz
        {
            g_ipcamData[ch].iris_control = dc_iris[i].value;
            _set_ipcam_data(ch);

            break;
        }
    }
}

static void _set_max_shutter_speed_db(gint ch, NFIPCamOption_onvif *max_shutter, gint max_shutter_cnt)
{
    gint i;

    for (i = 0; i < max_shutter_cnt; i++)
    {
        if(max_shutter[i].selected)    break;
    }

    if( i == max_shutter_cnt) max_shutter[0].selected = 1;

    for (i = 0; i < max_shutter_cnt; i++)  // callback  >> selected add plz  
    {
        if(max_shutter[i].selected && (g_ipcamData[ch].max_shutter != max_shutter[i].value))
        {
            g_ipcamData[ch].max_shutter = max_shutter[i].value;
            _set_ipcam_data(ch);

            break;
        }
    }
}

static void _set_base_shutter_speed_db(gint ch, NFIPCamOption_onvif *base_shutter, gint base_shutter_cnt)
{
    gint i;

    for (i = 0; i < base_shutter_cnt; i++)
    {
        if(base_shutter[i].selected)    break;
    }

    if( i == base_shutter_cnt) base_shutter[0].selected = 1;
    
    for (i = 0; i < base_shutter_cnt; i++)  // callback >> seleted add plz
    {        
        if(base_shutter[i].selected && (g_ipcamData[ch].base_shutter != base_shutter[i].value))
        {
            g_ipcamData[ch].base_shutter = base_shutter[i].value;
            _set_ipcam_data(ch);

            break;
        }
    }
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#if 0
#endif

// :::::::::::::::::::::::::::  mem cpy :::::::::::::::::::::::::::::::::

// set memcpy for compare base shutter speed with max shutter speed 
void _set_cur_base_shutter_speed_memcpy_callback(NFIPCamOption_onvif *base_shutter_speed, gint cnt, gint ch)
{
    memcpy(&g_ipcam_data_temp[ch].base_shutter_temp, base_shutter_speed, sizeof(NFIPCamOption_onvif) * NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR);

    g_ipcam_data_temp[ch].base_shutter_temp_cnt = cnt;
}

void _set_cur_max_shutter_speed_memcpy_callback(NFIPCamOption_onvif *max_shutter_speed, gint cnt, gint ch)
{
    memcpy(&g_ipcam_data_temp[ch].max_shutter_temp, max_shutter_speed, sizeof(NFIPCamOption_onvif) * NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR);
    
    g_ipcam_data_temp[ch].max_shutter_temp_cnt = cnt;
}

#if 0
#endif

// ::::::::::::::::: dependent set db  anti / defog / slow shutter / blc control / wdr :::::::::::::::::::::

static void _set_combobox_obj_exposure(NFOBJECT *obj, gint idx)
{
    if(obj)
    {
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, idx); 
        nfui_signal_emit((NFCOMBOBOX*)obj, GDK_EXPOSE, TRUE);       
    }
}


static gint _get_ipcamData_setValue_index(gint ch, NFIPCamOption_onvif *ipcamData, gint ipcamData_cnt, gchar *caption, guint *ipcamData_value)
{
    gint i = 0;
    gint idx = 0;
    gint selected = 0;

    for (idx = 0; idx < ipcamData_cnt; idx++)
    {
        if(!caption) break;
        if(!strcmp(ipcamData[idx].caption, caption))  break;
    }
    
    if(idx == ipcamData_cnt) idx = 0;
    
    for( selected = 0; selected < ipcamData_cnt; selected++)
    {
        if(ipcamData[selected].selected) break;
    }

    if(idx != selected)
    {
        ipcamData[selected].selected = 0;      
        ipcamData[idx].selected = 1;

        if(*ipcamData_value != ipcamData[idx].value)
        {
            *ipcamData_value = ipcamData[idx].value;
            _set_ipcam_data(ch);
        }
    }
    else idx = -1;

    return idx;
}

#if 0
#endif

gboolean _set_antiflicker_value_sync(gint ch, gchar *antiflicker_caption)
{
    gint i;
    NFOBJECT *antiflicker_obj;
    gint antiflicker_idx = 0;

    antiflicker_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER, _post_antiflicker_combo_event_handler);
    if (antiflicker_obj) antiflicker_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker, g_ipcam_manage[ch].exposure.profile.antiflicker_cnt, antiflicker_caption, &g_ipcamData[ch].antiflicker);

    if (antiflicker_idx != -1) _set_combobox_obj_exposure(antiflicker_obj, antiflicker_idx);
    
    antiflicker_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION, _post_antiflicker_m_combo_event_handler);
    if (antiflicker_obj) antiflicker_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker_motion, g_ipcam_manage[ch].exposure.profile.antiflicker_motion_cnt, antiflicker_caption, &g_ipcamData[ch].antiflicker);

    if (antiflicker_idx != -1) _set_combobox_obj_exposure(antiflicker_obj, antiflicker_idx);

    antiflicker_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF, _post_antiflicker_m_off_combo_event_handler);
    if (antiflicker_obj) antiflicker_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off, g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off_cnt, antiflicker_caption, &g_ipcamData[ch].antiflicker);

    if (antiflicker_idx != -1) _set_combobox_obj_exposure(antiflicker_obj, antiflicker_idx);    

    antiflicker_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF, _post_antiflicker_a_off_combo_event_handler);
    if (antiflicker_obj) antiflicker_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off, g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off_cnt, antiflicker_caption, &g_ipcamData[ch].antiflicker);

    if (antiflicker_idx != -1) _set_combobox_obj_exposure(antiflicker_obj, antiflicker_idx);

    return 0;
}

gboolean _set_wdr_value_sync(gint ch, gchar *wdr_caption)
{
    gint i;
    NFOBJECT *wdr_obj;
    gint wdr_idx = 0;

    wdr_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE, _post_wide_dynamic_mode_combo_event_handler);
    
    if(wdr_obj)        wdr_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.wdr, g_ipcam_manage[ch].exposure.profile.wdr_cnt, wdr_caption, &g_ipcamData[ch].wdr_mode);
    if(wdr_idx != -1)  _set_combobox_obj_exposure(wdr_obj, wdr_idx);

    return 0;
}

gboolean _set_slow_shutter_value_sync(gint ch, gchar *slow_caption)
{
    gint i;
    NFOBJECT *slow_shutter_obj;
    gint slow_shutter_idx = 0; 
    
    slow_shutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_SLOWSHUTTER, _post_exposure_slowshutter_combo_event_handler);
    
    if(slow_shutter_obj)         slow_shutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.slowshutter, g_ipcam_manage[ch].exposure.profile.slowshutter_cnt, slow_caption, &g_ipcamData[ch].slow_shutter);
    if(slow_shutter_idx != -1)   _set_combobox_obj_exposure(slow_shutter_obj, slow_shutter_idx);
           
    return 0;
}

gboolean _set_defog_value_sync(gint ch, gchar *defog_caption)
{
    gint i;
    NFOBJECT *defog_obj;
    gint defog_idx = 0;

    defog_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DEFOG, _post_defog_combo_event_handler);

    if(defog_obj)       defog_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.defog, g_ipcam_manage[ch].exposure.profile.defog_cnt, defog_caption, &g_ipcamData[ch].defog);
    if(defog_idx != -1) _set_combobox_obj_exposure(defog_obj, defog_idx);

    return 0;
}

gboolean _set_blc_control_value_sync(gint ch, gchar *blc_control_caption)
{
    gint i;
    NFOBJECT *blc_control_obj;
    gint blc_control_idx = 0;

    blc_control_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE, _post_blc_control_combo_event_handler);

    if(blc_control_obj)         blc_control_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.blc, g_ipcam_manage[ch].exposure.profile.blc_cnt, blc_control_caption, &g_ipcamData[ch].blc_control);
    if(blc_control_idx != -1)   _set_combobox_obj_exposure(blc_control_obj, blc_control_idx);
       
    return 0;        
}

gboolean _set_max_shutter_value_sync(gint ch, gchar *max_caption)
{
    gint i;
    NFOBJECT *maxShutter_obj;
    gint maxShutter_idx = 0;

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_50,  _post_max_shutter_50_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_50, g_ipcam_manage[ch].exposure.profile.max_shutter_50_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_60,  _post_max_shutter_60_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_60, g_ipcam_manage[ch].exposure.profile.max_shutter_60_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF,  _post_max_shutter_off_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_off, g_ipcam_manage[ch].exposure.profile.max_shutter_off_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF,  _post_max_shutter_motion_off_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off, g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON,  _post_max_shutter_motion_off_on_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on, g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF,  _post_max_shutter_auto_off_off_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON,  _post_max_shutter_auto_off_on_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    maxShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV,  _post_max_shutter_auto_off_on_tv_combo_event_handler);
    if(maxShutter_obj)          maxShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv_cnt, max_caption, &g_ipcamData[ch].max_shutter); 
    if(maxShutter_idx != -1)    _set_combobox_obj_exposure(maxShutter_obj, maxShutter_idx);  

    return 0;
}

gboolean _set_base_shutter_value_sync(gint ch, gchar *base_caption)
{
    gint i;
    NFOBJECT *baseShutter_obj;
    gint baseShutter_idx = 0;

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_100, _post_base_shutter_100_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_100, g_ipcam_manage[ch].exposure.profile.base_shutter_100_cnt, base_caption, &g_ipcamData[ch].base_shutter); 
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120, _post_base_shutter_120_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120, g_ipcam_manage[ch].exposure.profile.base_shutter_120_cnt, base_caption, &g_ipcamData[ch].base_shutter); 
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300, _post_base_shutter_100_300_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_100_300, g_ipcam_manage[ch].exposure.profile.base_shutter_100_300_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000, _post_base_shutter_100_5000_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360, _post_base_shutter_120_360_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120_360, g_ipcam_manage[ch].exposure.profile.base_shutter_120_360_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000, _post_base_shutter_120_5000_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   
    
    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262, _post_base_shutter_120_262_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120_262, g_ipcam_manage[ch].exposure.profile.base_shutter_120_262_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262, _post_base_shutter_30_262_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_262, g_ipcam_manage[ch].exposure.profile.base_shutter_30_262_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100, _post_base_shutter_25_100_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_25_100, g_ipcam_manage[ch].exposure.profile.base_shutter_25_100_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300, _post_base_shutter_25_300_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_25_300, g_ipcam_manage[ch].exposure.profile.base_shutter_25_300_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000, _post_base_shutter_25_5000_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120, _post_base_shutter_30_120_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_120, g_ipcam_manage[ch].exposure.profile.base_shutter_30_120_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360, _post_base_shutter_30_360_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_360, g_ipcam_manage[ch].exposure.profile.base_shutter_30_360_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   

    baseShutter_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000, _post_base_shutter_30_5000_combo_event_handler); 
    if(baseShutter_obj)         baseShutter_idx = _get_ipcamData_setValue_index(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000_cnt, base_caption, &g_ipcamData[ch].base_shutter);
    if(baseShutter_idx != -1)  _set_combobox_obj_exposure(baseShutter_obj, baseShutter_idx);   
    
    return 0;
}

#if 0
#endif

static void _get_exposure_mode_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;
    
    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MODE == 0) return -1;
    
    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.mode_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.mode[i].value == tmpData->exposure_mode)
        {
            if(dependent) *dependent = g_ipcam_manage[ch].exposure.profile.mode[i].dependent_category;
            if(visible) *visible = g_ipcam_manage[ch].exposure.profile.mode[i].visible_category;
            if(enable) *enable = g_ipcam_manage[ch].exposure.profile.mode[i].enable_category;

            break;
        }
    }        
}

static void _get_wdr_mode_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.wdr[0].category == 0) return -1;

    for ( i = 0; i < g_ipcam_manage[ch].exposure.profile.wdr_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.wdr[i].value == tmpData->wdr_mode)
        {
            if(dependent) *dependent = g_ipcam_manage[ch].exposure.profile.wdr[i].dependent_category;
            if(visible) *visible = g_ipcam_manage[ch].exposure.profile.wdr[i].visible_category;
            if(enable) *enable = g_ipcam_manage[ch].exposure.profile.wdr[i].enable_category;

            break;
        }
    } 
}

static void _get_antiflicker_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_cnt; i++)
    {       
        if (g_ipcam_manage[ch].exposure.profile.antiflicker[i].selected)  // db �� ���� �ȵ�. because ī�װ���� ���������� ����� ��� db�� ���߿� set �ϱ⿡...
        {       
            if(dependent) *dependent = g_ipcam_manage[ch].exposure.profile.antiflicker[i].dependent_category;
            if(visible) *visible = g_ipcam_manage[ch].exposure.profile.antiflicker[i].visible_category;
            if(enable) *enable = g_ipcam_manage[ch].exposure.profile.antiflicker[i].enable_category;

            break;
        }
    }    
}

static void _get_antiflicker_m_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_motion_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].selected)
        {
            if(dependent) *dependent = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].dependent_category;
            if(visible) *visible = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].visible_category;
            if(enable) *enable = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].enable_category;
            
            break;
        }
    }      
}

static void _get_antiflicker_m_off_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].selected)
        {
            if(dependent) *dependent = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].dependent_category;
            if(visible) *visible = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].visible_category;
            if(enable) *enable = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].enable_category;

            break;
        }
    }
}
static void _get_antiflicker_a_off_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].selected)
        {
            if(dependent) *dependent = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].dependent_category;
            if(visible) *visible = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].visible_category;
            if(enable) *enable = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].enable_category;

            break;
        }
    }
}

static gint _get_dciris_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS == 0) return -1;
    
    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dc_iris_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.dc_iris[i].selected)
        {       
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.dc_iris[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.dc_iris[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.dc_iris[i].enable_category;
        }
    }

    return 0;
}

static gint _get_dciris_motion_category_dependent_check(guint64 *dependent, guint64 *visible, guint64 *enable, IPCamSetupData *tmpData, gint ch)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS_MOTION == 0) return -1;
    
    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dc_iris_motion_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].selected)
        {       
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].enable_category;
        }
    }

    return 0;
}


static guint64 _get_base_shutter_speed_category_dependent_check(gint ch)
{
    guint64 sig_type = 0;
    guint64 iris_control = 0;
    gint i;
    
    sig_type |= (1 << DAL_get_sig_type());

    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & (NF_IPCAM_EXPOSURE_DCIRIS | NF_IPCAM_EXPOSURE_DCIRIS_MOTION))
        iris_control = g_ipcamData[ch].iris_control;
    else
        iris_control |= ~iris_control;
   
    for ( i = 0; i < 15; i++) 
    {    
        if((g_ipcam_manage[ch].exposure.profile.base_shutter_speed_table[i][0] & g_ipcamData[ch].exposure_mode)
            && (g_ipcam_manage[ch].exposure.profile.base_shutter_speed_table[i][1] & iris_control)
            && (g_ipcam_manage[ch].exposure.profile.base_shutter_speed_table[i][2] & g_ipcamData[ch].wdr_mode)
            && (g_ipcam_manage[ch].exposure.profile.base_shutter_speed_table[i][3] & g_ipcamData[ch].antiflicker)
            && (g_ipcam_manage[ch].exposure.profile.base_shutter_speed_table[i][4] & sig_type))
        {        
            return g_ipcam_manage[ch].exposure.profile.base_shutter_speed_table[i][5];
        }
    }       

    return 0;
}


#if 0
#endif

gint get_antiflicker_category_manage_int_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData)
{
    guint64 dependent_category = 0;
    guint64 visible_category = 0;
    guint64 supported_category = 0;
    gint supp = 0;
    
    supported_category = g_ipcam_manage[ch].exposure.profile.supported_exposure;

    _get_exposure_mode_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);
    supported_category &= ~(dependent_category & ~visible_category);
 
    if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER)                   get_data_exposure_antiflicker(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF)   get_data_exposure_antiflicker_m_off(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION)       get_data_exposure_antiflicker_m(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF)     get_data_exposure_antiflicker_a_off(ch, lt, &supp);
     
    return supp;
}

gint get_dc_iris_category_manage_init_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData)
{
    guint64 dependent_category = 0;
    guint64 supported_category = 0;
    guint64 visible_category = 0;
    gint supp = 0;

    supported_category = g_ipcam_manage[ch].exposure.profile.supported_exposure;

    _get_exposure_mode_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);
    supported_category &= ~(dependent_category & ~visible_category);
 
    if(supported_category & NF_IPCAM_EXPOSURE_DCIRIS_MOTION)      get_data_exposure_dciris_motion(ch, lt, &supp);    
    else if(supported_category & NF_IPCAM_EXPOSURE_DCIRIS)        get_data_exposure_dciris(ch, lt, &supp);

    return supp;
}

gint get_max_shutter_speed_category_init_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData)
{
    guint64 dependent_category = 0;
    guint64 visible_category = 0;
    guint64 supported_category = 0;
    guint supp = 0;

    supported_category = g_ipcam_manage[ch].exposure.profile.supported_exposure;
    
    _get_exposure_mode_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);
    supported_category &= ~(dependent_category & ~visible_category);
    
    _get_wdr_mode_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);
    supported_category &= ~(dependent_category & ~visible_category);
 
    if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER)                   _get_antiflicker_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);
    else if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF)   _get_antiflicker_m_off_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);    
    else if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION)       _get_antiflicker_m_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);
    else if(supported_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF)     _get_antiflicker_a_off_category_dependent_check(&dependent_category, &visible_category, 0, tmpData, ch);

    supported_category &= ~(dependent_category & ~visible_category);    
 
    if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_50)                  get_data_exposure_max_shutter_50(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_60)             get_data_exposure_max_shutter_60(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF)            get_data_exposure_max_shutter_off(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF)     get_data_exposure_max_shutter_motion_off(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON)  get_data_exposure_max_shutter_motion_off_on(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF)   get_data_exposure_max_shutter_auto_off_off(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV) get_data_exposure_max_shutter_auto_off_on_tv(ch, lt, &supp);
    else if(supported_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON)    get_data_exposure_max_shutter_auto_off_on(ch, lt, &supp);
    
    return supp;
}

gint get_base_shutter_speed_category_manage_init_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData)
{   
    guint64 base_category = 0;
    gint supp = 1;
    
    base_category = _get_base_shutter_speed_category_dependent_check(ch);
     
    if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100)              get_data_exposure_base_shutter_100(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120)         get_data_exposure_base_shutter_120(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300)     get_data_exposure_base_shutter_100_300(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000)    get_data_exposure_base_shutter_100_5000(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360)     get_data_exposure_base_shutter_120_360(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000)    get_data_exposure_base_shutter_120_5000(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262)     get_data_exposure_base_shutter_120_262(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262)      get_data_exposure_base_shutter_30_262(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100)      get_data_exposure_base_shutter_25_100(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300)      get_data_exposure_base_shutter_25_300(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000)     get_data_exposure_base_shutter_25_5000(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120)      get_data_exposure_base_shutter_30_120(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360)      get_data_exposure_base_shutter_30_360(ch, lt, &supp);
    else if(base_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000)     get_data_exposure_base_shutter_30_5000(ch, lt, &supp);
    else supp = 0;

    if(supp && _check_base_shutter_speed_support(ch))   g_supported_base_shutter_speed[ch] = 1;
    else    g_supported_base_shutter_speed[ch] = 0;
 
    return supp;
}

#if 0
#endif

gint get_antiflicker_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported)
{
    static guint64 visible_category = 0;
    guint64 antiflicker_category = 0;
    gint supp = 1;

    if(supported) visible_category = *supported;
 
    if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER;
        _get_antiflicker_category_dependent_check(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], 0, ch);
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;
        _get_antiflicker_m_off_category_dependent_check(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], 0, ch);
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;
        _get_antiflicker_m_category_dependent_check(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], 0, ch);
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;
        _get_antiflicker_a_off_category_dependent_check(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], 0, ch);
    }
    else    supp = 0;
        
    if(antiflicker_category)
        _dependent_check_antiflicker(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], antiflicker_category);
     
    return supp;
}

gint get_dc_iris_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported)
{
    static guint64 visible_category = 0;
    guint64 dc_iris_category = 0;
    gint supp = 1;

    if(supported) visible_category = *supported;
 
    if(visible_category & NF_IPCAM_EXPOSURE_DCIRIS_MOTION)
    {
        dc_iris_category |= NF_IPCAM_EXPOSURE_DCIRIS_MOTION;    
        _get_dciris_motion_category_dependent_check(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], 0, ch);
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_DCIRIS)
    {
        dc_iris_category |= NF_IPCAM_EXPOSURE_DCIRIS;        
        _get_dciris_category_dependent_check(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], 0, ch);
    }
    else    supp = 0;
 
    if(dc_iris_category)
        _dependent_check_dc_iris(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], dc_iris_category);

    return supp;
}

gint get_max_shutter_speed_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported)
{
    static guint64 visible_category = 0;
    guint64 max_category = 0;
    guint supp = 1;

    if(supported) visible_category = *supported;
 
    if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_50)                  max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_50;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_60)             max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF)            max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF)     max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON)  max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF)   max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV) max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON)    max_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;
    else supp = 0;

    _dependent_check_max_shutter_speed(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], max_category);
    
    return supp;
}

gint get_base_shutter_speed_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported)
{   
    guint64 base_category = 0;
    gint supp = 1;
    
    base_category = _get_base_shutter_speed_category_dependent_check(ch);
     
    if(!base_category) supp = 0;

    if(supp && _check_base_shutter_speed_support(ch))   g_supported_base_shutter_speed[ch] = 1;
    else    g_supported_base_shutter_speed[ch] = 0;
 
    _dependent_check_base_shutter_speed(&lt->dependent_category[lt->init], &lt->visible_category[lt->init], &lt->enable_category[lt->init], base_category);

    return supp;
}

#if 0
#endif

gint get_antiflicker_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category)
{
    static guint64 visible_category = 0;
    guint64 antiflicker_category = 0;
    gint supp = 0;

    if(visible) visible_category = *supported_category;
     
    if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER;         
        _get_antiflicker_category_dependent_check(dependent, visible, enable, 0, ch);
        _set_antiflicker_db(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker, g_ipcam_manage[ch].exposure.profile.antiflicker_cnt);
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;
        _get_antiflicker_m_off_category_dependent_check(dependent, visible, enable, 0, ch);
        _set_antiflicker_db(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off, g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off_cnt);        
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION) 
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;   
        _get_antiflicker_m_category_dependent_check(dependent, visible, enable, 0, ch);
        _set_antiflicker_db(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker_motion, g_ipcam_manage[ch].exposure.profile.antiflicker_motion_cnt);        
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF)
    {
        antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;  
        _get_antiflicker_a_off_category_dependent_check(dependent, visible, enable, 0, ch);
        _set_antiflicker_db(ch, &g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off, g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off_cnt);        
    }

    _dependent_check_antiflicker(dependent, visible, enable, antiflicker_category);
        
    return supp;

}

gint get_dc_iris_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category)
{
    static guint64 visible_category = 0;
    guint64 dc_iris_category = 0;
    gint supp = 0;

    if(visible) visible_category = *supported_category;

    if(visible_category & NF_IPCAM_EXPOSURE_DCIRIS_MOTION)
    {
        dc_iris_category |= NF_IPCAM_EXPOSURE_DCIRIS_MOTION;    
        _get_dciris_motion_category_dependent_check(dependent, visible, enable, 0, ch);
        _set_dc_iris_db(ch, &g_ipcam_manage[ch].exposure.profile.dc_iris_motion, g_ipcam_manage[ch].exposure.profile.dc_iris_motion_cnt);
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_DCIRIS)
    {
        dc_iris_category |= NF_IPCAM_EXPOSURE_DCIRIS;          
        _get_dciris_category_dependent_check(dependent, visible, enable, 0, ch);
        _set_dc_iris_db(ch, &g_ipcam_manage[ch].exposure.profile.dc_iris, g_ipcam_manage[ch].exposure.profile.dc_iris_cnt);
    }

    _dependent_check_dc_iris(dependent, visible, enable, dc_iris_category);
         
    return supp;
}

gint get_maxshutter_speed_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category) 
{   
    static guint64 visible_category = 0;
    guint64 max_shutter_category = 0;
    guint supp = 0;

    if(visible) visible_category = *supported_category;
    
    if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_50)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_50, g_ipcam_manage[ch].exposure.profile.max_shutter_50_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_50, g_ipcam_manage[ch].exposure.profile.max_shutter_50_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_50;              
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_60)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_60, g_ipcam_manage[ch].exposure.profile.max_shutter_60_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_60, g_ipcam_manage[ch].exposure.profile.max_shutter_60_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;     
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_off, g_ipcam_manage[ch].exposure.profile.max_shutter_off_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_off, g_ipcam_manage[ch].exposure.profile.max_shutter_off_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;          
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off, g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off, g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF;    
    }        
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on, g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on, g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;   
    }
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;    
    }        
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;  
    }        
    else if(visible_category & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON)
    {
        _set_max_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_cnt);
        _set_cur_max_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on, g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_cnt, ch);
        max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;     
    }        

    if(!max_shutter_category)   *supported_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;  //  3�� �۾��� �ȵǴ� ���� ���� Ȯ��

     _dependent_check_max_shutter_speed(dependent, visible, enable, max_shutter_category);

    return 0;
}

gint get_base_shutter_speed_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category)
{
    static guint64 visible_category = 0;
    guint64 base_shutter_category = 0;
    gint    supp = 1;
   
    base_shutter_category = _get_base_shutter_speed_category_dependent_check(ch);
     
    if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_100, g_ipcam_manage[ch].exposure.profile.base_shutter_100_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_100, g_ipcam_manage[ch].exposure.profile.base_shutter_100_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120, g_ipcam_manage[ch].exposure.profile.base_shutter_120_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_120, g_ipcam_manage[ch].exposure.profile.base_shutter_120_cnt, ch);    
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_100_300, g_ipcam_manage[ch].exposure.profile.base_shutter_100_300_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_100_300, g_ipcam_manage[ch].exposure.profile.base_shutter_100_300_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120_360, g_ipcam_manage[ch].exposure.profile.base_shutter_120_360_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_120_360, g_ipcam_manage[ch].exposure.profile.base_shutter_120_360_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_120_262, g_ipcam_manage[ch].exposure.profile.base_shutter_120_262_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_120_262, g_ipcam_manage[ch].exposure.profile.base_shutter_120_262_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_262, g_ipcam_manage[ch].exposure.profile.base_shutter_30_262_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_262, g_ipcam_manage[ch].exposure.profile.base_shutter_30_262_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_25_100, g_ipcam_manage[ch].exposure.profile.base_shutter_25_100_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_25_100, g_ipcam_manage[ch].exposure.profile.base_shutter_25_100_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300) 
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_25_300, g_ipcam_manage[ch].exposure.profile.base_shutter_25_300_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_25_300, g_ipcam_manage[ch].exposure.profile.base_shutter_25_300_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000) 
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_120, g_ipcam_manage[ch].exposure.profile.base_shutter_30_120_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_120, g_ipcam_manage[ch].exposure.profile.base_shutter_30_120_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_360, g_ipcam_manage[ch].exposure.profile.base_shutter_30_360_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_360, g_ipcam_manage[ch].exposure.profile.base_shutter_30_360_cnt, ch);
    }
    else if(base_shutter_category & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000)
    {
        _set_base_shutter_speed_db(ch, &g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000_cnt);
        _set_cur_base_shutter_speed_memcpy_callback(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000, g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000_cnt, ch);
    }
    else supp = 0;

    if(supp && _check_base_shutter_speed_support(ch))   g_supported_base_shutter_speed[ch] = 1;
    else    g_supported_base_shutter_speed[ch] = 0;

    _dependent_check_base_shutter_speed(dependent, visible, enable, base_shutter_category);         

    return 0;
}


#if 0
#endif

static gint _dependent_check_antiflicker(guint64 *dependent, guint64 *visible, guint64 *enable, guint64 antiflicker_category)
{
    guint64 all_antiflicker_category = 0;

    all_antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER;
    all_antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;
    all_antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;
    all_antiflicker_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;
    
    *dependent |= all_antiflicker_category;    

    if(!antiflicker_category)
    {
        if(visible)   *visible &= ~all_antiflicker_category;
        if(enable)    *enable &= ~all_antiflicker_category;
    }
    else
    {
        if(visible)
        {
            *visible &= ~all_antiflicker_category;
            *visible |= antiflicker_category;
        }
        if(enable)
        {   
            *enable  &= ~all_antiflicker_category;
            *enable  |= antiflicker_category;
        }
    }
    
    return 0;
}

static gint _dependent_check_dc_iris(guint64 *dependent,  guint64 *visible, guint64 *enable, guint64 dc_iris_category)
{
    guint64 all_iris_category = 0;

    all_iris_category |= NF_IPCAM_EXPOSURE_DCIRIS;
    all_iris_category |= NF_IPCAM_EXPOSURE_DCIRIS_MOTION;

    *dependent |= all_iris_category;

    if(!dc_iris_category)
    {
        if(visible) *visible &= ~all_iris_category;
        if(enable)  *enable  &= ~all_iris_category;
    }
    else
    {
        if(visible)
        {
            *visible &= ~all_iris_category;
            *visible |= dc_iris_category;
        }
        if(enable)
        {
            *enable  &= ~all_iris_category;
            *enable  |= dc_iris_category;
        }
    }
    
    return 0;
}

static gint _dependent_check_max_shutter_speed(guint64 *dependent, guint64 *visible, guint64 *enable, guint64 max_shutter_category)
{
    guint64 all_max_shutter_category = 0;

    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_50;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;
    all_max_shutter_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;

    *dependent |= all_max_shutter_category;

    if(!max_shutter_category)
    {
        if(visible) *visible |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;
        if(enable)  *enable &= ~all_max_shutter_category; 
    }
    else
    {
        if(visible)
        {
            *visible &= ~all_max_shutter_category;
            *visible |= max_shutter_category;
        }
        if(enable)
        {
            *enable &= ~all_max_shutter_category;
            *enable |= max_shutter_category; 
        }
    }

    return 0;
}

static gint _dependent_check_base_shutter_speed(guint64 *dependent, guint64 *visible, guint64 *enable, guint64 base_shutter_category)
{
    guint64 all_base_shutter_category = 0;

    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_50;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_60;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_100;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360;
    all_base_shutter_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000;

    *dependent |= all_base_shutter_category;

    if(!base_shutter_category)
    {
        if(visible) *visible |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000;
        if(enable)  *enable  &= ~all_base_shutter_category;
    }
    else 
    {
        if(visible)
        {
            *visible &= ~all_base_shutter_category;
            *visible |= base_shutter_category;    
        }
        if(enable)
        {
            *enable &= ~all_base_shutter_category;
            *enable |= base_shutter_category;     
        }
    }
    
    return 0;
}

#if 0
#endif

static gint _check_base_shutter_speed_support(gint ch)
{   
    guint64 check_category = 0;

    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_100;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360;
    check_category |= NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000;   
    
    if(g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category)
    {
       return 1; 
    }

    return 0;
}


gboolean get_supported_base_shutter_speed_support(gint ch)
{   
    return g_supported_base_shutter_speed[ch];
}
