#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfipeditor.h"
#include "viewers/objects/nfbutton.h"
#include "vw_ip_editor_popup.h"

#include "vw_change_ipsetup_popup.h"

#define IPSET_PP_SIZE_WID		(538)
#define IPSET_PP_POS_X			((DISPLAY_ACTIVE_WIDTH- IPSET_PP_SIZE_WID)/2)

#define	IPSET_PP_LABEL_WIDTH	(220)
#define	IPSET_PP_LABEL_HEIGHT	(40)
#define	IPSET_PP_VALUE_WIDTH	(278)

static mb_type g_retVal = 0;
static NFOBJECT *g_curwnd;
static IP_SET_T *g_ipset;
static IPCHANGE_MASK_E g_mask;

enum
{
	IPSET_DHCP_ROW = 0,
	IPSET_IP_ROW,
	IPSET_GATEWAY_ROW,
	IPSET_SUBNET_ROW,
	IPSET_DNS1_ROW,
	IPSET_DNS2_ROW,
	IPSET_RTSP_PORT_ROW,
	IPSET_WEB_PORT_ROW,

	IPSET_MAX_ROW
};
static NFOBJECT *value[IPSET_MAX_ROW];

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
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

static gboolean post_dhcp_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		guint dhcp;
		gint i;

		dhcp = nfui_check_button_get_active((NFCHECKBUTTON*)value[IPSET_DHCP_ROW]);

		for (i = IPSET_IP_ROW; i <= IPSET_DNS2_ROW; i++) 
		{
			if (dhcp) {
				if (value[i]) {
					nfui_nfobject_disable(value[i]);
					nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);
				}
			}
			else {
				if (value[i]) {
					nfui_nfobject_enable(value[i]);
					nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);
				}
			}
		}

	}

	return FALSE;
}

static gboolean post_ipe_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	IP_EDITOR_T ip_editor_data = {0, };
	gint x, y;
	NFOBJECT *top;

	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x+obj->width+4;
		y += top->y;
		
		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
		result = vw_ip_editor_popup_open(g_curwnd, x, y, &ip_editor_data);		

		if (result == 0) {
			nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);

			if(obj == value[IPSET_IP_ROW]) {
				if (value[IPSET_GATEWAY_ROW]) {
					ip_editor_data.field[3] = 1;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[IPSET_GATEWAY_ROW], ip_editor_data.field);
				}

				if (value[IPSET_SUBNET_ROW]) {
					ip_editor_data.field[0] = 255;
					ip_editor_data.field[1] = 255;
					ip_editor_data.field[2] = 255;
					ip_editor_data.field[3] = 0;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[IPSET_SUBNET_ROW], ip_editor_data.field);
				}
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += top->x+obj->width+4;
			y += top->y;

			nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
			result = vw_ip_editor_popup_open(g_curwnd, x, y, &ip_editor_data);		

			if (result == 0)
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);

			if(obj == value[IPSET_IP_ROW]) {
				if (value[IPSET_GATEWAY_ROW]) {
					ip_editor_data.field[3] = 1;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[IPSET_GATEWAY_ROW], ip_editor_data.field);
				}

				if (value[IPSET_SUBNET_ROW]) {
					ip_editor_data.field[0] = 255;
					ip_editor_data.field[1] = 255;
					ip_editor_data.field[2] = 255;
					ip_editor_data.field[3] = 0;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[IPSET_SUBNET_ROW], ip_editor_data.field);
				}
			}
		}
	}

	return FALSE;
}

static gboolean _check_port_validity(NFOBJECT *obj, int port_num)
{
	int pnum_temp;

	if (port_num >= 65536) {
		nftool_mbox(g_curwnd, "NOTICE", "The port number must be less than 65536.", NFTOOL_MB_OK);
		return FALSE;
	}

	if (obj == value[IPSET_RTSP_PORT_ROW])
	{
		if (!value[IPSET_WEB_PORT_ROW]) return TRUE;	
		pnum_temp = nfui_nflabel_get_number((NFLABEL*)(value[IPSET_WEB_PORT_ROW]));
	}
	else
	{
		if (!value[IPSET_RTSP_PORT_ROW]) return TRUE;	
		pnum_temp = nfui_nflabel_get_number((NFLABEL*)(value[IPSET_RTSP_PORT_ROW]));
	}

	if (pnum_temp == port_num) {
		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
		return FALSE;
	}


	if (obj == value[IPSET_RTSP_PORT_ROW]) {
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

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);

		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}


static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if(value[IPSET_DHCP_ROW]) g_ipset->dhcp = nfui_check_button_get_active((NFCHECKBUTTON*)value[IPSET_DHCP_ROW]);
		if(value[IPSET_IP_ROW]) nfui_nfipeditor_get_ip((NFIPEDITOR*)value[IPSET_IP_ROW], g_ipset->ip);
		if(value[IPSET_GATEWAY_ROW]) nfui_nfipeditor_get_ip((NFIPEDITOR*)value[IPSET_GATEWAY_ROW], g_ipset->gateway);
		if(value[IPSET_SUBNET_ROW]) nfui_nfipeditor_get_ip((NFIPEDITOR*)value[IPSET_SUBNET_ROW], g_ipset->subnet);
		if(value[IPSET_DNS1_ROW]) nfui_nfipeditor_get_ip((NFIPEDITOR*)value[IPSET_DNS1_ROW], g_ipset->dns1);
		if(value[IPSET_DNS2_ROW]) nfui_nfipeditor_get_ip((NFIPEDITOR*)value[IPSET_DNS2_ROW], g_ipset->dns2);
		if(value[IPSET_RTSP_PORT_ROW]) g_ipset->rtspPort = nfui_nflabel_get_number((NFLABEL*)value[IPSET_RTSP_PORT_ROW]);
		if(value[IPSET_WEB_PORT_ROW]) g_ipset->webPort = nfui_nflabel_get_number((NFLABEL*)value[IPSET_WEB_PORT_ROW]);

		g_retVal = 1;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}


mb_type VW_Open_IPSet_Popup(NFWINDOW *parent, gchar *title, IP_SET_T *ip_set, IPCHANGE_MASK_E change_msk)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *chk_fixed;
	NFOBJECT *obj;

	g_retVal = 0;
	gint i, cnt;
	gint win_height, win_posy;
	guint pos_x, pos_y;
	guint chk_w, chk_h;

	const gchar *strTitle[] = {"DHCP", "IP ADDRESS", "GATEWAY", "SUBNET MASK",
		                       "1ST DNS SERVER", "2ND DNS SERVER",
		                       "RTSP SERVICE PORT", "WEB SERVICE PORT" };
	const gchar *strButton[] = {"OK", "CANCEL"};

	for (i = 0; i < IPSET_MAX_ROW; i++)
	{
		value[i] = 0;
	}

	g_ipset = ip_set;
	g_mask = change_msk;

	cnt = 0;
	for( i = 0; i < IPSET_MSK_MAX; i++ )
	{
		if(g_mask & (1 << i))
			cnt++;
	}

	win_height = (cnt * 43) + 120; 
	win_posy = ((DISPLAY_ACTIVE_HEIGHT - win_height)/2 - 10);

	main_wnd = nftool_create_popup_window(parent, IPSET_PP_POS_X, win_posy, IPSET_PP_SIZE_WID, 
	                                       win_height, title, TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	g_curwnd = main_wnd;

	pos_y = 56;
	for(i = 0; i < IPSET_MAX_ROW; i++)
	{
		pos_x = 20;
		if( i == IPSET_DHCP_ROW)
		{
			if(!(g_mask & (1 << MSK_DHCP)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;

			chk_fixed = nfui_nffixed_new();
			nfui_nfobject_set_size(chk_fixed, IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(292));
			nfui_nfobject_show(chk_fixed);
			nfui_nffixed_put((NFFIXED*)main_fixed, chk_fixed, pos_x, pos_y);

			value[i] = (NFOBJECT*)nfui_checkbutton_new((gboolean)ip_set->dhcp);
			nfui_check_button_set_skin_type(NF_CHECKBUTTON(value[i]), NFCHECK_TYPE_NORMAL);
			nfui_check_get_size(value[i], &chk_w, &chk_h);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)chk_fixed, value[i], (IPSET_PP_VALUE_WIDTH-chk_w)/2, (40-chk_h)/2);
			nfui_regi_post_event_callback(value[i], post_dhcp_chk_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;
		}
		else if( i == IPSET_IP_ROW)
		{
			if(!(g_mask & (1 << MSK_IPADDR)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;
			value[i] = (NFOBJECT*)nfui_nfipeditor_new_with_ip( ip_set->ip[0],
			                                                   ip_set->ip[1],
															   ip_set->ip[2],
															   ip_set->ip[3]);
			nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_ipe_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;
		}
		else if( i == IPSET_GATEWAY_ROW)
		{
			if(!(g_mask & (1 << MSK_GATEWAY)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;
			value[i] = (NFOBJECT*)nfui_nfipeditor_new_with_ip( ip_set->gateway[0], 
			                                                   ip_set->gateway[1],
															   ip_set->gateway[2],
															   ip_set->gateway[3]);
			nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_ipe_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;

		}
		else if( i == IPSET_SUBNET_ROW)
		{
			if(!(g_mask & (1 << MSK_SUBNET)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;
			value[i] = (NFOBJECT*)nfui_nfipeditor_new_with_ip( ip_set->subnet[0], 
			                                                   ip_set->subnet[1],
															   ip_set->subnet[2],
															   ip_set->subnet[3]);
			nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_ipe_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;

		}
		else if( i == IPSET_DNS1_ROW)
		{
			if(!(g_mask & (1 << MSK_DNS1)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;
			value[i] = (NFOBJECT*)nfui_nfipeditor_new_with_ip( ip_set->dns1[0], 
			                                                   ip_set->dns1[1],
															   ip_set->dns1[2],
															   ip_set->dns1[3]);
			nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_ipe_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;

		}
		else if( i == IPSET_DNS2_ROW)
		{
			if(!(g_mask & (1 << MSK_DNS2)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;
			value[i] = (NFOBJECT*)nfui_nfipeditor_new_with_ip( ip_set->dns2[0], 
			                                                   ip_set->dns2[1],
															   ip_set->dns2[2],
															   ip_set->dns2[3]);
			nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_ipe_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;

		}
		else if( i == IPSET_RTSP_PORT_ROW)
		{
			if(!(g_mask & (1 << MSK_RTSP_PORT)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;

			value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_use_number(value[i], TRUE);
			nfui_nflabel_set_number(value[i], ip_set->rtspPort);
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_INPUT);
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_port_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;

		}
		else // if( i == IPSET_WEB_PORT_ROW)
		{
			if(!(g_mask & (1 << MSK_WEB_PORT)))
				continue;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, IPSET_PP_LABEL_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += IPSET_PP_LABEL_WIDTH;

			value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_use_number(value[i], TRUE);
			nfui_nflabel_set_number(value[i], ip_set->webPort);
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_INPUT);
			nfui_nfobject_set_size(value[i], IPSET_PP_VALUE_WIDTH, IPSET_PP_LABEL_HEIGHT);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_port_event_handler);

			pos_y += IPSET_PP_LABEL_HEIGHT + 3;

		}

	}

	for (i = IPSET_IP_ROW; i <= IPSET_DNS2_ROW; i++) 
	{
		if (ip_set->dhcp) {
			if (value[i]) {
				nfui_nfobject_disable(value[i]);
				nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);
			}
		}
	}

	pos_y = win_height - 55;

	obj = nftool_normal_button_create_type1("CANCEL", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 278, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	
	obj = nftool_normal_button_create_type1("APPLY", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 100, pos_y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	gtk_main();

	return g_retVal;
}

