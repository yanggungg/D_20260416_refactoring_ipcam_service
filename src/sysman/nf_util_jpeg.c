#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <jpeglib.h>

// #include <gst/gst.h>		// for GST_BUFFER;
// #include <gst/gstinfo.h>
#include <gobj.h>
#include <gobjmedia.h>

#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_util_jpeg.h"
#include "nf_api_live.h"
#include "nf_debug.h"


#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_jpeg"

#define DEBUG_JPEG_MAN_LOG
#define	DEBUG_JPEG_MAN_JBSHELL
#define ENABLE_JPEG_MAN_FILE_OUTPUT


static GQuark 
_jpeg_man_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_JPEG_MAN_ERROR 	_jpeg_man_error_quark()

#ifdef DEBUG_JPEG_MAN_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_JPEG_MAN_IDX_E
{ 
	DEBUG_JPEG_MAN_IDX_INIT				= 0,
	DEBUG_JPEG_MAN_IDX_THREAD			= 1,
	DEBUG_JPEG_MAN_IDX_JPEG_SNAPSHOT	= 2,
	DEBUG_JPEG_MAN_IDX_JPEG_ENCODE		= 3,
	
	DEBUG_JPEG_MAN_IDX_DOWN_SCALER		= 4,
	DEBUG_JPEG_MAN_IDX_YUV_CB			= 5,
	DEBUG_JPEG_MAN_IDX_API_FREE			= 6,
	DEBUG_JPEG_MAN_IDX_API_CHECK		= 7,
	
	DEBUG_JPEG_MAN_IDX_API_GET			= 8,
	DEBUG_JPEG_MAN_IDX_API_REQ			= 9,
	DEBUG_JPEG_MAN_IDX_QOUTA			= 10,

	DEBUG_JPEG_MAN_IDX_NR				= 11
}DEBUG_JPEG_MAN_IDX_E;

static const char *_DEBUG_JPEG_MAN_str[32] =
{
	"JPEG_MAN_IDX_INIT",
	"JPEG_MAN_IDX_THREAD",
	"JPEG_MAN_IDX_JPEG_SNAPSHOT",
	"JPEG_MAN_IDX_JPEG_ENCODE",
	
	"JPEG_MAN_IDX_DOWN_SCALER",
	"JPEG_MAN_IDX_YUV_CB",
	"JPEG_MAN_IDX_API_FREE",
	"JPEG_MAN_IDX_API_CHECK",
	
	"JPEG_MAN_IDX_API_GET",
	"JPEG_MAN_IDX_API_REQ",
	"JPEG_MAN_IDX_QOUTA",
				
	"JPEG_MAN_IDX_NR"
};

static gint _DEBUG_JPEG_MAN_log[32] = 
{
	1,1,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 
};


/* Object signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	PROP_0,		
	LAST_PROP	
	/* FILL ME */
};

static void nf_jpeg_man_class_init (NfJpegManClass * klass);
static void nf_jpeg_man_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_jpeg_man_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_jpeg_man_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_jpeg_man_dispose (GObject * object);
static void nf_jpeg_man_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfJpegMan	*_nf_jpeg_man = NULL;

static void _jpeg_man_thread_func (NfJpegMan * self);

#ifdef ENABLE_JPEG_ENCODE_LIB

static gint  _jpeg_man_yuv_capture_cb(gint ch, guint width, guint height, void *buf);
static gboolean _jpeg_lib_down_scaler(char *buf, guint *buf_size, guint *width, guint *height);
static guint _jpeg_lib_encode_yuv_raw( 
		char *src, guint src_size, guint width, guint height, 
		gint jpeg_quality, char *dst );

#endif

GType
nf_jpeg_man_get_type (void)
{
	static GType nf_jpeg_man_type = 0;

	if (G_UNLIKELY (nf_jpeg_man_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfJpegManClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_jpeg_man_class_init,
			NULL,
			NULL,
			sizeof (NfJpegMan),
			0,
			(GInstanceInitFunc) nf_jpeg_man_instance_init,
			NULL
		};

		nf_jpeg_man_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfJpegMan", &object_info, 0);
	}
	
	return nf_jpeg_man_type;
}

static void
nf_jpeg_man_class_init (NfJpegManClass * klass)
{	
	GObjectClass *gobject_class;
	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_jpeg_man_set_property;
	gobject_class->get_property = nf_jpeg_man_get_property;
			
	gobject_class->dispose = nf_jpeg_man_dispose;
	gobject_class->finalize = nf_jpeg_man_finalize;

}

static void
nf_jpeg_man_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfJpegMan *self = NF_JPEG_MAN (instance);
	gint i,j;
				
	self->init_done	= 0;
	
	// queue ����
	self->queue = g_async_queue_new();
 		 
	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)_jpeg_man_thread_func, 
									self, FALSE, NULL);

	memset( self->ch, 0x00, sizeof(self->ch));
	
	for(i=0;i<NUM_CHANNEL; ++i)
	{
		
		for(j=0;j<2; ++j)
			self->ch[i][j].lock = g_mutex_new();
	}
	
	g_get_current_time( &self->time_change_chk_tv );
	
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_jpeg_man_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_jpeg_man_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_jpeg_man_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
 	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_jpeg_man_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
  }
}

/******************************************************************************/

static void _jpeg_man_set_jpeg( NF_JPEG_MAN_JOB *job, 
								NF_JPEG_MAN_CH_INFO *pChJpegInfo, 
								GobjBuddyBuffer *jpeg_gst_buff, 
								guint width, guint height)
{
	int idx = 0;
	char buff[256];
	FILE *fp = NULL;						
	GTimeVal	ctv;
	
	g_mutex_lock( pChJpegInfo->lock );
	{		
		pChJpegInfo->ctime = job->ctime;
		pChJpegInfo->width = width;
		pChJpegInfo->height = height;
		
		if(pChJpegInfo->gst_buff)
			g_object_unref(pChJpegInfo->gst_buff);
		
		pChJpegInfo->gst_buff = jpeg_gst_buff;
		pChJpegInfo->data	= gobj_buddy_buffer_buf_get_addr( jpeg_gst_buff );
		pChJpegInfo->data_size = gobj_buddy_buffer_buf_get_size( jpeg_gst_buff );
		
		pChJpegInfo->output_idx++;
		pChJpegInfo->output_idx %= NF_JPEG_MAN_OUTPUT_MAX;
		idx = pChJpegInfo->output_idx;

#ifdef ENABLE_JPEG_MAN_FILE_OUTPUT	

		if( pChJpegInfo->output[idx][0] && unlink(pChJpegInfo->output[idx]) )
		{
			g_warning("%s unlink failed[%s] [%d](%s)", __FUNCTION__, 
							pChJpegInfo->output[idx], 
							errno, strerror(errno) );
		}
		
		GUINT64_TO_GTIMEVAL( gobj_buddy_buffer_get_timestamp( jpeg_gst_buff ), ctv );		
		pChJpegInfo->ctime = ctv;
		
		snprintf(buff, sizeof(buff)-1, NF_JPEG_MAN_OUTPUT_PATH"/ch%02d_%d_%06d.jpg",
							job->ch, ctv.tv_sec, ctv.tv_usec);
		
#ifdef DEBUG_JPEG_MAN_LOG
		if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_THREAD ] )
		{			
			g_message("%s output[%s] size[%d]",__FUNCTION__, buff,
				gobj_buddy_buffer_buf_get_size( jpeg_gst_buff ));
		}
#endif		
		fp = fopen( buff, "w+");
		if(fp)
		{
			int ret = fwrite( gobj_buddy_buffer_buf_get_addr( jpeg_gst_buff ), 
								gobj_buddy_buffer_buf_get_size( jpeg_gst_buff ), 1, fp);
			if(ret == 1)
				{
				snprintf( pChJpegInfo->output[pChJpegInfo->output_idx],
							sizeof(pChJpegInfo->output[pChJpegInfo->output_idx])-1,
							"%s", buff);			
			}else{
				pChJpegInfo->output[pChJpegInfo->output_idx][0] = '\0';
				g_warning("%s fwrite failed!! [%s]", __FUNCTION__, buff);
			}
			fflush(fp);
			fclose(fp);	
		}else{
			g_warning("%s fopen failed!! [%s]", __FUNCTION__, buff);
		}													
#endif // ENABLE_JPEG_MAN_FILE_OUTPUT	

	}
	g_mutex_unlock( pChJpegInfo->lock );	
}

static void
_jpeg_man_thread_func (NfJpegMan * self)
{	
	int merge_cnt = 0;
	
	g_message("%s start", __FUNCTION__);

    {
        int policy;
        struct sched_param sched;
        pthread_t thread;
        
        policy = SCHED_FIFO;
        thread = pthread_self();
		sched.sched_priority = sched_get_priority_min(policy);
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT;
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT+1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }

	// wait init complete
	while( _nf_jpeg_man == NULL ) g_usleep(10*1000);
		
	self->init_done = 1;
	
	
	while(self->thread_run)
	{
		NF_JPEG_MAN_JOB *job = g_async_queue_pop( self->queue);
		//NF_JPEG_MAN_CH_INFO *pChJpegInfo = &_nf_jpeg_man->ch[ job->ch ];
		GTimeVal	nowtime;
		GTimeVal gettime; //fix get time
				
#ifdef DEBUG_JPEG_MAN_LOG
		if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_THREAD ] )
		{
			g_message("%s type[%d] ch[%d] 0x%08x 0x%08x 0x%08x 0x%08x", __FUNCTION__,
					job->type, job->ch,
					job->d.params[0], job->d.params[1], job->d.params[2], job->d.params[3] );
		}
#endif

		g_get_current_time( &nowtime);

		if( _nf_jpeg_man->time_quata.tv_sec != nowtime.tv_sec )
		{
			_nf_jpeg_man->time_quata = nowtime;
			_nf_jpeg_man->qouta = 0;
		}
		
		++_nf_jpeg_man->qouta;

		{
			NF_JPEG_MAN_JOB *peek_job;
			if( peek_job = g_queue_peek_head( self->queue) )
			{
				if(peek_job->ch == job->ch && merge_cnt < 4)
				{	
					++merge_cnt;
					g_free(job);
					continue;
				}								
				merge_cnt = 0;
			}
		}			
				
		if( job->type == NF_JPEG_MAN_JOB_JPEG_SNAPSHOT ) {

			g_warning("%s skip NF_JPEG_MAN_JOB_JPEG_SNAPSHOT type[%d] ch[%d] 0x%08x 0x%08x 0x%08x 0x%08x", __FUNCTION__,
					job->type, job->ch,
					job->d.params[0], job->d.params[1], job->d.params[2], job->d.params[3] );
			
		}else if( job->type == NF_JPEG_MAN_JOB_YUV_CAPTURE ) {	

			g_warning("%s skip NF_JPEG_MAN_JOB_YUV_CAPTURE type[%d] ch[%d] 0x%08x 0x%08x 0x%08x 0x%08x", __FUNCTION__,
					job->type, job->ch,
					job->d.params[0], job->d.params[1], job->d.params[2], job->d.params[3] );

		}else if( job->type == NF_JPEG_MAN_JOB_NETRA_ENCODER ) {	

			GobjBuddyBuffer *jpeg_gst_buff = NULL;			
			NF_JPEG_MAN_CH_INFO *pChJpegInfo = NULL;
						
			gint	width=0, height=0, size=0, ret=0;
			guint	timestamp = 0;
			gchar	*buf = NULL;
			gint    retry_cnt = 10;

#if 0			
			if( _nf_jpeg_man->qouta > NF_JPEG_MAN_QOUTA ) // qouta failed
			{
#ifdef DEBUG_JPEG_MAN_LOG
				if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_QOUTA] )
				{
					g_warning("%s JOB_YUV qouta failed [%d] ",__FUNCTION__, 
								_nf_jpeg_man->qouta);
				}
#endif		
				goto qouta_fail;				
			}
#endif

            while( retry_cnt-- > 0) {
			ret =nf_live_stream_jpeg( job->ch, 
                                                &width, &height, &size, &buf, 
                                                0, 0, &timestamp,job->size);
				
                if(ret == TRUE) {
                    pChJpegInfo = &_nf_jpeg_man->ch[ job->ch ][job->size];

                    jpeg_gst_buff = gobj_buddy_buffer_new_malloc( size );
                    if(	jpeg_gst_buff == NULL )
                    {
                        g_warning("%s jpeg_gst_buff failed",__FUNCTION__);
                        goto encode_fail;
                    }

                    memcpy( gobj_buddy_buffer_buf_get_addr(jpeg_gst_buff), buf, size);
                    //fix get time
                    g_get_current_time( &gettime);				
                    gobj_buddy_buffer_set_timestamp(jpeg_gst_buff, GTIMEVAL_TO_GUINT64 ( gettime));

                    _jpeg_man_set_jpeg( job, pChJpegInfo, jpeg_gst_buff, width, height);

                    break;
                }
                g_usleep(10*1000);
            }
	
encode_fail:	
			if(buf) {
				free(buf);				
				buf = NULL;
			}
			
		} else{
			g_warning("%s unknown job type[%d]", __FUNCTION__, job->type);			
		}


qouta_fail:			
		g_free(job);				
		g_usleep(10*1000);

	}
	g_message("%s end", __FUNCTION__);
}


static void _time_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	
	NfJpegMan	*jpeg_man = (NfJpegMan	*)data;
	
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_jpeg_man != NULL);

	g_get_current_time( &jpeg_man->time_change_chk_tv );
		
	g_message("%s time_changed", __FUNCTION__);
}


/**
	@brief				jpeg_man �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_jpeg_man_init( int wait )
{
	gboolean ret = TRUE;	
	gulong	cb_handle;
	
	g_return_val_if_fail (_nf_jpeg_man == NULL, FALSE);	
	
	_nf_jpeg_man = g_object_new ( NF_TYPE_JPEG_MAN , NULL);

	nf_debug_category_add( "jpeg", _DEBUG_JPEG_MAN_str, _DEBUG_JPEG_MAN_log, DEBUG_JPEG_MAN_IDX_NR);

#ifdef ENABLE_647DSP_JPEG_ENCODE	
	mkdir(NF_JPEG_MAN_OUTPUT_PATH, 0755);
	g_message("%s mkdir path[%s]", __FUNCTION__, NF_JPEG_MAN_OUTPUT_PATH);
#endif
	
	cb_handle= nf_notify_connect_cb( "time_change", _time_change_cb_func, (gpointer)_nf_jpeg_man);
	g_message("%s time_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
																																	
	if( wait )
	{
		while( _nf_jpeg_man->init_done != 1)
			g_usleep(10*1000);
	}
			
	return ret;
}

void
nf_jpeg_man_free_snapshot( NF_JPEG_MAN_SNAPSHOT *snapshot )
{
	g_return_if_fail(snapshot != NULL );

#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_FREE ] )
	{
		g_message("%s addr[0x%08x] ch[%d] ctime[%d]", __FUNCTION__, 
					snapshot, snapshot->ch, snapshot->ctime.tv_sec );
	}
#endif

	if(snapshot->data)
	{			
		snapshot->data = NULL;
		snapshot->data_size = 0;
				
		g_object_unref( snapshot->reserved );
	}		
	g_free(snapshot);
}


gboolean 
nf_jpeg_man_request_snapshot( gint ch, NF_JPEG_SIZE_E  srcSize, gint msec )
{
	NF_JPEG_MAN_CH_INFO *pChJpegInfo = NULL;
	GTimeVal	nowtv;
	
	g_return_val_if_fail( _nf_jpeg_man != NULL, 0);
	g_return_val_if_fail( ch >=0 && ch < NUM_CHANNEL, 0);
	g_return_val_if_fail( msec >=0 &&  msec<= 60000, 0); // 60�� �̳�

#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_REQ ] )
	{
		g_message("%s ch[%d] gint msec[%d]", __FUNCTION__, ch, msec);
	}
#endif
	
	pChJpegInfo = &_nf_jpeg_man->ch[ch][srcSize];

	g_get_current_time( &nowtv );	
	g_time_val_add( &nowtv , msec*(-1000) );

#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_CHECK ] )
	{
		g_message("%s ch[%d] msec[%d] check[%ld.%06ld] rtime[%ld.%06ld]", __FUNCTION__, 
					ch, msec, nowtv.tv_sec, nowtv.tv_usec,
					pChJpegInfo->rtime.tv_sec, pChJpegInfo->rtime.tv_usec );
	}
#endif

	if( (pChJpegInfo->rtime.tv_sec < nowtv.tv_sec) 
		|| ( pChJpegInfo->rtime.tv_sec == nowtv.tv_sec 
			&& pChJpegInfo->rtime.tv_usec <= nowtv.tv_usec ) 
		|| pChJpegInfo->rtime.tv_sec == 0
		|| pChJpegInfo->ttime.tv_sec != _nf_jpeg_man->time_change_chk_tv.tv_sec )
	{				

		NF_JPEG_MAN_JOB *job = NULL;

		if( g_async_queue_length(  _nf_jpeg_man->queue ) > 32) {
			g_warning("%s que full!! %d", __FUNCTION__, g_async_queue_length(  _nf_jpeg_man->queue ) );
			return 0;
		}
						
		job = g_malloc0( sizeof( NF_JPEG_MAN_JOB));	
		if(!job)
		{
			g_warning("%s job malloc failed",__FUNCTION__);
			return 0;
		}

		job->ch = ch;
		job->type = NF_JPEG_MAN_JOB_NETRA_ENCODER;			
		g_get_current_time( &job->ctime );	
		job->size = srcSize;
		
		g_async_queue_push( _nf_jpeg_man->queue, job);

		g_get_current_time( &pChJpegInfo->rtime);
		pChJpegInfo->ttime = _nf_jpeg_man->time_change_chk_tv;

		++_nf_jpeg_man->qouta;

	}	
	return TRUE;
}


gboolean 
nf_jpeg_man_check_snapshot( gint ch,  NF_JPEG_SIZE_E  srcSize, gint msec )
{
	NF_JPEG_MAN_CH_INFO *pChJpegInfo = NULL;
	GTimeVal	nowtv;
	
	g_return_val_if_fail( _nf_jpeg_man != NULL, 0);
	g_return_val_if_fail( ch >=0 && ch < NUM_CHANNEL, 0);
	g_return_val_if_fail( msec >=0 &&  msec<= 60000, 0); // 60�� �̳�

	pChJpegInfo = &_nf_jpeg_man->ch[ch][srcSize];
#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_CHECK ] )
	{	
		g_return_val_if_fail( pChJpegInfo->data != NULL, 0);
		//g_return_val_if_fail( pChJpegInfo->ttime.tv_sec != _nf_jpeg_man->time_change_chk_tv.tv_sec, 0);
	}	
#endif	

	if( pChJpegInfo->data == NULL)
		return 0;
/*
	if(pChJpegInfo->ttime.tv_sec != _nf_jpeg_man->time_change_chk_tv.tv_sec)
		return 0;
*/
	g_get_current_time( &nowtv );	
	g_time_val_add( &nowtv , msec*(-1000) );

#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_CHECK ] )
	{
		g_message("%s  ch[%d] msec[%d] check[%ld.%06ld] ctime[%ld.%06ld]", __FUNCTION__, 
					ch, msec, nowtv.tv_sec, nowtv.tv_usec,
					pChJpegInfo->ctime.tv_sec, pChJpegInfo->ctime.tv_usec );
	}
#endif

	if( (pChJpegInfo->ctime.tv_sec > nowtv.tv_sec) 
		|| ( pChJpegInfo->ctime.tv_sec == nowtv.tv_sec 
			&& pChJpegInfo->ctime.tv_usec >= nowtv.tv_usec )  )
	{
				
		return 1;
	}
	
	return 0;
}


gboolean 
nf_jpeg_man_get_snapshot( gint ch,  NF_JPEG_SIZE_E  srcSize, NF_JPEG_MAN_SNAPSHOT **snapshot )
{
	NF_JPEG_MAN_CH_INFO *pChJpegInfo = NULL;
	NF_JPEG_MAN_SNAPSHOT  *pSnap = NULL;
	GTimeVal	nowtime;
	
	g_return_val_if_fail( _nf_jpeg_man != NULL, 0);
	g_return_val_if_fail( ch >=0 && ch < NUM_CHANNEL, 0);

#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_GET ] )
	{
	}
#endif
	
	pChJpegInfo = &_nf_jpeg_man->ch[ch][srcSize];

	g_return_val_if_fail( pChJpegInfo->data != NULL, 0); // no_frame

	pSnap = g_malloc0( sizeof(NF_JPEG_MAN_SNAPSHOT) );
	g_return_val_if_fail( pSnap != NULL, 0);
	
	g_mutex_lock ( pChJpegInfo->lock );
	{
		pSnap->ch	 = ch;
		
		pSnap->ctime = pChJpegInfo->ctime;

		pSnap->width = pChJpegInfo->width;
		pSnap->height = pChJpegInfo->height;
		
		pSnap->data =  pChJpegInfo->data;
		pSnap->data_size = pChJpegInfo->data_size;
		
		snprintf( pSnap->output, sizeof(pSnap->output)-1, "%s", 
					pChJpegInfo->output[pChJpegInfo->output_idx] );	
						
		pSnap->reserved = pChJpegInfo->gst_buff;
		
		g_get_current_time( &pChJpegInfo->atime );
		g_object_ref( pChJpegInfo->gst_buff );
				
	}		
	g_mutex_unlock ( pChJpegInfo->lock );


#ifdef DEBUG_JPEG_MAN_LOG
	if( _DEBUG_JPEG_MAN_log[ DEBUG_JPEG_MAN_IDX_API_GET ] )
	{
		g_message("%s  ch[%d] snap[%ld.%06ld] output[%s] size[%d]", __FUNCTION__, 
					ch,	pSnap->ctime.tv_sec, pSnap->ctime.tv_usec, 
					pSnap->output, pSnap->data_size );
	}
#endif
	
	*snapshot = pSnap;	
	
	return TRUE;
}

#ifdef DEBUG_JPEG_MAN_JBSHELL

static char jpeg_man_req_help[] = "jpeg_man_req [ch] [msec:1000]";
static int jpeg_man_req(int argc, char **argv)
{		
	int ch = 0;
	int msec= 1000;
	
	g_return_val_if_fail( _nf_jpeg_man != NULL ,0);
	
	if(argc<2){
		printf("%s\n",jpeg_man_req_help);
		return -1;
	}
	
	ch = strtol(argv[1],NULL,0);	
	if(argc>2) 
		msec = strtol(argv[2],NULL,0);
			 	
	nf_jpeg_man_request_snapshot( ch, 0, msec);

	return 0;
}
__commandlist(jpeg_man_req, "jpeg_man_req",  jpeg_man_req_help, jpeg_man_req_help);


static char jpeg_man_dump_help[] = "jpeg_man_dump ";
static int jpeg_man_dump(int argc, char **argv)
{		
	g_return_val_if_fail( _nf_jpeg_man != NULL ,0);

	return 0;
}
__commandlist(jpeg_man_dump, "jpeg_man_dump",  jpeg_man_dump_help, jpeg_man_dump_help);

#endif



// Callback function
static gint 
_jpeg_man_yuv_capture_cb(gint ch, guint width, guint height, void *buf)
{	// buf --> GST_BUFFER
    
    NF_JPEG_MAN_JOB *job = NULL;        
    
  	g_return_val_if_fail( buf != NULL, 0);
  	
	if(  ch> NUM_CHANNEL || width > 1024 || height > 1024)
	{
		g_warning("%s invalid param ch[%d] width[%d] height[%d]",
					__FUNCTION__, ch, width, height);
		goto err_ret;			
	}
	
	job = g_malloc0( sizeof( NF_JPEG_MAN_JOB));	
	if(!job)
	{
		g_warning("%s job malloc failed",__FUNCTION__);
		goto err_ret;	
	}
	
	job->ch = ch;
	job->type = NF_JPEG_MAN_JOB_YUV_CAPTURE;
	
	g_get_current_time( &job->ctime );
	
	job->p.len = gobj_buddy_buffer_buf_get_size(G_OBJECT(buf));
	job->p.ptr = buf;
	job->p.reserved[0] = width;
	job->p.reserved[1] = height;
	
	g_async_queue_push( _nf_jpeg_man->queue, job);
					  	  	  	
  	return 0;
  	
err_ret:
	
	g_object_unref( buf);
	g_object_unref( buf);
		    
    return -1;
}

#define CHK_JPEG_BOUNDARY 2048
gboolean
nf_jpeg_get_snapshot_size(guchar* image, guint len, gint* width, gint* height)
{
	g_return_val_if_fail(width != NULL, 0);
	g_return_val_if_fail(height != NULL, 0);
	g_return_val_if_fail(image != NULL, 0);
	g_return_val_if_fail(len > 0, 0);

	int i = 0, found = 0, emergency_cnt = 0, chk_len, payload_len;
	chk_len = MIN(CHK_JPEG_BOUNDARY, len - 1);
	*width = 0; *height = 0;

	while(1)
	{
#if 0
		printf("%s debug|payload %dth:\n%02x %02x %02x %02x\n\n", __FUNCTION__, i,
				*(unsigned char*)(image + i),
				*(unsigned char*)(image + i + 1),
				*(unsigned char*)(image + i + 2),
				*(unsigned char*)(image + i + 3)
		);
#endif
		if(*(image + i) == 0xFF)
		{
			switch(*(image + i + 1))
			{
				case 0xD8:	// Start of Image
					i += 2;
					break;
				case 0xC0:	// Start of Frame(size)
					*width = ntohs(*(unsigned short *)((unsigned char *)image + i + 7));
					*height = ntohs(*(unsigned short *)((unsigned char *)image + i + 5));
					found = 1;
					break;
				case 0xDA:	// Start of Scan
				case 0xD9:	// End of Image
					/* cannot found 0xffc0.. exit */
					emergency_cnt = 99;
					break;
				case 0xDD:	// Define Restart Interval - doesn't have payload
					i += 6;
					break;
				default:
					payload_len = ntohs(*(unsigned short *)((unsigned char *)image + i + 2));
					i += 2 + payload_len;
					break;
			}
		}
		else
		{
			/* cannot found 0xff.. it's bug */
			break;
		}

		if(i > chk_len || found || emergency_cnt > 10)
		{
			break;
		}
		/* escape routine */
		emergency_cnt++;
	}

	//g_message("%s | rtn : %d , jpeg size %d x %d\n", __FUNCTION__, found, *width, *height);

	return found;
}

/******************************************************************************/
#ifdef ENABLE_JPEG_ENCODE_LIB

#define NF_JPEG_MAN_OUTPUT_BUF_SIZE 4096

typedef struct _NF_JPEG_MAN_DESTMGR_T {
  struct jpeg_destination_mgr pub; /* public fields */

  JOCTET 			*buffer;    /* start of buffer */

  guchar 			*outbuffer;
  gint 				outbuffer_size;
  guchar 			*outbuffer_cursor;
  gint 				*written;
} NF_JPEG_MAN_DESTMGR;


METHODDEF(void) _jpeg_lib_init_destination(j_compress_ptr cinfo) 
{
  NF_JPEG_MAN_DESTMGR * dest = (NF_JPEG_MAN_DESTMGR*) cinfo->dest;

  /* Allocate the output buffer --- it will be released when done with
   * image */
  dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) 
  					((j_common_ptr) cinfo, JPOOL_IMAGE, 
  					NF_JPEG_MAN_OUTPUT_BUF_SIZE * sizeof(JOCTET));

  *(dest->written) = 0;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = NF_JPEG_MAN_OUTPUT_BUF_SIZE;
}

/******************************************************************************
Description.: called whenever local jpeg buffer fills up
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(boolean) _jpeg_lib_empty_output_buffer(j_compress_ptr cinfo) 
{
  NF_JPEG_MAN_DESTMGR *dest = (NF_JPEG_MAN_DESTMGR *) cinfo->dest;

  memcpy(dest->outbuffer_cursor, dest->buffer, NF_JPEG_MAN_OUTPUT_BUF_SIZE);
  dest->outbuffer_cursor += NF_JPEG_MAN_OUTPUT_BUF_SIZE;
  *(dest->written) += NF_JPEG_MAN_OUTPUT_BUF_SIZE;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = NF_JPEG_MAN_OUTPUT_BUF_SIZE;

  return TRUE;
}

METHODDEF(void) _jpeg_lib_term_destination(j_compress_ptr cinfo) 
{
  NF_JPEG_MAN_DESTMGR * dest = (NF_JPEG_MAN_DESTMGR *) cinfo->dest;
  size_t datacount = NF_JPEG_MAN_OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  /* Write any data remaining in the buffer */
  memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
  dest->outbuffer_cursor += datacount;
  *(dest->written) += datacount;
}

static void _jpeg_lib_init_dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, gint *written) 
{
  NF_JPEG_MAN_DESTMGR * dest;

  if (cinfo->dest == NULL) {
    cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small) 
    				((j_common_ptr) cinfo, JPOOL_PERMANENT, 
			    	sizeof(NF_JPEG_MAN_DESTMGR));
  }

  dest = (NF_JPEG_MAN_DESTMGR*) cinfo->dest;
  dest->pub.init_destination 	= _jpeg_lib_init_destination;
  dest->pub.empty_output_buffer = _jpeg_lib_empty_output_buffer;
  dest->pub.term_destination 	= _jpeg_lib_term_destination;
  dest->outbuffer = buffer;
  dest->outbuffer_size = size;
  dest->outbuffer_cursor = buffer;
  dest->written = written;
}


/******************************************************************************/
static guint _jpeg_lib_encode_yuv_raw( 
		char *src, guint src_size, guint width, guint height, 
		gint jpeg_quality, char *dst ) 
{
    
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    NF_JPEG_MAN_DESTMGR dest;

    JSAMPROW y[16],cb[16],cr[16];
    JSAMPARRAY data[3];	//unsigned char * 

    gchar *buffcb, *buffcr;
    gchar *buffcb_tmp, *buffcr_tmp;
    gchar *src_tmp;
    
    guint written, size;                /* for count file size */
    guint i, line;    	

	g_return_val_if_fail( src != NULL, 0);	
	g_return_val_if_fail( dst != NULL, 0);
			
	size = width * height;
	
    buffcb = g_malloc( size / 4);    
    buffcr = g_malloc( size / 4);    

	if ( buffcb == NULL || buffcr == NULL)
	{
		if (buffcb != NULL)
			g_free(buffcb);
		if (buffcr != NULL)
			g_free(buffcr);
			
		g_return_val_if_fail( buffcb != NULL && buffcb != NULL , 0);
	}

	buffcb_tmp = buffcb;
	buffcr_tmp = buffcr;
	src_tmp = (char*)src + width*height;
	
    for ( i = 0; i < size/4; i++)
    {
		*buffcb_tmp =  *src_tmp; ++buffcb_tmp; ++src_tmp;
		*buffcr_tmp =  *src_tmp; ++buffcr_tmp; ++src_tmp;
    }

    memcpy(src + size , buffcb, size /4);
    memcpy(src + size  + size /4, buffcr, size/4);
    
    g_free(buffcb); g_free(buffcr);
    
#ifdef __DEBUG__WEB_JPEG    		
    g_message("Encoding a %dx%d frame\n", width, height);
#endif

	memset(&cinfo , 0, sizeof(cinfo));

    data[0] = y;
    data[1] = cb;
    data[2] = cr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);   
	 
    //init JPEG dest mgr
    _jpeg_lib_init_dest_buffer(&cinfo, dst, src_size, &written);

    //jpeg_set_quality(&cinfo, 70, TRUE);
    jpeg_set_quality(&cinfo, jpeg_quality, TRUE);

    cinfo.input_components = 3;
    cinfo.image_width = width;
    cinfo.image_height = height;

    cinfo.in_color_space = JCS_YCbCr;
    jpeg_set_defaults(&cinfo);
    jpeg_set_colorspace(&cinfo, JCS_YCbCr);
    
    cinfo.raw_data_in = TRUE; // supply downsampled data

    /* set_defaults will set YUV 420 format */
    cinfo.comp_info[0].h_samp_factor = 2; 
    cinfo.comp_info[0].v_samp_factor = 2;
    cinfo.comp_info[1].h_samp_factor = 1; 
    cinfo.comp_info[1].v_samp_factor = 1; 
    cinfo.comp_info[2].h_samp_factor = 1; 
    cinfo.comp_info[2].v_samp_factor = 1;
    cinfo.dct_method = JDCT_IFAST;
    //cinfo.dct_method = JDCT_FLOAT;
    
    jpeg_start_compress( &cinfo, TRUE );

    for (line=0; line<height; line+=16) {
    	
        for (i=0; i<16; i++) {
            y[i] = src + width * (i+line);
            if ( !(i&0x1) ) {
                cb[i/2] = src + size + width/2*((i+line)/2);
                cr[i/2] = src + size + size /4 + width/2*((i+line)/2);
          }
        }
        jpeg_write_raw_data(&cinfo, data, 16);
        g_usleep(10*1000);
    }
    
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);   
	
    return written;
}


static gboolean 
_jpeg_lib_down_scaler(char *buf, guint *buf_size, guint *width, guint *height)
{
    
    guint src_width, src_height, src_size;
    guint dst_width, dst_height, dst_size, dst_outsize;
    
    g_return_val_if_fail( buf != NULL, 0);
    g_return_val_if_fail( buf_size != NULL, 0);
    g_return_val_if_fail( width != NULL, 0);
    g_return_val_if_fail( height != NULL, 0);

	dst_width	= src_width = *width;
	dst_height	= src_height = *height;		
	dst_size 	= src_size	= (src_width * src_height);
    dst_outsize = dst_width * dst_height * 3 / 2;
    
	if( src_width != 352 ) // D1,2CIF -> CIF 
	{		

		int x,y;

		dst_width = src_width/2;
				
		if( src_height == 480 || src_height == 576 ) // 240, 288
			dst_height /= 2;
						        
        dst_size	= dst_width * dst_height;
		dst_outsize = dst_width * dst_height * 3 / 2;
        
		for(y=0; y< dst_height; ++y) // Y
		{
			unsigned char *src = &buf[ src_width * y];
			unsigned char *dest = &buf[ dst_width * y ];

			for(x=0; x< dst_width;++x)
			{
				dest[x] = ( src[0] + src[1] )/2;
				++src;++src; 
			}
		}
		
        {	// CbCr
    		unsigned short *src =  &buf[ src_size ];
    		unsigned short *dest =  &buf[ dst_size ];
    		
    		for(x=0; x< dst_size/4 ; ++x )
    		{
    			dest[x] = *src;
    			++src; ++src;
    		}
        }
        
        *buf_size	= dst_outsize;
        *width		= dst_width;
        *height		= dst_height;
        
	}
	
	return 1;
	
}

#endif  // ENABLE_JPEG_ENCODER_LIB

/******************************************************************************/
