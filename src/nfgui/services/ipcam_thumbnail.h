#ifndef __IPCAM_THUMBNAIL_H__
#define __IPCAM_THUMBNAIL_H__

typedef struct _THUMBNAIL_T CAM_THUMBNAIL_T;
struct _THUMBNAIL_T
{
    gchar *jpec_data;
    size_t jpec_size;
    gchar model[64];
	gchar hostname[256];
	gint port;
	guchar macaddr[8];
	gint status;
};


int iis_thumb_manager_start();
int iis_thumb_manager_stop();

#endif
