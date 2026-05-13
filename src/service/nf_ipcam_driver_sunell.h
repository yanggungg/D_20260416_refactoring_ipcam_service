/*
 * ITXM2M
 * SUNELL HTTP API
 * 2017-10-26
 */

#ifndef NF_IPCAM_DRIVER_SUNELL_H
#define NF_IPCAM_DRIVER_SUNELL_H

/* ================================================================================ */
// Include

#include "nf_ipcam_defs.h"

/* ================================================================================ */
// Define

#define SUNELL_DEBUG	(1)

#define SUNELL_PRINT(fmt,...) \
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

int nf_sunell_get_mount(int ch, NF_IPCAM_MOUNT_TYPES_E *mount);
int nf_sunell_set_mount(int ch, NF_IPCAM_MOUNT_TYPES_E mount);

int nf_sunell_get_dewarp(int ch, NF_IPCAM_DEWARP_MODES_E *dewarp);
int nf_sunell_set_dewarp(int ch, NF_IPCAM_DEWARP_MODES_E dewarp);

int nf_sunell_get_ePTZ_layout(int ch, NFIPCamEPTZLayout *layout);
int nf_sunell_move_ePTZ(int ch, ptz_info_onvif *ptz);
int nf_sunell_stop_ePTZ(int ch, ptz_info_onvif *ptz);

int nf_sunell_enable_mount(int ch);
int nf_sunell_enable_dewarp(int ch);
int nf_sunell_enable_ePTZ(int ch);

#endif

