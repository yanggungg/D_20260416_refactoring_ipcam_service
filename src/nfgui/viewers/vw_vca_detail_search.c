// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2012 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2012/07/18 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_vca_rule_config.c
 * @brief  This file contains VCA rule setup routines.
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "tools/ix_mem.h"
#include "tools/nf_ui_tool.h"
#include "viewers/objects/cw_slider.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/vw_colorsel.h"
#include "viewers/vw_vkeyboard.h"
#include "vw_tools.h"
#include "vw_vca_rule_config.h"
#include "nf_meta_data.h"


#include "ivca_def.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	SEARCH_WIN_X		(guint)((DISPLAY_ACTIVE_WIDTH - SEARCH_WIN_W) / 2)
#define	SEARCH_WIN_Y		(guint)((DISPLAY_ACTIVE_HEIGHT - SEARCH_WIN_H - 72) / 2)
#define	SEARCH_WIN_W		540
#define	SEARCH_WIN_H		580

#define	SEARCH_FXD_X		12
#define	SEARCH_FXD_Y		52
#define	SEARCH_FXD_W		(SEARCH_WIN_W - 12 * 2)
#define	SEARCH_FXD_H		(SEARCH_WIN_H - SEARCH_FXD_Y - 12)

#define	SEARCH_BTN1_X		((SEARCH_FXD_W - 10) / 2 - SEARCH_BTN_W)
#define	SEARCH_BTN2_X		((SEARCH_FXD_W + 10) / 2)
#define	SEARCH_BTN_Y		(SEARCH_FXD_H - SEARCH_FXD_Y)
#define	SEARCH_BTN_W		160

#define   VCA_EVENT_CNT		7

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFOBJECT *main_wnd;
static NFOBJECT *btn_ok;
static NFOBJECT *btn_cancel;

static NFOBJECT *ck_rule_id_all;
static NFOBJECT *ck_rule_id[IVCA_MAX_ZONES];
static NFOBJECT *ck_event_all;
static NFOBJECT *ck_event[VCA_EVENT_CNT];

static gboolean retval = FALSE;

static int rules_cnt;
static int rule_id_mask;
static int event_mask;

static int event_mask_check[VCA_EVENT_CNT] = {IVCA_ET_DIR_POS,IVCA_ET_DIR_NEG,IVCA_ET_ENTER,IVCA_ET_EXIT,IVCA_ET_STOPPED,IVCA_ET_REMOVED,IVCA_ET_LOITERED};

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static gboolean
post_ckbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean value;
	guint mask;
	gint i;
	gint value_all = 1;

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		value = nfui_check_button_get_active(NF_CHECKBUTTON(obj));
		for(i = 0 ; i < rules_cnt ;i++){
			if ( obj == ck_rule_id[i]){
				if(value)
					rule_id_mask |= 0x1 << i;
				else
					rule_id_mask &= ~(0x1 << i);
			}
		}

		for(i=0; i < VCA_EVENT_CNT ; i++){
			if ( obj == ck_event[i]){
				if(value)
					event_mask |= event_mask_check[i];
				else
					event_mask &= ~(event_mask_check[i]);
			}

		}
		
		if ( obj == ck_rule_id_all ){
			for(i = 0; i<rules_cnt;i++){
				nfui_check_button_set_active(ck_rule_id[i],value);
			}
			if(value)
				rule_id_mask = 0xffff;
			else
				rule_id_mask = 0;
		}
		else if(obj == ck_event_all ){
			for(i = 0; i<VCA_EVENT_CNT;i++){
				nfui_check_button_set_active(ck_event[i],value);
			}
			if(value)
				event_mask = 0xffff;
			else
				event_mask = 0;
		}

		
		for(i = 0 ; i < rules_cnt ;i++){
			if(!(rule_id_mask & (0x1 << i)))
				value_all = 0;
		}
		if(value_all)
			nfui_check_button_set_active(ck_rule_id_all,1);
		else
			nfui_check_button_set_active(ck_rule_id_all,0);
		
		value_all = 1;
		for(i = 0 ; i < VCA_EVENT_CNT ;i++){
			if(!(event_mask & event_mask_check[i]))
				value_all = 0;
		}
		if(value_all)
			nfui_check_button_set_active(ck_event_all,1);
		else
			nfui_check_button_set_active(ck_event_all,0);
		
		
	}
	return FALSE;
}	/* post_ckbtn_zone_cb(... */



static gboolean
post_mainwin_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_DELETE )
		gtk_main_quit();
	return FALSE;
}	/* post_mainwin_cb(... */

static gboolean
post_btn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		retval = obj == btn_ok;
		nfui_nfobject_destroy(main_wnd);
	}
	return FALSE;
}	/* post_btn_cb(... */


gboolean
VW_VCA_Detail_Search_Open(NFWINDOW *parent, gint* rule_id , gint* event , gint cnt)
{
	NFOBJECT *main_fixed, *fixed;
	gint i;
	gchar sbuf[16];
	static int p_rule_id_mask = 0xff;
	static int p_event_mask = 0xffff;
	gint rule_all = 1;
	gint event_all = 1;

	main_wnd = (NFOBJECT *)nftool_create_popup_window(parent,
			SEARCH_WIN_X, SEARCH_WIN_Y, SEARCH_WIN_W, SEARCH_WIN_H, "DETAIL SEARCH", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_mainwin_cb);

	main_fixed = ((NFWINDOW *)main_wnd)->child;

	fixed = vw_fixed_create(main_fixed, -1, 1,
			SEARCH_FXD_X, SEARCH_FXD_Y, SEARCH_FXD_W, SEARCH_FXD_H, NULL);

	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);
	
	rules_cnt = cnt;
	rule_id_mask = p_rule_id_mask;
	event_mask = p_event_mask;

	for(i = 0; i < rules_cnt; i++){
		if(!(rule_id_mask & (0x1 << i)))
			rule_all = 0;
	}
		
	/* Rule ID. */
	vw_label_create(fixed, "Rule ID", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 10, 80, 30, NULL);

	ck_rule_id_all = vw_ckbutton_lb_create(fixed, "All", rule_all,
			292, 130, 10, post_ckbtn_cb);
	
	for(i = 0; i < rules_cnt; i++){
		sprintf(sbuf, "Rule %d", i);
		ck_rule_id[i] = vw_ckbutton_lb_create(fixed, sbuf,
				rule_id_mask & (0x1 << i), 292, 10 + (i%4)*120, 60 + (i/4)*40, post_ckbtn_cb);

	}

	for(i = 0; i < VCA_EVENT_CNT; i++){
		if(!(event_mask & IVCA_ET_DIR_POS))
			event_all = 0;
		else if(!(event_mask & IVCA_ET_DIR_NEG))
			event_all = 0;
		else if(!(event_mask & IVCA_ET_ENTER))
			event_all = 0;
		else if(!(event_mask & IVCA_ET_EXIT))
			event_all = 0;
		else if(!(event_mask & IVCA_ET_STOPPED))
			event_all = 0;
		else if(!(event_mask & IVCA_ET_REMOVED))
			event_all = 0;
		else if(!(event_mask & IVCA_ET_LOITERED))
			event_all = 0;
	}

	/* Evnet */
	vw_label_create(fixed, "Event", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 10 + 200, 80, 30, NULL);

	ck_event_all= vw_ckbutton_lb_create(fixed, "All", event_all,
			292, 130, 10 + 200, post_ckbtn_cb);
	
	sprintf(sbuf, "Forward Direction");
	ck_event[0] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_DIR_POS), 292, 10, 10 + 200 + 50, post_ckbtn_cb);
	sprintf(sbuf, "Reverse Direction");
	ck_event[1] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_DIR_NEG), 292, 10 + 250, 10 + 200 + 50, post_ckbtn_cb);
	sprintf(sbuf, "Enter");
	ck_event[2] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_ENTER), 292, 10, 10 + 200 + 50 + 40, post_ckbtn_cb);
	sprintf(sbuf, "Exit");
	ck_event[3] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_EXIT), 292, 10 + 120, 10 + 200 + 50 + 40, post_ckbtn_cb);
	sprintf(sbuf, "Stopped");
	ck_event[4] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_STOPPED), 292, 10 + 250, 10 + 200 + 50 + 40, post_ckbtn_cb);
	sprintf(sbuf, "Removed");
	ck_event[5] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_REMOVED), 292, 10, 10 + 200 + 50 + 40 +40, post_ckbtn_cb);
	sprintf(sbuf, "Loitering");
	ck_event[6] = vw_ckbutton_lb_create(fixed, sbuf,
			(event_mask & IVCA_ET_LOITERED), 292, 10 + 250, 10 + 200 + 50 + 40 +40, post_ckbtn_cb);


	/* OK, CANCEL buttons. */
	btn_ok = vw_nmbutton_create(fixed, "SEARCH", 1, TRUE,
			SEARCH_BTN1_X, SEARCH_BTN_Y, SEARCH_BTN_W, post_btn_cb);
	btn_cancel = vw_nmbutton_create(fixed, "CANCEL", 1, TRUE,
			SEARCH_BTN2_X, SEARCH_BTN_Y, SEARCH_BTN_W, post_btn_cb);

	nfui_make_key_hierarchy((NFWINDOW *)main_wnd);
	nfui_set_key_focus(btn_ok, TRUE);

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

printf("rule_id %x event %x \n",rule_id_mask, event_mask);
	*rule_id = rule_id_mask;
	*event = event_mask;
	p_rule_id_mask = rule_id_mask;
	p_event_mask = event_mask;

	return retval;
}	/* VW_VCA_Rule_Config_Open(... */

