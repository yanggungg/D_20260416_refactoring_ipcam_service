#ifndef __NF_UTIL_JPEG_H__
#define __NF_UTIL_JPEG_H__

#include "nf_object.h"
#include "nf_common.h"

/* type macro */
#define NF_TYPE_JPEG_MAN					(nf_jpeg_man_get_type ())

#define NF_IS_JPEG_MAN(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_JPEG_MAN))
#define NF_IS_JPEG_MAN_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_JPEG_MAN))

#define NF_JPEG_MAN_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_JPEG_MAN, NfJpegManClass))
#define NF_JPEG_MAN(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_JPEG_MAN, NfJpegMan))
#define NF_JPEG_MAN_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_JPEG_MAN, NfJpegManClass))

#define NF_JPEG_MAN_CAST(obj)			((NfJpegMan*)(obj))
#define NF_JPEG_MAN_CLASS_CAST(klass)	((NfJpegManClass*)(klass))

typedef struct _NfJpegMan 		NfJpegMan;
typedef struct _NfJpegManClass 	NfJpegManClass;


typedef enum _NF_JPEG_MAN_JOB_TYPE_E {

	NF_JPEG_MAN_JOB_JPEG_SNAPSHOT = 0,	// for NF
	NF_JPEG_MAN_JOB_YUV_CAPTURE = 1,
	NF_JPEG_MAN_JOB_SOLO_ENCODER = 2,
	NF_JPEG_MAN_JOB_NETRA_ENCODER = 3,
	NF_JPEG_MAN_JOB_NR
	
} NF_JPEG_MAN_JOB_TYPE_E;


typedef struct NF_JPEG_MAN_JOB_T {

	NF_JPEG_MAN_JOB_TYPE_E	type;

	gint		ch;
	GTimeVal	ctime;
	gint size;
	
	union {
		struct    {   guint       params[4];
					  guint	      reserved; } d;
		struct    {   guint       len;
						gpointer    ptr;
						guint       reserved[2]; } p; 
	};
} NF_JPEG_MAN_JOB;

#define NF_JPEG_MAN_QOUTA			5
#define NF_JPEG_MAN_OUTPUT_MAX		3
#define NF_JPEG_MAN_OUTPUT_PATH		"/tmp/webra-image"

typedef struct NF_JPEG_MAN_CH_INFO_T {

	GMutex		*lock;
			
	GTimeVal	rtime;		// reqest(update cache) time
	GTimeVal	atime;		// access time
	GTimeVal	ctime;		// create time

	GTimeVal	ttime;		// time change monitor

	guint		width;
	guint		height;
		
	gpointer	data;
	guint		data_size;
	gpointer	gst_buff;
			
	guint		output_idx;		// current_idx								
	gchar		output[NF_JPEG_MAN_OUTPUT_MAX][256];	// output file ( triple buffer)
	
} NF_JPEG_MAN_CH_INFO;


/**
 * NfJpegMan:
 *
 * NfDVR jpeg_man class
 */
struct _NfJpegMan {
	NfObject 	 	object;
	
	/*< public >*/	
	gint			init_done;
	
	GAsyncQueue		*queue;	
	
	GThread			*thread;	
	gint			thread_run;
	gint			thread_status;
	
	NF_JPEG_MAN_CH_INFO		ch[NUM_CHANNEL][2];
	
	GTimeVal		time_change_chk_tv;
	GTimeVal		time_quata;
	guint			qouta;
	/*< public >*/ /* with LOCK */

	/*< private >*/	
};

struct _NfJpegManClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};


typedef struct _NF_JPEG_MAN_SNAPSHOT_T {

	guint		ch;
		
	GTimeVal	ctime;		// create time

	guint		width;
	guint		height;
			
	gpointer	data;
	guint		data_size;
	
	gchar		output[256];	// output file
	
	gpointer	reserved;
	
} NF_JPEG_MAN_SNAPSHOT;


gboolean 
nf_jpeg_man_init( int wait );

void
nf_jpeg_man_free_snapshot( NF_JPEG_MAN_SNAPSHOT *snapshot );

gboolean 
nf_jpeg_man_request_snapshot( gint ch, NF_JPEG_SIZE_E  srcSize, gint msec );

gboolean 
nf_jpeg_man_check_snapshot( gint ch, NF_JPEG_SIZE_E  srcSize, gint msec );

gboolean 
nf_jpeg_man_get_snapshot( gint ch, NF_JPEG_SIZE_E  srcSize, NF_JPEG_MAN_SNAPSHOT **snapshot );

gboolean
nf_jpeg_get_snapshot_size(guchar* image, guint len, gint* width, gint* height);

#endif
