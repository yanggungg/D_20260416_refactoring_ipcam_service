/*
 * vw_sys_camera_analysis.h
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 28, 2019
 *
 */

////////////////////////////////////////////////////////////
//
// public interfaces
//
#ifndef _VW_SYS_CAMERA_ANALYSIS_H_
#define _VW_SYS_CAMERA_ANALYSIS_H_

#include "objects/nfobject.h"

gint VW_analysis_init_page(NFOBJECT *parent);

gboolean VW_analysis_tab_in_handler(void);
gboolean VW_analysis_tab_out_handler(void);
gboolean VW_analysis_tab_changed_handler(void);

#endif

