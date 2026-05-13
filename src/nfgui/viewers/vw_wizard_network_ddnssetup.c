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
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"

#include "vw_passage_popup.h"
#include "vw_sys_net_main.h"
#include "nf_api_eventlog.h"
#include "nf_network.h"
#include "vw_vkeyboard.h"
#include "../service/ddns2_manager.h"
#include "viewers/vw_sys_net_info_map.h"

#include "vw_wizard_init.h"
#include "vw_wizard_network_ddnssetup.h"

#include "nf_api_disk.h"
#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"

#define HELP_STR "Setting up DDNS allows you to more easily connect\nto the device with a specified address rather than needing\nto memorize the device IP address and port numbers."

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
#define SUBJECT_LABEL_WIDTH         (360)
#define SUBJECT_LABEL_MARGIN        (0)

typedef enum
{
	ND_DDNS_ROW = 0,
	ND_DDNS_SERVER_ROW,
	ND_DDNS_NAME_ROW,
	ND_DDNS_ID_ROW,
	ND_DDNS_PWD_ROW,
	ND_DDNS_ADDR_ROW,
	NUM_ND_ROWS,
}
nd_row_type;

enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

typedef struct _DDNS_PARAM_T {
    DDNS_INFO_T info;
	gchar       addr[STRING_SIZE_256];
} DDNS_PARAM_T;

typedef struct _DDNS_NET_T {
    DDNS_CFG_T      cfg;
	DDNS_PARAM_T    param;
	DDNS_PARAM_T    org_param;
} DDNS_NET_T;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *value[NUM_ND_ROWS] = {0, };
static NFOBJECT *g_next_button;

static WIZARD_USERDATA_T *g_wizard_data;
static DDNSData ddnsdata;
static DDNSData org_ddnsdata;
static gint agr_policy;
static gint org_agr_policy;


static DDNS_NET_T g_ddnsNet[MAX_SUPPORT_DDNS];

static mb_type g_popup_ret = 0;
static gint g_idx = 0;
static gint make_nvr_row = 0, make_mac_row = 0;


static gint _init_ddns_info()
{
    gint i;

	memset(&g_ddnsNet, 0x00, sizeof(DDNS_NET_T)*MAX_SUPPORT_DDNS);

    for (i = 0; i < NUM_ND_ROWS; i++)
        value[i] = 0;

	ifn_tolower(ddnsdata.server);

    for (i = 0; i < var_get_ddns_cnt(); i++)
    {
        var_get_ddns_cfg(&g_ddnsNet[i].cfg, i);
    	ifn_tolower(g_ddnsNet[i].cfg.server);

        strcpy(g_ddnsNet[i].param.info.server, g_ddnsNet[i].cfg.server);

        if (!strcmp(g_ddnsNet[i].param.info.server, ddnsdata.server))
        {
        	strcpy(g_ddnsNet[i].param.info.hostname, ddnsdata.host_name);
            strcpy(g_ddnsNet[i].param.info.id, ddnsdata.id);
            strcpy(g_ddnsNet[i].param.info.pwd, ddnsdata.passwd);
        }
        else
        {
            scm_get_mac_addr_str(g_ddnsNet[i].param.info.hostname, 64);
        }

        if (!strcmp(g_ddnsNet[i].param.info.server, "s1.co.kr"))
            g_sprintf(g_ddnsNet[i].param.addr, "%s", g_ddnsNet[i].param.info.hostname);
        else
	        g_sprintf(g_ddnsNet[i].param.addr, "%s.%s", g_ddnsNet[i].param.info.hostname, g_ddnsNet[i].param.info.server);

    	ifn_tolower(g_ddnsNet[i].param.addr);

    	memcpy(&g_ddnsNet[i].org_param, &g_ddnsNet[i].param, sizeof(DDNS_PARAM_T));
    }

    return 0;
}

static int _make_ddns_result_msg(int ddns_ret, char *title, char *msg)
{
	switch (ddns_ret) {
    	case DDNS2_RES_NOTFOUND_ERR:
    		sprintf(title, "ERROR");
    		sprintf(msg, "It's an invalid URL.");
		break;
    	case DDNS2_RES_NOTACCEPT_MSG_ERR:
    		sprintf(title, "ERROR");
    		sprintf(msg, "It's an invalid owner name.");
		break;
    	case DDNS2_RES_HOST_CONFLICT_ERR:
    		sprintf(title, "ERROR");
    		sprintf(msg, "The host name is conflicted.");
		break;
    	case FUJIKODNS_RES_BADAUTH_ERROR:
    		sprintf(title, "NOTICE");
    		sprintf(msg, "Already Registered");
		break;
    	default:
    		sprintf(title, "ERROR");
    		sprintf(msg, lookup_string("The operation occured an error as a code %d."), ddns_ret);
		break;
	}
	return 0;
}

static gint _get_current_cfg_idx()
{
    gint idx;

    if (var_get_ddns_cnt() == 1) idx = 0;
    else idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ND_DDNS_SERVER_ROW]);

    return idx;
}

static gint _save_ddns_data()
{
    DAL_set_ddns_data(&ddnsdata);   
    g_memmove(&g_wizard_data->networkData.ddns_data, &ddnsdata, sizeof(DDNSData));    
    
    return 0;
}

static gint _save_policy_data()
{
    DAL_set_agr_policy(agr_policy);
    g_wizard_data->systemData.agr_policy = agr_policy;

	return 0;
}

static gint _exit_proc()
{
    gint retVal = 0;

    if (ivsc.dfunc.support_protect)
    {
        if (g_wizard_data->systemData.agr_policy == 0) {
            retVal = vw_provide_devinfo_warning_open(g_curwnd, "PREVIOUS", "FINISH");
            if (retVal == 0) return -1;
        }
    }

    nfui_nfobject_destroy(g_curwnd);    
	_wizard_finish();

	return 0;
}

static gint _next_step_proc()
{
    gint retVal = 0;

    if (ivsc.dfunc.support_protect)
    {
        if (g_wizard_data->systemData.agr_policy == 0) {
            retVal = vw_provide_devinfo_warning_open(g_curwnd, "PREVIOUS", "NEXT");
            if (retVal == 0) return -1;
        }
    }

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

static gboolean _register_test()
{
    gint idx;
    //DDNS_INFO_T info;

	idx = _get_current_cfg_idx();

	if (g_ddnsNet[idx].cfg.on_nvr)
	{
		if (!strlen(g_ddnsNet[idx].param.info.hostname)) {
			nftool_mbox(g_curwnd, "NOTICE", "DVR name is empty\nFill and save the DVR name.", NFTOOL_MB_OK);
			return FALSE;
		}
	}

    if (g_ddnsNet[idx].cfg.on_id)
    {
    	if (!strlen(g_ddnsNet[idx].param.info.id)) {
    		nftool_mbox(g_curwnd, "NOTICE", "User name is empty\nFill and save the user name.", NFTOOL_MB_OK);
    		return FALSE;
    	}
    }

    if (g_ddnsNet[idx].cfg.on_pwd)
	{
		if (!strlen(g_ddnsNet[idx].param.info.pwd)) {
			nftool_mbox(g_curwnd, "NOTICE", "password is empty\nFill and save the password.", NFTOOL_MB_OK);
    		return FALSE;
    	}
    }

	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
	scm_register_ddns_with_info(&g_ddnsNet[idx].param.info, IRET_SCM_REG_DDNS);

	return FALSE;
}

static gboolean _connection_test()
{
    gint idx;


    idx = _get_current_cfg_idx();

	if (g_ddnsNet[idx].cfg.on_nvr)
    {
		if (!strlen(g_ddnsNet[idx].param.info.hostname)) {
			nftool_mbox(g_curwnd, "NOTICE", "DVR name is empty\nFill and save the DVR name.", NFTOOL_MB_OK);
			return FALSE;
		}
    }

    if (g_ddnsNet[idx].cfg.on_id)
    {
    	if (!strlen(g_ddnsNet[idx].param.info.id)) {
    		nftool_mbox(g_curwnd, "NOTICE", "User name is empty\nFill and save the user name.", NFTOOL_MB_OK);
    		return FALSE;
    	}
    }

    if (g_ddnsNet[idx].cfg.on_pwd)
    {
    	if (!strlen(g_ddnsNet[idx].param.info.pwd)) {
    		nftool_mbox(g_curwnd, "NOTICE", "password is empty\nFill and save the password.", NFTOOL_MB_OK);
    		return FALSE;
    	}
    }

	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
	scm_test_ddns_with_info(&g_ddnsNet[idx].param.info, IRET_SCM_TST_DDNS);

	return FALSE;
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

static int _on_reg_ddns(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = ((CMM_MESSAGE_T *)data)->param;
	char *alias = ((CMM_MESSAGE_T *)data)->data;
	char title[32];
	char msg[256];
	mb_type popup_ret;

	if (wait_pop)
	{
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	if(ret == DDNS2_RES_OK_GOOD || ret == FUJIKODNS_RES_OK)
	{

		if (make_mac_row)
			_connection_test();
		else
		{
            _save_ddns_data();
			_next_step_proc();
		}
		return 0;
	}

	_make_ddns_result_msg(ret, title, msg);
	popup_ret = nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OKCANCEL);

	if(popup_ret == NFTOOL_MB_OK)
	{
		if(!_is_connected_cable())
		{
			popup_ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(popup_ret == NFTOOL_MB_OK) return 0;
		}
    	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
		scm_register_ddns_with_info(&g_ddnsNet[g_idx].param.info, IRET_SCM_REG_DDNS);
		return 0;
	}
	return 0;
}

static int _on_tst_ddns(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type popup_ret;
	int ret = ((CMM_MESSAGE_T *)data)->param;

	if (wait_pop)
	{
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	if (ret != 0)
	{
		popup_ret = nftool_mbox(g_curwnd, "ERROR", "DDNS Server failed to communicate.\nAre you sure you want to retry?", NFTOOL_MB_OKCANCEL);
		if(popup_ret == NFTOOL_MB_OK)
		{
			if(!_is_connected_cable())
			{
				popup_ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
				if(popup_ret == NFTOOL_MB_OK) return FALSE;
			}
        	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
			scm_test_ddns_with_info(&g_ddnsNet[g_idx].param.info, IRET_SCM_TST_DDNS);
			return 0;
		}
	}
	else
	{
        _save_ddns_data();
		_next_step_proc();
	}
	return 0;
}

static gint _sync_ddns_server_field(gint idx)
{
    if (ddnsdata.enable)
    {
        nfui_nfobject_enable(value[ND_DDNS_SERVER_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_SERVER_ROW]);
    }

    if (var_get_ddns_cnt() >= 1) {
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ND_DDNS_SERVER_ROW], idx);
    }

    return 0;
}

static gint _sync_ddns_host_field(gint idx)
{
    if (!value[ND_DDNS_NAME_ROW]) return -1;

    if ((g_ddnsNet[idx].cfg.on_nvr) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_NAME_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_NAME_ROW]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_NAME_ROW], g_ddnsNet[idx].param.info.hostname);

    return 0;
}

static gint _sync_ddns_id_field(gint idx)
{
    if (!value[ND_DDNS_ID_ROW]) return -1;

    if ((g_ddnsNet[idx].cfg.on_id) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_ID_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_ID_ROW]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_ID_ROW], g_ddnsNet[idx].param.info.id);

    return 0;
}

static gint _sync_ddns_pwd_field(gint idx)
{
    if (!value[ND_DDNS_PWD_ROW]) return -1;

    if ((g_ddnsNet[idx].cfg.on_pwd) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_PWD_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_PWD_ROW]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_PWD_ROW], g_ddnsNet[idx].param.info.pwd);

    return 0;
}

static gint _sync_ddns_addr_field(gint idx)
{
    if (!value[ND_DDNS_ADDR_ROW]) return -1;

    if ((g_ddnsNet[idx].cfg.on_mac) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_ADDR_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_ADDR_ROW]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_ADDR_ROW], g_ddnsNet[idx].param.addr);

    return 0;
}

static gint _refresh_ddns_addr(gint idx)
{
	gchar strBuf[256];

    memset(strBuf, 0x00, sizeof(strBuf));

    if (!strcmp(g_ddnsNet[idx].param.info.server, "s1.co.kr"))
        g_sprintf(strBuf, "%s", g_ddnsNet[idx].param.info.hostname);
    else
    g_sprintf(strBuf, "%s.%s", g_ddnsNet[idx].param.info.hostname, g_ddnsNet[idx].cfg.server);

    strcpy(g_ddnsNet[idx].param.addr, strBuf);
	return 0;
}

static gint _update_next_button_status()
{
    if (ivsc.dfunc.support_protect == 0) return -1;

    if (ddnsdata.enable)
    {
        if (agr_policy) nfui_nfobject_enable(g_next_button);
        else nfui_nfobject_disable(g_next_button);
    }
    else
    {
        nfui_nfobject_enable(g_next_button);
    }

    return 0;
}

static gboolean post_sub_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkPixbuf *pbuf = NULL;
	GdkGC *gc;
	gint gap_x, gap_y, size_w, size_h;

	switch (evt->type) {
    	case GDK_EXPOSE :
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

	        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
	        nfui_nfobject_get_size(obj, &size_w, &size_h);
	        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	        nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

			nfui_nfobject_gc_unref(gc);
		break;

    	case GDK_DELETE:
    		uxm_unreg_imsg_event(obj, IRET_SCM_REG_DDNS);
    		uxm_unreg_imsg_event(obj, IRET_SCM_TST_DDNS);
		break;

    	case IRET_SCM_REG_DDNS:
    		_on_reg_ddns(obj, evt, data);
		break;

    	case IRET_SCM_TST_DDNS:
    		_on_tst_ddns(obj, evt, data);
		break;

    	default :
		break;
	}
	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint img_w, img_h;

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
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
	    }

		if(memcmp(&org_ddnsdata, &ddnsdata, sizeof(DDNSData)))
		{
			g_memmove(&ddnsdata, &org_ddnsdata, sizeof(DDNSData));
			DAL_set_ddns_data(&ddnsdata);

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
		GTimeVal last_temp;
		gint i;
		gchar *pw;

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
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

        if(!_is_connected_cable())
        {
            ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
            if(ret == NFTOOL_MB_OK) return FALSE;
        }

        g_idx = _get_current_cfg_idx();

		ddnsdata.enable = nfui_radio_button_get_toggled((NFBUTTON*)value[ND_DDNS_ROW]);
        g_stpcpy(ddnsdata.server, g_ddnsNet[g_idx].param.info.server);
        g_stpcpy(ddnsdata.host_name, g_ddnsNet[g_idx].param.info.hostname);
        g_stpcpy(ddnsdata.id, g_ddnsNet[g_idx].param.info.id);
        g_stpcpy(ddnsdata.passwd, g_ddnsNet[g_idx].param.info.pwd); 

        _save_ddns_data();
        _save_policy_data();

		if(ddnsdata.enable)
		{
				//gboolean ret;
			gint idx;

			if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

			//// DVR NAME
			if (make_nvr_row)
				_register_test();
			//// DVR ADDRESS
			else if (make_mac_row)
				_connection_test();
		}
		else
		{
			if(memcmp(&org_ddnsdata, &ddnsdata, sizeof(DDNSData)))
			{
				for (i = 0; i < var_get_ddns_cnt(); i++)
					memcpy(&g_ddnsNet[i].org_param, &g_ddnsNet[i].param, sizeof(DDNS_PARAM_T));
			}
			_next_step_proc();
		}
	}

	return FALSE;
}

static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		gint idx;

		ddnsdata.enable = nfui_radio_button_get_toggled((NFBUTTON*)value[ND_DDNS_ROW]);

		if (!ddnsdata.enable) {
			nftool_mbox(g_curwnd, "NOTICE", "Remote video search from DDNS information Is not supported.", NFTOOL_MB_OK);
		}

        idx = _get_current_cfg_idx();

        if (_sync_ddns_server_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_SERVER_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_host_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_NAME_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_id_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ID_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_pwd_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_PWD_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_addr_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
        }     

        _update_next_button_status();
        nfui_signal_emit(g_next_button, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_ddns_server_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint idx;

        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if (_sync_ddns_host_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_NAME_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_id_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ID_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_pwd_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_PWD_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_addr_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
        }
	}

	return FALSE;
}

static gboolean post_ddns_field_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gint max_string_size = 12;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
		gint idx;
    	gchar buf[256];

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{
				return FALSE;
	  	   	}

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

        if (obj == value[ND_DDNS_NAME_ROW])
        {
            if (!strcmp(nfui_nflabel_get_text((NFLABEL*)obj), "FUJIKO.BIZ"))
                max_string_size = 24;
        }

		if(obj == value[ND_DDNS_PWD_ROW])
			strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_PASSWORD);
		else
			strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_ALPHANUMERIC);

		if(strTemp)
		{
			if(strlen(strTemp) == 0) return FALSE;

            idx = _get_current_cfg_idx();

            if (obj == value[ND_DDNS_NAME_ROW])
            {
                strcpy(g_ddnsNet[idx].param.info.hostname, strTemp);
                _refresh_ddns_addr(idx);

                nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_ADDR_ROW], g_ddnsNet[idx].param.addr);
    			nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
            }
            else if (obj == value[ND_DDNS_ID_ROW])
            {
                strcpy(g_ddnsNet[idx].param.info.id, strTemp);
            }
            else if (obj == value[ND_DDNS_PWD_ROW])
            {
                strcpy(g_ddnsNet[idx].param.info.pwd, strTemp);
            }

			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_provide_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
		gboolean act;

		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

        if (act == TRUE) {
            vw_provide_devinfo_notice_open(g_curwnd);
        }

        agr_policy = act;
        
        _update_next_button_status();        
        nfui_signal_emit(g_next_button, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_helpbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		NFOBJECT *top;
		guint x, y;
        PARAGRAPH_STR *para;
        gint i, cnt = 0;

  	   	if (evt->button.button == MOUSE_RIGTH_BUTTON)  return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x;
		y += top->y + obj->height + 2;

        para = imalloc(sizeof(PARAGRAPH_STR));

        para->intro[0] = g_strdup(HELP_STR);

		para->intro_type = BULLET_BLANK;
        para->intro_cnt = 1;

        vw_passage_popup_open(g_curwnd, x, y, DIR_TOP_RIGHT, &para, 1);

        for (i = 0; i < para->intro_cnt; i++)
        {
            if (para->intro[i]) g_free(para->intro[i]);
        }

        for (i = 0; i < para->body_cnt; i++)
        {
            if (para->body[i]) g_free(para->body[i]);
        }

        ifree(para);
	}

	return FALSE;
}

gint vw_wizard_network_ddnssetup_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *fixed2;
	NFOBJECT *obj;
	NFOBJECT *help_obj;

	NFOBJECT *btns[PIB_BUTTONS];
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;

	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};
	const gchar *strTitle[] = {"Disable", "Enable"};
	const gchar *strSub[] = {"DDNS", "DDNS SERVER", "NVR NAME", "USER NAME", "PASSWORD", "NVR ADDRESS"};
    gchar outBuf[1024];

	guint width1[3];
	gint tbl_pos_y = 0;

    gint init_ddns = 0;
    gint make_id_row = 0, make_pwd_row = 0;
    gint make_row_cnt;
	gint pos_x,pos_y,size_w,size_h;

    gint i, cnt;
    gint idx;

	char title[64];


    g_wizard_data = (WIZARD_USERDATA_T*)user_data;


    memset(&ddnsdata, 0x00, sizeof(DDNSData));
    memset(&org_ddnsdata, 0x00, sizeof(DDNSData));

    g_memmove(&ddnsdata, &g_wizard_data->networkData.ddns_data, sizeof(DDNSData));
    g_memmove(&org_ddnsdata, &g_wizard_data->networkData.ddns_data, sizeof(DDNSData));

    agr_policy = g_wizard_data->systemData.agr_policy;
    org_agr_policy = g_wizard_data->systemData.agr_policy;

    _init_ddns_info();

	width1[0] = 300;
	width1[1] = 400;
	width1[2] = 322;
	tbl_pos_y = 42;

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
	nfui_nfimage_set_text((NFIMAGE*)obj, "SETTING THE DDNS");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	obj = nftool_normal_button_create_type1("?", 42);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_helpbutton_event_handler);
	help_obj = obj;

	for (i = 0; i < var_get_ddns_cnt(); i++)
	{
		if (!strcmp(g_ddnsNet[i].param.info.server, ddnsdata.server))
			init_ddns = i;

		if (g_ddnsNet[i].cfg.on_id) 	make_id_row = 1;
		if (g_ddnsNet[i].cfg.on_pwd)	make_pwd_row = 1;
		if (g_ddnsNet[i].cfg.on_nvr)	make_nvr_row = 1;
		if (g_ddnsNet[i].cfg.on_mac)	make_mac_row = 1;
	}


	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x += (guint)28;
	pos_y += (guint)60;

	for(i=0; i<2; i++)
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);

		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_onoff_event_cb);
		nfui_nfobject_show(obj);

		if(i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			if(!ddnsdata.enable)
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		} else {
			pos_y += (guint)size_h + 20;
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
			value[ND_DDNS_ROW] = obj;

			if(ddnsdata.enable)
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		}
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x,pos_y);

		/* label */
		if(i == 0){
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		}
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, 300, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + 50, pos_y);
	}

	pos_y += (guint)40;

    make_row_cnt = 1+make_id_row+make_pwd_row+make_nvr_row+make_mac_row;

	fixed2 = nfui_nffixed_new();
    nfui_nfobject_set_size(fixed2, 20+SUBJECT_LABEL_WIDTH*2, IPS_LABEL_HEIGHT*make_row_cnt+30);    
    nfui_nfobject_show(fixed2);
	nfui_nffixed_put((NFFIXED*)fixed1, fixed2, pos_x, pos_y);

	pos_x = (guint)10;
	pos_y = (guint)10;

	//// DDNS SERVER

	obj = nfui_nflabel_new_with_pango_font(strSub[ND_DDNS_SERVER_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

    if (var_get_ddns_cnt() > 1)
    {
    	obj = nfui_combobox_new(NULL, 0, 0);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
        nfui_regi_post_event_callback(obj, post_ddns_server_combo_event_handler);

        for (i = 0; i < var_get_ddns_cnt(); i++)
            nfui_combobox_append_data(NF_COMBOBOX(obj), g_ddnsNet[i].param.info.server);

        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), init_ddns);
    }
    else
    {
        if (!strcmp(ddnsdata.server, "s1.co.kr"))
         	obj = (NFOBJECT*)nfui_nflabel_new_text_box(ddnsdata.serverip, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        else
     	obj = (NFOBJECT*)nfui_nflabel_new_text_box(ddnsdata.server, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    	nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
    	nfui_nfobject_use_focus(obj, 0);
    }
	nfui_nfobject_show(obj);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x+SUBJECT_LABEL_WIDTH, pos_y);
    value[ND_DDNS_SERVER_ROW] = obj;

//// USER NAME

	if (make_id_row)
	{
		pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
		obj = nfui_nflabel_new_with_pango_font(strSub[ND_DDNS_ID_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.info.id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x+SUBJECT_LABEL_WIDTH, pos_y);
		nfui_regi_post_event_callback(obj, post_ddns_field_event_handler);
		value[ND_DDNS_ID_ROW] = obj;
	}


//// PASSWORD

	if (make_pwd_row)
	{

		pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
		obj = nfui_nflabel_new_with_pango_font(strSub[ND_DDNS_PWD_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.info.pwd, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
		nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
		nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x+SUBJECT_LABEL_WIDTH, pos_y);
		nfui_regi_post_event_callback(obj, post_ddns_field_event_handler);
		value[ND_DDNS_PWD_ROW] = obj;
	}


//// DVR NAME

	if (make_nvr_row)
	{
		pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
		obj = nfui_nflabel_new_with_pango_font(strSub[ND_DDNS_NAME_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.info.hostname, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_regi_post_event_callback(obj, post_ddns_field_event_handler);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x+SUBJECT_LABEL_WIDTH, pos_y);
		value[ND_DDNS_NAME_ROW] = obj;
	}

//// DVR ADDRESS

	if (make_mac_row)
	{
		pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
		obj = nfui_nflabel_new_with_pango_font(strSub[ND_DDNS_ADDR_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.addr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_use_focus(obj, 0);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x+SUBJECT_LABEL_WIDTH, pos_y);
		value[ND_DDNS_ADDR_ROW] = obj;
	}

    if (ivsc.dfunc.support_protect)
    {
        pos_x = 4;
        pos_y = fixed2->y + fixed2->height + 30;
    
        memset(outBuf, 0x00, sizeof(outBuf));
        
        nfutil_get_line_feed_string(lookup_string("Network-related services, including DDNS, utilize device information to operate properly."), 
            fixed1->width-10, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
        size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, NORMAL_SPACING);        
    
        obj = nfui_nflabel_new_with_pango_font(outBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nfobject_set_size(obj, fixed1->width-10, size_h+4); 
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        pos_y += (size_h+4);
        
        nfutil_get_line_feed_string(lookup_string("Please agree to providing device information in order to use this service."), 
            fixed1->width-10, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
        size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, NORMAL_SPACING);        
    
        obj = nfui_nflabel_new_with_pango_font(outBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nfobject_set_size(obj, fixed1->width-10, size_h+4); 
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        pos_x += 10;
        pos_y += (size_h+8);
        
        obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_provide_check_event_handler);

        if (g_wizard_data->systemData.agr_policy) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(obj), TRUE);
        }

        if (g_wizard_data->org_systemData.agr_policy) {
            nfui_nfobject_disable(obj);
        }
  
        obj = nfui_nflabel_new_with_pango_font("I agree to provide device information.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nfobject_set_size(obj, fixed1->width-80, 40); 
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+size_w+10, pos_y);
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
	nfui_regi_post_event_callback(fixed2, post_sub_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

    g_next_button = btns[PIB_NEXT];

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

    idx = _get_current_cfg_idx();

    _sync_ddns_server_field(idx);
    _sync_ddns_host_field(idx);
    _sync_ddns_id_field(idx);
    _sync_ddns_pwd_field(idx);
    _sync_ddns_addr_field(idx);

    _update_next_button_status();        

	uxm_reg_imsg_event(fixed2, IRET_SCM_REG_DDNS);
	uxm_reg_imsg_event(fixed2, IRET_SCM_TST_DDNS);

	gtk_main();

	return 0;
}
