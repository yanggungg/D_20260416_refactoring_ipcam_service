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
#include "objects/nfspinbutton.h"

#include "vw_disk_main.h"
#include "vw_disk_information.h"
#include "vw_disk_external_smart.h"
#include "vw_disk_smart_info.h"




static DISK_SMARTINFO_T *gSmart_i;

static NFOBJECT *g_tbl;
static NFWINDOW *g_curwnd = 0;


static void display_smart_info(gboolean expose)
{
	NFOBJECT *obj;
	gint i;
	gchar buf[32];

	// disk 1~5
	for(i=0; i<EXT_DISP_DISK_COUNT; i++) {
		if (gSmart_i->disk_unit[i].valid == DISK_INVALID) continue;

		obj = nfui_nftable_get_child((NFTABLE*)g_tbl, i + 1, 1);
		nfui_nflabel_set_text((NFLABEL*)obj, conv_smart_status_to_string(gSmart_i->disk_unit[i].disk_status, buf));

		if(expose)
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}
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

static gboolean post_info_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		VW_DiskSmartInfo_Open(g_curwnd, EXTERNAL_DISK_GRP);
	}

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

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
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


void VW_Init_DiskExternal_Smart_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	
	gchar *strCol[] = {"DISK 1", "DISK 2", "DISK 3", "DISK 4", "DISK 5"};
	guint table_w[] = {260, 198, 198, 198, 198, 198, 202};
	gchar buf[10];
	gint i;

	g_curwnd = nfui_nfobject_get_top(parent);
	

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);


	/* table */
	g_tbl = (NFOBJECT*)nfui_nftable_new(EXT_DISP_DISK_COUNT+2, 3, 2, 1, table_w, 40);	
	nfui_nfobject_modify_bg(g_tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(g_tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_tbl, 27, 55);


	// col label
	for(i=0; i<EXT_DISP_DISK_COUNT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)g_tbl, obj,  (i + 1), 0);
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("S.M.A.R.T STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nftable_attach((NFTABLE*)g_tbl, obj,  0, 1);

	// status info
	for(i=0; i<EXT_DISP_DISK_COUNT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)g_tbl, obj, i + 1, 1);
	}

	// smart check 
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("S.M.A.R.T CHECK INTERVAL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_spacing((NFLABEL*)obj, CONDENSED_SPACING);
	nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)g_tbl, obj,  0, 2);


	i = DAL_get_disk_smart_interval();
	g_sprintf(buf, "%d HOUR", i);
	
	obj = (NFOBJECT*)nfui_nflabel_new_text_box(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)g_tbl, obj, 1, 2);

	/* button */
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


	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

	obj = nftool_normal_button_create_type1("APPLY", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_ext_smart_data_changed()
{
	return FALSE;
}

void save_ext_smart_data()
{
}

void restore_ext_smart_data()
{
}

gboolean display_external_disk_smart(gboolean expose)
{
	// get smart infor
	gSmart_i = get_smart_disk_info(EXTERNAL_DISK_GRP);

	display_smart_info(expose);

	return TRUE;
}

void reset_check_interval()
{
	NFOBJECT *obj = NULL;
	gchar buf[10];
	gint inv;

	if(g_tbl) {
		inv = DAL_get_disk_smart_interval();
		g_sprintf(buf, "%d HOUR", inv);

		obj = nfui_nftable_get_child((NFTABLE*)g_tbl, 1, 2);
		if(obj)
			nfui_nflabel_set_text((NFLABEL*)obj, buf);
	}
}
