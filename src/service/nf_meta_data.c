#include <glib.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// //#include <gst/nf/GobjBuddyBuffer2.h>
// #include <gst/nf/GobjBuddyBuffer.h>
// #include <gst/nf/gstnflistbuffer.h>
#include <gobj.h>
#include <gobjmedia.h>

#include <pthread.h>
#include <sys/time.h>

#include "libsst.h"
#include "libicmem.h"

#include "unp.h"

#include "nf_common.h"
#include "nf_debug.h"
#include "nf_watchdog.h"

#include "nf_record.h"
#include "nf_rec_audio.h"

#include "nf_notify.h"
#include "nf_timer.h"
#include "nf_codec_header.h"
#include "nf_logevtdef.h"

#include "nf_api_disk.h"
#include "nf_api_live.h"

#include "nf_api_ipcam.h"

#include "nf_util_device.h"
#include "nf_util_time.h"

//#include <nmf_mrtp_pipe.h>

#ifdef USE_RTSP4VLC
#include "nf_nvs_common.h"
#endif

#include "ivca_def.h"
#include "libivcam.h"

#include "nf_meta_data.h"
//#include <nmf_display.h>


#include "nf_ipcam_zmq_utils.h"
#include "nf_ipcam_sql_utils.h"

#include "nf_ipcam_defs.h"

extern VA_GNR_EVT_DATA* nf_ipcam_get_vabox_popped_event_data();
extern VA_EVT_DATA* nf_ipcam_get_vabox_popped_data();

//#define USE_CALLBACK_FUN
//#define META_DATA_RECORD_ENABLE
#define META_DATA_MERGE
//#define DUMP_META_DATA

//#define AI_RECORD_ENABLE

#define GST_BUF_META_SIZE (32768*2)
#define GST_BUF_META_TIMESTAMP (1)
#define META_MAX_SIZE (3000)
#define META_OPER_DEBUG 0 

NF_META *nf_meta = NULL;

Meta_Data_Display_Mode meta_data_display = 0;
int meta_data_display_ch = -1;
int meta_data_display_src = 0;

typedef struct _META_STAT {
	int cnt[NUM_ACTIVE_CH];
	int size[NUM_ACTIVE_CH];	
} META_STAT;

META_STAT meta_stat;
META_STAT ai_meta_stat;

int searched_frame;

GobjBuddyBuffer	*gst_buf_meta[NUM_ACTIVE_CH];
int gst_buf_meta_offset[NUM_ACTIVE_CH];
int gst_buf_meta_cnt[NUM_ACTIVE_CH];
int gst_buf_meta_time[NUM_ACTIVE_CH];
guint gst_buf_meta_base_time[NUM_ACTIVE_CH];
guint g_tmp_rulechange[NUM_ACTIVE_CH];
guint g_tmp_metaofs[NUM_ACTIVE_CH];

time_t smart_search_from;
time_t smart_search_to;

static void _meta_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

/*
 * @return
 *  - 0 if the point is exterior.
 *  - 1 if the point is interior.
*/

static int
check_zone(ivca_s16_t x, ivca_s16_t y,
		ivca_s32_t npts, ivca_point_t *pt)
{
	ivca_s32_t i, j, left = 0, dotp;
	ivca_point_t *p = pt;

	for (i = 0, j = npts - 1; i < npts; j = i, i++) {
		if ( (p[i].y < y && y <= p[j].y) || (p[j].y < y && y <= p[i].y) ) {
			/* Check the sign of dot(l, x). */
			dotp = (p[j].y - p[i].y) * x + (p[i].x - p[j].x) * y +
					p[j].x * p[i].y - p[i].x * p[j].y;
			if ( p[j].y < p[i].y )
				dotp = -dotp;
			if ( dotp < 0 )
				left++;
		}
	}
	return left & 1;	/* Odd count means (x, y) is interior point */
}	/* _point_polygon_test(... */

// for AI
NF_AI_META *nf_ai_meta = NULL;

static void 
record_sst_ch_reset(NF_RECORD_SST *psst)
{
	psst->stream_id = -1;	
				
	memset( &psst->last_iframe_header, 0x00, sizeof(ICODEC_HEADER));
	memset( &psst->last_header, 0x00, sizeof(ICODEC_HEADER));
	
	return;	
}

void init_gst_buf_meta(NF_RECORD_SST *psst)
{
	int ch;

	g_mutex_lock(nf_meta->lock);
	
	record_sst_ch_reset( psst );
	

	for(ch =0 ; ch < NUM_ACTIVE_CH ; ch++){
		if(gst_buf_meta[ch] != NULL)
			g_object_unref(gst_buf_meta[ch]);
		gst_buf_meta[ch] = NULL;
		gst_buf_meta_offset[ch] = 0;
		gst_buf_meta_cnt[ch] = 0;
		g_tmp_rulechange[ch] = 0;
	}
	printf("~~ init_gst_buf_meta ~~\n");
	
	g_mutex_unlock(nf_meta->lock);

}

void nf_record_meta_close()
{
	// ALL meta record stream close
	
	int ch;
	gint	ret = 0;

	printf("[META] nf_record_meta_close ENTER!!! \n");

	g_mutex_lock(nf_meta->lock);
	
	for(ch =0 ; ch < NUM_ACTIVE_CH ; ch++){
		NF_RECORD_SST		*psst = &nf_meta->sst[ch];

		if(psst->stream_id >= 0){
			sst_record_close( psst->stream_id, 0);
			g_assert( ret == 0);
			record_sst_ch_reset( psst );
		}
		
		if(gst_buf_meta[ch] != NULL)
			g_object_unref(gst_buf_meta[ch]);
		gst_buf_meta[ch] = NULL;
		gst_buf_meta_offset[ch] = 0;
		gst_buf_meta_cnt[ch] = 0;
		g_tmp_rulechange[ch] = 0;
	}

	g_mutex_unlock(nf_meta->lock);

	printf("[META] nf_record_meta_close EXIT!!! \n");
}

int  nf_meta_data_get_frame_cnt()
{
	return searched_frame;
}

void nf_meta_data_reset_cnt()
{
	searched_frame = 0;
}

int nf_meta_data_display_live_on(int ch, int src)
{
	meta_data_display = META_DATA_DISPLAY_LIVE_ON;
#if META_OPER_DEBUG	
	printf("\nnf_meta_data_display_live_on CH %d state %d \n", ch, src);
#endif
	meta_data_display_ch = ch;
	meta_data_display_src = ch;
}

int nf_meta_data_display_live_off()
{
	meta_data_display = META_DATA_DISPLAY_OFF;
}

int nf_meta_data_display_playback_on()
{
	meta_data_display = META_DATA_DISPLAY_PLAYBACK_ON;
}

int nf_meta_data_display_playback_off()
{
	meta_data_display = META_DATA_DISPLAY_OFF;
}

int get_vca_enable(int chan)
{
	int check = nf_aciton_get_double_knock_ch(chan);
	if(!check)
		return 0;
	return nf_meta->sched[chan];
}

int get_ai_enable(int chan)
{
	int check = nf_aciton_get_double_knock_ch(chan);
	if(!check)
		return 0;
	return nf_ai_meta->sched[chan];
}
	
int nf_smart_search_set_rule(int chan, ivca_rule_t *rules,time_t from, time_t to)
{
	smart_search_from = from;
	smart_search_to = to;
	return ivcam_vca_set_rules(nf_meta->vca_model[chan], rules);
}

int nf_smart_search_reset(int chan, ivca_rule_t *rules)
{
	ivcam_vca_release(nf_meta->vca_model[chan]);
	
	nf_meta->vca_model[chan] = ivcam_vca_create(chan,200,32,rules);
	
	return 0;
}

void nf_meta_data_push(gpointer frame)
{	
	while( g_async_queue_length( nf_meta->queue ) > (480*4) ) g_usleep(10*1000);

	g_async_queue_push( nf_meta->queue, frame );
}

void nf_ai_meta_data_push(gpointer frame)
{	
	while( g_async_queue_length( nf_ai_meta->queue ) > (480*4) ) g_usleep(10*1000);

	g_async_queue_push( nf_ai_meta->queue, frame );
}

void set_meta_data_callback(MatadataCbHandoff cb_handoff_meta_data , CounterCbHandoff cb_handoff_counter)
{
	nf_meta->fxn_get_meta_data = cb_handoff_meta_data;
	nf_meta->fxn_get_counter = cb_handoff_counter;
}

void cb_display_pipe_put_meta_data(GObject *buffer, 
                                gpointer user_data)
{
	ICODEC_HEADER	*pheader;

       pheader = gobj_buddy_buffer_buf_get_addr(buffer);
		
	//pb data
	pheader->chan = pheader->chan + 16;

	void *tmp_gst_ret = NULL;
     tmp_gst_ret =   g_object_ref(buffer);
	if(tmp_gst_ret == NULL) {
			fprintf(stderr, "- ERROR - [%s:%d] g_object_ref ret is NULL\n", __FUNCTION__, __LINE__);
	}
	nf_meta_data_push(buffer);

}

static gboolean 
_is_record_ch_reopen( guint ch, ICODEC_HEADER *pheader, ICODEC_HEADER *prev_pheader)
{
	
	// ������ ���� ��������� ���ؼ� ���ڵ� ���°� �ٲ���� Ȯ���ؾ� �Ѵ�.			
	if(
		   (pheader->codec		== prev_pheader->codec		)
		&& (pheader->version	== prev_pheader->version	)
		&& (pheader->frame_rate	== prev_pheader->frame_rate	)
		&& (pheader->resolution	== prev_pheader->resolution	)
		&& (pheader->flags		== prev_pheader->flags		)  )
		return 0;
	else
		return 1;
		
}


gchar *
nf_vca_event_type_string(guint32 event_type)
{
	switch ( event_type ) {
		case IVCA_ET_DIR_POS:
			return "Positive direction";
		case IVCA_ET_DIR_NEG:
			return "Negative direction";
		case IVCA_ET_ENTER:
			return "Enter";
		case IVCA_ET_EXIT:
			return "Exit";
		case IVCA_ET_STOPPED:
			return "Stopped";
		case IVCA_ET_ABANDONED:
			return "Abandoned";
		case IVCA_ET_REMOVED:
			return "Removed";
		case IVCA_ET_LOITERED:
			return "Loitering";
		case IVCA_ET_COUNTER:
			return "Counter";
		case IVCA_ET_TAMPER:
			return "Camera tamper";
		case IVCA_ET_INTRUSION:
			return "Intrusion";
	}
	return "Unknown";
}	/* nf_vca_event_type_string(... */

gchar *
nf_vca_class_type_string(guint8 class_type)
{
	switch ( class_type ) {
		case IVCA_CLASS_PERSON:
			return "Person";
		case IVCA_CLASS_VEHICLE:
			return "Vehicle";
		case IVCA_CLASS_PEOPLE:
			return "People";
		case IVCA_CLASS_NA:
			return "N.A.";
		case IVCA_CLASS_UNKNOWN:
		default:
			return "Unknown";
	}
}	/* nf_vca_class_type_string(... */

int
nf_api_vca_get_event_string(ivca_rule_event_t* evt, gchar *out)
{
	gchar key[64];

	switch ( evt->type ) {
		case IVCA_ET_DIR_POS:
			return sprintf(out,
					"Ch %u Object %u crossed over zone %u in positive direction",
					evt->ch + 1,evt->object_id, evt->rule_id);
		case IVCA_ET_DIR_NEG:
			return sprintf(out,
					"Ch %u Object %u crossed over zone %u in negative direction",
					evt->ch + 1,evt->object_id, evt->rule_id);
		case IVCA_ET_ENTER:
			return sprintf(out, "Ch %u Object %u entered to zone %u",
					evt->ch + 1,evt->object_id, evt->rule_id);
		case IVCA_ET_EXIT:
			return sprintf(out, "Ch %u Object %u exited from zone %u",
					evt->ch + 1,evt->object_id, evt->rule_id);
		case IVCA_ET_STOPPED:
			return sprintf(out, "Ch %u Object %u stopped at zone %u", evt->ch + 1,evt->object_id,evt->rule_id);
		case IVCA_ET_REMOVED:
			return sprintf(out, "Ch %u Object %u removed from zone %u", evt->ch + 1,evt->object_id,evt->rule_id);
		case IVCA_ET_COUNTER:
			sprintf(key, "cam.vca.rule.R%u.C%u.e_value", evt->ch + 1, evt->rule_id);
			return sprintf(out, "Ch %u Counter %u exceeded %d", evt->ch + 1,evt->rule_id,
					nf_sysdb_get_int(key));
		case IVCA_ET_TAMPER:
			return sprintf(out, "Ch %u Tamper detected",evt->ch + 1);
		case IVCA_ET_ABANDONED:
			return sprintf(out, "Ch %u Object Abandoned zone %u", evt->ch + 1,evt->rule_id);
		case IVCA_ET_LOITERED:
			return sprintf(out, "Ch %u Object %u Loiter zone %u", evt->ch + 1,evt->object_id,evt->rule_id);
		default:
			return sprintf(out, "Unknown event");
	}
}	/* nf_api_vca_get_event_string(... */


static gboolean
meta_data_queue_process( gpointer frame, guint chan, gint mode)
{	
	
#if 0	// ksi_test
	GobjBuddyBuffer	*buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = buffer->frame ? buffer->frame : gobj_buddy_buffer_buf_get_addr(frame);	
	
	NF_RECORD_MAN		*pman = &nf_meta->man[chan];	
	NF_RECORD_SST		*psst = &nf_meta->sst[chan];	
		
	gint				ret = 0;
	gboolean			is_icodec_change = 0;
	guint 			sst_chan = chan + 48;

	//captainnn test
	pheader->frame_type = NF_FRAME_TYPE_I;
	pheader->frame_rate = 1;

	if( _is_record_ch_reopen(chan, pheader, &psst->last_header) ){
		is_icodec_change = 1;
	}
	
	
	if( psst->stream_id >=0 && (is_icodec_change) ){
					
		gboolean is_flush = 0;
		guint pre_close_sec = 0;
				
		if( (pman->record_reason > 1 && pman->record_reason < 6) && pman->pre_record ) {
			is_flush = 1;			
			pre_close_sec = nf_meta->pre_rec_time;
		}
						
		g_message("sst_record_close ch[%2d] sid[%2d] pre_flush[%d][%d]  [%d]", 
					sst_chan, psst->stream_id , is_flush, pre_close_sec, psst->put_frame_count ); 

		ret = sst_record_close( psst->stream_id, pre_close_sec);
		g_assert( ret == 0);
		
		record_sst_ch_reset( psst );									
	}


	if( psst->last_header.timestamp > pheader->timestamp ||
			(psst->last_header.timestamp == pheader->timestamp 
				&& psst->last_header.timestampl > pheader->timestampl ) )
	{		
		//dump_icodec_header("ERR TS skip frame", pheader);
		printf("ERR TS skip frame %d  %d ",psst->last_header.timestamp,pheader->timestamp);
		goto out;
	}
	
	if( psst->stream_id == -1 && pman->record_reason != 0 && nf_record_is_rec_off() != TRUE )
	{
		gint stream_id = -1;			
		gint pre_rec_sec = (pman->record_reason == NF_RECORD_REASON_PRE) ? nf_meta->pre_rec_time:0;

		stream_id = sst_record_open( (guint8)sst_chan,
										(guint8)pre_rec_sec,
										(guint8)pman->record_reason, 
										(guint8)pheader->codec,
										(guint8)pheader->resolution,
										(guint8)pheader->frame_rate,
										//(guint8)pman->fps,
										(guint8)pman->quality);
			
		g_message("sst_record_open  ch[%2d] sid[%2d] reason[%d] pre[%d]codec[%d]res[%x]fps[%2d]q[%d]",
						sst_chan, stream_id ,
						pman->record_reason,
						pre_rec_sec, // pre_max
						pheader->codec,
						pheader->resolution,
						pheader->frame_rate,
						pman->quality );

		if( stream_id <0)
		{
			//dump_icodec_header("ERR header frame", pheader);
			g_message("sst_record_open result[%d](%s)", stream_id, sst_get_error_string(stream_id)); 	
			
			g_assert( stream_id >= 0 || stream_id == -SST_ERR_DISKFULL );	// sid ���� ���и� g_assert

		}else{
			psst->stream_id = stream_id;			
		}

	}
	
	if( psst->stream_id >=0 && pheader->frame_type == NF_FRAME_TYPE_I)
	{
		if(mode)
		{
			void *tmp_gst_ret = NULL;
			tmp_gst_ret = g_object_ref(frame);
			if(tmp_gst_ret == NULL)
				fprintf(stderr, "- ERROR - [%s:%d] g_object_ref ret is NULL\n", __FUNCTION__, __LINE__);
		}

		
		//printf("sst_record_put_frame timestamp %d",pheader->timestamp);
		ret = sst_record_put_frame ( psst->stream_id, pheader );
		if(ret){

#ifdef ENABLE_PUT_FRAME_ASSERT
       	    g_assert( ret == -SST_ERR_DISKFULL || ret == -SST_ERR_INVPARAM );
#endif       	    
			if( ret == -SST_ERR_INVPARAM )
			{

				if(psst->err_frame_ts != pheader->timestamp){
					++psst->err_frame_count;

					g_message("sst_record_put_frame result[%d](%s)", ret, sst_get_error_string(ret));
					//dump_icodec_header("ERR frame", pheader);

				}
				psst->err_frame_ts = pheader->timestamp;

#ifdef ENABLE_PUT_FRAME_ASSERT
				g_assert( psst->err_frame_count < PUT_FRAME_ASSERT_CNT);	// 1 min
#endif
			}					
			g_object_unref(frame);
			
		}else{
			
			++psst->put_frame_count;
			memcpy( &psst->last_header, pheader, sizeof(ICODEC_HEADER));		

			meta_stat.cnt[chan] ++;
			meta_stat.size[chan] += pheader->frame_size + sizeof(ICODEC_HEADER);
		}
	}
	else {
	

	}
	
#endif

out:	
	return 1;
	
}




#define SYSDB_CAM_VCA_CFG_SCHED		"cam.vca.cfg.R%d.sched"
static gboolean
meta_data_timer(gpointer data)
{	
	gint date, hour, ch;
	time_t tick;
	struct tm cur_cal_time;
	GTimeVal cur_time;

	char buff[256];
	gint idx;	

	tick = time(NULL);
	nf_datetime_localtime(&tick, nf_meta->is_dst, &cur_cal_time);
						
	hour = cur_cal_time.tm_hour;
	date = cur_cal_time.tm_wday;

	for(ch=0;ch< NUM_ACTIVE_CH;ch++){
		sprintf(buff, SYSDB_CAM_VCA_CFG_SCHED, ch);
		idx = date * 24 + hour;
		nf_meta->sched[ch] = nf_sysdb_get_strmap(buff, idx) -'0';
	}

	return TRUE;
	
}

#define SYSDB_CAM_DVABX_CFG_SCHED		"cam.dvabx.cfg.R%d.sched"
char meta_ch_mask[NUM_ACTIVE_CH] = {0,};
static gboolean
ai_meta_data_timer(gpointer data)
{	
	gint date, hour, ch;
	time_t tick;
	struct tm cur_cal_time;
	GTimeVal cur_time;

	char buff[256];
	gint idx;	

	tick = time(NULL);
	nf_datetime_localtime(&tick, nf_ai_meta->is_dst, &cur_cal_time);
						
	hour = cur_cal_time.tm_hour;
	date = cur_cal_time.tm_wday;

	for(ch=0;ch< NUM_ACTIVE_CH;ch++){
		sprintf(buff, SYSDB_CAM_DVABX_CFG_SCHED, ch);
		idx = date * 24 + hour;
		nf_ai_meta->sched[ch] = nf_sysdb_get_strmap(buff, idx) -'0';
	}

	if((meta_data_display_ch != -1) && (meta_ch_mask[meta_data_display_ch]==0)){
		void *r;
		int size = 2 * sizeof(int);
		//printf("clear meta data !!!\n");
		r = malloc(size);
		((int *)r)[0] = meta_data_display_ch;
		((int *)r)[1] = 0;
		
		nf_notify_fire_pointer("ai_trackinfo", r, (int)size);
		free(r);
	}
	else if(meta_data_display_ch != -1)
		meta_ch_mask[meta_data_display_ch] = 0;
		

	return TRUE;
	
}

static gboolean
meta_data_stat(gpointer data)
{	
	int i;
	
	printf("\n meta_data_stat (in 5 sec) \n");

	printf("count : ");
	for(i =0;i< NUM_ACTIVE_CH; i++){
		printf("%d ", meta_stat.cnt[i]);
	}
	printf("\n");

	printf("size : ");
	for(i =0;i< NUM_ACTIVE_CH; i++){
		printf("%d ", meta_stat.size[i]);
	}
	printf("\n");

	memset(&meta_stat, 0x00 , sizeof(meta_stat));

	printf("ai meta_data_stat (in 5 sec) \n");

	printf("count : ");
	for(i =0;i< NUM_ACTIVE_CH; i++){
		printf("%d ", ai_meta_stat.cnt[i]);
	}
	printf("\n");

	memset(&ai_meta_stat, 0x00 , sizeof(ai_meta_stat));

	return TRUE;
	
}

int enc_b64_decode_str(char *p_dst, int i_dst, const char *p_src )
{
    static const int b64[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
    };
    char *p_start = p_dst;
    char *p = (char *)p_src;

    int i_level;
    int i_last;

    for( i_level = 0, i_last = 0; (size_t)( p_dst - p_start ) < i_dst && *p != '\0'; p++ )
    {
        const int c = b64[(unsigned int)*p];
        if( c == -1 )
            continue;

        switch( i_level )
        {
            case 0:
                i_level++;
                break;
            case 1:
                *p_dst++ = ( i_last << 2 ) | ( ( c >> 4)&0x03 );
                i_level++;
                break;
            case 2:
                *p_dst++ = ( ( i_last << 4 )&0xf0 ) | ( ( c >> 2 )&0x0f );
                i_level++;
                break;
            case 3:
                *p_dst++ = ( ( i_last &0x03 ) << 6 ) | c;
                i_level = 0;
        }
        i_last = c;
    }

    return p_dst - p_start;
}

int nf_meta_data_analysis(GObject *frame, guint chan)
{

	GobjBuddyBuffer	*buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = gobj_buddy_buffer_buf_get_addr(G_OBJECT(buffer));	
	int i,j;
	int meta_ofs=0;
	int frame_size = pheader->frame_size;
	
	ivca_meta_hdr_t *pMeta;
	ivca_meta_ckh_t *pCKH;
	ivca_meta_obj_t *pObj;
	ivca_meta_cnt_t *pCnt;

	
	ivca_meta_ckh_t *pObj_start_pCKH;

	ivca_meta_obj_t *pObj_start;
	ivca_meta_cnt_t *pCnt_start;
#if META_OPER_DEBUG	
	printf("mode %d ch_num %d type %d size %d \n",meta_data_display,pheader->chan,pheader->frame_type,pheader->frame_size);
#endif	

	unsigned char	*pdata = (unsigned char*)((char*)(pheader) +  sizeof(ICODEC_HEADER)); //+ 1;//sizeof(ICODEC_HEADER);
	static int cap=0;

	int send_meta=0;
	int obj_cnt=0;
	int cnt_cnt=0;
	char * p;
	guint frame_timestamp = pheader->timestamp;
	
	size_t size;
	void *r;
	int vca_model_ch = (meta_data_display == META_DATA_DISPLAY_LIVE_ON ? pheader->chan : NUM_ACTIVE_CH);

	int meta_start = 0;
	char* meta_dec = NULL;

	if((pdata[4] & 0x1f) == 0x06 && pdata[5] == 5){
		// h.264
		int sei_size=0;
		guchar temp;
		
		meta_start = 6; // h.264 sei

		for(i=0; pdata[i+6] == 0xff;i++){
			sei_size += pdata[i+6] + 1;
			meta_start++;
		}
		if(pdata[i+6+1] != 0xfe){
			sei_size += pdata[i+6] + 1;
			meta_start++;
		}
			
		sei_size += 6; //offset

		if(frame_size != sei_size){
			printf("\n nfcodec size %d sei_size %d %d %x %x %x %x !!!!\n", frame_size, sei_size, i, pdata[6], pdata[7], pdata[8], pdata[9]);
		return -1;
			}

	}
	else if( (((pdata[4] >> 1) & 0x3f) == 39 || ((pdata[4] >> 1) & 0x3f) == 40) && pdata[5] == 1 && pdata[6] == 5)
	{
		// h.265
		int sei_size=0;
		guchar temp;
		
		meta_start = 7; // h.265 sei

		for(i=0; pdata[i+7] == 0xff;i++){
			sei_size += pdata[i+7] + 1;
			meta_start++;
		}
		if(pdata[i+7+1] != 0xfe){
			sei_size += pdata[i+7] + 1;
			meta_start++;
		}
			
		sei_size += 7; //offset

		if(frame_size != sei_size){
			printf("\n nfcodec size %d sei_size %d %d %x %x %x %x !!!!\n", frame_size, sei_size, i, pdata[6], pdata[7], pdata[8], pdata[9]);
			return -1;
		}

	}
	else{
		printf("ERROR !!! SEI ???? \n");
		return -1;

	}

	meta_start += 16; //+UUID

	meta_dec = malloc(frame_size -meta_start);

	if(meta_dec == NULL){
		printf("ERROR !!!MALLOC \n");
		return -1;
	}

	enc_b64_decode_str(meta_dec, frame_size -meta_start , pdata + meta_start);
	
	for(meta_ofs=0;meta_ofs<frame_size;meta_ofs++){
		if(*(meta_dec+meta_ofs) == 'M' && *(meta_dec+meta_ofs+1) == 'E' && *(meta_dec+meta_ofs+2) == 'T' && *(meta_dec+meta_ofs+3) == 'A')
			break;

	}
	
	if(meta_ofs >= frame_size){
		// ERROR
		free(meta_dec);
		printf("ERROR !!!DIDNOT FIND META meta_ofs %d size %d \n",meta_ofs,frame_size);
		return -1;
	}

#if 0	

	/////////// timestamp gap save
	
	pMeta = pdata + meta_ofs;
	g_tmp_metaofs[chan] = meta_ofs;
	
	if(gst_buf_meta[chan] == NULL || gst_buf_meta_offset[chan] == 0){
		g_tmp_rulechange[chan] = 0; 
		gst_buf_meta_base_time[chan] = frame_timestamp;
	}
	else{
		g_tmp_rulechange[chan] = frame_timestamp - gst_buf_meta_base_time[chan];
	}
#endif	
	
	if((meta_data_display == META_DATA_DISPLAY_LIVE_ON && chan == meta_data_display_ch)){
			
		//pMeta = pdata + meta_ofs;
		pMeta = meta_dec;
#ifdef	DUMP_META_DATA
		printf("ch %d meta_ofs %d size %d n_height %d n_width %d ckcount %d \n",chan,meta_ofs,pMeta->size, pMeta->n_height,pMeta->n_width, pMeta->ckcount);
#endif
		pCKH = (void *)(pMeta + 1);		

#ifdef	DUMP_META_DATA
		printf("pCKH size %d type %d obj_cnt(priv) %d \n",pCKH->size, pCKH->type, pCKH->priv);
#endif

		//captainnn start code work around
if(0){
		int k=0;
		char* temp;

		temp = pMeta;

		for(k=0; k < frame_size - meta_ofs -1 ; k++){
			if(temp[k] == 0x12 && temp[k+1] == 0x34){// && temp[k] == 0x56 && temp[k] == 0x78){
				temp [k] = 0x00;
				temp [k+1] = 0x00;
				//temp [k+2] = 0x00;
				//temp [k+3] = 0x01;
			}
		}
}
		//test
		if(pMeta->ckcount > 2)
			pMeta->ckcount = 2;
		
		for(i=0;i<pMeta->ckcount;i++){

			switch(pCKH->type){
				case IVCA_META_CKTYPE_JNK:
					break;
				case IVCA_META_CKTYPE_OBJ:
					pObj_start_pCKH = pCKH;
					pObj = (void *)(pCKH + 1);
					pObj_start = (void *)(pCKH + 1);
					obj_cnt = pCKH->priv;
					//if(pObj_start->valid3d){
					//	printf("width %d height %d speed %d \n",pObj_start->width3d,pObj_start->height3d,pObj_start->speed3d);////

					//}
					send_meta=1;
					break;
				case IVCA_META_CKTYPE_CNT:
					pCnt = (void *)(pCKH + 1);
					pCnt_start = (void *)(pCKH + 1);
					cnt_cnt = pCKH->priv;		
					break;
					
				case IVCA_META_CKTYPE_FGM:
					break;
				case IVCA_META_CKTYPE_SHM:
					break;
				case IVCA_META_CKTYPE_DBG:
					break;
				default:
					break;
			}

			p = pCKH;
			p += pCKH->size + sizeof(ivca_meta_ckh_t);
			pCKH = p;
			
		}
		
		if(obj_cnt){
			int objcount=0;
			int result=0;
			//static int send_data=0;
			
			result = ivcam_vca_process(nf_meta->vca_model[vca_model_ch] , pMeta, pObj_start_pCKH);
			
			if(result < 0)
				printf("ivcam_vca_process error @@!!@!@!@@\n");

			objcount = ivcam_vca_get_trackinfo(nf_meta->vca_model[vca_model_ch] , 200, nf_meta->m_TrackObjs);

				
#ifdef USE_CALLBACK_FUN
			size = (size_t)objcount * sizeof(ivcam_obj_t);
			r = malloc(size);
			memcpy((int *)r, (void *)nf_meta->m_TrackObjs, (size_t)objcount* sizeof(ivcam_obj_t));
			
			if(nf_meta->fxn_get_meta_data != NULL)
				nf_meta->fxn_get_meta_data(chan, objcount , r);
			free(r);

#else
			static int skip_flag= 0;
			int tmp;
			tmp = obj_cnt/10 + (meta_data_display_src == 0 ? 2 : 1);
			skip_flag++;
			if(skip_flag%tmp == 0){
				size = 2 * sizeof(int) + (size_t)objcount * sizeof(ivcam_obj_t);
//printf("SKSHIN-2] NOTIFY TO (%d) (%d), (%d), (%d)\n", chan, objcount, size, sizeof(ivcam_obj_t));

				r = malloc(size);
				((int *)r)[0] = chan;//pMeta->chid;
				((int *)r)[1] = objcount;
				
				memcpy((int *)r + 2, (void *)nf_meta->m_TrackObjs, (size_t)objcount* sizeof(ivcam_obj_t));
				
				nf_notify_fire_pointer("vca_trackinfo", r, (int)size);
				free(r);
			}
#endif
			
		}
		
		if(cnt_cnt){
			
#ifdef USE_CALLBACK_FUN
			size = (size_t)cnt_cnt * sizeof(ivca_meta_cnt_t);
			r = malloc(size);
			memcpy((int *)r, (void *)pCnt_start, (size_t)cnt_cnt* sizeof(ivca_meta_cnt_t));
			
			if(nf_meta->fxn_get_counter != NULL)
				nf_meta->fxn_get_counter(chan, cnt_cnt , r);
			free(r);
#else			
			size = 2 * sizeof(int) + (size_t)cnt_cnt * sizeof(ivca_meta_cnt_t);
			r = malloc(size);
			((int *)r)[0] = chan;
			((int *)r)[1] = cnt_cnt;
			memcpy((int *)r + 2, (void *)pCnt_start, (size_t)cnt_cnt* sizeof(ivca_meta_cnt_t));
			
			nf_notify_fire_pointer("vca_counter", r, (int)size);
			free(r);
#endif

		}
	}
	
	free(meta_dec);

	return 0;

}


int nf_meta_data_analysis_smart(GObject *frame, guint chan)
{

#if 0	// ksi_test
	GobjBuddyBuffer	*buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = buffer->frame ? buffer->frame : gobj_buddy_buffer_buf_get_addr(frame);	
	int i,j;
	int meta_ofs=0;
	int frame_size = pheader->frame_size;
	
	ivca_meta_hdr_t *pMeta;
	ivca_meta_ckh_t *pCKH;
	ivca_meta_obj_t *pObj;
	ivca_meta_cnt_t *pCnt;

	
	ivca_meta_ckh_t *pObj_start_pCKH;

	ivca_meta_obj_t *pObj_start;
	ivca_meta_cnt_t *pCnt_start;

	char	*pdata = pheader + 1;//sizeof(ICODEC_HEADER);
	static int cap=0;

	int send_meta=0;
	int obj_cnt=0;
	int cnt_cnt=0;
	char * p;
	guint frame_timestamp = pheader->timestamp;
	
	int objcount=0;
	int result=0;
	
	size_t size;
	void *r;
	//int vca_model_ch = (meta_data_display == META_DATA_DISPLAY_LIVE_ON ? pheader->chan : NUM_ACTIVE_CH);
	int vca_model_ch = NUM_ACTIVE_CH;
	
	ivca_rule_event_t* event_data;
	int e_count=0;

	while(1){
		for(;meta_ofs<frame_size;meta_ofs++){
			if(*(pdata+meta_ofs) == 'M' && *(pdata+meta_ofs+1) == 'E' && *(pdata+meta_ofs+2) == 'T' && *(pdata+meta_ofs+3) == 'A')
				break;

		}

		if(meta_ofs >= frame_size){
			break;
		}

		obj_cnt = 0;
		
		pMeta = pdata + meta_ofs;
		meta_ofs += pMeta->size;
		pCKH = (void *)(pMeta + 1);		
		
		for(i=0;i<pMeta->ckcount;i++){
			switch(pCKH->type){
				case IVCA_META_CKTYPE_OBJ:
					pObj_start_pCKH = pCKH;
					pObj = (void *)(pCKH + 1);
					pObj_start = (void *)(pCKH + 1);
					obj_cnt = pCKH->priv;
					//send_meta=1;
					break;
				case IVCA_META_CKTYPE_JNK:
				case IVCA_META_CKTYPE_CNT:
				case IVCA_META_CKTYPE_FGM:
				case IVCA_META_CKTYPE_SHM:
				case IVCA_META_CKTYPE_DBG:
					break;
				default:
					break;
			}
			p = pCKH;
			p += pCKH->size + sizeof(ivca_meta_ckh_t);
			pCKH = p;

		}
#if 0
		if(obj_cnt){
			//static int send_data=0;
			
			result = ivcam_vca_process(nf_meta->vca_model[vca_model_ch] , pMeta, pObj_start_pCKH);
			
			if(result < 0)
				printf("ivcam_vca_process error @@!!@!@!@@\n");

			objcount = ivcam_vca_get_trackinfo(nf_meta->vca_model[vca_model_ch] , 200, nf_meta->m_TrackObjs);

			searched_frame++;
			if(searched_frame%2000 ==0){
#ifdef USE_CALLBACK_FUN
				size = (size_t)objcount * sizeof(ivcam_obj_t);
				r = malloc(size);
				memcpy((int *)r, (void *)nf_meta->m_TrackObjs, (size_t)objcount* sizeof(ivcam_obj_t));
				if(nf_meta->fxn_get_meta_data != NULL)
					nf_meta->fxn_get_meta_data(chan -16, objcount , r);
				free(r);

#else
				size = 2 * sizeof(int) + (size_t)objcount * sizeof(ivcam_obj_t);
				r = malloc(size);
				((int *)r)[0] = chan -16 ;
				((int *)r)[1] = objcount;
				memcpy((int *)r + 2, (void *)nf_meta->m_TrackObjs, (size_t)objcount* sizeof(ivcam_obj_t));
				
				nf_notify_fire_pointer("vca_trackinfo", r, (int)size);
				free(r);
#endif
			}

		

		}

		if(1){
			int e_count=0;
			ivca_rule_event_t* event_data;
			
			result = ivcam_vca_get_events(nf_meta->vca_model[vca_model_ch], &e_count, &event_data);
			if(result < 0)
				printf("ivcam_vca_get_events FAIL !!!!\n");

			if(e_count){
				event_data->timestamp = frame_timestamp + pMeta->rulechange_num;

				
				if(e_count >1){
					ivca_rule_event_t* temp = event_data;
					
					//printf("ivcam_vca_get_events DETECT vca_model_ch %d count %d  !!!!\n",vca_model_ch,e_count);
					
					for(i=0;i< e_count;i++,temp++){
						temp->timestamp = frame_timestamp;
					}
				}

				size = 2 * sizeof(int) + e_count*sizeof(ivca_rule_event_t);
				r = malloc(size);
				((int *)r)[0] = chan;
				((int *)r)[1] = e_count;
				
				memcpy((int *)r + 2, (void *)event_data, e_count*sizeof(ivca_rule_event_t));
				
				nf_notify_fire_pointer("vca_event", r, (int)size);
				free(r);
			}
		}
		
#else
			
		//printf("from smart_search_from %d to %d frame %d frame_timestamp %d pMeta->rulechange_num %d  framenum %d size %d \n",smart_search_from,smart_search_to,frame_timestamp + pMeta->rulechange_num,frame_timestamp,pMeta->rulechange_num,pMeta->framenum,frame_size);
		if(smart_search_from > frame_timestamp + pMeta->rulechange_num)
			continue;
		if(smart_search_to < frame_timestamp + pMeta->rulechange_num)
			break;
		
		if(obj_cnt){
			//static int send_data=0;
			
			e_count = ivcam_vca_process_for_smart_search(nf_meta->vca_model[vca_model_ch] , pMeta, pObj_start_pCKH,&event_data);

			if(e_count < 0)
				printf("ivcam_vca_process error @@!!@!@!@@\n");

			searched_frame++;

			if(e_count > 0){
				ivca_rule_event_t* temp = event_data;
				
				//printf("ivcam_vca_get_events DETECT vca_model_ch %d count %d  !!!!\n",vca_model_ch,e_count);
				
				for(i=0;i< e_count;i++,temp++){
					temp->timestamp = frame_timestamp + pMeta->rulechange_num;
				}

				size = 2 * sizeof(int) + e_count*sizeof(ivca_rule_event_t);
				r = malloc(size);
				((int *)r)[0] = chan;
				((int *)r)[1] = e_count;
				
				memcpy((int *)r + 2, (void *)event_data, e_count*sizeof(ivca_rule_event_t));
				
				nf_notify_fire_pointer("vca_event", r, (int)size);
				free(r);
			}
#if 0			
			if(searched_frame%5000 ==0){
				objcount = ivcam_vca_get_trackinfo(nf_meta->vca_model[vca_model_ch] , 200, nf_meta->m_TrackObjs);

#ifdef USE_CALLBACK_FUN
				size = (size_t)objcount * sizeof(ivcam_obj_t);
				r = malloc(size);
				memcpy((int *)r, (void *)nf_meta->m_TrackObjs, (size_t)objcount* sizeof(ivcam_obj_t));
				if(nf_meta->fxn_get_meta_data != NULL)
					nf_meta->fxn_get_meta_data(chan -16, objcount , r);
				free(r);

#else
				size = 2 * sizeof(int) + (size_t)objcount * sizeof(ivcam_obj_t);
				r = malloc(size);
				((int *)r)[0] = chan -16 ;
				((int *)r)[1] = objcount;
				memcpy((int *)r + 2, (void *)nf_meta->m_TrackObjs, (size_t)objcount* sizeof(ivcam_obj_t));
				
				nf_notify_fire_pointer("vca_trackinfo", r, (int)size);
				free(r);
#endif
			}
#endif
		}

#endif
	}
#endif
	return 0;

}


static void*
_nf_meta_thread_func(void *param)
{
	struct sched_param sched;
	gpointer que_poped_data;
	gulong cb_handle;
	guint i, ch, on, start, stop;
	time_t tick;
	struct tm c_tm;
	
       int policy,res=0;

	ICODEC_HEADER	*pheader;
	GobjBuddyBuffer		*frame;

	int record_size;

	#if PRI_ADJUST
		sched.sched_priority = sched_get_priority_max(policy)-2;
	#else		
		sched.sched_priority = sched_get_priority_max(policy)-1;
	#endif
	
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);

	nf_meta->init_done = 1;
	
	for(i=0;i<NUM_ACTIVE_CH + 1;i++){
		nf_meta->vca_model[i] = ivcam_vca_create(i,200,32,NULL);
		if(nf_meta->vca_model[i]  == NULL)
			printf("\n vca_meta == NULL \n");
	}	

	for(i=0;i<NUM_TOTAL_CHANNEL;i++){
		nf_meta->sst[i].stream_id = -1;
		nf_meta->sst[i].last_header.timestamp= 0;

	}

#ifdef META_DATA_RECORD_ENABLE
	for(i=0;i<NUM_ACTIVE_CH;i++){
		gst_buf_meta[i] = NULL;
		gst_buf_meta_offset[i] = NULL;
		gst_buf_meta_cnt[i] = 0;
	}
#endif

	memset(nf_meta->m_TrackObjs, 0x00 , sizeof(nf_meta->m_TrackObjs));
	memset(&meta_stat, 0x00 , sizeof(meta_stat));
	
	nf_timer_add( 1000, meta_data_timer, NULL);

#ifdef META_DATA_RECORD_ENABLE
	nf_timer_add( 5000, meta_data_stat, NULL);
#endif
	
	while (nf_meta->thread_run) {
		que_poped_data = g_async_queue_pop( nf_meta->queue);
		if( que_poped_data == NULL)	// get_data
			continue;
		//printf("g_async_queue_pop !!!\n");

		frame = (GobjBuddyBuffer *)que_poped_data;
		pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(frame);
		record_size = pheader->frame_size + sizeof(ICODEC_HEADER);
		ch = pheader->chan;

		//test
		if(ch < NUM_ACTIVE_CH && record_size > GST_BUF_META_SIZE){
			printf("\n\n Meta data size is too big @@@@@ %d \n\n", record_size);
			g_object_unref(que_poped_data);
			continue; 
		}

		if(ch < NUM_ACTIVE_CH && !(get_vca_enable(ch))){
			g_object_unref(que_poped_data);
			continue; 
		}

		
		g_mutex_lock(nf_meta->lock);

#ifdef META_DATA_RECORD_ENABLE
		if(ch < NUM_ACTIVE_CH)
			res = nf_meta_data_analysis((GObject *)que_poped_data,ch);
		else
			res = nf_meta_data_analysis_smart((GObject *)que_poped_data,ch);
#else
		res = nf_meta_data_analysis((GObject *)que_poped_data,ch);

#endif
			
		if(res < 0){

			g_object_unref(que_poped_data);

		}
		else{

			if(ch < NUM_ACTIVE_CH){

#ifdef META_DATA_RECORD_ENABLE
				//record
				if(ch != 15){ // captainnn sst 63 disable 
				#ifdef META_DATA_MERGE
					
					ivca_meta_hdr_t *pMeta;

					#if 1
					if(gst_buf_meta[ch] == NULL || gst_buf_meta_offset[ch] == 0){
						// init
						gst_buf_meta[ch] = gst_nf_buddy_buffer_new_and_alloc( GST_BUF_META_SIZE , NULL);
						gst_buf_meta_offset[ch] = 0;
						if(gst_buf_meta[ch]){
							memset(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), 0x00 , GST_BUF_META_SIZE);
							pheader->flags = nf_meta->man[ch].record_reason;
							pheader->gst_buffer = gst_buf_meta[ch];
							memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), gobj_buddy_buffer_buf_get_addr(frame) , record_size);
							pMeta = gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + sizeof(ICODEC_HEADER) + g_tmp_metaofs[ch];
							pMeta->rulechange_num = 0;
							gst_buf_meta_offset[ch] = record_size;
							gst_buf_meta_cnt[ch] = 1;
							//g_object_ref(gst_buf_meta[ch]);
						}
					}
					else{
						if(gst_buf_meta_offset[ch] ==0)
							printf("gst_buf_meta != NULL & gst_buf_meta_offset =0 ~~~~\n");

						if(gst_buf_meta_offset[ch] + (record_size - sizeof(ICODEC_HEADER)) >=  GST_BUF_META_SIZE - META_MAX_SIZE){
							ICODEC_HEADER	*tmp;
							printf("Meta Data Save CH %d frame_cnt %d offset %d size %d \n",ch,gst_buf_meta_cnt[ch], gst_buf_meta_offset[ch],record_size);

							
							memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + gst_buf_meta_offset[ch], gobj_buddy_buffer_buf_get_addr(frame) + sizeof(ICODEC_HEADER), record_size - sizeof(ICODEC_HEADER));
							pMeta = gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + gst_buf_meta_offset[ch] + g_tmp_metaofs[ch];
							pMeta->rulechange_num = g_tmp_rulechange[ch];
							gst_buf_meta_offset[ch] += record_size - sizeof(ICODEC_HEADER);
							gst_buf_meta_cnt[ch]++;

							
							tmp = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]);
							tmp->frame_size = gst_buf_meta_offset[ch];
							// overflow . send & save
							meta_data_queue_process(gst_buf_meta[ch], ch, 0);
							//printf("g_object_unref(gst_buf_meta[ch])\n");
							//g_object_unref(gst_buf_meta[ch]);
							//printf("g_object_unref2(gst_buf_meta[ch])\n");
							gst_buf_meta_offset[ch] = 0;
							/*
							gst_buf_meta[ch] = gst_nf_buddy_buffer_new_and_alloc( GST_BUF_META_SIZE , NULL);
							gst_buf_meta_offset[ch] = 0;
							if(gst_buf_meta[ch]){
								memset(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), 0x00 , GST_BUF_META_SIZE);
								pheader->flags = nf_meta->man[ch].record_reason;
								pheader->gst_buffer = gst_buf_meta[ch];
								memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), gobj_buddy_buffer_buf_get_addr(frame) , record_size);
								pMeta = gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + sizeof(ICODEC_HEADER) + g_tmp_metaofs[ch];
								printf("REC META rulechange_num %d framenum %d sig %s size %d \n",pMeta->rulechange_num,pMeta->framenum,pMeta->signature,pheader->frame_size);
								pMeta->rulechange_num = 0;
								gst_buf_meta_offset[ch] = record_size;
								gst_buf_meta_cnt[ch] = 1;
								
								//g_object_ref(gst_buf_meta[ch]);
							}
							*/
						}
						else{
							// save only
							memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + gst_buf_meta_offset[ch], gobj_buddy_buffer_buf_get_addr(frame) + sizeof(ICODEC_HEADER), record_size - sizeof(ICODEC_HEADER));
							pMeta = gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + gst_buf_meta_offset[ch] + g_tmp_metaofs[ch];
							pMeta->rulechange_num = g_tmp_rulechange[ch];
							gst_buf_meta_offset[ch] += record_size - sizeof(ICODEC_HEADER);
							gst_buf_meta_cnt[ch]++;
						}

					}
					
					g_object_unref(que_poped_data);

					#else
					if(gst_buf_meta[ch] == NULL){
						// init
						gst_buf_meta[ch] = gst_nf_buddy_buffer_new_and_alloc( GST_BUF_META_SIZE , NULL);
						gst_buf_meta_offset[ch] = 0;
						if(gst_buf_meta[ch]){
							memset(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), 0x00 , GST_BUF_META_SIZE);
							pheader->flags = nf_meta->man[ch].record_reason;
							pheader->gst_buffer = gst_buf_meta[ch];
							memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), gobj_buddy_buffer_buf_get_addr(frame) , record_size);
							gst_buf_meta_offset[ch] = record_size;
							gst_buf_meta_cnt[ch] = 1;
							gst_buf_meta_time[ch] = pheader->timestamp;
							//g_object_ref(gst_buf_meta[ch]);
						}
					}
					else{
						if(gst_buf_meta_offset[ch] ==0)
							printf("gst_buf_meta != NULL & gst_buf_meta_offset =0 ~~~~\n");

						//if(gst_buf_meta_offset[ch] + (record_size - sizeof(ICODEC_HEADER)) >=  GST_BUF_META_SIZE){
						if(pheader->timestamp - gst_buf_meta_time[ch] >= GST_BUF_META_TIMESTAMP){
							ICODEC_HEADER	*tmp;
							printf("Meta Data Save CH %d frame_cnt %d frmae size %d \n",ch,gst_buf_meta_cnt[ch],gst_buf_meta_offset[ch]);
							
							tmp = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]);
							tmp->frame_size = gst_buf_meta_offset[ch];
							// overflow . send & save
							meta_data_queue_process(gst_buf_meta[ch], ch, 0);
							//printf("g_object_unref(gst_buf_meta[ch])\n");
							//g_object_unref(gst_buf_meta[ch]);
							//printf("g_object_unref2(gst_buf_meta[ch])\n");
							gst_buf_meta[ch] = gst_nf_buddy_buffer_new_and_alloc( GST_BUF_META_SIZE , NULL);
							gst_buf_meta_offset[ch] = 0;
							if(gst_buf_meta[ch]){
								memset(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), 0x00 , GST_BUF_META_SIZE);
								pheader->flags = nf_meta->man[ch].record_reason;
								pheader->gst_buffer = gst_buf_meta[ch];
								memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]), gobj_buddy_buffer_buf_get_addr(frame) , record_size);
								gst_buf_meta_offset[ch] = record_size;
								gst_buf_meta_cnt[ch] = 1;
								gst_buf_meta_time[ch] = pheader->timestamp;
								//g_object_ref(gst_buf_meta[ch]);
							}
						}
						else{
							// save only
							memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ch]) + gst_buf_meta_offset[ch], gobj_buddy_buffer_buf_get_addr(frame) + sizeof(ICODEC_HEADER), record_size - sizeof(ICODEC_HEADER));
							gst_buf_meta_offset[ch] += record_size - sizeof(ICODEC_HEADER);
							gst_buf_meta_cnt[ch]++;
						}

					}
					
					g_object_unref(que_poped_data);
					
					#endif
				#else
					pheader->flags = nf_meta->man[ch].record_reason;	
					meta_data_queue_process(frame, ch, 1);
					g_object_unref(que_poped_data);
				#endif
				}
				else{
					g_object_unref(que_poped_data);
				}
#else
				g_object_unref(que_poped_data);
#endif
			}
			else{
				// playback
				g_object_unref(que_poped_data);

			}

		}
		
		g_mutex_unlock(nf_meta->lock);

	}
	return NULL;
}

ivca_rule_t rule[NUM_ACTIVE_CH];
ivca_calib_t calib[NUM_ACTIVE_CH];
ivca_option_t opt[NUM_ACTIVE_CH];
#define SYSDB_CAM_DVABX_TRACK_REF "cam.dvabx.opt.R%d.track_ref"

u32 track_ref[NUM_ACTIVE_CH] = {0,}; 
int counter[NUM_ACTIVE_CH][IVCA_MAX_ZONES] = {0,};

int nf_get_counter_value(int chan, int* value)
{
	int k;
	ivca_cntr_t *c;

	for(k=0, c = rule[chan].cntrlist; k< rule[chan].ncntrs; k++, c++)
		*(value+k) = c->value;
	return rule[chan].ncntrs;
}

int set_rule[NUM_ACTIVE_CH]= {0,};
#define MAX_FILTER_STRING 256
char ai_rule_filter[IVCA_MAX_ZONES][MAX_FILTER_STRING];
char enable_nvr_rule_engine[NUM_ACTIVE_CH]={0,};

int nf_ai_set_rule(int chan)
{
	char buff[256];
	int k;
	ivca_cntr_t *c;
	size_t size;
	void *r;
	ai_meta_cnt_t *pCnt_start;
	int counter_cnt=0;
	ai_meta_cnt_t tmp_Cnt;
	int tmp_counter[16] = {0,};
	GValue ret_value = {0,};
	char is_aicam=0;
	
	prvGetAIRuleData(&rule[chan], chan);
	prvGetAICalibData(&calib[chan], chan);
	prvGetAIOptData(&opt[chan], chan);
	
	//memset(counter[chan],0x00,sizeof(int)*IVCA_MAX_ZONES);
	for (k = 0, c = rule[chan].cntrlist; k < rule[chan].ncntrs; k++, c++)
		c->value = 0;

	sprintf(buff, SYSDB_CAM_DVABX_TRACK_REF, chan);
	
	track_ref[chan] = nf_sysdb_get_uint ( buff );

	counter_cnt = rule[chan].ncntrs;
	size = 2 * sizeof(int) + counter_cnt * sizeof(ai_meta_cnt_t);
	r = malloc(size);
	((int *)r)[0] = chan;
	((int *)r)[1] = counter_cnt;
	pCnt_start = (int *)r + 2;
	for (k = 0, c = rule[chan].cntrlist; k < counter_cnt; k++, c++) {
		tmp_Cnt.id = c->id;
		tmp_Cnt.value = c->value;
		memcpy(pCnt_start + k, &tmp_Cnt, sizeof(ai_meta_cnt_t));
	}
	
	nf_notify_fire_pointer("ai_counter", r, (int)size);
	free(r);	
	
	set_rule[chan] = 1;

	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.en_engine", chan, &ret_value, NULL)) {
		enable_nvr_rule_engine[chan] = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.devcam", chan, &ret_value, NULL)) {
		is_aicam = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	if(enable_nvr_rule_engine[chan] && is_aicam /* && (nf_ipcam_get_cam_ai_type(chan) == CAM_AI_TYPE_CLAIR1)*/){
		enable_nvr_rule_engine[chan] = TRUE;
	}
	else{
		enable_nvr_rule_engine[chan] = FALSE;
		memset(&rule[chan],0x00,sizeof(ivca_rule_t));
	}

	printf("nf_ai_set_rule CH %d AI active : %d rule 0 %x \n", chan, enable_nvr_rule_engine[chan], rule[chan].zonelist[0].enabled);
	
}

#define G_CAPTION_SIZE 45
void
save_generic_event_caption(char* caption)
{
	char* rbuf;
	static char g_caption[128][G_CAPTION_SIZE];
	static int caption_cnt = -1;
   	FILE *fp;
	char tmp[128*G_CAPTION_SIZE];
	int k;
	int is_write=0;
	
	if(caption_cnt == -1){
		caption_cnt = 0;
		memset(g_caption, 0x00, sizeof(g_caption));
		// file read
		if((fp=fopen(GENERIC_FILE, "r")) == NULL){
			char tmp1[G_CAPTION_SIZE];
			fp = fopen(GENERIC_FILE, "a+");
			if(fp){
				printf("action %s\n",caption);
				sprintf(tmp1, "%s,",caption);
				fwrite(tmp1, 1, strlen(tmp1), fp);
				fclose(fp);
				strcpy(g_caption[caption_cnt],caption);
				caption_cnt++;
			}
		}
		else{
			int size_f=0;
			int res=0;
			
			fseek(fp, 0, SEEK_END);
			size_f = ftell(fp);
			rewind(fp);

			res = fread(tmp, 1, size_f, fp);
			if(res){
				char *ptr = strtok_r(tmp, ",", &rbuf);  

				while (ptr != NULL){  
				       printf("%s\n", ptr);
					strcpy(g_caption[caption_cnt],ptr);  
					caption_cnt++;    
				       ptr = strtok_r(NULL, ",", &rbuf);
				}
			}
			printf("caption_cnt %d \n",caption_cnt);
		
			fclose(fp);

			for(k=0;k<caption_cnt;k++){
				
			printf("data %s \n",g_caption[k]);
				
			}		

			if(caption_cnt){
				is_write = 1;
				for(k=0;k<caption_cnt;k++){
					if(strcmp(g_caption[k],caption) == 0){
						is_write = 0;
						break;
					}				
				}

			}
			else{
				is_write = 1;
			}
			
			if(is_write){
				fp = fopen(GENERIC_FILE, "a+");
				if(fp){
					char tmp1[G_CAPTION_SIZE];
					sprintf(tmp1, "%s,",caption);
					fwrite(tmp1, 1,  strlen(tmp1), fp);
					fclose(fp);
					strcpy(g_caption[caption_cnt],caption);
					caption_cnt++;
				}
			}
		}

	}
	else{
		// comp data
		is_write = 1;
		for(k=0;k<caption_cnt;k++){
			 //printf("strcmp %s %s\n", g_caption[k],caption);      
			if(strcmp(g_caption[k],caption) == 0){
				is_write = 0;
				break;
			}				
		}

		// skip or save
		if(is_write){
			fp = fopen(GENERIC_FILE, "a+");
			if(fp){
				char tmp1[64];
				sprintf(tmp1, "%s,",caption);
				fwrite(tmp1, 1,  strlen(tmp1), fp);
				fclose(fp);
				strcpy(g_caption[caption_cnt],caption);
				caption_cnt++;
			}
		}
	
	}
}


void
ai_process(VA_EVT_DATA* ai_meta)
{
	ai_meta_hdr_t *pMeta;
	ai_meta_ckh_t *pCKH;
	ai_meta_obj_t *pObj;
	ai_meta_cnt_t *pCnt;
	
	ai_meta_ckh_t *pObj_start_pCKH;

	ai_meta_obj_t *pObj_start;
	ai_meta_cnt_t *pCnt_start;

	int meta_size;
	int k,j;
	int result=0;
	int objcount;
	size_t size;
	void *r;
	char* meta_dec = NULL;
	
	ai_rule_event_t* event_data;
	int e_count=0;
	static int flag=0;
	static int count[NUM_ACTIVE_CH] = {0,};
	int zone_cnt=0;
	int counter_cnt=0;
	char* counter_p = NULL;
	ai_meta_cnt_t tmp_Cnt;
	static int idle_id = 0xff000000;
	int flag_ai = 0;
	ICODEC_HEADER	pheader;
	int oldcval;
	int ecount[IVCA_MAX_ZONES];
	ivca_cntr_t *c;
	ivca_zone_t *z;
	
	GValue ret_value = {0,};
	guint i;
	
	ai_meta_stat.cnt[ai_meta->ch] += ai_meta->meta_count;
	
	//printf("ai_process time %d meta cnt %d \n",ai_meta->timestamp, ai_meta->meta_count);

	//if(rule[ai_meta->ch].nzones){
	if(set_rule[ai_meta->ch]){
		ivcam_vca_set_rules(nf_ai_meta->ai_model[ai_meta->ch] , &(rule[ai_meta->ch]));
		ai_ivcam_vca_set_calib(nf_ai_meta->ai_model[ai_meta->ch], &(calib[ai_meta->ch]));
		ai_ivcam_vca_set_opt(nf_ai_meta->ai_model[ai_meta->ch], &(opt[ai_meta->ch]));

		for(i=0; i <rule[ai_meta->ch].nzones; i++){
			if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.interest_obj", ai_meta->ch, i, &ret_value, NULL))
				strncpy(ai_rule_filter[i],g_value_get_string(&ret_value),MAX_FILTER_STRING);
			g_value_unset(&ret_value);
		}
			
		ai_ivcam_vca_set_rules_filter(nf_ai_meta->ai_model[ai_meta->ch] ,ai_rule_filter);
		//printf("[set_rule] ch %d %s\n",ai_meta->ch,ai_rule_filter[0]);
		set_rule[ai_meta->ch] = 0;
	}
	
	zone_cnt = rule[ai_meta->ch].nzones;
	counter_cnt = rule[ai_meta->ch].ncntrs;

	//printf("zone_cnt %d counter_cnt %d reserved %d \n",zone_cnt, counter_cnt,rule[ai_meta->ch].zonelist[0].reserved2[0]);

	memset(ecount, 0, sizeof(ecount));

	//if(counter_cnt > 0)
		//meta_size = sizeof(ai_meta_hdr_t) + sizeof(ai_meta_ckh_t) + (ai_meta->meta_count*sizeof(ai_meta_obj_t)) + sizeof(ai_meta_ckh_t) + counter_cnt*sizeof(ai_meta_cnt_t);
	//else
		meta_size = sizeof(ai_meta_hdr_t) + sizeof(ai_meta_ckh_t) + (ai_meta->meta_count*sizeof(ai_meta_obj_t));
	meta_dec = malloc(meta_size);

	memset(meta_dec, 0x00, meta_size);
	pMeta = meta_dec;
	pMeta->signature = ('M' << 24) | ('X' << 16) | ('T' << 8) | 'I';
	pMeta->version = 1;
	pMeta->size = meta_size;
	pMeta->ch = ai_meta->ch;
	pMeta->track_ref = track_ref[ai_meta->ch];
	pMeta->n_width = 1920*2;
	pMeta->n_height = 1080*2;
	pMeta->ckcount = 1 + (counter_cnt > 0 ? 1 : 0);
	pMeta->timestamp = ai_meta->timestamp;
	pMeta->timestampl = ai_meta->timestampl;
	pMeta->process_time = ai_meta->process_time;
	memcpy(pMeta->topic , ai_meta->topic , 64);

	pCKH = (void *)(pMeta + 1);
	pCKH->size = ai_meta->meta_count*sizeof(ai_meta_obj_t);
	pCKH->type= AI_META_CKTYPE_OBJ;
	pCKH->priv = ai_meta->meta_count;

	pObj = (void *)(pCKH + 1);
	
	for(k=0; k < ai_meta->meta_count; k++){
		pObj->id = ai_meta->meta_data[k].id;
		if(ai_meta->meta_data[k].id == -1){
			pObj->id =0;
		}
			
		if(pObj->id == 0){
			pObj->id = idle_id++;
			if(idle_id == 0xffffffff)
				idle_id = 0xff000000;
		}
		memcpy(pObj->object_class, ai_meta->meta_data[k].class, 64);
		pObj->nevents = 0;
		pObj->confidence = ai_meta->meta_data[k].confidence;
		pObj->hvalid = 0;
		pObj->isstatic = 0;
		memcpy(pObj->bbx_position , ai_meta->meta_data[k].bbx_position , sizeof(double)*4);

		//printf("ID (%d) x(%lf) y(%lf) class(%s)", pObj->id,pObj->bbx_position[0],pObj->bbx_position[1],pObj->object_class);

		if(k+1 == ai_meta->meta_count)
			break;
		else
			pObj = (void *)(pObj + 1);
	
	}
	
	result = ai_ivcam_vca_process(nf_ai_meta->ai_model[ai_meta->ch] , pMeta, pCKH);
	
	if(result < 0){
		free(meta_dec);
		printf("ai_ivcam_vca_process error @@!!@!@!@@\n");
		
		return TRUE;
	}
	

	if((meta_data_display == META_DATA_DISPLAY_LIVE_ON) && (ai_meta->ch == meta_data_display_ch)){
		ai_obj_t* temp;
		
		objcount = ai_ivcam_vca_get_trackinfo(nf_ai_meta->ai_model[ai_meta->ch] , 200, nf_ai_meta->m_TrackObjs);
		//printf("ai_ivcam_vca_get_trackinfo objcount %d\n",objcount);
		if(objcount < 0){
			free(meta_dec);
			printf("ai_ivcam_vca_get_trackinfo error %d @@!!@!@!@@\n",objcount);
			
			return TRUE;
		}
		if(0){
			ai_obj_t 	temp;
			
			temp = nf_ai_meta->m_TrackObjs[0];
//			printf("ai_ivcam_vca_get_trackinfo ID (%d) x(%lf) y(%lf) class(%s) confidence (%lf) tra x(%lf) tra y(%lf) V(%d)  "
//				, temp.mobj.id,temp.mobj.bbx_position[0],temp.mobj.bbx_position[0],temp.mobj.object_class,temp.mobj.confidence,temp.bbx_traj[0][0],temp.bbx_traj[0][1],temp.mobj.speed3d>>8);

		}
		
		size = 2 * sizeof(int) + (size_t)objcount * sizeof(ai_obj_t);

		r = malloc(size);
		((int *)r)[0] = ai_meta->ch;
		((int *)r)[1] = objcount;
		
		if(enable_nvr_rule_engine[ai_meta->ch] ==0 ){
			ai_obj_t* temp = nf_ai_meta->m_TrackObjs;
			
			for(j=0;j<objcount;j++){
				temp->mobj.nevents = 0;
				temp++;
			}
		}
		
		memcpy((int *)r + 2, (void *)nf_ai_meta->m_TrackObjs, (size_t)objcount* sizeof(ai_obj_t));
		
		nf_notify_fire_pointer("ai_trackinfo", r, (int)size);
		free(r);
	}

	if(get_ai_enable(ai_meta->ch) && enable_nvr_rule_engine[ai_meta->ch]){
	
		result = ai_ivcam_vca_get_events(nf_ai_meta->ai_model[ai_meta->ch],&e_count ,&event_data);

		if(result >0){
			ai_rule_event_t* temp = event_data;
			
			//printf("ai_ivcam_vca_get_events DETECT TYPE %d vca_model_ch %d count %d  rule id %d zonecnt %d !!!!\n",temp->type,ai_meta->ch,e_count,temp->rule_id,zone_cnt);

			for(k=0;k< e_count;k++,temp++){
				for (j = 0, z = rule[ai_meta->ch].zonelist;
							j < zone_cnt; j++, z++) {
					if(z->id == temp->rule_id)
						ecount[j]++;
				}
			}	

			size = 2 * sizeof(int) + e_count*sizeof(ai_rule_event_t);
			r = malloc(size);
			((int *)r)[0] = ai_meta->ch;
			((int *)r)[1] = e_count;
			
			memcpy((int *)r + 2, (void *)event_data, e_count*sizeof(ai_rule_event_t));
			
			nf_notify_fire_pointer("ai_event", r, (int)size);
			free(r);
		}

		/* Check counters. */
		for (k = 0, c = rule[ai_meta->ch].cntrlist; k < counter_cnt; k++, c++) {
			if ( !c->active )
				continue;
			oldcval = c->value;

			/* Count. */
			for (j = 0, z = rule[ai_meta->ch].zonelist; j < zone_cnt; j++, z++) {
				if ( !z->active || !ecount[j] )
					continue;
				if ( z->id == c->zid_up )
					c->value += ecount[j];
				if ( z->id == c->zid_dn )
					c->value -= ecount[j];
			}

			/* Check event. */
			if ( (c->enabled & IVCA_ET_COUNTER) &&
					((oldcval < c->evalue && c->value >= c->evalue) ||
					(oldcval > c->evalue && c->value <= c->evalue))) {
				ai_rule_event_t temp;

				temp.type = IVCA_ET_COUNTER;
				temp.object_id = -1;
				temp.rule_id = c->id;
				memset(temp.object_class,0x00,64);
				memset(temp.topic,0x00,64);
				temp.ch = ai_meta->ch;
				temp.bbx_position[0]=0;
				temp.bbx_position[1]=0;
				temp.bbx_position[2]=0;
				temp.bbx_position[3]=0;
				temp.timestamp = pMeta->timestamp;
				temp.timestampl = pMeta->timestampl;
				temp.process_time = pMeta->process_time;
				temp.confidence = 0;
						
				if ( c->resetalert )
					c->value = 0;		/* Reset counter after alert. */
					
				size = 2 * sizeof(int) + sizeof(ai_rule_event_t);
				r = malloc(size);
				((int *)r)[0] = ai_meta->ch;
				((int *)r)[1] = 1;
				
				memcpy((int *)r + 2, &temp, sizeof(ai_rule_event_t));
				
				nf_notify_fire_pointer("ai_event", r, (int)size);
				free(r);
			}
		}

		if(counter_cnt){
			size = 2 * sizeof(int) + counter_cnt * sizeof(ai_meta_cnt_t);
			r = malloc(size);
			((int *)r)[0] = ai_meta->ch;
			((int *)r)[1] = counter_cnt;
			pCnt_start = (int *)r + 2;
			for (k = 0, c = rule[ai_meta->ch].cntrlist; k < counter_cnt; k++, c++) {
				tmp_Cnt.id = c->id;
				tmp_Cnt.value = c->value;
				memcpy(pCnt_start + k, &tmp_Cnt, sizeof(ai_meta_cnt_t));
			}
			
			nf_notify_fire_pointer("ai_counter", r, (int)size);
			free(r);
		}
	}
	

#ifdef AI_RECORD_ENABLE

	if(gst_buf_meta[ai_meta->ch] == NULL || gst_buf_meta_offset[ai_meta->ch] == 0){
		// init
		gst_buf_meta[ai_meta->ch] = gobj_buddy_buffer_new_malloc( GST_BUF_META_SIZE , NULL);
		gst_buf_meta_offset[ai_meta->ch] = 0;
		if(gst_buf_meta[ai_meta->ch]){
			memset(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]), 0x00 , GST_BUF_META_SIZE);
			pheader.chan = ai_meta->ch;
			pheader.frame_size = meta_size;
			pheader.timestampl = pMeta->timestampl;
			pheader.timestamp = pMeta->timestamp;
			pheader.flags = nf_meta->man[ai_meta->ch].record_reason;
			pheader.gst_buffer = gst_buf_meta[ai_meta->ch];
			
			memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]), &pheader , sizeof(ICODEC_HEADER));
			memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]) + sizeof(ICODEC_HEADER) , pMeta , meta_size);
			gst_buf_meta_offset[ai_meta->ch] = sizeof(ICODEC_HEADER) + meta_size;
			gst_buf_meta_cnt[ai_meta->ch] = 1;
		}
	}
	else{
		if(gst_buf_meta_offset[ai_meta->ch] ==0)
			printf("gst_buf_meta != NULL & gst_buf_meta_offset =0 ~~~~\n");

		if(gst_buf_meta_offset[ai_meta->ch] + meta_size >=  GST_BUF_META_SIZE - META_MAX_SIZE){
			ICODEC_HEADER	*tmp;
			printf("Meta Data Save CH %d frame_cnt %d offset %d size %d \n",ai_meta->ch,gst_buf_meta_cnt[ai_meta->ch], gst_buf_meta_offset[ai_meta->ch],meta_size);

			memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]) + gst_buf_meta_offset[ai_meta->ch], pMeta, meta_size);
			gst_buf_meta_offset[ai_meta->ch] += meta_size;
			gst_buf_meta_cnt[ai_meta->ch]++;

			
			tmp = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]);
			tmp->frame_size = gst_buf_meta_offset[ai_meta->ch];
			// overflow . send & save
			meta_data_queue_process(gst_buf_meta[ai_meta->ch], ai_meta->ch, 0);
			gst_buf_meta_offset[ai_meta->ch] = 0;
		}
		else{
			// save only
			memcpy(gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]) + gst_buf_meta_offset[ai_meta->ch], pMeta, meta_size);
			pMeta = gobj_buddy_buffer_buf_get_addr(gst_buf_meta[ai_meta->ch]) + gst_buf_meta_offset[ai_meta->ch] + g_tmp_metaofs[ai_meta->ch];
			pMeta->rulechange_num = g_tmp_rulechange[ai_meta->ch];
			gst_buf_meta_offset[ai_meta->ch] += meta_size;
			gst_buf_meta_cnt[ai_meta->ch]++;
		}

	}

#endif
		
	free(meta_dec);
	
	return TRUE;

}

void
ai_generic_event_process(VA_GNR_EVT_DATA* g_evt)
{
	size_t size;
	void *r;
	ai_generic_event_t event_data;

	if(get_ai_enable(g_evt->ch) == 0){
		free(g_evt->trigger_zone_list);
		return TRUE;
	}
#if META_OPER_DEBUG	
	printf("generic ch %d time %d arer %f %f cap %s title %s des %s \n",g_evt->ch,g_evt->timestamp,g_evt->event_area[0].y,g_evt->event_area[0].y,g_evt->caption,g_evt->title,g_evt->description);
#endif
		
	size = 2 * sizeof(int) + sizeof(ai_generic_event_t);
	r = malloc(size);
	((int *)r)[0] = g_evt->ch;
	((int *)r)[1] = 1;

	event_data.type = IVCA_ET_GENERIC;
	event_data.ch = g_evt->ch;
	memcpy(event_data.event_area,g_evt->event_area, sizeof(ai_point_t)*2);
	event_data.timestamp = g_evt->timestamp;
	event_data.timestampl = g_evt->timestampl;

	save_generic_event_caption(g_evt->caption);
	
	memcpy(event_data.caption,g_evt->caption, sizeof(event_data.caption));
	memcpy(event_data.title,g_evt->title, sizeof(event_data.title));
	memcpy(event_data.description,g_evt->description, sizeof(event_data.description));

	memcpy(event_data.trigger_type,g_evt->trigger_type, sizeof(event_data.trigger_type));
	memcpy(event_data.trigger_name,g_evt->trigger_name, sizeof(event_data.trigger_name));

	if(g_evt->trigger_zone_count > MAX_ZONE_LIST)
		event_data.trigger_zone_count = 0;
	else
		memcpy(event_data.trigger_zone_list, g_evt->trigger_zone_list , g_evt->trigger_zone_count*(sizeof(ai_point_t)));
	
	free(g_evt->trigger_zone_list);
	
	memcpy((int *)r+2, (void *)&event_data, sizeof(ai_generic_event_t));
	nf_notify_fire_pointer("ai_generic_event", r, (int)size);
	free(r);	
		
	return TRUE;
}



static void*
_nf_ai_meta_thread_func(void *param)
{
	struct sched_param sched;
	gpointer que_poped_data;
	gulong cb_handle;
	guint i, ch, on, start, stop;
	time_t tick;
	struct tm c_tm;
	
       int policy,res=0;

	ICODEC_HEADER	*pheader;
	GobjBuddyBuffer		*frame;

	int record_size;
	
	VA_EVT_DATA* p_obj = NULL;
	VA_GNR_EVT_DATA* g_evt = NULL;

	#if PRI_ADJUST
		sched.sched_priority = sched_get_priority_max(policy)-2;
	#else		
		sched.sched_priority = sched_get_priority_max(policy)-1;
	#endif
	
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched);

	nf_ai_meta->init_done = 1;
	
	for(i=0;i<NUM_ACTIVE_CH + 1;i++){
		nf_ai_meta->ai_model[i] = ai_ivcam_vca_create(i,200,32,NULL);
		if(nf_ai_meta->ai_model[i]  == NULL)
			printf("\n vca_meta == NULL \n");
	}	

#ifdef AI_RECORD_ENABLE
	for(i=0;i<NUM_ACTIVE_CH;i++){
		gst_buf_meta[i] = NULL;
		gst_buf_meta_offset[i] = NULL;
		gst_buf_meta_cnt[i] = 0;
	}
#endif

	memset(nf_ai_meta->m_TrackObjs, 0x00 , sizeof(nf_ai_meta->m_TrackObjs));
	memset(&meta_stat, 0x00 , sizeof(meta_stat));
	
	nf_timer_add( 1000, ai_meta_data_timer, NULL);

	nf_timer_add( 5000, meta_data_stat, NULL);

	for(i=0;i<NUM_ACTIVE_CH;i++)
		nf_ai_set_rule(i);

	printf("\n _nf_ai_meta_thread_func START !!! \n");
			
	while (nf_ai_meta->thread_run) {

		p_obj = nf_ipcam_get_vabox_popped_data();

		if(p_obj != NULL){
			/*
			if(p_obj->ch < NUM_ACTIVE_CH && !(get_ai_enable(p_obj->ch))){
				free(p_obj);
				continue; 
			}
			*/
			meta_ch_mask[p_obj->ch] = 1;
			//printf("_nf_ai_meta_thread_func class %s\n",p_obj->meta_data[0].class);
			ai_process(p_obj);
			free(p_obj);
		}

		g_evt = nf_ipcam_get_vabox_popped_event_data();
		if(g_evt != NULL){
			ai_generic_event_process(g_evt);
			free(g_evt);
		}

		usleep(5000);
		
	}
	return NULL;
}


void nf_meta_data_init()
{
	gint status;
	gulong cb_handle;
	
	g_message("%s START !!!\n.", __FUNCTION__);

	cb_handle= nf_notify_connect_cb( "sysdb_change", _meta_sysdb_reload_cb_func , (gpointer)NULL );
	g_message("%s sysdb_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	nf_meta = calloc(1, sizeof(NF_META));
	g_assert(nf_meta);

	nf_meta->queue = g_async_queue_new();
	g_assert(nf_meta->queue);
	nf_meta->lock = g_mutex_new();
	g_assert(nf_meta->lock);
	
	nf_meta->init_done =0;
	nf_meta->thread_run = 1;

	nf_meta->fxn_get_meta_data = NULL;
	nf_meta->fxn_get_counter = NULL;
	
	status = pthread_create(&nf_meta->thread, NULL, _nf_meta_thread_func, nf_meta);
	g_assert(!status);

	while ( !nf_meta->init_done )
		usleep(10000);

	nf_ai_meta = calloc(1, sizeof(NF_AI_META));
	g_assert(nf_ai_meta);

	nf_ai_meta->queue = g_async_queue_new();
	g_assert(nf_ai_meta->queue);
	nf_ai_meta->lock = g_mutex_new();
	g_assert(nf_ai_meta->lock);
	
	nf_ai_meta->init_done =0;
	nf_ai_meta->thread_run = 1;
	
	status = pthread_create(&nf_meta->thread, NULL, _nf_ai_meta_thread_func, nf_meta);
	g_assert(!status);

	while ( !nf_ai_meta->init_done )
		usleep(10000);

	nf_ai_meta->is_dst = nf_sysdb_get_bool("sys.date.daylight");

	g_message("%s completed.", __FUNCTION__);
	
	return TRUE;

}

//#define IVCA_FRAME_HEADER_SIZE 7
#define IVCA_EVENT_UUID_LEN 16
#define IVCA_EVENT_RTSP_FORMAT_UNIT_SIZE 32
#define IVCA_EVENT_NOTIFY_FORMAT_UNIT_SIZE 36

void nf_event_data_func(gpointer *p_frame)
{
	int i = 0;
	int channel = -1;
	int frame_len = 0;
	int event_total_size = 0;

	unsigned char *data = NULL;
	GobjBuddyBuffer *frame = NULL;
	ICODEC_HEADER *pheader = NULL;
	char *event_start = NULL;

	frame = (GobjBuddyBuffer *)p_frame;
	pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(frame);
	channel = pheader->chan;

	int evt_size = 0;
	size_t rebuild_size = 0;
	ivca_rule_event_t *tmp = NULL;
	void* rebuild_data = NULL;

	char* meta_dec;

	data = (unsigned char*)((char*)(pheader) + sizeof(ICODEC_HEADER));

	int test_len;
	int meta_start = 0;
	test_len = pheader->frame_size;

	if(pheader->frame_type == 8)	//event data
	{
		
		if((data[4] & 0x1f) == 0x06 && data[5] == 0x05)
		{
			// h.264
			int sei_size=0;
			guchar temp;
			
			meta_start = 6; // h.264 sei

			for(i=0; data[i+6] == 0xff;i++){
				sei_size += data[i+6] + 1;
				meta_start++;
			}
			if(data[i+6+1] != 0xfe){
				sei_size += data[i+6] + 1;
				meta_start++;
			}
				
			sei_size += 6; //offset

			if(pheader->frame_size != sei_size){
				printf("\n nfcodec size %d sei_size %d %d %x %x %x %x !!!!\n", pheader->frame_size, sei_size, i, data[6], data[7], data[8], data[9]);
			return -1;
			}

		}
		else if( (((data[4] >> 1) & 0x3f) == 39 || ((data[4] >> 1) & 0x3f) == 40) && data[5] == 1 && data[6] == 5)
		{
			// h.265
			int sei_size=0;
			guchar temp;
			
			meta_start = 7; // h.265 sei

			for(i=0; data[i+7] == 0xff;i++){
				sei_size += data[i+7] + 1;
				meta_start++;
			}
			if(data[i+7+1] != 0xfe){
				sei_size += data[i+7] + 1;
				meta_start++;
			}
				
			sei_size += 7; //offset

			if(pheader->frame_size != sei_size){
				printf("\n nfcodec size %d sei_size %d %d %x %x %x %x !!!!\n", pheader->frame_size, sei_size, i, data[6], data[7], data[8], data[9]);
				return -1;
			}

		}
		else{
			printf("ERROR !!! SEI ???? \n");
			return -1;

		}

		meta_dec = malloc(pheader->frame_size -(meta_start + IVCA_EVENT_UUID_LEN));

		enc_b64_decode_str(meta_dec, pheader->frame_size -(meta_start + IVCA_EVENT_UUID_LEN), data  + meta_start + IVCA_EVENT_UUID_LEN );
		
		evt_size = (int)(*meta_dec);

#if 0
		for(i = 0; i < test_len; i ++)
		{
			printf("\e[33m %02x \e[0m", *(data + i));
		}
		printf("\n");
		printf("\e[95m [%s][%d] frame size : %d evt_size: %d \e[0m\n", __FUNCTION__, __LINE__, pheader->frame_size - IVCA_EVENT_UUID_LEN - IVCA_FRAME_HEADER_SIZE - 4, evt_size);
#endif
	/*
		if((pheader->frame_size - IVCA_EVENT_UUID_LEN - IVCA_FRAME_HEADER_SIZE - 4) != evt_size)
		{
			printf("[%s][%d] SEI slice event length err.. \n", __FUNCTION__, __LINE__);
			return;
		}
		*/

		if(evt_size % sizeof(ivca_rule_event_t) != 0)
		{
			printf("[%s][%d] SEI slice event length err.. \n", __FUNCTION__, __LINE__);
			free(meta_dec);
			return;
		}
		
		if(evt_size > 0)
		{
			event_start = meta_dec + 4;//4 is evt_size feild byte size
			rebuild_size = 2 * sizeof(int) + evt_size;

			tmp = event_start;

			for(i = 0; i < evt_size/sizeof(ivca_rule_event_t); i++)
			{
				if(tmp->rule_id < 0 || tmp->rule_id > 15)
				{
					printf("never be here: rule_id(%d) is error\n", tmp->rule_id);
					break;
				}

				tmp->ch = channel;
				tmp = tmp + 1;
			}

			rebuild_data = malloc(rebuild_size);
			((int *)rebuild_data)[0] = channel;
			((int *)rebuild_data)[1] = evt_size/(sizeof(ivca_rule_event_t));
			memcpy((int *)rebuild_data + 2, (void *)event_start, evt_size);


			//debug
			//printf("\e[33m VCA_EVENT_DATA_FORMAT CH(%d) \e[0m\n", ((int *)rebuild_data)[0]);
			//printf("\e[33m VCA_EVENT_DATA_FORMAT event cnt(%d) \e[0m\n", ((int *)rebuild_data)[1]);
			//printf("\e[33m VCA_EVENT_DATA_FORMAT event type (%02x) \e[0m\n",((int *)rebuild_data)[2]);
			//printf("\e[33m VCA_EVENT_DATA_FORMAT obj ID (%02x) \e[0m\n", ((int *)rebuild_data)[3]);
			//printf("\e[33m VCA_EVENT_DATA_FORMAT rule ID  (%d) \e[0m\n", ((short *)rebuild_data)[8]);


			nf_notify_fire_pointer("vca_event", rebuild_data, (int)rebuild_size);
			free(rebuild_data);
		}
		
		free(meta_dec);
	}
}

static void
_meta_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	int ch, ret;
	g_return_if_fail(pinfo != NULL);
	
	g_message("%s param[%d]",__FUNCTION__, pinfo->d.params[0]);
	
	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS){	// DST is_off
		nf_ai_meta->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );
	}
}
