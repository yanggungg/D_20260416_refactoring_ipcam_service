/*
 * vw_sys_camera_vca_rev.h
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 10, 2014
 *
 */

////////////////////////////////////////////////////////////
//
// public interfaces
//
#include "objects/nfobject.h"

gint VW_VCA_Rev_init_page(NFOBJECT *parent);
gint VCA_Rev_start_preview();

gboolean VW_VCA_Rev_tab_in_handler(void);
gboolean VW_VCA_Rev_tab_out_handler(void);

