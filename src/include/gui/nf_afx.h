#ifndef _NF_AFX_H
#define _NF_AFX_H

#ifndef ENABLE_HNF_IPCAM
#define __IPCAM_DISABLE__
#endif

#define NF_TOOLTIP_ENABLE
#define	SHOW_TOOLTIP_ONLY_STRIP_STRING

#if defined(_ANF_0824CS)
#define	SHOW_TOOLTIP_ONLY_STRIP_STRING
#endif


#define _FTP_SETUP_

#if defined(GUI_4CH_SUPPORT)
    #define ALARM_OUT_1CH_SUPPORT
    //#define ZOOM_NOT_SUPPORT
    //#define SPOT_1CH_SUPPORT

    #define GUI_CHANNEL_CNT           4 
    #define NF_DISP_DEFAULT_MODE      NF_DISPLAY_QUAD
    #if defined(_ATM_0424) || defined(_ATM_0412)
        #define ZOOM_NOT_SUPPORT
    #endif
#elif defined(GUI_8CH_SUPPORT)

    #if defined(_ATM_0824) || defined(_ANF_0824CS) || defined(_ANF_0824CL) || defined(_OTM_0824)
        #define ALARM_OUT_1CH_SUPPORT
        #define _ATM1624_SQE_TEST_
	#elif defined(_ANF_0824) || defined(_SNF_0824) 
        #define ALARM_OUT_12CH_SUPPORT	// alarm ch 8 + relay out 4 ch
    #endif

    #define GUI_CHANNEL_CNT           8
    #define NF_DISP_DEFAULT_MODE      NF_DISPLAY_NONA
    
#elif defined(GUI_16CH_SUPPORT)
    #if defined(_ATM_1648) || defined(_ATM_1624) || defined(_OTM_1648)
        #define _ATM1624_SQE_TEST_
        #define ALARM_OUT_1CH_SUPPORT
        #define NF_DISP_DEFAULT_MODE      NF_DISPLAY_HEXADECA
	#elif defined(_ANF_1648) || defined(_SNF_1648)
        #define ALARM_OUT_20CH_SUPPORT	// alarm ch 16 + relay out 4 ch
    #endif
    
    #define GUI_CHANNEL_CNT           16
    #define NF_DISP_DEFAULT_MODE      NF_DISPLAY_HEXADECA

#elif defined(GUI_32CH_SUPPORT)
    #define GUI_CHANNEL_CNT           32
    #define NF_DISP_DEFAULT_MODE      NF_DISPLAY_HEXATRICONTA
#endif

//#define _MULTI_POPUP_

#if defined(_ATM_1648) || defined(_ATM_1624) || defined(_ATM_0824) ||   \
    defined(_ANF_0824CS) || defined(_ATM_0424) || defined(_ATM_0412) ||    \
    defined(_OTM_1648) || defined(_OTM_0824) || defined(_SNF_0424)      ||  \
    defined(_HDI_0412)
#define ALARM_IN_4CH_SUPPORT
#define ALARM_CH_OFF_SELECT_SUPPORT

#elif defined(_ANF_0824) || defined(_ANF_0824CL) || defined(_SNF_0824)
#define ALARM_IN_8CH_SUPPORT

#endif


#ifdef ALARM_IN_4CH_SUPPORT
#define ALARM_IN_CH_MAX    (4)
#else
#define ALARM_IN_CH_MAX    (GUI_CHANNEL_CNT)
#endif



#if defined(ALARM_OUT_1CH_SUPPORT)
#define ALARM_RELAY_CNT   1
#elif defined(ALARM_OUT_8CH_SUPPORT)
#define ALARM_RELAY_CNT   8
#elif defined(ALARM_OUT_12CH_SUPPORT)
#define ALARM_RELAY_CNT   12		// alarm ch 8 + relay out 4 ch
#elif defined(ALARM_OUT_20CH_SUPPORT)
#define ALARM_RELAY_CNT   20	    // alarm ch 16 + relay out 4 ch
#else
#define ALARM_RELAY_CNT   16
#endif


#ifdef __DIGI_SUPPORT__
#define PRE_5SEC_EVENT_SEARCH
#define USER_AUTHORITY_CREATE
#define ADMIN_ONLY_AUTHORITY
#define LIVE_SYS_INFO
#define OPTI_SEQ
#define REMOCON_EDIT_DELAY
#define DIGI_DDNS
//#define PLAYBACK_CIF_SCALER
#define SCHEDULED_ARCHIVING
#define AUTO_TIME_SYNC
#define DEFALT_E_MAIL_SERVER
#define __XRPLUS_UI__
#elif defined(__CBC_UI__)
#define ADMIN_ONLY_AUTHORITY
#endif


#if 0		//#ifdef _GUIDEVP	// Desktop

	#include <nf_ui_gtk.h>
	#include "emu_service.h"
	#include "nfdal.h"

	// Display size related...
	#define	CIF_WIDTH	352
	#define	CIF_HEIGHT	240
	#define	DISPLAY_CIF_WIDTH	352
	#define	DISPLAY_CIF_HEIGHT	240
	#define	DISPLAY_D1_WIDTH	704
	#define	DISPLAY_D1_HEIGHT	480

#else				// Target Board

	#include "nf_common.h"
	#include <nf_ui_gtk.h>
	#include <nfdal.h>

	#define NFUI_CIF_WIDTH		(DISPLAY_ACTIVE_WIDTH/4)    // D1, cultfactory
	#define NFUI_CIF_HEIGHT		(DISPLAY_ACTIVE_HEIGHT/4)   // D1, cultfactory
	#define	NFUI_D1_WIDTH		(DISPLAY_ACTIVE_WIDTH/2)    // D1, cultfactory
	#define	NFUI_D1_HEIGHT		(DISPLAY_ACTIVE_HEIGHT/2)   // D1, cultfactory
#endif


enum {
	DISPLAY_MODE_D1 = 0,
	DISPLAY_MODE_4D1,
	NUM_DISP_MODE
};




/************************************
 * TIME RELATED...
 * **********************************/

// FROM "nf_common.h"
// 2008-01-01 00:00:00
//#define NF_DATETIME_MIN	(1199145600)

//#define	NFUI_LOWER_TIMELIMIT	((time_t)(1072915200))	// GMT 2004/01/01 00:00:00
#define	NF_LOWER_TIMELIMIT		((time_t)(1199145600))	// GMT 2008-01-01 00:00:00	// NF_DATETIME_MIN
// #define	NF_UPPER_TIMELIMIT		((time_t)(2082758400 - 1))	// GMT 2035/12/31 23:59:59
#define	NF_UPPER_TIMELIMIT		((time_t)(4102444800 - 1))	// GMT 2099/12/31 23:59:59




/**************************************
 * NUMBER OF CHANNELS
 * ***********************************/
#define	NF_MAX_CAM_CHANNEL	(guint)(DISPLAY_IS_D1 ? 16:32)
#define	NF_VIEW_CAM_CHANNEL	8//(guint)(DISPLAY_IS_D1 ? 8:16)
#define	NF_MAX_VIEW_CHANNEL	16




/***********************************
 * STRING RELATED....
 * *********************************/
#define	NFUI_MAX_PW_SIZE		        (8)
#define	NFUI_MAX_ENHANCED_PW_SIZE		(16)


#endif


