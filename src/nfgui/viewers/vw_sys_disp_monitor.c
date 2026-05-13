#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"

#include "vw_sys_disp_main.h"
#include "vw_sys_disp_monitor.h"
#include "support/event_loop.h"
#include "scm.h"
#include "smt.h"

#include "nf_sysman.h"


#define	NUM_DM_COLUMNS			(2)
#define	DM_COL_SPACE			(2)
#define	DM_ROW_SPACE			(1)

#define	DM_TABLE_LEFT			(28)
#define	DM_TABLE_TOP			(42)

#define	DM_LABEL_HEIGHT			(40)


enum{
	DM_SEQUENCE_DWELL = 0,
	DM_SPOT_DWELL,
	DM_ASPECT_RATIO,
	DM_RESOLUTION_TYPE,
	NUM_DM_ROWS
};


enum {
	DMB_CANCEL = 0,
	DMB_APPLY,
	DMB_CLOSE,
	DMB_BUTTONS
};

enum {
#if !defined (_IPX_0412M4) && !defined (_IPX_0824M4) && !defined (_IPX_1648M4) && !defined (_IPX_0824P4E) && !defined (_IPX_1648P4E) \
 && !defined (_IPX_0412M4E) && !defined (_IPX_0824M4E) && !defined(_IPX_1648M4E) &&!defined(_IPX_32P4E) &&! defined(_IPX_32M4E) &&! defined(_IPX_32P5)
	DWELL_1_SEC = 0,
#endif
	DWELL_2_SEC,
	DWELL_3_SEC,
	DWELL_5_SEC,
	DWELL_10_SEC,
	DWELL_15_SEC,
	DWELL_20_SEC,
	DWELL_30_SEC,
	DWELL_40_SEC,
	DWELL_60_SEC,
	
	NUM_DWELL_TIMES,
};

static MonitorData mondata;
static MonitorData org_mondata;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_value_object[NUM_DM_ROWS];


static gint _is_supported_menu(gint idx)
{
    if (idx == DM_SPOT_DWELL)
    {
        if (!ivsc.dfunc.support_spot && !ivsc.dfunc.dualmonitor.support) return 0;
    }

    if (idx == DM_ASPECT_RATIO)
    {
        if (!ivsc.dfunc.support_aspect_ratio) return 0;
    }
    
    if (idx == DM_RESOLUTION_TYPE)
    {
        if (ivsc.dfunc.dualmonitor.advance_type) return 0;
    }

    return 1;
}

static guint prvIndexToDwellTime(gint index)
{
	guint ret = 0;

	switch(index)
	{
#if !defined (_IPX_0412M4) && !defined (_IPX_0824M4) && !defined (_IPX_1648M4) && !defined (_IPX_0824P4E) && !defined (_IPX_1648P4E) \
 && !defined (_IPX_0412M4E) && !defined (_IPX_0824M4E) && !defined (_IPX_1648M4E) &&! defined(_IPX_32P4E) &&! defined(_IPX_32M4E) &&! defined(_IPX_32P5)
		case DWELL_1_SEC:	ret = 1;		break;
#endif
		case DWELL_2_SEC:	ret = 2;		break;
		case DWELL_3_SEC:	ret = 3;		break;
		case DWELL_5_SEC:	ret = 5;		break;
		case DWELL_10_SEC:	ret = 10;		break;
		case DWELL_15_SEC:	ret = 15;		break;
		case DWELL_20_SEC:	ret = 20;		break;
		case DWELL_30_SEC:	ret = 30;		break;
		case DWELL_40_SEC:	ret = 40;		break;
		case DWELL_60_SEC:	ret = 60;		break;

		default:	ret = 5;	break;
	}

	return ret;
}

static gint prvDwellTimeToIndex(guint dtime)
{
	gint ret = 0;

#if !defined (_IPX_0412M4) && !defined (_IPX_0824M4) && !defined (_IPX_1648M4) && !defined (_IPX_0824P4E) && !defined (_IPX_1648P4E) \
 && !defined (_IPX_0412M4E) && !defined (_IPX_0824M4E) && !defined (_IPX_1648M4E) &&!defined(_IPX_32P4E) &&! defined(_IPX_32M4E) &&! defined(_IPX_32P5)
	if(dtime < 2)		ret = DWELL_1_SEC;
	else if(dtime < 3)	ret = DWELL_2_SEC;
#else
	if(dtime < 3)	    ret = DWELL_2_SEC;
#endif
	else if(dtime < 5)	ret = DWELL_3_SEC;
	else if(dtime < 10)	ret = DWELL_5_SEC;
	else if(dtime < 15)	ret = DWELL_10_SEC;
	else if(dtime < 20)	ret = DWELL_15_SEC;
	else if(dtime < 30)	ret = DWELL_20_SEC;
	else if(dtime < 40)	ret = DWELL_30_SEC;
	else if(dtime < 60)	ret = DWELL_40_SEC;
	else	ret = DWELL_60_SEC;

	return ret;
}

static gboolean _prvSetResolutionToData(int data)
{
	switch(data)
	{
		case 1:
			nf_sysman_user_set_resolution(ITX_DDC_PRI_3840_2160_30);
		break;
		case 2:
			nf_sysman_user_set_resolution(ITX_DDC_PRI_1080P_60);
		break;
		case 3:
			nf_sysman_user_set_resolution(ITX_DDC_PRI_720P_60);
		break;
		case 4:
			nf_sysman_user_set_resolution(ITX_DDC_PRI_1280_1024_60);
		break;
		case 5:
			nf_sysman_user_set_resolution(ITX_DDC_PRI_800_600_60);
		break;
		case 0:
		default:
			nf_sysman_user_set_resolution(ITX_DDC_PRI_UNKNOWN);
		break;
	}
	return FALSE;
}

static guint _prvGetDataFromResolution()
{
	int ret;
	int data;
	data = nf_sysman_user_get_resolution();

	switch(data)
	{
		case ITX_DDC_PRI_3840_2160_30:
			ret = 1;
		break;
		case ITX_DDC_PRI_1080P_60:
			ret = 2;
		break;
		case ITX_DDC_PRI_720P_60:
			ret = 3;
		break;
		case ITX_DDC_PRI_1280_1024_60:
			ret = 4;
		break;
		case ITX_DDC_PRI_800_600_60:
			ret = 5;
		break;
		case ITX_DDC_PRI_UNKNOWN:
		default:
			ret = 0;
		break;
	}
 	return ret;
}


static gboolean _proc_escape(void *data)
{
	NFOBJECT *popup = (NFOBJECT *)data;

	nftool_remove_waitbox(popup);
	
    smt_set_service(SMT_SHUTDOWN);
    scm_reboot_system(RR_DBCHANGE, 3000);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

    nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

	return FALSE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
			/*
			x -= 14;
			y -= 6;
			nfutil_draw_image(drawable, gc, IMG_MAINBG2, x, y, -1, -1, NFALIGN_LEFT, 0);
			*/
			nfui_nfobject_gc_unref(gc);
		}
		break;

		default :
			break;
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{					
			return FALSE;
      	}

		g_memmove(&mondata, &org_mondata, sizeof(MonitorData));

        if (g_value_object[DM_SEQUENCE_DWELL]) nfui_spin_button_set_index(g_value_object[DM_SEQUENCE_DWELL], prvDwellTimeToIndex(mondata.seqDwell));
        if (g_value_object[DM_SPOT_DWELL]) nfui_spin_button_set_index(g_value_object[DM_SPOT_DWELL], prvDwellTimeToIndex(mondata.spotDwell));
        if (g_value_object[DM_ASPECT_RATIO]) nfui_spin_button_set_index(g_value_object[DM_ASPECT_RATIO], mondata.aspect);
        if (g_value_object[DM_RESOLUTION_TYPE]) nfui_spin_button_set_index(g_value_object[DM_RESOLUTION_TYPE], mondata.resolution);

        for (i = 0; i < NUM_DM_ROWS; i++)
        {
            if (g_value_object[i]) nfui_signal_emit(g_value_object[i], GDK_EXPOSE, TRUE);
        }
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		gint resolution_changed = 0;
		int modified = 0;
		mb_type ret;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	        
        if (g_value_object[DM_SEQUENCE_DWELL]) mondata.seqDwell = prvIndexToDwellTime(nfui_spin_button_get_index(g_value_object[DM_SEQUENCE_DWELL]));
        if (g_value_object[DM_SPOT_DWELL]) mondata.spotDwell = prvIndexToDwellTime(nfui_spin_button_get_index(g_value_object[DM_SPOT_DWELL]));
        if (g_value_object[DM_ASPECT_RATIO]) mondata.aspect = nfui_spin_button_get_index(g_value_object[DM_ASPECT_RATIO]);
        if (g_value_object[DM_RESOLUTION_TYPE]) mondata.resolution= nfui_spin_button_get_index(g_value_object[DM_RESOLUTION_TYPE]);

		if(memcmp(&org_mondata, &mondata, sizeof(MonitorData)))
		{
			if (mondata.resolution != org_mondata.resolution)
			{
				ret = nftool_mbox(g_curwnd, "CONFIRM", "Display resolution has been changed.\nSystem will be reboot.", NFTOOL_MB_OKCANCEL);

				if (ret == NFTOOL_MB_OK)
				{
					resolution_changed = 1;
				}
				else
				{
					mondata.resolution = org_mondata.resolution;
					nfui_spin_button_set_index_no_expose(g_value_object[DM_RESOLUTION_TYPE], mondata.resolution);
					nfui_signal_emit(g_value_object[DM_RESOLUTION_TYPE], GDK_EXPOSE, TRUE);
				}

				if(memcmp(&org_mondata, &mondata, sizeof(MonitorData)))
                {
        			g_memmove(&org_mondata, &mondata, sizeof(MonitorData));
        			DAL_set_monitor_data(&mondata);
					scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
        			modified = 1;
                }
			}
			else
			{
				g_memmove(&org_mondata, &mondata, sizeof(MonitorData));
    			DAL_set_monitor_data(&mondata);
				scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
    			modified = 1;
			}
		}

		if (modified == 1)
		{
			if (resolution_changed == 1)
			{
				NFOBJECT *save_pop;
			
				save_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");

				if (sysdisp_get_changeflag())
					DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);	

				_prvSetResolutionToData(mondata.resolution);
				scm_put_log(CHANGE_DISP_MONITOR, 0, 0);
				g_timeout_add(1000, _proc_escape, save_pop);
			}
           	else
         	{		
    			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
    			sysdisp_set_changeflag(1);
    		}
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
	
		Monitor_tab_out_handler();

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


void init_DispMonitor_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *ntb;
    NFOBJECT *obj;
    
	NFOBJECT *subject_object[NUM_DM_ROWS];
	NFOBJECT *dispmon_btns[DMB_BUTTONS];

	const gchar *strTitle[] = {
					"SEQUENCE DWELL",
					"SPOT DWELL",
					"VIDEO ASPECT RATIO",
					"DISPLAY RESOLUTION",
	};

    static gchar *strDwell[NUM_DWELL_TIMES] = {
#if !defined (_IPX_0412M4) && !defined (_IPX_0824M4) && !defined (_IPX_1648M4) && !defined (_IPX_0824P4E) && !defined (_IPX_1648P4E) \
 && !defined (_IPX_0412M4E) && !defined (_IPX_0824M4E) && !defined (_IPX_1648M4E) &&! defined(_IPX_32P4E) &&! defined(_IPX_32M4E) &&! defined(_IPX_32P5)
    	"1 SEC",	
#endif
    	"2 SEC", "3 SEC", "5 SEC", "10 SEC", "15 SEC", "20 SEC", "30 SEC", "40 SEC", "60 SEC"
    };
	const gchar *strOffOn[] = {"OFF", "ON"};				
	const gchar *strRatio[] = {"KEEP", "FULL"};
	const gchar *strResolution[] = {"AUTO", "3840 X 2160", "1920 X 1080", "1280 X 720", "1280 X 1024"};

    gint menu_cnt = 0, menu_idx = 0;

	guint width[NUM_DM_COLUMNS];
	guint btn_x, btn_y, btn_space;
	guint i;


	g_curwnd = nfui_nfobject_get_top(parent);

	// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	memset(&mondata, 0x00, sizeof(MonitorData));
	memset(&org_mondata, 0x00, sizeof(MonitorData));

	DAL_get_monitor_data(&mondata);
	mondata.resolution = _prvGetDataFromResolution();

    for (i = 0; i < NUM_DM_ROWS; i++)
    {
        g_value_object[i] = 0;
        if (_is_supported_menu(i)) menu_cnt++;
    }

	width[0] = 310;
	width[1] = 261;

	ntb = nfui_nftable_new(NUM_DM_COLUMNS, NUM_DM_ROWS, DM_COL_SPACE, DM_ROW_SPACE, width, DM_LABEL_HEIGHT);	
	nfui_nfobject_show(ntb);
	nfui_nffixed_put(content_fixed, ntb, DM_TABLE_LEFT, DM_TABLE_TOP);

	for(i=0; i<NUM_DM_ROWS; i++)
	{
        if (!_is_supported_menu(i)) continue;

        obj = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);      
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, menu_idx);
        
        if (i == DM_SEQUENCE_DWELL) obj = nfui_spinbutton_new(strDwell, NUM_DWELL_TIMES, prvDwellTimeToIndex(mondata.seqDwell));
        if (i == DM_SPOT_DWELL) obj = nfui_spinbutton_new(strDwell, NUM_DWELL_TIMES, prvDwellTimeToIndex(mondata.spotDwell));
        if (i == DM_ASPECT_RATIO) obj = nfui_spinbutton_new(strRatio, 2, mondata.aspect);
        if (i == DM_RESOLUTION_TYPE) obj = nfui_spinbutton_new(strResolution, 5, mondata.resolution);

        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
        nfui_nfobject_show(obj);      
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, menu_idx);
        g_value_object[i] = obj;
        
        menu_idx++;
	}
	
	dispmon_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(dispmon_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(dispmon_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, dispmon_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	dispmon_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(dispmon_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(dispmon_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, dispmon_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	dispmon_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(dispmon_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(dispmon_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, dispmon_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(dispmon_btns[DMB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(dispmon_btns[DMB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(dispmon_btns[DMB_CLOSE], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(&org_mondata, &mondata, sizeof(MonitorData));

}

gboolean Monitor_tab_out_handler()
{
	mb_type ret;
	guint resolution_changed = 0;

    if (g_value_object[DM_SEQUENCE_DWELL]) mondata.seqDwell = prvIndexToDwellTime(nfui_spin_button_get_index(g_value_object[DM_SEQUENCE_DWELL]));
    if (g_value_object[DM_SPOT_DWELL]) mondata.spotDwell = prvIndexToDwellTime(nfui_spin_button_get_index(g_value_object[DM_SPOT_DWELL]));
    if (g_value_object[DM_ASPECT_RATIO]) mondata.aspect = nfui_spin_button_get_index(g_value_object[DM_ASPECT_RATIO]);
    if (g_value_object[DM_RESOLUTION_TYPE]) mondata.resolution= nfui_spin_button_get_index(g_value_object[DM_RESOLUTION_TYPE]);
			
	if(!memcmp(&org_mondata, &mondata, sizeof(MonitorData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{		
		if(org_mondata.resolution != mondata.resolution)
		{
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Display resolution has been changed.\nSystem will be reboot.", NFTOOL_MB_OKCANCEL);
			
			if (ret == NFTOOL_MB_OK)
			{
				resolution_changed = 1;		
			}
			else
			{
				mondata.resolution = org_mondata.resolution;		
				nfui_spin_button_set_index_no_expose(g_value_object[DM_RESOLUTION_TYPE], mondata.resolution);
			}
		}

		g_memmove(&org_mondata, &mondata, sizeof(MonitorData));
		DAL_set_monitor_data(&mondata);
		sysdisp_set_changeflag(1);
		scm_put_log(CHANGE_DISP_MONITOR, 0, 0);

		if((resolution_changed == 1))
		{
			if (sysdisp_get_changeflag())
				DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
		
			_prvSetResolutionToData(mondata.resolution);
			smt_set_service(SMT_SHUTDOWN);
			scm_reboot_system(RR_DBCHANGE, 3000);
			scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

			nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");		
			gtk_main();
		}
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&mondata, &org_mondata, sizeof(MonitorData));

        if (g_value_object[DM_SEQUENCE_DWELL]) nfui_spin_button_set_index_no_expose(g_value_object[DM_SEQUENCE_DWELL], prvDwellTimeToIndex(mondata.seqDwell));
        if (g_value_object[DM_SPOT_DWELL]) nfui_spin_button_set_index_no_expose(g_value_object[DM_SPOT_DWELL], prvDwellTimeToIndex(mondata.spotDwell));
        if (g_value_object[DM_ASPECT_RATIO]) nfui_spin_button_set_index_no_expose(g_value_object[DM_ASPECT_RATIO], mondata.aspect);
        if (g_value_object[DM_RESOLUTION_TYPE]) nfui_spin_button_set_index_no_expose(g_value_object[DM_RESOLUTION_TYPE], mondata.resolution);
	}
	return FALSE;
}




