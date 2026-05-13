
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
#include "objects/nftab.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"

#include "services/scm.h"

#include "ix_mem.h"

#include "vaa.h"
#include "vaa_s1.h"
#include "vw_s1_vca_setup_main.h"
#include "vw_s1_vca_internal.h"
#include "vw_vca_select_channel.h"


#define VIDEO_AREA_W		(960)
#define VIDEO_AREA_H		(540)

#define MENU_LEFT			(8)
#define MENU_TOP			(10)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ruleSelect_fixed = 0;
static NFOBJECT *g_ruleSched_fixed = 0;
static NFOBJECT *g_ruleSetup_fixed = 0;
static NFOBJECT *g_ruleVideo_fixed = 0;

static NFOBJECT *g_ch_combo = 0;
static NFOBJECT *g_use_combo = 0;
static NFOBJECT *g_vpopup_combo = 0;
static NFOBJECT *g_sched_btn = 0;
static NFOBJECT *g_rule_btn = 0;

static gchar *strOnOff[2] = { "OFF", "ON" };

static VCASchedData g_vsd[GUI_CHANNEL_CNT];
static VCASchedData g_org_vsd[GUI_CHANNEL_CNT];

static EA_VCAEvtData g_ved[GUI_CHANNEL_CNT];
static EA_VCAEvtData g_org_ved[GUI_CHANNEL_CNT];

static gint _start_preview(gint ch)
{
	VAAID vaaid;

	vsm_live_preview_start_vca(ch, MENU_V_PAGE_X+MENU_V_INNER_X+MENU_LEFT, 
				        MENU_V_PAGE_Y+MENU_V_INNER_Y+MENU_TOP+59, VIDEO_AREA_W, VIDEO_AREA_H);

	vaaid = vaa_get_vaaid(ch);
	vaa_activate_all_rule(vaaid);
	vaa_activate_meta(vaaid, VAA_META_BBOX);    
    return 0;
}

static gint _stop_preview()
{
	vsm_live_preview_stop();
	return 0;
}

static gint _is_changed_data()
{
	guint db_changed = 0;
	VAAID vaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{
		vaaid = vaa_get_vaaid(i);
		if (vaa_s1_is_db_changed(vaaid)) return 1;
		if (memcmp(&g_org_vsd[i], &g_vsd[i], sizeof(VCASchedData))) return 1;
		if (memcmp(&g_org_ved[i], &g_ved[i], sizeof(EA_VCAEvtData))) return 1;
	}

	return 0;
}

static gint _is_active_vca()
{
	guint vca_enabled = 0;
	VAAID vaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{
		vaaid = vaa_get_vaaid(i);
		if (vaa_s1_is_active_changed(vaaid)) return 1;
	}

	return 0;
}

static gint _load_data(gint expose)
{
	NFOBJECT *top;	
	gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ch_combo);
	VAAID vaaid;
	
	if (_is_changed_data() == 0) return -1;

	vaa_reload();
	g_memmove(g_vsd, g_org_vsd, sizeof(VCASchedData)*var_get_ch_count());
	g_memmove(g_ved, g_org_ved, sizeof(EA_VCAEvtData)*var_get_ch_count());

	nfui_nfobject_hide(g_rule_btn);
	nfui_nfobject_show(g_sched_btn);

	nfui_nfobject_hide(g_ruleSched_fixed);
	nfui_nfobject_show(g_ruleSelect_fixed);

	vaaid = vaa_get_vaaid(ch);
	vaa_s1_raiseup(vaaid);

	if (vaa_s1_is_enabled(vaaid)) {
		nfui_nfobject_enable(g_rule_btn);		
		nfui_nfobject_enable(g_sched_btn);
		_S1_VCA_RuleVideo_set_mevent(1, 1);
		nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_use_combo, 1);
		nfui_nfobject_enable(g_vpopup_combo);
	}
	else {
		nfui_nfobject_disable(g_rule_btn);		
		nfui_nfobject_disable(g_sched_btn);			
		_S1_VCA_RuleVideo_set_mevent(0, 1);
		nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_use_combo, 0);
		nfui_nfobject_disable(g_vpopup_combo);
	}
	nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_vpopup_combo, g_ved[ch].vpop);

	_S1_VCA_RuleSelect_update();
	_S1_VCA_RuleSetup_update();
	_S1_VCA_RuleSetup_hide();

	if (expose) {
		nfui_signal_emit(g_use_combo, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_vpopup_combo, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_rule_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sched_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSelect_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSched_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSetup_fixed, GDK_EXPOSE, TRUE);		
	}

	nfui_make_key_hierarchy(g_curwnd);
	return 0;
}

static gint _save_data()
{
	guint db_changed = 0;
	VAAID vaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{
		vaaid = vaa_get_vaaid(i);
		if (vaa_s1_is_db_changed(vaaid)) {
			vaa_s1_save_db(vaaid);
			db_changed |= (1 << i);
		}

		if (memcmp(&g_org_vsd[i], &g_vsd[i], sizeof(VCASchedData))) {
			DAL_set_vca_schd_data(g_vsd[i], i);
			g_memmove(&g_org_vsd[i], &g_vsd[i], sizeof(VCASchedData));
			db_changed |= (1 << i);
		}
	}

	if (db_changed) {
		DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
		DAL_notify_fire_ipcam_change(NF_IPCAM_CATE_VCA, db_changed);
		evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);
	}
	
	return db_changed;
}

static gint _save_act_data()
{
	guint db_changed = 0;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{
		if (memcmp(&g_org_ved[i], &g_ved[i], sizeof(EA_VCAEvtData))) {
			DAL_set_VCAEvt_data(g_ved[i], i);
			g_memmove(&g_org_ved[i], &g_ved[i], sizeof(EA_VCAEvtData));
			db_changed |= (1 << i);
		}
	}

	if (db_changed) {
		DAL_notify_fire_DB_change(NF_SYSDB_CATE_ACT);
	}
	
	return db_changed;
}

static gboolean _proc_escape(void *data)
{
	NFOBJECT *popup = (NFOBJECT *)data;

	nftool_remove_waitbox(popup);
	gtk_main_quit();
	return FALSE;
}

static gboolean post_vpopup_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ch_combo);
				
		if (idx == 1) {		
			g_ved[ch].vpop = TRUE;
		} 
		else {
			g_ved[ch].vpop = FALSE;
		}
	}

	return FALSE;
}

static gboolean post_use_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		gint idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ch_combo);
		VAAID vaaid = vaa_get_vaaid(ch);
		gint i;
		mb_type ret = -1;

		if (idx == vaa_s1_is_enabled(vaaid)) return FALSE;

		if (scm_get_ipcam_vca_supp(ch) == -1)
		{
			if (idx == 1) {
				nftool_mbox(g_curwnd, "NOTICE", "Current channel does not support vca.", NFTOOL_MB_OK);
				nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_use_combo, 0);
				nfui_signal_emit(g_use_combo, GDK_EXPOSE, TRUE);
				return FALSE;
			}
		}
		
		if (idx == 1) {		
			vaa_s1_enable(vaaid);
			_S1_VCA_RuleVideo_set_mevent(1, 1);
			nfui_nfobject_enable(g_rule_btn);
			nfui_nfobject_enable(g_sched_btn);				
			nfui_nfobject_enable(g_vpopup_combo);
		} 
		else {
			nftool_mbox(g_curwnd, "NOTICE", "You can use higher resolution of the camera if VA is off.", NFTOOL_MB_OK);

			vaa_s1_disable(vaaid);			
			_S1_VCA_RuleVideo_set_mevent(0, 1);
			nfui_nfobject_disable(g_rule_btn);		
			nfui_nfobject_disable(g_sched_btn);
			nfui_nfobject_disable(g_vpopup_combo);
		}

		nfui_nfobject_hide(g_rule_btn);
		nfui_nfobject_show(g_sched_btn);

		nfui_nfobject_hide(g_ruleSched_fixed);
		nfui_nfobject_show(g_ruleSelect_fixed);

		_S1_VCA_RuleSelect_update();
		_S1_VCA_RuleSetup_hide();

		nfui_signal_emit(g_vpopup_combo, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_rule_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sched_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSelect_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSched_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSetup_fixed, GDK_EXPOSE, TRUE);

		nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{	
		gint ch = nfui_combobox_get_cur_index(obj);
		VAAID vaaid = vaa_get_vaaid(ch);

		if ((scm_get_ipcam_vca_supp(ch) == -1) && (vaa_s1_is_enabled(vaaid)))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Current channel does not support vca.", NFTOOL_MB_OK);
			vaa_s1_disable(vaaid);
		}

		nfui_nfobject_hide(g_rule_btn);
		nfui_nfobject_show(g_sched_btn);

		nfui_nfobject_hide(g_ruleSched_fixed);
		nfui_nfobject_show(g_ruleSelect_fixed);

		if (vaa_s1_is_enabled(vaaid)) {
			nfui_nfobject_enable(g_rule_btn);		
			nfui_nfobject_enable(g_sched_btn);
			_S1_VCA_RuleVideo_set_mevent(1, 1);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_use_combo, 1);
			nfui_nfobject_enable(g_vpopup_combo);
		}
		else {
			nfui_nfobject_disable(g_rule_btn);		
			nfui_nfobject_disable(g_sched_btn);			
			_S1_VCA_RuleVideo_set_mevent(0, 1);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_use_combo, 0);
			nfui_nfobject_disable(g_vpopup_combo);	
		}
		nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_vpopup_combo, g_ved[ch].vpop);

		_start_preview(ch);
		_S1_VCA_RuleSelect_set_channel(ch);
		_S1_VCA_RuleSetup_set_channel(ch);
		_S1_VCA_RuleVideo_set_channel(ch);		
		_S1_VCA_RuleSchedule_set_channel(ch, &g_vsd[ch]);

		_S1_VCA_RuleSelect_update();
		_S1_VCA_RuleSetup_update();
		_S1_VCA_RuleSetup_hide();
		
		vaa_s1_raiseup(vaaid);

		nfui_signal_emit(g_use_combo, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_vpopup_combo, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_rule_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sched_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSelect_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSched_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSetup_fixed, GDK_EXPOSE, TRUE);
		
		nfui_make_key_hierarchy(g_curwnd);		
	}

	return FALSE;
}

static gboolean post_sched_setup_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_S1_VCA_RuleSelect_set_ruleSched();
		_S1_VCA_RuleSetup_set_ruleSched();

		nfui_nfobject_hide(g_sched_btn);
		nfui_nfobject_show(g_rule_btn);

		nfui_nfobject_hide(g_ruleSelect_fixed);
		nfui_nfobject_show(g_ruleSched_fixed);

		nfui_signal_emit(g_sched_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_rule_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSelect_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSched_fixed, GDK_EXPOSE, TRUE);		
		nfui_signal_emit(g_ruleSetup_fixed, GDK_EXPOSE, TRUE);

		_S1_VCA_RuleVideo_set_mevent(0, 1);

		nfui_make_key_hierarchy(g_curwnd);	
	}
	
	return FALSE;
}

static gboolean post_rule_setup_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ruleIdx;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_S1_VCA_RuleVideo_set_mevent(1, 1);
		
		nfui_nfobject_hide(g_rule_btn);
		nfui_nfobject_show(g_sched_btn);

		nfui_nfobject_show(g_ruleSelect_fixed);
		nfui_nfobject_hide(g_ruleSched_fixed);
		_S1_VCA_RuleSetup_hide();

		nfui_signal_emit(g_rule_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sched_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSelect_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSched_fixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ruleSetup_fixed, GDK_EXPOSE, TRUE);
		
		nfui_make_key_hierarchy(g_curwnd);	
	}
	
	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint db_changed = 0;
	VAAID vaaid;
	int i;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_load_data(1);
	}
	
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret = -1;
		gint cam_change, act_change;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (_is_active_vca())
		{
			ret = nftool_mbox(g_curwnd, "NOTICE", "The cameras will be reconnected to use vca.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_CANCEL)
			{
				_load_data(1);
				return FALSE;
			}
		}

		cam_change = _save_data();
		if (cam_change) syscam_set_changeflag(1);

		act_change = _save_act_data();
		if (act_change) sysact_set_changeflag(1);

		if (cam_change || act_change) {
			nftool_mbox_auto(g_curwnd, 3, "NOTICE", "Configuration has been saved.");
		}
	}
	
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret;
		guint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_S1_VCA_Main_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ((evt->type == INFY_CAMDB_CHANGE_NOTIFY) || (evt->type == INFY_USRDB_CHANGE_NOTIFY))
	{
		gchar strCh[STRING_SIZE_CAMTITLE];
		gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ch_combo);
		gint i, j;

		nfui_combobox_remove_all((NFCOMBOBOX*)g_ch_combo);

		for (i = 0; i<GUI_CHANNEL_CNT; i++) 
		{
			memset(strCh, 0x00, sizeof(strCh));
			var_get_camtitle(strCh, (guint)i);
			nfui_combobox_append_data((NFCOMBOBOX*)g_ch_combo, strCh);
		}
		
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_ch_combo, ch);
		nfui_signal_emit(g_ch_combo, GDK_EXPOSE, TRUE);
	}
	else if (evt->type == INFY_VAA_SELECT_RULE_ID) 
	{
		gint ruleid = ((CMM_MESSAGE_T *)data)->param;

		g_message("%s, %d, select ruleid:%d", __FUNCTION__, __LINE__, ruleid);

		if (ruleid == -1) _S1_VCA_RuleSetup_hide();
		else _S1_VCA_RuleSetup_set_ruleIdx(ruleid);

		nfui_signal_emit(g_ruleSetup_fixed, GDK_EXPOSE, TRUE);
		nfui_make_key_hierarchy(g_curwnd);	
	}	
	else if (evt->type == GDK_DELETE)  
	{
		uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_VAA_SELECT_RULE_ID);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gint i;
	
		g_ruleSelect_fixed = 0;
		g_ruleSched_fixed = 0;
		g_ruleSetup_fixed = 0;
		g_ruleVideo_fixed = 0;
		g_curwnd = 0;
	}

	return FALSE;
}

void VW_Init_S1_VCA_Main_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixed_temp;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gint pos_x = 0, pos_y = 0;
	gint i, j;
	VAAID vaaid;

	gchar strCh[STRING_SIZE_CAMTITLE];

	g_curwnd = nfui_nfobject_get_top(parent);
	vaaid = vaa_get_vaaid(0);
	vaa_activate_all_rule(vaaid);
	vaa_activate_calb(vaaid);
	vaa_activate_meta(vaaid, VAA_META_BBOX);

	if ((scm_get_ipcam_vca_supp(0) == -1) && (vaa_s1_is_enabled(vaaid)))
	{
		vaa_s1_disable(vaaid);
	}
		
	DAL_get_vca_schd_data_all(g_vsd, GUI_CHANNEL_CNT);
	g_memmove(g_org_vsd, g_vsd, sizeof(VCASchedData)*GUI_CHANNEL_CNT);

	for(i=0; i<GUI_CHANNEL_CNT; i++)
		DAL_get_VCAEvt_data(&g_ved[i], i);

	g_memmove(g_org_ved, g_ved, sizeof(EA_VCAEvtData)*GUI_CHANNEL_CNT);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
	nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);

	pos_x = MENU_LEFT;
	pos_y = MENU_TOP;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 120, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);

	pos_x += 120;

	obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_nfobject_set_size(obj, 240, 40);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_channel_event_handler);
	g_ch_combo = obj;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) 
	{
		memset(strCh, 0x00, sizeof(strCh));	
		var_get_camtitle(strCh, i);
		nfui_combobox_append_data((NFCOMBOBOX*)g_ch_combo, strCh);
	}

	pos_x += 280;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USE VA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);

	pos_x += 220;

	obj = nfui_combobox_new(strOnOff, 2, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_nfobject_set_size(obj, 200, 40);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_use_event_handler);
	g_use_combo = obj;

	if (vaa_s1_is_enabled(vaaid)) nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_use_combo, 1);
	
	pos_x += 280;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_nfobject_hide(obj);

	pos_x += 220;

	obj = nfui_combobox_new(strOnOff, 2, g_ved[0].vpop);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_nfobject_set_size(obj, 200, 40);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_nfobject_hide(obj);
	nfui_regi_post_event_callback(obj, post_vpopup_event_handler);
	g_vpopup_combo = obj;

	if (!vaa_s1_is_enabled(vaaid)) nfui_nfobject_disable(g_vpopup_combo);
	
	pos_x = MENU_LEFT;
	pos_y += 60;

	g_ruleSelect_fixed = _S1_VCA_RuleSelect_Page(content_fixed, pos_x, MENU_V_INNER_H-293, MENU_V_INNER_W-20, 293);
	g_ruleSched_fixed = _S1_VCA_RuleSchedule_Page(content_fixed, &g_vsd[0], pos_x, MENU_V_INNER_H-293, MENU_V_INNER_W-20, 293);
	g_ruleSetup_fixed = _S1_VCA_RuleSetup_Page(content_fixed, MENU_V_INNER_W-10-507, pos_y, 507, VIDEO_AREA_H);
	g_ruleVideo_fixed = _S1_VCA_RuleVideo_Page(content_fixed, pos_x, pos_y, VIDEO_AREA_W, VIDEO_AREA_H);	

	obj = (NFOBJECT*)nftool_normal_button_create_type1("SCHEDULE SETUP", 250);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 0, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_sched_setup_button_event_handler);
	g_sched_btn = obj;

	if (!vaa_s1_is_enabled(vaaid)) nfui_nfobject_disable(obj);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("RULE SETUP", 250);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 0, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_rule_setup_button_event_handler);
	g_rule_btn = obj;
	
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
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);	
	uxm_reg_imsg_event(content_fixed, INFY_VAA_SELECT_RULE_ID);
	uxm_monitor_on_imsg_event(content_fixed, INFY_VAA_SELECT_RULE_ID);

	vaa_s1_raiseup(vaaid);
	vw_dit_display_set_vca_ditid(0);
	_S1_VCA_RuleVideo_set_mevent(1, 1);	
}

gboolean VW_S1_VCA_Main_tab_in_handler()
{
	gint channel = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ch_combo);

	g_message("%s, %d, channel:%d", __FUNCTION__, __LINE__, channel);
	
	_start_preview(channel);
	_S1_VCA_RuleVideo_set_preview_on();
	return FALSE;
}

gboolean VW_S1_VCA_Main_tab_out_handler()
{
	mb_type ret;
	NFOBJECT *save_popup;
	gint cam_change, act_change;

	g_message("%s, %d", __FUNCTION__, __LINE__);

	_S1_VCA_RuleVideo_set_preview_off();
	_stop_preview();

	if (_is_changed_data() == 0) return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
							NFTOOL_MB_OKCANCEL);	

	if (ret == NFTOOL_MB_OK) 
	{
		if (_is_active_vca())
		{
			ret = nftool_mbox(g_curwnd, "NOTICE", "The cameras will be reconnected to use vca.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_CANCEL)
			{
				_load_data(0);
				return FALSE;
			}
		}

		cam_change = _save_data();
		if (cam_change) syscam_set_changeflag(1);

		act_change = _save_act_data();
		if (act_change) sysact_set_changeflag(1);

		if (cam_change || act_change) {
			save_popup = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");
			g_timeout_add(3000, _proc_escape, save_popup);
			gtk_main();
		}
	}
	else if (ret == NFTOOL_MB_CANCEL) 
	{
		_load_data(0);
	}
	return FALSE;
}

gint VW_S1_VCA_start_preview()
{
	_start_preview(0);
	return 0;
}

gint _S1_VCA_Main_get_channel()
{
	gint channel = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ch_combo);
	return channel;
}

NFOBJECT *_get_S1_VCA_ruleSelect_fixed()
{
	return g_ruleSelect_fixed;
}

NFOBJECT *_get_S1_VCA_ruleSched_fixed()
{
	return g_ruleSched_fixed;
}

NFOBJECT *_get_S1_VCA_ruleSetup_fixed()
{
	return g_ruleSetup_fixed;
}

NFOBJECT *_get_S1_VCA_ruleVideo_fixed()
{
	return g_ruleVideo_fixed;
}

