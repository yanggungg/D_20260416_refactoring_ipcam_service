#ifndef _VW_AI_ANALYTICS_ADD_FACE_POPUP_H_
#define _VW_AI_ANALYTICS_ADD_FACE_POPUP_H_

#include "nf_api_dlva.h"

typedef struct _FACE_DEV_DATA_T {
	gint ch;
	guint dvabox_ipaddr;
	gint face_id;
} FACE_DEV_DATA_T;

gint vw_ai_analytics_add_face_popup_open(NFWINDOW *parent, FACE_DEV_DATA_T *data);

#endif
