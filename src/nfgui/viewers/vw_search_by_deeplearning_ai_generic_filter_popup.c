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
#include "objects/nflistbox.h"

#include "vw_vkeyboard.h"
#include "vw_dvabx_object_filter_edit_popup.h"
#include "vw_search_by_deeplearning_filter_popup.h"

#define GENERIC_FILE "/NFDVR/log/g_data.log"

#define SAF_OBJ_START_POS_X         (15)
#define SAF_OBJ_START_POS_Y         (55)
#define SAF_OBJ_SIZE_H              (40)
#define SAF_OBJ_GAP                 (1)

#define SAF_BUTTON_SIZE_W           (150)

#define SAF_POPUP_SIZE_H            (400)
#define SAF_POPUP_SIZE_W            (820)

#define SKP_OBJ_GAP                 (2)
#define SKP_OPER_SIZE_W             (100)
#define SKP_KEYWORD_SIZE_W          (300)

static NFWINDOW *g_curwnd;
static gint g_ret;
static OPT_AI_GENERIC_T *g_opt_ai_generic = NULL;
static OPT_AI_GENERIC_T g_org_opt_ai_generic;

static NFOBJECT *g_name;
static NFOBJECT *g_evtlist;
static NFOBJECT *g_groupcombo;
static NFOBJECT *g_gender;
static NFOBJECT *g_keyword[3];
static NFOBJECT *g_oper[2];
static NFOBJECT *g_match_case_obj;
static NFOBJECT *g_match_whole_obj;
static NFOBJECT *g_select_obj;
//static NFOBJECT *g_deselect_obj;
static NFOBJECT *g_result_obj;
static NFOBJECT *g_list_chk_obj[MAX_SHOW_LIST_CNT];

static void _update_file_data()
{
    FILE *fp = NULL;
    char *buf = NULL;
    char *pbuf = NULL;
    char* word = NULL;
    char *pnext = NULL;
    size_t f_size = 0;

    fp = fopen(GENERIC_FILE, "rt");

    if (fp == NULL)
    {
        g_message("FILE OPEN ERROR");
        return;
    }

    fseek(fp, 0, SEEK_END);
	f_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

    buf = (char*)malloc(sizeof(char)*(f_size+1));
    fread(buf, 1, f_size, fp);
    buf[f_size] = '\0';

    nfui_nflabel_set_text((NFLABEL*)g_result_obj, g_opt_ai_generic->evt_type);

    word = strtok_r(buf, ",", &pnext);
    pbuf = pnext;
    
	while (word != NULL) {
        if (strlen(word))   
        {
            nfui_listbox_set_text((NFLISTBOX*)g_evtlist, &word);
            g_message("%s => %s(%d)", __FUNCTION__, word, strlen(word));
        }
        word = strtok_r(pbuf, ",", &pnext);
        pbuf = pnext;
	}

    free(buf);
    fclose(fp);
    return;
}

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

static gboolean post_dva_select_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        int idx;
        gchar *sel_str;
        gchar *org_str;
        gchar* word;
        gchar buf[64 * 128];
        gchar *pbuf = NULL;
        gchar *pnext = NULL;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_evtlist);
        if (idx != -1) 
        {
            sel_str = nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, idx, 0);
            org_str = nfui_nflabel_get_text((NFLABEL*)g_result_obj);

            if (org_str != NULL && sel_str != NULL)
            {
                memset(buf, 0x00, sizeof(buf));
                if (strstr(org_str, sel_str) != NULL)
                {
                    word = strtok_r(org_str, ",", &pnext);
                    pbuf = pnext;
                    while (word != NULL) {
                        if(strstr(word, sel_str) == NULL)   
                        {
                            sprintf(buf, "%s,%s", buf, word);
                            nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "SELECT");
                        }
                        word = strtok_r(pbuf, ",", &pnext);
                        pbuf = pnext;
                    }
                }
                else
                {
                    memcpy(buf, org_str, strlen(org_str));
                    if(strlen(org_str) == 0)   sprintf(buf, ",%s", sel_str);
                    else   sprintf(buf, "%s,%s", buf, sel_str);
                    nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "DESELECT");
                }
            }

            nfui_nflabel_set_text((NFLABEL*)g_result_obj, buf);
            nfui_signal_emit(g_result_obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_select_obj, GDK_EXPOSE, TRUE);
        }
    }
	return FALSE;
}

static gboolean post_dva_select_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        guint i, j;
        guint idx;
        guint val_idx = 0;
        gchar *sel_str;
        gchar *org_str;
        gchar* word;
        gchar buf[64 * 128];
        gchar *pbuf = NULL;
        gchar *pnext = NULL;

        if (obj == g_list_chk_obj[0]) idx = 0;
        else if(obj == g_list_chk_obj[1]) idx = 1;
        else if(obj == g_list_chk_obj[2]) idx = 2;
        else if(obj == g_list_chk_obj[3]) idx = 3;
        else if(obj == g_list_chk_obj[4]) idx = 4;

        for (i=0; i<((NFLISTBOX*)g_evtlist)->text_row_cnt; i++)
        {
            if(strcmp(nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, i, 0), nfui_listbox_get_text_of_box((NFLISTBOX*)g_evtlist, idx, 0)) == 0) break;
            val_idx++;
        }

        g_opt_ai_generic->evt_type_chk[val_idx] = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        sel_str = nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, val_idx, 0);
        org_str = nfui_nflabel_get_text((NFLABEL*)g_result_obj);

        if (org_str != NULL && sel_str != NULL)
        {
            memset(buf, 0x00, sizeof(buf));
            if (strstr(org_str, sel_str) != NULL)
            {
                word = strtok_r(org_str, ",", &pnext);
                pbuf = pnext;
                
                while (word != NULL) {
                    if(strstr(word, sel_str) == NULL)   
                    {
                        sprintf(buf, "%s,%s", buf, word);
                    }
                    word = strtok_r(pbuf, ",", &pnext);
                    pbuf = pnext;
                }
            }
            else
            {
                memcpy(buf, org_str, strlen(org_str));
                if(strlen(org_str) == 0)   sprintf(buf, ",%s", sel_str);
                else   sprintf(buf, "%s,%s", buf, sel_str);
            }
        }

        nfui_nflabel_set_text((NFLABEL*)g_result_obj, buf);
        nfui_signal_emit(g_result_obj, GDK_EXPOSE, TRUE);

        if (strlen(nfui_nflabel_get_text((NFLABEL*)g_result_obj)) == 0) nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "SELECT ALL");
        else 
        {
            for (i=0; i<((NFLISTBOX*)g_evtlist)->text_row_cnt; i++)
            {
                if(strstr(nfui_nflabel_get_text((NFLABEL*)g_result_obj), nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, i, 0)) == NULL)
                {
                    return FALSE;
                }
            }
            nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "DESELECT ALL");
        }
        nfui_signal_emit((NFBUTTON*)g_select_obj, GDK_EXPOSE, TRUE);
    }

	return FALSE;
}

static gboolean post_dva_deselect_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint i;
        gchar buf[48*100];

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)g_select_obj), "DESELECT ALL") == 0) 
        {
            for (i=0; i<MAX_SHOW_LIST_CNT; i++)
            {
                nfui_check_button_set_active(g_list_chk_obj[i], FALSE);
                nfui_signal_emit(g_list_chk_obj[i], GDK_EXPOSE, TRUE);
            }

            for (i=0; i<((NFLISTBOX*)g_evtlist)->text_row_cnt; i++)
            {
                g_opt_ai_generic->evt_type_chk[i] = FALSE;
            }
            nfui_nflabel_set_text((NFLABEL*)g_result_obj, "");
            nfui_signal_emit(g_result_obj, GDK_EXPOSE, TRUE);

            nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "SELECT ALL");
            nfui_signal_emit(g_select_obj, GDK_EXPOSE, TRUE);
        }
        else
        {
            for (i=0; i<MAX_SHOW_LIST_CNT; i++)
            {
                nfui_check_button_set_active(g_list_chk_obj[i], TRUE);
                nfui_signal_emit(g_list_chk_obj[i], GDK_EXPOSE, TRUE);
            }

            memset(buf, 0x00, sizeof(buf));
            for (i=0; i<((NFLISTBOX*)g_evtlist)->text_row_cnt; i++)
            {
                g_opt_ai_generic->evt_type_chk[i] = TRUE;
                if (i == 0) sprintf(buf, "%s", nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, i, 0));
                else sprintf(buf, "%s,%s", buf, nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, i, 0));
            }
            nfui_nflabel_set_text((NFLABEL*)g_result_obj, buf);
            nfui_signal_emit(g_result_obj, GDK_EXPOSE, TRUE);

            nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "DESELECT ALL");
            nfui_signal_emit(g_select_obj, GDK_EXPOSE, TRUE);
        }
    }

	return FALSE;
}
static gboolean post_dva_listbox_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_SCROLL)
    {
        GdkEventScroll *sevt;
        guint i;
        guint chk = 0;

        sevt = (GdkEventScroll*)evt;

        if (sevt->direction == GDK_SCROLL_UP) 
        {
            g_message("SCROLL UP");
            for (i=0; i<MAX_SHOW_LIST_CNT; i++)
            {
                if (strstr(nfui_nflabel_get_text((NFLABEL*)g_result_obj), nfui_listbox_get_text_of_box((NFLISTBOX*)g_evtlist, i, 0)) != NULL)
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], TRUE);
                }
                else
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], FALSE);
                }
            }
        }
        else if(sevt->direction == GDK_SCROLL_DOWN) 
        {
            g_message("SCROLL DOWN");
            for (i=0; i<MAX_SHOW_LIST_CNT; i++)
            {
                if (strstr(nfui_nflabel_get_text((NFLABEL*)g_result_obj), nfui_listbox_get_text_of_box((NFLISTBOX*)g_evtlist, i, 0)) != NULL)
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], TRUE);
                }
                else
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], FALSE);
                }
            }
        }

        for (i=0; i<MAX_SHOW_LIST_CNT; i++)
        {
            nfui_signal_emit((NFOBJECT*)g_list_chk_obj[i], GDK_EXPOSE, TRUE);
        }
    }
#if 0
    else if (evt->type == NFEVENT_LISTBOX_CHANGED)
    {
        gchar *evt_type;
        gchar *tot_evt_type;

        tot_evt_type = nfui_nflabel_get_text((NFLABEL*)g_result_obj);
        evt_type = nfui_listbox_get_text_of_list((NFLISTBOX*)g_evtlist, nfui_listbox_get_focus_idx((NFLISTBOX*)g_evtlist), 0);

        if (tot_evt_type != NULL && evt_type != NULL)
        {
            if (strstr(tot_evt_type, evt_type) != NULL)
            {
                nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "DESELECT");
            }
            else
            {
                nfui_nfbutton_set_text((NFBUTTON*)g_select_obj, "SELECT");
            }
            nfui_signal_emit((NFOBJECT*)g_select_obj, GDK_EXPOSE, TRUE);
        }
    }
#endif
	return FALSE;
}

static gboolean post_dva_listbox_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        guint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (((NFLISTBOX*)g_evtlist)->down_btn == obj)
        {
            for (i=0; i<MAX_SHOW_LIST_CNT; i++)
            {
                if (strstr(nfui_nflabel_get_text((NFLABEL*)g_result_obj), nfui_listbox_get_text_of_box((NFLISTBOX*)g_evtlist, i, 0)) != NULL)
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], TRUE);
                }
                else
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], FALSE);
                }
            }
        }
        else if (((NFLISTBOX*)g_evtlist)->up_btn == obj)
        {
            for (i=0; i<MAX_SHOW_LIST_CNT; i++)
            {
                if (strstr(nfui_nflabel_get_text((NFLABEL*)g_result_obj), nfui_listbox_get_text_of_box((NFLISTBOX*)g_evtlist, i, 0)) != NULL)
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], TRUE);
                }
                else
                {
                    nfui_check_button_set_active(g_list_chk_obj[i], FALSE);
                }
            }
        }

        for (i=0; i<MAX_SHOW_LIST_CNT; i++)
        {
            nfui_signal_emit((NFOBJECT*)g_list_chk_obj[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_match_case_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        g_opt_ai_generic->match_case = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_case_obj);
    }

    return FALSE;
}

static gboolean post_match_whole_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        g_opt_ai_generic->match_whole = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_whole_obj);
    }

    return FALSE;
}

static gboolean post_aibox_object_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
    }

    return FALSE;
}

static gboolean post_aibox_rule_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;
    }

    return FALSE;
}

static gboolean post_aibox_rule_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
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

        memcpy(g_opt_ai_generic, &g_org_opt_ai_generic, sizeof(OPT_AI_GENERIC_T));
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_btn_ok_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gint i;
    gint evt_cnt;
    gchar *evt_type;
    gchar buf[64];
    gchar **type;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        strcpy(g_opt_ai_generic->evt_type, nfui_nflabel_get_text((NFLABEL*)g_result_obj));
        for (i = 0; i < 2; i++) {
            if (nfui_nfobject_is_disabled(g_oper[i])) continue;

            g_opt_ai_generic->oper[i] = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_oper[i]);
        }
        //g_message("OK_TEST: %d %d", g_opt_ai_generic->oper[0], g_opt_ai_generic->oper[1]);

        for (i = 0 ; i < 3; i++)
        {
            strcpy(g_opt_ai_generic->text[i], nfui_nflabel_get_text((NFLABEL*)g_keyword[i]));
        }

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

gint VW_Search_By_Ai_Generic_Filter_Popup(NFWINDOW *parent, OPT_AI_GENERIC_T *opt)
{
    NFWINDOW *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y, pos_tmp_x;
    gint btn_w, btn_h;
    gint i;
    guint size_w, size_h;
    gchar buf[62];
    gchar *strOperator[] = {"AND", "OR"};
	guint lc_size[] = {380, };

    g_ret = 0;
    g_opt_ai_generic = opt;
    memcpy(&g_org_opt_ai_generic, g_opt_ai_generic, sizeof(OPT_AI_GENERIC_T));

    main_wnd = nftool_create_popup_window((NFWINDOW*)parent, (DISPLAY_ACTIVE_WIDTH-SAF_POPUP_SIZE_W)/2, (DISPLAY_ACTIVE_HEIGHT-SAF_POPUP_SIZE_H)/2, 
                                         SAF_POPUP_SIZE_W, SAF_POPUP_SIZE_H, "AI SEARCH FILTER", FALSE);
    nfui_nfobject_show(main_wnd);
    nfui_regi_post_event_callback(main_wnd, post_window_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = SAF_OBJ_START_POS_X;
    pos_y = SAF_OBJ_START_POS_Y;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);   

    pos_y += SAF_OBJ_SIZE_H + 1; 

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_result_obj = obj;

    //pos_y += SAF_OBJ_SIZE_H + 2; 

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 340, 200);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x + 40, pos_y);
    nfui_regi_post_event_callback(obj, post_dva_listbox_event_handler);
    nfui_regi_post_event_callback(((NFLISTBOX*)obj)->up_btn, post_dva_listbox_button_event_handler);
    nfui_regi_post_event_callback(((NFLISTBOX*)obj)->down_btn, post_dva_listbox_button_event_handler);
    g_evtlist = obj;
    _update_file_data();

    for (i=0; i<MAX_SHOW_LIST_CNT; i++)
    {
        obj = (NFOBJECT*)nfui_checkbutton_new(g_opt_ai_generic->evt_type_chk[i]);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_enable(obj);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (38-size_h)/2 + (41*i));
        nfui_regi_post_event_callback(obj, post_dva_select_chk_event_handler);
        g_list_chk_obj[i] = obj;
        g_message("%d", g_opt_ai_generic->evt_type_chk[i]);
    }

    pos_y += 200 + 2;

#if 0
    obj = nftool_normal_button_create_type3("SELECT", 123);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x + 380 - 123, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_dva_select_event_handler);
    g_select_obj = obj;
#elif 1
    if (strlen(nfui_nflabel_get_text((NFLABEL*)g_result_obj)) != 0) memcpy(buf, "DESELECT ALL", sizeof("DESELECT ALL"));
    else memcpy(buf, "SELECT ALL", sizeof("SELECT ALL"));

    obj = nftool_normal_button_create_type3(buf, 143);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x + 380 - 143, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_dva_deselect_all_event_handler);
    g_select_obj = obj;
#endif

    pos_x += 380 + 10;
    pos_y = SAF_OBJ_START_POS_Y;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ITEM KEYWORD", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += SAF_OBJ_SIZE_H + 1;
    pos_tmp_x = pos_x;
    
	for (i = 0; i < 3; i++)
    {
        if (i >= 1)
        {
            obj = (NFOBJECT *)nfui_combobox_new(strOperator, 2, g_opt_ai_generic->oper[i-1]);
            nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
            nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
            nfui_nfobject_set_size(obj, SKP_OPER_SIZE_W, SAF_OBJ_SIZE_H);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
            g_oper[i-1] = obj;
        }

        pos_x += SKP_OPER_SIZE_W + SKP_OBJ_GAP;

        obj = nfui_nflabel_new_text_box(g_opt_ai_generic->text[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
        nfui_nfobject_set_size(obj, SKP_KEYWORD_SIZE_W, SAF_OBJ_SIZE_H);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
        nfui_regi_post_event_callback(obj, post_label_keyword_event_handler);
        g_keyword[i] = obj;

        pos_x = pos_tmp_x;
        pos_y += 40 + SKP_OBJ_GAP;
    }
    _set_textbox_obj(0);

    pos_y += 40;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_button_set_active((NFCHECKBUTTON*)obj, g_opt_ai_generic->match_case);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_enable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (38-size_h)/2);
    nfui_regi_post_event_callback(obj, post_match_case_event_handler);
    g_match_case_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MATCH CASE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_set_size(obj, 317, 38);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);

    pos_y += 39;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_button_set_active((NFCHECKBUTTON*)obj, g_opt_ai_generic->match_whole);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_enable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y + (38-size_h)/2);
    nfui_regi_post_event_callback(obj, post_match_whole_event_handler);
    g_match_whole_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MATCH WHOLE WORD ONLY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_set_size(obj, 317, 38);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);

    pos_x = (SAF_POPUP_SIZE_W / 2) - SAF_BUTTON_SIZE_W - 1;
    pos_y = SAF_POPUP_SIZE_H - 55;
    
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
