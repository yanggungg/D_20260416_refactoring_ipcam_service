#include "nf_afx.h"

#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/nf_ui_color.h"
#include "../support/color.h"
#include "../support/util.h"

#include "../tools/nf_ui_tool.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfcombobox.h"
#include "../viewers/objects/nfimage.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"

#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"

#include "vw.h"
#include "vw_sys_camera_install_mode.h"
#include "vw_install_mode_warning_popup.h"
#include "vw_passage_popup.h"


#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

#define	CD_LABEL_LEFT			(3)
#define	CD_LABEL_TOP			(0)
#define	CD_LABEL_HEIGHT			(40)

#define STR_INSTALL_CCTV_MODE		"CCTV MODE (Recommended)"
#define STR_INSTALL_OPEN_MODE		"OPEN MODE (Advanced)"
#define STR_INSTALL_DUAL_LAN_MODE	"DUAL NETWORK MODE (Configurable LAN2 Network)"

#define HELP_OPEN_DESC	        "The recorder can scan LAN1 network for cameras. All Network ports on the recorder (WAN, LAN & POE) are ports\non the existing LAN network."
#define HELP_DUAL_NET_DESC	    "The recorder can scan the existing LAN1 network for cameras as well as creating its own LAN2 network on\na customised IP range. In this mode the recorder can distribute up to 253 DHCP IP addresses to any IP equipment.\nTo find this menu navigate to Menu > System Setup > Network > IP setup > LAN2v4."

#define HELP_BTN_OBJ_IDX		 (6)
#define HELP_BTN_X               (4)

#if defined(GUI_32CH_SUPPORT)
static gchar* g_install_mode_type[] = {STR_INSTALL_OPEN_MODE, STR_INSTALL_DUAL_LAN_MODE};
#else
static gchar* g_install_mode_type[] = {STR_INSTALL_CCTV_MODE, STR_INSTALL_OPEN_MODE, STR_INSTALL_DUAL_LAN_MODE};
#endif

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_mode_obj;
static NFOBJECT *g_sample_fixed;
static NFOBJECT *g_sample_obj[3][10] = {0, };
static NFOBJECT *g_help_btn_obj;


static void _get_install_mode_value(gchar *str_mode, gboolean *install_mode, gboolean *use_dual_lan)
{
	if (!strcmp(str_mode, STR_INSTALL_DUAL_LAN_MODE)) {
		*install_mode = TRUE;
		*use_dual_lan = TRUE;
	}
	else if (!strcmp(str_mode, STR_INSTALL_OPEN_MODE)) {
		*install_mode = TRUE;
		*use_dual_lan = FALSE;
	}
	else {
		*install_mode = FALSE;
		*use_dual_lan = FALSE;
	}

	g_message("%s, %d, str_mode:%s, install_mode:%d, use_dual_lan:%d", __FUNCTION__, __LINE__, str_mode, *install_mode, *use_dual_lan);
}

static gchar* _get_install_mode_string(gboolean install_mode, gboolean use_dual_lan)
{
	gchar *str_mode = STR_INSTALL_CCTV_MODE;

	if (install_mode && use_dual_lan) str_mode = STR_INSTALL_DUAL_LAN_MODE;
	else if (install_mode && !use_dual_lan) str_mode = STR_INSTALL_OPEN_MODE;
	else str_mode = STR_INSTALL_CCTV_MODE;

	g_message("%s, %d, install_mode:%d, use_dual_lan:%d, str_mode:%s, ", __FUNCTION__, __LINE__, install_mode, use_dual_lan, str_mode);
	return str_mode;
}

static gboolean post_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		nfui_signal_emit(g_sample_fixed, GDK_EXPOSE, FALSE);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gboolean pre_install_mode = DAL_get_cam_install_mode();
		gboolean pre_use_dual_lan = DAL_get_cam_install_use_dual_lan();
		gchar *str_install_mode = nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj);
		gboolean post_install_mode = FALSE, post_use_dual_lan = FALSE;

		_get_install_mode_value(str_install_mode, &post_install_mode, &post_use_dual_lan);

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (pre_install_mode == post_install_mode && pre_use_dual_lan == post_use_dual_lan) return FALSE;

		nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_mode_obj, _get_install_mode_string(pre_install_mode, pre_use_dual_lan));
		nfui_signal_emit(g_mode_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sample_fixed, GDK_EXPOSE, FALSE);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gboolean pre_install_mode = DAL_get_cam_install_mode();
		gboolean pre_use_dual_lan = DAL_get_cam_install_use_dual_lan();
		gchar *str_install_mode = nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj);
		gboolean post_install_mode = FALSE, post_use_dual_lan = FALSE;
		mb_type ret;

		_get_install_mode_value(str_install_mode, &post_install_mode, &post_use_dual_lan);

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (pre_install_mode == post_install_mode && pre_use_dual_lan == post_use_dual_lan) return FALSE;

		ret = VW_Open_InstallMode_Warning_Popup(g_curwnd, post_install_mode && !post_use_dual_lan);

		if (ret == NFTOOL_MB_OK)
		{
			if (post_install_mode == 1) nf_ipcam_opmode_change();
		
			DAL_set_cam_install_mode(post_install_mode);
			DAL_set_cam_install_use_dual_lan(post_use_dual_lan);
			DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);	

			scm_reboot_system(RR_INSTALLMODE_CHANGE, 0);
		}
		else
		{
			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_mode_obj, _get_install_mode_string(pre_install_mode, pre_use_dual_lan));
			nfui_signal_emit(g_mode_obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_sample_fixed, GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		InstallMode_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_help_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_RELEASE || kpid == KEYPAD_ENTER)
    {
        NFOBJECT *top;
        guint x, y;
        PARAGRAPH_STR *para;
        gint i;
		
		gchar *str_mode = nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj);
		gboolean install_mode = FALSE, use_dual_lan = FALSE;

		_get_install_mode_value(str_mode, &install_mode, &use_dual_lan);

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        x += top->x;
        y += top->y + obj->height + 2;

        para = imalloc(sizeof(PARAGRAPH_STR));
        
		if(install_mode == 1 && use_dual_lan == 1){
        	para->intro[0] = g_strdup(HELP_DUAL_NET_DESC);
		} else if(install_mode == 1 && use_dual_lan == 0) {
        	para->intro[0] = g_strdup(HELP_OPEN_DESC);
		}
        para->intro_cnt = 1;
        para->intro_type = BULLET_BLANK;

        vw_passage_popup_open(g_curwnd, x, y, DIR_TOP_RIGHT, &para, 1);

        if (para->intro[0]) g_free(para->intro[0]);

        ifree(para);
    }

    return FALSE;
}

static gboolean post_sample_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{     
		gchar *str_mode = nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj);
		gboolean install_mode = FALSE, use_dual_lan = FALSE;
        gint gap_x, gap_y;
		gint i;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);

		_get_install_mode_value(str_mode, &install_mode, &use_dual_lan);

		if (install_mode == 0) {
	        nfutil_draw_image(drawable, gc, IMG_INSTALL_CCTV_MODE, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);
			nfui_nfobject_hide(g_help_btn_obj);
		}
		else if (install_mode == 1 && use_dual_lan == 0) nfutil_draw_image(drawable, gc, IMG_INSTALL_OPEN_MODE, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);    
		else if (install_mode == 1 && use_dual_lan == 1) nfutil_draw_image(drawable, gc, IMG_INSTALL_DUAL_NET_MODE, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);    

 		nfui_nfobject_gc_unref(gc);		


 		for (i = 0; i < 10; i++)
 		{		
			if (install_mode == 0) 
			{
				if (g_sample_obj[0][i]) nfui_nfobject_show(g_sample_obj[0][i]);
				if (g_sample_obj[1][i]) nfui_nfobject_hide(g_sample_obj[1][i]);
				if (g_sample_obj[2][i]) nfui_nfobject_hide(g_sample_obj[2][i]);
			}
			else if (install_mode == 1 && use_dual_lan == 0)
			{
				if (g_sample_obj[0][i]) nfui_nfobject_hide(g_sample_obj[0][i]);
				if (g_sample_obj[1][i]) nfui_nfobject_show(g_sample_obj[1][i]);
				if (g_sample_obj[2][i]) nfui_nfobject_hide(g_sample_obj[2][i]);
			}
			else if (install_mode == 1 && use_dual_lan == 1)
			{
				if (g_sample_obj[0][i]) nfui_nfobject_hide(g_sample_obj[0][i]);
				if (g_sample_obj[1][i]) nfui_nfobject_hide(g_sample_obj[1][i]);
				if (g_sample_obj[2][i]) nfui_nfobject_show(g_sample_obj[2][i]);
			}

			if (g_sample_obj[0][i]) nfui_signal_emit(g_sample_obj[0][i], GDK_EXPOSE, TRUE);
			if (g_sample_obj[1][i]) nfui_signal_emit(g_sample_obj[1][i], GDK_EXPOSE, TRUE);
			if (g_sample_obj[2][i]) nfui_signal_emit(g_sample_obj[2][i], GDK_EXPOSE, TRUE);
 		}

		 nfui_signal_emit(g_help_btn_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) 
	{

	}

	return FALSE;
}


static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}



void init_InstallMode_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *sub_fixed;	
	NFOBJECT *obj;

	gboolean install_mode = DAL_get_cam_install_mode();
	gboolean use_dual_lan = DAL_get_cam_install_use_dual_lan();

	gint pos_x, pos_y;
	gint sub_fixed_x, sub_fixed_y;
	gint obj_cnt = 0;

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	pos_x = CD_LABEL_LEFT;
	pos_y = CD_LABEL_TOP;	

	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "INSTALLATION MODE");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_y += (CD_LABEL_HEIGHT+21);	
	
	obj = nfui_combobox_new(g_install_mode_type, ARRAY_LEN(g_install_mode_type), 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);		
	nfui_nfobject_set_size(obj, 560, CD_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+20, pos_y);
	nfui_regi_post_event_callback(obj, post_mode_combo_event_handler);	
	g_mode_obj = obj;

	nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, _get_install_mode_string(install_mode, use_dual_lan));

	pos_y = MENU_V_INNER_H;	
	pos_y -= (3+687+21+CD_LABEL_HEIGHT);	

	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "EXAMPLE NETWORK MAP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	sub_fixed_x = 3;
	sub_fixed_y = 213;

	sub_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(sub_fixed, 1506, 687);
	nfui_nfobject_show(sub_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, sub_fixed, sub_fixed_x, sub_fixed_y);
	nfui_regi_post_event_callback(sub_fixed, post_sample_fixed_event_handler);
	g_sample_fixed = sub_fixed;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LAN", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(758));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 46, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 403+101+162-sub_fixed_x, 286-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PROTECTED NETWORK", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(759));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 211, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, MENU_V_INNER_W-422-211-sub_fixed_x, 286-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INTERNET", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 101, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 403-sub_fixed_x, 458-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GATEWAY /\nSWITCH", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 101, 44);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, MENU_V_INNER_W-711-101-sub_fixed_x, 286+22+105-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PC", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 92, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 289-sub_fixed_x, 458+22+190-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PC", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 92, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 289+92+225-sub_fixed_x, 458+22+190-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEDICATED SWITCH", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 227, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, MENU_V_INNER_W-379-227-sub_fixed_x, MENU_V_INNER_H-158-22-sub_fixed_y);
	g_sample_obj[0][obj_cnt++] = obj;
	
	obj_cnt = 0;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LAN / WAN  (OPEN NETWORK)", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(758));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 286, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 521-sub_fixed_x, 286-sub_fixed_y);
	g_sample_obj[1][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INTERNET", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 149, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 472-sub_fixed_x, 286+22+105-sub_fixed_y);
	g_sample_obj[1][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GATEWAY", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 89, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, MENU_V_INNER_W-723-89-sub_fixed_x, 286+22+105-sub_fixed_y);
	g_sample_obj[1][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PC", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 92, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 210-sub_fixed_x, 413+22+240-sub_fixed_y);
	g_sample_obj[1][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PC", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 92, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 209+92+300-sub_fixed_x, 413+22+235-sub_fixed_y);
	g_sample_obj[1][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GENERAL SWITCH", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 227, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 209+92+300+92+213-sub_fixed_x, MENU_V_INNER_H-158-22-sub_fixed_y);
	g_sample_obj[1][obj_cnt++] = obj;
	
	obj_cnt = 0;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LAN", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(758));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 100, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 630-sub_fixed_x, 286-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PROTECTED NETWORK", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(780));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 220, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 882-sub_fixed_x, 286-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INTERNET", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 149, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 375-sub_fixed_x, 455-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GATEWAY", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 89, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, MENU_V_INNER_W-726-163-sub_fixed_x, 288+55-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PC", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 92, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 285-sub_fixed_x, 413+22+238-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PC", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 92, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 209+92+405-sub_fixed_x, 288+55-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GENERAL SWITCH", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 170, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 209+92+300+92+290-sub_fixed_x, MENU_V_INNER_H-542-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GENERAL SWITCH", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 227, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 209+92+270-sub_fixed_x, MENU_V_INNER_H-178-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GENERAL SWITCH", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(121));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 227, 22);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)sub_fixed, obj, 209+92+300+92+213-sub_fixed_x, MENU_V_INNER_H-178-sub_fixed_y);
	g_sample_obj[2][obj_cnt++] = obj;

    obj = nftool_normal_button_create_subtab_type1("HELP", 95);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)sub_fixed, obj, HELP_BTN_X, MENU_V_INNER_H-50-sub_fixed_y);
    nfui_regi_post_event_callback(obj, post_help_button_event_handler);
	g_help_btn_obj = obj;

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	
	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	
	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);
	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean InstallMode_tab_in_handler()
{

	return FALSE;
}

gboolean InstallMode_tab_out_handler()
{
	gboolean pre_install_mode = DAL_get_cam_install_mode();
	gboolean pre_use_dual_lan = DAL_get_cam_install_use_dual_lan();
	gchar *str_install_mode = nfui_combobox_get_value((NFCOMBOBOX*)g_mode_obj);
	gboolean post_install_mode = FALSE, post_use_dual_lan = FALSE;
	mb_type ret;

	_get_install_mode_value(str_install_mode, &post_install_mode, &post_use_dual_lan);

	if (pre_install_mode == post_install_mode && pre_use_dual_lan == post_use_dual_lan) return FALSE;
		
	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
		ret = VW_Open_InstallMode_Warning_Popup(g_curwnd, post_install_mode && !post_use_dual_lan);

		if (ret == NFTOOL_MB_OK)
		{
			if (post_install_mode == 1) nf_ipcam_opmode_change();
		
			DAL_set_cam_install_mode(post_install_mode);
			DAL_set_cam_install_use_dual_lan(post_use_dual_lan);
			DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);	

			scm_reboot_system(RR_INSTALLMODE_CHANGE, 0);

			return FALSE;
		}
	}
	
	nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_mode_obj, _get_install_mode_string(pre_install_mode, pre_use_dual_lan));
	return FALSE;
}

