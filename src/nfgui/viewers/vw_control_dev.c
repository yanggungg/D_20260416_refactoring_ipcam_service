#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfipeditor.h"

#include "support/color.h"

#include "vw_sys_main.h"
#include "vw_control_dev.h"
#include "scm.h"
#include "ix_mem.h"
#include "nf_keyctrl.h"
#include "vw_vkeyboard.h"
#include "vw_ip_editor_popup.h"


#define TITLE_MARGIN					(20)
#define CD_LABEL_H					(40)

#define CD_LABEL_WIDTH				(300)
#define CD_CELL1_WIDTH				(200)
#define CD_CELL2_WIDTH				(250)
#define CD_CELL3_WIDTH				(200)

#define MAX_STRING_SIZE				(64)

// TIME SETTING.
#define CD_TITLE_X					(8)
#define CD_TITLE_Y					(0)

#define CD_TABLE_ROW					(2)

#define CD_TABLE_ROW_SPACE			(2)
#define CD_TABLE_COL_SPACE			(1)

#define MAX_PROTO_NAME_SIZE			(64)


typedef enum
{
	CD_NONE	= 0,
	CD_SYSTEM_ID,
	CD_PROTOCOL,
	CD_BAUD,
	CD_TABLE_COL
} CD_TABLE_COL_TYPE;



static ControlDevData contdata;
static ControlDevData org_contdata;

static gint proto_cnt;
static gchar **strProto = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *cd_obj[CD_TABLE_COL] = {0, };
static NFOBJECT *rmcID_obj;
static NFOBJECT *g_sd_obj[3]; // secom dual
static NFOBJECT *g_sd_btn[3]; // secom dual

static void _control_dev_init(void)
{
	guint i;

	memset(&contdata, 0x00, sizeof(ControlDevData));
	memset(&org_contdata, 0x00, sizeof(ControlDevData));

	DAL_get_controlDev_data(&contdata);

	proto_cnt = nf_keyctrl_protocol_get_cnt();
	strProto  = imalloc(sizeof(gchar*) * proto_cnt);

	for(i = 0; i < proto_cnt; i++)
	{
		strProto[i] = imalloc(sizeof(gchar) * MAX_PROTO_NAME_SIZE);
		DAL_get_controlDev_protocol(strProto[i], i);

	}

}

static void _control_dev_cleanup(void)
{
	guint i;

	for(i = 0; i < proto_cnt; i++)
	{
		if (strProto[i]) ifree(strProto[i]);
	}

	if (strProto) ifree(strProto);
}

static void _sync_secomdual_onoff(gint onoff)
{
	gint i;

	if (onoff)
	{
		for (i = 1; i < 3; i++)
			nfui_nfobject_enable(g_sd_obj[i]);

		for (i = 0; i < 3; i++)
			nfui_nfobject_enable(g_sd_btn[i]);
	}
	else
	{
		for (i = 1; i < 3; i++)
			nfui_nfobject_disable(g_sd_obj[i]);

		for (i = 0; i < 3; i++)
			nfui_nfobject_disable(g_sd_btn[i]);
	}

//tmp
	for (i = 0; i < 3; i++)
		nfui_nfobject_disable(g_sd_btn[i]);
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(&contdata, &org_contdata, sizeof(ControlDevData));

		if (var_get_supported_keyctrl())
		{
			nfui_spin_button_set_index(cd_obj[CD_SYSTEM_ID], contdata.sysID);
			nfui_combobox_set_index(cd_obj[CD_PROTOCOL], contdata.proto);
			nfui_combobox_set_index(cd_obj[CD_BAUD], contdata.baud);

			nfui_signal_emit(cd_obj[CD_SYSTEM_ID], GDK_EXPOSE, TRUE);
			nfui_signal_emit(cd_obj[CD_PROTOCOL], GDK_EXPOSE, TRUE);
			nfui_signal_emit(cd_obj[CD_BAUD], GDK_EXPOSE, TRUE);
		}

		if ((ivsc.dfunc.buzzer.support_remocon) && (ivsc.dfunc.support_remocon_id))
		{
			nfui_nflabel_set_number((NFLABEL*)rmcID_obj, contdata.rmcID);
			nfui_signal_emit(rmcID_obj, GDK_EXPOSE, TRUE);
		}

		if (var_get_supported_secom_dual())
		{
			nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sd_obj[0], contdata.secomdual_act);
			nfui_nfipeditor_set_ip_array_no_expose((NFIPEDITOR*)g_sd_obj[1], contdata.secomdual_ipaddr);
			nfui_nflabel_set_number((NFLABEL*)g_sd_obj[2], contdata.secomdual_port);

			_sync_secomdual_onoff(contdata.secomdual_act);

			for (i = 0; i < 3; i++)
				nfui_signal_emit(g_sd_obj[i], GDK_EXPOSE, TRUE);

			for (i = 0; i < 3; i++)
				nfui_signal_emit(g_sd_btn[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (var_get_supported_keyctrl())
		{
			contdata.sysID = nfui_spin_button_get_index(cd_obj[CD_SYSTEM_ID]);
			contdata.proto = nfui_combobox_get_cur_index(cd_obj[CD_PROTOCOL]);
			contdata.baud = nfui_combobox_get_cur_index(cd_obj[CD_BAUD]);
		}

		if ((ivsc.dfunc.buzzer.support_remocon) && (ivsc.dfunc.support_remocon_id))
        {
    		contdata.rmcID = nfui_nflabel_get_number((NFLABEL*)rmcID_obj);
        }

		if(memcmp(&org_contdata, &contdata, sizeof(ControlDevData)))
		{
			g_memmove(&org_contdata, &contdata, sizeof(ControlDevData));
			DAL_set_controlDev_data(&contdata);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			scm_put_log(CHANGE_CTRL_DEV, 0, 0);

			VW_SetupSystem_set_changeflag(1);
		}
	}

	return FALSE;
}

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gint numTemp;
		gint numRet;
		guint x, y;
		gint string_size;

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
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 99);

		if (numRet == -1) return FALSE;

		nfui_nflabel_set_number((NFLABEL*)obj, numRet);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_secomdual_act_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, idx;

	if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		idx = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

		_sync_secomdual_onoff(idx);

		for (i = 1; i < 3; i++)
			nfui_signal_emit(g_sd_obj[i], GDK_EXPOSE, TRUE);

		for (i = 0; i < 3; i++)
			nfui_signal_emit(g_sd_btn[i], GDK_EXPOSE, TRUE);

		contdata.secomdual_act = idx;
	}

	return FALSE;
}

static gboolean post_secomdual_ipaddr_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	IP_EDITOR_T ip_editor_data = {0, };
	gint x, y;
	gint i;

	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
		result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);

		if (result == 0)
		{
			nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
			for(i=0; i<4; i++)
				contdata.secomdual_ipaddr[i] = ip_editor_data.field[i];
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

			nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
			result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);

			if (result == 0)
			{
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
				for(i=0; i<4; i++)
					contdata.secomdual_ipaddr[i] = ip_editor_data.field[i];
			}
		}
	}

	return FALSE;
}

static gboolean post_secomdual_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
  	if(evt->type == GDK_2BUTTON_PRESS)
	{
		gint numTemp;
		guint x, y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		numTemp = NumberKey_Open(g_curwnd, nfui_nflabel_get_number((NFLABEL*)obj), x, y, 99999);

		if(numTemp >= 0)
			nfui_nflabel_set_number((NFLABEL*)obj, numTemp);

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

		contdata.secomdual_port = numTemp;
	}

	return FALSE;
}

static gboolean post_request_registration_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}

	return FALSE;
}

static gboolean post_view_registration_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}

	return FALSE;
}


static gboolean post_view_communication_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_ControlDev_tab_out_handler();
		VW_SetupSystem_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_sysid_spin_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}
	return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}

void VW_Init_SysControlDev_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	const gchar *strTitle[] = {"", "SYSTEM ID", "PROTOCOL", "BAUD RATE"};
	const gchar *strTitle2[] = {"CONTROLLER LINKAGE", "CONTROLLER IP ADDRESS", "CONTROLLER PORT"};
	const gchar *strBaud[] = {"2400", "4800", "9600", "19200", "38400", "57600", "115200"};
	const gchar *strOffOn[] = {"OFF", "ON"};

	guint table_w[] = {CD_LABEL_WIDTH, CD_CELL1_WIDTH, CD_CELL2_WIDTH, CD_CELL3_WIDTH};
	gint pos_x, pos_y;
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);
	_control_dev_init();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	pos_x = CD_TITLE_X;
	pos_y = CD_TITLE_Y;

	if (var_get_supported_keyctrl())
	{
// <----- CONTROL DEVICE.
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, "CONTROL DEVICE");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, TITLE_MARGIN);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

		pos_y += 61;

// <----- TABLE
		tbl = (NFOBJECT*)nfui_nftable_new(CD_TABLE_COL, CD_TABLE_ROW, CD_TABLE_COL_SPACE, CD_TABLE_ROW_SPACE, table_w, CD_LABEL_H);
		nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_show(tbl);
		nfui_nffixed_put((NFFIXED*)content_fixed, tbl, pos_x+TITLE_MARGIN, pos_y);

		for (i = 1; i < CD_TABLE_COL; i++)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)tbl, obj, i, 0);
		}

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEVICE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 1);

// <----- SYSTEM ID.
		cd_obj[CD_SYSTEM_ID] = nfui_spinbutton_new_value_with_range(contdata.sysID, 0, 255, 1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)cd_obj[CD_SYSTEM_ID], NFSPINBUTTON_TYPE_1);
		nfui_nfobject_show(cd_obj[CD_SYSTEM_ID]);
		nfui_nftable_attach((NFTABLE*)tbl, cd_obj[CD_SYSTEM_ID], CD_SYSTEM_ID, 1);

// <----- PROTOCOL.
		cd_obj[CD_PROTOCOL] = nfui_combobox_new(strProto, proto_cnt, contdata.proto);
		nfui_combobox_set_skin_type(NF_COMBOBOX(cd_obj[CD_PROTOCOL]), NFCOMBOBOX_TYPE_1);
		nfui_combobox_set_align(NF_COMBOBOX(cd_obj[CD_PROTOCOL]), NFALIGN_CENTER, 0);
		nfui_nfobject_support_multi_lang((NFOBJECT*)cd_obj[CD_PROTOCOL], FALSE);
		nfui_nfobject_show(cd_obj[CD_PROTOCOL]);
		nfui_nftable_attach((NFTABLE*)tbl, cd_obj[CD_PROTOCOL], CD_PROTOCOL, 1);

// <----- BAUD RATE.
		cd_obj[CD_BAUD] = nfui_combobox_new(strBaud, 7, contdata.baud);
		nfui_combobox_set_skin_type(NF_COMBOBOX(cd_obj[CD_BAUD]), NFCOMBOBOX_TYPE_1);
		nfui_combobox_set_align(NF_COMBOBOX(cd_obj[CD_BAUD]), NFALIGN_CENTER, 0);
		nfui_nfobject_show(cd_obj[CD_BAUD]);
		nfui_nftable_attach((NFTABLE*)tbl, cd_obj[CD_BAUD], CD_BAUD, 1);

		pos_y += 150;
	}

// <----- REMOTE CONTROLLER.
	if ((ivsc.dfunc.buzzer.support_remocon) && (ivsc.dfunc.support_remocon_id))
	{
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, "REMOTE CONTROLLER");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, TITLE_MARGIN);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

		pos_y += 60;

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REMOCON ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, CD_LABEL_WIDTH, 40);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+TITLE_MARGIN, pos_y);

		obj= nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
		nfui_nflabel_set_align(obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, CD_CELL1_WIDTH, 40);
		nfui_nfobject_show(obj);
		nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
		nfui_nflabel_set_number((NFLABEL*)obj, contdata.rmcID);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+TITLE_MARGIN+CD_LABEL_WIDTH, pos_y);
		nfui_regi_post_event_callback(obj, post_label_event_handler);
		nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(3));
		rmcID_obj = obj;

		pos_x = CD_TITLE_X;
		pos_y += 110;
	}

// <----- SECOM DUAL.
	if (var_get_supported_secom_dual())
	{
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, "SECOM DUAL");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, TITLE_MARGIN);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

		pos_x += TITLE_MARGIN;
		pos_y += 60;

		table_w[1] = 300;

		tbl = (NFOBJECT*)nfui_nftable_new(2, 3, CD_TABLE_COL_SPACE, CD_TABLE_ROW_SPACE, table_w, CD_LABEL_H);
		nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_show(tbl);
		nfui_nffixed_put((NFFIXED*)content_fixed, tbl, pos_x, pos_y);

		for (i = 0; i < 3; i++)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle2[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
		}

		obj = nfui_spinbutton_new(strOffOn, 2, contdata.secomdual_act);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	    nfui_regi_post_event_callback(obj, post_secomdual_act_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 0);
		g_sd_obj[0] = obj;

		obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(contdata.secomdual_ipaddr[0], contdata.secomdual_ipaddr[1], contdata.secomdual_ipaddr[2], contdata.secomdual_ipaddr[3]);
		nfui_nfipeditor_set_pango_font(obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(134));
		nfui_regi_post_event_callback(obj, post_secomdual_ipaddr_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 1);
		g_sd_obj[1] = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
		nfui_nflabel_set_number((NFLABEL*)obj, contdata.secomdual_port);
		nfui_regi_post_event_callback(obj, post_secomdual_port_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 2);
		g_sd_obj[2] = obj;

		pos_x += CD_LABEL_WIDTH;
		pos_y += 128;

		obj = nftool_normal_button_create_type3("REGISTRATION REQUEST", 300);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_request_registration_event_handler);
		g_sd_btn[0] = obj;

		obj = nftool_normal_button_create_type3("REGISTRATION VIEW", 300);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+302, pos_y);
		nfui_regi_post_event_callback(obj, post_view_registration_event_handler);
		g_sd_btn[1] = obj;
		pos_y += 42;

		obj = nftool_normal_button_create_type3("COMMUNICATION VIEW", 300);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_view_communication_event_handler);
		g_sd_btn[2] = obj;
		pos_y += 42;

		_sync_secomdual_onoff(contdata.secomdual_act);
	}


// <---- CANCEL, APPLY, CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, pre_page_event_handler);

	g_memmove(&org_contdata, &contdata, sizeof(ControlDevData));

	_control_dev_cleanup();
}


gboolean VW_ControlDev_tab_out_handler(void)
{
	mb_type ret;

	if (var_get_supported_keyctrl())
	{
		contdata.sysID = nfui_spin_button_get_index(cd_obj[CD_SYSTEM_ID]);
		contdata.proto = nfui_combobox_get_cur_index(cd_obj[CD_PROTOCOL]);
		contdata.baud = nfui_combobox_get_cur_index(cd_obj[CD_BAUD]);
	}

	if ((ivsc.dfunc.buzzer.support_remocon) && (ivsc.dfunc.support_remocon_id))
	{
		contdata.rmcID = nfui_nflabel_get_number((NFLABEL*)rmcID_obj);
	}

	if(!memcmp(&org_contdata, &contdata, sizeof(ControlDevData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		g_memmove(&org_contdata, &contdata, sizeof(ControlDevData));
		DAL_set_controlDev_data(&contdata);

		scm_put_log(CHANGE_CTRL_DEV, 0, 0);

		VW_SetupSystem_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&contdata, &org_contdata, sizeof(ControlDevData));

		if (var_get_supported_keyctrl())
		{
			nfui_spin_button_set_index_no_expose(cd_obj[CD_SYSTEM_ID], contdata.sysID);
			nfui_combobox_set_index_no_expose(cd_obj[CD_PROTOCOL], contdata.proto);
			nfui_combobox_set_index_no_expose(cd_obj[CD_BAUD], contdata.baud);
		}

		if ((ivsc.dfunc.buzzer.support_remocon) && (ivsc.dfunc.support_remocon_id))
		{
			nfui_nflabel_set_number((NFLABEL*)rmcID_obj, contdata.rmcID);
		}

		if (var_get_supported_secom_dual())
		{
			nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sd_obj[0], contdata.secomdual_act);
			nfui_nfipeditor_set_ip_array_no_expose((NFIPEDITOR*)g_sd_obj[1], contdata.secomdual_ipaddr);
			nfui_nflabel_set_number((NFLABEL*)g_sd_obj[2], contdata.secomdual_port);

			_sync_secomdual_onoff(contdata.secomdual_act);
		}
	}

	return FALSE;
}







