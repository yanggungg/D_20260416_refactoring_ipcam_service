
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfimage.h"

#include "vw_evt_act_main.h"
#include "vw_textin_evt.h"
#include "vw_alarm_sensor_evt.h"


#define DEV_COUNT   8

enum {
	ACTION_LCAMERA = 0,
	ACTION_AOUT,
	ACTION_BUZZER,
	ACTION_VPOPUP,	
	ACTION_EMAIL,
	ACTION_FTP,
#if defined(_SUPPORT_USER_PHONE)	
	ACTION_MOBILE,		
#endif	
	ACTION_MAX
};




static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_lcamObj[DEV_COUNT];
static NFOBJECT *g_almObj[DEV_COUNT];
static NFOBJECT *g_buzzObj[DEV_COUNT];
static NFOBJECT *g_vpopObj[DEV_COUNT];
static NFOBJECT *g_mailObj[DEV_COUNT];
static NFOBJECT *g_ftpObj[DEV_COUNT];
static NFOBJECT *g_mobileObj[DEV_COUNT];
static NFOBJECT *g_buzzAll;
static NFOBJECT *g_vpopAll;
static NFOBJECT *g_mailAll;
static NFOBJECT *g_ftpAll;
static NFOBJECT *g_mobileAll;


static void set_data_to_obj(gint expose)
{
    gint i;

    for (i = 0; i < DEV_COUNT; i++) 
    {
        if (expose)
        {
/*        
            nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzObj[i], g_vcad[i].buzzer);
            nfui_check_button_set_active((NFCHECKBUTTON*)g_vpopObj[i], g_vcad[i].vpop);         
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mailObj[i], g_vcad[i].email);
            nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpObj[i], g_vcad[i].ftp);       
#if defined(_SUPPORT_USER_PHONE)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileObj[i], g_vcad[i].mobile);         
#endif
*/
        }
        else
        {
/*        
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_buzzObj[i], g_vcad[i].buzzer);
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_vpopObj[i], g_vcad[i].vpop);           
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mailObj[i], g_vcad[i].email);
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ftpObj[i], g_vcad[i].ftp);         
#if defined(_SUPPORT_USER_PHONE)
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mobileObj[i], g_vcad[i].mobile);
#endif
*/
        }
    }
}

static void get_data_from_obj()
{
    gint i;
    gchar *str;

    for (i = 0; i < DEV_COUNT; i++) 
    {
/*    
        g_vcad[i].buzzer = nfui_check_button_get_active((NFCHECKBUTTON*)g_buzzObj[i]);
        g_vcad[i].vpop = nfui_check_button_get_active((NFCHECKBUTTON*)g_vpopObj[i]);        
        g_vcad[i].email = nfui_check_button_get_active((NFCHECKBUTTON*)g_mailObj[i]);
        g_vcad[i].ftp = nfui_check_button_get_active((NFCHECKBUTTON*)g_ftpObj[i]);
#if defined(_SUPPORT_USER_PHONE)        
        g_vcad[i].mobile = nfui_check_button_get_active((NFCHECKBUTTON*)g_mobileObj[i]);        
#endif
*/
    }
}

static void conv_cam_data_to_string(guint data, gchar str[])
{
	guint i, j;
	gint bytes;

	for(i=0, j=0; i<GUI_CHANNEL_CNT; i++) {
		if(data & (1 << i)) {
			if(j < 1) bytes = g_sprintf(&str[j], "%d", i+1);
			else 	  bytes = g_sprintf(&str[j], ",%d", i+1);

			j += bytes;
		}
	}

	if(j == 0) g_sprintf(str, "%s", "N/A");
}

static void conv_alm_out_data_to_string(guint64 data, gchar str[])
{
	guint i, j;
	gchar tmp[8];

#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	for(i=0, j=0; i<ALARM_OUT_COUNT; i++) {
		memset(tmp, 0x00, sizeof(tmp));	
		if(data & (1 << i)) {
			if(j < 1) 	g_sprintf(tmp, "A%d", i+1);
			else 	  	g_sprintf(tmp, ",A%d", i+1);

			j++;
		}
		strcat(str, tmp);
	}
#else
	gint bytes;

	for(i=0, j=0; i<ALARM_OUT_COUNT; i++) {
		if(data & (1LL << i)) {
			if(i < CAM_ALARM_OUT) 
			{
				if(j < 1) bytes = g_sprintf(&str[j], "%d", i+1);
				else 	  bytes = g_sprintf(&str[j], ",%d", i+1);
			}
#if defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			else if (i < CAM_ALARM_OUT+NVR_RELAY_OUT)
			{
					if(j < 1) bytes = g_sprintf(&str[j], "R%d", i-CAM_ALARM_OUT+1);
					else      bytes = g_sprintf(&str[j], ",R%d", i-CAM_ALARM_OUT+1);
			}
            else 
			{
				if(j < 1) bytes = g_sprintf(&str[j], "A%d", i-CAM_ALARM_OUT-NVR_RELAY_OUT+1);
				else 	  bytes = g_sprintf(&str[j], ",A%d", i-CAM_ALARM_OUT-NVR_RELAY_OUT+1);
			}
#else
			else 
			{
				if(j < 1) bytes = g_sprintf(&str[j], "A%d", i-CAM_ALARM_OUT+1);
				else 	  bytes = g_sprintf(&str[j], ",A%d", i-CAM_ALARM_OUT+1);
			}
#endif
			j += bytes;
		}
	}
#endif

	if(j == 0) g_sprintf(str, "%s", "N/A");
}

static gboolean post_lc_all_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_PRESS) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	}

	return FALSE;
}

static gboolean post_ao_all_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_PRESS) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	}

	return FALSE;
}

static gboolean post_lc_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}
	
	return FALSE;
}

static gboolean post_ao_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}
	
	return FALSE;
}

static gboolean post_lc_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_PRESS) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	}

	return FALSE;
}

static gboolean post_ao_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_PRESS) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	}

	return FALSE;
}

static gboolean post_buzzer_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gint i;
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        for (i = 0; i < DEV_COUNT; i++)
        {
            nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzObj[i], state);
        }
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_DOWN) 
        {
            change_obj_focus(obj, g_buzzObj[0]);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean post_buzzer_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if (!state) nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzAll, state);
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;
        gint i;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < DEV_COUNT; i++) 
        {
            if (g_buzzObj[i] == obj) break;
        }

        if (kpid == KEYPAD_UP) 
        {
            if (i - 1 >= 0) 
            {
                change_obj_focus(obj, g_buzzObj[i-1]);
                return TRUE;
            }
            else 
            {
                change_obj_focus(obj, g_buzzAll);
                return TRUE;
            }
        }
        else if(kpid == KEYPAD_DOWN) 
        {
            if (i + 1 < DEV_COUNT) {
                change_obj_focus(obj, g_buzzObj[i+1]);
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean post_vpop_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gint i;
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        for (i = 0; i < DEV_COUNT; i++)
        {
            nfui_check_button_set_active((NFCHECKBUTTON*)g_vpopObj[i], state);
        }
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_DOWN) 
        {
            change_obj_focus(obj, g_vpopObj[0]);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_vpop_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if (!state) nfui_check_button_set_active((NFCHECKBUTTON*)g_vpopAll, state);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        NFOBJECT *bottom;
        GdkEventKey *kevt;
        KEYPAD_KID kpid;
        gint i;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < DEV_COUNT; i++) 
        {
            if (g_vpopObj[i] == obj) break;
        }

        if (kpid == KEYPAD_UP) 
        {
            if (i - 1 >= 0) 
            {
                change_obj_focus(obj, g_vpopObj[i-1]);
                return TRUE;
            }
            else 
            {
                change_obj_focus(obj, g_vpopAll);
                return TRUE;
            }
        }
        else if (kpid == KEYPAD_DOWN) 
        {
            if (i + 1 < DEV_COUNT) 
            {
                change_obj_focus(obj, g_vpopObj[i+1]);
                return TRUE;
            }
            else 
            {
                bottom = nfui_nfobject_get_data(obj, "bottom focus");
                if(bottom) {
                    change_obj_focus(obj, bottom);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static gboolean post_email_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gint i;
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        for (i = 0; i < DEV_COUNT; i++)
        {
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mailObj[i], state);
        }
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_DOWN) 
        {
            change_obj_focus(obj, g_mailObj[0]);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_mail_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if (!state) nfui_check_button_set_active((NFCHECKBUTTON*)g_mailAll, state);
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        NFOBJECT *bottom;
        GdkEventKey *kevt;
        KEYPAD_KID kpid;
        gint i;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < DEV_COUNT; i++) 
        {
            if (g_mailObj[i] == obj) break;
        }

        if (kpid == KEYPAD_UP) 
        {
            if (i - 1 >= 0) 
            {
                change_obj_focus(obj, g_mailObj[i-1]);
                return TRUE;
            }
            else 
            {
                change_obj_focus(obj, g_mailAll);
                return TRUE;
            }
        }
        else if(kpid == KEYPAD_DOWN) 
        {
            if (i + 1 < DEV_COUNT) 
            {
                change_obj_focus(obj, g_mailObj[i+1]);
                return TRUE;
            }
            else 
            {
                bottom = nfui_nfobject_get_data(obj, "bottom focus");
                if(bottom) {
                    change_obj_focus(obj, bottom);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static gboolean post_ftp_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gint i;
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        for (i = 0; i < DEV_COUNT; i++)
        {
            nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpObj[i], state);
        }
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_DOWN) 
        {
            change_obj_focus(obj, g_ftpObj[0]);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_ftp_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if (!state) nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpAll, state);
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        NFOBJECT *bottom;
        GdkEventKey *kevt;
        KEYPAD_KID kpid;
        gint i;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < DEV_COUNT; i++)
        {
            if (g_ftpObj[i] == obj) break;
        }

        if (kpid == KEYPAD_UP) 
        {
            if (i - 1 >= 0) 
            {
                change_obj_focus(obj, g_ftpObj[i-1]);
                return TRUE;
            }
            else 
            {
                change_obj_focus(obj, g_ftpAll);
                return TRUE;
            }
        }
        else if (kpid == KEYPAD_DOWN) 
        {
            if (i + 1 < DEV_COUNT) 
            {
                change_obj_focus(obj, g_ftpObj[i+1]);
                return TRUE;
            }
            else 
            {
                bottom = nfui_nfobject_get_data(obj, "bottom focus");
                if(bottom) 
                {
                    change_obj_focus(obj, bottom);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static gboolean post_mobile_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gint i;
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        for (i = 0; i < DEV_COUNT; i++)
        {
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileObj[i], state);
        }
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_DOWN) 
        {
            change_obj_focus(obj, g_mobileObj[0]);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_mobile_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {
        gboolean state;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if (!state) nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileAll, state);
    }
    else if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        NFOBJECT *bottom;
        GdkEventKey *kevt;
        KEYPAD_KID kpid;
        gint i;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < DEV_COUNT; i++) 
        {
            if (g_mobileObj[i] == obj) break;
        }

        if (kpid == KEYPAD_UP) 
        {
            if (i - 1 >= 0) 
            {
                change_obj_focus(obj, g_mobileObj[i-1]);
                return TRUE;
            }
            else 
            {
                change_obj_focus(obj, g_mobileAll);
                return TRUE;
            }
        }
        else if(kpid == KEYPAD_DOWN) 
        {
            if (i + 1 < DEV_COUNT) 
            {
                change_obj_focus(obj, g_mobileObj[i+1]);
                return TRUE;
            }
            else 
            {
                bottom = nfui_nfobject_get_data(obj, "bottom focus");
                if(bottom) {
                    change_obj_focus(obj, bottom);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

    }

    return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


    }

    return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_TextInEvt_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}



void VW_Init_TextInEvt_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    NFOBJECT *fixed_temp;
    NFOBJECT *tbl = NULL;

	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *dropdown_img2[NFOBJECT_STATE_COUNT];

	gchar buf[16];
	gchar *strParam[] = {"ID", "DEVICE", "NAME", "LINKED CAMERA"};
	gchar *strAct[] = { "LINKED CAMERA", "ALARM OUT"};	
	gchar *strNone[] = {"NONE1", "NONE2"};

	guint pos_x, pos_y;
	gint size_w, size_h;
	guint i, j;
	guint tbl_w[12] = {35, 200, 35, 200, 35, 200, 35, 200, 35, 200, 35, 200};

    guint action_w[ACTION_MAX];
    guint action_pos_x;
    guint action_tot_w = 0; 
    guint legend_tot_w = 0; 
    guint legend_cnt = 0;   

    
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

    legend_cnt = ACTION_MAX-ACTION_BUZZER;

    for (i = 0; i < legend_cnt*2; i++)
        legend_tot_w += tbl_w[i];

    // legend ////////////////////////////////////////////////////////////////////////////////////////
    tbl = (NFOBJECT*)nfui_nftable_new(legend_cnt*2, 1, 1, 1, tbl_w, 35);    
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 1485-legend_tot_w, 0);
//  nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 700-legend_tot_w, 0);

    legend_cnt = 0;

    obj = nfui_nfimage_new(IMG_ACTION_BUZZER_02);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUZZER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

       obj = nfui_nfimage_new(IMG_ACTION_VIDEO_02);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

/*
    obj = nfui_nfimage_new(IMG_ACTION_OSD_02);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OSD POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);
*/

    obj = nfui_nfimage_new(IMG_ACTION_EMAIL_02);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("E-MAIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);   

    obj = nfui_nfimage_new(IMG_ACTION_FTP_02);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FTP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);   

#if defined(_SUPPORT_USER_PHONE)
    obj = nfui_nfimage_new(IMG_ACTION_MOBILE_02);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOBILE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);   
#endif  
    
    pos_x = 27;
    pos_y = 81;       


	// event parameter ////////////////////////////////////////////////////////////////////////////////////////
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT PARAMETER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 330, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 193, 40);

	pos_x = 27;
	pos_y = 81;

	for (i = 0; i < 3; i++) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strParam[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 164, 40);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (pos_x + (i * 166)), pos_y);
	}


	pos_y += (40 + 1);

	for (i = 0; i < DEV_COUNT; i++) 
	{
		g_sprintf(buf, "%d", i + 1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_set_size(obj, 164, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, (pos_y + (i * 41)));
	}
	
	pos_x += (164 + 2);

	for (i = 0; i < DEV_COUNT; i++) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("SENSOR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 164, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, (pos_y + (i * 41)));
	}
	
	pos_x += (164 + 2);

	for (i = 0; i < DEV_COUNT; i++) 
	{
		obj = nfui_spinbutton_new(strNone, 2, 0);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 164, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, (pos_y + (i * 41)));
	}


	// action ////////////////////////////////////////////////////////////////////////////////////////	
	pos_x += (164 + 4);
	action_pos_x = pos_x;
	pos_y = 40;

	nfui_get_image_size(IMG_N_DROPDOWN_02, &size_w, &size_h);

    action_w[ACTION_LCAMERA] = 177+size_w;
    action_w[ACTION_AOUT] = 150+size_w;
    action_w[ACTION_BUZZER] = 68;
    action_w[ACTION_VPOPUP] = 68;
    action_w[ACTION_EMAIL] = 68;
    action_w[ACTION_FTP] = 68;
#if defined(_SUPPORT_USER_PHONE)    
    action_w[ACTION_MOBILE] = 68;
#endif

    for (i = 0; i < ACTION_MAX; i++)
        action_tot_w += action_w[i];

    g_message("%s, %d, pos_x:%d, pos_y:%d, w:%d", __FUNCTION__, __LINE__, pos_x, pos_y, action_tot_w);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ACTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, action_tot_w + 2*(ACTION_MAX-1), 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_y += (40 + 1);

	for (i = 0; i < ACTION_MAX; i++) 
	{
		if (i <= ACTION_AOUT) 
		{
        	dropdown_img2[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_02), NULL);
        	dropdown_img2[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_02), NULL);
        	dropdown_img2[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_02), NULL);
        	dropdown_img2[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_02), NULL);

        	nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strAct[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			if (i == ACTION_LCAMERA)        nfui_nfobject_set_size(obj, action_w[ACTION_LCAMERA]-size_w, 40);
			else if (i == ACTION_AOUT)	    nfui_nfobject_set_size(obj, action_w[ACTION_AOUT]-size_w, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

			if (i == ACTION_LCAMERA)        pos_x += (action_w[ACTION_LCAMERA]-size_w);
			else if (i == ACTION_AOUT)      pos_x += (action_w[ACTION_AOUT]-size_w);

			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image((NFBUTTON*)obj, dropdown_img2);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
			if (i == ACTION_LCAMERA)        nfui_regi_post_event_callback(obj, post_lc_all_btn_event_handler);
			else if (i == ACTION_AOUT)	    nfui_regi_post_event_callback(obj, post_ao_all_btn_event_handler);

			pos_x += (size_w + 2);
		}
		else 
		{
			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_set_size(fixed_temp, action_w[i], 40);
			nfui_nfobject_show(fixed_temp);
			nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, (guint)pos_x, (guint)pos_y);

            if (i == ACTION_BUZZER)
            {
                gint icon_size_w, icon_size_h;
                
                nfui_get_image_size(IMG_ACTION_BUZZER_01, &icon_size_w, &icon_size_h);
                
    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_buzzer_all_event_handler);
                g_buzzAll = obj;

                for (j = 0; j < DEV_COUNT; j++) 
                {
//                    if (!g_asd[j].buzzer)   
//                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }
            
    			obj = nfui_nfimage_new(IMG_ACTION_BUZZER_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
            else if (i == ACTION_VPOPUP)
            {
                gint icon_size_w, icon_size_h;
                
                nfui_get_image_size(IMG_ACTION_VIDEO_01, &icon_size_w, &icon_size_h);
                
    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_vpop_all_event_handler);
                g_vpopAll = obj;

                for (j = 0; j < DEV_COUNT; j++) 
                {
//                    if (!g_asd[j].vpop)   
//                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }
                
    			obj = nfui_nfimage_new(IMG_ACTION_VIDEO_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);    			

            }
            else if (i == ACTION_EMAIL)
            {
                gint icon_size_w, icon_size_h;
                
                nfui_get_image_size(IMG_ACTION_EMAIL_01, &icon_size_w, &icon_size_h);
                
    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_email_all_event_handler);
                g_mailAll = obj;

                for (j = 0; j < DEV_COUNT; j++) 
                {
//                    if (!g_asd[j].email)   
//                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }
                
    			obj = nfui_nfimage_new(IMG_ACTION_EMAIL_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);    			
            }            
            else if (i == ACTION_FTP)
            {
                gint icon_size_w, icon_size_h;
                
                nfui_get_image_size(IMG_ACTION_FTP_01, &icon_size_w, &icon_size_h);
                
    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_ftp_all_event_handler);
                g_ftpAll = obj;

                for (j = 0; j < DEV_COUNT; j++) 
                {
//                    if (!g_asd[j].ftp)
//                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }
                
    			obj = nfui_nfimage_new(IMG_ACTION_FTP_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);    			
            }
#if defined(_SUPPORT_USER_PHONE)
            else if (i == ACTION_MOBILE)
            {
                gint icon_size_w, icon_size_h;
                
                nfui_get_image_size(IMG_ACTION_MOBILE_01, &icon_size_w, &icon_size_h);
                
    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_mobile_all_event_handler);
                g_mobileAll = obj;

                for (j = 0; j < DEV_COUNT; j++) 
                {
//                    if (!g_asd[j].mobile)
//                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }
                
    			obj = nfui_nfimage_new(IMG_ACTION_MOBILE_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);    			
            }
#endif
			pos_x += (action_w[i] + 2);        				
		}
	}

	pos_x = action_pos_x;
	pos_y += (40 + 1);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	for(j = 0; j < DEV_COUNT; j++) 
	{	
		memset(buf, 0x00, sizeof(buf));
		conv_cam_data_to_string(0, buf);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(129));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
		nfui_nfobject_use_tooltip(obj, FALSE);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_set_size(obj, action_w[ACTION_LCAMERA]-size_w, 40);
		nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_lc_label_event_handler);
		nfui_nfobject_set_data(obj, "channel", GUINT_TO_POINTER(j));
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)pos_x, (guint)(pos_y + (j * 41)));
        g_lcamObj[j] = obj;

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "channel", GUINT_TO_POINTER(j));
	    nfui_regi_post_event_callback(obj, post_lc_btn_event_handler);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)pos_x+action_w[ACTION_LCAMERA]-size_w, (guint)(pos_y + (j * 41)));
	}

	pos_x += (action_w[ACTION_LCAMERA] + 2);   

	for(j = 0; j < DEV_COUNT; j++) {	
		memset(buf, 0x00, sizeof(buf));
		conv_alm_out_data_to_string(0, buf);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(129));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
		nfui_nfobject_use_tooltip(obj, FALSE);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
        nfui_nfobject_set_size(obj, (ALARM_OUT_COUNT >= 2 ? action_w[ACTION_AOUT]-size_w : action_w[ACTION_AOUT]), 40);
		nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_ao_label_event_handler);
		nfui_nfobject_set_data(obj, "channel", GUINT_TO_POINTER(j));
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)pos_x, (guint)(pos_y + (j * 41)));
		g_almObj[j] = obj;

		if(ALARM_OUT_COUNT >= 2) {
			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nfobject_set_data(obj, "channel", GUINT_TO_POINTER(j));
			nfui_regi_post_event_callback(obj, post_ao_btn_event_handler);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)(pos_x+action_w[ACTION_AOUT]-size_w), (guint)(pos_y + (j * 41)));
		}
	}

	pos_x += (action_w[ACTION_AOUT] + 2);   

	for (i = ACTION_BUZZER ; i < ACTION_MAX; i++) 
	{
		for (j = 0; j < DEV_COUNT; j++) 
		{
			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(fixed_temp, action_w[i], 40);
			nfui_nfobject_show(fixed_temp);
			nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, pos_x, (pos_y + (j * 41)));

			if(i == ACTION_BUZZER)		    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
			else if(i == ACTION_VPOPUP)	    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
			else if(i == ACTION_EMAIL)	    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
			else if(i == ACTION_FTP)	    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)	    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
#endif

			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);			
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(action_w[i]-size_w)/2, (guint)(40-size_h)/2);

			if(i == ACTION_BUZZER)			g_buzzObj[j] = obj;
			else if(i == ACTION_VPOPUP)		g_vpopObj[j] = obj;
			else if(i == ACTION_EMAIL)		g_mailObj[j] = obj;
			else if(i == ACTION_FTP)		g_ftpObj[j] = obj;			
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)		g_mobileObj[j] = obj;
#endif

			if(i == ACTION_BUZZER) 		    nfui_regi_post_event_callback(obj, post_buzzer_chk_event_handler);
			else if(i == ACTION_VPOPUP)     nfui_regi_post_event_callback(obj, post_vpop_chk_event_handler);
			else if(i == ACTION_EMAIL)      nfui_regi_post_event_callback(obj, post_mail_chk_event_handler);
			else if(i == ACTION_FTP)        nfui_regi_post_event_callback(obj, post_ftp_chk_event_handler);			
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)     nfui_regi_post_event_callback(obj, post_mobile_chk_event_handler);
#endif
		}

		pos_x += (action_w[i] + 2);		
	}
	
	// button
	obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);	
}

gboolean VW_TextInEvt_tab_out_handler()
{
	return TRUE;
}
