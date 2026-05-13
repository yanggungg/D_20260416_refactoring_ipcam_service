#ifndef __NF_HI_VO_H__
#define __NF_HI_VO_H__

#include "nf_HI_common.h"

HI_S32 nf_HI_vo_StartDevLayer(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr, HI_U32 u32SrcFrmRate);
HI_S32 nf_HI_VO_HdmiStart(VO_INTF_SYNC_E enIntfSync);
HI_S32 nf_HI_VO_GetWH(VO_INTF_SYNC_E enIntfSync, HI_U32 *pu32W,HI_U32 *pu32H, HI_U32 *pu32Frm);
static HI_VOID nf_HI_VO_HdmiConvertSync(VO_INTF_SYNC_E enIntfSync, HI_HDMI_VIDEO_FMT_E *penVideoFmt);

#endif

