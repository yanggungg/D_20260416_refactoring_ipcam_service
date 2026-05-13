#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_sys_user_main.h"
#include "vw_sys_user_auth.h"
#include "scm.h"
#include "support/event_loop.h"
#include "nf_sysman.h"


#define __ALARM_PANIC__

#define	NUM_UA_ROWS				(10)

#define	UA_TABLE_X				(28)
#define	UA_TABLE_Y				(42)
#define	UA_TABLE_COLUMNS		(3)
#define	UA_TABLE_ROWS			(NUM_UA_ROWS+1)
#define	UA_TABLE_COL_SPACE		(guint)(DISPLAY_IS_D1 ? 4:2)
#define	UA_TABLE_ROW_SPACE		(guint)(DISPLAY_IS_D1 ? 2:1)

#define	UA_LABEL_HEIGHT			(guint)(DISPLAY_IS_D1 ? 16:40)
#define	UA_LABEL_MARGIN			(guint)(DISPLAY_IS_D1 ? 10:20)

enum {
	UAB_CANCEL = 0,
	UAB_APPLY,
	UAB_CLOSE,
	UAB_BUTTONS
};

static UserAuthData authdata[MAX_GROUP_COUNT];
static UserAuthData org_authdata[MAX_GROUP_COUNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *manager_val[UA_TABLE_ROWS];
static NFOBJECT *user_val[UA_TABLE_ROWS];

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
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

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	       }

		g_memmove(authdata, org_authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT);
				
		nfui_check_button_set_active(manager_val[1], authdata[1].search);
		nfui_check_button_set_active(manager_val[2], authdata[1].archive);
		nfui_check_button_set_active(manager_val[3], authdata[1].sys_setup);
		nfui_check_button_set_active(manager_val[4], authdata[1].rec_setup);
		nfui_check_button_set_active(manager_val[5], authdata[1].event);
		nfui_check_button_set_active(manager_val[6], authdata[1].audio);
		nfui_check_button_set_active(manager_val[7], authdata[1].microphone);
		nfui_check_button_set_active(manager_val[8], authdata[1].remote);
		nfui_check_button_set_active(manager_val[9], authdata[1].ptz);
		nfui_check_button_set_active(manager_val[10], authdata[1].shutdown);
		
		nfui_check_button_set_active(user_val[1], authdata[2].search);
		nfui_check_button_set_active(user_val[2], authdata[2].archive);
		nfui_check_button_set_active(user_val[3], authdata[2].sys_setup);
		nfui_check_button_set_active(user_val[4], authdata[2].rec_setup);
		nfui_check_button_set_active(user_val[5], authdata[2].event);
		nfui_check_button_set_active(user_val[6], authdata[2].audio);
		nfui_check_button_set_active(user_val[7], authdata[2].microphone);
		nfui_check_button_set_active(user_val[8], authdata[2].remote);
		nfui_check_button_set_active(user_val[9], authdata[2].ptz);
		nfui_check_button_set_active(user_val[10], authdata[2].shutdown);

		for(i=1; i<UA_TABLE_ROWS; i++)
		{
			nfui_signal_emit(manager_val[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(user_val[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	    }
		
		authdata[1].search		= nfui_check_button_get_active(manager_val[1]);
		authdata[1].archive		= nfui_check_button_get_active(manager_val[2]);
		authdata[1].sys_setup 	= nfui_check_button_get_active(manager_val[3]);
		authdata[1].rec_setup	= nfui_check_button_get_active(manager_val[4]);
		authdata[1].event		= nfui_check_button_get_active(manager_val[5]);
		authdata[1].audio		= nfui_check_button_get_active(manager_val[6]);
		authdata[1].microphone	= nfui_check_button_get_active(manager_val[7]);
		authdata[1].remote		= nfui_check_button_get_active(manager_val[8]);
		authdata[1].ptz			= nfui_check_button_get_active(manager_val[9]);
		authdata[1].shutdown	= nfui_check_button_get_active(manager_val[10]);

		authdata[2].search		= nfui_check_button_get_active(user_val[1]);
		authdata[2].archive		= nfui_check_button_get_active(user_val[2]);
		authdata[2].sys_setup 	= nfui_check_button_get_active(user_val[3]);
		authdata[2].rec_setup	= nfui_check_button_get_active(user_val[4]);
		authdata[2].event		= nfui_check_button_get_active(user_val[5]);
		authdata[2].audio		= nfui_check_button_get_active(user_val[6]);
		authdata[2].microphone	= nfui_check_button_get_active(user_val[7]);
		authdata[2].remote		= nfui_check_button_get_active(user_val[8]);
		authdata[2].ptz			= nfui_check_button_get_active(user_val[9]);
		authdata[2].shutdown	= nfui_check_button_get_active(user_val[10]);

		if(memcmp(org_authdata, authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT))
		{
			scm_put_log(CHANGE_USER_GRP, 0, 0);

			g_memmove(org_authdata, authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT);
			DAL_set_userAuth_data_all(authdata, 3);	// 3 : number of groups

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			sysuser_set_changeflag(1);
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
	
		UserAuthority_tab_out_handler();
		SystemSetupUser_Destroy(obj);
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



void init_UserAuth_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *subject_object[NUM_UA_ROWS];
	NFOBJECT *ntb;
	NFOBJECT *ua_btns[UAB_BUTTONS];
	NFOBJECT *fixed_temp;

	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};

	const gchar *strTitle[] = {
					"SEARCH",
					"ARCHIVING",
					"SYSTEM SETUP",
					"RECORD SETUP",
					"EVENT ACTION CONTROL",
					"AUDIO LISTENING",
					"MICROPHONE",
					"REMOTE LOG IN",
					"PTZ CONTROL",
					"SHUTDOWN",
	};

	guint width[3];
	guint chk_w, chk_h;
	guint btn_x, btn_y, btn_space;
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);


	width[0] = 340;
	width[1] = 261;
	width[2] = 261;

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	ntb = (NFOBJECT*)nfui_nftable_new(UA_TABLE_COLUMNS, UA_TABLE_ROWS, UA_TABLE_COL_SPACE, UA_TABLE_ROW_SPACE, width, UA_LABEL_HEIGHT + 5);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)content_fixed, ntb, UA_TABLE_X, UA_TABLE_Y);	

	memset(authdata, 0x00, sizeof(UserAuthData)*MAX_GROUP_COUNT);
	memset(org_authdata, 0x00, sizeof(UserAuthData)*MAX_GROUP_COUNT);

	for(i=0; i<MAX_GROUP_COUNT; i++)
	{
		DAL_get_userAuth_data(&(authdata[i]), i);
	}

	manager_val[0] = nfui_nflabel_new_with_pango_font(authdata[1].grp_name, 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));

	nfui_nfobject_support_multi_lang(manager_val[0], FALSE);
	nfui_nfobject_modify_bg(manager_val[0], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));

	
	manager_val[1] = nfui_checkbutton_new(authdata[1].search);
	manager_val[2] = nfui_checkbutton_new(authdata[1].archive);
	manager_val[3] = nfui_checkbutton_new(authdata[1].sys_setup);
	manager_val[4] = nfui_checkbutton_new(authdata[1].rec_setup);
	manager_val[5] = nfui_checkbutton_new(authdata[1].event);
	manager_val[6] = nfui_checkbutton_new(authdata[1].audio);
	manager_val[7] = nfui_checkbutton_new(authdata[1].microphone);
	manager_val[8] = nfui_checkbutton_new(authdata[1].remote);
	manager_val[9] = nfui_checkbutton_new(authdata[1].ptz);
	manager_val[10] = nfui_checkbutton_new(authdata[1].shutdown);

	if (!DAL_get_support_audio()) nfui_nfobject_disable(manager_val[6]);
	if (!DAL_get_support_audio() || (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B)) nfui_nfobject_disable(manager_val[7]);

	user_val[0] = nfui_nflabel_new_with_pango_font(authdata[2].grp_name, 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nfobject_support_multi_lang(user_val[0], FALSE);
	nfui_nfobject_modify_bg(user_val[0], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	
	
	user_val[1] = nfui_checkbutton_new(authdata[2].search);
	user_val[2] = nfui_checkbutton_new(authdata[2].archive);
	user_val[3] = nfui_checkbutton_new(authdata[2].sys_setup);
	user_val[4] = nfui_checkbutton_new(authdata[2].rec_setup);
	user_val[5] = nfui_checkbutton_new(authdata[2].event);
	user_val[6] = nfui_checkbutton_new(authdata[2].audio);
	user_val[7] = nfui_checkbutton_new(authdata[2].microphone);
	user_val[8] = nfui_checkbutton_new(authdata[2].remote);
	user_val[9] = nfui_checkbutton_new(authdata[2].ptz);
	user_val[10] = nfui_checkbutton_new(authdata[2].shutdown);
	
	if (!DAL_get_support_audio()) nfui_nfobject_disable(user_val[6]);
	if (!DAL_get_support_audio() || (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B)) nfui_nfobject_disable(user_val[7]);

	nfui_nfobject_use_focus(manager_val[0], NFOBJECT_FOCUS_OFF);
	nfui_nfobject_use_focus(user_val[0], NFOBJECT_FOCUS_OFF);

	for(i=0; i<(UA_TABLE_ROWS); i++)
	{
		if(i == 0)
		{
			nfui_nftable_attach((NFTABLE*)ntb, 	manager_val[i], 1, i);
			nfui_nftable_attach((NFTABLE*)ntb, 	user_val[i], 2, i);
		}
		else
		{
			subject_object[i-1] = nfui_nflabel_new_with_pango_font(strTitle[i-1], 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			
			nfui_nflabel_set_align((NFLABEL*)subject_object[i-1], NFALIGN_LEFT, UA_LABEL_MARGIN);
			nfui_nfobject_modify_bg(subject_object[i-1], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(subject_object[i-1], NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(subject_object[i-1]);
			nfui_nftable_attach((NFTABLE*)ntb, subject_object[i-1], 0, i);

			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(fixed_temp, width[1], UA_LABEL_HEIGHT);
			nfui_nfobject_show(fixed_temp);
			
			nfui_check_button_set_skin_type(NF_CHECKBUTTON(manager_val[i]), NFCHECK_TYPE_NORMAL);
			nfui_check_get_size(manager_val[i], &chk_w, &chk_h);
			nfui_nffixed_put((NFFIXED*)fixed_temp, manager_val[i], (width[1]-chk_w)/2, (UA_LABEL_HEIGHT-chk_h)/2);
			nfui_nftable_attach((NFTABLE*)ntb, 	fixed_temp, 1, i);


			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(fixed_temp, width[2], UA_LABEL_HEIGHT);
			nfui_nfobject_show(fixed_temp);

			nfui_check_button_set_skin_type(NF_CHECKBUTTON(user_val[i]), NFCHECK_TYPE_NORMAL);
			nfui_check_get_size(user_val[i], &chk_w, &chk_h);
			nfui_nffixed_put((NFFIXED*)fixed_temp, user_val[i], (width[2]-chk_w)/2, (UA_LABEL_HEIGHT-chk_h)/2);
			nfui_nftable_attach((NFTABLE*)ntb, 	fixed_temp, 2, i);
		}

		nfui_nfobject_show(manager_val[i]);
		nfui_nfobject_show(user_val[i]);
	}

	ua_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(ua_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(ua_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, ua_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	ua_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(ua_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(ua_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, ua_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	ua_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(ua_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(ua_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, ua_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(ua_btns[UAB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(ua_btns[UAB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(ua_btns[UAB_CLOSE], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(org_authdata, authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT);
}


gboolean UserAuthority_tab_out_handler()
{
	mb_type ret;
	guint i;

	authdata[1].search		= nfui_check_button_get_active(manager_val[1]);
	authdata[1].archive		= nfui_check_button_get_active(manager_val[2]);
	authdata[1].sys_setup 	= nfui_check_button_get_active(manager_val[3]);
	authdata[1].rec_setup	= nfui_check_button_get_active(manager_val[4]);
	authdata[1].event		= nfui_check_button_get_active(manager_val[5]);
	authdata[1].audio		= nfui_check_button_get_active(manager_val[6]);
	authdata[1].microphone	= nfui_check_button_get_active(manager_val[7]);
	authdata[1].remote		= nfui_check_button_get_active(manager_val[8]);
	authdata[1].ptz			= nfui_check_button_get_active(manager_val[9]);
	authdata[1].shutdown	= nfui_check_button_get_active(manager_val[10]);

	authdata[2].search		= nfui_check_button_get_active(user_val[1]);
	authdata[2].archive		= nfui_check_button_get_active(user_val[2]);
	authdata[2].sys_setup 	= nfui_check_button_get_active(user_val[3]);
	authdata[2].rec_setup	= nfui_check_button_get_active(user_val[4]);
	authdata[2].event		= nfui_check_button_get_active(user_val[5]);
	authdata[2].audio		= nfui_check_button_get_active(user_val[6]);
	authdata[2].microphone	= nfui_check_button_get_active(user_val[7]);
	authdata[2].remote		= nfui_check_button_get_active(user_val[8]);
	authdata[2].ptz			= nfui_check_button_get_active(user_val[9]);
	authdata[2].shutdown	= nfui_check_button_get_active(user_val[10]);

	if(!memcmp(org_authdata, authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		scm_put_log(CHANGE_USER_GRP, 0, 0);

		g_memmove(org_authdata, authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT);
		DAL_set_userAuth_data_all(authdata, 3);	// 3 : number of groups
		
		sysuser_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(authdata, org_authdata, sizeof(UserAuthData)*MAX_GROUP_COUNT);
		
		nfui_check_button_set_active_no_expose(manager_val[1], authdata[1].search);
		nfui_check_button_set_active_no_expose(manager_val[2], authdata[1].archive);
		nfui_check_button_set_active_no_expose(manager_val[3], authdata[1].sys_setup);
		nfui_check_button_set_active_no_expose(manager_val[4], authdata[1].rec_setup);
		nfui_check_button_set_active_no_expose(manager_val[5], authdata[1].event);
		nfui_check_button_set_active_no_expose(manager_val[6], authdata[1].audio);
		nfui_check_button_set_active_no_expose(manager_val[7], authdata[1].microphone);
		nfui_check_button_set_active_no_expose(manager_val[8], authdata[1].remote);
		nfui_check_button_set_active_no_expose(manager_val[9], authdata[1].ptz);
		nfui_check_button_set_active_no_expose(manager_val[10], authdata[1].shutdown);
		
		nfui_check_button_set_active_no_expose(user_val[1], authdata[2].search);
		nfui_check_button_set_active_no_expose(user_val[2], authdata[2].archive);
		nfui_check_button_set_active_no_expose(user_val[3], authdata[2].sys_setup);
		nfui_check_button_set_active_no_expose(user_val[4], authdata[2].rec_setup);
		nfui_check_button_set_active_no_expose(user_val[5], authdata[2].event);
		nfui_check_button_set_active_no_expose(user_val[6], authdata[2].audio);
		nfui_check_button_set_active_no_expose(user_val[7], authdata[2].microphone);
		nfui_check_button_set_active_no_expose(user_val[8], authdata[2].remote);
		nfui_check_button_set_active_no_expose(user_val[9], authdata[2].ptz);
		nfui_check_button_set_active_no_expose(user_val[10], authdata[2].shutdown);

	}

	return FALSE;
	
}






