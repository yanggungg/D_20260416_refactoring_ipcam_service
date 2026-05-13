#include "iux_afx.h"
#include "../support/event_loop.h"
#include "nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"

#include "vw_vkeyboard.h"
#include "vw_search_keyword_popup.h"



#define SKP_OBJ_START_POS_X         (8)
#define SKP_OBJ_START_POS_Y         (55)
#define SKP_OBJ_GAP                 (2)

#define SKP_TITLE_SIZE_W            (200)
#define SKP_OPER_SIZE_W             (100)
#define SKP_KEYWORD_SIZE_W          (300)    
#define SKP_BUTTON_SIZE_W           (150)

#define SKP_POPUP_SIZE_H            (280)
#define SKP_POPUP_SIZE_W            (SKP_OBJ_START_POS_X + SKP_TITLE_SIZE_W + SKP_OPER_SIZE_W + SKP_KEYWORD_SIZE_W + (SKP_OBJ_GAP * 2) + 13)

static SEARCH_KEY_T *g_keyword_info;

static NFWINDOW *g_curwnd;
static NFOBJECT *g_keyword[3];
static NFOBJECT *g_oper[2];
static gint g_keyword_cnt;
static gint g_ret;



static void _set_textbox_obj(gint expose)
{
    gchar *strKeyword = NULL;
    
    strKeyword = nfui_nflabel_get_text((NFLABEL*)g_keyword[0]);

    if (strlen(strKeyword)) 
    {
        nfui_nfobject_enable(g_keyword[1]);
        nfui_nfobject_enable(g_oper[0]);
    }
    else 
    {
        nfui_nfobject_disable(g_keyword[1]);
        nfui_nfobject_disable(g_oper[0]);
    }

    if (!nfui_nfobject_is_disabled(g_keyword[1])) 
    {
        strKeyword = nfui_nflabel_get_text((NFLABEL*)g_keyword[1]);

        if (strlen(strKeyword)) 
        {
            nfui_nfobject_enable(g_keyword[2]);
            nfui_nfobject_enable(g_oper[1]);
        }
        else 
        {
            nfui_nfobject_disable(g_keyword[2]);
            nfui_nfobject_disable(g_oper[1]);
        }
    }
    else
    {
        nfui_nfobject_disable(g_keyword[2]);
        nfui_nfobject_disable(g_oper[1]);
    }
    
    if (expose) {
        nfui_signal_emit(g_keyword[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_keyword[2], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_oper[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_oper[1], GDK_EXPOSE, TRUE);
    }
    
}

static gboolean post_label_keyword_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    guint x, y;
    gchar *strTemp = NULL;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}
	
    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 64, VKEY_NORMAL);

        if (strTemp)
        {
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            _set_textbox_obj(1);

            ifree(strTemp);
            strTemp = NULL;
        }
    }

    return FALSE;
}

static gboolean post_btn_cancel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_ret = 0;
        
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_btn_ok_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gint i;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(g_keyword_info, 0x00, sizeof(SEARCH_KEY_T));
        
        for (i = 0; i < g_keyword_cnt; i++) {
            if (nfui_nfobject_is_disabled(g_keyword[i])) continue;
            
            strcpy(g_keyword_info->keyword[i], nfui_nflabel_get_text((NFLABEL*)g_keyword[i]));
        }

        for (i = 0; i < g_keyword_cnt-1; i++) {
            if (nfui_nfobject_is_disabled(g_oper[i])) continue;
            
            g_keyword_info->oper[i] = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_oper[i]);
        }

        g_ret = 1;

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_window_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;

        gtk_main_quit();
    }
    
    return FALSE;
}

gint VW_Search_Keyword_Popup(NFOBJECT *parent, SEARCH_KEY_T *keyword_info, gint keyword_cnt, gint x, gint y)
{
    NFWINDOW *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gchar strTitle[16];
    gchar *strOperator[] = {"AND", "OR"};
    gint pos_x, pos_y;
    gint i;

    g_ret = 0;
    g_keyword_info = keyword_info;
    g_keyword_cnt = keyword_cnt;

    main_wnd = nftool_create_popup_window((NFWINDOW*)parent, x, y, SKP_POPUP_SIZE_W, SKP_POPUP_SIZE_H, "ITEM KEYWORD", FALSE);
    nfui_nfobject_show(main_wnd);
    nfui_regi_post_event_callback(main_wnd, post_window_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = SKP_OBJ_START_POS_X;
    pos_y = SKP_OBJ_START_POS_Y;

    for (i = 0; i < keyword_cnt; i++)
    {
        memset(strTitle, 0x00, sizeof(strTitle));
        sprintf(strTitle, "%s %d",lookup_string("ITEM KEYWORD"), i + 1);
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, SKP_TITLE_SIZE_W, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

        pos_x += SKP_TITLE_SIZE_W + SKP_OBJ_GAP;

        if (i >= 1)
        {
            obj = (NFOBJECT *)nfui_combobox_new(strOperator, 2, keyword_info->oper[i-1]);
            nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
            nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
            nfui_nfobject_set_size(obj, SKP_OPER_SIZE_W, 40);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
            g_oper[i-1] = obj;
        }

        pos_x += SKP_OPER_SIZE_W + SKP_OBJ_GAP;

        obj = nfui_nflabel_new_text_box(keyword_info->keyword[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
        nfui_nfobject_set_size(obj, SKP_KEYWORD_SIZE_W, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
        nfui_regi_post_event_callback(obj, post_label_keyword_event_handler);
        g_keyword[i] = obj;

        pos_x = 8;
        pos_y += 40 + SKP_OBJ_GAP;
    }

    pos_y = SKP_POPUP_SIZE_H - 40 - 15;
    pos_x = (SKP_POPUP_SIZE_W / 2) - SKP_BUTTON_SIZE_W - 2;
    
    obj = nftool_normal_button_create_popup_type1("OK", SKP_BUTTON_SIZE_W);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_btn_ok_event_handler);

    pos_x = (SKP_POPUP_SIZE_W / 2) + 2;
    
    obj = nftool_normal_button_create_popup_type1("CANCEL", SKP_BUTTON_SIZE_W);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_btn_cancel_event_handler);

    _set_textbox_obj(0);

    nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);

    gtk_main();

    return g_ret;
}
