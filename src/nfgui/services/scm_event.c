/*
 * scm_event.c
 * 	- front external event services
 *	- dependency :
 *
 * Written by JungKyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Nov 18, 2019
 *
 */

#include "scm.h"
#include "iux_afx.h"
#include "nf_api_ipcam.h"
#include "scm_internal.h"
#include "ix_mem.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"SCM_EVENT"





////////////////////////////////////////////////////////////
//
// private functions
//

static int _send_to_viewer_analytics_event(int ch, ANALYTICS_ADDITIONAL_EVENT_T *event)
{
	ANALYTICS_ADDITIONAL_EVENT_T *send_data;

	send_data = imalloc(sizeof(ANALYTICS_ADDITIONAL_EVENT_T));
	memcpy(&send_data->timestamp, &event->timestamp, sizeof(GTimeVal));
	strcpy(send_data->caption, event->caption);
	strcpy(send_data->title, event->title);
	strcpy(send_data->description, event->description);
	if (event->jpeg_len) {
		send_data->jpeg_buff = imalloc(event->jpeg_len);
		memcpy(send_data->jpeg_buff, event->jpeg_buff, event->jpeg_len);
		send_data->jpeg_len = event->jpeg_len;
	}

	evt_send_to_local(INFY_AIBOX_ANALYTICS_ADDITIONAL_EVENT, ch, 1, send_data);
	return 0;
}




////////////////////////////////////////////////////////////
//
// protected interfaces
//





////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_detect_analytics_additional_event(int ch, ANALYTICS_ADDITIONAL_EVENT_T *event)
{
	_send_to_viewer_analytics_event(ch, event);
	return 0;
}
