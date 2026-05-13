/*
 * vw_sys_camera_analysis_plateno_database.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Aug 20, 2019
 *
 */


////////////////////////////////////////////////////////////
//
// public interfaces
//

#ifndef	_VW_SYS_CAMERA_ANALYSIS_PALTENO_DATABASE_H_
#define _VW_SYS_CAMERA_ANALYSIS_PALTENO_DATABASE_H_

gint VW_analysis_plateno_database_init_page(NFOBJECT *parent);

gboolean VW_analysis_plateno_database_tab_in_handler();
gboolean VW_analysis_plateno_database_tab_out_handler();

#endif

