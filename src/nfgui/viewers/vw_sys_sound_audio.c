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
#include "viewers/objects/nfcombobox.h"

#include "vw_sys_sound_main.h"
#include "vw_sys_sound_audio.h"
#include "scm.h"

#include "support/event_loop.h"
#include "nf_sysman.h"


#define	AUDIO_LABLE_LEFT		(28)
#define	AUDIO_LABLE_TOP			(42)

#define	AUDIO_LABLE_WIDTH		(433)
#define	AUDIO_LABLE_HEIGHT		(40)

#define	AUDIO_CELL_WIDTH		(261)
#define	AUDIO_CELL_HEIGHT		AUDIO_LABLE_HEIGHT

#define	AUDIO_COL_SPACE					(guint)(DISPLAY_IS_D1 ? 4:1)
#define	AUDIO_LABLE_HEIGHT_WITH_SPACE	(guint)(DISPLAY_IS_D1 ? 18:41)


#define AUDIO_INPUT_COUNT	GUI_CHANNEL_CNT

#define _SUPPORT_HDMI_AUDIO		(DAL_get_support_audio())

enum {
#if defined(_SUPPORT_HDMI_AUDIO)
	ROW_AUDIO_TYPE = 0,
#endif
	ROW_LIVE_AUD,
#if defined(_ENABLE_MICROPHONE_)
	ROW_MICROPHONE,
#endif
	ROW_NET_AUDIO_TR,
	ROW_NET_AUDIO_RE,
#if defined(_SEL_AUDIO_IN_DEV)
	ROW_CH1_IN_DEV,
	ROW_CH2_IN_DEV,
	ROW_CH3_IN_DEV,
	ROW_CH4_IN_DEV,
#ifndef GUI_4CH_SUPPORT
	ROW_CH5_IN_DEV,
	ROW_CH6_IN_DEV,
	ROW_CH7_IN_DEV,
	ROW_CH8_IN_DEV,
#ifndef GUI_8CH_SUPPORT
	ROW_CH9_IN_DEV,
	ROW_CH10_IN_DEV,
	ROW_CH11_IN_DEV,
	ROW_CH12_IN_DEV,
	ROW_CH13_IN_DEV,
	ROW_CH14_IN_DEV,
	ROW_CH15_IN_DEV,
	ROW_CH16_IN_DEV,
#ifndef GUI_16CH_SUPPORT
	ROW_CH17_IN_DEV,
	ROW_CH18_IN_DEV,
	ROW_CH19_IN_DEV,
	ROW_CH20_IN_DEV,			
	ROW_CH21_IN_DEV,
	ROW_CH22_IN_DEV,
	ROW_CH23_IN_DEV,
	ROW_CH24_IN_DEV,
	ROW_CH25_IN_DEV,
	ROW_CH26_IN_DEV,
	ROW_CH27_IN_DEV,
	ROW_CH28_IN_DEV,
	ROW_CH29_IN_DEV,
	ROW_CH30_IN_DEV,
	ROW_CH31_IN_DEV,
	ROW_CH32_IN_DEV,			
#endif	
#endif	
#endif	
#endif	
	ROW_MAX
};

enum {
	SAB_CANCEL = 0,
	SAB_APPLY,
	SAB_CLOSE,
	SAB_BUTTONS
};

static AudioData auddata;
static AudioData org_auddata;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[ROW_MAX];

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
		}
		break;

		default :
			break;
	}

	return FALSE;
}

static gboolean pre_test_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
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

		g_memmove(&auddata, &org_auddata, sizeof(AudioData));

#if defined(_SUPPORT_HDMI_AUDIO)
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_AUDIO_TYPE], auddata.audioType);
#endif

		if (auddata.liveAudio == 0xff)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_LIVE_AUD], 0);
		else
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_LIVE_AUD], auddata.liveAudio+1);
#if defined(_ENABLE_MICROPHONE_)
		nfui_combobox_set_index_no_expose(value[ROW_MICROPHONE], auddata.channel);
#endif
		nfui_spin_button_set_index(value[ROW_NET_AUDIO_TR], auddata.netAudioTx);
		nfui_spin_button_set_index(value[ROW_NET_AUDIO_RE], auddata.netAudioRx);
#if defined(_SEL_AUDIO_IN_DEV)
		for(i=0; i<GUI_CHANNEL_CNT; i++)
			nfui_combobox_set_index_no_expose(value[ROW_CH1_IN_DEV+i], auddata.inDev[i]);
#endif
		for(i=0; i<ROW_MAX; i++)
		{
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, tmp;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

#if defined(_SUPPORT_HDMI_AUDIO)
		auddata.audioType = nfui_combobox_get_cur_index(value[ROW_AUDIO_TYPE]);
#endif
	
		tmp = nfui_combobox_get_cur_index(value[ROW_LIVE_AUD]);
		if (tmp == 0)
			auddata.liveAudio = 0xff;
		else
			auddata.liveAudio = tmp-1;	
#if defined(_ENABLE_MICROPHONE_)
		auddata.channel = nfui_combobox_get_cur_index(value[ROW_MICROPHONE]);
#endif
		auddata.netAudioTx = nfui_spin_button_get_index(value[ROW_NET_AUDIO_TR]);
		auddata.netAudioRx = nfui_spin_button_get_index(value[ROW_NET_AUDIO_RE]);
#if defined(_SEL_AUDIO_IN_DEV)
		for(i=0; i<GUI_CHANNEL_CNT; i++)
			auddata.inDev[i] = nfui_combobox_get_cur_index(value[ROW_CH1_IN_DEV+i]);
#endif
		if(memcmp(&org_auddata, &auddata, sizeof(AudioData)))
		{
			scm_put_log(CHANGE_AUDIO, 0, 0);

			g_memmove(&org_auddata, &auddata, sizeof(AudioData));

			DAL_set_audio_data(&auddata);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			syssnd_set_changeflag(1);
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
	
		Audio_tab_out_handler();

		SystemSetupSound_Destroy(obj);
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


void init_SndAudio_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *subject[ROW_MAX];
	NFOBJECT *audio_btns[SAB_BUTTONS];	
											
	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
	const gchar *strTitle[] = {
#if defined(_SUPPORT_HDMI_AUDIO)
				"AUDIO OUTPUT TYPE",
#endif
				"DEFAULT LIVE AUDIO CHANNEL",
#if defined(_ENABLE_MICROPHONE_)					
				"MICROPHONE",
#endif				
				"NETWORK AUDIO TRANSMISSION",
				"NETWORK AUDIO RECEIVE",
#if defined(_SEL_AUDIO_IN_DEV)
				"AUDIO CH1 INPUT",
				"AUDIO CH2 INPUT",
				"AUDIO CH3 INPUT",
				"AUDIO CH4 INPUT",
#ifndef GUI_4CH_SUPPORT
				"AUDIO CH5 INPUT",				
				"AUDIO CH6 INPUT",
				"AUDIO CH7 INPUT",
				"AUDIO CH8 INPUT",
#ifndef GUI_8CH_SUPPORT
				"AUDIO CH9 INPUT",
				"AUDIO CH10 INPUT",
				"AUDIO CH11 INPUT",
				"AUDIO CH12 INPUT",
				"AUDIO CH13 INPUT",
				"AUDIO CH14 INPUT",
				"AUDIO CH15 INPUT",
				"AUDIO CH16 INPUT",
#ifndef GUI_16CH_SUPPORT
				"AUDIO CH17 INPUT",
				"AUDIO CH18 INPUT",
				"AUDIO CH19 INPUT",
				"AUDIO CH20 INPUT",
				"AUDIO CH21 INPUT",
				"AUDIO CH22 INPUT",
				"AUDIO CH23 INPUT",
				"AUDIO CH24 INPUT",
				"AUDIO CH25 INPUT",
				"AUDIO CH26 INPUT",
				"AUDIO CH27 INPUT",
				"AUDIO CH28 INPUT",
				"AUDIO CH29 INPUT",
				"AUDIO CH30 INPUT",
				"AUDIO CH31 INPUT",
				"AUDIO CH32 INPUT",																								
#endif
#endif
#endif
#endif
	};
	const gchar *strOffOn[] = {"OFF", "ON"};
	
	const gchar *strAudioCh[] = {"CAM 1", "CAM 2", "CAM 3", "CAM 4",
								 "CAM 5", "CAM 6", "CAM 7", "CAM 8",
								 "CAM 9", "CAM 10", "CAM 11", "CAM 12",
								 "CAM 13", "CAM 14", "CAM 15", "CAM 16",
								 "CAM 17", "CAM 18", "CAM 19", "CAM 20",
								 "CAM 21", "CAM 22", "CAM 23", "CAM 24",
								 "CAM 25", "CAM 26", "CAM 27", "CAM 28",
								 "CAM 29", "CAM 30", "CAM 31", "CAM 32"};

	const gchar *strInDev[] = {"REAR", "CAMERA"};
#if defined(_SUPPORT_HDMI_AUDIO)
	const gchar *strType[] = {"RCA", "HDMI"};
#endif
	guint btn_x, btn_y, btn_space;
	guint pos_y;
	guint i;



	g_curwnd = nfui_nfobject_get_top(parent);


// FIXED
	memset(&auddata, 0x00, sizeof(AudioData));
	memset(&org_auddata, 0x00, sizeof(AudioData));

	DAL_get_audio_data(&auddata);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
#if defined(_SUPPORT_HDMI_AUDIO)
	value[ROW_AUDIO_TYPE] = nfui_combobox_new(strType, 2, auddata.audioType);
#endif
	value[ROW_LIVE_AUD] = nfui_combobox_new(strAudioCh, AUDIO_INPUT_COUNT, 0);
	nfui_combobox_prepend_data((NFCOMBOBOX*)value[ROW_LIVE_AUD], "OFF");
	if (auddata.liveAudio == 0xff)
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_LIVE_AUD], 0);
	else
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_LIVE_AUD], auddata.liveAudio+1);

	if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B)	nfui_nfobject_disable(value[ROW_AUDIO_TYPE]);
	if (!DAL_get_support_audio()) nfui_nfobject_disable(value[ROW_LIVE_AUD]);

#if defined(_ENABLE_MICROPHONE_)
	value[ROW_MICROPHONE] = nfui_combobox_new(strAudioCh, AUDIO_INPUT_COUNT, auddata.channel);
#endif
	value[ROW_NET_AUDIO_TR] = nfui_spinbutton_new(strOffOn, 2, auddata.netAudioTx);
	value[ROW_NET_AUDIO_RE] = nfui_spinbutton_new(strOffOn, 2, auddata.netAudioRx);
	
	if (!DAL_get_support_audio()) {
		nfui_nfobject_disable(value[ROW_NET_AUDIO_TR]);
		nfui_nfobject_disable(value[ROW_NET_AUDIO_RE]);
	}

#if defined(_SEL_AUDIO_IN_DEV)
	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		value[ROW_CH1_IN_DEV+i] = nfui_combobox_new(strInDev, 2, auddata.inDev[i]);
#endif

	pos_y = AUDIO_LABLE_TOP;

	for(i = 0; i < ROW_MAX; i++)
	{
		subject[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)subject[i], NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(subject[i], AUDIO_LABLE_WIDTH, AUDIO_LABLE_HEIGHT);
		nfui_nfobject_modify_bg(subject[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(subject[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(subject[i]);

		if (i < ROW_NET_AUDIO_TR)
		{
			nfui_combobox_set_skin_type(NF_COMBOBOX(value[i]), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(value[i]), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(value[i], AUDIO_CELL_WIDTH, AUDIO_CELL_HEIGHT);
			nfui_nfobject_show(value[i]);
		}
#if defined(_SEL_AUDIO_IN_DEV)
		else if ((i >= ROW_CH1_IN_DEV) && (i < ROW_MAX))
		{
			nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);			
			nfui_combobox_set_arrow_image(NF_COMBOBOX(value[i]), dropdown_img, size_w, size_h);
			nfui_combobox_set_pango_font(NF_COMBOBOX(value[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), CR_FG_NR_CELL_TYP2);
			nfui_combobox_set_menu_bg_color(NF_COMBOBOX(value[i]), btn_color);
			nfui_combobox_set_menu_font_color(NF_COMBOBOX(value[i]), font_color);
			nfui_combobox_set_align(NF_COMBOBOX(value[i]), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(value[i], AUDIO_CELL_WIDTH, AUDIO_CELL_HEIGHT);
			nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, CR_BG_NR_CELL_TYP2);
			nfui_nfobject_show(value[i]);
		}		
#endif		
		else
		{
			nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[i], NFSPINBUTTON_TYPE_1);
			nfui_nfobject_set_size(value[i], AUDIO_CELL_WIDTH, AUDIO_CELL_HEIGHT);
			nfui_nfobject_show(value[i]);
		}
		
		nfui_nffixed_put((NFFIXED*)content_fixed, subject[i], AUDIO_LABLE_LEFT, pos_y);
		nfui_nffixed_put((NFFIXED*)content_fixed, value[i], AUDIO_LABLE_LEFT+AUDIO_LABLE_WIDTH+AUDIO_COL_SPACE, pos_y);

		pos_y += AUDIO_LABLE_HEIGHT_WITH_SPACE;
#if defined(_SEL_AUDIO_IN_DEV)
		if (i == ROW_NET_AUDIO_RE) pos_y += 8;
#endif		
	}
	
	audio_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(audio_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(audio_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, audio_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	audio_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(audio_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(audio_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, audio_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	audio_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(audio_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(audio_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, audio_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(audio_btns[SAB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(audio_btns[SAB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(audio_btns[SAB_CLOSE], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(&org_auddata, &auddata, sizeof(AudioData));
}

gboolean Audio_tab_out_handler()
{
	gint i, tmp;
	mb_type ret;

#if defined(_SUPPORT_HDMI_AUDIO)
	auddata.audioType = nfui_combobox_get_cur_index(value[ROW_AUDIO_TYPE]);
#endif
	tmp = nfui_combobox_get_cur_index(value[ROW_LIVE_AUD]);

	if (tmp == 0)
		auddata.liveAudio = 0xff;
	else
		auddata.liveAudio = tmp - 1;
#if defined(_ENABLE_MICROPHONE_)
	auddata.channel = nfui_combobox_get_cur_index(value[ROW_MICROPHONE]);
#endif
	auddata.netAudioTx = nfui_spin_button_get_index(value[ROW_NET_AUDIO_TR]);
	auddata.netAudioRx = nfui_spin_button_get_index(value[ROW_NET_AUDIO_RE]);
#if defined(_SEL_AUDIO_IN_DEV)
	for(i=0; i<GUI_CHANNEL_CNT; i++)
		auddata.inDev[i] = nfui_combobox_get_cur_index(value[ROW_CH1_IN_DEV+i]);
#endif

	if(!memcmp(&org_auddata, &auddata, sizeof(AudioData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		scm_put_log(CHANGE_AUDIO, 0, 0);

		g_memmove(&org_auddata, &auddata, sizeof(AudioData));
		DAL_set_audio_data(&auddata);
		
		syssnd_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&auddata, &org_auddata, sizeof(AudioData));
		
#if defined(_SUPPORT_HDMI_AUDIO)
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_AUDIO_TYPE], auddata.audioType);
#endif

		if (auddata.liveAudio == 0xff)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_LIVE_AUD], 0);
		else
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_LIVE_AUD], auddata.liveAudio+1);
#if defined(_ENABLE_MICROPHONE_)
		nfui_combobox_set_index_no_expose(value[ROW_MICROPHONE], auddata.channel);
#endif
		nfui_spin_button_set_index_no_expose(value[ROW_NET_AUDIO_TR], auddata.netAudioTx);
		nfui_spin_button_set_index_no_expose(value[ROW_NET_AUDIO_RE], auddata.netAudioRx);
#if defined(_SEL_AUDIO_IN_DEV)
		for(i=0; i<GUI_CHANNEL_CNT; i++)
			nfui_combobox_set_index_no_expose(value[ROW_CH1_IN_DEV+i], auddata.inDev[i]);
#endif		
	}

	return FALSE;

}


