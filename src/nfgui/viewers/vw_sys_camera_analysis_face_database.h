/*
 * vw_sys_camera_analysis_face_database.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Aug 20, 2019
 *
 */


////////////////////////////////////////////////////////////
//
// public interfaces
//

#ifndef	__VW_SYS_CAMERA_ANALYSIS_FACE_DATABASE_H__
#define	__VW_SYS_CAMERA_ANALYSIS_FACE_DATABASE_H__

gint VW_analysis_face_database_init_page(NFOBJECT *parent);

gboolean VW_analysis_face_database_tab_in_handler();
gboolean VW_analysis_face_database_tab_out_handler();

#endif

