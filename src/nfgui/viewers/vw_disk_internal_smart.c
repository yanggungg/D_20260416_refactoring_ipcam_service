#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftab.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_disk_main.h"
#include "vw_disk_information.h"
#include "vw_disk_smart.h"
#include "vw_disk_smart_info.h"
#include "vw_disk_internal_smart.h"


#define INTERVAL_1HOUR		"1 HOUR"
#define INTERVAL_2HOUR		"2 HOUR"
#define INTERVAL_3HOUR		"3 HOUR"
#define INTERVAL_4HOUR		"4 HOUR"
#define INTERVAL_5HOUR		"5 HOUR"
#define INTERVAL_6HOUR		"6 HOUR"
#define INTERVAL_7HOUR		"7 HOUR"
#define INTERVAL_8HOUR		"8 HOUR"
#define INTERVAL_9HOUR		"9 HOUR"
#define INTERVAL_10HOUR		"10 HOUR"
#define INTERVAL_11HOUR		"11 HOUR"
#define INTERVAL_12HOUR		"12 HOUR"
#define INTERVAL_13HOUR		"13 HOUR"
#define INTERVAL_14HOUR		"14 HOUR"
#define INTERVAL_15HOUR		"15 HOUR"
#define INTERVAL_16HOUR		"16 HOUR"
#define INTERVAL_17HOUR		"17 HOUR"
#define INTERVAL_18HOUR		"18 HOUR"
#define INTERVAL_19HOUR		"19 HOUR"
#define INTERVAL_20HOUR		"20 HOUR"
#define INTERVAL_21HOUR		"21 HOUR"
#define INTERVAL_22HOUR		"22 HOUR"
#define INTERVAL_23HOUR		"23 HOUR"
#define INTERVAL_24HOUR		"24 HOUR"
#define HOUR_TIME_STRING	"%d HOUR"

#define INTERVAL_MAX		(3)

static gchar* use_interval[INTERVAL_MAX] = {INTERVAL_6HOUR,
											INTERVAL_12HOUR, 
											INTERVAL_24HOUR};

static NFWINDOW *g_curwnd = 0;
static DISK_SMARTINFO_T *gSmart_i;

static NFOBJECT *g_tbl;
static NFOBJECT *g_intvObj;

static guint g_intVal = 0;


static gint _translate_int_into_str(gchar *time)
{
	g_return_val_if_fail(time != NULL, 0);

	if(time) {
		if(!g_ascii_strcasecmp(time, INTERVAL_1HOUR ))			return 1;
		else if(!g_ascii_strcasecmp(time, INTERVAL_2HOUR ))		return 2;
		else if(!g_ascii_strcasecmp(time, INTERVAL_3HOUR ))		return 3;
		else if(!g_ascii_strcasecmp(time, INTERVAL_4HOUR ))		return 4;
		else if(!g_ascii_strcasecmp(time, INTERVAL_5HOUR ))		return 5;
		else if(!g_ascii_strcasecmp(time, INTERVAL_6HOUR ))		return 6;
		else if(!g_ascii_strcasecmp(time, INTERVAL_7HOUR ))		return 7;
		else if(!g_ascii_strcasecmp(time, INTERVAL_8HOUR ))		return 8;
		else if(!g_ascii_strcasecmp(time, INTERVAL_9HOUR ))		return 9;
		else if(!g_ascii_strcasecmp(time, INTERVAL_10HOUR ))	return 10;
		else if(!g_ascii_strcasecmp(time, INTERVAL_11HOUR ))	return 11;
		else if(!g_ascii_strcasecmp(time, INTERVAL_12HOUR ))	return 12;
		else if(!g_ascii_strcasecmp(time, INTERVAL_13HOUR ))	return 13;
		else if(!g_ascii_strcasecmp(time, INTERVAL_14HOUR ))	return 14;
		else if(!g_ascii_strcasecmp(time, INTERVAL_15HOUR ))	return 15;
		else if(!g_ascii_strcasecmp(time, INTERVAL_16HOUR ))	return 16;
		else if(!g_ascii_strcasecmp(time, INTERVAL_17HOUR ))	return 17;
		else if(!g_ascii_strcasecmp(time, INTERVAL_18HOUR ))	return 18;
		else if(!g_ascii_strcasecmp(time, INTERVAL_19HOUR ))	return 19;
		else if(!g_ascii_strcasecmp(time, INTERVAL_20HOUR ))	return 20;
		else if(!g_ascii_strcasecmp(time, INTERVAL_21HOUR ))	return 21;
		else if(!g_ascii_strcasecmp(time, INTERVAL_22HOUR ))	return 22;
		else if(!g_ascii_strcasecmp(time, INTERVAL_23HOUR ))	return 23;
		else if(!g_ascii_strcasecmp(time, INTERVAL_24HOUR ))	return 24;
	}

	return -1;
}

static void display_smart_info(gboolean expose)
{
	NFOBJECT *obj;
	gint i;

	gchar buf[32];

	// disk 1~5
	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		if (gSmart_i->disk_unit[i].valid == DISK_INVALID) continue;

		obj = nfui_nftable_get_child((NFTABLE*)g_tbl, i + 1, 1);
		nfui_nflabel_set_text((NFLABEL*)obj, conv_smart_status_to_string(gSmart_i->disk_unit[i].disk_status, buf));

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}
}

static gboolean post_info_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		VW_DiskSmartInfo_Open(g_curwnd, INTERNAL_DISK_GRP);
	}

	return FALSE;
}

static gboolean update_smart_info(gpointer data)
{
	NFOBJECT *mbox = (NFOBJECT*)data;

	update_smart_disk_info();

	NFUTIL_THREADS_ENTER();
	nftool_remove_waitbox((NFOBJECT*)mbox);
	mbox = NULL;

	display_smart_info(TRUE);
	display_disk_inforamtion(FALSE);
	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static gboolean post_check_now_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		static NFOBJECT *wait_mbox = NULL;

		wait_mbox = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
		g_timeout_add(300, update_smart_info, wait_mbox);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gchar buf[10];
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_intVal = DAL_get_disk_smart_interval();
		memset(buf, 0x00, sizeof(buf));
		g_sprintf(buf, HOUR_TIME_STRING, g_intVal);
		nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_intvObj, buf);
		nfui_signal_emit(g_intvObj, GDK_EXPOSE, TRUE);
	}
	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *str;
		gint org_time;
	
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		org_time = DAL_get_disk_smart_interval();
		str = nfui_combobox_get_value(NF_COMBOBOX(g_intvObj));
		g_intVal = _translate_int_into_str(str);

		if (org_time != g_intVal)
		{
			DAL_set_disk_smart_interval(g_intVal);
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		}
	}
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		VW_DiskSmart_tab_out_handler();
		VW_DiskSetup_Destroy(obj);
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

void VW_Init_DiskInternal_Smart_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	gchar *strCol[] = {"DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	gchar *strRow[] = {"S.M.A.R.T STATUS", "S.M.A.R.T CHECK INTERVAL"};
	guint table_w[] = {260, 198, 198, 198, 198, 198, 202};
	gchar buf[10];
	gint i;
	nffont_type font_idx;

	
	g_curwnd = nfui_nfobject_get_top(parent);


	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);


	/* table */
	g_tbl = (NFOBJECT*)nfui_nftable_new(INT_DISP_DISK_COUNT+2, 3, 2, 1, table_w, 40);	
	nfui_nfobject_modify_bg(g_tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(g_tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_tbl, 27, 55);


	// col label
	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)g_tbl, obj,  (i + 1), 0);
	}
	
	// row label
	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRow[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		if(i == 1) nfui_nflabel_set_spacing((NFLABEL*)obj, CONDENSED_SPACING);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)g_tbl, obj,  0, (i + 1));
	}

	// status info
	for(i=0; i<INT_DISP_DISK_COUNT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)g_tbl, obj, i + 1, 1);
	}

	memset(buf, 0x00, sizeof(buf));

	// check now!!
	g_intVal = DAL_get_disk_smart_interval();
	obj = nfui_combobox_new(use_interval, INTERVAL_MAX, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	g_sprintf(buf, HOUR_TIME_STRING, g_intVal);
	nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	//nfui_nfobject_set_size(obj, 202, 40);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)g_tbl, obj,  1, 2);

	g_intvObj = obj;


	// button
	obj = nftool_normal_button_create_subtab_type1("DETAIL INFO", 202);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)g_tbl, obj,  6, 1);
	nfui_regi_post_event_callback(obj, post_info_button_event_handler);

	obj = nftool_normal_button_create_subtab_type1("CHECK NOW", 198);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_check_now_event_handler);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)g_tbl, obj,  2, 2);

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_int_smart_data_changed()
{
	gchar *str;
	guint val = 0;

	val = DAL_get_disk_smart_interval();
	str = nfui_combobox_get_value(NF_COMBOBOX(g_intvObj));
	g_intVal = _translate_int_into_str(str);

	if(val != g_intVal)
		return TRUE;

	return FALSE;
}

void save_int_smart_data()
{
	DAL_set_disk_smart_interval(g_intVal);
}

void restore_int_smart_data()
{
	gchar buf[10];

	memset(buf, 0x00, sizeof(buf));
	g_intVal = DAL_get_disk_smart_interval();
	g_sprintf(buf, HOUR_TIME_STRING, g_intVal);
	nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_intvObj, buf);
	nfui_signal_emit(g_intvObj, GDK_EXPOSE, TRUE);	
}

gboolean display_internal_disk_smart(gboolean expose)
{
	// get smart infor
	gSmart_i = get_smart_disk_info(INTERNAL_DISK_GRP);

	display_smart_info(expose);

	return TRUE;
}
