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
#include "vw_sys_camera_ipcam_dependent_profile_check.h"

#define DBG_LEVEL		1
#define DBG_MODULE		"CAMERA MANAGE"

static ROW_INFO_T *g_row_category[CATEGORY_NR] = {0, };
static ROW_MANAGE_T g_row_manage[GUI_CHANNEL_CNT] = {0, };

static guint64 g_image_visible[GUI_CHANNEL_CNT] = {0, };
static guint64 g_image_enable[GUI_CHANNEL_CNT] = {0, };
static guint64 g_exposure_visible[GUI_CHANNEL_CNT] = {0, };
static guint64 g_exposure_enable[GUI_CHANNEL_CNT] = {0, };

IPCAM_MANAGE_T g_ipcam_manage[GUI_CHANNEL_CNT];
IPCamSetupData g_ipcamData[GUI_CHANNEL_CNT];
IPCamSetupData g_org_ipcamData[GUI_CHANNEL_CNT];

static gint _is_onvif_support(gint ch)
{
    if (!g_ipcam_manage[ch].is_onvif) return -1;

    return 0;
}

static gint _get_data_brightness(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.brightness.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.brightness.value;
    nm->min = g_ipcam_manage[ch].image.profile.brightness.min;
    nm->max = g_ipcam_manage[ch].image.profile.brightness.max;

    return supp;
}

static gint _get_data_contrast(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.contrast.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.contrast.value;
    nm->min = g_ipcam_manage[ch].image.profile.contrast.min;
    nm->max = g_ipcam_manage[ch].image.profile.contrast.max;

    return supp;
}

static gint _get_data_tint(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.tint.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.tint.value;
    nm->min = g_ipcam_manage[ch].image.profile.tint.min;
    nm->max = g_ipcam_manage[ch].image.profile.tint.max;

    return supp;
}

static gint _get_data_color(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.color.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    if (!nm) return supp;

    nm->init = g_ipcam_manage[ch].image.profile.color.value;
    nm->min = g_ipcam_manage[ch].image.profile.color.min;
    nm->max = g_ipcam_manage[ch].image.profile.color.max;

    return supp;
}

static gint _get_data_sharpness(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.sharpness.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    if (!nm) return supp;

    nm->init = g_ipcam_manage[ch].image.profile.sharpness.value;
    nm->min = g_ipcam_manage[ch].image.profile.sharpness.min;
    nm->max = g_ipcam_manage[ch].image.profile.sharpness.max;

    return supp;
}

static gint _get_data_rotate(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i, j;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.mirror[0].category;
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].image.profile.mirror_cnt;

    for (i = 0; i < lt->cnt; i++)
    {   
        lt->val[i] = g_ipcam_manage[ch].image.profile.mirror[i].value;       
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.mirror[i].dependent_category;        
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.mirror[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.mirror[i].enable_category;            
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.mirror[i].caption);

        if (g_ipcam_manage[ch].image.profile.mirror[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_focus_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint visible_cnt = 0;
    gint i, j;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.focus[0].category;
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].image.profile.focus_cnt;

    for (i = 0; i < lt->cnt; i++)
    {    
        lt->val[i] = g_ipcam_manage[ch].image.profile.focus[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.focus[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.focus[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.focus[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.focus[i].caption);

        if (g_ipcam_manage[ch].image.profile.focus[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_one_push(gint ch, guint64 *category)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH) supp = 1;
    }

    *category = NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH;

    return supp;
}

static gint _get_data_home(gint ch, guint64 *category)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME) supp = 1;
    }

    *category = NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME;

    return supp;
}

static gint _get_data_near_limit(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.nearlimit.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.nearlimit.value;
    nm->min = g_ipcam_manage[ch].image.profile.nearlimit.min;
    nm->max = g_ipcam_manage[ch].image.profile.nearlimit.max;

    return supp;
}

static gint _get_data_far_limit(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.farlimit.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.farlimit.value;
    nm->min = g_ipcam_manage[ch].image.profile.farlimit.min;
    nm->max = g_ipcam_manage[ch].image.profile.farlimit.max;

    return supp;
}

static gint _get_data_default_speed(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.defaultspeed.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.defaultspeed.value;
    nm->min = g_ipcam_manage[ch].image.profile.defaultspeed.min;
    nm->max = g_ipcam_manage[ch].image.profile.defaultspeed.max;

    return supp;
}

static gint _get_data_absolute_position(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.abposition.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.abposition.value;
    nm->min = g_ipcam_manage[ch].image.profile.abposition.min;
    nm->max = g_ipcam_manage[ch].image.profile.abposition.max;

    return supp;
}

static gint _get_data_absolute_speed(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.abspeed.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.abspeed.value;
    nm->min = g_ipcam_manage[ch].image.profile.abspeed.min;
    nm->max = g_ipcam_manage[ch].image.profile.abspeed.max;

    return supp;
}

static gint _get_data_relative_distance(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.redistance.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.redistance.value;
    nm->min = g_ipcam_manage[ch].image.profile.redistance.min;
    nm->max = g_ipcam_manage[ch].image.profile.redistance.max;

    return supp;
}

static gint _get_data_relative_speed(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.respeed.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.respeed.value;
    nm->min = g_ipcam_manage[ch].image.profile.respeed.min;
    nm->max = g_ipcam_manage[ch].image.profile.respeed.max;

    return supp;
}

static gint _get_data_continuous_speed(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.cospeed.category;
        if (g_ipcam_manage[ch].image.profile.supported_image & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.cospeed.value;
    nm->min = g_ipcam_manage[ch].image.profile.cospeed.min;
    nm->max = g_ipcam_manage[ch].image.profile.cospeed.max;

    return supp;
}

static gint _get_data_push_focus(gint ch, guint64 *category)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR) supp = 1;
    }

    *category = NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR;

    return supp;
}

static gint _get_data_focus_limit(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.focus_limit[0].category;  
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
        
    }
    if (!lt) return supp;

    lt->cnt = g_ipcam_manage[ch].image.profile.focus_limit_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].image.profile.focus_limit[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.focus_limit[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.focus_limit[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.focus_limit[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.focus_limit[i].caption);

        if (g_ipcam_manage[ch].image.profile.focus_limit[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_ir_correction(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.ir_correction[0].category;  
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
        
    }
    if (!lt) return supp;

    lt->cnt = g_ipcam_manage[ch].image.profile.ir_correction_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].image.profile.ir_correction[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.ir_correction[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.ir_correction[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.ir_correction[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.ir_correction[i].caption);

        if (g_ipcam_manage[ch].image.profile.ir_correction[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_stabilizer(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.stabilizer[0].category;  
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
        
    }
    if (!lt) return supp;

    lt->cnt = g_ipcam_manage[ch].image.profile.stabilizer_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].image.profile.stabilizer[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.stabilizer[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.stabilizer[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.stabilizer[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.stabilizer[i].caption);

        if (g_ipcam_manage[ch].image.profile.stabilizer[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_wb_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.wb[0].category;
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].image.profile.wb_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].image.profile.wb[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.wb[i].dependent_category;    
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.wb[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.wb[i].enable_category;    
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.wb[i].caption);

        if (g_ipcam_manage[ch].image.profile.wb[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_wb_cr_gain(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.crgain.category;
        if (g_ipcam_manage[ch].image.profile.supported_image &  nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.crgain.value;
    nm->min = g_ipcam_manage[ch].image.profile.crgain.min;
    nm->max = g_ipcam_manage[ch].image.profile.crgain.max;

    return supp;
}

static gint _get_data_wb_cb_gain(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].image.done)
    {
        nm->category = g_ipcam_manage[ch].image.profile.cbgain.category;
        if (g_ipcam_manage[ch].image.profile.supported_image &  nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].image.profile.cbgain.value;
    nm->min = g_ipcam_manage[ch].image.profile.cbgain.min;
    nm->max = g_ipcam_manage[ch].image.profile.cbgain.max;

    return supp;
}

static gint _get_data_mwb_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].image.done)
    {
        lt->category = g_ipcam_manage[ch].image.profile.mwb[0].category;
        if (g_ipcam_manage[ch].image.profile.supported_image & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].image.profile.mwb_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].image.profile.mwb[i].value; 
        lt->dependent_category[i] = g_ipcam_manage[ch].image.profile.mwb[i].dependent_category;              
        lt->visible_category[i] = g_ipcam_manage[ch].image.profile.mwb[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].image.profile.mwb[i].enable_category;              
        strcpy(lt->text[i], g_ipcam_manage[ch].image.profile.mwb[i].caption);

        if (g_ipcam_manage[ch].image.profile.mwb[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_ircut_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;
       
    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.ircut[0].category;  
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
        
    }
    if (!lt) return supp;

    lt->cnt = g_ipcam_manage[ch].exposure.profile.ircut_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.ircut[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.ircut[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.ircut[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.ircut[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.ircut[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.ircut[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_ircutm_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;
       
    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.ircutm[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    if (!lt) return supp;

    lt->cnt = g_ipcam_manage[ch].exposure.profile.ircutm_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.ircutm[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.ircutm[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.ircutm[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.ircutm[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.ircutm[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.ircutm[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_dnn_toggle(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.dnn_toggle[0].category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure& lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.dnn_toggle_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].dependent_category;              
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].enable_category;              
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_ircut_dn_mode(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.dnn_schedule.category;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
        
    return supp;
}

static gint _get_data_adaptive_ir(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.adaptive_ir[0].category;
        if(g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.adaptive_ir_cnt;

    for(i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].caption);

        if(g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].selected)
            lt->init = i;
    }
    return supp;
}

static gint _get_data_dnn_sense_dton(gint ch, NM_TYPE_T * nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->value = g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.max;
    nm->dependent_category = g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.dependent_category;
    nm->difference = g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.difference;

    return supp;
}

static gint _get_data_dnn_sense_ntod(gint ch, NM_TYPE_T * nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->value = g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.max;
    nm->dependent_category = g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.dependent_category;
    nm->difference = g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.difference;
    
    return supp;
}

static gint _get_data_illumination_control(gint ch, LT_TYPE_T *lt)
{
    gint supp = 1;
    gint i;

    lt->cnt = NF_IPCAM_COLORVU_CTRL_NR - 1;

    for(i = 0; i < lt->cnt; i++)
    {
        switch(i)
        {
            case 0:
            lt->val[i] = NF_IPCAM_COLORVU_CTRL_MANUAL;
            strcpy(lt->text[i], "MANUAL");
            break;
            case 1:
            lt->val[i] = NF_IPCAM_COLORVU_CTRL_AUTO;
            strcpy(lt->text[i], "AUTO");
            break;
        }

        if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_MANUAL) lt->init = 0;
        else if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_AUTO) lt->init = 1;

    }
    g_message("%s(%d) => %d", __FUNCTION__, __LINE__, g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value);
    return supp;
}

static gint _get_data_illumination_level(gint ch, NM_TYPE_T * nm)
{
    gint supp = 1;

    nm->category = g_ipcam_manage[ch].exposure.profile.colorvu_level.category;
    nm->value = g_ipcam_manage[ch].exposure.profile.colorvu_level.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.colorvu_level.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.colorvu_level.max;
    nm->dependent_category = g_ipcam_manage[ch].exposure.profile.colorvu_level.dependent_category;
    nm->difference =  g_ipcam_manage[ch].exposure.profile.colorvu_level.difference;
    
    return supp;
}

static gint _get_data_exposure_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.mode[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.mode_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.mode[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.mode[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.mode[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.mode[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.mode[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.mode[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_priority(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.priority[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.priority_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.priority[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.priority[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.priority[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.priority[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.priority[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.priority[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_blc_control(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.blc[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.blc_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.blc[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.blc[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.blc[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.blc[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.blc[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.blc[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_blc_level(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.blclevel.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].exposure.profile.blclevel.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.blclevel.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.blclevel.max;

    return supp;
}

static gint _get_data_exposure_defog(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.defog[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.defog_cnt;
    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.defog[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.defog[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.defog[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.defog[i].enable_category;   
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.defog[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.defog[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_hlc(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.hlc[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.hlc_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.hlc[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.hlc[i].dependent_category;  
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.hlc[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.hlc[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.hlc[i].caption);
        
        if (g_ipcam_manage[ch].exposure.profile.hlc[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_antiflicker_m(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].value;
        
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_antiflicker(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_ANTI_FLICKER;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.antiflicker_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.antiflicker[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.antiflicker[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.antiflicker[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_antiflicker_m_off(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off_cnt;

    for (i =0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].selected)
                lt->init = i;  
    }

    return supp;
}
static gint _get_data_exposure_antiflicker_a_off(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].selected)
                lt->init = i;  
    }

    return supp;
}
static gint _get_data_exposure_max_shutter_motion_off_on(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].selected)
            lt->init = i;        
    }

    return supp;
}
static gint _get_data_exposure_max_shutter_auto_off_off(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].caption);
        
        if (g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].selected)
            lt->init = i;        
    }

    return supp;
}
static gint _get_data_exposure_max_shutter_auto_off_on_tv(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].selected)
            lt->init = i;        
    }

    return supp;    
}
static gint _get_data_exposure_max_shutter_auto_off_on(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_max_shutter_60(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_60_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_max_shutter_50(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_50;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_50_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].selected)
            lt->init = i;              
    }

    return supp;
}

static gint _get_data_exposure_max_shutter_off(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_off_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].selected)
            lt->init = i;              
    }

    return supp;
}

static gint _get_data_exposure_max_shutter_motion_off(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF;
        if( g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_cnt;

    for( i = 0; i< lt->cnt; i++)
    {

        lt->val[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].caption);
        
        if( g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_100(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_100;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_100_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_120(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_120_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_100_300(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_100_300_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_100_5000(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {        
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].selected)
            lt->init = i;
    }
    
    return supp;
}

static gint _get_data_exposure_base_shutter_120_360(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_120_360_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_120_5000(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_120_262(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_120_262_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_30_262(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_30_262_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_25_100(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_25_100_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_25_300(gint ch, LT_TYPE_T *lt)

{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_25_300_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_25_5000(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_30_120(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_30_120_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_base_shutter_30_360(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_30_360_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].selected)
            lt->init = i;
    }

    return supp;
}


static gint _get_data_exposure_base_shutter_30_5000(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000_cnt;

    for( i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].enable_category;
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].selected)
            lt->init = i;
    }

    return supp;
}

static gint _get_data_exposure_minetime(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.minetime.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].exposure.profile.minetime.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.minetime.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.minetime.max;

    return supp;
}

static gint _get_data_exposure_maxetime(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.maxetime.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
    
    nm->init = g_ipcam_manage[ch].exposure.profile.maxetime.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.maxetime.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.maxetime.max;

    return supp;
}

static gint _get_data_exposure_etime(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.etime.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
    
    nm->init = g_ipcam_manage[ch].exposure.profile.etime.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.etime.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.etime.max;

    return supp;
}

static gint _get_data_exposure_mingain(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.mingain.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].exposure.profile.mingain.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.mingain.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.mingain.max;

    return supp;
}

static gint _get_data_exposure_maxgain(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.maxgain.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
    
    nm->init = g_ipcam_manage[ch].exposure.profile.maxgain.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.maxgain.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.maxgain.max;

    return supp;
}

static gint _get_data_exposure_gain(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.gain.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
    
    nm->init = g_ipcam_manage[ch].exposure.profile.gain.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.gain.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.gain.max;

    return supp;
}

static gint _get_data_exposure_miniris(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.miniris.category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].exposure.profile.miniris.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.miniris.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.miniris.max;

    return supp;
}

static gint _get_data_exposure_maxiris(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.maxiris.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
    
    nm->init = g_ipcam_manage[ch].exposure.profile.maxiris.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.maxiris.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.maxiris.max;

    return supp;
}

static gint _get_data_exposure_iris(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.iris.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }
    
    nm->init = g_ipcam_manage[ch].exposure.profile.iris.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.iris.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.iris.max;

    return supp;
}

static gint _get_data_exposure_slowshutter(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.slowshutter[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.slowshutter_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.slowshutter[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.slowshutter[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.slowshutter[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.slowshutter[i].enable_category;             
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.slowshutter[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.slowshutter[i].selected)
            lt->init = i;        
    }

    return supp;
}


static gint _get_data_exposure_maxagc(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.maxagc[0].category;    
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.maxagc_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.maxagc[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.maxagc[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.maxagc[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.maxagc[i].enable_category;               
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.maxagc[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.maxagc[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_dciris(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_DCIRIS;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.dc_iris_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.dc_iris[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.dc_iris[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.dc_iris[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.dc_iris[i].enable_category;              
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.dc_iris[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.dc_iris[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_exposure_dciris_motion(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i; 

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = NF_IPCAM_EXPOSURE_DCIRIS_MOTION;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.dc_iris_motion_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].dependent_category;
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].enable_category;              
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].selected)
            lt->init = i;        
    }

    return supp;
    
}

static gint _get_data_exposure_dciris_calibration(gint ch, guint64 *category)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DC_IRIS_CAL ) supp = 1;
    }

    *category = NF_IPCAM_EXPOSURE_ONVIF_DC_IRIS_CAL ;

    return supp;
}

static gint _get_data_exposure_dnr(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if(g_ipcam_manage[ch].exposure.done)
    {                 
        lt->category = g_ipcam_manage[ch].exposure.profile.dnr[0].category;
        if(g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp =1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.dnr_cnt;

    for (i = 0; i < lt->cnt; i++)
    {
   
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.dnr[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.dnr[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.dnr[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.dnr[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.dnr[i].caption);

        if (g_ipcam_manage[ch].exposure.profile.dnr[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_wdr_mode(gint ch, LT_TYPE_T *lt)
{
    gint supp = 0;
    gint i;

    if (g_ipcam_manage[ch].exposure.done)
    {
        lt->category = g_ipcam_manage[ch].exposure.profile.wdr[0].category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & lt->category) supp = 1;
    }

    lt->cnt = g_ipcam_manage[ch].exposure.profile.wdr_cnt;



    for (i = 0; i < lt->cnt; i++)
    {
        lt->val[i] = g_ipcam_manage[ch].exposure.profile.wdr[i].value;
        lt->dependent_category[i] = g_ipcam_manage[ch].exposure.profile.wdr[i].dependent_category;      
        lt->visible_category[i] = g_ipcam_manage[ch].exposure.profile.wdr[i].visible_category;
        lt->enable_category[i] = g_ipcam_manage[ch].exposure.profile.wdr[i].enable_category;      
        strcpy(lt->text[i], g_ipcam_manage[ch].exposure.profile.wdr[i].caption);
   

        if (g_ipcam_manage[ch].exposure.profile.wdr[i].selected)
            lt->init = i;        
    }

    return supp;
}

static gint _get_data_wdr_level(gint ch, NM_TYPE_T *nm)
{
    gint supp = 0;

    if (g_ipcam_manage[ch].exposure.done)
    {
        nm->category = g_ipcam_manage[ch].exposure.profile.wdrlevel.category;
        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & nm->category) supp = 1;
    }

    nm->init = g_ipcam_manage[ch].exposure.profile.wdrlevel.value;
    nm->min = g_ipcam_manage[ch].exposure.profile.wdrlevel.min;
    nm->max = g_ipcam_manage[ch].exposure.profile.wdrlevel.max;

    return supp;
}

static gint _get_visible_image_category(gint ch, guint64 *visible)
{
    LT_TYPE_T lt;
    gint support;

    *visible = g_ipcam_manage[ch].image.profile.supported_image;

    g_message("[IPCAM_MANAGE] %s, %d, ch:%d, visible:%08llX", __FUNCTION__, __LINE__, ch, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_rotate(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_focus_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_focus_limit(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ir_correction(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_stabilizer(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);


    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_wb_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_mwb_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    return 0;
}

static gint _get_enable_image_category(gint ch, guint64 *enable)
{
    LT_TYPE_T lt;
    gint support;

    *enable = g_ipcam_manage[ch].image.profile.supported_image;

    g_message("[IPCAM_MANAGE] %s, %d, ch:%d, enable:%08llX", __FUNCTION__, __LINE__, ch, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_rotate(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_focus_mode(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_focus_limit(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ir_correction(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_stabilizer(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_wb_mode(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_mwb_mode(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *enable);

    return 0;
}

static gint _get_visible_exposure_category(gint ch, guint64 *visible)
{
    LT_TYPE_T lt;
    gint support;

    *visible = g_ipcam_manage[ch].exposure.profile.supported_exposure;

    g_message("[IPCAM_MANAGE] %s, %d, ch:%d, visible:%08llX", __FUNCTION__, __LINE__, ch, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);
    
    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_priority(ch, &lt);  
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_wdr_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_slowshutter(ch, &lt);  
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_maxagc(ch, &lt);  
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_dc_iris_category_manage(ch, &lt, visible);  
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

        memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_blc_control(ch, &lt);  
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_antiflicker_category_manage(ch, &lt, visible);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);    

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_max_shutter_speed_category_manage(ch, &lt, visible);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_base_shutter_speed_category_manage(ch, &lt, visible);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_dnr(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);
    
//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ircut_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ircutm_mode(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_dnn_toggle(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_adaptive_ir(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_hlc(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_defog(ch, &lt);
    if (support) *visible &= ~(lt.dependent_category[lt.init] & ~lt.visible_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.visible_category[lt.init]);
//   g_message("[IPCAM_MANAGE] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);
    
    return 0;
}

static gint _get_enable_exposure_category(gint ch, guint64 *enable)
{
    LT_TYPE_T lt;
    gint support;

    *enable = g_ipcam_manage[ch].exposure.profile.supported_exposure;

    g_message("[IPCAM_MANAGE] %s, %d, ch:%d, enable:%08llX", __FUNCTION__, __LINE__, ch, *enable);
    
    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_mode(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_priority(ch, &lt);  
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_wdr_mode(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_slowshutter(ch, &lt);  
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_maxagc(ch, &lt);  
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_dc_iris_category_manage(ch, &lt, enable);   
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

        memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_blc_control(ch, &lt);  
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_antiflicker_category_manage(ch, &lt, enable);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);    

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_max_shutter_speed_category_manage(ch, &lt, enable);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = get_base_shutter_speed_category_manage(ch, &lt, enable);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_dnr(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_adaptive_ir(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_hlc(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_defog(ch, &lt);
    if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);
    
//    g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//    g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    if(g_ipcamData[ch].exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT &&
       g_ipcamData[ch].exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT_X10)
    {
        memset(&lt, 0x00, sizeof(LT_TYPE_T));
        support = _get_data_ircutm_mode(ch, &lt);
        if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//        g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//        g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);
    }
    else
    {
        memset(&lt, 0x00, sizeof(LT_TYPE_T));
        support = _get_data_ircut_mode(ch, &lt);
        if (support) *enable &= ~(lt.dependent_category[lt.init] & ~lt.enable_category[lt.init]);

//        g_message("[IPCAM_MANAGE] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, lt.dependent_category[lt.init], lt.enable_category[lt.init]);
//        g_message("[IPCAM_MANAGE] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);
    }
    
    return 0;
}

static gint _sync_db_data(gint ch)
{
    IPCamSetupData tmpData;
    LT_TYPE_T lt;
    NM_TYPE_T nm;

    if (!g_ipcam_manage[ch].image.done) return 0;
    if (!g_ipcam_manage[ch].exposure.done) return 0;
   
	memset(&tmpData, 0x00, sizeof(IPCamSetupData));

	DAL_get_IPCamSetup_data(&tmpData, ch);	

	memset(&lt, 0x00, sizeof(LT_TYPE_T));
	if (_get_data_exposure_dnr( ch, &lt)) tmpData.dnr = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_brightness(ch, &nm)) tmpData.bright = nm.init;
    
    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_contrast(ch, &nm)) tmpData.contrast = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_tint(ch, &nm)) tmpData.tint = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_color(ch, &nm)) tmpData.color = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_sharpness(ch, &nm)) tmpData.sharpness = nm.init;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_rotate(ch, &lt)) tmpData.rotate = lt.val[lt.init];

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_focus_mode(ch, &lt)) tmpData.focus_mode = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_default_speed(ch, &nm)) tmpData.focus_default_speed = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_near_limit(ch, &nm)) tmpData.focus_near_limit = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_far_limit(ch, &nm)) tmpData.focus_far_limit = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_absolute_position(ch, &nm)) tmpData.focus_abposition = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_absolute_speed(ch, &nm)) tmpData.focus_abspeed = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_relative_distance(ch, &nm)) tmpData.focus_redistance = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_relative_speed(ch, &nm)) tmpData.focus_respeed = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_continuous_speed(ch, &nm)) tmpData.focus_cospeed = nm.init;    

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_wb_mode(ch, &lt)) tmpData.wb_mode = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_wb_cr_gain(ch, &nm)) tmpData.wb_crgain = nm.init;    

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_wb_cb_gain(ch, &nm)) tmpData.wb_cbgain = nm.init;    

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_mwb_mode(ch, &lt)) tmpData.mwb_mode = lt.val[lt.init];

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_wdr_mode(ch, &lt)) tmpData.wdr_mode = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_wdr_level(ch, &nm)) tmpData.wdr_level = nm.init;    

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_mode(ch, &lt)) tmpData.exposure_mode = lt.val[lt.init]; 

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (tmpData.exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT || 
        tmpData.exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT_X10)
    {
        if (_get_data_ircutm_mode(ch, &lt))
        {
            tmpData.day_night_mode = lt.val[lt.init];
        }
    }
    else
    {
        if (_get_data_ircut_mode(ch, &lt))
        {
            tmpData.day_night_mode = lt.val[lt.init];
        }
    }

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_dnn_toggle(ch, &lt)) tmpData.day_night_duration = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_ircut_dn_mode(ch, &nm))
    {
        tmpData.dnn_start_hour = g_ipcam_manage[ch].exposure.profile.dnn_schedule.start.hour;
        tmpData.dnn_start_min = g_ipcam_manage[ch].exposure.profile.dnn_schedule.start.min;
        tmpData.dnn_end_hour = g_ipcam_manage[ch].exposure.profile.dnn_schedule.end.hour;
        tmpData.dnn_end_min = g_ipcam_manage[ch].exposure.profile.dnn_schedule.end.min;
    }

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_adaptive_ir(ch, &lt)) tmpData.adaptive_ir = lt.val[lt.init]; 

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_dnn_sense_dton(ch, &nm)) tmpData.dnn_sense_dton = nm.value;
    
    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_dnn_sense_ntod(ch, &nm)) tmpData.dnn_sense_ntod = nm.value;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_priority(ch, &lt)) tmpData.exposure_priority = lt.val[lt.init];

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_blc_control(ch, &lt)) tmpData.blc_control = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_blc_level(ch, &nm)) tmpData.blc_level = nm.init;    

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_hlc(ch, &lt)) tmpData.hlc = lt.val[lt.init]; 

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_defog(ch, &lt)) tmpData.defog = lt.val[lt.init]; 

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_etime(ch, &nm)) tmpData.etime = nm.init;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_minetime(ch, &nm)) tmpData.min_etime = nm.init;   

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_maxetime(ch, &nm)) tmpData.max_etime = nm.init;    

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_slowshutter(ch, &lt)) tmpData.slow_shutter = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_gain(ch, &nm)) tmpData.gain = nm.init;    

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_mingain(ch, &nm)) tmpData.min_gain = nm.init;    

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_maxgain(ch, &nm)) tmpData.max_gain = nm.init;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (_get_data_exposure_maxagc(ch, &lt)) tmpData.max_agc = lt.val[lt.init];

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_iris(ch, &nm)) tmpData.iris = nm.init;    

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_miniris(ch, &nm)) tmpData.min_iris = nm.init;    

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    if (_get_data_exposure_maxiris(ch, &nm)) tmpData.max_iris = nm.init;        

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (get_dc_iris_category_manage_init_db_set(ch, &lt, &tmpData))	tmpData.iris_control = lt.val[lt.init];    

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (get_antiflicker_category_manage_int_db_set(ch, &lt, &tmpData)) tmpData.antiflicker= lt.val[lt.init];
    
    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (get_max_shutter_speed_category_init_db_set(ch, &lt, &tmpData)) tmpData.max_shutter = lt.val[lt.init];
  
    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    if (get_base_shutter_speed_category_manage_init_db_set(ch, &lt, &tmpData)) tmpData.base_shutter = lt.val[lt.init];

    if (memcmp(&tmpData, &g_ipcamData[ch], sizeof(IPCamSetupData)))
    {
        g_memmove(&g_ipcamData[ch], &tmpData, sizeof(IPCamSetupData));
        return -1;
    }

    return 0;
}

static gint _set_row_info_image_category(ROW_INFO_T *row_info, guint64 category)
{
    row_info->key = category;
    return 0;
}

static gint _set_row_info_exposure_category(ROW_INFO_T *row_info, guint64 category)
{
    row_info->key = category;
    return 0;
}

static OBJECT_INFO_T *_set_col_info(ROW_INFO_T *row_info, guint64 category)
{
    COL_INFO_T *col_info;
    OBJECT_INFO_T *obj_info;
    gint col_cnt = row_info->col_cnt;

    col_info = imalloc(sizeof(COL_INFO_T));
    col_info->key = category;
    
    obj_info = imalloc(sizeof(OBJECT_INFO_T));
    col_info->obj_info = obj_info;

    row_info->col_info[col_cnt] = col_info;        
    row_info->col_cnt++;    

    return obj_info;
}

static gint _unset_col_info(COL_INFO_T *col_info)
{
    ifree(col_info);    
    col_info = 0;
    return 0;
}

static gint _unset_obj_info(OBJECT_INFO_T *obj_info)
{
    ifree(obj_info);    
    obj_info = 0;
    return 0;
}

static gint _set_obj_info_enable(OBJECT_INFO_T *obj_info, gint enable)
{
    obj_info->enable = enable;    
    return 0;
}

static gint _set_obj_info_width(OBJECT_INFO_T *obj_info, gint width)
{
    obj_info->width = width;    
    return 0;
}

static gint _set_obj_info_handler(OBJECT_INFO_T *obj_info, gpointer handler)
{
    obj_info->handler = handler;    
    return 0;
}

static gint _set_data_label_category(OBJECT_INFO_T *obj_info, gchar *img_name, gchar *text)
{
    obj_info->label.data.category.img = imalloc(sizeof(gchar)*TEXT_MAX_LEN);
    obj_info->label.data.category.text = imalloc(sizeof(gchar)*TEXT_MAX_LEN); 

    obj_info->obj_type = OBJ_LABEL;
    obj_info->label.data.type = LABEL_CATEGORY;
    strcpy(obj_info->label.data.category.img, img_name);    
    strcpy(obj_info->label.data.category.text, text);
    
    return 0;
}

static gint _set_data_label_title(OBJECT_INFO_T *obj_info, gchar *text)
{
    obj_info->label.data.title.text = imalloc(sizeof(gchar)*TEXT_MAX_LEN);

    obj_info->obj_type = OBJ_LABEL;
    obj_info->label.data.type = LABEL_TITLE;    
    strcpy(obj_info->label.data.title.text, text);
   
    return 0;
}

static gint _set_data_label_number(OBJECT_INFO_T *obj_info, gint init, gint min, gint max)
{
    obj_info->obj_type = OBJ_LABEL;
    obj_info->label.data.type = LABEL_NUMBER;
    obj_info->label.data.number.init = init;
    obj_info->label.data.number.min = min;
    obj_info->label.data.number.max = max;    

    return 0;
}

static gint _set_data_label_letter(OBJECT_INFO_T *obj_info, gchar *text)
{
    obj_info->label.data.letter.text = imalloc(sizeof(gchar)*TEXT_MAX_LEN);

    obj_info->obj_type = OBJ_LABEL;
    obj_info->label.data.type = LABEL_LETTER;
    strcpy(obj_info->label.data.letter.text, text);
   
    return 0;
}

static gint _set_data_label_legend(OBJECT_INFO_T *obj_info, gint min, gint max)
{
    obj_info->label.data.legend.text = imalloc(sizeof(gchar)*TEXT_MAX_LEN);

    obj_info->obj_type = OBJ_LABEL;
    obj_info->label.data.type = LABEL_LEGEND;
    g_sprintf(obj_info->label.data.legend.text, "[%d .. %d]", min, max);
        
    return 0;
}

static gint _set_data_label_legend_string(OBJECT_INFO_T * obj_info, gchar *str1, gchar *str2)
{
    obj_info->label.data.letter.text = imalloc(sizeof(gchar)*TEXT_MAX_LEN);

    obj_info->obj_type = OBJ_LABEL;
    obj_info->label.data.type = LABEL_LEGEND; 
    g_sprintf(obj_info->label.data.legend.text, "[%s : %s]", str1, str2);
    
    return 0;
}

static gint _unset_data_label(OBJECT_INFO_T *obj_info)
{
    if (obj_info->label.data.type == LABEL_CATEGORY)
    {
        ifree(obj_info->label.data.category.img);
        ifree(obj_info->label.data.category.text);        

        obj_info->label.data.category.img = 0;
        obj_info->label.data.category.text = 0;        
    }
    else if (obj_info->label.data.type == LABEL_TITLE)
    {
        ifree(obj_info->label.data.title.text);
        obj_info->label.data.title.text = 0;
    }
    else if (obj_info->label.data.type == LABEL_NUMBER)
    {

    }
    else if (obj_info->label.data.type == LABEL_LETTER)
    {
        ifree(obj_info->label.data.letter.text);
        obj_info->label.data.letter.text = 0;
    }
    else if (obj_info->label.data.type == LABEL_LEGEND)
    {
        ifree(obj_info->label.data.legend.text);
        obj_info->label.data.legend.text = 0;
    }
 
    return 0;
}

static gint _set_data_spin_number(OBJECT_INFO_T *obj_info, gint init, gint min, gint max, gint step)
{   
    obj_info->obj_type = OBJ_SPIN;
    obj_info->spin.data.type = SPIN_NUMBER;
    obj_info->spin.data.number.init = init;
    obj_info->spin.data.number.min = min;
    obj_info->spin.data.number.max = max;
    obj_info->spin.data.number.step = step;

    return 0;
}

static gint _set_data_spin_letter(OBJECT_INFO_T *obj_info, gint init)
{   
    obj_info->obj_type = OBJ_SPIN;
    obj_info->spin.data.type = SPIN_LETTER;
    obj_info->spin.data.letter.init = init;

    return 0;
}

static gint _append_spin_letter_data(SPIN_DATA_T *data, guint64 val, gchar *text)
{
    gint cnt = data->letter.cnt;

    if (data->type != SPIN_LETTER) return -1;

    data->letter.text[cnt] = imalloc(sizeof(gchar)*TEXT_MAX_LEN);

    data->letter.val = val;
    strcpy(data->letter.text[cnt], text);    
    data->letter.cnt++;

    return 0;
}

static gint _unset_data_spin(OBJECT_INFO_T *obj_info)
{
    gint i;

    if (obj_info->spin.data.type == SPIN_NUMBER)
    {
    
    }
    else if (obj_info->spin.data.type == SPIN_LETTER)
    {
        for (i = 0; i < obj_info->spin.data.letter.cnt; i++)
        {
            ifree(obj_info->spin.data.letter.text[i]);
            obj_info->spin.data.letter.text[i] = 0;
        }
    }
 
    return 0;
}

static gint _set_data_combo_number(OBJECT_INFO_T *obj_info, gint init, gint min, gint max, gint step)
{
    obj_info->obj_type = OBJ_COMBO;
    obj_info->combo.data.type = COMBO_NUMBER;
    obj_info->combo.data.number.init = init;
    obj_info->combo.data.number.min = min;
    obj_info->combo.data.number.max = max;
    obj_info->combo.data.number.step = step;

    return 0;
}

static gint _set_data_combo_letter(OBJECT_INFO_T *obj_info, gint init)
{
    obj_info->obj_type = OBJ_COMBO;
    obj_info->combo.data.type = COMBO_LETTER;
    obj_info->combo.data.letter.init = init;
   
    return 0;
}

static gint _append_combo_letter_data(COMBO_DATA_T *data, guint64 val, gchar *text)
{
    gint cnt = data->letter.cnt;

    if (data->type != COMBO_LETTER) return -1;

    data->letter.text[cnt] = imalloc(sizeof(gchar)*TEXT_MAX_LEN);
    data->letter.val = val;
    strcpy(data->letter.text[cnt], text);    
    data->letter.cnt++;

    return 0;
}

static gint _unset_data_combo(OBJECT_INFO_T *obj_info)
{
    gint i;

    if (obj_info->combo.data.type == COMBO_NUMBER)
    {
    
    }
    else if (obj_info->combo.data.type == COMBO_LETTER)
    {
        for (i = 0; i < obj_info->combo.data.letter.cnt; i++)
        {
            ifree(obj_info->combo.data.letter.text[i]);
            obj_info->combo.data.letter.text[i] = 0;
        }
    }
 
    return 0;
}

static gint _set_data_slider_normal(OBJECT_INFO_T *obj_info, gint init, gint min, gint max, gint cnt)
{    
    obj_info->obj_type = OBJ_SLIDER;
    obj_info->slider.data.type = SLIDER_NORMAL;
    obj_info->slider.data.normal.init = init;
    obj_info->slider.data.normal.min = min;
    obj_info->slider.data.normal.max = max;
    obj_info->slider.data.normal.cnt = cnt;
       
    return 0;
}

static gint _unset_data_slider(OBJECT_INFO_T *obj_info)
{
    gint i;

    if (obj_info->slider.data.type == SLIDER_NORMAL)
    {
    
    }
 
    return 0;
}

static gint _set_data_button_letter(OBJECT_INFO_T *obj_info, gchar *text)
{
    obj_info->button.data.letter.text = imalloc(sizeof(gchar)*TEXT_MAX_LEN);

    obj_info->obj_type = OBJ_BUTTON;
    obj_info->button.data.type = BUTTON_LETTER;
    strcpy(obj_info->button.data.letter.text, text);
    
    return 0;
}

static gint _set_data_button_image(OBJECT_INFO_T *obj_info, GdkPixbuf **img)
{
    gint i;

    obj_info->obj_type = OBJ_BUTTON;
    obj_info->button.data.type = BUTTON_IMAGE;

    for (i = 0; i < 4; i++)
        obj_info->button.data.image.name[i] = *(img+i);
    
    return 0;
}

static gint _unset_data_button(OBJECT_INFO_T *obj_info)
{
    gint i;

    if (obj_info->button.data.type == BUTTON_LETTER)
    {
        ifree(obj_info->button.data.letter.text);
        obj_info->button.data.letter.text = 0;
    }
    else if (obj_info->button.data.type == BUTTON_IMAGE)
    {

    }
 
    return 0;
}


// image set

static ROW_INFO_T *_make_row_info_brightness(gint ch)
{
    ROW_INFO_T *row_info;    
    OBJECT_INFO_T *obj_info;    
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_brightness(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BRIGHTNESS");

    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_brightness_slider_event_handler);   
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_obj_info_handler(obj_info, _post_brightness_spin_event_handler);   
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 100);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
  
    return row_info;
}

static ROW_INFO_T *_make_row_info_contrast(gint ch)
{
    ROW_INFO_T *row_info;    
    OBJECT_INFO_T *obj_info;
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_contrast(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
      
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "CONTRAST");

    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_contrast_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_obj_info_handler(obj_info, _post_contrast_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 100);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_tint(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_tint(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
      
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "TINT");

    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_tint_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_obj_info_handler(obj_info, _post_tint_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 100);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_color(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_color(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
        
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "COLOR");

    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_color_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_obj_info_handler(obj_info, _post_color_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 100);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_sharpness(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_sharpness(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
       
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "SHARPNESS");

    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_sharpness_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_obj_info_handler(obj_info, _post_sharpness_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 100);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}


// rotate

static ROW_INFO_T *_make_row_info_rotate(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;   
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_rotate(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);
    
    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MIRROR IMAGE");

    obj_info = _set_col_info(row_info, lt.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_rotate_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
        
    return row_info;
}


// focus

static ROW_INFO_T *_make_row_info_focus_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "FOCUS");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;   
    LT_TYPE_T lt;
    guint64 category;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_focus_mode(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "AUTO FOCUS");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_mode_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);   

    support = _get_data_one_push(ch, &category);

    obj_info = _set_col_info(row_info, category);   
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 150);
    _set_obj_info_handler(obj_info, _post_one_push_button_event_handler);
    _set_data_button_letter(obj_info, "ONE PUSH");

    support = _get_data_home(ch, &category);

    obj_info = _set_col_info(row_info, category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_home_button_event_handler);
    _set_data_button_letter(obj_info, "HOME");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_near_limit(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_near_limit(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
          
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "NEAR LIMIT");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_near_limit_slider_event_handler);    
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_near_limit_spin_event_handler);    
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
   
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_far_limit(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_far_limit(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "FAR LIMIT");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_far_limit_slider_event_handler);        
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_far_limit_spin_event_handler);        
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_default_speed(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_default_speed(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "DEFAULT SPEED");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_default_speed_slider_event_handler);            
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_default_speed_spin_event_handler);            
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_absolute_position(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_absolute_position(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "POSITION");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_absolute_position_slider_event_handler);                    
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_absolute_position_spin_event_handler);                
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_absolute_speed(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_absolute_speed(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "SPEED");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_absolute_speed_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_absolute_speed_spin_event_handler);                    
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_relative_distance(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_relative_distance(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "DISTANCE");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_relative_distance_slider_event_handler);    
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_relative_distance_spin_event_handler);    
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_relative_speed(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_relative_speed(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "SPEED");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_relative_speed_slider_event_handler);        
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_relative_speed_spin_event_handler);        
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_continuous_nearfar(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;
    guint64 category;
    GdkPixbuf *img[NFOBJECT_STATE_COUNT];
    gint width, height;

    support = _get_data_push_focus(ch, &category);

    row_info = imalloc(sizeof(ROW_INFO_T));
  
    _set_row_info_image_category(row_info, category);

    obj_info = _set_col_info(row_info, category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "FOCUS");

	img[0] = nfui_get_image_from_file(IMG_N_BT_NEAR, NULL);
	img[1] = nfui_get_image_from_file(IMG_O_BT_NEAR, NULL);
	img[2] = nfui_get_image_from_file(IMG_P_BT_NEAR, NULL);
	img[3] = nfui_get_image_from_file(IMG_D_BT_NEAR, NULL);

    nfui_get_pixbuf_size(img[0], &width, &height);
	
    obj_info = _set_col_info(row_info, category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, width);
    _set_obj_info_handler(obj_info, _post_continuous_near_button_event_handler);            
    _set_data_button_image(obj_info, img);

	img[0] = nfui_get_image_from_file(IMG_N_BT_FAR, NULL);
	img[1] = nfui_get_image_from_file(IMG_O_BT_FAR, NULL);
	img[2] = nfui_get_image_from_file(IMG_P_BT_FAR, NULL);
	img[3] = nfui_get_image_from_file(IMG_D_BT_FAR, NULL);

    nfui_get_pixbuf_size(img[0], &width, &height);
	
    obj_info = _set_col_info(row_info, category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, width);
    _set_obj_info_handler(obj_info, _post_continuous_far_button_event_handler);            
    _set_data_button_image(obj_info, img);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_limit(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_focus_limit(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "FOCUS NEAR LIMIT");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_limit_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_ir_correction(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ir_correction(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "IR CORRECTION");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_ir_correction_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_stabilizer(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_stabilizer(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "STABILIZER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_stabilizer_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_focus_continuous_speed(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_continuous_speed(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "SPEED");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_focus_continuous_speed_slider_event_handler);                
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_focus_continuous_speed_spin_event_handler);            
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);  
    
    return row_info;
}

// white balance

static ROW_INFO_T *_make_row_info_white_balance_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "WHITE BALANCE");
   
    return row_info;
}

static ROW_INFO_T *_make_row_info_white_balance_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;   
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_wb_mode(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);
    
    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "WHITE BALANCE");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_white_balance_mode_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);   
        
    return row_info;
}

static ROW_INFO_T *_make_row_info_white_balance_cr_gain(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_wb_cr_gain(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "CR GAIN");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_cr_gain_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_cr_gain_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_white_balance_cb_gain(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_wb_cb_gain(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "CB GAIN");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_cb_gain_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_cb_gain_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_white_balance_mwb_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;   
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_mwb_mode(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MWB MODE");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 250);
    _set_obj_info_handler(obj_info, _post_mwb_mode_combo_event_handler);    
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);   

    return row_info;
}

 
// wide dynamic range

static ROW_INFO_T *_make_row_info_wide_dynamic_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "WIDE DYNAMIC RANGE");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_wide_dynamic_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;   
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_wdr_mode(ch, &lt);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "WIDE DYNAMIC MODE");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_wide_dynamic_mode_combo_event_handler);    
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
         _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_wide_dynamic_level(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_wdr_level(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_image_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "LEVEL");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_wide_dynamic_level_slider_event_handler);        
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_wide_dynamic_level_spin_event_handler);            
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);

    return row_info;
}

// day night

static ROW_INFO_T *_make_row_info_day_night_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "DAY(COLOR) / NIGHT(BW)");
    
    return row_info;
}

// colorvu
static ROW_INFO_T *_make_row_info_colorvu_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "ILLUMINATION CONTROL");

    return row_info;
}

static ROW_INFO_T *_make_row_info_ircut_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ircut_mode(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);
    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu)
    {
    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "IR CUT FILTER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_ircut_mode_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    }
    else
    {
        obj_info = _set_col_info(row_info, lt.category);
        _set_obj_info_enable(obj_info, support);    
        _set_obj_info_width(obj_info, 280);
        _set_data_label_title(obj_info, "ILLUMINATION MODE");
    
        obj_info = _set_col_info(row_info, lt.category);
        _set_obj_info_enable(obj_info, support);    
        _set_obj_info_width(obj_info, 220);
        _set_obj_info_handler(obj_info, _post_ircut_mode_combo_event_handler);        
        _set_data_combo_letter(obj_info, lt.init);

        _append_combo_letter_data(&obj_info->combo.data, lt.val[0], "AUTO");
        _append_combo_letter_data(&obj_info->combo.data, lt.val[1], "OFF");
        _append_combo_letter_data(&obj_info->combo.data, lt.val[2], "ON");
        _append_combo_letter_data(&obj_info->combo.data, lt.val[3], "SCHEDULE");
    }
    return row_info;
}

static ROW_INFO_T *_make_row_info_ircutm_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_ircutm_mode(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);
    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu)
    {
    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "IR CUT FILTER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_ircutm_mode_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    }
    else
    {
        obj_info = _set_col_info(row_info, lt.category);
        _set_obj_info_enable(obj_info, support);    
        _set_obj_info_width(obj_info, 280);
        _set_data_label_title(obj_info, "ILLUMINATION MODE");

        obj_info = _set_col_info(row_info, lt.category);
        _set_obj_info_enable(obj_info, support);    
        _set_obj_info_width(obj_info, 220);
        _set_obj_info_handler(obj_info, _post_ircutm_mode_combo_event_handler);        
        _set_data_combo_letter(obj_info, lt.init);

        _append_combo_letter_data(&obj_info->combo.data, lt.val[0], "AUTO");
        _append_combo_letter_data(&obj_info->combo.data, lt.val[1], "OFF");
        _append_combo_letter_data(&obj_info->combo.data, lt.val[2], "ON");
        _append_combo_letter_data(&obj_info->combo.data, lt.val[3], "SCHEDULE");
    }
    
    return row_info;
}


static ROW_INFO_T *_make_row_info_dnn_toggle(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_dnn_toggle(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "DNN TOGGLE");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_dnn_toggle_combo_event_handler);            
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_adaptive_ir(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_adaptive_ir(ch, &lt);
    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);
    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu)
    {
    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "ADAPTIVE IR");
    }
    else
    {
        obj_info = _set_col_info(row_info, lt.category);
        if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
        else 
        {
            if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_MANUAL) _set_obj_info_enable(obj_info, 0);
            else if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_AUTO) _set_obj_info_enable(obj_info, 1);
        }
        _set_obj_info_width(obj_info, 280);
        _set_data_label_title(obj_info, "ANTI-OVEREXPOSURE MODE");
    }

    obj_info = _set_col_info(row_info, lt.category);
    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_MANUAL) _set_obj_info_enable(obj_info, 0);
        else if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_AUTO) _set_obj_info_enable(obj_info, 1);
    }
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_adaptive_ir_combo_event_handler);            
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;
}


static ROW_INFO_T *_make_row_info_dnn_sense_dton(gint ch)
{
    ROW_INFO_T *row_info;    
    OBJECT_INFO_T *obj_info;    
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_dnn_sense_dton(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "DAY TO NIGHT");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_dnn_sense_dton_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.value, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_dnn_sense_dton_spin_event_handler);
    _set_data_spin_number(obj_info, nm.value, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);     
  
    return row_info;
}

static ROW_INFO_T *_make_row_info_dnn_sense_ntod(gint ch)
{
    ROW_INFO_T *row_info;    
    OBJECT_INFO_T *obj_info;    
    gint support;
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_dnn_sense_ntod(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "NIGHT TO DAY");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_dnn_sense_ntod_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.value, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_dnn_sense_ntod_spin_event_handler);
    _set_data_spin_number(obj_info, nm.value, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);      
  
    return row_info;
}

static ROW_INFO_T *_make_row_info_dnn_schedule(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    NM_TYPE_T nm;
    gchar str[64];

    memset(str, 0x00, sizeof(str));
    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    
    support = _get_data_ircut_dn_mode(ch, &nm);
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "DAY MODE");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_ddn_schedule_handler);

    g_sprintf(str,"%02d : %02d  ~  %02d : %02d", g_ipcamData[ch].dnn_start_hour, g_ipcamData[ch].dnn_start_min, g_ipcamData[ch].dnn_end_hour, g_ipcamData[ch].dnn_end_min);
      
    _set_data_label_letter(obj_info, str);  

    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);
    _set_data_label_legend_string(obj_info, "hh","mm");

    return row_info; 
}

static ROW_INFO_T *_make_row_info_illumination_control(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_illumination_control(ch, &lt);
    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "CONTROL");

    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        for(i=0; i<g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if((g_ipcam_manage[ch].exposure.profile.ircut[i].value == NF_IPCAM_IRCUT_MODE_ITX_DAYTIME) && (g_ipcam_manage[ch].exposure.profile.ircut[i].selected == TRUE))
            {
                _set_obj_info_enable(obj_info, 0);
                break;
            }
            else
            {
                _set_obj_info_enable(obj_info, 1);
            }
        }
    }

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_illumination_ctrl_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        for(i=0; i<g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if((g_ipcam_manage[ch].exposure.profile.ircut[i].value == NF_IPCAM_IRCUT_MODE_ITX_DAYTIME) && (g_ipcam_manage[ch].exposure.profile.ircut[i].selected == TRUE))
            {
                _set_obj_info_enable(obj_info, 0);
                break;
            }
            else
            {
                _set_obj_info_enable(obj_info, 1);
            }
        }
    }

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;
}

static ROW_INFO_T *_make_row_info_illumination_level(gint ch)
{
    ROW_INFO_T *row_info;    
    OBJECT_INFO_T *obj_info;    
    gint support;
    NM_TYPE_T nm;
    gint i;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_illumination_level(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));
    
    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "LEVEL");

    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        for(i=0; i<g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if((g_ipcam_manage[ch].exposure.profile.ircut[i].value == NF_IPCAM_IRCUT_MODE_ITX_DAYTIME) && (g_ipcam_manage[ch].exposure.profile.ircut[i].selected == TRUE))
            {
                _set_obj_info_enable(obj_info, 0);
                break;
            }
            else
            {
                _set_obj_info_enable(obj_info, 1);
            }
        }
    }

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_illumination_level_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.value, nm.min, nm.max, nm.max-nm.min+1);

    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        for(i=0; i<g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if((g_ipcam_manage[ch].exposure.profile.ircut[i].value == NF_IPCAM_IRCUT_MODE_ITX_DAYTIME) && (g_ipcam_manage[ch].exposure.profile.ircut[i].selected == TRUE))
            {
                _set_obj_info_enable(obj_info, 0);
                break;
            }
            else
            {
                _set_obj_info_enable(obj_info, 1);
            }
        }
    }

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_illumination_level_spin_event_handler);
    _set_data_spin_number(obj_info, nm.value, nm.min, nm.max, 1);
    
    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        for(i=0; i<g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if((g_ipcam_manage[ch].exposure.profile.ircut[i].value == NF_IPCAM_IRCUT_MODE_ITX_DAYTIME) && (g_ipcam_manage[ch].exposure.profile.ircut[i].selected == TRUE))
            {
                _set_obj_info_enable(obj_info, 0);
                break;
            }
            else
            {
                _set_obj_info_enable(obj_info, 1);
            }
        }
    }

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 140);
    _set_data_label_legend(obj_info, nm.min, nm.max);      

    if (!g_ipcam_manage[ch].exposure.profile.supported_colorvu) _set_obj_info_enable(obj_info, support);    
    else 
    {
        for(i=0; i<g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if((g_ipcam_manage[ch].exposure.profile.ircut[i].value == NF_IPCAM_IRCUT_MODE_ITX_DAYTIME) && (g_ipcam_manage[ch].exposure.profile.ircut[i].selected == TRUE))
            {
                _set_obj_info_enable(obj_info, 0);
                break;
            }
            else
            {
                _set_obj_info_enable(obj_info, 1);
            }
        }
    }

    return row_info;
}

// exposure

static ROW_INFO_T *_make_row_info_exposure_dnr(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;
 
    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_dnr(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "DNR");

    obj_info = _set_col_info(row_info, lt.category);
    strcpy(obj_info->nickname, "DNR_VALUE_OBJ");
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_exposure_dnr_combo_event_handler);    
    _set_data_combo_letter(obj_info, lt.init);
    for (i = 0; i < lt.cnt; i++) {
        if (g_ipcam_manage[ch].exposure.profile.wdr[1].selected) {
            if (lt.val[i] == NF_IPCAM_DNR_MODE_AUTO_SMART) continue;
        }

        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    }

    return row_info;
}


static ROW_INFO_T *_make_row_info_exposure_mode(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_mode(ch, &lt);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MODE");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_exposure_mode_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_priority(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_priority(ch, &lt);

    if (!support) return 0;
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "PRIORITY");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_exposure_priority_combo_event_handler);    
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;
}

static ROW_INFO_T *_make_row_info_antiflicker(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_antiflicker(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "ANTI-FLICKER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_antiflicker_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_antiflicker_m(gint ch)
{   
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_antiflicker_m(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "ANTI-FLICKER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_antiflicker_m_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for( i =0; i< lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;
}

static ROW_INFO_T *_make_row_info_antiflicker_m_off(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_antiflicker_m_off(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "ANTI-FLICKER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);

    _set_obj_info_handler(obj_info, _post_antiflicker_m_off_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for( i =0; i< lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;   
}
static ROW_INFO_T *_make_row_info_antiflicker_a_off(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));   
    support = _get_data_exposure_antiflicker_a_off(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "ANTI-FLICKER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);

    _set_obj_info_handler(obj_info, _post_antiflicker_a_off_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for( i =0; i< lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    return row_info;   
}
static ROW_INFO_T *_make_row_info_max_shutter_motion_off_on(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_motion_off_on(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_motion_off_on_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}
static ROW_INFO_T *_make_row_info_max_shutter_auto_off_off(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_auto_off_off(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_auto_off_off_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}
static ROW_INFO_T *_make_row_info_max_shutter_auto_off_on_tv(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_auto_off_on_tv(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_auto_off_on_tv_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}
static ROW_INFO_T *_make_row_info_max_shutter_auto_off_on(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_auto_off_on(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_auto_off_on_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

// jaeyyoung test code end

static ROW_INFO_T *_make_row_info_max_shutter_60(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_60(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_60_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_max_shutter_50(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_50(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_50_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_max_shutter_off(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_off(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_off_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_max_shutter_motion_off(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_max_shutter_motion_off(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "MAX SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_max_shutter_motion_off_combo_event_handler);
    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;
}

static ROW_INFO_T *_make_row_info_base_shutter_100(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_100(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_100_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;
}

static ROW_INFO_T *_make_row_info_base_shutter_120(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_120(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_120_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;
}

static ROW_INFO_T *_make_row_info_base_shutter_100_300(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_100_300(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_100_300_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;
}

static ROW_INFO_T *_make_row_info_base_shutter_100_5000(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_100_5000(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_100_5000_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}


static ROW_INFO_T *_make_row_info_base_shutter_120_360(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_120_360(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_120_360_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_120_5000(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_120_5000(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_120_5000_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_120_262(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_120_262(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_120_262_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_30_262(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_30_262(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_30_262_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_25_100(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_25_100(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_25_100_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}


static ROW_INFO_T *_make_row_info_base_shutter_25_300(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_25_300(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_25_300_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_25_5000(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_25_5000(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_25_5000_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_30_120(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_30_120(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_30_120_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_30_360(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_30_360(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_30_360_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_base_shutter_30_5000(gint ch)
{
    ROW_INFO_T *row_info;
    OBJECT_INFO_T *obj_info;
    gint support;
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_base_shutter_30_5000(ch, &lt);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BASE SHUTTER SPEED");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_base_shutter_30_5000_combo_event_handler);

    _set_data_combo_letter(obj_info, lt.init);

    for(i =0; i< lt.cnt; i++)
    {
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);

    }
     
    return row_info;    
}

static ROW_INFO_T *_make_row_info_blc_control(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_blc_control(ch, &lt);
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BLC CONTROL");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_blc_control_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    return row_info;
}

static ROW_INFO_T *_make_row_info_blc_level(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;
    gint i;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_blc_level(ch, &nm);

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "BLC LEVEL");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_blc_level_slider_event_handler);            
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_obj_info_handler(obj_info, _post_blc_level_spin_event_handler);            
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 90);
    _set_data_label_legend(obj_info, nm.min, nm.max);   
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_hlc(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_hlc(ch, &lt);
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "HLC");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_hlc_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
        
    return row_info;
}

static ROW_INFO_T *_make_row_info_defog(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_defog(ch, &lt);
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_data_label_title(obj_info, "DEFOG");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 200);
    _set_obj_info_handler(obj_info, _post_defog_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
        
    return row_info;
}


static ROW_INFO_T *_make_row_info_exposure_time_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "EXPOSURE TIME");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_gain_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "GAIN");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_iris_category()
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, 0);

    obj_info = _set_col_info(row_info, 0);
    _set_obj_info_enable(obj_info, 1);    
    _set_data_label_category(obj_info, IMG_TITLE_BG2, "IRIS");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_time_min(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_minetime(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MIN EXPOSURE TIME");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_auto_exposure_time_min_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_auto_exposure_time_min_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);  
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    

    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_time_max(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_maxetime(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MAX EXPOSURE TIME");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_auto_exposure_time_max_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_auto_exposure_time_max_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_gain_min(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_mingain(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MIN GAIN");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_auto_exposure_gain_min_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_auto_exposure_gain_min_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    

    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_gain_max(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_maxgain(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MAX GAIN");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_auto_exposure_gain_max_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_auto_exposure_gain_max_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_iris_min(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_miniris(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MIN IRIS");

    obj_info = _set_col_info(row_info, nm.category);  
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_auto_exposure_iris_min_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_auto_exposure_iris_min_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    

    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_iris_max(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_maxiris(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MAX IRIS");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_auto_exposure_iris_max_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_auto_exposure_iris_max_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    

    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_time(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_etime(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "EXPOSURE TIME");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_manual_exposure_time_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_manual_exposure_time_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    

    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_gain(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_gain(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "GAIN");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_manual_exposure_gain_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_manual_exposure_gain_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_exposure_iris(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    NM_TYPE_T nm;

    memset(&nm, 0x00, sizeof(NM_TYPE_T));
    support = _get_data_exposure_iris(ch, &nm);
    
    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, nm.category);

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "IRIS");

    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_manual_exposure_iris_slider_event_handler);
    _set_data_slider_normal(obj_info, nm.init, nm.min, nm.max, nm.max-nm.min+1);
    
    obj_info = _set_col_info(row_info, nm.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 120);
    _set_obj_info_handler(obj_info, _post_manual_exposure_iris_spin_event_handler);
    _set_data_spin_number(obj_info, nm.init, nm.min, nm.max, 1);
    
    obj_info = _set_col_info(row_info, nm.category); 
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 170);
    _set_data_label_legend(obj_info, nm.min, nm.max);    
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_slowshutter(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_slowshutter(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "SLOW SHUTTER");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_exposure_slowshutter_combo_event_handler);    
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_maxagc(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_maxagc(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "MAX AGC");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_exposure_maxagc_combo_event_handler);        
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_dciris(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    guint64 category;    
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_dciris(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "IRIS CONTROL");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_exposure_dciris_combo_event_handler);            
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    support = _get_data_exposure_dciris_calibration(ch, &category);

    obj_info = _set_col_info(row_info, category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 150);
    _set_obj_info_handler(obj_info, _post_calibration_button_event_handler);
    _set_data_button_letter(obj_info, "CALIBRATION");
    
    return row_info;
}

static ROW_INFO_T *_make_row_info_dciris_motion(gint ch)
{
    ROW_INFO_T *row_info; 
    OBJECT_INFO_T *obj_info;
    gint support;       
    LT_TYPE_T lt;
    guint64 category;    
    gint i;

    memset(&lt, 0x00, sizeof(LT_TYPE_T));
    support = _get_data_exposure_dciris_motion(ch, &lt);

    if (!support) return 0;

    row_info = imalloc(sizeof(ROW_INFO_T));

    _set_row_info_exposure_category(row_info, lt.category);

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 280);
    _set_data_label_title(obj_info, "IRIS CONTROL");

    obj_info = _set_col_info(row_info, lt.category);
    _set_obj_info_enable(obj_info, support);    
    _set_obj_info_width(obj_info, 220);
    _set_obj_info_handler(obj_info, _post_exposure_dciris_motion_combo_event_handler);            
    _set_data_combo_letter(obj_info, lt.init);

    for (i = 0; i < lt.cnt; i++)
        _append_combo_letter_data(&obj_info->combo.data, lt.val[i], lt.text[i]);
    
    support = _get_data_exposure_dciris_calibration(ch, &category);

    obj_info = _set_col_info(row_info, category);
    _set_obj_info_enable(obj_info, support);
    _set_obj_info_width(obj_info, 150);
    _set_obj_info_handler(obj_info, _post_calibration_button_event_handler);
    _set_data_button_letter(obj_info, "CALIBRATION");
    
    return row_info;
}



static ROW_INFO_T *_make_row_info_image(gint ch, guint64 category)
{
    gint i;

    for (i = 0; i < NF_IPCAM_IMAGE_ONVIF_NR; i++)
    {    
        if ((1 << i) == category)
            break;
    }

    if (i == NF_IPCAM_IMAGE_ONVIF_NR) 
    {
        g_message("%s, %d, ch :%d, %08llX, unsupported category", __FUNCTION__, __LINE__, ch, category);
        g_assert(0);
    }

    if (g_row_manage[ch].image[i]) 
    {
        g_message("%s, %d, error", __FUNCTION__, __LINE__);
        return g_row_manage[ch].image[i];
    }

    switch (category) {
        case NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS :  
            g_row_manage[ch].image[i] = _make_row_info_brightness(ch);   
        break;
        case NF_IPCAM_IMAGE_ONVIF_CONTRAST :    
            g_row_manage[ch].image[i] = _make_row_info_contrast(ch); 
        break;
        case NF_IPCAM_IMAGE_ONVIF_SHARPNESS :   
            g_row_manage[ch].image[i] = _make_row_info_sharpness(ch);    
        break;
        case NF_IPCAM_IMAGE_ONVIF_COLOR :       
            g_row_manage[ch].image[i] = _make_row_info_color(ch);    
        break;
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE :  
            g_row_manage[ch].image[i] = _make_row_info_focus_mode(ch);   
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED :  
            g_row_manage[ch].image[i] = _make_row_info_focus_default_speed(ch);  
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT :     
            g_row_manage[ch].image[i] = _make_row_info_focus_near_limit(ch); 
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT :      
            g_row_manage[ch].image[i] = _make_row_info_focus_far_limit(ch);  
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH :       
            g_row_manage[ch].image[i] = 0; 
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION :    
            g_row_manage[ch].image[i] = _make_row_info_focus_absolute_position(ch);  
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED :       
            g_row_manage[ch].image[i] = _make_row_info_focus_absolute_speed(ch);     
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE :    
            g_row_manage[ch].image[i] = _make_row_info_focus_relative_distance(ch);  
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED :       
            g_row_manage[ch].image[i] = _make_row_info_focus_relative_speed(ch);     
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED :       
            g_row_manage[ch].image[i] = _make_row_info_focus_continuous_speed(ch);   
        break;            
        case NF_IPCAM_IMAGE_ONVIF_WB_MODE :     
            g_row_manage[ch].image[i] = _make_row_info_white_balance_mode(ch);       
        break;            
        case NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN :   
            g_row_manage[ch].image[i] = _make_row_info_white_balance_cr_gain(ch);    
        break;            
        case NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN :   
            g_row_manage[ch].image[i] = _make_row_info_white_balance_cb_gain(ch);    
        break;              
        case NF_IPCAM_IMAGE_ONVIF_ROTATION :    
            g_row_manage[ch].image[i] = _make_row_info_rotate(ch);                   
        break;            
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME :  
            g_row_manage[ch].image[i] = 0; 
        break;            
        case NF_IPCAM_IMAGE_ONVIF_WB_PRESET :   
            g_row_manage[ch].image[i] = _make_row_info_white_balance_mwb_mode(ch);   
        break;                        
        case NF_IPCAM_IMAGE_ONVIF_TINT :        
            g_row_manage[ch].image[i] = _make_row_info_tint(ch); 
        break;                                                          
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR :       
            g_row_manage[ch].image[i] = _make_row_info_focus_continuous_nearfar(ch);    
        break;
        case NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT:       
            g_row_manage[ch].image[i] = _make_row_info_focus_limit(ch);    
        break;
        case NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION :      
            g_row_manage[ch].image[i] = _make_row_info_ir_correction(ch);    
        break;
        case NF_IPCAM_IMAGE_ONVIF_STABILIZER :              
            g_row_manage[ch].image[i] = _make_row_info_stabilizer(ch);    
        break;
        default :   
            g_message("%s, %d, unsupport func", __FUNCTION__, __LINE__);
            return 0;
        break;            
    }

    return g_row_manage[ch].image[i];
}

static ROW_INFO_T *_make_row_info_exposure(gint ch, guint64 category)
{
    gint i;

    for (i = 0; i < NF_IPCAM_EXPOSURE_ONVIF_NR; i++)
    {    
        if ((1LL << i) == category)
            break;
    }

    if (i == NF_IPCAM_EXPOSURE_ONVIF_NR) 
    {
        g_message("%s, %d, ch :%d, %08llX, unsupported category", __FUNCTION__, __LINE__, ch, category);
        g_assert(0);
    }

    if (g_row_manage[ch].exposure[i]) 
    {
        g_message("%s, %d, error", __FUNCTION__, __LINE__);
        return g_row_manage[ch].exposure[i];
    }

    switch (category) {
        case NF_IPCAM_EXPOSURE_DNR_MODE :
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_dnr(ch);
        break; 
        case NF_IPCAM_EXPOSURE_ONVIF_MODE :  
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_mode(ch);   
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_PRIORITY :    
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_priority(ch); 
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_MINETIME :   
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_time_min(ch);    
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_MAXETIME :       
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_time_max(ch);    
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_ETIME :  
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_time(ch);   
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_MINGAIN :  
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_gain_min(ch);  
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN :     
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_gain_max(ch); 
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_GAIN :      
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_gain(ch);  
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_MINIRIS :       
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_iris_min(ch); 
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS :    
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_iris_max(ch);  
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_IRIS :       
            g_row_manage[ch].exposure[i] = _make_row_info_exposure_iris(ch);     
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_BOTTOM :    
            if(g_ipcam_manage[ch].exposure.profile.supported_colorvu)   g_row_manage[ch].exposure[i] = _make_row_info_illumination_control(ch);     
            else    g_row_manage[ch].exposure[i] = 0;  
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_TOP :       
            if(g_ipcam_manage[ch].exposure.profile.supported_colorvu)   g_row_manage[ch].exposure[i] = _make_row_info_illumination_level(ch);     
            else    g_row_manage[ch].exposure[i] = 0;  
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_RIGHT :       
            g_row_manage[ch].exposure[i] = 0;   
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_LEFT :     
            g_row_manage[ch].exposure[i] = 0;       
        break;            
        case NF_IPCAM_EXPOSURE_SLOWSHUTTER :
            g_row_manage[ch].exposure[i] = _make_row_info_slowshutter(ch);    
        break;            
        case NF_IPCAM_EXPOSURE_MAXAGC :   
            g_row_manage[ch].exposure[i] = _make_row_info_maxagc(ch);    
        break;            
        case NF_IPCAM_EXPOSURE_DUMMY :    
            g_row_manage[ch].exposure[i] = 0;        
        break;            
        case NF_IPCAM_EXPOSURE_DCIRIS :   
            g_row_manage[ch].exposure[i] = _make_row_info_dciris(ch);       
        break;        
        case NF_IPCAM_EXPOSURE_DCIRIS_MOTION :
            g_row_manage[ch].exposure[i] = _make_row_info_dciris_motion(ch);
        break;       
        case NF_IPCAM_EXPOSURE_ONVIF_DC_IRIS_CAL :       
            g_row_manage[ch].exposure[i] = 0; 
        break; 
        case NF_IPCAM_EXPOSURE_ANTI_FLICKER :   
            g_row_manage[ch].exposure[i] = _make_row_info_antiflicker(ch);       
        break;  
        case NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION:
            g_row_manage[ch].exposure[i] = _make_row_info_antiflicker_m(ch);
        break;
        case NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF:
            g_row_manage[ch].exposure[i] = _make_row_info_antiflicker_m_off(ch);
        break;        
        case NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF:
            g_row_manage[ch].exposure[i] = _make_row_info_antiflicker_a_off(ch);
        break;
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_50 :   
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_50(ch);       
        break;  
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_60 :   
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_60(ch);       
        break;          
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON:
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_motion_off_on(ch);
        break;
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF:
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_auto_off_off(ch);
        break;
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV:
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_auto_off_on_tv(ch);
        break;
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON:
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_auto_off_on(ch);
        break;
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF :   
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_off(ch);       
        break;          
        case NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF:
            g_row_manage[ch].exposure[i] = _make_row_info_max_shutter_motion_off(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_100:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_100(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_120:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_120(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_100_300(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_100_5000(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_120_360(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_120_5000(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_120_262(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_30_262(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_25_100(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_25_300(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_25_5000(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_30_120(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_30_360(ch);
        break;
        case NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000:
            g_row_manage[ch].exposure[i] = _make_row_info_base_shutter_30_5000(ch);
        break;        
        case NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE :   
            g_row_manage[ch].exposure[i] = _make_row_info_blc_control(ch);       
        break;  
        case NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL :   
            g_row_manage[ch].exposure[i] = _make_row_info_blc_level(ch);       
        break;      
        case NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE :    
            g_row_manage[ch].exposure[i] = _make_row_info_wide_dynamic_mode(ch);        
        break;            
        case NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL :   
            g_row_manage[ch].exposure[i] = _make_row_info_wide_dynamic_level(ch);       
        break;  
        case NF_IPCAM_EXPOSURE_ONVIF_IRCUT :       
            g_row_manage[ch].exposure[i] = _make_row_info_ircut_mode(ch);               
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_IRCUTM:       
            g_row_manage[ch].exposure[i] = _make_row_info_ircutm_mode(ch);               
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE :  
            g_row_manage[ch].exposure[i] = _make_row_info_dnn_toggle(ch);   
        break;  
        case NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR :
            g_row_manage[ch].exposure[i] = _make_row_info_adaptive_ir(ch);   
        break;  
        case NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON:
            g_row_manage[ch].exposure[i] = _make_row_info_dnn_sense_dton(ch);   
        break; 
        case NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD:
            g_row_manage[ch].exposure[i] = _make_row_info_dnn_sense_ntod(ch);   
        break;
        case NF_IPCAM_EXPOSURE_DN_SCHEDULE:
            g_row_manage[ch].exposure[i] = _make_row_info_dnn_schedule(ch);
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_HLC:
            g_row_manage[ch].exposure[i] = _make_row_info_hlc(ch);   
        break;
        case NF_IPCAM_EXPOSURE_ONVIF_DEFOG:
            g_row_manage[ch].exposure[i] = _make_row_info_defog(ch);   
        break;

        default :   
            g_message("%s, %d, unsupport func", __FUNCTION__, __LINE__);
            return 0;
        break;            
    }

    return g_row_manage[ch].exposure[i];
}

gint _get_fixed_info_image_set(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;
    check_category |= NF_IPCAM_IMAGE_ONVIF_CONTRAST;
    check_category |= NF_IPCAM_IMAGE_ONVIF_TINT;
    check_category |= NF_IPCAM_IMAGE_ONVIF_COLOR;
    check_category |= NF_IPCAM_IMAGE_ONVIF_SHARPNESS;    

    if ((g_ipcam_manage[ch].image.profile.supported_image & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_CONTRAST);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_TINT)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_TINT);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_COLOR);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_SHARPNESS);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    fixed_info->visible = g_image_visible[ch];
    fixed_info->enable = g_image_enable[ch];    
    fixed_info->row_cnt = row_cnt;   

    return row_cnt;
}

gint _get_fixed_info_rotate(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_IMAGE_ONVIF_ROTATION;

    if ((g_ipcam_manage[ch].image.profile.supported_image & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_ROTATION)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_ROTATION);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    fixed_info->visible = g_image_visible[ch];
    fixed_info->enable = g_image_enable[ch];
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_focus_mode(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR;
    check_category |= NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION;
    check_category |= NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT;
    check_category |= NF_IPCAM_IMAGE_ONVIF_STABILIZER;

    if ((g_ipcam_manage[ch].image.profile.supported_image & check_category) == 0) return 0;

    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_FOCUS];

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }    

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }  

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }  

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }      

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }      

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_STABILIZER)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_STABILIZER);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    fixed_info->visible = g_image_visible[ch];
    fixed_info->enable = g_image_enable[ch];
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_wb_mode(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_IMAGE_ONVIF_WB_MODE;
    check_category |= NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN;
    check_category |= NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN;
    check_category |= NF_IPCAM_IMAGE_ONVIF_WB_PRESET;

    if ((g_ipcam_manage[ch].image.profile.supported_image & check_category) == 0) return 0;

    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_WB];

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_MODE)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_WB_MODE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }      

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }      

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }          

    if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_PRESET)
    {
        info = _make_row_info_image(ch, NF_IPCAM_IMAGE_ONVIF_WB_PRESET);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }          
    
    fixed_info->visible = g_image_visible[ch];
    fixed_info->enable = g_image_enable[ch];
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_dn_mode(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_IRCUT;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_IRCUTM;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD;
    check_category |= NF_IPCAM_EXPOSURE_DN_SCHEDULE;

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;
    
    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_DN];

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRCUT);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUTM)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRCUTM);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }      

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR)
    {   
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DN_SCHEDULE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_DN_SCHEDULE);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_cv_mode(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_IRCUT;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_IRCUTM;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD;
    check_category |= NF_IPCAM_EXPOSURE_DN_SCHEDULE;

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;
    
    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_CV];

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRCUT);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUTM)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRCUTM);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR)
    {   
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if(g_ipcam_manage[ch].exposure.profile.supported_colorvu)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_BOTTOM);
        if(info) fixed_info->row_info[row_cnt++] = info;

        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_TOP);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DN_SCHEDULE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_DN_SCHEDULE);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_exposure_mode(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MODE;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_PRIORITY;

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MODE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MODE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_PRIORITY)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_PRIORITY);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
        
    return row_cnt;
}

gint _get_fixed_info_exposure_blc_mode(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL;

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_exposure_antiflicker(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER;
    check_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;

    check_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;
    check_category |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;

    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;
    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;
    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;
    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;

    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_50;
    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_60;
    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;
    check_category |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF;

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

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }    

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_50)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_50);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_60)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_60);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF);
        if( info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_100);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000);
        if(info) fixed_info->row_info[row_cnt++] = info;
    }  
        
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_exposure_hlc(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_HLC;

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_HLC)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_HLC);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
 
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;

    return row_cnt;
}

gint _get_fixed_info_exposure_defog(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_DEFOG;
    
    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DEFOG)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_DEFOG);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }  
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
        
    return  row_cnt;
}

gint _get_fixed_info_exposure_dnr(gint ch, FIXED_INFO_T *fixed_info)
{
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_DNR_MODE;
    
    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DNR_MODE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_DNR_MODE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
        
    return row_cnt;
}

gint _get_fixed_info_exposure_time(gint ch, FIXED_INFO_T *fixed_info)
{   
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MINETIME;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MAXETIME;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_ETIME;
    check_category |= NF_IPCAM_EXPOSURE_SLOWSHUTTER;
    
    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_EXPOSURE_TIME];

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MINETIME);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MAXETIME);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_ETIME);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_SLOWSHUTTER)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_SLOWSHUTTER);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
    
    return row_cnt;
}

gint _get_fixed_info_exposure_gain(gint ch, FIXED_INFO_T *fixed_info)
{       
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MINGAIN;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_GAIN;
    check_category |= NF_IPCAM_EXPOSURE_MAXAGC;
    
    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_GAIN];

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MINGAIN);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_GAIN);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_MAXAGC)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_MAXAGC);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
    
    return row_cnt;
}

gint _get_fixed_info_exposure_iris(gint ch, FIXED_INFO_T *fixed_info)
{       
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MINIRIS;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_IRIS;
    check_category |= NF_IPCAM_EXPOSURE_DCIRIS;
    check_category |= NF_IPCAM_EXPOSURE_DCIRIS_MOTION;
    
    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_IRIS];

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MINIRIS);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRIS);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_DCIRIS);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS_MOTION)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_DCIRIS_MOTION);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
    
    return row_cnt;
}

gint _get_fixed_info_wdr_mode(gint ch, FIXED_INFO_T *fixed_info)
{    
    ROW_INFO_T *info;
    guint64 check_category = 0;
    gint row_cnt = 0;

    check_category |= NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE;
    check_category |= NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL;

    if ((g_ipcam_manage[ch].exposure.profile.supported_exposure & check_category) == 0) return 0;

    fixed_info->row_info[row_cnt++] = g_row_category[CATEGORY_WDR];

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
    {
        info = _make_row_info_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL);
        if (info) fixed_info->row_info[row_cnt++] = info;
    }
    
    fixed_info->visible = g_exposure_visible[ch];
    fixed_info->enable = g_exposure_enable[ch];    
    fixed_info->row_cnt = row_cnt;
    
    return row_cnt;
}

void get_data_exposure_antiflicker(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_antiflicker(ch, lt);
}

void get_data_exposure_antiflicker_m(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_antiflicker_m(ch, lt);
}

void get_data_exposure_antiflicker_m_off(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_antiflicker_m_off(ch, lt);
}

void get_data_exposure_antiflicker_a_off(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_antiflicker_a_off(ch, lt);
}

void get_data_exposure_max_shutter_50(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_50(ch, lt);
}

void get_data_exposure_max_shutter_60(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_60(ch, lt);
}

void get_data_exposure_max_shutter_off(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_off(ch, lt);
}

void get_data_exposure_max_shutter_motion_off(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_motion_off(ch, lt);
}

void get_data_exposure_max_shutter_motion_off_on(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_motion_off_on(ch, lt);
}

void get_data_exposure_max_shutter_auto_off_off(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_auto_off_off(ch, lt);
}

void get_data_exposure_max_shutter_auto_off_on_tv(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_auto_off_on_tv(ch, lt);
}

void get_data_exposure_max_shutter_auto_off_on(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_max_shutter_auto_off_on(ch, lt);
}

void get_data_exposure_base_shutter_100(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_100(ch, lt);
}

void get_data_exposure_base_shutter_120(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_120(ch, lt);
}

void get_data_exposure_base_shutter_100_300(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_100_300(ch, lt);
}

void get_data_exposure_base_shutter_100_5000(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_100_5000(ch, lt);
}

void get_data_exposure_base_shutter_120_360(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_120_360(ch, lt);
}

void get_data_exposure_base_shutter_120_5000(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_120_5000(ch, lt);
}

void get_data_exposure_base_shutter_120_262(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_120_262(ch, lt);
}

void get_data_exposure_base_shutter_30_262(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_30_262(ch, lt);
}

void get_data_exposure_base_shutter_25_100(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_25_100(ch, lt);
}

void get_data_exposure_base_shutter_25_300(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_25_300(ch, lt);
}

void get_data_exposure_base_shutter_25_5000(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_25_5000(ch, lt);
}

void get_data_exposure_base_shutter_30_120(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_30_120(ch, lt);
}

void get_data_exposure_base_shutter_30_360(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_30_360(ch, lt);
}

void get_data_exposure_base_shutter_30_5000(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_base_shutter_30_5000(ch, lt);
}

void get_data_exposure_dciris(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_dciris(ch, lt);
}

void get_data_exposure_dciris_motion(gint ch, LT_TYPE_T *lt, gint *supp)
{
    *supp = _get_data_exposure_dciris_motion(ch, lt);
}

void get_data_exposure_colorvu(gint ch, gint *supp)
{
    *supp = g_ipcam_manage[ch].exposure.profile.supported_colorvu;
}

gint _make_category_label()
{
    gint i;

    for (i = 0; i < CATEGORY_NR; i++)
    {
        switch (i) {
            case CATEGORY_FOCUS :   
                g_row_category[i] = _make_row_info_focus_category();    
            break;
            case CATEGORY_WB :      
                g_row_category[i] = _make_row_info_white_balance_category();    
            break;
            case CATEGORY_WDR :     
                g_row_category[i] = _make_row_info_wide_dynamic_category(); 
            break;
            case CATEGORY_DN :      
                g_row_category[i] = _make_row_info_day_night_category();    
            break;
            case CATEGORY_CV :      
                g_row_category[i] = _make_row_info_colorvu_category();    
            break;
            case CATEGORY_EXPOSURE_TIME :   
                g_row_category[i] = _make_row_info_exposure_time_category();    
            break;
            case CATEGORY_GAIN :    
                g_row_category[i] = _make_row_info_exposure_gain_category();    
            break;
            case CATEGORY_IRIS :    
                g_row_category[i] = _make_row_info_exposure_iris_category();    
            break;            
            default :                       
                g_row_category[i] = 0;
            break;
        }
    }
    
    return 0;
}

gint _destory_category_label()
{
    COL_INFO_T      *col_info;
    OBJECT_INFO_T   *obj_info;
    gint row, col;

    for (row = 0; row < CATEGORY_NR; row++)
    {   
        if (g_row_category[row])
        {
            for (col = 0; col < g_row_category[row]->col_cnt; col++)
            {
                col_info = g_row_category[row]->col_info[col];
                obj_info = col_info->obj_info;
                
                if (obj_info->obj_type == OBJ_LABEL)        _unset_data_label(obj_info);
                else if (obj_info->obj_type == OBJ_SPIN)    _unset_data_spin(obj_info);
                else if (obj_info->obj_type == OBJ_COMBO)   _unset_data_combo(obj_info);
                else if (obj_info->obj_type == OBJ_SLIDER)  _unset_data_slider(obj_info);
                else if (obj_info->obj_type == OBJ_BUTTON)  _unset_data_button(obj_info);

                _unset_obj_info(obj_info);
                _unset_col_info(col_info);
            }
           
            ifree(g_row_category[row]);
            g_row_category[row] = 0;            
        }
    }

    return 0;
}

gint _destory_row_info(gint ch)
{
    COL_INFO_T      *col_info;
    OBJECT_INFO_T   *obj_info;
    gint row, col;

    for (row = 0; row < NF_IPCAM_IMAGE_ONVIF_NR; row++)
    {   
        if (g_row_manage[ch].image[row])
        {
            for (col = 0; col < g_row_manage[ch].image[row]->col_cnt; col++)
            {
                col_info = g_row_manage[ch].image[row]->col_info[col];
                obj_info = col_info->obj_info;
                
                if (obj_info->obj_type == OBJ_LABEL)        _unset_data_label(obj_info);
                else if (obj_info->obj_type == OBJ_SPIN)    _unset_data_spin(obj_info);
                else if (obj_info->obj_type == OBJ_COMBO)   _unset_data_combo(obj_info);
                else if (obj_info->obj_type == OBJ_SLIDER)  _unset_data_slider(obj_info);
                else if (obj_info->obj_type == OBJ_BUTTON)  _unset_data_button(obj_info);

                _unset_obj_info(obj_info);
                _unset_col_info(col_info);
            }
           
            ifree(g_row_manage[ch].image[row]);
            g_row_manage[ch].image[row] = 0;
        }
    }

    for (row = 0; row < NF_IPCAM_EXPOSURE_ONVIF_NR; row++)
    {   
        if (g_row_manage[ch].exposure[row])
        {
            for (col = 0; col < g_row_manage[ch].exposure[row]->col_cnt; col++)
            {
                col_info = g_row_manage[ch].exposure[row]->col_info[col];
                obj_info = col_info->obj_info;
                
                if (obj_info->obj_type == OBJ_LABEL)        _unset_data_label(obj_info);
                else if (obj_info->obj_type == OBJ_SPIN)    _unset_data_spin(obj_info);
                else if (obj_info->obj_type == OBJ_COMBO)   _unset_data_combo(obj_info);
                else if (obj_info->obj_type == OBJ_SLIDER)  _unset_data_slider(obj_info);
                else if (obj_info->obj_type == OBJ_BUTTON)  _unset_data_button(obj_info);

                _unset_obj_info(obj_info);
                _unset_col_info(col_info);
            }
           
            ifree(g_row_manage[ch].exposure[row]);
            g_row_manage[ch].exposure[row] = 0;
        }
    }

    return 0;
}


gint _update_ipcam_profile(gint ch)
{
	NFIPCamImageProfile_onvif       image;
	NFIPCamExposureProfile_onvif    exposure;

	int tmp = 0;
    memset(&g_ipcam_manage[ch], 0x00, sizeof(IPCAM_MANAGE_T));

    g_ipcam_manage[ch].is_onvif = nf_ipcam_is_onvif_support(ch);   
   
    if (nf_ipcam_get_image_profile_onvif(ch, &image) == IPCAM_SETUP_RTN_DONE)
    {       
        memcpy(&g_ipcam_manage[ch].image.profile, &image, sizeof(NFIPCamImageProfile_onvif));
        g_ipcam_manage[ch].image.done = 1;
    }

    if (!g_ipcam_manage[ch].image.profile.supported_image) g_ipcam_manage[ch].image_default = 1;

    if ((tmp = nf_ipcam_get_exposure_profile_onvif(ch, &exposure)) == IPCAM_SETUP_RTN_DONE)
    {
        memcpy(&g_ipcam_manage[ch].exposure.profile, &exposure, sizeof(NFIPCamExposureProfile_onvif));
        g_ipcam_manage[ch].exposure.done = 1;
    }

    if (!g_ipcam_manage[ch].exposure.profile.supported_exposure) g_ipcam_manage[ch].exposure_default = 1;

    return 0;
}

gint _set_drawing_profile()
{
    gint ch;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (g_ipcam_manage[ch].image_default)
        {
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_CONTRAST;
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_SHARPNESS;
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_COLOR;
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_TINT;
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;        
            g_ipcam_manage[ch].image.profile.supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_MODE;        
        
            g_image_visible[ch] = g_ipcam_manage[ch].image.profile.supported_image;
            g_image_enable[ch] = 0;
        }
        else
        {
            _get_visible_image_category(ch, &g_image_visible[ch]);
            _get_enable_image_category(ch, &g_image_enable[ch]);  
            //g_message("[###yanggungg] %s, %d, g_image_visible[%d]:%08llX", __FUNCTION__, __LINE__, ch, g_image_visible[ch]);
            //g_message("[###yanggungg] %s, %d, g_image_visible[%d]:%08llX", __FUNCTION__, __LINE__, ch, g_image_visible[ch]);

        }
        
        if (g_ipcam_manage[ch].exposure_default)
        {
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MODE;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_HLC;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DEFOG;
            //g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ANTI_FLICKER;
            //g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;
            //g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_DNR_MODE;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINETIME;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXETIME;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINGAIN;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINIRIS;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRCUT;
            g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE;
            //g_ipcam_manage[ch].exposure.profile.supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR;
            g_exposure_visible[ch] = g_ipcam_manage[ch].exposure.profile.supported_exposure;
            g_exposure_enable[ch] = 0;
        }
        else
        {
            _get_visible_exposure_category(ch, &g_exposure_visible[ch]);
            _get_enable_exposure_category(ch, &g_exposure_enable[ch]);
            //g_message("[###yanggungg] %s, %d, g_exposure_visible[ch]:%08llX", __FUNCTION__, __LINE__, g_exposure_visible[ch]);
            //g_message("[###yanggungg] %s, %d, g_exposure_enable[ch]:%08llX", __FUNCTION__, __LINE__, g_exposure_enable[ch]);
        }      
    }

    return 0;
}

gint _load_ipcam_db()
{
    gint ch;

	memset(g_ipcamData, 0x00, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT);
	memset(g_org_ipcamData, 0x00, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
    	DAL_get_IPCamSetup_data(&g_ipcamData[ch], ch);	
    }

	g_memmove(g_org_ipcamData, g_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {   
        _sync_db_data(ch);
    }

	if (memcmp(g_org_ipcamData, g_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT))
    {
        DAL_set_IPCamSetup_data_all(g_ipcamData, GUI_CHANNEL_CNT);
        syscam_set_changeflag(1);
    	g_memmove(g_org_ipcamData, g_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT);        
    }

	return 0;
}

gint _save_ipcam_db()
{
	if(memcmp(g_org_ipcamData, g_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT))
	{
		scm_put_log(CHANGE_CAM_IMAGE, 0, 0);

		g_memmove(g_org_ipcamData, g_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT);
		DAL_set_IPCamSetup_data_all(g_ipcamData, GUI_CHANNEL_CNT);
        DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);

		syscam_set_changeflag(1);
	}

    return 0;
}

gint _cancel_ipcam_db()
{   
    gint i;

	if (memcmp(g_ipcamData, g_org_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT))
	{
        DAL_set_IPCamSetup_data_all(g_org_ipcamData, GUI_CHANNEL_CNT);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
            DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, i);            
    }

	g_memmove(g_ipcamData, g_org_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        nfui_user_signal_emit(g_ipcam_subFixed[i].imageSet.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        nfui_user_signal_emit(g_ipcam_subFixed[i].rotate.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        nfui_user_signal_emit(g_ipcam_subFixed[i].focusMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        nfui_user_signal_emit(g_ipcam_subFixed[i].wbMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        nfui_user_signal_emit(g_ipcam_subFixed[i].dnMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureBlc.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureAnti.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureTime.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureGain.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);                
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureIris.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE); 
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureDnr.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        nfui_user_signal_emit(g_ipcam_subFixed[i].exposureWdr.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
    }

    return 0;
}

gint _copy_ipcam_db(gint src_ch, guint copy_mask)
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (copy_mask & (1 << i))
        {
            g_memmove(&g_ipcamData[i], &g_ipcamData[src_ch], sizeof(IPCamSetupData));
            
            DAL_set_IPCamSetup_data(g_ipcamData[i], i);            
            DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, i);  

            nfui_user_signal_emit(g_ipcam_subFixed[i].imageSet.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
            nfui_user_signal_emit(g_ipcam_subFixed[i].rotate.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
            nfui_user_signal_emit(g_ipcam_subFixed[i].focusMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
            nfui_user_signal_emit(g_ipcam_subFixed[i].wbMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
            nfui_user_signal_emit(g_ipcam_subFixed[i].dnMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureMode.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureBlc.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureAnti.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);                    
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureTime.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);        
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureGain.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);                
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureIris.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);    
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureDnr.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
            nfui_user_signal_emit(g_ipcam_subFixed[i].exposureWdr.fixed, NFEVENT_IPCAMSETUP_DB_SYNC, TRUE);
        }
    }

    return 0;
}

gint _is_changed_ipcam_db()
{
	if (!memcmp(g_org_ipcamData, g_ipcamData, sizeof(IPCamSetupData)*GUI_CHANNEL_CNT))
        return -1;           
        
    return 0;
}

ROW_INFO_T *find_row_info_exposure(gint ch, guint64 category)
{
    gint i;

    for (i = 0; i < NF_IPCAM_EXPOSURE_ONVIF_NR; i++)
    {    
        if ((1LL << i) == category)
            break;
    }

    if (i == NF_IPCAM_EXPOSURE_ONVIF_NR) 
    {
        g_message("%s, %d, ch :%d, %08llX, unsupported category", __FUNCTION__, __LINE__, ch, category);
        g_assert(0);
    }

    if (g_row_manage[ch].exposure[i]) 
    {
        return g_row_manage[ch].exposure[i];
    }

    return 0;
}

NFOBJECT *find_obj_exposure(gint ch, guint64 category, gpointer data)
{
    ROW_INFO_T *row_info;
    gint i;

    row_info = find_row_info_exposure(ch, category);
    if(!row_info) return 0;

    for(i = 0; i < row_info->col_cnt; i++)
    {
        if(row_info->col_info[i]->obj_info->handler == data) break;
    }

    if(i == row_info->col_cnt) return 0;
    
    return row_info->col_info[i]->obj;
}

