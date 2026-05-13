#ifndef __LCD200_FB_H__
#define __LCD200_FB_H__

#include <novatek/lcd200_v3/lcd200_api.h>

#define LCD200_IOC_MAGIC  'h'

#define LCD200_IOC_GET_EDID             _IOR(LCD200_IOC_MAGIC, 1, struct lcd200_edid)

#endif

