#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <semaphore.h>

#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_debug.h"
#include "nf_codec_header.h" 
#include "nf_api_live.h"
#include "nf_dspcomm_app.h" 

#include "jbshell.h" 

//#define DEBUG_DSP_RECCMD
//#define DEBUG_DSP_STREAM_TEST
int check_dsp_id(int dsp_id)
{
	if( dsp_id<0 && dsp_id>2)
	{
		printf("invalid dsp_id[%d]\n", dsp_id);
		return -1;
	}
	else
		return 1;	
}


void print_header(ICODEC_HEADER *pheader )
{
	printf("APP chan[%d]\n"		,pheader->chan);
	printf("APP codec[%d]\n"	,pheader->codec);
	printf("APP flags[%d]\n"	,pheader->flags);
	printf("APP version[%d]\n"	,pheader->version);
	printf("APP frame_size[%d]\n",pheader->frame_size);			/*Framesizeexcludeheadersize*/
	printf("APP frame_type[%d]\n",pheader->frame_type);
	printf("APP timestampl[%d]\n",pheader->timestampl);			/*5msecsubtickforuTimeStamp*/
	printf("APP resolution[%d]\n",pheader->resolution);
	printf("APP frame_rate[%d]\n",pheader->frame_rate);
	printf("APP timestamp[%d]\n",pheader->timestamp);	
}


#ifdef DEBUG_DSP_STREAM_TEST

pthread_t threads[64];
int	 ch_stat[16][64];

enum
{
	IDX_TOT = 0,
	IDX_CRC_ERROR = 1,
	IDX_CALL_CNT = 2,
	IDX_THREAD_RUN =3
};

void *thread_main(void *arg)
{
	int i, fd=0, rtn, clen;
	unsigned char buf[256*1024];
	unsigned int crc;
	unsigned int ch_num = (int)arg;
	
	ICODEC_HEADER *pheader = (ICODEC_HEADER *)buf;
	unsigned char *pdata = (buf+sizeof(ICODEC_HEADER));        
	        
	pthread_detach(pthread_self());

	printf("thread_main: %d, %d\n", (int)arg, pthread_self());
		
	if(ch_stat[ch_num][IDX_THREAD_RUN])
	{
		printf("already running %d\n", ch_num);	
		goto exit_thread;		
	}
	

	fd = nf_dspcomm_open_chan(ch_num, NF_DSPCOMM_STREAM);
	if(fd <0)
	{
		printf("dspcomm fd open failed [%d]\n",fd );	
		goto exit_thread;
	}			
	ch_stat[ch_num][IDX_THREAD_RUN]	= 1;
	
						
	while( ch_stat[ch_num][IDX_THREAD_RUN] )
	{        	
		memset(buf, 0x00, sizeof(ICODEC_HEADER));        	        	
		rtn = Readn(fd, pheader, NF_DSPCOMM_READ_SIZE );
	
		ch_stat[ch_num][IDX_TOT] += rtn;
	
#if 0
		printf("APP frame_size[%6d] [%2d][%1d][%02x]\n",
			pheader->frame_size, pheader->chan, pheader->codec, pheader->flags );
#endif
#if 0
		printf("APP rtn[%d]\n"		,rtn);			
		print_header(pheader);
#endif 

		//printf("APP crc[%x][%x]\n",pheader->crc1,pheader->crc1);	
		if( pheader->chan > 16 || pheader->codec != 1)
		{
			printf("error\n");
			print_header(pheader);

			goto exit_thread;				
		}

		clen = pheader->frame_size + sizeof(ICODEC_HEADER);
		clen = NF_DSPCOMM_ALIGN_SIZE( clen , NF_DSPCOMM_READ_SIZE ) - NF_DSPCOMM_READ_SIZE;
											
		if( clen > 0 )
		{
			rtn = Readn( fd, pdata, clen);	
			//printf("APP Readn Frame rtn[%d]\n", rtn);
			ch_stat[ch_num][IDX_TOT] += rtn;

#if 0 // CRC
			crc = crc32( 0xffffffff, pdata, pheader->frame_size);
			
			if( crc != pheader->crc1)
			{
				printf("APP crc error[%x][%x] [%x]\n",pheader->crc1,pheader->crc2, crc);
				++ch_stat[ch_num][IDX_CRC_ERROR];
				
				print_header(pheader);
				//++crc_error_cnt;
				//sleep(2);
			#if 1
				printf("0x%08x ",0);
				for(i=0;i< ((rtn >128) ?128 :rtn) ;i++)
				{
					printf("%02x", pdata[i]);
					if((i+1)%8 == 0) 
						printf(" ");
					
					if((i+1)%16 == 0) 
						printf("\n0x%08x ", i+1);
				}
				printf("\n");					
			#endif					
			}				
#endif
		}

#if 0
		printf("[%02d] APP tot----------------[%d]rokbytes  [%d]\n",
			ch_num,
			ch_stat[ch_num][IDX_TOT]/1024,
			ch_stat[ch_num][IDX_CRC_ERROR]);
		//sleep(1);
#endif			
			
		++ch_stat[ch_num][IDX_CALL_CNT];
		sleep(0);
#if 0
		if (ch_stat[ch_num][IDX_CALL_CNT] > 0x8000)
			break;
#endif				
	}
        
exit_thread:		

		if(fd>0)
			close(fd);
			
		printf("thread %d, %d terminated....\n", (int)arg, pthread_self());			
		ch_stat[ch_num][IDX_THREAD_RUN] = 0;
		pthread_exit((void *) 0);
}

static char stream_get_help[] = "stream_get [dsp_id]";
static int stream_get(int argc, char **argv){
	
	int dsp_id;

	if(argc < 2){
		printf("%s\n",stream_get_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;

	pthread_create(&threads[dsp_id], NULL, &thread_main, (void *)dsp_id);
				
	return 0;
}
__commandlist(stream_get,"stream_get","stream_get command",stream_get_help);

static char stream_stop_help[] = "stream_stop [dsp_id]";
static int stream_stop(int argc, char **argv){
	
	int dsp_id;

	if(argc < 2){
		printf("%s\n",stream_stop_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;
				
	ch_stat[dsp_id][IDX_THREAD_RUN]	= 0;				
	
	return 0;
}
__commandlist(stream_stop,"stream_stop","stream_stop command",stream_stop_help);

#endif


static char dsp_sync_time_help[] = "dsp_sync_time";
static int dsp_sync_time(int argc, char **argv){
	
	int dsp_id;
	
	nf_dspcomm_sync_time();
					
	return 0;
}
__commandlist(dsp_sync_time,"dsp_sync_time","dsp sync time",dsp_sync_time_help);


#ifdef DEBUG_DSP_RECCMD

static int record_info_fps = NF_FPS_CR16;
static int record_info_res = NF_RES_NTSC_CIF;

static char rec_set_help[] = "rec_set [res:1,2,4] [fps:1,2,4,8,16,32]";
static int rec_set(int argc, char **argv){

	int fps, res = 0;			
	if(argc < 3){
		printf("%s\n",rec_set_help);
		return -1;
	} 
		
	res = atoi(argv[1]);
	fps = atoi(argv[2]);

	switch (res)
	{
		case 1:
		case 2:
		case 4:
			record_info_res = res;
			break;
		default:
			break;
	}
		
	switch (fps)
	{
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
			record_info_fps = fps;
			break;
		default:
			break;
	}
	printf("res[%d] fps[%d]\n",record_info_res, record_info_fps);
					
	return 0;
}
__commandlist(rec_set,"rec_set",rec_set_help, rec_set_help);



static char rec_start_help[] = "rec_start [dsp_id]";
static int rec_start(int argc, char **argv){
	
	int dsp_id,i=0;
	DRREQ_RECORD_START	req;
	
	if(argc < 2){
		printf("%s\n",rec_start_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;

	memset(&req, 0x00, sizeof(DRREQ_RECORD_START));
	
	for(i=0;i<NUM_CHANNEL; i++)
	{
		req.record_info[i].channel_id = i;
		req.record_info[i].flags = NF_RECORD_REASON_TIMER<<2;
		req.record_info[i].quality = NF_QUALITY_HIGHEST;
		req.record_info[i].codec = NF_CODEC_TYPE_H264K;
		req.record_info[i].fps = record_info_fps;
		req.record_info[i].res = record_info_res;
	}
		
	nf_dspcomm_send_cmd(dsp_id, NF_DSPCOMM_CH_COMMAND, DP_RECORD_START, &req, sizeof(DRREQ_RECORD_START), 0);
					
	return 0;
}
__commandlist(rec_start,"rec_start",rec_start_help,rec_start_help);

static char rec_stop_help[] = "rec_stop [dsp_id]";
static int rec_stop(int argc, char **argv){
	
	int dsp_id,i=0;
	DRREQ_RECORD_START	req;

	if(argc < 2){
		printf("%s\n",rec_stop_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;

	memset(&req, 0x00, sizeof(DRREQ_RECORD_START));
	
	for(i=0;i<NUM_CHANNEL; i++)
	{
		req.record_info[i].channel_id = i;
		req.record_info[i].flags = NF_RECORD_REASON_NOTHING <<2;
		req.record_info[i].quality = NF_QUALITY_HIGHEST;
		req.record_info[i].codec = NF_CODEC_TYPE_H264K;		
		req.record_info[i].fps = record_info_fps;
		req.record_info[i].res = record_info_res;

	}
	//DP_RECORD_CHANGE_SIGNLE
	nf_dspcomm_send_cmd(dsp_id, NF_DSPCOMM_CH_COMMAND, DP_RECORD_STOP, &req, sizeof(DRREQ_RECORD_START), 0);
				
	return 0;
}
__commandlist(rec_stop,"rec_stop",rec_stop_help,rec_stop_help);

static char rec_change_help[] = "rec_change [dsp_id]";
static int rec_change(int argc, char **argv){
	
	int dsp_id;

	if(argc < 2){
		printf("%s\n",rec_change_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;
				
	return 0;
}
__commandlist(rec_change,"rec_change","rec_change command",rec_change_help);

#endif

#if 0
static char is_alive_help[] = "is_alive [dsp_id]";
static int is_alive(int argc, char **argv){
	
	int dsp_id;

	if(argc < 2){
		printf("%s\n",is_alive_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;
				
	return 0;
}
__commandlist(is_alive,"is_alive","is_alive command",is_alive_help);
#endif

static int _live_change( gint mode, guint ch_mask, gint de_int)
{
	int i,j;
	
	DPREQ_LIVE_CHANGE	req;

	// init
	memset(&req, 0x00, sizeof(DPREQ_LIVE_CHANGE));
	memset(req.screen_num, 0xff, sizeof(req.screen_num));
	req.dein_mode = 0;	
	req.num_display_ch = mode;
	
	if(mode == 1)
	{
		req.display_mode = DISPLAY_FULL;
		req.dein_mode = de_int;		

		if( DISPLAY_IS_D1 )	// 2008-12-12 오전 1:01:24 choissi
				req.dein_mode = 0;
	}
	else if(mode == 4)
		req.display_mode = DISPLAY_QUAD;
	else if(mode == 6)
		req.display_mode = NF_DISPLAY_HEXA_A;
	else if(mode == 8)
		req.display_mode = NF_DISPLAY_OCTA_A;
	else if(mode == 9)
		req.display_mode = NF_DISPLAY_NONA;
	else if(mode == 16)
		req.display_mode = DISPLAY_HEXADECA;
	else			
	{
		mode = 16;
		req.display_mode = DISPLAY_HEXATRICONTA;
		req.num_display_ch = 36;
	}	
	
	for(i=0,j=0;i<16 && j<mode;i++)
	{		
		if( ch_mask & 0x01<<(i) )
			req.screen_num[i] = j++;
	}
	
	for(i=0; i<16;i++)
	{
		if(i%4==0)
			printf(" ");

		printf("[%02x]", req.screen_num[i]);
	}		
	printf("\n");

	for(i=0; i<NUM_ACTIVE_DSP; ++i)
	{
		nf_dspcomm_send_cmd(i, NF_DSPCOMM_CH_COMMAND, DP_LIVE_CHANGE, &req, sizeof(DPREQ_LIVE_CHANGE), 0);
	}
	
	return 0;		
}

static char live_seq_help[] = "live_seq [cnt]";
static int live_seq(int argc, char **argv){
	

	int cnt = 0;
	guint ch_mask = 0xffffffff;
	int mode_arr[] = {1,4,8,16,36};
			
	if(argc < 2){
		printf("%s\n",live_seq_help);
		return -1;
	} 

	cnt = atoi( argv[1] );				
	
	while(--cnt)
	{		
		_live_change(mode_arr[cnt%5], ch_mask, 
						(mode_arr[cnt%5] == 1) ? 1 : 0 );
		sleep(1);
	}
		
	return 0;
}
__commandlist(live_seq,"live_seq","live_seq command",live_seq_help);


static char live_change_help[] = "live_change [mode]";
static int live_change(int argc, char **argv){
	
	int mode;
	int de_int = 0;
	guint ch_mask = 0xffffffff;

		
	if(argc < 2){
		printf("%s\n",live_change_help);
		return -1;
	} 
	
	mode = atoi( argv[1] );			

	if(argc >2 ){
		ch_mask = strtol(argv[2],NULL,16); //atoi(argv[2]);
	} 

	if(argc >3 ){
		de_int = atoi(argv[3]);
		if(de_int != 1)
			de_int = 0;
	} 
	
	_live_change(mode, ch_mask, de_int);
						
	return 0;
}
__commandlist(live_change,"live_change","live_change command",live_change_help);


static char live_covert_help[] = "live_covert [ch_mask]";
static int live_covert(int argc, char **argv){
	
	DPREQ_LIVE_COVERT 	req;
	int i;
	
	if(argc < 2){
		printf("%s\n",live_covert_help);
		return -1;
	} 

	memset( &req, 0x00, sizeof(DPREQ_LIVE_COVERT));
	
	req.covert = strtol(argv[1],NULL,32);
												
	printf("mask[0x%08x]\n", req.covert);

	for(i=0; i<NUM_ACTIVE_DSP; ++i)
	{
		nf_dspcomm_send_cmd(i, NF_DSPCOMM_CH_COMMAND, DP_LIVE_COVERT, &req, sizeof(DPREQ_LIVE_COVERT), 0);
	}	
	
	return 0;
}
__commandlist(live_covert,"live_covert","live_covert command",live_covert_help);

static char live_freeze_help[] = "live_freeze [ch_mask]";
static int live_freeze(int argc, char **argv){
	
	DPREQ_LIVE_FREEZE 	req;
	int i;
	
	if(argc < 2){
		printf("%s\n",live_freeze_help);
		return -1;
	} 

	memset( &req, 0x00, sizeof(DPREQ_LIVE_FREEZE));
	
	req.freeze = strtol(argv[1],NULL,16);
												
	printf("mask[0x%08x]\n", req.freeze);

	for(i=0; i<NUM_ACTIVE_DSP; ++i)
	{
		nf_dspcomm_send_cmd(i, NF_DSPCOMM_CH_COMMAND, DP_LIVE_FREEZE, &req, sizeof(DPREQ_LIVE_FREEZE), 0);
	}
	
	
	return 0;
}
__commandlist(live_freeze,"live_freeze","live_freeze command",live_freeze_help);


// 2008-11-24 오후 8:06:28
#if 0
/*
params 의 codec 필드는 icodec_header_t 의 codec 필드에
params 의 reserved 필드는 icodec_header_t 의 timestamp2 필드에 그대로 copy 한다.

DM6446 이 snapshot 을 요청 하면 DM647 이 dpacket header + icodec_header + stream을 
응답으로 3 번 Tx. 채널(DM647 기준)을 통해 전송.
*/
#endif


static void	dump_jpg_data( ICODEC_HEADER	*pheader )
{
	static guint frame_cnt[32] = {0,};
	gchar	fname[256];
	FILE	*fp = NULL;
	gchar	*data = (((char *)pheader)+sizeof(ICODEC_HEADER));
	gint	channel = pheader->chan;
				
	sprintf(fname, "dump_ch%02d_%08d.jpg", channel, frame_cnt[channel]++);
	fp = fopen(fname, "w");
	fwrite( data, pheader->frame_size, 1, fp);
	fclose(fp);			
}

#include "nf_util_mail.h"
static void	mail_jpg_data( ICODEC_HEADER	*pheader, char *email )
{
	static guint frame_cnt[32] = {0,};
	gchar	fname[256];
	gchar	*data = (((char *)pheader)+sizeof(ICODEC_HEADER));
	gint	channel = pheader->chan;				
	gboolean ret = 0;
	
	NF_MAIL_SEND_CONTENT cont;

	memset(&cont, 0x00, sizeof(cont));		

	sprintf(fname, "dump_ch%02d_%08d.jpg", channel, frame_cnt[channel]++);		
	snprintf( cont.from, sizeof(cont.from), "%s", "test@itxsecurity.com");
	snprintf( cont.subject, sizeof(cont.subject), "%s subject", fname);
	cont.to_cnt = 1;	
	snprintf( cont.to[0], sizeof(cont.to[0]), "%s", email);
	snprintf( cont.contents, sizeof(cont.contents), "%s content", fname);
	cont.image_size = pheader->frame_size;
	snprintf( cont.image_name, sizeof(cont.image_name), "%s", fname);		
	cont.image_data = data;	

	nf_mail_send_request( &cont, NULL);
		
}

static char live_snapshot_help[] = "live_snapshot [ch_mask] [email]";
static int live_snapshot(int argc, char **argv){
	
	int i, dsp_id=0, ret;
	unsigned int mask;
	char *ret_data = NULL;
	ICODEC_HEADER *pheader = NULL;
	
	DPREQ_LIVE_SNAPSHOT 	req;
		
	if(argc < 2){
		printf("%s\n",live_snapshot_help);
		return -1;
	} 

	memset( &req, 0x00, sizeof(DPREQ_LIVE_SNAPSHOT));
		
	mask = strtol(argv[1],NULL,16);
			
	printf("mask[0x%08x]\n", mask);
	if(!mask)
		return 0;
		
	
	for(i=0;i<NUM_ANALOG_CHANNEL;i++)
	{	
		if( mask & (1<<i) )
			req.enable[i] = 1;
	}
	
	req.param.codec = 3;	/* ICODEC, H_264, H_264_KEY, JPEG:3, MPEG4 */

	for(i=0; i<NUM_ACTIVE_DSP; ++i)
	{
		nf_dspcomm_send_cmd(i, NF_DSPCOMM_CH_COMMAND, DP_LIVE_SNAPSHOT, &req, sizeof(DPREQ_LIVE_SNAPSHOT), 0);
	}
		
	for(i=0; i<NUM_ANALOG_CHANNEL;i++)
	{	
		if( mask & (1<<i) )
		{			
			dsp_id = (i<8) ? 0:1;										
			ret = nf_dspcomm_recv_cmd_malloc( dsp_id, NF_DSPCOMM_CH_SNAPSHOT, &ret_data, 0);
			if(ret>0)
			{	
				char *email = "choissi@itxsecurity.com";
				
				if( argc>2) email = argv[2];
					
				printf("nf_dspcomm_recv_cmd_malloc ret[%d]\n", ret);
				nf_debug_hexdump( ret_data, (ret>256)?256 : ret );
									
				pheader = (ICODEC_HEADER *)(ret_data+sizeof(DPACKET));
				print_header(pheader);
				
				dump_jpg_data(pheader);
				mail_jpg_data(pheader, email);
				
				g_free( ret_data );
			}
		}
	}
	return 0;
}
__commandlist(live_snapshot,"live_snapshot","live_snapshot command",live_snapshot_help);


static char set_md_help[] = "set_md [dsp_id] [enable:1] [sens:5] [interval:5]";
static int set_md(int argc, char **argv){
	
	int dsp_id,i;
	DPREQ_SET_MD_CFG req;
	int enable=1,sens=5,interval=5;
	unsigned int ch_mask=0xffffffff;
	
	if(argc < 2){
		printf("%s\n",set_md_help);
		return -1;
	} 
		
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;
	
	if(argc >2 ){
		enable = strtol(argv[2],NULL,0); 
	} 
	if(argc >3 ){
		sens = strtol(argv[3],NULL,0); 
	} 
	if(argc >4 ){
		interval = strtol(argv[4],NULL,0); 
	} 
	if(argc >5 ){
		ch_mask = strtol(argv[5],NULL,0); 
	} 

	if(	enable )
		memset(&req, 0xff, sizeof(DPREQ_SET_MD_CFG));
	else		
		memset(&req, 0x00, sizeof(DPREQ_SET_MD_CFG));

	printf("dsp_id[%d] en[%d] sens[%d] inteval[%d] ch_mask[0x%08x]\n", 
				dsp_id, enable, sens, interval, ch_mask);
	
	req.enable = 1;
	req.eval_interval = interval;

	req.sensor_display = 1;
	req.sensor_color = 4;

	for(i=0;i<DSP_MAX_CHAN;i++)
	{
		req.cfg[i].sensitivity = sens;
		req.cfg[i].reserved[0] = 0;
		req.cfg[i].reserved[1] = 0;
		req.cfg[i].reserved[2] = 0;
	}
	
	nf_dspcomm_send_cmd(dsp_id, NF_DSPCOMM_CH_COMMAND, DP_SET_MD_CFG, &req, sizeof(DPREQ_SET_MD_CFG), 0);
					
	return 0;
}
__commandlist(set_md,"set_md","set_md command",set_md_help);

 
/*
#define NF_DSPCOMM_TRACE_ADDR 	(0xE7FFD400)
#define NF_DSPCOMM_TRACE_SIZE 	(0x10240)

#define NF_DSPCOMM_TICK_ADDR 	(0xE7FFFC00)
#define NF_DSPCOMM_TICK_SIZE 	(0x8)

#define NF_DSPCOMM_DSP_ADDR_L2	(0x00A00000)
#define NF_DSPCOMM_DSP_ADDR_DDR	(0xE6000000)
#define NF_DSPCOMM_DSP_ADDR		(NF_DSPCOMM_DSP_ADDR_DDR)

#define NF_DSPCOMM_MON_ADDR_L2	(0x00A1FC00)
#define NF_DSPCOMM_MON_ADDR_DDR	(0xE7FFFC08)	// 2008-12-05 오전 10:47:59  choissinf
#define NF_DSPCOMM_MON_ADDR		(NF_DSPCOMM_MON_ADDR_DDR)

#define NF_DSPCOMM_VERSION_ADDR		(0xE0010080)
#define NF_DSPCOMM_PARAM_ADDR		(0xE0010000)
*/

//int nf_dspcomm_ioctl_read( NF_DSPCOMM_IOCTL_COMM *ioctl_buff );
#define DSP_DUMP_BLOCK_SIZE		256

static char dsp_dump_help[] = "dsp_dump [dsp_id] [addr] [len:1024] [filename]";
static int dsp_dump(int argc, char **argv){
	
	int dsp_id,r,i;
	unsigned int addr,cur_addr, len = DSP_DUMP_BLOCK_SIZE;
	NF_DSPCOMM_IOCTL_COMM ioctl_comm;
	unsigned int block_cnt = 1;
	unsigned int extra_byte = 0;
	char *filename = NULL;
						
	if(argc < 3){
		printf("%s\n",dsp_dump_help);
		
		printf("0 NF_DSPCOMM_PARAM_ADDR		(0xE0010000)\n");		
		printf("1 NF_DSPCOMM_VERSION_ADDR	(0xE0010080)\n");
		printf("2 NF_DSPCOMM_MON_ADDR_L2	(0x00A1FC00)\n");
		printf("3 NF_DSPCOMM_MON_ADDR_DDR	(0xE7FFFC08)\n");
		printf("4 NF_DSPCOMM_DSP_ADDR_L2	(0x00A00000)\n");
		printf("5 NF_DSPCOMM_DSP_ADDR_DDR	(0xE6000000)\n");
		printf("6 NF_DSPCOMM_TRACE_ADDR 	(0xE7FFD400)\n");
		printf("7 NF_DSPCOMM_TICK_ADDR 		(0xE7FFFC00)\n");
		
		return -1;
	} 	
				
	dsp_id = atoi( argv[1] );			
	if( check_dsp_id(dsp_id) <0 )
		return -1;

	addr = strtoul(argv[2],NULL,0);
	
	if(addr == 0)      addr = 0xE0010000;
	else if(addr == 1) addr = 0xE0010080;
	else if(addr == 2) addr = 0x00A1FC00;
	else if(addr == 3) addr = 0xE7FFFC08;
	else if(addr == 4) addr = 0x00A00000;
	else if(addr == 5) addr = 0xE6000000;
	else if(addr == 6) addr = 0xE7FFD400;
	else if(addr == 7) addr = 0xE7FFFC00;
					
	if(argc > 3)
	{
		len = strtoul(argv[3],NULL,0);
		len = (len > 1024*1024) ? (1024*1024) : len;

		block_cnt= len / DSP_DUMP_BLOCK_SIZE;
		extra_byte= len % DSP_DUMP_BLOCK_SIZE;	 
	}	
	
	if(argc > 4) filename = argv[4];
	
	
	cur_addr = addr;
	
	for(i=0;i<block_cnt;++i)
	{	
		printf("dsp_id[%d] addr[0x%08x] len[0x%x]\n", dsp_id, cur_addr, DSP_DUMP_BLOCK_SIZE);	
		memset( &ioctl_comm, 0x00, sizeof(ioctl_comm));		
		ioctl_comm.dsp_id = dsp_id;
		ioctl_comm.addr = cur_addr;
		ioctl_comm.len = DSP_DUMP_BLOCK_SIZE;
		r = nf_dspcomm_ioctl_read( &ioctl_comm );
		if(!r)
		{
			nf_debug_hexdump( ioctl_comm.buff, DSP_DUMP_BLOCK_SIZE );
		}
		cur_addr += DSP_DUMP_BLOCK_SIZE;
	}
	
	if( extra_byte >0)
	{
		printf("dsp_id[%d] addr[0x%08x] len[0x%x]\n", dsp_id, cur_addr, extra_byte);	
		memset( &ioctl_comm, 0x00, sizeof(ioctl_comm));		
		ioctl_comm.dsp_id = dsp_id;
		ioctl_comm.addr = cur_addr;
		ioctl_comm.len = extra_byte;	
		r = nf_dspcomm_ioctl_read( &ioctl_comm );
		if(!r)
		{
			nf_debug_hexdump( ioctl_comm.buff, extra_byte );
		}
	}
			
	return 0;
}
__commandlist(dsp_dump,"dsp_dump","dsp_dump command",dsp_dump_help);
 
#if 0
GTimeVal nf_dspcomm_get_last_sync_time()
GTimeVal nf_dspcomm_get_time( gint dsp_id )
#endif



static char dsp_time_help[] = "dsp_time [dsp_id]";
static int dsp_time(int argc, char **argv){
	
	int dsp_id,r,i;
	GTimeVal arm_time[2][2], dsp_time[2], sync_time;
	guint64 time_diff[2], dsp_diff[2];
	guint64 arm_conv[2], dsp_conv[2];

	sync_time = nf_dspcomm_get_last_sync_time();

	do{	
		g_get_current_time( &arm_time[0][0] );
		dsp_time[0] = nf_dspcomm_get_time(0);
		g_get_current_time( &arm_time[0][1] );
		//dsp_time[1] = nf_dspcomm_get_time(1);		
		time_diff[0] = GTIMEVAL_TO_GUINT64(arm_time[0][1]) - GTIMEVAL_TO_GUINT64(arm_time[0][0]);	

	}while( time_diff[0] > 80000); // 80usec 이내

	do{	
		g_get_current_time( &arm_time[1][0] );
		dsp_time[1] = nf_dspcomm_get_time(1);
		g_get_current_time( &arm_time[1][1] );
		
		time_diff[1] = GTIMEVAL_TO_GUINT64(arm_time[1][1]) - GTIMEVAL_TO_GUINT64(arm_time[1][0]);

	}while( time_diff[1] > 80000); // 80usec 이내
			
	arm_conv[0] = GTIMEVAL_TO_GUINT64( arm_time[0][0] );
	arm_conv[1] = GTIMEVAL_TO_GUINT64( arm_time[1][0] );

	dsp_conv[0] = GTIMEVAL_TO_GUINT64(dsp_time[0]);
	dsp_conv[1] = GTIMEVAL_TO_GUINT64(dsp_time[1]);

	if(dsp_conv[0]>arm_conv[0])
		dsp_diff[0] = dsp_conv[0]-arm_conv[0];
	else
		dsp_diff[0] = arm_conv[0]-dsp_conv[0];

	if(dsp_conv[1]>arm_conv[1])
		dsp_diff[1] = dsp_conv[1]-arm_conv[1];
	else
		dsp_diff[1] = arm_conv[1]-dsp_conv[1];
					
	g_print("\n[0] ARM %d.%06d  DSP %d.%06d   %c %9lld  (%lld)\n", 
				arm_time[0][0].tv_sec, arm_time[0][0].tv_usec,
				dsp_time[0].tv_sec, dsp_time[0].tv_usec, 
				dsp_conv[0]>arm_conv[0] ? 'D':'A',
				dsp_diff[0]/1000, time_diff[0]/1000);

	g_print("[1] ARM %d.%06d  DSP %d.%06d   %c %9lld  (%lld)\n\n", 
				arm_time[0][0].tv_sec, arm_time[1][0].tv_usec,
				dsp_time[1].tv_sec, dsp_time[1].tv_usec, 				
				dsp_conv[1]>arm_conv[1] ? 'D':'A',
				dsp_diff[1]/1000, time_diff[1]/1000);
	return 0;
}
__commandlist(dsp_time,"dsp_time","dsp_time command",dsp_time_help);



static char dsp2arm_time_sync_help[] = "dsp2arm_time_sync [dsp_id]";
static int dsp2arm_time_sync(int argc, char **argv){
	
	int dsp_id,r,i;
	GTimeVal arm_time[2][2], dsp_time[2], sync_time;
	guint64 time_diff[2], dsp_diff[2];
	guint64 arm_conv[2], dsp_conv[2];

	sync_time = nf_dspcomm_get_last_sync_time();

	do{	
		g_get_current_time( &arm_time[0][0] );
		dsp_time[0] = nf_dspcomm_get_time(0);
		g_get_current_time( &arm_time[0][1] );
		//dsp_time[1] = nf_dspcomm_get_time(1);		
		time_diff[0] = GTIMEVAL_TO_GUINT64(arm_time[0][1]) - GTIMEVAL_TO_GUINT64(arm_time[0][0]);	

	}while( time_diff[0] > 80000); // 80usec 이내
		
	arm_conv[0] = GTIMEVAL_TO_GUINT64(arm_time[0][0]);	
	dsp_conv[0] = GTIMEVAL_TO_GUINT64(dsp_time[0]);	

	if(dsp_conv[0]>arm_conv[0])
		dsp_diff[0] = dsp_conv[0]-arm_conv[0];
	else
		dsp_diff[0] = arm_conv[0]-dsp_conv[0];

	if(	dsp_diff[0] > 1000000) // 1ms
	{
		settimeofday( &dsp_time[0], NULL);		
		g_message("settimeofday  %d.%06d",dsp_time[0].tv_sec, dsp_time[0].tv_usec);					
	}
						
	g_print("\n[0] ARM %d.%06d  DSP %d.%06d   %c %9lld  (%lld)\n", 
				arm_time[0][0].tv_sec, arm_time[0][0].tv_usec,
				dsp_time[0].tv_sec, dsp_time[0].tv_usec, 
				dsp_conv[0]>arm_conv[0] ? 'D':'A',
				dsp_diff[0]/1000, time_diff[0]/1000);

	return 0;
}
__commandlist(dsp2arm_time_sync,"dsp2arm_time_sync","dsp2arm_time_sync command",dsp2arm_time_sync_help);

