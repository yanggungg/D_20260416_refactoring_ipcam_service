
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfvklabel.h"

#include "vw_pin_check_popup.h"
#include "ix_mem.h"
#include "uxm.h"


#define WIN_SIZE_WID				(400)
#define WIN_SIZE_HEI				(600)

#define PIN_LABEL_W					(42)
#define PIN_LABEL_H					(42)

#define BTN_SIZE_W					(80)
#define BTN_SIZE_H					(52)
#define BTN_COL_GAP					(16)
#define BTN_ROW_GAP					(16)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_number_obj[10];
static NFOBJECT *g_pin_label[8];
static NFOBJECT *g_help_label;

static gchar g_pin_number[16];
static gint g_pin_len;

static gchar g_input_pin_number[16];
static gint g_cursor_posion = 0;

static gint g_retVal = 0;


static gint _get_input_pin_number(gchar *pin_number)
{
	gchar *p;
	gint i;
	
	for (i = 0; i < g_pin_len; i++)
	{
		p = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_pin_label[i]);
		pin_number[i] = p[0];
	}
	return 0;
}

static gint _reset_input_pin_number()
{
	gint i;

	nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[0], 1);

	for (i = 0; i < g_pin_len; i++)
	{
		nfui_nfvklabel_set_string((NFVKLABEL*)g_pin_label[i], "");
		if (i != 0) nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[i], 0);
		nfui_signal_emit(g_pin_label[i], GDK_EXPOSE, TRUE);
	}
	return 0;
}

static gboolean post_number_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		NFOBJECT *top;
		gchar strBuf[8];
		gint i;

		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < 10; i++) {
			if (g_number_obj[i] == obj) break;
		}
		if (i == 10) return FALSE;

		memset(strBuf, 0x00, sizeof(strBuf));
		sprintf(strBuf, "%d", i);
		nfui_nfvklabel_set_string((NFVKLABEL*)g_pin_label[g_cursor_posion], strBuf);

		if (g_cursor_posion != g_pin_len - 1) 
		{
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_cursor_posion], 0);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_cursor_posion+1], 1);
			nfui_signal_emit(g_pin_label[g_cursor_posion], GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_pin_label[g_cursor_posion+1], GDK_EXPOSE, TRUE);
			g_cursor_posion++;
		}
		else 
		{
			g_cursor_posion = 0;
			memset(g_input_pin_number, 0x00, sizeof(g_input_pin_number));
			_get_input_pin_number(g_input_pin_number);

			if (strcmp(g_pin_number, g_input_pin_number) == 0) {
				g_retVal = 1;
				top = nfui_nfobject_get_top(obj);
				nfui_nfobject_destroy(top);
			}
			else {
				_reset_input_pin_number();
				nftool_mbox(g_curwnd, "WARNING", "Incorrect pin number.", NFTOOL_MB_OK);
			}
		}
	}

	return FALSE;
}

static gboolean post_pin_switch_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_retVal = -1;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}


static gboolean post_delete_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (g_cursor_posion == 0) return FALSE;

		nfui_nfvklabel_set_string((NFVKLABEL*)g_pin_label[g_cursor_posion-1], "");
		nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_cursor_posion], 0);
		nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_cursor_posion-1], 1);
		nfui_signal_emit(g_pin_label[g_cursor_posion], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_pin_label[g_cursor_posion-1], GDK_EXPOSE, TRUE);
		g_cursor_posion--;
	}

	return FALSE;
}

static gboolean post_forgot_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_retVal = -1;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
#if defined(_SUPPORT_LOGIN_AUTOHIDE)
		vw_autohide_unset_obj(g_curwnd);
#endif
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_pin_check_popup_open(NFWINDOW *parent, gchar *title, gint usr_idx)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj;

	GdkPixbuf *btn_pbuf1[NFOBJECT_STATE_COUNT];
	GdkPixbuf *btn_pbuf2[NFOBJECT_STATE_COUNT];

	GdkPixbuf *switch_icon[NFOBJECT_STATE_COUNT];
	GdkPixbuf *backspace_icon[NFOBJECT_STATE_COUNT];

	guint btn_bg[] = {COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707), COLOR_IDX(707)}; 
	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};

	ICONPOSITION icon_pos;

	gint btn_w, btn_h;
	gint pos_x, pos_y;	
	guint i, j;

	gchar strBuf[64];

	UserManageData userdata;


	btn_pbuf1[0] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_N_BUTTON, BTN_SIZE_W, IMG_N_POPUP52_BTN_LEFT, IMG_N_POPUP52_BTN_MIDDLE, IMG_N_POPUP52_BTN_RIGHT);
	btn_pbuf1[1] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_O_BUTTON, BTN_SIZE_W, IMG_O_POPUP52_BTN_LEFT, IMG_O_POPUP52_BTN_MIDDLE, IMG_O_POPUP52_BTN_RIGHT);
	btn_pbuf1[2] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_P_BUTTON, BTN_SIZE_W, IMG_P_POPUP52_BTN_LEFT, IMG_P_POPUP52_BTN_MIDDLE, IMG_P_POPUP52_BTN_RIGHT);
	btn_pbuf1[3] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_D_BUTTON, BTN_SIZE_W, IMG_D_POPUP52_BTN_LEFT, IMG_D_POPUP52_BTN_MIDDLE, IMG_D_POPUP52_BTN_RIGHT);

	btn_pbuf2[0] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_N_BUTTON2, BTN_SIZE_W, IMG_N_POPUP52_BTN2_LEFT, IMG_N_POPUP52_BTN2_MIDDLE, IMG_N_POPUP52_BTN2_RIGHT);
	btn_pbuf2[1] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_O_BUTTON, BTN_SIZE_W, IMG_O_POPUP52_BTN_LEFT, IMG_O_POPUP52_BTN_MIDDLE, IMG_O_POPUP52_BTN_RIGHT);
	btn_pbuf2[2] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_P_BUTTON, BTN_SIZE_W, IMG_P_POPUP52_BTN_LEFT, IMG_P_POPUP52_BTN_MIDDLE, IMG_P_POPUP52_BTN_RIGHT);
	btn_pbuf2[3] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_D_BUTTON, BTN_SIZE_W, IMG_D_POPUP52_BTN_LEFT, IMG_D_POPUP52_BTN_MIDDLE, IMG_D_POPUP52_BTN_RIGHT);

	switch_icon[0] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);
	switch_icon[1] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);
	switch_icon[2] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);
	switch_icon[3] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);

	backspace_icon[0] = nfui_get_image_from_file((IMG_N_KEY_BACK), NULL);
	backspace_icon[1] = nfui_get_image_from_file((IMG_O_KEY_BACK), NULL);
	backspace_icon[2] = nfui_get_image_from_file((IMG_P_KEY_BACK), NULL);
	backspace_icon[3] = nfui_get_image_from_file((IMG_D_KEY_BACK), NULL);


	if (ivsc.dfunc.pin.digit > 6) g_assert(0);


	memset(&userdata, 0x00, sizeof(UserManageData));
	DAL_get_userManage_data(&userdata, usr_idx);
	memset(g_pin_number, 0x00, sizeof(g_pin_number));
	strcpy(g_pin_number, userdata.pin_number);
	g_pin_len = ivsc.dfunc.pin.digit;

	memset(g_input_pin_number, 0x00, sizeof(g_input_pin_number));	
	g_cursor_posion = 0;
	g_retVal = 0;

// <---- WINDOW
	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, (DISPLAY_ACTIVE_WIDTH-WIN_SIZE_WID)/2, (DISPLAY_ACTIVE_HEIGHT-WIN_SIZE_HEI)/2, WIN_SIZE_WID, WIN_SIZE_HEI, title, FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_nfobject_show(main_fixed);
	g_curwnd = main_wnd;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(userdata.id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));		
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, WIN_SIZE_WID-48, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 24, 62);
	nfui_nfobject_show(obj);

	obj = nfui_nflabel_new_with_pango_font("Please enter pin number.", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(340));
	nfui_nfobject_set_size(obj, WIN_SIZE_WID- 32, 40);
	nfui_nflabel_set_align(obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 16, 114);
	nfui_nfobject_show(obj);
	g_help_label = obj;

	for (i = 0; i < g_pin_len; i++) 
	{
		obj = (NFOBJECT*)nfui_nfvklabel_new_str("", 1);
		nfui_nfvklabel_set_invisible((NFVKLABEL*)obj, TRUE);
		nfui_nfvklabel_set_pango_font((NFVKLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
		nfui_nfvklabel_set_align((NFVKLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, PIN_LABEL_W, PIN_LABEL_H);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, 40+PIN_LABEL_W*i+(WIN_SIZE_WID-80-PIN_LABEL_W*g_pin_len)/(g_pin_len-1)*i, 160);
		g_pin_label[i] = obj;

		if (i != 0) {
			nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 0);
		}
	}

	for (i = 0; i < 3; i++) 
	{
		for (j = 0; j < 3; j++) 
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			g_sprintf(strBuf, "%d", i*3+j+1);
			obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf1, strBuf);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
			nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
			nfui_nfobject_set_size(obj, BTN_SIZE_W, BTN_SIZE_H);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, (WIN_SIZE_WID-BTN_SIZE_W*3-BTN_COL_GAP*2)/2+(BTN_SIZE_W+BTN_COL_GAP)*j, 240+(BTN_SIZE_H+BTN_ROW_GAP)*i);
			nfui_regi_post_event_callback(obj, post_number_button_event_handler);
			g_number_obj[i*3+j+1] = obj;
		}
	}

	memset(&icon_pos, 0x00, sizeof(ICONPOSITION));
	icon_pos.x = (BTN_SIZE_W-gdk_pixbuf_get_width(switch_icon[0]))/2;
	icon_pos.y = (BTN_SIZE_H-gdk_pixbuf_get_height(switch_icon[0]))/2;

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf2, "");
	nfui_nfbutton_set_icon_image((NFBUTTON*)obj, switch_icon);
	nfui_nfbutton_set_icon_position(NF_BUTTON(obj), icon_pos);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
	nfui_nfobject_set_size(obj, BTN_SIZE_W, BTN_SIZE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (main_fixed->width-BTN_SIZE_W*3-BTN_COL_GAP*2)/2, 240+(BTN_SIZE_H+BTN_ROW_GAP)*3);
	nfui_regi_post_event_callback(obj, post_pin_switch_button_event_handler);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf2, "0");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
	nfui_nfobject_set_size(obj, BTN_SIZE_W, BTN_SIZE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (WIN_SIZE_WID-BTN_SIZE_W*3-BTN_COL_GAP*2)/2+(BTN_SIZE_W+BTN_COL_GAP), 240+(BTN_SIZE_H+BTN_ROW_GAP)*3);
	nfui_regi_post_event_callback(obj, post_number_button_event_handler);
	g_number_obj[0] = obj;

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf2, "");
	nfui_nfbutton_set_icon_image((NFBUTTON*)obj, backspace_icon);
	nfui_nfbutton_set_icon_position(NF_BUTTON(obj), icon_pos);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
	nfui_nfobject_set_size(obj, BTN_SIZE_W, BTN_SIZE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (WIN_SIZE_WID-BTN_SIZE_W*3-BTN_COL_GAP*2)/2+(BTN_SIZE_W+BTN_COL_GAP)*2, 240+(BTN_SIZE_H+BTN_ROW_GAP)*3);
	nfui_regi_post_event_callback(obj, post_delete_button_event_handler);

	
// <---- OK, CANCEL BUTTON
	pos_y = WIN_SIZE_HEI - 44;

	obj = nftool_normal_button_create_type1("Forgot PIN?", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, WIN_SIZE_WID/2-164, WIN_SIZE_HEI-64);
	nfui_regi_post_event_callback(obj, post_forgot_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, WIN_SIZE_WID/2+4, WIN_SIZE_HEI-64);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);  
	
	nfui_nfobject_show(main_wnd);

	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

#if defined(_SUPPORT_LOGIN_AUTOHIDE)
	vw_autohide_set_obj(g_curwnd, 10);
#endif

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);
	
	return g_retVal;
}
