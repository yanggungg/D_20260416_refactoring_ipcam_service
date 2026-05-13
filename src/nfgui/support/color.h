#ifndef _COLOR_H_
#define _COLOR_H_

#include "nf_afx.h"
#include "nf_ui_color.h"

#if 0

//  CR_  : prefix
//  종류
/*********************************************
      FG : text color
      BG : background color
      LN : line color
      FR : background를 둘러싸는 외곽프레임
      OT : 외곽프레임을 둘러싸는 외곽선

// 상태
     NR : normal
     CL : click
     OV : mouse over
     DS : disable
     SE : selected
     DE : deselected
     FC : focused
	 PR : prelight
	 AT : active
*********************************************/     


//################################################################################################

//################################################################################################
/*###################################################*/
// FG
/*###################################################*/

// tab title color  
#define CR_FG_NR_SWND_TAB               NF_COLOR_CODE_EFF4FA
#define CR_FG_CL_SWND_TAB               
#define CR_FG_OV_SWND_TAB               
#define CR_FG_DS_SWND_TAB               
#define CR_FG_SE_SWND_TAB               NF_COLOR_CODE_FFB638
#define CR_FG_DE_SWND_TAB                    

// subtab(record setup) title color

#define CR_FG_OV_SWND_SUBTAB					
#define CR_FG_DS_SWND_SUBTAB
#define CR_FG_SE_SWND_SUBTAB
#define CR_FG_DE_SWND_SUBTAB

// setup window의 table에서 row/col에 위치하는 항목의 title color 

// tab page 안에서 사용되는 text color.. 
// ex) setup에서 up, down text or archiving에서의 log text 등.
#define CR_FG_SWND_TEXT					NF_COLOR_CODE_4B80BD//

// POPUP WINDOW (PWND)
// TITLE: popup window의 맨 상위에 위치하는 title
// SUBT: 각 항목의 title
// TEXT: popup window에서 사용되는 text color ex) archiving info 창에서의 ch1 ~ ch16 & log text.
// CATE: snapshot에서 ARCHIVING, E-MAIL 과 같이 사용되는 text color.
#define CR_FG_PWND_SUBT					NF_COLOR_CODE_FFFFFF//
#define CR_FG_PWND_TEXT					NF_COLOR_CODE_81A6D1
#define CR_FG_PWND_CATE					NF_COLOR_CODE_B2DFFF//
#define CR_FG_PWND_BOOT					NF_COLOR_CODE_084A94//
#define CR_FG_PWND_MBOX					NF_COLOR_CODE_84A5CE//

// popup window의 table에서 row/col에 위치하는 항목의 title color 
#define CR_FG_PWND_SUBT_ROW				NF_COLOR_CODE_84A5CE//
#define CR_FG_PWND_SUBT_COL				NF_COLOR_CODE_84A5CE//

// 공통적으로 사용되는 text color (label, spinbutton, timespin, etc.)
#define CR_FG_NR_SPOT_CELL              NF_COLOR_CODE_F8FCF8//

// Live Time Display
#define CR_FG_NF_LIVETIME               NF_COLOR_CODE_D6E6F7//

// archiving 에서 timeline의 시간을 표시하는 label의 text color
#define CR_FG_ARCH_TIME              	NF_COLOR_CODE_000819//

// widgets안에서 사용되는 default text color
#define CR_FG_NR_WIDGET					CR_FG_NR_CELL//
#define CR_FG_FC_WIDGET					NF_COLOR_CODE_000819//

// TEXT SHADOW
#define CR_FG_TEXT_SHADOW				NF_COLOR_CODE_24262C

// calendar
#define CR_FG_NR_CAL_YEAR               NF_COLOR_CODE_C5F7FF//
#define CR_FG_NR_CAL_MONTH				NF_COLOR_CODE_C5F7FF//
#define CR_FG_NR_CAL_SUN                NF_COLOR_CODE_F80000//
#define CR_FG_NR_CAL_MON                NF_COLOR_CODE_81A6D1//
#define CR_FG_NR_CAL_SAT                NF_COLOR_CODE_80D0E0//

#define CR_FG_NR_CAL_NODATA_DAY			NF_COLOR_CODE_81A6D1
#define CR_FG_FC_CAL_NODATA_DAY			NF_COLOR_CODE_0C1941
#define CR_FG_NR_CAL_DATA_DAY			NF_COLOR_CODE_D6E7FD
#define CR_FG_FC_CAL_DATA_DAY			NF_COLOR_CODE_FFFFFF

// button

// COLOR TEST
//#define CR_FG_NR_BTN                    NF_COLOR_CODE_FFFF00//
//#define CR_FG_CL_BTN                    NF_COLOR_CODE_FFFF00//
//#define CR_FG_OV_BTN                    NF_COLOR_CODE_FFFF00//
//#define CR_FG_DS_BTN                    NF_COLOR_CODE_FFFF00//

//#define CR_BG_TEST                      NF_COLOR_CODE_000080 

// main menu button
#define CR_FG_NR_MMENU_BTN              NF_COLOR_CODE_B2D1F1
#define CR_FG_CL_MMENU_BTN              NF_COLOR_CODE_B2D1F1
#define CR_FG_OV_MMENU_BTN              NF_COLOR_CODE_B2D1F1
#define CR_FG_DS_MMENU_BTN              NF_COLOR_CODE_B2D1F1

// main menu의 submenu  
#define CR_FG_NR_MMENU_SUBM				NF_COLOR_CODE_1378DD
#define CR_FG_NR_MMENU_SUBM_TITLE   	NF_COLOR_CODE_B2D1F1  // otm

// livedisplay ctrlbar fg 
#define CR_FG_NR_LIVE_CTRL_TEXT			NF_COLOR_CODE_4A84BD//
#define CR_FG_CL_LIVE_CTRL_TEXT			NF_COLOR_CODE_4A84BD//
#define CR_FG_NR_LIVE_CTRL_TIME_TEXT	NF_COLOR_CODE_D4E6F6//

// livedisplay에서 각 분할화면에 표시되는 text & shadow color 
#define CR_FG_LIVE_TEXT					NF_COLOR_CODE_FFFFFF
#define CR_FG_LIVE_TEXT_SHADOW			NF_COLOR_CODE_505050

// livelog 
#define	CR_FG_NR_LIVE_LOG				NF_COLOR_CODE_81A6D1//
#define CR_FG_FC_LIVE_LOG				NF_COLOR_CODE_00091C//
#define CR_VRT_BG_TIMELINE				NF_COLOR_CODE_04101E//
#define CR_HRZ_BG_TIMELINE				NF_COLOR_CODE_16222B//

// power window의 button text color
#define CR_FG_NR_PWR_BTN				NF_COLOR_CODE_FFFFFF

// live playback fg color
#define CR_FG_NR_PB_TEXT				NF_COLOR_CODE_FFFFFF

// popup menu text color
#define CR_FG_NR_LIVE_PMENU				NF_COLOR_CODE_FFFFFF//
#define CR_FG_FC_LIVE_PMENU				NF_COLOR_CODE_3A2100//
#define CR_FG_AT_LIVE_PMENU				NF_COLOR_CODE_3A2100//
#define CR_FG_DE_LIVE_PMENU				NF_COLOR_CODE_263140

//setup menu title FG
#define CR_FG_SETUP_MENU_TITLE			NF_COLOR_CODE_FFFFFF

// main menu title FG
#define CR_FG_MAIN_MENU_TITLE			NF_COLOR_CODE_FFFFFF

// Zoom Camera title layout Color
#define CR_FG_ZOOM_LAYOUT				NF_COLOR_CODE_101010

////################################################################################################
// BG

// setup window
#define CR_BG_SWND						NF_COLOR_CODE_0B131F

// tab bg color
#define CR_BG_NR_SWND_TAB				CR_BG_SWND
#define CR_BG_CL_SWND_TAB				CR_BG_SWND_TAB_PAGE
#define CR_BG_OV_SWND_TAB
#define CR_BG_DS_SWND_TAB
#define CR_BG_SE_SWND_TAB
#define CR_BG_DE_SWND_TAB

// subtab(record setup) title bg 
#define CR_BG_NR_SWND_SUBTAB			NF_COLOR_CODE_000819
#define CR_BG_CL_SWND_SUBTAB			NF_COLOR_CODE_84A5C5	
#define CR_BG_OV_SWND_SUBTAB
#define CR_BG_DS_SWND_SUBTAB
#define CR_BG_SE_SWND_SUBTAB
#define CR_BG_DE_SWND_SUBTAB				

// tab / subtab page color
#define CR_BG_SWND_SUBTAB_PAGE			NF_COLOR_CODE_000819//

// table쩔징쩌짯 row/col쩔징 ?짠횆징횉횕쨈횂 횉횞쨍챰?횉 title bg color 


// popup window
// INMOST: 가장 안쪽에 위치하는 영역의 bg.
// INNER: INMOST 바깥에 위치하는 영역의 bg (inmost의 border line으로 사용함). 
// OUTER: INNER 바깥에 위치하는 영역의 bg.
// OUTMOST: 가장 바깥에 위치하는 영역의 bg (popup window의 border line으로 사용함).
// MODIFY_OUTMOST: OUTMOST와 다른 색을 정의할 때 사용 (outmost와 다른 색의 border line으로 사용함).
// TAREA: title 영역의 bg.
// SUBT: 각 항목의 bg.
#define CR_BG_PWND						NF_COLOR_CODE_000819//
#define CR_BG_PWND_INMOST				NF_COLOR_CODE_224365//
#define CR_BG_PWND_INMOST_BOOT			NF_COLOR_CODE_00091C//
#define CR_BG_PWND_MODIFY_INMOST		NF_COLOR_CODE_00091C
#define CR_BG_PWND_INNER				NF_COLOR_CODE_084A94//
#define CR_BG_PWND_OUTER				NF_COLOR_CODE_00091C//
#define CR_BG_PWND_OUTMOST				NF_COLOR_CODE_00A8FF//
#define CR_BG_PWND_MODIFY_OUTMOST		NF_COLOR_CODE_00A8FF
#define CR_BG_PWND_TAREA				NF_COLOR_CODE_0B131F//
#define CR_BG_PWND_SUBT					CR_BG_PWND_INMOST//

// table에서 row/col에 위치하는 항목의 title color .
#define CR_BG_PWND_SUBT_ROW				NF_COLOR_CODE_2E4B6D//
#define CR_BG_PWND_SUBT_COL				NF_COLOR_CODE_2E4B6D//

#define	CR_BG_NR_PWND_CELL				CR_BG_NR_CELL
#define	CR_BG_DS_PWND_CELL				CR_BG_DS_CELL

// sequece conf popup window bg
#define CR_BG_PWND_SEQ_CONF				NF_COLOR_CODE_000819
#define	CR_LN_SEQ_CONF_BORDER			CR_BG_PWND_OUTMOST

// sequence setup button area bg
#define CR_BG_PWND_SEQ_BAREA			NF_COLOR_CODE_000819

// sequence Setup Rectangle BG (select Area)
#define CR_BG_PWND_SEQ_INMOST			NF_COLOR_CODE_212121

// sequence conf & cell
#define CR_BG_NR_SEQ_CONF				NF_COLOR_CODE_F8FCF8
#define CR_BG_NR_SEQ_CELL				NF_COLOR_CODE_5A708F

// spot conf popup window bg
#define CR_BG_PWND_SPOT_CONF			NF_COLOR_CODE_000819
#define	CR_LN_SPOT_CONF_BORDER			CR_BG_PWND_OUTMOST

// sequence Setup Rectangle BG (select Area)
#define CR_BG_PWND_SPOT_INMOST			NF_COLOR_CODE_212121

// spot setup button bg
#define CR_BG_PWND_SPOT_BAREA			NF_COLOR_CODE_000819

// spot conf & cell
#define CR_BG_NR_SPOT_CONF				NF_COLOR_CODE_F8FCF8
#define CR_BG_NR_SPOT_CELL				NF_COLOR_CODE_224365

// 분할 화면의 빈 분할 화면의 bg 
// ex) 8ch UI일 때 9분할 화면에서 마지막 분할 화면의 bg.
#define CR_BG_WND_BLANK                	NF_COLOR_CODE_24262C 

// livedisplay ctrlbar win 
#define CR_BG_LIVE_CTRL					NF_COLOR_CODE_000008 

// livedisplay popup menu 
//#define CR_BG_NR_LIVE_PMENU             NF_COLOR_CODE_9C9CAD

// main menu
#define CR_BG_NR_MMENU					NF_COLOR_CODE_101010
#define CR_BG_NR_MMENU_TITLE			NF_COLOR_CODE_212929

// alpha bg
#define CR_BG_NR_ALP					NF_COLOR_CODE_000000 // TEST

// livelog normal/focus log text 영역의 bg
#define CR_BG_NR_LIVELOG				NF_COLOR_CODE_224365//
#define CR_BG_FC_LIVELOG				NF_COLOR_CODE_00C6FF//

// 공통적으로 사용되는 bg (label, spinbutton, timespin, etc.)

// archiving timeline all/time label bg
#define CR_BG_ARCH_TIMELINE_ALL			NF_COLOR_CODE_213A52
#define CR_BG_ARCH_TIMELINE_TIME_FIXED	NF_COLOR_CODE_212121
#define CR_BG_ARCH_TIMELINE_TIME		NF_COLOR_CODE_9FF3EE//

// panorama ctrl box bg 
#define CR_BG_NR_PANO_CTRL				NF_COLOR_CODE_101010

// remocon id wnd
#define CR_BG_REM_ID					CR_BG_PWND_OUTER

// common widgets bg
#define CR_BG_NR_WIDGET					NF_COLOR_CODE_243751//
#define CR_BG_FC_WIDGET					NF_COLOR_CODE_00C5FF//

// timeline bg
#define CR_BG_NR_TIMELINE              	NF_COLOR_CODE_213A52//
#define CR_BG_TIMELINE_ODD				NF_COLOR_CODE_19293A
#define CR_BG_TIMELINE_EVEN				NF_COLOR_CODE_213A52

// timeline의 timebar 
#define CR_BG_NR_TIMELINE_TBAR			NF_COLOR_CODE_00FF00//
// timeline의 event별 statusbar 
#define CR_BG_NR_TLN_SBAR_USER			NF_COLOR_CODE_0000FF

// listbox scroll 영역의 bg 
#define CR_BG_NR_LIST					NF_COLOR_CODE_19293A//
#define CR_BG_NR_LIST_SCL				NF_COLOR_CODE_19293A//
#define CR_BG_NR_LIST_SCL_BTN			NF_COLOR_CODE_6B7D9F

// progressbar bg color
#define CR_BG_NR_PRB					NF_COLOR_CODE_101010
#define CR_BG_PR_PRB					NF_COLOR_CODE_6B7D9F
#define CR_BG_AT_PRB					NF_COLOR_CODE_101010

// calendar data color
#define CR_BG_CAL_TAREA 				NF_COLOR_CODE_213A52//
#define CR_BG_CAL_WAREA					NF_COLOR_CODE_213A52//
#define CR_BG_CAL_DAREA					NF_COLOR_CODE_000819//

//#define CR_BG_NR_CAL_NODATA_DAY			NF_COLOR_CODE_224365
#define CR_BG_NR_CAL_NODATA_DAY			NF_COLOR_CODE_00091C
#define CR_BG_NR_CAL_DATA_DAY			NF_COLOR_CODE_52717C
#define CR_BG_FC_CAL_NODATA_DAY			NF_COLOR_CODE_00CDFF
#define CR_BG_FC_CAL_DATA_DAY			NF_COLOR_CODE_049792



// popup menu bg color
#define CR_BG_NR_LIVE_PMENU				NF_COLOR_CODE_00091C//
#define CR_BG_FC_LIVE_PMENU				NF_COLOR_CODE_00C6FF//
#define CR_BG_AT_LIVE_PMENU				NF_COLOR_CODE_00C5FF//
#define CR_BG_DE_LIVE_PMENU				NF_COLOR_CODE_001042

//Live playback BG
#define CR_BG_LIVE_PLAYBACK				NF_COLOR_CODE_396384

// sub menu background outline
#define CR_BG_DE_MAIN_SUBMENU			NF_COLOR_CODE_000819
#define CR_FG_DE_MAIN_SUBMENU 			NF_COLOR_CODE_7BCEFF

//##########################################################
// FRAME

//##########################################################

// LINE
// IL: inline
// OL: outline

// common widgets outline
#define CR_LN_NR_WIDGET_OL				NF_COLOR_CODE_FF8023//
#define CR_LN_SE_WIDGET_OL				NF_COLOR_CODE_FF4E00//

// calendar outline 
#define CR_LN_NR_CAL_OL					NF_COLOR_CODE_000819//

// tile inner line (default)
#define CR_LN_NR_TILE_IL				NF_COLOR_CODE_101010	
#define CR_LN_FC_TILE_IL				NF_COLOR_CODE_EF6321//

// record setup에서의 tile outline
#define CR_LN_REC_TILE_OL		  	    NF_COLOR_CODE_6FA2FF

// livedisplay화면에서 각 분할 화면의 focus line
#define CR_LN_FC_LIVE_OL				NF_COLOR_CODE_A04141

// livelog의 preview 영상의 outline color
#define CR_LN_LIVELOG_IMG_OL			NF_COLOR_CODE_00A5FF

// remocon id outline 
#define CR_LN_REM_ID_OL					CR_BG_PWND_OUTMOST

// timespin outline in search_by_time 
#define CR_LN_SEARCH_TMSPIN_OL			NF_COLOR_CODE_000819

// sequence conf normal outline
#define CR_LN_PWND_SEQ_CONF_OL			CR_BG_PWND_OUTMOST		

#define CR_LN_FC_SEQ_CONF_OL			NF_COLOR_CODE_FFCC00

// spot conf focus outline
#define CR_LN_PWND_SPOT_CONF_OL			CR_BG_PWND_OUTMOST		

#define CR_LN_FC_SPOT_CONF_OL			NF_COLOR_CODE_FFCC00

// logOn의 가로줄  
#define CR_LN_LOGON_STRAIGHT           	NF_COLOR_CODE_224365
#define CR_LN_LOGON_STRAIGHT2          	NF_COLOR_CODE_214263

// zoom 화면에서 우측 하단에 작은 box line 
#define CR_LN_NR_ZOOM_BOX_OL			NF_COLOR_CODE_FFFFFF
#define CR_LN_NR_ZOOM_ZBOX_OL			NF_COLOR_CODE_FFFFFF

// archiving burning 창의 progressbar outline
#define CR_LN_PRB_OL					NF_COLOR_CODE_314A29

//search playback inner line 
#define CR_LN_SEARCH_PB_IL				NF_COLOR_CODE_FFFFFF

//Popup menu line color
#define CR_LN_PMENU_IL					NF_COLOR_CODE_00A5FF//

//Record Tab Outline
#define CR_LN_NR_REC_TAB                NF_COLOR_CODE_84A5C5

//Live Playback Borderline Color
#define	CR_LN_LIVE_PB_OUTMOST			CR_BG_PWND_OUTMOST
#define	CR_LN_LIVE_PB_OUTER				CR_BG_PWND_OUTMOST
#define	CR_LN_LIVE_PB_DISP_BORDER		NF_COLOR_CODE_FFFFFF

// Tooltip
#define	CR_LN_TOOLTIP					NF_COLOR_CODE_505050	
#define	CR_BG_TOOLTIP					NF_COLOR_CODE_FFFFA0	
#define	CR_FG_TOOLTIP					NF_COLOR_CODE_505050	

//live status menu text color
#define CR_FG_STATUSNR_BTN				NF_COLOR_CODE_4291DF
#define CR_FG_STATUSCL_BTN              NF_COLOR_CODE_2CA6DF
#define CR_FG_STATUSOV_BTN				NF_COLOR_CODE_00A7FF
#define CR_FG_STATUSDS_BTN				CR_FG_DS_BTN

//power window text color
#define CR_FG_PWIN_NR_BTN				NF_COLOR_CODE_75A7E1
#define CR_FG_PWIN_CL_BTN               NF_COLOR_CODE_4487CB
#define CR_FG_PWIN_OV_BTN				NF_COLOR_CODE_7DD6FF
#define CR_FG_PWIN_DS_BTN				NF_COLOR_CODE_043461

// msg box ui text color
#define CR_FG_PWND_MSG					NF_COLOR_CODE_111931

//remocon id window text color, only ctype
#define CR_FG_RMCID_NR_CELL             NF_COLOR_CODE_C4E2FF

// userpwd btn text color
#define CR_FG_NR_NOR_BTN                NF_COLOR_CODE_A59C8C
#define CR_FG_CL_NOR_BTN                NF_COLOR_CODE_945A08
#define CR_FG_OV_NOR_BTN                NF_COLOR_CODE_945A08
#define CR_FG_DS_NOR_BTN                NF_COLOR_CODE_4A4242


#define CR_MORMAL_PB_TIME                NF_COLOR_CODE_FFB638

/////////////////////////////// ETC ////////////////////////////////

// COMMON
#define CR_LN_LISTBOX_SEL				NF_COLOR_CODE_FFFFFF

// COMMON
#define CR_FG_ST_REC_REASON_TEXT		NF_COLOR_CODE_0B131F

// COMMON
#define CR_FG_SWND_TITLE				NF_COLOR_CODE_0B131F

#define CR_FG_NR_CELL                  	NF_COLOR_CODE_FFFFFF
#define CR_FG_DS_CELL                  	NF_COLOR_CODE_232C37

#define CR_BG_NR_CELL					NF_COLOR_CODE_425570
#define CR_BG_DS_CELL					NF_COLOR_CODE_111A2D

#define CR_FG_SWND_SUBT_ROW				NF_COLOR_CODE_0B131F
#define CR_BG_SWND_SUBT_ROW				CR_BG_SWND_TAB_PAGE

#define CR_FG_SWND_SUBT_COL				NF_COLOR_CODE_0B131F
#define CR_BG_SWND_SUBT_COL				CR_BG_SWND_TAB_PAGE







///////////////////////////////////////////////////////////////
//////
//////		IPX MODIFY LIST.

#define CR_LN_PREVIEW_BOX				NF_COLOR_CODE_273B57

#define CR_BG_SWND_TAB_PAGE				NF_COLOR_CODE_A8B6C4
#define CR_BG_SWND_POPUP_PAGE			NF_COLOR_CODE_0B131F

#define CR_FG_SWND_SUBT					NF_COLOR_CODE_0B131F
#define CR_BG_SWND_SUBT					CR_BG_SWND_TAB_PAGE

#define CR_FG_SWND_SUBT_HEAD			NF_COLOR_CODE_FFFFFF
#define CR_BG_SWND_SUBT_HEAD			NF_COLOR_CODE_273B57

#define CR_BG_HRZ_TML_RULER				NF_COLOR_CODE_273B57

// TYP1 : FG-WHITE, BG-BLUE
#define CR_FG_NR_CELL_TYP1             	NF_COLOR_CODE_FFFFFF
#define CR_FG_DS_CELL_TYP1             	NF_COLOR_CODE_232C37

#define CR_BG_NR_CELL_TYP1				NF_COLOR_CODE_425570
#define CR_BG_DS_CELL_TYP1				NF_COLOR_CODE_111A2D

// TYP2 : FG-BLUE, BG-WHITE
#define CR_FG_NR_CELL_TYP2             	NF_COLOR_CODE_106296
#define CR_FG_DS_CELL_TYP2             	NF_COLOR_CODE_555A63

#define CR_BG_NR_CELL_TYP2				NF_COLOR_CODE_EFF4FA
#define CR_BG_DS_CELL_TYP2				NF_COLOR_CODE_797F88

#define CR_FG_NR_BTN                    NF_COLOR_CODE_FFFFFF
#define CR_FG_CL_BTN                    NF_COLOR_CODE_3A2100
#define CR_FG_OV_BTN                    NF_COLOR_CODE_3A2100
#define CR_FG_DS_BTN                    NF_COLOR_CODE_263140

/*
 * snf
#define CR_BG_NR_TLN_SBAR_TIMER			NF_COLOR_CODE_BBDFB7
#define CR_BG_NR_TLN_SBAR_ALARM			NF_COLOR_CODE_FF4D58
#define CR_BG_NR_TLN_SBAR_MOT			NF_COLOR_CODE_FF9C39
#define CR_BG_NR_TLN_SBAR_USER			NF_COLOR_CODE_0000FF
#define CR_BG_NR_TLN_SBAR_PANIC			NF_COLOR_CODE_FDFF55
*/

#define CR_BG_NR_TLN_SBAR_TIMER			NF_COLOR_CODE_2C5A27
#define CR_BG_NR_TLN_SBAR_ALARM			NF_COLOR_CODE_FF0000
#define CR_BG_NR_TLN_SBAR_MOT			NF_COLOR_CODE_00BEFE
#define CR_BG_NR_TLN_SBAR_USER			NF_COLOR_CODE_0000FF
#define CR_BG_NR_TLN_SBAR_PANIC			NF_COLOR_CODE_FFFF00
#define CR_BG_NR_TLN_SBAR_PRE			NF_COLOR_CODE_F60FFF
#define CR_BG_NR_TLN_SBAR_TIMER_I		NF_COLOR_CODE_70A573
#define CR_BG_NR_TLN_SBAR_ALARM_I		NF_COLOR_CODE_E07979
#define CR_BG_NR_TLN_SBAR_MOT_I			NF_COLOR_CODE_B8E4FF
#define CR_BG_NR_TLN_SBAR_USER_I		NF_COLOR_CODE_0000FF
#define CR_BG_NR_TLN_SBAR_PANIC_I		NF_COLOR_CODE_FFFFC0
#define CR_BG_NR_TLN_SBAR_PRE_I			NF_COLOR_CODE_EA71D5
#define CR_TML_PLAYBAR					NF_COLOR_CODE_00FFFF
#define CR_TML_CTI						NF_COLOR_CODE_FF0000

#define CR_FG_NR_SWND_SUBTAB			NF_COLOR_CODE_A8B6C4
#define CR_FG_CL_SWND_SUBTAB			NF_COLOR_CODE_0B131F

#define CR_TML_RULER_TEXT				NF_COLOR_CODE_FFFFFF
#define CR_TML_DATE						NF_COLOR_CODE_FFFF00

// tile의 상태에 따른 color
#define CR_BG_NR_TILE					NF_COLOR_CODE_00FFFF
#define CR_BG_OV_TILE					NF_COLOR_CODE_00FF00
#define CR_BG_SE1_TILE					NF_COLOR_CODE_16222B
#define CR_BG_SE2_TILE					NF_COLOR_CODE_16222B
#define CR_BG_SE3_TILE					NF_COLOR_CODE_16222B
#define CR_BG_SE4_TILE					NF_COLOR_CODE_16222B
#define CR_BG_SE5_TILE					NF_COLOR_CODE_16222B
#define CR_BG_SE6_TILE					NF_COLOR_CODE_16222B

#define CR_BG_TILE_REC_SELECT			NF_COLOR_CODE_00FF00
#define CR_BG_TILE_REC_ON				NF_COLOR_CODE_00FFFF
#define CR_BG_TILE_REC_OFF				NF_COLOR_CODE_16222B

#define CR_FG_PWND_TITLE				NF_COLOR_CODE_B4CBE2//

#endif

#endif

