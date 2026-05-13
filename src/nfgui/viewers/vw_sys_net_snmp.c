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


#define TEXT_SIZE           (64)

///

#define NE_SNMP_TTILE       (28)

#define NE_LABEL_LEFT       (28)
#define NE_LABEL_TOP        (48+40+20)

#define NE_LABEL_LEFT_SPACE (50)
#define NE_LABEL_UP_SPACE   (15)

#define NE_LABEL_SPACE      (5)

#define NE_LABEL_H          (40)
#define NE_LABEL_W          (300)

#define NE_LABEL_X          (NE_LABEL_LEFT_SPACE+NE_LABEL_W)

// SNMP 서버 없음 

// SNMP V2c 

#define NE_V2C_TITLE_X      (NE_LABEL_LEFT)    
#define NE_V2C_TITLE_Y      (NE_LABEL_TOP+NE_LABEL_H)


#define NE_V2C_CM_X         (NE_LABEL_LEFT_SPACE)
#define NE_V2C_CM_Y         (NE_V2C_TITLE_Y+NE_LABEL_UP_SPACE+NE_LABEL_H)
#define NE_V2C_CM_L_X       (NE_LABEL_X)
#define NE_V2C_CM_L_Y       (NE_V2C_CM_Y)


#define NE_V2C_TR_X         (NE_LABEL_LEFT_SPACE)
#define NE_V2C_TR_Y         (NE_V2C_CM_Y+NE_LABEL_H+NE_LABEL_SPACE)

#define NE_V2C_AD_X         (NE_LABEL_LEFT_SPACE+40)
#define NE_V2C_AD_Y         (NE_V2C_TR_Y+NE_LABEL_H+NE_LABEL_SPACE)
#define NE_V2C_AD_L_X       (NE_LABEL_X)
#define NE_V2C_AD_L_Y       (NE_V2C_AD_Y)

#define NE_V2C_CM2_X2       (NE_LABEL_LEFT_SPACE+40)
#define NE_V2C_CM2_Y2       (NE_LABEL_H+NE_LABEL_SPACE+NE_V2C_AD_L_Y)
#define NE_V2C_CM2_L_X2     (NE_LABEL_X)
#define NE_V2C_CM2_L_Y2     (NE_V2C_CM2_Y2)


// SNMP V3

#define NE_V3_TITLE_X       (NE_LABEL_LEFT)
#define NE_V3_TITLE_Y       (NE_V2C_CM2_L_Y2+NE_LABEL_H+NE_LABEL_SPACE+NE_LABEL_UP_SPACE)

#define NE_V3_EN_X          (NE_LABEL_LEFT_SPACE)
#define NE_V3_EN_Y          (NE_LABEL_H+NE_LABEL_UP_SPACE+NE_V3_TITLE_Y)
#define NE_V3_EN_T_X        (NE_LABEL_X)
#define NE_V3_EN_T_Y        (NE_V3_EN_Y)

#define NE_V3_USER_X        (NE_LABEL_LEFT_SPACE)    
#define NE_V3_USER_Y        (NE_V3_EN_T_Y)
#define NE_V3_USER_T_X      (NE_LABEL_X)
#define NE_V3_USER_T_Y      (NE_V3_USER_Y)

#define NE_V3_LABEL_3_X      (NE_LABEL_LEFT_SPACE)
#define NE_V3_LABEL_3_Y      (NE_LABEL_H+NE_LABEL_SPACE+NE_V3_USER_T_Y)
#define NE_V3_LABEL_3_SPIN_X (NE_LABEL_X)
#define NE_V3_LABEL_3_SPIN_Y (NE_V3_LABEL_3_Y)
#define NE_V3_LABEL_3_T_X    (NE_LABEL_X+305)
#define NE_V3_LABEL_3_T_Y    (NE_V3_LABEL_3_Y)

#define NE_V3_LABEL_4_X      (NE_LABEL_LEFT_SPACE)
#define NE_V3_LABEL_4_Y      (NE_LABEL_H+NE_LABEL_SPACE+NE_V3_LABEL_3_T_Y)
#define NE_V3_LABEL_4_SPIN_X (NE_LABEL_X)
#define NE_V3_LABEL_4_SPIN_Y (NE_V3_LABEL_4_Y)
#define NE_V3_LABEL_4_T_X    (NE_LABEL_X+305) 
#define NE_V3_LABEL_4_T_Y    (NE_V3_LABEL_4_Y)

#define NE_V3_LABEL_5_X     (NE_LABEL_LEFT_SPACE)
#define NE_V3_LABEL_5_Y     (NE_LABEL_H+NE_LABEL_SPACE+NE_V3_LABEL_4_T_Y)

#define NE_V3_LABEL_6_X     (NE_LABEL_LEFT_SPACE+40)
#define NE_V3_LABEL_6_Y     (NE_LABEL_H+NE_LABEL_SPACE+NE_V3_LABEL_5_Y)
#define NE_V3_LABEL_6_T_X   (NE_LABEL_X)
#define NE_V3_LABEL_6_T_Y   (NE_V3_LABEL_6_Y)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *snmp_sel_obj[3];

static SnmpData snmp_data;
static SnmpData org_snmp_data;

static NFOBJECT *auth_combo;
static NFOBJECT *priv_combo;
static NFOBJECT *v2_cmt_str;
static NFOBJECT *v2_addr_tr;
static NFOBJECT *v2_cmt_str_tr;

static NFOBJECT *v3_user_id;
static NFOBJECT *v3_auth_key;
static NFOBJECT *v3_priv_key;
static NFOBJECT *v3_addr_tr;

enum{
    SNMP_NONE =0,
    SNMP_V2C,
    SNMP_V3,
    SNMP_CNT
};


static void init_snmp()
{
    memset(&snmp_data,0x00,sizeof(SnmpData));
    memset(&org_snmp_data,0x00,sizeof(SnmpData));

    DAL_get_SnmpData(&snmp_data);

    g_memmove(&org_snmp_data,&snmp_data, sizeof(SnmpData));
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
        }
        break;

        default :
            break;  
          
    }

    return FALSE;
}


static gboolean post_snmp_select_event_cb(NFOBJECT *obj,GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS)
    {
        gint idx; 

        idx = nfui_radio_button_get_index((NFBUTTON*)obj);
        
        snmp_data.snmp_mode = idx;
        
        nfui_user_signal_emit(g_curwnd, NFEVENT_SNMP_RADIO_BUTTON_PRESS, TRUE);

    }
    return FALSE;
}

static gboolean post_comunity_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    
    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE, VKEY_NORMAL);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v2c_cmt_str,str);
                
                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V2C]);
       
        if(!ret) nfui_nfobject_disable(obj);
        else    nfui_nfobject_enable(obj);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}

static gboolean post_address_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE, VKEY_NORMAL);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v2c_address_tr,str);
                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V2C]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else    nfui_nfobject_enable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}
static gboolean post_comunity_str_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE, VKEY_NORMAL);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v2c_cmt_str_tr,str);

                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V2C]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else    nfui_nfobject_enable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_engineid_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else    nfui_nfobject_enable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_user_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    
    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE, VKEY_NORMAL);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v3_userid,str);
                         
                ifree(str);
                str = NULL;

                nfui_user_signal_emit(g_curwnd, NFEVENT_SNMP_RADIO_BUTTON_PRESS, TRUE);
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else    nfui_nfobject_enable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_auth_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint ret;

        ret = nfui_combobox_get_cur_index(obj);

        if(!ret)
        {
            nfui_nfobject_disable(v3_auth_key);
            nfui_nfobject_disable(priv_combo);
            nfui_nfobject_disable(v3_priv_key);
        }
        else 
        {
            nfui_nfobject_enable(v3_auth_key);
            nfui_nfobject_enable(priv_combo);

            if(nfui_combobox_get_cur_index(priv_combo))
                nfui_nfobject_enable(v3_priv_key);
        }

        snmp_data.v3_auth_combo = ret;

        nfui_signal_emit(v3_auth_key,GDK_EXPOSE,TRUE);
        nfui_signal_emit(priv_combo,GDK_EXPOSE, TRUE);
        nfui_signal_emit(v3_priv_key, GDK_EXPOSE, TRUE);
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 
        gint idx;

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);

        idx = nfui_combobox_get_cur_index(obj);

        if(!idx)
        {   
            nfui_nfobject_disable(v3_auth_key);
            nfui_nfobject_disable(priv_combo);
            nfui_nfobject_disable(v3_priv_key);
        }
        else
        {
            nfui_nfobject_enable(v3_auth_key);
            nfui_nfobject_enable(priv_combo);
            
            if(nfui_combobox_get_cur_index(priv_combo))
                nfui_nfobject_enable(v3_priv_key);
        }
        
        if(!ret) nfui_nfobject_disable(obj);
        else
        {   
            
            if(!nfui_nflabel_get_strlen(v3_user_id))
            {
                nfui_nfobject_disable(obj);
            }
            else
            {
                nfui_nfobject_enable(obj);
            }
        }

        nfui_signal_emit(v3_auth_key, GDK_EXPOSE, TRUE);
        nfui_signal_emit(priv_combo, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v3_priv_key, GDK_EXPOSE, TRUE);
        
        nfui_user_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_auth_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey3_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v3_auth_key,str);

                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 
        gint idx;

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else
        {
            idx = nfui_combobox_get_cur_index(auth_combo);
               
            if(!idx) nfui_nfobject_disable(obj);
            else
            { 
                if(!nfui_nflabel_get_strlen(v3_user_id))
                {
                    nfui_nfobject_disable(obj);
                }
                else
                {
                    nfui_nfobject_enable(obj);
                }
            }
        }
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_personal_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint ret;

        ret = nfui_combobox_get_cur_index(obj);

        if(!ret)
            nfui_nfobject_disable(v3_priv_key);
        else
            nfui_nfobject_enable(v3_priv_key);


        snmp_data.v3_priv_combo = ret;

        nfui_signal_emit(v3_priv_key, GDK_EXPOSE, TRUE);
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        gint idx;

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);

        idx = nfui_combobox_get_cur_index(obj);
        
        if(!ret)
            nfui_nfobject_disable(obj);
        else
        {
            if(nfui_nfobject_is_disabled(v3_auth_key))
            {
                nfui_nfobject_disable(obj);
            }
            else
            { 
                if(!nfui_nflabel_get_strlen(v3_user_id))
                {
                    nfui_nfobject_disable(obj);
                }
                else
                {
                    nfui_nfobject_enable(obj);
                }
            }

            if(!idx)
                nfui_nfobject_disable(v3_priv_key);
            else
                nfui_nfobject_enable(v3_priv_key);
        }    

        nfui_signal_emit(v3_priv_key, GDK_EXPOSE, TRUE);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_personal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey3_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v3_priv_key,str);

                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 
        gint idx;

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else
        {
            idx = nfui_combobox_get_cur_index(priv_combo);
            
            if(!idx) nfui_nfobject_disable(obj);
            else
            {   
                if(!nfui_combobox_get_cur_index(auth_combo))
                    nfui_nfobject_disable(obj);
                else
                { 
                    if(!nfui_nflabel_get_strlen(v3_user_id))
                    {
                        nfui_nfobject_disable(obj);
                    }
                    else
                    {
                        nfui_nfobject_enable(obj);
                    }
                }
            }
        }       
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}
static gboolean post_address_tr_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, TEXT_SIZE, VKEY_NORMAL);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);
                strcpy(snmp_data.v3_address_tr,str);

                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    else if(evt->type == NFEVENT_SNMP_RADIO_BUTTON_PRESS)
    {
        gint ret; 

        ret = nfui_radio_button_get_toggled((NFBUTTON *) snmp_sel_obj[SNMP_V3]);
        
        if(!ret) nfui_nfobject_disable(obj);
        else nfui_nfobject_enable(obj);
        /*else
        { 
            if(!nfui_nflabel_get_strlen(v3_user_id))
            {
                nfui_nfobject_disable(obj);
            }
            else
            {
                nfui_nfobject_enable(obj);
            }
        }*/

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}


static gboolean post_cancelbutton_event_handler(NFOBJECT *obj,GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }
        
        g_memmove(&snmp_data, &org_snmp_data, sizeof(SnmpData));        

        nfui_radio_button_set_toggled(snmp_sel_obj[snmp_data.snmp_mode], TRUE);

        nfui_nflabel_set_text(v2_cmt_str, org_snmp_data.v2c_cmt_str);
        nfui_nflabel_set_text(v2_addr_tr, org_snmp_data.v2c_address_tr);
        nfui_nflabel_set_text(v2_cmt_str_tr, org_snmp_data.v2c_cmt_str_tr);

        nfui_combobox_set_index_no_expose(auth_combo, org_snmp_data.v3_auth_combo);
        nfui_combobox_set_index_no_expose(priv_combo, org_snmp_data.v3_priv_combo);

        nfui_nflabel_set_text(v3_user_id, org_snmp_data.v3_userid);
        nfui_nflabel_set_text(v3_auth_key, org_snmp_data.v3_auth_key);
        nfui_nflabel_set_text(v3_priv_key, org_snmp_data.v3_auth_key);
        nfui_nflabel_set_text(v3_addr_tr, org_snmp_data.v3_auth_key);

        for(i=0; i<SNMP_CNT; i++)
            nfui_signal_emit(snmp_sel_obj[i], GDK_EXPOSE, TRUE);
            
        nfui_user_signal_emit(g_curwnd, NFEVENT_SNMP_RADIO_BUTTON_PRESS, TRUE);

        /*
        nfui_signal_emit(v2_cmt_str, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v2_addr_tr, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v2_cmt_str_tr, GDK_EXPOSE, TRUE);
        nfui_signal_emit(auth_combo, GDK_EXPOSE, TRUE);
        nfui_signal_emit(priv_combo, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v3_user_id, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v3_auth_key, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v3_priv_key, GDK_EXPOSE, TRUE);
        nfui_signal_emit(v3_addr_tr, GDK_EXPOSE, TRUE);       
        */
    }   
    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj,GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }
        
        if(memcmp(&org_snmp_data,&snmp_data, sizeof(SnmpData)))
        {
            g_memmove(&org_snmp_data, &snmp_data, sizeof(SnmpData));

            DAL_set_SnmpData(&snmp_data);
            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			sysnet_set_changeflag(1);
        }        
    }

    return FALSE;
}
static gboolean post_closebutton_event_handler(NFOBJECT *obj,GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        NetSnmp_tab_out_handler();
        SystemSetupNetwork_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if( evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

void init_NetSnmp_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    GdkPixbuf   *radio_img[NFOBJECT_STATE_COUNT];  
    GSList *slist = NULL;

    gchar *combo_str1[] = {"NONE","MD5","SHA"};
    gchar *combo_str2[] = {"NONE","DES","AES"};
    gint size_w, size_h;
    gint i;   

    g_curwnd = nfui_nfobject_get_top(parent);

    radio_img[0] = nfui_get_image_from_file((IMG_N_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_RADIO_OFF), NULL);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
    nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);   
    
    // mem set data
    // DAL get 하기 

    init_snmp();

    nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    for(i=0; i<SNMP_CNT; i++)
    {
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_snmp_select_event_cb);
        
        if(i == snmp_data.snmp_mode)  nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
       
        if(i == SNMP_NONE) slist = nfui_radio_button_get_group(NF_BUTTON(obj));
        else    nfui_radio_button_add_group(NF_BUTTON(obj),slist);

        if(i == SNMP_NONE)
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, NE_LABEL_LEFT, NE_LABEL_TOP);
        else if( i == SNMP_V2C)
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, NE_V2C_TITLE_X, NE_V2C_TITLE_Y);
        else
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, NE_V3_TITLE_X,NE_V3_TITLE_Y);
        snmp_sel_obj[i]= obj;
    }

    obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SNMP CONFIGURATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, NE_LABEL_LEFT, NE_SNMP_TTILE);
    
    obj = nfui_nflabel_new_with_pango_font("SNMP SERVER NONE",nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_TITLE_X+size_w+10,NE_LABEL_TOP-((40-size_h)/2));     

    obj = nfui_nflabel_new_with_pango_font("SNMP V2c",nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_TITLE_X+size_w+10,NE_V2C_TITLE_Y-((40-size_h)/2));     

    obj = nfui_nflabel_new_with_pango_font("SNMP V3",nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_TITLE_X+size_w+10,NE_V3_TITLE_Y-((40-size_h)/2));     

    obj = nfui_nflabel_new_with_pango_font("COMMUNITY STRINGS",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_CM_X,NE_V2C_CM_Y);     

    obj = nfui_nflabel_new_text_box(snmp_data.v2c_cmt_str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_CM_L_X ,NE_V2C_CM_L_Y);
    nfui_regi_post_event_callback(obj, post_comunity_event_handler);
    v2_cmt_str = obj;


    obj = nfui_nflabel_new_with_pango_font("TRAP CONFIGURATION",nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_TR_X,NE_V2C_TR_Y);     

    obj = nfui_nflabel_new_with_pango_font("ADDRESS",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_AD_X,NE_V2C_AD_Y);
    

    obj = nfui_nflabel_new_text_box(snmp_data.v2c_address_tr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_AD_L_X ,NE_V2C_AD_L_Y);
    nfui_regi_post_event_callback(obj, post_address_event_handler);
    v2_cmt_str_tr = obj;


    obj = nfui_nflabel_new_with_pango_font("COMMUNITY STRINGS",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_CM2_X2,NE_V2C_CM2_Y2);    
    

    obj = nfui_nflabel_new_text_box(snmp_data.v2c_cmt_str_tr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V2C_CM2_L_X2 ,NE_V2C_CM2_L_Y2);
    nfui_regi_post_event_callback(obj, post_comunity_str_event_handler);
    v2_addr_tr = obj;
    
    /* not use jaeyoung 
    obj = nfui_nflabel_new_with_pango_font("ENGINE ID",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_EN_X,NE_V3_EN_Y);    


    obj = nfui_nflabel_new_text_box(snmp_data.v3_engine_id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_EN_T_X ,NE_V3_EN_T_Y);
    nfui_regi_post_event_callback(obj, post_engineid_event_handler);
    v3_engine_id = obj;
    */

    obj = nfui_nflabel_new_with_pango_font("SNMP USER",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_USER_X,NE_V3_USER_Y);    
    
    obj = nfui_nflabel_new_text_box(snmp_data.v3_userid, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_USER_T_X ,NE_V3_USER_T_Y);
    nfui_regi_post_event_callback(obj, post_user_event_handler);
    v3_user_id = obj;
    

    obj = nfui_nflabel_new_with_pango_font("AUTHENTICATION",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_3_X,NE_V3_LABEL_3_Y);    

    obj = nfui_combobox_new(combo_str1, 3, snmp_data.v3_auth_combo);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_3_SPIN_X,NE_V3_LABEL_3_SPIN_Y);
    nfui_regi_post_event_callback(obj, post_auth_combo_event_handler);
    auth_combo = obj;
    
    
    obj = nfui_nflabel_new_text_box(snmp_data.v3_auth_key, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_3_T_X ,NE_V3_LABEL_3_T_Y);
    nfui_regi_post_event_callback(obj, post_auth_event_handler);
    v3_auth_key = obj;
    

    obj = nfui_nflabel_new_with_pango_font("PERSONAL INFORMATION",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_4_X,NE_V3_LABEL_4_Y);    

    obj = nfui_combobox_new(combo_str2, 3, snmp_data.v3_priv_combo);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_4_SPIN_X,NE_V3_LABEL_4_SPIN_Y);
    nfui_regi_post_event_callback(obj, post_personal_combo_event_handler);
    priv_combo = obj;
       
    obj = nfui_nflabel_new_text_box(snmp_data.v3_priv_key, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_4_T_X ,NE_V3_LABEL_4_T_Y);
    nfui_regi_post_event_callback(obj, post_personal_event_handler);
    v3_priv_key = obj;
    

    obj = nfui_nflabel_new_with_pango_font("TRAP CONFIGURATION",nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_5_X,NE_V3_LABEL_5_Y);    

    obj = nfui_nflabel_new_with_pango_font("ADDRESS",nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120)); 
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_6_X,NE_V3_LABEL_6_Y);    

    obj = nfui_nflabel_new_text_box(snmp_data.v3_address_tr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj,300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj,NE_V3_LABEL_6_T_X ,NE_V3_LABEL_6_T_Y);
    nfui_regi_post_event_callback(obj, post_address_tr_event_handler);
    v3_addr_tr = obj;
    

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

    obj = nftool_normal_button_create_type1("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_user_signal_emit(g_curwnd, NFEVENT_SNMP_RADIO_BUTTON_PRESS, TRUE);
}

gboolean NetSnmp_tab_out_handler()
{
    mb_type ret;

    if(!memcmp(&org_snmp_data, &snmp_data, sizeof(SnmpData)))
        return FALSE;
   	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
							NFTOOL_MB_OKCANCEL);
    if(ret == NFTOOL_MB_OK)
    {
        if(memcmp(&org_snmp_data, &snmp_data, sizeof(SnmpData)))
        {
            g_memmove(&org_snmp_data, &snmp_data, sizeof(SnmpData));
            DAL_set_SnmpData(&snmp_data);
        }

        sysnet_set_changeflag(1);
    }
    else
    {
        g_memmove(&snmp_data, &org_snmp_data, sizeof(SnmpData));        

        snmp_data.snmp_mode = org_snmp_data.snmp_mode;

        nfui_nflabel_set_text(v2_cmt_str, org_snmp_data.v2c_cmt_str);
        nfui_nflabel_set_text(v2_addr_tr, org_snmp_data.v2c_address_tr);
        nfui_nflabel_set_text(v2_cmt_str_tr, org_snmp_data.v2c_cmt_str_tr);

        nfui_combobox_set_index_no_expose(auth_combo, org_snmp_data.v3_auth_combo);
        nfui_combobox_set_index_no_expose(priv_combo, org_snmp_data.v3_priv_combo);

        nfui_nflabel_set_text(v3_user_id, org_snmp_data.v3_userid);
        nfui_nflabel_set_text(v3_auth_key, org_snmp_data.v3_auth_key);
        nfui_nflabel_set_text(v3_priv_key, org_snmp_data.v3_auth_key);
        nfui_nflabel_set_text(v3_addr_tr, org_snmp_data.v3_auth_key);
     
    }
    
    return FALSE;
}
