#ifndef _NF_SOLO_NET_H_
#define _NF_SOLO_NET_H_

/*******************************************************************************
*  (c) COPYRIGHT 2010 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  tikim                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_net.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
01/31/2010 1.0   tikim          Created.

................................................................................

DESCRIPTION:

  This module contains dual stream part for SOLO6110.

................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

#include "nf_solo_common.h"
#include "nf_record.h"
#include "nf_notify.h"

/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/

#define SYSDB_NET_STREAM_AUDIO		"%s.netstream.audio"
#define SYSDB_NET_STREAM_FPS		"%s.netstream.fps"
#define SYSDB_NET_STREAM_QUALITY	"%s.netstream.quality"
#define SYSDB_NET_STREAM_SIZE		"%s.netstream.size"

#define SOLO_REC_REF_SYSDB_CHANGED_CB_FUNC	( 0 )
#define SOLO_REC_REF_STATUS_NOTIFY_CB_FUNC	( 1 )

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

typedef enum _solo_net_status_e
{
	SOLO_NET_STATUS_STOP = 0,
	SOLO_NET_STATUS_DISCONNECT,
	SOLO_NET_STATUS_CONNECT,
	SOLO_NET_STATUS_FPS_REDUCE,
} SOLO_NET_STATUS_E;

typedef struct _solo_net_t
{
	NfObject 	 			object;
	gint					init_done;
	GAsyncQueue				*queue;		
	GThread					*thread;	
	gint					thread_run;
	gint					thread_status;
	gint					network_start;
	gint					req_network;
	SOLO_NET_STATUS_E		status;
	NF_RECORD_HANDOFF_FUNC	handoff_func;
	guint					handoff_ch_mask;
	NF_RECORD_NOTIFY_DATA	notify_data;
	NF_RECORD_MAN			man[NETWORK_CH_NUM];	
} SOLO_NET_T;

/** ********************************************************************* **
 ** function protoypes
 ** ********************************************************************* **/

SOLO_NET_T **solo_net_get_address(void);
SOLO_NET_T *solo_net_get_struct(void);

gboolean solo_net_create(SOLO_NET_T **p);
gboolean solo_net_init(SOLO_NET_T **p);

gboolean solo_net_send_cmd(guchar cmd);
gboolean solo_net_send_frame(gpointer data);
gboolean solo_net_onoff(gint ch, gboolean onoff);
gboolean solo_net_register_handoff(guint ch_mask, NF_RECORD_HANDOFF_FUNC handoff_func);
gboolean solo_net_cmp_cfg(SOLO_NET_T *t, guint *p);
gboolean solo_net_sdp_start(SOLO_NET_T *t);
gboolean solo_net_request_start(void);
gboolean solo_net_request_stop(void);
void solo_net_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
void solo_net_status_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

#endif /* _NF_SOLO_NET_H_ */






