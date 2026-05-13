#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "ix_mem.h"

#include "vw.h"
#include "vw_sys_camera_ipcam_internal.h"

#define ROW_H               (40)
#define ROW_GAP             (2)

IPCAM_SUBFIXED_T g_ipcam_subFixed[GUI_CHANNEL_CNT];

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

static gint _make_image_set(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;
    max_visible = _get_fixed_info_image_set(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
   
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_rotate(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;
    max_visible = _get_fixed_info_rotate(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_focus_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_focus_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_white_balance_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_wb_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_wide_dynamic_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_wdr_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_day_night_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_dn_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_colorvu_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_cv_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);

    return max_visible*(ROW_H+ROW_GAP);
}
static gint _make_exposure_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_dnr(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;
    max_visible = _get_fixed_info_exposure_dnr(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);


    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_blc_mode(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_blc_mode(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_antiflicker(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_antiflicker(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
    
    if(max_visible > 4) max_visible = 4;
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_hlc(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_hlc(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_defog(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_defog(ch, fixed_info);
    _make_objs_of_subFixed(fixed_info, LEFT_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_time(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;

    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_time(ch, fixed_info);  
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_gain(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;
   
    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_gain(ch, fixed_info);   
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}

static gint _make_exposure_iris(gint ch, FIXED_INFO_T *fixed_info, NFOBJECT* fixed)
{
    gint max_visible;
   
    memset(fixed_info, 0x00, sizeof(FIXED_INFO_T));
    fixed_info->fixed = fixed;    
    max_visible = _get_fixed_info_exposure_iris(ch, fixed_info);   
    _make_objs_of_subFixed(fixed_info, RIGHT_PAGE_FIXED_W);
    
    return max_visible*(ROW_H+ROW_GAP);
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

static gint _make_image_set_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;

    height = _make_image_set(ch, &g_ipcam_subFixed[ch].imageSet, subFixed);    

    _set_visible_subFixed(&g_ipcam_subFixed[ch].imageSet);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].imageSet);
    
    return height;
}

static gint _make_rotate_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;

    height = _make_rotate(ch, &g_ipcam_subFixed[ch].rotate, subFixed);    

    _set_visible_subFixed(&g_ipcam_subFixed[ch].rotate);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].rotate);
    
    return height;
}

static gint _make_focus_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;

    height = _make_focus_mode(ch, &g_ipcam_subFixed[ch].focusMode, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].focusMode);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].focusMode);
   
    return height;   
}

static gint _make_white_balance_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;
    
    height = _make_white_balance_mode(ch, &g_ipcam_subFixed[ch].wbMode, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].wbMode);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].wbMode);
    
    return height;   
}

static gint _make_day_night_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;
    
    height = _make_day_night_mode(ch, &g_ipcam_subFixed[ch].dnMode, subFixed);
   
    _set_visible_subFixed(&g_ipcam_subFixed[ch].dnMode);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].dnMode);
  
    return height;   
}

static gint _make_exposure_mode_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_mode(ch, &g_ipcam_subFixed[ch].exposureMode, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureMode);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureMode);
   
    return height;   
}

static gint _make_colorvu_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;
    
    height = _make_colorvu_mode(ch, &g_ipcam_subFixed[ch].dnMode, subFixed);
   
    _set_visible_subFixed(&g_ipcam_subFixed[ch].dnMode);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].dnMode);
  
    return height;   
}

static gint _make_exposure_blc_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_blc_mode(ch, &g_ipcam_subFixed[ch].exposureBlc, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureBlc);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureBlc);
   
    return height;   
}

static gint _make_exposure_hlc_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_hlc(ch, &g_ipcam_subFixed[ch].exposureHlc, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureHlc);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureHlc);
   
    return height;   
}

static gint _make_exposure_defog_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_defog(ch, &g_ipcam_subFixed[ch].exposureDefog, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureDefog);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureDefog);
   
    return height;   
}

static gint _make_exposure_dnr_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height =0;

    height = _make_exposure_dnr(ch, &g_ipcam_subFixed[ch].exposureDnr,subFixed);
     _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureDnr);
     _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureDnr);

    return height;
}


static gint _make_exposure_antiflicker_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_antiflicker(ch, &g_ipcam_subFixed[ch].exposureAnti, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureAnti);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureAnti);
   
    return height;   
}

static gint _make_exposure_time_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_time(ch, &g_ipcam_subFixed[ch].exposureTime, subFixed);
    
    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureTime);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureTime);
   
    return height;   
}

static gint _make_exposure_gain_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_gain(ch, &g_ipcam_subFixed[ch].exposureGain, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureGain);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureGain);
   
    return height;   
}

static gint _make_exposure_iris_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height = 0;
    
    height = _make_exposure_iris(ch, &g_ipcam_subFixed[ch].exposureIris, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureIris);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureIris);
   
    return height;   
}

static gint _make_wide_dynamic_subFixed(gint ch, NFOBJECT* subFixed)
{
    gint height;
    
    height = _make_wide_dynamic_mode(ch, &g_ipcam_subFixed[ch].exposureWdr, subFixed);

    _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureWdr);
    _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureWdr);
    
    return height;   
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

NFOBJECT* _make_subFixed_image_set(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_image_set_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    	
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_rotate(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_rotate_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    	
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_focus(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_focus_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_white_balance(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_white_balance_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_wide_dynamic(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_wide_dynamic_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_day_night(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_day_night_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_mode(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_mode_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height+20;
    return subFixed;
}

NFOBJECT* _make_subFixed_colorvu(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_colorvu_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_blc(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_blc_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
///    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_00FF00));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_dnr(gint ch, gint *subFixed_height)
{
  
    NFOBJECT *subFixed;
    gint height; 

    subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_dnr_subFixed(ch, subFixed);
    nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
    nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}



NFOBJECT* _make_subFixed_exposure_antiflicker(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_antiflicker_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
 //   nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FF0000));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_hlc(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_hlc_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
 //   nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FF0000));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_defog(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_defog_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, LEFT_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
 //   nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FF0000));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_time(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_time_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_gain(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_gain_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

NFOBJECT* _make_subFixed_exposure_iris(gint ch, gint *subFixed_height)
{
	NFOBJECT *subFixed;
    gint height;

	subFixed = (NFOBJECT*)nfui_nffixed_new();
    height = _make_exposure_iris_subFixed(ch, subFixed);
	nfui_nfobject_set_size(subFixed, RIGHT_PAGE_FIXED_W, height);
    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    		
//    nfui_nfobject_modify_bg(subFixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_0000FF));        	
	nfui_nfobject_show(subFixed);
    *subFixed_height = height;
    return subFixed;
}

