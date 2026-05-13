#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_sys_net_main.h"

#include "vw_vkeyboard.h"
#include "vw_vkeyboard_str_size.h"

#include "scm.h"
#include "ix_mem.h"

#include "vw_cabletest_start_popup.h"

#define STR1    "* Please select a network port to test. A camera must be connected to the NVR."
#define STR2    "NOTE : All recording will stop and all cameras will be disconnected during testing. The test may take up to 25 seconds per port."
#define STR3    "- Ports without cameras connected will receive a failing test result."
#define STR4    "- Measured distances may be impossible or inaccurate if greater than 10m."
#define STR5    "- Detailed descriptions of test successes/failures is as follows :"
#define STR6    "* Normal cable :"
#define STR7    "- Successful test."
#define STR8    "* Line 3,4 is disconnected after 50m :"
#define STR9    "- Line 3 or 4 of the 8-strand line disconnects after 50m or isn't connected."
#define STR10   "* Line 3,4 short circuits after 50m :"
#define STR11   "- Line 3 or 4 of the 8-strand line short circuits after 50m."
#define STR12   "* Test failed. Please check the camera connection :"
#define STR13   "- If the NVR or camera isn't connected."
#define STR14   "* Test failed. Please check the hub connection :"
#define STR15   "- If a hub isn't connected to the NVR or hub LAN port."
#define STR16   "* Testing now in progress :"
#define STR17   "- Approximately 25 seconds is needed to test each port."

static NFWINDOW *g_curwnd = 0;

static VCT_TEST g_vct;

static gboolean g_cur_l3_model = 0;
static gint g_hub_cnt = 0;
static guint g_used_hub = 0;

static gboolean post_nvrcheck_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint i;
        gint idx;
        gboolean status;

        idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "nvr_check"));

        status = nfui_check_button_get_active(obj);

        if(idx == 10)
        {
            if(status)  
            {
                g_vct.nvr_port_check |= (1 << NVR_PORT_CAM1);
                g_vct.nvr_port_check |= (1 << NVR_PORT_CAM2);
                g_vct.nvr_port_check |= (1 << NVR_PORT_CAM3);
                g_vct.nvr_port_check |= (1 << NVR_PORT_CAM4);

                if(GUI_CHANNEL_CNT > 4)
                {
                    g_vct.nvr_port_check |= (1 << NVR_PORT_CAM5);
                    g_vct.nvr_port_check |= (1 << NVR_PORT_CAM6);
                    g_vct.nvr_port_check |= (1 << NVR_PORT_CAM7);
                    g_vct.nvr_port_check |= (1 << NVR_PORT_CAM8);
                }

                if(!g_cur_l3_model)
                {
                    g_vct.nvr_port_check |= (1 << NVR_PORT_LAN);
                    g_vct.nvr_port_check |= (1 << NVR_PORT_WAN);
                }
            }
            else
            {
                g_vct.nvr_port_check = 0;
            }

            for( i = 0; i < NVR_PORT_CNT; i++)
            {
                if(i >= GUI_CHANNEL_CNT && i < 8) continue;
                if(g_cur_l3_model && i > NVR_PORT_CAM8)  continue;
            
                nfui_check_button_set_active(g_vct.sel_check.nvr_port[i], status);
                nfui_signal_emit(g_vct.sel_check.nvr_port[i], GDK_EXPOSE, TRUE);
            }
        }
        else
        {
            if(status) 
                g_vct.nvr_port_check |= (1 << idx);
            else
                g_vct.nvr_port_check &= ~(1 << idx);
        }

        for( i = 0; i < NVR_PORT_CNT; i++)
        {
            if(g_vct.nvr_port_check & (1 << i))
                continue;
            else if( i >= GUI_CHANNEL_CNT && i < 8)
                continue;
            
            else
                break;
        }

        if(i == NVR_PORT_LAN)
        {
            if(g_cur_l3_model) i = NVR_PORT_CNT;
        }


        if(i == NVR_PORT_CNT) 
        {
            nfui_check_button_set_active(g_vct.nvr_all_check, TRUE);
            nfui_signal_emit(g_vct.nvr_all_check, GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_check_button_set_active(g_vct.nvr_all_check, FALSE);
            nfui_signal_emit(g_vct.nvr_all_check, GDK_EXPOSE, TRUE);
        }

    }

    return FALSE;
}

static gboolean post_hubcheck_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint i;
        gint idx;
        gint hub_num;
        gboolean status;

        hub_num = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "hub_num"));
        idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "hub_check"));

        status = nfui_check_button_get_active(obj);        

        if(idx == 10)
        {
            if(status)
            {
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM1);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM2);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM3);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM4);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM5);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM6);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM7);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_CAM8);
                g_vct.hub_port_check[hub_num] |= (1 << HUB_PORT_LAN);
            }
            else
            {
                g_vct.hub_port_check[hub_num] = 0;
            }

            for (i = 0; i < HUB_PORT_CNT; i++)
            {
                nfui_check_button_set_active(g_vct.sel_check.hub_port[hub_num][i], status);
                nfui_signal_emit(g_vct.sel_check.hub_port[hub_num][i], GDK_EXPOSE, TRUE);
            }
        }
        else
        {
            if(status)
            {
                g_vct.hub_port_check[hub_num] |= (1 << idx);
            }
            else
            {
                g_vct.hub_port_check[hub_num] &= ~(1 << idx);
            }
        }

        for( i = 0; i < HUB_PORT_CNT; i++)
        {
            if(g_vct.hub_port_check[hub_num] & (1 << i))
                continue;
            else
                break;
        }

        if( i == HUB_PORT_CNT)
        {
            nfui_check_button_set_active(g_vct.hub_all_check[hub_num], TRUE);
            nfui_signal_emit(g_vct.hub_all_check[hub_num], GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_check_button_set_active(g_vct.hub_all_check[hub_num], FALSE);
            nfui_signal_emit(g_vct.hub_all_check[hub_num], GDK_EXPOSE, TRUE);
        }
        
    }

    return FALSE;
}

static gboolean post_cabletest_start_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        mb_type ret;
    
        gint i = -1;
        gboolean all_check_btn = FALSE;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        ret = nftool_mbox(g_curwnd, "NOTICE", "All recording operations will stop and all cameras will disconnect.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

        if(ret == NFTOOL_MB_CANCEL)        
            return FALSE;

        //VW_CABLE_TEST_POPUP...    
        
        if(nfui_check_button_get_active(g_vct.nvr_all_check))
        {
            for(i = 0; i < g_hub_cnt; i++)
            {            
               if(!(nfui_check_button_get_active(g_vct.hub_all_check[i])))
               {
                    break;
               }
            }
        }

        if(i == g_hub_cnt) all_check_btn = TRUE;

        nf_record_stop(NULL);

        VW_CableTest_Start_Open(g_curwnd, &g_vct, g_used_hub, g_hub_cnt, all_check_btn);        
        
    }

    return FALSE;
}

static gboolean _init_set_cable_test_check_data()
{
    gint i;

    g_vct.nvr_port_check = 0;
    g_cur_l3_model = FALSE;

    if (g_hub_cnt != 0)
    {   
        for( i = 0; i < g_hub_cnt; i++)
        {
            g_vct.hub_port_check[i] = 0;
        }
    }
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        NetCableTest_tab_out_handler();

        _init_set_cable_test_check_data();

        SystemSetupNetwork_Destroy(obj);
    }

    return FALSE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkDrawable *drawable;
            GdkGC *gc;
            guint x, y;

            drawable = nfui_nfobject_get_window(obj);
            gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset(obj, &x, &y);
    
            //gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));
            //gdk_draw_rectangle(drawable, gc, FALSE, x, y, 500, 500);
        }
        break;

        default :
            break;
    }

    return FALSE;
}

void init_CableTest_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *fixed_temp;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint size_w, size_h;
    gint i, j;
    gint hub_count = 0;
    gchar *hub_str[HUB_MAX_NUM] = {"HUB1","HUB2","HUB3","HUB4"};
    gint tmp = 0;

    
    gchar str[64];
    gchar *note_str[3] = {STR3, STR4, STR5};   
    gchar *note_str2[12] = {STR6, STR7, STR8, STR9, STR10, STR11, STR12, STR13, STR14, STR15, STR16, STR17};
        
    g_curwnd = nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
    nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);

    pos_x = 27;
    pos_y = 20;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR1, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 1400, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
        
    pos_y = 61 + 45 + 20;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NVR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 210, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    #if defined(_NOT_SUPPORT_VCT_WAN_LAN)
        g_cur_l3_model = TRUE;
    #endif

    pos_y += 40 + 1;

    g_used_hub = nf_sysman_get_hub_info(0);

    for(i = 0; i < 4; i++)
    {
        if(g_used_hub & (1 << i))
        {
            strcpy(g_vct.hub_title_text[hub_count].text, hub_str[i]);
                       
            hub_count++;
        }
    }

    g_hub_cnt = hub_count;
    
    for( i = 0; i < g_hub_cnt; i++)
    {
        memset(str, 0x00, sizeof(str));
    
        if(hub_count == 1) g_sprintf(str, "%s", "HUB");
        else               g_sprintf(str, "%s", g_vct.hub_title_text[i].text);
    
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 210, 40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

        pos_y += 40 + 1;
    }    

    pos_x = 27 + 210 + 2;
    pos_y = 61 + 45 - 41 + 20;
    
    for(i = 0; i < 11; i++)
    {
        memset(str, 0x00, sizeof(str));
        
        if (i < 8)       g_sprintf(str, "CAM%d",i+1);
        else if(i == 8)  g_sprintf(str, "%s", "LAN");
        else if(i == 9)  g_sprintf(str, "%s", "WAN"); 
        else if(i == 10) g_sprintf(str, "%s", "ALL");
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));    
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 90,40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
       
        pos_x += 90+2;
    }   
    
    pos_x = 27 + 210 + 2;
    pos_y += 41;

    for (i = 0; i < 11; i++)
    {
        fixed_temp = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
        nfui_nfobject_set_size(fixed_temp, 90, 40);
        nfui_nfobject_show(fixed_temp);
        nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, (guint)pos_x, (guint)pos_y);
        if( i == 10) nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        
        obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(90 - size_w)/2, (guint)(40-size_h)/2);      

        nfui_nfobject_set_data(obj,"nvr_check", GUINT_TO_POINTER(i));
        nfui_regi_post_event_callback(obj, post_nvrcheck_button_handler);

        if(i >= GUI_CHANNEL_CNT && i < 8) nfui_nfobject_disable(obj);

        if( i == 8 || i == 9)
        {
            if(g_cur_l3_model)
                nfui_nfobject_disable(obj);
        }

        if( i == 10)    g_vct.nvr_all_check = obj;  // all button
        else            g_vct.sel_check.nvr_port[i] = obj;

        pos_x += 90+2;
    }

    for (j = 0; j < g_hub_cnt; j++)
    {    
        pos_x = 27 + 210 + 2;
        pos_y += 41;
        
        for (i = 0; i < 11; i++)
        {
            fixed_temp = (NFOBJECT*)nfui_nffixed_new();
            nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
            nfui_nfobject_set_size(fixed_temp, 90, 40);
            nfui_nfobject_show(fixed_temp);
            nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, (guint)pos_x, (guint)pos_y);
            if(i == 10) nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));

            if(i == 9)
            {
                pos_x += 90 + 2;                
                
                continue;
            }
                 
            obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
            nfui_check_get_size(obj, &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(90 - size_w)/2, (guint)(40-size_h)/2);
            nfui_regi_post_event_callback(obj, post_hubcheck_button_handler);   

            nfui_nfobject_set_data(obj, "hub_num", GUINT_TO_POINTER(j));
            nfui_nfobject_set_data(obj, "hub_check", GUINT_TO_POINTER(i));

            if(i == 10) g_vct.hub_all_check[j] = obj;
            else        g_vct.sel_check.hub_port[j][i] = obj;

            pos_x += 90+2;
        }
    }

    pos_y += 41;
    pos_x = 27;

    // pos_y current... >>  286

    pos_y = 351;

    obj = nfui_nflabel_new_with_pango_font(STR2, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(402));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 1400, 36);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);  
    
    pos_y += 36 + 1 ;

    for( i = 0; i < 3; i++)
    {
        obj = nfui_nflabel_new_with_pango_font(note_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
        
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, 1400, 36);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

        pos_y += 36 + 1;
    }        

    for( i = 0; i < 12; i++)
    {
        if(i == 2 || i == 4 || i == 6 || i == 8)
            obj = nfui_nflabel_new_with_pango_font(note_str2[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(402));
        else            
            obj = nfui_nflabel_new_with_pango_font(note_str2[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));           
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 50);
        nfui_nfobject_set_size(obj, 900, 30);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

        pos_y += 30 + 1;
    }
       
    obj = nftool_normal_button_create_type3("CABLE TEST START", 300);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, 1249 - 300, MENU_V_INNER_H - 60);
    nfui_regi_post_event_callback(obj, post_cabletest_start_event_handler);

    obj = nftool_normal_button_create_type1("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
   
}

gboolean NetCableTest_tab_out_handler()
{
    mb_type tet;

    return FALSE;
}
