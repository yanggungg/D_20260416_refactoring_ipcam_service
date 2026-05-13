#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"
#include "tools/ix_mem.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "support/multi_language_support.h"

//#include "nf_ui_disp_main.h"
//#include "nf_ui_osd.h"

#include "vw_sys_disp_main.h"
#include "vw_sys_disp_osd.h"
#include "scm.h"
#include "evt.h"

#define	NUM_DO_COLUMNS			(2)
#define	DO_COL_SPACE			(2)
#define	DO_ROW_SPACE			(1)

#define	DO_TABLE_LEFT			(8)
#define	DO_TABLE_TOP			(0)

#define	DO_LABEL_HEIGHT			(40)
#define NUM_MOTION_COLORS		(4)
#define LANGUAGE_STR_SIZE		(32)

#define	__TRANSPARENT_DISABLE__

enum{
	DO_CAMERA_TITLE = 0,
	DO_EVENT_ICON,
	DO_AUDIO,
	DO_SIDE_ICON,
	DO_STATUS_ICON,
	//
	DO_STATUS_BAR,
	DO_TIMELINE,
	DO_ZOOMPIP,
	DO_BORDER,
	DO_BORDER_COLOR,
	DO_USER_NAME,
	DO_TIME,
	//
	DO_LANG,
	//
	NUM_DO_ROWS
};



enum {
	DOB_CANCEL = 0,
	DOB_APPLY,
	DOB_CLOSE,
	DOB_BUTTONS
};

enum {
	STATUSBAR_TIMEOUT_AUTO_HIDE = 0,
	STATUSBAR_TIMEOUT_ALWAYS,
	STATUSBAR_TIMEOUT_5_SEC,
	STATUSBAR_TIMEOUT_10_SEC,
	STATUSBAR_TIMEOUT_15_SEC,
	STATUSBAR_TIMEOUT_20_SEC,
	STATUSBAR_TIMEOUT_30_SEC,
	STATUSBAR_TIMEOUT_60_SEC,
	NUM_STATUSBAR_TIMEOUT,
};

enum {
	BORDER_COLOR_NONE = -1,
	BORDER_COLOR_WHITE = 0,
	BORDER_COLOR_GRAY,
	BORDER_COLOR_YELLOW,
	BORDER_COLOR_BLUE,
	BORDER_COLOR_GREEN,
	BORDER_COLOR_RED,

	NUM_BORDER_COLORS,
};

enum {
	LANG_ENGLISH = 0,
	LANG_KOREAN,
	LANG_JAPANESE,
#if 0
	LANG_ITALIAN,
	LANG_SPANISH,
	LANG_FRENCH,
	LANG_GERMAN,
	LANG_RUSSIAN,
	LANG_POLISH,
	LANG_TURKISH,
	LANG_THAI,
	LANG_CHINESE_S,
	LANG_CHINESE_T,
	LANG_PORTUGUESE,
	LANG_BRAZIL,
	LANG_DUTCH,
	LANG_GREEK,
	LANG_BULGARIAN,
#endif
	NUM_LANGUAGES,
};

enum {
	TIMELINE_AUTO_HIDE = 0,
	TIMELINE_ALWAYS_OFF,
	TIMELINE_ALWAYS_ON,
	NUM_TIMELINE,
};

enum {
    ZOOMPIP_AUTOHIDE_ALWAYS = 0,
    ZOOMPIP_AUTOHIDE_1_SEC,
    ZOOMPIP_AUTOHIDE_3_SEC,
    ZOOMPIP_AUTOHIDE_5_SEC,    
    NUM_ZOOMPIP_AUTOHIDE
};

static const gchar *strTimeline[NUM_TIMELINE] = {
	"AUTO HIDE",
	"ALWAYS OFF",
	"ALWAYS ON",
};


static OsdData osddata;
static OsdData org_osddata;

static const gchar *strTimeout[NUM_STATUSBAR_TIMEOUT] = {
	"AUTO HIDE",
	"ALWAYS ON",
	"5 SEC",
	"10 SEC",
	"15 SEC",
	"20 SEC",
	"30 SEC",
	"1 MIN"
};

static const gchar *motColor[NUM_MOTION_COLORS] = {
	"YELLOW",
	"BLUE",
	"GREEN",
	"RED"
};

static const gchar *strColor[NUM_BORDER_COLORS] = {
	"WHITE",
	"DARK GRAY",
	"YELLOW",
	"BLUE",
	"GREEN",
	"RED"
};

static const gchar *strZoompip[NUM_ZOOMPIP_AUTOHIDE] = {
    "ALWAYS ON",
    "1 SEC",
    "3 SEC",
    "5 SEC"
};

#if 0
static const gchar *strLang[NUM_LANGUAGES] = {
	"ENGLISH",
	"KOREAN",
    "JAPANESE",
#if 0
	"ITALIAN",
    "SPANISH",
	"FRENCH",
	"GERMAN",
    "RUSSIAN",
    "POLISH",
	"TURKISH",
    "THAI",
    "CHINESE(S)",
    "CHINESE(T)",
    "PORTUGUESE PT",
    "PORTUGUESE BR",
    "DUTCH",     
    "GREEK",
    "BULGARIAN",
#endif
};
#else
static gchar **strLang = NULL;
static gint langCnt = 0;
#endif

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value_object[NUM_DO_ROWS];

static guint prvIndexToTimeout(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case STATUSBAR_TIMEOUT_AUTO_HIDE:	ret = 1;		break;
		case STATUSBAR_TIMEOUT_ALWAYS:		ret = 0;		break;
		case STATUSBAR_TIMEOUT_5_SEC:		ret = 5;		break;
		case STATUSBAR_TIMEOUT_10_SEC:		ret = 10;		break;
		case STATUSBAR_TIMEOUT_15_SEC:		ret = 15;		break;
		case STATUSBAR_TIMEOUT_20_SEC:		ret = 20;		break;
		case STATUSBAR_TIMEOUT_30_SEC:		ret = 30;		break;
		case STATUSBAR_TIMEOUT_60_SEC:		ret = 60;		break;
		default:							ret = 10;		break;
	}

	return ret;
}

static guint prvIndexToTimeline(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case TIMELINE_AUTO_HIDE:	ret = 0;		break;
		case TIMELINE_ALWAYS_OFF:	ret = 1;		break;
		case TIMELINE_ALWAYS_ON:	ret = 2;		break;
		
		default:	ret = 10;	break;
	}

	return ret;
}

static guint  prvTimelineToIndex(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case TIMELINE_AUTO_HIDE:	ret = 0;		break;
		case TIMELINE_ALWAYS_OFF:	ret = 1;		break;
		case TIMELINE_ALWAYS_ON:	ret = 2;		break;
		
		default:	ret = 10;	break;
	}

	return ret;
}

static guint prvTimeoutToIndex(guint timeout)
{
	gint ret = 0;

	if(timeout == 1)		ret = STATUSBAR_TIMEOUT_AUTO_HIDE;
	else if(timeout < 5)	ret = STATUSBAR_TIMEOUT_ALWAYS;
	else if(timeout < 10)	ret = STATUSBAR_TIMEOUT_5_SEC;
	else if(timeout < 15)	ret = STATUSBAR_TIMEOUT_10_SEC;
	else if(timeout < 20)	ret = STATUSBAR_TIMEOUT_15_SEC;
	else if(timeout < 30)	ret = STATUSBAR_TIMEOUT_20_SEC;
	else if(timeout < 60)	ret = STATUSBAR_TIMEOUT_30_SEC;
	else					ret = STATUSBAR_TIMEOUT_60_SEC;

	return ret;
}

static guint prvIndexToZoompip(gint index)
{
    guint ret = 0;

    switch(index)
    {
        case ZOOMPIP_AUTOHIDE_ALWAYS:   ret = 0;    break;
        case ZOOMPIP_AUTOHIDE_1_SEC:    ret = 1;    break;
        case ZOOMPIP_AUTOHIDE_3_SEC:    ret = 3;    break;
        case ZOOMPIP_AUTOHIDE_5_SEC:    ret = 5;    break;
        default:                        ret = 0;    break;
    }

    return ret;
}

static guint prvZoompipToIndex(guint timeout)
{
    gint ret = 0;

    if(timeout == 0)        ret = ZOOMPIP_AUTOHIDE_ALWAYS;
    if(timeout == 1)        ret = ZOOMPIP_AUTOHIDE_1_SEC;
    else if(timeout == 3)   ret = ZOOMPIP_AUTOHIDE_3_SEC;
    else if(timeout == 5)   ret = ZOOMPIP_AUTOHIDE_5_SEC;
    else                    ret = ZOOMPIP_AUTOHIDE_ALWAYS;

    return ret;
}

static gboolean init_lang()
{
	gint i, j = 0;
	gchar strLangAlias[64];
	gint cnt;

	memset(strLangAlias, 0x00, sizeof(strLangAlias));

	cnt = DAL_get_support_lang_cnt();
	
	if (cnt < 0) return FALSE;

	for (i = 0; i < cnt; i++)
	{
		memset(strLangAlias, 0x00, sizeof(strLangAlias));
		DAL_get_support_lang_alias(i, strLangAlias);
		if (strlen(strLangAlias)) langCnt++;
	}

	strLang = (gchar**)imalloc(sizeof(gchar*)*langCnt);
	
	for (i = 0; i < cnt; i++) 
	{
		memset(strLangAlias, 0x00, sizeof(strLangAlias));
		DAL_get_support_lang_alias(i, strLangAlias);
		
		if (strlen(strLangAlias)) {
			strLang[j] = (gchar*)imalloc(LANGUAGE_STR_SIZE);
			g_assert(strLang[j]);

			DAL_get_support_lang(i, strLang[j]);
			j++;
		}
	}

	g_message("%s, %d, langCnt:%d, j:%d", __FUNCTION__, __LINE__, langCnt, j);

	return TRUE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		default :
			break;
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

		g_memmove(&osddata, &org_osddata, sizeof(OsdData));

		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_STATUS_BAR]), prvTimeoutToIndex(osddata.statusBar));
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_CAMERA_TITLE]), osddata.camTitle);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_EVENT_ICON]), osddata.evtIcon);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_SIDE_ICON]), osddata.sideicon);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_STATUS_ICON]), osddata.statusicon);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_BORDER]), osddata.border);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_BORDER_COLOR]), osddata.borderColor);

        nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)(value_object[DO_LANG]), osddata.lang);		

		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_AUDIO]), osddata.audio);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_TIMELINE]), osddata.timeline);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_USER_NAME]), osddata.user_name);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_TIME]), osddata.time);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[DO_ZOOMPIP]), prvZoompipToIndex(osddata.zoompip));

		for(i=0; i<NUM_DO_ROWS; i++)
		{
			nfui_signal_emit(value_object[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean _proc_escape(void *data)
{
	NFOBJECT *wait_pop = (NFOBJECT *)data;
	
	nftool_remove_waitbox(wait_pop);
	evt_send_to_window("SYSTEM SETUP", WND_CLOSE, 0, 0, 0);
	evt_send_to_local(IREQ_CHANGE_LANG, 0, 0, 0);

//	VW_Live_StatusBar_Close();
//	VW_Timeline_Close();
//	VW_Destroy_OSD_Popup();
//	VW_Destroy_ShortCut_Menu();

//	evt_send_to_local(INFY_LANG_CHANGED, 0, 0, 0);
	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *wait_pop;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	       }
	
		int language_changed = 0;

		osddata.statusBar = prvIndexToTimeout(nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_STATUS_BAR])));
		
		osddata.camTitle = nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_CAMERA_TITLE]));
		osddata.evtIcon = nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_EVENT_ICON]));
		osddata.sideicon = nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_SIDE_ICON]));
		osddata.statusicon = nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_STATUS_ICON]));
		osddata.border = nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_BORDER]));
		osddata.borderColor = nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_BORDER_COLOR]));
		if(strcmp(osddata.lang, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[DO_LANG]))))	{
			language_changed = 1;
		}
		g_stpcpy(osddata.lang, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[DO_LANG])));

		osddata.audio 		= nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_AUDIO]));
		osddata.timeline	= nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_TIMELINE]));
		osddata.user_name	= nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_USER_NAME]));
		osddata.time		= nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_TIME]));
		osddata.zoompip		= prvIndexToZoompip(nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_ZOOMPIP])));

		if(memcmp(&org_osddata, &osddata, sizeof(OsdData)))
		{
			g_memmove(&org_osddata, &osddata, sizeof(OsdData));
			DAL_set_osd_data(&osddata);

			scm_put_log(CHANGE_DISP_OSD, 0, 0);

			if(language_changed)	{
				wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");
				sysdisp_set_changeflag(1);

				DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);

				g_timeout_add(1000, _proc_escape, wait_pop);
			}
			else {
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
	
		Osd_tab_out_handler();

		SystemSetupDisp_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		gint i;

		g_curwnd = 0;

		if (strLang) 
		{
			for (i = 0; i < langCnt; i++)  
			{
				if (strLang[i]) {
					ifree(strLang[i]);
					strLang[i] = NULL;
				}
			}

			ifree(strLang);
			strLang = 0;
		}
	}

	return FALSE;
}



//////////////////////////////////////////////////////////////////
//
//
//

void init_DispOsd_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *subject_object[NUM_DO_ROWS];
	NFOBJECT *disposd_btns[DOB_BUTTONS];

	const gchar *strTitle[] = {
					"CAMERA TITLE",
					"RECORDING MODE ICON",
					"AUDIO ICON",
					"WALLPAPER ICON",
					"SYSTEM STATUS ICON",
					
					"STATUS BAR ON FULL SCREEN MODE",
					"TIMELINE ON FULL SCREEN MODE",
					"ZOOM PIP",
					"BORDER LINE",
					"BORDER COLOR",
					"USER NAME",
					"TIME",

					"LANGUAGE"
	};


	const gchar *strOffOn[] = {"OFF", "ON"};

	guint width[NUM_DO_COLUMNS];
	guint btn_x, btn_y, btn_space;
	guint i;


	g_curwnd = nfui_nfobject_get_top(parent);

		width[0] = 420;
		width[1] = 261;

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	memset(&osddata, 0x00, sizeof(OsdData));
	memset(&org_osddata, 0x00, sizeof(OsdData));

	DAL_get_osd_data(&osddata);

	langCnt = 0;

	if(!init_lang()) {
		g_warning("%s [%d] : init support language error", __FUNCTION__, __LINE__);
		return;
	}


// TITLE BAR
	const gchar *title_name[] = {"DISPLAY", "SCREEN MODE", "LANGUAGE"};
	gint title_xpos = 8;
	gint title_ypos[3] = {
						 DO_TABLE_TOP,
						 DO_TABLE_TOP+40+21+42+42+42+42+40+60,
						 DO_TABLE_TOP+40+21+42+42+42+42+42+40+60+40+21+42+42+42+42+42+40+60
						 };
	
	for( i = 0 ; i < 3 ; i++ )
	{
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, title_name[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), 
									COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, title_xpos, title_ypos[i]);
	}


// SHOW
	value_object[DO_CAMERA_TITLE] 	= (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 
													2, (gint)osddata.camTitle);
	value_object[DO_EVENT_ICON] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 
													2, (gint)osddata.evtIcon);
	value_object[DO_AUDIO] 		= nfui_spinbutton_new((gchar**)strOffOn, 
													2, (gint)osddata.audio);

	if (!DAL_get_support_audio()) nfui_nfobject_disable(value_object[DO_AUDIO]);

	value_object[DO_SIDE_ICON] 	= nfui_spinbutton_new((gchar**)strOffOn, 
													2, (gint)osddata.sideicon);

#if !defined(GUI_8CH_SUPPORT)
	nfui_nfobject_disable(value_object[DO_SIDE_ICON]);
#endif

	value_object[DO_STATUS_ICON] 	= nfui_spinbutton_new((gchar**)strOffOn, 
													2, (gint)osddata.statusicon);

	/////////////////////////////////////////////////////////////////////

	value_object[DO_STATUS_BAR] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strTimeout, 
										NUM_STATUSBAR_TIMEOUT, 
										(gint)prvTimeoutToIndex(osddata.statusBar));

	value_object[DO_TIMELINE] 	= nfui_spinbutton_new((gchar**)strTimeline, 
										NUM_TIMELINE, 
										(gint)osddata.timeline);

	value_object[DO_ZOOMPIP] 	= nfui_spinbutton_new((gchar**)strZoompip,
										NUM_ZOOMPIP_AUTOHIDE,
										(gint)prvZoompipToIndex(osddata.zoompip));

	value_object[DO_BORDER] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 
										2, (gint)osddata.border);
	
	value_object[DO_BORDER_COLOR] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strColor, 
										NUM_BORDER_COLORS, (gint)osddata.borderColor);

	value_object[DO_USER_NAME] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 
										2, (gint)osddata.user_name);
	
	value_object[DO_TIME] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 
										2, (gint)osddata.time);

	/////////////////////////////////////////////////////////////////////

	//value_object[DO_LANG] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strLang, 
	//									NUM_LANGUAGES, 0);
	value_object[DO_LANG] = (NFOBJECT*)nfui_spinbutton_new(strLang, langCnt, 0);
	nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)(value_object[DO_LANG]), osddata.lang);
// below is test, SKSHIN
//nfui_spinbutton_set_skin_type(value_object[DO_LANG], NFSPINBUTTON_TYPE_1);
//nfui_spin_button_set_pango_font(value_object[DO_LANG], nffont_get_pango_font(NFFONT_SMALL_THIN));

	gint xpos = 28;
	gint ypos[13] =  {
					DO_TABLE_TOP+40+21,
					DO_TABLE_TOP+40+21+42,
					DO_TABLE_TOP+40+21+42+42,
					DO_TABLE_TOP+40+21+42+42+42,
					DO_TABLE_TOP+40+21+42+42+42+42,

					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21,
					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42,
					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42+42,
					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42+42+42,
					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42+42+42+42,
					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42+42+42+42+42,
					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42+42+42+42+42+42,

					DO_TABLE_TOP+40+21+42+42+42+42+40+60+40+21+42+42+42+42+42+42+40+60+40+21,
					};

	
	for(i = 0 ; i < NUM_DO_ROWS ; i++)
	{
		subject_object[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)strTitle[i], 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)subject_object[i], NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(subject_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(subject_object[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(subject_object[i], 400, 40);
		nfui_nfobject_show(subject_object[i]);
		nfui_nffixed_put((NFFIXED*)content_fixed, subject_object[i], xpos, ypos[i]);

		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value_object[i], NFSPINBUTTON_TYPE_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)value_object[i], NFALIGN_LEFT, 4);
		nfui_spin_button_set_spacing((NFSPINBUTTON*)value_object[i], CONDENSED_SPACING);
		nfui_nfobject_set_size(value_object[i], 250, 40);
		nfui_nfobject_show(value_object[i]);
		nfui_nffixed_put((NFFIXED*)content_fixed, value_object[i], xpos+400, ypos[i]);
	}

	
	disposd_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(disposd_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(disposd_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, disposd_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	disposd_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(disposd_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(disposd_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, disposd_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	disposd_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(disposd_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(disposd_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, disposd_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(disposd_btns[0], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(disposd_btns[1], post_applybutton_event_handler);
	nfui_regi_post_event_callback(disposd_btns[2], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(&org_osddata, &osddata, sizeof(OsdData));
}



gboolean Osd_tab_out_handler()
{
	mb_type ret;
	int language_changed = 0;

	osddata.statusBar = (guint)prvIndexToTimeout(nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_STATUS_BAR])));
	osddata.camTitle = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_CAMERA_TITLE]));
	osddata.evtIcon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_EVENT_ICON]));
	osddata.sideicon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_SIDE_ICON]));
	osddata.statusicon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_STATUS_ICON]));
	osddata.border = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_BORDER]));
	osddata.borderColor = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_BORDER_COLOR]));

	if(strcmp(osddata.lang, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[DO_LANG]))))	{
		language_changed = 1;
	}
	
	g_stpcpy(osddata.lang, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[DO_LANG])));

	osddata.audio 		= (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_AUDIO]));
	osddata.timeline 	= (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_TIMELINE]));
	osddata.user_name 	= (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_USER_NAME]));
	osddata.time	 	= (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_TIME]));
	osddata.zoompip 	= (guint)prvIndexToZoompip(nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[DO_ZOOMPIP])));

	if(!memcmp(&org_osddata, &osddata, sizeof(OsdData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		g_memmove(&org_osddata, &osddata, sizeof(OsdData));
		DAL_set_osd_data(&osddata);

		sysdisp_set_changeflag(1);

		scm_put_log(CHANGE_DISP_OSD, 0, 0);
		
		if(language_changed)	{
			DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
	
//			nftool_mbox(g_curwnd, "NOTICE", "Language has been changed.\nThe system will be reboot soon.", NFTOOL_MB_OK);
			// for convenience 
			//
			evt_send_to_window("SYSTEM SETUP", WND_CLOSE, 0, 0, 0);
			evt_send_to_local(IREQ_CHANGE_LANG, 0, 0, 0);
			
		}
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&osddata, &org_osddata, sizeof(OsdData));

		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_STATUS_BAR], 
										prvTimeoutToIndex(osddata.statusBar));
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_CAMERA_TITLE], osddata.camTitle);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_EVENT_ICON], osddata.evtIcon);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_SIDE_ICON], osddata.sideicon);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_STATUS_ICON], osddata.statusicon);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_AUDIO], osddata.audio);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_TIMELINE], osddata.timeline);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_USER_NAME], osddata.user_name);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_TIME], osddata.time);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_BORDER], osddata.border);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_BORDER_COLOR], osddata.borderColor);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value_object[DO_ZOOMPIP], prvZoompipToIndex(osddata.zoompip));

		nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)(value_object[DO_LANG]), osddata.lang);

	}

	return FALSE;

}
