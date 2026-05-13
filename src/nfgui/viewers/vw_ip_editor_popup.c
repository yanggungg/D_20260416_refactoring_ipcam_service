
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

#include "vw_ip_editor_popup.h"
#include "ix_mem.h"


#define NUM_KEY_SIZE_WID				(275)
#define NUM_KEY_SIZE_HEI				(372)

#define NUM_KEY_FIXED_POS_X				(15)
#define NUM_KEY_FIXED_POS_Y				(52)
#define NUM_KEY_FIXED_SIZE_WID			(NUM_KEY_SIZE_WID - NUM_KEY_FIXED_POS_X - 22)
#define NUM_KEY_FIXED_SIZE_HEI			(NUM_KEY_SIZE_HEI - NUM_KEY_FIXED_POS_Y - 25)

#define NORMAL_OUTLINE_COLOR		    (220)
#define FOCUS_OUTLINE_COLOR			    (146)
#define SELECT_OUTLINE_COLOR		    (147)

enum {
	MV_LEFT		= 0,
	MV_RIGHT    = 1,
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *field_obj[4];
static NFOBJECT *dot_obj[3];
static NFOBJECT *edit_field_obj;
static NFOBJECT *num_btns[10];

static IP_EDITOR_T *g_ip_editor_data;
static gint editor_res;


static gint _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

    nfui_signal_emit(from, GDK_EXPOSE, TRUE);
    nfui_signal_emit(to, GDK_EXPOSE, TRUE);

	return 0;
}

static gint _set_field_obj(NFOBJECT *obj)
{
    gint i;
    NFOBJECT *move_obj;

    if (edit_field_obj == obj) return -1;

    nfui_nfvklabel_set_use_cursor((NFVKLABEL*)edit_field_obj, 0);
    nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 1);

    nfui_nfvklabel_set_select_state((NFVKLABEL*)obj, 0);

    nfui_signal_emit(edit_field_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    edit_field_obj = obj;
     
    return 0;
}

static gint _move_field_obj(gint dir)
{
    gint i;
    NFOBJECT *move_obj;

    for (i = 0; i < 4; i++)
    {
        if (edit_field_obj == field_obj[i]) break;
    }

    if (dir == MV_LEFT)
    {
        if (i == 0)
            _set_field_obj(field_obj[3]);   
        else
            _set_field_obj(field_obj[i-1]);   
    }
    else if (dir == MV_RIGHT)
    {
        if (i == 3)
            _set_field_obj(field_obj[0]);   
        else if(i != 4)
            _set_field_obj(field_obj[i+1]);
    }
    
    return 0;
}

static gboolean pre_ip_field_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

    NFOBJECT *move_obj;
    gint i;

	if (event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;
	}
	
	if (event->type == GDK_BUTTON_PRESS)
    {
        if (edit_field_obj == obj) return FALSE;
    
        nfui_nfvklabel_set_use_cursor((NFVKLABEL*)edit_field_obj, 0);
        nfui_signal_emit(edit_field_obj, GDK_EXPOSE, TRUE);
        
        nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 1);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
       
        edit_field_obj = obj;
    }
	else if ((kpid == KEYPAD_LEFT) || (kpid == KEYPAD_RIGHT))
	{
        if (((NFVKLABEL*)obj)->object.grab_kfocus) return FALSE;

        if (kpid == KEYPAD_LEFT)        _move_field_obj(MV_LEFT);
        else if (kpid == KEYPAD_RIGHT)  _move_field_obj(MV_RIGHT);
	}

    return FALSE;
}

static gboolean post_number_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

    gchar *field_str;
	gint i, len;
	gint input_num, val;
    gint old_cc, new_cc;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		for (i = 0; i < 10; i++)
		{
			if(obj == num_btns[i]) break;
        }

        if (i == 9) input_num = 0;
        else        input_num = i+1;

        if (nfui_nfvklabel_get_cursor_state((NFVKLABEL*)edit_field_obj) == VK_SELECT)
        {       
            val = nfui_nfvklabel_get_num((NFVKLABEL*)edit_field_obj);

            if (val != -1)
            {
                val = (val*10) + input_num;
                
                if (val > 255) return FALSE;
            }
        }

        old_cc = nfui_nfvklabel_get_cursor_cc((NFVKLABEL*)edit_field_obj);

        nfui_nfvklabel_input_num((NFVKLABEL*)edit_field_obj, input_num);
        nfui_signal_emit(edit_field_obj, GDK_EXPOSE, TRUE);

        new_cc = nfui_nfvklabel_get_cursor_cc((NFVKLABEL*)edit_field_obj);

        if (edit_field_obj == field_obj[3]) return FALSE;
        if ((old_cc != 2) || (new_cc != 2)) return FALSE;

        for (i = 0; i < 4; i++)
        {
            if (edit_field_obj == field_obj[i]) break;
        }

        if (i == 3 || i == 4) return FALSE;

        field_str = nfui_nfvklabel_get_all_str((NFVKLABEL*)field_obj[i+1]);
       
        len = strlen(field_str);

        if (len) 
        {
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)field_obj[i], 0);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)field_obj[i+1], 1);

            nfui_nfvklabel_set_drag_state((NFVKLABEL*)field_obj[i+1], 0, len-1);

            nfui_signal_emit(field_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(field_obj[i+1], GDK_EXPOSE, TRUE);

            edit_field_obj = field_obj[i+1];
        } 
        else 
        {
            _move_field_obj(MV_RIGHT);
        }
	}
	else if (kpid == KEYPAD_UP)
	{
		for (i = 0; i < 10; i++)
		{
			if(obj == num_btns[i]) break;
        }

        if ((i == 0) || (i == 1) || (i == 2))
        {
            _set_field_obj(field_obj[i]);
            _change_obj_focus(num_btns[i], field_obj[i]);
			return TRUE;
        }
	}
	
	return FALSE;
}

static gboolean post_backspace_key_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfvklabel_erase((NFVKLABEL*)edit_field_obj, VK_ERASE_BACKSPACE);
		nfui_signal_emit(edit_field_obj, GDK_EXPOSE, TRUE);
	}
	else if (kpid == KEYPAD_UP)
	{
        _set_field_obj(field_obj[3]);
        _change_obj_focus(obj, field_obj[3]);        
		return TRUE;
	}
	
	return FALSE;
}

static gboolean post_del_key_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfvklabel_erase((NFVKLABEL*)edit_field_obj, VK_ERASE_DEL);
		nfui_signal_emit(edit_field_obj, GDK_EXPOSE, TRUE);		
	}
	
	return FALSE;
}

static gboolean post_dot_key_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	gint i, len;
    gchar *field_str;
    
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        for (i = 0; i < 4; i++)
        {
            if (edit_field_obj == field_obj[i]) break;
        }

        if (i == 3 || i == 4) return FALSE;

        field_str = nfui_nfvklabel_get_all_str((NFVKLABEL*)field_obj[i+1]);
       
        len = strlen(field_str);

        if (len) 
        {
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)field_obj[i], 0);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)field_obj[i+1], 1);

            nfui_nfvklabel_set_drag_state((NFVKLABEL*)field_obj[i+1], 0, len-1);

            nfui_signal_emit(field_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(field_obj[i+1], GDK_EXPOSE, TRUE);

            edit_field_obj = field_obj[i+1];
        } 
        else 
        {
            _move_field_obj(MV_RIGHT);
        }
	}
	
	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;
	gint i;

	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		for (i = 0; i < 4; i++)
		{
            if (nfui_nfvklabel_get_num((NFVKLABEL*)field_obj[i]) == -1)
                g_ip_editor_data->field[i] = 0;
            else
                g_ip_editor_data->field[i] = nfui_nfvklabel_get_num((NFVKLABEL*)field_obj[i]);            
		}

		editor_res = 0;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}
	else if (kpid == KEYPAD_DOWN)
	{
        _set_field_obj(field_obj[0]);
        _change_obj_focus(obj, field_obj[0]);        
		return TRUE;
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		editor_res = -1;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}
	else if (kpid == KEYPAD_DOWN)
	{
        _set_field_obj(field_obj[2]);
        _change_obj_focus(obj, field_obj[2]);             
		return TRUE;
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_ip_editor_popup_open(NFWINDOW *parent, guint x, guint y, IP_EDITOR_T *ip_editor_data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *field_fixed;	
	NFOBJECT *ntb;
	NFOBJECT *obj;
	GdkPixbuf *keybg_img[3][NFOBJECT_STATE_COUNT];
	GdkPixbuf *back_img[NFOBJECT_STATE_COUNT];

	gchar strBuf[16];
	guint editor_width[7];
	guint num_btn_width[10];

	const gchar *strVkBtn[] = {"OK", "CANCEL"};
	const guint btn1_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(278), COLOR_IDX(279), COLOR_IDX(280), COLOR_IDX(281)}; 
	const guint btn2_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(282), COLOR_IDX(283), COLOR_IDX(285), COLOR_IDX(286)}; 
	
	gint btn_w, btn_h;
	gint pos_x, pos_y;	
	guint i;

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

	g_ip_editor_data = ip_editor_data;

	if(x + NUM_KEY_SIZE_WID >= DISPLAY_ACTIVE_WIDTH)	x = DISPLAY_ACTIVE_WIDTH - NUM_KEY_SIZE_WID - 20;
	if(y + NUM_KEY_SIZE_HEI >= DISPLAY_ACTIVE_HEIGHT)	y = DISPLAY_ACTIVE_HEIGHT - NUM_KEY_SIZE_HEI - 20;

// <---- WINDOW
	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, x, y, NUM_KEY_SIZE_WID, NUM_KEY_SIZE_HEI, "IP EDITOR", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_nfobject_show(main_fixed);
	g_curwnd = main_wnd;

// <---- FIXED
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NUM_KEY_FIXED_SIZE_WID, NUM_KEY_FIXED_SIZE_HEI);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, NUM_KEY_FIXED_POS_X, NUM_KEY_FIXED_POS_Y);

// <---- FIELD FIXED
	field_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(field_fixed, NUM_KEY_FIXED_SIZE_WID, 40);
	nfui_nfobject_modify_bg(field_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
	nfui_nfobject_show(field_fixed);
	nfui_nffixed_put((NFFIXED*)fixed1, field_fixed, 0, 0);

// <---- FIELD LABEL
	nfui_get_pixbuf_size(keybg_img[0][0], &btn_w, &btn_h);

	editor_width[0] = btn_w - 6;
	editor_width[1] = 6;
	editor_width[2] = btn_w - 6;
	editor_width[3] = 6;
	editor_width[4] = btn_w - 6;	
	editor_width[5] = 6;

	nfui_get_pixbuf_size(keybg_img[2][0], &btn_w, &btn_h);
	editor_width[6] = btn_w - 4;

	ntb = (NFOBJECT*)nfui_nftable_new(7, 1, 2, 0, editor_width, 36);
	nfui_nffixed_put((NFFIXED*)field_fixed, ntb, 2, 2);
	nfui_nfobject_show(ntb);

	for (i = 0; i < 4; i++)
	{
    	obj = (NFOBJECT*)nfui_nfvklabel_new_num(ip_editor_data->field[i], 3);
    	nfui_nfvklabel_set_pango_font((NFVKLABEL*)obj, nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(221));
        nfui_nfvklabel_set_align((NFVKLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 0);        
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
    	nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, i*2, 0);
		nfui_regi_pre_event_callback(obj, pre_ip_field_event_handler);
		field_obj[i] = obj;		
	}

	for (i = 0; i < 3; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(".", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, (i*2)+1, 0);
		dot_obj[i] = obj;	
	}

	if (ip_editor_data->subnet24) {
		nfui_nfvklabel_set_use_cursor((NFVKLABEL*)field_obj[3], 1);
		nfui_nfvklabel_set_select_state((NFVKLABEL*)field_obj[3], 0);
		edit_field_obj = field_obj[3];

		nfui_nfvklabel_modify_fg((NFVKLABEL*)field_obj[0], COLOR_IDX(231));
		nfui_nfobject_modify_bg(field_obj[0], NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_use_focus(field_obj[0], NFOBJECT_FOCUS_OFF);
		nfui_nfvklabel_modify_fg((NFVKLABEL*)field_obj[1], COLOR_IDX(231));
		nfui_nfobject_modify_bg(field_obj[1], NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_use_focus(field_obj[1], NFOBJECT_FOCUS_OFF);
		nfui_nfvklabel_modify_fg((NFVKLABEL*)field_obj[2], COLOR_IDX(231));
		nfui_nfobject_modify_bg(field_obj[2], NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_use_focus(field_obj[2], NFOBJECT_FOCUS_OFF);
	}
	else {
		nfui_nfvklabel_set_use_cursor((NFVKLABEL*)field_obj[0], 1);
		nfui_nfvklabel_set_select_state((NFVKLABEL*)field_obj[0], 0);
		edit_field_obj = field_obj[0];
	}

// <---- NUMBER KEY BUTTON
	pos_x = 0;
	pos_y = 48;
	
	nfui_get_pixbuf_size(keybg_img[0][0], &btn_w, &btn_h);

	for (i = 0; i < 9; i++)
		num_btn_width[i] = btn_w;

	ntb = (NFOBJECT*)nfui_nftable_new(3, 3, 4, 4, num_btn_width, btn_h);
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
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);	
	nfui_regi_post_event_callback(obj, post_dot_key_event_handler);

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
	nfui_page_open(PGID_IP_EDITOR_POPUP, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_IP_EDITOR_POPUP, main_wnd);
	
	return editor_res;
}
