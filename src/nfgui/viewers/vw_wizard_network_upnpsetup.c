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
#include "objects/nfcheckbutton.h"
#include "objects/nfimage.h"
#include "vw_passage_popup.h"

#include "ix_mem.h"
#include "scm.h"
#include "uxm.h"

#include "vw_wizard_init.h"
#include "vw_wizard_network_upnpsetup.h"

#include "vw_vkeyboard.h"
#include "vw_ip_editor_popup.h"

#define HELP_STR "Using automatic port forwarding helps establish a better\nconnection between remote clients and the device."

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
  NIS_RTSP_PORT_ROW = 0,
  NIS_WEB_PORT_ROW,
  NIS_AUTO_PORT_ROW,
  NUM_NIS_ROWS
}nis_row_type;

enum {
	EQUAL_NONE	= 0,
	EQUAL_RTSP,
	EQUAL_WEB,
	EQUAL_ALL,
	NO_DEVICE,
};

enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};


static NFWINDOW *g_curwnd;
static NFOBJECT *value[NUM_NIS_ROWS];
static NFOBJECT *wait_pop = NULL;

static WIZARD_USERDATA_T *g_wizard_data;
static IPSetupData ipdata;
static IPSetupData org_ipdata;

static int port_error = EQUAL_NONE;


static gint _prvLoadDataFromObjects()
{
	ipdata.autoPort = nfui_check_button_get_active((NFCHECKBUTTON*)value[NIS_AUTO_PORT_ROW]);	
	ipdata.rtspport = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);
	ipdata.webPort = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);

	return 0;
}

static int _make_upnp_result_msg(int upnp_ret, char *title, char *msg)
{
	char *str_type[] = {
		"registered",
		"removed",
	};

	char *str_msg[] = {
		"The port has been %s successfully.",
		"Service port is already in use.\nRTSP service port : %d",
		"Service port is already in use. \nWeb service port: %d",
		"Service port is already in use.\nRTSP service port : %d\nWeb service port: %d",
		"Please confirm that your router or NAT support automatic\nport forwarding and that this feature is enabled.",
	};

	int rtsp_port, web_port;
	
	rtsp_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);
	web_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);
	switch (upnp_ret) 
	{
		case RES_UPNP_SUCCESS:
		case RES_UPNP_SUCCESS_FORCE:
		case RES_UPNP_EQUAL_PORT:
			sprintf(title, "%s", "ERROR");
			if(port_error == EQUAL_RTSP)
				sprintf(msg, lookup_string(str_msg[1]), rtsp_port);	
				
			else if(port_error == EQUAL_WEB)
			{
				sprintf(msg, lookup_string(str_msg[2]), web_port);
			}
			
			else if(port_error == EQUAL_ALL)
				sprintf(msg, lookup_string(str_msg[3]), rtsp_port, web_port);
				
			break;
		default:
			sprintf(title, "%s", "ERROR");
			sprintf(msg, "%s",str_msg[4]);
			break;
		
	}
	return 0;
}

static gint _save_port_data()
{
	DAL_set_ipSetup_data(&ipdata);	
	g_memmove(&g_wizard_data->networkData.ipsetup_data, &ipdata, sizeof(IPSetupData));
	
	return 0;
}

static gint _restart_network_service()
{
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
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

static gboolean _check_port_validity(NFOBJECT *obj, int port_num)
{
	int pnum_temp;

	if (port_num >= 65536) {
		nftool_mbox(g_curwnd, "NOTICE", "The port number must be less than 65536.", NFTOOL_MB_OK);
		return FALSE;
	}

	if (obj == value[NIS_RTSP_PORT_ROW])
		pnum_temp = nfui_nflabel_get_number((NFLABEL*)(value[NIS_WEB_PORT_ROW]));
	else
		pnum_temp = nfui_nflabel_get_number((NFLABEL*)(value[NIS_RTSP_PORT_ROW]));

	if (pnum_temp == port_num) {
		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
		return FALSE;
	}


	if (obj == value[NIS_RTSP_PORT_ROW]) {
		if (port_num != 554 && port_num <= 1024) {
			nftool_mbox(g_curwnd, "NOTICE", "Port number must be 554 or\nlarge than 1024.", 
					NFTOOL_MB_OK);
			return FALSE;
		}              
	}
	else {
		if (port_num != 80 && port_num <= 1024) {
			nftool_mbox(g_curwnd, "NOTICE", "Port number must be 80 or\nlarge than 1024.", 
					NFTOOL_MB_OK);
			return FALSE;
		}
	}

	return TRUE;
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

static void current_set_ipSetup_data(void)
{
	NF_NETIF_GET_INFO ret_net_info;

	DAL_get_ipSetup_data(&ipdata);
	scm_get_sys_netinfo(&ret_net_info);

	g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	NF_NOTIFY_INFO *pInfo;
		
	switch (evt->type) {
	case GDK_EXPOSE :
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
	
		nfui_nfobject_gc_unref(gc);
		break;
	default :
		break;
	}

	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, IRET_SCM_REG_RTSP);
		uxm_unreg_imsg_event(obj, IRET_SCM_RMV_RTSP);
		uxm_unreg_imsg_event(obj, IRET_SCM_REG_WEB);
		uxm_unreg_imsg_event(obj, IRET_SCM_RMV_WEB);
		uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
		
		g_curwnd = 0;
		gtk_main_quit();
	}	
	else if(evt->type == IRET_SCM_REG_RTSP)
	{
		int ret = ((CMM_MESSAGE_T *)data)->param;
		char title[32];
		char msg[256];
		mb_type tet;
		
		if (wait_pop) 
		{
			nftool_remove_waitbox(wait_pop);
			wait_pop = NULL;
			port_error = EQUAL_NONE;
		}
		
		if (ret == RES_UPNP_EQUAL_PORT || ret == RES_UPNP_SUCCESS || ret == RES_UPNP_SUCCESS_FORCE)
		{

			if (ret == RES_UPNP_EQUAL_PORT)
				port_error = EQUAL_RTSP;	
			
			wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
			scm_register_web_port(ipdata.webPort, IRET_SCM_REG_WEB);
		}
		else
		{
			_make_upnp_result_msg( ret, title, msg);
			nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK); 
			port_error = NO_DEVICE;
		}			
	}			
	else if(evt->type == IRET_SCM_REG_WEB)
	{	
		int ret = ((CMM_MESSAGE_T *)data)->param;
		char title[32];
		char msg[256];
		
		if (wait_pop) 
		{
			nftool_remove_waitbox(wait_pop);
			wait_pop = NULL;
		}
		
		if(ret == RES_UPNP_EQUAL_PORT && port_error == EQUAL_RTSP)
			port_error = EQUAL_ALL;
		else if (ret == RES_UPNP_EQUAL_PORT && port_error == EQUAL_NONE)
			port_error = EQUAL_WEB;

		if(port_error == EQUAL_NONE && (ret == RES_UPNP_SUCCESS || ret == RES_UPNP_SUCCESS_FORCE))
		{
			_save_port_data();
			_restart_network_service();
		}
		else
		{
			_make_upnp_result_msg( ret, title, msg);
			nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK);  
		}			
	}
	else if(evt->type == IRET_SCM_RMV_RTSP)
	{
		scm_remove_web_port(ipdata.webPort, IRET_SCM_RMV_WEB);
	}	
	else if(evt->type == IRET_SCM_RMV_WEB)
	{
		if (wait_pop) 
		{
			nftool_remove_waitbox(wait_pop);
			wait_pop = NULL;
		}
		
		if(memcmp(&org_ipdata, &ipdata, sizeof(IPSetupData)))
		{	
			_save_port_data();
		}

		_restart_network_service();
	}
	else if (evt->type == IRET_SCM_APPLY_NETINFO)
	{
		if (wait_pop) 
		{
			nftool_remove_waitbox(wait_pop);
			wait_pop = NULL;
		}
		
		_next_step_proc();
	}
	
	return FALSE;
}
static gboolean post_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if (numRet == -1) return FALSE;

		if (_check_port_validity(obj, numRet)) 
		{
			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}
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
	mb_type ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		_prvLoadDataFromObjects();
		if(!_is_connected_cable())
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(ret == NFTOOL_MB_OK)	return FALSE;
		}

		if(ipdata.autoPort)
		{
			wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
			scm_register_rtsp_port(ipdata.rtspport, IRET_SCM_REG_RTSP);

		}
		else
		{
			wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
			scm_remove_rtsp_port(ipdata.rtspport, IRET_SCM_RMV_RTSP);
		}
	}

	return FALSE;
}

static gboolean post_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
		_prvLoadDataFromObjects();
		
		if(!ipdata.autoPort)
			nftool_mbox(g_curwnd, "NOTICE", "If you use the router you need to set manual port forwarding.", NFTOOL_MB_OK);		
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
        gint i;
		
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

gint vw_wizard_network_upnpsetup_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;
	NFOBJECT *help_obj;
	NFOBJECT *subject[NUM_NIS_ROWS];
	NFOBJECT *btns[PIB_BUTTONS];
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;

	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};
	const gchar *strTitle[] = {"RTSP SERVICE PORT", "WEB SERVICE PORT"};

	guint pos_x, pos_y;
	gint size_w, size_h;
	guint i;
	char title[64];


    g_wizard_data = (WIZARD_USERDATA_T*)user_data;


	memset(&ipdata, 0x00, sizeof(IPSetupData));
	memset(&org_ipdata, 0x00, sizeof(IPSetupData));

	g_memmove(&ipdata, &g_wizard_data->networkData.ipsetup_data, sizeof(IPSetupData));		
	g_memmove(&org_ipdata, &g_wizard_data->networkData.ipsetup_data, sizeof(IPSetupData));

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
	nfui_nfimage_set_text((NFIMAGE*)obj, "SETTING THE SERVICE PORT");
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

	pos_x += (guint)28;
	pos_y += (guint)60;
	
	value[NIS_RTSP_PORT_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	value[NIS_WEB_PORT_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

	for(i=0; i<=NIS_WEB_PORT_ROW; i++)
	{
		subject[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nflabel_set_align((NFLABEL*)subject[i], NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
		
		nfui_nfobject_set_size(subject[i], SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);		
		
		nfui_nfobject_modify_bg(subject[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(subject[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(subject[i]);
		nfui_nffixed_put((NFFIXED*)fixed1, subject[i], pos_x, pos_y);

		if(i==NIS_RTSP_PORT_ROW)
		{			
			nfui_nflabel_use_number(value[i], TRUE);
			nfui_nflabel_set_number(value[i], ipdata.rtspport);
		}
		
		else if(i==NIS_WEB_PORT_ROW)
		{
			nfui_nflabel_use_number(value[i], TRUE);
			nfui_nflabel_set_number(value[i], ipdata.webPort);
		}			

		if((i==NIS_WEB_PORT_ROW)||(i==NIS_RTSP_PORT_ROW))
		{
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_INPUT);
			nfui_nfobject_set_size(value[i],SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);	  		
			nfui_nffixed_put((NFFIXED*)fixed1, value[i], pos_x + SUBJECT_LABEL_WIDTH + 4, pos_y);	
		}
		nfui_nfobject_show(value[i]);
		nfui_regi_post_event_callback(value[i], post_port_event_handler);

	pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);

	}

	pos_y += 20;
	pos_x += 20;
 
	value[NIS_AUTO_PORT_ROW] = (NFOBJECT*)nfui_checkbutton_new(ipdata.autoPort);
 
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(value[NIS_AUTO_PORT_ROW]), NFCHECK_TYPE_POPUP_NORMAL);
	nfui_check_get_size(value[NIS_AUTO_PORT_ROW], &size_w, &size_h);
	nfui_nfobject_show(value[NIS_AUTO_PORT_ROW]);
	nfui_nffixed_put((NFFIXED*)fixed1,value[NIS_AUTO_PORT_ROW], pos_x, pos_y);
	nfui_regi_post_event_callback(value[NIS_AUTO_PORT_ROW], post_chk_event_handler);

	obj = nfui_nflabel_new_with_pango_font("USE AUTO PORT FORWARDING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);		
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID, 40);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + size_w + 20, pos_y-10);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

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
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

	uxm_reg_imsg_event(main_wnd, IRET_SCM_REG_RTSP);
	uxm_reg_imsg_event(main_wnd, IRET_SCM_RMV_RTSP);
	uxm_reg_imsg_event(main_wnd, IRET_SCM_REG_WEB);
	uxm_reg_imsg_event(main_wnd, IRET_SCM_RMV_WEB);
	uxm_reg_imsg_event(main_wnd, IRET_SCM_APPLY_NETINFO);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	gtk_main();

	return 0;
}

