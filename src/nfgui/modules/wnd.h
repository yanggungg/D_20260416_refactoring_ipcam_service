/*
 * wnd.h
 * 	- auto-logout manager
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Aug 9, 2011
 *
 */

#ifndef __WND_H
#define __WND_H

#include "ix_func.h"
#include "../viewers/objects/nfwindow.h"



////////////////////////////////////////////////////////////
//
// public data type 
//



////////////////////////////////////////////////////////////
//
// protected interfaces
//

int wnd_init(GtkWindow *top_wnd);
int wnd_cleanup();
int wnd_register(NFWINDOW *parent, NFWINDOW *wnd);
int wnd_close(NFWINDOW *wnd, NFWINDOW *req, int retkey);
int wnd_delete(NFWINDOW *wnd);
int wnd_set_level(NFWINDOW *wnd);
int wnd_set_title(NFWINDOW *wnd, char *title);
NFWINDOW *wnd_find_window(const char *name);
int wnd_get_level(NFWINDOW *wnd);
int wnd_broadcast_event(GdkEventType type);


#endif
