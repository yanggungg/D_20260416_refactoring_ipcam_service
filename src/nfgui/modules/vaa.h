/*
 * vaa.h
 * 	- video analytics agent
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */


#ifndef __VAA_H
#define __VAA_H

#include "dit.h"
#include "nf_notify.h"
#include <ivca_def.h>


////////////////////////////////////////////////////////////
//
// public data type
//

typedef enum _VAA_META_E {
	VAA_META_BBOX				= 0,
	VAA_META_TRAJ				= 1,
	VAA_MAX_META
} VAA_META_E;

#if defined(__ITXGUI64)
typedef unsigned long int VAAID;
#else
typedef unsigned int VAAID;
#endif


////////////////////////////////////////////////////////////
//
// public interfaces 
//

int vaa_init();
VAAID vaa_get_vaaid(int ch);
VAAID vaa_get_pb_vaaid(int ch);
int vaa_is_supported();
int vaa_reload();
DITID vaa_get_ditid(VAAID id);
int vaa_get_counted_rule(VAAID id, unsigned short countid);
int	vaa_parse_event(int ch, ivca_rule_event_t *pevt, int *zid, int *cid);
int vaa_activate_all_rule(VAAID id);
int vaa_deactivate_all_rule(VAAID id);
int vaa_activate_calb(VAAID id);
int vaa_deactivate_calb(VAAID id);
int vaa_activate_meta(VAAID id, VAA_META_E meta);
int vaa_deactivate_meta(VAAID id, VAA_META_E meta);
int vaa_blink_pattern(int ch, int patt);
int vaa_notify_vca_event(NF_NOTIFY_INFO *data);
int vaa_notify_vca_track_info(NF_NOTIFY_INFO *data);

#endif
