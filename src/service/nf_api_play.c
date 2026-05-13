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
*/

#include "nf_common.h"
#include <gobjmedia.h>
#include <nf_api_play.h>
#include <string.h>
#include "nf_watchdog.h"
#include "libicmem.h"

static GOBJDecPort *_dec_port;

#define NUM_IPX_CHANNEL NUM_ACTIVE_CH

#define ENABLE_PLAYBACK_CMD_SKIP
#define ENABLE_ZOOM_CMD_SKIP                                      

/*FIXME*/
/* Current version does not support multi playback instance. */

typedef enum _NF_PLAY_API_STATUS_E {
	NF_PLAY_API_STATUS_RUNNING, 
	NF_PLAY_API_STATUS_STOP
}NF_PLAY_API_STATUS_E;

typedef struct 
{
    GOBJMediaObj *h_display;
    guint vtick_interval_usec;
    GMutex *lock;
    GMutex *lock_thumb;

    NF_DISPLAY_E prev_disp_mode;
    int prev_vr_index[32];
    NF_PLAY_STATUS_E play_status[32];
    NF_PLAY_STATUS_E prev_play_status[32];
	NF_PLAY_API_STATUS_E	play_api_status;
    NF_PLAY_PARAM_MODE_E    prev_play_mode;
    NF_PLAY_PARAM_RATE_E    prev_play_rate;
    NF_PLAY_PARAM_DIR_E     prev_direction;
    NF_PLAY_PARAM_SPEED_E   prev_speed_flag;
    GTimeVal        prev_start_time;        // play start time
    GTimeVal        prev_end_time;      // play end time    
    gint            prev_interval;
    guint           prev_win_xpos;
    guint           prev_win_ypos;
    guint           prev_win_width;
    guint           prev_win_height;
    gint            prev_zoom_ch;
    gint            prev_zoom_x;
    gint            prev_zoom_y;
    gint            prev_zoom_w;
    gint            prev_zoom_h;
    guint64 prev_ch_mask;
	GOBJMediaUserConfigMode config_mode;
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];

	GAsyncQueue		*queue;
		
    guint64 last_render_time;
}PlayPipeObj;
PlayPipeObj _play_pipe;

struct playback_data pb_data;

typedef struct
{
	PLAYBACK_THREAD_STATUS state;

}PLAYBACK_CMD;


void local_playback_thread (GAsyncQueue* p_queue);
guint64 playback_thread_get_eos_mask();
guint64 playback_thread_get_overlapped_mask();
guint64 playback_thread_get_norecord_mask();
guint64 playback_thread_get_last_timestamp();
guint64 playback_thread_get_timestamp();


int playback_pipe_on = 0;
int playback_pipe_state = 0;
int playback_streams_open = 0;

/* 
 * Command Thread for Nonblocking API Call 
 */
#ifdef ENABLE_PLAYBACK_CMD_SKIP
static gboolean nf_play_change_internal( gpointer handle, NF_PLAY_PARAM *param, GError **error );
static GOBJMediaObj *_play_change_handle = NULL;
static NF_PLAY_PARAM _play_change_param;
static volatile int _play_change_is_pendding = 0;
static volatile int _prev_play_change_is_pendding = 0;
static volatile GStaticMutex _play_change_lock = G_STATIC_MUTEX_INIT;

typedef struct {
    int channel;
    int width;
    int height;
    int source;
} PlaybackResolutionChangeData;

static gboolean on_playback_resolution_changed_in_main_thread(gpointer user_data)
{
    PlaybackResolutionChangeData *data = user_data;
    int ret = -1;
    
    g_message("PLAYBACK MAIN THREAD: Resolution changed! ch=%d %dx%d src=%d", 
              data->channel, data->width, data->height, data->source);
    
    g_static_mutex_lock (&_play_change_lock); 
    
    if (_play_pipe.h_display) {
        ret = gobj_media_mode_change(
            _play_pipe.h_display,                  
            _play_pipe.config_mode,       
            _play_pipe.prev_win_xpos,          
            _play_pipe.prev_win_ypos,          
            _play_pipe.prev_win_width,         
            _play_pipe.prev_win_height,        
            _play_pipe.h_display->disp_channels[0],
            _play_pipe.coverts,
            0,
            0
        );

        if (ret < 0) {
            g_warning("gobj_media_mode_change failed for playback refresh! Ret=%d, Ch=%d", ret, data->channel);
        } else {
            g_message("Playback screen refresh successful. Ch=%d", data->channel);
        }
    } else {
        g_warning("Playback display handle (_play_change_handle) is NULL. Cannot refresh screen.");
    }
    
    g_static_mutex_unlock (&_play_change_lock);
    
    g_free(data);
    return G_SOURCE_REMOVE;
}

void gobj_media_playback_resolution_changed_cb_handler(gint channel, gint width, gint height, gint source, void *user_data)
{
    PlaybackResolutionChangeData *data = g_new(PlaybackResolutionChangeData, 1);
    data->channel = channel;
    data->width   = width;
    data->height  = height;
    data->source  = source;
    
    g_idle_add(on_playback_resolution_changed_in_main_thread, data);
}

static void _play_change_thread_func (gpointer args)
{
	gboolean	ret = 0;
	
	g_message("%s start", __FUNCTION__);
    {
        int policy;
        struct sched_param sched;
        pthread_t thread;
        
        policy = SCHED_FIFO;
        thread = pthread_self();
		sched.sched_priority = sched_get_priority_max(policy)-1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }
    while(1)
    {
		if( _play_change_is_pendding )		
		{	
            while(1)
            {
                g_static_mutex_lock (&_play_change_lock);
                _prev_play_change_is_pendding = _play_change_is_pendding;
                g_static_mutex_unlock (&_play_change_lock);
 //               g_message("SLEEP START = %d, %d\n", _prev_play_change_is_pendding, _play_change_is_pendding);
                g_usleep(100000);
  //              g_message("SLEEP END = %d, %d\n", _prev_play_change_is_pendding, _play_change_is_pendding);
                if (_play_change_is_pendding == _prev_play_change_is_pendding)
                    break;
   //             g_message("%s SKIP COMMAND!!!\n", __FUNCTION__);
            }
	//		g_message("%s RUN PLAY_CHANGE!!!!!!!", __FUNCTION__ );
            g_static_mutex_lock (&_play_change_lock);
		    if( _play_change_is_pendding )		
            {
                ret = nf_play_change_internal( &_play_change_handle, &_play_change_param, NULL );
                _play_change_is_pendding = 0;
            }
            g_static_mutex_unlock (&_play_change_lock);
//			g_message("%s RUN PLAY_CHANGE!!!!!!! ret[%d]", __FUNCTION__, ret);
        
        }
        g_usleep(50000);
    }
}
#endif   
#ifdef ENABLE_ZOOM_CMD_SKIP
static gboolean nf_play_zoom_move_internal(gint xpos, gint ypos, gint zoom_w, gint zoom_h);
static gboolean nf_play_zoom_move_internal_start(gint xpos, gint ypos, gint zoom_w, gint zoom_h);

static volatile int _zoom_move_is_pendding = 0;
static volatile int _prev_zoom_move_is_pendding = 0;
volatile GStaticMutex _zoom_move_lock = G_STATIC_MUTEX_INIT;

static void _zoom_move_thread_func (gpointer args)
{
	gboolean	ret = 0;
	
	g_message("%s start", __FUNCTION__);
    {
        int policy;
        struct sched_param sched;
        pthread_t thread;
        
        policy = SCHED_FIFO;
        thread = pthread_self();
		sched.sched_priority = sched_get_priority_max(policy)-1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }
    while(1)
    {
		if( _zoom_move_is_pendding )		
		{	
            g_usleep(200000);
            g_static_mutex_lock (&_zoom_move_lock);
		    if( _zoom_move_is_pendding )		
            {
                ret = nf_play_zoom_move_internal(_play_pipe.prev_zoom_x, _play_pipe.prev_zoom_y, _play_pipe.prev_zoom_w, _play_pipe.prev_zoom_h);
                _zoom_move_is_pendding = 0;
            }
            g_static_mutex_unlock (&_zoom_move_lock);
        }
        else
        {
            g_usleep(50000);
        }
    }
}
#endif   
/*
 *
 */



static GOBJMediaUserConfigMode _get_gobj_media_mode(NF_DISPLAY_E mode)
{
    GOBJMediaUserConfigMode    config_mode;
    switch (mode) {
        case NF_DISPLAY_FULL:
            config_mode = GOBJ_MEDIA_USER_PLAY_FULL;
        break;
        case NF_DISPLAY_QUAD:
            config_mode = GOBJ_MEDIA_USER_PLAY_QUAD;
        break;
        case NF_DISPLAY_HEXA_A:
            config_mode = GOBJ_MEDIA_USER_PLAY_HEXA_A;
        break;
        case NF_DISPLAY_HEXA_B:
            config_mode = GOBJ_MEDIA_USER_PLAY_HEXA_B;
        break;
        case NF_DISPLAY_OCTA_A:
            config_mode = GOBJ_MEDIA_USER_PLAY_OCTA_A;
        break;
        case NF_DISPLAY_OCTA_B: 
            config_mode = GOBJ_MEDIA_USER_PLAY_OCTA_B;
        break;
        case NF_DISPLAY_TRIDECA:
            config_mode = GOBJ_MEDIA_USER_PLAY_TRIDECA;
        break;
        case NF_DISPLAY_NONA:
            config_mode = GOBJ_MEDIA_USER_PLAY_NONA;
        break;
        case NF_DISPLAY_HEXADECA:
            config_mode = GOBJ_MEDIA_USER_PLAY_HEXADECA;
        break;
        case NF_DISPLAY_PLAYBACK_ZOOM:
            config_mode = GOBJ_MEDIA_USER_PLAY_ZOOM;
        break;
        case NF_DISPLAY_HEXATRICONTA:
			config_mode = GOBJ_MEDIA_USER_PLAY_HEXATRICONTA;
		break;
    }
    return config_mode;
}



#if defined (ENABLE_PLAY_COVERT)
static void _get_gobj_media_channel(int gobj_disp_ch[GOBJ_MAX_MEDIA_PORT], 
																				int gobj_covert_ch[GOBJ_MAX_MEDIA_PORT],
																				int ch_arr[32],
																				gboolean covert_arr[32],
																				int vr_num, guint64 *ch_mask, gboolean preview)
{
	int ch, i;

	*ch_mask = 0;
	for(i=0;i<GOBJ_MAX_MEDIA_PORT; i++)
	{
		gobj_disp_ch[i] = -1;
		gobj_covert_ch[i] = FALSE;		
	}

	for(ch=0; ch<NUM_IPX_CHANNEL;ch++)
	{
		if(ch_arr[ch] != -1)
		{
			g_assert(ch_arr[ch] < NUM_IPX_CHANNEL);
			//         gobj_disp_ch[ch_arr[ch]] = ch;
			gobj_disp_ch[ch_arr[ch]] = ch_arr[ch];   //20110513 for VICP Performance

			if(covert_arr[ch] == TRUE) {
				gobj_covert_ch[(int)ch_arr[ch]] = TRUE;
			}

            if(preview) *ch_mask |= (1ULL << ch+32);
            else *ch_mask |= (1ULL << ch);
		}
	}
}
#else
static void _get_gobj_media_channel(int gobj_disp_ch[GOBJ_MAX_MEDIA_PORT], int ch_arr[32], int vr_num, guint64 *ch_mask)
{
    int ch, i;

    *ch_mask = 0;
    for(i=0;i<GOBJ_MAX_MEDIA_PORT; i++)
    {
        gobj_disp_ch[i] = -1;
    }

    for(ch=0; ch<NUM_IPX_CHANNEL;ch++)
    {
        if(ch_arr[ch] != -1)
        {
            g_assert(ch_arr[ch] < NUM_IPX_CHANNEL);
//            gobj_disp_ch[ch_arr[ch]] = ch;
            gobj_disp_ch[ch_arr[ch]] = ch_arr[ch];   //20110513 for VICP Performance
            *ch_mask |= (1ULL << ch);
        }
    }
}
#endif

//FIXME: use second channel
static void _get_gobj_dec_param(GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT], gboolean all_on, int *ch_arr)
{
    int i;
    memset(dec_param, 0, sizeof(GOBJDecodeParam)*GOBJ_MAX_AV_PORT); //FIXME

#if 0
    if(ch_arr != NULL)
    for(i=0; i<32; i++)
    {
       printf("ch_arr[%d] = %d\n", i, ch_arr[i]);
    }
#endif

    if(all_on==TRUE)
    {
        for(i=_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET; 
                    i<_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
        {
            dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
            dec_param[i].dec_port = i-_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET;
        }
        return;
    }
    if(ch_arr == NULL)
        return;
    for(i=_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET; 
                i<_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
        {
        if(ch_arr[i-_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET] != -1)
            {
            dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
 //           dec_param[i].dec_port = i-GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET;
            dec_param[i].dec_port = ch_arr[i-_dec_port->GOBJ_PLAYBACK_1ST_CHANNEL_OFFSET];  //20110513 for VICP Performance
         }
    }
}

void
playback_send_cmd (PLAYBACK_THREAD_STATUS state)
{
	PLAYBACK_CMD* pb_cmd;
	pb_cmd = malloc(sizeof(PLAYBACK_CMD));
	pb_cmd->state = state;
	g_async_queue_push(_play_pipe.queue, pb_cmd);
}


gboolean nf_play_init()
{
    	int ret;

	memset(&_play_pipe, 0x00 , sizeof(PlayPipeObj));
	_dec_port = gboj_media_get_dec_port();	
	_play_pipe.h_display = (GOBJMediaObj *)nf_live_get_display_handle();

	g_assert(_play_pipe.h_display != NULL);

	_play_pipe.vtick_interval_usec =  DISPLAY_IS_PAL ? 40000 : 34000; //(guint)(1000000./DISPLAY_RATE);

	_play_pipe.lock = g_mutex_new();
	_play_pipe.lock_thumb = g_mutex_new();
	_play_pipe.prev_play_mode = NF_PLAY_PARAM_MODE_STOP;
	_play_pipe.play_api_status = NF_PLAY_API_STATUS_STOP;

	_play_pipe.prev_win_width = DISPLAY_ACTIVE_WIDTH;
	_play_pipe.prev_win_height = DISPLAY_ACTIVE_HEIGHT;
	_play_pipe.prev_zoom_ch = 0;
	_play_pipe.prev_zoom_x = 0;
	_play_pipe.prev_zoom_y = 0;
	_play_pipe.prev_zoom_w = DISPLAY_ACTIVE_WIDTH;
	_play_pipe.prev_zoom_h = DISPLAY_ACTIVE_HEIGHT;

	_play_pipe.queue = g_async_queue_new();

	g_thread_create( (GThreadFunc)local_playback_thread, _play_pipe.queue, FALSE, NULL);

	gobj_media_set_playback_resolution_changed_callback(
        gobj_media_playback_resolution_changed_cb_handler,
        NULL // user_data
    );	

#ifdef ENABLE_PLAYBACK_CMD_SKIP
	memset( &_play_change_param, 0x00, sizeof(NF_PLAY_PARAM) );
	g_thread_create( (GThreadFunc)_play_change_thread_func, NULL, FALSE, NULL);
#endif
#ifdef ENABLE_ZOOM_CMD_SKIP
	g_thread_create( (GThreadFunc)_zoom_move_thread_func, NULL, FALSE, NULL);
#endif
    return TRUE;
}

/*
 defined(_ANF5HG_1648D)    4
 defined(_ANF5HG_0824D)	   4	 
 defined(_UTM5HG_1648D)    1
 defined(_UTM5HG_0824D)    4
 defined(_UTM5HG_0412D)	   1 
 defined(_UTM5HGA_1648D)   4
 defined(_UTM5HGA_0824D)   4		 
 defined(_UTM5HGA_0412D)   4	 
 defined(_UTM6GB_0412D)    1
 defined(_UTM6GB_0824D)    4
*/
gint nf_play_get_stream_limit()
{
	gint limit = 4;
#if defined(_UTM5HG_1648D) || defined(_UTM5HG_0412D) || defined(_UTM6GB_0412D) || defined(_UTM7G_1648D) || defined(_UTM7G_0824D) || defined(_UTM7G_0412D)
	limit = 2;
#endif	
	return limit;
}

gint nf_play_get_stream_second_offset()
{
	gint offset = 32;
#if defined(_UTM5HGA_3296D)
	offset = 32;
#endif	
	return offset;
}

gboolean nf_play_start( gpointer *handle, NF_PLAY_PARAM *param, GError **error )
{
	int i, ret;
	GOBJMediaUserConfigMode    config_mode;
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _play_pipe.h_display;
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	guint64 ch_mask = 0;
	int audio_cnt = 0;  //FIXME
	int zoom_ch = -1;

	g_mutex_lock(_play_pipe.lock);
	if(_play_pipe.play_api_status != NF_PLAY_API_STATUS_STOP)
	{
		g_warning("%s: nf_play_start is aleady running !\n", __FUNCTION__);
		g_mutex_unlock(_play_pipe.lock);
		return FALSE;
	}
	_play_pipe.play_api_status = NF_PLAY_API_STATUS_RUNNING;

	*handle = (gpointer)param;  //Dummy handle

	g_assert(param->vr_num < 64);

	if(param->play_mode == NF_PLAY_PARAM_MODE_STOP) {
		g_warning("nf_play_start: invalid play param !!\n");
		g_mutex_unlock(_play_pipe.lock);
		return FALSE;
	}

    	_play_pipe.last_render_time = 0;

	for(i=0;i<GOBJ_MAX_MEDIA_PORT; i++){
	    	coverts[i] = FALSE;
	}
    
	for(i=0; i<32 ;i++){
		_play_pipe.play_status[i] = NF_PLAY_STATUS_NONE;
		_play_pipe.prev_play_status[i] = NF_PLAY_STATUS_NONE;
	}
	config_mode = _get_gobj_media_mode(param->disp_mode);
#if defined (ENABLE_PLAY_COVERT) 
	_get_gobj_media_channel(h_display->disp_channels[0], coverts, param->vr_index, param->vr_covert, param->vr_num, &ch_mask, param->preview_flag);
#else
	_get_gobj_media_channel(h_display->disp_channels[0], param->vr_index, param->vr_num, &ch_mask);
#endif
    gchar ch_addr[NUM_CHANNEL];
	for( i = 0; i < NUM_CHANNEL; i++ )
	{
	    ch_addr[i] = (gchar)param->vr_index[i];
	}
	gobj_media_set_geo_org_channel(ch_addr, NUM_CHANNEL, GOBJ_MEDIA_ID_MAIN);

	audio_cnt = 0;
	if(param->vr_num == 1)  
	{

		if(G_UNLIKELY(param->audio_in_video_chan >= 0))  
		{
			gboolean audio_covert=FALSE;

#if defined (ENABLE_PLAY_COVERT)
			for(i=0; i<NUM_IPX_CHANNEL;i++)
			{
				if( (param->vr_index[i] != -1) && (param->vr_covert[i] == TRUE)) {
					audio_covert = TRUE;
					//g_message("audio_in_video_chan:%d covert !", param->audio_in_video_chan );
					break;
				}
			}
#endif
			if(audio_covert == FALSE) {
				 //ch_mask |= (1ULL << (param->audio_in_video_chan+32));//FIXME - AUDIO CHANNEL BASE 32
				 audio_cnt = 1;
				//g_message("audio_in_video_chan:%d", param->audio_in_video_chan );           
			}
		}

	    for(i=0; i<NUM_IPX_CHANNEL;i++)
	    {
	        if(param->vr_index[i] != -1)
	            break;
	    }
	    zoom_ch = i;
	//       zoom_ch = h_display->disp_channels[0];
	    g_assert(zoom_ch != -1);
	}

	printf("=== nf_play_start: mode=%d, ch_mask=0x%016llx, audio_cnt=%d zoom_ch=%d,%d ===\n", param->disp_mode, ch_mask, audio_cnt, zoom_ch, h_display->disp_channels[0]);

	ret = gobj_media_mode_change(
	                        h_display,
	                        config_mode, 
	                        param->win_xpos,
	                        param->win_ypos,
	                        param->win_width,
	                        param->win_height,
	                        h_display->disp_channels[0], 
	                        coverts,
	                        0,
							0 //multi_disp_dev.DispID[0]		// Layer
	                        );    //FIXME audio == 0

	g_assert(ret >= 0);

	if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE) 
		gobj_media_pause_frame_push(zoom_ch);	

	pb_data.pr_id = (gint)0;
	pb_data.vtick_interval_usec =  _play_pipe.vtick_interval_usec;
	pb_data.open_rate = (gint)param->play_rate;
	pb_data.rate = (gint)param->play_rate;
	pb_data.direction = (gint)param->direction;
	pb_data.rate_control = (gboolean)0;
	pb_data.hide = (gboolean)0;
	pb_data.ch_num = (gint)(param->vr_num + audio_cnt);
	pb_data.audio_ch= (gint)param->audio_in_video_chan; //-1;
	pb_data.ch_mask = (guint64)(ch_mask);
	pb_data.mode = (gint)0;
	pb_data.begin_time = (guint64)(SST_TIMEVAL_TO_TIME(param->start_time));
	pb_data.end_time = (guint64)(SST_TIMEVAL_TO_TIME(param->end_time));
	pb_data.search_time = (guint64)(SST_TIMEVAL_TO_TIME(param->search_time));
	pb_data.time_offset = (gint64)(param->interval*SST_SECOND);
	pb_data.srm_offset = (gint)nf_play_get_stream_second_offset();
	pb_data.srm_limit = (gint)nf_play_get_stream_limit();
    
	if(param->play_mode == NF_PLAY_PARAM_MODE_START) 
	{
		_get_gobj_dec_param(dec_param, FALSE, param->vr_index);
		gobj_media_decode_change(h_display, 
		            dec_param,
		            GOBJ_MEDIA_ID_MAIN
		            );
		playback_thread_clear_status();
		playback_send_cmd(PLAYBACK_START);
	}
	else if(param->play_mode == NF_PLAY_PARAM_MODE_PAUSE) 
	{
		_get_gobj_dec_param(dec_param, FALSE, param->vr_index);
		gobj_media_decode_change(h_display, 
		                dec_param,
		                GOBJ_MEDIA_ID_MAIN
		                );
		playback_thread_clear_status();
		playback_send_cmd(PLAYBACK_PAUSE);
	}
	else
	{
		g_warning("First play start cmd can not be NEXT FRAME or STOP\n");
		g_mutex_unlock(_play_pipe.lock);
		return FALSE;
	}

	_play_pipe.prev_play_mode = param->play_mode;
	_play_pipe.prev_disp_mode = param->disp_mode;  
	memcpy(_play_pipe.prev_vr_index, param->vr_index, sizeof(_play_pipe.prev_vr_index));
	_play_pipe.prev_direction = param->direction;
	_play_pipe.prev_play_rate = param->play_rate;
	_play_pipe.prev_speed_flag = param->speed_flag;
	_play_pipe.prev_ch_mask = ch_mask;
	_play_pipe.prev_start_time = param->start_time;
	_play_pipe.prev_end_time = param->end_time;
	_play_pipe.prev_interval = param->interval;
	_play_pipe.prev_win_width = param->win_width;
	_play_pipe.prev_win_height = param->win_height;
	_play_pipe.prev_win_xpos = param->win_xpos;
	_play_pipe.prev_win_ypos = param->win_ypos;
	_play_pipe.config_mode = config_mode;
	memcpy(_play_pipe.coverts, coverts, sizeof(_play_pipe.coverts));

	g_mutex_unlock(_play_pipe.lock);
	return TRUE;
}

gboolean nf_play_get_thumbnail_jpeg( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint *width, gint *height, 
                                gint *size, 
                                void** out_buffer, 
                                GTimeVal *out_buffer_time,
                                NF_JPEG_SIZE_E srcSize
                                )
{
       GOBJMediaObj *h_display = _play_pipe.h_display;
    gpointer h_iframe_buf;
    int ret;
    unsigned int jpg_timestamp;
	guint64 begin_time_sst;
	guint64 end_time_sst;

	g_mutex_lock(_play_pipe.lock_thumb);
    g_message("ENTER: %s \n", __FUNCTION__);

	begin_time_sst = (guint64)(SST_TIMEVAL_TO_TIME(begin_time));
	end_time_sst = (guint64)(SST_TIMEVAL_TO_TIME(end_time));

    if( srcSize == NF_MAIN_SIZE )
    {
    }
    else
    {
        ch = ch + 32;
    }

        if( _get_first_iframe(&h_iframe_buf, out_buffer_time, ch, begin_time_sst, end_time_sst) < 0)
        {
              g_message("EXIT: %s - false gobj_sst_pipe_get_iframe \n", __FUNCTION__);
        	g_mutex_unlock(_play_pipe.lock_thumb);
              return FALSE;
        }

        ret = gobj_media_get_jpeg_snapshot(h_display, ch, GOBJJpegSnap_PlaySnap, srcSize, 0, 0, h_iframe_buf, out_buffer, width, height, size, &jpg_timestamp);
        if(ret < 0) {
            printf("%s : %d : ==== get jpeg fail!!! ret: %d ====\n", __FUNCTION__, __LINE__, ret);
            g_object_unref (h_iframe_buf);            
            g_mutex_unlock(_play_pipe.lock_thumb);
            return FALSE;
        }

       g_object_unref (h_iframe_buf);

        //must free out_buffer after use !!!!

        g_message("EXIT: %s - true \n", __FUNCTION__);
        g_mutex_unlock(_play_pipe.lock_thumb);
    return TRUE;   
}

gboolean nf_play_get_thumbnail_bmp( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint *width, gint *height, 
                                gint *size, 
                                void** out_buffer, 
                                GTimeVal *out_buffer_time
                                )
{
        g_message("not support api\n");
        return FALSE;
}

gboolean nf_play_get_jpeg_snapshot(gint ch, 
                                gint *width, gint *height, 
                                gint *size,
                                void** out_buffer,
                                gint timeimg,
                                gint dst,
                                guint *timestamp
                               ) 
{
    int jpg_width=0, jpg_height=0, jpg_size=0;
    int ret =0;
    char *jpeg_buf = NULL;
    GOBJMediaObj *h_display = _play_pipe.h_display;
    GOBJMediaUserConfigMode dispmode;
    unsigned int jpg_timestamp=0;
    GOBJJpegEncSrcSize srcSize;

    printf("%s: %d: ch : %d\n", __FUNCTION__, __LINE__, ch);

    dispmode = _get_gobj_media_mode(_play_pipe.prev_disp_mode);
    if( _play_pipe.prev_disp_mode == NF_DISPLAY_FULL )
    {
        srcSize = GOBJ_MAIN_SIZE;
    }
    else
    {
        srcSize = GOBJ_SECOND_SIZE;
    }                    

     printf("%s : %d : ch:%d, dispmode : %d, _play_pipe.prev_play_mode : %d srcSize:%d \n", 
                    __FUNCTION__, __LINE__, ch, dispmode, _play_pipe.prev_play_mode, srcSize);

    ret = gobj_media_get_jpeg_snapshot(h_display, ch, GOBJJpegSnap_PlaySnap, srcSize, timeimg, dst, NULL, &jpeg_buf, &jpg_width, &jpg_height, &jpg_size, &jpg_timestamp);
    if(ret < 0) {
        printf("%s : %d : ==== get jpeg fail!!! ret: %d ====\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    *out_buffer = jpeg_buf;
    *width = jpg_width;
    *height = jpg_height;
    *size = jpg_size;
    *timestamp = jpg_timestamp;

	 printf("%s : %d : get jpeg success!!\n",  __FUNCTION__, __LINE__ );
 
    return TRUE;
}

static gboolean _compare_gobj_media_channel(int ch_arr[32]) 
{
	gboolean ret=FALSE;
	int ch=0;

	for(ch=0; ch<NUM_IPX_CHANNEL;ch++) {
		//if( (_play_pipe.prev_vr_index[ch] != -1) && (ch_arr[ch] != -1)) {
		if(_play_pipe.prev_vr_index[ch] != ch_arr[ch]) {
			//g_printf("%s:%d: _play_pipe.prev_vr_index[%d]: %d != ch_arr[%d]: %d\n", __FUNCTION__, __LINE__, 
			//				ch, _play_pipe.prev_vr_index[ch], ch, ch_arr[ch]);
			ret=TRUE;
			break;
		}
	}

	return ret;

}

/* state  (rs:restart) 
                next status
        | play |  reverse |  pause |  next | 
play    |  rs  |   rs     |        |   err |
reverse |  rs  |   rs     |        |   err |
pause   |      |   rs     |   rs   |   rs  |     
next    |  rs  |   rs     |   err  |   rs  |
*/
/*Rule 2: Next Frame Mode
    Speed 1 -> Pause -> Next  (Do not reopen the stream)
    Speed -1 -> Pause -> Prev  (Do not reopen the stream)
*/
#ifdef ENABLE_PLAYBACK_CMD_SKIP

gboolean nf_play_change( gpointer handle, NF_PLAY_PARAM *param, GError **error )
{
	gboolean	ret;

	g_static_mutex_lock (&_play_change_lock);		
				
	_play_pipe.play_api_status = NF_PLAY_API_STATUS_RUNNING;

	if( param->play_mode == NF_PLAY_PARAM_MODE_START)
	{			
		_play_change_is_pendding++;
       // g_message("_play_change_is_pending++\n");
		_play_change_handle = handle;
		memcpy( &_play_change_param, param, sizeof(NF_PLAY_PARAM) );				
		ret = 1;					
		
	}else{
				
		_play_change_is_pendding = 0;
		_play_change_handle = NULL;
		memset( &_play_change_param, 0x00, sizeof(NF_PLAY_PARAM) );	
		
		ret = nf_play_change_internal( handle, param, NULL );	
	}			
	g_static_mutex_unlock (&_play_change_lock);
	

    //20110722 for changing mosaic quality.
	if( param->play_mode == NF_PLAY_PARAM_MODE_START
        && _play_pipe.prev_disp_mode != param->disp_mode
            )
    {
        g_usleep(100*1000);
    }

	return ret;

}

static gboolean nf_play_change_internal( gpointer handle, NF_PLAY_PARAM *param, GError **error )
#else
gboolean nf_play_change( gpointer handle, NF_PLAY_PARAM *param, GError **error )
#endif
{

	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	gboolean    ret;
	int audio_cnt;
	GOBJMediaObj *h_display = _play_pipe.h_display;
	guint64 last_time = 0;
	guint vtick_interval_usec = _play_pipe.vtick_interval_usec;
	guint64 ch_mask = 0;
	GOBJMediaUserConfigMode    config_mode;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	gint zoom_ch;
	int i;

	 gboolean vr_index_change = FALSE;
    
	_play_pipe.play_api_status = NF_PLAY_API_STATUS_RUNNING;
	
	for(i=0;i<GOBJ_MAX_MEDIA_PORT; i++)
	{
	    coverts[i] = FALSE;
	}

	g_mutex_lock(_play_pipe.lock);

	if(param->disp_mode == NF_DISPLAY_PLAYBACK_ZOOM)
	{
	    zoom_ch = _play_pipe.prev_zoom_ch;
	}
	else
	{
	    zoom_ch = -1;
	}
    
#if defined (ENABLE_PLAY_COVERT) 
	_get_gobj_media_channel(h_display->disp_channels[0], coverts, param->vr_index, param->vr_covert, param->vr_num, &ch_mask, param->preview_flag);
#else
	_get_gobj_media_channel(h_display->disp_channels[0], param->vr_index, param->vr_num, &ch_mask);
#endif
	gchar ch_addr[NUM_CHANNEL];
	for( i = 0; i < NUM_CHANNEL; i++ )
	{
	    ch_addr[i] = (gchar)param->vr_index[i];
	}
	gobj_media_set_geo_org_channel(ch_addr, NUM_CHANNEL,GOBJ_MEDIA_ID_MAIN);
	vr_index_change = _compare_gobj_media_channel(param->vr_index);

    /* 
     * set params 
     */
    audio_cnt = 0;
    if(param->vr_num == 1)  
    {
		if(G_UNLIKELY(param->audio_in_video_chan >= 0))  
		{
			gboolean audio_covert=FALSE;
#if defined (ENABLE_PLAY_COVERT)
			for(i=0; i<NUM_IPX_CHANNEL;i++)
			{
				if( (param->vr_index[i] != -1) && (param->vr_covert[i] == TRUE)) {
					audio_covert = TRUE;
					//g_message("audio_in_video_chan:%d covert !", param->audio_in_video_chan );
					break;
				}
			}
#endif	
			if(audio_covert == FALSE) {
				//ch_mask |= (1ULL << (param->audio_in_video_chan+32));//FIXME - AUDIO CHANNEL BASE 32
				audio_cnt = 1;
				//g_message("audio_in_video_chan:%d", param->audio_in_video_chan );           
			}
		}

        for(i=0; i<NUM_IPX_CHANNEL;i++)
        {
            if(param->vr_index[i] != -1)
                break;
        }
        zoom_ch = i;
        g_assert(zoom_ch != -1);
    }
    /* 
	g_message("[##+$*#] =================================\n");
	g_message("[##+$*#] _play_pipe.prev_play_mode : %d, param->play_mode: %d\n",
		_play_pipe.prev_play_mode, param->play_mode);
	g_message("[##+$*#]_play_pipe.prev_direction: %d, param->direction : %d\n",
                _play_pipe.prev_direction,param->direction); 
	g_message("[##+$*#]===================================\n");
	*/

    /* CASE0: RESUME  (PAUSE -> START)*/
    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE 
            && param->play_mode == NF_PLAY_PARAM_MODE_START
            && param->direction == _play_pipe.prev_direction
            && param->play_rate == _play_pipe.prev_play_rate
            && param->speed_flag == _play_pipe.prev_speed_flag
            && ch_mask == _play_pipe.prev_ch_mask
            /*add by hjkim, 20150626*/
            && vr_index_change == FALSE
            && _play_pipe.prev_win_width == param->win_width
            && _play_pipe.prev_win_height == param->win_height
            && _play_pipe.prev_win_xpos == param->win_xpos
            && _play_pipe.prev_win_ypos == param->win_ypos
            ) 
    {
            playback_send_cmd(PLAYBACK_CONTINUE);
            goto return_success;
    }

    /* PAUSE->PAUSE */
    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE 
            && param->play_mode == NF_PLAY_PARAM_MODE_PAUSE
            && param->direction == _play_pipe.prev_direction
            && param->play_rate == _play_pipe.prev_play_rate
            && param->speed_flag == _play_pipe.prev_speed_flag
            && ch_mask == _play_pipe.prev_ch_mask
            /* add by ugieipx, 20110714*/
            && _play_pipe.prev_win_width == param->win_width
            && _play_pipe.prev_win_height == param->win_height
            && _play_pipe.prev_win_xpos == param->win_xpos
            && _play_pipe.prev_win_ypos == param->win_ypos
            /* */
		     /*add by hjkim, 20150626*/
            && vr_index_change == FALSE
            ) 
    {
            goto return_success;
    }



    /* NEXT->NEXT */
    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME 
            && param->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME
            && param->direction == _play_pipe.prev_direction
            && param->play_rate == _play_pipe.prev_play_rate
            && param->speed_flag == _play_pipe.prev_speed_flag
            && ch_mask == _play_pipe.prev_ch_mask
            /* add by ugieipx, 20110714*/
            && _play_pipe.prev_win_width == param->win_width
            && _play_pipe.prev_win_height == param->win_height
            && _play_pipe.prev_win_xpos == param->win_xpos
            && _play_pipe.prev_win_ypos == param->win_ypos
            /* */
		     /*add by hjkim, 20150626*/
            && vr_index_change == FALSE		
            ) 
    {
            _get_gobj_dec_param(dec_param, FALSE, param->vr_index);
            gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN );

            playback_send_cmd(PLAYBACK_NEXT);
            goto return_success;
    }

    /* PAUSE->NEXT */
    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE 
            && param->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME
            && param->direction == _play_pipe.prev_direction
            && param->play_rate == 1
            && param->speed_flag == _play_pipe.prev_speed_flag
            && ch_mask == _play_pipe.prev_ch_mask
            /* add by ugieipx, 20110714*/
            && _play_pipe.prev_win_width == param->win_width
            && _play_pipe.prev_win_height == param->win_height
            && _play_pipe.prev_win_xpos == param->win_xpos
            && _play_pipe.prev_win_ypos == param->win_ypos
            /* */
		     /*add by hjkim, 20150626*/
            && vr_index_change == FALSE		
            ) 
    {
            _get_gobj_dec_param(dec_param, FALSE, param->vr_index);
            gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN );
           
            playback_send_cmd(PLAYBACK_NEXT);
            goto return_success;
    }



    /* non-pause and next -> pause or next */
    if(_play_pipe.prev_play_mode != NF_PLAY_PARAM_MODE_PAUSE  &&
       _play_pipe.prev_play_mode != NF_PLAY_PARAM_MODE_NEXT_FRAME
       )
    {
        /*CASE1: Non-Pause -> Next : error */
       if(param->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
       {
           g_warning("%s: Non-pause -> next transition is not available\n", __FUNCTION__);
           g_mutex_unlock(_play_pipe.lock);
           return FALSE;
       }

        /*CASE2: Non-Pause -> Pause : error */
       if(param->play_mode == NF_PLAY_PARAM_MODE_PAUSE)
       {
            playback_send_cmd(PLAYBACK_PAUSE);
            goto return_success;
       }
    }
        /*CASE3: ALL -> Stop  */
    if(param->play_mode == NF_PLAY_PARAM_MODE_STOP)
    {
        if(_play_pipe.prev_play_mode != NF_PLAY_PARAM_MODE_STOP)
        {
            playback_send_cmd(PLAYBACK_STOP);
			while(1){
					if(playback_pipe_state == 0)
						break;
					usleep(100000);
				}
            _play_pipe.last_render_time = playback_thread_get_last_timestamp();
            _get_gobj_dec_param(dec_param, FALSE, NULL);
            gobj_media_decode_change(h_display, 
                                dec_param,
                                GOBJ_MEDIA_ID_MAIN
                                );
        }
        goto return_success;
    }
    


    if(_play_pipe.prev_play_mode != NF_PLAY_PARAM_MODE_STOP)
    {
            playback_send_cmd(PLAYBACK_STOP);
			while(1){
					if(playback_pipe_state == 0)
						break;
					usleep(100000);
				}
        _play_pipe.last_render_time = playback_thread_get_last_timestamp();
        _get_gobj_dec_param(dec_param, FALSE, NULL);
        gobj_media_decode_change(h_display, 
                                dec_param,
                                GOBJ_MEDIA_ID_MAIN
                                );
    }

    last_time = _play_pipe.last_render_time;
    if(last_time == 0)
    {
        last_time = SST_TIMEVAL_TO_TIME(param->search_time);
        g_warning("%s: last_time is 0\n", __FUNCTION__);
    }
    else {
        if( param->direction == NF_PLAY_PARAM_DIR_BACKWARD) {
            last_time += vtick_interval_usec*SST_USECOND;
        } else {
            last_time -= vtick_interval_usec*SST_USECOND;
        }
    }

    if(param->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME
        && last_time != 0 
        && ch_mask != _play_pipe.prev_ch_mask)
    {
        if( param->direction == NF_PLAY_PARAM_DIR_BACKWARD) {
            last_time -= vtick_interval_usec*SST_USECOND;
        } else {
            last_time += vtick_interval_usec*SST_USECOND;
        }
    }

    if((u32)ch_mask != (u32)_play_pipe.prev_ch_mask
        || _play_pipe.prev_win_width != param->win_width
        || _play_pipe.prev_win_height != param->win_height
        || _play_pipe.prev_win_xpos != param->win_xpos
        || _play_pipe.prev_win_ypos != param->win_ypos
        || vr_index_change == TRUE
        )   //Geometry changed 
    {
        config_mode = _get_gobj_media_mode(param->disp_mode);

        //wait for flushing remain video stream. //20110721
        if(_play_pipe.prev_disp_mode == param->disp_mode || param->disp_mode == NF_DISPLAY_FULL)
        {

            if(_play_pipe.prev_win_width == param->win_width
            && _play_pipe.prev_win_height == param->win_height
            && _play_pipe.prev_win_xpos == param->win_xpos
            && _play_pipe.prev_win_ypos == param->win_ypos
            )   //Geometry changed 
            {
               g_usleep(60*1000); 
            }
        }

        g_message("##### %s called!! 2 config_mode = %d, w=%d, h=%d, vrnum= %d\n", __FUNCTION__, config_mode, param->win_width, param->win_height, param->vr_num);
        ret = gobj_media_mode_change(
                                h_display,
                                config_mode, 
                                param->win_xpos,
                                param->win_ypos,
                                param->win_width,
                                param->win_height,
                                h_display->disp_channels[0], 
                                coverts,
                                0,
								0 //multi_disp_dev.DispID[0]		// Layer
                                );    //FIXME audio == 0
        g_assert(ret >= 0);


    }

	
	pb_data.ch_mask = (guint64)ch_mask;
	pb_data.pr_id= (gint)0;
	pb_data.vtick_interval_usec= _play_pipe.vtick_interval_usec;

	if(param->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME || param->play_mode == NF_PLAY_PARAM_MODE_PAUSE) 
	{    //20080104
		pb_data.rate= (gint)1;
		pb_data.open_rate= (gint)1;
		pb_data.rate_control= (gboolean)0;
		pb_data.audio_ch= (gint)-1;
		pb_data.is_slow= (gboolean)FALSE;
	} 
	else 
	{
		gboolean is_slow = (param->speed_flag == NF_PLAY_PARAM_SPEED_SLOW ? TRUE: FALSE);
		g_warning("rate = %d\n", param->play_rate);

		
		pb_data.rate= (gint)param->play_rate;
		if(is_slow == FALSE)
		{
			pb_data.open_rate= (gint)param->play_rate;
		}
		else
		{
			pb_data.open_rate= (gint)1;
		}
		pb_data.rate_control= (gboolean)0;
		pb_data.audio_ch= (gint)param->audio_in_video_chan; //-1;
		pb_data.is_slow= (gboolean)is_slow;

	}

	pb_data.hide= (gboolean)0;
	pb_data.mode= (gint)0;
	pb_data.ch_num= (gint)param->vr_num+audio_cnt;

	if(param->direction != _play_pipe.prev_direction) 
	{
		pb_data.direction = (gint)param->direction;
		pb_data.end_time = SST_TIMEVAL_TO_TIME(param->end_time);
		pb_data.begin_time= SST_TIMEVAL_TO_TIME(param->start_time);
	} 
	else 
	{
		if(param->end_time.tv_sec != _play_pipe.prev_end_time.tv_sec 
		    && param->end_time.tv_usec != _play_pipe.prev_end_time.tv_usec ) 
		{
			pb_data.end_time = SST_TIMEVAL_TO_TIME(param->end_time);
		}
		if(param->start_time.tv_sec != _play_pipe.prev_start_time.tv_sec 
		        && param->start_time.tv_usec != _play_pipe.prev_start_time.tv_usec ) 
		{
			pb_data.begin_time= SST_TIMEVAL_TO_TIME(param->start_time);
		}
	}

	pb_data.search_time= last_time;

	if(param->interval != _play_pipe.prev_interval)
	{
		pb_data.time_offset= param->interval*SST_SECOND;
	}
    /*
     * set params done 
     */
    
    if(param->play_mode == NF_PLAY_PARAM_MODE_START)
    {
        _get_gobj_dec_param(dec_param, FALSE, param->vr_index);
        gobj_media_decode_change(h_display, 
                                dec_param , GOBJ_MEDIA_ID_MAIN
                                );
        playback_send_cmd(PLAYBACK_START);
    }
    else if(param->play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME)
    {
        _get_gobj_dec_param(dec_param, FALSE, param->vr_index);
        gobj_media_decode_change(h_display, 
                                dec_param, GOBJ_MEDIA_ID_MAIN
                                );
        playback_send_cmd(PLAYBACK_NEXT);
    }
    else if(param->play_mode == NF_PLAY_PARAM_MODE_PAUSE)
    {
        _get_gobj_dec_param(dec_param, FALSE, param->vr_index);
        gobj_media_decode_change(h_display, 
                                dec_param, GOBJ_MEDIA_ID_MAIN
                                );
        playback_send_cmd(PLAYBACK_PAUSE);
    }
    
return_success:
    _play_pipe.prev_play_mode = param->play_mode;
    _play_pipe.prev_disp_mode = param->disp_mode;  
    memcpy(_play_pipe.prev_vr_index, param->vr_index, sizeof(_play_pipe.prev_vr_index));
    _play_pipe.prev_direction = param->direction;
    _play_pipe.prev_play_rate = param->play_rate;
    _play_pipe.prev_speed_flag = param->speed_flag;
    _play_pipe.prev_ch_mask = ch_mask;
    _play_pipe.prev_start_time = param->start_time;
    _play_pipe.prev_end_time = param->end_time;
    _play_pipe.prev_interval = param->interval;
    _play_pipe.prev_win_width = param->win_width;
    _play_pipe.prev_win_height = param->win_height;
    _play_pipe.prev_win_xpos = param->win_xpos;
    _play_pipe.prev_win_ypos = param->win_ypos;
	_play_pipe.config_mode = config_mode;
	memcpy(_play_pipe.coverts, coverts, sizeof(_play_pipe.coverts));

    g_mutex_unlock(_play_pipe.lock);
    return TRUE;
	
}

gboolean   nf_play_get_status( gpointer handle, NF_PLAY_STATUS *status, GError **error )
{
    int i, disp_chan;
    guint64 eos_mask = 0;
    guint64 overlapped_mask = 0;
    guint64 norecord_mask = 0;
    guint64 cur_clock;
    
	int pipe_mask = 0;
  		
    g_mutex_lock(_play_pipe.lock);
    //FIXME: AT EndVideo ????
    cur_clock = playback_thread_get_timestamp();

    //printf("!!!!!!!!!!!cur_clock=  %"GST_TIME_FORMAT"\r\n", GST_TIME_ARGS(cur_clock));

    if(cur_clock == 0)
    {
        g_mutex_unlock(_play_pipe.lock);
        return FALSE;
    }

    eos_mask = playback_thread_get_eos_mask();
    overlapped_mask = playback_thread_get_overlapped_mask();
    norecord_mask = playback_thread_get_norecord_mask();

	if(_play_pipe.play_api_status == NF_PLAY_API_STATUS_STOP)
		eos_mask = 0;
 
    for(i=0; i< 32; i++)
    {
        disp_chan = _play_pipe.prev_vr_index[i];
        
        if(disp_chan != -1)
        {
            if(eos_mask & (1ULL<<(pipe_mask))) 
            {
                status->play_status[i] = NF_PLAY_STATUS_ENDVIDEO;
              //  status->play_status[i] = NF_PLAY_STATUS_ENDVIDEO;   //20110513
            }
            else if(overlapped_mask & (1ULL<<pipe_mask)) 
            {
                status->play_status[i] = NF_PLAY_STATUS_OVERLAPPED;
               // status->play_status[i] = NF_PLAY_STATUS_OVERLAPPED;
            }
            else if(norecord_mask & (1ULL<<pipe_mask)) 
            {
                status->play_status[i] = NF_PLAY_STATUS_NORECORD;
              //  status->play_status[i] = NF_PLAY_STATUS_NORECORD;
            }
			
			pipe_mask++;
        }
	else{
                status->play_status[i] = NF_PLAY_STATUS_ENDVIDEO;
        }
	
    }

    SST_TIME_TO_TIMEVAL(cur_clock, status->play_time);
	
    g_mutex_unlock(_play_pipe.lock);
    return TRUE;    
}


gboolean nf_play_stop( gpointer handle )
{
	GOBJMediaObj *h_display = _play_pipe.h_display;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];

	g_mutex_lock(_play_pipe.lock);

	if(_play_pipe.play_api_status != NF_PLAY_API_STATUS_RUNNING)
	{
		g_warning("%s: nf_play_stop is aleady stopping !\n", __FUNCTION__);
		g_mutex_unlock(_play_pipe.lock);
		return TRUE;    
	}
	_play_pipe.play_api_status = NF_PLAY_API_STATUS_STOP;

	playback_thread_clear_status();
	playback_send_cmd(PLAYBACK_STOP);
	while(1){
		if(playback_pipe_state == 0)
			break;
		usleep(100000);
	}

	_play_pipe.last_render_time = playback_thread_get_last_timestamp();

	_get_gobj_dec_param(dec_param, FALSE, NULL);
	gobj_media_decode_change(h_display, 
	                dec_param,
	                GOBJ_MEDIA_ID_MAIN
	                );
	g_mutex_unlock(_play_pipe.lock);

	return TRUE; 
}


gboolean nf_play_set_thumbnail_geometry( 
                                guint base_x, 
                                guint base_y,
                                guint width, guint height
                                )
{
GOBJMediaObj *h_display = _play_pipe.h_display;
    int disp_channels[GOBJ_MAX_MEDIA_PORT];
    gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];    
    int i, ret;

//    g_message("%s\n", __FUNCTION__);

    for(i=0; i<GOBJ_MAX_MEDIA_PORT; i++)
    {
        disp_channels[i] = -1;
        coverts[i] = FALSE;
    }
   // disp_channels[0] = h_display->disp_channel_num;   //decport h
    disp_channels[0] = 0;   //decport h

    
    /*video layer x position for thumnail must be even value*/
#if 0
    base_x = (h_display->disp_width - width  + 1) & 0xFFFFFFFE;  
    base_y = h_display->disp_height - height;
#else
    if(base_x != 0)
       base_x = (base_x -1) & 0xFFFFFFFE;
#endif

    g_message("============= set thumbnail w=%d, h=%d\n", width, height);
    ret = gobj_media_mode_change(
                            h_display,
                            GOBJ_MEDIA_USER_PLAY_THUMBNAIL, //GOBJ_MEDIA_USER_PLAY_FULL, 
                            base_x,
                            base_y,
                            width,
                            height,
                            disp_channels, 
                            coverts,
                            0,
							0 //multi_disp_dev.DispID[0]		// Layer
                            );    //FIXME Audio
    g_assert(ret >= 0);

    _get_gobj_dec_param(dec_param, FALSE, disp_channels);
    gobj_media_decode_change(h_display, 
                        dec_param,
                        GOBJ_MEDIA_ID_MAIN
                        );	

    return TRUE;
}

static int _get_resol_size_icodec(guint res, guint *w, guint *h)
{
    switch( res )
    {
        case RES_NTSC_CIF:   *w = 352; *h = 240; break;
        case RES_NTSC_2CIF:  *w = 704; *h = 240; break;
        case RES_NTSC_4CIF:  *w = 704; *h = 480; break;
        case RES_NTSC_4CIFP: *w = 704; *h = 480; break;
        case RES_PAL_CIF:    *w = 352; *h = 288; break;
        case RES_PAL_2CIF:   *w = 704; *h = 288; break;
        case RES_PAL_4CIF:   *w = 704; *h = 576; break;
        case RES_PAL_4CIFP:  *w = 704; *h = 576; break;
        case RES_640x480:    *w = 640; *h = 480; break;
        case RES_720x480:    *w = 720; *h = 480; break;
        case RES_720x576:    *w = 720; *h = 576; break;
        case RES_800x600:    *w = 800; *h = 600; break;
        case RES_1024x768:   *w = 1024;*h = 768; break;
        case RES_1280x1024:  *w = 1280;*h = 1024; break;
        case RES_1600x1200:  *w = 1600;*h = 1200; break;
        case RES_1280x720:   *w = 1280;*h = 720; break;
        case RES_1920x1080:  *w = 1920;*h = 1080; break;
        case RES_640x352:    *w = 640; *h = 352; break;
        case RES_640x360:    *w = 640; *h = 360; break;
        case RES_640x360I:   *w = 640; *h = 360; break;
        case RES_1280x720I:  *w = 1280;*h = 720; break;
        case RES_1920x1080I: *w = 1920;*h = 1080; break;
        case RES_640x400:    *w = 640; *h = 400; break;
        case RES_800x450:    *w = 800; *h = 450; break;
        case RES_1440x900:   *w = 1440;*h = 900; break;
        case RES_320x180:    *w = 320; *h = 180; break;
        case RES_2304x1296:  *w = 2304;*h = 1296; break;
        case RES_2048x1536:  *w = 2048;*h = 1536; break;
        case RES_2560x1440:  *w = 2560;*h = 1440; break;
        case RES_2688x1520:  *w = 2592;*h = 1520; break;
        case RES_2560x1600:  *w = 2560;*h = 1600; break;
        case RES_2560x1920:  *w = 2560;*h = 1920; break;
        case RES_2592x1920:  *w = 2592;*h = 1920; break;
        case RES_2592x1944:  *w = 2592;*h = 1944; break;
        case RES_2992x1680:  *w = 2992;*h = 1680; break;
        case RES_2880x1800:  *w = 2880;*h = 1800; break;
        case RES_3200x1800:  *w = 3200;*h = 1800; break;
        case RES_2880x2160:  *w = 2880;*h = 2160; break;
        case RES_3072x2048:  *w = 3072;*h = 2048; break;
        case RES_3200x2400:  *w = 3200;*h = 2400; break;
        case RES_3840x2160:  *w = 3840;*h = 2160; break;
        case RES_2592x1520:  *w = 2592;*h = 1520; break;
        case RES_3000x3000:  *w = 3000;*h = 3000; break;
        case RES_2048x2048:  *w = 2048;*h = 2048; break;
        case RES_1280x1280:  *w = 1280;*h = 1280; break;
        case RES_640x640:    *w = 640; *h = 640; break;
        case RES_320x320:    *w = 320; *h = 320; break;
        case RES_960H_NTSC_CIF:   *w = 352; *h = 240; break;
        case RES_960H_NTSC_2CIF:  *w = 704; *h = 240; break;
        case RES_960H_NTSC_4CIF:  *w = 960; *h = 480; break;
        case RES_960H_NTSC_4CIFP: *w = 960; *h = 480; break;
        case RES_960H_PAL_CIF:    *w = 352; *h = 288; break;
        case RES_960H_PAL_2CIF:   *w = 704; *h = 288; break;
        case RES_960H_PAL_4CIF:   *w = 960; *h = 576; break;
        case RES_960H_PAL_4CIFP:  *w = 960; *h = 576; break;
        case RES_360x640:   *w = 360; *h = 640; break;
        case RES_480x640:   *w = 480; *h = 640; break;
        case RES_480x704:   *w = 480; *h = 704; break;
        case RES_576x704:   *w = 576; *h = 704; break;
        case RES_720x1280:  *w = 720; *h = 1280; break;
        case RES_768x1024:  *w = 768; *h = 1024; break;
        case RES_1024x1280: *w = 1024; *h = 1280; break;
        case RES_1080x1920: *w = 1080; *h = 1920; break;
        case RES_1536x2048: *w = 1536; *h = 2048; break;
        case RES_1296x2304: *w = 1296; *h = 2304; break;
        case RES_1520x2592: *w = 1520; *h = 2592; break;
        case RES_1944x2592: *w = 1944; *h = 2592; break;
        case RES_2160x3840: *w = 2160; *h = 3840; break;
        case RES_NTSC_NONE: *w = 0;   *h = 0; break;
        default:
            printf("wrong input rel:%d!", res);
            *w = 0;   *h = 0;
        break;
    }

    return 0;
}

gboolean nf_play_get_thumbnail( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint width, gint height, 
                                gint bits_per_pixel, 
								gpointer out_buffer,
                                GTimeVal *out_buffer_time
                                )
{
    GOBJMediaObj *h_display = _play_pipe.h_display;
    gpointer h_iframe_buf;
	guint64 begin_time_sst;
	guint64 end_time_sst;

    g_message("ENTER: %s \n", __FUNCTION__);

	
	begin_time_sst = (guint64)(SST_TIMEVAL_TO_TIME(begin_time));
	end_time_sst = (guint64)(SST_TIMEVAL_TO_TIME(end_time));

    if( _get_first_iframe(&h_iframe_buf, out_buffer_time, ch, begin_time_sst, end_time_sst) < 0)
    {
        g_message("EXIT: %s - false gobj_sst_pipe_get_iframe \n", __FUNCTION__);
        return FALSE;
    }

    g_message("%s w[%d] h[%d] bpp[%d]", __FUNCTION__, width, height, bits_per_pixel);

    if( gobj_media_decode_image(h_display, h_iframe_buf, width, height, bits_per_pixel, out_buffer) < 0)
    {
        g_message("EXIT: %s - false gobj_media_decode_image \n", __FUNCTION__);
        return FALSE;
    }
    
    g_message("EXIT: %s - true \n", __FUNCTION__);
    return TRUE; 
}

gboolean nf_play_get_thumbnail_v2( gint ch, 
                                GTimeVal begin_time, GTimeVal end_time, 
                                gint *width, gint *height, 
                                gint bits_per_pixel, 
								gpointer out_buffer,
                                GTimeVal *out_buffer_time
                                )
{
    GOBJMediaObj *h_display = _play_pipe.h_display;
    gpointer h_iframe_buf;
	guint64 begin_time_sst;
	guint64 end_time_sst;
	gint res_w = 0, res_h = 0;

    g_message("ENTER: %s", __FUNCTION__);
	
	begin_time_sst = (guint64)(SST_TIMEVAL_TO_TIME(begin_time));
	end_time_sst = (guint64)(SST_TIMEVAL_TO_TIME(end_time));

    if( _get_first_iframe_with_size(&h_iframe_buf, out_buffer_time, ch, begin_time_sst, end_time_sst, &res_w, &res_h) < 0)
    {
        g_message("EXIT: %s - false gobj_sst_pipe_get_iframe", __FUNCTION__);
        return FALSE;
    }

    g_message("%s %d ch[%d] w[%d] h[%d] bpp[%d]", __FUNCTION__, __LINE__, ch, res_w, res_h, bits_per_pixel);

    if( gobj_media_decode_image(h_display, h_iframe_buf, res_w, res_h, bits_per_pixel, out_buffer) < 0)
    {
        g_message("EXIT: %s - false gobj_media_decode_image", __FUNCTION__);
        return FALSE;
    }

	*width = res_w;
	*height = res_h;
    
    g_message("EXIT: %s - true", __FUNCTION__);
	
    return TRUE; 
}

gboolean nf_play_zoom_start(int ch, int base_x, int base_y, 
        int zoom_w, int zoom_h, int pip_x, int pip_y, int pip_w, int pip_h) 
{
    int i, ret;
    int zoom_ch;
    GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
    GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
//  int channels[GOBJ_MAX_MEDIA_PORT];
    gboolean coverts[GOBJ_MAX_MEDIA_PORT];
    GOBJMediaObj *h_display = _play_pipe.h_display;


#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_lock (&_zoom_move_lock);		
    _zoom_move_is_pendding = 0;
#endif

    
    if(_play_pipe.prev_disp_mode != NF_DISPLAY_FULL 
            && _play_pipe.prev_disp_mode != NF_DISPLAY_PLAYBACK_ZOOM
            && _play_pipe.prev_play_mode != NF_PLAY_PARAM_MODE_STOP
            )
    {
        g_warning("%s: full screen only supported.\n", __FUNCTION__);
#ifdef ENABLE_ZOOM_CMD_SKIP
	    g_static_mutex_unlock (&_zoom_move_lock);
#endif
        return FALSE;
    }

    for(i=0; i<GOBJ_MAX_MEDIA_PORT; i++)
    {
        coverts[i] = FALSE;
    }

    printf("## %s : %d : zoom start = %d, disp_channel_num = %d\n", 
        __FUNCTION__, __LINE__,ch, h_display->disp_channel_num);
    
    ret = gobj_media_mode_change(
                            h_display,
                            GOBJ_MEDIA_USER_PLAY_ZOOM, 
                            0,
                            0,
                            DISPLAY_ACTIVE_WIDTH,
                            DISPLAY_ACTIVE_HEIGHT,
                            h_display->disp_channels[0], 
                            coverts,
                            0,
							0 //multi_disp_dev.DispID[0]		// Layer
                            );    //FIXME Audio
    
    g_assert(ret >= 0);

	if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE) 
		gobj_media_pause_frame_push(ch);

    _play_pipe.prev_zoom_x = 0;
    _play_pipe.prev_zoom_y = 0;
    _play_pipe.prev_zoom_w = DISPLAY_ACTIVE_WIDTH;
    _play_pipe.prev_zoom_h = DISPLAY_ACTIVE_HEIGHT;
    _play_pipe.prev_disp_mode = NF_DISPLAY_PLAYBACK_ZOOM;
    
    //FIXME (IPX0412 ONLY)
    //enable decode all  
    //
    //q
    //FIXME: Full screen only
    for(i=0; i<32; i++)
    {
        if(_play_pipe.prev_vr_index[i] != -1)
           zoom_ch = i;
    }
    

    _play_pipe.prev_zoom_ch = zoom_ch;
    _get_gobj_dec_param(dec_param, FALSE, _play_pipe.prev_vr_index);
    gobj_media_decode_change(h_display, 
                        dec_param,
                        GOBJ_MEDIA_ID_MAIN
                        );

    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE 
       || _play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME
            )
    {
#ifdef ENABLE_ZOOM_CMD_SKIP
        //ret = nf_play_zoom_move_internal(_play_pipe.prev_zoom_x, 
        ret = nf_play_zoom_move_internal_start(_play_pipe.prev_zoom_x, 
                            _play_pipe.prev_zoom_y,
                            _play_pipe.prev_zoom_w,
                            _play_pipe.prev_zoom_h);
#else
        ret = nf_play_zoom_move(_play_pipe.prev_zoom_x, 
                            _play_pipe.prev_zoom_y,
                            _play_pipe.prev_zoom_w,
                            _play_pipe.prev_zoom_h);
#endif
    }
#ifdef ENABLE_ZOOM_CMD_SKIP
    g_static_mutex_unlock (&_zoom_move_lock);
#endif
    return TRUE;
}


#define MIN_ZOOM_WIDTH  64
#define MIN_ZOOM_HEIGHT 36
#ifdef ENABLE_ZOOM_CMD_SKIP
gboolean nf_play_zoom_move(gint xpos, gint ypos, gint zoom_w, gint zoom_h) 
{
	gboolean	ret;

    //make odd position to even position for netra scaler : 20110811 
    xpos = xpos & (int)(~1);
    ypos = ypos & (int)(~1);
    zoom_w = zoom_w & (int)(~1);
    zoom_h = zoom_h & (int)(~1);
    //


    if(xpos+zoom_w > DISPLAY_ACTIVE_WIDTH || xpos < 0)
        return FALSE;
    if(ypos+zoom_h > DISPLAY_ACTIVE_HEIGHT || ypos < 0)
        return FALSE;
    if(zoom_w < MIN_ZOOM_WIDTH)
        return FALSE;
    if(zoom_h < MIN_ZOOM_HEIGHT)
        return FALSE;

	g_static_mutex_lock (&_zoom_move_lock);		

    _zoom_move_is_pendding++;
    _play_pipe.prev_zoom_x = xpos;
    _play_pipe.prev_zoom_y = ypos;
    _play_pipe.prev_zoom_w = zoom_w;
    _play_pipe.prev_zoom_h = zoom_h;

    ret = 1;					

	g_static_mutex_unlock (&_zoom_move_lock);
	
	return ret;

}

static gboolean nf_play_zoom_move_internal_start(gint xpos, gint ypos, gint zoom_w, gint zoom_h) 
{
    int ret;
    GOBJMediaObj *h_display = _play_pipe.h_display;
    GOBJVideoRect *vr = gobj_media_get_current_vr(h_display, 0);

    g_assert(vr != NULL);

    if(_play_pipe.prev_disp_mode != NF_DISPLAY_PLAYBACK_ZOOM)
    {
        g_warning("%s: \n", __FUNCTION__);
        return FALSE;
    }

    //printf("%s: xpos=%d, ypos=%d, zoom_w=%d, zoom_h=%d\n", __FUNCTION__, xpos, ypos, zoom_w, zoom_h);

    if(xpos+zoom_w > DISPLAY_ACTIVE_WIDTH || xpos < 0)
        return FALSE;
    if(ypos+zoom_h > DISPLAY_ACTIVE_HEIGHT || ypos < 0)
        return FALSE;
    if(zoom_w < MIN_ZOOM_WIDTH)
        return FALSE;
    if(zoom_h < MIN_ZOOM_HEIGHT)
        return FALSE;


    vr[0].out_crop_left = xpos;
    vr[0].out_crop_right = xpos+zoom_w;
    vr[0].out_crop_top = ypos;
    vr[0].out_crop_bottom = ypos+zoom_h;
    
    //FIXME: omx crop bug
    if(vr[0].out_crop_left == 0 && vr[0].out_crop_right == DISPLAY_ACTIVE_WIDTH &&
        vr[0].out_crop_top == 0 && vr[0].out_crop_bottom == DISPLAY_ACTIVE_HEIGHT
            )
    {
        vr[0].out_crop_left = 0;
        vr[0].out_crop_right = DISPLAY_ACTIVE_WIDTH;
        vr[0].out_crop_top = 0;
        vr[0].out_crop_bottom = DISPLAY_ACTIVE_HEIGHT;
    }

    
    ret = gobj_media_change(h_display, 
                        vr,
                        0,   //FIXME AUDIO
                        TRUE,   /*Fast Change Off */
						0 //multi_disp_dev.DispID[0]		// Layer
                        );
    
#if 0 
    g_assert(ret >= 0);
#else
    if(ret < 0)
       g_warning("%s: display_change fail\n", __FUNCTION__);
#endif

    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE 
       || _play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME
            )
    {
        printf("=== pause zoom: port = %d, %d\n", vr[1].video_channel_id, h_display->disp_channel_num);
        //gobj_media_push_dec_frame_buffer(h_display, vr[1].video_channel_id, 1);//HDI
    }

    _play_pipe.prev_zoom_x = xpos;
    _play_pipe.prev_zoom_y = ypos;
    _play_pipe.prev_zoom_w = zoom_w;
    _play_pipe.prev_zoom_h = zoom_h;
    
    return TRUE;
}


static gboolean nf_play_zoom_move_internal(gint xpos, gint ypos, gint zoom_w, gint zoom_h) 
#else
gboolean nf_play_zoom_move(gint xpos, gint ypos, gint zoom_w, gint zoom_h) 
#endif
{
    int ret;
    GOBJMediaObj *h_display = _play_pipe.h_display;
    GOBJVideoRect *vr = gobj_media_get_current_vr(h_display, 0);

    g_assert(vr != NULL);

    if(_play_pipe.prev_disp_mode != NF_DISPLAY_PLAYBACK_ZOOM)
    {
        g_warning("%s: \n", __FUNCTION__);
        return FALSE;
    }

    //printf("%s: xpos=%d, ypos=%d, zoom_w=%d, zoom_h=%d\n", __FUNCTION__, xpos, ypos, zoom_w, zoom_h);

    if(xpos+zoom_w > DISPLAY_ACTIVE_WIDTH || xpos < 0)
        return FALSE;
    if(ypos+zoom_h > DISPLAY_ACTIVE_HEIGHT || ypos < 0)
        return FALSE;
    if(zoom_w < MIN_ZOOM_WIDTH)
        return FALSE;
    if(zoom_h < MIN_ZOOM_HEIGHT)
        return FALSE;


    vr[0].out_crop_left = xpos;
    vr[0].out_crop_right = xpos+zoom_w;
    vr[0].out_crop_top = ypos;
    vr[0].out_crop_bottom = ypos+zoom_h;
    
    //FIXME: omx crop bug
    if(vr[0].out_crop_left == 0 && vr[0].out_crop_right == DISPLAY_ACTIVE_WIDTH &&
        vr[0].out_crop_top == 0 && vr[0].out_crop_bottom == DISPLAY_ACTIVE_HEIGHT
            )
    {
        vr[0].out_crop_left = 0;
        vr[0].out_crop_right = DISPLAY_ACTIVE_WIDTH;
        vr[0].out_crop_top = 0;
        vr[0].out_crop_bottom = DISPLAY_ACTIVE_HEIGHT;
    }
    ret = gobj_media_change(h_display, 
                        vr,
                        0,   //FIXME AUDIO
                        FALSE   /*Fast Change Off */
						,0		// Layer
                        );
#if 0 
    g_assert(ret >= 0);
#else
    if(ret < 0)
       g_warning("%s: display_change fail\n", __FUNCTION__);
#endif

    if(_play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_PAUSE 
       || _play_pipe.prev_play_mode == NF_PLAY_PARAM_MODE_NEXT_FRAME
            )
    {
        printf("=== pause zoom: port = %d, %d\n", vr[1].video_channel_id, h_display->disp_channel_num);
        //gobj_media_push_dec_frame_buffer(h_display, vr[1].video_channel_id, 1);//HDI
    }

    _play_pipe.prev_zoom_x = xpos;
    _play_pipe.prev_zoom_y = ypos;
    _play_pipe.prev_zoom_w = zoom_w;
    _play_pipe.prev_zoom_h = zoom_h;

    return TRUE;
}
gboolean nf_play_zoom_stop( void )
{
    int i, ret;
    GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
    gboolean coverts[GOBJ_MAX_MEDIA_PORT];
    GOBJMediaObj *h_display = _play_pipe.h_display;
    //change to previous live config
#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_lock (&_zoom_move_lock);		
    _zoom_move_is_pendding = 0;
#endif
    
    if(_play_pipe.prev_disp_mode != NF_DISPLAY_PLAYBACK_ZOOM)
    {
        g_warning("%s: \n", __FUNCTION__);
#ifdef ENABLE_ZOOM_CMD_SKIP
	    g_static_mutex_unlock (&_zoom_move_lock);
#endif
        return FALSE;
    }

    for(i=0; i<GOBJ_MAX_MEDIA_PORT; i++)
    {
        coverts[i] = FALSE;
    }

    usleep(40000);
    printf("=====================zoom stop vr[0].ch = %d\n", h_display->vr[0][0].video_channel_id);
    ret = gobj_media_mode_change(
                            h_display,
                            GOBJ_MEDIA_USER_PLAY_FULL, 
                            _play_pipe.prev_win_xpos,
                            _play_pipe.prev_win_ypos,
                            _play_pipe.prev_win_width,
                            _play_pipe.prev_win_height, 
                            h_display->disp_channels[0], 
                            coverts,
                            0,
							0 //multi_disp_dev.DispID[0]
                            );    //FIXME Audio
    
    g_assert(ret >= 0);

    //FIXME (IPX0412 ONLY)
    //enable decode all  
    _get_gobj_dec_param(dec_param, FALSE, _play_pipe.prev_vr_index);
    gobj_media_decode_change(h_display, 
                        dec_param,
                        GOBJ_MEDIA_ID_MAIN
                        );
    _play_pipe.prev_disp_mode = NF_DISPLAY_FULL;


#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_unlock (&_zoom_move_lock);
#endif
    usleep(40000);

    return TRUE;
}

int nf_play_zoom_get_pos_sx()
{
    return _play_pipe.prev_zoom_x;
}


int nf_play_zoom_get_pos_sy()
{
    return _play_pipe.prev_zoom_y;
}

int nf_play_zoom_get_pos_ex()
{
    return _play_pipe.prev_zoom_x + _play_pipe.prev_zoom_w;
}

int nf_play_zoom_get_pos_ey()
{
    return _play_pipe.prev_zoom_y + _play_pipe.prev_zoom_h;
}

int nf_play_zoom_get_pos_dx()
{
    return _play_pipe.prev_zoom_w;
}

int nf_play_zoom_get_pos_dy()
{
    return _play_pipe.prev_zoom_h;
}

#ifndef USE_SST_EMUL
gboolean nf_timeline_get( NF_TIMELINE_PARAM *param, gchar **elem, GError **error)
{
    gint result = 0;
   
	#if 0	
		g_message("%s called!!", __FUNCTION__);
	#endif

    g_return_val_if_fail( param != NULL, FALSE);
    g_return_val_if_fail( elem != NULL, FALSE);

#ifdef DEBUG_TIMELINE_GET
    g_message("%s sec[%ld]", __FUNCTION__, param->time_begin.tv_sec );
    g_message("%s res[%d]", __FUNCTION__, param->resolution );
    g_message("%s count[%d]", __FUNCTION__, param->count );
    g_message("%s max_ch[%d]", __FUNCTION__, param->max_channel );
    g_message("%s split[%d]", __FUNCTION__, param->split_channel );
    g_message("%s mask[0x%04x]", __FUNCTION__, param->channel_mask );
#endif

#ifdef ENABLE_WATCHDOG
    //nf_watchdog_kick( NF_WATCHDOG_MEMBER_NR );
#endif  

#ifdef ENABLE_CALENDAR_OPTIMIZE	
	if( param->resolution == 86400 && param->split_channel == 0 
			&& param->max_channel != 1 ) {
		
		// Calendar optimize  choissi 2011-10-11 ???? 3:58:53		
	    g_message("%s sec[%ld]", __FUNCTION__, param->time_begin.tv_sec );
	    g_message("%s res[%d]", __FUNCTION__, param->resolution );
	    g_message("%s count[%d]", __FUNCTION__, param->count );
	    g_message("%s max_ch[%d]", __FUNCTION__, param->max_channel );
	    g_message("%s split[%d]", __FUNCTION__, param->split_channel );
	    g_message("%s mask[0x%04x]", __FUNCTION__, param->channel_mask );
	    		
    	result = sst_tl_get(param->time_begin.tv_sec,
                        86400,
                        param->count,
                        64,
                        0,
                        param->hide,
                        0x8000000000000000ll,
                        elem);                                                                        
	} else {
    	result = sst_tl_get(param->time_begin.tv_sec,
                        param->resolution,
                        param->count,
                        param->max_channel,
                        param->split_channel,
                        param->hide,
                        param->channel_mask,
                        elem);
	}
#else 
    result = sst_tl_get(param->time_begin.tv_sec,
                        param->resolution,
                        param->count,
                        param->max_channel,
                        param->split_channel,
                        param->hide,
                        param->channel_mask,
                        elem);
#endif

    if ( result < 0 ) {
        //g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result));
    }

    return ( result == 0 ) ? 1:0;

}
#endif

int nf_play_set_disp_ratio(NF_PLAY_PARAM_RATIO_E ratio)
{
	int ret = 0;
		
	return ret;
}

int nf_play_pip_hide(int ch)
{
	int ret;

	//ret = gobj_media_pip_ch_hide( multi_disp_dev.DispID[0], ch, GOBJ_MEDIA_PLAYBACK);
	
	if( ret < 0 )
		g_warning("%s result[%d] failed!\n", __FUNCTION__, ret );
	
	return ret;	
}

int nf_play_pip_show(int ch)
{
	int ret;

	//ret = gobj_media_pip_ch_show( multi_disp_dev.DispID[0], ch, GOBJ_MEDIA_PLAYBACK);
	
	if( ret < 0 )
		g_warning("%s result[%d] failed!\n", __FUNCTION__, ret );
	
	return ret;	
}

#define FEATURE_NO_DROP_FRAME   1   //for P-reference Decoder 

#define DEBUG_NO_RECORD	0

#define DEBUG_NO_SST    0
#define DEBUG_DROP_CNT  1
#define TEST_NO_DROP    0   //For the HTM playback flickering bug, when it is  "foward, full playback mode", do not drop it.
#define DEBUG_TS        0

//#define _DEBUG_MSG
/* Error codes */
#define SST_ERR_NONE            0   /**< No error */
#define SST_ERR_SEQERR          1   /**< Call sequence error */
#define SST_ERR_INVPARAM        2   /**< Invalid parameter */
#define SST_ERR_INVSTM          3   /**< Invalid stream id or not present */
#define SST_ERR_STMBSY          4   /**< Improper stream id or already opened */
#define SST_ERR_STMERR          5   /**< The stream has internal errors */
#define SST_ERR_DATALOCKED      6   /**< Some of the data are locked */
#define SST_ERR_DISKFULL        7   /**< Disks are full */
#define SST_ERR_NODISK          8   /**< There is no disk */
#define SST_ERR_NOMEM           9   /**< There is not enough memory */
#define SST_ERR_NOSPACE         10  /**< There is not enough space on disks */
#define SST_ERR_IO              11  /**< An I/O error is occurred */
#define SST_ERR_TIMEOUT         12  /**< Request timed out */
#define SST_ERR_NODATA          13  /**< There is no data */
#define SST_ERR_ENDDATA         14  /**< End data is reached */
#define SST_ERR_EMPTY           15  /**< The stream is empty */
#define SST_ERR_OVERLAP         16  /**< The (playback) stream is overlapped */
#define SST_ERR_MAX             16  /**< Maximum value (!= count) */


#define JUMP_INTERVAL_MS	2500

#define BASE_NORECORD_INTERVAL_MS     2500 /* 2500 : 20130911 modify for S1 IPX */
#define BASE_NORECORD_INTERVAL_MS_2  5000
#define BASE_NORECORD_INTERVAL_MS_4  10000
#define BASE_NORECORD_INTERVAL_MS_8  10000
#define BASE_NORECORD_INTERVAL_MS_16  20000
#define BASE_NORECORD_INTERVAL_MS_32  40000
#define BASE_NORECORD_INTERVAL_MS_64  80000

#define Uint8 unsigned char
#define Uint16 unsigned short
#define Uint32 unsigned int
#define Uint64 unsigned long long

/*
 * play functions
 */
#include <fcntl.h>		// defines open, read, write methods
#include <unistd.h>		// defines close and sleep methods
#include <sys/mman.h>		// defines mmap method
#include <sys/ioctl.h>		// defines ioctl method

#include <asm/types.h>          // standard typedefs required by play header
#include <linux/videodev2.h>    // play driver definitions

/* FIXME END */

enum _pb_pipe {
	PLAYBACK_PIPE_OK = 0,
	PLAYBACK_PIPE_FAIL = 1	
};

#define NFTIME_TO_SSTTIME(ts, tsl)     ((guint64)(ts)*SST_SECOND + (guint64)(tsl)*SST_MSECOND*5)


static void gsttime_to_nftime(icodec_header_t *ih, guint64 time)
{
    ih->timeStamp = time/SST_SECOND;
    
    ih->timeStampL = (time -(guint64)ih->timeStamp*SST_SECOND) / (SST_MSECOND*5);
}


static gboolean find_jumptime(gint dir, guint64 eos_mask, guint64 *next_time, int ch_num, guint64 *jump_time)
{
        int i;
        if(dir == SST_SRC_DIR_FORWARD) {
            *jump_time = (guint64)-1;
            for(i=0; i < ch_num; i++) {
                if( ! (eos_mask  & (1ULL << i)) ) {
                    if(*jump_time > next_time[i])
                        *jump_time = next_time[i];
                }
            }
        } else {
            *jump_time = 0;
            for(i=0; i < ch_num; i++) {
                if( ! (eos_mask  & (1ULL << i)) ) {
                    if(*jump_time < next_time[i])
                        *jump_time = next_time[i];
                }
            }
        }
        return TRUE;
}

void _icodec_data_free(icodec_header_t *ih) 
{
#if 1
	ICMEM_free(ih, ih->frameSize + sizeof(icodec_header_t));
	ih = NULL;
#else
    GobjBuddyBuffer *buf;
    int length;

    length = ih->frameSize + sizeof(icodec_header_t);
    buf = gobj_buddy_buffer_new();
    gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih);
    gobj_buddy_buffer_buf_set_size(buf, length);
    g_object_unref(buf);
#endif
}


static inline GObject  *_list_buffer_push(GobjListBuffer  *list_buf, GObject  *buf)
{
 //   g_warning("SSTSRC: push priv = %d\n", GST_NF_BUDDY_BUFFER(buf)->priv);
	gobj_list_buffer_push(list_buf, buf);
}

gint
_get_first_iframe(GObject  **out_stream_buf, GTimeVal *out_time, gint ch, guint64 begin_time, guint64 end_time)
{
    int sid;      
    icodec_header_t *ih_frame = NULL;
    int r, length;

    *out_stream_buf = NULL;
    sid = sst_play_open_ex(0, //pr_id
                            //ch + SECOND_STM_OFFSET, //hdi also use second frame
                            ch,
                            1, //rate
                            SST_SRC_DIR_FORWARD, //dir
                            0, //rate control
                            1, //hide 
                            begin_time,
                            end_time, 
                            begin_time, 
                            0, 
                            1,  //iframe only
                            1   //only 1 frame
                            );

    
    if(sid < 0)
    {
        g_message("%s: there is no frame\n", __FUNCTION__);
        return -1;
    }

    //g_message("call play_wait_frame = %d\n", sid);
    r = sst_play_wait_frame(&sid, 1, 1, 3000);   //Recommended by HTM
    if(r < 0) {
		sst_play_close(sid);
        return -1;
	}

    while(1)
    {
       r = sst_play_get_frame (sid, &ih_frame,5000);  //wait 5second
       if(r<0)
       {
           *out_stream_buf = NULL;
           sst_play_close(sid);
           return -1;
       }
       
       if(ih_frame->frameType == FRAME_TYPE_END)
       {
          _icodec_data_free(ih_frame);
           *out_stream_buf = NULL;
           sst_play_close(sid);
           return -1;
       }
 //      else if(ih_frame->frameType == FRAME_TYPE_I || ih_frame->frameType == FRAME_TYPE_RI)
       else if(ih_frame->frameType == FRAME_TYPE_I)
       {
            length = ih_frame->frameSize + sizeof(icodec_header_t);
            *out_stream_buf = (GObject  *)gobj_buddy_buffer_new();
            gobj_buddy_buffer_buf_set_addr(*out_stream_buf, (guint8 *)ih_frame);
            gobj_buddy_buffer_buf_set_size(*out_stream_buf, length);
            gobj_buddy_buffer_set_timestamp(*out_stream_buf, NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL));
            out_time->tv_sec = ih_frame->timeStamp;
            out_time->tv_usec = ih_frame->timeStampL * 5 * 1000;
           break;
       }
       else
       {
          _icodec_data_free(ih_frame);
       }
    }
        
close_exit:
    sst_play_close(sid);

    return 0;
}

gint
_get_first_iframe_with_size(GObject  **out_stream_buf, GTimeVal *out_time, gint ch, guint64 begin_time, guint64 end_time, gint *width, gint *height)
{
    int sid;      
    icodec_header_t *ih_frame = NULL;
    int r, length;

    *out_stream_buf = NULL;
    sid = sst_play_open_ex(0, //pr_id
                            //ch + SECOND_STM_OFFSET, //hdi also use second frame
                            ch,
                            1, //rate
                            SST_SRC_DIR_FORWARD, //dir
                            0, //rate control
                            1, //hide 
                            begin_time,
                            end_time, 
                            begin_time, 
                            0, 
                            1,  //iframe only
                            1   //only 1 frame
                            );

    
    if(sid < 0)
    {
        g_message("%s: there is no frame\n", __FUNCTION__);
        return -1;
    }

    //g_message("call play_wait_frame = %d\n", sid);
    r = sst_play_wait_frame(&sid, 1, 1, 3000);   //Recommended by HTM
    if(r < 0) {
		sst_play_close(sid);
        return -1;
	}

    while(1)
    {
       r = sst_play_get_frame (sid, &ih_frame,5000);  //wait 5second
       if(r<0)
       {
           *out_stream_buf = NULL;
           sst_play_close(sid);
           return -1;
       }
       
       if(ih_frame->frameType == FRAME_TYPE_END)
       {
          _icodec_data_free(ih_frame);
           *out_stream_buf = NULL;
           sst_play_close(sid);
           return -1;
       }
 //      else if(ih_frame->frameType == FRAME_TYPE_I || ih_frame->frameType == FRAME_TYPE_RI)
       else if(ih_frame->frameType == FRAME_TYPE_I)
       {
            length = ih_frame->frameSize + sizeof(icodec_header_t);
            *out_stream_buf = (GObject  *)gobj_buddy_buffer_new();
            gobj_buddy_buffer_buf_set_addr(*out_stream_buf, (guint8 *)ih_frame);
            gobj_buddy_buffer_buf_set_size(*out_stream_buf, length);
            gobj_buddy_buffer_set_timestamp(*out_stream_buf, NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL));
			_get_resol_size_icodec(ih_frame->resolution, (guint *)width, (guint *)height);
            out_time->tv_sec = ih_frame->timeStamp;
            out_time->tv_usec = ih_frame->timeStampL * 5 * 1000;
           break;
       }
       else
       {
          _icodec_data_free(ih_frame);
       }
    }
        
close_exit:
    sst_play_close(sid);

    return 0;
}


FILE *pFile = NULL;
gchar fileName[256];
static int cnt_f=0;
int start_f=0;
	
icodec_header_t *ih_delta = NULL;
icodec_header_t *ih_frame = NULL;

static int
playback_pipe_process ()
{
	GobjListBuffer *list_buf = NULL;
	int result;
	int length;
	int i;
	//   gboolean isStart, isEnd;
	GObject *buf;
	guint64 next_time[MAX_SST_PB_CHANNEL];
	guint64 jump_time;
	guint64 audio_preroll_time = 0;

	gboolean isJump = FALSE;

	GTimeVal tv,tv_n;
	unsigned long gap;
	gettimeofday(&tv, NULL);

	pb_data.last_render_checktime = 0;


test:
        list_buf = (GobjListBuffer *)(gobj_list_buffer_new());
        if(G_UNLIKELY(list_buf == NULL)) {
            g_warning("NFSSTSRC: cannot alloc list_buf\n");
            goto eos;
        }

next:


	if(pb_data.is_prev_start == TRUE) {

		if(pb_data.is_start_overlapped == TRUE)
		{
			pb_data.is_prev_start = FALSE;
			goto eos;
		}
		//20110809  endvideo timestamp bug fixed.   0010582
		if(pb_data.is_start_endvideo)
		{
			pb_data.is_prev_start = FALSE;
			goto eos;
		}

		if(pb_data.eos_mask == -1ULL)
			pb_data.base_time = 0;
		pb_data.is_prev_start = FALSE;
		pb_data.prev_eos_mask = pb_data.eos_mask;
		pb_data.prev_norecord_mask = pb_data.norecord_mask;
		pb_data.prev_overlapped_mask = pb_data.overlapped_mask;

	}

	for(i=0; i<pb_data.ch_num; i++) {
		gboolean last_is_delta = FALSE;
		int ri_frame_count = 0;
		gboolean is_audio = FALSE;
		
		ih_delta = NULL;
		ih_frame = NULL;

		if(G_UNLIKELY(pb_data.ch[i] >= SST_AUDIO_CHANNEL_BASE)) {
			is_audio = TRUE;
			audio_preroll_time = SST_SRC_AUDIO_PREROLL_TIME;
		}


		if(  (pb_data.eos_mask |pb_data.overlapped_mask) & (1ULL << i) ) { 
			continue;
		}
		

		while(1) {
			icodec_header_t ih_check;
			icodec_header_t *ih_temp;
			Uint8 overlap_status;
			int check_cnt = 0;
			
			if(sst_play_check_overlap(pb_data.sid[i],&overlap_status) == 0) {
				if(overlap_status == 1) {
					if( pb_data.mode == SST_SRC_MODE_PANORAMA2 )
					{						
						pb_data.overlapped_mask |= (1ULL<<i);						
						break;							
					}else{
						g_warning("overlapped  ch[%d] sid[%d]", i, pb_data.sid[i]);
						goto eos;
					}
				}
			}
			
			while (1) {               
				result = sst_play_check_frame( pb_data.sid[i], &ih_check);
				check_cnt ++;            
				if(G_UNLIKELY (result == -SST_ERR_EMPTY))
					usleep(10000);
				else 
					break;
			}
                
			if(G_UNLIKELY (result == -SST_ERR_ENDDATA)) {
				pb_data.eos_mask |= (1ULL<<i);
				break;
			}
			else if(G_UNLIKELY(result < 0)) {
				g_warning("unexpected result - sst_play_check_frame\n");
				pb_data.eos_mask |= (1ULL<<i);
				break;
			}

			if(G_UNLIKELY(ih_check.frameType == FRAME_TYPE_START)) {
				result = sst_play_get_frame (pb_data.sid[i], &ih_temp,5000);  //wait 5second
				if(result < 0) {

					if(result == -SST_ERR_EMPTY)
					{
						g_warning("!! sst_play_get_frame: Timeout 5000msec \n");
						goto eos;
					}
					else if (result == -SST_ERR_OVERLAP)
					{
						g_warning("!! sst_play_get_frame: overlap was occured\n");
						goto eos;
					}
					g_warning("SSTSRC: get_frame error after check_frame!!\n");
					goto eos;
				}
				_icodec_data_free(ih_temp);
				if(G_UNLIKELY(is_audio)) {
					audio_preroll_time = 0;
				}

				continue;
			}
				
			if(G_UNLIKELY(ih_check.frameType == FRAME_TYPE_END)) {
				result = sst_play_get_frame (pb_data.sid[i], &ih_temp,5000);  //wait 5second
				if(result < 0) {
					if(result == -SST_ERR_EMPTY)
					{
						g_warning("!! sst_play_get_frame: Timeout 5000msec \n");
						goto eos;
					}
					else if (result == -SST_ERR_OVERLAP)
					{
						g_warning("!! sst_play_get_frame: overlap was occured\n");
						goto eos;
					}
					g_warning("SSTSRC: get_frame error after check_frame!!\n");
					goto eos;
				}

				_icodec_data_free(ih_temp);
				continue;
			}

               	next_time[i] = NFTIME_TO_SSTTIME(ih_check.timeStamp, ih_check.timeStampL);

			if(G_UNLIKELY(is_audio)) {
				if( (pb_data.direction == SST_SRC_DIR_BACKWARD)) 
					next_time[i] -= audio_preroll_time;
				else 
					next_time[i] += audio_preroll_time;
			}

			if(pb_data.mode == SST_SRC_MODE_PANORAMA2) //20081219
				next_time[i] -= pb_data.time_offset*i;


			if( (pb_data.direction == SST_SRC_DIR_BACKWARD)) {
                    	if(ih_check.frameType == FRAME_TYPE_RI) {
                        		if(ih_delta) {
                            		//free prev ih_delta
                            		_icodec_data_free(ih_delta);
                            		pb_data.drop_cnt[i]++;
                        		}
                        		result = sst_play_get_frame (pb_data.sid[i], &ih_delta, 5000);  //wait 5second
                        		if(result < 0) {
                            		if(result == -SST_ERR_EMPTY)
                            		{
                                			g_warning("!! sst_play_get_frame: Timeout 5000msec \n");
                                			goto eos;
                            		}
                            		else if (result == -SST_ERR_OVERLAP)
                            		{
                                			g_warning("!! sst_play_get_frame: overlap was occured\n");
                                			goto eos;
                            		}
                            		g_warning("SSTSRC: get_frame error after check_frame!!\n");
                            		goto eos;
                        		}
                        		g_assert(result >= 0);
					last_is_delta = TRUE;
					ri_frame_count++;
                		} 
				else {    //I or P frame
                			if( next_time[i] >= pb_data.running_time-pb_data.vtick_interval_usec*SST_USECOND/2) {
                            		if(ih_frame) {
                                			//free prev ih_frame
                                			_icodec_data_free(ih_frame);
                                			pb_data.drop_cnt[i]++;
                            		}
                            		result = sst_play_get_frame (pb_data.sid[i], &ih_frame, 5000);  //wait 5second
                           		if(result < 0) {
	                                		if(result == -SST_ERR_EMPTY)
	                                		{
	                                    		g_warning("!! sst_play_get_frame: Timeout 5000msec \n");
	                                    		goto eos;
	                                		}
	                                		else if (result == -SST_ERR_OVERLAP)
	                                		{
	                                    		g_warning("!! sst_play_get_frame: overlap was occured\n");
	                                    		goto eos;
	                                		}
	                                		g_warning("SSTSRC: get_frame error after check_frame!!\n");
	                                		goto eos;
	                            	}

						if(ih_frame->frameType == FRAME_TYPE_I && is_audio) {
							ih_frame->frameType = FRAME_TYPE_AUDIO;
						}
	                            	g_assert(result >= 0);
	                            	last_is_delta = FALSE;

                        		} 
					else {
	                            	break;
	                        	}
                    	}
			} 
			else {  //FORWARD
   
                          if( next_time[i] <= pb_data.running_time+pb_data.vtick_interval_usec*SST_USECOND/2) {     //XXX vtick_interval*GST_USECOND/2 is running time correction value for playback flickering.
                          		if(ih_check.frameType == FRAME_TYPE_I || ih_check.frameType == FRAME_TYPE_AUDIO|| ih_check.frameType == FRAME_TYPE_RI) {
                                		if(ih_delta) {
                                    		//free prev ih_delta
                                    		_icodec_data_free(ih_delta);
                                    		pb_data.drop_cnt[i]++;
                                		}
                                		result = sst_play_get_frame (pb_data.sid[i], &ih_delta, 5000);  //wait 5second
                                		if(result < 0) {
                                    		if(result == -SST_ERR_EMPTY)
                                    		{
                                        		g_warning("!! sst_play_get_frame: Timeout 5000msec \n");
                                        		goto eos;
                                    		}
                                    		else if (result == -SST_ERR_OVERLAP)
                                    		{
                                        		g_warning("!! sst_play_get_frame: overlap was occured\n");
                                        		goto eos;
                                    		}
                                    		g_warning("SSTSRC: get_frame error after check_frame!!\n");
                                    		goto eos;
                                		}
										
	                                	if(ih_delta->frameType == FRAME_TYPE_I && is_audio) {
	                                    	ih_delta->frameType = FRAME_TYPE_AUDIO;
	                                	}
									
                                		g_assert(result >= 0);
                                		last_is_delta = TRUE;
#if 0
if(ih_delta->chan ==0){
				
				gchar *data = (((char *)ih_delta)+sizeof(icodec_header_t));

	//			g_assert( ch < NUM_ACTIVE_CH<<1 );

		//		if ( ch == 4 || ch == 19)
			//	{

				//if ( cnt[ch]++ < 75 )
				//{
//						dump_icodec_header( "kti", pheader);		

if(start_f){
					fflush(pFile);
					fclose(pFile);
					cnt_f++;

}

					sprintf(fileName, "ITX_%03d%c.h264", cnt_f, (ih_delta->frameType == FRAME_TYPE_I ? 'i': 'p'));
					
					pFile = fopen( fileName, "wb");
					
					fwrite( data, ih_delta->frameSize, 1, pFile);

					start_f=1;
				//}
				//}
}
#endif



		                                #if 0//DEBUG_TS   //debug
										if(i ==0)
		                                g_message("[%s:%d]ch:%2d,ft:%2d,size %d ,ts:%d,tsl:%3d,nt:%lld,rt:%lld,vtick:%d",
		                                            __FUNCTION__, __LINE__,
		                                            ih_delta->chan, ih_delta->frameType, ih_delta->frameSize, 
		                                            ih_delta->timeStamp, ih_delta->timeStampL,
		                                            next_time[i], pb_data.running_time, pb_data.vtick_interval_usec );
		                                #endif
                            	} 
					else {    
                            			//Pframe
                               		if(ih_frame) {
#if FEATURE_NO_DROP_FRAME
                                    		break;
#endif
                                    		//free prev ih_frame
                                    		_icodec_data_free(ih_frame);
                                    		pb_data.drop_cnt[i]++;
                                		}
                                		result = sst_play_get_frame (pb_data.sid[i], &ih_frame, 5000);  //wait 5second
                                		if(result < 0) {
                                    		if(result == -SST_ERR_EMPTY)
                                    		{
                                        		g_warning("!! sst_play_get_frame: Timeout 5000msec \n");
                                        		goto eos;
                                    		}
                                    		else if (result == -SST_ERR_OVERLAP)
                                    		{
                                        		g_warning("!! sst_play_get_frame: overlap was occured\n");
                                        		goto eos;
                                    		}
	                                    	g_warning("SSTSRC: get_frame error after check_frame!!\n");
                                    		goto eos;
                                		}
                                		g_assert(result >= 0);
                                		last_is_delta = FALSE;
#if 0
if(ih_frame->chan ==0){
				
				gchar *data = (((char *)ih_frame)+sizeof(icodec_header_t));

	//			g_assert( ch < NUM_ACTIVE_CH<<1 );

		//		if ( ch == 4 || ch == 19)
			//	{

				//if ( cnt[ch]++ < 75 )
				//{
//						dump_icodec_header( "kti", pheader);		

					
					fwrite( data, ih_frame->frameSize, 1, pFile);

				//}
				//}
}
#endif
                                
                                #if 0//DEBUG_TS   //debug
								if(i ==0)
                                g_message("[%s:%d]ch:%2d,ft:%2d,size %d, ts:%d,tsl:%3d,nt:%lld,rt:%lld,vtick:%d",
                                            __FUNCTION__, __LINE__,
                                            ih_frame->chan, ih_frame->frameType, ih_frame->frameSize, 
                                            ih_frame->timeStamp, ih_frame->timeStampL,
                                            next_time[i], pb_data.running_time, pb_data.vtick_interval_usec );
                                #endif

                            	}
#if FEATURE_NO_DROP_FRAME
                            	if(pb_data.is_iframe_open == FALSE)    //20110711 : x64 play bug fixed. (time and video mismatching bug)
                               		break;
#endif
                        	} 
				else {
                            	//time to get frame!
                            	//if(i ==0)
						//			printf("runningtime skip rt:%lld !!!\n",pb_data.running_time);
                            	break;
                        	}
                	}
           	}   //while(1) - get most recent frame 
 
		//2nd stage
             if( (pb_data.direction == SST_SRC_DIR_BACKWARD)) {
             		if(last_is_delta) {
                        	if(ih_frame) {  //I, P frame
                            	//we already decoded it. send refframe
                            	if(pb_data.prev_ri_time[i] == next_time[i]) {
                                		length = ih_frame->frameSize + sizeof(icodec_header_t);
                                		buf = (GObject  *)gobj_buddy_buffer_new();
                                		//FIXME - if (buf == NULL)
						gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_frame);
						gobj_buddy_buffer_buf_set_size(buf, length);
						gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);
						//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch

                 				pb_data.cur_frame_time[i] = NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime > pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
                                		g_assert(ih_frame->chan < MAX_SST_PB_CHANNEL);

                                		_list_buffer_push(list_buf, buf);
                                 
                            	} 
					else {
                                		if(ri_frame_count < 2) {
							//send frame
							length = ih_frame->frameSize + sizeof(icodec_header_t);
							buf = gobj_buddy_buffer_new();
							//FIXME - if (buf == NULL)
							gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_frame);
							gobj_buddy_buffer_buf_set_size(buf, length);
							gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);
							//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch

							g_assert(ih_frame->chan < MAX_SST_PB_CHANNEL);
								pb_data.cur_frame_time[i] = NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime > pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
							_list_buffer_push(list_buf, buf);
                                		}
                            	}

                        	}
							
                        	if(ih_delta) {  //RIframe
					//send frame
					length = ih_delta->frameSize + sizeof(icodec_header_t);
					buf = gobj_buddy_buffer_new();
					//FIXME - if (buf == NULL)
					gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_delta);
					gobj_buddy_buffer_buf_set_size(buf, length);
					gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

					pb_data.prev_ri_time[i] = NFTIME_TO_SSTTIME(ih_delta->timeStamp, ih_delta->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime > pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
					//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
					g_assert(ih_delta->chan < MAX_SST_PB_CHANNEL);

					_list_buffer_push(list_buf, buf);
                        	}
                    }
			else {    //last is notdelta
                        	if(ih_delta) {  //RIframe
					//send frame
					length = ih_delta->frameSize + sizeof(icodec_header_t);
					buf = gobj_buddy_buffer_new();
					//FIXME - if (buf == NULL)
					gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_delta);
					gobj_buddy_buffer_buf_set_size(buf, length);
					gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

					pb_data.prev_ri_time[i] = NFTIME_TO_SSTTIME(ih_delta->timeStamp, ih_delta->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime > pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
					//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
					g_assert(ih_delta->chan < MAX_SST_PB_CHANNEL);
					_list_buffer_push(list_buf, buf);
                        	}

                        	if(ih_frame) {  //I, P frame
                            	//we already decoded it. send refframe
                            	if(pb_data.prev_ri_time[i] == next_time[i]) {
						//TODO
						//not decoding this, send ref frame
						//send frame
						length = ih_frame->frameSize + sizeof(icodec_header_t);
						buf = gobj_buddy_buffer_new();
						//FIXME - if (buf == NULL)
						gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_frame);
						gobj_buddy_buffer_buf_set_size(buf, length);
						gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

						//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
						g_assert(ih_frame->chan < MAX_SST_PB_CHANNEL);
							pb_data.cur_frame_time[i] = NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime > pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
						_list_buffer_push(list_buf, buf);
                                 
                            	} 
					else {
						//send frame
						length = ih_frame->frameSize + sizeof(icodec_header_t);
						buf = gobj_buddy_buffer_new();
						//FIXME - if (buf == NULL)
						gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_frame);
						gobj_buddy_buffer_buf_set_size(buf, length);
						gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

						//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
						g_assert(ih_frame->chan < MAX_SST_PB_CHANNEL);
							pb_data.cur_frame_time[i] = NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime > pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
						_list_buffer_push(list_buf, buf);
                            	}

                        	}   //if_frame
                    }
		}
		else { //forward
              	if (ih_delta)
                    {
                        	if(ih_delta->frameType == FRAME_TYPE_RI) {
					length = ih_delta->frameSize + sizeof(icodec_header_t);
					buf = gobj_buddy_buffer_new();
					//FIXME - if (buf == NULL)
					gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_delta);
					gobj_buddy_buffer_buf_set_size(buf, length);
					gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

					//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
					g_assert(ih_delta->chan < MAX_SST_PB_CHANNEL);
					g_warning("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! delta\n");
					  _list_buffer_push(list_buf, buf);
                        	}
				else {    //I-frame
					length = ih_delta->frameSize + sizeof(icodec_header_t);
					buf = gobj_buddy_buffer_new();
					//FIXME - if (buf == NULL)
					gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_delta);
					gobj_buddy_buffer_buf_set_size(buf, length);
					gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

					//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
					g_assert(ih_delta->chan < MAX_SST_PB_CHANNEL);
						pb_data.cur_frame_time[i] = NFTIME_TO_SSTTIME(ih_delta->timeStamp, ih_delta->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime < pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
					_list_buffer_push(list_buf, buf);

#if 0 //DEBUG_TS   //debug
					g_message("[%s:%d]ch:%2d,ft:%2d,ts:%d,tsl:%3d,nt:%lld,rt:%lld,vtick:%d",
					            __FUNCTION__, __LINE__,
					            ih_delta->chan, ih_delta->frameType, 
					            ih_delta->timeStamp, ih_delta->timeStampL,
					            next_time[i], pb_data.running_time, pb_data.vtick_interval_usec );
#endif

                        	}
                    }
                    if(!last_is_delta)
                    {
                        	if(ih_frame) {
					length = ih_frame->frameSize + sizeof(icodec_header_t);
					buf = gobj_buddy_buffer_new();
					//FIXME - if (buf == NULL)
					gobj_buddy_buffer_buf_set_addr(buf,(guint8 *)ih_frame);
					gobj_buddy_buffer_buf_set_size(buf, length);
					gobj_buddy_buffer_set_timestamp(buf, pb_data.running_time);

					//GST_NF_BUDDY_BUFFER(buf)->priv = pb_data.disp_ch[i];         //XXX set ch
					g_assert(ih_frame->chan < MAX_SST_PB_CHANNEL);
						pb_data.cur_frame_time[i] = NFTIME_TO_SSTTIME(ih_frame->timeStamp, ih_frame->timeStampL);
						if((pb_data.last_render_checktime == 0) || (pb_data.last_render_checktime < pb_data.cur_frame_time[i]))
							pb_data.last_render_checktime = pb_data.cur_frame_time[i];
					_list_buffer_push(list_buf, buf);
#if 0   //debug
					g_message("[%s:%d]ch:%2d,ft:%2d,ts:%d,tsl:%3d,nt:%lld,rt:%lld,vtick:%d",
					            __FUNCTION__, __LINE__,
					            ih_frame->chan, ih_frame->frameType, 
					            ih_frame->timeStamp, ih_frame->timeStampL,
					            next_time[i], pb_data.running_time, pb_data.vtick_interval_usec );
#endif
                        	}
                    }
			else {
                       	if(ih_frame) {
          				_icodec_data_free(ih_frame);
                        	}
                    }
		}
                

		//XXX ???
		if(G_UNLIKELY(is_audio)) {
			if( (pb_data.direction == SST_SRC_DIR_BACKWARD)) 
				next_time[i] += audio_preroll_time;
			else 
				next_time[i] -= audio_preroll_time;
		}
			
			
		//if(pb_data.usleep1)
			//g_usleep(pb_data.usleep1);

	}
 
	if(pb_data.last_render_checktime != 0)
		pb_data.last_render_time = pb_data.last_render_checktime;
 

	if( (pb_data.eos_mask | pb_data.overlapped_mask)  == -1ULL) {
		goto eos;
	}

	find_jumptime(pb_data.direction, (pb_data.eos_mask | pb_data.overlapped_mask), next_time, pb_data.ch_num, &jump_time);

	//printf("jump_time:%lld nt %lld \n", jump_time, next_time[0]);

#if 0
       g_print("SSTSRC: *********** cur -prev  %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(jump_time - src->running_time));
       g_print("SSTSRC: *********** jump  %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(jump_time));
       g_print("SSTSRC: *********** running  %"GST_TIME_FORMAT"\n", GST_TIME_ARGS( (jump_time + src->rate*src->vtick_interval_usec*GST_USECOND)- src->running_time ));
#endif

	for(i=0; i<MAX_SST_PB_CHANNEL; i++) {

		if( ! ( (pb_data.eos_mask|pb_data.overlapped_mask) & (1ULL << i)) ) {
			if( (pb_data.direction == SST_SRC_DIR_FORWARD 
						&& next_time[i] > pb_data.cur_frame_time[i] + pb_data.norecord_interval_ms*SST_MSECOND)
				  || (pb_data.direction == SST_SRC_DIR_BACKWARD
						&& next_time[i] < pb_data.cur_frame_time[i] - pb_data.norecord_interval_ms*SST_MSECOND) ) 
			{
			//printf("NORECORD i %d %lld %lld ???\n",i, next_time[i] , pb_data.cur_frame_time[i]);
				pb_data.norecord_mask |= (1ULL<<i);

	                    //20110809 for NORECORD DISPLAY BUG 0010406
	                    if(pb_data.direction == SST_SRC_DIR_BACKWARD && pb_data.cur_frame_time[i] == (guint64)-1)
	                    {
					if(next_time[i] >= pb_data.running_time - pb_data.norecord_interval_ms*SST_MSECOND)
						pb_data.norecord_mask &= ~(1ULL<<i);
	                    }
	                    if(pb_data.direction == SST_SRC_DIR_FORWARD && pb_data.cur_frame_time[i] == (guint64)0)
	                    {
					if(next_time[i] <= pb_data.running_time + pb_data.norecord_interval_ms*SST_MSECOND)
						pb_data.norecord_mask &= ~(1ULL<<i);
	                    }

			} 
			else 
			{
				pb_data.norecord_mask &= ~(1ULL<<i);
			}
		}
	}

       
//
/*
        gobj_buddy_buffer_buf_set_addr(list_buf, (guint8 *)NULL);
        gobj_buddy_buffer_buf_set_size(list_buf, 0);

        if(pb_data.direction == SST_SRC_DIR_FORWARD) {
           gobj_buddy_buffer_set_timestamp(list_buf, pb_data.running_time - pb_data.base_time);
        } else {
           gobj_buddy_buffer_set_timestamp(list_buf,  pb_data.base_time - pb_data.running_time);
        }
if(pb_data.direction == SST_SRC_DIR_FORWARD)
	printf("gobj_buddy_buffer_set_timestamp %d \n",pb_data.running_time - pb_data.base_time);
else
	printf("gobj_buddy_buffer_set_timestamp %d \n",pb_data.base_time - pb_data.running_time);
        */

        if((pb_data.direction == SST_SRC_DIR_FORWARD
                && jump_time > pb_data.running_time + (guint64)(pb_data.norecord_interval_ms*SST_MSECOND))
            || (pb_data.direction == SST_SRC_DIR_BACKWARD
               && jump_time < pb_data.running_time - (guint64)(pb_data.norecord_interval_ms*SST_MSECOND))){
        	pb_data.is_prev_start = TRUE;
 

            	if(pb_data.direction == SST_SRC_DIR_FORWARD) {
               	pb_data.base_time = (jump_time - pb_data.running_time) + pb_data.base_time;
              	// src->running_time = jump_time + src->rate*src->vtick_interval_usec*GST_USECOND+src->vtick_interval_usec*GST_USECOND/2;    //FIXME NTSC or PAL?
              	pb_data.running_time = jump_time + pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;    //FIXME NTSC or PAL?
            	}
            	else {
               	pb_data.base_time = pb_data.base_time - (pb_data.running_time - jump_time);
 //              src->running_time = jump_time - src->rate*src->vtick_interval_usec*GST_USECOND-src->vtick_interval_usec*GST_USECOND/2;
               	pb_data.running_time = jump_time - pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;
            	}
       }
	else {
             if(pb_data.direction == SST_SRC_DIR_FORWARD) {
                	pb_data.running_time += pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;
            	} 
		else {
                pb_data.running_time -= pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;
            }
       }
	
       if(G_UNLIKELY(list_buf->buffer_list == NULL && pb_data.is_nextframe_mode)) {
		if(pb_data.direction == SST_SRC_DIR_FORWARD) {
			// src->running_time = jump_time + src->rate*src->vtick_interval_usec*GST_USECOND+src->vtick_interval_usec*GST_USECOND/2;    //FIXME NTSC or PAL?
			pb_data.running_time = jump_time + pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;
		}
		else {
			//              src->running_time = jump_time - src->rate*src->vtick_interval_usec*GST_USECOND-src->vtick_interval_usec*GST_USECOND/2;
			pb_data.running_time = jump_time - pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;
		}
		// g_warning("sstsrc: buffer_list is NULL\n");

		goto next;    
	}

	{
		if(pb_data.eos_mask != pb_data.prev_eos_mask 
			|| pb_data.norecord_mask != pb_data.prev_norecord_mask
			|| pb_data.overlapped_mask != pb_data.prev_overlapped_mask ) {
			gboolean ret;
			pb_data.prev_eos_mask = pb_data.eos_mask;
			pb_data.prev_norecord_mask = pb_data.norecord_mask;
			pb_data.prev_overlapped_mask = pb_data.overlapped_mask;
		}
	}
//printf("push_buffers ~~~~~~~\n");
	gobj_media_push_buffers(GOBJ_ID_PLAYBACK, list_buf);
	
	pb_data.isAfterSeek = 0;
/*
	if(pb_data.is_nextframe_mode && pb_data.is_first_frame == FALSE)
	{
		g_mutex_lock(pb_data.block_lock);
		g_cond_wait(pb_data.cond, pb_data.block_lock);
		g_mutex_unlock(pb_data.block_lock);
	}
	*/
	
	pb_data.is_first_frame = FALSE;


	if(playback_pipe_on){
		// running_time fixed
		usleep(pb_data.vtick_interval_usec);
		gettimeofday(&tv_n, NULL);
		gap = (tv_n.tv_sec - tv.tv_sec)*1000000 + tv_n.tv_usec - tv.tv_usec;
		if(gap > pb_data.vtick_interval_usec){
			if(pb_data.direction == SST_SRC_DIR_FORWARD && (pb_data.rate == 1)) {
				pb_data.running_time += (gap-pb_data.vtick_interval_usec)*pb_data.rate*SST_USECOND;
			}
			else{
			//	pb_data.running_time -= (gap-pb_data.vtick_interval_usec)*pb_data.rate*SST_USECOND;
			}
		}
	
	}
		
	//if(pb_data.usleep2)
		//g_usleep(pb_data.usleep2);
	
    	return PLAYBACK_PIPE_OK;

could_not_read:
	{
		return PLAYBACK_PIPE_FAIL;
	}
unexpected_eos:
	{
		return PLAYBACK_PIPE_FAIL;
	}
eos:
	{
		g_object_unref(list_buf);
		g_message("nfsstsrc: eos\n");
	   	return PLAYBACK_PIPE_FAIL;
	}
}

void
playback_pipe (gpointer args)
{
	int res;
	while(1){
		if(playback_pipe_on ==1){
			playback_pipe_state = 1;

			res = playback_pipe_process();
			if(res == PLAYBACK_PIPE_FAIL){
				playback_pipe_on = 0;
			}
			//else
				//g_usleep(10000);
		}
		else if(playback_pipe_on == 2){
			res = playback_pipe_process();
			playback_pipe_on = 0;
			playback_pipe_state = 0;
		}
		else{
			playback_pipe_state = 0;
			g_usleep(10000);
		}
	}
}



		

struct timespec start; 	     
struct timespec end;
double delta;

static gboolean
playback_pipe_stop ()
{
	int ret = 0, i;

	g_mutex_lock(pb_data.block_lock);
	g_cond_signal(pb_data.cond);
	g_mutex_unlock(pb_data.block_lock);

	clock_gettime(CLOCK_REALTIME, &start);
	for(i=0; i<MAX_SST_PB_CHANNEL; i++)
	{
	    if(pb_data.sid[i] >= 0) {
	       ret = sst_play_close (pb_data.sid[i]);

	       pb_data.sid[i] = -1;
	       if(ret < 0)
	           goto close_failed;
	    }
	}

	pb_data.init = 0;
	g_warning("SSTSRC: All streams are closed. \n");

	clock_gettime(CLOCK_REALTIME, &end);
	delta = (end.tv_sec - start.tv_sec)*1000.0 
	      + (end.tv_nsec - start.tv_nsec)/1.0e6;
	g_warning("SST close: it takes %f ms\n", delta); 		

	pb_data.is_first_frame = FALSE;

	if(ret >= 0)
		return TRUE;

close_failed:
    return FALSE;
}

static gboolean
playback_pipe_open ()
{
	int i, j;
	guint64 real_starttime;
	gboolean is_iframe;
	int stm_offset;

	clock_gettime(CLOCK_REALTIME, &start);

	pb_data.is_start_overlapped = FALSE;
	pb_data.is_start_endvideo = FALSE;
	pb_data.is_prev_start = FALSE;
	pb_data.is_first_frame = TRUE;
    
	for(i=0; i<MAX_SST_PB_CHANNEL; i++)
	{
		pb_data.ch[i] = -1;
		pb_data.disp_ch[i] = -1;
		pb_data.sid[i] = -1;
		pb_data.drop_cnt[i] = 0;
		pb_data.prev_ri_time[i] = (guint64)-1;
		if(pb_data.direction == SST_SRC_DIR_FORWARD)
			pb_data.cur_frame_time[i] = 0; 
		else
			pb_data.cur_frame_time[i] = (guint64)-1; 
	}

	g_message("open mode: %d\n", pb_data.mode);
	g_message("ch num: %d direction %d rate %d \n", pb_data.ch_num,pb_data.direction,pb_data.open_rate);
	//g_message("srm offset: %d\n", pb_data.srm_offset); 
	//g_message("srm limit: %d\n", pb_data.srm_limit);
	
	
	if(pb_data.rate == 1 && pb_data.direction == SST_SRC_DIR_FORWARD)
		pb_data.vtick_interval_usec = 10000;
	else
		pb_data.vtick_interval_usec = 66666;
	

	if(pb_data.mode == SST_SRC_MODE_NORMAL) {
		j=0;
		for(i=0; i<MAX_SST_PB_CHANNEL; i++)
		{
			if(pb_data.ch_mask & (1ULL<<i))
			{
				pb_data.disp_ch[j] = i;
				pb_data.ch[j++] = i;
			}
		}
        if((pb_data.ch_num == 2) && (pb_data.audio_ch != -1)){
            pb_data.disp_ch[j] = pb_data.audio_ch + SST_AUDIO_CHANNEL_BASE;
            pb_data.ch[j++] = pb_data.audio_ch + SST_AUDIO_CHANNEL_BASE;
        }
		if(G_UNLIKELY (j!= pb_data.ch_num))
			g_warning("j = %d, src->ch_num = %d\n", j,pb_data.ch_num);
		g_assert(j == pb_data.ch_num);
	} else if (pb_data.mode == SST_SRC_MODE_PANORAMA1) {
		g_message("--NFSSTSRC: PANORAMA1 PLAYBACK \n");
		for(i=0; i<MAX_SST_PB_CHANNEL; i++)
		{
			if(pb_data.ch_mask & (1ULL<<i))
			{
				pb_data.ch[0] = i;
				pb_data.disp_ch[0] = i;
				break;
			}
		}
		g_assert(pb_data.ch_num == 1);
	} else if (pb_data.mode == SST_SRC_MODE_PANORAMA2) {
		for(i=0; i<MAX_SST_PB_CHANNEL; i++)
		{
			if(pb_data.ch_mask & (1ULL<<i))
			{
				pb_data.ch[0] = i;
				break;
			}
		}
		g_message("--NFSSTSRC: PANORAMA2 PLAYBACK \n");
		for(i=0; i<pb_data.ch_num; i++) {
		       pb_data.ch[i] = pb_data.ch[0];
		       pb_data.disp_ch[i] = i;
		}
	} else {
		g_message("unsupported mode\n");
		goto open_failed;
	}

	pb_data.prev_eos_mask = pb_data.eos_mask = -1ULL;


	/* decide i-frame only */
	if(pb_data.open_rate == 1 && pb_data.direction == SST_SRC_DIR_FORWARD)
	{
	    is_iframe = FALSE;
	}
	else
	{
	    is_iframe = TRUE;
	}
	pb_data.is_iframe_open = is_iframe;

	if(pb_data.ch_num <= pb_data.srm_limit)
	{
	    stm_offset = 0;
	}
	else
	{
	    stm_offset = pb_data.srm_offset;
	}

   	for(i=0; i < pb_data.ch_num; i++) {
		guint64 begin_offset = 0;
		guint64 end_offset = 0;
		guint64 search_offset = 0;

		if(pb_data.begin_time != 0)
		        begin_offset = pb_data.time_offset*i;
		if(pb_data.end_time != 0)
		        end_offset = pb_data.time_offset*i;
		if(pb_data.search_time != 0)
		        search_offset = pb_data.time_offset*i;
#if 0
        g_message("open param: pr_id=%d, src->ch[%d]=%d, open_rate=%d, direction=%d, \
                rate_control=%d, hide=%d, begin_t=%lld, end_t=%lld,search_t=%lld\n", 
                pb_data.pr_id, i, pb_data.ch[i], 
                                pb_data.open_rate, pb_data.direction, pb_data.rate_control, pb_data.hide, 
                                pb_data.begin_time + begin_offset,
                                pb_data.end_time + end_offset, 
                                pb_data.search_time + search_offset);
#endif
	
		pb_data.sid[i] = sst_play_open_ex(pb_data.pr_id, pb_data.ch[i] + stm_offset, 
		                        pb_data.open_rate, pb_data.direction, pb_data.rate_control, pb_data.hide, 
		                        pb_data.begin_time + begin_offset,
		                        pb_data.end_time + end_offset, 
		                        pb_data.search_time + search_offset 
		                         + ( ( pb_data.ch[i] >=SST_AUDIO_CHANNEL_BASE ) ? 1100000 * SST_USECOND : 0 ) , 0, 
		                        is_iframe, 
		                        0
		                        );
		//printf("sst_play_open_ex pr_id=%d, ch=%d, sid=%d\n", pb_data.pr_id, pb_data.ch[i] + stm_offset, pb_data.sid[i]);
		if(G_UNLIKELY(pb_data.sid[i] < 0)) {                   //20090326 Overlap bug fix.
			if(pb_data.sid[i] == -SST_ERR_OVERLAP)
				pb_data.is_start_overlapped = TRUE;
		}
		else  {
			pb_data.eos_mask &= ~(1ULL<<i);
		}
	}

	pb_data.open_mask = ~pb_data.eos_mask;
	pb_data.norecord_mask = 0;
	pb_data.prev_norecord_mask = 0;
	pb_data.overlapped_mask = 0;
	pb_data.prev_overlapped_mask = 0;
    
    	g_print("open success %16llx\n", pb_data.eos_mask);

	//20110809  endvideo timestamp bug fixed. 0010582
	if(pb_data.open_mask == 0)
	{
		pb_data.is_start_endvideo = TRUE;
		pb_data.base_time = pb_data.search_time;
		pb_data.is_prev_start = TRUE;
		return TRUE;
	}
	if(pb_data.is_start_overlapped)
	{
		pb_data.base_time = pb_data.search_time;
		pb_data.is_prev_start = TRUE;
		return TRUE;
	}

	{
		int r;
		int retry_cnt;
		gdouble rate;
		retry_cnt = 0;

		while(1) 
		{
			    r = sst_play_wait_frame(pb_data.sid, pb_data.ch_num, 1, 3000);   //Recommended by HTM
			    retry_cnt++;

			    if(r >= 0 || retry_cnt > 30)
			        break;
			    g_print("nfsstsrc: wait fail, retry !! %d\n", retry_cnt);
			    g_usleep(100000);
		}  

		if ( r >= 0 ) {
		    guint64 start_time;
		    int result = 0;

		    if(pb_data.direction == SST_SRC_DIR_FORWARD) {
		        real_starttime = (guint64)-1;
		        rate = 1*(gdouble)pb_data.rate;
		        pb_data.gst_rate = rate;
		        for(i=0; i < pb_data.ch_num; i++) {
		            if( ! (pb_data.eos_mask  & (1ULL << i)) ) {
		               result = sst_play_check_starttime(pb_data.sid[i], &start_time);
		               if(result < 0)
		                   break;
		               if(pb_data.mode == SST_SRC_MODE_PANORAMA2)
		                  start_time -= pb_data.time_offset*i;
		                if(real_starttime > start_time)
		                    real_starttime = start_time;
		            }
		        }
		    } else {
		        real_starttime = 0;
		        rate = -1*(gdouble)pb_data.rate;
		        pb_data.gst_rate = rate;
		        for(i=0; i < pb_data.ch_num; i++) {
		            if( ! (pb_data.eos_mask  & (1ULL << i)) ) {
		               result = sst_play_check_starttime(pb_data.sid[i], &start_time);
		               if(result < 0)
		                   break;

		               if(pb_data.mode == SST_SRC_MODE_PANORAMA2)
		                  start_time -= pb_data.time_offset*i;
		                if(real_starttime < start_time)
		                    real_starttime = start_time;
		            }
		        }
		    }

		    if(result < 0)
		        pb_data.is_start_overlapped = TRUE;

		}
		else {
			g_warning("NFSSTSRC: Failed to wait frame. Close opened streams\n");
			for(i=0; i<MAX_SST_PB_CHANNEL; i++)
			{
			    if(pb_data.sid[i] >= 0) {
			       sst_play_close (pb_data.sid[i]);
			       pb_data.sid[i] = -1;
			    }
			}

		    	pb_data.eos_mask = -1ULL;
		    	pb_data.is_prev_start = TRUE;
			
			goto open_failed;
		}

#if 0
	{
	    GstDateTime *date;
	    g_message("get date\n");
	    date = gst_date_time_new_from_unix_epoch_local_time(real_starttime/GST_SECOND);
	    g_message("open stream at %s.%s/%s %s:%s:%s\n", 
	                                                gst_date_time_get_year(date), 
	                                                gst_date_time_get_month(date), 
	                                                gst_date_time_get_day(date), 
	                                                gst_date_time_get_hour(date), 
	                                                gst_date_time_get_minute(date), 
	                                                gst_date_time_get_second(date));
	    gst_date_time_unref(date);
	}
#endif
		//     GST_INFO_OBJECT (src, "opening stream at %"GST_TIME_FORMAT "\n", src->search_time);

		pb_data.running_time = real_starttime;

		g_message("vtick_interval_usec = %d\n", pb_data.vtick_interval_usec);
		if(pb_data.direction == SST_SRC_DIR_FORWARD) {

		   pb_data.running_time += pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;    //FIXME NTSC or PAL?
		}
		else {
		   pb_data.running_time -= pb_data.rate*pb_data.vtick_interval_usec*SST_USECOND;
		}

		pb_data.base_time = pb_data.running_time;

		pb_data.is_prev_start = TRUE;

		pb_data.isAfterSeek = TRUE;

		if(G_UNLIKELY(pb_data.rate == 32))
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS_32;
		}
		else if(G_UNLIKELY(pb_data.rate == 64))
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS_64;
		}
		else if(G_UNLIKELY(pb_data.rate == 16))
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS_16;
		}
		else if(G_UNLIKELY(pb_data.rate == 8))
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS_8;
		}
		else if(G_UNLIKELY(pb_data.rate == 4))
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS_4;
		}
		else if(G_UNLIKELY(pb_data.rate == 2))
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS_2;
		}
		else 
		{
		    pb_data.norecord_interval_ms = BASE_NORECORD_INTERVAL_MS;
		}
	}

	clock_gettime(CLOCK_REALTIME, &end);
	delta = (end.tv_sec - start.tv_sec)*1000.0 
	      + (end.tv_nsec - start.tv_nsec)/1.0e6;
	g_message("SST open: it takes %f ms\n", delta); 		

  return TRUE;

/* ERROR */
open_failed:
	{
    //    g_error("%s: %s\n", __FUNCTION__, sst_get_error_string(src->sid));
	    	g_print("nfsstsrc: open failed\n");
		return FALSE;
	}	
}


guint64 playback_thread_get_eos_mask()
{
	if (playback_streams_open) {
		return pb_data.eos_mask;
	} else {
		return 0x0;
	}
}

guint64 playback_thread_get_overlapped_mask()
{
    return pb_data.overlapped_mask;
}

guint64 playback_thread_get_norecord_mask()
{
    return pb_data.norecord_mask;
}

guint64 playback_thread_get_last_timestamp()
{
	return pb_data.last_render_time;
}
guint64 playback_thread_get_timestamp()
{
	return pb_data.last_render_time;
}

void
playback_thread_clear_status(void)
{
	pb_data.eos_mask = 0;
	pb_data.norecord_mask = 0;
	pb_data.overlapped_mask = 0;
}

static void
playback_data_init ()
{
	int i;

	for(i=0; i<MAX_SST_PB_CHANNEL; i++) {
		pb_data.sid[i] = -1;
		pb_data.ch[i] = 0;
		pb_data.disp_ch[i] = 0;
	}

	pb_data.time_offset = 0;
	pb_data.init = 1;
	pb_data.running_time = 0;
	pb_data.pr_id = 0;
	pb_data.rate = 1;
	pb_data.open_rate = pb_data.rate;
	pb_data.direction = 0;
	pb_data.rate_control = 0;
	pb_data.hide = FALSE;
	pb_data.base_time = 0;
	pb_data.begin_time = 0;
	pb_data.end_time = 0;
	pb_data.search_time = 0;
	pb_data.norecord_mask = 0;
	pb_data.prev_norecord_mask = 0;
	pb_data.vtick_interval_usec = 33367;
	pb_data.is_continue = FALSE;
	pb_data.is_slow = FALSE;
	pb_data.is_iframe_open = FALSE;

	pb_data.overlapped_mask = 0;
	pb_data.prev_overlapped_mask = 0;

	pb_data.state_lock = g_mutex_new();
	pb_data.block_lock = g_mutex_new();

	pb_data.cond = g_cond_new();
}

void
local_playback_thread (GAsyncQueue* p_queue)
{
	PLAYBACK_CMD* pb_cmd;
	int res=0;
	playback_data_init(&pb_data);
	
	g_thread_create( (GThreadFunc)playback_pipe, NULL, FALSE, NULL);
	
	while(1){

		pb_cmd = g_async_queue_pop(_play_pipe.queue);

		switch(pb_cmd->state){
			case PLAYBACK_START:
		printf("local_playback_thread START \n");
				pb_data.is_nextframe_mode = (gboolean)FALSE;
				res = playback_pipe_open();
				if(res) {
					playback_pipe_on =1;
					playback_streams_open = 1;
				}
				break;
			case PLAYBACK_STOP:
		printf("local_playback_thread STOP \n");
				pb_data.is_nextframe_mode = (gboolean)FALSE;
				playback_pipe_on =0;
				while(1){
					if(playback_pipe_state == 0)
						break;
					usleep(100000);
				}
				playback_pipe_stop();
				playback_streams_open = 0;
				break;
			case PLAYBACK_PAUSE:
		printf("local_playback_thread PAUSE \n");
				pb_data.is_nextframe_mode = (gboolean)FALSE;
				playback_pipe_on =0;
				break;
			case PLAYBACK_NEXT:
		printf("local_playback_thread NEXT \n");
				pb_data.is_nextframe_mode = (gboolean)TRUE;
				if (playback_streams_open == 0) {
					res = playback_pipe_open();
					if(res) {
						playback_pipe_on =1;
						playback_streams_open = 1;
					}
				}
				playback_pipe_on =2;
				break;
			case PLAYBACK_CONTINUE:
		printf("local_playback_thread CONTINUE \n");
				pb_data.is_nextframe_mode = (gboolean)FALSE;
				playback_pipe_on =1;
				break;
			default:
				break;

		}
		free(pb_cmd);

	}

}

//ksi_test
gboolean nf_play_smart_set_fgdebug(gboolean enable)
{
	return 1;
}

gboolean nf_play_set_smart_geometry(gint ch, gint x, gint y, gint width, gint height)
{
	return 1;
}

gboolean nf_play_unset_smart_geometry(void)
{
	return 1;
}