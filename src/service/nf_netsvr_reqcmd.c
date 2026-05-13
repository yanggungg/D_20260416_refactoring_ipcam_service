#include <sys/ioctl.h>
#include <linux/sockios.h>

#include "nf_common.h"
#include "nf_debug.h"

#include "nf_sysdb.h"
#include "nf_network.h"
#include "nf_codec_header.h"

#include "nf_netsvr.h"
#include "nf_netsvr_drdef.h"

#include "unp.h"
#include "unpthread.h" 
#include "gsocket.h" 
#include "queue.h" 

#include "nf_ptz.h"
#include "nf_api_eventlog.h"
#include "nf_api_play.h"
#include "nf_util_device.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "netsvr"

#define DEBUG_NETSVR_LOG
#define	DEBUG_NETSVR_JBSHELL

#ifdef DEBUG_NETSVR_JBSHELL
	#include "jbshell.h"
#endif

static gchar _DEBUG_NETSVR_cmd[256] = { 0,};

extern guint nf_ptz_protocol_get_attr( gint channel );
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_currenttime(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{

	vpacket_t vp;
	time_t	ltime;
	int cs = info->cs;

	ltime = time(0);

	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(time_t));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	ltime = NF_BSWAP_32(ltime);

	if(!gsock_writen(cs, &ltime, sizeof(time_t), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}

	return NETSVR_RET_OK;
}

/*
typedef struct _DRREQ_START_LIVE_T {
	unsigned int		channel_mask;
	unsigned short		audio_channel_id;
	unsigned char		audio_mute;
	unsigned char		reserved;
} DRREQ_START_LIVE;
*/
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_start_live(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	DRREQ_START_LIVE startlive;
	gint audio_channel = 0;
	int cs = info->cs, ds = info->ds;
	guint covert_status;
	if( req_vp->dlen != sizeof(DRREQ_START_LIVE) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_START_LIVE) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
		
	memcpy( &startlive, req_buff, sizeof(DRREQ_START_LIVE));
	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_START_LIVE ] ){
		g_message("%s channel_mask[0x%08x]", __FUNCTION__, startlive.channel_mask);
		g_message("%s audio_channel_id[0x%04x]", __FUNCTION__, startlive.audio_channel_id);
		g_message("%s audio_mute[0x%02x] reserved[0x%02x]", __FUNCTION__, 
					startlive.audio_mute, startlive.reserved);	
	}
#endif

	if(	startlive.audio_mute == 1 || startlive.audio_channel_id == 0xffff)
	{
		audio_channel = -1;	// mute이면
		info->channel_audio_ch = -1;
	}else{
		audio_channel = NF_BSWAP_16(startlive.audio_channel_id);
		info->channel_audio_ch = audio_channel;
	}
					
	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	info->channel_mask = (NF_BSWAP_32(startlive.channel_mask)&0xFFFFFFFF);		
	info->channel_imask = info->channel_mask;

#if 0	// 2009-02-16 오후 3:41:23 choissinf (RA bug)
	if ( !(info->auth == 0xffff) )
	{
		covert_status = _netsvr_get_convert_flag();

		info->channel_mask ^= covert_status;
		info->channel_imask ^= covert_status;
	}
#endif	
	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_START_LIVE ] )
	{
		g_message("%s channel_mask[0x%x]", __FUNCTION__, info->channel_mask);
		g_message("%s channel_imask[0x%x]", __FUNCTION__, info->channel_imask);
	}
#endif

	info->total_traffic = info->total_fps_traffic = 0;
	
	memset( &info->ch_stat, 0x00, sizeof(info->ch_stat));
	
//	_livemgr_init_fps_data ( info );	
			
	memset( info->last_frame_header, 0x00, sizeof(info->last_frame_header));
	memset( info->last_iframe_header, 0x00, sizeof(info->last_iframe_header));

	info->live_buff_len = 0;

	//Pthread_mutex_lock(&info->ds_mutex); 
	
	if(_send_controlframe(ds, NF_FRAME_TYPE_STARTDATA) < 0) 
	{
		perror("send_controlframe()");
		goto __error_startlive;
	}
	
	g_message("%s uid[%3d] ds[%d] NF_FRAME_TYPE_STARTDATA", 
				__FUNCTION__, info->uniqueid, info->ds);

		
	_client_set_mode( info, CLIENT_MODE_LIVE);
	
		
	// entry DeQueue;
	Pthread_mutex_lock(&gLive_info.vd_mutex); 
	{			
		++gLive_info.client_count;
	}
	Pthread_mutex_unlock(&gLive_info.vd_mutex);
	
	return NETSVR_RET_OK;

__error_startlive:		
	Pthread_mutex_unlock(&info->ds_mutex); 
	return NETSVR_RET_ERR_SOCKET;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_stop_live(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	int cs = info->cs, ds = info->ds;	

	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");	
		return NETSVR_RET_ERR_SOCKET;
	}

	_client_set_mode( info, CLIENT_MODE_LIVE_STOP);
	

#if 0
	Pthread_mutex_lock(&info->ds_mutex); 
	g_message("%s cs[%d]ds[%d] line[%d]",__FUNCTION__, info->cs, info->ds, __LINE__);
	Pthread_mutex_unlock(&info->ds_mutex); 
#endif	
	// entry DeQueue;
	Pthread_mutex_lock(&gLive_info.vd_mutex); 
	{			
		--gLive_info.client_count;
	}
	Pthread_mutex_unlock(&gLive_info.vd_mutex);	

	g_message("%s cs[%d]ds[%d] line[%d]",__FUNCTION__, info->cs, info->ds, __LINE__);
	
	return NETSVR_RET_OK;	
}


//#define DEBUG_SKIP_PASSWD_CHK
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static gint _check_setup_login( char *id, char *passwd )
{
	char *sysdb_id, *sysdb_passwd;
	char buff[256];
	guint usr_count, sysdb_usr_count;
	int length;
	
	if ( id == NULL)
	{
		g_warning("%s id error", __FUNCTION__);
		return 0;
	}
	if ( passwd == NULL)
	{
		g_warning("%s passwd error", __FUNCTION__);
		return 0;
	}

	sysdb_usr_count = nf_sysdb_get_uint("usr.UCNT");
	//g_message("%s id[%s] passwd[%s] sydb usr count[%d]", __FUNCTION__, id, passwd, sysdb_usr_count);
	
	for ( usr_count = 0; usr_count < sysdb_usr_count; ++usr_count)
	{
		snprintf(buff, sizeof(buff), "usr.U%d.name", usr_count);
		sysdb_id = nf_sysdb_get_str_nocopy(buff);	
		length = strlen(sysdb_id);

		if (!strncmp(sysdb_id, id, length))
		{
			snprintf(buff, sizeof(buff), "usr.U%d.pass", usr_count);
			sysdb_passwd= nf_sysdb_get_str_nocopy(buff);	 
			length = strlen(sysdb_id);	
			if (!strncmp(sysdb_passwd, passwd, length))
			{
				return 1;
			}
			else 
			{
				g_warning("%s passwd error", __FUNCTION__);
				return 0;
			}
		}
	}

	return 0;
	
	
}

int _setup_changed_log_put(CLIENT_INFO *info, int idx, char *buff )
{
	char tmp_buff[64*1024];
	int tmp_size = 0;
	int ret;
	int is_changed = 0;
					
	ret = nf_sysdb_sysdb_to_buff( idx, tmp_buff, &tmp_size);	
	if(!ret)
	{
		g_warning("%s cs[%d]ds[%d] nf_sysdb_sysdb_to_buff failed [%d]",
					__FUNCTION__, info->cs, info->ds, ret );		
		
		return -1;
	}	
	
	if( memcmp(buff, tmp_buff, tmp_size) != 0 )
	{
		GTimeVal	curr_time;

		is_changed = 1;

		g_get_current_time( &curr_time);
		
		if(idx == NF_SYSDB_CONVERT_TYPE_NET){
			
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_NET_EMAIL, info->userid);
			
		}else if(idx == NF_SYSDB_CONVERT_TYPE_AUDIO){
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_AUDIO, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_AUDIO_BUZZER, info->userid);
						
		}else if(idx == NF_SYSDB_CONVERT_TYPE_CAM){
			
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_TITLE, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_COLOR, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_PTZ, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_OSD, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_COVERT, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_MOTION_SENSOR, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_CAMERA_AUDIO_MAPPING, info->userid);
			
		}else if(idx == NF_SYSDB_CONVERT_TYPE_USR){
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_USER_ID, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_USER_GRP, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_AUTO_LOGOUT, info->userid);
							
		}else if(idx == NF_SYSDB_CONVERT_TYPE_NET){
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_NET_EMAIL, info->userid);
						
		}else if(idx == NF_SYSDB_CONVERT_TYPE_ALARM){

			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_EVENT_SENSOR, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_EVENT_STORAGE, info->userid);

		}else if(idx == NF_SYSDB_CONVERT_TYPE_ACT){
			
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_ACT_RELAY, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_ACT_EMAIL, info->userid);
			nf_eventlog_put_param( &curr_time, LT_SYSTEM_SETUP_CHANGED, 1, LP2_SYSTEM_SETUP_CHANGED_ACT_BUZZ, info->userid);
			
		}else if(idx == NF_SYSDB_CONVERT_TYPE_REC){
			
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_MODE, info->userid);
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_PANIC_PARAM, info->userid);
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_ALARM_PARAM, info->userid);
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_ALARM_SCHED, info->userid);
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_CONTMOT_PARAM, info->userid);
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_CONTMOT_SCHED, info->userid);
			nf_eventlog_put_param( &curr_time, LT_RECORD_SETUP_CHANGED, 1, LP2_RECORD_SETUP_CHANGED_DUAL_PARAM, info->userid);
		}
	}
	
	return is_changed;
}

int _dr_set_setup(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	
	int cs = info->cs, ret;
	DRREQ_SETUP	setup;
	vpacket_t vp;	
	
	JOB_INFO *pJob = NULL; 
	
	int is_changed = 0;
	int tmp_size = 0;
	unsigned int	idx = 0;	

	if( req_vp->dlen < sizeof(DRREQ_SETUP) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_SETUP) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
	
	memcpy( &setup, req_buff, sizeof(DRREQ_SETUP));

	if(!(info->auth & AUTH_SETUP)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_SETUP);
	                                  
	    goto __authack;
	}

	// 이곳에 DVR에서 셋업 중인지를 체크 
	//DRE_LOCAL_SETUP
	if(nf_notify_get_param0("gui_setup")) 
	{
	    g_warning("%s : local setup RA Setup closed ", __FUNCTION__);
	                                  
	    goto __losetupack;
	}
	
	// client는 요청할 때 idx가 1부터 시작해서 -1 한다.
	idx = NF_BSWAP_32(setup.dbidx) - 1;	
	
	if( idx >= (NF_SYSDB_CONVERT_TYPE_NR) )
	{
		g_warning("%s cs[%d]ds[%d] idx overflow[%d]",
					__FUNCTION__, info->cs, info->ds, idx );
		
		return NETSVR_RET_ERR_PARAM;
	}	
	tmp_size = req_vp->dlen - sizeof(DRREQ_SETUP);
	
#ifndef DEBUG_SKIP_PASSWD_CHK
	if( (ret = _check_setup_login( info->userid, setup.passwd)) != 1 )
	{
		g_warning("%s wrong passwd id[%s] pass[%s] ret[%d]", __FUNCTION__, 
				info->userid, setup.passwd, ret );						
		goto __passwd;
	}		
#endif

#ifdef DEBUG_NETSVR_LOG
	g_message("%s cs[%d]ds[%d] idx[%d](%s)", 
				__FUNCTION__, info->cs, info->ds, idx, _sysdb_convert_cate_str[idx] );
#endif			

	ret = nf_sysdb_convert_value_validate_check(idx, req_buff+sizeof(DRREQ_SETUP), tmp_size);		
	if(!ret)
	{
		g_warning("%s cs[%d]ds[%d] validate_check failed [%d]",
					__FUNCTION__, info->cs, info->ds, ret );		
		return NETSVR_RET_ERR_INTERNAL;
	}

	is_changed = _setup_changed_log_put( info, idx, req_buff+sizeof(DRREQ_SETUP) );
	
	ret = nf_sysdb_buff_to_sysdb( idx, req_buff+sizeof(DRREQ_SETUP), tmp_size);		
	if(!ret)
	{
		g_warning("%s cs[%d]ds[%d] nf_sysdb_buff_to_sysdb failed [%d]",
					__FUNCTION__, info->cs, info->ds, ret );		

		return NETSVR_RET_ERR_INTERNAL;
	}

	if( is_changed == 1)
	{
		nf_notify_fire_params("sysdb_change", idx, 0, 0, 1);
	}

	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");			
		return NETSVR_RET_ERR_SOCKET;
	}

	if( idx == NF_SYSDB_CONVERT_TYPE_REC ) 
	{
		
		pJob = _client_new_job( info, DR_SET_SETUP, NULL, 0);
		if(pJob == NULL)
		{
			g_warning("%s cs[%d]ds[%d] job memory error", 
						__FUNCTION__, info->cs, info->ds );
									
			return NETSVR_RET_ERR_INTERNAL;
		}	
		if( _client_enque_job(pJob ) != 1)
		{
			g_warning("%s cs[%d]ds[%d] job enque_failed", 
						__FUNCTION__, info->cs, info->ds );		
			_client_free_job(pJob, 0);
			return NETSVR_RET_ERR_INTERNAL;		
		}
	}

	if (idx == NF_SYSDB_CONVERT_TYPE_SYS)
	{
		//BF26 "REMOTE LOG ON: SYSTEM SETUP
		_client_eventlog_put(info, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_LOCAL_LOG_OFF_SYSTEM_SETUP, NULL);
	}

	if (idx == NF_SYSDB_CONVERT_TYPE_REC)
	{
		//BF27 "REMOTE LOG ON: RECORD SETUP
		_client_eventlog_put(info, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_LOCAL_LOG_OFF_RECORD_SETUP, NULL);
	}
	
	return NETSVR_RET_OK;
	
__passwd:
	if(!gsock_simpleReply(cs, DRE_PASSWD, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;

__authack:
	if(!gsock_simpleReply(cs, DRE_AUTHORITY, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;
__losetupack:
    if(!gsock_simpleReply(cs, DRE_LOCAL_SETUP, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;	
	
}

/*
gboolean
nf_sysdb_buff_to_sysdb(guint, gchar*);

gboolean
nf_sysdb_sysdb_to_buff(guint , gchar*);

gboolean
nf_sysdb_convert_value_validate_check(guint, gchar*);

typedef struct _DRREQ_SETSETUP_T {
	char				passwd[32];
	unsigned int		dbidx;
} DRREQ_SETUP;
*/
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_setup(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	int cs = info->cs, ret;
	DRREQ_SETUP	setup;
	vpacket_t vp;	

	int tmp_size = 0;
	unsigned int	idx = 0;
	unsigned short	ret_size = 0;
	char			ret_buff[64*1024];

	CLIENT_INFO *client_info = NULL;
		
	if( req_vp->dlen != sizeof(DRREQ_SETUP) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_SETUP) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
		
	memcpy( &setup, req_buff, sizeof(DRREQ_SETUP));

	if(!(info->auth & AUTH_SETUP)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_SETUP);
	                                  
	    goto __authack;
	}

	// 이곳에 DVR에서 셋업 중인지를 체크 
	//DRE_LOCAL_SETUP
	if(nf_notify_get_param0("gui_setup")) 
	{
	    g_warning("%s : local setup RA Setup closed ", __FUNCTION__);
	                                  
	    goto __losetupack;
	}
	
	// client는 요청할 때 idx가 1부터 시작해서 -1 한다.
	idx = NF_BSWAP_32(setup.dbidx) - 1;	
	
	if( idx >= (NF_SYSDB_CONVERT_TYPE_NR) )
	{
		g_warning("%s cs[%d]ds[%d] idx overflow[%d]",
					__FUNCTION__, info->cs, info->ds, idx );
		
		return NETSVR_RET_ERR_PARAM;
	}
	
#ifndef DEBUG_SKIP_PASSWD_CHK
	if( (ret = _check_setup_login( info->userid, setup.passwd)) != 1 )
	{
		err_msg("%s wrong passwd id[%s] pass[%s] ret[%d]", __FUNCTION__, 
				info->userid, setup.passwd, ret );						
		goto __passwd;
	}		
#endif

	// 이곳에 RA 셋업 중인지를 체크 
	if ( !idx )
	{
		client_info = _netsvr_find_client_setup( &gServer_info );
		if ( client_info != NULL )
		{	
			g_warning("%s  Admin RA Setup is Aready in! Your RA Setup closed ", __FUNCTION__); 
		      
			goto __remotesetupack;
		} 	
		info->setup_mode = 1;   
	}
	
#ifdef DEBUG_NETSVR_LOG
	g_message("%s cs[%d]ds[%d] idx[%d](%s)", 
				__FUNCTION__, info->cs, info->ds, idx, _sysdb_convert_cate_str[idx] );
#endif			
	ret = nf_sysdb_sysdb_to_buff( idx, ret_buff, &tmp_size);	
	if(!ret)
	{
		g_warning("%s cs[%d]ds[%d] nf_sysdb_sysdb_to_buff failed [%d]",
					__FUNCTION__, info->cs, info->ds, ret );		

		return NETSVR_RET_ERR_INTERNAL;
	}	
	ret_size = (unsigned short)tmp_size;

#ifdef DEBUG_NETSVR_LOG
	g_message("%s cs[%d]ds[%d] ret_size[%d]", 
				__FUNCTION__, info->cs, info->ds, ret_size );

	if( _DEBUG_NETSVR_cmd[ DR_GET_SETUP ] ){				
		nf_debug_hexdump(ret_buff, ret_size);
	}
#endif
	
//	return NETSVR_RET_ERR_INTERNAL;

	vp.type	= VPT_REPLY;
	vp.code	= VPE_SUCCESS;
	vp.dlen	= htons(ret_size);

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, ret_buff, ret_size, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if (idx == NF_SYSDB_CONVERT_TYPE_SYS)
	{
		//BF21 "REMOTE LOG ON: SYSTEM SETUP
		_client_eventlog_put(info, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_LOCAL_LOG_ON_SYSTEM_SETUP, NULL);
	}

	if (idx == NF_SYSDB_CONVERT_TYPE_REC)
	{
		//BF22 "REMOTE LOG ON: SYSTEM SETUP
		_client_eventlog_put(info, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_LOCAL_LOG_ON_RECORD_SETUP, NULL);
	}
	
	return NETSVR_RET_OK;

__passwd:
	if(!gsock_simpleReply(cs, DRE_PASSWD, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;

__authack:
	if(!gsock_simpleReply(cs, DRE_AUTHORITY, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;

__remotesetupack:
	if(!gsock_simpleReply(cs, DRE_REMOTE_SETUP, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;
	
__losetupack:
	if(!gsock_simpleReply(cs, DRE_LOCAL_SETUP, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;
	
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_release_setup(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	g_message("%s", __FUNCTION__);
	if(!(info->auth & AUTH_SETUP)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_SETUP);
	                                  
		if(!gsock_simpleReply(info->cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
			g_warning("gsock_simpleReplay()");
			return NETSVR_RET_ERR_SOCKET;
	    }
	    return NETSVR_RET_OK;
	}

	info->setup_mode = 0; 

	return NETSVR_RET_ERR_NO_IMPL;	
}



/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_camera_title(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	DR_PACKET_GET_CAMERATITLE	cameratitle;
	vpacket_t vp;
	
	int i;	
	int cs = info->cs;

	memset( &cameratitle, 0x00, sizeof(DR_PACKET_GET_CAMERATITLE));
	
	for(i=0;i<NUM_ANALOG_CHANNEL;i++)
	{
		char buf[128];
		char *cam_title;
		snprintf(buf, sizeof(buf), "cam.C%d.title", i);
		
		cam_title = nf_sysdb_get_str_nocopy(buf);
		if(cam_title == NULL)
		{
			return NETSVR_RET_ERR_INTERNAL;
		}
						
		snprintf( cameratitle.title[i], 13, cam_title);
	}


#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_CAMERA_TITLE ] ){				
		nf_debug_hexdump( &cameratitle, sizeof(DR_PACKET_GET_CAMERATITLE) );
	}
#endif
			
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(DR_PACKET_GET_CAMERATITLE));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}
	if(!gsock_writen(cs, &cameratitle, sizeof(DR_PACKET_GET_CAMERATITLE), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	
	return NETSVR_RET_OK;	
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*
typedef struct _DRRPY_GET_SYS_INFO_T {
	char				model_name[64];
	unsigned int		hw_version;
	unsigned char		video_format;		// NTSC=0, PAL=1
	unsigned char		channel_count;
	unsigned int		channel_mask;
	unsigned short		product_ver;
	unsigned short		protocol_ver;
	unsigned char		audio_count;
	unsigned char		audio_mask;
	
} DRRPY_GET_SYS_INFO;
*/

int _dr_get_sysinfo(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	DRRPY_GET_SYS_INFO sysinfo;
	vpacket_t vp;
	
	int cs = info->cs;
	char *dbgetsysinfo;
	char *model_name;

	memset( &sysinfo, 0x00, sizeof(DRRPY_GET_SYS_INFO));

	model_name = nf_sysdb_get_str_nocopy("sys.info.sysid");
	if(model_name == NULL)
	{
		return NETSVR_RET_ERR_INTERNAL;
	}
	snprintf( sysinfo.model_name, 64, model_name);
	
	sysinfo.hw_version = nf_sysdb_get_uint("sys.info.hwver");
#if 0
	sysinfo.video_format = nf_sysdb_get_bool("video format");
	sysinfo.channel_count = nf_sysdb_get_int("channel_count");
	sysinfo.audio_count = nf_sysdb_get_int("audio_count");
	sysinfo.audio_mask = nf_sysdb_get_int("audio_mask");
	sysinfo.channel_mask = nf_sysdb_get_int("channel_mask");
	sysinfo.product_ver = nf_sysdb_get_int("product_ver");
	sysinfo.protocol_ver = nf_sysdb_get_int("protocol_ver");
#endif	

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_SYSINFO ] ){				
		nf_debug_hexdump( &model_name, sizeof(DRRPY_GET_SYS_INFO));
	}
#endif

	return NETSVR_RET_ERR_NO_IMPL;

	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(DR_PACKET_GET_SYSINFO));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, &sysinfo, sizeof(DRRPY_GET_SYS_INFO), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	return NETSVR_RET_OK;	
}


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_covert_status(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	unsigned char	covertstatus[16];
	vpacket_t vp;
	
	int i;	
	int cs = info->cs;
		
	memset(covertstatus, 0x00, sizeof(covertstatus));

	if( info->auth != 0xffff)	
	{
		unsigned int covert_flag = _netsvr_get_convert_flag();
		
		for(i=0;i<NUM_ANALOG_CHANNEL;i++)
		{
			covertstatus[i] = (covert_flag & (1<<i) ) ? 1:0;
		}
	}
	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_COVERT_STATUS ] ){				
		nf_debug_hexdump( covertstatus, sizeof(covertstatus));
	}
#endif
		
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(covertstatus));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, covertstatus, sizeof(covertstatus), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	return NETSVR_RET_OK;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_ptz_status(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	vpacket_t vp;
	int cs = info->cs;

	gint 	channel = 0; 			// input
	guint	ptz_attr = 0;
	guint	ptz_flag = 0;	// output

	if( req_vp->dlen != sizeof(int) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(int) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}

	channel = *(int *)req_buff;

	if( channel <0 || channel>= NUM_ANALOG_CHANNEL)
		return NETSVR_RET_ERR_PARAM;
		
	ptz_attr = nf_ptz_protocol_get_attr(channel);

	ptz_flag |= (ptz_attr & NF_PTZ_ATTR_TOUR) ? 0x1 : 0;
	ptz_flag |= (ptz_attr & NF_PTZ_ATTR_PRESET) ? 0x2 : 0;
	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_PTZ_STATUS ] ){				
		nf_debug_hexdump( &ptz_flag, sizeof(guint));
	}
#endif
		
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(int));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, &ptz_flag, sizeof(int), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	return NETSVR_RET_OK;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_alarm_status(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	vpacket_t vp;
	int cs = info->cs;
	guint	alarm = 0;	// output

	alarm = nf_notify_get_param0("alarm");
		
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_ALARM_STATUS ] ){				
		nf_debug_hexdump( &alarm, sizeof(alarm));
	}
#endif

	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(alarm));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, &alarm, sizeof(alarm), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	return NETSVR_RET_OK;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_alarm_control(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{	
	int	alarm_control = 0;	// input
	vpacket_t vp;
	int cs = info->cs;
	guint cnt; 
	
	if( req_vp->dlen != sizeof(int) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(int) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}

	alarm_control = *(int *)req_buff;

#ifdef DEBUG_NETSVR_LOG
	g_message("%s alarm_control : %d", __FUNCTION__,alarm_control);	
#endif

	for(cnt = 0; cnt < NUM_ANALOG_CHANNEL; cnt++)
	{
		guint mask = (1<<cnt); 
		mask = (alarm_control & mask)? 1 : 0; 
		if (mask)
		{
			nf_dev_relay_ch_on(cnt);
		}
		else
		{
			nf_dev_relay_off(cnt);
		}
	}
	
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = 0;

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	return NETSVR_RET_OK;
}
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*
typedef struct _DR_PACKET_GET_HDD_SIZE_T
{
	unsigned int 		total_size;
	unsigned int		used_size;
} DR_PACKET_GET_HDD_SIZE;
*/
int _dr_get_hdd_size(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	vpacket_t vp;
	int cs = info->cs;
	DR_PACKET_GET_HDD_SIZE hdd_size;

	memset(&hdd_size, 0x00, sizeof(DR_PACKET_GET_HDD_SIZE));
#if 0
{
	unsigned int used;
	unsigned int total;
	
	sst_fs_get_diskusage( 0, &used, &total );
	hdd_size.total_size = total;
	hdd_size.used_size = used;
	printf("disk id : %d, used : %d (MB), total : %d (MB)\n", disk_id, used, total);
}
#else
	hdd_size.used_size = nf_notify_get_param0("disk_usage");	// mb
	hdd_size.total_size = nf_notify_get_param1("disk_usage");
#endif
	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_HDD_SIZE ] ){				
		nf_debug_hexdump( &hdd_size, sizeof(DR_PACKET_GET_HDD_SIZE));
	}
#endif

	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(DR_PACKET_GET_HDD_SIZE));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, &hdd_size, sizeof(DR_PACKET_GET_HDD_SIZE), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	return NETSVR_RET_OK;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/

/*
typedef struct _DR_PACKET_GET_TIMEZONE_T
{
	unsigned int 		daylight_saving;
	unsigned int 		time_zone;
} DR_PACKET_GET_TIMEZONE; 	
*/
int _dr_get_timezone(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	DR_PACKET_GET_TIMEZONE	timezone;
	vpacket_t vp;
	
	int i;	
	int cs = info->cs;
	gboolean is_dst = 0;
	guint	idx_tz = 0;
	
	memset( &timezone, 0x00, sizeof(DR_PACKET_GET_TIMEZONE));
	
	is_dst = nf_sysdb_get_bool("sys.date.daylight");	
	timezone.daylight_saving = is_dst;
		
	idx_tz = nf_sysdb_get_uint("sys.date.tz_index");
	timezone.time_zone = idx_tz;

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_TIMEZONE ] ){				
		nf_debug_hexdump( &timezone, sizeof(DR_PACKET_GET_TIMEZONE));
	}
#endif
			
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(DR_PACKET_GET_TIMEZONE));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}
	if(!gsock_writen(cs, &timezone, sizeof(DR_PACKET_GET_TIMEZONE), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;
	
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/

/*
typedef struct _DR_PACKET_GET_DATE_TIME_FORMAT_T
{
	unsigned short 		time_format_idx;
	unsigned short 		date_format_idx;
} DR_PACKET_GET_DATE_TIME_FORMAT;
*/
int _dr_get_datetime_format(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	
	DR_PACKET_GET_DATE_TIME_FORMAT	datetimeformat;
	vpacket_t vp;
	
	int i;	
	int cs = info->cs;

	unsigned int timeform=0, dateform=0;

	memset( &datetimeformat, 0x00, sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));
	
	timeform = nf_sysdb_get_uint("sys.date.timeform");
	datetimeformat.time_format_idx = timeform;

	dateform = nf_sysdb_get_uint("sys.date.dateform");
	datetimeformat.date_format_idx = dateform;

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_DATETIME_FORMAT ] ){				
		nf_debug_hexdump( &datetimeformat, sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));
	}
#endif
			
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}
	if(!gsock_writen(cs, &datetimeformat, sizeof(DR_PACKET_GET_DATE_TIME_FORMAT), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	return NETSVR_RET_OK;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int _dr_get_camera_novideo_status(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	vpacket_t vp;
	int cs = info->cs;
	unsigned char 	videostatus[16];
	int i;
	
	guint		vloss;

	vloss = nf_notify_get_param0("vloss");
	
	for(i=0;i<NUM_ANALOG_CHANNEL;i++)
	{	
		videostatus[i] = (vloss & (1<<i)) ? 1:0;
	}	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_CAMERA_NOVIDEO_STATUS ] ){
		nf_debug_hexdump( &videostatus, sizeof(videostatus));
	}
#endif

	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(videostatus));

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, &videostatus, sizeof(videostatus), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}
	
	return NETSVR_RET_OK;
}
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*
typedef struct _DRREQ_PTZ_T {
	unsigned char		channel;
	unsigned char		action;
	unsigned char		action_param1;
	unsigned char		action_param2;
} DRREQ_PTZ;
*/
int _dr_ptz(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	vpacket_t vp;
	int cs = info->cs;
	
	DRREQ_PTZ drreqptz;
	NF_PTZ_CMD ptz_cmd;
	gint ch, cmd, param0 = 0, param1 = 0;
	
	if( req_vp->dlen != sizeof(DRREQ_PTZ) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_PTZ) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}

	//drreqptz = *(DRREQ_PTZ *)req_buff;
	memcpy( &drreqptz, req_buff, sizeof(DRREQ_PTZ) );

	g_return_val_if_fail ( drreqptz.action <= 0x0b , NETSVR_RET_ERR_PARAM);	

	if(!(info->auth & AUTH_PTZ)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_PTZ);
	                                  
		if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
			g_warning("gsock_simpleReplay()");
			return NETSVR_RET_ERR_SOCKET;
	    }
	    return NETSVR_RET_OK;
	}
	
	ch = drreqptz.channel;
	cmd = drreqptz.action;
	param0 = drreqptz.action_param1;
	param1 = drreqptz.action_param2;

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_PTZ ] ){	
		g_message("%s channel[%d] action[%d] param [%d][%d]", __FUNCTION__,ch, cmd, param0, param1);
	}
#endif

	memset( &ptz_cmd, 0x00, sizeof(NF_PTZ_CMD)); 
	
	switch(cmd)
	{
		case DR_PTZ_CMD_STOP: 		cmd = NF_PTZ_CMD_STOP; break;
		case DR_PTZ_CMD_PAN_LEFT:	cmd = NF_PTZ_CMD_PAN_LEFT; break;
		case DR_PTZ_CMD_PAN_RIGHT:	cmd = NF_PTZ_CMD_PAN_RIGHT;	break;
		case DR_PTZ_CMD_TILT_UP:	cmd = NF_PTZ_CMD_TILT_UP; break;
		case DR_PTZ_CMD_TILT_DOWN:	cmd = NF_PTZ_CMD_TILT_DOWN; break;
		case DR_PTZ_CMD_FOCUS_FAR:	cmd = NF_PTZ_CMD_FOCUS_FAR; break;
		case DR_PTZ_CMD_FOCUS_NEAR:	cmd = NF_PTZ_CMD_FOCUS_NEAR; break;
		case DR_PTZ_CMD_ZOOM_WIDE:	cmd = NF_PTZ_CMD_ZOOM_WIDE; break;
		case DR_PTZ_CMD_ZOOM_TELE:	cmd = NF_PTZ_CMD_ZOOM_TELE; break;
		case DR_PTZ_CMD_SET_PRESET:	cmd = NF_PTZ_CMD_SET_PRESET; break;
		case DR_PTZ_CMD_GOTO_PRESET: cmd = NF_PTZ_CMD_GOTO_PRESET; break;
/*
		case DR_PTZ_CMD_SWING:
			cmd = NF_PTZ_CMD_GOTO_PRESET;
			break;
*/
		default : break;
	}
	ptz_cmd.ch = ch;
	ptz_cmd.cmd = cmd;

	if ( cmd != NF_PTZ_CMD_STOP && cmd != NF_PTZ_CMD_SET_PRESET && cmd != NF_PTZ_CMD_GOTO_PRESET ) 
	{
		ptz_cmd.params[0] = (param0 + 1) * 10;
		ptz_cmd.params[1] = (param1 + 1) * 10;
	}
			
	nf_ptz_cmd( &ptz_cmd);

	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = 0;

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	return NETSVR_RET_OK;
}

/*
typedef struct _DRREQ_TIMELINE_T {
	unsigned int	time_beg;
	unsigned int	resolution;
	unsigned short	count;
	unsigned char	max_channel;
	unsigned char	split_channel;
	unsigned int	channel_mask;
} DRREQ_TIMELINE;
*/
#define MAX_TIMELINE_CNT	1024
#define MAX_TIMELINE_DATA	(MAX_TIMELINE_CNT)

int _dr_timeline(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	int cs = info->cs;
	int ret;
	vpacket_t vp;
	guint covert_status;
	
	DRREQ_TIMELINE	timeline;
	JOB_INFO *pJob = NULL; 

	DRRPY_TIMELINE_ELEM	*ret_tl;
	guint tl_size = 0;	
	guint tl_count = 0;	

	if( req_vp->dlen != sizeof(DRREQ_TIMELINE) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_TIMELINE) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
	
	memcpy( &timeline, req_buff, sizeof(DRREQ_TIMELINE) );

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_TIMELINE ] ){	

		time_t ltime;
		struct tm st_buff;
		struct tm *st = &st_buff;
								
		ltime = (timeline.time_beg);
		localtime_r(&ltime, st);
							
		g_message("%s time_beg [%04d/%02d/%02d-%02d:%02d:%02d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec);			
		g_message("%s resolution: %d",__FUNCTION__, timeline.resolution);
		g_message("%s count: %d",__FUNCTION__, timeline.count);
		g_message("%s max_channel: %d",__FUNCTION__, timeline.max_channel);
		g_message("%s split_channel: 0x%x",__FUNCTION__, timeline.split_channel);
		g_message("%s channel_mask: 0x%x",__FUNCTION__, timeline.channel_mask);
	}
#endif

#if 0
	if(!(info->auth & AUTH_SEARCH)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_SEARCH);
	                                  
		if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
			g_warning("gsock_simpleReplay()");
			return NETSVR_RET_ERR_SOCKET;
	    }
	    return NETSVR_RET_OK;
	}
#endif
	
	if( timeline.count >= 1024 )
	{
		g_warning("%s cs[%d]ds[%d] count overflow[%d]",
					__FUNCTION__, info->cs, info->ds, timeline.count);
		return NETSVR_RET_ERR_PARAM;
	}
	
	{
		NF_TIMELINE_PARAM param;
		gchar *elem;	
		gboolean ret = 0;
		guint i = 0;
		
		memset( &param, 0x00, sizeof(param));
				
		param.time_begin.tv_sec = timeline.time_beg;		
		param.resolution = timeline.resolution;
		param.count = timeline.count;
		param.max_channel = timeline.max_channel;
		param.split_channel = timeline.split_channel;		
		param.channel_mask = (guint64)timeline.channel_mask;	
				
		ret = nf_timeline_get( &param, &elem, NULL);
		if(ret == 0){
			return NETSVR_RET_ERR_INTERNAL;
		}

		if( timeline.split_channel)
			tl_count = 	param.count * timeline.max_channel;	
		else
			tl_count = param.count;

		tl_size = tl_count * sizeof(DRRPY_TIMELINE_ELEM);		

#ifdef DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_cmd[ DR_TIMELINE ] ){	
			nf_debug_hexdump( elem,	tl_count);
		}
#endif		
										
		ret_tl = g_malloc0( tl_size );
		if(!ret_tl)
		{
			g_free(elem);
			return NETSVR_RET_ERR_INTERNAL;
		}

		for(i=0;i<tl_count;i++)
			ret_tl[i].RTRes = elem[i] & 0x0f;

		g_free(elem);
	}
			
	vp.type = VPT_REPLY;
    vp.code = VPE_SUCCESS;
    vp.dlen = htons(sizeof(int)+tl_size);

	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		g_free(ret_tl);
		return NETSVR_RET_ERR_SOCKET;
	}

	if(!gsock_writen(cs, &tl_count, sizeof(tl_count), GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		g_free(ret_tl);
		return NETSVR_RET_ERR_SOCKET;
	}							

	if(!gsock_writen(cs, ret_tl, tl_size, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		g_free(ret_tl);
		return NETSVR_RET_ERR_SOCKET;
	}					
	
	g_free(ret_tl);
		
	return NETSVR_RET_OK;
}

int _dr_start_play(CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff)
{
	int cs = info->cs;
	int ret;
	int playid = -1;
	
	vpacket_t vp;
	
	JOB_INFO *pJob = NULL; 
	DRREQ_START_PLAY	play;

	if( req_vp->dlen != sizeof(DRREQ_START_PLAY) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_START_PLAY) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
	
	memcpy( &play, req_buff, sizeof(DRREQ_START_PLAY) );
		
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_START_PLAY ] ){	

		time_t ltime;
		struct tm st_buff;
		struct tm *st = &st_buff;
								
		ltime = (play.start_time);
		localtime_r(&ltime, st);
		
		g_message("%s start_time [%04d/%02d/%02d-%02d:%02d:%02d.%03d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec, play.start_time_sub*5);			


		ltime = (play.end_time);
		localtime_r(&ltime, st);

		g_message("%s end_time [%04d/%02d/%02d-%02d:%02d:%02d.%03d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec, play.end_time_sub*5);	
				
		g_message("%s direction[%d] speed[%d] audio_channel_id[%d] mute[%d]", __FUNCTION__, 
					play.direction, play.speed,
					play.audio_channel_id, play.audio_mute);

		g_message("%s channel_mask[0x%08x]", __FUNCTION__, play.channel_mask);
	
	}
#endif

	// FIXME 권한체크, param check
	if(!(info->auth & AUTH_SEARCH)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_SEARCH);
		                                  
		if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
			g_warning("gsock_simpleReplay()");
			return NETSVR_RET_ERR_SOCKET;
	    }
	    return NETSVR_RET_OK;
	}

	if( !(info->auth == 0xffff) )
		play.channel_mask ^= _netsvr_get_convert_flag();

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_START_PLAY ] )
	{
		g_message("%s channel_mask[0x%x]", __FUNCTION__, play.channel_mask);
	}
#endif
	
	pJob = _client_peek_job(info, DR_START_PLAY);
	if(pJob || _client_find_streamid_by_pclient(info) != -1 )
	{
		g_warning("%s cs[%d]ds[%d] ERR_CMD_INQUE", __FUNCTION__, info->cs, info->ds );
		return NETSVR_RET_ERR_CMD_INQUE;
	}

	playid = _client_set_streamid(info);
	if( playid == -1)
	{
		g_warning("%s cs[%d]ds[%d] ERR_BUSY", __FUNCTION__, info->cs, info->ds );
		return NETSVR_RET_ERR_BUSY;						
	}

	pJob = _client_new_job( info, DR_START_PLAY, &play, sizeof(DRREQ_START_PLAY));
	if(pJob == NULL)
	{
		g_warning("%s cs[%d]ds[%d] job memory error", 
					__FUNCTION__, info->cs, info->ds );
							
		_client_unset_streamid(info, playid);
		return NETSVR_RET_ERR_INTERNAL;
	}
	pJob->params[0] = playid;
	
	if( _client_enque_job(pJob ) != 1)
	{
		g_warning("%s cs[%d]ds[%d] job enque_failed", 
					__FUNCTION__, info->cs, info->ds );

		_client_unset_streamid(info, playid);
		_client_free_job(pJob, 0);
		return NETSVR_RET_ERR_INTERNAL;		
	}

	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");			
		return NETSVR_RET_ERR_SOCKET;
	}

	_client_set_mode( info, CLIENT_MODE_PLAYBACK);

	//BF19 "REMOTE LOG ON SEARCH
	_client_eventlog_put(info, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_LOCAL_LOG_ON_PLAY_BACK, NULL);
			
	return NETSVR_RET_OK;	
}


int _dr_stop_play(CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff)
{	
	int cs = info->cs, ds = info->ds;
	guint covert_status;

	if(!(info->auth == 0xffff))
	{
		if(!(info->auth & AUTH_SEARCH)) 
		{
		    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
				info->auth, AUTH_SEARCH);
		                                  
			if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
				g_warning("gsock_simpleReplay()");
				return NETSVR_RET_ERR_SOCKET;
		    }
		    return NETSVR_RET_OK;
		}
	}
	
	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");			
		return NETSVR_RET_ERR_SOCKET;
	}
	
	_client_set_mode( info, CLIENT_MODE_INIT);

	//BF24 "REMOTE LOG OFF SEARCH
	_client_eventlog_put(info, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_LOCAL_LOG_OFF_PLAY_BACK, NULL);
	
	return NETSVR_RET_OK;
}

/*
typedef struct _DRREQ_START_BACKUP_T {	
	unsigned int		start_time;
	unsigned int		end_time;
	unsigned char		start_time_sub;
	unsigned char		end_time_sub;
	unsigned char		reserved;
	unsigned char		include_audio;
	unsigned int		channel_mask;
} DRREQ_START_BACKUP;
*/
int _dr_start_backup(CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff)
{

	int cs = info->cs;
	int ret;
	vpacket_t vp;
	int playid = -1;
	
	JOB_INFO *pJob = NULL; 
	DRREQ_START_BACKUP	backup;

	if( req_vp->dlen != sizeof(DRREQ_START_BACKUP) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_START_BACKUP) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
	
	memcpy( &backup, req_buff, sizeof(DRREQ_START_BACKUP) );

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_START_BACKUP ] ){	

		time_t ltime;
		struct tm st_buff;
		struct tm *st = &st_buff;
								
		ltime = (backup.start_time);
		localtime_r(&ltime, st);
						
		g_message("%s start_time [%04d/%02d/%02d-%02d:%02d:%02d.%03d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec, backup.start_time_sub*5);			

		ltime = (backup.end_time);
		localtime_r(&ltime, st);

		g_message("%s end_time [%04d/%02d/%02d-%02d:%02d:%02d.%03d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec, backup.end_time_sub*5);	
				
		g_message("%s include_audio[%d]", __FUNCTION__, 
					backup.include_audio );

		g_message("%s channel_mask[0x%08x]", __FUNCTION__, backup.channel_mask);	
	}
#endif

	// FIXME 권한체크, param check

	if(!(info->auth & AUTH_ARCHIVE)) 
	{
	    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
		info->auth, AUTH_ARCHIVE);
                                  
		if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
			g_warning("gsock_simpleReplay()");
			return NETSVR_RET_ERR_SOCKET;
	    }
	    return NETSVR_RET_OK;
	}

	if(!(info->auth == 0xffff))
	{
		backup.channel_mask ^= _netsvr_get_convert_flag();
	}

#ifdef DEBUG_NETSVR_LOG
	if(  _DEBUG_NETSVR_cmd[ DR_START_BACKUP ] )
	{
		g_message("%s channel_mask[0x%x]", __FUNCTION__, backup.channel_mask);
	}
#endif

	pJob = _client_peek_job(info, DR_START_BACKUP);
	if(pJob || _client_find_streamid_by_pclient(info) != -1 )
	{
		g_warning("%s cs[%d]ds[%d] ERR_CMD_INQUE", __FUNCTION__, info->cs, info->ds );
		return NETSVR_RET_ERR_CMD_INQUE;
	}

	playid = _client_set_streamid(info);
	if( playid == -1)
	{
		g_warning("%s cs[%d]ds[%d] ERR_BUSY", __FUNCTION__, info->cs, info->ds );
		return NETSVR_RET_ERR_BUSY;
	}

	pJob = _client_new_job( info, DR_START_BACKUP, &backup, sizeof(DRREQ_START_BACKUP));
	if(pJob == NULL)
	{
		g_warning("%s cs[%d]ds[%d] job memory error", 
					__FUNCTION__, info->cs, info->ds );
							
		_client_unset_streamid(info, playid);
		return NETSVR_RET_ERR_INTERNAL;
	}
	pJob->params[0] = playid;
	
	if( _client_enque_job(pJob ) != 1)
	{
		g_warning("%s cs[%d]ds[%d] job enque_failed", 
					__FUNCTION__, info->cs, info->ds );

		_client_unset_streamid(info, playid);
		_client_free_job(pJob, 0);
		return NETSVR_RET_ERR_INTERNAL;		
	}
		
	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");			
		return NETSVR_RET_ERR_SOCKET;
	}
	
	_client_set_mode( info, CLIENT_MODE_ARCHIVING);

	//BF20 "REMOTE LOG ON ARCHIVE
	_client_eventlog_put(info, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_LOCAL_LOG_ON_ARCHIVING, NULL);
	
	return NETSVR_RET_OK;	
}

int _dr_stop_backup(CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff)
{
	int cs = info->cs, ds = info->ds;	

	if(!(info->auth == 0xffff))
	{
		if(!(info->auth & AUTH_ARCHIVE)) 
		{
		    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
			info->auth, AUTH_ARCHIVE);
	                                  
			if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
				g_warning("gsock_simpleReplay()");
				return NETSVR_RET_ERR_SOCKET;
		    }
		    return NETSVR_RET_OK;
		}
	}
	
	if(!gsock_simpleReply(cs, VPE_SUCCESS, VNET_TIMEOUT)) {
		perror("gsock_simpleReplay()");			
		return NETSVR_RET_ERR_SOCKET;
	}
	
	_client_set_mode( info, CLIENT_MODE_INIT);

	//BF25 "REMOTE LOG OFF ARCHIVE
	_client_eventlog_put(info, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_LOCAL_LOG_OFF_ARCHIVING, NULL);
	
	return NETSVR_RET_OK;
}


#define MAX_LOG_CNT 	128
#define	MAX_LOG_DATA	(sizeof(NF_LOG_DATA_OLD)*MAX_LOG_CNT)
#define SIZE_LOG_BUFF	(sizeof(int)+sizeof(NF_LOG_RESULT_HEADER)+MAX_LOG_DATA)

typedef enum _NF_RA_LOG_TYPE_MASK_E {
	NF_RA_LOG_TYPE_MASK_SYSTEM		= 0x00000001,
	NF_RA_LOG_TYPE_MASK_HDD			= 0x00000002,
	NF_RA_LOG_TYPE_MASK_ALARM		= 0x00000004,
	NF_RA_LOG_TYPE_MASK_MOTION		= 0x00000008,
	NF_RA_LOG_TYPE_MASK_RECORD		= 0x00000010,
	NF_RA_LOG_TYPE_MASK_TIMER		= 0x00000020,
	NF_RA_LOG_TYPE_MASK_MANUAL		= 0x00001000,
	NF_RA_LOG_TYPE_MASK_IP_EVENT 	= 0x00002000
}NF_RA_LOG_TYPE_MASK_E;

/**
	@brief				RA 이벤트 로그를 NF DVR 로그로 변환 
	@ra_type[in]		ra_type	param->type 이벤트 로그 
	@return	guint	%TRUE on success, %FALSE if an error occurred
*/
static guint _eventlog_param_type_bitmask(guint ra_type)
{
	guint type = 0;

	if(ra_type &NF_RA_LOG_TYPE_MASK_SYSTEM)
	{
		type |= LT_MASK_SYSTEM_STARTED | LT_MASK_SYSTEM_SHUTDOWN | 
			LT_MASK_ABNORMAL_SHUTDOWN_DETECTED | LT_MASK_SYSTEM_RECOVERED | 
			LT_MASK_SYSTEM_TIME_CHANGED | LT_MASK_SYSTEM_FW_UPGRADE |
			LT_MASK_SYSTEM_FORMAT | LT_MASK_SYSTEM_CHECKDISK | 
			LT_MASK_LOCAL_LOG_ON | LT_MASK_LOCAL_LOG_OFF |
			LT_MASK_REMOTE_LOG_ON | LT_MASK_REMOTE_LOG_OFF |
			LT_MASK_RECORD_SETUP_CHANGED | LT_MASK_SYSTEM_SETUP_CHANGED |
			LT_MASK_VIDEO_IN | LT_MASK_VIDEO_LOSS | LT_MASK_TAMPER | 
			LT_MASK_SMART_WARNING | LT_MASK_HDD_OVER_TEMP | 
			LT_MASK_HDD_FULL | LT_MASK_HDD_OW | LT_MASK_SYSTEM_EVENT;
	}
	if (ra_type & NF_RA_LOG_TYPE_MASK_ALARM)
	{
		type |= LT_MASK_SENSOR_INPUT;
	}
	if (ra_type & NF_RA_LOG_TYPE_MASK_MOTION)
	{
		type |= LT_MASK_MOTION_DETECTION;
	}
	if (ra_type & NF_RA_LOG_TYPE_MASK_TIMER)
	{
		type |= LT_MASK_RECORD_STARTED | LT_MASK_RECORD_STOPPED;
	}

	return type;
}

int _dr_get_log(CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff)
{
	int cs = info->cs;
	int ret,i;
	vpacket_t vp;
	guint covert_status;
		
	gint	send_size=0, data_size=0;	
	
	char buff[ SIZE_LOG_BUFF ];
	DRREQ_GET_LOG			get_log;
	NF_LOG_PARAM 			param;
	NF_LOG_DATA				elem[MAX_LOG_CNT];

	int	*result_count = (int *)&buff[0];
	NF_LOG_RESULT_HEADER	*header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)];
	NF_LOG_DATA_OLD			*elem_old = (NF_LOG_DATA_OLD *)&buff[sizeof(int)+sizeof(NF_LOG_RESULT_HEADER)];
				
	if( req_vp->dlen != sizeof(DRREQ_GET_LOG) )
	{
		g_warning("%s cs[%d]ds[%d] PROTOCOL size error vp->dlen[%d] sizeof[%d]",
				__FUNCTION__, info->cs, info->ds, 
				req_vp->dlen, sizeof(DRREQ_GET_LOG) );
		
		return NETSVR_RET_ERR_PROTOCOL;
	}
	
	memcpy( &get_log, req_buff, sizeof(DRREQ_GET_LOG) );

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_LOG ] ){	

		time_t ltime;
		struct tm st_buff;
		struct tm *st = &st_buff;
								
		ltime = (get_log.start_time);
		localtime_r(&ltime, st);
		
		g_message("%s mode[%d] direction[%d] max_count[%d]", __FUNCTION__,
					get_log.mode, get_log.direction, get_log.max_count);

		g_message("%s log_id[%u][%u] type_mask[0x%08x] ch_mask[0x%08x]", __FUNCTION__,
					get_log.log_id, get_log.log_id_sub, get_log.type_mask,
					get_log.channel_mask );		
										
		g_message("%s start_time [%04d/%02d/%02d-%02d:%02d:%02d.%03d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec, get_log.start_time_sub*5);

		ltime = (get_log.end_time);
		localtime_r(&ltime, st);
		
		g_message("%s end_time [%04d/%02d/%02d-%02d:%02d:%02d.%03d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec, get_log.end_time_sub*5);
		
	}
#endif

	if(!(info->auth == 0xffff))
	{
		if(!(info->auth & AUTH_SEARCH)) 
		{
		    g_warning("%s : auth[0x%04x] & [0x%04x] failed!!", __FUNCTION__, 
				info->auth, AUTH_SEARCH);
		                                  
			if(!gsock_simpleReply(cs, DRE_AUTHORITY, VNET_TIMEOUT)) {
				g_warning("gsock_simpleReplay()");
				return NETSVR_RET_ERR_SOCKET;
		    }
		    return NETSVR_RET_OK;
		}
	}
	
	if( get_log.max_count >= MAX_LOG_CNT )
	{
		g_warning("%s cs[%d]ds[%d] max_count over[%d]/[%d]",
				__FUNCTION__, info->cs, info->ds, 
				get_log.max_count, MAX_LOG_CNT );
		
		return NETSVR_RET_ERR_PARAM;
	}

	memset(&param, 0x00, sizeof(NF_LOG_PARAM));
				
	param.mode = get_log.mode;
	if(param.mode == NF_LOG_PARAM_MODE_TIME) 
	{			
		param.time_begin.tv_sec = get_log.start_time;
		param.time_begin.tv_usec = get_log.start_time_sub*5000;

		param.time_end.tv_sec = get_log.end_time;
		param.time_end.tv_usec = get_log.end_time_sub*5000;
		
	}else{
		param.log_id = ( (guint64)get_log.log_id << 32ULL ) | get_log.log_id_sub; 
	}
	
	//param.type_mask = get_log.type_mask;
	param.type_mask = _eventlog_param_type_bitmask(get_log.type_mask);

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_LOG ] ){	
		if (!param.type_mask)
		{
			g_warning("%s param.type_mask[0x%08x]",__FUNCTION__,  param.type_mask);
		}
		else
		{
			g_message("%s param.type_mask[0x%08x]",__FUNCTION__,  param.type_mask);
		}
	}
#endif
	
	param.request_count = get_log.max_count;
	
	memset( param.param1_mask, 0xff, sizeof(param.param1_mask) );	
	
	ret = nf_eventlog_get( &param, header, elem, NULL);
	if(ret == 0)
	{
		return NETSVR_RET_ERR_INTERNAL;
	}

	data_size = header->result_count * sizeof(NF_LOG_DATA_OLD);
	send_size = sizeof(int) + sizeof(NF_LOG_RESULT_HEADER) + data_size;

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_GET_LOG ] ){	
		g_message("%s header request[%d] result[%d] data[%d] send[%d]", __FUNCTION__,
					header->request_count, header->result_count,
					data_size, send_size);
	}
#endif

	for(i=0;i<header->result_count;i++)
	{
		nf_eventlog_convert( &elem[i], &elem_old[i]);

#ifdef DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_cmd[ DR_GET_LOG ] & 0x2 ){	
			g_message( "%s [%d] timestamp[%lld] type[%d] p1[%d]p2[%d] text[%16.16s]", 
					__FUNCTION__, i,
					elem[i].timestamp, elem[i].type, 
					elem[i].param1, elem[i].param2, elem[i].text );
		}
#endif
	}		
							
	vp.type = VPT_REPLY;
	vp.code = VPE_SUCCESS;	
	vp.dlen = htons(send_size);
		
	if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
		perror("gsock_writevp()");
		return NETSVR_RET_ERR_SOCKET;
	}

	*result_count = header->result_count;
	
	if(!gsock_writen(cs, buff, send_size, GSOCK_TIMEOUT)) {
		perror("gsock_writen()");
		return NETSVR_RET_ERR_SOCKET;
	}							
	
	return NETSVR_RET_OK;
}

/******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_send_event(JOB_INFO *pJobEntry)
{
	int send_size, ret;	
	char buff[1024];
	
	NF_LOG_DATA	*log_data;
	NF_LOG_DATA_OLD	*old_data;
	
	vpacket_t *vp = (vpacket_t *)buff ;
	int		*pCnt = (int *)&buff[sizeof(vpacket_t)];
	char	*pData = &buff[sizeof(vpacket_t)+sizeof(int)];	
	
	
	send_size = sizeof(NF_LOG_DATA_OLD) + sizeof(int);
	
	vp->type = VPT_REQUEST;
    vp->code = DR_SEND_EVENT;
    vp->dlen = htons( send_size );

	*pCnt = 1;

	log_data = pJobEntry->msg;	
	nf_eventlog_convert( log_data , (NF_LOG_DATA_OLD *)pData);
	
	send_size += sizeof(vpacket_t);
	
	old_data = (NF_LOG_DATA_OLD	*)pData;

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_cmd[ DR_SEND_EVENT ] ){

		g_message("%s timestamp[%lld] type[%d] param1[%d] param2[%d] text[%s]",
				__FUNCTION__, 
				log_data->timestamp, log_data->type, 
				log_data->param1, log_data->param2, 
				log_data->text );

		g_message("%s --> timestamp[%d] type[%d] param1[%d] param2[%d] text[%s]",
				__FUNCTION__, 
				old_data->timestamp, old_data->type, 
				old_data->param1, old_data->param2, 
				old_data->text );
	}	
#endif
				
	return _client_broadcast_msg((char *)&buff, send_size );
}

/******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_keepalive(JOB_INFO *pJobEntry)
{
	vpacket_t vp;

	vp.type = VPT_REQUEST;
    vp.code = DR_KEEPALIVE;
    vp.dlen = 0;

	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );
}

/******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_localsetup_started(JOB_INFO *pJobEntry)
{
	vpacket_t vp;

	vp.type = VPT_REQUEST;
    vp.code = DR_LOCALSETUP_STARTED;
    vp.dlen = 0;

	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );
}

/******************************************************************************/
/*  [S->C]                                                                    */
/* send packet struct : |---vpacket----|----int----|						  */
/* send size          : |<-----vpacket + int ----->|						  */
/* 1. buffer create : (sizeof(vp) + sizeof(int)) 							  */
/* 2. vpacket copy  : vpacket  --> vpacket range in buffer					  */	
/* 3. data init     : 0 (int)  --> int range in buffer						  */
/* 4. send			: buffer and buffer size								  */
/******************************************************************************/
int _dr_change_networkaudio(JOB_INFO *pJobEntry)
{	
	vpacket_t vp;
	char buff[ sizeof(vp) + sizeof(int) ];
	
	int size_send = 0; 

	vp.type = VPT_REQUEST;
    vp.code = DR_GET_TIMEZONE;
    vp.dlen = htons(sizeof(int));

	size_send = sizeof(int) + sizeof(vp);
	
	memcpy(buff, &vp, sizeof(vp));
	*(int *)&buff[sizeof(vp)] = 0;
		
	return _client_broadcast_msg(buff, size_send );

}

/******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_change_cameratitle(JOB_INFO *pJobEntry)
{
		
	vpacket_t vp;

	vp.type = VPT_REQUEST;
    vp.code = DR_CHANGE_CAMERATITLE;
    vp.dlen = 0;
       
	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );

}

/*******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_change_covert(JOB_INFO *pJobEntry)
{
	vpacket_t vp;
	
	vp.type = VPT_REQUEST;
    vp.code = DR_CHANGE_COVERT;
    vp.dlen = 0;

	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );

}

/*******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_change_ptz(JOB_INFO *pJobEntry)
{
	vpacket_t vp;
	
	vp.type = VPT_REQUEST;
    vp.code = DR_CHANGE_PTZ;
    vp.dlen = 0;

	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );
}

/*******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_change_alarm(JOB_INFO *pJobEntry)
{
	vpacket_t vp;
	
	vp.type = VPT_REQUEST;
    vp.code = DR_CHANGE_ALARM;
    vp.dlen = 0;

	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );
}

/***********************************************************************************************/
/*  [S->C]                                                                     				   */
/* send packet struct : |---vpacket----|----DR_PACKET_GET_TIMEZONE----|		   				   */
/* send size          : |<-----vpacket + DR_PACKET_GET_TIMEZONE ----->|		   				   */
/* 1. buffer create : (sizeof(vp) + sizeof(DR_PACKET_GET_TIMEZONE)) 		   				   */
/* 2. vpacket copy  : vpacket  --> vpacket range in buffer					   				   */	
/* 3. data init     : 0 (DR_PACKET_GET_TIMEZONE) --> DR_PACKET_GET_TIMEZONE range in buffer	   */
/* 4. send			: buffer and buffer size								   				   */
/***********************************************************************************************/
int _dr_chnage_timezone(JOB_INFO *pJobEntry)
{
	vpacket_t vp;
	DR_PACKET_GET_TIMEZONE	timezone;

	char buff[ sizeof(DR_PACKET_GET_TIMEZONE) + sizeof(vp) ];
	int size_send = 0; 

	gboolean is_dst = 0;
	guint	idx_tz = 0;
	
	memset( &timezone, 0x00, sizeof(DR_PACKET_GET_TIMEZONE));
	
	is_dst = nf_sysdb_get_bool("sys.date.daylight");	
	timezone.daylight_saving = is_dst;
		
	idx_tz = nf_sysdb_get_uint("sys.date.tz_index");
	timezone.time_zone = idx_tz;

	vp.type = VPT_REQUEST;
    vp.code = DR_CHNAGE_TIMEZONE;
    vp.dlen = htons(sizeof(DR_PACKET_GET_TIMEZONE));

	size_send = sizeof(DR_PACKET_GET_TIMEZONE) + sizeof(vp);

	memcpy(buff, &vp, sizeof(vp) );
	//memset((DR_PACKET_GET_TIMEZONE *)&buff[ sizeof(vp) ] , 0, sizeof(DR_PACKET_GET_TIMEZONE)); 
	memcpy((DR_PACKET_GET_TIMEZONE *)&buff[ sizeof(vp) ] , &timezone , sizeof(DR_PACKET_GET_TIMEZONE)); 
	
	return _client_broadcast_msg(buff, size_send );

}

/*******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_chnage_novideo(JOB_INFO *pJobEntry)
{
	vpacket_t vp;
		
	vp.type = VPT_REQUEST;
    vp.code = DR_CHNAGE_NOVIDEO;
    vp.dlen = 0;

	return _client_broadcast_msg((char *)&vp, sizeof(vpacket_t) );
}

/************************************************************************************************************/
/*  [S->C]                                                                     				   		   		*/
/* send packet struct : |---vpacket----|----DR_PACKET_GET_DATE_TIME_FORMAT----|		   				   		*/
/* send size          : |<-----vpacket + DR_PACKET_GET_DATE_TIME_FORMAT ----->|		   				   		*/
/* 1. buffer create : (sizeof(vp) + sizeof(DR_PACKET_GET_DATE_TIME_FORMAT)) 		   				   		*/
/* 2. vpacket copy  : vpacket  --> vpacket range in buffer					   				   		   		*/	
/* 3. data init     : 0 (DR_PACKET_GET_DATE_TIME_FORMAT) --> DR_PACKET_GET_DATE_TIME_FORMAT range in buffer	*/
/* 4. send			: buffer and buffer size								   				   				*/
/************************************************************************************************************/
int _dr_chnage_datetimeformat(JOB_INFO *pJobEntry)
{
	DR_PACKET_GET_DATE_TIME_FORMAT	datetimeformat;
	vpacket_t vp;

	int size_send = 0; 
	char buff[ sizeof(DR_PACKET_GET_DATE_TIME_FORMAT) + sizeof(vp) ];

	unsigned int timeform=0, dateform=0;

	memset( &datetimeformat, 0x00, sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));
	
	timeform = nf_sysdb_get_uint("sys.date.timeform");
	datetimeformat.time_format_idx = timeform;

	dateform = nf_sysdb_get_uint("sys.date.dateform");
	datetimeformat.date_format_idx = dateform;
				
	vp.type = VPT_REQUEST;
    vp.code = DR_CHNAGE_DATETIMEFORMAT;
    vp.dlen = htons(sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));

	size_send = sizeof(DR_PACKET_GET_DATE_TIME_FORMAT) + sizeof(vp);

	memcpy( buff, &vp , sizeof(vp) );
	//memset((DR_PACKET_GET_DATE_TIME_FORMAT *)&buff[ sizeof(vp) ], 0, sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));
	memcpy((DR_PACKET_GET_DATE_TIME_FORMAT *)&buff[ sizeof(vp) ], &datetimeformat, sizeof(DR_PACKET_GET_DATE_TIME_FORMAT));
	
	return _client_broadcast_msg(buff, size_send );
}

/*******************************************************************************/
/*  [S->C]                                                                    */
/******************************************************************************/
int _dr_disconnect(unsigned short high, unsigned short low)
{
	vpacket_t vp;
	int disconnect = 0;

	char buff [ sizeof(int) + sizeof(vp) ];
	int size_send = 0;

	g_return_val_if_fail ( high >0 && high <= 2, -1);	
	g_return_val_if_fail ( low >0 && low <= NF_DISCONN_SVR_NR, -1);	
	
	disconnect = (high << 16) | low;

	vp.type = VPT_REQUEST;
    vp.code = DR_DISCONNECT;
    vp.dlen = htons(sizeof(int));

	size_send = sizeof(vp) + sizeof(int);

	memcpy(buff, &vp, sizeof(vp));
	
	*(int *)&buff[ sizeof(vp) ] = disconnect;
	
	return _client_broadcast_msg(buff, size_send );
}

/******************************************************************************/
/*     JBSHELL  CMD                                                           */
/******************************************************************************/

#ifdef DEBUG_NETSVR_JBSHELL

static char net_cmd_log_help[] = "net_cmd_log [idx] [val]";
static int net_cmd_log(int argc, char **argv)
{	
	gint idx, val,i;
		
	if(argc < 3){
		printf("%s\n",net_cmd_log_help);		
		for(i=0;i<256;i++)
		{
			char *proto = _str_dr_proto(i);

			if(strcmp(proto, "DR_unknown") !=0)
				g_print("%-32s = %2d, 0x%08x\n", proto, i, _DEBUG_NETSVR_cmd[i]);
		}
		return -1;
	}
		
	idx = strtol(argv[1],NULL,0);
	val = strtol(argv[2],NULL,0);
	
	g_return_val_if_fail( idx <= 256, -1 );
	
	if( idx >= 256 )
	{
		for(i=0;i<256;i++)		
			_DEBUG_NETSVR_cmd[i] = val;
	}else{
		_DEBUG_NETSVR_cmd[idx] = val;
	}
	
	return 0;
}
__commandlist(net_cmd_log,"net_cmd_log", net_cmd_log_help, net_cmd_log_help);


static char net_inform_help[] = "net_inform [idx][param1][param2]";
static int net_inform(int argc, char **argv)
{	
	gint idx, val, i;
	
	if (argc < 2)
	{
		printf("%s\n",net_inform_help);		
		for(i=0;i<DR_INFORM_NR;i++)
		{
			char *proto = _str_dr_proto_inform(i);
			
			if(strcmp(proto, "DR_unknown") !=0)
			{
				g_print("[%03d][%-32s] \n",i, proto);
			}			
		}
		return -1;
	}

	idx = strtol(argv[1],NULL,0);
	g_return_val_if_fail( idx < DR_INFORM_NR, -1 );

	if ((idx == DR_INFORM_DISCONNECT) && (argc < 4))
	{
		printf("%s\n",net_inform_help);		
		for(i=0;i<DR_INFORM_NR;i++)
		{
			char *proto = _str_dr_proto_inform(i);
			
			if(strcmp(proto, "DR_unknown") !=0)
			{
				g_print("[%03d][%-32s]\n",i, proto);
			}			
		}
		return -1;
	}
	
	if (idx == DR_INFORM_DISCONNECT)
	{
		unsigned short high;
		unsigned short low;
		
		high = strtol(argv[2],NULL,0);
		low = strtol(argv[3],NULL,0);

		g_printf("%s high[%x] low[%x]\n",__FUNCTION__, high, low );
		
		_dr_disconnect(high, low);
	}
	else
	{
		switch(idx)
		{
			case DR_INFORM_KEEPALIVE             : _dr_keepalive				(NULL);		break;  
			case DR_INFORM_LOCALSETUP_STARTED    : _dr_localsetup_started		(NULL);     break;			
			case DR_INFORM_CHANGE_NETWORKAUDIO   : _dr_localsetup_started		(NULL);     break;			
			case DR_INFORM_CHANGE_CAMERA_TITLE   : _dr_change_cameratitle		(NULL);     break;			
			case DR_INFORM_CHANGE_COVERT         : _dr_change_covert    		(NULL);     break;			
			case DR_INFORM_CHANGE_PTZ	         : _dr_change_ptz	            (NULL);		break;		
			case DR_INFORM_CHANGE_ALARM          : _dr_change_alarm             (NULL);     break;   			
			case DR_INFORM_CHNAGE_TIMEZONE       : _dr_chnage_timezone          (NULL);     break;   			
			case DR_INFORM_CHNAGE_NOVIDEO        : _dr_chnage_novideo          	(NULL);     break;    			
			case DR_INFORM_CHNAGE_DATETIMEFORMAT : _dr_chnage_datetimeformat  	(NULL);     break;    			
		}
	}

	return 0;
}

__commandlist(net_inform,"net_inform", net_inform_help, net_inform_help);

#endif

