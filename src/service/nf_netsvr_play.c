#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <unistd.h>		
#include <stdlib.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstinfo.h>
//#include <gst/nf/gstnfbuddybuffer2.h>
#include <gst/nf/gstnfbuddybuffer.h>

#include "nf_common.h"
#include "nf_debug.h"
#include "nf_network.h"
#include "nf_timer.h"

#include "nf_netsvr.h"
#include "nf_netsvr_drdef.h"
#include "nf_codec_header.h"
#include "nf_api_play.h"

#include "unp.h" 
#include "unpthread.h" 
#include "gsocket.h" 
#include "queue.h" 

#include "libsst.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "nplay"

#define MAX_SST_CHANNEL 	(64)

//#define DEBUG_IFRAME_ONLY_SEND
#define DEBUG_NETSVR_LOG
#define ENABLE_PLAYBACK_AUDIO_SYNC

typedef enum _DEBUG_NETSVR_IDX_E_PLAY
{ 
	DEBUG_NETSVR_IDX_PLAY_PROCESS		= 17,
	DEBUG_NETSVR_IDX_PLAY_THREAD		= 18,
	DEBUG_NETSVR_IDX_PLAY_THREAD_SEND	= 19,
	
	DEBUG_NETSVR_IDX_NR					= 20
	
}DEBUG_NETSVR_IDX_E_PLAY;

extern int	_netsvr_get_debug_val(int idx);

/*
api_play-Message: nf_play_change param->start_time.tv_sec[0]
api_play-Message: nf_play_change param->search_time.tv_sec[1227612300]
api_play-Message: nf_play_change param->end_time.tv_sec[0]
api_play-Message: nf_play_change param->interval[0]
api_play-Message: nf_play_change param->video_object_cnt[1]
api_play-Message: nf_play_change param->arr[0] [0] [0][0][1328][1080] [2][1]
api_play-Message: nf_play_change param->play_mode[1]
api_play-Message: nf_play_change param->play_rate[1]
api_play-Message: nf_play_change param->direction[0]
api_play-Message: nf_play_change param->slow_flag[0]

netsvr-Message: _dr_start_play start_time [2008/11/25-18:58:20.000]
netsvr-Message: _dr_start_play end_time [2008/11/25-22:44:56.000]
netsvr-Message: _dr_start_play direction[0] speed[1] audio_channel_id[65535] mute[0]
netsvr-Message: _dr_start_play channel_mask[0x0000000f]

netsvr-Message: _dr_start_backup start_time [2008/11/27-15:06:00.000]
netsvr-Message: _dr_start_backup end_time [2008/11/27-15:07:00.000]
netsvr-Message: _dr_start_backup include_audio[0]
netsvr-Message: _dr_start_backup channel_mask[0x00000001]
*/	


typedef struct _NF_NET_PLAY_PARAM_T {	
	
	
	CLIENT_INFO 	*pclient;	
	gint			playid;
	guchar			mode;
	
	GTimeVal  		start_time;		// play start time
	GTimeVal 		search_time;	// play search time
	GTimeVal 		end_time;		// play end time

	guchar			pr_id;	// 0
	guchar			hide;	// 0	
	guchar			slow;	// 0

	guchar			direction;
	guchar			rate;	
	guchar			rate_ctrl;
	
	guint64			ch_mask;

	gint			ch_cnt;
	gint			audio_ch;
	
	guint64			open_mask;
	guint64			eos_mask;
	guint64			norecord_mask;	

	gint			ch_arr[MAX_SST_CHANNEL];
	gint			sid[MAX_SST_CHANNEL];
	
	ICODEC_HEADER	frame[MAX_SST_CHANNEL];
	ICODEC_HEADER	*ri_frame[MAX_SST_CHANNEL];
			
} NF_NET_PLAY_PARAM; 

static void 
dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] ", str , 
					pheader->chan, 
					pheader->flags, 
					pheader->frame_type, 
					pheader->frame_rate, 
					pheader->resolution, 						
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size	 );	
}

static void 
free_icodec_data(ICODEC_HEADER *ih) 
{
	GstBuffer *buf;
	int length;

	length = ih->frame_size + sizeof(ICODEC_HEADER);
	buf = gst_nf_buddy_buffer_new(NULL);
	GST_BUFFER_DATA(buf) = (guint8 *)ih;
	GST_BUFFER_SIZE(buf) = length;
	gst_buffer_unref(buf);
}

static void
_play_thread_func(NF_NET_PLAY_PARAM	*param)
{
	CLIENT_INFO *info = param->pclient;	
	int cs = info->cs;
	int ds = info->ds;	
	int playid = param->playid;
	int uniqueid = info->uniqueid;	
	int ret,i,ch;
	int send_start_flag = 1, send_end_flag = 1; 
	int flag_SORB = 1;
	
	unsigned int wait_time = 0;
	GTimeVal tv_max, tv_min, tv_tmp;
	GTimeVal tv_audio = {0,0};
	
	int tv_max_ch =0, tv_min_ch=0;

	g_message("%s start!!",__FUNCTION__);
	Pthread_mutex_lock(&info->ds_mutex);
		
	if(info->magic_key != CLIENT_MAGIC)
	{	
		g_warning("%s error uid[%d] magic_key[0x%08x] start",__FUNCTION__,
						uniqueid, info->magic_key);
						
		send_start_flag = send_end_flag = 0;
		goto error_send_ctrl;
	}
	
	if(info->mode != param->mode )
		goto error_send_ctrl;
	
	g_message("%s uid[%d] playid[%d] mode[%c] ch_mask[0x%llx][%d]",__FUNCTION__, 
				uniqueid, playid, param->mode, param->ch_mask, param->ch_cnt);
							
	send_start_flag = 0;
	if(_send_controlframe(ds, NF_FRAME_TYPE_STARTDATA) < 0){
		perror("send_controlframe() DRFT_STARTDATA");
		send_end_flag = 0;
		goto error_send_ctrl;
	}		

	if(param->audio_ch != -1 && param->direction == 0)
	{
		param->ch_mask |= (1ULL<<(param->audio_ch + BASE_AUDIO_CHANNEL) );
		++param->ch_cnt;

		g_message("%s audio_ch[%d] ch_mask[0x%llx] [%d]", __FUNCTION__, 
						param->audio_ch,
						param->ch_mask, param->ch_cnt);								
	}

	for(i=0,ch=0;i<MAX_SST_CHANNEL;i++)
	{
		if( param->ch_mask & (1ULL<<i) )
		{
			param->ch_arr[ch] = i;
			param->sid[ch] = sst_play_open( param->pr_id, i, 
											param->rate, 
											param->direction, 
											param->rate_ctrl,	// 0
											param->hide,		// 0
											GTIMEVAL_TO_GUINT64(param->start_time),											
											GTIMEVAL_TO_GUINT64(param->end_time),											
											GTIMEVAL_TO_GUINT64(param->search_time ) , 
											PB_TYPE_REMOTE );

			if( param->sid[ch] == -SST_ERR_NODATA){				
				g_message("%s sst_play_open ch[%2d] NODATA",__FUNCTION__, i);				
				param->eos_mask |= (1ULL<<i);
				++ch;
				continue;				
			}else if( param->sid[ch] < 0) {
				g_warning("%s sst_play_open ch[%2d] ret[%d]",__FUNCTION__, i, param->sid[ch]);
				goto error_send_ctrl;
			}
			param->open_mask |= (1ULL<<i);

#ifdef DEBUG_NETSVR_LOG
			if(  _netsvr_get_debug_val(DEBUG_NETSVR_IDX_PLAY_THREAD)  )	
				g_message("%s sst_play_open ch[%2d] sid[%d]",__FUNCTION__, i, param->sid[ch]);
#endif
			while(1)
			{			
				ret = sst_play_check_frame( param->sid[ch], &param->frame[ch] );
				if(ret == 0){ 
#ifdef ENABLE_PLAYBACK_AUDIO_SYNC
					if( param->ch_arr[ch] >=BASE_AUDIO_CHANNEL
						&& param->mode == CLIENT_MODE_ARCHIVING )
					{
						GTimeVal tmptv = { param->frame[ch].timestamp, param->frame[ch].timestampl * 5000 };						
						g_time_val_add( &tmptv, 1020000);									
						param->frame[ch].timestamp = tmptv.tv_sec;
						param->frame[ch].timestampl = tmptv.tv_usec/5/1000;						
					}
#endif						
					break;
				}else if(ret == -SST_ERR_ENDDATA){
					param->eos_mask |= (1ULL<<param->ch_arr[ch]);
					g_message("%s sst_play_check_frame ch[%2d] sid[%d] ENDDATA", 
								__FUNCTION__, param->ch_arr[ch], param->sid[ch]);
					break;
				}else if(ret == -SST_ERR_EMPTY){
					g_usleep(10*1000);
					continue;
				}else{
					g_warning("%s sst_play_check_frame ch[%2d] sid[%d] ret[%d]", 
								__FUNCTION__, param->ch_arr[ch], param->sid[ch], ret);
					goto error_send_ctrl;
				}
			}
#ifdef DEBUG_NETSVR_LOG
			if(  _netsvr_get_debug_val(DEBUG_NETSVR_IDX_PLAY_THREAD)  )	
				dump_icodec_header(__FUNCTION__,  &param->frame[ch]);			
#endif			
			++ch;
		}
	}

	wait_time = 100000*param->ch_cnt;
	g_message("%s wait sst_buffering [%d]usec [%d]", __FUNCTION__, wait_time, param->ch_cnt);
	g_usleep(wait_time);

	while(1) // send
	{	
		ICODEC_HEADER *ih_frame;
		int write_size = 0;
		int target_ch = 0;

		tv_max.tv_sec = 0;
		tv_max.tv_usec = 0;	
		tv_min.tv_sec = 0x7FFFFFFF;
		tv_min.tv_usec = 0x7FFFFFFF;
		
		if(info->magic_key != CLIENT_MAGIC)
		{
			g_warning("%s error uid[%d] magic_key[0x%08x] after",__FUNCTION__,
						uniqueid, info->magic_key);
			send_start_flag = send_end_flag = 0;
			goto error_send_ctrl;
		}

		
		// choissi  CMS keepalive timeout delay
		++info->play_frame_count;
		
				
		if(info->mode != param->mode)
		{
			send_end_flag = 1;
			goto error_send_ctrl;
		}		
			
		for(ch=0;ch<param->ch_cnt;++ch)	// get lastest/oldest frame header 
		{										
			if( param->eos_mask & (1ULL<<param->ch_arr[ch]) ) 
				continue;
											
			tv_tmp.tv_sec = param->frame[ch].timestamp;
			tv_tmp.tv_usec = param->frame[ch].timestampl * 5000;
			
			if( param->frame[ch].frame_type == NF_FRAME_TYPE_RI )
			{	
				tv_min_ch = tv_max_ch = ch;	
				break;
			}

			if( tv_tmp.tv_sec < tv_min.tv_sec ||
				(tv_tmp.tv_sec == tv_min.tv_sec 
					&& tv_tmp.tv_usec < tv_min.tv_usec) ) {
				tv_min = tv_tmp; 
				tv_min_ch = ch;
			}

			if( tv_tmp.tv_sec > tv_max.tv_sec ||
				(tv_tmp.tv_sec == tv_max.tv_sec 
					&& tv_tmp.tv_usec > tv_max.tv_usec) ) {
				tv_max = tv_tmp; 
				tv_max_ch = ch;
			}
		}
		
		if( param->eos_mask == param->ch_mask )
		{
			g_message("%s ALL ch eos_maks[0x%llx] ch_mask[0x%llx]",__FUNCTION__, 
							param->eos_mask, param->ch_mask);
			break;
		}
		
		if(param->direction == NF_PLAY_PARAM_DIR_FORWARD)
			target_ch = tv_min_ch;		
		else
			target_ch = tv_max_ch;

		ret = sst_play_get_frame(param->sid[target_ch], &ih_frame, 5000);  //wait 5second		
		if(ret<0){
			g_warning("%s sst_play_get_frame ch[%d] sid[%d] ret[%d]",
					__FUNCTION__,param->ch_arr[target_ch], param->sid[target_ch], ret);
			goto error_send_ctrl;
		}

#ifdef ENABLE_PLAYBACK_AUDIO_SYNC
		if( param->ch_arr[target_ch] >= BASE_AUDIO_CHANNEL 
			&& param->mode == CLIENT_MODE_ARCHIVING )
		{
			GTimeVal tmptv = { ih_frame->timestamp, ih_frame->timestampl * 5000 };
			g_time_val_add( &tmptv, 1020000);
			ih_frame->timestamp = tmptv.tv_sec;
			ih_frame->timestampl = tmptv.tv_usec/5/1000;
		}
#endif

		if( param->mode == CLIENT_MODE_ARCHIVING)	// FIXME !! 2008-12-30 ¿ÀÈÄ 2:01:54 choissinf
		{
			if( flag_SORB ){
				ih_frame->flags = NF_CODEC_FLAG_SORB;
				flag_SORB = 0;
			}else{
				ih_frame->flags = 0;
			}
		}
		
#ifdef DEBUG_NETSVR_LOG
		if(  _netsvr_get_debug_val(DEBUG_NETSVR_IDX_PLAY_THREAD_SEND)  )
			dump_icodec_header(__FUNCTION__, ih_frame);
#endif

#ifdef DEBUG_IFRAME_ONLY_SEND
		if( ih_frame->frame_type == NF_FRAME_TYPE_I)
#endif			
		{
			write_size = ih_frame->frame_size + sizeof(ICODEC_HEADER);		
			ret = Writen( ds, ih_frame, write_size);
		}

		free_icodec_data(ih_frame); 
		ih_frame = NULL;			
		
		if(write_size != ret)
		{
			g_warning("%s Writen error size[%d] ret[%d]",__FUNCTION__, 
						write_size, ret);
															
			goto error_send_ctrl;
		}

		ch = target_ch;
		while(1) // get new header
		{			
			ret = sst_play_check_frame( param->sid[ch], &param->frame[ch] );
			if(ret == 0){
#ifdef ENABLE_PLAYBACK_AUDIO_SYNC
				if( param->ch_arr[ch] >=BASE_AUDIO_CHANNEL 
					&& param->mode == CLIENT_MODE_ARCHIVING )
				{
					GTimeVal tmptv = { param->frame[ch].timestamp, param->frame[ch].timestampl * 5000 };
					g_time_val_add( &tmptv, 1020000);														
					param->frame[ch].timestamp = tmptv.tv_sec;
					param->frame[ch].timestampl = tmptv.tv_usec/5/1000;						
				}
#endif	
			 break;
			}else if(ret == -SST_ERR_ENDDATA){
				param->eos_mask |= (1ULL<<param->ch_arr[ch]);
				g_message("%s sst_play_check_frame ch[%2d] sid[%d] ENDDATA", 
							__FUNCTION__, param->ch_arr[ch], param->sid[ch]);
											
				break;
			}else if(ret == -SST_ERR_EMPTY){
				g_usleep(10*1000);
				continue;
			}else{
				g_warning("%s sst_play_check_frame ch[%2d] sid[%d] ret[%d]", 
							__FUNCTION__, param->ch_arr[ch], param->sid[ch], ret);
				goto error_send_ctrl;
			}
		}		
	}
							
error_send_ctrl:

	for(ch=0;ch<param->ch_cnt;ch++){
		if( param->sid[ch] >= 0)
		{
			ret = sst_play_close(param->sid[ch]);

#ifdef DEBUG_NETSVR_LOG
			if(  _netsvr_get_debug_val(DEBUG_NETSVR_IDX_PLAY_THREAD)  )	
				g_message("%s sst_play_close ch[%2d] sid[%d]",__FUNCTION__, 
							param->ch_arr[ch], param->sid[ch]);	
#endif

			param->sid[ch] = -1;
		}		
	}
	
	if(param)
		g_free(param);

	if(send_start_flag)
		if(_send_controlframe(ds, NF_FRAME_TYPE_STARTDATA) < 0)
			perror("send_controlframe() DRFT_ENDDATA");

	if(send_end_flag)
		if(_send_controlframe(ds, NF_FRAME_TYPE_ENDDATA) < 0)
			perror("send_controlframe() DRFT_ENDDATA");
	
	_client_unset_streamid(info, playid);	

	Pthread_mutex_unlock(&info->ds_mutex);
	g_message("%s end",__FUNCTION__);
		
	return 1;	
			
}

int _process_play(JOB_INFO *pJobEntry)
{	
	CLIENT_INFO *info = pJobEntry->pclient;	
	int cs = info->cs;
	int ds = info->ds;	
	int playid = pJobEntry->params[0];
	int uniqueid = info->uniqueid;
	int ret,i,j;
	int send_ctrl_flag = 1;
			
	NF_NET_PLAY_PARAM	*param = NULL;
	DRREQ_START_PLAY	*play = (DRREQ_START_PLAY *)pJobEntry->msg;

	g_message("%s uid[%d] playid[%d]",__FUNCTION__, uniqueid, playid);
	
	if(info->magic_key != CLIENT_MAGIC)
	{
		g_warning("%s error uid[%d] magic_key[0x%08x]",__FUNCTION__,
					uniqueid, info->magic_key);
		send_ctrl_flag = 0;
		goto error_send_ctrl;
	}
		
	if(info->mode != CLIENT_MODE_PLAYBACK)
		goto error_send_ctrl;

	param = g_malloc0(sizeof(NF_NET_PLAY_PARAM));
	if(param == NULL)
	{
		g_warning("%s error uid[%d] malloc sizeof(NF_NET_PLAY_PARAM)", 
						__FUNCTION__, uniqueid);
		goto error_send_ctrl;
	}else{	// param set

		param->pclient	= info;
		param->playid 	= playid;
		param->mode		= CLIENT_MODE_PLAYBACK;
		
		param->direction = (guchar)play->direction;

#if 0			
		param->start_time.tv_sec = play->start_time;
		param->start_time.tv_usec = play->start_time_sub*5000;
				
		param->end_time.tv_sec = play->end_time;
		param->end_time.tv_usec = play->end_time_sub*5000;
#endif

		if( param->direction == 0){ //FORWARD
			param->search_time.tv_sec = play->start_time;
			param->search_time.tv_usec = play->start_time_sub*5000;

			param->end_time.tv_sec = play->end_time;
			param->end_time.tv_usec = play->end_time_sub*5000;

		}else{
			param->search_time.tv_sec = play->end_time;
			param->search_time.tv_usec = play->end_time_sub*5000;

			param->end_time.tv_sec = play->start_time;
			param->end_time.tv_usec = play->start_time*5000;
		}
		
		param->rate		 = (guchar)play->speed;
		param->ch_mask	 = (guint64)play->channel_mask;		
		param->hide		 = 1;
		
		for(i=0;i<MAX_SST_CHANNEL;i++)
			param->sid[i] = -1;

		for(i=0;i<32;i++)
		{
			if( param->ch_mask & (1<<i) ) 
				++param->ch_cnt;
		}
									
		param->audio_ch = (play->audio_channel_id == 0xffff) ? -1: play->audio_channel_id ;
	}
	
	if( !g_thread_create( (GThreadFunc)_play_thread_func, param, FALSE, NULL) )
	{
		g_warning("%s error uid[%d] g_thread_create failed",__FUNCTION__, 
					uniqueid);
		goto error_send_ctrl;
	}
	return 1;

error_send_ctrl:

	if(param)
		g_free(param);

	if(send_ctrl_flag)
	{	
		if(_send_controlframe(ds, NF_FRAME_TYPE_STARTDATA) < 0) 
			perror("send_controlframe() DRFT_STARTDATA");
				
		if(_send_controlframe(ds, NF_FRAME_TYPE_ENDDATA) < 0)
			perror("send_controlframe() DRFT_ENDDATA");
	}
	_client_unset_streamid(info, playid);	
			
	return 0;		
}

	
int _process_backup(JOB_INFO *pJobEntry)
{

	CLIENT_INFO *info = pJobEntry->pclient;	
	int cs = info->cs;
	int ds = info->ds;	
	int playid = pJobEntry->params[0];
	int uniqueid = info->uniqueid;
	int ret,i,j;
	int send_ctrl_flag = 1;
			
	NF_NET_PLAY_PARAM	*param = NULL;
	DRREQ_START_BACKUP	*backup = (DRREQ_START_BACKUP *)pJobEntry->msg;

	g_message("%s uid[%d] playid[%d]",__FUNCTION__, uniqueid, playid);
	
	if(info->magic_key != CLIENT_MAGIC)
	{
		g_warning("%s error uid[%d] magic_key[0x%08x]",__FUNCTION__,
					uniqueid, info->magic_key);
		send_ctrl_flag = 0;
		goto error_send_ctrl;
	}
		
	if(info->mode != CLIENT_MODE_ARCHIVING)
		goto error_send_ctrl;

	param = g_malloc0(sizeof(NF_NET_PLAY_PARAM));
	if(param == NULL)
	{
		g_warning("%s error uid[%d] malloc sizeof(NF_NET_PLAY_PARAM)", 
						__FUNCTION__, uniqueid);
		goto error_send_ctrl;
	}else{	// param set

		param->pclient	= info;
		param->playid 	= playid;
		param->mode		= CLIENT_MODE_ARCHIVING;
		
		param->direction = 0;

		param->start_time.tv_sec = backup->start_time;
		param->start_time.tv_usec = backup->start_time_sub*5000;
				
		param->end_time.tv_sec = backup->end_time;
		param->end_time.tv_usec = backup->end_time_sub*5000;

		param->search_time.tv_sec = backup->start_time;
		param->search_time.tv_usec = backup->start_time_sub*5000;
		
		param->rate		 = 1;
		param->ch_mask	 = (guint64)backup->channel_mask;
		param->ch_cnt	 = 1;
		
		param->hide		 = 1;
		
		for(i=0;i<MAX_SST_CHANNEL;i++)
			param->sid[i] = -1;

		param->audio_ch  = -1;
		
		if( backup->include_audio )	{
			for(i=0;i<32 ;i++)
			{
				if( backup->channel_mask & (1<<i) )
				{
					param->audio_ch  = i;
					break;
				}
			}
		}
					
	}
	
	if( !g_thread_create( (GThreadFunc)_play_thread_func, param, FALSE, NULL) )
	{
		g_warning("%s error uid[%d] g_thread_create failed",__FUNCTION__, 
					uniqueid);
		goto error_send_ctrl;
	}
	return 1;

error_send_ctrl:

	if(param)
		g_free(param);

	if(send_ctrl_flag)
	{	
		if(_send_controlframe(ds, NF_FRAME_TYPE_STARTDATA) < 0) 
			perror("send_controlframe() DRFT_STARTDATA");
				
		if(_send_controlframe(ds, NF_FRAME_TYPE_ENDDATA) < 0)
			perror("send_controlframe() DRFT_ENDDATA");
	}
	_client_unset_streamid(info, playid);	
			
	return 0;		
}
