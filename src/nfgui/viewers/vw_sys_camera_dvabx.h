/*
 * vw_sys_camera_dvabx.h
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

////////////////////////////////////////////////////////////
//
// public interfaces
//
#include "objects/nfobject.h"

gint VW_dvabx_init_page(NFOBJECT *parent);
gint VW_dvabx_start_preview();

gboolean VW_dvabx_tab_in_handler(void);
gboolean VW_dvabx_tab_out_handler(void);

