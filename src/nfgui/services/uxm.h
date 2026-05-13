/*
 * uxm.h
 * 	- dependency :
 *                   
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */

#ifndef __UXM_H
#define __UXM_H


#include "iux_afx.h"
#include "cmm.h"
#include "../viewers/objects/nfobject.h"

////////////////////////////////////////////////////////////////
//
// public data type
//




////////////////////////////////////////////////////////////////
//
// public interfaces
//

IUXAPI int uxm_init();
IUXAPI int uxm_reg_imsg_event(NFOBJECT *obj, IMSG msgid);
IUXAPI int uxm_unreg_imsg_event(NFOBJECT *obj, IMSG msgid);
IUXAPI int uxm_monitor_on_imsg_event(NFOBJECT *obj, IMSG msgid);
IUXAPI int uxm_monitor_off_imsg_event(NFOBJECT *obj, IMSG msgid);
IUXAPI int uxm_bootup();
IUXAPI int uxm_run();

IUXAPI int uxm_init_auto_logout();
IUXAPI int uxm_start_auto_logout();
IUXAPI int uxm_stop_auto_logout();
IUXAPI int uxm_reset_auto_logout_timer();

IUXAPI int uxm_leave_from_live();
IUXAPI int uxm_is_booting();
#endif
