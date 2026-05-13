#ifndef __VW_SYS_LICENSE_H__
#define __VW_SYS_LICENSE_H__

#include "nf_util_netif.h"
#include "scm.h"

typedef struct {
    gchar camtitle[GUI_CHANNEL_CNT][32];
    CAM_PROFILE_T profile[GUI_CHANNEL_CNT];
    SysInfoData sys_info;
    NF_NETIF_GET_INFO netif_info;
}LIC_DEVINFO_T;

void VW_Init_SysLicense_page(NFOBJECT *parent);
gboolean VW_SysLicense_tab_out_handler(void);

#endif
