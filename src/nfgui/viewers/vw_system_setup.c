
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "smt.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/cw_mainmenu.h"
#include "viewers/objects/cw_submenu.h"

#include "vw_archiving.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_disp_main.h"
#include "vw_sys_sound_main.h"
#include "vw_sys_user_main.h"
#include "vw_sys_net_main.h"
#include "vw_evt_act_main.h"
#include "vw_disk_main.h"
#include "vw_system_setup.h"
#include "vw_sys_main.h"

#include "vw_menu.h"


#define VMM_WIN_W				1649
#define VMM_WIN_H				211

static CWMAINMENU *cmm;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *sub_window[8];
static CWSUBMENU *csm[8];

static gint subw_start_xpos;
static gint subw_start_ypos;

static GdkRectangle rect_main;
static GdkRectangle rect_sub;

static gint vw_mm_subm_xpos[8] = {0, 207, 414, 621, 828, 1035, 1242, 1449};

static gboolean vw_mm_is_subm_size = FALSE;
//static guint vw_mm_subm_height[8] = {470, 424, 378, 378, 470, 470, 470, 608};
static gint sel_mm_num = 0;
static gint previous_sel_mm_num = 0;
static gint sel_sm_num = 0;

// TOPMENU
#define TOP_CAMERA		0
#define TOP_DISPLAY		1
#define TOP_SOUND		2
#define TOP_USER		3
#define TOP_NETWORK		4
#define TOP_SYSTEM		5
#define TOP_STORAGE		6
#define TOP_EVENT		7

static gint	cw_sm_submenu_counts_of_item[8];

static gchar *_get_top_img_name(gint cnt)
{
	switch (cnt)
	{
		case 0 : return IMG_SUBMENU_F_CAMERA;	break;	
		case 1 : return IMG_SUBMENU_F_DISPLAY;	break;
		case 2 : return IMG_SUBMENU_F_SOUND;	break;	
		case 3 : return IMG_SUBMENU_F_USER;		break;
		case 4 : return IMG_SUBMENU_F_NETWORK;	break;	
		case 5 : return IMG_SUBMENU_F_SYSTEM;	break;
		case 6 : return IMG_SUBMENU_F_STORAGE;	break;	
		case 7 : return IMG_SUBMENU_F_EVENT;	break;			
		default : 	break;
	}

	return 0;
}

static gchar *_get_bottom_img_name(gint cnt)
{
	switch (cnt)
	{
		case 1 : return IMG_SUBMENU_ROW1;	break;	
		case 2 : return IMG_SUBMENU_ROW2;	break;
		case 3 : return IMG_SUBMENU_ROW3;	break;	
		case 4 : return IMG_SUBMENU_ROW4;	break;
		case 5 : return IMG_SUBMENU_ROW5;	break;	
		case 6 : return IMG_SUBMENU_ROW6;	break;
		case 7 : return IMG_SUBMENU_ROW7;	break;	
		case 8 : return IMG_SUBMENU_ROW8;	break;
		case 9 : return IMG_SUBMENU_ROW9;	break;
		case 10 : return IMG_SUBMENU_ROW10;	break;		
		case 11 : return IMG_SUBMENU_ROW11;	break;		
		case 12 : return IMG_SUBMENU_ROW12;	break;		
		case 13 : return IMG_SUBMENU_ROW13;	break;		
		case 14 : return IMG_SUBMENU_ROW14;	break;		
		case 15 : return IMG_SUBMENU_ROW15;	break;		
		case 16 : return IMG_SUBMENU_ROW16;	break;		
		default : 	break;
	}
	
	return 0;
}

static gchar *_get_mk_sub_img_name(gint cnt)
{
	switch (cnt)
	{
		case 0 : return MK_IMG_SUBMENU_CAMERA;	break;	
		case 1 : return MK_IMG_SUBMENU_DISPLAY;	break;
		case 2 : return MK_IMG_SUBMENU_SOUND;	break;	
		case 3 : return MK_IMG_SUBMENU_USER;	break;
		case 4 : return MK_IMG_SUBMENU_NETWORK;	break;	
		case 5 : return MK_IMG_SUBMENU_SYSTEM;	break;
		case 6 : return MK_IMG_SUBMENU_STORAGE;	break;	
		case 7 : return MK_IMG_SUBMENU_EVENT;	break;			
		default : 	break;
	}
	
	return 0;
}

static void _create_sub_menu_img()
{
	gint i, cnt;
	gchar *top_img;
	gchar *bottom_img;
	gchar *mk_img;

    cw_sm_submenu_counts_of_item[CAMERA_SUBMENU] = mcf.sys_sub1.cnt;
    cw_sm_submenu_counts_of_item[DISPLAY_SUBMENU] = mcf.sys_sub2.cnt;    
    cw_sm_submenu_counts_of_item[AUDIO_SUBMENU] = mcf.sys_sub3.cnt;    
    cw_sm_submenu_counts_of_item[USER_SUBMENU] = mcf.sys_sub4.cnt;    
    cw_sm_submenu_counts_of_item[NETWORK_SUBMENU] = mcf.sys_sub5.cnt;    
    cw_sm_submenu_counts_of_item[SYSTEM_SUBMENU] = mcf.sys_sub6.cnt;    
    cw_sm_submenu_counts_of_item[STORAGE_SUBMENU] = mcf.sys_sub7.cnt;    
    cw_sm_submenu_counts_of_item[EVENT_SUBMENU] = mcf.sys_sub8.cnt;   

	for (i = 0; i < 8; i++)
	{
		cnt = cw_sm_submenu_counts_of_item[i];

		mk_img = _get_mk_sub_img_name(i);
		top_img = _get_top_img_name(i);
		bottom_img = _get_bottom_img_name(cnt);

		nf_ui_create_main_submenu_image(mk_img, top_img, bottom_img);		
	}
}

static guint _get_width_sub_img(int page, gint *width, gint *height)
{
	gchar *mk_img;

	mk_img = _get_mk_sub_img_name(page);
	nfui_get_image_size(mk_img, width, height);
}


static void
vw_system_setup_selected_menu(NFOBJECT *obj, guint topmenu, guint submenu)
{
	int topmenu_num = topmenu;
	int submenu_num = submenu;

	nfui_disable_semi_modal_mode();

	switch(topmenu_num)
	{
		case TOP_CAMERA :
			SystemSetupCam_Open(g_curwnd, NULL, submenu_num);
		break;

		case TOP_DISPLAY :
			SystemSetupDisp_Open(g_curwnd, NULL, submenu_num);
		break;

		case TOP_SOUND :
			SystemSetupSound_Open(g_curwnd, NULL, submenu_num);
		break;
		
		case TOP_USER :
			SystemSetupUser_Open(g_curwnd, NULL, submenu_num);
		break;
		
		case TOP_NETWORK :
			SystemSetupNetwork_Open(g_curwnd, NULL, submenu_num);
		break;
		
		case TOP_SYSTEM :
			VW_SetupSystem_Open(g_curwnd, NULL, submenu_num);
		break;

		case TOP_STORAGE :
			VW_DiskSetup_Open(g_curwnd, NULL, submenu_num);
		break;

		case TOP_EVENT :
			VW_Evt_Act_Open(g_curwnd, NULL, submenu_num);
		break;
		
		default:
		break;
	}

	if (g_curwnd) nfui_enable_semi_modal_mode(g_curwnd);
}

static gboolean vw_system_setup_show_submenu_timer(void *data)
{
	int xpos = (int)data;

	NFUTIL_THREADS_ENTER();

	cw_sm_redraw_bg(csm[sel_mm_num]);
	cw_sm_sel_submenu_init(csm[sel_mm_num]);

	nfui_regi_semi_modal(sub_window[sel_mm_num]);
	nfui_nfobject_show(sub_window[sel_mm_num]);
	nfui_page_open(PGID_SUB_SETUPMENU, sub_window[sel_mm_num], ssm_get_cur_id(NULL));
	
	gdk_window_get_geometry(((NFWINDOW *)sub_window[sel_mm_num])->main_widget->window, 
							&rect_sub.x, &rect_sub.y,
							&rect_sub.width, &rect_sub.height, 0);	
	NFUTIL_THREADS_LEAVE();

	return FALSE;
}


static gboolean
vw_subwindow_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type)	
	{
		case NFEVENT_KEYPAD_PRESS :
		case NFEVENT_REMOCON_PRESS :
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			switch(kpid)
			{
				case KEYPAD_LEFT:
				{
					nfui_unregi_semi_modal(sub_window[sel_mm_num]);
					nfui_nfobject_hide(sub_window[sel_mm_num]);
					nfui_page_close(PGID_SUB_SETUPMENU, sub_window[sel_mm_num]);

					previous_sel_mm_num = sel_mm_num--;

					if(sel_mm_num < TOP_CAMERA)
						sel_mm_num = 7;

					guint xpos = vw_mm_subm_xpos[sel_mm_num] + subw_start_xpos;
					gdk_flush();
					cw_mm_set_sel_icon(cmm, sel_mm_num);

					sel_sm_num = 0;
					
					g_timeout_add(10, vw_system_setup_show_submenu_timer, xpos-30);

				}
				break;
				case KEYPAD_RIGHT:
				{
					
					nfui_unregi_semi_modal(sub_window[sel_mm_num]);
					nfui_nfobject_hide(sub_window[sel_mm_num]);
					nfui_page_close(PGID_SUB_SETUPMENU, sub_window[sel_mm_num]);

					previous_sel_mm_num = sel_mm_num++;

					if(sel_mm_num > TOP_EVENT)
						sel_mm_num = 0;

					guint xpos = vw_mm_subm_xpos[sel_mm_num] + subw_start_xpos;
					gdk_flush();
					cw_mm_set_sel_icon(cmm, sel_mm_num);

					sel_sm_num = 0;
					
					g_timeout_add(10, vw_system_setup_show_submenu_timer, xpos-30);
				}
				break;

				case KEYPAD_UP:
				{
					sel_sm_num--;
					if(sel_sm_num < 0) 
					{
						sel_sm_num = cw_sm_get_menu_counts(csm[sel_mm_num], sel_mm_num);
					}

					cw_sm_set_remote_select(csm[sel_mm_num], sel_sm_num);
				}
				break;

				case KEYPAD_DOWN:
				{
					sel_sm_num++;
					gint last_sm_num=0;
					last_sm_num = cw_sm_get_menu_counts(csm[sel_mm_num], sel_mm_num);

					if(sel_sm_num > last_sm_num)
					{
						sel_sm_num = 0;
					}
					
					cw_sm_set_remote_select(csm[sel_mm_num], sel_sm_num);
				}
				break;

				case KEYPAD_ENTER:
				sel_sm_num = cw_sm_get_selected_subitem(csm[sel_mm_num]);

				smt_set_service(SMT_SYSTEM_SETUP);	
				vw_system_setup_selected_menu(sub_window[sel_mm_num], sel_mm_num, sel_sm_num);
				smt_set_service(SMT_MAIN_MENU);
				return TRUE;

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

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_EXIT)
			{
				vw_destroy_system_setup();
				return TRUE;
			}
		}
		break;

		default: 
		break;
	}

	return FALSE;
}


static gboolean
vw_csm_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_SETUP_SUBMENU_CHANGED_RELEASE) 
	{
		sel_sm_num = cw_sm_get_selected_subitem(csm[sel_mm_num]);

		smt_set_service(SMT_SYSTEM_SETUP);	
		vw_system_setup_selected_menu(obj, sel_mm_num, sel_sm_num);
    	smt_set_service(SMT_MAIN_MENU);		
	}

	return FALSE;
}

static void _open_subwindow(NFWINDOW *parent, guint sel_num, guint xpos)
{
	NFOBJECT *subfixed;
	gint i;
//	guint sub_height = vw_mm_subm_height[sel_num];
	guint sub_width, sub_height;

	_create_sub_menu_img();
	_get_width_sub_img(sel_num, &sub_width, &sub_height);
	
	sub_window[sel_num] = (NFOBJECT*)nfui_nfwindow_new(parent, 
											xpos, subw_start_ypos, 
											sub_width, sub_height);

	nfui_nfwindow_set_title(sub_window[sel_num], "SUB MENU");

	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)sub_window[sel_num])->main_widget), FALSE);
	nfui_regi_post_event_callback(sub_window[sel_num], vw_subwindow_post_event_handler);


	subfixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(subfixed, xpos, sub_height);
	nfui_nfobject_show(subfixed);


	csm[sel_num] = cw_sm_new(sel_num, sub_width, sub_height);
	nfui_nfobject_set_size(csm[sel_num], sub_width, sub_height);
	nfui_nfobject_show(csm[sel_num]);
	nfui_regi_post_event_callback(csm[sel_num], vw_csm_post_event_handler);
	nfui_nffixed_put((NFFIXED*)subfixed, csm[sel_num], 0, 0);
	//cw_sm_update(csm);


	nfui_nfwindow_add((NFWINDOW*)sub_window[sel_num], subfixed);
	nfui_run_main_event_handler(sub_window[sel_num]);
	nfui_nfobject_show(sub_window[sel_num]);
	nfui_make_key_hierarchy(sub_window[sel_num]);
	for(i=0; i<=sel_num; i++)	
		nfui_set_key_focus((NFOBJECT*)csm[i],TRUE);
	
}


static gboolean 
post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type)	
	{
		case GDK_EXPOSE :
		{
			cw_mm_set_draw_segment(cmm, previous_sel_mm_num);
		}
		break;
		
		case GDK_DELETE :
			vsm_osd_refresh();
			break;
		default :
			break;
			
	}
		
	return FALSE;
	
}

static gboolean vw_system_setup_show_submenu_first_timer(void *data)
{
	nfui_nfobject_show(sub_window[0]);
	return FALSE;
}


static gboolean
vw_mm_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type)	
	{
		case NFEVENT_SETUP_MAINMENU_CHANGED_RELEASE :
		{
			nfui_unregi_semi_modal(sub_window[sel_mm_num]);
			nfui_nfobject_hide(sub_window[sel_mm_num]);
			nfui_page_close(PGID_SUB_SETUPMENU, sub_window[sel_mm_num]);

			previous_sel_mm_num = sel_mm_num;
			sel_mm_num = cw_mm_get_topmenu_number(cmm);
				
			guint xpos = vw_mm_subm_xpos[sel_mm_num] + subw_start_xpos;
			
			//gdk_flush();
			g_timeout_add(10, vw_system_setup_show_submenu_timer, xpos-30);
		}
		break;

		default : 
		break;
		
	}

	return FALSE;
}

static gboolean 
vw_mm_windows_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_PRESS) 
	{
		if (!ifn_is_in_rect(&rect_main, event->button.x_root, event->button.y_root)
				&& !ifn_is_in_rect(&rect_sub, event->button.x_root, event->button.y_root)) 
		{
			vw_destroy_system_setup();
		}
	}
	else if(event->type == GDK_DELETE)
	{       
		nfui_disable_semi_modal_mode();		
		nfui_page_close(PGID_SETUPMENU, obj);

		sel_mm_num = 0;
		sel_sm_num = 0;
		g_curwnd = 0;

        nfui_ui_unlock();
        
		gtk_main_quit();
	}
	else if(event->type == WND_PRE_CLOSE)
	{
		gint i;

        nfui_ui_lock();
        
		nfui_nfobject_hide(sub_window[sel_mm_num]);
		nfui_nfobject_hide((NFOBJECT*)g_curwnd);

		for(i = 0 ; i < 8 ; i++) 
			nfui_unregi_semi_modal(sub_window[i]);

		nfui_page_close(PGID_SUB_SETUPMENU, sub_window[sel_mm_num]);
		sel_mm_num = -1;
		nfui_unregi_semi_modal((NFOBJECT*)g_curwnd);
	}

	return FALSE;
}

void vw_open_system_setup(NFWINDOW *parent, guint xpos, guint ypos)
{
	NFOBJECT *window;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	guint x, y;
	
	gint i;
	guint icon_x_pos[8] = {25, 232, 439, 646, 853, 1060, 1267, 1474};

	subw_start_xpos = xpos;
	subw_start_ypos = ypos - 38;

	sel_mm_num = 0;

	// main menu
	window = (NFOBJECT*)nfui_nfwindow_new(parent, xpos, ypos, VMM_WIN_W, VMM_WIN_H);
	nfui_nfwindow_set_title(window, "SYSTEM SETUP");
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)window)->main_widget), FALSE);
	nfui_regi_post_event_callback(window, vw_mm_windows_event_handler);
	g_curwnd = window;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, VMM_WIN_W, VMM_WIN_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);

	cmm = cw_mm_new();
	cw_mm_icon_xpos(cmm, icon_x_pos);
	cw_mm_icon_ypos(cmm, 8);
	nfui_nfobject_set_size(cmm, 1649, 211);
	nfui_nfobject_show(cmm);
	nfui_regi_post_event_callback(cmm, vw_mm_post_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, cmm, 0, 0);

	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);

	nfui_regi_semi_modal(window);
	nfui_hook_evt_in_semi_modal(GDK_BUTTON_PRESS);
	nfui_enable_semi_modal_mode(window);

	gdk_window_get_geometry(((NFWINDOW *)window)->main_widget->window, 
							&rect_main.x, &rect_main.y,
							&rect_main.width, &rect_main.height, 0);
	nfui_make_key_hierarchy(window);
	nfui_page_open(PGID_SETUPMENU, window, ssm_get_cur_id(NULL));

	// submenu
	for( i = 0 ; i < 8 ; i++ )
	{
		_open_subwindow(g_curwnd, i, vw_mm_subm_xpos[i] + (subw_start_xpos - 30) );
		nfui_nfobject_hide(sub_window[i]);
	}

	gdk_window_get_geometry(((NFWINDOW *)sub_window[0])->main_widget->window, 
							&rect_sub.x, &rect_sub.y,
							&rect_sub.width, &rect_sub.height, 0);


	nfui_regi_semi_modal(sub_window[0]);
	nfui_nfobject_show(sub_window[0]);
	
	nfui_page_open(PGID_SUB_SETUPMENU, sub_window[0], ssm_get_cur_id(NULL));

	gtk_main();
}

void 
vw_destroy_system_setup()
{
	gint i;

	nfui_nfobject_hide(sub_window[sel_mm_num]);
	nfui_nfobject_hide((NFOBJECT*)g_curwnd);

	for(i = 0 ; i < 8 ; i++) {
		nfui_unregi_semi_modal(sub_window[i]);
		nfui_nfobject_destroy(sub_window[i]);
	}

	nfui_page_close(PGID_SUB_SETUPMENU, sub_window[sel_mm_num]);

	nfui_unregi_semi_modal((NFOBJECT*)g_curwnd);
	nfui_nfobject_destroy((NFOBJECT*)g_curwnd); 
}

