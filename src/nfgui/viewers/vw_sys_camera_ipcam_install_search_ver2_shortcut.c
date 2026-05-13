#include "nf_afx.h"
#include "scm.h"
#include "nvm.h"

#include "services/uxm.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflabel.h"

#include "vw_sys_camera_ipcam_install_search_ver2.h"
#include "vw_sys_camera_ipcam_install_search_ver2_shortcut.h"
#include "vw_sys_camera_ipcam_install_search_cam_login_popup.h"


enum {
    ISS_LOGIN = 0,
    ISS_DETAIL,

    ISS_OPT_MAX
};

#define ISS_SIZE_W							(300)
#define ISS_SIZE_H							(32)

#define ISS_BUTTON_POS_X					(12)
#define ISS_BUTTON_POS_Y					(15)
#define ISS_BUTTON_MARGIN					(15) 
#define ISS_BUTTON_SIZE_W					(ISS_SIZE_W - ISS_BUTTON_POS_X - ISS_BUTTON_MARGIN) 
#define ISS_BUTTON_SIZE_H					(40) 
#define ISS_BUTTON_GAP_2					(2)

static CAMERA_INFO_T *g_cam_info = NULL;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_btn[ISS_OPT_MAX];
static GdkPixbuf *g_menuBG[2][NFOBJECT_STATE_COUNT];
static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};



////////////////////////////////////////////////////////////////////
//
//
//

static void make_mac_bg_images()
{
	// menu button bg
	g_menuBG[0][NFOBJECT_STATE_NORMAL] = nf_ui_create_image_button_method(MK_IMG_CAM_INSTALL_SHORTCUT_N_BUTTON, ISS_BUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[0][NFOBJECT_STATE_PRELIGHT] = nf_ui_create_image_button_method(MK_IMG_CAM_INSTALL_SHORTCUT_O_BUTTON, ISS_BUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[0][NFOBJECT_STATE_ACTIVE] = nf_ui_create_image_button_method(MK_IMG_CAM_INSTALL_SHORTCUT_P_BUTTON, ISS_BUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[0][NFOBJECT_STATE_DISABLE] = nf_ui_create_image_button_method(MK_IMG_CAM_INSTALL_SHORTCUT_D_BUTTON, ISS_BUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

}

static gboolean post_login_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    
	switch(evt->type) 
	{
    	case GDK_BUTTON_RELEASE:
    	{
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    		
        	nfui_nfobject_hide(g_curwnd);

        	VW_OpenMode_Camera_Login_Popup_Open(g_curwnd, g_cam_info);
    	}
		break;

    	default:
    		break;
	}

	return FALSE;
	
}

static gboolean post_hide_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

	switch(evt->type) 
	{
    	case GDK_BUTTON_RELEASE:
    	{
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;    			
    		
        	nfui_nfobject_hide(g_curwnd);
    	}
		break;

    	default:
    		break;
	}

	return FALSE;
	
}

static NFOpenmodeCamInfo *_find_matched_info()
{
	NFOpenmodeDeviceList *dlist = NULL;
	NFOpenmodeCamInfo *info = NULL;
	gint i;

	dlist = nf_openmode_get_list();
	if (dlist == NULL) return FALSE;

    info = dlist->head;
    
	for (i = 0; i < dlist->entry_cnt; i++)
	{
	    if ((strcmp(info->hostname, g_cam_info->url) == 0) && (info->http_port == g_cam_info->port)) return info;
	    
        info = info->next;
	}

	return NULL;
}

static gboolean post_setup_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

	switch(evt->type) 
	{
    	case GDK_BUTTON_RELEASE:
    	{
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        	
    		NFOpenmodeCamInfo *info = NULL;

    		info = _find_matched_info();
    		if (info == NULL) return FALSE;

    		nf_openmode_set_preview(info->index, 0);
        	nfui_nfobject_hide(g_curwnd);

    		VW_Open_Cam_Ipcam_Setup_Popup(g_curwnd, info);
    		
    	}
		break;
    		
    	default:
    		break;
	}

	return FALSE;
	
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    
	switch (evt->type) 
	{
        case NFOUTEVT_BUTTON_PRESS :
        {          
        	nfui_nfobject_hide(obj);
        }
        break;
        
    	case GDK_DELETE:
    	{
    		g_curwnd = 0;
		}
		break;

	}
	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_Install_Search_Shortcut_Menu_Open(NFWINDOW *parent)
{
	NFOBJECT *wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
	
	gchar *strBtn[ISS_OPT_MAX] = {"LOG IN", "DETAIL"};
	guint pos_y = 0;
	gint i;
	gint win_h;


	/* window */
	win_h = ISS_SIZE_H + (ISS_BUTTON_SIZE_H * ISS_OPT_MAX);
	
	wnd = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, ISS_SIZE_W, win_h);
	g_curwnd = wnd;
	nfui_nfwindow_set_title(wnd, "CAMERA INSTALL SHORTCUT");
	nfui_regi_post_event_callback(wnd, post_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)wnd, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)wnd, GDK_BUTTON_PRESS, TRUE);

	make_mac_bg_images();

	/* fixed */
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, ISS_SIZE_W, win_h);
	nfui_regi_post_event_callback(main_fixed, post_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	pos_y = ISS_BUTTON_POS_Y;

	for(i = 0; i < ISS_OPT_MAX; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[0], strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, ISS_BUTTON_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)g_font_color);
		nfui_nfobject_set_size(obj, ISS_BUTTON_SIZE_W, ISS_BUTTON_SIZE_H);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, ISS_BUTTON_POS_X, pos_y);
		nfui_nfobject_show(obj);
		g_btn[i] = obj;
			
		if (i == ISS_LOGIN)
			nfui_regi_post_event_callback(obj, post_login_btn_handler);
			
		if (i == ISS_DETAIL)
			nfui_regi_post_event_callback(obj, post_setup_btn_handler);
			
		pos_y += (ISS_BUTTON_SIZE_H + ISS_BUTTON_GAP_2);
	}

	nfui_nfwindow_add((NFWINDOW*)g_curwnd, main_fixed);
	nfui_run_main_event_handler(g_curwnd);
	nfui_nfobject_hide(g_curwnd);

	return TRUE;
}

int VW_Destroy_Install_Search_Shortcut_Menu()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

int VW_Show_Install_Search_Shortcut_Menu(CAMERA_INFO_T *cam_info, gint x, gint y)
{
	if (!g_curwnd) return 0;

	g_cam_info = cam_info;

	if (g_cam_info->state != OPENMODE_CAM_STATE_LOGIN_FAIL) {
	    nfui_nfobject_disable(g_btn[ISS_LOGIN]);
	}
	else {
	    nfui_nfobject_enable(g_btn[ISS_LOGIN]);
	}
	
	nfui_nfobject_move(g_curwnd, (guint)x, (guint)y);

	nfui_nfobject_show(g_curwnd);
	nfui_make_key_hierarchy(g_curwnd);
	
	return 0;
}

