/*
 * vw_search_by_smart_rev_internal.h
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 26, 2014
 *
 */

#ifndef	__VW_SEARCH_BY_SMART_REV_INTERNAL_H
#define __VW_SEARCH_BY_SMART_REV_INTERNAL_H


#define SMART_SEARCH_STATUS     "smart_search_status"

enum {
	SMART_SEARCH_READY = 0,
	SMART_SEARCH_START,
	SMART_SEARCH_CMPL,
};


////////////////////////////////////////////////////////////
//
// public interfaces
//

gint _search_by_smart_rev_rule_page(NFOBJECT *parent);
void _search_by_smart_rev_rule_show(NFOBJECT *parent);
void _search_by_smart_rev_rule_hide(NFOBJECT *parent);

gint _search_by_smart_rev_result_page(NFOBJECT *parent);
void _search_by_smart_rev_result_show(NFOBJECT *parent);
void _search_by_smart_rev_result_hide(NFOBJECT *parent);

NFOBJECT *_get_searchbysmart_search_btn();
NFOBJECT *_get_searchbysmart_playback_btn();

gint vw_search_by_smart_rev_filter_open(NFWINDOW *parent, guint *rule_msk, guint *event_msk);

#endif
