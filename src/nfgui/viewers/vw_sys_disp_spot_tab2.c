#include "nf_afx.h"
#include "nfdal.h"

#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_sys_disp_main.h"

#include "vw_sys_disp_spot_tab1.h"
#include "vw_sys_disp_spot_edit.h"
#include "vw_sys_disp_spot_setup.h"
#include "scm.h"


#define PORT_MAX                (4)

#define	DS_COL_SPACE			(2)
#define	DS_ROW_SPACE			(1)

#define	DS_TABLE_LEFT			(28)
#define	DS_TABLE_TOP			(42)

#define	DS_LABEL_HEIGHT			(40)

//#define _ENABLE_CREATEDBY

enum {
	DS_ACTIVATION = 0,
	DS_LIST,
	DS_CREATED_BY,
	NUM_DS_COLUMNS
};

enum {
	DSB_CANCEL = 0,
	DSB_APPLY,
	DSB_CLOSE,
	DSB_BUTTONS
};

static SpotData spotData[PORT_MAX];
static SpotData org_spotData[PORT_MAX];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *active[PORT_MAX];
static NFOBJECT *name[PORT_MAX];
static NFOBJECT *created[PORT_MAX];
static NFOBJECT *edit[PORT_MAX];

static guint g_spt_start;
static guint g_port_cnt;
static guint g_max_div;
static guint g_output_ch;

static void set_split_data()
{
	gint i;

    g_memmove(org_spotData, spotData, sizeof(SpotData)*g_port_cnt);
	for(i = 0; i < g_port_cnt; i++)
	{
		DAL_set_spot_data(spotData[i], i+g_spt_start);
    }
    scm_put_log(CHANGE_DISP_SPOTSEQ, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	gint i;

	if (expose)
    {
        for(i = 0; i < g_port_cnt; i++)
        {
			if(spotData[i].valid_mode == SPOT_MODE_VALID)
				nfui_check_button_set_active(active[i], TRUE);
			else
				nfui_check_button_set_active(active[i], FALSE);
				
			nfui_nflabel_set_text(name[i], spotData[i].title);
#if defined(_ENABLE_CREATEDBY)
			nfui_nflabel_set_text(created[i], spotData[i].createdby);
#endif
		}
    }
    else
    {
        for(i = 0; i < g_port_cnt; i++)
        {
			if(spotData[i].valid_mode == SPOT_MODE_VALID)
				nfui_check_button_set_active_no_expose(active[i], TRUE);
			else
				nfui_check_button_set_active_no_expose(active[i], FALSE);
				
			nfui_nflabel_set_text(name[i], spotData[i].title);
#if defined(_ENABLE_CREATEDBY)
			nfui_nflabel_set_text(created[i], spotData[i].createdby);
#endif
		}

    }
}

static void get_data_from_obj()
{
	gint i;

	for(i = 0; i < g_port_cnt; i++)
	{
		if(nfui_check_button_get_active(active[i]))
			spotData[i].valid_mode = SPOT_MODE_VALID;
		else
			spotData[i].valid_mode = SPOT_MODE_INVALID;
			
		g_stpcpy(spotData[i].title, nfui_nflabel_get_text(name[i]));
#if defined(_ENABLE_CREATEDBY)			
		g_stpcpy(spotData[i].createdby, nfui_nflabel_get_text(created[i]));
#endif
	}
}

static gboolean post_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		guint i, j;
		guint ret=0;

  	   	if((evt->type == GDK_2BUTTON_PRESS) && (evt->button.button == MOUSE_RIGTH_BUTTON)) {
			return FALSE;
		}

		for (i = 0; i < g_port_cnt; i++)
		{
			if (name[i] == obj)   break;
		}

		if(i == g_port_cnt)	return FALSE;

		if(nfui_check_button_get_active((NFCHECKBUTTON*)active[i]))
			spotData[i].valid_mode = SPOT_MODE_VALID;
		else
			spotData[i].valid_mode = SPOT_MODE_INVALID;

		ret = SpotEditDlg_Open(g_curwnd, &spotData[i], g_max_div, g_output_ch);

		if((ret == SPOT_EDIT_RET_SAVE) || (ret == SPOT_EDIT_RET_MODIFY))
		{
			if(spotData[i].valid_mode == SPOT_MODE_VALID) {
				nfui_check_button_set_active(active[i], TRUE);
			}else if(spotData[i].valid_mode == SPOT_MODE_INVALID)
				nfui_check_button_set_active(active[i], FALSE);

			nfui_nflabel_set_text(name[i], spotData[i].title);

			nfui_signal_emit(active[i]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(created[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_editbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i, j;
		guint ret=0;

  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		for (i = 0; i < g_port_cnt; i++)
		{
			if (edit[i] == obj)   break;
		}

		if(i == g_port_cnt)	return FALSE;

		if(nfui_check_button_get_active((NFCHECKBUTTON*)active[i]))
			spotData[i].valid_mode = SPOT_MODE_VALID;
		else
			spotData[i].valid_mode = SPOT_MODE_INVALID;

		ret = SpotEditDlg_Open(g_curwnd, &spotData[i], g_max_div, g_output_ch);

		if((ret == SPOT_EDIT_RET_SAVE) || (ret == SPOT_EDIT_RET_MODIFY))
		{
			if(spotData[i].valid_mode == SPOT_MODE_VALID) {
				nfui_check_button_set_active(active[i], TRUE);
			}else if(spotData[i].valid_mode == SPOT_MODE_INVALID)
				nfui_check_button_set_active(active[i], FALSE);

			nfui_nflabel_set_text(name[i], spotData[i].title);

			nfui_signal_emit(active[i]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(created[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(spotData, org_spotData, sizeof(SpotData)*g_port_cnt);

		set_data_to_obj(1);
		nfui_signal_emit(name[0]->parent, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		get_data_from_obj();

		if(memcmp(org_spotData, spotData, sizeof(SpotData)*g_port_cnt))
		{
			set_split_data();

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			sysdisp_set_changeflag(1);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
        Spot_multi_tab_out_handler();
        SystemSetupDisp_Destroy(obj);
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

void init_DispSpot_tab2_page(NFOBJECT *parent, guint spt_start, gint port_cnt, gint max_div, guint output_ch)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixedTemp;
	NFOBJECT *ntb;
	NFOBJECT *obj;

	GdkPixbuf *chk_image[NFCHECK_STATES];

	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
	const gchar *strTitle[] = {"ACTIVATION", "LIST", "CREATED BY"};
	guint width[NUM_DS_COLUMNS];
	//guint btn_x, btn_y, btn_space;
	guint chk_w, chk_h;
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);
	g_spt_start = spt_start;
	g_port_cnt = port_cnt;
	g_max_div = max_div;
    g_output_ch = output_ch;
    
	
	width[0] = 205;
	width[1] = 477;
	width[2] = 100;

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	ntb = nfui_nftable_new(NUM_DS_COLUMNS, port_cnt+1, DS_COL_SPACE, DS_ROW_SPACE, width, DS_LABEL_HEIGHT);
	nfui_nfobject_show(ntb);

	nfui_nffixed_put(content_fixed, ntb, DS_TABLE_LEFT, DS_TABLE_TOP);

	for(i=0; i<NUM_DS_COLUMNS; i++)
	{
		obj = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));		
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		if(i != 2) nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, i, 0);
	}

	memset(spotData, 0x00, sizeof(SpotData)*PORT_MAX);
	memset(org_spotData, 0x00, sizeof(SpotData)*PORT_MAX);

	for(i = 0; i < g_port_cnt; i++)
	{
        // init data
		DAL_get_spot_data(&spotData[i], i+spt_start);

		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_set_size(fixedTemp, width[0], DS_LABEL_HEIGHT);

		active[i] = nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(active[i]), NFCHECK_TYPE_NORMAL);
		nfui_check_button_sensitive(NF_CHECKBUTTON(active[i]), FALSE);
		nfui_check_get_size(active[i], &chk_w, &chk_h);
		nfui_nfobject_use_focus(active[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixedTemp, active[i], (width[0]-chk_w)/2, (DS_LABEL_HEIGHT-chk_w)/2);


		name[i] = nfui_nflabel_new_text_box(spotData[i].title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)name[i], NFTEXTBOX_TYPE_INPUT);
        nfui_nfobject_support_multi_lang((NFOBJECT*)name[i], FALSE);
		nfui_regi_post_event_callback(name[i], post_name_label_event_handler);

		edit[i] = nftool_normal_button_create_type3("EDIT", 100);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(edit[i]), NFALIGN_CENTER, 0);	
		nfui_regi_post_event_callback(edit[i], post_editbtn_event_handler);

		created[i] = nfui_nflabel_new_text_box(spotData[i].createdby, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)created[i], NFTEXTBOX_TYPE_INPUT);
		nfui_nfobject_use_focus(created[i], NFOBJECT_FOCUS_OFF);

		nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, name[i], 1, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, edit[i], 2, i+1);
//		nfui_nftable_attach((NFTABLE*)ntb, created[i], 2, i+1);

		if(spotData[i].valid_mode == SPOT_MODE_VALID)
			nfui_check_button_set_active(active[i], TRUE);
		else if(spotData[i].valid_mode == SPOT_MODE_INVALID)
			nfui_check_button_set_active(active[i], FALSE);

		nfui_nfobject_show(fixedTemp);
		nfui_nfobject_show(active[i]);
		nfui_nfobject_show(name[i]);
		nfui_nfobject_show(edit[i]);
#if defined(_ENABLE_CREATEDBY)		
		nfui_nfobject_show(created[i]);
#endif
	}

	obj = nftool_normal_button_create_type1(strButton[0], MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

	obj = nftool_normal_button_create_type1(strButton[1], MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

	obj = nftool_normal_button_create_type2(strButton[2], MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(org_spotData, spotData, sizeof(SpotData)*g_port_cnt);	
}


gboolean check_DispSpot_tab2_changed()
{
	get_data_from_obj();
	
    if(!memcmp(org_spotData, spotData, sizeof(SpotData)*g_port_cnt))
		return FALSE;
			
    return TRUE;
}

void save_DispSpot_tab2_data()
{
    set_split_data();
}

void restore_DispSpot_tab2_data()
{
    g_memmove(spotData, org_spotData, sizeof(SpotData)*g_port_cnt);
    set_data_to_obj(0);
}

