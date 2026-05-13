#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"

#include "vw_evt_act_main.h"
#include "vw_alarm_out_evt.h"
#include "vw_alarm_out_setup.h"
#include "var.h"

// FIXME
#include "nf_action.h"
#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "scm.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (CAM_ALARM_OUT / PAGE_FIXED_CNT)


enum {
	DURATION_TRANSPARENT = 0,
	DURATION_5_SEC,
	DURATION_10_SEC,
	DURATION_15_SEC,
	DURATION_20_SEC,
	DURATION_30_SEC,
	DURATION_40_SEC,
	DURATION_60_SEC,
	DURATION_120_SEC,
	DURATION_180_SEC,
	DURATION_300_SEC,
	DURATION_UNTIL_KEY,

	NUM_DURATIONS,
};

static EA_AlmOutData g_aod[CAM_ALARM_OUT];
static EA_AlmOutData g_oaod[CAM_ALARM_OUT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_operObj[CAM_ALARM_OUT];
static NFOBJECT *g_durObj[CAM_ALARM_OUT];
static NFOBJECT *g_tstObj[CAM_ALARM_OUT];
static NFOBJECT *g_nameObj[CAM_ALARM_OUT];
static NFOBJECT *g_tstallObj;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;


static void init_ao_data();
static void set_ao_data();
static void set_data_to_obj(gint expose);
static void get_data_from_obj();
static gint conv_duration_to_index(gint duration);
static gint conv_index_to_duration(gint index);
static void test_alarm_out(gint ch, gboolean active);
static void test_all_alarm_out(gboolean active);


static void init_ao_data()
{
	guint i;

	memset(g_aod, 0x00, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);
	memset(g_oaod, 0x00, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);
	
	for(i = 0; i < CAM_ALARM_OUT; i++)
		DAL_get_almOut_data(&g_aod[i], i);

	g_memmove(g_oaod, g_aod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);
}

static void set_ao_data()
{
	g_memmove(g_oaod, g_aod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);
	DAL_set_almOut_data_all(g_aod, CAM_ALARM_OUT);

	scm_put_log(CHANGE_EVT_ALARMOUT, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	gint i;

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		nfui_nflabel_set_text((NFLABEL*)g_nameObj[i], g_aod[i].name);
		if (expose)	nfui_signal_emit(g_nameObj[i], GDK_EXPOSE, FALSE);
	}

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_operObj[i], (guint)g_aod[i].oper);
		if (expose)	nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
	}

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_durObj[i], (guint)conv_duration_to_index(g_aod[i].duration));
		if (expose)	nfui_signal_emit(g_durObj[i], GDK_EXPOSE, TRUE);
	}
}

static void get_data_from_obj()
{
	gint i;
	gint index;
	gchar *pStr = NULL;

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		pStr = nfui_nflabel_get_text((NFLABEL*)g_nameObj[i]);
		if(pStr) strcpy(g_aod[i].name, pStr);
	}

	for(i = 0; i < CAM_ALARM_OUT; i++)
		g_aod[i].oper = (guchar)nfui_combobox_get_cur_index((NFCOMBOBOX*)g_operObj[i]);

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_durObj[i]);
		g_aod[i].duration = conv_index_to_duration(index);
	}
}

static gint conv_duration_to_index(gint duration)
{
	if(duration < 5)			return DURATION_TRANSPARENT;
	else if(duration < 10)		return DURATION_5_SEC;
	else if(duration < 15)		return DURATION_10_SEC;
	else if(duration < 20)		return DURATION_15_SEC;
	else if(duration < 30)		return DURATION_20_SEC;
	else if(duration < 40)		return DURATION_30_SEC;
	else if(duration < 60)		return DURATION_40_SEC;
	else if(duration < 120)		return DURATION_60_SEC;
	else if(duration < 180)		return DURATION_120_SEC;
	else if(duration < 300)		return DURATION_180_SEC;
	else if(duration < 9999)	return DURATION_300_SEC;
	else						return DURATION_UNTIL_KEY;

	return 0;
}

static gint conv_index_to_duration(gint index)
{
	if(index == DURATION_TRANSPARENT)		return 0;	
	else if(index == DURATION_5_SEC)		return 5;	
	else if(index == DURATION_10_SEC)		return 10;	
	else if(index == DURATION_15_SEC)		return 15;	
	else if(index == DURATION_20_SEC)		return 20;	
	else if(index == DURATION_30_SEC)		return 30;	
	else if(index == DURATION_40_SEC)		return 40;	
	else if(index == DURATION_60_SEC)		return 60;	
	else if(index == DURATION_120_SEC)		return 120;	
	else if(index == DURATION_180_SEC)		return 180;	
	else if(index == DURATION_300_SEC)		return 300;	
	else if(index == DURATION_UNTIL_KEY)	return 9999;	
	else									return 0;	

	return 0;
}

static void test_alarm_out(gint ch, gboolean active)
{
	if(g_aod[ch].oper) nf_action_relay_test(active, ch, TRUE);
	else		   	   nf_action_relay_test(active, ch, FALSE);

	if(active) {
	    nfui_nfobject_disable(g_operObj[ch]);
    }
	else
	{
#if defined(_NOT_SUPPORT_NO_NC)
        nfui_nfobject_disable(g_operObj[ch]);
#else 
        nfui_nfobject_enable(g_operObj[ch]);
#endif  
	}

	if(active) 		   nfui_nfobject_disable(g_durObj[ch]);
	else	           nfui_nfobject_enable(g_durObj[ch]);

	nfui_signal_emit(g_operObj[ch], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_durObj[ch], GDK_EXPOSE, TRUE);

	if(nfui_check_button_get_active(NF_CHECKBUTTON(g_tstallObj))) 
		nfui_check_button_set_active(NF_CHECKBUTTON(g_tstallObj), FALSE);
}

static void test_all_alarm_out(gboolean active)
{
	gint i;

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		if(active && nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[i])))
			continue;
		else if(!active && !nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[i])))
			continue;

		if(g_aod[i].oper) nf_action_relay_test(active, i, TRUE);
		else		   	  nf_action_relay_test(active, i, FALSE);

		if(active) {
		    nfui_nfobject_disable(g_operObj[i]);
	    }
		else
		{
#if defined(_NOT_SUPPORT_NO_NC)
            nfui_nfobject_disable(g_operObj[i]);
#else 
            nfui_nfobject_enable(g_operObj[i]);
#endif 
		}

		if(active) 		  nfui_nfobject_disable(g_durObj[i]);
		else	   		  nfui_nfobject_enable(g_durObj[i]);
	}

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_durObj[i], GDK_EXPOSE, TRUE);

		nfui_check_button_set_active(NF_CHECKBUTTON(g_tstObj[i]), active);
	}
}

static gboolean post_test_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gboolean act = FALSE;
		gint ch = -1;

		ch = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "alarm out ch"));
		g_message("%s :::::::::::::::::::: channel %d ", __FUNCTION__, ch);
		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

		get_data_from_obj();

		if(ch == CAM_ALARM_OUT) test_all_alarm_out(act);
		else 	   				  test_alarm_out(ch, act);
	}

	return FALSE;
}

static gboolean post_name_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *title;
	gchar *strTemp = NULL;
	guint x, y;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    mb_type ret = NFTOOL_MB_OK;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(kpid == KEYPAD_ENTER) {
			nfui_nfobject_get_offset(obj, &x, &y);
			y += obj->height;
		}else {
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

        title = nfui_nflabel_get_text((NFLABEL*)obj);

        if (!ifn_is_all_itxstyle_title(title))
        {
            vw_mbox(g_curwnd, "ERROR", IMBX_UNSUPPORTED_LETTER, NFTOOL_MB_OK);        
            return FALSE;
        }

		strTemp = VirtualKey_Open(g_curwnd, title, x, y, 16, VKEY_ITXSTYLE_TITLE);

		if(strTemp) 
		{
			if(strlen(strTemp) <= 0) 
				ret = nftool_mbox(g_curwnd, "NOTICE", "There is no input.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

            if (ret == NFTOOL_MB_OK)
            {
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
    			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
            }

			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_oper_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint idx;
		gint i;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		for(i=0; i<CAM_ALARM_OUT; i++) 
		{
#if defined(_NOT_SUPPORT_NO_NC)
	        if( i < CAM_ALARM_OUT) continue;
#endif
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_operObj[i], (guint)idx);
        }
		for(i=0; i<CAM_ALARM_OUT; i++) 
			nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
	}
	return FALSE;
}

static gboolean post_dur_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint idx;
		gint i;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		for(i=0; i<CAM_ALARM_OUT; i++) 
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_durObj[i], (guint)idx);

		for(i=0; i<CAM_ALARM_OUT; i++) 
			nfui_signal_emit(g_durObj[i], GDK_EXPOSE, TRUE);
	}
	return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    	
		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(g_aod, g_oaod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);

		set_data_to_obj(1);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		get_data_from_obj();

		if(memcmp(g_oaod, g_aod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT)) {
			set_ao_data();

			g_memmove(g_oaod, g_aod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			event_act_data_changed(TRUE);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		VW_AlarmOut_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_AlarmOut_SetupPage(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

	gchar buf[128];
	gchar *strCol[] = {"CAM", "NAME", "OPERATION", "DURATION"};
	//gchar *strCol[] = {"CAM", "NAME", "TYPE", "OPERATION", "DURATION"};
	gchar *strOper[] = {"N/O", "N/C"};
	gchar *strDur[] = {"TRANSPARENT", "5 SEC", "10 SEC", "15 SEC", "20 SEC", 
		"30 SEC", "40 SEC", "60 SEC", "120 SEC", 
		"180 SEC", "300 SEC", "UNTIL KEY"};

	guint tbl_w[5] = {164, 164, 164, 184, 164};
	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	gint i;

	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];

	gint pos_x, pos_y;
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

	g_curwnd = nfui_nfobject_get_top(parent);

	// init data
	init_ao_data();

	chk_inact_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_NI_164", 164, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	chk_inact_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_inact_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_inact_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	chk_act_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_SI_164", 164, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_act_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_act_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_act_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

    pos_y = 13;
    
	for(i = 0; i < 4; i++) {
		if ( i == 0) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, tbl_w[i], 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_show(obj);
			
			pos_x = 27;
		}
		else if(i == 1) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, tbl_w[i], 40);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			
			pos_x += tbl_w[i-1] + 2;
		}
		else {
			if(i == 2)	obj = nfui_combobox_new(strOper, 2, 0);
			else 	  	obj = nfui_combobox_new(strDur, NUM_DURATIONS, 0);
			nfui_combobox_set_display_string(NF_COMBOBOX(obj), strCol[i]);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_2);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, tbl_w[i], 40);
			nfui_nfobject_show(obj);

			if(i == 2) 	nfui_regi_post_event_callback(obj, post_oper_all_event_handler);
			else		nfui_regi_post_event_callback(obj, post_dur_all_event_handler);
			
			pos_x += tbl_w[i-1] + 2;
		}

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	}

	pos_x += 184 + 2;
	
	g_tstallObj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(g_tstallObj), NFCHECK_TYPE_DEF);
	nfui_check_set_text(NF_CHECKBUTTON(g_tstallObj), "TEST ALL");
	nfui_check_button_set_inactive_image(NF_CHECKBUTTON(g_tstallObj), chk_inact_img);
	nfui_check_button_set_active_image(NF_CHECKBUTTON(g_tstallObj), chk_act_img);	
	nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(g_tstallObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
	nfui_check_set_active_pango_font(NF_CHECKBUTTON(g_tstallObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);		
	nfui_regi_post_event_callback(g_tstallObj, post_test_event_handler);
	nfui_nfobject_set_size(g_tstallObj, 164, 40);
	nfui_nfobject_show(g_tstallObj);

	nfui_nfobject_set_data(g_tstallObj, "alarm out ch", GINT_TO_POINTER(CAM_ALARM_OUT));
	nfui_nffixed_put((NFFIXED*)content_fixed, g_tstallObj, pos_x, pos_y);

    pos_x = 27;
    pos_y += 42;
	
    size_w = 0;
    for (i = 0; i < 5; i++) {
        size_w += tbl_w[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, pos_x, pos_y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(5, ROW_CNT_PER_PAGE, 2, 1, tbl_w, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

	for(i = 0; i < CAM_ALARM_OUT; i++) 
	{
    	memset(buf, 0x00, sizeof(buf));
		g_sprintf(buf, "%d", i + 1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

    page_num = row_num = 0;

	for(i = 0; i < CAM_ALARM_OUT; i++) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_aod[i].name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_regi_post_event_callback(obj, post_name_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 1, row_num);

		g_nameObj[i] = obj;

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

    page_num = row_num = 0;

	for(i = 0; i < CAM_ALARM_OUT; i++) 
	{
		g_operObj[i] = nfui_combobox_new(strOper, 2, (gint)g_aod[i].oper);
		nfui_combobox_set_skin_type(NF_COMBOBOX(g_operObj[i]), NFCOMBOBOX_TYPE_SUBTAB_1);
		nfui_combobox_set_align(NF_COMBOBOX(g_operObj[i]), NFALIGN_CENTER, 0);
#if defined(_NOT_SUPPORT_NO_NC)
        nfui_nfobject_disable(g_operObj[i]);
#endif  
		    nfui_nfobject_show(g_operObj[i]);

		nfui_nftable_attach((NFTABLE*)page_ntb[page_num], g_operObj[i], 2, row_num);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

    page_num = row_num = 0;

	for(i = 0; i < CAM_ALARM_OUT; i++) 
	{
		g_durObj[i] = nfui_combobox_new(strDur, NUM_DURATIONS, (guint)conv_duration_to_index(g_aod[i].duration));
		nfui_combobox_set_skin_type(NF_COMBOBOX(g_durObj[i]), NFCOMBOBOX_TYPE_SUBTAB_1);
		nfui_combobox_set_align(NF_COMBOBOX(g_durObj[i]), NFALIGN_CENTER, 0);
		nfui_nfobject_show(g_durObj[i]);
		nfui_nftable_attach((NFTABLE*)page_ntb[page_num], g_durObj[i], 3, row_num);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

    page_num = row_num = 0;

	for(i = 0; i < CAM_ALARM_OUT; i++) 
	{
		g_tstObj[i] = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(g_tstObj[i]), NFCHECK_TYPE_DEF);
		nfui_check_set_text(NF_CHECKBUTTON(g_tstObj[i]), "TEST");
		nfui_check_button_set_inactive_image(NF_CHECKBUTTON(g_tstObj[i]), chk_inact_img);
		nfui_check_button_set_active_image(NF_CHECKBUTTON(g_tstObj[i]), chk_act_img);	
		nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(g_tstObj[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
		nfui_check_set_active_pango_font(NF_CHECKBUTTON(g_tstObj[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);		
		nfui_regi_post_event_callback(g_tstObj[i], post_test_event_handler);
		nfui_nfobject_set_size(g_tstObj[i], 164, 40);
		nfui_nfobject_show(g_tstObj[i]);

		nfui_nfobject_set_data(g_tstObj[i], "alarm out ch", GINT_TO_POINTER(i));
		nfui_nftable_attach((NFTABLE*)page_ntb[page_num], g_tstObj[i], 4, row_num);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_alarm_out_data_changed()
{
	gboolean act = FALSE;
	mb_type mb_ret = NFTOOL_MB_NONE;
	gint i;

	for(i = 0; i < CAM_ALARM_OUT; i++) {
		act = nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[i]));

		if(act) 
		{
			if(mb_ret == NFTOOL_MB_NONE)
				mb_ret = nftool_mbox(g_curwnd, "NOTICE", "For all channels to stop the alarm test.", NFTOOL_MB_OK);

			if(g_aod[i].oper) nf_action_relay_test(FALSE, i, TRUE);
			else              nf_action_relay_test(FALSE, i, FALSE);

#if defined(_NOT_SUPPORT_NO_NC)
            nfui_nfobject_disable(g_operObj[i]);
#else 
            nfui_nfobject_enable(g_operObj[i]);
#endif
			nfui_nfobject_enable(g_durObj[i]);
		}
		
		NF_CHECKBUTTON(g_tstallObj)->checked = FALSE;
		NF_CHECKBUTTON(g_tstObj[i])->checked = FALSE;
	}

	get_data_from_obj();

	if(!memcmp(g_oaod, g_aod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT))
		return FALSE;

	return TRUE;
}

void save_alarm_out_data()
{
	set_ao_data();
}

void restore_alarm_out_data()
{
	g_memmove(g_aod, g_oaod, sizeof(EA_AlmOutData) * CAM_ALARM_OUT);

	set_data_to_obj(0);
}
