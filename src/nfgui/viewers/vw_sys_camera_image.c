#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_image.h"
//#include "nf_ui_color_setup_bar.h"

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
#include "vw_image_setup_preview.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "vw_vkeyboard.h"
#include "scm.h"
#include "vsm.h"
#include "vw.h"

#include "vw_image_setup_advanced.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define	COLOR_SETUP_COLUMNS				(5)

#define	COLOR_SETUP_COL_SPACE			(2)
#define	COLOR_SETUP_ROW_SPACE			(1)

#define	COLOR_SETUP_LABEL_HEIGHT		(40)

#define IMG_THUMBNAIL_START_X			(8+1)
#define IMG_THUMBNAIL_START_Y			(39+1)

#define CHANNEL_LABEL_X					(8)
#define CHANNEL_LABEL_Y					(39+1+228+1+8)
#define CHANNEL_LABEL_W					(112)
#define CHANNEL_LABEL_H					(40)

#define CHANNEL_COMBO_X					(CHANNEL_LABEL_X + CHANNEL_LABEL_W + 10)
#define CHANNEL_COMBO_Y					(39+1+228+1+8)
#define CHANNEL_COMBO_W					(220+8+60)

#define BRIGHTNESS_LABEL_X				(CHANNEL_LABEL_X)
#define BRIGHTNESS_LABEL_Y				(CHANNEL_LABEL_Y + CHANNEL_LABEL_H + 10)
#define BRIGHTNESS_LABEL_W				(CHANNEL_LABEL_W)

#define BRIGHTNESS_SLIDER_X				(BRIGHTNESS_LABEL_X + BRIGHTNESS_LABEL_W + 10)
#define BRIGHTNESS_SLIDER_Y				(BRIGHTNESS_LABEL_Y)
#define BRIGHTNESS_SLIDER_W				(220)

#define BRIGHTNESS_CONTROL_X			(BRIGHTNESS_SLIDER_X + BRIGHTNESS_SLIDER_W + 8)
#define BRIGHTNESS_CONTROL_Y			(BRIGHTNESS_SLIDER_Y)
#define BRIGHTNESS_CONTROL_W			(60)

#define DEF_BTN_X						(BRIGHTNESS_CONTROL_X + BRIGHTNESS_CONTROL_W - DEF_BTN_W)
#define DEF_BTN_Y						(BRIGHTNESS_CONTROL_Y + COLOR_SETUP_LABEL_HEIGHT*4 + 2*3 + 10)
#define DEF_BTN_W						(185)

#define	COLOR_SETUP_TABLE_LEFT			(8+1+408+1+32)
#define	COLOR_SETUP_TABLE_TOP			(39)

enum {
	ATTR_BRIGHT = 0,
	ATTR_CONTRAST,
	ATTR_TINT,
	ATTR_COLOR,
	ATTR_TYPE
};

enum {
	CSB_CANCEL = 0,
	CSB_APPLY,
	CSB_CLOSE,
	CSB_BUTTONS
};

enum {
	OBJ_NO_EXPOSE = 0,
	OBJ_EXPOSE,
};

static ColorData coldata[GUI_CHANNEL_CNT];
static ColorData org_coldata[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static AdvancedData advdata[GUI_CHANNEL_CNT];
static AdvancedData org_advdata[GUI_CHANNEL_CNT];

static gint g_reset_msg_check;

static NFOBJECT *channel_obj;
static CWSLIDER *g_attr_slider[ATTR_TYPE];
static NFOBJECT *g_attr_label[ATTR_TYPE];
static NFOBJECT *g_attr_bright_spin[GUI_CHANNEL_CNT];
static NFOBJECT *g_attr_cont_spin[GUI_CHANNEL_CNT];
static NFOBJECT *g_attr_tint_spin[GUI_CHANNEL_CNT];
static NFOBJECT *g_attr_color_spin[GUI_CHANNEL_CNT];
static NFOBJECT *g_ch_imglabel_obj[GUI_CHANNEL_CNT];
static NFOBJECT *preview_obj[GUI_CHANNEL_CNT];
static NFOBJECT *advanced_obj[GUI_CHANNEL_CNT];
static NFOBJECT *video_obj;
static NFOBJECT *def_btn;

static NFIPCamImageProfile ipcam_pf[GUI_CHANNEL_CNT];

static const gchar *strRange[101] ={
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
	"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
	"21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
	"31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
	"41", "42", "43", "44", "45", "46", "47", "48", "49", "50",

	"51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
	"61", "62", "63", "64", "65", "66", "67", "68", "69", "70",
	"71", "72", "73", "74", "75", "76", "77", "78", "79", "80",
	"81", "82", "83", "84", "85", "86", "87", "88", "89", "90",
	"91", "92", "93", "94", "95", "96", "97", "98", "99", "100"
};

static gboolean start_preview = FALSE;

static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch, 
        MENU_V_PAGE_X+MENU_V_INNER_X+IMG_THUMBNAIL_START_X, 
        MENU_V_PAGE_Y+MENU_V_INNER_Y+IMG_THUMBNAIL_START_Y, 
        408, 
        228);
    
    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static gint _show_video_obj()
{
    nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    return 0;
}

static gint _hide_video_obj()
{
    nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_signal_emit(video_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static void _check_cam_value()
{
	gint i, ch;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_SHARPNESS)
		{
			if (advdata[ch].sharpness != ipcam_pf[ch].sharpness.value)
			{
				g_message("%s, %d, <ch:%d, cam:%d, db:%d>", __FUNCTION__, __LINE__, ch, ipcam_pf[ch].sharpness.value, advdata[ch].sharpness);
				advdata[ch].sharpness = ipcam_pf[ch].sharpness.value;
			}
		}

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_MIRRORING)
		{		
			for (i = 0; i < ipcam_pf[ch].mirror_cnt; i++)
			{
				if (ipcam_pf[ch].mirror[i].selected)
				{			
					if (advdata[ch].rotate!= ipcam_pf[ch].mirror[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].mirror[i].value, advdata[ch].rotate);
						advdata[ch].rotate = ipcam_pf[ch].mirror[i].value;
					}
				}	
			}
		}

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_EXPOSURE)
		{		
			for (i = 0; i < ipcam_pf[ch].exposure_cnt; i++)
			{
				if (ipcam_pf[ch].exposure[i].selected)
				{			
					if (advdata[ch].exposure_mode != ipcam_pf[ch].exposure[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].exposure[i].value, advdata[ch].exposure_mode);
						advdata[ch].exposure_mode = ipcam_pf[ch].exposure[i].value;
					}
				}	
			}
		}

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_AGC)
		{
			if (advdata[ch].agc_gain != ipcam_pf[ch].agc.value)
			{
				g_message("%s, %d, <ch:%d, cam:%d, db:%d>", __FUNCTION__, __LINE__, ch, ipcam_pf[ch].agc.value, advdata[ch].agc_gain);			
				advdata[ch].agc_gain = ipcam_pf[ch].agc.value;
			}
		}

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_ESHUTTER)
		{
			if (advdata[ch].shutter_speed != ipcam_pf[ch].eshutter.value)
			{
				g_message("%s, %d, <ch:%d, cam:%d, db:%d>", __FUNCTION__, __LINE__, ch, ipcam_pf[ch].eshutter.value, advdata[ch].shutter_speed);
				advdata[ch].shutter_speed = ipcam_pf[ch].eshutter.value;
			}
		}
		

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_SLOWSHUTTER)
		{		
			for (i = 0; i < ipcam_pf[ch].slowshutter_cnt; i++)
			{
				if (ipcam_pf[ch].slowshutter[i].selected)
				{
					if (advdata[ch].slow_shutter != ipcam_pf[ch].slowshutter[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].slowshutter[i].value, advdata[ch].slow_shutter);
						advdata[ch].slow_shutter = ipcam_pf[ch].slowshutter[i].value;
					}
				}	
			}
		}

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_MAXAGC)
		{		
			for (i = 0; i < ipcam_pf[ch].maxagc_cnt; i++)
			{
				if (ipcam_pf[ch].maxagc[i].selected)
				{
					if (advdata[ch].max_agc != ipcam_pf[ch].maxagc[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].maxagc[i].value, advdata[ch].max_agc);					
						advdata[ch].max_agc = ipcam_pf[ch].maxagc[i].value;
					}
				}	
			}
		}		
			
		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_DCIRIS)
		{		
			for (i = 0; i < ipcam_pf[ch].dc_iris_cnt; i++)
			{
				if (ipcam_pf[ch].dc_iris[i].selected)
				{
					if (advdata[ch].iris_control != ipcam_pf[ch].dc_iris[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].maxagc[i].value, advdata[ch].max_agc);										
						advdata[ch].iris_control = ipcam_pf[ch].dc_iris[i].value;
					}
				}	
			}
		}	
			
		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_DNN)
		{		
			for (i = 0; i < ipcam_pf[ch].dnn_cnt; i++)
			{
				if (ipcam_pf[ch].dnn[i].selected)
				{
					if (advdata[ch].day_night_mode != ipcam_pf[ch].dnn[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].dnn[i].value, advdata[ch].day_night_mode);															
						advdata[ch].day_night_mode = ipcam_pf[ch].dnn[i].value;
					}
				}	
			}
		}	

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_DNN_TOGGLE)
		{		
			for (i = 0; i < ipcam_pf[ch].dnn_toggle_cnt; i++)
			{
				if (ipcam_pf[ch].dnn_toggle[i].selected)
				{
					if (advdata[ch].day_night_duration != ipcam_pf[ch].dnn_toggle[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].dnn_toggle[i].value, advdata[ch].day_night_duration);															
						advdata[ch].day_night_duration = ipcam_pf[ch].dnn_toggle[i].value;
					}
				}	
			}
		}	

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_WB)
		{		
			for (i = 0; i < ipcam_pf[ch].wb_cnt; i++)
			{
				if (ipcam_pf[ch].wb[i].selected)
				{
					if (advdata[ch].wb_mode!= ipcam_pf[ch].wb[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].wb[i].value, advdata[ch].wb_mode);															
						advdata[ch].wb_mode = ipcam_pf[ch].wb[i].value;
					}
				}	
			}
		}	

		if (ipcam_pf[ch].supported & NF_IPCAM_IMAGE_MWB)
		{		
			for (i = 0; i < ipcam_pf[ch].mwb_cnt; i++)
			{
				if (ipcam_pf[ch].mwb[i].selected)
				{
					if (advdata[ch].mwb_mode != ipcam_pf[ch].mwb[i].value)
					{
						g_message("%s, %d, <ch:%d, index:%d, cam:%08X, db:%08X>", __FUNCTION__, __LINE__, ch, i, ipcam_pf[ch].mwb[i].value, advdata[ch].mwb_mode);															
						advdata[ch].mwb_mode = ipcam_pf[ch].mwb[i].value;
					}
				}	
			}
		}	
	}

	if (memcmp(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT))
	{
		g_memmove(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT);
		DAL_set_advanced_data_all(advdata, GUI_CHANNEL_CNT);
		syscam_set_changeflag(1);
	}	
}

static gboolean vw_cam_check_supported_func(gint ch, guint check_bit)
{
	if (ipcam_pf[ch].supported & check_bit)
		return TRUE;
	else 
		return FALSE;		
}

static void _check_supported_func_spin(gint ch)
{
	gint i;

	for (i = 0; i < NF_IPCAM_IMAGE_NR; i++)
	{
		if (!vw_cam_check_supported_func(ch, 1 << i))
		{
			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_disable(g_attr_bright_spin[ch]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_disable(g_attr_cont_spin[ch]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_disable(g_attr_tint_spin[ch]);
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_disable(g_attr_color_spin[ch]);			
		}
	}
}

static void _check_supported_func_slider(gint ch)
{
	gint i;

	for (i = 0; i < NF_IPCAM_IMAGE_NR; i++)
	{
		if (vw_cam_check_supported_func(ch, 1 << i))
		{
			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_enable(g_attr_slider[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_enable(g_attr_slider[ATTR_CONTRAST]);		
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_enable(g_attr_slider[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_enable(g_attr_slider[ATTR_COLOR]);			

			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_enable(g_attr_label[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_enable(g_attr_label[ATTR_CONTRAST]);		
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_enable(g_attr_label[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_enable(g_attr_label[ATTR_COLOR]);			
		}
		else
		{
			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_disable(g_attr_slider[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_disable(g_attr_slider[ATTR_CONTRAST]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_disable(g_attr_slider[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_disable(g_attr_slider[ATTR_COLOR]);

			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_disable(g_attr_label[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_disable(g_attr_label[ATTR_CONTRAST]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_disable(g_attr_label[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_disable(g_attr_label[ATTR_COLOR]);
		}
	}
}

static void image_setup_call(gint ch)
{
	NFIPCamSetupColor info;
	
	memset(&info, 0x00, sizeof(info));
	
	info.ch = ch;
	info.brightness = coldata[ch].bright;
	info.contrast = coldata[ch].contrast;
	info.tint = coldata[ch].tint;
	info.color = coldata[ch].color;
	
	scm_set_ipcam_image(&info);
}

static void image_setup_cll_allCh()
{
	int i;

	for(i = 0; i < GUI_CHANNEL_CNT; i++)
		image_setup_call(i);
}

static void _send_command_ipcam_advanced()
{
	NFIPCamSetupImage info;
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		memset(&info, 0x00, sizeof(NFIPCamSetupImage));
	
		info.ch = i;

		info.brightness = coldata[i].bright;
		info.contrast = coldata[i].contrast;
		info.tint = coldata[i].tint;
		info.color = coldata[i].color;

		info.sharpness = advdata[i].sharpness;
		info.exposure = advdata[i].exposure_mode;
		info.agc = advdata[i].agc_gain;
		info.eshutter_speed = advdata[i].shutter_speed;
		info.max_agc = advdata[i].max_agc;
		info.iris = advdata[i].iris_control;
		info.blc = advdata[i].blc_control;
		info.day_night = advdata[i].day_night_mode;
		info.det_time = advdata[i].day_night_duration;
		info.white_balance = advdata[i].wb_mode;
		info.mwb = advdata[i].mwb_mode;

		scm_set_ipcam_rotation(i, advdata[i].rotate);
		scm_set_ipcam_antiflicker(i, advdata[i].antiflicker);		
		scm_set_ipcam_image_advanced(&info);
	}
}

static gint _set_attr_slider_obj_value(gint ch, gint attr_type, ColorData *data, gint expose)
{
	gint current_ch = nfui_combobox_get_cur_index(channel_obj);

	if (current_ch != ch) return 0;

	if (attr_type == ATTR_BRIGHT) 
		cw_slider_set_value(g_attr_slider[ATTR_BRIGHT], data->bright);
		
	if (attr_type == ATTR_CONTRAST) 
		cw_slider_set_value(g_attr_slider[ATTR_CONTRAST], data->contrast);
		
	if (attr_type == ATTR_TINT) 
		cw_slider_set_value(g_attr_slider[ATTR_TINT], data->tint);
	
	if (attr_type == ATTR_COLOR) 
		cw_slider_set_value(g_attr_slider[ATTR_COLOR], data->color);

	if (expose)
	{
		if (attr_type == ATTR_BRIGHT) 
			nfui_signal_emit(g_attr_slider[ATTR_BRIGHT], GDK_EXPOSE, TRUE);
			
		if (attr_type == ATTR_CONTRAST) 
			nfui_signal_emit(g_attr_slider[ATTR_CONTRAST], GDK_EXPOSE, TRUE);			
			
		if (attr_type == ATTR_TINT) 
			nfui_signal_emit(g_attr_slider[ATTR_TINT], GDK_EXPOSE, TRUE);
			
		if (attr_type == ATTR_COLOR) 
			nfui_signal_emit(g_attr_slider[ATTR_COLOR], GDK_EXPOSE, TRUE);
	}

	return 1;
}

static gint _set_attr_label_obj_value(gint ch, gint attr_type, ColorData *data, gint expose)
{
	gchar tmp_val[10];
	gint current_ch = nfui_combobox_get_cur_index(channel_obj);

	if (current_ch != ch) return 0;

	if (attr_type == ATTR_BRIGHT) 
	{
		memset(tmp_val, 0x00, sizeof(tmp_val));
		g_sprintf(tmp_val, "%d", data->bright);
		nfui_nflabel_set_text(g_attr_label[ATTR_BRIGHT], tmp_val);
	}

	if (attr_type == ATTR_CONTRAST) 
	{	
		memset(tmp_val, 0x00, sizeof(tmp_val));
		g_sprintf(tmp_val, "%d", data->contrast);				
		nfui_nflabel_set_text(g_attr_label[ATTR_CONTRAST], tmp_val);
	}
	
	if (attr_type == ATTR_TINT) 
	{
		memset(tmp_val, 0x00, sizeof(tmp_val));
		g_sprintf(tmp_val, "%d", data->tint);				
		nfui_nflabel_set_text(g_attr_label[ATTR_TINT], tmp_val);
	}
	
	if (attr_type == ATTR_COLOR) 
	{
		memset(tmp_val, 0x00, sizeof(tmp_val));
		g_sprintf(tmp_val, "%d", data->color);				
		nfui_nflabel_set_text(g_attr_label[ATTR_COLOR], tmp_val);
	}
	
	if (expose)
	{
		if (attr_type == ATTR_BRIGHT) 	
			nfui_signal_emit(g_attr_label[ATTR_BRIGHT], GDK_EXPOSE, TRUE);

		if (attr_type == ATTR_CONTRAST) 	
			nfui_signal_emit(g_attr_label[ATTR_CONTRAST], GDK_EXPOSE, TRUE);			

		if (attr_type == ATTR_TINT) 	
			nfui_signal_emit(g_attr_label[ATTR_TINT], GDK_EXPOSE, TRUE);

		if (attr_type == ATTR_COLOR) 	
			nfui_signal_emit(g_attr_label[ATTR_COLOR], GDK_EXPOSE, TRUE);
	}

	return 1;
}

static gint _set_attr_spin_obj_value(gint ch, gint attr_type, ColorData *data, gint expose)
{
	if (attr_type == ATTR_BRIGHT) 
		nfui_spin_button_set_index_no_expose(g_attr_bright_spin[ch], data->bright);
	
	if (attr_type == ATTR_CONTRAST) 
		nfui_spin_button_set_index_no_expose(g_attr_cont_spin[ch], data->contrast);
	
	if (attr_type == ATTR_TINT) 
		nfui_spin_button_set_index_no_expose(g_attr_tint_spin[ch], data->tint);
	
	if (attr_type == ATTR_COLOR) 
		nfui_spin_button_set_index_no_expose(g_attr_color_spin[ch], data->color);

	if (expose)
	{
		if (attr_type == ATTR_BRIGHT) 
			nfui_signal_emit(g_attr_bright_spin[ch], GDK_EXPOSE, TRUE);
		
		if (attr_type == ATTR_CONTRAST) 
			nfui_signal_emit(g_attr_cont_spin[ch], GDK_EXPOSE, TRUE);		
		
		if (attr_type == ATTR_TINT) 
			nfui_signal_emit(g_attr_tint_spin[ch], GDK_EXPOSE, TRUE);
		
		if (attr_type == ATTR_COLOR) 
			nfui_signal_emit(g_attr_color_spin[ch], GDK_EXPOSE, TRUE);
	}	
}

static void prvSetDataToObjects_colorsetup(gint expose)
{
	guint i, j;
	gchar tmp_val[4][10];
	
	for(i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		for (j = 0; j < ATTR_TYPE; j++)
		{
			_set_attr_slider_obj_value(i, j, &coldata[i], expose);
			_set_attr_label_obj_value(i, j, &coldata[i], expose);	
			_set_attr_spin_obj_value(i, j, &coldata[i], expose);
		}
	}
}

static gboolean mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{	
    switch(evt->type)
    {
		case GDK_EXPOSE :
		break;

        case INFY_CAMDB_CHANGE_NOTIFY:
        case INFY_USRDB_CHANGE_NOTIFY:
        {
        	gint i;
        	gchar strBuf[STRING_SIZE_CAMTITLE];
	        gchar strBuf_num[STRING_SIZE_CAMTITLE+8];

        	nfui_combobox_remove_all(NF_COMBOBOX(channel_obj));

        	for (i = 0; i < GUI_CHANNEL_CNT; ++i)
        	{
        		var_get_camtitle(strBuf, i);
        		nfui_nfimglabel_set_text((NFIMGLABEL*)g_ch_imglabel_obj[i], strBuf);
        		nfui_signal_emit(g_ch_imglabel_obj[i], GDK_EXPOSE, TRUE);
        		
        		g_sprintf(strBuf_num, "%d. %s", i+1, strBuf);
          		nfui_combobox_append_data(NF_COMBOBOX(channel_obj), strBuf_num);       		
        	}

    		nfui_signal_emit(channel_obj, GDK_EXPOSE, TRUE);        	
		}
		break;

		case GDK_DELETE:
		{			
            uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);			
            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);			
		}
		break;
			
		default :
			break;
	}

	return FALSE;

}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case NFEVENT_COMBOBOX_CHANGED :
		{
			gint i;
			gint ch = nfui_combobox_get_cur_index(channel_obj);

			for (i = 0; i < ATTR_TYPE; i++) 
			{
				_set_attr_slider_obj_value(ch, i, &coldata[ch], OBJ_EXPOSE);
				_set_attr_label_obj_value(ch, i, &coldata[ch], OBJ_EXPOSE);	
				_set_attr_spin_obj_value(ch, i, &coldata[ch], OBJ_EXPOSE);
			}
			
			_check_supported_func_slider(ch);
			_start_preview(ch);
		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS: 
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_DOWN)
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);

				return TRUE;
			}
		}
		break;

		case GDK_DELETE: 
		break;

		default: 
		break;
		
	}
	return FALSE;
}

static gboolean post_image_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		break;

		case GDK_BUTTON_PRESS :
		break;
						
		case GDK_LEAVE_NOTIFY :
		case GDK_BUTTON_RELEASE :
		{
			gint i;	
			gint ch = nfui_combobox_get_cur_index(channel_obj);			
            ColorData data;

			data.bright = cw_slider_get_value(g_attr_slider[ATTR_BRIGHT]);
			data.contrast = cw_slider_get_value(g_attr_slider[ATTR_CONTRAST]);
			data.tint = cw_slider_get_value(g_attr_slider[ATTR_TINT]);
			data.color = cw_slider_get_value(g_attr_slider[ATTR_COLOR]);

            if(memcmp(&coldata[ch], &data, sizeof(ColorData)))
            {
    			for(i = 0; i < ATTR_TYPE; i++) 
    			{
    				if(g_attr_slider[i] == obj) break;
    			}

    			switch(i)
    			{
    				case ATTR_BRIGHT: 
    					coldata[ch].bright = cw_slider_get_value(obj);
    				break;
    	
    				case ATTR_CONTRAST: 
    					coldata[ch].contrast = cw_slider_get_value(obj);
    				break;

    				case ATTR_TINT: 
    					coldata[ch].tint = cw_slider_get_value(obj);
    				break;

    				case ATTR_COLOR: 
    					coldata[ch].color = cw_slider_get_value(obj);
    				break;

    				default: 
    				break;
    			}

    			image_setup_call(ch);
            }
		}
		break;
	
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :
		{
			gint i;
			gint ch = nfui_combobox_get_cur_index(channel_obj);			
            ColorData data;
			
			for(i = 0; i < ATTR_TYPE; i++) 
			{
				if (g_attr_slider[i] == obj) break;
			}

			data.bright = cw_slider_get_value(g_attr_slider[ATTR_BRIGHT]);
			data.contrast = cw_slider_get_value(g_attr_slider[ATTR_CONTRAST]);
			data.tint = cw_slider_get_value(g_attr_slider[ATTR_TINT]);
			data.color = cw_slider_get_value(g_attr_slider[ATTR_COLOR]);

			_set_attr_label_obj_value(ch, i, &data, OBJ_EXPOSE);
			_set_attr_spin_obj_value(ch, i, &data, OBJ_EXPOSE);
		}
		break;

		default : 
		break;
	}
	return FALSE;
}

static gboolean post_default_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;
		gint ch = nfui_combobox_get_cur_index(channel_obj);

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		coldata[ch].bright = 50;
		coldata[ch].contrast = 50;
		coldata[ch].tint = 50;
		coldata[ch].color = 50;		

		for (i = 0; i < ATTR_TYPE; i++)
		{
			_set_attr_slider_obj_value(ch, i, &coldata[ch], OBJ_EXPOSE);
			_set_attr_label_obj_value(ch, i, &coldata[ch], OBJ_EXPOSE);
			_set_attr_spin_obj_value(ch, i, &coldata[ch], OBJ_EXPOSE);	
		}

		image_setup_call(ch);

	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			nfui_set_key_focus(obj, FALSE);
			nfui_set_key_focus(channel_obj, TRUE);

			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(channel_obj, GDK_EXPOSE, TRUE);

			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean post_bright_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		gint i;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (g_attr_bright_spin[i] == obj) break;
		}

		coldata[i].bright = nfui_spin_button_get_index(obj);

		_set_attr_slider_obj_value(i, ATTR_BRIGHT, &coldata[i], OBJ_EXPOSE);
		_set_attr_label_obj_value(i, ATTR_BRIGHT, &coldata[i], OBJ_EXPOSE);

		image_setup_call(i);	
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN)
		{
			if(obj == g_attr_bright_spin[GUI_CHANNEL_CNT - 1])
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_LEFT)
		{
			if(GUI_CHANNEL_CNT < 5)
				return FALSE;

			for(i=0; i<GUI_CHANNEL_CNT; i++)
			{
				if(i > 5)
				{
					if(obj == g_attr_bright_spin[i]) 
					{
						nfui_set_key_focus(obj, FALSE);
						nfui_set_key_focus(advanced_obj[i], TRUE);

						nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
						nfui_signal_emit(advanced_obj[i], GDK_EXPOSE, TRUE);
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_contrast_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		gint i;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (g_attr_cont_spin[i] == obj) break;
		}

		coldata[i].contrast = nfui_spin_button_get_index(obj);

		_set_attr_slider_obj_value(i, ATTR_CONTRAST, &coldata[i], OBJ_EXPOSE);
		_set_attr_label_obj_value(i, ATTR_CONTRAST, &coldata[i], OBJ_EXPOSE);

		image_setup_call(i);	
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN)
		{
			if(obj == g_attr_cont_spin[GUI_CHANNEL_CNT - 1])
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_tint_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		gint i;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (g_attr_tint_spin[i] == obj) break;
		}

		coldata[i].tint = nfui_spin_button_get_index(obj);

		_set_attr_slider_obj_value(i, ATTR_TINT, &coldata[i], OBJ_EXPOSE);
		_set_attr_label_obj_value(i, ATTR_TINT, &coldata[i], OBJ_EXPOSE);

		image_setup_call(i);		
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN)
		{
			if(obj == g_attr_tint_spin[GUI_CHANNEL_CNT - 1])
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_color_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{		
		gint i;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (g_attr_color_spin[i] == obj) break;
		}
		
		coldata[i].color = nfui_spin_button_get_index(obj);

		_set_attr_slider_obj_value(i, ATTR_COLOR, &coldata[i], OBJ_EXPOSE);
		_set_attr_label_obj_value(i, ATTR_COLOR, &coldata[i], OBJ_EXPOSE);

		image_setup_call(i);		
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN)
		{
			if(obj == g_attr_color_spin[GUI_CHANNEL_CNT - 1])
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_preview_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i, j;
    	gint ch = nfui_combobox_get_cur_index(channel_obj);

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        _hide_video_obj();
    	_stop_preview();
		
		for(i = 0; i < GUI_CHANNEL_CNT; i++) {
			if(preview_obj[i] == obj) break;
		}

		vw_open_image_setup_preview(g_curwnd, i, coldata, ipcam_pf);

		for (j = 0; j < ATTR_TYPE; j++)
		{
			_set_attr_slider_obj_value(i, j, &coldata[i], OBJ_NO_EXPOSE);
			_set_attr_label_obj_value(i, j, &coldata[i], OBJ_NO_EXPOSE);	
			_set_attr_spin_obj_value(i, j, &coldata[i], OBJ_NO_EXPOSE);
		}

        _start_preview(ch);
        _show_video_obj();        
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
		
		if(kpid == KEYPAD_DOWN)
		{
			if(obj == preview_obj[GUI_CHANNEL_CNT - 1])
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
	}

	return FALSE; 
}


static gboolean post_advanced_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
	gint i, j;
	gint ch = nfui_combobox_get_cur_index(channel_obj);
	mb_type ret = -1;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (g_reset_msg_check == 0)
		{
			ret = vw_mbox_check(g_curwnd, "NOTICE", IMBX_WARNING_IPCAM_RESET, IMBX_NOT_SHOW_AGAIN, &g_reset_msg_check, NFTOOL_MB_OKCANCEL);
			if (ret == NFTOOL_MB_CANCEL) return FALSE;
		}

        _hide_video_obj();
    	_stop_preview();

		for(i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if(advanced_obj[i] == obj) break;
		}

		vw_open_image_setup_advanced(g_curwnd, i, &coldata[i], &advdata[i], &ipcam_pf[i]);

		for (j = 0; j < ATTR_TYPE; j++)
		{
			_set_attr_slider_obj_value(i, j, &coldata[i], OBJ_NO_EXPOSE);
			_set_attr_label_obj_value(i, j, &coldata[i], OBJ_NO_EXPOSE);	
			_set_attr_spin_obj_value(i, j, &coldata[i], OBJ_NO_EXPOSE);
		}

        _start_preview(ch);
        _show_video_obj();

	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_RIGHT)
		{
			if(GUI_CHANNEL_CNT < 5)
				return FALSE;

			for(i=0; i<GUI_CHANNEL_CNT; i++)
			{
				if(obj == advanced_obj[i])
				{
					if(i > 5)
					{
						if(obj == advanced_obj[i])
						{
							nfui_set_key_focus(obj, FALSE);
							nfui_set_key_focus(g_attr_bright_spin[i], TRUE);

							nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
							nfui_signal_emit(g_attr_bright_spin[i], GDK_EXPOSE, TRUE);
							return TRUE;
						}
					}
				}
			}
		}
		else if(kpid == KEYPAD_DOWN)
		{
			if(obj == advanced_obj[GUI_CHANNEL_CNT - 1])
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(def_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(def_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
	}

	return FALSE; 
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

    		g_memmove(coldata, org_coldata, sizeof(ColorData)*GUI_CHANNEL_CNT);		
    		prvSetDataToObjects_colorsetup(OBJ_EXPOSE);

    		g_memmove(advdata, org_advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT);
		_send_command_ipcam_advanced();
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i, changed = 0;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (g_reset_msg_check != DAL_get_cam_reset_msg_check())
		{
			DAL_set_cam_reset_msg_check(g_reset_msg_check);
			syscam_set_changeflag(1);
		}

		if(memcmp(org_coldata, coldata, sizeof(ColorData)*GUI_CHANNEL_CNT))
		{
			g_memmove(org_coldata, coldata, sizeof(ColorData)*GUI_CHANNEL_CNT);
			DAL_set_color_data_all(coldata, GUI_CHANNEL_CNT);
			changed = 1;

			for(i = 0; i < GUI_CHANNEL_CNT; i++)
				image_setup_call(i);
		}

		if (memcmp(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT))
		{
			g_memmove(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT);
			DAL_set_advanced_data_all(advdata, GUI_CHANNEL_CNT);
			changed = 1;
		}
		
		if (changed)
		{
			scm_put_log(CHANGE_CAM_IMAGE, 0, 0);
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");			
			syscam_set_changeflag(1);
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
	
    	_stop_preview();
		ColorSetup_tab_out_handler();
		SystemSetupCam_Destroy(obj);
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

void init_ColorSetup_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *ntb;
	NFOBJECT *title_object[COLOR_SETUP_COLUMNS];
	NFOBJECT *colorsetup_btns[CSB_BUTTONS];
	NFOBJECT *pgdn_btn, *pgup_btn;
	NFOBJECT *lbTemp;
	NFOBJECT *obj;

	GdkPixbuf *pbCamImage[32];
	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
	const gchar *strControl[] = {"BRIGHTNESS", "CONTRAST", "TINT", "COLOR"};
	const gchar *strTitle[COLOR_SETUP_COLUMNS] = {"CHANNEL", "BRIGHTNESS", "CONTRAST", "TINT", "COLOR"};

	gchar strBuf[STRING_SIZE_CAMTITLE];
	gchar strBuf_num[STRING_SIZE_CAMTITLE+8];
	gchar val_tmp[10];
	
	gint xpos, ypos;
	gint control_val_tmp[4];

	guint width[COLOR_SETUP_COLUMNS];
	guint i, tmp_supp_func;

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	width[0] = 250;
	width[1] = 140;
	width[2] = 130;
	width[3] = 100;
	width[4] = 100;



// CAMERA IMAGE LOAD
	pbCamImage[0]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
	pbCamImage[1]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
	pbCamImage[2]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
	pbCamImage[3]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL); 
	pbCamImage[4]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL); 		
	pbCamImage[5]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
	pbCamImage[6]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
	pbCamImage[7]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL); 
	pbCamImage[8]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
	pbCamImage[9]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL); 	
	pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
	pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
	pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
	pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
	pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL); 		
	pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL); 
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL); 
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL);         
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL); 
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL); 
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL);     
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL); 
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL); 
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL);             
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL);    

	memset(coldata, 0x00, sizeof(ColorData)*GUI_CHANNEL_CNT);
	memset(org_coldata, 0x00, sizeof(ColorData)*GUI_CHANNEL_CNT);

	memset(advdata, 0x00, sizeof(AdvancedData)*GUI_CHANNEL_CNT);
	memset(org_advdata, 0x00, sizeof(AdvancedData)*GUI_CHANNEL_CNT);

	memset(ipcam_pf, 0x00, sizeof(NFIPCamImageProfile)*GUI_CHANNEL_CNT);

	for(i=0; i<GUI_CHANNEL_CNT; i++)
		DAL_get_color_data(&coldata[i], i);

	for(i=0; i<GUI_CHANNEL_CNT; i++)
		DAL_get_advanced_data(&advdata[i], i);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		nf_ipcam_get_image_profile(i, &ipcam_pf[i]);

	g_memmove(org_coldata, coldata, sizeof(ColorData)*GUI_CHANNEL_CNT);
	g_memmove(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT);

	_check_cam_value();

	g_reset_msg_check = DAL_get_cam_reset_msg_check();
	
	video_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nflabel_set_align((NFLABEL*)video_obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(video_obj, 408, 228);
	nfui_nfobject_use_focus(video_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(video_obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, video_obj, IMG_THUMBNAIL_START_X, IMG_THUMBNAIL_START_Y);

// CHANNEL COMBO
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 112, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, CHANNEL_LABEL_X, CHANNEL_LABEL_Y);

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_nfobject_set_size(obj, CHANNEL_COMBO_W, 40); 
	nfui_nfobject_show(obj); 
	nfui_regi_post_event_callback(obj, post_channel_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, CHANNEL_COMBO_X, CHANNEL_COMBO_Y);
	channel_obj = obj;

	for (i = 0; i < GUI_CHANNEL_CNT; ++i)
	{
		//DAL_get_camera_title(strBuf, i);
		var_get_camtitle(strBuf, i);
		g_sprintf(strBuf_num, "%d. %s", i+1, strBuf);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf_num);
	}


// CONTROL
	xpos = BRIGHTNESS_LABEL_X;
	ypos = BRIGHTNESS_LABEL_Y;

	for(i = 0 ; i < 4 ; i++)
	{
		//TITLE
		if(nftool_cur_language_is_japanese())
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strControl[i], nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(121));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strControl[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(121));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_set_size(obj, BRIGHTNESS_LABEL_W, COLOR_SETUP_LABEL_HEIGHT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos);

		ypos += (COLOR_SETUP_LABEL_HEIGHT + 2);
		
	}

	// SLIDER
	xpos = BRIGHTNESS_SLIDER_X;
	ypos = BRIGHTNESS_SLIDER_Y;
	
	g_attr_slider[0] = cw_slider_new(coldata[0].bright, BRIGHTNESS_SLIDER_W, COLOR_SETUP_LABEL_HEIGHT);
	g_attr_slider[1] = cw_slider_new(coldata[0].contrast, BRIGHTNESS_SLIDER_W, COLOR_SETUP_LABEL_HEIGHT);
	g_attr_slider[2] = cw_slider_new(coldata[0].tint, BRIGHTNESS_SLIDER_W, COLOR_SETUP_LABEL_HEIGHT);
	g_attr_slider[3] = cw_slider_new(coldata[0].color, BRIGHTNESS_SLIDER_W, COLOR_SETUP_LABEL_HEIGHT);

	for(i = 0; i < 4; i++)
	{
		cw_slider_set_range(g_attr_slider[i], 0, 100, 101); 
		nfui_nfobject_set_size(g_attr_slider[i], BRIGHTNESS_SLIDER_W, COLOR_SETUP_LABEL_HEIGHT);
		nfui_nfobject_modify_bg(g_attr_slider[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));		
		nfui_nfobject_show(g_attr_slider[i]);
		nfui_regi_post_event_callback(g_attr_slider[i], post_image_slider_event_handler);
		nfui_nffixed_put((NFFIXED*)content_fixed, g_attr_slider[i], xpos, ypos);

		ypos += (COLOR_SETUP_LABEL_HEIGHT + 2);
	}

// CONTROL
	xpos = BRIGHTNESS_CONTROL_X;
	ypos = BRIGHTNESS_CONTROL_Y;

	control_val_tmp[0] = coldata[0].bright;
	control_val_tmp[1] = coldata[0].contrast;
	control_val_tmp[2] = coldata[0].tint;
	control_val_tmp[3] = coldata[0].color;

	for(i = 0; i < 4; i++)
	{
		g_sprintf(val_tmp, "%d", control_val_tmp[i]);
		
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(val_tmp, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, BRIGHTNESS_CONTROL_W, COLOR_SETUP_LABEL_HEIGHT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos);
		g_attr_label[i] = obj;
		
		ypos += (COLOR_SETUP_LABEL_HEIGHT + 2);
	}

	// DEFAULT BUTTON
	obj = nftool_normal_button_create_type3("DEFAULT", DEF_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_default_btn_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, DEF_BTN_X, DEF_BTN_Y);
	def_btn = obj;
	

	ntb = (NFOBJECT*)nfui_nftable_new(COLOR_SETUP_COLUMNS, GUI_CHANNEL_CNT+1, COLOR_SETUP_COL_SPACE, COLOR_SETUP_ROW_SPACE, width, COLOR_SETUP_LABEL_HEIGHT);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)content_fixed, ntb, COLOR_SETUP_TABLE_LEFT, COLOR_SETUP_TABLE_TOP);

	for(i = 0; i < COLOR_SETUP_COLUMNS; i++)
	{
		if(nftool_cur_language_is_japanese())
			title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(116));
		else
			title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));		
		nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(title_object[i]);
		nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
	}

	for(i = 0; i < GUI_CHANNEL_CNT; i++)
	{
 		//DAL_get_camera_title(strBuf, i);
 		var_get_camtitle(strBuf, i);

		obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i+1);
		g_ch_imglabel_obj[i] = obj;
	}

	for( i=0 ; i<GUI_CHANNEL_CNT ; i++ )
	{
		g_attr_bright_spin[i] 	= nfui_spinbutton_new(strRange, 101, coldata[i].bright);
		g_attr_cont_spin[i] 	= nfui_spinbutton_new(strRange, 101, coldata[i].contrast);
		g_attr_tint_spin[i] 	= nfui_spinbutton_new(strRange, 101, coldata[i].tint);
		g_attr_color_spin[i] 	= nfui_spinbutton_new(strRange, 101, coldata[i].color);

		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_attr_bright_spin[i], NFSPINBUTTON_TYPE_1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_attr_cont_spin[i], NFSPINBUTTON_TYPE_1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_attr_tint_spin[i], NFSPINBUTTON_TYPE_1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_attr_color_spin[i], NFSPINBUTTON_TYPE_1);

		nfui_nfobject_show(g_attr_bright_spin[i]);
		nfui_nfobject_show(g_attr_cont_spin[i]);
		nfui_nfobject_show(g_attr_tint_spin[i]);
		nfui_nfobject_show(g_attr_color_spin[i]);

		nfui_nftable_attach((NFTABLE*)ntb, g_attr_bright_spin[i], 1, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, g_attr_cont_spin[i], 2, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, g_attr_tint_spin[i], 3, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, g_attr_color_spin[i], 4, i+1);

		nfui_regi_post_event_callback(g_attr_bright_spin[i], post_bright_spin_event_handler);
		nfui_regi_post_event_callback(g_attr_cont_spin[i], post_contrast_spin_event_handler);
		nfui_regi_post_event_callback(g_attr_tint_spin[i], post_tint_spin_event_handler);
		nfui_regi_post_event_callback(g_attr_color_spin[i], post_color_spin_event_handler);
		
	}
	
	// PREVIEW BUTTONS
	xpos = (COLOR_SETUP_TABLE_LEFT + width[0] + width[1] + width[2] + width[3] + width[4] + COLOR_SETUP_COL_SPACE*4);
	ypos = (COLOR_SETUP_TABLE_TOP + COLOR_SETUP_LABEL_HEIGHT + COLOR_SETUP_ROW_SPACE);
	
	for(i = 0 ; i < GUI_CHANNEL_CNT ; i++)
	{
		obj = nftool_normal_button_create_type3("PREVIEW", 142);
		preview_obj[i] = obj;
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_preview_event_handler);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos);

		obj = nftool_normal_button_create_type3("ADVANCED", 160);
		advanced_obj[i] = obj;
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_advanced_event_handler);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos + 144, ypos);

		ypos += (COLOR_SETUP_LABEL_HEIGHT + COLOR_SETUP_ROW_SPACE);
	}

	colorsetup_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(colorsetup_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(colorsetup_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, colorsetup_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	colorsetup_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(colorsetup_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(colorsetup_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, colorsetup_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	colorsetup_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(colorsetup_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(colorsetup_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, colorsetup_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
	nfui_regi_post_event_callback(colorsetup_btns[0], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(colorsetup_btns[1], post_applybutton_event_handler);
	nfui_regi_post_event_callback(colorsetup_btns[2], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		_check_supported_func_spin(i);
		
	_check_supported_func_slider(0);

	uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);	
	uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);		
	uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);	
	uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
}

static gboolean _init_preview(gpointer data)
{
    _start_preview(0);
//    _show_video_obj();  
	return FALSE;
}

gint ColorSetup_start_preview()
{
    _start_preview(0);
    _show_video_obj();
//	g_timeout_add(2000, _init_preview, 0);    
	return 0;	
}

gboolean ColorSetup_tab_in_handler()
{
	gint ch = nfui_combobox_get_cur_index(channel_obj);

    _start_preview(ch);
    _show_video_obj();

	return FALSE;
}

gboolean ColorSetup_tab_out_handler()
{
	gint index = nfui_combobox_get_cur_index(channel_obj);
	mb_type ret;
	guint i;

    _hide_video_obj();
	_stop_preview();

	if (g_reset_msg_check != DAL_get_cam_reset_msg_check())
	{
		DAL_set_cam_reset_msg_check(g_reset_msg_check);
		syscam_set_changeflag(1);
	}

	if ((!memcmp(org_coldata, coldata, sizeof(ColorData)*GUI_CHANNEL_CNT)) &&
		(!memcmp(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		scm_put_log(CHANGE_CAM_IMAGE, 0, 0);

		g_memmove(org_coldata, coldata, sizeof(ColorData)*GUI_CHANNEL_CNT);
		g_memmove(org_advdata, advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT);
		
		DAL_set_color_data_all(coldata, GUI_CHANNEL_CNT);
		DAL_set_advanced_data_all(advdata, GUI_CHANNEL_CNT);
		
		syscam_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(coldata, org_coldata, sizeof(ColorData)*GUI_CHANNEL_CNT);
		g_memmove(advdata, org_advdata, sizeof(AdvancedData)*GUI_CHANNEL_CNT);

		prvSetDataToObjects_colorsetup(OBJ_NO_EXPOSE);
		_send_command_ipcam_advanced();		
	}

	return FALSE;
}


