#ifndef __LCD300_FB_H__
#define __LCD300_FB_H__

#include <novatek/lcd300/lcd300_api.h>

#define MAX_EDID_NU 256
typedef struct edid_info {
	unsigned int is_valid[MAX_EDID_NU];
	unsigned int w[MAX_EDID_NU];
	unsigned int h[MAX_EDID_NU];
	unsigned char refresh_rate[MAX_EDID_NU];
	unsigned char is_progress[MAX_EDID_NU];
	unsigned int aspect_rate[MAX_EDID_NU];
	unsigned int    bEdidValid;
	unsigned int u32Edidlength;
	unsigned char u8Edid[512];
} edid_info_t;

#define LCD300_IOC_MAGIC						'g'

#define LCD300_IOC_GET_EDID           			_IOR(LCD300_IOC_MAGIC, 1, struct lcd300_edid)

#define LCD300_IOC_GET_DEVICE_CAPABILITY		_IOR(LCD300_IOC_MAGIC, 49, struct edid_info)

#endif

