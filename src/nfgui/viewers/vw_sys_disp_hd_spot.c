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

#include "vw_sys_disp_hd_spot_edit.h"
#include "vw_sys_disp_hd_spot_setup.h"
#include "scm.h"
#include "evt.h"
#include "smt.h"
#include "vw.h"

#include "nf_sysman.h"

#if defined(_SUPPORT_HD_SPOT_4CH)
#define	MAX_SPOT_DATA 			(4)
#elif defined(_SUPPORT_HD_SPOT_2CH)
#define	MAX_SPOT_DATA 			(2)
#else
#define	MAX_SPOT_DATA 			(1)
#endif

#define	DS_ROWS					(MAX_SPOT_DATA + 1)
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

static SpotData spotdata[MAX_SPOT_DATA];
static SpotData org_spotdata[MAX_SPOT_DATA];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *active[MAX_SPOT_DATA];
static NFOBJECT *name[MAX_SPOT_DATA];
static NFOBJECT *created[MAX_SPOT_DATA];

//static NFOBJECT *fixed;

static gboolean _proc_escape(void *data)
{
	NFOBJECT *popup = (NFOBJECT *)data;

	nftool_remove_waitbox(popup);
	
    smt_set_service(SMT_SHUTDOWN);
    scm_shutdown_system(IRET_SCM_SHUTDOWN_SPOT_CHANGE);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

    nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

	return FALSE;
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

		for (i = 0; i < MAX_SPOT_DATA; i++)
		{
			if (name[i] == obj)	break;
		}

		if(i == MAX_SPOT_DATA)	return FALSE;

		if(nfui_check_button_get_active((NFCHECKBUTTON*)active[i]))
			spotdata[i].valid_mode = SPOT_MODE_VALID;
		else
			spotdata[i].valid_mode = SPOT_MODE_INVALID;

		ret = HD_SpotEditDlg_Open(g_curwnd, &spotdata[i]);

		if((ret == SPOT_EDIT_RET_SAVE) || (ret == SPOT_EDIT_RET_MODIFY))
		{
			if(spotdata[i].valid_mode == SPOT_MODE_VALID) {
				nfui_check_button_set_active(active[i], TRUE);
			}else if(spotdata[i].valid_mode == SPOT_MODE_INVALID)
				nfui_check_button_set_active(active[i], FALSE);

			nfui_nflabel_set_text(name[i], spotdata[i].title);

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

		g_memmove(spotdata, org_spotdata, sizeof(SpotData)*MAX_SPOT_DATA);

		for(i=0; i<MAX_SPOT_DATA; i++)
		{
			if(spotdata[i].valid_mode == SPOT_MODE_VALID)
				nfui_check_button_set_active(active[i], TRUE);
			else
				nfui_check_button_set_active(active[i], FALSE);
				
			nfui_nflabel_set_text(name[i], spotdata[i].title);
#if defined(_ENABLE_CREATEDBY)						
			nfui_nflabel_set_text(created[i], spotdata[i].createdby);
#endif
		}

		nfui_signal_emit(name[0]->parent, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
        gint ret_val, act_changed = 0;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		for(i = 0; i < MAX_SPOT_DATA; i++)
		{
			if(nfui_check_button_get_active(active[i]))
				spotdata[i].valid_mode = SPOT_MODE_VALID;
			else
				spotdata[i].valid_mode = SPOT_MODE_INVALID;
				
			g_stpcpy(spotdata[i].title, nfui_nflabel_get_text(name[i]));
#if defined(_ENABLE_CREATEDBY)			
			g_stpcpy(spotdata[i].createdby, nfui_nflabel_get_text(created[i]));
#endif			
		}

		if(memcmp(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA))
		{
            for(i = 0; i < MAX_SPOT_DATA; i++)
            {
            	if (spotdata[i].valid_mode != org_spotdata[i].valid_mode)
            	{
                    act_changed = 1;
                    break;        	                    
                }
            }

            if (act_changed)
            {
                ret_val = vw_mbox(g_curwnd, "NOTICE", IMBX_REBOOT_HD_ACTIVE_CHANGED, NFTOOL_MB_OKCANCEL);	

                if (ret_val == NFTOOL_MB_CANCEL)
                {
                    for(i = 0; i < MAX_SPOT_DATA; i++)
                    {
                        spotdata[i].valid_mode = org_spotdata[i].valid_mode;
                    
            			if(spotdata[i].valid_mode == SPOT_MODE_VALID)
            				nfui_check_button_set_active(active[i], TRUE);
            			else
            				nfui_check_button_set_active(active[i], FALSE);
                    }

                    act_changed = 0;

                	if(!memcmp(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA))
                		return FALSE;                    
                }
            }
	
			scm_put_log(CHANGE_DISP_SPOTSEQ, 0, 0);
			g_memmove(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA);
			DAL_set_HD_spot_data_all(spotdata, MAX_SPOT_DATA);

    		if (act_changed)	{
            	NFOBJECT *save_pop;
            	
				save_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");
				sysdisp_set_changeflag(1);

				DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
				g_timeout_add(1000, _proc_escape, save_pop);
    		}
    		else
    		{
    			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
    			sysdisp_set_changeflag(1);
    		}
			
		}
	}
	else if(evt->type == IRET_SCM_SHUTDOWN_SPOT_CHANGE)
	{
		scm_reboot_system(RR_SPOT_CHANGE, 0);
	}
	else if(evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, IRET_SCM_SHUTDOWN_SPOT_CHANGE);
	}
	
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		HD_Spot_tab_out_handler();
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

void init_HD_DispSpot_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixedTemp;
	NFOBJECT *ntb;
	NFOBJECT *obj;
	NFOBJECT *spot_btns[DSB_BUTTONS];

	GdkPixbuf *chk_image[NFCHECK_STATES];

	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
	const gchar *strTitle[] = {"ACTIVATION", "LIST", "CREATED BY"};
	guint width[NUM_DS_COLUMNS];
	//guint btn_x, btn_y, btn_space;
	guint chk_w, chk_h;
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);

	width[0] = 205;
	width[1] = 477;
	width[2] = 204;

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	/*
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, NF_COLOR_CODE_A8B6C4);
	nfui_nfobject_set_size(fixed, 1548, 931);
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)parent, fixed, 1, 1);
	*/

	ntb = nfui_nftable_new(NUM_DS_COLUMNS, DS_ROWS, DS_COL_SPACE, DS_ROW_SPACE, width, DS_LABEL_HEIGHT);
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

	memset(spotdata, 0x00, sizeof(SpotData)*MAX_SPOT_DATA);
	memset(org_spotdata, 0x00, sizeof(SpotData)*MAX_SPOT_DATA);

	for(i = 0; i < MAX_SPOT_DATA; i++)
	{
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_set_size(fixedTemp, width[0], DS_LABEL_HEIGHT);

		active[i] = nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(active[i]), NFCHECK_TYPE_NORMAL);
		nfui_check_button_sensitive(NF_CHECKBUTTON(active[i]), FALSE);
		nfui_check_get_size(active[i], &chk_w, &chk_h);
		nfui_nfobject_use_focus(active[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixedTemp, active[i], (width[0]-chk_w)/2, (DS_LABEL_HEIGHT-chk_w)/2);

		DAL_get_HD_spot_data(&spotdata[i], i);

		name[i] = nfui_nflabel_new_text_box(spotdata[i].title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)name[i], NFTEXTBOX_TYPE_INPUT);
        nfui_nfobject_support_multi_lang((NFOBJECT*)name[i], FALSE);
		nfui_regi_post_event_callback(name[i], post_name_label_event_handler);

		created[i] = nfui_nflabel_new_text_box(spotdata[i].createdby, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)created[i], NFTEXTBOX_TYPE_INPUT);
		nfui_nfobject_use_focus(created[i], NFOBJECT_FOCUS_OFF);

		nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, name[i], 1, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, created[i], 2, i+1);

		if(spotdata[i].valid_mode == SPOT_MODE_VALID)
			nfui_check_button_set_active(active[i], TRUE);
		else if(spotdata[i].valid_mode == SPOT_MODE_INVALID)
			nfui_check_button_set_active(active[i], FALSE);

		nfui_nfobject_show(fixedTemp);
		nfui_nfobject_show(active[i]);
		nfui_nfobject_show(name[i]);
#if defined(_ENABLE_CREATEDBY)		
		nfui_nfobject_show(created[i]);
#endif
	}

	/*
	btn_x = 770+196;
	btn_y = 943;
	btn_space = 192;

	for(i = 0; i < DSB_BUTTONS; i++)
	{
		if( i == 3 )
			obj = nftool_normal_button_create_type2(strButton[i], btn_space);
		else
			obj = nftool_normal_button_create_type1(strButton[i], btn_space);
			
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)parent, obj, btn_x, btn_y);
		spot_btns[i] = obj;
		
		btn_x += btn_space + 4;
	}
	*/

	spot_btns[0] = nftool_normal_button_create_type1(strButton[0], MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(spot_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(spot_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, spot_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	spot_btns[1] = nftool_normal_button_create_type1(strButton[1], MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(spot_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(spot_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, spot_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	spot_btns[2] = nftool_normal_button_create_type2(strButton[2], MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(spot_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(spot_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, spot_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(spot_btns[DSB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(spot_btns[DSB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(spot_btns[DSB_CLOSE], post_closebutton_event_handler);

	uxm_reg_imsg_event(spot_btns[DSB_APPLY], IRET_SCM_SHUTDOWN_SPOT_CHANGE);	

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA);	
}

gboolean HD_Spot_tab_out_handler()
{
	mb_type ret;
	gint i;
	gint ret_val, act_changed = 0;
	
	for(i = 0; i < MAX_SPOT_DATA; i++)
	{
		if(nfui_check_button_get_active(active[i]))
			spotdata[i].valid_mode = SPOT_MODE_VALID;
		else
			spotdata[i].valid_mode = SPOT_MODE_INVALID;
			
		g_stpcpy(spotdata[i].title, nfui_nflabel_get_text(name[i]));
		g_stpcpy(spotdata[i].createdby, nfui_nflabel_get_text(created[i]));
	}

	if(!memcmp(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
							 NFTOOL_MB_OKCANCEL);
							 
	if(ret == NFTOOL_MB_OK)
	{
        for(i = 0; i < MAX_SPOT_DATA; i++)
        {
        	if (spotdata[i].valid_mode != org_spotdata[i].valid_mode)
        	{
                act_changed = 1;
                break;
            }
        }

        if (act_changed)
        {
            ret_val = vw_mbox(g_curwnd, "NOTICE", IMBX_REBOOT_HD_ACTIVE_CHANGED, NFTOOL_MB_OKCANCEL);	

            if (ret_val == NFTOOL_MB_CANCEL)
            {
                for(i = 0; i < MAX_SPOT_DATA; i++)
                {
                    spotdata[i].valid_mode = org_spotdata[i].valid_mode;
                
        			if(spotdata[i].valid_mode == SPOT_MODE_VALID)
        				nfui_check_button_set_active(active[i], TRUE);
        			else
        				nfui_check_button_set_active(active[i], FALSE);
                }

                act_changed = 0;

            	if(!memcmp(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA))
            		return FALSE;
            }
        }
	
		scm_put_log(CHANGE_DISP_SPOTSEQ, 0, 0);
	
		g_memmove(org_spotdata, spotdata, sizeof(SpotData)*MAX_SPOT_DATA);
		DAL_set_HD_spot_data_all(spotdata, MAX_SPOT_DATA);

		sysdisp_set_changeflag(1);

		if (act_changed)	{
			DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);

			smt_set_service(SMT_SHUTDOWN);
			scm_shutdown_system(IRET_SCM_SHUTDOWN_SPOT_CHANGE);
			scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

			nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
			gtk_main();
		}		
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(spotdata, org_spotdata, sizeof(SpotData)*MAX_SPOT_DATA);

		for(i = 0; i < MAX_SPOT_DATA; i++)
		{
			if(spotdata[i].valid_mode == SPOT_MODE_VALID)
				nfui_check_button_set_active(active[i], TRUE);
			else
				nfui_check_button_set_active(active[i], FALSE);
				
			nfui_nflabel_set_text(name[i], spotdata[i].title);
#if defined(_ENABLE_CREATEDBY)
			nfui_nflabel_set_text(created[i], spotdata[i].createdby);
#endif
		}
	}
	return FALSE;
}






