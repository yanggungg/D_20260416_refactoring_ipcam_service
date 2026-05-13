/*
 * vw_dvabx_schd.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

////////////////////////////////////////////////////////////
//
// public interfaces
//

#ifndef __VW_DVAVX_SCHED_H__
#define __VW_DVAVX_SCHED_H__

gint VW_dvabx_schd_init_page(NFOBJECT *parent);

gboolean VW_dvabx_schd_tab_in_handler();
gboolean VW_dvabx_schd_tab_out_handler();

#endif

