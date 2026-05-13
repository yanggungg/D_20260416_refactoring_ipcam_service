/*
 * spt.h
 * 	- spot module
 *	- dependencies :
 *	
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Feb 22, 2012
 *
 */

#ifndef __SPT_H
#define __SPT_H

#include "nfdal.h"


#define MAX_CH					4
#define MAX_LIST				4
#define	MAX_ITEM				MAX_SPOT_ITEMS_NUM


////////////////////////////////////////////////////////////
//
// public data type 
//

typedef enum _SPOT_SCR_TYPE_E {
	SCR_NONE		= -1,
	SCR_1DIV		= SCR_DIV_TYPE1,
	SCR_4DIV		= SCR_DIV_TYPE4,
#ifndef GUI_4CH_SUPPORT	
#ifndef _NOT_SUPPORT_SPC_DIV
	SCR_6DIV		= SCR_DIV_TYPE6,
	SCR_8DIV		= SCR_DIV_TYPE8,
#endif	
	SCR_9DIV		= SCR_DIV_TYPE9,
#ifndef GUI_8CH_SUPPORT	
	SCR_16DIV		= SCR_DIV_TYPE16,
#ifndef GUI_16CH_SUPPORT	
	SCR_36DIV		= SCR_DIV_TYPE36,
#endif
#endif	
#endif	
} SPOT_SCR_TYPE_E; 

typedef struct _SPOT_SCREEN_T {
	SPOT_SCR_TYPE_E		type;
	int 				conf[MAX_ITEM];
} SPOT_SCREEN_T;

typedef enum _SPOT_CMD_E {
	SPOT_UPDATED,
	SPOT_CONFIG,
	SPOT_SEQ_CTRL,
	SPOT_OSD_CTRL,
} SPOT_CMD_E;

typedef enum _SPOT_MODE_E {
	SPOT_SEQUENCE,
	SPOT_POPUP,
	SPOT_FIX,
} SPOT_MODE_E;

typedef enum _SPOT_TYPE_E {
    SPOT_TYPE_SD,
    AUX_TYPE_SD,
    SPOT_TYPE_HD,
    SPOT_TYPE_DUAL,    
} SPOT_TYPE_E;

typedef struct _SPOT_CMD_T {
	SPOT_CMD_E		type;
	int 			spch;
    SPOT_TYPE_E     spottype;
	SPOT_SCREEN_T	scr;
	SECOND_D		dwell;
	int				param;
} SPOT_CMD_T;

typedef enum _SPOT_PIP_MODE {
	SPOT_PIP_LAYER_ON,
	SPOT_PIP_LAYER_OFF,
} SPOT_PIP_MODE;

#if defined(__ITXGUI64)
typedef unsigned long int SPT_ID;
#else
typedef unsigned int SPT_ID;
#endif

typedef int (*SPOT_SERVICE_PROC)(SPT_ID *pspt);


////////////////////////////////////////////////////////////
//
// public interfaces
//

SPT_ID *spt_create(int ch, SPOT_TYPE_E spottype);
int spt_destroy(SPT_ID pspt);
int spt_push_updating(SPT_ID pspt);
int spt_display_spot(SPT_ID sptid, SPOT_SCREEN_T *spot_scr, SECOND_D dwell);
int spt_display_previous(SPT_ID sptid);
int spt_control_spot_sequence(SPT_ID sptid, int mode);
int spt_control_spot_osd(SPT_ID sptid, ONOFF_E onoff);
#endif
