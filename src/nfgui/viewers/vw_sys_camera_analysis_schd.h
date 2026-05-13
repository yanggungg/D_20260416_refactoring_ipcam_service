/*
 * vw_sys_camera_analysis_schd.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 28, 2019
 *
 */


////////////////////////////////////////////////////////////
//
// public interfaces
//

gint VW_analysis_schd_init_page(NFOBJECT *parent);

gboolean VW_analysis_schd_tab_in_handler();
gboolean VW_analysis_schd_tab_out_handler();
