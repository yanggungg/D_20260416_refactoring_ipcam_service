#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfipeditor.h"
#include "objects/nfbutton.h"

#include "tools/nf_ui_function.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_lan2_ipsetup.h"
#include "vw_vkeyboard.h"
#include "vw_ip_editor_popup.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(280)
#define	SUBJECT_LABEL_MARGIN		(0)

#define	IPS_LABEL_LEFT				(SUBJECT_LABEL_LEFT + SUBJECT_LABEL_WIDTH + 2)
#define	IPS_LABEL_WIDTH				(278)
#define	IPS_LABEL_HEIGHT			(40)

#define	IPS_LABEL_ROW_SPACE			(2)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_wait_pop = NULL;

static NFOBJECT *g_dhcpsvr_obj = NULL;
static NFOBJECT *g_ipaddr_obj = NULL;
static NFOBJECT *g_dhcp_pool_start_obj = NULL;
static NFOBJECT *g_dhcp_pool_start_empty = NULL;
static NFOBJECT *g_dhcp_pool_end_obj = NULL;
static NFOBJECT *g_dhcp_pool_end_empty = NULL;

static IPDualLanData g_ipdata;
static IPDualLanData g_org_ipdata;

static gboolean g_install_mode = 0;
static gboolean g_use_dual_lan = 0;


static void current_set_ipSetup_data(void)
{
	NF_NETIF_GET_INFO ret_net_info;

	DAL_get_ipDualLan_data(&g_ipdata);
	
	scm_get_sys_netinfo(&ret_net_info);
	convertIntToIP(g_ipdata.ip, ret_net_info.lan2_ipaddr);
}

static gboolean _is_invalid_setting()
{
	if (g_ipdata.ip[0] == 0 && g_ipdata.ip[1] == 0 && g_ipdata.ip[2] == 0 && g_ipdata.ip[3] == 0) {
		return TRUE;
	}

	if (g_ipdata.dhcpsvr) 
	{
		if (g_ipdata.dhcp_pool_start[0] == 0 && g_ipdata.dhcp_pool_start[1] == 0 && g_ipdata.dhcp_pool_start[2] == 0 && g_ipdata.dhcp_pool_start[3] == 0) {
			return TRUE;
		}
		if (g_ipdata.dhcp_pool_end[0] == 0 && g_ipdata.dhcp_pool_end[1] == 0 && g_ipdata.dhcp_pool_end[2] == 0 && g_ipdata.dhcp_pool_end[3] == 0) {
			return TRUE;
		}
		if (g_ipdata.dhcp_pool_start[3] >= g_ipdata.dhcp_pool_end[3]) {
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	switch (evt->type) 
	{
		case GDK_EXPOSE :
		break;

		case GDK_DELETE:
		break;

		default :
		break;
	}

	return FALSE;
}

static gboolean post_dhcp_svr_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
		if (nfui_check_button_get_active((NFCHECKBUTTON*)obj)) {
			nfui_nfobject_show(g_dhcp_pool_start_obj);
			nfui_nfobject_show(g_dhcp_pool_end_obj);
			nfui_nfobject_hide(g_dhcp_pool_start_empty);
			nfui_nfobject_hide(g_dhcp_pool_end_empty);
		}
		else {
			nfui_nfobject_show(g_dhcp_pool_start_empty);
			nfui_nfobject_show(g_dhcp_pool_end_empty);
			nfui_nfobject_hide(g_dhcp_pool_start_obj);
			nfui_nfobject_hide(g_dhcp_pool_end_obj);
		}

		nfui_signal_emit(g_dhcp_pool_start_empty, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_end_empty, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_start_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_end_obj, GDK_EXPOSE, TRUE);

		nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
	}

	return FALSE;
}

static gboolean post_ipe_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NF_NETIF_GET_INFO ret_net_info;
	gint result;
	IP_EDITOR_T editor_data = {0,};
	IP_EDITOR_T tmp_editor_data = {0,};
	gint x, y;
	guint lan1_ip[4];

	if (evt->type == GDK_2BUTTON_PRESS || evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		if (evt->type == GDK_2BUTTON_PRESS) {
			if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		}
		
		nfui_nfobject_get_offset(obj, &x, &y);

		if ((obj == g_dhcp_pool_start_obj) || (obj == g_dhcp_pool_end_obj)) {
			editor_data.subnet24 = TRUE;
		}

		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, editor_data.field);
		result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &editor_data);		

		if (result == 0) 
		{
			if (obj == g_ipaddr_obj) 
			{
				if (g_install_mode && g_use_dual_lan)
				{
					memset(&ret_net_info, 0, sizeof(ret_net_info));
					scm_get_sys_netinfo(&ret_net_info);
					convertIntToIP(lan1_ip, ret_net_info.ipaddr);
					
					if (lan1_ip[0] == editor_data.field[0] && lan1_ip[1] == editor_data.field[1] && lan1_ip[2] == editor_data.field[2])
					{
						nftool_mbox(g_curwnd, "ERROR", "The IP bands of Lan1 and Lan2 are the same. Please set it up again.\n\nex) Lan1 : 192.168.0.100, Lan2 : 192.168.0.101 (X)\n       Lan1 : 192.168.0.100, Lan2 : 192.168.10.101 (O)", NFTOOL_MB_OK);
						return FALSE;
					}
				}
				
				nfui_nfipeditor_get_ip((NFIPEDITOR*)g_dhcp_pool_start_obj, tmp_editor_data.field);

				if (editor_data.field[0] != tmp_editor_data.field[0] || editor_data.field[1] != tmp_editor_data.field[1] || editor_data.field[2] != tmp_editor_data.field[2])
				{
					tmp_editor_data.field[0] = editor_data.field[0];
					tmp_editor_data.field[1] = editor_data.field[1];
					tmp_editor_data.field[2] = editor_data.field[2];
					if (tmp_editor_data.field[3] == 0) tmp_editor_data.field[3] = 2;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_start_obj, tmp_editor_data.field);
					nfui_signal_emit(g_dhcp_pool_start_obj, GDK_EXPOSE, TRUE);
				}

				nfui_nfipeditor_get_ip((NFIPEDITOR*)g_dhcp_pool_end_obj, tmp_editor_data.field);

				if (editor_data.field[0] != tmp_editor_data.field[0] || editor_data.field[1] != tmp_editor_data.field[1] || editor_data.field[2] != tmp_editor_data.field[2])
				{
					tmp_editor_data.field[0] = editor_data.field[0];
					tmp_editor_data.field[1] = editor_data.field[1];
					tmp_editor_data.field[2] = editor_data.field[2];
					if (tmp_editor_data.field[3] == 0) tmp_editor_data.field[3] = 254;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_end_obj, tmp_editor_data.field);
					nfui_signal_emit(g_dhcp_pool_end_obj, GDK_EXPOSE, TRUE);
				}
 
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, editor_data.field);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
			else if (obj == g_dhcp_pool_start_obj) 
			{
				if (editor_data.field[3] < 2) {
					nftool_mbox(g_curwnd, "NOTICE", "Setting is invalid.\nPlease resetting.", NFTOOL_MB_OK);
					return FALSE;
				}

				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, editor_data.field);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
			else if (obj == g_dhcp_pool_end_obj) 
			{
				if (editor_data.field[3] > 254) {
					nftool_mbox(g_curwnd, "NOTICE", "Setting is invalid.\nPlease resetting.", NFTOOL_MB_OK);
					return FALSE;
				}

				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, editor_data.field);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(&g_ipdata, &g_org_ipdata, sizeof(IPDualLanData));
		
		nfui_check_button_set_active((NFCHECKBUTTON*)g_dhcpsvr_obj, (gboolean)g_ipdata.dhcpsvr);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_ipaddr_obj, g_ipdata.ip);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_start_obj, g_ipdata.dhcp_pool_start);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_end_obj, g_ipdata.dhcp_pool_end);

		if (g_ipdata.dhcpsvr == 1) {
			nfui_nfobject_show(g_dhcp_pool_start_obj);
			nfui_nfobject_show(g_dhcp_pool_end_obj);
			nfui_nfobject_hide(g_dhcp_pool_start_empty);
			nfui_nfobject_hide(g_dhcp_pool_end_empty);
		}
		else {
			nfui_nfobject_show(g_dhcp_pool_start_empty);
			nfui_nfobject_show(g_dhcp_pool_end_empty);
			nfui_nfobject_hide(g_dhcp_pool_start_obj);
			nfui_nfobject_hide(g_dhcp_pool_end_obj);
		}

		nfui_signal_emit(g_dhcpsvr_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ipaddr_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_start_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_end_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_start_empty, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_dhcp_pool_end_empty, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	switch (evt->type) 
	{
		case GDK_BUTTON_RELEASE:
		{
			if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

			g_ipdata.dhcpsvr = nfui_check_button_get_active((NFCHECKBUTTON*)g_dhcpsvr_obj);
			nfui_nfipeditor_get_ip((NFIPEDITOR*)g_ipaddr_obj, g_ipdata.ip);
			nfui_nfipeditor_get_ip((NFIPEDITOR*)g_dhcp_pool_start_obj, g_ipdata.dhcp_pool_start);
			nfui_nfipeditor_get_ip((NFIPEDITOR*)g_dhcp_pool_end_obj, g_ipdata.dhcp_pool_end);
		
			if (memcmp(&g_org_ipdata, &g_ipdata, sizeof(IPDualLanData)))
			{
				if (_is_invalid_setting()) {
					nftool_mbox(g_curwnd, "NOTICE", "Setting is invalid.\nPlease resetting.", NFTOOL_MB_OK);
					return FALSE;
				}

				DAL_set_ipDualLan_data(&g_ipdata);
				g_memmove(&g_org_ipdata, &g_ipdata, sizeof(IPDualLanData));

				g_wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
				DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
				scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
				sysnet_set_changeflag(1);
			}
		}
		break;

		case IRET_SCM_APPLY_NETINFO:
		case IRET_SCM_APPLY_NETINFO2:
    	{
			if (g_wait_pop) {
				nftool_remove_waitbox(g_wait_pop);
				g_wait_pop = NULL;
			}

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			
			if (evt->type == IRET_SCM_APPLY_NETINFO2) gtk_main_quit();
        }   
		break;

		case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
			uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO2);
        }
		break;
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		IpSetup_Lan2_tab_out_handler();
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

void init_NetIPSetup_Lan2_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixedTemp;
	NFOBJECT *obj;

	guint btn_x, btn_y, btn_space;
	guint pos_x, pos_y;
	guint chk_w, chk_h;
	guint i;


	memset(&g_ipdata, 0x00, sizeof(IPDualLanData));
	memset(&g_org_ipdata, 0x00, sizeof(IPDualLanData));

	current_set_ipSetup_data();
	g_memmove(&g_org_ipdata, &g_ipdata, sizeof(IPDualLanData));

	g_install_mode = DAL_get_cam_install_mode();
	g_use_dual_lan = DAL_get_cam_install_use_dual_lan();


	g_curwnd = nfui_nfobject_get_top(parent);


// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);

	pos_y = SUBJECT_LABEL_TOP;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DHCP SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SUBJECT_LABEL_LEFT, pos_y);

	fixedTemp = nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_set_size(fixedTemp, IPS_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(fixedTemp);
	nfui_nffixed_put((NFFIXED*)content_fixed, fixedTemp, IPS_LABEL_LEFT, pos_y);

	obj = nfui_checkbutton_new(g_ipdata.dhcpsvr);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
	nfui_nfobject_show(obj);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (IPS_LABEL_WIDTH-chk_w)/2, (IPS_LABEL_HEIGHT-chk_h)/2);
	nfui_regi_post_event_callback(obj, post_dhcp_svr_chk_event_handler);
	g_dhcpsvr_obj = obj;

	pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE) * 2;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);		
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SUBJECT_LABEL_LEFT, pos_y);

	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(g_ipdata.ip[0], g_ipdata.ip[1], g_ipdata.ip[2], g_ipdata.ip[3]);
	nfui_nfipeditor_set_pango_font(obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_set_size(obj, IPS_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, IPS_LABEL_LEFT, pos_y);
	nfui_regi_post_event_callback(obj, post_ipe_event_handler);
	g_ipaddr_obj = obj;

	pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DHCP POOL START", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);		
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SUBJECT_LABEL_LEFT, pos_y);

	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(g_ipdata.dhcp_pool_start[0], g_ipdata.dhcp_pool_start[1], g_ipdata.dhcp_pool_start[2], g_ipdata.dhcp_pool_start[3]);
	nfui_nfipeditor_set_pango_font(obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_set_size(obj, IPS_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, IPS_LABEL_LEFT, pos_y);
	nfui_regi_post_event_callback(obj, post_ipe_event_handler);
	g_dhcp_pool_start_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, IPS_LABEL_WIDTH, IPS_LABEL_HEIGHT);	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, IPS_LABEL_LEFT, pos_y);
	g_dhcp_pool_start_empty = obj;

	pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DHCP POOL END", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);		
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SUBJECT_LABEL_LEFT, pos_y);

	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(g_ipdata.dhcp_pool_end[0], g_ipdata.dhcp_pool_end[1], g_ipdata.dhcp_pool_end[2], g_ipdata.dhcp_pool_end[3]);
	nfui_nfipeditor_set_pango_font(obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_set_size(obj, IPS_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, IPS_LABEL_LEFT, pos_y);
	nfui_regi_post_event_callback(obj, post_ipe_event_handler);
	g_dhcp_pool_end_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, IPS_LABEL_WIDTH, IPS_LABEL_HEIGHT);	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, IPS_LABEL_LEFT, pos_y);
	g_dhcp_pool_end_empty = obj;

	if (g_ipdata.dhcpsvr == 1) {
		nfui_nfobject_show(g_dhcp_pool_start_obj);
		nfui_nfobject_show(g_dhcp_pool_end_obj);
		nfui_nfobject_hide(g_dhcp_pool_start_empty);
		nfui_nfobject_hide(g_dhcp_pool_end_empty);
	}
	else {
		nfui_nfobject_show(g_dhcp_pool_start_empty);
		nfui_nfobject_show(g_dhcp_pool_end_empty);
		nfui_nfobject_hide(g_dhcp_pool_start_obj);
		nfui_nfobject_hide(g_dhcp_pool_end_obj);
	}

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	uxm_reg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
	uxm_reg_imsg_event(obj, IRET_SCM_APPLY_NETINFO2);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean IpSetup_Lan2_tab_out_handler()
{
	mb_type ret;
	gint is_changed;
	gint i;

	g_ipdata.dhcpsvr = nfui_check_button_get_active((NFCHECKBUTTON*)g_dhcpsvr_obj);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)g_ipaddr_obj, g_ipdata.ip);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)g_dhcp_pool_start_obj, g_ipdata.dhcp_pool_start);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)g_dhcp_pool_end_obj, g_ipdata.dhcp_pool_end);

	if (!memcmp(&g_org_ipdata, &g_ipdata, sizeof(IPDualLanData)))
		return FALSE;

	if (_is_invalid_setting()) {
		nftool_mbox(g_curwnd, "NOTICE", "Setting is invalid.\nPlease resetting.", NFTOOL_MB_OK);
		
		g_memmove(&g_ipdata, &g_org_ipdata, sizeof(IPDualLanData));
		
		nfui_check_button_set_active((NFCHECKBUTTON*)g_dhcpsvr_obj, (gboolean)g_ipdata.dhcpsvr);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_ipaddr_obj, g_ipdata.ip);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_start_obj, g_ipdata.dhcp_pool_start);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_end_obj, g_ipdata.dhcp_pool_end);

		if (g_ipdata.dhcpsvr == 1) {
			nfui_nfobject_show(g_dhcp_pool_start_obj);
			nfui_nfobject_show(g_dhcp_pool_end_obj);
			nfui_nfobject_hide(g_dhcp_pool_start_empty);
			nfui_nfobject_hide(g_dhcp_pool_end_empty);
		}
		else {
			nfui_nfobject_show(g_dhcp_pool_start_empty);
			nfui_nfobject_show(g_dhcp_pool_end_empty);
			nfui_nfobject_hide(g_dhcp_pool_start_obj);
			nfui_nfobject_hide(g_dhcp_pool_end_obj);
		}
		return FALSE;
	}

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
		DAL_set_ipDualLan_data(&g_ipdata);
		g_memmove(&g_org_ipdata, &g_ipdata, sizeof(IPDualLanData));

		g_wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
		scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO2);
		sysnet_set_changeflag(1);

		gtk_main();
	}
	else if (ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&g_ipdata, &g_org_ipdata, sizeof(IPDualLanData));
		
		nfui_check_button_set_active((NFCHECKBUTTON*)g_dhcpsvr_obj, (gboolean)g_ipdata.dhcpsvr);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_ipaddr_obj, g_ipdata.ip);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_start_obj, g_ipdata.dhcp_pool_start);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)g_dhcp_pool_end_obj, g_ipdata.dhcp_pool_end);

		if (g_ipdata.dhcpsvr == 1) {
			nfui_nfobject_show(g_dhcp_pool_start_obj);
			nfui_nfobject_show(g_dhcp_pool_end_obj);
			nfui_nfobject_hide(g_dhcp_pool_start_empty);
			nfui_nfobject_hide(g_dhcp_pool_end_empty);
		}
		else {
			nfui_nfobject_show(g_dhcp_pool_start_empty);
			nfui_nfobject_show(g_dhcp_pool_end_empty);
			nfui_nfobject_hide(g_dhcp_pool_start_obj);
			nfui_nfobject_hide(g_dhcp_pool_end_obj);
		}
	}

	return FALSE;

}
