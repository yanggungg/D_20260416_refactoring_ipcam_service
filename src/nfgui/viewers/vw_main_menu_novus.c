
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
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

#include "vw_main_menu_novus.h"



static NFWINDOW *g_curwnd = 0;


static gboolean post_menu_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *title;
	gint menu;

	switch(evt->type) {
		case GDK_EXPOSE:
			{
				title = (NFOBJECT*)nfui_nfobject_get_data(obj, "MENU TITLE");
				if(NF_BUTTON(obj)->object.kfocus == NFOBJECT_FOCUS)
					nfui_nfimage_set_pango_font((NFIMAGE*)title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
				else
					nfui_nfimage_set_pango_font((NFIMAGE*)title, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
				nfui_signal_emit(title, GDK_EXPOSE, FALSE);
			}	
			break;

		case GDK_ENTER_NOTIFY:
			title = (NFOBJECT*)nfui_nfobject_get_data(obj, "MENU TITLE");
			nfui_nfimage_set_pango_font((NFIMAGE*)title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(700));
			nfui_signal_emit(title, GDK_EXPOSE, FALSE);
			break;

		case GDK_LEAVE_NOTIFY:
			title = (NFOBJECT*)nfui_nfobject_get_data(obj, "MENU TITLE");
			nfui_nfimage_set_pango_font((NFIMAGE*)title, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
			nfui_signal_emit(title, GDK_EXPOSE, FALSE);
			break;

		case GDK_BUTTON_RELEASE:
			smt_set_service(SMT_SYSTEM_SETUP);	

			menu = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "MENU INDEX"));
			if(menu == 0) SystemSetupCam_Open(g_curwnd, NULL, 0);
			else if(menu == 1) SystemSetupDisp_Open(g_curwnd, NULL, 0);
			else if(menu == 2) SystemSetupSound_Open(g_curwnd, NULL, 0);
			else if(menu == 3) SystemSetupUser_Open(g_curwnd, NULL, 0);
			else if(menu == 4) SystemSetupNetwork_Open(g_curwnd, NULL, 0);
			else if(menu == 5) VW_SetupSystem_Open(g_curwnd, NULL, 0);
			else if(menu == 6) VW_DiskSetup_Open(g_curwnd, NULL, 0);
			else if(menu == 7) VW_Evt_Act_Open(g_curwnd, NULL, 0);

   			smt_set_service(SMT_MAIN_MENU);			
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean post_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFOUTEVT_BUTTON_PRESS) {
		nfui_nfobject_destroy((NFOBJECT*)g_curwnd); 
	}
	else if (evt->type == GDK_DELETE) {
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

gboolean VW_NOVUS_MainMenu_Open(NFWINDOW *parent, gint x, gint y)
{
	NFOBJECT* window;
	NFOBJECT* fixed;
	NFOBJECT *obj1, *obj2;

	GdkPixbuf *pbImg[8][NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgImg;	
	GdkPixbuf *pbBG;

	gint size_w, size_h;
	gint pos_x, pos_y;	
	gint i;

	const gchar *strBtn[8] = {"CAMERA", "DISPLAY", "SOUND", "USER", 
							"NETWORK", "SYSTEM", "STORAGE", "EVENT"};



	// window
	window = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)x, (guint)y, 1529, 158);
	nfui_nfwindow_set_title((NFWINDOW*)window, "SYSTEM SETUP");
	nfui_regi_post_event_callback(window, post_wnd_event_handler);
	nfui_nfwindow_set_mask((NFWINDOW*)window, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)window, TRUE);
	g_curwnd = (NFWINDOW*)window;

	// fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 1529, 158);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);


	// button
	bgImg = nfui_get_image_from_file(IMG_MAINMENU_BG, NULL);

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

	nfui_get_image_size(IMG_SUBMENU_N_CAMERA, &size_w, &size_h);
	
	pos_x = 5;
	pos_y = 5;

	for(i=0; i<8; i++) {
		obj1 = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj1), pbImg[i]);
		nfui_nfobject_set_size(obj1, size_w, size_h);
		nfui_regi_post_event_callback(obj1, post_menu_button_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed, obj1, (guint)pos_x, (guint)pos_y);
		nfui_nfobject_show(obj1);


		pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, size_w, 28);
		gdk_pixbuf_copy_area(bgImg, pos_x, pos_y+size_h, size_w, 28, pbBG, 0, 0);

		obj2 = (NFOBJECT*)nfui_nfimage_new_pixbuf(pbBG);
		nfui_nfimage_set_text((NFIMAGE*)obj2, strBtn[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj2, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(702));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj2, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj2, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixed, obj2, (guint)pos_x, (guint)(pos_y+size_h));
		nfui_nfobject_show(obj2);

		nfui_nfobject_set_data(obj1, "MENU INDEX", GINT_TO_POINTER(i));
		nfui_nfobject_set_data(obj1, "MENU TITLE", obj2);

		pos_x += size_w;
		pos_x += 1;
	}

	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_SETUPMENU, window, ssm_get_cur_id(NULL));

	gtk_main();

	return TRUE;
}
