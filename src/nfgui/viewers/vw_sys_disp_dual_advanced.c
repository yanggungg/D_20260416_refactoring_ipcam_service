#include "nf_afx.h"
#include "nf_api_param_app.h"
#include "nf_sysman.h"

#include "nf_edid.h"

#include "scm.h"
#include "smt.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw.h"
#include "vw_sys_disp_main.h"
#include "vw_sys_disp_dual_advanced.h"
#include "vw_sys_disp_spot_conf.h"


#define STR_RESOL_AUTO          "AUTO"
#define STR_RESOL_800_600       "800 X 600"
#define STR_RESOL_1280_720      "1280 X 720"
#define STR_RESOL_1280_1024     "1280 X 1024"
#define STR_RESOL_1920_1080     "1920 X 1080"
#define STR_RESOL_2560_1440     "2560 X 1440"
#if !defined (CHIP_NVT)
#define STR_RESOL_2560_1600     "2560 X 1600"
#endif
#define STR_RESOL_3840_2160     "3840 X 2160"

#define TABLE_LEFT           (28)
#define TABLE_TOP            (42)

#define LABEL_HEIGHT         (40)

#if defined(GUI_32CH_SUPPORT)
    #define SPOT_SPLIT_CNT      (32)
    #define DUAL_M_SPOT_OUTPUT_CH   0xffffffff
#elif defined(GUI_16CH_SUPPORT)
    #if defined (_ANF8G_1648D)
        #define SPOT_SPLIT_CNT      (4)
    #else
        #define SPOT_SPLIT_CNT      (16)
    #endif
    #define DUAL_M_SPOT_OUTPUT_CH   0xffff
#elif defined(GUI_8CH_SUPPORT) 
    #define SPOT_SPLIT_CNT      (8)
    #define DUAL_M_SPOT_OUTPUT_CH   0x00ff
#else
    #define SPOT_SPLIT_CNT      (4)
    #define DUAL_M_SPOT_OUTPUT_CH   0x000f
#endif

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_display_mode;
static NFOBJECT *g_monitor_image[2];
static NFOBJECT *g_monitor_type[2][2];
static NFOBJECT *g_monitor_resol[2];
static NFOBJECT *g_spot_edit[2];

static AdvDualData g_advDual_data;
static AdvDualData g_org_advDual_data;


static gboolean _proc_escape(void *data)
{
    NFOBJECT *popup = (NFOBJECT *)data;

    if (popup) nftool_remove_waitbox(popup);

    nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
    
    smt_set_service(SMT_SHUTDOWN);
    scm_shutdown_system(IRET_SCM_DUAL_MONITOR_CHANGE);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);


    return FALSE;
}

static gint _is_need_set_sysman_app_param()
{
    if ((g_org_advDual_data.mode == g_advDual_data.mode) &&
        (g_org_advDual_data.monitor_type[0] == g_advDual_data.monitor_type[0]) &&
        (g_org_advDual_data.monitor_type[1] == g_advDual_data.monitor_type[1]) &&
        (g_org_advDual_data.monitor_resol[0] == g_advDual_data.monitor_resol[0]) &&
        (g_org_advDual_data.monitor_resol[1] == g_advDual_data.monitor_resol[1]))
    {
        return 0;
    }
    
    return 1;
}

static gint _get_monitor_type(gint *dual_mode, gint *main_monitor)
{
    if (g_advDual_data.mode == 0)
    {
        *dual_mode = NF_SYSMAN_DUAL_DISP_TYPE_TIE;
        *main_monitor = 0;
    }
    else
    {
        *dual_mode = NF_SYSMAN_DUAL_DISP_TYPE_DUAL;

        if (g_advDual_data.monitor_type[0] == 0) *main_monitor = 1;
        else *main_monitor = 0;
    }

    return 0;
}

static gint _is_valid_monitor_param(gint monitor_idx, gint *ddc_resol)
{
    NFOBJECT *resol_obj;
    gint is_vga;
    
    if (monitor_idx == 0) 
    {
        resol_obj = g_monitor_resol[0];
        is_vga = 0;
    }
    else 
    {
        resol_obj = g_monitor_resol[1];
        is_vga = 1;
    }
    
    if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_AUTO)) {*ddc_resol = EDID_PRI_UNKNOWN; return 1;}

    if (IS_PAL)
    {
        if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_3840_2160)) 
        {
            *ddc_resol = EDID_PRI_3840_2160_30_PAL;
/*
            if (nf_live_check_monitor_resolution(NF_EDID_RES_2160P_50, is_vga)) {*ddc_resol = EDID_PRI_3840_2160_60_PAL; return 1;}
            if (nf_live_check_monitor_resolution(NF_EDID_RES_2160P_25, is_vga)) {*ddc_resol = EDID_PRI_3840_2160_30_PAL; return 1;}
*/
            if (nf_live_check_monitor_resolution(NF_EDID_RES_2160P_50, is_vga) || nf_live_check_monitor_resolution(NF_EDID_RES_2160P_25, is_vga)) 
            {
                return 1;
            }
        }
#if !defined CHIP_NVT
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_2560_1600)) 
        {
            *ddc_resol = EDID_PRI_2560_1600_60_PAL; 

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1600P_60, is_vga)) {*ddc_resol = EDID_PRI_2560_1600_60_PAL; return 1;}
        }
#endif
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_2560_1440)) 
        {
            *ddc_resol = EDID_PRI_2560_1440_30_PAL;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1440P_60, is_vga)) {*ddc_resol = EDID_PRI_2560_1440_60_PAL; return 1;}
            if (nf_live_check_monitor_resolution(NF_EDID_RES_1440P_30, is_vga)) {*ddc_resol = EDID_PRI_2560_1440_30_PAL; return 1;}
        }            
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_1920_1080))  
        {
            *ddc_resol = EDID_PRI_1080P_50_PAL;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1080P_50, is_vga)) {*ddc_resol = EDID_PRI_1080P_50_PAL; return 1;}
            if (nf_live_check_monitor_resolution(NF_EDID_RES_1080P_25, is_vga)) {*ddc_resol = EDID_PRI_1080P_25_PAL; return 1;}
        }
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_1280_720)) 
        {
            *ddc_resol = EDID_PRI_720P_60_PAL;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_720P_50, is_vga)) {*ddc_resol = EDID_PRI_720P_60_PAL; return 1;}
        }
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_1280_1024)) 
        {
            *ddc_resol = EDID_PRI_1280_1024_60_PAL;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1280_1024_60, is_vga)) {*ddc_resol = EDID_PRI_1280_1024_60_PAL; return 1;}
        }
    }
    else
    {
        if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_3840_2160)) 
        {
            *ddc_resol = EDID_PRI_3840_2160_30;
/*
            if (nf_live_check_monitor_resolution(NF_EDID_RES_2160P_60, is_vga)) {*ddc_resol = EDID_PRI_3840_2160_60; return 1;}
            if (nf_live_check_monitor_resolution(NF_EDID_RES_2160P_30, is_vga)) {*ddc_resol = EDID_PRI_3840_2160_30; return 1;}
*/        
            if (nf_live_check_monitor_resolution(NF_EDID_RES_2160P_60, is_vga) || nf_live_check_monitor_resolution(NF_EDID_RES_2160P_30, is_vga)) 
            {
                return 1;
            }
        }
#if !defined CHIP_NVT
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_2560_1600)) 
        {
            *ddc_resol = EDID_PRI_2560_1600_60;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1600P_60, is_vga)) {*ddc_resol = EDID_PRI_2560_1600_60; return 1;}
        }
#endif
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_2560_1440)) 
        {
            *ddc_resol = EDID_PRI_2560_1440_30;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1440P_60, is_vga)) {*ddc_resol = EDID_PRI_2560_1440_60; return 1;}
            if (nf_live_check_monitor_resolution(NF_EDID_RES_1440P_30, is_vga)) {*ddc_resol = EDID_PRI_2560_1440_30; return 1;}
        }            
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_1920_1080))
        {
            *ddc_resol = EDID_PRI_1080P_60;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1080P_60, is_vga)) {*ddc_resol = EDID_PRI_1080P_60; return 1;}
            if (nf_live_check_monitor_resolution(NF_EDID_RES_1080P_30, is_vga)) {*ddc_resol = EDID_PRI_1080P_30; return 1;}
        }
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_1280_720)) 
        {
            *ddc_resol = EDID_PRI_720P_60;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_720P_60, is_vga)) {*ddc_resol = EDID_PRI_720P_60; return 1;}
        }
        else if (!strcmp(nfui_combobox_get_value((NFCOMBOBOX*)resol_obj), STR_RESOL_1280_1024)) 
        {
            *ddc_resol = EDID_PRI_1280_1024_60;

            if (nf_live_check_monitor_resolution(NF_EDID_RES_1280_1024_60, is_vga)) {*ddc_resol = EDID_PRI_1280_1024_60; return 1;}
        }
    }    

    return 0;
}

static gchar *_get_disp_resol(gint ddc_resol)
{
    if (ddc_resol == EDID_PRI_UNKNOWN) return STR_RESOL_AUTO;
    else if (ddc_resol == EDID_PRI_800_600_60) return STR_RESOL_800_600;
    else if (ddc_resol == EDID_PRI_1080P_25) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_1280_1024_60) return STR_RESOL_1280_1024;
    else if (ddc_resol == EDID_PRI_720P_60) return STR_RESOL_1280_720;
    else if (ddc_resol == EDID_PRI_1080P_50) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_1080P_30) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_1080P_60) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_2560_1440_30) return STR_RESOL_2560_1440;
    else if (ddc_resol == EDID_PRI_2560_1440_60) return STR_RESOL_2560_1440;
#if !defined CHIP_NVT
    else if (ddc_resol == EDID_PRI_2560_1600_60) return STR_RESOL_2560_1600;
#endif
    else if (ddc_resol == EDID_PRI_3840_2160_30) return STR_RESOL_3840_2160;
    else if (ddc_resol == EDID_PRI_3840_2160_60) return STR_RESOL_3840_2160;
    else if (ddc_resol == EDID_PRI_800_600_60_PAL) return STR_RESOL_800_600;
    else if (ddc_resol == EDID_PRI_1280_1024_60_PAL) return STR_RESOL_1280_1024;
    else if (ddc_resol == EDID_PRI_720P_60_PAL) return STR_RESOL_1280_720;
    else if (ddc_resol == EDID_PRI_1080P_30_PAL) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_1080P_60_PAL) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_1080P_25_PAL) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_1080P_50_PAL) return STR_RESOL_1920_1080;
    else if (ddc_resol == EDID_PRI_2560_1440_30_PAL) return STR_RESOL_2560_1440;
    else if (ddc_resol == EDID_PRI_2560_1440_60_PAL) return STR_RESOL_2560_1440;
#if !defined CHIP_NVT
    else if (ddc_resol == EDID_PRI_2560_1600_60_PAL) return STR_RESOL_2560_1600;
#endif
    else if (ddc_resol == EDID_PRI_3840_2160_30_PAL) return STR_RESOL_3840_2160;
    else if (ddc_resol == EDID_PRI_3840_2160_60_PAL) return STR_RESOL_3840_2160;

    return STR_RESOL_AUTO;
}

static gint _proc_set_sysman_app_param()
{
    gint dual_mode, main_monitor;
    
    _get_monitor_type(&dual_mode, &main_monitor);

    g_message("enum _NF_SYSMAN_DUAL_DISP_TYPE_E");
    g_message("    NF_SYSMAN_DUAL_DISP_TYPE_NONE    = %d", NF_SYSMAN_DUAL_DISP_TYPE_NONE);
    g_message("    NF_SYSMAN_DUAL_DISP_TYPE_TIE     = %d", NF_SYSMAN_DUAL_DISP_TYPE_TIE);
    g_message("    NF_SYSMAN_DUAL_DISP_TYPE_DUAL    = %d", NF_SYSMAN_DUAL_DISP_TYPE_DUAL);
    g_message("    NF_SYSMAN_DUAL_DISP_TYPE_CF      = %d", NF_SYSMAN_DUAL_DISP_TYPE_CF);
    g_message(" ");
    g_message("enum _NF_APP_PARAM_RES_E");
    g_message("    EDID_PRI_UNKNOWN          = %d", EDID_PRI_UNKNOWN);
    g_message("    EDID_PRI_1080P_25         = %d", EDID_PRI_1080P_25);
    g_message("    EDID_PRI_1280_1024_60     = %d", EDID_PRI_1280_1024_60);
    g_message("    EDID_PRI_720P_60          = %d", EDID_PRI_720P_60);
    g_message("    EDID_PRI_1080P_50         = %d", EDID_PRI_1080P_50);
    g_message("    EDID_PRI_1080P_30         = %d", EDID_PRI_1080P_30);
    g_message("    EDID_PRI_1080P_60         = %d", EDID_PRI_1080P_60);
    g_message("    EDID_PRI_2560_1440_30     = %d", EDID_PRI_2560_1440_30);
    g_message("    EDID_PRI_2560_1440_60     = %d", EDID_PRI_2560_1440_60);
    g_message("    EDID_PRI_2560_1600_60     = %d", EDID_PRI_2560_1600_60);
    g_message("    EDID_PRI_3840_2160_30     = %d", EDID_PRI_3840_2160_30);
    g_message("    EDID_PRI_3840_2160_60     = %d", EDID_PRI_3840_2160_60);
    g_message("    EDID_PRI_1280_1024_60_PAL = %d", EDID_PRI_1280_1024_60_PAL);
    g_message("    EDID_PRI_720P_60_PAL      = %d", EDID_PRI_720P_60_PAL);
    g_message("    EDID_PRI_1080P_30_PAL     = %d", EDID_PRI_1080P_30_PAL); 
    g_message("    EDID_PRI_1080P_60_PAL     = %d", EDID_PRI_1080P_60_PAL);
    g_message("    EDID_PRI_1080P_25_PAL     = %d", EDID_PRI_1080P_25_PAL);
    g_message("    EDID_PRI_1080P_50_PAL     = %d", EDID_PRI_1080P_50_PAL);
    g_message("    EDID_PRI_2560_1440_30_PAL = %d", EDID_PRI_2560_1440_30_PAL);    
    g_message("    EDID_PRI_2560_1440_60_PAL = %d", EDID_PRI_2560_1440_60_PAL);   
    g_message("    EDID_PRI_2560_1600_60_PAL = %d", EDID_PRI_2560_1600_60_PAL);   
    g_message("    EDID_PRI_3840_2160_30_PAL = %d", EDID_PRI_3840_2160_30_PAL);   
    g_message("    EDID_PRI_3840_2160_60_PAL = %d", EDID_PRI_3840_2160_60_PAL);   
    g_message(" ");
    g_message("%s, %d, dual_mode:%d, main_monitor:%d", __FUNCTION__, __LINE__, dual_mode, main_monitor);
    g_message("%s, %d, res_hdmi:%d, res_vga:%d", __FUNCTION__, __LINE__, g_advDual_data.monitor_resol[0], g_advDual_data.monitor_resol[1]);

    nf_api_param_app_set_cate(NF_PARAM_APP_CATE_DUAL_TYPE, dual_mode);

    if (dual_mode == NF_SYSMAN_DUAL_DISP_TYPE_DUAL) 
    {
        nf_api_param_app_set_cate(NF_PARAM_APP_CATE_SET_MONITOR_MAIN, main_monitor);
    }

    nf_sysman_user_set_resolution_dual(g_advDual_data.monitor_resol[0], g_advDual_data.monitor_resol[1]);

    return 0;
}

static gint _change_object(gint expose)
{
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_display_mode, g_advDual_data.mode);

    if (g_advDual_data.mode == 0) // TIE
    {
        nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_TIE);
        nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_TIE);    

        nfui_nfobject_enable(g_monitor_type[0][0]);
        nfui_nfobject_enable(g_monitor_type[1][0]);

        nfui_nfobject_disable(g_monitor_type[0][1]);
        nfui_nfobject_disable(g_monitor_type[1][1]); 

        nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[0][0], TRUE);
        nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[1][0], TRUE);    
        
        nfui_nfobject_disable(g_spot_edit[0]);
        nfui_nfobject_disable(g_spot_edit[1]);

        nfui_combobox_remove_all((NFCOMBOBOX*)g_monitor_resol[0]);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_AUTO);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_3840_2160);
#if !defined CHIP_NVT
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_2560_1600);
#endif
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_2560_1440);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1920_1080);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_1024);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_720);

    }
    else if (g_advDual_data.mode == 1)  // MAIN OSD + SPOT
    {
        nfui_nfobject_enable(g_monitor_type[0][0]);
        nfui_nfobject_enable(g_monitor_type[0][1]);      
        
        nfui_nfobject_enable(g_monitor_type[1][0]);
        nfui_nfobject_enable(g_monitor_type[1][1]);

        if (g_advDual_data.monitor_type[0] == 0)
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_MAIN);
            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[0][0], TRUE);
            nfui_nfobject_disable(g_spot_edit[0]);
        }
        else 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_SUB);
            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[0][1], TRUE);
            nfui_nfobject_enable(g_spot_edit[0]);
        }
        
        if (g_advDual_data.monitor_type[1] == 0)
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_MAIN);
            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[1][0], TRUE);      
            nfui_nfobject_disable(g_spot_edit[1]);            
        }
        else 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_SUB);
            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[1][1], TRUE);
            nfui_nfobject_enable(g_spot_edit[1]);
        }

        nfui_combobox_remove_all((NFCOMBOBOX*)g_monitor_resol[0]);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_AUTO);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1920_1080);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_1024);
        nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_720);       
    }

    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_monitor_resol[0], _get_disp_resol(g_advDual_data.monitor_resol[0]));
    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_monitor_resol[1], _get_disp_resol(g_advDual_data.monitor_resol[1]));

    if (expose)
    {
        nfui_signal_emit(g_display_mode, GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_monitor_image[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_image[1], GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_monitor_type[0][0]->parent, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_type[0][1]->parent, GDK_EXPOSE, TRUE);        
        nfui_signal_emit(g_monitor_type[1][0]->parent, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_type[1][1]->parent, GDK_EXPOSE, TRUE);      

        nfui_signal_emit(g_spot_edit[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_spot_edit[1], GDK_EXPOSE, TRUE);            

        nfui_signal_emit(g_monitor_resol[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_resol[1], GDK_EXPOSE, TRUE);
    }

    return 0;
}

static gboolean post_display_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint display_mode;
         
        display_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_advDual_data.mode = display_mode;
    
        if (display_mode == 0) // TIE
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_TIE);
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_TIE);    

            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[0][0]);
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[0][1]);
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[1][0]);
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[1][1]);

            nfui_nfobject_enable(g_monitor_type[0][0]);
            nfui_nfobject_enable(g_monitor_type[1][0]);

            nfui_nfobject_disable(g_monitor_type[0][1]);        
            nfui_nfobject_disable(g_monitor_type[1][1]);

            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[0][0], TRUE);
            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[1][0], TRUE);
            
            nfui_nfobject_disable(g_spot_edit[0]);
            nfui_nfobject_disable(g_spot_edit[1]);

            nfui_combobox_remove_all((NFCOMBOBOX*)g_monitor_resol[0]);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_AUTO);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_3840_2160);
#if !defined CHIP_NVT
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_2560_1600);
#endif
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_2560_1440);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1920_1080);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_1024);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_720);

            nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_AUTO);
            nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_monitor_resol[1], STR_RESOL_AUTO);

            g_advDual_data.monitor_type[0] = 0;
            g_advDual_data.monitor_type[1] = 0;
        }
        else if (display_mode == 1)  // MAIN OSD + SPOT
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_MAIN);            
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_SUB);
        
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[0][0]);
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[0][1]);
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[1][0]);
            nfui_radio_button_unset_toggled((NFBUTTON*)g_monitor_type[1][1]);

            nfui_nfobject_enable(g_monitor_type[0][0]);
            nfui_nfobject_enable(g_monitor_type[0][1]);    
            
            nfui_nfobject_enable(g_monitor_type[1][0]); 
            nfui_nfobject_enable(g_monitor_type[1][1]);  

            nfui_nfobject_disable(g_spot_edit[0]);
            nfui_nfobject_enable(g_spot_edit[1]);

            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[0][0], TRUE);
            nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[1][1], TRUE);

            nfui_combobox_remove_all((NFCOMBOBOX*)g_monitor_resol[0]);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_AUTO);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1920_1080);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_1024);
            nfui_combobox_append_data((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_1280_720);

            nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_monitor_resol[0], STR_RESOL_AUTO);
            nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_monitor_resol[1], STR_RESOL_AUTO);

            g_advDual_data.monitor_type[0] = 0;
            g_advDual_data.monitor_type[1] = 1;         
        }

        nfui_signal_emit(g_monitor_image[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_image[1], GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_monitor_type[0][0]->parent, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_type[0][1]->parent, GDK_EXPOSE, TRUE);        
        nfui_signal_emit(g_monitor_type[1][0]->parent, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_type[1][1]->parent, GDK_EXPOSE, TRUE);      

        nfui_signal_emit(g_spot_edit[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_spot_edit[1], GDK_EXPOSE, TRUE);            

        nfui_signal_emit(g_monitor_resol[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_resol[1], GDK_EXPOSE, TRUE);    
    }

    return FALSE;
}

static gboolean post_monitor1_type_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_RADIO_GET_FOCUS)
    {
        gint display_mode;
        gint index;
        
        display_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_display_mode);
        if (display_mode == 0) return FALSE;        

        index = nfui_radio_button_get_index((NFBUTTON*)obj);
        g_advDual_data.monitor_type[0] = index;

        if (index == 0) 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_MAIN);
            nfui_nfobject_disable(g_spot_edit[0]);
        }
        else 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_SUB);          
            nfui_nfobject_enable(g_spot_edit[0]);
        }
        
        nfui_signal_emit(g_monitor_image[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_spot_edit[0], GDK_EXPOSE, TRUE);

        index = (index == 0 ? 1 : 0);
        
        if (index == 0) 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_MAIN);
            nfui_nfobject_disable(g_spot_edit[1]);
        }
        else 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_SUB);   
            nfui_nfobject_enable(g_spot_edit[1]);
        }

        nfui_signal_emit(g_monitor_image[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_spot_edit[1], GDK_EXPOSE, TRUE);

        nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[1][index], TRUE);
        nfui_signal_emit(g_monitor_type[1][0]->parent, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_type[1][1]->parent, GDK_EXPOSE, TRUE);
        
        g_advDual_data.monitor_type[1] = index;
    }

    return FALSE;
}

static gboolean post_monitor2_type_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_RADIO_GET_FOCUS)
    {
        gint display_mode;
        gint index;
        
        display_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_display_mode);
        if (display_mode == 0) return FALSE;        

        index = nfui_radio_button_get_index((NFBUTTON*)obj);
        g_advDual_data.monitor_type[1] = index;

        if (index == 0) 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_MAIN);
            nfui_nfobject_disable(g_spot_edit[1]);
        }
        else 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[1], IMG_2ND_MONITOR_SUB);          
            nfui_nfobject_enable(g_spot_edit[1]);
        }
        
        nfui_signal_emit(g_monitor_image[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_spot_edit[1], GDK_EXPOSE, TRUE);

        index = (index == 0 ? 1 : 0);
        
        if (index == 0) 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_MAIN);
            nfui_nfobject_disable(g_spot_edit[0]);
        }
        else 
        {
            nfui_nfimage_change_image((NFIMAGE*)g_monitor_image[0], IMG_1ST_MONITOR_SUB);   
            nfui_nfobject_enable(g_spot_edit[0]);
        }

        nfui_signal_emit(g_monitor_image[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_spot_edit[0], GDK_EXPOSE, TRUE);

        nfui_radio_button_set_toggled((NFBUTTON*)g_monitor_type[0][index], TRUE);
        nfui_signal_emit(g_monitor_type[0][0]->parent, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_monitor_type[0][1]->parent, GDK_EXPOSE, TRUE);
        
        g_advDual_data.monitor_type[0] = index;
    }

    return FALSE;
}

static gboolean post_monitor1_resolution_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint display_mode;
        gchar *str_resol;
        
        display_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_display_mode);        
        str_resol = nfui_combobox_get_value((NFCOMBOBOX*)obj);
    }

    return FALSE;
}

static gboolean post_monitor2_resolution_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint display_mode;
        gchar *str_resol;
        
        display_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_display_mode);
        str_resol = nfui_combobox_get_value((NFCOMBOBOX*)obj);
    }

    return FALSE;
}

static gboolean post_spot_edit_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        SpotElementData elem_data[GUI_CHANNEL_CNT];
        guint num_items = 0;
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        num_items = g_advDual_data.spot_cnt;
        g_memmove(elem_data, g_advDual_data.spot, sizeof(SpotElementData)*GUI_CHANNEL_CNT);

        SpotConf_Open(g_curwnd, elem_data, &num_items, SPOT_SPLIT_CNT, DUAL_M_SPOT_OUTPUT_CH, DUAL_MONITOR);

        g_advDual_data.spot_cnt = num_items;
        g_memmove(g_advDual_data.spot, elem_data, sizeof(SpotElementData)*GUI_CHANNEL_CNT);
    }

    return FALSE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE :
            break;

        default :
            break;
    }

    return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
    if (evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable;
        GdkGC *gc;
        guint pos_x, pos_y;
        gint line_gap;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(115));
        gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

        nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
        
        gdk_draw_rectangle(drawable, gc, FALSE, pos_x + 348, pos_y + 103, 364, 1);
        gdk_draw_rectangle(drawable, gc, FALSE, pos_x + 732, pos_y + 103, 364, 1);
        gdk_draw_rectangle(drawable, gc, FALSE, pos_x + 348, pos_y + 464, 364, 1);
        gdk_draw_rectangle(drawable, gc, FALSE, pos_x + 732, pos_y + 464, 364, 1);
        
        nfui_nfobject_gc_unref(gc);
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
        g_memmove(&g_advDual_data, &g_org_advDual_data, sizeof(AdvDualData));
        _change_object(1);
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        mb_type ret;
        gint ddc_resol, retVal;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        retVal = _is_valid_monitor_param(0, &ddc_resol);
        if (retVal == 0)
        {
            ret = nftool_mbox(g_curwnd, "NOTICE", "The HDMI monitor is not connected or set to an unsupported resolution.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
            if (ret == NFTOOL_MB_CANCEL) return FALSE;
        }
        g_advDual_data.monitor_resol[0] = ddc_resol;

        retVal = _is_valid_monitor_param(1, &ddc_resol);
        if (retVal == 0)
        {
            ret = nftool_mbox(g_curwnd, "NOTICE", "The VGA monitor is not connected or set to an unsupported resolution.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
            if (ret == NFTOOL_MB_CANCEL) return FALSE;
        }
        g_advDual_data.monitor_resol[1] = ddc_resol;

        if (memcmp(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData)))
        {       
            if (_is_need_set_sysman_app_param() == 1)
            {
                NFOBJECT *save_pop = NULL;
                
                ret = nftool_mbox(g_curwnd, "NOTICE", "DualMonitor configurations has been changed.\nThe system will be reboot.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
                if (ret == NFTOOL_MB_CANCEL) return FALSE;

                g_memmove(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData));
                DAL_set_advanced_dual_data(&g_advDual_data);            
                
                scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
                save_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");
                DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);

                _proc_set_sysman_app_param();

                g_timeout_add(1000, _proc_escape, save_pop);                
            }
            else
            {
                g_memmove(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData));
                DAL_set_advanced_dual_data(&g_advDual_data);            
                
                scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
                nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
                sysdisp_set_changeflag(1);
            }
        }
    }
    else if (evt->type == IRET_SCM_DUAL_MONITOR_CHANGE)
    {
        scm_reboot_system(RR_NA, 1000);
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, IRET_SCM_DUAL_MONITOR_CHANGE);
    }
    
    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
        DispAdvancedDual_tab_out_handler();
        SystemSetupDisp_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}


void init_DispAdvancedDual_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *tmp_fixed;
    NFOBJECT *obj;

    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

    const gchar *strMode[] = {"DUPLICATE THESE DISPLAYS", "MAIN MONITOR + SPOT"};
    const gchar *strResol[] = {STR_RESOL_AUTO, STR_RESOL_1920_1080, STR_RESOL_1280_1024, STR_RESOL_1280_720};

    GSList *slist = NULL;

    guint size_w, size_h;
    gint pos_x, pos_y;    
    gint i;

    
    g_curwnd = nfui_nfobject_get_top(parent);

    memset(&g_advDual_data, 0x00, sizeof(AdvDualData));
    DAL_get_advanced_dual_data(&g_advDual_data);
    g_memmove(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData));


    radio_img[0] = nfui_get_image_from_file((IMG_N_RADIO_OFF), NULL);
    radio_img[1] = nfui_get_image_from_file((IMG_O_RADIO_ON), NULL);
    radio_img[2] = nfui_get_image_from_file((IMG_P_RADIO_ON), NULL);
    radio_img[3] = nfui_get_image_from_file((IMG_D_RADIO_OFF), NULL);   

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

    pos_x = TABLE_LEFT;
    pos_y = TABLE_TOP;

    obj = nfui_nflabel_new_with_pango_font("DISPLAY MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);      
    nfui_nffixed_put(content_fixed, obj, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("DISPLAY TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);      
    nfui_nffixed_put(content_fixed, obj, pos_x, pos_y+101+356);

    obj = nfui_nflabel_new_with_pango_font("DISPLAY RESOLUTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);      
    nfui_nffixed_put(content_fixed, obj, pos_x, pos_y+101+478);

    obj = nfui_combobox_new(strMode, 2, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+320, pos_y);
    nfui_regi_post_event_callback(obj, post_display_mode_event_handler);
    g_display_mode = obj;

    pos_x += 315;
    pos_y += 101;

    for (i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            obj = nfui_nfimage_new(IMG_HDMI_PORT_ICON);
            nfui_nfobject_show(obj);    
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

#if defined (_IPX_32P5)
            if (1 /*var_get_vendor_code() == 28*/)
                obj = nfui_nflabel_new_with_pango_font("HDMI 1", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
            else
                obj = nfui_nflabel_new_with_pango_font("HDMI", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
#else
            obj = nfui_nflabel_new_with_pango_font("HDMI", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
#endif
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_set_size(obj, 200, 40);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);      
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+80, pos_y);

            obj = nfui_nfimage_new(IMG_1ST_MONITOR_TIE);
            nfui_nfobject_show(obj);    
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y+43);
            g_monitor_image[i] = obj;
        }
        else if (i == 1)
        {
#if defined (_IPX_32P5)
            if (1 /*var_get_vendor_code() == 28*/)
                obj = nfui_nfimage_new(IMG_HDMI_PORT_ICON);
            else
                obj = nfui_nfimage_new(IMG_VGA_PORT_ICON);
#else
            obj = nfui_nfimage_new(IMG_VGA_PORT_ICON);
#endif
            nfui_nfobject_show(obj);    
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

#if defined (_IPX_32P5)
            if (1 /*var_get_vendor_code() == 28*/)
                obj = nfui_nflabel_new_with_pango_font("HDMI 2", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
            else
                obj = nfui_nflabel_new_with_pango_font("VGA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
#else
            obj = nfui_nflabel_new_with_pango_font("VGA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
#endif
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_set_size(obj, 200, 40);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);      
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+80, pos_y);

            obj = nfui_nfimage_new(IMG_2ND_MONITOR_TIE);
            nfui_nfobject_show(obj);    
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y+43);
            g_monitor_image[i] = obj;
        }

        pos_x += 10;

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(tmp_fixed, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(tmp_fixed);
        nfui_nffixed_put((NFFIXED*)content_fixed, tmp_fixed, pos_x, pos_y+356+(40-size_h)/2);

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 0, 0);
        g_monitor_type[i][0] = obj;
        
        slist = nfui_radio_button_get_group(NF_BUTTON(obj));

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MAIN MONITOR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+40, pos_y+356);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(tmp_fixed, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(tmp_fixed);
        nfui_nffixed_put((NFFIXED*)content_fixed, tmp_fixed, pos_x, pos_y+398+(40-size_h)/2);

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 0, 0);
        g_monitor_type[i][1] = obj;     

        nfui_radio_button_add_group(NF_BUTTON(obj), slist);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SPOT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, 120, 40);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+40, pos_y+398);

        obj = nftool_normal_button_create_type3("EDIT", 120);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
        nfui_nfobject_show(obj);    
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+180, pos_y+398);
        g_spot_edit[i] = obj;

        obj = nfui_combobox_new(strResol, 4, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
        nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_LEFT, 10);
        nfui_nfobject_set_size(obj, 364, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y+478);
        g_monitor_resol[i] = obj;

        pos_x += 379;
    }

    nfui_regi_post_event_callback(g_monitor_type[0][0], post_monitor1_type_event_handler);
    nfui_regi_post_event_callback(g_monitor_type[0][1], post_monitor1_type_event_handler);
    nfui_regi_post_event_callback(g_monitor_type[1][0], post_monitor2_type_event_handler);
    nfui_regi_post_event_callback(g_monitor_type[1][1], post_monitor2_type_event_handler);

    nfui_regi_post_event_callback(g_monitor_resol[0], post_monitor1_resolution_event_handler);
    nfui_regi_post_event_callback(g_monitor_resol[1], post_monitor2_resolution_event_handler);

    nfui_regi_post_event_callback(g_spot_edit[0], post_spot_edit_event_handler);
    nfui_regi_post_event_callback(g_spot_edit[1], post_spot_edit_event_handler);
    
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
    uxm_reg_imsg_event(obj, IRET_SCM_DUAL_MONITOR_CHANGE);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
    nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);    
    nfui_regi_post_event_callback(parent, post_page_event_handler);

    _change_object(0);
}

gboolean DispAdvancedDual_tab_out_handler()
{
    mb_type ret;
    gint ddc_resol, retVal;

    retVal = _is_valid_monitor_param(0, &ddc_resol);
    if (retVal == -1)
    {
        ret = nftool_mbox(g_curwnd, "NOTICE", "The HDMI monitor is not connected or set to an unsupported resolution.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) 
        {
            g_memmove(&g_advDual_data, &g_org_advDual_data, sizeof(AdvDualData));
            _change_object(0);            
            return FALSE;
        }
    }
    g_advDual_data.monitor_resol[0] = ddc_resol;

    retVal = _is_valid_monitor_param(1, &ddc_resol);
    if (retVal == -1)
    {
        ret = nftool_mbox(g_curwnd, "NOTICE", "The VGA monitor is not connected or set to an unsupported resolution.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) 
        {
            g_memmove(&g_advDual_data, &g_org_advDual_data, sizeof(AdvDualData));
            _change_object(0);            
            return FALSE;
        }
    }
    g_advDual_data.monitor_resol[1] = ddc_resol;

    if (!memcmp(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData))) return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if (ret == NFTOOL_MB_OK)
    {   
        if (_is_need_set_sysman_app_param() == 1)
        {        
            NFOBJECT *save_pop = NULL;
            
            ret = nftool_mbox(g_curwnd, "NOTICE", "DualMonitor configurations has been changed.\nThe system will be reboot.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
            if (ret == NFTOOL_MB_CANCEL) 
            {
                g_memmove(&g_advDual_data, &g_org_advDual_data, sizeof(AdvDualData));
                _change_object(0);            
                return FALSE;
            }

            g_memmove(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData));
            DAL_set_advanced_dual_data(&g_advDual_data);            
            
            scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
            save_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");
            DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
            
            _proc_set_sysman_app_param();

            g_timeout_add(1000, _proc_escape, save_pop);
            gtk_main();
        }
        else
        {
            g_memmove(&g_org_advDual_data, &g_advDual_data, sizeof(AdvDualData));
            DAL_set_advanced_dual_data(&g_advDual_data);            
            
            scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
            sysdisp_set_changeflag(1);
        }
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(&g_advDual_data, &g_org_advDual_data, sizeof(AdvDualData));
        _change_object(0);
    }

    return FALSE;
}

