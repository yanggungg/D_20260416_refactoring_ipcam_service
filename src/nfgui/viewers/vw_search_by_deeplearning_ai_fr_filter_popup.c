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


#define SAF_OBJ_START_POS_X         (15)
#define SAF_OBJ_START_POS_Y         (55)
#define SAF_OBJ_SIZE_H              (40)
#define SAF_OBJ_GAP                 (1)

#define SAF_BUTTON_SIZE_W           (150)

#define SAF_POPUP_SIZE_H            (530)
#define SAF_POPUP_SIZE_W            (820)

static NFWINDOW *g_curwnd;
static gint g_ret;
static OPT_AI_FR_T *g_fropt = NULL;

static NFOBJECT *g_name;
static NFOBJECT *g_grouplist;
static NFOBJECT *g_groupcombo;
static NFOBJECT *g_gender;



static gint _append_group_data(NFOBJECT *obj)
{
    DvaBxFaceZoneData data;
    gchar buf[1024], buf1[64];
    gchar *ptr = NULL;
    gint i, j;
    gint len = 0;
    NFCOMBOBOX *combo = (NFCOMBOBOX *)obj;
    gchar *pbuf = NULL;
	gchar *pnext = NULL;


    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(DvaBxFaceZoneData));
        memset(buf, 0x00, sizeof(buf));
        
        DAL_get_dvabx_face_grp(&data, i);
        strcpy(buf, data.zone[0].group_filter);
        
        ptr = strtok_r(buf, ",", &pnext);
        pbuf = pnext;
        if (!ptr) continue;

        memset(buf1, 0x00, sizeof(buf1));
        strcpy(buf1, ptr);

        while(ptr != NULL)
        {
            len = g_slist_length(combo->data);
            for(j=0; j<len; j++)
            {
                if(!strcmp(buf1, g_slist_nth_data(combo->data, j)))
                {
                    goto PASS;
                }
            }

            nfui_combobox_append_data((NFCOMBOBOX*)obj, buf1);
PASS:
            ptr = strtok_r(pbuf, ",", &pnext);
            pbuf = pnext;
            if (!ptr) break;

            memset(buf1, 0x00, sizeof(buf1));
            strcpy(buf1, ptr);
        }
    }
    return 0;
}

static gboolean post_group_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *grp[1], *added_grp;
    gint list_cnt = 0, i;
    gint is_add = 0;
    
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        grp[0] = nfui_combobox_get_value((NFCOMBOBOX*)obj);
        list_cnt = nfui_listbox_get_box_count((NFLISTBOX*)g_grouplist);

        for (i = 0; i < list_cnt; i++)
        {
            added_grp = nfui_listbox_get_text_of_list((NFLISTBOX*)g_grouplist, i, 0);
            if (strcmp(added_grp, grp[0]) == 0) {
                is_add = 1;
                break;
            }
        }
        
        if (is_add) return FALSE;

        nfui_listbox_set_text((NFLISTBOX*)g_grouplist, grp);
    }
    return FALSE;
}

static gboolean post_aibox_object_filter_edit_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        gchar filter_string[256]; 
        gchar exception_string[64]; 
        gchar buf[1024], buf1[64];
        gchar *ptr = NULL;
        gint i, j;
        gint len = 0;
        NFCOMBOBOX *combo = (NFCOMBOBOX *)g_groupcombo;
        gchar *pbuf = NULL;
        gchar *pnext = NULL;

        memset(filter_string, 0x00, sizeof(filter_string));
        memset(exception_string, 0x00, sizeof(exception_string));

        vw_dvabx_object_filter_add_popup_open(g_curwnd, 1300, 310, exception_string, filter_string, sizeof(filter_string)-sizeof(exception_string));

        nfui_combobox_remove_all((NFCOMBOBOX*)g_groupcombo);
        nfui_combobox_set_display_string((NFCOMBOBOX*)g_groupcombo, "Select group that will be searched.");
        nfui_signal_emit(g_groupcombo, GDK_EXPOSE, TRUE);
        _append_group_data(g_groupcombo);
        
        ptr = strtok_r(filter_string, ",", &pnext);
        pbuf = pnext;
        if (!ptr) 
        {
            if (strlen(filter_string) != 0) nfui_combobox_append_data((NFCOMBOBOX*)combo, filter_string);
            return FALSE;
        }

        memset(buf1, 0x00, sizeof(buf1));
        strcpy(buf1, ptr);

        while(ptr != NULL)
        {
            len = g_slist_length(combo->data);
            for(j=0; j<len; j++)
            {
                if(!strcmp(buf1, g_slist_nth_data(combo->data, j)))
                {
                    goto PASS;
                }
            }
            nfui_combobox_append_data((NFCOMBOBOX*)combo, buf1);
PASS:
            ptr = strtok_r(pbuf, ",", &pnext);
            pbuf = pnext;
            if (!ptr) break;

            memset(buf1, 0x00, sizeof(buf1));
            strcpy(buf1, ptr);
        }
        //nfui_combobox_append_data((NFCOMBOBOX*)g_groupcombo, filter_string);
    }

    return FALSE;
}

static gboolean post_aibox_object_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;
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

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 64, VKEY_MULTIKEYPD);

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
    gint grp_cnt;
    gchar *grplist;
    gchar buf[64];
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        grp_cnt = nfui_listbox_get_box_count((NFLISTBOX*)g_grouplist);
        g_fropt->filter_grp_cnt = grp_cnt;
 
        if (grp_cnt != -1)
            g_fropt->group = (gchar**)imalloc(sizeof(gchar*) * grp_cnt);
        else
            g_fropt->group = NULL;

        for (i = 0; i < grp_cnt; i++)
        {
            g_fropt->group[i] = (gchar*)imalloc(sizeof(gchar) * 64);
            memset(g_fropt->group[i], 0x00, (sizeof(gchar) * 64));
            
            grplist = nfui_listbox_get_text_of_list((NFLISTBOX*)g_grouplist, i, 0);
            strcpy(g_fropt->group[i], grplist);
        }

        strcpy(g_fropt->name, nfui_nflabel_get_text((NFLABEL*)g_name));
        strcpy(g_fropt->gender, nfui_combobox_get_value((NFCOMBOBOX*)g_gender));
        
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

gint VW_Search_By_Ai_FR_Filter_Popup(NFWINDOW *parent, OPT_AI_FR_T *opt)
{
    NFWINDOW *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint btn_w, btn_h;
    gint i;
    gchar buf[62];
	guint lc_size[] = {380, };

    g_ret = 0;
    g_fropt = opt;

    main_wnd = nftool_create_popup_window((NFWINDOW*)parent, (DISPLAY_ACTIVE_WIDTH-SAF_POPUP_SIZE_W)/2, (DISPLAY_ACTIVE_HEIGHT-SAF_POPUP_SIZE_H)/2, 
                                         SAF_POPUP_SIZE_W, SAF_POPUP_SIZE_H, "AI SEARCH FILTER", FALSE);
    nfui_nfobject_show(main_wnd);
    nfui_regi_post_event_callback(main_wnd, post_window_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = SAF_OBJ_START_POS_X;
    pos_y = SAF_OBJ_START_POS_Y;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);   

    pos_y += SAF_OBJ_SIZE_H + 1; 

	obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_group_combo_event_handler);
    g_groupcombo = obj;

    _append_group_data(obj);
	nfui_combobox_set_display_string((NFCOMBOBOX*)obj, "Select group that will be searched.");

    pos_y += SAF_OBJ_SIZE_H + 2; 
    
	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 380, 280);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_grouplist = obj;

    pos_y += 280 + 2;
    obj = nftool_normal_button_create_popup_type1("ADD GROUP FILTER", 240);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+140, pos_y);
    nfui_regi_post_event_callback(obj, post_aibox_object_filter_edit_event_handler);

    pos_x += 380 + 30;
    pos_y = SAF_OBJ_START_POS_Y;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += SAF_OBJ_SIZE_H + 1;
    
	obj = (NFOBJECT*)nfui_nflabel_new_text_box(opt->name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
	nfui_regi_post_event_callback(obj, post_label_keyword_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_name = obj;

    pos_y += 40 + 20;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GENDER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	
    pos_y += 40 + 1;

	obj = nfui_combobox_new(0, 2, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 10);
	nfui_nfobject_set_size(obj, 380, SAF_OBJ_SIZE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_gender = obj;

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "ALL");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "MALE");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "FEMALE");

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
