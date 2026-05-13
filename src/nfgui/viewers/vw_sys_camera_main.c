
#include "nf_afx.h"

#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/event_loop.h"

#include "nf_api_live.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_title.h"
#include "vw_sys_camera_image.h"
#include "vw_sys_camera_covert.h"
#include "vw_sys_camera_motion.h"
#include "vw_sys_camera_ptz.h"
#include "vw_sys_camera_fisheye.h"
#include "vw_sys_camera_install_mode.h"
#include "vw_sys_camera_ipcam_install.h"

#include "tools/nf_ui_tool.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfwindow.h"

#include "iux_afx.h"
#include "vsm.h"
#include "vw_menu.h"

#include "vw_sys_camera_vca.h"
#include "vw_sys_camera_vca_rev.h"
#include "vw_sys_camera_dvabx.h"
#include "vw_sys_camera_analysis.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE		"LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static guint change_flag = 0;
static guint act_change_flag = 0;
static guint unit_change_flag = 0;


#if 1
static void _init_cam_main2(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TITLE]; 
    if (pos == page) 
    {
        DMSG(1, "init_CamSetup_page");
    
        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_CamSetup_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_IMAGE_SETUP];      
    if (pos == page) 
    {
        DMSG(1, "init_ColorSetup_page");
    
        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_ColorSetup_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_IMAGE_SETUP];        
    if (pos == page) 
    {
        DMSG(1, "init_IPCamSetup_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_IPCamSetup_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_COVERT_SETUP];     
    if (pos == page) 
    {
        DMSG(1, "init_CamCovert_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_CamCovert_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_MOTION_SENSOR];        
    if (pos == page) 
    {
        DMSG(1, "init_MotSen_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_MotSen_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_MOTION_SENSOR];      
    if (pos == page) 
    {
        DMSG(1, "init_IPCam_MotSen_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_IPCam_MotSen_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TYPE];      
#if defined(_HDI_MODEL_UX) || defined(_HDY_MODEL_UX)
    if (pos == page) 
    {
        DMSG(1, "init_CamType_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_CamType_page(((NFTAB*)nftab)->page[pos]);   
    }
#endif

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP];        
    if (pos == page) 
    {
        DMSG(1, "init_PtzSetup_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_PtzSetup_page(((NFTAB*)nftab)->page[pos]);  
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_PRIVACY_MASK];     
    if (pos == page) 
    {
        DMSG(1, "init_PrivacyMask_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_PrivacyMask_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_FISHEYE_SETUP];     
    if (pos == page) 
    {
        DMSG(1, "init_FisheyeSetup_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_FisheyeSetup_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_INSTALL_MODE];     
    if (pos == page) 
    {
        DMSG(1, "init_InstallMode_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  init_InstallMode_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_VCA_SETUP_S1];     
    if (pos == page) 
    {
        DMSG(1, "VW_Init_S1_VCA_Main_Page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  VW_Init_S1_VCA_Main_Page(((NFTAB*)nftab)->page[pos]);   
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_ANALYSIS_SETUP_ITX];     
    if (pos == page) 
    {
        DMSG(1, "VW_analysis_init_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children))  VW_analysis_init_page(((NFTAB*)nftab)->page[pos]);   
    }    

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_IPCAMERA_INSTALL];     
    if (pos == page) 
    {
        DMSG(1, "init_IPCamInstall_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children)) init_IPCamInstall_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_SPECIAL_CAM_SETUP];     
    if (pos == page) 
    {
        DMSG(1, "VW_init_Special_Cam_Setup_page");

        if (!(((NFFIXED*)((NFTAB*)nftab)->page[pos])->children)) VW_init_Special_Cam_Setup_page(((NFTAB*)nftab)->page[pos]);   
    }
}
#else
static void _init_cam_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TITLE]; 
    if (pos != -1) 
    {
        DMSG(1, "init_CamSetup_page");
    
        init_CamSetup_page(((NFTAB*)nftab)->page[pos]);
        if(page == pos) CamSetup_start_preview();
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_IMAGE_SETUP];      
    if (pos != -1) 
    {
        DMSG(1, "init_ColorSetup_page");
    
        init_ColorSetup_page(((NFTAB*)nftab)->page[pos]);
        if(page == pos) ColorSetup_start_preview();
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_IMAGE_SETUP];        
    if (pos != -1) 
    {
        DMSG(1, "init_IPCamSetup_page");

        init_IPCamSetup_page(((NFTAB*)nftab)->page[pos]);
        if(page == pos) IPCamSetup_start_preview();
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_COVERT_SETUP];     
    if (pos != -1) 
    {
        DMSG(1, "init_CamCovert_page");

        init_CamCovert_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_MOTION_SENSOR];        
    if (pos != -1) 
    {
        DMSG(1, "init_MotSen_page");

        init_MotSen_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_MOTION_SENSOR];      
    if (pos != -1) 
    {
        DMSG(1, "init_IPCam_MotSen_page");

        init_IPCam_MotSen_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TYPE];      
#if defined(_HDI_MODEL_UX) || defined(_HDY_MODEL_UX)
    if (pos != -1) 
    {
        DMSG(1, "init_CamType_page");

        init_CamType_page(((NFTAB*)nftab)->page[pos]);   
    }
#endif

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP];        
    if (pos != -1) 
    {
        DMSG(1, "init_PtzSetup_page");
        init_PtzSetup_page(((NFTAB*)nftab)->page[pos]);  
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_PRIVACY_MASK];     
    if (pos != -1) 
    {
        DMSG(1, "init_PrivacyMask_page");

        init_PrivacyMask_page(((NFTAB*)nftab)->page[pos]);
        if(page == pos) PrivacyMask_Area_Show_Preview();
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_FISHEYE_SETUP];     
    if (pos != -1) 
    {
        DMSG(1, "init_FisheyeSetup_page");

        init_FisheyeSetup_page(((NFTAB*)nftab)->page[pos]);
        if(page == pos) FisheyeSetup_Show_Preview();
    }    

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_INSTALL_MODE];     
    if (pos != -1) 
    {
        DMSG(1, "init_InstallMode_page");

        init_InstallMode_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_VCA_SETUP_S1];     
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_S1_VCA_Main_Page");

        VW_Init_S1_VCA_Main_Page(((NFTAB*)nftab)->page[pos]);   
        if(page == pos) VW_S1_VCA_start_preview();      
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_ANALYSIS_SETUP_ITX];     
    if (pos != -1) 
    {
        DMSG(1, "VW_analysis_init_page");

        VW_analysis_init_page(((NFTAB*)nftab)->page[pos]);   
    }    

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_IPCAMERA_INSTALL];     
    if (pos != -1) 
    {
        DMSG(1, "init_IPCamInstall_page");

        init_IPCamInstall_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub1.menu_pos[SYS_SUB1_SPECIAL_CAM_SETUP];     
    if (pos != -1) 
    {
        DMSG(1, "VW_init_Special_Cam_Setup_page");

        VW_init_Special_Cam_Setup_page(((NFTAB*)nftab)->page[pos]);   
    }
}
#endif

static void _in_handler_cam_main(gint page)
{
    if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TITLE])
    {
        DMSG(1, "CamTitle_tab_in_handler");
        
        CamTitle_tab_in_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_IMAGE_SETUP])
    {
        DMSG(1, "ColorSetup_tab_in_handler");
        
        ColorSetup_tab_in_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_IMAGE_SETUP])
    {
        DMSG(1, "IPCamSetup_tab_in_handler");    
        
        IPCamSetup_tab_in_handler();
    }    
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_COVERT_SETUP])
    {
        DMSG(1, "CamCovert_tab_in_handler");    
        
        CamCovert_tab_in_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_MOTION_SENSOR])
    {
        DMSG(1, "MotSen_tab_in_handler");
        
        MotSen_tab_in_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_MOTION_SENSOR])
    {
        DMSG(1, "IPCam_MotSen_tab_in_handler");
    
        if(IPCam_MotSen_cur_tab_page() == 1) MotSen_Show_Preview();
    }    
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TYPE])
    {
        DMSG(1, "CamType_tab_in_handler");
    
#if defined(_HDI_MODEL_UX) || defined(_HDY_MODEL_UX)    
        CamType_tab_in_handler();
#endif        
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP])
    {
        DMSG(1, "PtzSetup_tab_in_handler");
        PtzSetup_tab_in_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_PRIVACY_MASK])
    {
        DMSG(1, "PrivacyMask_tab_in_handler");
        PrivacyMask_Area_Show_Preview();
        PrivacyMask_tab_in_handler();
    }  
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_FISHEYE_SETUP])
    {
        DMSG(1, "FisheyeSetup_tab_in_handler");

        FisheyeSetup_tab_in_handler();
    }    
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_INSTALL_MODE])
    {
        DMSG(1, "InstallMode_tab_in_handler");

        InstallMode_tab_in_handler();
    }     
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_VCA_SETUP_S1])
    {
        DMSG(1, "VW_S1_VCA_Main_tab_in_handler");

        VW_S1_VCA_Main_tab_in_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ANALYSIS_SETUP_ITX])
    {
        DMSG(1, "VW_analysis_tab_in_handler");

        VW_analysis_tab_in_handler();
    }     
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_IPCAMERA_INSTALL])
    {
        DMSG(1, "IPCamInstall_tab_in_handler");

        IPCamInstall_tab_in_handler();
    }     
}

static gint _out_handler_cam_main(gint page)
{
    if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TITLE])
    {
        DMSG(1, "CamTitle_tab_out_handler");
    
        CamTitle_tab_out_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_IMAGE_SETUP])
    {
        DMSG(1, "ColorSetup_tab_out_handler");

        ColorSetup_tab_out_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_IMAGE_SETUP])
    {
        DMSG(1, "IPCamSetup_tab_out_handler");

        IPCamSetup_tab_out_handler();
    }   
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_COVERT_SETUP])
    {
        DMSG(1, "CamCovert_tab_out_handler");

        CamCovert_tab_out_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_MOTION_SENSOR])
    {
        DMSG(1, "MotSen_tab_out_handler");

        MotSen_tab_out_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_MOTION_SENSOR])
    {
        if (check_motsen_block_cnt() == 0) return -1;
        //if (check_motsen_detect_object() == 0) return -1;
    
        DMSG(1, "IPCam_MotSen_tab_out_handler");
        
        MotSen_Pause_Preview();
        IPCam_MotSen_tab_out_handler();
    }   
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TYPE])
    {
        DMSG(1, "CamType_tab_out_handler");
    
#if defined(_HDI_MODEL_UX) || defined(_HDY_MODEL_UX)    
        CamType_tab_out_handler();
#endif        
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP])
    {
        DMSG(1, "PtzSetup_tab_out_handler");
        PtzSetup_tab_out_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_PRIVACY_MASK])
    {
        DMSG(1, "PrivacyMask_tab_out_handler");
        PrivacyMask_Area_Stop_Preview();
        PrivacyMask_tab_out_handler();
    }   
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_FISHEYE_SETUP])
    {
        DMSG(1, "FisheyeSetup_tab_out_handler");
    
        FisheyeSetup_tab_out_handler();
    }    
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_INSTALL_MODE])
    {
        DMSG(1, "InstallMode_tab_out_handler");

        InstallMode_tab_out_handler();
    }    
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_IPCAMERA_INSTALL])
    {
        DMSG(1, "IPCamInstall_tab_out_handler");
    
        IPCamInstall_tab_out_handler();
    }        
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_VCA_SETUP_S1])
    {
        DMSG(1, "VW_S1_VCA_Main_tab_out_handler");
        
        VW_S1_VCA_Main_tab_out_handler();
    }
    else if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ANALYSIS_SETUP_ITX])
    {
        DMSG(1, "VW_analysis_tab_out_handler");
    
        VW_analysis_tab_out_handler();
    }
    else if (page = mcf.sys_sub1.menu_pos[SYS_SUB1_SPECIAL_CAM_SETUP])
    {
        VW_Special_Cam_Tab_Out_Handler();
    }

    return 0;
}

static void _changed_handler_cam_main(gint page)
{
    if (page == mcf.sys_sub1.menu_pos[SYS_SUB1_ANALYSIS_SETUP_ITX])
    {
        DMSG(1, "VW_analysis_tab_changed_handler");

        VW_analysis_tab_changed_handler();
    }     

    return 0;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    NFOBJECT *nftab = NULL;
    gint cur_page;

    nftab = nftool_get_nftab_from_setup_window(top);
    if(!nftab)  return FALSE;

    cur_page = nfui_nftab_get_cur_page((NFTAB*)nftab);

    if(cur_page < 0 || cur_page >= ((NFTAB*)nftab)->pages)
        return FALSE;

    _out_handler_cam_main(cur_page);

    return TRUE;
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int edited = 0;
    if(evt->type == GDK_DELETE)
    {
        edited = syscam_get_changeflag();

        if(edited & 0x01) {
            DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);
        }

        if(edited & 0x02) {
            DAL_save_setup_db(NFSETUP_WINDOW_USER);
        }

        if (sysact_get_changeflag())
            DAL_save_setup_db(NFSETUP_WINDOW_EVENT);
            
        vsm_live_start();

        g_curwnd = 0;

        // SKSHIN
        gtk_main_quit();
    }
    else if(evt->type == WND_PRE_CLOSE) {
        vsm_live_preview_stop();
    }
    
    return FALSE;
}

static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint cur_page;
    gint new_page;
    gint ret;

    switch(evt->type)
    {
        case NFEVENT_TAB_BEFORE_CHANGE:
            cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
            new_page = nfui_nftab_get_new_page((NFTAB*)obj);

            if(cur_page == new_page)    return FALSE;

            ret = _out_handler_cam_main(cur_page);
            if (ret == -1) {
                nfui_nftab_set_new_page((NFTAB*)obj, cur_page);
                return FALSE;
            }
            _init_cam_main2(obj, new_page);
            _in_handler_cam_main(new_page);
        break;

        case NFEVENT_TAB_CHANGED:
            new_page = nfui_nftab_get_new_page((NFTAB*)obj);
            _changed_handler_cam_main(new_page);
        break;

        default:
        break;
    }

    return FALSE;
}

void syscam_set_changeflag(guint flag)
{
    change_flag = flag;
}

guint syscam_get_changeflag()
{
    return change_flag;
}

void sysact_set_changeflag(guint flag)
{
    act_change_flag = flag;
}

guint sysact_get_changeflag()
{
    return act_change_flag;
}

void set_unit_change_yard(guint flag)
{
    unit_change_flag = flag;
}
guint get_unit_change_yard()
{
    return unit_change_flag;
}


void SystemSetupCam_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page) 
{
    NFOBJECT *cam_wnd, *fixed, *nftab;

    DMSG(1, "SystemSetupCam_Open");

    vsm_live_stop();    
    syscam_set_changeflag(0);
    sysact_set_changeflag(0);

    cam_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_CAMERA, page);
    g_curwnd = cam_wnd;
    nfui_nfwindow_set_title(cam_wnd, "SYSTEM SETUP - CAMERA");
    nftab = nftool_get_nftab_from_setup_window(cam_wnd);
    nfui_nftab_set_cur_page((NFTAB*)nftab, page ); 

    _init_cam_main2(nftab, page);
    _in_handler_cam_main(page);
    
    nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
    nfui_regi_pre_event_callback(cam_wnd, pre_main_wnd_event_handler);
    nfui_nfwindow_set_returnkey_proc(cam_wnd, returnkey_proc);

    nfui_nfobject_show(cam_wnd);
    nfui_make_key_hierarchy(cam_wnd);

    nfui_set_key_focus(nftab, TRUE);

    gtk_main();

    DMSG(1, "SystemSetupCam_Open EXIT");	    
}



void SystemSetupCam_Destroy(NFOBJECT *obj)
{
    NFOBJECT *topwin = NULL;

    topwin = nfui_nfobject_get_top(obj);
    if(topwin) 
        nftool_destroy_setup_window(topwin);
}



