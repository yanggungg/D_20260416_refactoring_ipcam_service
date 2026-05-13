/* set tabspace4 */
/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION:

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
02/14/2011 DongUk Park    Created. (Based on nmf 2.0 api )
02/18/2011 DongUk Park    Unit test done at IPX Board and PC
*/
#ifndef __NF_API_PLAY_H__
#define __NF_API_PLAY_H__
#include "nf_common.h"

#define ENABLE_PLAY_COVERT

#define MAX_LOCAL_PLAY_CHAN   32

typedef enum _NF_PLAY_PARAM_MODE_E {
	NF_PLAY_PARAM_MODE_START			= 0,	//STOP_MODE to START_MODE transition avaliable
	NF_PLAY_PARAM_MODE_PAUSE			= 1,	//STOP_MODE to PAUSE_MODE transition avaliable
	NF_PLAY_PARAM_MODE_NEXT_FRAME		= 2,    //PAUSE_MODE to NEXT_MODE transition avaliable 
	NF_PLAY_PARAM_MODE_STOP				= 3
} NF_PLAY_PARAM_MODE_E;


typedef enum _NF_PLAY_PARAM_RATE_E {
	NF_PLAY_PARAM_RATE_001				= 1,
	NF_PLAY_PARAM_RATE_002				= 2,
	NF_PLAY_PARAM_RATE_004				= 4,
	NF_PLAY_PARAM_RATE_008				= 8,
	NF_PLAY_PARAM_RATE_016				= 16,
	NF_PLAY_PARAM_RATE_032				= 32,
	NF_PLAY_PARAM_RATE_064				= 64,
	NF_PLAY_PARAM_RATE_128				= 128
} NF_PLAY_PARAM_RATE_E;                                     

typedef enum _NF_PLAY_PARAM_DIR_E {
	NF_PLAY_PARAM_DIR_FORWARD			= 0,
	NF_PLAY_PARAM_DIR_BACKWARD			= 1
} NF_PLAY_PARAM_DIR_E;                                     

typedef enum NF_PLAY_PARAM_SPEED_E {
	NF_PLAY_PARAM_SPEED_NORMAL			= 0,
	NF_PLAY_PARAM_SPEED_SLOW			= 1
} NF_PLAY_PARAM_SPEED_E;                                     

typedef enum NF_PLAY_STATUS_E {
    NF_PLAY_STATUS_NONE            = 0,
	NF_PLAY_STATUS_NORECORD	    = 1,
	NF_PLAY_STATUS_ENDVIDEO		= 2,
	NF_PLAY_STATUS_OVERLAPPED		= 3
} NF_PLAY_STATUS_E;                                     

typedef enum NF_PLAY_PARAM_PANORAMA_E { //XXX CHECK GST_SST_SRC Panorama mode
    NF_PLAY_PARAM_PANORAMA_NO,
    NF_PLAY_PARAM_PANORAMA_1,               //frame by frame playback
    NF_PLAY_PARAM_PANORAMA_2,               //multi hour plack
} NF_PLAY_PARAM_PANORAMA_E;

typedef enum _NF_PLAY_PARAM_RATIO_E {
	NF_PLAY_RATIO_4_BY_3   = 0,
	NF_PLAY_RATIO_16_BY_9  = 1,	
}NF_PLAY_PARAM_RATIO_E;

typedef struct _NF_PLAY_STATUS_T {
    GTimeVal            play_time;
    NF_PLAY_STATUS_E    play_status[MAX_LOCAL_PLAY_CHAN];
} NF_PLAY_STATUS __attribute__ ((aligned(4))); 


typedef struct _NF_PLAY_PARAM_T {	
	GTimeVal  		start_time;		// play start time
	GTimeVal 		search_time;		// play search time
	GTimeVal 		end_time;		// play end time	
	gint   		    interval;		// ÆA³e¶o¸¶ ¼­A¡ ?I ¶§
	
    NF_DISPLAY_E 	disp_mode;
	gint 			vr_index[MAX_LOCAL_PLAY_CHAN];	//video channel array (value: vr num)
	guint			vr_num;
	int 			audio_in_video_chan;
	guint			win_xpos;
	guint			win_ypos;
	guint			win_width;
	guint			win_height;
	
    NF_PLAY_PARAM_PANORAMA_E    panorama_mode;    
	NF_PLAY_PARAM_MODE_E	play_mode;	
	NF_PLAY_PARAM_RATE_E	play_rate;
	NF_PLAY_PARAM_DIR_E		direction;
	NF_PLAY_PARAM_SPEED_E	speed_flag;

#if defined (ENABLE_PLAY_COVERT)
	gboolean 	vr_covert[MAX_LOCAL_PLAY_CHAN];	//video channel array (value: vr num)
#endif
	gboolean 	preview_flag;
	
} NF_PLAY_PARAM __attribute__ ((aligned(4))); 


gboolean nf_play_init();

gboolean nf_play_start( gpointer *handle, NF_PLAY_PARAM *param, GError **error );

gboolean nf_play_change( gpointer handle, NF_PLAY_PARAM *param, GError **error );

gboolean nf_play_get_status( gpointer handle, NF_PLAY_STATUS *status, GError **error );
gboolean nf_play_stop( gpointer handle );

#ifdef	SUPPORT_VCA_CAMERA

typedef enum _NF_PLAY_SMART_SEARCH_MODE {
	NF_PLAY_SMART_SEARCH_META			= 0,	  
	NF_PLAY_SMART_SEARCH_PREVIEW		= 1,	  
	NF_PLAY_SMART_SEARCH_META_VIEW		= 2
} NF_PLAY_SMART_SEARCH_MODE;

/* Smart search functions. */
gboolean nf_play_set_smart_geometry(gint ch, gint x, gint y, gint width, gint height);
gboolean nf_play_unset_smart_geometry(void);
gboolean nf_play_smart_start(gpointer *handle, NF_PLAY_PARAM *param,
		NF_PLAY_SMART_SEARCH_MODE search_mode ,GError *error);
gboolean nf_play_smart_pause(gpointer handle, NF_PLAY_SMART_SEARCH_MODE search_mode, gboolean pause);
gboolean nf_play_smart_stop(gpointer handle, NF_PLAY_SMART_SEARCH_MODE search_mode);
gboolean nf_play_smart_get_status(gpointer handle, NF_PLAY_STATUS *status,
		GError *error);
gboolean nf_play_smart_set_fgdebug(gboolean enable);
#endif	/* SUPPORT_VCA_CAMERA */


// Get TimeLine
typedef struct _NF_TIMELINE_PARAM_T {
	GTimeVal	time_begin;
	
	guint		resolution;	// Resolution in seconds, 1 day = 86400, 1hour = 3600, ...
	gushort		count;		// Requested time slice count per channel.	
	guchar		max_channel;
	guchar		split_channel;	// if 0 usCount*sizeof(NF_TIMELINE_ELEM_T)
								// if 1 usCount*sizeof(NF_TIMELINE_ELEM_T)*ucMaxChannel
	guchar		hide;
	guchar		reserved[3];	
	guint64		channel_mask;	// Bitwise channel mask.
	
} NF_TIMELINE_PARAM __attribute__ ((aligned(4))) ;


gboolean nf_play_get_thumbnail_jpeg( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint *width, gint *height, 
                                gint *size, 
                                void** out_buffer, 
                                GTimeVal *out_buffer_time,
                                NF_JPEG_SIZE_E srcSize
                                );

gboolean nf_play_get_thumbnail_bmp( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint *width, gint *height, 
                                gint *size, 
                                void** out_buffer, 
                                GTimeVal *out_buffer_time
                                );

/* Netra base IPX Specific Function */
gboolean nf_play_set_thumbnail_geometry( 
                                guint base_x, 
                                guint base_y,
                                guint width, guint height
                                );
gboolean nf_play_get_thumbnail( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint width, gint height, 
                                gint bits_per_pixel,
                                gpointer out_buffer, 
                                GTimeVal *out_buffer_time
                                );
gboolean nf_play_get_thumbnail_v2( gint ch, 
								GTimeVal begin_time, GTimeVal end_time, 
								gint *width, gint *height, 
								gint bits_per_pixel, 
								gpointer out_buffer,
								GTimeVal *out_buffer_time
								);
	
gboolean nf_timeline_get( NF_TIMELINE_PARAM *param, gchar **elem, GError **error);

gboolean nf_play_get_jpeg_snapshot(gint ch, 
                                gint *width, gint *height, 
                                gint *size,
                                void **out_buffer, 
                                gint timeimg,
                                gint dst,
                                guint *timestamp
                               );

// written by SKSHIN
gboolean nf_play_zoom_start(int ch, int base_x, int base_y, 
        int zoom_w, int zoom_h, int pip_x, int pip_y, int pip_w, int pip_h);
gboolean nf_play_zoom_stop( void );
gboolean nf_play_zoom_move(gint xpos, gint ypos, gint zoom_w, gint zoom_h) ;
int nf_play_zoom_get_pos_sx();
int nf_play_zoom_get_pos_sy();
int nf_play_zoom_get_pos_ex();
int nf_play_zoom_get_pos_ey();
int nf_play_zoom_get_pos_dx();
int nf_play_zoom_get_pos_dy();

int nf_play_pip_hide(int ch);
int nf_play_pip_show(int ch);
int nf_play_set_disp_ratio(NF_PLAY_PARAM_RATIO_E ratio);

#define MAX_SST_PB_CHANNEL (64)

#define SST_SECOND		(1000000*1000)
#define SST_MSECOND	(1000*1000)
#define SST_USECOND	(1000)
#define SST_TIMEVAL_TO_TIME(tv)	((tv).tv_sec * SST_SECOND + (tv).tv_usec * SST_USECOND)
#define SST_TIME_TO_TIMEVAL(t,tv) 	 \
		(tv).tv_sec  = (glong) (((guint64) (t)) / SST_SECOND);   \
  		(tv).tv_usec = (glong) ((((guint64) (t)) -               \
                  ((guint64) (tv).tv_sec) * SST_SECOND)    \
                 / SST_USECOND);   

typedef enum _PLAYBACK_THREAD_STATUS {
	PLAYBACK_START, 
	PLAYBACK_STOP,
	PLAYBACK_PAUSE,
	PLAYBACK_CONTINUE,
	PLAYBACK_NEXT
}PLAYBACK_THREAD_STATUS;

enum _PLAYBACK_THREAD_DIR {
    SST_SRC_DIR_FORWARD = 0,
    SST_SRC_DIR_BACKWARD = 1
};

enum _PLAYBACK_THREAD_MODE {
    SST_SRC_MODE_NORMAL = 0,
    SST_SRC_MODE_PANORAMA1 = 1,
    SST_SRC_MODE_PANORAMA2 = 2
};


#define SST_AUDIO_CHANNEL_BASE  (64)
#define SST_SRC_AUDIO_PREROLL_TIME (0)
#define SST_SRC_AUDIO_DURATION   (SST_SECOND)

struct playback_data {
	GMutex *state_lock;
	gchar *filename;			/* filename */
	gint file_num;
	//FILE *fd[MAX_SST_PB_CHANNEL];
	gint sid[MAX_SST_PB_CHANNEL];					/* sst id */
	gint drop_cnt[MAX_SST_PB_CHANNEL];
	guint64 time_offset;		/* stream base_time */
	guint64 running_time;	/* stream running_time */
	gint pr_id;
	gint mode;                  //GstNfSstMode
	guint64 ch_mask;
	guint64 eos_mask;
	guint64 open_mask;
	guint64 norecord_mask;
	guint64 overlapped_mask;

	guint64 prev_eos_mask;
	guint64 prev_norecord_mask;
	guint64 prev_overlapped_mask;

	gint ch[MAX_SST_PB_CHANNEL];
	gint disp_ch[MAX_SST_PB_CHANNEL];
	guint64 prev_ri_time[MAX_SST_PB_CHANNEL];
	guint64 cur_frame_time[MAX_SST_PB_CHANNEL];
	gint audio_ch;              //-1: no audio
	gint ch_num;
	guint srm_offset;
	guint srm_limit;
	gint rate;
	gint direction;
	gint rate_control;
	guint64 vtick_interval_usec;
	guint64 norecord_interval_ms;
	gdouble gst_rate;
	gboolean hide;
	gboolean isAfterSeek;
	gboolean is_prev_start;
	gboolean is_first_frame;
	gboolean is_start_overlapped;
	gboolean is_start_endvideo;
	gboolean is_continue;
	gboolean is_slow;
	gboolean is_iframe_open;    
	guint display_interval;
	guint64 base_time;    /* stream begin_time */
	guint64 begin_time;    /* stream begin_time */
	guint64 end_time;      /* stream end_time */
	guint64 search_time;   /* stream search_time */
	gint init;
	volatile gint next_mode;
	gint open_rate;
	/* version 2 */
	GMutex *block_lock;
	gboolean is_nextframe_mode;
	GCond *cond;

	guint usleep1;
	guint usleep2;
	guint64 last_render_checktime;
	guint64 last_render_time;
    
};

#endif
