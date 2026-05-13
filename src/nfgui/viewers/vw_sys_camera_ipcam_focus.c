#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "vw_sys_camera_ipcam_setup_main.h"
#include "vw_sys_camera_ipcam_focus.h"

#define VIDEO_AREA_SIZE_W			(1000)
#define VIDEO_AREA_SIZE_H			(600)

// focus preview 

#define PREVIEW_TITLE_X     (20)
#define PREVIEW_TITLE_Y     (20)    

#define PREVIEW_AREA_X      (20)          
#define PREVIEW_AREA_Y      (20+40+5)

// focuse compensation 

#define FOCUS_CPS_TITLE_X   (20)
#define FOCUS_CPS_TITLE_Y   (PREVIEW_TITLE_Y)

#define TEMPER_TITLE_X      (20)
#define TEMPER_TITLE_Y      (FOCUS_CPS_TITLE_Y+40+10)

#define TEMPER_COMBO_X      (20+230+10)
#define TEMPER_COMBO_Y      (TEMPER_TITLE_Y)

#define DAYNIGHT_TITLE_X    (20)
#define DAYNIGHT_TITLE_Y    (TEMPER_TITLE_Y+40+5)

#define DAYNIGHT_COMBO_X    (20+230+10)
#define DAYNIGHT_COMBO_Y    (DAYNIGHT_TITLE_Y)




// focuse compensation 

// drag view 개발시 주석 change  jaeyoung
/*
#define FOCUS_CPS_TITLE_X   (20+VIDEO_AREA_SIZE_W+20)
#define FOCUS_CPS_TITLE_Y   (PREVIEW_AREA_Y)

#define TEMPER_TITLE_X      (20+VIDEO_AREA_SIZE_W+20)
#define TEMPER_TITLE_Y      (FOCUS_CPS_TITLE_Y+40+10)

#define TEMPER_COMBO_X      (20+230+10+VIDEO_AREA_SIZE_W+20)
#define TEMPER_COMBO_Y      (TEMPER_TITLE_Y)

#define DAYNIGHT_TITLE_X    (20+VIDEO_AREA_SIZE_W+20)
#define DAYNIGHT_TITLE_Y    (TEMPER_TITLE_Y+40+5)

#define DAYNIGHT_COMBO_X    (20+230+10+VIDEO_AREA_SIZE_W+20)
#define DAYNIGHT_COMBO_Y    (DAYNIGHT_TITLE_Y)
*/




static NFWINDOW *g_curwnd = 0;
static NFOBJECT *preview_obj;
static NFOBJECT *temp_obj;
static NFOBJECT *dnn_obj;

static FocusData focusdata[GUI_CHANNEL_CNT];
static FocusData org_focusdata[GUI_CHANNEL_CNT];

static gboolean cur_tab = FALSE;
static gint g_ch;

static void _show_preview(gint ch)
{
    guint ch_mask = 0;
    gint x, y;
    
    if(ch < 0) return;

    ch_mask |= (1<<ch);

    nfui_nfobject_get_offset(preview_obj, &x, &y);
    vsm_live_preview_start(ch_mask, (guint)x, (guint)y, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);

    nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_signal_emit(preview_obj, GDK_EXPOSE, TRUE);
}

static void _stop_preview()
{
    vsm_live_preview_stop();
    nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_signal_emit(preview_obj, GDK_EXPOSE, TRUE);
}

static gboolean _update_object_focus_info(gint ch)
{
    nfui_combobox_set_index_no_expose(dnn_obj, focusdata[ch].day_night);
    nfui_combobox_set_index_no_expose(temp_obj, focusdata[ch].temperature);

    nfui_signal_emit(dnn_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(temp_obj, GDK_EXPOSE, TRUE);

    DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, ch);

    return FALSE;
}

static gboolean focus_update_data(gint ch)
{
    NFIPCamFocusCompProfile profile;
    guint support;
    gint ret;
    
    ret = scm_support_ipcam_focus_profile(ch, &profile);   

    if(ret){
    
        focusdata[ch].day_night   = profile.dnn_comp_mode;
        focusdata[ch].temperature = profile.tem_comp_mode;        
        
        if(profile.supported & NF_IPCAM_FOCUS_DNN_COMP) nfui_nfobject_enable(dnn_obj);
        else    nfui_nfobject_disable(dnn_obj);
    
        if(profile.supported & NF_IPCAM_FOCUS_TEM_COMP) nfui_nfobject_enable(temp_obj);
        else    nfui_nfobject_disable(temp_obj);

        _update_object_focus_info(ch);
    }
    else{        
        nfui_combobox_set_index_no_expose(dnn_obj, focusdata[ch].day_night);
        nfui_combobox_set_index_no_expose(temp_obj, focusdata[ch].temperature);
        
        nfui_nfobject_disable(dnn_obj);
        nfui_nfobject_disable(temp_obj);

        nfui_signal_emit(dnn_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(temp_obj, GDK_EXPOSE, TRUE);
        
        g_message("\n FAIL GET FOCUS RPOFILE ... FAIL .. \n");
    }


    
    return FALSE;
}

static gboolean _init_data()
{
    gint i;

    for( i = 0; i< GUI_CHANNEL_CNT; i++)
        DAL_get_focus_data(&focusdata[i], i);

    g_memmove(&org_focusdata, &focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT);
        
    return FALSE;
}

static gboolean _init_sync_data()
{
    if(memcmp(&org_focusdata, &focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT))
    {
        DAL_set_focus_data_all(focusdata, GUI_CHANNEL_CNT);
        syscam_set_changeflag(1);
        g_memmove(&org_focusdata,&focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT);
    }
    
    return FALSE;
}


static gboolean post_temperature_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint idx;

        idx = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));	

        focusdata[g_ch].temperature = idx;

        //DAL_set_focus_data(focusdata, g_ch);
        //DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, g_ch);
    }

    return FALSE;
}

static gboolean post_daynight_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint idx;

        idx = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

        focusdata[g_ch].day_night= idx;

        //DAL_set_focus_data(focusdata, g_ch);
        //DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, g_ch);
        
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        gint i;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(&focusdata, &org_focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT);
		_update_object_focus_info(g_ch);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *buf = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if(memcmp(&org_focusdata, &focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT))
		{
			g_memmove(&org_focusdata, &focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT);
			DAL_set_focus_data_all(focusdata, GUI_CHANNEL_CNT);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			syscam_set_changeflag(1);
			DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
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

	    IPCamFocus_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}


void VW_Init_IPCamFocus_Page(NFOBJECT *parent, gint ch)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;

    gchar *onoffstr[] = {"OFF","ON"};
    gint i;
    g_ch = ch;
    
    g_curwnd = nfui_nfobject_get_top(parent); 

    memset(focusdata, 0x00, sizeof(FocusData)*GUI_CHANNEL_CNT);
    memset(org_focusdata, 0x00, sizeof(FocusData)*GUI_CHANNEL_CNT);

    _init_data();
   
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_W, MENU_V_IPCAMSET_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_X, MENU_V_IPCAMSET_SUBTAB_INNER_Y);

    /*  drag 영역 추가시 주석 제거 jaeyoung
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 227, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, PREVIEW_TITLE_X, PREVIEW_TITLE_Y);
    nfui_nfobject_show(obj);


    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_PRG_IDX(UX_COLOR_FFFFFF));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_nfobject_set_size(obj, VIDEO_AREA_SIZE_W, VIDEO_AREA_SIZE_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, PREVIEW_AREA_X, PREVIEW_AREA_Y);
	preview_obj = obj;
	*/	

    obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "FOCUS COMPENSATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 460, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, FOCUS_CPS_TITLE_X, FOCUS_CPS_TITLE_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TEMPERATURE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 230, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, TEMPER_TITLE_X, TEMPER_TITLE_Y);
    nfui_nfobject_show(obj);

	obj = nfui_combobox_new(onoffstr, 2, focusdata[ch].temperature);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, TEMPER_COMBO_X, TEMPER_COMBO_Y);
	nfui_nfobject_disable(obj);
	nfui_regi_post_event_callback(obj, post_temperature_event_handler);
	temp_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAY/NIGHT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 230, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, DAYNIGHT_TITLE_X, DAYNIGHT_TITLE_Y);
    nfui_nfobject_show(obj);

	obj = nfui_combobox_new(onoffstr, 2, focusdata[ch].day_night);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DAYNIGHT_COMBO_X, DAYNIGHT_COMBO_Y);
	nfui_nfobject_disable(obj);
	nfui_regi_post_event_callback(obj, post_daynight_event_handler);
    dnn_obj = obj;   

    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R3_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R2_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R1_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    focus_update_data(ch);
    _init_sync_data();
    
}

gint IPCamFocus_update_channel(gint ch)
{	
	g_ch = ch;

    // if(cur_tab) _show_preview(ch);
	focus_update_data(ch);

    return 0;
}

gboolean IPCamFocus_tab_in_handler()
{
    //_show_preview(g_ch);
    cur_tab = TRUE;
    focus_update_data(g_ch);
    _init_sync_data();
    
	return FALSE;
}

gboolean IPCamFocus_tab_out_handler()
{
	gchar *buf = NULL;
	mb_type ret;

    
	if (!memcmp(&org_focusdata, &focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		g_memmove(&org_focusdata, &focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT);
		DAL_set_focus_data_all(focusdata, GUI_CHANNEL_CNT);

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		syscam_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&focusdata, &org_focusdata, sizeof(FocusData)*GUI_CHANNEL_CNT);
		_update_object_focus_info(g_ch);
	}


    cur_tab = FALSE;
    //_stop_preview();
    
	return FALSE;
}

	

