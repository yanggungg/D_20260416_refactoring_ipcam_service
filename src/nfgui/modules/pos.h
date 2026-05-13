/*
 * pos.h
 * 	- POS(ATM) log table
 *	- dependencies :
 *	
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Sep 4, 2014
 *
 */

#ifndef __POS_H
#define __POS_H

#include "nf_logevtdef.h"
#include "nf_api_eventlog.h"
#include "nf_api_pos_eventlog.h"


////////////////////////////////////////////////////////////
//
// public data type 
//

typedef struct _POS_CONF_T {
	int             align;
	int             font_type;
} POS_CONF_T;

typedef struct _POS_DATA_T {
	guint64         id;
	guint64			timestamp;	
	int             font_color;
    char            text[128];
} POS_DATA_T;

typedef struct _POSX_T {
    int             pageid;
    guint64	        slogid;
    int             spos;
	int				count;
	POS_DATA_T		*data;
} POSX_T;


// page idx
// 0 ~ 15   : live page
// 16 ~ 31  : playback page
// 32       : test page



////////////////////////////////////////////////////////////
//
// public interfaces
//

int posx_init();
int posx_reload_property();

int posx_put_live_testlog(int ch, int index, char *str);
int posx_put_live_log(NF_LOG_DATA *log);
int posx_put_playback_log(NF_LOG_DATA *log);
int posx_put_test_log(int ch, char *str);
int posx_clear_test_log();


POSX_T *posx_create(int pageid);
int posx_destroy(POSX_T *posx);

int posx_get_live_display_onoff();
int posx_get_playback_display_onoff();

int posx_get_pos_conf(POS_CONF_T *conf);
int posx_get_pos_table(POSX_T *posx, int tbl_row);
int posx_get_pos_table_with_time(POSX_T *posx, int tbl_row, time_t gettime);

#endif 

