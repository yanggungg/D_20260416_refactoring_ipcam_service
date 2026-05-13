#include "iux_afx.h"
#include "../support/event_loop.h"
#include "nf_ui_tool.h"
#include "../support/nf_ui_page_manager.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_vkeyboard.h"
#include "vw_dvabx_object_filter_edit_popup.h"
#include "vw_search_by_deeplearning_filter_popup.h"


#define SAF_OBJ_START_POS_X         (15)
#define SAF_OBJ_START_POS_Y         (55)
#define SAF_OBJ_SIZE_H              (40)
#define SAF_OBJ_GAP                 (1)

#define SAF_BUTTON_SIZE_W           (150)

#define SAF_POPUP_SIZE_H            (420)
#define SAF_POPUP_SIZE_W            (790)

static NFWINDOW *g_curwnd;
static gint g_ret;
static OPT_AI_LPR_T *g_lpropt = NULL;

static NFOBJECT *g_number;
static NFOBJECT *g_grp[9];
static NFOBJECT *g_grpall;


static gboolean _set_grp_all_chkbtn()
{
    gboolean ret = TRUE;
    gint i;

    for (i = 0; i < 9; i++)
    {
        if (!g_lpropt->group[i]) ret = FALSE;
    }

    return ret;
}

static gboolean post_grp_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gint i;
        gboolean state = TRUE;

        for (i = 0; i < 9; i++) 
        {
            state = nfui_check_button_get_active((NFCHECKBUTTON*)g_grp[i]);
            if (!state) break;
        }

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_grpall, state);
        nfui_signal_emit((NFOBJECT*)g_grpall, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_grp_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        gint i;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        for (i = 0; i < 9; i++)
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_grp[i], state);
            nfui_signal_emit((NFOBJECT*)g_grp[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
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

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 64, VKEY_MULTIKEYPD|VKEY_MULTIKEYPD);

        if (strTemp)
        {
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

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

        for (i = 0; i < 9; i++)
        {
            g_lpropt->group[i] = nfui_check_button_get_active((NFCHECKBUTTON*)g_grp[i]);
        }
        
        strcpy(g_lpropt->number, nfui_nflabel_get_text((NFLABEL*)g_number));
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

gint VW_Search_By_Ai_LPR_Filter_Popup(NFWINDOW *parent, OPT_AI_LPR_T *opt)
{
    NFWINDOW *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint btn_w, btn_h;
    gint i;
    gchar buf[62];

    g_ret = 0;
    g_lpropt = opt;

    main_wnd = nftool_create_popup_window((NFWINDOW*)parent, (DISPLAY_ACTIVE_WIDTH-SAF_POPUP_SIZE_W)/2, (DISPLAY_ACTIVE_HEIGHT-SAF_POPUP_SIZE_H)/2, 
                                         SAF_POPUP_SIZE_W, SAF_POPUP_SIZE_H, "AI SEARCH FILTER", FALSE);
    nfui_nfobject_show(main_wnd);
    nfui_regi_post_event_callback(main_wnd, post_window_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = SAF_OBJ_START_POS_X;
    pos_y = SAF_OBJ_START_POS_Y;

    obj = nfui_checkbutton_new(_set_grp_all_chkbtn());
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_grp_all_event_handler);
    g_grpall = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    pos_x += 40;
    pos_y += 41; 

    for (i = 0; i < 9; i++)
    {
        if (i == 8)
        {
            obj = nfui_checkbutton_new(opt->group[i]);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
            nfui_check_get_size(obj, &btn_w, &btn_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
            nfui_regi_post_event_callback(obj, post_grp_event_handler);
            g_grp[i] = obj;

            strcpy(g_lpropt->group_name[i], "UNREGISTERED");
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("UNREGISTERED", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
            nfui_nfobject_show(obj);
        }
        else
        {
            obj = nfui_checkbutton_new(opt->group[i]);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
            nfui_check_get_size(obj, &btn_w, &btn_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
            nfui_regi_post_event_callback(obj, post_grp_event_handler);    
            g_grp[i] = obj;

            memset(buf, 0x00, sizeof(buf));
            nf_sql_search_lpr_group_from_id(i+1, buf, sizeof(buf));
            strcpy(g_lpropt->group_name[i], buf);
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
            nfui_nfobject_support_multi_lang(obj, FALSE);
            nfui_nfobject_show(obj);  	
        }
        
        pos_x += 240;
        if (i == 2 || i == 5) {
            pos_x = SAF_OBJ_START_POS_X + 40;
            pos_y += 41; 
        }
    }

    pos_x = SAF_OBJ_START_POS_X;
    pos_y += 60;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LICENSE PLATE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 42;
    
	obj = (NFOBJECT*)nfui_nflabel_new_text_box(opt->number, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_nfobject_set_size(obj, 760, SAF_OBJ_SIZE_H);
	nfui_regi_post_event_callback(obj, post_label_keyword_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_number = obj;

    pos_x = (SAF_POPUP_SIZE_W / 2) - SAF_BUTTON_SIZE_W - 1;
    pos_y = SAF_POPUP_SIZE_H - 60;
    
    obj = nftool_normal_button_create_type1("OK", SAF_BUTTON_SIZE_W);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_btn_ok_event_handler);

    pos_x = (SAF_POPUP_SIZE_W / 2) + 1;
    
    obj = nftool_normal_button_create_type1("CANCEL", SAF_BUTTON_SIZE_W);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_btn_cancel_event_handler);

    nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);

    nfui_page_open(PGID_AI_SEARCH_FILTER_EDIT_POPUP, g_curwnd, ssm_get_cur_id(NULL));

    gtk_main();

    nfui_page_close(PGID_AI_SEARCH_FILTER_EDIT_POPUP, g_curwnd);

    return g_ret;
}

