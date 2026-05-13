/*
 * vw_search_by_smart_rev.h
 *
 * Written by eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 22, 2014
 *
 */

#ifndef	__VW_SEARCH_BY_SMART_REV_H
#define __VW_SEARCH_BY_SMART_REV_H


////////////////////////////////////////////////////////////
//
// public interfaces
//

void vw_init_SearchBySmart_rev_page(NFOBJECT *parent, time_t from, time_t to);
gboolean vw_SearchBySmart_rev_tab_out_handler(void);
gboolean vw_SearchBySmart_rev_tab_in_handler(void);
gboolean vw_SearchBySmart_rev_tab_show(void);

#endif

