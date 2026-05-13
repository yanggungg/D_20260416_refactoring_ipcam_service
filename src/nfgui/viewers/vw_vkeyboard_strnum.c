
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

#include "vw_vkeyboard_strnum.h"
#include "ix_mem.h"


#define NUM_KEY_SIZE_WID				(275)
#define NUM_KEY_SIZE_HEI				(372)

#define NUM_KEY_FIXED_POS_X				(15)
#define NUM_KEY_FIXED_POS_Y				(52)
#define NUM_KEY_FIXED_SIZE_WID			(NUM_KEY_SIZE_WID - NUM_KEY_FIXED_POS_X - 22)
#define NUM_KEY_FIXED_SIZE_HEI			(NUM_KEY_SIZE_HEI - NUM_KEY_FIXED_POS_Y - 25)

#define NUM_KEY_LABEL_POS_X				(0)
#define NUM_KEY_LABEL_POS_Y				(0)
#define NUM_KEY_LABEL_SIZE_WID			(NUM_KEY_FIXED_SIZE_WID)
#define NUM_KEY_LABEL_SIZE_HEI			(40)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *num_label;
static NFOBJECT *num_btns[10];

static gchar *strResult = NULL;
static gint g_max_ch;

static gboolean post_number_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	gint i;
	gchar keyVal[8];
	
	if (evt->type == GDK_BUTTON_PRESS)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(keyVal, 0x00, sizeof(keyVal));

		for (i = 0; i < 10; i++)
		{
			if (obj == num_btns[i])
			{
				if (i == 9)
					g_sprintf(keyVal, "%d", 0);
				else
					g_sprintf(keyVal, "%d", i+1);
					
    			nfui_nfvklabel_input_character((NFVKLABEL*)num_label, keyVal[0]);
				nfui_signal_emit(num_label, GDK_EXPOSE, TRUE);
			}
		}
	}
	
	return FALSE;
}

static gboolean post_backspace_key_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	gint value;

	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        nfui_nfvklabel_erase((NFVKLABEL*)num_label, VK_ERASE_BACKSPACE);		
		nfui_signal_emit(num_label, GDK_EXPOSE, TRUE);
	}
	
	return FALSE;
}

static gboolean post_del_key_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	gint value;

	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        nfui_nfvklabel_erase((NFVKLABEL*)num_label, VK_ERASE_DEL);		
		nfui_signal_emit(num_label, GDK_EXPOSE, TRUE);
	}
	
	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;
	guint len;
	gchar *st;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		st = nfui_nfvklabel_get_all_str((NFVKLABEL*)num_label);
		len = strlen(st);

		strResult = imalloc(sizeof(gchar)*(len+1));
		if(len)	g_stpcpy(strResult, st);
		
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		strResult = NULL;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ((evt->type == NFEVENT_REMOCON_RELEASE) || (evt->type == NFEVENT_KEYPAD_RELEASE))
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		gchar buf[8];

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid) 
		{
			case KEYPAD_CH01:
			case KEYPAD_CH02:
			case KEYPAD_CH03:
			case KEYPAD_CH04:
			case KEYPAD_CH05:
			case KEYPAD_CH06:
			case KEYPAD_CH07:
			case KEYPAD_CH08:
			case KEYPAD_CH09:
			{
				g_sprintf(buf, "%d", kpid + 1);

				if (nfui_nfvklabel_input_character((NFVKLABEL*)num_label, buf[0]) == 0)
					nfui_signal_emit(num_label, GDK_EXPOSE, TRUE);
			}
			break;

			case KEYPAD_CH00:
			case RMC_NUM_0:
			{
				g_sprintf(buf, "%d", 0);

				if (nfui_nfvklabel_input_character((NFVKLABEL*)num_label, buf[0]) == 0)
					nfui_signal_emit(num_label, GDK_EXPOSE, TRUE);
			}
			break;
			
			default:
			return FALSE;
		}		
	}	
	else if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gchar* NumberKey_Str_Open(NFWINDOW *parent, gchar *str, guint x, guint y, gint max_ch)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *ntb;
	NFOBJECT *obj;
	GdkPixbuf *keybg_img[3][NFOBJECT_STATE_COUNT];
	GdkPixbuf *back_img[NFOBJECT_STATE_COUNT];

	gchar strBuf[16];
	guint cell_width[10];

    gint char_cnt;

	const gchar *strVkBtn[] = {"OK", "CANCEL"};
	const guint btn1_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(278), COLOR_IDX(279), COLOR_IDX(280), COLOR_IDX(281)}; 
	const guint btn2_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(282), COLOR_IDX(283), COLOR_IDX(285), COLOR_IDX(286)}; 

	gint btn_w, btn_h;
	gint pos_x, pos_y;	
	guint i;

	strResult = NULL;
	g_max_ch = max_ch;

	keybg_img[0][0] = nfui_get_image_from_file((IMG_N_NUM_KEY_01), NULL);
	keybg_img[0][1] = nfui_get_image_from_file((IMG_O_NUM_KEY_01), NULL);
	keybg_img[0][2] = nfui_get_image_from_file((IMG_P_NUM_KEY_01), NULL);
	keybg_img[0][3] = nfui_get_image_from_file((IMG_D_NUM_KEY_01), NULL);

	keybg_img[1][0] = nfui_get_image_from_file((IMG_N_NUM_KEY_02), NULL);
	keybg_img[1][1] = nfui_get_image_from_file((IMG_O_NUM_KEY_02), NULL);
	keybg_img[1][2] = nfui_get_image_from_file((IMG_P_NUM_KEY_02), NULL);
	keybg_img[1][3] = nfui_get_image_from_file((IMG_D_NUM_KEY_02), NULL);

	keybg_img[2][0] = nfui_get_image_from_file((IMG_N_NUM_KEY_03), NULL);
	keybg_img[2][1] = nfui_get_image_from_file((IMG_O_NUM_KEY_03), NULL);
	keybg_img[2][2] = nfui_get_image_from_file((IMG_P_NUM_KEY_03), NULL);
	keybg_img[2][3] = nfui_get_image_from_file((IMG_D_NUM_KEY_03), NULL);

	back_img[0] = nfui_get_image_from_memory(MK_IMG_N_NUM_KEY_BACKSPACE);
	back_img[1] = nfui_get_image_from_memory(MK_IMG_O_NUM_KEY_BACKSPACE);
	back_img[2] = nfui_get_image_from_memory(MK_IMG_P_NUM_KEY_BACKSPACE);
	back_img[3] = nfui_get_image_from_memory(MK_IMG_D_NUM_KEY_BACKSPACE);

	if(x + NUM_KEY_SIZE_WID >= DISPLAY_ACTIVE_WIDTH)	x = DISPLAY_ACTIVE_WIDTH - NUM_KEY_SIZE_WID - 20;
	if(y + NUM_KEY_SIZE_HEI >= DISPLAY_ACTIVE_HEIGHT)	y = DISPLAY_ACTIVE_HEIGHT - NUM_KEY_SIZE_HEI - 20;


// <---- WINDOW
	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, x, y, NUM_KEY_SIZE_WID, NUM_KEY_SIZE_HEI, "VIRTUAL KEYBOARD", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_nfobject_show(main_fixed);
	g_curwnd = main_wnd;

// <---- FIXED
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NUM_KEY_FIXED_SIZE_WID, NUM_KEY_FIXED_SIZE_HEI);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, NUM_KEY_FIXED_POS_X, NUM_KEY_FIXED_POS_Y);

// <---- NUM LABEL
	obj = (NFOBJECT*)nfui_nfvklabel_new_str((gchar*)str, max_ch);
	nfui_nfvklabel_set_pango_font((NFVKLABEL*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(221));
    nfui_nfvklabel_set_align((NFVKLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, NUM_KEY_LABEL_SIZE_WID, NUM_KEY_LABEL_SIZE_HEI);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
	nfui_nffixed_put((NFFIXED*)fixed1, obj, NUM_KEY_LABEL_POS_X, NUM_KEY_LABEL_POS_Y);
	nfui_nfobject_show(obj);
	num_label = obj;

// <---- NUMBER KEY BUTTON
	pos_x = 0;
	pos_y = 48;
	
	nfui_get_pixbuf_size(keybg_img[0][0], &btn_w, &btn_h);

	for (i = 0; i < 9; i++)
		cell_width[i] = btn_w;

	ntb = (NFOBJECT*)nfui_nftable_new(3, 3, 4, 4, cell_width, btn_h);
	nfui_nffixed_put((NFFIXED*)fixed1, ntb, pos_x, pos_y);
	nfui_nfobject_show(ntb);

	for (i = 0; i < 9; i++)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "%d", i+1);
		
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(keybg_img[0], strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn1_font_color);
		nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, i%3, i/3);		
		nfui_regi_post_event_callback(obj, post_number_event_handler);
		num_btns[i] = obj;
	}

	pos_y += (btn_h+4)*3;

	nfui_get_pixbuf_size(keybg_img[1][0], &btn_w, &btn_h);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(keybg_img[1], "0");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn1_font_color);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);	
	nfui_regi_post_event_callback(obj, post_number_event_handler);
	num_btns[9] = obj;

// <---- DOT KEY BUTTON
	pos_x += (btn_w+4);

	nfui_get_pixbuf_size(keybg_img[0][0], &btn_w, &btn_h);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(keybg_img[0], ".");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn1_font_color);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_disable(obj);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);	

// <---- BACKSPACE KEY BUTTON
	nfui_get_pixbuf_size(keybg_img[0][0], &btn_w, &btn_h);

	pos_x = (btn_w+4)*3;
	pos_y = 48;

	nfui_get_pixbuf_size(back_img[0], &btn_w, &btn_h);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(back_img, "");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn2_font_color);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_backspace_key_event_handler);

// <---- DEL KEY BUTTON
	pos_y += (btn_h+4);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(keybg_img[2], "DEL");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn2_font_color);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_del_key_event_handler);
	
// <---- OK, CANCEL BUTTON
	pos_x = 0;
	pos_y = NUM_KEY_FIXED_SIZE_HEI - 44;

	obj = nftool_normal_button_create_type1("OK", 116);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 116);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+116+6, pos_y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
	
	nfui_nfobject_show(main_wnd);

	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);
	
	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_SETUP_VKEY_NUM, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SETUP_VKEY_NUM, main_wnd);
	
	return strResult;
}
