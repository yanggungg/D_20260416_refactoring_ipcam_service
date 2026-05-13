
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
#include "viewers/objects/nflabel.h"

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

#include "vw_main_menu_cbc.h"
#include "vw_menu.h"

#define WIN_SIZE_W				(1920)
#define WIN_SIZE_H				(1080)

enum {
	CAMERA_MENU     = CAMERA_SUBMENU,
	DISPLAY_MENU    = DISPLAY_SUBMENU,
	AUDIO_MENU      = AUDIO_SUBMENU,
	USER_MENU       = USER_SUBMENU,
	NETWORK_MENU    = NETWORK_SUBMENU,
	SYSTEM_MENU     = SYSTEM_SUBMENU,
	STORAGE_MENU    = STORAGE_SUBMENU,
	EVENT_MENU      = EVENT_SUBMENU, 
	EXIT_MENU       = EVENT_SUBMENU+1,
	MENU_CNT
};

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_menu[MENU_CNT];
static NFOBJECT *g_menu_text[MENU_CNT];


static gboolean post_cam_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[CAMERA_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[CAMERA_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[CAMERA_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[CAMERA_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[CAMERA_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[CAMERA_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[CAMERA_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			SystemSetupCam_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);	
		break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disp_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[DISPLAY_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[DISPLAY_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[DISPLAY_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[DISPLAY_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[DISPLAY_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[DISPLAY_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[DISPLAY_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			SystemSetupDisp_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_snd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[AUDIO_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[AUDIO_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[AUDIO_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[AUDIO_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[AUDIO_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[AUDIO_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[AUDIO_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			SystemSetupSound_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;
		
		default:
			break;
	}
	return FALSE;
}

static gboolean post_usr_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[USER_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[USER_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[USER_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[USER_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[USER_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[USER_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[USER_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			SystemSetupUser_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;
	
		default:
			break;
	}
	return FALSE;
}

static gboolean post_net_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[NETWORK_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[NETWORK_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[NETWORK_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[NETWORK_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[NETWORK_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[NETWORK_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[NETWORK_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			SystemSetupNetwork_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;
	
		default:
			break;
	}
	return FALSE;
}

static gboolean post_sys_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[SYSTEM_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[SYSTEM_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[SYSTEM_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[SYSTEM_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[SYSTEM_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[SYSTEM_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[SYSTEM_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);			
			VW_SetupSystem_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disk_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[STORAGE_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[STORAGE_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[STORAGE_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[STORAGE_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[STORAGE_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[STORAGE_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[STORAGE_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);			
			VW_DiskSetup_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;
	
		default:
			break;
	}
	return FALSE;
}

static gboolean post_evt_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EVENT_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EVENT_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));

			nfui_signal_emit(g_menu_text[EVENT_MENU], GDK_EXPOSE, TRUE);
		}	
		break;
		
		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EVENT_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[EVENT_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EVENT_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(g_menu_text[EVENT_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	
			VW_Evt_Act_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);				
		break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_exit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
		{
			if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EXIT_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			else
				nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EXIT_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(708));

			nfui_signal_emit(g_menu_text[EXIT_MENU], GDK_EXPOSE, TRUE);
		}	
		break;

		case GDK_ENTER_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EXIT_MENU], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(g_menu_text[EXIT_MENU], GDK_EXPOSE, TRUE);
		break;

		case GDK_LEAVE_NOTIFY:
			nfui_nfimage_set_pango_font((NFIMAGE*)g_menu_text[EXIT_MENU], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(708));
			nfui_signal_emit(g_menu_text[EXIT_MENU], GDK_EXPOSE, TRUE);
		break;
		
		case GDK_BUTTON_RELEASE:
			nfui_nfobject_destroy((NFOBJECT*)g_curwnd); 
		break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		nfui_page_close(PGID_SETUPMENU, obj);	
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

gboolean VW_CBC_MainMenu_Open(NFWINDOW *parent)
{
	NFOBJECT* window;
	NFOBJECT* fixed;
	NFOBJECT* obj;

	GdkPixbuf *pbImg[9][NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgImg;	
	GdkPixbuf *pbBG;

	const gchar *strBtn[MENU_CNT] = {"CAMERA", 
									"DISPLAY", 
									"SOUND", 
									"USER", 
									"NETWORK", 
									"SYSTEM", 
									"STORAGE", 
									"EVENT",
									"EXIT"};

	gint size_w, size_h;
	gint pos_x, pos_y;	
	gint i;

	window = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, WIN_SIZE_W, WIN_SIZE_H);
	nfui_nfwindow_set_title((NFWINDOW*)window, "SYSTEM SETUP");
	nfui_regi_post_event_callback(window, post_wnd_event_handler);
	g_curwnd = (NFWINDOW*)window;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WIN_SIZE_W, WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);

	// TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYSTEM SETUP", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(707));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 514, 36);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 703, 116);
	
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

	pos_x = 690;
	pos_y = 218;
	bgImg = nfui_get_image_from_file(IMG_MAINMENU_BG, NULL);

	for(i=0; i<MENU_CNT; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), pbImg[i]);
		nfui_nfobject_set_size(obj, size_w, size_h);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
		nfui_nfobject_show(obj);

		if (i == CAMERA_MENU)		 nfui_regi_post_event_callback(obj, post_cam_button_event_handler);
		else if (i == DISPLAY_MENU)  nfui_regi_post_event_callback(obj, post_disp_button_event_handler);
		else if (i == AUDIO_MENU)    nfui_regi_post_event_callback(obj, post_snd_button_event_handler);
		else if (i == USER_MENU) 	 nfui_regi_post_event_callback(obj, post_usr_button_event_handler);
		else if (i == NETWORK_MENU)  nfui_regi_post_event_callback(obj, post_net_button_event_handler);
		else if (i == SYSTEM_MENU) 	 nfui_regi_post_event_callback(obj, post_sys_button_event_handler);
		else if (i == STORAGE_MENU)  nfui_regi_post_event_callback(obj, post_disk_button_event_handler);
		else if (i == EVENT_MENU) 	 nfui_regi_post_event_callback(obj, post_evt_button_event_handler);
		else if (i == EXIT_MENU) 	 nfui_regi_post_event_callback(obj, post_exit_button_event_handler);

		g_menu[i] = obj;

		pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, size_w, 40);
		gdk_pixbuf_copy_area(bgImg, pos_x, pos_y+size_h, size_w, 40, pbBG, 0, 0);

		obj = nfui_nfimage_new_pixbuf(pbBG);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBtn[i]);
		if (i == EXIT_MENU)	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(708));
		else				nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+size_h);
		nfui_nfobject_show(obj);

		g_menu_text[i] = obj;	

		pos_x += size_w;

		if ((i == AUDIO_MENU) || (i == SYSTEM_MENU))
		{
			pos_x = 690;
			pos_y += (size_h+40);
		}		
	}

	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_SETUPMENU, window, ssm_get_cur_id(NULL));

	gtk_main();

	return TRUE;
}

