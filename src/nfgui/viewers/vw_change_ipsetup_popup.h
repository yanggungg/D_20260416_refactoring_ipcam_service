#ifndef _VW_CHANGE_IPSETUP_POPUP_H_
#define _VW_CHANGE_IPSETUP_POPUP_H_

#include "nf_ui_tool.h"

typedef enum _IPCHANGE_MASK_E {
	MSK_DHCP				= 0,
	MSK_IPADDR				= 1,
	MSK_SUBNET				= 2,
	MSK_GATEWAY				= 3,
	MSK_DNS1				= 4,
	MSK_DNS2				= 5,
	MSK_RTSP_PORT			= 6,
	MSK_WEB_PORT			= 7,
	IPSET_MSK_MAX
} IPCHANGE_MASK_E;

typedef struct _IP_SET_T {
	guint	dhcp;
	guint	ip[4];
	guint	subnet[4];
	guint	gateway[4];
	guint	dns1[4];
	guint	dns2[4];
	guint	rtspPort;
	guint	webPort;
} IP_SET_T;

mb_type VW_Open_IPSet_Popup(NFWINDOW *parent, gchar *title, IP_SET_T *ip_set, IPCHANGE_MASK_E change_msk);

#endif
