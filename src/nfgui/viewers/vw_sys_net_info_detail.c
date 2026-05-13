#include <string.h>

#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"

#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"

#include "tools/nf_ui_tool.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_info.h"
#include "vw_sys_net_info_detail.h"
#include "vw_sys_net_info_detail_popup.h"

#include <nf_api_ipcam.h>

#define SYS_DETAIL_TAB_PAGE_WIDTH       1550
#define SYS_DETAIL_TAB_PAGE_HEIGHT      887

#define STATUS_NORMAL_COLOR             (COLOR_IDX(139))
#define STATUS_ERROR_COLOR              (COLOR_IDX(190))



static GdkGC *gc;
static GdkDrawable *drawable;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *listlbl_obj[GUI_CHANNEL_CNT+1][4];    // LIST
static NFOBJECT *configbtns_obj[GUI_CHANNEL_CNT];   // DELETE ITEM BUTTONS

static gint selrow = 0;
static gint mt_level = 0;

static NFIPCamPortStatus cam_stats[GUI_CHANNEL_CNT];

static void 
get_cam_status()
{
    int i;
    GError *err = NULL;
    char data_stats[4][256];
    guint font_color;

    memset(cam_stats, 0x00, sizeof(cam_stats));
    
    for(i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        nf_ipcam_get_port_status(i, &cam_stats[i], &err);

        // printf("[%d] model : %d %s - %s , status : %d , detail : %s \n"
        //          , i
        //          , cam_stats[i].device_class, cam_stats[i].vendor, cam_stats[i].model
        //          , cam_stats[i].status
        //          , cam_stats[i].detail);

        memset(data_stats, 0x00, sizeof(data_stats));

        var_get_camtitle(data_stats[0], i);
        if(cam_stats[i].device_class != 0)
        {       
            if(cam_stats[i].status == 0)
            {
                g_sprintf(data_stats[1]," %s %s", cam_stats[i].vendor, cam_stats[i].model);
                g_sprintf(data_stats[2], "OK");
                font_color = STATUS_NORMAL_COLOR;
            }
            else
            {
                g_sprintf(data_stats[1]," NONE");
                g_sprintf(data_stats[2], "FAIL");
                g_sprintf(data_stats[3], " %s", cam_stats[i].detail);               
                font_color = STATUS_ERROR_COLOR;
            }
        }
        else
        {
            g_sprintf(data_stats[1]," NONE");
            g_sprintf(data_stats[2], "N/A");
            font_color = STATUS_NORMAL_COLOR;
        }
        
        nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i+1][0], data_stats[0]);
        nfui_nflabel_set_align((NFLABEL*)listlbl_obj[i+1][0], NFALIGN_LEFT, 2);
        nfui_signal_emit(listlbl_obj[i+1][0], GDK_EXPOSE, TRUE);

        nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i+1][1], data_stats[1]);
        nfui_nflabel_set_align((NFLABEL*)listlbl_obj[i+1][1], NFALIGN_LEFT, 2);
        nfui_signal_emit(listlbl_obj[i+1][1], GDK_EXPOSE, TRUE);

        nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i+1][2], data_stats[2]);
        nfui_nflabel_set_align((NFLABEL*)listlbl_obj[i+1][2], NFALIGN_LEFT, 2);
        nfui_nflabel_modify_fg((NFLABEL*)listlbl_obj[i+1][2], font_color);
        nfui_signal_emit(listlbl_obj[i+1][2], GDK_EXPOSE, TRUE);

        nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i+1][3], data_stats[3]);
        nfui_nflabel_set_align((NFLABEL*)listlbl_obj[i+1][3], NFALIGN_LEFT, 2);
        nfui_nflabel_modify_fg((NFLABEL*)listlbl_obj[i+1][3], font_color);
        nfui_signal_emit(listlbl_obj[i+1][3], GDK_EXPOSE, TRUE);
        
    }
}

static void 
detail_draw_network_detail(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc)
{
    guint x, y;
    guint xpos, ypos;
    
    nfui_nfobject_get_offset(obj, &x, &y);
}

static gboolean 
detail_page_parent_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    
    if(evt->type == GDK_EXPOSE) 
    {
        gint i;
        
        drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
        gc = nfui_nfobject_get_gc(obj);
                    
        detail_draw_network_detail(obj, drawable, gc);
                
        nfui_nfobject_gc_unref(gc); 

    }
    else if(evt->type == GDK_DELETE) 
    {
        //if(isReloaded)
        //  isReloaded = FALSE;
    }
    
    return FALSE;
}


static gboolean 
detail_configbtns_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, sel_item;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        for(i = 0 ; i < GUI_CHANNEL_CNT; i++)
        {
            if(configbtns_obj[i] == obj)
            {
                sel_item = i;
                break;
            }
        }

        if(cam_stats[sel_item].device_class != 0 && cam_stats[sel_item].status == 0)
        {
            open_vw_sys_net_detail_popup(g_curwnd, sel_item);
        }
        else
        {
            nftool_mbox(g_curwnd, "NOTICE", "Please check the camera.", 
                                NFTOOL_MB_OK);
        }
    }
    
    return FALSE;
}

static gboolean 
detail_post_refresh_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        printf("REFRESH GDK_BUTTON_RELEASE \n");
        get_cam_status();
    }
    
    return FALSE;
}

static gboolean 
detail_post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        
    }
    
    return FALSE;
}

static gboolean 
detail_post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        
    }
    
    return FALSE;
}

static gboolean 
detail_post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        SystemSetupNetwork_Destroy(obj);
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


/////////////////////////////////////////////////////////////////////
//
//
//

void vw_init_NetInfoDetail_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *scrolled_fixed;
    NFOBJECT *obj;

    const gchar *listTitlelbl[] = {"CHANNEL", "MODEL", "STATUS", "DETAIL"};
    gint list_xpos[4] = {0, 260, 620, 760};
    gint list_width[4] = {258, 358, 138, 600};
    gchar list_tmp[20];
    gint ypos = 0;
    gint i, j;  

	gint scrolledfixed_h;
	gint scrolledfixed_w = 0;

    g_curwnd = nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

    scrolledfixed_h = 17 * 42;

    for (i = 0; i < 4; i++)
        scrolledfixed_w += list_width[i];

    scrolledfixed_w += 142;

	scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
	nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, scrolledfixed_h/3, 80, scrolledfixed_h/5);
	nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_8594A6)));
	nfui_nfobject_set_size(scrolled_fixed, scrolledfixed_w, scrolledfixed_h);
	nfui_nfobject_show(scrolled_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, scrolled_fixed, 0, 42);
	
    for( j = 0 ; j < 4 ; j++ )
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(listTitlelbl[j], 
                            nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, list_xpos[j], 0);     
        nfui_nfobject_set_size(obj, list_width[j], 40);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, list_xpos[j], 0); 
        
        listlbl_obj[0][j] = obj;
    }

    ypos = 0;
    
    for( i = 1 ; i < GUI_CHANNEL_CNT+1 ; i++ )
    {
        for( j = 0 ; j < 4 ; j++ )
        {
            obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
            nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
            nfui_nfobject_set_size(obj, list_width[j], 40);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, obj, list_xpos[j], ypos);
            listlbl_obj[i][j] = obj;
        }
        
        nfui_nfobject_support_multi_lang(listlbl_obj[i][0], FALSE);
        ypos += 42;
    }

// CONFIG ITEM BUTTONS
    guint size_w, size_h;
    GdkPixbuf *config_btn_img[NFOBJECT_STATE_COUNT];

    config_btn_img[0] = nfui_get_image_from_file(IMG_BT_SUB_SETTING_N, NULL);
    config_btn_img[1] = nfui_get_image_from_file(IMG_BT_SUB_SETTING_O, NULL);
    config_btn_img[2] = nfui_get_image_from_file(IMG_BT_SUB_SETTING_P, NULL);
    config_btn_img[3] = nfui_get_image_from_file(IMG_BT_SUB_SETTING_D, NULL);
    nfui_get_pixbuf_size(config_btn_img[0], &size_w, &size_h);

    ypos = 0;

    for( i = 0 ; i < GUI_CHANNEL_CNT; i++ )
    {
        obj = (NFOBJECT*)nfui_nfbutton_new();
        configbtns_obj[i] = obj;
        nfui_nfbutton_set_image(NF_BUTTON(obj), config_btn_img);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, obj, 1362, ypos);
        nfui_regi_post_event_callback(obj, detail_configbtns_btn_handler);
        
        ypos += 42;
    }

// DATA MANGE
    get_cam_status();

    obj = nftool_normal_button_create_subtab_type1("REFRESH", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, 1260, 825);
    nfui_regi_post_event_callback(obj, detail_post_refresh_button_event_handler);


// BELOW BUTTONS
    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, detail_post_cancel_button_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, detail_post_apply_button_event_handler);
    
    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, detail_post_close_button_event_handler);

    nfui_regi_pre_event_callback(content_fixed, detail_page_parent_event_handler);
    
    nfui_regi_post_event_callback(parent, post_page_event_handler);
}




