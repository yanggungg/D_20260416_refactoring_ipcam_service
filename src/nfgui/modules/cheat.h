/*
 * cheat.h
 * 	- cheat key manager
 *	- dependency : event_loop
 *
 * Written by Eunji Lee. <jyoonl@itxsecurity.com>
 * Copyright (c) ITX security, Feb 21, 2013
 *
 */
#ifndef __CHEAT_H
#define __CHEAT_H

#include "iux_msg.h"
#include "cmm.h"
#include "nf_notify.h"
#include "support/event_loop.h"

void cheat_init();
void cheat_set_kpid(KEYPAD_KID kpid);
#endif
