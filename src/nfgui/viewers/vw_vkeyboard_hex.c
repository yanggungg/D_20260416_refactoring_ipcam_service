
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

#include "vw_vkeyboard_hex.h"
#include "ix_mem.h"

#include "nf_pos.h"

#define WIN_SIZE_WID				(650)
#define WIN_SIZE_HEI				(310)

enum {
	MV_LEFT		= 0,
	MV_RIGHT    = 1,
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_field_obj[8];
static NFOBJECT *g_edit_field_obj;
static NFOBJECT *g_num_btns[16];

static gint *g_hex_val;
static gint g_hex_cnt;


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

    if (g_edit_field_obj == obj) return -1;

    nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_edit_field_obj, 0);
    nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 1);

    nfui_nfvklabel_set_select_state((NFVKLABEL*)obj, 0);

    nfui_signal_emit(g_edit_field_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    g_edit_field_obj = obj;
     
    return 0;
}

static gint _move_field_obj(gint dir)
{
    gint i;
    NFOBJECT *move_obj;

    for (i = 0; i < 8; i++)
    {
        if (g_edit_field_obj == g_field_obj[i]) break;
    }

		if (i == 8) return 0;

    if (dir == MV_LEFT)
    {
        if (i == 0)
            _set_field_obj(g_field_obj[7]);   
        else
            _set_field_obj(g_field_obj[i-1]);   
    }
    else if (dir == MV_RIGHT)
    {
        if (i == 7)
            _set_field_obj(g_field_obj[0]);   
        else
            _set_field_obj(g_field_obj[i+1]);
    }
    
    return 0;
}

static gboolean pre_field_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
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
        if (g_edit_field_obj == obj) return FALSE;
    
        nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_edit_field_obj, 0);
        nfui_signal_emit(g_edit_field_obj, GDK_EXPOSE, TRUE);
        
        nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 1);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
       
        g_edit_field_obj = obj;
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
	KEYPAD_KID kpid = 0;

    gchar *field_str;
	gint i, len;
    gchar *strbuf;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_BUTTON_PRESS)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        strbuf = nfui_nfbutton_get_text((NFBUTTON*)obj);
        nfui_nfvklabel_input_character((NFVKLABEL*)g_edit_field_obj, strbuf[0]);
        nfui_signal_emit(g_edit_field_obj, GDK_EXPOSE, TRUE);

        if (g_edit_field_obj == g_field_obj[7]) return FALSE;

        strbuf = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_edit_field_obj);       
        if (strlen(strbuf) < 2) return FALSE;

        for (i = 0; i < 8; i++)
        {
            if (g_edit_field_obj == g_field_obj[i]) break;
        }

        if (i >= 7) return FALSE;
        
        field_str = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_field_obj[i+1]);
       
        len = strlen(field_str);

        if (len) 
        {
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_field_obj[i], 0);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_field_obj[i+1], 1);

            nfui_nfvklabel_set_drag_state((NFVKLABEL*)g_field_obj[i+1], 0, len-1);

            nfui_signal_emit(g_field_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_field_obj[i+1], GDK_EXPOSE, TRUE);

            g_edit_field_obj = g_field_obj[i+1];
        } 
        else 
        {
            _move_field_obj(MV_RIGHT);
        }
	}
	else if (kpid == KEYPAD_UP)
	{
		for (i = 0; i < 8; i++)
		{
			if (obj == g_num_btns[i])
			{
                _set_field_obj(g_field_obj[0]);
                _change_obj_focus(g_num_btns[i], g_field_obj[0]);
    			return TRUE;
			}
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

		nfui_nfvklabel_erase((NFVKLABEL*)g_edit_field_obj, VK_ERASE_BACKSPACE);
		nfui_signal_emit(g_edit_field_obj, GDK_EXPOSE, TRUE);
	}
	else if (kpid == KEYPAD_UP)
	{
        _set_field_obj(g_field_obj[0]);
        _change_obj_focus(obj, g_field_obj[0]);        
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

		nfui_nfvklabel_erase((NFVKLABEL*)g_edit_field_obj, VK_ERASE_DEL);
		nfui_signal_emit(g_edit_field_obj, GDK_EXPOSE, TRUE);		
	}
	
	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
        gchar *str;
        gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        for (i = 0; i < g_hex_cnt; i++)
        {
            str = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_field_obj[i]);
            if (strlen(str)) sscanf(str, "%02X", &g_hex_val[i]);
            else g_hex_val[i] = -1;
        }
	
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

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{	
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_vkey_hex_open(NFWINDOW *parent, guint x, guint y, gint *hex_val, gint cnt)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *fixed;
	NFOBJECT *btn_fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj;
	
	GdkPixbuf *keybg_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *back_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *del_img[NFOBJECT_STATE_COUNT];

	const guint btn1_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(278), COLOR_IDX(279), COLOR_IDX(280), COLOR_IDX(281)}; 
	const guint btn2_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(282), COLOR_IDX(283), COLOR_IDX(285), COLOR_IDX(286)}; 
	
	gchar strBuf[16];
	guint cell_width[8];
    
	gint btn_w, btn_h;
	gint pos_x, pos_y;	
	guint i;


    g_hex_val = hex_val;
    g_hex_cnt = cnt;


	keybg_img[0] = nfui_get_image_from_file((IMG_N_NUM_KEY_01), NULL);
	keybg_img[1] = nfui_get_image_from_file((IMG_O_NUM_KEY_01), NULL);
	keybg_img[2] = nfui_get_image_from_file((IMG_P_NUM_KEY_01), NULL);
	keybg_img[3] = nfui_get_image_from_file((IMG_D_NUM_KEY_01), NULL);

	back_img[0] = nfui_get_image_from_memory(MK_IMG_N_KEY03_BACKSPACE);
	back_img[1] = nfui_get_image_from_memory(MK_IMG_O_KEY03_BACKSPACE);
	back_img[2] = nfui_get_image_from_memory(MK_IMG_P_KEY03_BACKSPACE);
	back_img[3] = nfui_get_image_from_memory(MK_IMG_D_KEY03_BACKSPACE);

	del_img[0] = nfui_get_image_from_file((IMG_N_KEY_03), NULL);
	del_img[1] = nfui_get_image_from_file((IMG_O_KEY_03), NULL);
	del_img[2] = nfui_get_image_from_file((IMG_P_KEY_03), NULL);
	del_img[3] = nfui_get_image_from_file((IMG_D_KEY_03), NULL);


	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, (1920-WIN_SIZE_WID)/2, (1080-WIN_SIZE_HEI)/2, WIN_SIZE_WID, WIN_SIZE_HEI, "VIRTUAL KEYBOARD", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WIN_SIZE_WID-15-22, WIN_SIZE_HEI-72-10);
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 15, 72);
    btn_fixed = fixed;
	    
	pos_x = 0;
	pos_y = 0;

	nfui_get_pixbuf_size(keybg_img[0], &btn_w, &btn_h);

	for (i = 0; i < 8; i++)
	{
		cell_width[i] = btn_w+20;
    }

    ntb = (NFOBJECT*)nfui_nftable_new(8, 1, 1, 0, cell_width, 40);
    nfui_nftable_set_draw_outline((NFTABLE*)ntb, TRUE);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)btn_fixed, ntb, pos_x, pos_y);

	for (i = 0; i < 8; i++)
    {
    	fixed = (NFOBJECT*)nfui_nffixed_new();
    	nfui_nfobject_show(fixed);
		nfui_nftable_attach((NFTABLE*)ntb, fixed, i%8, i/8);		

        obj = nfui_nflabel_new_with_pango_font("0x", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 2);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    	nfui_nfobject_set_size(obj, 28, 36);        
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, 2, 2);

        if (i < cnt)
        {
    		memset(strBuf, 0x00, sizeof(strBuf));
            if (hex_val[i] >= 0) g_sprintf(strBuf, "%02X", hex_val[i]);
        }
	
    	obj = (NFOBJECT*)nfui_nfvklabel_new_str(strBuf, 2);
    	nfui_nfvklabel_set_pango_font((NFVKLABEL*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(221));
        nfui_nfvklabel_set_align((NFVKLABEL*)obj, NFALIGN_LEFT, 2);
        nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 0);        
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
    	nfui_nfobject_set_size(obj, fixed->width-34, 36);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, 30, 2);
		nfui_regi_pre_event_callback(obj, pre_field_event_handler);
		g_field_obj[i] = obj;		

        if (i >= cnt) nfui_nfobject_disable(obj);
    }

    nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_field_obj[0], 1);
    nfui_nfvklabel_set_select_state((NFVKLABEL*)g_field_obj[0], 0);    
    g_edit_field_obj = g_field_obj[0];

	pos_y += nfui_nftable_get_height((NFTABLE*)ntb) + 30;

	for (i = 0; i < 8; i++)
	{
		cell_width[i] = btn_w;
    }

    pos_x = 20;

	ntb = (NFOBJECT*)nfui_nftable_new(8, 2, 4, 6, cell_width, btn_h);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)btn_fixed, ntb, pos_x, pos_y);

	for (i = 0; i < 16; i++)
	{
		memset(strBuf, 0x00, sizeof(strBuf));

		if (i < 10) g_sprintf(strBuf, "%d", i);
		else if (i == 10) strcpy(strBuf, "A");
		else if (i == 11) strcpy(strBuf, "B");
		else if (i == 12) strcpy(strBuf, "C");
		else if (i == 13) strcpy(strBuf, "D");
		else if (i == 14) strcpy(strBuf, "E");
		else if (i == 15) strcpy(strBuf, "F");		
		
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(keybg_img, strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn1_font_color);
		nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, i%8, i/8);		
		nfui_regi_post_event_callback(obj, post_number_event_handler);
		g_num_btns[i] = obj;
	}

	pos_x += nfui_nftable_get_width((NFTABLE*)ntb) + 8;

	nfui_get_pixbuf_size(back_img[0], &btn_w, &btn_h);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(back_img, "");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn2_font_color);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)btn_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_backspace_key_event_handler);

	pos_y += (btn_h+4);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(del_img, "DEL");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn2_font_color);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)btn_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_del_key_event_handler);
	
	pos_x = 0;
	pos_y = btn_fixed->height - 44;

	obj = nftool_normal_button_create_type1("OK", 140);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)btn_fixed, obj, WIN_SIZE_WID/2-145, pos_y);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 140);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)btn_fixed, obj, WIN_SIZE_WID/2+5, pos_y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
	
	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	
	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_SETUP_VKEY_HEX, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SETUP_VKEY_HEX, main_wnd);
	
	return 0;
}

