/*
 * scm_zig.c
 * 	- system 
 *	- dependencies :
 *		
 *
 * Written by Jungkyu PARK. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Dec 24, 2012
 *
 */

#include <glib.h>
#include "iux_afx.h"
#include "scm_internal.h"
#include "scm.h"
#include "evt.h"
#include "smt.h"
#include "var.h"
#include "ix_mem.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_SCREEN"



////////////////////////////////////////////////////////////
//
// private functions
//

static int _send_cmd_live_div_change(char ch_arr[32], NF_DISPLAY_E div)
{
    char *parr;
    
    DMSG(1, "called");

    parr = imalloc(sizeof(char)*32);
    memcpy(parr, ch_arr, sizeof(char)*32);
    evt_send_to_local(INFY_SCREEN_LIVE_DIV_CHANGE, div, 1, parr);
    return 0;
}

static int _send_cmd_live_full_toggle()
{
    DMSG(1, "called");
    
    evt_send_to_local(INFY_SCREEN_LIVE_FULL_TOGGLE, 0, 0, 0);
    return 0;
}

static int _send_cmd_live_osd_toggle()
{
    DMSG(1, "called");

    evt_send_to_local(INFY_SCREEN_LIVE_OSD_TOGGLE, 0, 0, 0);
    return 0;
}

static int _send_cmd_live_sequence_toggle()
{
    DMSG(1, "called");

    evt_send_to_local(INFY_SCREEN_LIVE_SEQ_TOGGLE, 0, 0, 0);
    return 0;
}



////////////////////////////////////////////////////////////
//
// protected interfaces
//




////////////////////////////////////////////////////////////
//
// protected handlers
//




////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_screen_live_div_change(char ch_arr[32], NF_DISPLAY_E div)
{
    if (is_iux_initialized() != 1) return -1;
    if ((smt_get_service() != SMT_LIVE) && (smt_get_service() != SMT_LOGOUT)) return -1;

    _send_cmd_live_div_change(ch_arr, div);
	return 0;
}

int scm_screen_live_full_toggle()
{
    if (is_iux_initialized() != 1) return -1;
    if ((smt_get_service() != SMT_LIVE) && (smt_get_service() != SMT_LOGOUT)) return -1;

    _send_cmd_live_full_toggle();
	return 0;
}

int scm_screen_live_osd_toggle()
{
    if (is_iux_initialized() != 1) return -1;
    if ((smt_get_service() != SMT_LIVE) && (smt_get_service() != SMT_LOGOUT)) return -1;

    _send_cmd_live_osd_toggle();
	return 0;
}

int scm_screen_live_sequence_toggle()
{
    if (is_iux_initialized() != 1) return -1;
    if ((smt_get_service() != SMT_LIVE) && (smt_get_service() != SMT_LOGOUT)) return -1;

    _send_cmd_live_sequence_toggle();
	return 0;
}

