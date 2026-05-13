#include "nf_afx.h"

#include "cmm.h"
#include "uxm.h"
#include "iux_msg.h"
#include "scm.h"

#include "modules/vfs.h"
#include "modules/var.h"

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
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"

#include "vw_disk_main.h"
#include "vw_disk_operation.h"
#include "vw_delete.h"
#include "vw_vkeyboard.h"
#include "ssm.h"
#include "smt.h"
#include "vsm.h"

#define PROG_POS_X								((DISPLAY_ACTIVE_WIDTH - 653) / 2)
#define PROG_POS_Y								((DISPLAY_ACTIVE_HEIGHT - 106) / 2)


enum {
    TIME_LIMIT_OFF,
    TIME_LIMIT_HOUR,
    TIME_LIMIT_DAY,
    TIME_LIMIT_WEEK,

	NUM_TIME_LIMITS,
};

static NFWINDOW *g_curwnd = 0;
static DiskManageData g_dmd;
static DiskManageData g_odmd;

static NFOBJECT *g_wmodeObj;
static NFOBJECT *g_rtlObj;
static NFOBJECT *g_numObj;
static NFOBJECT *g_waitPop = NULL;
static NFOBJECT *cancel_btn = NULL;
static NFOBJECT *g_ccobj;

static void init_do_data();
static void set_do_data();
static void get_data_from_obj();
static guint prvNumberToRTL(guint timelimit, guint timeType);
static gint prvRTLToNumber(guint timelimit, guint timeType);
static void change_disk_operation();


static void init_do_data()
{
	memset(&g_dmd, 0x00, sizeof(DiskManageData));
	memset(&g_odmd, 0x00, sizeof(DiskManageData));

	DAL_get_diskManage_data(&g_dmd);

	g_memmove(&g_odmd, &g_dmd, sizeof(DiskManageData));
}
/*
static int _put_disk_log(int type)
{
	switch(type) {
	case 0:
		scm_put_log(CHANGE_TO_RTL, 0, 0);
		break;
		
	case 1:
		scm_put_log(CHANGE_TO_OW, 0, 0);
		break;

	case 2:
		scm_put_log(CHANGE_TO_ONCE, 0, 0);
		break;
	}

	return 0;
}*/

static void set_do_data()
{
	g_memmove(&g_odmd, &g_dmd, sizeof(DiskManageData));

	DAL_set_diskManage_data(&g_dmd);

	scm_put_log(CHANGE_STRG_OP, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	nfui_spin_button_set_index((NFSPINBUTTON*)g_wmodeObj, (guint)g_dmd.overWrite);
	if (expose) nfui_signal_emit(g_wmodeObj, GDK_EXPOSE, TRUE);

    nfui_nflabel_set_number(g_numObj, (guint)prvRTLToNumber(g_dmd.timeLimit, g_dmd.timeType));
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_rtlObj, g_dmd.timeType);
	
	if(ivsc.dfunc.support_usrdef_holiday)
		nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ccobj, g_dmd.custom_cal);
	
	if(g_dmd.overWrite) 
	{
		if(g_dmd.timeType == TIME_LIMIT_OFF) {
			nfui_nfobject_disable(g_numObj);
            
			if(ivsc.dfunc.support_usrdef_holiday)
				nfui_nfobject_disable(g_ccobj);
        }
        else {
		    nfui_nfobject_enable(g_numObj);

			if(ivsc.dfunc.support_usrdef_holiday)
				nfui_nfobject_enable(g_ccobj);
	    }
		nfui_nfobject_enable(g_rtlObj);
	}
	else			 	
	{
		nfui_nfobject_disable(g_numObj);
		nfui_nfobject_disable(g_rtlObj);
		
		if(ivsc.dfunc.support_usrdef_holiday)
			nfui_nfobject_disable(g_ccobj);
	}
	if (expose) 
	{		
		nfui_signal_emit(g_numObj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_rtlObj, GDK_EXPOSE, TRUE);
		
		if(ivsc.dfunc.support_usrdef_holiday)
			nfui_signal_emit(g_ccobj, GDK_EXPOSE, TRUE);
	}
}

static void get_data_from_obj()
{
	gint num;

	g_dmd.overWrite = nfui_spin_button_get_index((NFSPINBUTTON*)g_wmodeObj);

    num = nfui_nflabel_get_number((NFLABEL*)g_numObj);
	g_dmd.timeType = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_rtlObj);
	g_dmd.timeLimit = prvNumberToRTL(num ,g_dmd.timeType);

	if(ivsc.dfunc.support_usrdef_holiday)
		g_dmd.custom_cal = nfui_check_button_get_active((NFCHECKBUTTON*)g_ccobj);
}

static guint prvNumberToRTL(guint timelimit, guint timeType)
{
	guint time = 0;
	guint ret = 0;

	switch(timeType)
	{
		case TIME_LIMIT_OFF:	time = 0;		break;
		case TIME_LIMIT_HOUR:	time = 3600;		break;
		case TIME_LIMIT_DAY:	time = 86400;	break;
		case TIME_LIMIT_WEEK:	time = 604800;	break;
		default:				time = 0;		break;
	}
	if(time != 0)
		ret = timelimit * time;

	return ret;
}

static gint prvRTLToNumber(guint timelimit, guint timeType)
{
	gint time = 0;
	guint ret = 0;

    
	switch(timeType)
	{
		case TIME_LIMIT_OFF:	time = 0;		break;
		case TIME_LIMIT_HOUR:	time = 3600;	    break;
		case TIME_LIMIT_DAY:	time = 86400;	break;
		case TIME_LIMIT_WEEK:	time = 604800;	break;
		default:				time = 0;		break;
	}
	if(time != 0)
   		ret = timelimit/time;

	return ret;
}

static void change_disk_operation()
{                                  
    gchar str[128];

    memset(str, 0x00, sizeof(str));

    sprintf(str,"(%s)",lookup_string("This may take several minutes depending on HDD capacity."));
    
	g_waitPop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", str);
	smt_set_service(SMT_DISK_RESTART);
	scm_restart_service(IRET_RESTART_SERVICE, RS_DISK);
	gtk_main();
}

static gboolean post_write_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) 
	{
		gint idx = -1;

		idx = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

		if(idx > 0) 
		{
    		if(g_dmd.timeType == TIME_LIMIT_OFF) {
                nfui_nfobject_disable(g_numObj);
                
				if(ivsc.dfunc.support_usrdef_holiday)
					nfui_nfobject_disable(g_ccobj);
            }
            else {
    		    nfui_nfobject_enable(g_numObj);
    		    
				if(ivsc.dfunc.support_usrdef_holiday)
					nfui_nfobject_enable(g_ccobj);
    	    }
    	    
		    nfui_nfobject_enable(g_rtlObj);			// overwrite mode
		}
		else
		{
    		nfui_nfobject_disable(g_numObj);
		    nfui_nfobject_disable(g_rtlObj);		// once mode
		    
			if(ivsc.dfunc.support_usrdef_holiday)
				nfui_nfobject_disable(g_ccobj);
        }
        nfui_signal_emit(g_numObj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_rtlObj, GDK_EXPOSE, TRUE);
		
		if(ivsc.dfunc.support_usrdef_holiday)
			nfui_signal_emit(g_ccobj, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(cancel_btn);
		nfui_signal_emit(cancel_btn, GDK_EXPOSE, FALSE);
	}

	return FALSE;
}

static gboolean post_format_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT* prog_pop = NULL;

	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				mb_type ret;

				if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
					return FALSE;
				}

				if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;

				ret = nftool_mbox(g_curwnd, "CONFIRM", "All recorded data will be removed.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
				if(ret == NFTOOL_MB_CANCEL)
					return FALSE;

				if(!prog_pop) 
					prog_pop = nftool_prog_pop_open(g_curwnd, "FORMAT", TRUE, (guint)PROG_POS_X, (guint)PROG_POS_Y, 0);

				nfui_nfobject_disable(cancel_btn);
				nfui_signal_emit(cancel_btn, GDK_EXPOSE, FALSE);

				smt_set_service(SMT_FORMAT);
				scm_format_storage(IRET_SCM_FORMAT_STORAGE);
			}
			break;

		case INFY_FORMAT_RATE:
			{
				guint rate = ((CMM_MESSAGE_T *)data)->param;

				nftool_prog_pop_set_rate(prog_pop, rate);
			}
			break;

		case INFY_FORMAT_CMPL:
			{
				g_message("FORMAT completed...but filesystem is not started yet\n");
			}
			break;

		case IRET_SCM_FORMAT_STORAGE:
			init_do_data();
			set_data_to_obj(1);

			if(prog_pop) {
				nftool_prog_pop_set_rate(prog_pop, 100);

				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;
			}

			smt_return_to_previous();
			break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_FORMAT_RATE);
			uxm_unreg_imsg_event(obj, INFY_FORMAT_CMPL);
			uxm_unreg_imsg_event(obj, IRET_SCM_FORMAT_STORAGE);
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean post_delete_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				mb_type ret;
				SecurityData secdata;

				if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

				stm_set_time_by_sys();

				DAL_get_security_data(&secdata);
				if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK")) return FALSE;

				VW_Delete_Data_Open(g_curwnd, vsm_create_livestart_obj(), 0);
			}
			break;

		case GDK_DELETE:
			//uxm_unreg_imsg_event(obj, INFY_FORMAT_RATE);
			//uxm_unreg_imsg_event(obj, INFY_FORMAT_CMPL);
			//uxm_unreg_imsg_event(obj, IRET_SCM_FORMAT_STORAGE);
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(&g_dmd, &g_odmd, sizeof(DiskManageData));

		set_data_to_obj(1);
	}
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        mb_type ret;
        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		get_data_from_obj();

		if(memcmp(&g_odmd, &g_dmd, sizeof(DiskManageData))) {
			set_do_data();
			DAL_save_setup_db(NFSETUP_WINDOW_DISK);
			change_disk_operation();

			nfui_nfobject_disable(cancel_btn);
			nfui_signal_emit(cancel_btn, GDK_EXPOSE, FALSE);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			disk_data_changed(TRUE);
			// skshin
			scm_put_log(CHANGE_STRG_OP, 0, 0);
		}
	}
	else if(evt->type == IRET_RESTART_SERVICE) 
	{
		if(g_waitPop) {
			nftool_remove_waitbox(g_waitPop);
			g_waitPop = NULL;

			smt_return_to_previous();
			gtk_main_quit();
		}
	}
	else if(evt->type == GDK_DELETE) 
	{
		uxm_unreg_imsg_event(obj, IRET_RESTART_SERVICE);
	}

	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		get_data_from_obj();

		if(!memcmp(&g_odmd, &g_dmd, sizeof(DiskManageData))) {
			VW_DiskSetup_Destroy(obj);
			return FALSE;
		}

		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
		if(ret == NFTOOL_MB_OK) {
		    change_disk_operation();

			set_do_data();

			disk_data_changed(TRUE);
			
			// skshin
			scm_put_log(CHANGE_STRG_OP, 0, 0);
		}
	
		VW_DiskSetup_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_time_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;	
	mb_type ret;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if(numRet == -1)
		    numRet = numTemp;
		    
        if(prvNumberToRTL(numRet, g_dmd.timeType) > 7776000)
        {
            ret = nftool_mbox(g_curwnd, "WARNING", "Recording time limit must be less than three months.", NFTOOL_MB_OK);
            return FALSE;
        }

        if(prvNumberToRTL(numRet, g_dmd.timeType) < 1)
        {
            ret = nftool_mbox(g_curwnd, "WARNING", "Recording time limit must be large than 0.", NFTOOL_MB_OK);
            return FALSE;
        }

		if(numTemp != numRet)
	    {
    	    g_dmd.timeLimit = numRet;
		    nfui_nfobject_enable(cancel_btn);
		    nfui_signal_emit(cancel_btn, GDK_EXPOSE, FALSE);
		}

		if (numRet == -1) return FALSE;
		
        nfui_nflabel_set_number((NFLABEL*)obj, numRet);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

	}

	return FALSE;
}

static gboolean post_rtl_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
	    g_dmd.timeType = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_rtlObj);
	    if (g_dmd.timeType == TIME_LIMIT_OFF)
	    {
	        nfui_nfobject_disable(g_numObj);
	        nfui_nflabel_set_number(g_numObj, 0);

		    if(ivsc.dfunc.support_usrdef_holiday)
				nfui_nfobject_disable(g_ccobj);
        }
        else
        {
            nfui_nfobject_enable(g_numObj);
            nfui_nflabel_set_number(g_numObj, 1);

		    if(ivsc.dfunc.support_usrdef_holiday)
				nfui_nfobject_enable(g_ccobj);
        }

		nfui_nfobject_enable(cancel_btn);
		
        nfui_signal_emit(g_numObj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(cancel_btn, GDK_EXPOSE, FALSE);
		
		if(ivsc.dfunc.support_usrdef_holiday)
			nfui_signal_emit(g_ccobj, GDK_EXPOSE, TRUE);
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



void VW_Init_DiskOper_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;
	NFOBJECT *chk_fixed;

	gchar *strLabel[5] = {"DISK WRITE MODE", "RECORDING TIME LIMIT", "EXCEPT HOLIDAYS", "DISK FORMAT", 
#if defined(_ZICOM_STRING_FIX)
	"PRIVACY PROTECTION(GDPR)"
#else
	"ERASE VIDEO"
#endif
    };

	gchar *strNothing[3] = {"NOTHING 1", "NOTHING 2", "NOTHING 3"};
	const gchar *strMode[] = {"ONCE", "OVERWRITE"};
	const gchar *strRTL[NUM_TIME_LIMITS] = {"OFF",
											"HOUR",
											"DAY",
											"WEEK"};
	guint table_w[3] = {450, 250, 250};
	gint size_w, size_h;
	gint i;

	g_curwnd = nfui_nfobject_get_top(parent);



	// init data
	init_do_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	/* table */
	tbl = (NFOBJECT*)nfui_nftable_new(3, 6, 3, 4, table_w, 40);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 28, 42);

	// label
	for(i=0; i<6; i++) {
		if((i == 2) && (!ivsc.dfunc.support_usrdef_holiday)) continue;
		if (i == 3) continue;
        if ((i == 5) && (!ivsc.dfunc.support_video_erase)) continue;
        
		if(i < 3)
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i - 1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));

		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, i);
	}
	
	// write mode
	obj = nfui_spinbutton_new(strMode, 2, g_dmd.overWrite);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_write_mode_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, 0);

	if(!var_get_running_disk_count())
		nfui_nfobject_disable(obj);
	
	g_wmodeObj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_use_number(obj, TRUE);
	nfui_nflabel_set_number(obj, prvRTLToNumber(g_dmd.timeLimit, g_dmd.timeType));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nfobject_set_size(obj, 250, 40);	  		
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, 1);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_time_event_handler);
    g_numObj = obj;

	// rec timelimit
	obj = nfui_combobox_new(strRTL, NUM_TIME_LIMITS, g_dmd.timeType);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  2, 1);
	nfui_regi_post_event_callback(obj, post_rtl_handler);

	if(!var_get_running_disk_count())
		nfui_nfobject_disable(obj);
	
	g_rtlObj = obj;

	if(!g_dmd.overWrite)
	{
        nfui_nfobject_disable(g_numObj);
		nfui_nfobject_disable(g_rtlObj);
	}
	
    if (g_dmd.timeType == TIME_LIMIT_OFF){
        nfui_nfobject_disable(g_numObj); 

	}
	if(ivsc.dfunc.support_usrdef_holiday){
		chk_fixed = nfui_nffixed_new();
		nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_show(chk_fixed);
		nfui_nftable_attach((NFTABLE*)tbl, chk_fixed,  1, 2);

		obj = nfui_checkbutton_new(g_dmd.custom_cal);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
		nfui_check_get_size(obj, &size_w, &size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)chk_fixed, obj, (250-size_w)/2, (40-size_h)/2);
		g_ccobj = obj;

		if (g_dmd.timeType == TIME_LIMIT_OFF)
			nfui_nfobject_disable(g_ccobj);
		
	}
	
	// button
	obj = nftool_normal_button_create_type3("FORMAT", 250);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	uxm_reg_imsg_event(obj, INFY_FORMAT_RATE);
	uxm_reg_imsg_event(obj, INFY_FORMAT_CMPL);
	uxm_reg_imsg_event(obj, IRET_SCM_FORMAT_STORAGE);

	nfui_regi_post_event_callback(obj, post_format_event_handler);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, 4);

	if(!var_get_running_disk_count())
		nfui_nfobject_disable(obj);

	obj = nftool_normal_button_create_type3("ERASE", 250);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	if (ivsc.dfunc.support_video_erase) nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, 5);
	nfui_regi_post_event_callback(obj, post_delete_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
	cancel_btn = obj;

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);
	uxm_reg_imsg_event(obj, IRET_RESTART_SERVICE);

	obj = nftool_normal_button_create_type1("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean VW_DiskOper_tab_out_handler()
{
	mb_type ret;

	get_data_from_obj();

	if(memcmp(&g_odmd, &g_dmd, sizeof(DiskManageData))) {
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK) {			            
			change_disk_operation();

			set_do_data();

			//
			scm_put_log(CHANGE_STRG_OP, 0, 0);

		}else if(ret == NFTOOL_MB_CANCEL) {
			g_memmove(&g_dmd, &g_odmd, sizeof(DiskManageData));

			set_data_to_obj(0);
		}

		if(ret == NFTOOL_MB_OK) 
			disk_data_changed(TRUE);
	}

	return FALSE;
}
