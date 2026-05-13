
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"

#include "modules/ssm.h"
#include "smt.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_disp_main.h"
#include "vw_sys_sound_main.h"
#include "vw_sys_user_main.h"
#include "vw_sys_net_main.h"
#include "vw_evt_act_main.h"
#include "vw_sys_main.h"
#include "vw_disk_main.h"

#include "vw_main_menu_itx2.h"
#include "vw_menu.h"


#define WIN_SIZE_W				(1920)
#define WIN_SIZE_H				(1080)

#define PRE_MENU_FOCUS			(g_menu_focus[0])
#define CUR_MENU_FOCUS			(g_menu_focus[1])
#define CUR_SUBMENU_FOCUS		(g_submenu_focus)
#define FOCUSING_SUBMENU		(g_focusing_submenu)

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_menu[SYS_SUBMENU_CNT];
static NFOBJECT *g_subWnd[SYS_SUBMENU_CNT];
static NFOBJECT *g_goto;

static gint g_menu_focus[2] = {-1, -1};
static gint g_submenu_focus = -1;
static gboolean g_focusing_submenu = FALSE;

static gint submenu_cnt[SYS_SUBMENU_CNT];

static void set_menu_focus(gint focus)
{
	if(PRE_MENU_FOCUS != -1)
		nfui_set_key_focus(g_menu[PRE_MENU_FOCUS], FALSE);
	nfui_set_key_focus(g_menu[focus], TRUE);

	if(PRE_MENU_FOCUS != -1)
		nfui_signal_emit(g_menu[PRE_MENU_FOCUS], GDK_EXPOSE, FALSE);
	nfui_signal_emit(g_menu[focus], GDK_EXPOSE, FALSE);
}

static void show_submenu(gint focus)
{
	if(PRE_MENU_FOCUS >= 0)  {
		nfui_unregi_semi_modal((NFWINDOW*)g_subWnd[PRE_MENU_FOCUS]);
		nfui_nfobject_hide(g_subWnd[PRE_MENU_FOCUS]);
		nfui_page_close(PGID_SUB_SETUPMENU, g_subWnd[PRE_MENU_FOCUS]);
	}

	nfui_regi_semi_modal((NFWINDOW*)g_subWnd[focus]);
	nfui_nfobject_show(g_subWnd[focus]);
	nfui_make_key_hierarchy((NFWINDOW*)g_subWnd[focus]);
	nfui_page_open(PGID_SUB_SETUPMENU, g_subWnd[focus], ssm_get_cur_id(NULL));

	PRE_MENU_FOCUS = focus;

	if(CUR_MENU_FOCUS != focus) {
		CUR_MENU_FOCUS = focus;
	}
}

static void set_submenu_focus(gint focus, gint pre, gint cur)
{
	NFFIXED *fixed;
	NFOBJECT *old_obj;
	NFOBJECT *new_obj;

	fixed = (NFFIXED*)(((NFWINDOW*)g_subWnd[focus])->child);
	if(pre >= 0) { 
		old_obj = g_slist_nth_data(fixed->children, (guint)pre);

		nfui_set_key_focus(old_obj, FALSE);
		nfui_signal_emit(old_obj, GDK_EXPOSE, FALSE);
	}

	if(cur >= 0) { 
		new_obj = g_slist_nth_data(fixed->children, (guint)cur);

		nfui_set_key_focus(new_obj, TRUE);
		nfui_signal_emit(new_obj, GDK_EXPOSE, FALSE);
	}
}

static void hide_submenu()
{
	if(CUR_MENU_FOCUS != -1)
		set_submenu_focus(CUR_MENU_FOCUS, CUR_SUBMENU_FOCUS, -1);

	if(PRE_MENU_FOCUS >= 0) {
		nfui_unregi_semi_modal((NFWINDOW*)g_subWnd[PRE_MENU_FOCUS]);
		nfui_nfobject_hide(g_subWnd[PRE_MENU_FOCUS]);
		nfui_page_close(PGID_SUB_SETUPMENU, g_subWnd[PRE_MENU_FOCUS]);
	}

	PRE_MENU_FOCUS = -1;
	CUR_MENU_FOCUS = -1;
	CUR_SUBMENU_FOCUS = -1;
	FOCUSING_SUBMENU = FALSE;
}

static void unset_focusing_submenu()
{
	if(FOCUSING_SUBMENU) {
		set_submenu_focus(CUR_MENU_FOCUS, CUR_SUBMENU_FOCUS, -1);

		FOCUSING_SUBMENU = FALSE;
		CUR_SUBMENU_FOCUS = -1;
	}
}

static void close_main_menu()
{
	nfui_nfobject_destroy((NFOBJECT*)g_curwnd); 
}

static gboolean post_cam_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			SystemSetupCam_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(CAMERA_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != CAMERA_SUBMENU) {
				set_menu_focus(CAMERA_SUBMENU);
				show_submenu(CAMERA_SUBMENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disp_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			SystemSetupDisp_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(DISPLAY_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != DISPLAY_SUBMENU) {
				set_menu_focus(DISPLAY_SUBMENU);
				show_submenu(DISPLAY_SUBMENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_snd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			SystemSetupSound_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(AUDIO_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != AUDIO_SUBMENU) {
				set_menu_focus(AUDIO_SUBMENU);
				show_submenu(AUDIO_SUBMENU);
			}
			break;
		
		default:
			break;
	}
	return FALSE;
}

static gboolean post_usr_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			SystemSetupUser_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(USER_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != USER_SUBMENU) {
				set_menu_focus(USER_SUBMENU);
				show_submenu(USER_SUBMENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_net_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			SystemSetupNetwork_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(NETWORK_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != NETWORK_SUBMENU) {
				set_menu_focus(NETWORK_SUBMENU);
				show_submenu(NETWORK_SUBMENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_sys_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			VW_SetupSystem_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(SYSTEM_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != SYSTEM_SUBMENU) {
				set_menu_focus(SYSTEM_SUBMENU);
				show_submenu(SYSTEM_SUBMENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disk_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			VW_DiskSetup_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(STORAGE_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != STORAGE_SUBMENU) {
				set_menu_focus(STORAGE_SUBMENU);
				show_submenu(STORAGE_SUBMENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_evt_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			nfui_disable_semi_modal_mode();
			VW_Evt_Act_Open(g_curwnd, NULL, 0);
			if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
			smt_set_service(SMT_MAIN_MENU);				
			break;

		case GDK_ENTER_NOTIFY:
			unset_focusing_submenu();
			show_submenu(EVENT_SUBMENU);
			break;

		case GDK_MOTION_NOTIFY:
			if(CUR_MENU_FOCUS != EVENT_SUBMENU) {
				set_menu_focus(EVENT_SUBMENU);
				show_submenu(EVENT_SUBMENU);
			}
			break;
		
		default:
			break;
	}
	return FALSE;
}

static gboolean post_cam_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				SystemSetupCam_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disp_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				SystemSetupDisp_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);				
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_snd_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				SystemSetupSound_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);									
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);				
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_usr_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				SystemSetupUser_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);				
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_net_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				SystemSetupNetwork_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_sys_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				VW_SetupSystem_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);				
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disk_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				VW_DiskSetup_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);				
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_evt_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				gint page_idx = 0;

				smt_set_service(SMT_SYSTEM_SETUP);	
				nfui_disable_semi_modal_mode();

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));
				VW_Evt_Act_Open(g_curwnd, NULL, page_idx);
				if (g_curwnd) {
					nfui_set_key_focus(obj, FALSE);
					nfui_enable_semi_modal_mode(g_curwnd);
				}
    			smt_set_service(SMT_MAIN_MENU);				
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static void _draw_subline(NFOBJECT *obj, gint cnt, gint top)
{
    GdkGC *gc;
    GdkDrawable *drawable;

	drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
	gc = nfui_nfobject_get_gc(obj);

	switch(cnt)
	{
		case 1:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_01, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_01, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 2:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_02, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_02, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 3:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_03, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_03, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 4:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_04, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_04, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 5:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_05, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_05, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 6:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_06, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_06, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 7:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_07, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_07, 0, 0, -1, -1, NFALIGN_LEFT, 0);
		break;
		case 8:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_08, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_08, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 9:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_09, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_09, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 10:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_10, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_10, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 11:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_11, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_11, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 12:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_12, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_12, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 13:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_13, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_13, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 14:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_14, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_14, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 15:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_15, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_15, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;
		case 16:
		    if (top)    nfutil_draw_image(drawable, gc, IMG_SUBLINE_T_16, 0, 0, -1, -1, NFALIGN_LEFT, 0);
            else        nfutil_draw_image(drawable, gc, IMG_SUBLINE_B_16, 0, 0, -1, -1, NFALIGN_LEFT, 0);            
		break;								
    }
    
    nfui_nfobject_gc_unref(gc);    
}

static gboolean post_cam_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[CAMERA_SUBMENU], 1);
	}
	return FALSE;
}

static gboolean post_disp_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[DISPLAY_SUBMENU], 1);
	}
	return FALSE;
}

static gboolean post_snd_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[AUDIO_SUBMENU], 1);
	}
	return FALSE;
}

static gboolean post_usr_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[USER_SUBMENU], 1);
	}
	return FALSE;
}

static gboolean post_net_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[NETWORK_SUBMENU], 0);
	}
	return FALSE;
}

static gboolean post_sys_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[SYSTEM_SUBMENU], 0);
	}
	return FALSE;
}

static gboolean post_disk_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[STORAGE_SUBMENU], 0);
	}
	return FALSE;
}

static gboolean post_evt_subFixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {

		if(!nfui_nfobject_is_shown(obj))
			return FALSE;

        _draw_subline(obj, submenu_cnt[EVENT_SUBMENU], 0);
	}
	return FALSE;
}

static gboolean post_sub_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint pre_focus = -1;

	switch(evt->type) {
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				switch(kpid) {
					case KEYPAD_LEFT:
					case KEYPAD_RIGHT:
						{
							if(!FOCUSING_SUBMENU) {
								FOCUSING_SUBMENU = TRUE;
								CUR_SUBMENU_FOCUS = 0;
							}
							else {
								pre_focus = CUR_SUBMENU_FOCUS;

								FOCUSING_SUBMENU = FALSE;
								CUR_SUBMENU_FOCUS = -1;
							}

							set_submenu_focus(CUR_MENU_FOCUS, pre_focus, CUR_SUBMENU_FOCUS);
						}
						return TRUE;

					case KEYPAD_UP:
						{
							if(FOCUSING_SUBMENU) {
								pre_focus = CUR_SUBMENU_FOCUS;

								if(CUR_SUBMENU_FOCUS-1 < 0)
									CUR_SUBMENU_FOCUS = submenu_cnt[CUR_MENU_FOCUS]-1;
								else
									CUR_SUBMENU_FOCUS--;
								break;
							}

							if(CUR_MENU_FOCUS-1 < 0) {
								CUR_MENU_FOCUS = SYS_SUBMENU_CNT-1;

								if(0)
								{
									nfui_set_key_focus(g_menu[PRE_MENU_FOCUS], FALSE);
									nfui_set_key_focus(g_goto, TRUE);

									nfui_signal_emit(g_menu[PRE_MENU_FOCUS], GDK_EXPOSE, FALSE);
									nfui_signal_emit(g_goto, GDK_EXPOSE, FALSE);
								}
							}
							else {
								CUR_MENU_FOCUS--;
							}

							set_menu_focus(CUR_MENU_FOCUS);
							show_submenu(CUR_MENU_FOCUS);
						}
						return TRUE;

					case KEYPAD_DOWN:
						{
							if(FOCUSING_SUBMENU) {
								pre_focus = CUR_SUBMENU_FOCUS;

								if(CUR_SUBMENU_FOCUS+1 > submenu_cnt[CUR_MENU_FOCUS]-1) {
									CUR_SUBMENU_FOCUS = 0;

									if(0)
									{
										nfui_set_key_focus(g_menu[PRE_MENU_FOCUS], FALSE);
										nfui_set_key_focus(g_goto, TRUE);

										nfui_signal_emit(g_menu[PRE_MENU_FOCUS], GDK_EXPOSE, FALSE);
										nfui_signal_emit(g_goto, GDK_EXPOSE, FALSE);
									}
								}
								else {
									CUR_SUBMENU_FOCUS++;
								}
								break;
							}

							if(CUR_MENU_FOCUS+1 > SYS_SUBMENU_CNT-1) 	CUR_MENU_FOCUS = 0;
							else 								CUR_MENU_FOCUS++;

							set_menu_focus(CUR_MENU_FOCUS);
							show_submenu(CUR_MENU_FOCUS);
						}
						return TRUE;

					case KEYPAD_ENTER:
						{
							if(!FOCUSING_SUBMENU) {
								smt_set_service(SMT_SYSTEM_SETUP);	
								nfui_disable_semi_modal_mode();

								if(CUR_MENU_FOCUS == CAMERA_SUBMENU) 			SystemSetupCam_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == DISPLAY_SUBMENU) 	SystemSetupDisp_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == AUDIO_SUBMENU) 		SystemSetupSound_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == USER_SUBMENU) 		SystemSetupUser_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == NETWORK_SUBMENU) 	SystemSetupNetwork_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == SYSTEM_SUBMENU) 		VW_SetupSystem_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == STORAGE_SUBMENU) 	VW_DiskSetup_Open(g_curwnd, NULL, 0);
								else if(CUR_MENU_FOCUS == EVENT_SUBMENU) 		VW_Evt_Act_Open(g_curwnd, NULL, 0);

								if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
                    			smt_set_service(SMT_MAIN_MENU);

								return TRUE;
							}
						}
						break;

					default: 
						break;
				}
			}
			break;

		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_EXIT)
				{
					FOCUSING_SUBMENU = FALSE;
					CUR_SUBMENU_FOCUS = -1;
					close_main_menu();
					return TRUE;
				}
			}
			break;

		case GDK_DELETE:
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid) {
			case KEYPAD_UP:
				if(PRE_MENU_FOCUS == -1 && CUR_MENU_FOCUS == -1) {
					set_menu_focus(EVENT_SUBMENU);
					show_submenu(EVENT_SUBMENU);
					return TRUE;
				}
				else {
					if(0)
					{
						nfui_set_key_focus(g_goto, FALSE);
						nfui_signal_emit(g_goto, GDK_EXPOSE, FALSE);
					}

					set_menu_focus(CUR_MENU_FOCUS);
					show_submenu(CUR_MENU_FOCUS);
				}
				break;

			case KEYPAD_DOWN:
				if(PRE_MENU_FOCUS == -1 && CUR_MENU_FOCUS == -1) {
					set_menu_focus(CAMERA_SUBMENU);
					show_submenu(CAMERA_SUBMENU);
					return TRUE;
				}
				else {
					if(0)
					{
						nfui_set_key_focus(g_goto, FALSE);
						nfui_signal_emit(g_goto, GDK_EXPOSE, FALSE);
					}

					set_menu_focus(CUR_MENU_FOCUS);
					show_submenu(CUR_MENU_FOCUS);
				}
				break;

			case KEYPAD_LEFT:
			case KEYPAD_RIGHT:
			case KEYPAD_ENTER:
				if(PRE_MENU_FOCUS == -1 && CUR_MENU_FOCUS == -1) {
					set_menu_focus(CAMERA_SUBMENU);
					show_submenu(CAMERA_SUBMENU);
					return TRUE;
				}
				break;

			default:
				break;
		}

	}
	else if(evt->type == NFEVENT_KEYPAD_RELEASE || evt->type == NFEVENT_REMOCON_RELEASE) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid) {
			case KEYPAD_EXIT:
			{
				close_main_menu();
			}
			break;

			default:
				break;
		}		
	}	
	else if(evt->type == GDK_DELETE) {
		gint i;

		for(i=0; i<SYS_SUBMENU_CNT; i++) {
			nfui_unregi_semi_modal((NFWINDOW*)g_subWnd[i]);
			nfui_nfobject_destroy(g_subWnd[i]);

			nfui_page_close(PGID_SUB_SETUPMENU, g_subWnd[i]);
		}

		nfui_unregi_semi_modal((NFWINDOW*)obj);
		nfui_disable_semi_modal_mode();

		nfui_page_close(PGID_SETUPMENU, obj);

		PRE_MENU_FOCUS = -1;
		CUR_MENU_FOCUS = -1;
		CUR_SUBMENU_FOCUS = -1;
		FOCUSING_SUBMENU = FALSE;
		g_curwnd = 0;
		
		gtk_main_quit();
	}

	return FALSE;
}


static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkGC *gc;
		GdkDrawable *drawable;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfutil_draw_image(drawable, gc, IMG_MAINMENU_BG, 0, 0, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean post_exit_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		close_main_menu();
	else if(evt->type == GDK_ENTER_NOTIFY)
		hide_submenu();
	else if(evt->type == GDK_LEAVE_NOTIFY) 
		hide_submenu();

	return FALSE;
}

static NFOBJECT* create_camera_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub1(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, 245, 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[CAMERA_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_cam_subFixed_event_handler);
	nfui_nfobject_show(fixed);


	for(i=0; i<submenu_cnt[CAMERA_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_cam_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, (guint)21, (guint)(24 + (i * 35)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_display_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub2(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, 245 + (1 * 70), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[DISPLAY_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_disp_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[DISPLAY_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_disp_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(24 + (i * 35)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_sound_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub3(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, 245 + (2 * 70), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[AUDIO_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_snd_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[AUDIO_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_snd_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(24 + (i * 35)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_user_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub4(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, 245 + (3 * 70), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[USER_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_usr_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[USER_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_usr_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(24 + (i * 35)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_network_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub5(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, ((245 + ((4 + 1) * 70)) - 368), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[NETWORK_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_net_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[NETWORK_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_net_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(368 -13 - 35*(submenu_cnt[NETWORK_SUBMENU]-i)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_system_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub6(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, ((245 + ((5 + 1) * 70)) - 368), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[SYSTEM_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_sys_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[SYSTEM_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_sys_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(368 -13 - 35*(submenu_cnt[SYSTEM_SUBMENU]-i)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_storage_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub7(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, ((245 + ((6 + 1) * 70)) - 368), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[STORAGE_SUBMENU] = subwindow;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_disk_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[STORAGE_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_disk_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(368 -13 - 35*(submenu_cnt[STORAGE_SUBMENU]-i)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

static NFOBJECT* create_event_action_submenu(NFWINDOW *parent)
{
	NFOBJECT* subwindow;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	gint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};
	gint i;

	const gchar *strSubmenu[MAX_TAB_CNT];

    vw_menu_get_str_sys_menu_sub8(strSubmenu);

	subwindow = (NFOBJECT*)nfui_nfwindow_new(parent, 932, ((245 + ((7 + 1) * 70)) - 368), 226, 368);
	nfui_nfwindow_set_title((NFWINDOW*)subwindow, "SUB MENU");
	nfui_regi_post_event_callback(subwindow, post_sub_wnd_event_handler);
	nfui_nfwindow_set_modal((NFWINDOW*)subwindow, FALSE);

	g_subWnd[EVENT_SUBMENU] = subwindow;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 226, 368);
	nfui_regi_post_event_callback(fixed, post_evt_subFixed_event_handler);
	nfui_nfobject_show(fixed);

	for(i=0; i<submenu_cnt[EVENT_SUBMENU]; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_no_image(btn_bg, strSubmenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 0);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI), btn_fg);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_set_size(obj, 205, 28);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_evt_subButton_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 21, (guint)(368 -13 - 35*(submenu_cnt[EVENT_SUBMENU]-i)));
	}

	nfui_nfwindow_add((NFWINDOW*)subwindow, fixed);
	nfui_run_main_event_handler(subwindow);
	nfui_make_key_hierarchy((NFWINDOW*)subwindow);
	nfui_nfobject_hide(subwindow);

	return subwindow;
}

gboolean VW_ITX2_MainMenu_Open(NFWINDOW *parent)
{
	NFOBJECT* window;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	GdkPixbuf *pbImg[9][NFOBJECT_STATE_COUNT];

	const gchar *strBtn[8] = {"CAMERA", 
							"DISPLAY", 
							"SOUND", 
							"USER", 
							"NETWORK", 
							"SYSTEM", 
							"STORAGE", 
							"EVENT"};

	guint btn_fg[NFOBJECT_STATE_COUNT] = {COLOR_IDX(702), COLOR_IDX(700), COLOR_IDX(700), COLOR_IDX(702)};
	gint btn_bg[NFOBJECT_STATE_COUNT] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg2[NFOBJECT_STATE_COUNT] = {COLOR_IDX(708), COLOR_IDX(709), COLOR_IDX(709), COLOR_IDX(708)};

	gint size_w, size_h;
	gint i;


	window = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, WIN_SIZE_W, WIN_SIZE_H);
	nfui_nfwindow_set_title((NFWINDOW*)window, "SYSTEM SETUP");
	nfui_nfwindow_set_modal((NFWINDOW*)window, FALSE);
	nfui_regi_post_event_callback(window, post_wnd_event_handler);
	nfui_regi_semi_modal((NFWINDOW*)window);
	nfui_hook_evt_in_semi_modal(GDK_BUTTON_PRESS);
	nfui_enable_semi_modal_mode((NFWINDOW*)window);
	g_curwnd = (NFWINDOW*)window;


	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WIN_SIZE_W, WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);


	// TITLE
	obj = (NFOBJECT*)nfui_nfimage_new(IMG_MAINMENU_TITLE);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 672, 175);

	// MENU
	pbImg[0][0] = nfui_get_image_from_file((IMG_SUBMENU_N_CAMERA), NULL);
	pbImg[0][1] = nfui_get_image_from_file((IMG_SUBMENU_F_CAMERA), NULL);
	pbImg[0][2] = nfui_get_image_from_file((IMG_SUBMENU_F_CAMERA), NULL);
	pbImg[0][3] = nfui_get_image_from_file((IMG_SUBMENU_N_CAMERA), NULL);

	pbImg[1][0] = nfui_get_image_from_file((IMG_SUBMENU_N_DISPLAY), NULL);
	pbImg[1][1] = nfui_get_image_from_file((IMG_SUBMENU_F_DISPLAY), NULL);
	pbImg[1][2] = nfui_get_image_from_file((IMG_SUBMENU_F_DISPLAY), NULL);
	pbImg[1][3] = nfui_get_image_from_file((IMG_SUBMENU_N_DISPLAY), NULL);

	pbImg[2][0] = nfui_get_image_from_file((IMG_SUBMENU_N_SOUND), NULL);
	pbImg[2][1] = nfui_get_image_from_file((IMG_SUBMENU_F_SOUND), NULL);
	pbImg[2][2] = nfui_get_image_from_file((IMG_SUBMENU_F_SOUND), NULL);
	pbImg[2][3] = nfui_get_image_from_file((IMG_SUBMENU_N_SOUND), NULL);

	pbImg[3][0] = nfui_get_image_from_file((IMG_SUBMENU_N_USER), NULL);
	pbImg[3][1] = nfui_get_image_from_file((IMG_SUBMENU_F_USER), NULL);
	pbImg[3][2] = nfui_get_image_from_file((IMG_SUBMENU_F_USER), NULL);
	pbImg[3][3] = nfui_get_image_from_file((IMG_SUBMENU_N_USER), NULL);

	pbImg[4][0] = nfui_get_image_from_file((IMG_SUBMENU_N_NETWORK), NULL);
	pbImg[4][1] = nfui_get_image_from_file((IMG_SUBMENU_F_NETWORK), NULL);
	pbImg[4][2] = nfui_get_image_from_file((IMG_SUBMENU_F_NETWORK), NULL);
	pbImg[4][3] = nfui_get_image_from_file((IMG_SUBMENU_N_NETWORK), NULL);

	pbImg[5][0] = nfui_get_image_from_file((IMG_SUBMENU_N_SYSTEM), NULL);
	pbImg[5][1] = nfui_get_image_from_file((IMG_SUBMENU_F_SYSTEM), NULL);
	pbImg[5][2] = nfui_get_image_from_file((IMG_SUBMENU_F_SYSTEM), NULL);
	pbImg[5][3] = nfui_get_image_from_file((IMG_SUBMENU_N_SYSTEM), NULL);

	pbImg[6][0] = nfui_get_image_from_file((IMG_SUBMENU_N_STORAGE), NULL);
	pbImg[6][1] = nfui_get_image_from_file((IMG_SUBMENU_F_STORAGE), NULL);
	pbImg[6][2] = nfui_get_image_from_file((IMG_SUBMENU_F_STORAGE), NULL);
	pbImg[6][3] = nfui_get_image_from_file((IMG_SUBMENU_N_STORAGE), NULL);

	pbImg[7][0] = nfui_get_image_from_file((IMG_SUBMENU_N_EVENT), NULL);
	pbImg[7][1] = nfui_get_image_from_file((IMG_SUBMENU_F_EVENT), NULL);
	pbImg[7][2] = nfui_get_image_from_file((IMG_SUBMENU_F_EVENT), NULL);
	pbImg[7][3] = nfui_get_image_from_file((IMG_SUBMENU_N_EVENT), NULL);

	pbImg[8][0] = nfui_get_image_from_file((IMG_MAINMENU_N_EXIT), NULL);
	pbImg[8][1] = nfui_get_image_from_file((IMG_MAINMENU_F_EXIT), NULL);
	pbImg[8][2] = nfui_get_image_from_file((IMG_MAINMENU_F_EXIT), NULL);
	pbImg[8][3] = nfui_get_image_from_file((IMG_MAINMENU_N_EXIT), NULL);

	nfui_get_image_size(IMG_SUBMENU_N_CAMERA, &size_w, &size_h);

	for(i=0; i<SYS_SUBMENU_CNT; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), pbImg[i]);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
		nfui_nfbutton_set_text(NF_BUTTON(obj), strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 84);
		nfui_nfbutton_set_bg_color(NF_BUTTON(obj), btn_bg);
		nfui_nfobject_set_size(obj, size_w, size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 672, (guint)(245 + (i * 70)));

		if (i == CAMERA_SUBMENU)		 nfui_regi_post_event_callback(obj, post_cam_button_event_handler);
		else if (i == DISPLAY_SUBMENU)  nfui_regi_post_event_callback(obj, post_disp_button_event_handler);
		else if (i == AUDIO_SUBMENU)    nfui_regi_post_event_callback(obj, post_snd_button_event_handler);
		else if (i == USER_SUBMENU) 	 nfui_regi_post_event_callback(obj, post_usr_button_event_handler);
		else if (i == NETWORK_SUBMENU)  nfui_regi_post_event_callback(obj, post_net_button_event_handler);
		else if (i == SYSTEM_SUBMENU) 	 nfui_regi_post_event_callback(obj, post_sys_button_event_handler);
		else if (i == STORAGE_SUBMENU)  nfui_regi_post_event_callback(obj, post_disk_button_event_handler);
		else if (i == EVENT_SUBMENU) 	 nfui_regi_post_event_callback(obj, post_evt_button_event_handler);

		g_menu[i] = obj;
	}

    submenu_cnt[CAMERA_SUBMENU] = mcf.sys_sub1.cnt;
    submenu_cnt[DISPLAY_SUBMENU] = mcf.sys_sub2.cnt;    
    submenu_cnt[AUDIO_SUBMENU] = mcf.sys_sub3.cnt;    
    submenu_cnt[USER_SUBMENU] = mcf.sys_sub4.cnt;    
    submenu_cnt[NETWORK_SUBMENU] = mcf.sys_sub5.cnt;    
    submenu_cnt[SYSTEM_SUBMENU] = mcf.sys_sub6.cnt;    
    submenu_cnt[STORAGE_SUBMENU] = mcf.sys_sub7.cnt;    
    submenu_cnt[EVENT_SUBMENU] = mcf.sys_sub8.cnt;

	// SUBMENU
	create_camera_submenu(parent);
	create_display_submenu(parent);
	create_sound_submenu(parent);
	create_user_submenu(parent);
	create_network_submenu(parent);
	create_system_submenu(parent);
	create_storage_submenu(parent);
	create_event_action_submenu(parent);

	// go to
	nfui_get_image_size(IMG_MAINMENU_N_EXIT, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), pbImg[8]);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg2);
	nfui_nfbutton_set_text(NF_BUTTON(obj), "EXIT");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_exit_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 842, 835);
	g_goto = obj;


	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_SETUPMENU, window, ssm_get_cur_id(NULL));

	gtk_main();

	return TRUE;
}
