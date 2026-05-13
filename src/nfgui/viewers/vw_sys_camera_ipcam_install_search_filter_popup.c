#include "nf_afx.h"
#include "scm.h"

#include "services/uxm.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "modules/ocam.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfcheckbutton.h"
#include "vw_vkeyboard.h"

#include "vw_sys_camera_ipcam_install_search_filter_popup.h"


#define FS_SIZE_W							(1000)
#define FS_SIZE_H							(400)
#define FS_POS_X							((DISPLAY_ACTIVE_WIDTH - FS_SIZE_W) / 2)
#define FS_POS_Y							((DISPLAY_ACTIVE_HEIGHT - FS_SIZE_H) / 2)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_andor_radio[2];
static NFOBJECT *g_hostaddr_chk;
static NFOBJECT *g_model_chk;
static NFOBJECT *g_hidden_chk;
static NFOBJECT *g_assigned_chk;
static NFOBJECT *g_hostaddr_text;
static NFOBJECT *g_model_text;
static NFOBJECT *g_filter_chk;

static int _get_filter_oper()
{
	if (nfui_radio_button_get_toggled(g_andor_radio[0] == TRUE))
		return 0;
	else if (nfui_radio_button_get_toggled(g_andor_radio[1] == TRUE))
		return 1;

	return -1;
}

static int _is_hostaddr_excluding()
{
    gboolean active;
	active = nfui_check_button_get_active((NFCHECKBUTTON*)g_hostaddr_chk);

	if (active) return 1;
	return 0;
}

static int _is_model_excluding()
{
    gboolean active;
	active = nfui_check_button_get_active((NFCHECKBUTTON*)g_model_chk);

	if (active) return 1;
	return 0;
}

static int _is_hidden_excluding()
{
	return 0;
    gboolean active;
	active = nfui_check_button_get_active((NFCHECKBUTTON*)g_hidden_chk);

	if (active) return 1;
	return 0;
}

static int _is_assigned_excluding()
{
    gboolean active;
	active = nfui_check_button_get_active((NFCHECKBUTTON*)g_assigned_chk);

	if (active) return 1;
	return 0;
}

static int _is_prevfilter_on()
{
    gboolean active;
	active = nfui_check_button_get_active((NFCHECKBUTTON*)g_filter_chk);

	if (active) return 1;
	return 0;
}

static gboolean post_prev_filter_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		if (state) {
			nfui_nfobject_enable((NFBUTTON*)g_andor_radio[0]);
			nfui_nfobject_enable((NFBUTTON*)g_andor_radio[1]);

			nfui_radio_button_set_toggled(g_andor_radio[0], TRUE);
			
			for (i = 0; i < 2; i++)
        		nfui_signal_emit(g_andor_radio[i], GDK_EXPOSE, TRUE);
		}
		else {
			nfui_nfobject_disable((NFBUTTON*)g_andor_radio[0]);
			nfui_nfobject_disable((NFBUTTON*)g_andor_radio[1]);
			
			for (i = 0; i < 2; i++)
        		nfui_signal_emit(g_andor_radio[i], GDK_EXPOSE, TRUE);
		}

	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
	}

	return FALSE;
}
static gboolean post_text_field_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gint max_string_size = 64;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{
				return FALSE;
	  	   	}

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_NORMAL);

		if(strTemp)
		{
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
    		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static int _apply_filter()
{
	gboolean host_ex, model_ex;
	gboolean hide_ex, assg_ex;
	gboolean prev_filter;
	int andor;

	gchar *hostaddr;
	gchar *model;
	OCAM_FILTER_CMB cmb;

	prev_filter = _is_prevfilter_on();
	if (prev_filter) {
		andor = _get_filter_oper();
		if (andor = 0) cmb = OCAM_C_AND;
		else cmb = OCAM_C_OR; 
	}
	else {
		cmb = OCAM_C_RST;
	}

	host_ex = _is_hostaddr_excluding();
	model_ex = _is_model_excluding();
	hide_ex = _is_hidden_excluding();
	assg_ex = _is_assigned_excluding();

	hostaddr = nfui_nflabel_get_text((NFLABEL*)g_hostaddr_text);
	model = nfui_nflabel_get_text((NFLABEL*)g_model_text);

	if (strlen(hostaddr) > 0) {
		if (host_ex)
			ocam_filter(OCAM_F_HOSTADDR, "!", hostaddr, cmb);
		else
			ocam_filter(OCAM_F_HOSTADDR, "~", hostaddr, cmb);
	}

	if (strlen(model) > 0) {
		if (model_ex)
			ocam_filter(OCAM_F_MODEL, "!", model, cmb);
		else
			ocam_filter(OCAM_F_MODEL, "~", model, cmb);
	}

	if (hide_ex)
		ocam_filter(OCAM_F_HIDDEN, NULL, NULL, cmb);

	if (assg_ex)
		ocam_filter(OCAM_F_ASSIGNED, NULL, NULL, cmb);
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;		
		
		_apply_filter();
	

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	
	return FALSE;
}

////////////////////////////////////////////////////////////////////
//
//
//

gint VW_Create_Camera_Filter_Popup_Open(NFWINDOW *parent)
{
	NFOBJECT *wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
	
	guint pos_x, pos_y = 0;
	gint size_w, size_h;
	gint i;

	GSList *slist = NULL;

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	/* window */
	wnd = (NFOBJECT*)nftool_create_popup_window(parent, FS_POS_X, FS_POS_Y, FS_SIZE_W, FS_SIZE_H, "CAMERA LIST FILTER", FALSE);
	nfui_regi_post_event_callback(wnd, post_window_event_cb);
	g_curwnd = wnd;

	/* fixed */
	main_fixed = ((NFWINDOW*)wnd)->child;

    pos_x = 20;
    pos_y = 60;

/*
    obj = nfui_nflabel_new_with_pango_font("IP ADDRESS RANGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 250, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 250 + 5;

    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_text_field_event_handler);

    pos_x += 300 + 5;

    obj = nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 30, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 30 + 5;

    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_text_field_event_handler);

    pos_x = 20;
    pos_y += 40 + 5;
*/
    obj = nfui_nflabel_new_with_pango_font("IP / HOST NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 250, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 250 + 5;

    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 480, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_text_field_event_handler);
	g_hostaddr_text = obj;

    pos_x += 480 + 15;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (40-size_h)/2);
//    nfui_regi_post_event_callback(obj, post_buzzer_all_event_handler);
	g_hostaddr_chk = obj;

    pos_x += size_w + 5;
    
    obj = nfui_nflabel_new_with_pango_font("EXCLUDING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 150, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x = 20;
    pos_y += 40 + 5;

    obj = nfui_nflabel_new_with_pango_font("MODEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 250, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 250 + 5;

    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 480, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_text_field_event_handler);
	g_model_text = obj;

    pos_x += 480 + 15;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (40-size_h)/2);
//    nfui_regi_post_event_callback(obj, post_buzzer_all_event_handler);
	g_model_chk = obj;

    pos_x += size_w + 5;
    
    obj = nfui_nflabel_new_with_pango_font("EXCLUDING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 150, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

#if 0
    pos_x = 20;
    pos_y += 40 + 40;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (40-size_h)/2);
//    nfui_regi_post_event_callback(obj, post_buzzer_all_event_handler);
	g_hidden_chk = obj;

    pos_x += size_w + 5;
    
    obj = nfui_nflabel_new_with_pango_font("EXCEPT HIDDEN CAMERA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
#endif

    pos_x = 20;
    pos_y += 40 + 5;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (40-size_h)/2);
	g_assigned_chk = obj;

    pos_x += size_w + 5;
    
    obj = nfui_nflabel_new_with_pango_font("HIDE ASSIGNED CAMERA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x = 20;
    pos_y += 40 + 5;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_prev_filter_event_handler);
	g_filter_chk = obj;

    pos_x += size_w + 5;
    
    obj = nfui_nflabel_new_with_pango_font("KEEP PREVIOUS FILTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 400 + 5;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    for (i = 0; i < 2; i++)
    {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
//		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
		g_andor_radio[i] = obj;
        
		if(i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		} else {
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

		nfui_nfobject_disable((NFBUTTON*)obj);

		pos_x += size_w + 5;

        if (i == 0)
            obj = nfui_nflabel_new_with_pango_font("AND", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        else
            obj = nfui_nflabel_new_with_pango_font("OR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 100, 40);
//        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

		pos_x += 100 + 10;
	}

    
	obj = nftool_normal_button_create_type1("APPLY", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FS_SIZE_W/2-174-4, FS_SIZE_H-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FS_SIZE_W/2+4, FS_SIZE_H-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(wnd);
	nfui_make_key_hierarchy(wnd);
	nfui_set_key_focus(obj, TRUE);	

	nfui_page_open(PGID_CAMERA_INSTALL_CAM_LIST_FILTER_POPUP, wnd, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CAMERA_INSTALL_CAM_LIST_FILTER_POPUP, wnd);

	return 0;
}

