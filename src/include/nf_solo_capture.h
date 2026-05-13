#ifndef _NF_SOLO_CAPTURE_H_
#define _NF_SOLO_CAPTURE_H_

/*******************************************************************************
*  (c) COPYRIGHT 2009 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  wiggls                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_capture.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
01/17/2010 0.1   wiggls          Created.

................................................................................

DESCRIPTION:


................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

#include "nf_solo_common.h"

/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/

#define IOCTL_CAP_DEV			"/dev/solo6110_intr0"

#define timersub(a, b, result)	                          \
  do {                                                    \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;         \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;      \
    if ((result)->tv_usec < 0) {                          \
      --(result)->tv_sec;                                 \
      (result)->tv_usec += 1000000;                       \
    }                                                     \
  } while (0)

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

typedef struct _solo_capture
{
	GThread *thread;
	gint	fd; // device fd
} solo_cap_t;

typedef struct {
	int width;
	int height;
#if MEM_NO_COPY
	GstBuffer *gst_icodec_frame_data;
#else
	char *gst_icodec_frame_data;
#endif
	int frame_data_size;
	int display_ch_cnt;
	int display_start_ch;
	
	void *reserve1;
	void *reserve2;

} solo_cap_data_t;

typedef void (*GstNfAppBufferFinalizeFunc) (void *priv);	

int nf_cap_create(void **dec);
int nf_cap_delete(void *dec);
#if MEM_NO_COPY
int nf_cap_process_async(void *dec,                         
                         int out_width, int out_height, 
                         GstBuffer *gst_in_buf, int in_size, int colorspace, 
                         void (*callback) (const char *buf, int width, int height, gpointer data), gpointer data);
#else                   
int nf_cap_process_async(void *dec,                         
                         int out_width, int out_height, 
                         char *gst_in_buf, int in_size, int colorspace, 
                         void (*callback) (const char *buf, int width, int height, gpointer data), gpointer data);
#endif

guint64 get_capture_time(void);

#if SOLO_CAP_CTRL_QUEUE  
void solo_cap_set_qmanager(gboolean flag);
gboolean solo_cap_get_qmanager(void);
#endif

#endif /* _NF_SOLO_CAPTURE_H_ */
