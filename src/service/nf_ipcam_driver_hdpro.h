/*
 * ITXM2M
 * ONVIF HDPRO API
 * 2017-09-07
 */

#ifndef NF_IPCAM_DRIVER_HDPRO_H
#define NF_IPCAM_DRIVER_HDPRO_H

/* ================================================================================ */
// Include

#include "nf_ipcam_defs.h"

/* ================================================================================ */
// Define

#define HDPRO_DEBUG				(0)

#define HDPRO_PMASK_AREA_MAX	(8)
#define HDPRO_PMASK_CELL_NCOLS	(15)
#define HDPRO_PMASK_CELL_NROWS	(9)

#define HDPRO_PRINT(fmt,...) \
{ \
	if(1) \
	{ \
		fprintf(stdout, "%s(%d) : " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		fflush(stdout); \
	} \
} \

/* ================================================================================ */
// Enum



/* ================================================================================ */
// Struct



/* ================================================================================ */
// Extern Function Proto

int nf_hdpro_set_mirror(cam_info* info, int ch);
int nf_hdpro_set_privacy_mask(NFIPCamPrivacyMask *pmask_info, int ch);
int nf_hdpro_set_onepush(int ch);
int nf_hdpro_enable_mirror(int ch);
int nf_hdpro_enable_onepush(int ch);

#endif

