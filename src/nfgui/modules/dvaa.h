/*
 * dvaa.h
 * 	- deeplearning video analytics agent
 *	- dependencies :
 *		
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */


#ifndef __DVAA_H
#define __DVAA_H

#include "dit.h"
#include "nf_notify.h"
#include <itx_ai_def.h>


////////////////////////////////////////////////////////////
//
// public data type
//


typedef enum _DVAA_META_E {
	DVAA_META_BBOX				= 0,
	DVAA_META_TRAJ				= 1,
	DVAA_MAX_META
} DVAA_META_E;

#if defined(__ITXGUI64)
typedef unsigned long int DVAAID;
#else
typedef unsigned int DVAAID;
#endif


////////////////////////////////////////////////////////////
//
// public interfaces 
//

int dvaa_init();
DVAAID dvaa_get_dvaaid(int ch);
DVAAID dvaa_get_pb_dvaaid(int ch);
int dvaa_is_supported();
int dvaa_set_auto_update_external_rules(int auto_update);
int dvaa_update_external_airules();
int dvaa_reload();
int dvaa_sync_aicam(int ch);
DITID dvaa_get_ditid(DVAAID id);
int dvaa_get_counted_rule(DVAAID id, unsigned short countid);
int	dvaa_parse_event(int ch, ai_rule_event_t *pevt, int *zid, int *cid);
int dvaa_activate_all_rule(DVAAID id);
int dvaa_deactivate_all_rule(DVAAID id);
int dvaa_activate_calb(DVAAID id);
int dvaa_deactivate_calb(DVAAID id);
int dvaa_activate_meta(DVAAID id, DVAA_META_E meta);
int dvaa_deactivate_meta(DVAAID id, DVAA_META_E meta);
int dvaa_blink_pattern(int ch, int patt);
int dvaa_notify_event(NF_NOTIFY_INFO *data);
int dvaa_notify_generic_event(NF_NOTIFY_INFO *data);
int dvaa_notify_track_info(NF_NOTIFY_INFO *data);

#endif
