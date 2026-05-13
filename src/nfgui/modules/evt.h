/*
 * evt.h
 * 	- event transfer module between the scm and uxm
 *	- dependencies :
 *		Gtk+
 *		GdkEvent
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 19, 2011
 *
 */

#ifndef __EVT_H
#define __EVT_H

#include "iux_afx.h"
#include "cmm.h"
#include "../viewers/objects/nfobject.h"

////////////////////////////////////////////////////////////
//
// public interfaces
//

int evt_init(GtkWidget *vwmain);
/*inline*/ CMMPORT evt_get_cmmport();
int evt_unref();
int evt_send_to_local(IMSG msgid, long param, bool dyn_data, void *data);
int evt_send_to_window(char *title, IMSG msgid, long param, bool dyn_data, void *data);
int evt_send_to_widget(NFOBJECT *widget, IMSG msgid, long param, bool dyn_data, void *data);

#endif
