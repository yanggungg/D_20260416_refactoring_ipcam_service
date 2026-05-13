#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"

#include "ix_mem.h"
#include "scm.h"

#include "vw_wizard_init.h"
#include "vw_wizard_network_onestopsetup.h"

#define MAX_MARGIM_SIZE             (guint)12

#define PI_WND_SIZE_WID             (guint)(WIZARD_SIZE_WID)
#define PI_WND_SIZE_HEI             (guint)(WIZARD_SIZE_HEI)

#define SE_PP_POS_X                 (guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y                 (guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X              (guint)(12)
#define PI_FIXED_POS_Y              (guint)(56)
#define PI_FIXED_SIZE_WID           (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI           (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH              (162)
#define MENU_BTN_HEIGHT             (44)
#define MENU_BTN_GAP                (4)

#define MENU_V_BTN_R_START_X        (PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X             (MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X             (MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y                (PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)


#define IPS_LABEL_HEIGHT            (40)
#define IPS_LABEL_ROW_SPACE         (2)
#define SUBJECT_LABEL_LEFT          (28)
#define SUBJECT_LABEL_TOP           (42)
#define SUBJECT_LABEL_WIDTH         (260)
#define SUBJECT_LABEL_MARGIN        (0)

enum {
    PIB_PREVIOUS,
    PIB_NEXT,
    PIB_EXIT,
    PIB_BUTTONS
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *p2p_obj;

static WIZARD_USERDATA_T *g_wizard_data;
static DDNSData ddnsdata;
static DDNSData org_ddnsdata;


static gint _exit_proc()
{
    nfui_nfobject_destroy(g_curwnd);    
    _wizard_finish();

    return 0;
}

static gint _next_step_proc()
{
    nfui_nfobject_destroy(g_curwnd);
    _wizard_next_step(1);

    return 0;
}

static gint _prev_proc()
{
    nfui_nfobject_destroy(g_curwnd);
    _wizard_prev_step(1);

    return 0;   
}

static gboolean _is_connected_cable()
{   
    NF_NOTIFY_INFO info;
    
    if(!scm_get_net_rxtx_status_data(&info)) 
    {
        if (!info.d.params[3]) return FALSE;
    }
    
    return TRUE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;  
    GdkDrawable *drawable;
    
    if(evt->type == GDK_EXPOSE) 
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_gc_unref(gc);
    }   
    
    return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
        gtk_main_quit();
    }

    return FALSE;
}

static gboolean post_exitbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
        {
            return FALSE;
        }
        _exit_proc();
    }

    return FALSE;
}

static gboolean post_previousbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        _prev_proc();
    }

    return FALSE;
}
    
static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        mb_type ret;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        ddnsdata.p2p_enable = nfui_radio_button_get_toggled((NFBUTTON*)p2p_obj);
        
        if(!_is_connected_cable())
        {
            ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
            if(ret == NFTOOL_MB_OK) return FALSE;
        }

        if(memcmp(&org_ddnsdata, &ddnsdata, sizeof(DDNSData)))
        {
            DAL_set_ddns_data(&ddnsdata);                               
            g_memmove(&org_ddnsdata, &ddnsdata, sizeof(DDNSData));

        	g_memmove(&g_wizard_data->networkData.ddns_data, &ddnsdata, sizeof(DDNSData));            
        }
        
        _next_step_proc();      
    }

    return FALSE;
}

static gboolean post_p2p_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS)
        ddnsdata.p2p_enable = nfui_radio_button_get_toggled((NFBUTTON*)p2p_obj);

    return FALSE;
}

gint vw_wizard_network_onestopsetup_open(gpointer parent, gpointer user_data)
{
    NFOBJECT *main_wnd, *main_fixed;
    NFOBJECT *fixed1;
    NFOBJECT *obj;
    NFOBJECT *btns[PIB_BUTTONS];
    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    GSList *slist = NULL;

    const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};
    const gchar *strTitle[] = {"Disable", "Enable"};

    guint pos_x, pos_y;
    gint size_w, size_h;
    guint i, ret;

    char title[64];


    g_wizard_data = (WIZARD_USERDATA_T*)user_data;


    memset(&ddnsdata, 0x00, sizeof(DDNSData));
    memset(&org_ddnsdata, 0x00, sizeof(DDNSData));

    g_memmove(&ddnsdata, &g_wizard_data->networkData.ddns_data, sizeof(DDNSData));      
    g_memmove(&org_ddnsdata, &g_wizard_data->networkData.ddns_data, sizeof(DDNSData));
  
    main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
    nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
    nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    fixed1 = nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
    nfui_nfobject_show(fixed1);

    pos_x = (guint)4;
    pos_y = (guint)4;

    obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
    nfui_nfimage_set_text((NFIMAGE*)obj, "ONESTOP SERVICE SETTING");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    /* radio button */
    radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
    radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
    radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
    radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    pos_x += (guint)28;
    pos_y += (guint)60;

    for(i=0; i<2; i++) {
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);

        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_p2p_onoff_event_handler);

        if(i == 0) {
            slist = nfui_radio_button_get_group(NF_BUTTON(obj));
                
            if(!ddnsdata.p2p_enable){
                nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
                }
        } else {
            pos_y += (guint)size_h + 20;
            nfui_radio_button_add_group(NF_BUTTON(obj), slist);
            p2p_obj = obj;
                            
            if(ddnsdata.p2p_enable)
                {
                nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
                }
        }
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x,pos_y);
        
        /* label */
        if(i == 0)
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        else
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nfobject_set_size(obj, 300, 27);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);

        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + 50, pos_y);
            
        /* label */
        if(i == 0){
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        }
        else
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nfobject_set_size(obj, 100, 27);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);

        if(i == 0) nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + 50, pos_y);
        else       nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + 50, pos_y);
    }

    for( i=0; i<PIB_BUTTONS; i++ )
    {
        btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);    
        nfui_nfobject_show(btns[i]);
    }
    
    nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

    nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
    nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
    nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
    nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
    nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

    nfui_nfobject_show(main_wnd);

    /* set for key navi */
    nfui_make_key_hierarchy(main_wnd);
    nfui_set_key_focus(btns[PIB_NEXT], TRUE);

    gtk_main();

    return 0;
}

