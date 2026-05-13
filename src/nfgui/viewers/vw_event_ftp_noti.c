
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
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_evt_act_main.h"
#include "vw_event_noti_evt.h"
#include "vw_event_ftp_noti.h"
#include "vw_event_ftp_edit.h"
#include "vw_passage_popup.h"

#include "vw_vkeyboard.h"
#include "log.h"
#include "scm.h"
#include "ix_mem.h"

#define FTP_NOTI_HELP_BTN_Y				(MENU_V_SUBTAB_INNER_H - 40 - 4)
#define HELP_STR1 "Because the file name will continuously be overwritten if set to 'SYSTEM ID' or 'MANUAL',\nplease choose 'MANUAL-DATE-TIME' or 'SYSTEM ID-DATE-TIME' for FILENAME entry\nto maintain information for every event that occurs."
#define HELP_STR2 "When using OVERWRITE disk write mode, it may not be possible to play a Web-Link\nafter a long period of time has elapsed due to overwrite." 
#define HELP_STR_CNT (2)


enum {
    FREQ_IMMEDIATELY = 0,
	FREQ_1_MIN,
	FREQ_5_MIN,
	FREQ_10_MIN,
	FREQ_15_MIN,
	FREQ_30_MIN,
	FREQ_60_MIN,
	
	NUM_FREQS
};

static EA_EvtNotiFTPData g_efd;
static EA_EvtNotiFTPData g_oefd;

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_dirModeObj;
static NFOBJECT *g_dirManualObj;
static NFOBJECT *g_fileModeObj;
static NFOBJECT *g_fileManualObj;

static NFOBJECT *g_webLinkObj;
static NFOBJECT *g_freqObj;
static NFOBJECT *g_snapObj;
static NFOBJECT *g_videoObj;

static void init_ef_data()
{
	memset(&g_efd, 0x00, sizeof(EA_EvtNotiFTPData));
	memset(&g_oefd, 0x00, sizeof(EA_EvtNotiFTPData));

	DAL_get_evtNoti_ftp_data(&g_efd);
	g_memmove(&g_oefd, &g_efd, sizeof(EA_EvtNotiFTPData));
}

static void set_ef_data()
{
	g_memmove(&g_oefd, &g_efd, sizeof(EA_EvtNotiFTPData));

	DAL_set_evtNoti_ftp_data(g_efd);
	scm_put_log(CHANGE_EVT_NOTI, 0, 0);
}

static void changeManualLabel(NFOBJECT *obj, gint mode, gchar *str)
{
    if (mode == 0 || mode == 1)
    {
        nfui_nflabel_set_text((NFLABEL*)obj, str);
        nfui_nfobject_enable(obj);
    }
    else
    {
        nfui_nflabel_set_text((NFLABEL*)obj, "");
        nfui_nfobject_disable(obj);
    }
}

static guint prvIndexToFreq(gint index)
{
	guint ret = 0;

	switch(index)
	{
	    case FREQ_IMMEDIATELY:  ret = 0;    break;
		case FREQ_1_MIN:		ret = 1;	break;
		case FREQ_5_MIN:		ret = 5;	break;
		case FREQ_10_MIN:		ret = 10;	break;
		case FREQ_15_MIN:		ret = 15;	break;
		case FREQ_30_MIN:		ret = 30;	break;
		case FREQ_60_MIN:		ret = 60;	break;
		default:				ret = 0;	break;
	}

	return ret;
}

static gint prvFreqToIndex(guint freq)
{
	gint ret = 0;

    if (freq == 0)          ret = FREQ_IMMEDIATELY;
    else if (freq < 5)      ret = FREQ_1_MIN;
	else if (freq < 10)	    ret = FREQ_5_MIN;
	else if (freq < 15)	    ret = FREQ_10_MIN;
	else if (freq < 30)	    ret = FREQ_15_MIN;
	else if (freq < 60)	    ret = FREQ_30_MIN;
	else				    ret = FREQ_60_MIN;

	return ret;
}

static void set_data_to_obj(gint expose)
{
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_dirModeObj, g_efd.dir_mode);
    changeManualLabel(g_dirManualObj, g_efd.dir_mode, g_efd.dir_path); 

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_fileModeObj, g_efd.filename_mode);
    changeManualLabel(g_fileManualObj, g_efd.filename_mode, g_efd.filename); 

    nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_webLinkObj, g_efd.weblink);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_freqObj, prvFreqToIndex(g_efd.freq));
    nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_snapObj, g_efd.include_jpeg);
    nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_videoObj, g_efd.include_video);
  

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_snapObj);
		
    if (expose)
    {
        nfui_signal_emit(g_dirModeObj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_dirManualObj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_fileModeObj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_fileManualObj, GDK_EXPOSE, TRUE);        
        nfui_signal_emit(g_webLinkObj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_freqObj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_snapObj, GDK_EXPOSE, TRUE);        
        nfui_signal_emit(g_videoObj, GDK_EXPOSE, TRUE);        
    }    
}

static gboolean post_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
        gint pos_x, pos_y;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_offset(obj, &pos_x, &pos_y);		
        VW_EvtNoti_FTP_Edit(g_curwnd, pos_x, pos_y+44, &g_efd);
	}
	
	return FALSE;
}

static gboolean post_dir_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint mode;

		mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);       
        changeManualLabel(g_dirManualObj, mode, g_efd.dir_path);   
        nfui_signal_emit(g_dirManualObj, GDK_EXPOSE, TRUE);
        g_efd.dir_mode = (guint)mode;
	}
	
	return FALSE;
}

static gboolean post_dir_manual_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		guint off_x, off_y;
		gchar *strTemp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
    
		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), off_x, off_y, 31, VKEY_FTP_DIR);        

        if (strTemp)
        {
        	nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
        	nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);
            strcpy(g_efd.dir_path, strTemp);

        	ifree(strTemp);
        	strTemp = NULL;
        }
	}

	return FALSE;
}

static gboolean post_file_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint mode;

		mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        changeManualLabel(g_fileManualObj, mode, g_efd.filename);
        nfui_signal_emit(g_fileManualObj, GDK_EXPOSE, TRUE);
        g_efd.filename_mode = (guint)mode;        
	}
	
	return FALSE;
}

static gboolean post_file_manual_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		guint off_x, off_y;
		gchar *strTemp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
    
		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), off_x, off_y, 31, VKEY_ITXSTYLE_TITLE);        

        if (strTemp)
        {
        	nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
        	nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);
            strcpy(g_efd.filename, strTemp);

        	ifree(strTemp);
        	strTemp = NULL;
        }
	}

	return FALSE;
}

static gboolean post_weblink_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        gint onoff = 0;

        onoff = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        g_efd.weblink = (guint)onoff;
	}
	
	return FALSE;
}

static gboolean post_freq_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        gint freq_idx = 0;

        freq_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);	
        g_efd.freq = prvIndexToFreq(freq_idx);
	}
	
	return FALSE;
}

static gboolean post_snap_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        gboolean check = 0;

        check = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        g_efd.include_jpeg = check;
	}
	
	return FALSE;
}

static gboolean post_video_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        gboolean check = 0;

        check = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        g_efd.include_video = check;
	}
	
	return FALSE;
}

static gboolean post_help_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_RELEASE || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		guint x, y;
        PARAGRAPH_STR *para;
        gint i;

  	   	if (evt->button.button == MOUSE_RIGTH_BUTTON)  
  	   	    return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x;
		y += top->y + obj->height + 2;
		
        para = imalloc(sizeof(PARAGRAPH_STR));
        
        para->intro[0] = g_strdup(HELP_STR1);
        para->intro[1] = g_strdup(HELP_STR2);
        para->intro_cnt = HELP_STR_CNT;

        vw_passage_popup_open(g_curwnd, x, y, DIR_TOP_RIGHT, &para, 1);

        for (i = 0; i < para->intro_cnt; i++)
        {
            if (para->intro[i]) g_free(para->intro[i]);
        }

        for (i = 0; i < para->body_cnt; i++)
        {
            if (para->body[i]) g_free(para->body[i]);
        }

        ifree(para);
	}

	return FALSE;
}



static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(&g_efd, &g_oefd, sizeof(EA_EvtNotiFTPData));
		set_data_to_obj(1);
	}
	
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if(memcmp(&g_efd, &g_oefd, sizeof(EA_EvtNotiFTPData))) 
		{
			set_ef_data();
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			event_act_data_changed(TRUE);
		}
	}
	
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		VW_EvtNotiEvt_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_EvtNoti_FTP_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *chk_fixed;
	NFOBJECT *obj;

	const gchar *strOnOff[] = {"OFF", "ON"};	
	const gchar *strFrq[] = {"IMMEDIATELY",
	                        "1 MIN", 
                            "5 MIN", 
                            "10 MIN",
                            "15 MIN", 
                            "30 MIN", 
                            "60 MIN"
    };

	const gchar *strDirMode[] = {"MANUAL2", "MANUAL_DATE", "SYSTEM ID", "SYSTEM ID_DATE"};
	const gchar *strFileMode[] = {"MANUAL2", "MANUAL_DATE_TIME", "SYSTEM ID", "SYSTEM ID_DATE_TIME"};

	guint chk_w, chk_h;
    gint pos_x, pos_y;

	g_curwnd = nfui_nfobject_get_top(parent);

	init_ef_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "FTP NOTIFICATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 13);

    pos_x = 27;
    pos_y = 74;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FTP SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nftool_normal_button_create_subtab_type1("EDIT", 300);
	nfui_regi_post_event_callback(obj, post_edit_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);

    pos_y += (40 + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DIRECTORY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_combobox_new(strDirMode, 4, g_efd.dir_mode);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 300, 40);
	//nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_regi_post_event_callback(obj, post_dir_mode_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
    g_dirModeObj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);              
	nfui_nfobject_set_size(obj, 360, 40);		
	nfui_regi_post_event_callback(obj, post_dir_manual_event_handler);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4+300+2, pos_y);
    g_dirManualObj = obj;
      
    pos_y += (40 + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FILE NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_combobox_new(strFileMode, 4, g_efd.filename_mode);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 300, 40);
	//nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_regi_post_event_callback(obj, post_file_mode_event_handler);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
    g_fileModeObj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);           
	nfui_nfobject_set_size(obj, 360, 40);	
	nfui_regi_post_event_callback(obj, post_file_manual_event_handler);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4+300+2, pos_y);
    g_fileManualObj = obj;

    pos_y += (40 + 20);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MINIMUM FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_combobox_new(strFrq, NUM_FREQS, prvFreqToIndex(g_efd.freq));
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_regi_post_event_callback(obj, post_freq_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
    g_freqObj = obj;


    pos_y += (40 + 4);

	chk_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(chk_fixed, 300, 40);
	nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_show(chk_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, chk_fixed, pos_x+450+4, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCLUDE WEBRA LINK", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_checkbutton_new(g_efd.weblink);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_regi_post_event_callback(obj, post_weblink_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)chk_fixed, obj, (300-chk_w)/2, (40-chk_h)/2);
    g_webLinkObj = obj;

    pos_y += (40 + 4);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCLUDE SNAPSHOT IMAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	chk_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(chk_fixed, 300, 40);
	nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_show(chk_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, chk_fixed, pos_x+450+4, pos_y);

	obj = nfui_checkbutton_new(g_efd.include_jpeg);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_regi_post_event_callback(obj, post_snap_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)chk_fixed, obj, (300-chk_w)/2, (40-chk_h)/2);
    g_snapObj = obj;

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_snapObj);

//----->> INCLUDE VIDEO
    pos_y += (40 + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCLUDE VIDEO", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	chk_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(chk_fixed, 300, 40);
	nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_show(chk_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, chk_fixed, pos_x+450+4, pos_y);

	obj = nfui_checkbutton_new(g_efd.include_video);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_regi_post_event_callback(obj, post_video_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)chk_fixed, obj, (300-chk_w)/2, (40-chk_h)/2);
    g_videoObj = obj;


// HELP BUTTON
	obj = nftool_normal_button_create_subtab_type1("HELP", 95);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, FTP_NOTI_HELP_BTN_Y);
	nfui_regi_post_event_callback(obj, post_help_button_event_handler);


	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

    changeManualLabel(g_dirManualObj, (gint)g_efd.dir_mode, g_efd.dir_path);
    changeManualLabel(g_fileManualObj, (gint)g_efd.filename_mode, g_efd.filename);
	
}

gboolean check_evt_noti_ftp_changed()
{
	if (!memcmp(&g_efd, &g_oefd, sizeof(EA_EvtNotiFTPData))) 
        return FALSE;

	return TRUE;
}

void save_evt_noti_ftp_data()
{
	set_ef_data();
}

void restore_evt_noti_ftp_data()
{
	g_memmove(&g_efd, &g_oefd, sizeof(EA_EvtNotiFTPData));
	set_data_to_obj(0);
}

