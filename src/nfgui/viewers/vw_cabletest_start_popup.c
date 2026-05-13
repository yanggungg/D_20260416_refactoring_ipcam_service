#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfimage.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nflabel.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "nf_sysman.h"
#include "vsm.h"

#include "vw_cabletest_start_popup.h"
#include "nf_util_netif.h"

static VCT_TEST g_vct;

static NFWINDOW *g_parent;
static NFOBJECT *g_curwnd = 0;
static NFOBJECT *g_hub_label;
static NFOBJECT *g_all_check;

static NFOBJECT *g_hub_status[HUB_PORT_CNT];
static NFOBJECT *g_hub_detail[HUB_PORT_CNT];
static NFOBJECT *g_hub_check[HUB_PORT_CNT];
static NFOBJECT *g_hub_wait_obj[HUB_PORT_CNT];
static NFOBJECT *g_nvr_wait_obj[NVR_PORT_CNT];
static NFOBJECT *g_waitPop;

static gint g_nvr_waiting_timer[NVR_PORT_CNT] = {0,};
static gint g_hub_waiting_timer[HUB_PORT_CNT] = {0,}; 

static NFOBJECT *g_prev;
static NFOBJECT *g_next;
static NFOBJECT *g_retry;
static NFOBJECT *g_close;
static NFOBJECT *g_stop;

static gint g_cur_hub_num = 0;
static gint g_hub_cnt;

static gint g_hub_port;
static gint g_nvr_port;
static gboolean g_cur_l3_model = 0;

static guint g_used_hub;
static guint g_used_hub_tmp;

static gboolean g_test_stop = 0;

enum{

    TEST_PASS = 0,
    TEST_FAIL,
    TEST_OPEN,
    TEST_SHORT,
    TEST_ERROR,
    
};

enum{

    DEVICE_NVR = 0,
    DEVICE_HUB1,
    DEVICE_HUB2,
    DEVICE_HUB3,
    DEVICE_HUB4,
};

///////////////////////////////////////////////////////////

static gboolean _init_memcpy(VCT_TEST *vct)
{
    gint i;

    g_vct.nvr_port_check = vct->nvr_port_check;

    for (i = 0; i < g_hub_cnt; i++)
    {        
        g_vct.hub_port_check[i] =  vct->hub_port_check[i];
    }

    if(g_hub_cnt != 0)
    {
        for(i = 0; i < g_hub_cnt; i++)
        {
            strcpy(g_vct.hub_title_text[i].text, vct->hub_title_text[i].text);
        }
    }

    return FALSE;
}

static gboolean _nvr_check_waiting_timer(gpointer data)
{
    static gint img_idx = 0;   
    gint port; 
    
    port = GPOINTER_TO_INT(data);

    if( img_idx >= 5) img_idx = 0;

    NFUTIL_THREADS_ENTER();

    if (img_idx == 0)      nfui_nfimage_change_image((NFIMAGE*)g_nvr_wait_obj[port], IMG_LOADING_1);
    else if (img_idx == 1) nfui_nfimage_change_image((NFIMAGE*)g_nvr_wait_obj[port], IMG_LOADING_2);
    else if (img_idx == 2) nfui_nfimage_change_image((NFIMAGE*)g_nvr_wait_obj[port], IMG_LOADING_3);
    else if (img_idx == 3) nfui_nfimage_change_image((NFIMAGE*)g_nvr_wait_obj[port], IMG_LOADING_4);
    else if (img_idx == 4) nfui_nfimage_change_image((NFIMAGE*)g_nvr_wait_obj[port], IMG_LOADING_5);

    if(!nfui_nfobject_is_shown(g_nvr_wait_obj[port]))
        nfui_nfobject_show(g_nvr_wait_obj[port]);

    nfui_signal_emit(g_nvr_wait_obj[port], GDK_EXPOSE, TRUE);

    NFUTIL_THREADS_LEAVE();

    img_idx++;
    
    return TRUE;
}

static gboolean _hub_check_waiting_timer(gpointer data)
{
    static gint img_idx = 0;
    gint port;

    port = GPOINTER_TO_INT(data);

    if( img_idx >= 5) img_idx = 0;

    NFUTIL_THREADS_ENTER();

    if (img_idx == 0)      nfui_nfimage_change_image((NFIMAGE*)g_hub_wait_obj[port], IMG_LOADING_1);
    else if (img_idx == 1) nfui_nfimage_change_image((NFIMAGE*)g_hub_wait_obj[port], IMG_LOADING_2);
    else if (img_idx == 2) nfui_nfimage_change_image((NFIMAGE*)g_hub_wait_obj[port], IMG_LOADING_3);
    else if (img_idx == 3) nfui_nfimage_change_image((NFIMAGE*)g_hub_wait_obj[port], IMG_LOADING_4);
    else if (img_idx == 4) nfui_nfimage_change_image((NFIMAGE*)g_hub_wait_obj[port], IMG_LOADING_5);

    if(!nfui_nfobject_is_shown(g_hub_wait_obj[port]))
        nfui_nfobject_show(g_hub_wait_obj[port]);

    nfui_signal_emit(g_hub_wait_obj[port], GDK_EXPOSE, TRUE);

    NFUTIL_THREADS_LEAVE();

    img_idx++;

    return TRUE;
}

static gint _nvr_waiting_timer(gint port)
{
    if(!g_nvr_waiting_timer[port])
    {
        g_nvr_waiting_timer[port] = g_timeout_add(200, _nvr_check_waiting_timer, GUINT_TO_POINTER(port));
    }

    return 0;
}

static gint _nvr_end_waiting_timer(gint port)
{
    if (g_nvr_waiting_timer[port])
    {
        g_source_remove(g_nvr_waiting_timer[port]);

        g_nvr_waiting_timer[port] = 0;
    }
    
    return 0;
}

static gint _hub_waiting_timer(gint port)
{
    if(!g_hub_waiting_timer[port])
    {
        g_hub_waiting_timer[port] = g_timeout_add(200, _hub_check_waiting_timer, GUINT_TO_POINTER(port));
    }

    return 0;
}

static gint _hub_end_waiting_timer(gint port)
{
    if(g_hub_waiting_timer[port])
    {
        g_source_remove(g_hub_waiting_timer[port]);
    
        g_hub_waiting_timer[port] = 0;
    }

    return 0;
}

static void _set_nvr_refresh_status(gint port)
{
    nfui_nflabel_set_text(g_vct.status.nvr_port[port], "TESTING");
    nfui_nflabel_set_text(g_vct.detail.nvr_port[port], "");

    nfui_signal_emit(g_vct.detail.nvr_port[port], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_vct.status.nvr_port[port], GDK_EXPOSE, TRUE);
}

static void _set_hub_refresh_status(gint port)
{
    nfui_nflabel_set_text(g_hub_status[port], "TESTING");
    nfui_nflabel_set_text(g_hub_detail[port], "");
    
    nfui_signal_emit(g_hub_detail[port], GDK_EXPOSE, TRUE);    
    nfui_signal_emit(g_hub_status[port], GDK_EXPOSE, TRUE);
}

static gboolean _nvr_cable_test_start(gpointer data)
{
    gint port;
    gint device;

    port = GPOINTER_TO_INT(data);

    if(port == 8) port = 9;
    else if(port == 9) port = 8;

    uxm_reg_imsg_event(g_curwnd, INFY_CHECK_CABLE_TEST);
    uxm_monitor_on_imsg_event(g_curwnd, INFY_CHECK_CABLE_TEST);

    device = 0;

    scm_check_cable(port, device, INFY_CHECK_CABLE_TEST);
    
    return FALSE;
}

static gboolean _hub_cable_test_start(gpointer data)
{
    gint port;
    gint device;
    gint i;
    
    port = GPOINTER_TO_INT(data);

    if(port == 8) port = 9;
    else if(port == 9) port = 8;

    uxm_reg_imsg_event(g_curwnd, INFY_CHECK_CABLE_TEST);
    uxm_monitor_on_imsg_event(g_curwnd, INFY_CHECK_CABLE_TEST);

    for(i = 0; i < HUB_MAX_NUM; i++)
    {   
        if(g_used_hub & (1 << i)) break;
    }
        
    device = i + 1;
    
    scm_check_cable(port, device, INFY_CHECK_CABLE_TEST);

    return FALSE;
}

static void _run_nvr_cable_test(gint port)
{
    g_timeout_add(1000, _nvr_cable_test_start, GUINT_TO_POINTER(port));    
}

static void _run_hub_cable_test(gint port)
{
    g_timeout_add(1000, _hub_cable_test_start, GUINT_TO_POINTER(port));    
}

static void _start_nvr_cable_test_process(gint port)
{
    _set_nvr_refresh_status(port);
    _nvr_waiting_timer(port);
    _run_nvr_cable_test(port);
}

static void _start_hub_cable_test_process(gint port)
{
    _set_hub_refresh_status(port);
    _hub_waiting_timer(port);    
    _run_hub_cable_test(port);
}

static void _update_prev_next_button(gint hub_num)
{
    if(g_hub_cnt > 1)
    {
        if(hub_num == g_hub_cnt-1)
        {
            nfui_nfobject_enable(g_prev);
            nfui_nfobject_disable(g_next);
        }
        else if(hub_num == 0)
        {
            nfui_nfobject_disable(g_prev);
            nfui_nfobject_enable(g_next);
        }
        else if(hub_num > 0 && hub_num < (g_hub_cnt - 1))
        {
            nfui_nfobject_enable(g_prev);
            nfui_nfobject_enable(g_next);
        }
    }
    else
    {
        nfui_nfobject_disable(g_prev);
        nfui_nfobject_disable(g_next);
    }
  
    nfui_signal_emit(g_next, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_prev, GDK_EXPOSE, TRUE);
}

static void _set_disable_button()
{
    gint i;

    nfui_nfobject_disable(g_retry);
    nfui_nfobject_disable(g_close);
    nfui_nfobject_disable(g_all_check);

    nfui_nfobject_enable(g_stop);

    if(g_hub_cnt != 0)
    {
        nfui_nfobject_disable(g_prev);
        nfui_nfobject_disable(g_next);
        
        nfui_signal_emit(g_prev, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_next, GDK_EXPOSE, TRUE);
    }
    
    nfui_signal_emit(g_retry, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_close, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_stop, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_all_check, GDK_EXPOSE, TRUE);

    for( i = 0; i < NVR_PORT_CNT; i++)
    {    
        nfui_nfobject_disable(g_vct.sel_check.nvr_port[i]);
        nfui_signal_emit(g_vct.sel_check.nvr_port[i], GDK_EXPOSE, TRUE);
    }

    if(g_hub_cnt != 0)
    {
        for( i = 0; i < HUB_PORT_CNT; i++)
        {
            nfui_nfobject_disable(g_hub_check[i]);
            nfui_signal_emit(g_hub_check[i], GDK_EXPOSE, TRUE);
        }
    }
}

static void _set_enable_button()
{
    gint i;

    nfui_nfobject_enable(g_retry);
    nfui_nfobject_enable(g_close);
    nfui_nfobject_enable(g_all_check);
    nfui_nfobject_disable(g_stop);

    nfui_signal_emit(g_retry, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_close, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_stop, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_all_check, GDK_EXPOSE, TRUE);
    
    for( i = 0; i < NVR_PORT_CNT; i++)
    {
        if(i >= GUI_CHANNEL_CNT && i < 8) continue;
        if(g_cur_l3_model && i > NVR_PORT_CAM8)  continue;
        
        nfui_nfobject_enable(g_vct.sel_check.nvr_port[i]);
        nfui_signal_emit(g_vct.sel_check.nvr_port[i], GDK_EXPOSE, TRUE);
    }

    if(g_hub_cnt != 0)
    {
        for( i = 0; i < HUB_PORT_CNT; i++)
        {
            nfui_nfobject_enable(g_hub_check[i]);
            nfui_signal_emit(g_hub_check[i], GDK_EXPOSE, TRUE);
        }
    
        _update_prev_next_button(g_cur_hub_num);

    }
}

static gboolean _update_hub_label(gint hub_num)
{
    gint i;
    gchar str[16];

    memset(str, 0x00, sizeof(str));

    if(g_hub_cnt > 1 )
        g_sprintf(str, "%s", g_vct.hub_title_text[hub_num].text);
    else
        g_sprintf(str, "%s", "HUB");

    nfui_nflabel_set_text(g_hub_label, str);
    nfui_signal_emit(g_hub_label, GDK_EXPOSE, TRUE);
   
    for(i = 0; i < HUB_PORT_CNT; i++)
    {    
        nfui_nflabel_set_text(g_hub_status[i], g_vct.status.hub_text[hub_num][i].text);
        nfui_nflabel_set_text(g_hub_detail[i], g_vct.detail.hub_text[hub_num][i].text);

        if(g_vct.hub_port_check[hub_num] & ( 1 << i))
            nfui_check_button_set_active(g_hub_check[i], TRUE);
        else
            nfui_check_button_set_active(g_hub_check[i], FALSE);
            
        nfui_signal_emit(g_hub_status[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_hub_detail[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_hub_check[i], GDK_EXPOSE, TRUE);
    }
    
    return FALSE;   
}

static void _remove_wait_box()
{
    if(g_waitPop) {
		nftool_remove_waitbox(g_waitPop);
		g_waitPop = NULL;
    }
}

static gboolean _update_result_nvr_label()
{
    gint i;

    for(i = 0; i < NVR_PORT_CNT; i++)
    {
        nfui_nflabel_set_text(g_vct.status.nvr_port[i], g_vct.status.nvr_text[i].text);
        nfui_nflabel_set_text(g_vct.detail.nvr_port[i], g_vct.detail.nvr_text[i].text);

        nfui_signal_emit(g_vct.status.nvr_port[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_vct.detail.nvr_port[i], GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean _update_result_hub_label()
{
    gint i;

    if(g_hub_cnt != 0)
    {
        for(i = 0; i < HUB_PORT_CNT; i++)
        {
            nfui_nflabel_set_text(g_hub_status[i], g_vct.status.hub_text[g_cur_hub_num][i].text);
            nfui_nflabel_set_text(g_hub_detail[i], g_vct.detail.hub_text[g_cur_hub_num][i].text);

            nfui_signal_emit(g_hub_status[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_hub_detail[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean _update_wait_for_test_nvr_label()
{
    gint i;

    for (i = 0; i < NVR_PORT_CNT; i++)
    {
        if(g_vct.nvr_port_check & (1 << i))
        {
            nfui_nflabel_set_text(g_vct.status.nvr_port[i], "WAIT");
            nfui_nflabel_set_text(g_vct.detail.nvr_port[i], "Stand by for testing...");

            nfui_signal_emit(g_vct.status.nvr_port[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_vct.detail.nvr_port[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean _update_wait_for_test_hub_label(gint cur_hub)
{
    gint i;

    if(g_hub_cnt != 0)
    {
        for (i = 0; i < HUB_PORT_CNT; i++)
        {
            if(g_vct.hub_port_check[cur_hub] & (1 << i))
            {
                nfui_nflabel_set_text(g_hub_status[i], "WAIT");
                nfui_nflabel_set_text(g_hub_detail[i], "Stand by for testing...");

                nfui_signal_emit(g_hub_status[i], GDK_EXPOSE, TRUE);
                nfui_signal_emit(g_hub_detail[i], GDK_EXPOSE, TRUE);
            }
        }
    }

    return FALSE;
}


static void _start_cable_test_process()
{
    gint i, j, k;

    if(g_test_stop)
    {
        _set_enable_button();
        _update_result_nvr_label();
        _update_result_hub_label();
        _remove_wait_box();
        
        return ;
    }

    for(i = g_nvr_port; i < NVR_PORT_CNT; i++)
    {
        if(g_vct.nvr_port_check & (1 << i))
        {
            _start_nvr_cable_test_process(i);

            g_nvr_port = i + 1;

            return;
        }
    }

    if( i <  NVR_PORT_CNT ) return ;

    for(j = g_cur_hub_num; j < g_hub_cnt; j++)
    {            
        for(i = g_hub_port; i < HUB_PORT_CNT; i++)
        {
            if(g_vct.hub_port_check[j] & (1 << i))
            {
                _start_hub_cable_test_process(i);

                g_hub_port = i + 1;

                return;
            }
        }

        if( i == HUB_PORT_CNT)
        {
            if(g_cur_hub_num == (g_hub_cnt - 1)) break;

            for(k = 0; k < HUB_MAX_NUM; k++)
            {
                if(g_used_hub & (1 << k))
                {
                    g_used_hub &= ~(1 << k);
                    break;
                }
            }
             
            g_cur_hub_num++;
            g_hub_port = 0;

            _update_hub_label(g_cur_hub_num);
            _update_wait_for_test_hub_label(g_cur_hub_num);
        }
    }    
    
    _set_enable_button();
    _update_result_nvr_label();
    _update_result_hub_label();
}

static gboolean post_all_check_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint i;
        gint idx;
        gint status;

        status = nfui_check_button_get_active(obj);

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

            for(i = 0; i < g_hub_cnt; i++)
            {                
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM1);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM2);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM3);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM4);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM5);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM6);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM7);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_CAM8);
                g_vct.hub_port_check[i] |= (1 << HUB_PORT_LAN);
            }
        }
        else
        {
            g_vct.nvr_port_check = 0;

            for(i = 0; i < g_hub_cnt; i++)
            {                
                g_vct.hub_port_check[i] = 0;
            }
        }

        for(i = 0; i < NVR_PORT_CNT; i++)
        {
            if(g_cur_l3_model && i > NVR_PORT_CAM8)  continue;
            if(i >= GUI_CHANNEL_CNT && i < 8)  continue;
            
            nfui_check_button_set_active(g_vct.sel_check.nvr_port[i], status);
            nfui_signal_emit(g_vct.sel_check.nvr_port[i], GDK_EXPOSE, TRUE);
        }

        if(g_hub_cnt != 0)
        {
            for(i = 0; i < HUB_PORT_CNT; i++)
            {
                nfui_check_button_set_active(g_hub_check[i], status);
                nfui_signal_emit(g_hub_check[i], GDK_EXPOSE, TRUE);
            }
        }
        
    }

    return FALSE;
}

static gboolean post_nvr_check_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint i, j;
        gint idx;
        gint status;
      
        idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "popup_nvr_check"));

        status = nfui_check_button_get_active(obj);

        if(status)
            g_vct.nvr_port_check |= (1 << idx);
        else
            g_vct.nvr_port_check &= ~(1 << idx);

        for(i = 0; i < NVR_PORT_CNT; i++)
        {
           if(g_vct.nvr_port_check & (1 << i)) continue;
           else if(i >= GUI_CHANNEL_CNT && i < 8) continue;
           else     break;
        }

        if(i == NVR_PORT_LAN)
        {   
            if(g_cur_l3_model) i = NVR_PORT_CNT;
        }

        if(i == NVR_PORT_CNT)
        {
            for(j = 0; j < g_hub_cnt; j++)
            {                
                for(i = 0; i < HUB_PORT_CNT; i++)
                {
                    if(g_vct.hub_port_check[j] & (1 << i)) continue;
                    else break;
                }

                if(i == HUB_PORT_CNT) continue;
                else break;
            }

            if(g_hub_cnt == 0) i = HUB_PORT_CNT;
        }        

        if(i == HUB_PORT_CNT)
        {   
            nfui_check_button_set_active(g_all_check, TRUE);
            nfui_signal_emit(g_all_check, GDK_EXPOSE, TRUE);
        }
        else
        {
            if(nfui_check_button_get_active(g_all_check))
            {
                nfui_check_button_set_active(g_all_check, FALSE);
                nfui_signal_emit(g_all_check, GDK_EXPOSE, TRUE);
            }
        }
    }

    return FALSE;
}

static gboolean post_hub_check_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint i, j;
        gint idx;
        gint status;

        idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "popup_hub_check"));

        status = nfui_check_button_get_active(obj);

        if(status)
            g_vct.hub_port_check[g_cur_hub_num] |= (1 << idx);  
        else
            g_vct.hub_port_check[g_cur_hub_num] &= ~(1 << idx);

        for(i = 0; i < NVR_PORT_CNT; i++)
        {
            if(g_vct.nvr_port_check & (1 << i)) continue;
            else     break;
        }

        if(i == NVR_PORT_CNT)
        {
            for(j = 0; j < g_hub_cnt; j++)
            {            
                for(i = 0; i < HUB_PORT_CNT; i++)
                {
                    if(g_vct.hub_port_check[j] & (1 << i)) continue;
                    else break;
                }

                if(i == HUB_PORT_CNT) continue;
                else break;
            }
        }

        if(i == HUB_PORT_CNT)
        {   
            nfui_check_button_set_active(g_all_check, TRUE);
            nfui_signal_emit(g_all_check, GDK_EXPOSE, TRUE);
        }
        else
        {
            if(nfui_check_button_get_active(g_all_check))
            {
                nfui_check_button_set_active(g_all_check, FALSE);
                nfui_signal_emit(g_all_check, GDK_EXPOSE, TRUE);
            }
        }
    }
        
    return FALSE;
}

static gboolean _update_hub_port_status(gint hub_num)
{
    gint i;

    for(i = 0; i < HUB_PORT_CNT; i++)
    {    
        if(strcmp(nfui_nflabel_get_text(g_hub_status[i]), g_vct.status.hub_text[hub_num][i].text))
        {  
            nfui_nflabel_set_text(g_hub_status[i], g_vct.status.hub_text[hub_num][i].text);
            nfui_signal_emit(g_hub_status[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean _update_hub_port_detail(gint hub_num)
{
    gint i, j;

    for(i = 0; i < HUB_PORT_CNT; i++)
    {
        if(strcmp(nfui_nflabel_get_text(g_hub_detail[i]), g_vct.detail.hub_text[hub_num][i].text))
        {
            nfui_nflabel_set_text(g_hub_detail[i], g_vct.detail.hub_text[hub_num][i].text);
            nfui_signal_emit(g_hub_detail[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean _update_hub_port_check(gint hub_num)
{
    gint i;

    for ( i = 0; i < HUB_PORT_CNT; i++)
    {
        if(g_vct.hub_port_check[hub_num] & (1 << i))
        {
            if(!(nfui_check_button_get_active(g_hub_check[i])))
            {
                nfui_check_button_set_active(g_hub_check[i], TRUE);
                nfui_signal_emit(g_hub_check[i], GDK_EXPOSE, TRUE);
            }
        }
        else
        {
            if(nfui_check_button_get_active(g_hub_check[i]))
            {
                nfui_check_button_set_active(g_hub_check[i], FALSE);
                nfui_signal_emit(g_hub_check[i], GDK_EXPOSE, TRUE);
            }
        }
    }

    return FALSE;
}

static gboolean post_prev_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        gint i;
        gchar str[32];
        memset(str, 0x00, sizeof(str));
        
        g_cur_hub_num--;

        g_sprintf(str, "%s", g_vct.hub_title_text[g_cur_hub_num].text);
    
        nfui_nflabel_set_text(g_hub_label, str);
        nfui_signal_emit(g_hub_label, GDK_EXPOSE, TRUE);

        // STATUS ,DETAIL ,CHECK    

        _update_hub_port_status(g_cur_hub_num);
        _update_hub_port_detail(g_cur_hub_num);
        _update_hub_port_check(g_cur_hub_num);
        

        ////////////////////////
        
        _update_prev_next_button(g_cur_hub_num);
    }

    return FALSE;
}

static gboolean post_next_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        gint i;
        gchar str[32];
        memset(str, 0x00, sizeof(str));

        g_cur_hub_num++;

        g_sprintf(str, "%s", g_vct.hub_title_text[g_cur_hub_num].text);

        nfui_nflabel_set_text(g_hub_label, str);
        nfui_signal_emit(g_hub_label, GDK_EXPOSE, TRUE);

        // STATUS ,DETAIL ,CHECK    

        _update_hub_port_status(g_cur_hub_num);
        _update_hub_port_detail(g_cur_hub_num);
        _update_hub_port_check(g_cur_hub_num);

        /////////////////////////

        _update_prev_next_button(g_cur_hub_num);
    }

    return FALSE;
}

static gboolean post_teststop_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        mb_type ret;

        ret = nftool_mbox(g_curwnd, "NOTICE", "Testing now in progress. Cancelling the test requires at least 25 seconds.\nCancel the test?", NFTOOL_MB_OKCANCEL);

        if(ret == NFTOOL_MB_OK) 
        {
            g_test_stop = TRUE;
            g_waitPop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Cancelling test.", "Please wait...");
        }
        else 
        {
            g_test_stop = FALSE;
        }
    }

    return FALSE;
}

static gboolean _init_cable_test_info()
{
    g_used_hub = g_used_hub_tmp;

    g_cur_hub_num = 0;
    g_nvr_port = 0;
    g_hub_port = 0;
    
    g_test_stop = FALSE;

    if(g_hub_cnt != 0)
        _update_hub_label(g_cur_hub_num);
        
    _update_wait_for_test_nvr_label();
    _update_wait_for_test_hub_label(g_cur_hub_num);
  
    _set_disable_button();
    
    return FALSE;
}

static gboolean post_retry_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        _init_cable_test_info();
        _start_cable_test_process();
    }

    return FALSE;
}

static gboolean _init_set_data_popup_window()
{
    gint i, j;
    
    g_curwnd = 0;
    g_cur_hub_num = 0;
    g_cur_l3_model = FALSE;

    if(g_hub_cnt != 0)
    {
        for(i = 0; i < HUB_PORT_CNT; i++) g_hub_wait_obj[i] = 0; 
    }

    for(i = 0; i < NVR_PORT_CNT; i++) g_nvr_wait_obj[i] = 0;

    g_vct.nvr_port_check = 0;

    for (i = 0; i < g_hub_cnt; i++)
    {        
        g_vct.hub_port_check[i] =  0;
    }

    for(i = 0; i < g_hub_cnt; i++)
    {            
        for(j = 0; j < HUB_PORT_CNT; j++) 
        {
            memset(g_vct.detail.hub_text[i][j].text, 0x00, sizeof(VCT_TEXT));
            memset(g_vct.status.hub_text[i][j].text, 0x00, sizeof(VCT_TEXT));
        }
    }

    for(i = 0; i < NVR_PORT_CNT; i++)
    {
        memset(g_vct.detail.nvr_text[i].text, 0x00, sizeof(VCT_TEXT));
        memset(g_vct.status.nvr_text[i].text, 0x00, sizeof(VCT_TEXT));
    }

    nf_record_start(NULL);
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        NFOBJECT *topwin;
        
        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);

        _init_set_data_popup_window();
    }

    return FALSE;
}

static gint _get_vct_result(CHKCABLE_RES_T *res, gchar *result_str, gint *length, gint msg_result)
{
    gint ret;

    if(msg_result)
    {
        strcpy(result_str, "CABLE_TEST_ERROR");

        return TEST_ERROR;
    }

    switch(res->result)
    {
        case NF_SYSMAN_SWITCH_VCT_PASS:
        {
            strcpy(result_str, "OK_PASS");
            ret = TEST_PASS;
            break;
        }
        case NF_SYSMAN_SWITCH_VCT_OPEN :
        {
            ret = TEST_OPEN;
            strcpy(result_str, "FAIL");
            break;
        }
        case NF_SYSMAN_SWITCH_VCT_SHORT:
        {
            strcpy(result_str, "FAIL");
            ret = TEST_SHORT;
            break;
        }
        case NF_SYSMAN_SWITCH_VCT_FAIL :
        {
            strcpy(result_str, "FAIL");
            ret = TEST_FAIL;
            break;
        }
    }

    if(res->length < 10)
        *length = 0;
    else
        *length = res->length;
        
    return ret;
}

static void _set_nvr_detail_result(gint ret, gchar *result_str, gint length, gint port, guint cable_num)
{
    gchar str_buf[128];
    gint i;
    gint num_cnt = 0;
    gint num = 0;
    
    memset(str_buf, 0x00, sizeof(str_buf));   

    switch(ret)
    {
        case TEST_PASS  :
        {
            g_sprintf(str_buf,"%s", "Normal cable.");
            break;
        }
        case TEST_OPEN :
        {        
            if(length != 0)
            {
                for(i = 0; i < 8; i++)
                {
                    if(cable_num & (1 << i))
                    {                        
                        if(num_cnt == 1)    break;    

                        num = i;
                        num_cnt++;
                    }
                }

                g_sprintf(str_buf,"Line %d or %d is disconnected after %dM.", num, i, length);
            }
            else
                g_sprintf(str_buf,"%s: %s", lookup_string("LENGTH"), lookup_string("Not measured")); 

            break;
        }
        case TEST_SHORT  :
        {
            if(length != 0)
            {
                for(i = 0; i < 8; i++)
                {
                    if(cable_num & (1 << i))
                    {
                        if(num_cnt == 1)     break;
                        
                        num = i;
                        num_cnt++;
                    }
                }
                g_sprintf(str_buf,"Line %d or %d short circuits after %dM.", num, i, length);
            }
            else
                g_sprintf(str_buf,"%s: %s", lookup_string("LENGTH"), lookup_string("Not measured")); 

            break;
        }
        case TEST_FAIL  :
        {
            if(port == 8)              
                g_sprintf(str_buf,"%s","Please check the hub connection.");
            else if(port == 9)
                g_sprintf(str_buf,"%s","Check that the network cable is connected.");
            else
                g_sprintf(str_buf,"%s","Please check the camera connection.");
                
            break;
        }
        case TEST_ERROR :
        {
            g_sprintf(str_buf,"%s", result_str);
            break;
        }
    }   

    nfui_nfimage_change_image((NFIMAGE*) g_nvr_wait_obj[port], IMG_LOADING_0);
    nfui_signal_emit(g_nvr_wait_obj[port], GDK_EXPOSE, TRUE);
    nfui_nfobject_hide(g_nvr_wait_obj[port]);

    strcpy(g_vct.detail.nvr_text[port].text, str_buf); 
    strcpy(g_vct.status.nvr_text[port].text, result_str); 
    
    nfui_nflabel_set_text(g_vct.detail.nvr_port[port], str_buf);
    nfui_nflabel_set_text(g_vct.status.nvr_port[port], result_str); 
    
    nfui_signal_emit(g_vct.detail.nvr_port[port], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_vct.status.nvr_port[port], GDK_EXPOSE, TRUE); 
    
}

static void _set_hub_detail_result(gint ret, gchar *result_str, gint length, gint port, gint device, guint cable_num)
{
    gchar str_buf[128];
    gint num_cnt = 0;
    gint num = 0;
    gint i;

    memset(str_buf, 0x00, sizeof(str_buf));

    switch(ret)
    {
        case TEST_PASS  :
        {
            g_sprintf(str_buf,"%s", "Normal cable.");
            break;
        }
        case TEST_FAIL  :
        {
            if(port == 8)
                g_sprintf(str_buf,"%s","Please check the hub connection.");
            else
                g_sprintf(str_buf,"%s","Please check the camera connection.");
                
            break;
        }
        case TEST_SHORT :
        {
            if(length != 0)
            {
                for(i = 0; i < 8; i++)
                {
                    if(cable_num & (1 << i))
                    {
                        if(num_cnt == 1)     break;
                        
                        num = i;
                        num_cnt++;
                    }
                }
                g_sprintf(str_buf,"Line %d or %d short circuits after %dM.", num, i, length);
            }
            else
                g_sprintf(str_buf,"%s: %s", lookup_string("LENGTH"), lookup_string("Not measured")); 
        }
        case TEST_OPEN  :
        {
            if(length != 0)
            {
                for(i = 0; i < 8; i++)
                {
                    if(cable_num & (1 << i))
                    {
                        if(num_cnt == 1)     break;
                        
                        num = i;
                        num_cnt++;
                    }
                }
                g_sprintf(str_buf,"Line %d or %d is disconnected after %dM.", num, i, length);
            }
            else
                g_sprintf(str_buf,"%s: %s", lookup_string("LENGTH"), lookup_string("Not measured")); 
        }
        case TEST_ERROR :
        {
            g_sprintf(str_buf,"%s", result_str);
            break;
        }
    }   

    nfui_nfimage_change_image((NFIMAGE*) g_hub_wait_obj[port], IMG_LOADING_0);
    nfui_signal_emit(g_hub_wait_obj[port], GDK_EXPOSE, TRUE);
    nfui_nfobject_hide(g_hub_wait_obj[port]);

    strcpy(g_vct.detail.hub_text[device][port].text, str_buf); 
    strcpy(g_vct.status.hub_text[device][port].text, result_str); 
    
    nfui_nflabel_set_text(g_hub_detail[port], str_buf);
    nfui_nflabel_set_text(g_hub_status[port], result_str); 
    nfui_signal_emit(g_hub_detail[port], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_hub_status[port], GDK_EXPOSE, TRUE);
}

static void _update_cable_test_result(gint ret, gchar *result_str, gint length, gint port, gint device, guint cable_num)
{
    if(device == DEVICE_NVR)
    {
        _nvr_end_waiting_timer(port);   
        _set_nvr_detail_result(ret, result_str, length, port, cable_num);
    }
    else
    {
        device = g_cur_hub_num;
    
        _hub_end_waiting_timer(port);
        _set_hub_detail_result(ret, result_str, length, port, device, cable_num); //  hub num  index 0 start 
    }
}

static gboolean _init_page_start(gpointer data)
{
    _init_cable_test_info();
    _start_cable_test_process();

    return FALSE;
}

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
	{
	    g_curwnd = 0;
		gtk_main_quit();
	}
    else if (evt->type == INFY_CHECK_CABLE_TEST)
    {
        gchar result_str[32];
        gint length;
        gint ret;
        
        CHKCABLE_RES_T *res;

        gint msg_result = ((CMM_MESSAGE_T *)data)->param;

        memset(result_str, 0x00, sizeof(result_str));

        res = ((CMM_MESSAGE_T *)data)->data;

        uxm_unreg_imsg_event(g_curwnd, INFY_CHECK_CABLE_TEST); 
       
        ret = _get_vct_result(res, result_str, &length, msg_result);

        _update_cable_test_result(ret, result_str, length, res->port, res->device, res->cable_num);

        _start_cable_test_process();
    }
    
	return FALSE;
}

gboolean VW_CableTest_Start_Open(NFWINDOW *parent, VCT_TEST *vct, guint used_hub, gint hub_cnt, gboolean all_check_btn)
{
    NFOBJECT *main_wnd;
    NFOBJECT *nffixed;
    NFOBJECT *fixed_temp;
    NFOBJECT *content_fixed;
    NFOBJECT *table;
    NFOBJECT *status_t;
    NFOBJECT *obj;
    
    gint popup_size_h;
    gint pos_x, pos_y;
    guint size_w, size_h;

    gchar str[32];
    gint i, j;
    gint hub_count;

    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    GdkPixbuf *left[NFOBJECT_STATE_COUNT];
    GdkPixbuf *right[NFOBJECT_STATE_COUNT];

    prev_img[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_LEFT), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_LEFT), NULL);	
	prev_img[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_LEFT), NULL);	
	prev_img[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_LEFT), NULL);	

	next_img[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_RIGHT), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_RIGHT), NULL);	
	next_img[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_RIGHT), NULL);	
	next_img[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_RIGHT), NULL);	   

	left[0] = nfui_get_image_from_file((IMG_N_PTZ_LEFT_LIVE), NULL);
	left[1] = nfui_get_image_from_file((IMG_O_PTZ_LEFT_LIVE), NULL);
	left[2] = nfui_get_image_from_file((IMG_P_PTZ_LEFT_LIVE), NULL);
	left[3] = nfui_get_image_from_file((IMG_D_PTZ_LEFT_LIVE), NULL);

	right[0] = nfui_get_image_from_file((IMG_N_PTZ_RIGHT_LIVE), NULL);
	right[1] = nfui_get_image_from_file((IMG_O_PTZ_RIGHT_LIVE), NULL);
	right[2] = nfui_get_image_from_file((IMG_P_PTZ_RIGHT_LIVE), NULL);
	right[3] = nfui_get_image_from_file((IMG_D_PTZ_RIGHT_LIVE), NULL);
	 
    g_parent = parent;
    g_hub_cnt = hub_cnt;
    g_used_hub = used_hub;
    g_used_hub_tmp = used_hub;
    
    _init_memcpy(vct);

    if(g_hub_cnt == 0)
        main_wnd = nftool_create_popup_window(parent, 600, 300, POPUP_SIZE_WID, POPUP_SIZE_NVR_HEI, "CABLE TEST", FALSE);
    else
        main_wnd = nftool_create_popup_window(parent, 600, 110, POPUP_SIZE_WID, POPUP_SIZE_HEI, "CABLE TEST", FALSE);

    nfui_regi_post_event_callback(main_wnd, post_main_wnd_event_handler);

    g_curwnd = main_wnd;

    nffixed = ((NFWINDOW*)main_wnd)->child;   
    
    pos_x = 10;
    pos_y = 50 + 40 + 1;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("NVR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 94, 370);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);   

    pos_y += 370 + 2;

    #if defined(_NOT_SUPPORT_VCT_WAN_LAN)
        g_cur_l3_model = TRUE;
    #endif

    memset(str, 0x00, sizeof(str));

    if(g_hub_cnt != 0 )
    {    
        if(g_hub_cnt > 1 ) g_sprintf(str, "%s", g_vct.hub_title_text[0].text);
        else             g_sprintf(str, "%s", "HUB");

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 94, 284);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

        g_hub_label = obj;
    
        pos_y += 284 + 1;

        fixed_temp = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_set_size(fixed_temp, 94, 48);
        nfui_nfobject_show(fixed_temp);
        nfui_nffixed_put((NFFIXED*)nffixed, fixed_temp,  pos_x, pos_y);

        if(g_hub_cnt != 0)
        {
            obj = nfui_nfbutton_new();
            nfui_nfbutton_set_image((NFBUTTON*)obj, left);
            nfui_nfobject_set_size(obj, 42, 42);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 2, 3);
            nfui_nfobject_disable(obj);
            nfui_regi_post_event_callback(obj, post_prev_page_event_handler);
            g_prev = obj;

            pos_x = 44 + 4;

            obj = nfui_nfbutton_new();
            nfui_nfbutton_set_image((NFBUTTON*)obj, right);
            nfui_nfobject_set_size(obj, 42, 42);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed_temp, obj, pos_x, 3);
            nfui_regi_post_event_callback(obj, post_next_page_event_handler);    
            g_next = obj;

            if(g_hub_cnt == 1 ) nfui_nfobject_disable(obj);
        }
    }
   
    pos_x = 10 + 94 + 2;
    pos_y = 50;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 85, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 85 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 110, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y); 

    pos_x += 110 + 2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DETAIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 410, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 410 + 2;

    fixed_temp = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_set_size(fixed_temp, 40, 40);
    nfui_nfobject_show(fixed_temp);
    nfui_nffixed_put((NFFIXED*)nffixed, fixed_temp,  pos_x, pos_y);

    if(all_check_btn)           
        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    else
        obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(40-size_w)/2, (guint)(40-size_h)/2); 
    nfui_regi_post_event_callback(obj, post_all_check_button_handler);
    g_all_check = obj;
    

    // PORT

    pos_x = 10 + 94 + 2; 
    pos_y = 50 + 40 + 1;

    for (i = 0; i < 10; i++)
    {
        memset(str, 0x00, sizeof(str));

        if(i < 8)       g_sprintf(str, "CAM%d", i+1);
        else if(i == 8) g_sprintf(str, "%s", "LAN");
        else if(i == 9) g_sprintf(str, "%s", "WAN");
    
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 85, 36);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);
        
        pos_y += 36 + 1;

        if(i >= GUI_CHANNEL_CNT && i < 8) nfui_nfobject_disable(obj);

        if( i == 8 || i == 9)
        {
            if(g_cur_l3_model)
                nfui_nfobject_disable(obj);
        }
    }

    if(g_hub_cnt !=0)
    {
        pos_y += 2;

        for (i = 0; i < 9; i++)
        {
            memset(str, 0x00, sizeof(str));

            if(i < 8)       g_sprintf(str, "CAM%d", i +1);
            else if(i == 8) g_sprintf(str, "%s", "LAN");

            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, 85, 36);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);
            
            pos_y += 36 + 1;
        }
    }

    // STATUS

    pos_x = 10 + 94 + 2 + 85 + 2;
    pos_y = 50 + 40 +1;

    for (i = 0; i < 10; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 110, 36);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

        g_vct.status.nvr_port[i] = obj;
        
        pos_y += 36 + 1;
    }

    if(g_hub_cnt !=0)
    {
        pos_y += 2;

        for (i = 0; i < 9; i++)
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, 110, 36);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

            g_hub_status[i] = obj;

            pos_y += 36+1;
        }
    }
    
    // DETAIL

    pos_x += 110 + 2;
    pos_y = 50 + 40 + 1;

    nfui_get_image_size(IMG_LOADING_0, &size_w, &size_h);

    for (i = 0; i < 10; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 410, 36);
        nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);
        g_vct.detail.nvr_port[i] = obj;


        obj = nfui_nfimage_new(IMG_LOADING_0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_hide(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x + (410/2) - (size_w/2), pos_y + (36 - size_h)/2);
        g_nvr_wait_obj[i] = obj;

        pos_y += 36 + 1; 
    }

    if(g_hub_cnt != 0)
    {
        pos_y += 2;

        for ( i = 0; i < 9; i++)
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, 410, 36);
            nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);
            g_hub_detail[i] = obj;

            obj = nfui_nfimage_new(IMG_LOADING_0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
            nfui_nfobject_hide(obj);
            nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x + (410/2) - (size_w/2), pos_y + (36 - size_h)/2);        
            g_hub_wait_obj[i] = obj;

            pos_y += 36 + 1;
        }  
    }

    // CHECK BUTTON
    
    pos_x += 410 + 2;
    pos_y = 50 + 40 + 1;

    for (i = 0; i < 10; i++)
    {
        fixed_temp = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_set_size(fixed_temp, 40, 36);
        nfui_nfobject_show(fixed_temp);
        nfui_nffixed_put((NFFIXED*)nffixed, fixed_temp, (guint)pos_x, (guint)pos_y);

        if(g_vct.nvr_port_check & ( 1 << i ))
            obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        else
            obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
            
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(40 - size_w)/2, (guint)(36 - size_h)/2);

        nfui_nfobject_set_data(obj,"popup_nvr_check", GUINT_TO_POINTER(i));
        nfui_regi_post_event_callback(obj, post_nvr_check_button_handler);

        if(i >= GUI_CHANNEL_CNT && i < 8) nfui_nfobject_disable(obj);
        
        if( i == 8 || i == 9)
        {
            if(g_cur_l3_model)
                nfui_nfobject_disable(obj);
        }

        g_vct.sel_check.nvr_port[i] = obj;

        pos_y += 36 + 1;                            
    }

    if(g_hub_cnt != 0)
    {
        pos_y += 2;   

        for (i = 0; i < 9; i++)
        {
            fixed_temp = (NFOBJECT*)nfui_nffixed_new();
            nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_set_size(fixed_temp, 40, 36);
            nfui_nfobject_show(fixed_temp);
            nfui_nffixed_put((NFFIXED*)nffixed, fixed_temp, (guint)pos_x, (guint)pos_y);

            if(g_vct.hub_port_check[0] & ( 1 << i ))
                obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
            else
                obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(obj, &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(40 - size_w)/2, (guint)(36- size_h)/2);

            nfui_nfobject_set_data(obj,"popup_hub_check", GUINT_TO_POINTER(i));
            nfui_regi_post_event_callback(obj, post_hub_check_button_handler);

            g_hub_check[i] = obj;

            pos_y += 36 + 1;
        }
    }

    // DIRECTION
    
    #if 0  // not use
    if(g_hub_cnt != 1)
    {
        nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
        
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        //nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, POPUP_SIZE_WID/2 - (size_w + 10) , POPUP_SIZE_HEI - 95 + (40 - size_h)/2);
        nfui_regi_post_event_callback(obj, post_prev_page_event_handler);
        nfui_nfobject_disable(obj);

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        //nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, POPUP_SIZE_WID/2 + 10, POPUP_SIZE_HEI - 95 + (40 - size_h)/2);
        nfui_regi_post_event_callback(obj, post_next_page_event_handler);    

        if(hub_cnt == 1)       nfui_nfobject_disable(obj);
    }
    #endif

        
    // TEST STOP / RETRY / CLOSE

    if(g_hub_cnt == 0) popup_size_h = POPUP_SIZE_NVR_HEI;
    else               popup_size_h = POPUP_SIZE_HEI;

    obj = nftool_normal_button_create_popup_type2("TEST STOP", 170);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, 10, popup_size_h - 45);
    nfui_regi_post_event_callback(obj, post_teststop_button_event_handler);    
    nfui_nfobject_disable(obj);
    g_stop = obj;

    obj = nftool_normal_button_create_popup_type2("RETRY", 152);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, 10 + 170 + 10, popup_size_h - 45);
    nfui_regi_post_event_callback(obj, post_retry_button_event_handler);  
    g_retry = obj;
    
    
    obj = nftool_normal_button_create_popup_type2("CLOSE", 152);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, POPUP_SIZE_WID - 152 - 10, popup_size_h - 45);
    nfui_regi_post_event_callback(obj, post_close_button_event_handler);
    g_close = obj;

    nfui_nfobject_show(main_wnd);
    nfui_make_key_hierarchy(main_wnd);

    g_timeout_add(1000, _init_page_start, NULL);
    
    gtk_main();
    
    return FALSE;    
}
