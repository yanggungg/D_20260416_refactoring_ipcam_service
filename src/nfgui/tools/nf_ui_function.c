
#include <glib.h>


#include "nf_afx.h"
#include "nfdal.h"
#include "iux_afx.h"

#include "nf_ui_function.h"


/* for power off */
#include "libsst.h" 
#include "nf_api_disk.h"
#include "nf_record.h"
#include "nf_network.h"
#include "nf_util_device.h"
#include "nf_sysman.h"
/* for spot */
#include "nf_api_live.h"

/* for auto logout */
#include "../support/event_loop.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/util.h"
#include "../support/nf_ui_color.h"

#include "../tools/nf_ui_tool.h"
#include "vw_search_main.h"


// for alarm popup
//#include "../nf_ui_livedisplay.h"
//#include "../nf_ui_zoom.h"


// for ddns hostname
#include "nf_util_netif.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"UI_FUNC"



#if defined(_MULTI_POPUP_)
typedef enum
{
  EVENT_DET_NONE = 0,
  EVENT_DET_ALARM = 1,
  EVENT_DET_MOTION = 2,
  EVENT_DET_MAX,
}event_det_type;

static guint event_dwell_cnt[GUI_CHANNEL_CNT];
static event_popup_enable_type event_popup_enable = EVENT_POPUP_DISABLE;
static event_det_type event_det_ch[GUI_CHANNEL_CNT];
static gboolean pre_event_popup_ch[GUI_CHANNEL_CNT];
static gboolean popup_timer_on = 0;

#define ALARM_LED       40  /* status */
#endif

#if 0
GdkColor colors[END_NF_COLOR_CODE] = {
	// common color values
	{0, 0xff00, 0xff00, 0xff00},
	{0, 0x3900, 0x4200, 0x4a00},
	{0, 0xff00, 0xff00, 0x0000},
	{0, 0x0000, 0x0000, 0xff00},
	{0, 0x0000, 0xff00, 0x0000},
	{0, 0xff00, 0x0000, 0x0000},

	{0, 0x2400, 0x2600, 0x2c00},
	{0, 0x4600, 0x4c00, 0x5300},
	{0, 0x3500, 0x3e00, 0x4a00},
	{0, 0x7c00, 0x9000, 0xb500},
	{0, 0xb100, 0x9500, 0xc600},
	{0, 0x6f00, 0xa200, 0xff00},
	{0, 0xff00, 0xcc00, 0x0000},
	{0, 0xff00, 0x9d00, 0x0000},
	{0, 0x7600, 0x9400, 0xd600},
	{0, 0xfd00, 0x6e00, 0x6e00},
	{0, 0xff00, 0xcc00, 0x7000},
	{0, 0x9f00, 0xf300, 0xee00},
	{0, 0xf600, 0x0f00, 0xff00},
	{0, 0x8400, 0x8400, 0x8400},
	{0, 0x7E00, 0x9F00, 0xBE00},
	{0, 0x3C00, 0x6100, 0x8200},
	{0, 0x2200, 0x4300, 0x6500},
	{0, 0x8F00, 0xC400, 0xDC00},
	{0, 0x4A00, 0x5200, 0x5B00},
	{0, 0xc400, 0xdf00, 0x9b00},
	{0, 0xa000, 0x4100, 0x4100},
	{0, 0xe600, 0xfa00, 0xff00},
	{0, 0x8000, 0xd000, 0xe000},
	{0, 0x5000, 0x5000, 0x5000},
	{0, 0x1000, 0x1000, 0x1000},
	{0, 0xf800, 0x0000, 0x0000},
	{0, 0xff00, 0xff00, 0x0000},
	{0, 0xff00, 0x0000, 0x0000},
	{0, 0x0000, 0xff00, 0x0000},
	{0, 0x0000, 0x0000, 0xff00},
	{0, 0xff00, 0xff00, 0xff00},
// NOVUS UI COLOR	
	{0, 0x8400, 0x7b00, 0x7300},
	{0, 0xbd00, 0x8400, 0x3100},
	{0, 0xa500, 0x9c00, 0x8c00},
	{0, 0x6b00, 0x5a00, 0x5200},
	{0, 0x9400, 0x5a00, 0x0800},
	{0, 0x4a00, 0x4200, 0x4200},
	{0, 0x3900, 0x3800, 0x3600},
	{0, 0x2d00, 0x2d00, 0x2d00},
	{0, 0x2000, 0x2000, 0x2000},
	{0, 0x3100, 0x3000, 0x2e00},
	{0, 0x5200, 0x4f00, 0x4800},
	{0, 0xc100, 0x8700, 0x3500},
	{0, 0xa400, 0x9a00, 0x8f00},
	{0, 0x1d00, 0x0e00, 0x0000},
	{0, 0x2c00, 0x1a00, 0x0000},
	{0, 0x3b00, 0x3b00, 0x3b00},
	{0, 0x1e00, 0x1c00, 0x1900},
	{0, 0x6400, 0x6400, 0x6400},
	{0, 0x0c00, 0x0c00, 0x0c00},
	{0, 0x9100, 0xf800, 0x0000},
	{0, 0xe200, 0xcf00, 0x1700},
	{0, 0x4a00, 0x7000, 0x6000},
	{0, 0x7500, 0x9b00, 0x0000},

	{0, 0x4000, 0xdf00, 0x9b00},
	{0, 0xB500, 0xCE00, 0xEF00}, 
	{0, 0x7300, 0xA500, 0xDE00},
	{0, 0x4200, 0x8400, 0xCE00},
	{0, 0x7B00, 0xCE00, 0xFF00},
	{0, 0x1000, 0x3100, 0x6B00},
	{0, 0x1000, 0x7300, 0xCE00},
	{0, 0x0800, 0x4A00, 0x9400},
	{0, 0x0000, 0x0800, 0x1900},
	{0, 0x2100, 0x4200, 0x7300},
	{0, 0xC500, 0xDE00, 0xFF00},
	{0, 0x8400, 0xA500, 0xCE00},
	{0, 0x2100, 0x3A00, 0x5200},
	{0, 0x1900, 0x2900, 0x3A00},
	{0, 0x4A00, 0x8400, 0xBD00},
	{0, 0x0000, 0xC500, 0xFF00},
	{0, 0x1000, 0x1900, 0x2900},
	{0, 0xB500, 0xDE00, 0xFF00},
	{0, 0x0000, 0xA500, 0xFF00},
	{0,	0xB200, 0xDF00, 0xFF00},
	{0,	0x7B00, 0x9F00, 0xCA00},
	{0,	0xEF00, 0x6300, 0x2100},
	{0, 0x8400, 0xA500, 0xC500},
	{0,	0xD600, 0xE600, 0xF700},
	{0,	0x6B00, 0xA500, 0xFF00},	
	{0,	0xC500, 0xF700, 0xFF00},
	{0,	0xF700, 0x0000, 0x0000},
	{0,	0x8400, 0xCE00, 0xDE00},	
	{0,	0x0000, 0x1000, 0x4200},	
	{0,	0x2100, 0x2100, 0x2100},
	{0,	0x9C00, 0xB500, 0x9400},
	{0,	0x2100, 0x2900, 0x2900},
	{0,	0x2100, 0x4200, 0x6300},	
	{0,	0x3A00, 0x4200, 0x3A00},	
	{0,	0x3900, 0x6300, 0x8400},	
	{0,	0x6300, 0xB500, 0x3100},	
	{0,	0x7B00, 0x9400, 0xAD00},	
	{0,	0x3100, 0x4A00, 0x2900},	
	{0, 0x6b00, 0x7d00, 0x9f00},
	{0, 0x2100, 0x2900, 0x3a00},
	{0, 0x0000, 0x0000, 0x0800},
	{0, 0xB200, 0xD100, 0xF100},
	{0, 0x1300, 0x7800, 0xDD00},
	{0, 0x2400, 0x3F00, 0x7700},
	{0, 0xC400, 0xE200, 0xFF00},
	{0, 0x8100, 0xA600, 0xD100},
	{0, 0x4B00, 0x8000, 0xBD00},
	{0, 0x2300, 0x2C00, 0x3700},
	{0, 0x7500, 0xA700, 0xE100},
	{0, 0x7D00, 0xD600, 0xFF00},
	{0, 0x2C00, 0xA600, 0xDF00},
	{0, 0x0400, 0x3400, 0x6100},	
	{0, 0x0000, 0x0900, 0x1C00},
	{0, 0x2000, 0x3800, 0x5400},
	{0, 0x1800, 0x2A00, 0x3C00},
	{0, 0x1100, 0x1A00, 0x2D00},
	{0, 0x0000, 0xA800, 0xFF00},
	{0, 0x1300, 0x2E00, 0x6900},
	{0, 0x4400, 0x8700, 0xCB00},
	{0, 0x2600, 0x5400, 0x8400},
	{0, 0x0000, 0xC600, 0xFF00},
	{0, 0xD400, 0xE600, 0xF600},
	{0, 0x0000, 0xDE00, 0xFF00},
	{0, 0xF800, 0x6C00, 0x6800},
	{0, 0x9800, 0xF000, 0xE800},
	{0, 0x7000, 0x9400, 0xD000},
	{0, 0xF800, 0x9C00, 0x0000},
	{0, 0x6C00, 0xFD00, 0x3A00},
	{0, 0xD600, 0xE700, 0xFD00},
	{0, 0x0000, 0xCD00, 0xFF00},
	{0, 0x0400, 0x9700, 0x9200},
	{0, 0x5200, 0x7100, 0x7C00},
	{0, 0x0B00, 0x4D00, 0x9200},
	{0, 0xF800, 0xFC00, 0xF800},
	{0, 0x4200, 0x9100, 0xDF00},
	{0, 0x0000, 0xA700, 0xFF00},
	{0, 0x0C00, 0x1900, 0x4100},
	{0, 0x3500, 0x4200, 0x5500},
	{0, 0x5300, 0x5700, 0x5F00},
	{0, 0x4200, 0x7800, 0xB700},
	{0, 0xDC00, 0xEE00, 0xFF00},
	{0, 0x5800, 0x5B00, 0x6200},
	{0, 0x2E00, 0x4B00, 0x6D00},
	{0, 0x3000, 0x3500, 0x3F00},
	{0, 0x2400, 0x3700, 0x5100},
	
	{0, 0xEF00, 0xF400, 0xFA00},
	{0, 0xFF00, 0xB600, 0x3800},
	{0, 0x1100, 0x1900, 0x3100},
	{0, 0xC000, 0xD100, 0xE200},	
	{0, 0xBB00, 0xDF00, 0xB700},
	{0, 0xFF00, 0x4D00, 0x5800},
	{0, 0xFF00, 0x9C00, 0x3900},
	{0, 0xFD00, 0xFF00, 0x5500},
	{0, 0x0B00, 0x1300, 0x1F00},
	{0, 0x3A00, 0x2100, 0x0000},
	{0, 0x2600, 0x3100, 0x4000},
	{0, 0xB400, 0xCB00, 0xE200},
	{0, 0xff00, 0xff00, 0xa000},
	{0, 0xa800, 0xb600, 0xc400},
	{0, 0x2700, 0x3b00, 0x5700},
	{0, 0x4200, 0x5500, 0x7000},
	
	{0, 0x1000, 0x6200, 0x9600},
	{0, 0x9C00, 0xC600, 0xFF00},
	{0, 0x4B00, 0x5500, 0x6300},
	{0, 0xFF00, 0x8000, 0x2300},
	{0, 0x3B00, 0x4400, 0x5100},
	{0, 0xFF00, 0x4E00, 0x0000},
	{0, 0x7900, 0x7F00, 0x8800},
	{0, 0x5500, 0x5A00, 0x6300},
	{0, 0x1600, 0x2200, 0x2B00},
	{0, 0x1d00, 0x2400, 0x2f00},
	{0, 0x0700, 0x4600, 0x7400},

	{0, 0xB800, 0xE200, 0xFF00},
	// skshin
	//{0, 0x0000, 0xFF00, 0xFF00},
	{0, 0x3000, 0xCF00, 0xFF00},
	{0, 0xFF00, 0x1800, 0x3900},
	{0, 0x1800, 0xb000, 0xFF00},
	{0, 0x2B00, 0x2B00, 0x2B00},
	{0, 0x4000, 0x5500, 0x7100},
	{0, 0x5a00, 0x7000, 0x8f00},
	{0, 0x2C00, 0x5A00, 0x2700},
	{0, 0x0400, 0x1000, 0x1E00},
	{0, 0xFF00, 0xEA00, 0xA000},
	{0, 0x0000, 0xBE00, 0xFE00},
	{0, 0xFF00, 0x9B00, 0x1A00},
	{0, 0xFF00, 0x3C00, 0x0000},
	{0, 0x8000, 0x8000, 0x8000},
	{0, 0x9200, 0x9200, 0x9200},
	{0, 0x2700, 0x3100, 0x3e00},
	{0, 0x0100, 0x0100, 0x0100},
	{0, 0xFF00, 0x9600, 0x0000},
	
/*	{0, 0x7000, 0xA500, 0x7300},
	{0, 0xE000, 0x7900, 0x7900},
	{0, 0xB800, 0xE400, 0xFF00},
	{0, 0xFF00, 0xFF00, 0xC000},
	{0, 0xEA00, 0x7100, 0xD500},*/
	{0, 0x1a00, 0x3d00, 0x1600},
	{0, 0x8400, 0x0000, 0x0000},
	{0, 0x0000, 0x6900, 0x8d00},
	{0, 0x0000, 0x8500, 0x0000},
	{0, 0x7d00, 0x7e00, 0x0000},

	{0, 0x0000, 0x0000, 0x0000},
};



GdkColor rtile_colors[END_NF_RTILE_COLOR_MAX] = {
	// common color values
	{0, 0x0000, 0x0000, 0x0000},    //etc color
	{0, 0x0000, 0x0000, 0x0000},    //etc color

       {0, 0xff00, 0xf500, 0x9b00},    //FFF59B, color5
	{0, 0xF500, 0x6700, 0xFF00},    //F567FF, color22
	{0, 0x8d00, 0xcb00, 0xff00},    //8DCBFF, color13
	{0, 0x3700, 0xc600, 0x1f00},    //37C61F, color7

	{0, 0x0000, 0x0000, 0x0000},    //etc color
	{0, 0x0000, 0x0000, 0x0000},    //etc color
	{0, 0x0000, 0x0000, 0x0000},    //etc color
	{0, 0x0000, 0x0000, 0x0000},    //etc color
	{0, 0x0000, 0x0000, 0x0000},    //etc color
	{0, 0x0000, 0x0000, 0x0000},    //etc color

	{0, 0x6600, 0x4f00, 0xb400},    //664FB4, color18
	{0, 0xA700, 0x5C00, 0x7800},    //A75C78, color23
	{0, 0xff00, 0x5a00, 0x0900},    //FF5A09, color2
	{0, 0x1f00, 0x7300, 0x1100},    //1F7311, color8

	{0, 0x0000, 0x7e00, 0xff00},    //007EFF, color14
	{0, 0x4600, 0x1c00, 0xe300},    //461CE3, color19
	{0, 0xff00, 0x9500, 0x1800},    //FF9518, color3
	{0, 0x0f00, 0x4d00, 0x0c00},    //0F4D0C, color9

	{0, 0x0000, 0x2d00, 0xff00},    //002DFF, color15
	{0, 0x5E00, 0x0000, 0x9E00},    //5E009E, color20
	{0, 0xE000, 0x0000, 0xBE00},    //E000BE, color21
	{0, 0xff00, 0xe700, 0x1500},    //FFE715, color4

	{0, 0x1a00, 0x6f00, 0x7c00},    //1A6F7C, color10
	{0, 0x1900, 0x2a00, 0x6700},    //192A67, color16
	{0, 0x0000, 0xea00, 0xe500},    //00EAE5, color12
	{0, 0x8c00, 0x2400, 0x2200},    //8C2422, color0

	{0, 0x2f00, 0xae00, 0xc200},    //2FAEC2, color11
	{0, 0x9f00, 0xf000, 0x0c00},    //9FF00C, color6 
	{0, 0x3f00, 0x4900, 0x9600},    //3F4996, color17
	{0, 0xfe00, 0x1800, 0x0900},    //FE1809, color1			
	
};
#endif










/*************************************************************
 * ALARM / MOTION POPUP
 * **********************************************************/

static NF_DISPLAY_E g_prev_disp_mode = NF_DISPLAY_HEXADECA;
static gint g_prev_div_idx = 0;
static guint g_pop_id = 0;
static gulong alm_cb = 0;
static gulong mot_cb = 0;

static guint prev_ch = 100;

static MonitorData g_mon_data;

enum {
	POPUP_MODE_ALARM = 0,
	POPUP_MODE_MOTION,
};


#if 0
#if defined(_MULTI_POPUP_)
#if defined(_ATM_0424)||defined(_ATM_0412)
void change_event_display_popup_mode(guint view_ch_cnt, gint start_ch_idx, gint end_ch_idx, gboolean event_ch_change, gboolean dura_end)
{
#if 0
    guint display_mode = _prev_disp_mode;

    if(view_ch_cnt > GUI_CHANNEL_CNT)
    {
      return;
    }
    
    if(view_ch_cnt > DIV4_MAX_CH_CNT && view_ch_cnt <= DIV9_MAX_CH_CNT)
    {
        display_mode = NF_DISP_DEFAULT_MODE;
        start_ch_idx = 0;  		  
        event_popup_enable = EVENT_POPUP_ENABLE;
    }
    else if(view_ch_cnt > DIV1_MAX_CH_CNT && view_ch_cnt <= DIV4_MAX_CH_CNT)
    {
        if(end_ch_idx < start_ch_idx+DIV4_MAX_CH_CNT)
        {	      
            display_mode = NF_DISPLAY_QUAD;
            start_ch_idx = 0;
        }
        else
        {
            display_mode = NF_DISP_DEFAULT_MODE;
            start_ch_idx = 0;  		  
        }
        event_popup_enable = EVENT_POPUP_ENABLE;
    }
    else if(view_ch_cnt == DIV1_MAX_CH_CNT)
    {
        display_mode = NF_DISPLAY_FULL;
        event_popup_enable = EVENT_POPUP_ENABLE;  		  
    }
    else
    {
        if(dura_end == TRUE)
        {
            event_popup_end(TRUE);      
        }
        return;
    }

    if((_prev_disp_mode != display_mode) || (event_ch_change==TRUE))
    {
        nfdisp_change_disp_mode_by_NFDISP_E(display_mode, start_ch_idx);
    }
#endif
}

#elif defined(_ATM_0824) || defined(_OTM_0824)
void change_event_display_popup_mode(guint view_ch_cnt, gint start_ch_idx, gint end_ch_idx, gboolean event_ch_change, gboolean dura_end)
{
      guint display_mode = _prev_disp_mode;

      if(view_ch_cnt > GUI_CHANNEL_CNT)
      {
            return;
      }
  
      if(view_ch_cnt > DIV4_MAX_CH_CNT && view_ch_cnt <= DIV9_MAX_CH_CNT)
      {
            display_mode = NF_DISP_DEFAULT_MODE;
            start_ch_idx = 0;  		  
            event_popup_enable = EVENT_POPUP_ENABLE;
      }
      else if(view_ch_cnt > DIV1_MAX_CH_CNT && view_ch_cnt <= DIV4_MAX_CH_CNT)
      {
            if(end_ch_idx < start_ch_idx+DIV4_MAX_CH_CNT)
            {	      
                display_mode = NF_DISPLAY_QUAD;
            }
            else if(end_ch_idx < start_ch_idx+DIV9_MAX_CH_CNT)
            {
                display_mode = NF_DISPLAY_NONA;
                start_ch_idx = 0;
            }
            else
            {
                display_mode = NF_DISP_DEFAULT_MODE;
                start_ch_idx = 0;  		  
            }
            event_popup_enable = EVENT_POPUP_ENABLE;
      }
      else if(view_ch_cnt == DIV1_MAX_CH_CNT)
      {
            display_mode = NF_DISPLAY_FULL;
            event_popup_enable = EVENT_POPUP_ENABLE;  		  
      }
      else
      {
            if(dura_end == TRUE)
            {
              event_popup_end(TRUE);      
            }
            return;
      }

      DMSG(1, "display_mode : %d, start_ch_idx :%d\n", display_mode, start_ch_idx);
  
	if((_prev_disp_mode != display_mode) || (event_ch_change==TRUE))
	{
		nfdisp_change_disp_mode_by_NFDISP_E(display_mode, start_ch_idx);
	}
}
#else
void change_event_display_popup_mode(guint view_ch_cnt, gint start_ch_idx, gint end_ch_idx, gboolean event_ch_change, gboolean dura_end)
{
      guint display_mode = _prev_disp_mode;

      if(view_ch_cnt > DIV9_MAX_CH_CNT && view_ch_cnt <= DIV16_MAX_CH_CNT)
      {
    	    display_mode = NF_DISPLAY_HEXADECA;
    	    start_ch_idx = 0;
    	    event_popup_enable = EVENT_POPUP_ENABLE;
      }
      else if(view_ch_cnt > DIV4_MAX_CH_CNT && view_ch_cnt <= DIV9_MAX_CH_CNT)
      {
            if(end_ch_idx < start_ch_idx+DIV9_MAX_CH_CNT)
            {
    		  display_mode = NF_DISPLAY_NONA;        
            }
            else
            {
    		  display_mode = NF_DISP_DEFAULT_MODE;
    		  start_ch_idx = 0;  		  
            }
            event_popup_enable = EVENT_POPUP_ENABLE;
      }
      else if(view_ch_cnt > DIV1_MAX_CH_CNT && view_ch_cnt <= DIV4_MAX_CH_CNT)
      {
            if(end_ch_idx < start_ch_idx+DIV4_MAX_CH_CNT)
            {	      
              display_mode = NF_DISPLAY_QUAD;
            }
            else if(end_ch_idx < start_ch_idx+DIV9_MAX_CH_CNT)
            {
              display_mode = NF_DISPLAY_NONA;
            }
            else
            {
    		  display_mode = NF_DISP_DEFAULT_MODE;
    		  start_ch_idx = 0;  		  
            }
            event_popup_enable = EVENT_POPUP_ENABLE;
      }
      else if(view_ch_cnt == DIV1_MAX_CH_CNT)
      {
            display_mode = NF_DISPLAY_FULL;
            event_popup_enable = EVENT_POPUP_ENABLE;  		  
      }
      else
      {
            if(dura_end == TRUE)
            {
              event_popup_end(TRUE);      
            }
            return;
      }

      DMSG(1, "<display_mode:%d, start_ch_idx:%d, event_ch_change:%d>", display_mode, start_ch_idx, event_ch_change);
      
	if((_prev_disp_mode != display_mode) || (event_ch_change==TRUE))
	{
		nfdisp_change_disp_mode_by_NFDISP_E(display_mode, start_ch_idx);
	}
}
#endif
#endif

void change_event_display_popup_mode(guint view_ch_cnt, gint start_ch_idx, gint end_ch_idx, gboolean event_ch_change, gboolean dura_end)
{

}

static void nffunc_alarm_led_onoff(NF_NOTIFY_INFO *pinfo)
{
      guint i = 0;
      gboolean led_state = FALSE;
  
       for(i=0; i<ALARM_IN_CH_MAX; i++)
      {
          if(pinfo->d.params[0] & (1<<i))
          {
              led_state = TRUE;
          }
       }

       DMSG(1, "led_state : %d\n", led_state);

      if(led_state == TRUE)
      {
         nffunc_led_on(ALARM_LED);
      }
      else
      {
         nffunc_led_off(ALARM_LED);  
      }
}

static gboolean
popup_timer_action(gpointer data)
{	
	PAGEID pgid;
	NFOBJECT *page;

	pgid = nfui_get_cur_page(&page);
	if(pgid == PGID_CAM_COLOR_BAR || pgid == PGID_PLAYBACK || pgid == PGID_PLAYBACK_CONTROL ||
			pgid == PGID_PANORAMA || pgid == PGID_DIGIZOOM || pgid == PGID_MOTION_AREA || 
			pgid == PGID_PTZ || pgid == PGID_PTZ_PARAM || pgid == PGID_MOTION_AREA_MENU ||
			pgid == PGID_ARCHIVING || pgid == PGID_SEARCH || pgid == PGID_SEARCH_ARCH || 
			pgid == PGID_SETUP || pgid == PGID_SETUPMENU || pgid == PGID_MAINMENU || 
			pgid == PGID_MESSAGEBOX || pgid == PGID_SETUP_VKEY)
	{
	     popup_timer_on = 0;	
            return FALSE;
	}

	if(event_popup_enable == EVENT_POPUP_BLOCKING)
	{
	     popup_timer_on = 0;	
            return FALSE;
	}


	NFUTIL_THREADS_ENTER();

       guint type = (guint)data;
	guint view_ch_cnt = 0;
       guint i = 0;
       
	if((type == POPUP_MODE_ALARM) || (type == POPUP_MODE_MOTION))
	{
		/*6. 팝업 디스플레이 모드 변경을 위한 setting*/
		gint start_ch_idx = -1;
		gint end_ch_idx = -1;
		guint event_ch_change = FALSE;
		gboolean event_popup_ch[GUI_CHANNEL_CNT];

		memset(event_popup_ch, 0x00, sizeof(event_popup_ch));

		for(i=0; i<GUI_CHANNEL_CNT; i++)
		{

			if(event_det_ch[i] != EVENT_DET_NONE || event_dwell_cnt[i] != 0)
			{
				if(start_ch_idx == -1)
				{
					start_ch_idx = i;
				}

				end_ch_idx = i;
				++view_ch_cnt;
			}

			event_popup_ch[i] = get_event_popup_state(i);


			if(event_popup_ch[i]!=pre_event_popup_ch[i])
			{
				event_ch_change = TRUE;
			}      

			if (event_popup_enable == EVENT_POPUP_DISABLE)
			{
				event_ch_change = TRUE;
			}
		}

		/*7. 팝업 디스플레이 분할 모드 변경 */
		change_event_display_popup_mode(view_ch_cnt, start_ch_idx, end_ch_idx, event_ch_change, FALSE);        
		memcpy(pre_event_popup_ch, event_popup_ch, sizeof(event_popup_ch));
	}

	NFUTIL_THREADS_LEAVE();  

	popup_timer_on = 0;
      return FALSE;
}

gboolean get_popup_timer_on(void)
{
   return  popup_timer_on;
}

#if defined(_ATM_1648) || defined(_ATM_1624) || defined(_ATM_0824) ||  \
    defined(_ANF_0824CS) || defined(_ATM_0424) || defined(_ATM_0412) ||  \
    defined(_OTM_1648) || defined(_OTM_0824)
guint get_event_alarm_db_ch(guint ch)
{
	gchar alarm_db_ch_char = '0';
	guint alarm_db_ch = 0, alarm_ch = 0;
	RecordOperData recOpData;
	AlarmRecordParamData recParamData;
	time_t cur_time;
	struct tm time_info;


	time(&cur_time);    
	time_info = *NFLOCALTIME(&cur_time);

	DAL_get_recordOper_data(&recOpData);

	if(recOpData.schedMode == TRUE) //weekly
	{
		DAL_get_recordAlarmParam_data(&recParamData, time_info.tm_wday);
		alarm_db_ch_char = recParamData.alarmChParam[ch][time_info.tm_hour];
	}
	else  //daily
	{
		DAL_get_recordAlarmParam_data(&recParamData, REC_DAILY_PARAM);
		alarm_db_ch_char = recParamData.alarmChParam[ch][time_info.tm_hour];
	}


	switch(alarm_db_ch_char)
	{
		case '0':
			alarm_db_ch = 0;
		break;
		case '1':
			alarm_db_ch = 1;
		break;
		case '2':
			alarm_db_ch = 2;
		break;
		case '3':
			alarm_db_ch = 3;
		break;
		default:
			DMSG(1, "alarm_in_ch over 4");
			alarm_db_ch = 0;		
		break;
	}

	return alarm_db_ch;
}
#endif

static void popup_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
#if 0
	guint i;
	gint ch = -1;
	guint cnt = 0;
	guint dwell = 0;
	guint type;

	PAGEID pgid;
	NFOBJECT *page;

	CameraData camData;
	guint alarm_db_ch = 0, alarm_ch = 0;


	type = GPOINTER_TO_UINT(data);

	if(type == POPUP_MODE_ALARM)
	{
		nffunc_alarm_led_onoff(pinfo);
	}

	// Exception..
	// COLOR_SETUP_BAR, PLAYBACK, PTZCTRL1/2.. etc..
	pgid = nfui_get_cur_page(&page);
	if(pgid == PGID_CAM_COLOR_BAR || pgid == PGID_PLAYBACK || pgid == PGID_PLAYBACK_CONTROL ||
			pgid == PGID_PANORAMA || pgid == PGID_DIGIZOOM || pgid == PGID_MOTION_AREA || 
			pgid == PGID_PTZ || pgid == PGID_PTZ_PARAM || pgid == PGID_MOTION_AREA_MENU ||
			pgid == PGID_ARCHIVING || pgid == PGID_SEARCH || pgid == PGID_SEARCH_ARCH || 
			pgid == PGID_SETUP || pgid == PGID_SETUPMENU || pgid == PGID_MAINMENU || 
			pgid == PGID_MESSAGEBOX || pgid == PGID_SETUP_VKEY)
	{
		return;
	}

	if(event_popup_enable == EVENT_POPUP_BLOCKING)
	{
		return;
	}

	if(((type==POPUP_MODE_ALARM) && !(g_mon_data.alarmPopup)) || ((type==POPUP_MODE_MOTION) && !(g_mon_data.motPopup)))
		return;

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{	
		if((type==POPUP_MODE_ALARM) && (g_mon_data.alarmPopup))
		{
#if defined(_ATM_1648) || defined(_ATM_1624) || defined(_ATM_0824) ||  \
    defined(_ANF_0824CS) || defined(_ATM_0424) || defined(_ATM_0412) ||  \
    defined(_OTM_1648) || defined(_OTM_0824)
			alarm_db_ch = get_event_alarm_db_ch(i);
			
			for(alarm_ch = 0; alarm_ch < ALARM_IN_CH_MAX; alarm_ch++)
			{
				if(pinfo->d.params[0] & (1<<alarm_ch)) //alarm in occur ch
				{
					DAL_get_camera_data(&camData, i);

					if(!camData.covert)
					{   		
						if(alarm_db_ch == alarm_ch)
						{
							event_det_ch[i] = EVENT_DET_ALARM;
							event_dwell_cnt[i] = 0;
							break;
						}  
					}
				}      
				else {
					if (event_det_ch[i] == EVENT_DET_ALARM) {
						event_dwell_cnt[i] = g_mon_data.alarmDwell;
						event_det_ch[i] = EVENT_DET_NONE;
					}
				}
				
			}
#else
			if(pinfo->d.params[0] & (1<<i)) //alarm in occur ch
			{
				DAL_get_camera_data(&camData, i);

				if(!camData.covert)
				{   		
					event_det_ch[i] = EVENT_DET_ALARM;      		  
				}
			}	
			else 
			{
				if (event_det_ch[i] == EVENT_DET_ALARM) {
					event_dwell_cnt[i] = g_mon_data.alarmDwell;
					event_det_ch[i] = EVENT_DET_NONE;
				}
			}

#endif			
		}      
		else if((type==POPUP_MODE_MOTION) && (g_mon_data.motPopup))
		{
			/*1. 채널 마다의 motion event flag setting*/
			if(pinfo->d.params[0] & (1<<i))
			{
				DAL_get_camera_data(&camData, i);
				if(!camData.covert)
				{
					event_det_ch[i] = EVENT_DET_MOTION;      	
					event_dwell_cnt[i] = 0;
                    
				}
			}
			else
			{
				if (event_det_ch[i] == EVENT_DET_MOTION) {
					event_dwell_cnt[i] = g_mon_data.motDwell;
					event_det_ch[i] = EVENT_DET_NONE;
				}
			}
		}
		else
		{
			return;
		}
	}

	/*5. event popup 해제시 이전 display모드로 복귀하기 위한 setting*/
	if(event_popup_enable == EVENT_POPUP_DISABLE || event_popup_enable == EVENT_POPUP_BLOCKING)
	{
		if(_prev_disp_mode >= NF_DISPLAY_FULL && _prev_disp_mode <= NF_DISP_DEFAULT_MODE)
		{
			g_prev_disp_mode = _prev_disp_mode;
			g_prev_div_idx = _div_index;
		}
		else
		{
			g_prev_disp_mode = NF_DISP_DEFAULT_MODE;
			g_prev_div_idx = 0;
		}
	}

	if (popup_timer_on == 0) {
        g_timeout_add(300, popup_timer_action, type);
		popup_timer_on = 1;			
	}

#endif
}


guint get_event_popup_dwell_cnt(guint ch)
{
	return event_dwell_cnt[ch];
}


void set_event_popup_dwell_cnt(guint ch, guint dwell_cnt)
{
	event_dwell_cnt[ch] = dwell_cnt;
}


gboolean get_event_popup_state(guint ch)
{
	gboolean state = FALSE;

	if(event_dwell_cnt[ch] != 0 || event_det_ch[ch] != EVENT_DET_NONE)
	{
		state = TRUE;
	}
	else
	{
		state = FALSE;
	}

	return state;
}


void set_event_popup_enable(event_popup_enable_type enable)
{
	event_popup_enable = enable;
}


event_popup_enable_type get_event_popup_enable(void)
{
	return event_popup_enable;
}


void event_popup_end(gboolean pre_disp_state)
{
#if 0
	guint i = 0;

	event_popup_enable = EVENT_POPUP_DISABLE;


	live_win_alarm_popup_erase_text();

	if(pre_disp_state == TRUE)
	{	
		nfdisp_change_disp_mode_by_NFDISP_E(g_prev_disp_mode, g_prev_div_idx);
	}

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		event_dwell_cnt[i] = 0;
	}
#endif
}
#else
static void popup_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
#if 0
	guint i;
	gint ch = -1;
	guint cnt = 0;
	guint dwell = 0;
	guint type;

	PAGEID pgid;
	NFOBJECT *page;

	CameraData camData;

	if(g_pop_id)	return;
		
	// Exception..
	// COLOR_SETUP_BAR, PLAYBACK, PTZCTRL1/2.. etc..
	pgid = nfui_get_cur_page(&page);
	if(pgid == PGID_CAM_COLOR_BAR || pgid == PGID_PLAYBACK || pgid == PGID_PLAYBACK_CONTROL ||
		pgid == PGID_PANORAMA || pgid == PGID_DIGIZOOM || pgid == PGID_MOTION_AREA || 
		pgid == PGID_PTZ || pgid == PGID_PTZ_PARAM || pgid == PGID_MOTION_AREA_MENU)
		return;

	type = GPOINTER_TO_UINT(data);

	if(((type==POPUP_MODE_ALARM) && !(g_mon_data.alarmPopup)) || ((type==POPUP_MODE_MOTION) && !(g_mon_data.motPopup)))
		return;


	for(i=0; i<ALARM_IN_CH_MAX; i++)
	{
		if(pinfo->d.params[0] & (1<<i)) //alarm in occur ch
		{
			DAL_get_camera_data(&camData, i);
			if(!camData.covert)
				ch = i;
		}				
	}

	if(prev_ch != 100) 
	{
		if(type == POPUP_MODE_ALARM)		dwell = g_mon_data.alarmDwell * 1000;
		else if(type == POPUP_MODE_MOTION)	dwell = g_mon_data.motDwell * 1000;

		g_pop_id = g_timeout_add(dwell, popup_timeout, NULL);
	}
	else 
	{
		if(ch < 0) return;

		if(_prev_disp_mode >= NF_DISPLAY_FULL && _prev_disp_mode <= NF_DISPLAY_HEXADECA)
		{
			g_prev_disp_mode = _prev_disp_mode;
			g_prev_div_idx = _div_index;
		}
		else
		{
			g_prev_disp_mode = NF_DISPLAY_HEXADECA;
			g_prev_div_idx = 0;
		}

		NFUTIL_THREADS_ENTER();
		nfdisp_change_disp_mode_by_NFDISP_E(NF_DISPLAY_FULL, ch);
		NFUTIL_THREADS_LEAVE();

		prev_ch = ch;
	}

#endif
}

#endif

void nffunc_almmot_popup_init()
{
	nffunc_almmot_popup_term();

	DAL_get_monitor_data(&g_mon_data);

	prev_ch = 100;

#if defined(_MULTI_POPUP_)
	memset(event_dwell_cnt, 0x00, sizeof(event_dwell_cnt));
	memset(event_det_ch, 0x00, sizeof(event_det_ch));
	memset(pre_event_popup_ch, 0x00, sizeof(pre_event_popup_ch));

	guint alarm = nf_notify_get_param0("sensor");
	guint i = 0;
      
	for(i=0;i<ALARM_IN_CH_MAX;i++)
	{
		guint ch_mask = (1<<i);
		guint alarm_in = 0;
		alarm_in = (alarm & ch_mask) ? 1:0;
		
		if(alarm_in == 1)
		{
			nffunc_led_on(ALARM_LED);
		}
		break;
	}  
#endif  
	alm_cb = nf_notify_connect_cb("sensor", popup_notify_cb, GUINT_TO_POINTER(POPUP_MODE_ALARM));
	if(!alm_cb)	return;

	mot_cb = nf_notify_connect_cb("motion", popup_notify_cb, GUINT_TO_POINTER(POPUP_MODE_MOTION));
	if(!mot_cb)	return;

}

void nffunc_almmot_popup_ignore()
{	
	if(g_pop_id)
	{
		g_source_remove(g_pop_id);
		g_pop_id = 0;
	}

	prev_ch = 100;
}

void nffunc_almmot_popup_term()
{
#if 0
	if(g_pop_id)
	{
		nfdisp_change_disp_mode_by_NFDISP_E(g_prev_disp_mode, g_prev_div_idx);

		g_source_remove(g_pop_id);
		g_pop_id = 0;
	}

	if(alm_cb)	nf_notify_remove_cb("sensor", alm_cb);
	if(mot_cb)	nf_notify_remove_cb("motion", mot_cb);

	alm_cb = 0;
	mot_cb = 0;
#endif
}

/****************************************************************
 * LIVE AUDIO
 * **************************************************************/



gboolean nffunc_live_audio_init(guint on_off)
{
	AudioData audio_data;
	guint webra_status = 0;
	


	if(on_off == 0)
	{
		#if 0
			nf_dev_tw2864_set_dac(0xff);
		#else
			g_message("%s Not Implemented Yet!!", __FUNCTION__);
		#endif

		return FALSE;
	}

	webra_status = nf_network_get_webra_audio_status();
	
	if(webra_status == 1)
		return FALSE;

	DAL_get_audio_data(&audio_data);

#if 0
	if(audio_data.liveAudio)
		nf_dev_tw2864_set_dac(audio_data.channel);
	else
		nf_dev_tw2864_set_dac(0xff);
#else
	g_message("%s Not Implemented Yet!!", __FUNCTION__);
#endif

	return FALSE;
}




/****************************************************************
 * BEEP ON OFF
 * **************************************************************/

void nffunc_beep_on_off()
{
	BuzzerData buzzdata;
	
	DAL_get_buzzer_data(&buzzdata);
#if 0	
	if(buzzdata.keypad == 1)
	{
#if defined(USE_DEV_REMOCON)	
		nf_dev_remocon_beep_on();	
#endif
#if defined(USE_DEV_KEYPAD)	
		nf_dev_keypad_beep_on();
#endif		
	}
	else
	{
#if defined(USE_DEV_REMOCON)	
		nf_dev_remocon_beep_off();	
#endif
#if defined(USE_DEV_KEYPAD)	
		nf_dev_keypad_beep_off();
#endif		
	}
#endif

#if defined(USE_DEV_REMOCON)	
	if(buzzdata.remote == 1) nf_dev_remocon_beep_on();	
	else 					 nf_dev_remocon_beep_off();	
#endif

#if defined(USE_DEV_KEYPAD)	
	if(buzzdata.keypad == 1) nf_dev_keypad_beep_on();
	else 					 nf_dev_keypad_beep_off();
#endif		
}




/****************************************************************
 * LED ON/OFF
 * **************************************************************/

static guint g_led_mask[NUM_NF_PAGES][3] = {
	/* valid, high, low */
	{1, 0x00000000, 0xFC000000},	/* PGID_POWEROFF */
	{0, 0x00000000, 0x00000000},	/* PGID_SYS_INFO */
	{0, 0x00000000, 0x00000000},	/* PGID_SYS_FWUP */
	{1, 0x00000000, 0xFC000000},	/* PGID_LIVEDIVMENU */
	{1, 0x00000000, 0xFFFEFFFF},	/* PGID_LIVEDISPLAY */
	{1, 0x00000000, 0xFFFEFFFF},	/* PGID_LIVECTRLBAR */
	{1, 0x00000000, 0xFFFEFFFF},	/* PGID_LIVEWCTRLBAR */
	{1, 0x00000000, 0xFC000000},	/* PGID_LIVE_LOG */
	{1, 0x0000001F, 0xFC000000},	/* PGID_LIVE_PLAYBACK */
	{1, 0x00000000, 0xFC000000},	/* PGID_LIVE_PLAYBACK_TIME */
	{0, 0x00000000, 0x00000000},	/* PGID_LIVE_VKEY */
	{1, 0x0000001F, 0xFDC8FFFF},	/* PGID_PTZ */
	{1, 0x00000000, 0xFC800000},	/* PGID_PTZ_PARAM */
	{0, 0x00000000, 0x00000000},	/* PGID_PTZ_SETUP */
	{0, 0x00000000, 0x00000000},	/* PGID_PTZ_PROP */
#if !defined(_NO_PTZ_TOUR_)
	{1, 0x00000000, 0xFD000000},	/* PGID_PTZ_TOUR_CONF */
#endif	
	{0, 0x00000000, 0x00000000},	/* PGID_ARCH_BURN */
	{0, 0x00000000, 0x00000000},	/* PGID_ARCH_INFO */
	{1, 0x00000000, 0xFD000000},	/* PGID_RECORD */
	{1, 0x00000000, 0xFC400000},	/* PGID_ARCHIVING */
	{0, 0x00000000, 0x00000000},	/* PGID_REC_EVT_SCHED */
	{0, 0x00000000, 0x00000000},	/* PGID_REC_ACAM_PARAM */
	{1, 0x00000000, 0xFD000000},	/* PGID_SETUP */
	{1, 0x00000000, 0xFD000000},	/* PGID_SETUPMENU */
	{1, 0x00000000, 0xFC0001FF},	/* PGID_SETUP_VKEY */
	{0, 0x00000000, 0x00000000},	//{1, 0x00000000, 0xFC000000},	/* PGID_CAM_COLOR_BAR */
	{1, 0x00000000, 0xFD000000},	/* PGID_MAINMENU */
	{1, 0x00000000, 0xFC000000},	/* PGID_SNAPSHOT */
	{1, 0x00000000, 0xFE000000},	/* PGID_SEARCH */
	{1, 0x00000000, 0xFC000000},	/* PGID_SEARCH_ARCH */
	{1, 0x00000000, 0xFC100000},	/* PGID_DIGIZOOM */
	{0, 0x00000000, 0x00000000},	//{1, 0x00000000, 0xFC000000},	/* PGID_USER_ADD */
#if !defined(GUI_4CH_SUPPORT) && !defined(GUI_8CH_SUPPORT) && defined(__CBC_UI__)
	{1, 0x00000000, 0xDC0003FF},	/* PGID_USERPWD */
	{1, 0x00000000, 0xDC0003FF},	/* PGID_USERPWD_AUTOLOGOUT */
#else
	{1, 0x00000000, 0xDC0001FF},	/* PGID_USERPWD */
	{1, 0x00000000, 0xDC0001FF},	/* PGID_USERPWD_AUTOLOGOUT */
#endif
	{0, 0x00000000, 0x00000000},	/* PGID_MBOX */
	{0, 0x00000000, 0x00000000},	/* PGID_SEQ_CONF */
	{0, 0x00000000, 0x00000000},	/* PGID_SEQ_CONF_MENU */
	{0, 0x00000000, 0x00000000},	/* PGID_SEQ_SETUP */
	{0, 0x00000000, 0x00000000},	/* PGID_SPOT_CONF */
	{0, 0x00000000, 0x00000000},	/* PGID_SPOT_CONF_MENU */
	{0, 0x00000000, 0x00000000},	/* PGID_SPOT_SETUP */
	{1, 0x0000001F, 0xFC00FFFF},	/* PGID_PANORAMA */	
	{1, 0x0000001F, 0x6842FFFF},	/* PGID_PLAYBACK */
	{1, 0x0000001F, 0x6842FFFF},	/* PGID_PLAYBACK_CONTROL */
	{0, 0x00000000, 0x00000000},	/* PGID_MOTION_AREA */
	{0, 0x00000000, 0x00000000},	/* PGID_MOTION_AREA_MENU */
	{0, 0x00000000, 0x00000000},	/* PGID_IPCAM_PROP */
	{0, 0x00000000, 0x00000000},	/* PGID_POPUPWND */
	{1, 0x00000000, 0xFC000000},	/* PGID_MESSAGEBOX */
	{1, 0x00000000, 0x20000000},	/* PGID_RMCID_CONF */
	{0, 0x00000000, 0x00000000},	/* PGID_FW_INFO */		
};

gint nffunc_change_led(PAGEID pgid)
{
	gint i;

	if(!(g_led_mask[pgid][0]))
		return 0;

	nffunc_change_led_by_mask(g_led_mask[pgid][1], g_led_mask[pgid][2]);
	return 1;
}

gint nffunc_change_led_by_mask(guint hmask, guint lmask)
{
	gint i;

#if defined(USE_DEV_KEYPAD)	
	for(i=0; i<32; i++)
	{
		if(lmask & (1<<i))
			nf_dev_keypad_led_on(i);
		else
			nf_dev_keypad_led_off(i);
	}

	for(i=0; i<5; i++)
	{
		if(hmask & (1<<i))
			nf_dev_keypad_led_on(i+32);
		else
			nf_dev_keypad_led_off(i+32);
	}
#endif

	return 1;
}

void nffunc_led_on(guint idx)
{
#if defined(USE_DEV_KEYPAD)	
	nf_dev_keypad_led_on(idx);
#else
	DMSG(1, "## %08X LED ON", idx);
#endif
}

void nffunc_led_off(guint idx)
{
#if defined(USE_DEV_KEYPAD)	
	nf_dev_keypad_led_off(idx);
#else
	DMSG(1, "## %08X LED OFF", idx);
#endif
}

/* 변환과정에서 색감 손실이 약 5% 까지 발생 
   blue - y, green - u,  red - v 
*/

GdkColor
RGB_To_YUV(GdkColor rgb)    
{ 
	GdkColor color;
	GdkColor tmp_rgb=rgb;

	tmp_rgb.red = tmp_rgb.red >> 8; tmp_rgb.green = tmp_rgb.green >> 8; tmp_rgb.blue = tmp_rgb.blue >> 8;   
	tmp_rgb.red &= 0xff, tmp_rgb.green &= 0xff, tmp_rgb.blue &= 0xff;   

  if (tmp_rgb.red == 0 && tmp_rgb.green == 0 && tmp_rgb.blue == 0)
  {
    color.red = 0x8000;
    color.green = 0x8000;
    color.blue = 0;

    return color;
  }
#if 0	
	color.blue  = (int)(  ((19595 * tmp_rgb.red) >> 16) + ((38470 * tmp_rgb.green) >> 16) + ((7471  * tmp_rgb.blue) >> 16)       );    
	color.green = (int)( -((11059 * tmp_rgb.red) >> 16) - ((21709 * tmp_rgb.green) >> 16) + ((32768 * tmp_rgb.blue) >> 16) + 128 );    
	color.red = (int)(  ((32768 * tmp_rgb.red) >> 16) - ((27439 * tmp_rgb.green) >> 16) - ((5329  * tmp_rgb.blue) >> 16) + 128 );    
#else
	color.blue  = (int)(  ((16842 * tmp_rgb.red) >> 16) + ((33030 * tmp_rgb.green) >> 16) + ((6422  * tmp_rgb.blue) >> 16) +16      );    
	color.green = (int)( -((9699 * tmp_rgb.red) >> 16) - ((19071 * tmp_rgb.green) >> 16) + ((28770 * tmp_rgb.blue) >> 16) + 128 );    
	color.red = (int)(  ((28770 * tmp_rgb.red) >> 16) - ((24117 * tmp_rgb.green) >> 16) - ((4653  * tmp_rgb.blue) >> 16) + 128 );
#endif
#if 0
	color.blue  = (int)(  (color.blue  > 0) ? color.blue  : 0);  color.blue  = (int)((color.blue  < 255) ? color.blue  : 255        );    
	color.green = (int)(  (color.green > 0) ? color.green : 0);  color.green = (int)((color.green < 255) ? color.green : 255        );    
	color.red = (int)(  (color.red > 0) ? color.red : 0);  color.red = (int)((color.red < 255) ? color.red : 255        );    
#else
	color.blue  = (int)(  (color.blue  > 240) ? 240  : color.blue);  color.blue  = (int)((color.blue  < 16) ? 16  : color.blue        );    
	color.green = (int)(  (color.green > 240) ? 240 : color.green);  color.green = (int)((color.green < 16) ? 16 : color.green        );    
	color.red = (int)(  (color.red > 235) ? 235 : color.red);  color.red = (int)((color.red < 16) ? 16 : color.red        );    
#endif
	color.pixel = 0;

	color.red = color.red << 8; color.blue = color.blue << 8; color.green = color.green << 8;
	color.red &= 0xff00, color.green &= 0xff00, color.blue &= 0xff00;  

	return color;
}   

#if 0
void
nf_ui_Set_Color_Table()
{
	int i;

	for(i=0; i<=END_NF_COLOR_CODE; i++)	
		colors[i] = RGB_To_YUV(rgb_colors[i]);
		//colors[i] = nf_ui_get_yuv_color(rgb_colors[i]);		

       for(i=0; i<END_NF_RTILE_COLOR_MAX; i++)
              rtile_colors[i] = RGB_To_YUV(rtile_rgb_colors[i]);
}
#endif

void nffunc_ddns_hostname_init()
{
	DDNSData ddnsdata;

	NF_NETIF_GET_INFO ret_net_info;
	gchar mac_string[32];

	memset(&ret_net_info, 0x00, sizeof(ret_net_info));
	memset(mac_string, 0x00, sizeof(mac_string));

	nf_netif_get_info(&ret_net_info);   

	memset(&ddnsdata, 0x00, sizeof(ddnsdata));  

	g_sprintf(mac_string,"%02x%02x%02x%02x%02x%02x", 
			(guchar)ret_net_info.mac_addr[0],(guchar)ret_net_info.mac_addr[1], (guchar)ret_net_info.mac_addr[2], (guchar)ret_net_info.mac_addr[3], (guchar)ret_net_info.mac_addr[4], (guchar)ret_net_info.mac_addr[5]);

	DAL_get_ddns_data(&ddnsdata);


	if((strcmp(ddnsdata.host_name, "") == 0))
	{
		DMSG(1, "**********************host name DB setting :mac*********\n");
		strncpy(ddnsdata.host_name, mac_string, sizeof(mac_string));
		DAL_set_ddns_data(&ddnsdata);
	}
}

