/*
 * vw_sys_camera_analysis_prop.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 28, 2019
 *
 */


////////////////////////////////////////////////////////////
//
// public interfaces
//

#ifndef _VW_SYS_CAMERA_ANALYSIS_PROP_H_
#define _VW_SYS_CAMERA_ANALYSIS_PROP_H_

gint VW_analysis_prop_init_page(NFOBJECT *parent);

gboolean VW_analysis_prop_tab_in_handler();
gboolean VW_analysis_prop_tab_out_handler();
gboolean VW_analysis_prop_tab_changed_handler();

#endif

