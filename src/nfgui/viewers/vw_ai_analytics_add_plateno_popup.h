#ifndef _VW_AI_ANALYTICS_ADD_PLATENO_POPUP_H_
#define _VW_AI_ANALYTICS_ADD_PLATENO_POPUP_H_

#include "nf_api_dlva.h"

typedef struct _PLATENO_DEV_DATA_T {
	gint ch;
	guint dvabox_ipaddr;
	gint plateno_id;
} PLATENO_DEV_DATA_T;

gint vw_ai_analytics_add_plateno_popup_open(NFWINDOW *parent, PLATENO_DEV_DATA_T *data);

#endif
