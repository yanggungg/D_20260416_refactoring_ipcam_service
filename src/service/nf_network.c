#include <sys/ioctl.h>
#include <linux/sockios.h>

#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_debug.h"
#include "nf_network.h"
#include "nf_notify.h"

#include "nf_netsvr.h"
#include "nf_netsvr_drdef.h"
#include "nf_api_eventlog.h"

#include "nf_api_live.h"
#include "nf_sysman.h"
#include "nf_issm_ctl.h"
#include "nf_pds_ctl.h"
#include "nf_rtsp_over_ws.h"

#include "nf_util_device.h"
#include "nf_util_netif.h"

#include "unp.h"
#include "unpthread.h" 
#include "gsocket.h" 
#include "queue.h" 

#if defined(_OTM_MODEL) || defined(_SNF_MODEL)
#include "nf_solo_aud.h"
#include "nf_solo_net.h"
#endif /* _OTM_MODEL, _SNF_MODEL */

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "netsvr"

static char *_eth_dev = HOST_ETH_DEV;

#if 0
FILE    *audio_fp;
#endif


static GQuark 
_nf_network_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_NETSVR_ERROR 	_nf_network_error_quark()

#define DEBUG_NETSVR_LOG
#define	DEBUG_NETSVR_JBSHELL
//#define DEBUG_DISABLE_SEND_EVENT
//#define	DEBUG_DISABLE_WEBSVR
//#define	DEBUG_DISABLE_DDNSSVR
#define ENABLE_CMS_TIMEOUT_SKIP

#define WEBRA_FIXME_CHOISSI

//#define DEBUG_NOTIFY_STATUS
#define ENABLE_S1_DUAL			// 2013-05-28 ���� 1:53:05 choissi

#ifdef ENABLE_S1_DUAL
	extern int nf_s1_dual_init();
#endif 


#ifdef DEBUG_NETSVR_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_NETSVR_IDX_E
{ 
	DEBUG_NETSVR_IDX_INIT				= 0,
	DEBUG_NETSVR_IDX_CLIENT_ACCEPT		= 1,
	DEBUG_NETSVR_IDX_CLIENT_LOGIN		= 2,
	DEBUG_NETSVR_IDX_CLIENT_CMD_PROC	= 3,
	
	DEBUG_NETSVR_IDX_CLIENT_CMD_PROC_RET = 4,
	DEBUG_NETSVR_IDX_CLIENT_PACKET		= 5,
	DEBUG_NETSVR_IDX_CLIENT_AUDIO		= 6,	
	DEBUG_NETSVR_IDX_CLIENT_BCAST 		= 7,

	DEBUG_NETSVR_IDX_CLIENT_CMD_LIVE 	= 8,	
	DEBUG_NETSVR_IDX_CLIENT_CMD_PLAY 	= 9,
	DEBUG_NETSVR_IDX_CLIENT_CMD_ARCH	= 10,
	DEBUG_NETSVR_IDX_CLIENT_CMD_TIMELINE = 11,

	DEBUG_NETSVR_IDX_CLIENT_CMD_SYSDB	= 12,
	DEBUG_NETSVR_IDX_CLIENT_CMD_LOG		= 13,
	DEBUG_NETSVR_IDX_CLIENT_CMD_KEEPALIVE = 14,	
	DEBUG_NETSVR_IDX_CLIENT_CMD_GETINFO = 15,

	DEBUG_NETSVR_IDX_CLIENT_JOB_QUE		= 16,
	DEBUG_NETSVR_IDX_PLAY_PROCESS		= 17,
	DEBUG_NETSVR_IDX_PLAY_THREAD		= 18,
	DEBUG_NETSVR_IDX_PLAY_THREAD_SEND	= 19,	
	
	DEBUG_NETSVR_IDX_PLAY_THREAD_SKIP	= 20,			
	DEBUG_NETSVR_IDX_FIND_CLIENT_SETUP	= 21,	
	DEBUG_NETSVR_IDX_CONNECT_API		= 22,	
	DEBUG_NETSVR_IDX_NR					= 23
	
}DEBUG_NETSVR_IDX_E;

static const char *_DEBUG_NETSVR_str[32] =
{
	"NETSVR_IDX_INIT",
	"NETSVR_IDX_CLIENT_ACCEPT",
	"NETSVR_IDX_CLIENT_LOGIN",
	"NETSVR_IDX_CLIENT_CMD_PROC",
	
	"NETSVR_IDX_CLIENT_CMD_PROC_RET",
	"NETSVR_IDX_CLIENT_PACKET",
	"NETSVR_IDX_CLIENT_AUDIO",
	"NETSVR_IDX_CLIENT_BCAST",
   
	"NETSVR_IDX_CLIENT_CMD_LIVE",
	"NETSVR_IDX_CLIENT_CMD_PLAY",
	"NETSVR_IDX_CLIENT_CMD_ARCH",
	"NETSVR_IDX_CLIENT_CMD_TIMELINE",
	
	"NETSVR_IDX_CLIENT_CMD_SYSDB",
	"NETSVR_IDX_CLIENT_CMD_LOG",
	"NETSVR_IDX_CLIENT_CMD_KEEPALIVE",
	"NETSVR_IDX_CLIENT_CMD_GETINFO",	

	"NETSVR_IDX_CLIENT_JOB_QUE",
	"NETSVR_IDX_PLAY_PROCESS",
	"NETSVR_IDX_PLAY_THREAD",
	"NETSVR_IDX_PLAY_THREAD_SEND",
	
	"NETSVR_IDX_PLAY_THREAD_SKIP",	
	"NETSVR_IDX_FIND_CLIENT_SETUP",			
	"NETSVR_IDX_CONNECT_API",
	"NETSVR_IDX_NR"
};

static gint _DEBUG_NETSVR_log[32] = 
{
	1,1,1,1, 1,128,0,0, 0,0,0,0, 0,0,0,0,
	0,1,1,0, 1,0,1,0, 0,0,0,0, 0,0,0,0 
};

int	_netsvr_get_debug_val(int idx)
{
	g_return_val_if_fail( idx < DEBUG_NETSVR_IDX_NR, 0);
	
	return _DEBUG_NETSVR_log[idx];
}

LIVE_INFO 		gLive_info;
LIVE_INFO 		gRtp_live_info;

SERVER_INFO		gServer_info; 
CLIENT_INFO		gClient_info;

static int _netsvr_init_info(void);
static int _netsvr_init_sock(void);
static void _netsvr_close_sock(void);

static int _netsvr_accept_thread_func(void *arg);
static int _netsvr_accept_client(int s, struct sockaddr_in *ptr_addr);
static int _netsvr_login_user(CLIENT_INFO *pClient);
static int _netsvr_send_keycode(int s);
static int _netsvr_verify_keycode(int s);

static int _netsvr_process_rccmd( JOB_INFO *pJobEntry);

static void *_client_thread_func(void *arg);
static int _client_process_rccmd(CLIENT_INFO *info, vpacket_t *vp, char *vp_buff);
static int _client_csds_read(CLIENT_INFO *pClient, int timeout_sec);
static int _client_release(CLIENT_INFO *pClient);

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

static void nf_network_class_init (NfNetworkClass * klass);
static void nf_network_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_network_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_network_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_network_dispose (GObject * object);
static void nf_network_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfNetwork	*_nf_network = NULL;

static void network_thread_func(NfNetwork * test);
static void network_audio_thread_func (NfNetwork * self);

static void _netsvr_covert_flag_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static guint _covert_flag = 0;
static GStaticMutex _covert_mutex = G_STATIC_MUTEX_INIT;

static void _netsvr_vloss_flag_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static guint _vloss_flag = 0;
static GStaticMutex _vloss_mutex = G_STATIC_MUTEX_INIT;

static void network_reset_func (void);

#ifdef WEBRA_FIXME_CHOISSI

static volatile guint is_webra_audio_active = 0;
guint nf_network_get_webra_audio_status(void)
{
	return is_webra_audio_active;
}

void nf_network_set_webra_audio_status(guint val)
{
	is_webra_audio_active = val;
}

#endif

/**
 *
 * Description : notify net_status
 *               if parameter is 0, there will be no change.
 *                              -3, the value will be plused by one.
 *                              -2, the value will be cleared.
 *                              -1, the value will be minused.
 *                             n>0, the value will be set.
 * Param :
 *      conn - http connection status (-1, 0, 1)
 *      live - streaming connection status (-1, 0, n)
 *      play - straming playback connection status (-1, 0, n)
 *
 * by chcha
 * */
int nf_network_notify_net_status(int conn, int live, int play)
{
    int change = 0;

    NF_NOTIFY_INFO *pnotify = NULL;
    guint cur_conn = 0;
    guint cur_live = 0;
    guint cur_play = 0;
    guint stream = 0;

    pnotify = nf_notify_get("net_status");	
    if(!pnotify) 
    {
        g_warning("can't get net_status notify");
    }
    else 
    {
        cur_conn = pnotify->d.params[0];
        cur_live = pnotify->d.params[1];
        cur_play = pnotify->d.params[2];

        nf_notify_free(pnotify);
    }

    if( live ) {
        switch( live ) {
            case -3:
                cur_live += (guint)live;
                break;
            case -2:
                cur_live = 0;
                break;
            case -1:
                cur_live = (cur_live > 0 ) ? cur_live -1 : 0;
                break;
            default:
                cur_live = live;
                break;
        }

        change |= 2;
    }

    if( play ) {
        switch( play ) {
            case -3:
                cur_play += (guint)play;
                break;
            case -2:
                cur_play = 0;
                break;
            case -1:
                cur_play = (cur_play > 0 ) ? cur_play -1 : 0;
                break;
            default:
                cur_play = play;
                break;
        }

        change |= 4;
    }

    stream = cur_live + cur_play;

    if( stream > 0 ) {
        cur_conn = stream;
    } else {
        if( conn > 0 ) {
            cur_conn = (guint)conn;

            change |= 1;
        } else {
            cur_conn = 0;
        }
    }

#ifdef DEBUG_NOTIFY_STATUS
    g_message("%s [%d]notify[%d:%u][%d:%u][%d:%u]", 
            __func__, 
            change, 
            conn, cur_conn, live, cur_live, play, cur_play);
#endif

    nf_notify_fire_params("net_status", cur_conn, cur_live, cur_play, 0);

    return change;
}

GType
nf_network_get_type (void)
{
	static GType nf_network_type = 0;

	if (G_UNLIKELY (nf_network_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfNetworkClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_network_class_init,
			NULL,
			NULL,
			sizeof (NfNetwork),
			0,
			(GInstanceInitFunc) nf_network_instance_init,
			NULL
		};

		nf_network_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfNetwork", &object_info, 0);
	}
	
	return nf_network_type;
}

static void
nf_network_class_init (NfNetworkClass * klass)
{	
	GObjectClass *gobject_class;
	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_network_set_property;
	gobject_class->get_property = nf_network_get_property;
			
	gobject_class->dispose = nf_network_dispose;
	gobject_class->finalize = nf_network_finalize;

}

static void
nf_network_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfNetwork *self = NF_NETWORK (instance);
				
	self->init_done	= 0;
	
	// queue ����
	self->queue = g_async_queue_new();
 		 
	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)network_thread_func, 
									self, FALSE, NULL);

	// audio queue ����
	self->audio_queue = g_async_queue_new();
 		 
	// notification signal emit�� thread ����
	self->audio_thread_run = 1;
	self->audio_thread = g_thread_create(	(GThreadFunc)network_audio_thread_func, 
									self, FALSE, NULL);

#ifdef _IPX_0824VE
	//g_thread_create(network_reset_func, NULL, FALSE, NULL);
#endif
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_network_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_network_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_network_set_property (GObject * object, guint prop_id,
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
nf_network_get_property (GObject * object, guint prop_id,
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


static void
network_audio_thread_func (NfNetwork * self)
{
	int idle_cnt = 0;
	guint pre_dac_ch = 0;
	
	gpointer	que_poped_data = NULL;
	
	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_network == NULL ) g_usleep(1000*10);
	
	self->init_done = 1;
	
	while(self->audio_thread_run)
	{
		que_poped_data  = g_async_queue_try_pop( self->audio_queue );
		if(que_poped_data){
			JOB_INFO 	*pJobEntry = (JOB_INFO 	*)que_poped_data;
			guint dvr_status = nf_notify_get_param0("dvr_status");
			
#if 0
			g_message("%s JOB reqTime[%ld.%06ld]", __FUNCTION__,
						pJobEntry->req_timestamp.tv_sec,
						pJobEntry->req_timestamp.tv_usec);
#endif
			idle_cnt = 0;
			
			if( gLive_info.liveaudio_fd > 0 &&
				dvr_status != NF_DVR_STATUS_RUN_PLAYBACK )
			{

#ifdef WEBRA_FIXME_CHOISSI
				nf_network_set_webra_audio_status(1);
#endif

#if defined(USE_DEV_AUDIO)
				nf_dev_audio_set_netrx( 1 );	// netrx
#endif
				
#if defined(USE_DEV_TW2864)
				nf_dev_tw2864_set_dac( NF_TW2864_DAC_PLAYBACK );
#endif
#if defined(_OTM_MODEL) || defined(_SNF_MODEL)
				write( gLive_info.liveaudio_fd,
					   pJobEntry->msg,
					   pJobEntry->msg_len );
#else
				Write(gLive_info.liveaudio_fd, 
						pJobEntry->msg, 
						pJobEntry->msg_len);
#endif /* _OTM_MODEL */
			}

			_client_free_job(pJobEntry, 0);
						
		}else{
			++idle_cnt;
		}
		g_usleep(33000); 

#ifdef WEBRA_FIXME_CHOISSI
		if(idle_cnt == 7)
		{
			nf_network_set_webra_audio_status(0);
		}
#endif
				
		if(idle_cnt == 8)
		{
			guint dvr_status = nf_notify_get_param0("dvr_status");			
			if(  dvr_status != NF_DVR_STATUS_RUN_PLAYBACK ) {
				//nf_live_apply_live_audio();
			}

#if defined(USE_DEV_AUDIO)
			nf_dev_audio_set_netrx( 0 );
#endif

		}
	}
}

//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	
static void
network_thread_func (NfNetwork * self)
{
	GTimeVal 	que_pop_timeout;
	gpointer	que_poped_data = NULL;
	
	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_network == NULL ) g_usleep(1000*10);
	
	self->init_done = 1;
	
	while(self->thread_run)
	{
		que_poped_data  = g_async_queue_pop( self->queue);
		if(que_poped_data)
		{
			JOB_INFO *pJobEntry = que_poped_data;
			
#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_CLIENT_JOB_QUE] )
				g_message("%s JOB req[0x%08x] ack[0x%08x] reqTime[%ld.%06ld]", __FUNCTION__,
						pJobEntry->req_msg_id,
						pJobEntry->ack_msg_id,
						pJobEntry->req_timestamp.tv_sec,
						pJobEntry->req_timestamp.tv_usec);
#endif						
			_netsvr_process_rccmd( pJobEntry );

			_client_free_job(pJobEntry, 1); // deque, free
		}						
	}
	g_message("%s end", __FUNCTION__);
}

/*
typedef struct _JOB_INFO
{
	TAILQ_ENTRY(_JOB_INFO)	entries;
		
	struct _CLIENT_INFO		*pclient;
			
	// for reqest msg	
	unsigned int 			req_msg_id;
	struct timeval 			req_timestamp;	
	unsigned int			ack_msg_id;
	struct timeval 			ack_timestamp;

	void					*msg;
	unsigned int			msg_len;

	unsigned int			params[4];	
} JOB_INFO;
*/
static gboolean _notify_fire()
{
	int index = 0;
	
	g_message("%s index[%d]", __FUNCTION__, index);

#if 0	
	nf_notify_fire_params("sysdb_tformat", 0, 0, 0, 0);	
	nf_notify_fire_params("sysdb_tzone", 0, 0, 0, 0);			
	nf_notify_fire_params("sysdb_cam_title", 0, 0, 0, 0);	
	nf_notify_fire_params("sysdb_covert", 0, 0, 0, 0);
	nf_notify_fire_params("sysdb_ptz", 0, 0, 0, 0);	
#endif
			
	for ( index = 0; index < NF_SYSDB_CATE_NR; index++ )
	{
		nf_notify_fire_params("sysdb_change", index, 0, 0, 0);	
	}
	
	return 1;
	
}

int _process_set_setup(JOB_INFO *pJobEntry)
{
	g_message("%s called!", __FUNCTION__ );
		
	//_notify_fire();
	nf_sysdb_save_all();
	
	return 1;	
}

static int 
_netsvr_process_rccmd( JOB_INFO *pJobEntry )
{
	
	switch(pJobEntry->req_msg_id)
	{		
		case	DR_LOCALSETUP_STARTED:		_dr_localsetup_started(pJobEntry); break;

		case	DR_CHANGE_NETWORKAUDIO: 	_dr_change_networkaudio(pJobEntry); break;
		case	DR_CHANGE_COVERT: 			_dr_change_covert(pJobEntry); break;
		case	DR_CHANGE_CAMERATITLE: 		_dr_change_cameratitle(pJobEntry); break;
		case	DR_CHANGE_PTZ:				_dr_change_ptz(pJobEntry); break;
		case	DR_CHANGE_ALARM: 			_dr_change_alarm(pJobEntry); break;
		case	DR_CHNAGE_TIMEZONE: 		_dr_chnage_timezone(pJobEntry); break;
		case	DR_CHNAGE_NOVIDEO: 			_dr_chnage_novideo(pJobEntry); break;
		case	DR_CHNAGE_DATETIMEFORMAT: 	_dr_chnage_datetimeformat(pJobEntry); break;
						
		case	DR_SEND_EVENT:				_dr_send_event(pJobEntry); break;

		//case	DR_START_PLAY:				_process_play(pJobEntry); break;
		//case	DR_START_BACKUP:			_process_backup(pJobEntry); break;
		
		// background sysdb save	
		case	DR_SET_SETUP:				_process_set_setup(pJobEntry); break;
		
		default:
		{
			g_warning("%s JOB no_handler req[0x%08x] ack[0x%08x] reqTime[%ld.%06ld]", __FUNCTION__,
						pJobEntry->req_msg_id,
						pJobEntry->ack_msg_id,
						pJobEntry->req_timestamp.tv_sec,
						pJobEntry->req_timestamp.tv_usec);			
		}						
	}		
		
	return 1;	
}

static void
_netsvr_log_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
  //memcpy(send_log, pinfo->p.ptr, sizeof(NF_LOG_DATA));    

#ifdef DEBUG_NETSVR_LOGx
	if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_CLIENT_BCAST ] )
	{
		g_message("%s idx[0x%02x]", __FUNCTION__, (int)data);
	}
#endif

	g_return_if_fail( pinfo->type == NF_NOTIFY_POINTER) ;
				
	_client_enque_job_simple( NULL, (int)data, pinfo->p.ptr, sizeof(NF_LOG_DATA) );  
}

static void
_netsvr_sysdb_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
#ifdef DEBUG_NETSVR_LOGx
	if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_CLIENT_BCAST ] )
	{
		g_message("%s idx[0x%02x]", __FUNCTION__, (int)data);
	}
#endif	
	_client_enque_job_simple( NULL, (int)data, NULL, 0);
}

static void
_netsvr_notify_cb_init(void)
{
	gulong cb_handle = 0;	
	
	cb_handle= nf_notify_connect_cb( "sysdb_cam_title", _netsvr_sysdb_cb_func,	
										(gpointer)DR_CHANGE_CAMERATITLE);
	g_message("%s sysdb_cam_title connect_cb[%ld]",__FUNCTION__, cb_handle );

	cb_handle= nf_notify_connect_cb( "sysdb_covert", _netsvr_sysdb_cb_func,	
										(gpointer)DR_CHANGE_COVERT);
	g_message("%s sysdb_covert connect_cb[%ld]",__FUNCTION__, cb_handle );

	cb_handle= nf_notify_connect_cb( "sysdb_ptz", _netsvr_sysdb_cb_func, 
										(gpointer)DR_CHANGE_PTZ);
	g_message("%s sysdb_ptz connect_cb[%ld]",__FUNCTION__, cb_handle );

	cb_handle= nf_notify_connect_cb( "sysdb_tzone", _netsvr_sysdb_cb_func, 
										(gpointer)DR_CHNAGE_TIMEZONE);
	g_message("%s sysdb_tzone connect_cb[%ld]",__FUNCTION__, cb_handle );

	cb_handle= nf_notify_connect_cb( "sysdb_tformat", _netsvr_sysdb_cb_func,
										(gpointer)DR_CHNAGE_DATETIMEFORMAT);
	g_message("%s sysdb_tformat connect_cb[%ld]",__FUNCTION__, cb_handle );

	cb_handle= nf_notify_connect_cb( "alarm", _netsvr_sysdb_cb_func, 
										(gpointer)DR_CHANGE_ALARM);
	g_message("%s alarm connect_cb[%ld]",__FUNCTION__, cb_handle );

	cb_handle= nf_notify_connect_cb( "vloss", _netsvr_sysdb_cb_func,
										(gpointer)DR_CHNAGE_NOVIDEO);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );

#ifndef DEBUG_DISABLE_SEND_EVENT
	cb_handle= nf_notify_connect_cb( "log", _netsvr_log_cb_func, 
										(gpointer)DR_SEND_EVENT);
	g_message("%s log connect_cb[%ld]",__FUNCTION__, cb_handle );
#else
	g_message("%s DEBUG_DISABLE_SEND_EVENT !!!!!!!!!",__FUNCTION__ );
	g_message("%s DEBUG_DISABLE_SEND_EVENT !!!!!!!!!",__FUNCTION__ );	
#endif

	cb_handle= nf_notify_connect_cb( "gui_setup", _netsvr_sysdb_cb_func, 
										(gpointer)DR_LOCALSETUP_STARTED );
	g_message("%s gui_setup connect_cb[%ld]",__FUNCTION__, cb_handle );

#if 0	// choissi 2009-11-11 ���� 9:14:54 change sysdb_covert notify
	cb_handle= nf_notify_connect_cb( "sysdb_covert", _netsvr_covert_flag_cb_func, NULL);
	g_message("%s sysdb_covert connect_cb[%ld]",__FUNCTION__, cb_handle );
#endif

}


/**
	@brief				network �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_network_init( int wait )
{
	gboolean ret = TRUE;
	gint r = 0;

#if 0
  audio_fp = fopen("audio_temp.txt", "wb+");
#endif

	g_return_val_if_fail (_nf_network == NULL, FALSE);	
	
	_nf_network = g_object_new ( NF_TYPE_NETWORK , NULL);

	nf_debug_category_add( "netsvr", _DEBUG_NETSVR_str, _DEBUG_NETSVR_log, DEBUG_NETSVR_IDX_NR);

	_eth_dev = nf_netif_get_eth_str();

	_netsvr_init_info();	
//	_livemgr_init();
							
	_netsvr_covert_flag_cb_func(NULL, NULL);
	_netsvr_notify_cb_init();

	nf_notify_fire_params("net_status", 0, 0, 0, 0);
	nf_notify_fire_params("net_rxtx", 0, 0, 0, 0);

#ifndef	DEBUG_DISABLE_DDNSSVR
	create_ddns();
#else
	g_message("%s DEBUG_DISABLE_DDNSSVR !!!!!!!!!",__FUNCTION__ );
	g_message("%s DEBUG_DISABLE_DDNSSVR !!!!!!!!!",__FUNCTION__ );	
#endif	

#if 1 //defined(ENABLE_S1_UPNP_SCENARIO)
	create_upnp();
#endif

#ifndef	DEBUG_DISABLE_WEBSVR
//	create_webserver();
#else
	g_message("%s DEBUG_DISABLE_WEBSVR !!!!!!!!!",__FUNCTION__ );
	g_message("%s DEBUG_DISABLE_WEBSVR !!!!!!!!!",__FUNCTION__ );	
#endif

	
	if( wait )
	{
		while( _nf_network->init_done != 1)
			g_usleep(1000*10);
	}
		
	nf_network_start();   //-20091017, ASM

	nf_pds_ctl();

#ifdef ENABLE_S1_DUAL
	nf_s1_dual_init();
#endif

	nf_network_monitor_init();
		
	return ret;
}

static void _netsvr_grp_auth_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{


}

static void _netsvr_covert_flag_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	int i;
	
	g_static_mutex_lock (&_covert_mutex);
	_covert_flag = 0; 
	for(i=0;i<NUM_ANALOG_CHANNEL;i++)
	{
		char buf[128];
		gboolean covert_status;
		snprintf(buf, sizeof(buf), "cam.C%d.covert", i);

		covert_status = nf_sysdb_get_bool(buf);
		if(covert_status)
		{
			_covert_flag |= (1 << i);
		}	
	}
	g_static_mutex_unlock (&_covert_mutex);

#ifdef DEBUG_NETSVR_LOG	
	if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_INIT ] )
		g_message("%s _covert_flag[0x%04x]", __FUNCTION__, _covert_flag);	
#endif 
}

guint _netsvr_get_convert_flag(void)
{
	guint covert_tmp;

#if 1	
	covert_tmp = nf_notify_get_param0("sysdb_covert");
#else
	g_static_mutex_lock (&_covert_mutex);
	covert_tmp = _covert_flag;
	g_static_mutex_unlock (&_covert_mutex);

#endif

#ifdef DEBUG_NETSVR_LOG	
	if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_INIT  ] )
		g_message("%s _covert_flag[0x%04x]", __FUNCTION__, covert_tmp);
#endif

	return covert_tmp;		
}


static volatile guint _network_stop_reason = 0;

/**
	@brief				get network server last stop reason
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
guint	nf_network_get_stop_reason()
{
	return 	_network_stop_reason;
}

extern 	void rtp_drop_mudex_init(void);
extern int nf_openmode_dns_lookup(char *host, char *ip_str);

/**
	@brief				network server start
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_network_start( void )
{
	gint ret = 0;	
	static int frist_server_start = 0;
	
	g_message("%s",__FUNCTION__);	
	
	g_return_val_if_fail (_nf_network != NULL, FALSE);	
	g_return_val_if_fail (gServer_info.state == NETSVR_STAT_READY, FALSE);	
		

#if !defined(__RA_USED__)
	if(frist_server_start == 0)
	{
		nf_issm_ctl(ISSM_START, __FUNCTION__);
		nf_row_init();
		frist_server_start = 1;
	}
	else
	{
		nf_issm_ctl(ISSM_STOP, __FUNCTION__);
		nf_issm_ctl(ISSM_START, __FUNCTION__);
	}
	gServer_info.state = NETSVR_STAT_RUN;
	_network_stop_reason = 0;	// choissi 2009-11-30 ���� 4:56:56 for webra

	return 1; 
#else

	ret = _netsvr_init_sock();
	if( ret < 0)
	{
		g_warning("%s failed!! ret[%d]", __FUNCTION__, ret);
		return 0;
	}

	gServer_info.state = NETSVR_STAT_RUN;
	gServer_info.mode = CLI_WAITCS;	
	
	_nf_network->accept_thread = g_thread_create(
					(GThreadFunc)_netsvr_accept_thread_func,
					_nf_network, FALSE, NULL);
	
	_network_stop_reason = 0;	// choissi 2009-11-30 ���� 4:56:56 for webra
	
	return 1;
#endif
}


/**
	@brief				network server stop 						
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_network_stop( NF_DISCONN_REASON_E reason )
{
	unsigned short hi, low;
	CLIENT_INFO *pClient = NULL;
	int wait_cnt = 0;
	
	g_message("%s reason[0x%08x]",__FUNCTION__, reason);
	
	g_return_val_if_fail (_nf_network != NULL, FALSE);	
	g_return_val_if_fail (gServer_info.state == NETSVR_STAT_RUN, FALSE);	


#if !defined(__RA_USED__)
	
	gServer_info.state = NETSVR_STAT_STOP;

	nf_issm_ctl(ISSM_STOP, __FUNCTION__);

 	gServer_info.state = NETSVR_STAT_READY;
	if(reason)
		_network_stop_reason = reason; // choissi 2009-11-30 ���� 4:56:56 for webra

	return 1;

#endif
	
	gServer_info.state = NETSVR_STAT_STOP;
	_netsvr_close_sock();
	gServer_info.state = NETSVR_STAT_READY;
	
	if( reason )
	{		

		hi = (reason >> 16);
		low = (reason & 0xffff);
		
		_network_stop_reason = reason; // choissi 2009-11-30 ���� 4:56:56 for webra
		
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_INIT] )
		g_message("%s _dr_disconnect hi[0x%04x] low[0x%04x]", __FUNCTION__,hi,low);
#endif			
		
		_dr_disconnect(hi,low);
		g_usleep(1000*10);
		
		_dr_disconnect(hi,low);
		g_usleep(1000*10);

	}

	sleep(1);
			
	pClient = NULL;
	while(1)
	{										
		Pthread_mutex_lock(&gServer_info.client_mutex);
		{	// get client_info
			if(pClient == NULL)
				pClient = TAILQ_FIRST( &gServer_info.client_head );
			else
				pClient = TAILQ_NEXT(pClient, entries);
		}	
		Pthread_mutex_unlock(&gServer_info.client_mutex);
					
		if(pClient == NULL) break; 	
		if(pClient->magic_key != CLIENT_MAGIC) continue;
	
		_client_set_mode( pClient, CLIENT_MODE_CLOSE );	
	}

	sleep(1);
	
	wait_cnt = 0;
	while(1)	
	{
		int client_count = _netsvr_get_client_count();
		
		++wait_cnt;
		
		g_message("%s wait_cnt[%d] client_cnt[%d]",__FUNCTION__ , wait_cnt , client_count);
		if( client_count == 0 )
		{
			break;
		}
					
		if( wait_cnt > 10 )
		{
			g_warning("%s wait_cnt failed", __FUNCTION__);
			break;
		}		
		g_usleep(1000*1000);
		
	}

	nf_notify_fire_params("net_status", 0, 0, 0, 0);
	g_message("%s end",__FUNCTION__);
								
	return 1;
}

static 
gboolean _network_post_test_netsvr( gint is_ddns, GError **error )
{
	gint ret;

	g_return_val_if_fail (_nf_network != NULL, FALSE);	

 	ret = nf_sysdb_get_bool("net.proto.clienton"); 	
	if(ret == 0)
	{
		g_set_error( error, NF_NETSVR_ERROR, -3, "configuration error" );
		return ret;	
	}
		
	ret = port_test_netsvr(is_ddns);
	g_message("%s port_test_netsvr ret[%d]",__FUNCTION__, ret);
	
	if(ret == 1) // ok
		return 1;
	else if(ret == -1)	// dns resolve error
		g_set_error( error, NF_NETSVR_ERROR, ret, "ddns resolve error" );		
	else if(ret == -2)	// port forward error
		g_set_error( error, NF_NETSVR_ERROR, ret, "port forward error" );		
	else 
		g_set_error( error, NF_NETSVR_ERROR, ret, "unexpected error" );	
	
	return 0;	
}

static 
gboolean _network_post_test_websvr( gint is_ddns, GError **error )
{
	gint ret;
	
	g_return_val_if_fail (_nf_network != NULL, FALSE);	

	ret = nf_sysdb_get_bool("net.proto.webon");
	if(ret == 0)
	{
		g_set_error( error, NF_NETSVR_ERROR, -3, "configuration error" );	
		return ret;
	}
		
	ret = port_test_websvr(is_ddns);
	g_message("%s port_test_websvr ret[%d]",__FUNCTION__, ret);
	
	if(ret == 1) // ok
		return 1;
	else if(ret == -1)	// dns resolve error
		g_set_error( error, NF_NETSVR_ERROR, ret, "ddns resolve error" );		
	else if(ret == -2)	// port forward error
		g_set_error( error, NF_NETSVR_ERROR, ret, "port forward error" );		
	else 
		g_set_error( error, NF_NETSVR_ERROR, ret, "unexpected error" );	
				
	return 0;
}

/**
	@brief				netsvr port test
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_network_port_test_netsvr(  NF_NETWORK_PORT_TEST_ADDRESS_E is_ddns, GError **error )
{
	return _network_post_test_netsvr( is_ddns, error );
}


/**
	@brief				webserver port test
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_network_port_test_websvr( NF_NETWORK_PORT_TEST_ADDRESS_E is_ddns, GError **error )
{	
	return _network_post_test_websvr( is_ddns, error );
}



/**
	@brief				network server stop 						
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_network_ddns_test( GError **error )
{	
	gint ret;	
	g_return_val_if_fail (_nf_network != NULL, FALSE);	
	
	return 0;
}


#if 0
int _netsvr_debug(gint idx )
{		
	g_return_val_if_fail( idx >=0 && idx < DEBUG_NETSVR_IDX_NR, 0);
	
	return _DEBUG_NETSVR_log[idx];
}
#endif

static int _netsvr_init_info(void)
{
	g_message("%s called", __FUNCTION__);

	memset(&gServer_info, 0x00, sizeof(SERVER_INFO));
	gServer_info.state = NETSVR_STAT_READY;
	gServer_info.mode = CLI_WAITCS;	
	
	Pthread_mutex_init(&gServer_info.client_mutex, NULL);
	TAILQ_INIT(&gServer_info.client_head);

	Pthread_mutex_init(&gServer_info.job_mutex, NULL);
	TAILQ_INIT(&gServer_info.job_head);
		
	return 1;
}

static int _netsvr_init_sock(void)
{
	struct sockaddr_in addr;
	int ret;
	unsigned int len = sizeof(struct sockaddr_in);
	int value, conn_fd;

	int port = nf_sysdb_get_uint("net.proto.clientport");
	
	g_message("%s [Network server socket initialize]", __FUNCTION__);
	
	if( gServer_info.conn_fd > 0) 
	{
		g_warning("%s alreay conn_fd open[%d]",  __FUNCTION__, gServer_info.conn_fd);			
		return -1;
	}

	if( port == 0 || port > 65535) 
	{
		g_warning("%s port invalid[%d]",  __FUNCTION__, port);			
		return -2;
	}
			
	conn_fd = Socket(AF_INET, SOCK_STREAM, 0);
	if(conn_fd == -1)	goto error_fd;

	value=1;
    ret = Setsockopt(conn_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));
    if(ret == -1)	goto error_fd;
    	    	   
	value=SOCK_SND_BUFF; 
    ret = Setsockopt(conn_fd, SOL_SOCKET, SO_SNDBUF, &value, sizeof(int));
    if(ret == -1)	goto error_fd;
    
    value=SOCK_RCV_BUFF;
    ret = Setsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &value, sizeof(int));
    if(ret == -1)	goto error_fd;

	g_message("%s clientport[%d]", __FUNCTION__, port);
    	
	addr.sin_family 	= AF_INET;
	addr.sin_port		= htons(port);
	addr.sin_addr.s_addr= INADDR_ANY;

	ret = Bind(conn_fd, (struct sockaddr *)&addr, len);
	if(ret == -1) goto error_fd;

	ret = Listen(conn_fd, 2);
	if(ret == -1) goto error_fd;

	gServer_info.conn_fd = conn_fd;	
	g_message("%s open conn_fd[%d]", __FUNCTION__, conn_fd);

	return 1;
	
error_fd:		
	if(conn_fd >0)
	{
		Close(conn_fd); // gServer_info.conn_fd
		gServer_info.conn_fd = -1;
	}
	return -3;
}
 
 static void _netsvr_close_sock(void) 
{
	g_message("%s gServer_info socket resource close", __FUNCTION__);
	
	// SERVER SOCKET CLOSE
	if( gServer_info.conn_fd > 0) 
	{
		g_message("%s close conn_fd[%d]", __FUNCTION__, gServer_info.conn_fd);
		Close(gServer_info.conn_fd); 
		gServer_info.conn_fd = -1;
	}

}		
 
int _netsvr_get_client_count(void)
{
	return gServer_info.client_count;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _netsvr_accept_client(int s, struct sockaddr_in *ptr_addr)
{		
	pthread_t	tid;
	CLIENT_INFO *pClient = NULL;
	int			client_count,playid =-1;
	int			uniqueid = 0;
	char 		cs_ip[128], ds_ip[128];

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_ACCEPT] )
		g_message("%s svr mode[%d] fd[%d]", __FUNCTION__, gServer_info.mode, s);
#endif
		
	switch(gServer_info.mode) {
		case CLI_WAITCS:
									
//			g_message("%s fd[%d] send_keycode", __FUNCTION__, s);
			if(_netsvr_send_keycode(s) < 0) 
			{
				goto __error;
			}

			memset(&gClient_info, 0x00, sizeof(CLIENT_INFO));
						
			gClient_info.cs = s;
			gServer_info.mode = CLI_WAITDS;
			memcpy ( &gClient_info.peer_addr, ptr_addr, sizeof( struct sockaddr_in ));
			
			break;
			
		case CLI_WAITDS:
						
			sprintf(cs_ip,"%s", inet_ntoa(gClient_info.peer_addr.sin_addr));
			sprintf(ds_ip,"%s", inet_ntoa(ptr_addr->sin_addr));
								
			if( strncmp(cs_ip, ds_ip, sizeof(cs_ip))  )
			{
				g_message("%s diff ip addr cs[%s] ds[%s]", __FUNCTION__, cs_ip, ds_ip);
				goto __error_ds;
			}
			
//			g_message("%s fd[%d] verify_keycode", __FUNCTION__, s);
			if(_netsvr_verify_keycode(s) < 0)
			{
				goto __error_ds;
			}
			
//			g_message("%s fd[%d] login_user", __FUNCTION__, s);
			if(_netsvr_login_user(&gClient_info) < 0)
			{			
				goto __error_ds;
			}			
			gClient_info.ds = s;
			gServer_info.mode = CLI_WAITCS;
			
#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_ACCEPT] )
				g_message("%s auth [0x%04x]",__FUNCTION__, gClient_info.auth);
#endif

			Pthread_mutex_lock(&gServer_info.client_mutex);
			{			
				client_count = gServer_info.client_count;
				uniqueid = ++gServer_info.client_seq;
			}	
			Pthread_mutex_unlock(&gServer_info.client_mutex);
			
			if(client_count >= MAX_CLIENT)
			{
				g_warning("%s MAX_CLIENT[%d]", __FUNCTION__, client_count);

			    if(!gsock_simpleReply(gClient_info.cs, DRE_CLIENT_FULL, GSOCK_TIMEOUT)) 
			    	perror("gsock_writen()");				
				
				goto __error_ds;
			}

			// client tailq node malloc
			pClient = (CLIENT_INFO *)Calloc( sizeof(CLIENT_INFO), 1);
			if(!pClient)
			{
				goto __error_ds;
			}
			
			// client_node copy and init
			memcpy( pClient, &gClient_info, sizeof(CLIENT_INFO));
			
			Pthread_mutex_init(&pClient->mutex, NULL);
			Pthread_mutex_init(&pClient->ds_mutex, NULL);
			Pthread_mutex_init(&pClient->cs_mutex, NULL);
			
			pClient->magic_key = CLIENT_MAGIC;
			pClient->mode = CLIENT_MODE_INIT;
			pClient->playid = pClient->archid = -1;
			pClient->uniqueid = uniqueid;
			
			// global CLIENT_INFO reset						
			memset(&gClient_info, 0x00, sizeof(CLIENT_INFO));

			// client thread create
			if(Pthread_create(&tid, 0, _client_thread_func, pClient)) 
			{
				free( pClient );
				goto __error_ds;
			}
			pClient->cs_tid = tid;
			Pthread_detach(tid);

#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_ACCEPT] )
				g_message("%s tid[0x%08x] MAX_CLIENT[%d] playid[%d] uniqueod[%d]", __FUNCTION__,  
						tid, client_count+1, playid, uniqueid);
#endif			
			// client information update.
			Pthread_mutex_lock(&gServer_info.client_mutex);
			{				
				_netsvr_enque_client_entry(&gServer_info, pClient );
				_netsvr_print_client_entry(&gServer_info);
				//gServer_info.ptr_client_arr[playid] = pClient;
			}	
			Pthread_mutex_unlock(&gServer_info.client_mutex);
			break;

		default:
			break;
	}

	return 1;

__error_ds:	
	
	g_message("%s accept #1 socket error!!",__FUNCTION__);
	if(gClient_info.cs >0)
	{		
		g_message("%s close fd[%d]", __FUNCTION__, gClient_info.cs);
		Close(gClient_info.cs);			
	}

__error:
	g_message("%s accept #2 socket error!!",__FUNCTION__);
	if(s >0)
	{
		g_message("%s close fd[%d]", __FUNCTION__, s);
		Close(s);
	}
	
	memset(&gClient_info, 0x00, sizeof(CLIENT_INFO));
	gServer_info.mode = CLI_WAITCS;

	return -1;
}


#define DEFAULT_SOCK_TIMEOUT_SEC	1 
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _netsvr_accept_thread_func(void *arg)
{
	struct sockaddr_in 	addr;
	socklen_t len;
	fd_set wkset;
	int mx, i, n, sock, ret;
	struct timeval tv;

	unsigned int port = nf_sysdb_get_uint("net.proto.clientport");

	NF_NETIF_GET_STAT net_stat;	
	nf_netif_get_stat( _eth_dev, &net_stat);
	
	//memset( &_dac_playback, 0x00, sizeof(GTimeVal) );
	
	g_message("%s start", __FUNCTION__);
	
	while(1) {
		unsigned new_port = nf_sysdb_get_uint("net.proto.clientport");
		
		if(gServer_info.state != NETSVR_STAT_RUN)
		{
			g_message("%s netsvr stop[%d]", __FUNCTION__, gServer_info.state);
			break;
		}
		
		if( port != new_port)
		{
			g_warning("%s port change [%d]->[%d]",__FUNCTION__, port, new_port );
			_netsvr_close_sock();
			_netsvr_init_sock();
			port = new_port;
		}
		mx = gServer_info.conn_fd + 1;
		
		FD_ZERO(&wkset);
		FD_SET(gServer_info.conn_fd, &wkset);		
		
		tv.tv_sec	= 1;
		tv.tv_usec 	= 0;	

		n = Select(mx, &wkset, 0, 0, &tv);
		if(n == -1)
		{	// select error;
			g_usleep(1000*10);
			continue;
		}
		else if(n == 0) 
		{	// select timeout	// send stat	
			static int before_link_status = 0;
			
			NF_NETIF_GET_STAT delta_stat;
			int playback_cnt = 0;
			int live_cnt = gLive_info.client_count_runtime;
			int client_cnt = gServer_info.client_count;
			int link_status = 0;

			nf_netif_get_link_status(_eth_dev, &link_status );
			if( before_link_status != link_status)
			{
				g_warning("%s LINK status [%d]", _eth_dev, link_status);
				g_warning("%s LINK status [%d]", _eth_dev, link_status);
				
				before_link_status = link_status ;
			}
				
			// dev_name, prev, next, delta
			printf("%s LINK status [%d]", _eth_dev, link_status);
			if( nf_netif_get_delta(_eth_dev, &net_stat, &net_stat, &delta_stat) )
			{
				
#if 1 //def	DEBUG_NETSVR_NET_STAT
				g_message("%s rx[%lld][%lld][%lld] tx[%lld][%lld][%lld] %d.%06d",  
					__FUNCTION__, 
					delta_stat.rx_byte, delta_stat.rx_packet, delta_stat.rx_error,
					delta_stat.tx_byte, delta_stat.tx_packet, delta_stat.tx_error,
					delta_stat.tval.tv_sec, delta_stat.tval.tv_usec );
#endif

				gServer_info.rx_byte   = delta_stat.rx_byte   ;
				gServer_info.rx_packet = delta_stat.rx_packet ;
				gServer_info.rx_error  = delta_stat.rx_error  ;
				gServer_info.tx_byte   = delta_stat.tx_byte   ;
				gServer_info.tx_packet = delta_stat.tx_packet ;
				gServer_info.tx_error  = delta_stat.tx_error  ;
				// gServer_info.net_status = 
									
				nf_notify_fire_params("net_rxtx", gServer_info.net_status, 
											(unsigned int)(gServer_info.rx_byte/1024),
											(unsigned int)(gServer_info.tx_byte/1024),
											link_status);
												
				for(i=0;i<DPS_COMM_CNT;i++)
				{
					if( gServer_info.ptr_client_arr[i] != NULL)
						++playback_cnt;
				}
												
				live_cnt = ( client_cnt - playback_cnt > live_cnt ) ? live_cnt : (client_cnt - playback_cnt);										
				nf_notify_fire_params("net_status", 
											client_cnt, 
											live_cnt, 
											playback_cnt, 
											0);
			}			
			g_usleep(1000*10);
			continue;			
		}

		if(FD_ISSET(gServer_info.conn_fd, &wkset)) 
		{

			memset(&addr, 0x00, sizeof(struct sockaddr_in));		
			len = sizeof(struct sockaddr_in);

			sock = Accept(gServer_info.conn_fd, (struct sockaddr *)&addr, &len);

#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_ACCEPT] )										
				g_message("%s Accept peer IP[%s]PORT[%d] fd[%d]", __FUNCTION__, 
							inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),  sock);
#endif
			
			if(sock<0)
			{
				continue;
			}

			// net client on/off check
			if( nf_sysdb_get_bool("net.proto.clienton") != 1)
			{				
				Close(sock);
				continue;
			}
												
			_sock_set_timeout(sock, (unsigned int)DEFAULT_SOCK_TIMEOUT_SEC);
			ret = _netsvr_accept_client(sock, &addr);			
			
			if(ret == 1)
				_sock_set_timeout(sock, (unsigned int)0);
				
			//return 1;
		}
	}	
	
	//memset( &_dac_playback, 0x00, sizeof(GTimeVal) );
	
	g_message("%s end", __FUNCTION__);
	
	return 0;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _netsvr_send_keycode(int s)
{
	vpacket_t	vp;
	DRP_CONNECT	con;
	unsigned int seed = 9999;
	unsigned short dlen = 0;
	
	memset( &vp, 0x00, sizeof(vpacket_t));
	if(!gsock_readvp(s, &vp, 500)) {	// T0807_004
		g_warning("%s gsock_readvp fd[%d]", __FUNCTION__, s);
		return -1;
	}
	
	dlen = ntohs(vp.dlen);
	if(vp.type != VPT_REQUEST || vp.code != VPR_CONNECT || dlen != 0) 
	{
		g_warning("%s vp type[0x%02x]/[0x%02x] code[0x%02x]/[0x%02x] len[%d]",
					__FUNCTION__, 
					vp.type, VPT_REQUEST, 
					vp.code, VPR_CONNECT, dlen );
		return -1;
	}

	int client_count = gServer_info.client_count;
	if(client_count >= MAX_CLIENT )
	{
		g_warning("%s MAX_CLIENT[%d]", __FUNCTION__,  client_count);
		if(!gsock_simpleReply(s, DRE_CLIENT_FULL, GSOCK_TIMEOUT)) 
		{
			perror("gsock_writen()");
			return -1;
		}
	}

	vp.type	= VPT_REPLY;
	vp.code = VPE_SUCCESS;
	vp.dlen = htons(sizeof(DRP_CONNECT));

	gServer_info.key = rand_r((unsigned int *)&seed);

	memset( &con, 0x00, sizeof(DRP_CONNECT));
	
	con.key_code = htonl(gServer_info.key);
			
	con.product_ver = NF_BSWAP_16( nf_sysman_get_fwver_product() );	
	con.protocol_ver = NF_BSWAP_16( nf_sysman_get_fwver_protocol() );
	
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_ACCEPT] )
		g_message("%s key[%08x] product_ver[0x%04x] protocol_ver[0x%04x]", __FUNCTION__, 
				gServer_info.key, con.product_ver, con.protocol_ver );				
#endif

	if(!gsock_writevp(s, &vp, GSOCK_TIMEOUT)) {
		g_warning("%s gsock_writevp fd[%d]", __FUNCTION__, s);
		return -1;
	}

	if(!gsock_writen(s, &con, sizeof(DRP_CONNECT), GSOCK_TIMEOUT)) {
		g_warning("%s gsock_writen fd[%d]", __FUNCTION__, s);
		return -1;
	}

	return 0;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _netsvr_verify_keycode(int s)
{
	vpacket_t vp;
	
	unsigned int key;
	unsigned short dlen = 0;

	memset( &vp, 0x00, sizeof(vpacket_t));
	if(!gsock_readvp(s, &vp, 500)) 	// T0807_004
	{
		g_warning("%s gsock_readvp fd[%d]", __FUNCTION__, s);
		return -1;
	}

	dlen = ntohs(vp.dlen);
	if(vp.type != VPT_REQUEST || vp.code != VPR_CONNECT || dlen != 4) 
	{
		g_warning("%s vp type[0x%02x]/[0x%02x] code[0x%02x]/[0x%02x] len[%d]",
					__FUNCTION__, 
					vp.type, VPT_REQUEST, 
					vp.code, VPR_CONNECT, dlen  );
		return -2;
	}
	
	if(!gsock_readn(s, &key, 4, 500)) {
		g_warning("%s gsock_readn() fd[%d]",__FUNCTION__, s);
		return -3;
	}

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_ACCEPT] )
		g_message("%s key[%08x] input[%08x]", __FUNCTION__, 
				gServer_info.key, ntohl(key) );				
#endif

	if(ntohl(key) != gServer_info.key ) {
		g_message("%s key [0x%08x]/[0x%08x]",__FUNCTION__, 
				ntohl(key), gServer_info.key );
		return -4;
	}

	vp.type = VPT_REPLY;
	vp.code = VPE_SUCCESS;
	vp.dlen = htons( 4 );

	if(!gsock_writevp(s, &vp, GSOCK_TIMEOUT) ) {
		g_message("%s gsock_writevp()",__FUNCTION__);
		return -5;
	}

	if( !gsock_writen(s, &gServer_info.key, 4, GSOCK_TIMEOUT) ) {
		g_message("%s gsock_writen()",__FUNCTION__);
		return -6;
	}
	
	return 0; // all ok!!

}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int 
_sysdb_get_usridx_by_name( const char *username, const char *userpass )
{
	int idx;
	int usercnt;
	char 		 buff_name[256];
	char		 buff_pass[256];

//	g_message("%s", __FUNCTION__);
	usercnt = nf_sysdb_get_uint("usr.UCNT");
	if( usercnt <= 0)
	{
		g_warning("%s user count[%d]", __FUNCTION__, usercnt );
		return -1;
	}
	
	for(idx = 0; idx < usercnt; ++idx)
	{
		snprintf(buff_name, sizeof(buff_name), "usr.U%d.name", idx);	
		
		if(!strcmp(username, nf_sysdb_get_str_nocopy(buff_name))) 
		{
			return idx;		
		}
	}
	return -1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int 
_sysdb_get_grp_by_name( const char *grpname)
{
	unsigned int idx;
	unsigned int grpcnt;
	char 	buff_grp[256];
	
//	g_message("%s ", __FUNCTION__);
	grpcnt = nf_sysdb_get_uint("usr.grp.GCNT");

	for(idx = 0; idx < grpcnt; ++idx)
	{
		snprintf(buff_grp, sizeof(buff_grp), "usr.grp.G%d.name", idx);
		if (!strcmp(grpname, nf_sysdb_get_str_nocopy(buff_grp)))
		{
			return idx;
		}
	}
	
	return -1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static unsigned int 
_netsvr_login_auth_setup( char *sysdb_str, int idx, unsigned int authbit)
{
	char buff_group[256];
	snprintf(buff_group, sizeof(buff_group),"usr.grp.G%d.%s", idx, sysdb_str );	

	if(nf_sysdb_get_bool(buff_group))
	{
		return authbit;
	}
	else
	{
		return 0;
	}
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _netsvr_login_user(CLIENT_INFO *pClient)
{
	vpacket_t vp;
	DRREQ_LOGIN	log;
	
	unsigned int dlen;
	unsigned char errcode = 0;	
	unsigned char idx = 0;
	int cs = pClient->cs;
	int ret = 0;
	char *group, *pass;
	char buff_group[255];
	char buff_passwd[255];

	unsigned int usercnt = 0;
	
	memset( &vp, 0x00, sizeof(vpacket_t));

readvp_retry:
	if(!gsock_readvp(cs, &vp, VNET_TIMEOUT)) 
	{
		g_warning("%s gsock_readvp fd[%d]", __FUNCTION__, cs);
		
		if (errno == EINTR)		
			goto readvp_retry;

		return -1;
	}

	dlen = ntohs(vp.dlen);
	if(vp.type != VPT_REQUEST || vp.code != DR_LOGIN || dlen != sizeof(log) ) 
	{
		g_warning("%s vp type[0x%02x]/[0x%02x] code[0x%02x]/[0x%02x] len[%d]/[%d]",
					__FUNCTION__, 
					vp.type, VPT_REQUEST, 
					vp.code, DR_LOGIN, 
					dlen, sizeof(log) );
		return -1;
	}
	
	memset( &log, 0x00, sizeof(DRREQ_LOGIN));
	if(!gsock_readn(cs, &log, sizeof(log), GSOCK_TIMEOUT))
	{
		g_warning("%s gsock_readn fd[%d] len[%d]", __FUNCTION__, cs, sizeof(log) );
		return -1;
	}
	
#ifdef DEBUG_NETSVR_LOG	
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_LOGIN] )
		g_message("%s usrname[%s] passwd[%s]", __FUNCTION__, log.userName, log.passwd);	
#endif

#if 0
{
	// ---------------------
	// check user id
	errcode = DRE_USERID;
	
	for(idx=g_db.user.user.head; idx != 0xff; idx = g_db.user.user.user[idx].next) {
		if(!strcmp(log.userName, g_db.user.user.user[idx].name)) {
			
			// -----------------------
			// check user passwd
			if(!strcmp(log.passwd, g_db.user.user.user[idx].pass)) {
				
				group = g_db.user.user.user[idx].gidx;

				if(g_db.user.group.group[group].auth.search_archive)
					pClient->auth |= AUTH_SEARCH_ARCHIVE;
				if(g_db.user.group.group[group].auth.setup_alarm_evt_rec)
					pClient->auth |= AUTH_ALARM_EVT_REC;
				if(g_db.user.group.group[group].auth.setup_cam_dis_aud)
					pClient->auth |= AUTH_CAM_DISP_AUD;
				if(g_db.user.group.group[group].auth.setup_user)
					pClient->auth |= AUTH_USER;
				if(g_db.user.group.group[group].auth.setup_network)
					pClient->auth |= AUTH_NETWORK;
				if(g_db.user.group.group[group].auth.setup_system)
					pClient->auth |= AUTH_SYSTEM;
				if(g_db.user.group.group[group].auth.remote_logon)
					pClient->auth |= AUTH_REMOTE_LOGON;
				else {
					errcode = DRE_AUTHORITY;
					break;
				}
				if(g_db.user.group.group[group].auth.sys_shutdown)
					pClient->auth |= AUTH_SHUTDOWN;
				if(g_db.user.group.group[group].auth.sys_startup)
					pClient->auth |= AUTH_STARTUP;

				errcode = VPE_SUCCESS;				
				strncpy( pClient->userid, log.userName, sizeof(pClient->userid));
			}
			else 
				errcode = DRE_PASSWD;

			break;
		}
	}
}
#endif

	// ---------------------
	// check user id
	errcode = DRE_USERID;	
	ret = _sysdb_get_usridx_by_name( log.userName, log.passwd );
	if(ret == -1)
	{
		g_warning("%s user not found username[%s]", __FUNCTION__, log.userName);
		errcode = DRE_USERID;
		goto	__sendack;
	}
	snprintf(buff_passwd, sizeof(buff_passwd), "usr.U%d.pass", ret);
	pass = nf_sysdb_get_str_nocopy(buff_passwd);
	if( pass == NULL || strcmp(log.passwd, pass) != 0)
	{	
		g_warning("%s user wrong passwd[%s]", __FUNCTION__, log.passwd);
		errcode = DRE_PASSWD;
		goto	__sendack;
	}
	
	snprintf(buff_group, sizeof(buff_group), "usr.U%d.grpname", ret);				
	group = nf_sysdb_get_str_nocopy(buff_group);

	ret = _sysdb_get_grp_by_name(group);
	if (ret == -1)
	{
		g_warning("%s group not found[%s]", __FUNCTION__, group);
		errcode = DRE_USERID;
		goto	__sendack;
	}

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_LOGIN] )
		g_message("%s group name[%s]",__FUNCTION__, group);
#endif

	if (!strcmp(group, "ADMIN"))
	{	
		pClient->auth = 0xffff;
		errcode = VPE_SUCCESS;				
		strncpy( pClient->userid, log.userName, sizeof(pClient->userid));
	}
	else
	{
		pClient->auth  = _netsvr_login_auth_setup("auth_setup", ret , AUTH_SETUP);
		pClient->auth |= _netsvr_login_auth_setup("auth_search", ret  , AUTH_SEARCH);
		pClient->auth |= _netsvr_login_auth_setup("auth_archive", ret  , AUTH_ARCHIVE);
		pClient->auth |= _netsvr_login_auth_setup("auth_ptz", ret  , AUTH_PTZ);
		pClient->auth |= _netsvr_login_auth_setup("auth_alarm_off", ret  , AUTH_ALARM_OFF);
		pClient->auth |= _netsvr_login_auth_setup("auth_panic_rec", ret  , AUTH_PANIC_REC);
		pClient->auth |= _netsvr_login_auth_setup("auth_remote", ret  , AUTH_REMOTE);

		if ( pClient->auth & AUTH_REMOTE   )
		{
			errcode = VPE_SUCCESS;				
			strncpy( pClient->userid, log.userName, sizeof(pClient->userid));
		}
		else
		{
			g_warning("%s AUTH_REMOTE failed", __FUNCTION__);
			errcode = DRE_AUTHORITY;
		}			
	}
#ifdef DEBUG_NETSVR_LOG	
	g_message("%s pClient->auth[0x%04x]", __FUNCTION__,pClient->auth);
#endif 

	if(errcode != VPE_SUCCESS) {

#ifdef DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_LOGIN] )
		{
			if(errcode == DRE_USERID) 
				g_message("%s [Invalid userid]", __FUNCTION__);
			if(errcode == DRE_PASSWD)
				g_message("%s [Invalid passwd]", __FUNCTION__);
			if(errcode == DRE_AUTHORITY)
				g_message("%s [Invalid authority]", __FUNCTION__);
		}
#endif
		goto __sendack;
	}

__sendack:
	if(errcode == VPE_SUCCESS) {
		vp.type	= VPT_REPLY;
		vp.code = errcode;
		vp.dlen	= htons(sizeof(unsigned int));

		if(!gsock_writevp(cs, &vp, VNET_TIMEOUT)) {
			g_warning("%s gsock_writevp() fd[%d]", __FUNCTION__, cs);
			return -1;
		}

		if(!gsock_writen(cs, &group, sizeof(unsigned int), GSOCK_TIMEOUT)) {
			g_warning("%s gsock_writen() fd[%d]", __FUNCTION__, cs);
			return -1;
		}
	}
	else {
 	   	if(!gsock_simpleReply(cs, errcode, VNET_TIMEOUT)) {
   	     	g_warning("%s gsock_simpleReply() fd[%d] err_code[%d]",
   	     			__FUNCTION__, cs, errcode);
   	     	return -1;
   	 	}
	}

	return idx;
}

int	_client_find_streamid_by_pclient( CLIENT_INFO *pClient )
{
	int i;
	for( i=0; i< DPS_COMM_CNT; i++)
	{
		if( gServer_info.ptr_client_arr[i] == pClient)
			return i;
	}
	return -1;
}

int _client_get_empty_streamid()
{
	static unsigned int seq = 0;

	int ret_idx = -1, idx = 0;
	int try_cnt = DPS_COMM_CNT;
	
	while( --try_cnt )
	{				
		idx = (seq++) % DPS_COMM_CNT;
		if( gServer_info.ptr_client_arr[idx] == NULL)
		{
			ret_idx = idx;
			break;
		}
	}		
	return ret_idx;
}

int _client_set_streamid( CLIENT_INFO *info )
{
	int playid = -1;
	
	pthread_mutex_lock(&gServer_info.client_mutex);
	{
		playid = _client_get_empty_streamid();
		if(	playid != -1)
			gServer_info.ptr_client_arr[playid] = info;	
	}	
	Pthread_mutex_unlock(&gServer_info.client_mutex);
	
	return playid;
}

void _client_unset_streamid( CLIENT_INFO *info, int playid )
{	
	pthread_mutex_lock(&gServer_info.client_mutex);
	{
		if( gServer_info.ptr_client_arr[playid] == info)
			gServer_info.ptr_client_arr[playid] = NULL;
		else
			g_warning("%s not found playid[%d][%x]", 
				__FUNCTION__, playid, info);
	}	
	Pthread_mutex_unlock(&gServer_info.client_mutex);		
}

char *_str_dr_proto( int  prot ) {
	char *tmp=NULL;
	
	switch(prot) {	
		case DR_LOGIN 			  				:  tmp = "DR_LOGIN"	; break;
		case DR_GET_SYSINFO                     :  tmp = "DR_GET_SYSINFO"; break;
		case DR_START_LIVE                      :  tmp = "DR_START_LIVE"; break;
		case DR_STOP_LIVE                       :  tmp = "DR_STOP_LIVE"; break;
		case DR_START_PLAY                      :  tmp = "DR_START_PLAY"; break;
		case DR_STOP_PLAY                       :  tmp = "DR_STOP_PLAY"; break;
		case DR_GET_RECINFO                     :  tmp = "DR_GET_RECINFO"; break;
		case DR_GET_LOG                         :  tmp = "DR_GET_LOG"; break;
		case DR_SET_EVENTMASK                   :  tmp = "DR_SET_EVENTMASK"; break;
		case DR_START_BACKUP                    :  tmp = "DR_START_BACKUP"; break;
		case DR_STOP_BACKUP                     :  tmp = "DR_STOP_BACKUP"; break;
		case DR_PTZ                             :  tmp = "DR_PTZ"; break;
		case DR_GET_CURRENTTIME                 :  tmp = "DR_GET_CURRENTTIME"; break;
		case DR_SET_CURRENTTIME                 :  tmp = "DR_SET_CURRENTTIME"; break;
		case DR_AUTHENTICATION                  :  tmp = "DR_AUTHENTICATION"; break;
		case DR_GET_SETUP                       :  tmp = "DR_GET_SETUP"; break;
		case DR_SET_SETUP                       :  tmp = "DR_SET_SETUP"; break;
		case DR_RELEASE_SETUP                   :  tmp = "DR_RELEASE_SETUP"; break;
		case DR_GET_ACCOUNTINFO                 :  tmp = "DR_GET_ACCOUNTINFO"; break;
		case DR_DISCONNECT_CLIENT               :  tmp = "DR_DISCONNECT_CLIENT"; break;
		case DR_GET_FUNCTABLE                   :  tmp = "DR_GET_FUNCTABLE"; break;
		case DR_SYSTEM_FUNCTION                 :  tmp = "DR_SYSTEM_FUNCTION"; break;
		case DR_AUDIO_MUTE                      :  tmp = "DR_AUDIO_MUTE"; break;
		case DR_GET_CAMERA_TITLE                :  tmp = "DR_GET_CAMERA_TITLE"; break;
		case DR_GET_COVERT_STATUS               :  tmp = "DR_GET_COVERT_STATUS"; break;
		case DR_GET_PTZ_STATUS                  :  tmp = "DR_GET_PTZ_STATUS"; break;
		case DR_GET_ALARM_STATUS                :  tmp = "DR_GET_ALARM_STATUS"; break;
		case DR_ALARM_CONTROL                   :  tmp = "DR_ALARM_CONTROL"; break;
		case DR_GET_HDD_SIZE                    :  tmp = "DR_GET_HDD_SIZE"; break;
		case DR_GET_TIMEZONE                    :  tmp = "DR_GET_TIMEZONE"; break;
		case DR_GET_DATETIME_FORMAT             :  tmp = "DR_GET_DATETIME_FORMAT"; break;
		case DR_GET_CAMERA_NOVIDEO_STATUS       :  tmp = "DR_GET_CAMERA_NOVIDEO_STATUS"; break;
		case DR_TIMELINE                        :  tmp = "DR_TIMELINE"; break;
		case DR_DISCONNECT                      :  tmp = "DR_DISCONNECT"; break;
		case DR_SEND_EVENT                      :  tmp = "DR_SEND_EVENT"; break;
		case DR_KEEPALIVE                       :  tmp = "DR_KEEPALIVE"; break;
		case DR_LOCALSETUP_STARTED              :  tmp = "DR_LOCALSETUP_STARTED"; break;
		case DR_CHANGE_NETWORKAUDIO             :  tmp = "DR_CHANGE_NETWORKAUDIO"; break;
		case DR_CHANGE_CAMERATITLE              :  tmp = "DR_CHANGE_CAMERATITLE"; break;
		case DR_CHANGE_COVERT                   :  tmp = "DR_CHANGE_COVERT"; break;
		case DR_CHANGE_PTZ	                	:  tmp = "DR_CHANGE_PTZ"; break;
		case DR_CHANGE_ALARM                    :  tmp = "DR_CHANGE_ALARM"; break;
		case DR_CHNAGE_TIMEZONE                 :  tmp = "DR_CHNAGE_TIMEZONE"; break;
		case DR_CHNAGE_NOVIDEO                  :  tmp = "DR_CHNAGE_NOVIDEO"; break;
		case DR_CHNAGE_DATETIMEFORMAT           :  tmp = "DR_CHNAGE_DATETIMEFORMAT"; break;
		case DR_GET_FWVERSION                   :  tmp = "DR_GET_FWVERSION"; break;
		case DR_BACKDOOR                        :  tmp = "DR_BACKDOOR"; break;
		default:								   tmp = "DR_unknown"; break;
	}	
	return tmp;	
} 

char *_str_dr_proto_inform( int  prot ) {
	char *tmp=NULL;
	
	switch(prot) {	
		case DR_INFORM_KEEPALIVE                :  tmp = "DR_KEEPALIVE"; break;
		case DR_INFORM_LOCALSETUP_STARTED       :  tmp = "DR_LOCALSETUP_STARTED"; break;
		case DR_INFORM_CHANGE_NETWORKAUDIO      :  tmp = "DR_CHANGE_NETWORKAUDIO"; break;
		case DR_INFORM_CHANGE_CAMERA_TITLE      :  tmp = "DR_CHANGE_CAMERATITLE"; break;
		case DR_INFORM_CHANGE_COVERT            :  tmp = "DR_CHANGE_COVERT"; break;
		case DR_INFORM_CHANGE_PTZ	            :  tmp = "DR_CHANGE_PTZ"; break;
		case DR_INFORM_CHANGE_ALARM             :  tmp = "DR_CHANGE_ALARM"; break;
		case DR_INFORM_CHNAGE_TIMEZONE          :  tmp = "DR_CHNAGE_TIMEZONE"; break;
		case DR_INFORM_CHNAGE_NOVIDEO           :  tmp = "DR_CHNAGE_NOVIDEO"; break;
		case DR_INFORM_CHNAGE_DATETIMEFORMAT    :  tmp = "DR_CHNAGE_DATETIMEFORMAT"; break;
		case DR_INFORM_DISCONNECT				:  tmp = "DR_DISCONNECT"; break;
		default:                                   tmp = "DR_unknown"; break;
	}	
	return tmp;	
} 

void _client_free_job(JOB_INFO *pJobEntry, int in_queue)
{
	g_return_if_fail( pJobEntry != NULL);

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_CLIENT_JOB_QUE] )
		g_message("%s JOB req[0x%08x] ack[0x%08x] reqTime[%ld]", __FUNCTION__,
				pJobEntry->req_msg_id,
				pJobEntry->ack_msg_id,
				pJobEntry->req_timestamp.tv_sec );
#endif

	if(in_queue)
	{
		Pthread_mutex_lock(&gServer_info.job_mutex); 		
		{		
			TAILQ_REMOVE(&gServer_info.job_head, pJobEntry, entries);
			--gServer_info.job_count;
		}	
		Pthread_mutex_unlock(&gServer_info.job_mutex);
	}
	
	if(pJobEntry->msg)
		g_free(pJobEntry->msg);
			
	g_free(pJobEntry);
			
}


JOB_INFO *_client_peek_job(CLIENT_INFO *info, int req_msg_id)
{
	JOB_INFO *pJobEntry = NULL;
	JOB_INFO *pRetJobEntry = NULL;
							
	Pthread_mutex_lock(&gServer_info.job_mutex); 
	{												
		TAILQ_FOREACH( pJobEntry, &gServer_info.job_head, entries)
		{
			if(pJobEntry->req_msg_id== req_msg_id				 
				 && pJobEntry->pclient == info )
			{
				pRetJobEntry = pJobEntry;
				break;
			}
		}
	}
	Pthread_mutex_unlock(&gServer_info.job_mutex);

	return pRetJobEntry;
}

JOB_INFO *_client_new_job(CLIENT_INFO *pClient_info, int msg_id, void *msg, int msg_len )
{
	JOB_INFO *pJobEntry;
				
	pJobEntry = g_malloc0( sizeof(JOB_INFO) );		
	g_return_val_if_fail( pJobEntry, NULL);
		
	pJobEntry->pclient = pClient_info;
	pJobEntry->req_msg_id = msg_id;

	if( msg )
	{
		pJobEntry->msg = g_malloc( msg_len );
		if(!pJobEntry->msg)
		{
			g_free(pJobEntry);
			g_return_val_if_reached(0);
		}else{
			memcpy(pJobEntry->msg, msg, msg_len);
			pJobEntry->msg_len = msg_len;
		}
	}
	gettimeofday(&pJobEntry->req_timestamp, NULL);
	
	return 	pJobEntry;
}

int _client_enque_job(JOB_INFO *pJobEntry) 
{		

	g_return_val_if_fail( _nf_network != NULL, -1);
	g_return_val_if_fail( pJobEntry != NULL, -2);
			
#if 1
	Pthread_mutex_lock(&gServer_info.job_mutex); 		
	{		
		TAILQ_INSERT_TAIL( &gServer_info.job_head, pJobEntry, entries);
		++gServer_info.job_count;		
	}
	Pthread_mutex_unlock(&gServer_info.job_mutex);
#endif

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_CLIENT_JOB_QUE] )
		g_message("%s JOB req[0x%08x] ack[0x%08x] reqTime[%ld]", __FUNCTION__,
				pJobEntry->req_msg_id,
				pJobEntry->ack_msg_id,
				pJobEntry->req_timestamp.tv_sec );
#endif
	
	g_async_queue_push( _nf_network->queue, pJobEntry);		
	{
		gint q_len = g_async_queue_length( _nf_network->queue );
		if( !(q_len & 63) && q_len)
			g_warning("%s _nf_network->queue len[%d]", __FUNCTION__, q_len);
		else{
#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[ DEBUG_NETSVR_IDX_CLIENT_JOB_QUE] )
				g_message("%s _nf_network->queue len[%d]", __FUNCTION__, q_len);
#endif
		}
	}		
	return 1;
}

JOB_INFO *_client_enque_job_simple(CLIENT_INFO *pClient_info, int msg_id, void *msg, int msg_len )
{
	JOB_INFO *pJobEntry;
	gint	ret = 0;
	
	g_return_val_if_fail( _nf_network != NULL, NULL);	

	pJobEntry = _client_new_job(pClient_info, msg_id, msg, msg_len);	
	g_return_val_if_fail( pJobEntry != NULL, NULL);
	
	ret = _client_enque_job(pJobEntry);
	if(ret == 1){
		return pJobEntry;
	}else{
		_client_free_job(pJobEntry, 0);	 // only free	
		return NULL;		
	}
}


static short _ulaw2short[256]={
	-32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,-23932,-22908,
	-21884,-20860,-19836,-18812,-17788,-16764,-15996,-15484,-14972,-14460,
	-13948,-13436,-12924,-12412,-11900,-11388,-10876,-10364,-9852,-9340,
	-8828,-8316,-7932,-7676,-7420,-7164,-6908,-6652,-6396,-6140,
	-5884,-5628,-5372,-5116,-4860,-4604,-4348,-4092,-3900,-3772,
	-3644,-3516,-3388,-3260,-3132,-3004,-2876,-2748,-2620,-2492,
	-2364,-2236,-2108,-1980,-1884,-1820,-1756,-1692,-1628,-1564,
	-1500,-1436,-1372,-1308,-1244,-1180,-1116,-1052,-988,-924,
	-876,-844,-812,-780,-748,-716,-684,-652,-620,-588,
	-556,-524,-492,-460,-428,-396,-372,-356,-340,-324,
	-308,-292,-276,-260,-244,-228,-212,-196,-180,-164,
	-148,-132,-120,-112,-104,-96,-88,-80,-72,-64,
	-56,-48,-40,-32,-24,-16,-8,0,32124,31100,
	30076,29052,28028,27004,25980,24956,23932,22908,21884,20860,
	19836,18812,17788,16764,15996,15484,14972,14460,13948,13436,
	12924,12412,11900,11388,10876,10364,9852,9340,8828,8316,
	7932,7676,7420,7164,6908,6652,6396,6140,5884,5628,
	5372,5116,4860,4604,4348,4092,3900,3772,3644,3516,
	3388,3260,3132,3004,2876,2748,2620,2492,2364,2236,
	2108,1980,1884,1820,1756,1692,1628,1564,1500,1436,
	1372,1308,1244,1180,1116,1052,988,924,876,844,
	812,780,748,716,684,652,620,588,556,524,
	492,460,428,396,372,356,340,324,308,292,
	276,260,244,228,212,196,180,164,148,132,
	120,112,104,96,88,80,72,64,56,48,
	40,32,24,16,8,0
};

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void* _client_thread_func(void *arg)
{
	CLIENT_INFO *pClient;
	vpacket_t	vp;
	char		vp_buff[1024*64];
	char		*vp_proto_str;
	int 		ret, read_size;		
	unsigned int prev_frame_count = 0;		
		
	pClient = (CLIENT_INFO *)arg;
	
	if( pClient == NULL )
	{
		g_warning("%s pClient is null", __FUNCTION__);
		pthread_exit(NULL);
	}

#ifdef DEBUG_NETSVR_LOG
	g_message("%s cs[%d]ds[%d] uid[%d] ClientThread Start!! ", __FUNCTION__, 
					pClient->cs, pClient->ds, pClient->uniqueid);
#endif
	//BF18 "REMOTE LOG ON
	_client_eventlog_put(pClient, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_LOCAL_LOG_ON_LIVE_DISPLAY, NULL);
	
	//SendEventLogCmd( LT_REMOTE_LOG_ON, 1, LP2_REMOTE_LOG_ON_NR, pClient->userid);
	while(1) 
	{
		read_size = ret = 0;
		vp_proto_str = NULL;
								
		ret = _client_csds_read(pClient, 90);
		if( ret == 1 )
		{	// read from control socket
			memset( &vp, 0x00, sizeof(vpacket_t));
			read_size = sizeof(vpacket_t);
			if( (ret = Readn(pClient->cs, &vp, read_size)) !=  read_size) 
			{	
				if(ret != 0)		
					g_warning("%s cs[%d]ds[%d] Readn Err vpacket_t rtn[%d]",
							__FUNCTION__,pClient->cs, pClient->ds, ret );
												
				goto __error;
			}						
	
			vp.dlen = ntohs(vp.dlen);
			vp_proto_str = _str_dr_proto(vp.code);
			
			memset( vp_buff, 0x00, vp.dlen);
									
			if( vp.dlen >0)
			{				
				if( (ret = Readn(pClient->cs, vp_buff, vp.dlen)) !=  vp.dlen) 
				{			
					g_warning("%s cs[%d]ds[%d] [%02x][0x%02x][%s]/[%d] Readn Err vp_buff rtn[%d] ",
								__FUNCTION__,pClient->cs, pClient->ds, 
								vp.type, vp.code, vp_proto_str, vp.dlen, ret );
					goto __error;
				}						
			}
			
			if(vp.type == VPT_REPLY)
				continue;
							
			++pClient->req_count;

#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_CMD_PROC] )
				g_message("%s  cs[%d]ds[%d] [%x][0x%02x][%s]/[%d] recv",
							__FUNCTION__, pClient->cs, pClient->ds, 
							vp.type, vp.code, vp_proto_str, vp.dlen);			
			
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_PACKET] && vp.dlen > 0)
				nf_debug_hexdump(vp_buff, vp.dlen>32 ? 32:vp.dlen );
#endif							
			Pthread_mutex_lock(&pClient->cs_mutex);
			ret = _client_process_rccmd(pClient, &vp, vp_buff);			
			Pthread_mutex_unlock(&pClient->cs_mutex);
			
#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_CMD_PROC_RET] )
				g_message("%s  cs[%d]ds[%d] [%x][0x%02x][%s]/[%d] ret[%d]\n",
							__FUNCTION__, pClient->cs, pClient->ds, 
							vp.type, vp.code, vp_proto_str, vp.dlen, ret );
#endif	
			
			if( ret < 0) 
			{	
				int send_ret = VPE_FAIL;
				int fatal_error = 0;
					
				// we have some error, but it's ok
				if( ret == NETSVR_RET_ERR_NO_IMPL ){
					send_ret = VPE_FAIL;		// request failed
				}else if( ret == NETSVR_RET_ERR_PARAM ){
					send_ret = VPE_FAIL;		// invalid parameters/data
				}else if( ret == NETSVR_RET_ERR_INTERNAL ){
					send_ret = VPE_INTERNAL;	// internal implementation error

				}else if( ret == NETSVR_RET_ERR_CMD_INQUE ){
					send_ret = VPE_FAIL;		
				}else if( ret == NETSVR_RET_ERR_BUSY ){
					send_ret = VPE_FAIL;		

				// fatal_error 
				}else if( ret == NETSVR_RET_ERR_PROTOCOL ){
					fatal_error = 1;
					send_ret = VPE_PROTOCOL;	// protocol error
				}else if( ret == NETSVR_RET_ERR_SOCKET ){
					fatal_error = 1;
					send_ret = VPE_PROTOCOL;	// socket error
				}else{	// NETSVR_RET_ERR
					fatal_error = 1;
					send_ret = VPE_FAULT;		// request failed
				}
				
				if( fatal_error == 0 )
				{
					if( gsock_simpleReply(pClient->cs, send_ret, VNET_TIMEOUT) < 0) 
					{
						g_warning("%s cs[%d]ds[%d] [%02x][0x%02x][%s]/[%d] gsock_simpleReply(VPE_FAIL) ret[%d]", 
								__FUNCTION__, pClient->cs, pClient->ds, 
								vp.type, vp.code, vp_proto_str, vp.dlen, ret );

						goto __error;
					}
					continue;
				}else{

					gsock_simpleReply(pClient->cs, send_ret, VNET_TIMEOUT);
					
					g_warning("%s cs[%d]ds[%d] [%02x][0x%02x][%s]/[%d] ret[%d]", 
							__FUNCTION__, pClient->cs, pClient->ds, 
							vp.type, vp.code, vp_proto_str, vp.dlen, ret );
																		
					goto __error;
				}								
			}	
			//g_usleep(1);
			
		}
		else if( ret == 2 ) // liveaudio from RA
		{	
			unsigned char buff[800];
			short buff_conv[800];
			JOB_INFO 	*pJobEntry = NULL;
			
			//memset( buff, 0x00, sizeof(buff));
			read_size = Recv( pClient->ds, buff, sizeof(buff), 0);
			g_message("%s cs[%d]ds[%d] liveauido Recv ret[%d]", 
						__FUNCTION__, pClient->cs, pClient->ds, read_size );
						
			if(read_size > 0 )
			{
				gint is_rx_audio = (gint)nf_sysdb_get_bool("audio.rx");
				int i;

#ifdef DEBUG_NETSVR_LOG
			if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_AUDIO] )
				HexDump( buff, 128, 0);
#endif			
				
				if( gLive_info.liveaudio_fd > 0 && is_rx_audio )
				{	
									
					for(i=0;i<read_size;i++)
						buff_conv[i] = _ulaw2short[buff[i]];
												
					pJobEntry = _client_new_job(pClient, 0, buff_conv, read_size*2);
					if(pJobEntry) 
						g_async_queue_push( _nf_network->audio_queue,pJobEntry );
					
				}				
				else // liveauido fd error, pass
				{
					g_warning("%s cs[%d]ds[%d] liveauido Err fd[%d] rx_en[%d]",
								__FUNCTION__, pClient->cs, pClient->ds, 
								gLive_info.liveaudio_fd, is_rx_audio );
				}
			}
			else
				goto __error;

		}
		else if( ret == 0 )	// timeout 
		{
#ifdef	ENABLE_CMS_TIMEOUT_SKIP

			// choissi  CMS keepalive timeout delay
			if( prev_frame_count == 0 )
			{
				prev_frame_count = pClient->play_frame_count;
				g_message("%s cs[%d]ds[%d] timeout delay1 frame_count[%d]", __FUNCTION__, 
						pClient->cs, pClient->ds, pClient->play_frame_count);
																		
			}else{
																
				if(prev_frame_count == pClient->play_frame_count)
				{
					// select time out���� ��Ŷ�� �ȿö� ����
					// client�� ���� �ɷ� �����Ѵ�. release_client ���� ^>^ 	
					g_message("%s cs[%d]ds[%d] ClientThread timeout", __FUNCTION__, pClient->cs, pClient->ds);
					goto __error;					
				}else{
					prev_frame_count = pClient->play_frame_count;
					g_message("%s cs[%d]ds[%d] timeout delay2 frame_count[%d]", __FUNCTION__, 
						pClient->cs, pClient->ds, pClient->play_frame_count);					
				}
				
			}							
#else
			g_message("%s cs[%d]ds[%d] ClientThread Timeout", 
						__FUNCTION__, pClient->cs, pClient->ds);
			goto __error;
#endif			
		}
		else
			goto __error; // select error
		
	}

__error:
	{
		int ds = pClient->ds, cs = pClient->cs;
		int uid = pClient->uniqueid;
		
		//SendEventLogCmd( LT_REMOTE_LOG_OFF, 1, LP2_REMOTE_LOG_OFF_NR, pClient->userid);
		//BF23 "REMOTE LOG OFF: LIVE DISPLAY
		_client_eventlog_put(pClient, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_LOCAL_LOG_OFF_LIVE_DISPLAY, NULL);
	
		_client_release(pClient);

#ifdef DEBUG_NETSVR_LOG		
		g_message("%s cs[%d]ds[%d] uid[%d] ClientThread End!!", __FUNCTION__, cs, ds, uid);				
#endif		
	}
	pthread_exit(NULL);
}

#define MAX_CONV_BUFF	800

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void rtp_recv_audio_play(unsigned char *d, int read_size)
{

  char buff_conv[MAX_CONV_BUFF*4];
  JOB_INFO  *pJobEntry = NULL;
  unsigned int buff_int = 0;
  int buff_index;
  
  //g_printf("rtp_recv_audio_play: read size [%d]\n",read_size);
  
  if( read_size> MAX_CONV_BUFF )
  {
	g_warning("%s audio overflow [%d]", __FUNCTION__, read_size);
	read_size = MAX_CONV_BUFF;
  }
    	  	
  if(read_size > 0)
  {
    int i;
    gint is_rx_audio = (gint)nf_sysdb_get_bool("audio.rx");

//    g_printf("rtp_recv_audio_play: is_rx_audio [%d]\n",is_rx_audio);    
#ifdef DEBUG_NETSVR_LOG
  if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_AUDIO] )
    HexDump( d, 128, 0);
#endif
        
    if( gLive_info.liveaudio_fd > 0 && is_rx_audio 
    	&& g_async_queue_length (_nf_network->audio_queue) < 64 )
    {
#if 0
      for(i=0;i<read_size && ;i++)
      {
        buff_int = _ulaw2short[d[i]];

        buff_index = i * 4;
        buff_conv[buff_index]     = 0;
        buff_conv[buff_index+1]   = 0;
        buff_conv[buff_index+2]   = buff_int;
        buff_conv[buff_index+3]   = (buff_int >> 8 );
      }      
      pJobEntry = _client_new_job(NULL, 0, buff_conv, read_size*4);
#else 	  
	  pJobEntry = _client_new_job(NULL, 0, d, read_size);
#endif		
      if(pJobEntry) 
			g_async_queue_push( _nf_network->audio_queue, pJobEntry );      
    }       
    else // liveauido fd error, pass
    {
  //    g_warning("%s cs[%d]ds[%d] liveauido Err fd[%d] rx_en[%d]",
  //          __FUNCTION__, pClient->cs, pClient->ds, 
  //          gLive_info.liveaudio_fd, is_rx_audio );
  
  		if( g_async_queue_length(_nf_network->audio_queue) >= 64 ) {
  			g_warning("%s audio failed fd[%d]on[%d]que_len[%d]",__FUNCTION__, 
  					gLive_info.liveaudio_fd, is_rx_audio,
  					g_async_queue_length (_nf_network->audio_queue) ) ;
  		}
    }
  }
  
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _client_process_rccmd(CLIENT_INFO *info, vpacket_t *req_vp, char *req_buff)
{
	int res = 0;

	if(gServer_info.state != NETSVR_STAT_RUN) {
		return -1;
	}
	
	switch(req_vp->code) {
		case VPE_SUCCESS:
			//fprintf(stderr,"%s : VPE_SUCCESS\n",__FUNCTION__);
			break;
		case VPE_PROTOCOL:
			//fprintf(stderr,"%s : VPE_PROTOCOL\n",__FUNCTION__);
			break;
		case VPE_UNKNOWNREQUEST:
			//fprintf(stderr,"%s : VPE_UNKNOWNREQUEST\n",__FUNCTION__);
			break;
		case VPE_FAULT:
			//fprintf(stderr,"%s : VPE_FAULT\n",__FUNCTION__);
			break;
		case VPE_FAIL:
			//fprintf(stderr,"%s : VPE_FAIL\n",__FUNCTION__);
			break;
		case VPE_INTERNAL:
			fprintf(stderr,"%s : VPE_INTERNAL\n",__FUNCTION__);
			break;
		case VPE_USER:
			fprintf(stderr,"%s : VPE_USER\n",__FUNCTION__);
			break;
			
		case DR_GET_CURRENTTIME:
			res = _dr_get_currenttime(info, req_vp, req_buff);
			break;
		case DR_GET_SETUP: 
			res = _dr_get_setup(info, req_vp, req_buff);
			break; 
		case DR_SET_SETUP:
			res = _dr_set_setup(info, req_vp, req_buff);
			break;
		case DR_RELEASE_SETUP:
			res = _dr_release_setup(info, req_vp, req_buff);
			break;
			
		case DR_START_LIVE:
			res = _dr_start_live(info, req_vp, req_buff);
			break;
		case DR_STOP_LIVE:
			res = _dr_stop_live(info, req_vp, req_buff);
			break;
		case DR_GET_CAMERA_TITLE:
			res = _dr_get_camera_title(info, req_vp, req_buff);
			break;
		case DR_GET_SYSINFO:
			res = _dr_get_sysinfo(info, req_vp, req_buff);
			break;
		case DR_GET_COVERT_STATUS:
			res = _dr_get_covert_status(info, req_vp, req_buff);
			break;
		case DR_GET_PTZ_STATUS:
			res = _dr_get_ptz_status(info, req_vp, req_buff);
			break;
		case DR_GET_ALARM_STATUS:
			res = _dr_get_alarm_status(info, req_vp, req_buff);
			break;
		case DR_ALARM_CONTROL:
			res = _dr_alarm_control(info, req_vp, req_buff);
			break;
		case DR_GET_HDD_SIZE:
			res = _dr_get_hdd_size(info, req_vp, req_buff);
			break;
		case DR_GET_TIMEZONE:
			res = _dr_get_timezone(info, req_vp, req_buff);
			break;
		case DR_GET_DATETIME_FORMAT:
			res = _dr_get_datetime_format(info, req_vp, req_buff);
			break;
		case DR_GET_CAMERA_NOVIDEO_STATUS:
			res = _dr_get_camera_novideo_status(info, req_vp, req_buff);
			break;
		case DR_PTZ:
			res = _dr_ptz(info, req_vp, req_buff);
			break;			

		case DR_TIMELINE:
			res = _dr_timeline(info, req_vp, req_buff);
			break;			
		case DR_GET_LOG:
			res = _dr_get_log(info, req_vp, req_buff);
			break;

		case DR_START_PLAY:
			res = _dr_start_play(info, req_vp, req_buff);
			break;
		case DR_STOP_PLAY:
			res = _dr_stop_play(info, req_vp, req_buff);
			break;
			
		case DR_START_BACKUP:
			res = _dr_start_backup(info, req_vp, req_buff);
			break;
		case DR_STOP_BACKUP:
			res = _dr_stop_backup(info, req_vp, req_buff);
			break;

		default:
#if 0
			res = _dr_get_fwversion                   (info, req_vp, req_buff);             
			res = _dr_get_sysinfo                     (info, req_vp, req_buff);             
			res = _dr_get_recinfo                     (info, req_vp, req_buff);             
			res = _dr_get_accountinfo                 (info, req_vp, req_buff);             

			res = _dr_set_eventmask                   (info, req_vp, req_buff);             
			res = _dr_set_currenttime                 (info, req_vp, req_buff);             
			res = _dr_system_function                 (info, req_vp, req_buff);             
			res = _dr_audio_mute                      (info, req_vp, req_buff);

			g_message("%s cs[%d]ds[%d] [%x][0x%02x][%s]/[%d] NO_IMPL!!", 
						__FUNCTION__, info->cs, info->ds, 
						req_vp->type, req_vp->code, _str_dr_proto( req_vp->code ),
						req_vp->dlen );		
#endif
			res = NETSVR_RET_ERR_NO_IMPL;					
			break;
	}
	
	return res;
}
 
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _client_release(CLIENT_INFO *pClient)
{	
	int wait = 0;
	
	if (pClient == NULL)
		return -1;
	
	if(pClient->magic_key != CLIENT_MAGIC) 
		return -2;
		
	g_message("%s cs[%d]ds[%d] entry[0x%08x]  auth[0x%04x] mode[%c]", 
			__FUNCTION__, pClient->cs, pClient->ds, pClient, 
			pClient->auth, pClient->mode );	
	
	Pthread_mutex_lock(&pClient->mutex);
	pClient->magic_key = 0x12340000;
	
	g_message("%s close cs fd[%d]",__FUNCTION__, pClient->cs);
	if( pClient->cs > 0) 
	{
		Close(pClient->cs); pClient->cs = -1;
	}
	g_message("%s close ds fd[%d]",__FUNCTION__, pClient->ds);
	if(	pClient->ds > 0)
	{
		Close(pClient->ds); pClient->ds = -1;
	}	
	Pthread_mutex_unlock(&pClient->mutex);

	_client_set_mode( pClient, CLIENT_MODE_CLOSE );	
	
	// setup
	if(pClient->setup_mode != 0)
	{
		pClient->setup_mode = 0;	
		//sendto_sm_release_setup( pClient );
	}

#define CLOSE_WAIT_SEC	15

	g_message("%s wait close socket [%d]sec",__FUNCTION__, CLOSE_WAIT_SEC);
	sleep(CLOSE_WAIT_SEC); 
	
	while( 1 )
	{				
		if( !pthread_mutex_trylock(&pClient->ds_mutex) || wait == 16 ) {
			break;
		}else{
			g_message("%s wait stream free uid[%d] mode[%c] [%d]sec",__FUNCTION__, 
					pClient->uniqueid, pClient->mode, ++wait );
			g_usleep(1000000);
		}
	}
	g_message("%s stream free ok!! uid[%d] ", __FUNCTION__, pClient->uniqueid);
	// client information update.
	Pthread_mutex_lock(&gServer_info.client_mutex);
	{
		TAILQ_REMOVE( &gServer_info.client_head, pClient, entries);
		
		--gServer_info.client_count;
		++gServer_info.disconn_count;

		//gServer_info.ptr_client_arr[pClient->playid] = NULL;
		
		g_message("%s CLIENT count[%d] disconn[%d]",__FUNCTION__, 
					gServer_info.client_count,
					gServer_info.disconn_count);
					
#ifdef DEBUG_NETSVR_LOG
//		_netsvr_print_client_entry(&gServer_info);
#endif
	}	
	Pthread_mutex_unlock(&gServer_info.client_mutex);
		
	free(pClient);
	return 0;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _client_csds_read(CLIENT_INFO *pClient, int timeout_sec)
{	
	fd_set rxset, wkset;
	int mx, n;
	struct timeval tv;

	FD_ZERO(&rxset); FD_ZERO(&wkset);
	
	FD_SET(pClient->cs, &rxset);
	FD_SET(pClient->ds, &rxset);
		
	mx = gServer_info.conn_fd + 1;
	
	if( pClient->cs > pClient->ds)
		mx = pClient->cs;
	else
		mx = pClient->ds;
		
	tv.tv_sec = timeout_sec;  // timeout value set;	
	tv.tv_usec = 0;
	
	while(1) 
	{
		memcpy( &wkset, &rxset, sizeof(fd_set));
		
		n = Select(mx + 1, &wkset, 0, 0, &tv);
		if(n == -1)
			return -1;
		else if(n == 0) // timeout!!;
			break;

		if(FD_ISSET(pClient->cs, &wkset))
			return 1;
		else if(FD_ISSET(pClient->ds, &wkset))
			return 2;
	}	
	return 0;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
unsigned char _client_get_mode(CLIENT_INFO *pClient)
{
	unsigned char ret = 0;
	
	if( pClient == NULL)	return 0;
	if( pClient->magic_key != CLIENT_MAGIC) return 0;
				
	Pthread_mutex_lock(&pClient->mutex);	
	ret = pClient->mode;	
	Pthread_mutex_unlock(&pClient->mutex);	

	return ret;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int _client_broadcast_msg(char *msg, int size)
{
	int ret=0, cnt=0;	
	CLIENT_INFO				*pClient = NULL;	
	vpacket_t 		*vp = (vpacket_t *)msg;
	
	g_return_val_if_fail( msg, 0);
	
	// ��� Ŭ���̾�Ʈ����~;;
	pClient = NULL;
	while(1)
	{										
		Pthread_mutex_lock(&gServer_info.client_mutex);
		{	// get client_info
			if(pClient == NULL)
				pClient = TAILQ_FIRST( &gServer_info.client_head );
			else
				pClient = TAILQ_NEXT(pClient, entries);
		}	
		Pthread_mutex_unlock(&gServer_info.client_mutex);
					
		if(pClient == NULL) break; 	
		if(pClient->magic_key != CLIENT_MAGIC) continue;

		if( pthread_mutex_trylock(&pClient->cs_mutex) )
		{
			g_message("%s uid[%3d] fd[%3d] trylock fail code[%2x][%s]", 
				__FUNCTION__, pClient->uniqueid, pClient->cs,
				vp->code, _str_dr_proto(vp->code) );
				
			continue;
		}
		ret = Send( pClient->cs, msg , size, MSG_DONTWAIT);

		if ( ret == -1 )
		{
			g_warning("%s Send error ret[%d]", __FUNCTION__, ret);
		}
		
		Pthread_mutex_unlock(&pClient->cs_mutex);
		
#ifdef DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CLIENT_BCAST] )
			g_message("%s uid[%3d] fd[%3d] code[%2x][%s]len[%d]  ret[%d]", __FUNCTION__,
							pClient->uniqueid, pClient->cs,
							vp->code, _str_dr_proto(vp->code), 
							ntohs(vp->dlen), ret);
#endif
		++cnt;
				
	} //while(1)	
	
	return cnt;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int _client_eventlog_put(CLIENT_INFO *pClient, gint type, gint param1, gint param2, gchar *text)
{
	char buff[256];
	gboolean  ret = 0; 
	
	g_return_val_if_fail( pClient != NULL, -1);
	
	if(text == NULL){
		snprintf( buff, sizeof(buff), "%s %s", 
				pClient->userid,
				inet_ntoa(pClient->peer_addr.sin_addr) );	
	}else{
		snprintf( buff, sizeof(buff), "%s %s %s", 
				pClient->userid,
				inet_ntoa(pClient->peer_addr.sin_addr), text);
	}
								
	ret = nf_eventlog_put_param( NULL, type, param1, param2, buff);
				
	return (ret == 1) ? 0:-2;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void _client_set_mode(CLIENT_INFO *pClient, unsigned char mode)
{
	if( pClient == NULL)	return;
	if( pClient->magic_key != CLIENT_MAGIC) return;
				
	Pthread_mutex_lock(&pClient->mutex);	
	pClient->previous_mode = pClient->mode = mode;
	pClient->mode = mode;	
	Pthread_mutex_unlock(&pClient->mutex);
	return;
}

void _client_stat_sock_buff(CLIENT_INFO *pClient)
{// SO_SNDBUF, SIOCOUTQ
	int len, err, sndbufsiz, used, ds;
	
	if(pClient == NULL) return;
	if(pClient->magic_key != CLIENT_MAGIC) return;
	
	ds = pClient->ds;
	len = sizeof(sndbufsiz);
	err = Getsockopt(ds, SOL_SOCKET, SO_SNDBUF, &sndbufsiz, &len);
	if(err < 0) {
		sndbufsiz = 0;
	}
	err = Ioctl(ds, SIOCOUTQ, &used);
	if(err < 0) {
		used = sndbufsiz;
	}

	if(pClient == NULL) return;
	if(pClient->magic_key != CLIENT_MAGIC) return;
														
	pClient->sndbuff_size = sndbufsiz;
	pClient->sndbuff_used = used;

	if( pClient->shadow_sndbuff_used != used)
	{		
		pClient->shadow_sndbuff_used = used;
		pClient->shadow_count = 0;
	}else
		++pClient->shadow_count;

	return;	
}

int _send_controlframe(int ds, int type)
{
	int ret, size;
	ICODEC_HEADER ifh;

	memset(&ifh, 0x00, sizeof(ICODEC_HEADER));
	
	ifh.frame_type = type;
	size = sizeof(ICODEC_HEADER);
	
	if( (ret = Writen(ds, &ifh, size) ) != size  ) 
	{
		err_msg("%s Writen fd[%d] ret[%d] size[%d]", __FUNCTION__, ds, ret, size);
		return -1;
	}

	return 0;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int _sock_set_timeout(int sd, unsigned int sec) 
{
    struct timeval rcv, snd;
    if (sd < 0) return -1;

    rcv.tv_sec = snd.tv_sec = sec;
    rcv.tv_usec = snd.tv_usec = 0;
    
/*
    if (Getsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &snd, &len )==-1) 
    {        
        return -1;
    }
    g_message("%s Getsockopt fd[%d] SO_RCVTIMEO sec[%d] usec[%d]", __FUNCTION__, sd, 
    				snd.tv_sec, snd.tv_usec );
 */  
    if (Setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &rcv, sizeof(rcv))==-1) 
    {        
        return -1;
    }
/*
    if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &snd, sizeof(snd))==-1) 
   	{        
        return -1;
    }
*/
    return 0;
}
  
 /******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void _netsvr_enque_client_entry( SERVER_INFO *pServer_info, CLIENT_INFO *pEntry)
{	
	if( pServer_info == NULL || pEntry == NULL)
		return;

#ifdef DEBUG_NETSVR_LOG
	g_message("%s tailq_insert_tail [0x%08x]", __FUNCTION__, pEntry);
#endif

	TAILQ_INSERT_TAIL( &pServer_info->client_head, pEntry, entries);
	++pServer_info->client_count;
						
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void _netsvr_print_client_entry( SERVER_INFO *pServer_info)
{	
	CLIENT_INFO *pEntry = NULL;
	int i=0;
	
	if( pServer_info == NULL )
		return;
	
	g_message("%s cnt[%02d] ------------------------------------------", __FUNCTION__, 
			pServer_info->client_count );
	TAILQ_FOREACH( pEntry, &pServer_info->client_head, entries) 
	{
		g_message("%s [%02d] [0x%08x] IP[%s]PORT[%d] cs[%d]ds[%d] auth[0x%04x] mode[%c] uid[%d][%d]", 
			__FUNCTION__, i++, pEntry, 
			inet_ntoa(pEntry->peer_addr.sin_addr), ntohs(pEntry->peer_addr.sin_port), 
			pEntry->cs, pEntry->ds, 
			pEntry->auth, pEntry->mode,  pEntry->uniqueid, pEntry->setup_mode);
	}			
	g_message("%s --------------------------------------------------", __FUNCTION__ );
}



/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
CLIENT_INFO *_netsvr_find_client_setup( SERVER_INFO *pServer_info)
{	
	CLIENT_INFO *pEntry = NULL;
	int i=0;
		
	g_return_val_if_fail(  pServer_info != NULL, NULL);
		
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_FIND_CLIENT_SETUP ] )
		g_message("%s cnt[%02d] ------------------------------------------", 
					__FUNCTION__, pServer_info->client_count );
#endif	

	Pthread_mutex_lock(&gServer_info.client_mutex);	
	TAILQ_FOREACH( pEntry, &pServer_info->client_head, entries) 
	{

#ifdef DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_FIND_CLIENT_SETUP ] )
		{
			g_message("%s [%02d] [0x%08x] IP[%s]PORT[%d] cs[%d]ds[%d] auth[0x%04x] mode[%c] uid[%d][%d]", 
				__FUNCTION__, i++, pEntry, 
				inet_ntoa(pEntry->peer_addr.sin_addr), ntohs(pEntry->peer_addr.sin_port), 
				pEntry->cs, pEntry->ds, 
				pEntry->auth, pEntry->mode,  pEntry->uniqueid, pEntry->setup_mode);
		}		
#endif	
		if( pEntry->setup_mode )
		{
			Pthread_mutex_unlock(&gServer_info.client_mutex);	
			return pEntry;
		}
	}
	Pthread_mutex_unlock(&gServer_info.client_mutex);	
		
#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_FIND_CLIENT_SETUP ] )
		g_message("%s --------------------------------------------------", __FUNCTION__ );
#endif

	return NULL;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
char *my_inet_ntoa_r(struct in_addr ina, char *buf, int buf_len)
{
	g_return_val_if_fail( buf != NULL, NULL);
	g_return_val_if_fail( buf_len > 16, NULL);
	
	unsigned char *ucp = (unsigned char *)&ina;

	snprintf(buf, buf_len-1, "%d.%d.%d.%d",
		ucp[0] & 0xff,
		ucp[1] & 0xff,
		ucp[2] & 0xff,
		ucp[3] & 0xff);
				
	return buf;
}

char *my_inet_ntoa_r2(char *ucp, char *buf, int buf_len)
{
	g_return_val_if_fail( buf != NULL, NULL);
	g_return_val_if_fail( buf_len > 16, NULL);
		
	snprintf(buf, buf_len-1, "%d.%d.%d.%d",
		ucp[3] & 0xff,
		ucp[2] & 0xff,
		ucp[1] & 0xff,
		ucp[0] & 0xff);
				
	return buf;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int my_gethostbyname(const char *host, struct sockaddr_in *ret_addr)
{
	int ret = 0;
	char buff[256];	
	
	g_return_val_if_fail( host != NULL , -1);
	g_return_val_if_fail( ret_addr != NULL , -1);
			
	ret = inet_pton(AF_INET, host, &ret_addr->sin_addr);
	if(ret > 0 )
		goto ret_ok;	

	ret = my_getaddrinfo( host, ret_addr, 1);		
	if( ret != 1 )
	{
		g_warning("%s fail[%s]",__FUNCTION__, host);
		return -1;
	}
			
ret_ok:			

#ifdef DEBUG_NETSVR_LOGxx
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CONNECT_API] )		
	{
	memset(buff, 0x00, sizeof(buff));
	inet_ntop( AF_INET, &ret_addr->sin_addr, buff, sizeof(buff));
	g_message("%s : host[%s] ip[%s]", __FUNCTION__, host, buff);
	}
#endif 

	return 1;
}



/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int my_dns_gethostbyname(const char *host, char *ip_addr, int char_len)
{
	int ret = 0;
	struct sockaddr_in addr;							

	
	g_return_val_if_fail( host != NULL , -1);
	g_return_val_if_fail( ip_addr != NULL , -1);

	bzero(&addr, sizeof(struct sockaddr_in));
			
	ret = inet_pton(AF_INET, host, &addr.sin_addr);
	if(ret > 0 )
		goto ret_ok;	

	ret = my_getaddrinfo( host, &addr, 1);		
	if( ret != 1 )
	{
		g_warning("%s fail[%s]",__FUNCTION__, host);
		return -1;
	}
			
ret_ok:			

	inet_ntop( AF_INET, &addr.sin_addr, ip_addr, char_len);
	g_message("%s : host[%s] ip[%s]", __FUNCTION__, host, ip_addr);

	return 1;
}



/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int my_getaddrinfo(const char *host, struct sockaddr_in *ret_addr, int dump_all)
{
	int				ret;
	char			*serv = "domain";
	struct addrinfo	hints, *res_hints = NULL, *tmp_hints = NULL;
	struct sockaddr_in *addr = NULL;	// ipv4 16bytes
	
	g_assert(host);
	g_assert(ret_addr);
	
	bzero(&hints, sizeof(struct addrinfo));

	hints.ai_family =  AF_INET; // AF_UNSPEC;  AF_INET,AF_INET6
	hints.ai_socktype = SOCK_STREAM;

#ifdef DEBUG_NETSVR_LOGxx
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CONNECT_API] )		
	g_message("%s : host[%s] ret_addr[%x]", __FUNCTION__, host, ret_addr );	
#endif		

	if ( (ret = getaddrinfo(host, NULL, &hints, &res_hints)) != 0)
	{
		g_warning("%s : error host[%s] [%s](%d)", __FUNCTION__, host, gai_strerror(ret), ret);
		return -1;
	}		

#ifdef DEBUG_NETSVR_LOGxx
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CONNECT_API] )		
	g_message("%s : res_hints->ai_addr [%p][%p]", __FUNCTION__, res_hints, res_hints->ai_addr );
#endif
		
	addr = (struct sockaddr_in *)res_hints->ai_addr;

	g_assert(addr);
	
	memcpy(ret_addr, addr, sizeof(struct sockaddr_in));
		
	tmp_hints = res_hints;
	
	if(dump_all) {					
		char buff[256];									
		
		do {
			memset(buff, 0x00, sizeof(buff));

			addr = (struct sockaddr_in *)tmp_hints->ai_addr;		
			g_assert(addr);
			inet_ntop( AF_INET, &addr->sin_addr, buff, sizeof(buff));

			g_message("%s : hints [%d][%d][%d][%d] [%s](%d)", __FUNCTION__, 
				tmp_hints->ai_flags, tmp_hints->ai_family, tmp_hints->ai_socktype,
				tmp_hints->ai_protocol,	buff,tmp_hints->ai_addrlen);	

		} while ( (tmp_hints = tmp_hints->ai_next) != NULL);
		
	}
	
	freeaddrinfo(res_hints);
	return 1;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int connect_timeout(const SA *saptr, socklen_t salen, int usec)
{
	int				flags, n, ret, try_cnt = 1;
	int				sockfd;
	fd_set 			wset;
	struct timeval	tval;
	
	
	// socket create		
	sockfd = Socket(AF_INET, SOCK_STREAM, 0); 
	if(sockfd < 0)
		return	(-1);
			
	if( usec != 0 )	// if usec == 0, blocking mode
	{	
		flags = Fcntl(sockfd, F_GETFL, 0);
		Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);		
	}

	if(usec > 250000) {	// 0.250 sec		
		tval.tv_sec = usec/1000000;
		tval.tv_usec = usec%1000000;		
	} else{
		tval.tv_sec = 0;
		tval.tv_usec = 250000;
	}
		
	if ( (n = connect(sockfd, (struct sockaddr *)saptr, salen)) < 0)
	{	
		if ( !(errno == EINPROGRESS || errno == EALREADY ) )
		{
			//#define EALREADY	114 /* Operation already in progress */
			//#define EINPROGRESS 115 /* Operation now in progress */				
			g_warning("%s : connect errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
			ret = -2;  goto error;
		}

		FD_ZERO (&wset);
		FD_SET (sockfd, &wset); 
		
		n = select ( sockfd+1, NULL, &wset, NULL, &tval);
		if( n < 0) {
			g_warning("%s : select errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
			ret = -2; goto error;
		}else if (n){
			ret = 1; goto done;  // connect ok
		}
						
	}else{
		ret = 1;  goto done;
	}				
	
	ret = -3; //timeout	
	
error:
//	g_message("%s %d !!!!!!!!!!! error",__FUNCTION__, __LINE__);		
	Close(sockfd);
	return -1;
		
done:	
//	g_message("%s done!! fd[%d]",__FUNCTION__, sockfd);		
	if(usec != 0)
		Fcntl(sockfd, F_SETFL, flags);
		
	return sockfd;	// ok
} 


int connect_timeout_hostname(const char *host, const int port, const int usec)
{
	int ret;		
	struct sockaddr_in addr;
	
	bzero(&addr, sizeof(struct sockaddr_in));

#ifdef DEBUG_NETSVR_LOG
	if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CONNECT_API] )		
		g_message("%s : host[%s] port[%d] usec[%d]", __FUNCTION__, host, (int)port, usec);
#endif	

	ret =  my_gethostbyname( host, &addr);	
	if(ret != 1) 
	{
		g_warning("%s : my_gethostbyname host[%s] ret[%d]",  __FUNCTION__,host, ret);	
		return -1;		
	}else{

#ifdef DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CONNECT_API] )		
			g_message("%s : my_gethostbyname ip[%s]",  __FUNCTION__, inet_ntoa(addr.sin_addr) );
#endif

	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	ret = connect_timeout( (SA *)&addr, sizeof(struct sockaddr_in), usec ); 
	if(ret < 0 )
	{
		g_warning("%s : connect_timeout host[%s][%d] ret[%d]", __FUNCTION__, 
					host, port, ret);
		return -2;
	}

#if 0//def DEBUG_NETSVR_LOG
		if( _DEBUG_NETSVR_log[DEBUG_NETSVR_IDX_CONNECT_API] )		
			g_message("%s : connect_timeout fd[%d]",  __FUNCTION__, ret );
#endif
	  	
	return ret; // fd
}


#ifdef DEBUG_NETSVR_JBSHELL

static char net_clist_help[] = "net_clist";
static int net_clist(int argc, char **argv)
{	

	g_return_val_if_fail( _nf_network != NULL, -1 );

	Pthread_mutex_lock(&gServer_info.client_mutex);
	{				
		_netsvr_print_client_entry(&gServer_info);
	}	
	Pthread_mutex_unlock(&gServer_info.client_mutex);
						
	return 0;
}
__commandlist(net_clist,"net_clist", net_clist_help, net_clist_help);


static char net_start_help[] = "net_start";
static int net_start(int argc, char **argv)
{	
	g_return_val_if_fail( _nf_network != NULL, -1 );
	
	printf("nf_network_start ret[%d]\n", nf_network_start() );
	
	return 0;
}
__commandlist(net_start,"net_start", net_start_help, net_start_help);

static char net_stop_help[] = "net_stop [reason]";
static int net_stop(int argc, char **argv)
{	
	gint reason = 0;
	
	g_return_val_if_fail( _nf_network != NULL, -1 );
	
	if(argc < 2){
		printf("%s\n",net_stop_help);
		printf("SVR_IP_CHANGE	= 0x01\n");
		printf("SVR_DISK_FORMAT	= 0x02\n");
		printf("SVR_POWER_OFF	= 0x03\n");
		printf("SVR_TIME_CHANGE	= 0x04\n");
		return -1;
	}		
		
	reason = strtoul(argv[1],NULL,0);
			
	if     ( reason == 1) reason = 0x00010001;
	else if( reason == 2) reason = 0x00010002;
	else if( reason == 3) reason = 0x00010003;
	else if( reason == 4) reason = 0x00010004;
	else return 0;
		
	printf("nf_network_stop(0x%08x) ret[%d]\n", reason, nf_network_stop(reason) );
			
	return 0;
}
__commandlist(net_stop,"net_stop", net_stop_help, net_stop_help);

typedef struct _PORT_TEST_ARG_T
{
	char host[256];
	gint port;
	gint cnt;		
} PORT_TEST_ARG;

static void
_port_test_thread_func (PORT_TEST_ARG *arg)
{	
	int i;	
	PORT_TEST_ARG larg;
	
	memcpy( &larg, arg, sizeof( PORT_TEST_ARG ) );
	
	g_message("%s start", __FUNCTION__);
	for(i=0;i<larg.cnt;i++)
	{
		port_test( larg.host , (unsigned short)larg.port );
	}
	g_message("%s end", __FUNCTION__);
}

static char net_port_test_help[] = "net_port_test [domain:127.0.0.1] [port:clientport] [cnt:1] [thread:1]";
static int net_port_test(int argc, char **argv)
{	
	char *host= "127.0.0.1";
	gint port = nf_sysdb_get_uint("net.proto.clientport");
	gint cnt = 16, t_cnt = 1;
	gint i;

	PORT_TEST_ARG		arg;
			
	if(argc>1) host = argv[1];				
	if(argc>2) port = strtoul(argv[2],NULL,0);	
	if(argc>3) cnt = strtoul(argv[3],NULL,0);
	if(argc>4) t_cnt = strtoul(argv[4],NULL,0);
		
	memset(&arg, 0x00, sizeof(arg));
	
	strncpy ( arg.host, host, sizeof(arg.host));	
	arg.port = port;
	arg.cnt = cnt;
		
	for(i=0;i<t_cnt;i++)
	{	
		GThread *tid = g_thread_create((GThreadFunc)_port_test_thread_func, 
							&arg, FALSE, NULL);
							
		g_print("g_thread_create [%d][%p]\n",i, tid);
	}
	
	sleep(1);
			
	return 0;
}
__commandlist(net_port_test,"net_port_test", net_port_test_help, net_port_test_help);
// net_port_test 00115ff0029b.dvrlink.net 6400 128 16
// net_port_test 192.168.100.73 6400 128 4


static char net_ddns_dump_help[] = "net_ddns_dump";
static int net_ddns_dump(int argc, char **argv)
{	
	NF_DDNS_STATUS  tmp;
	
	g_return_val_if_fail( _nf_network != NULL, -1 );
	
	ddns_get_status(&tmp);
	ddns_dump_status(&tmp);
		
	return 0;
}
__commandlist(net_ddns_dump,"net_ddns_dump", net_ddns_dump_help, net_ddns_dump_help);

static char net_ddns_req_help[] = "net_ddns_req";
static int net_ddns_req(int argc, char **argv)
{	
	NF_DDNS_STATUS  tmp;
	
	g_return_val_if_fail( _nf_network != NULL, -1 );	
	ddns_force_register();
	
	return 0;
}
__commandlist(net_ddns_req,"net_ddns_req", net_ddns_req_help, net_ddns_req_help);


extern int _s1_send_ddns_packet(void);
extern int _s1_query_ddns_packet( char *str_ipaddr, int str_len );
extern int _s1_query_ddns_packet_for_test( char *str_ipaddr, int str_len, char *mac_addr );

static char net_ddns_s1_reg_help[] = "net_ddns_s1_reg";
static int net_ddns_s1_reg(int argc, char **argv)
{	

	_s1_send_ddns_packet();
	return 0;
}
__commandlist(net_ddns_s1_reg,"net_ddns_s1_reg", net_ddns_s1_reg_help, net_ddns_s1_reg_help);


static char net_ddns_s1_query_help[] = "net_ddns_s1_query";
static int net_ddns_s1_query(int argc, char **argv)
{	
	char buff[128];
	_s1_query_ddns_packet( buff, sizeof(buff) );
	return 0;
}
__commandlist(net_ddns_s1_query,"net_ddns_s1_query", net_ddns_s1_query_help, net_ddns_s1_query_help);


static char net_ddns_s1_query_mac_help[] = "net_ddns_s1_query_mac [mac]";
static int net_ddns_s1_query_mac(int argc, char **argv)
{	
	char buff[128];
	
	if(argc < 2){
		printf("%s\n",net_ddns_s1_query_mac_help);
		return -1;
	}							
	_s1_query_ddns_packet_for_test( buff, sizeof(buff), argv[1] );
	return 0;
}
__commandlist(net_ddns_s1_query_mac,"net_ddns_s1_query_mac", net_ddns_s1_query_mac_help, net_ddns_s1_query_mac_help);



static char net_ddns_test_help[] = "net_ddns_test [server] [host] [user] [nick]";
static int net_ddns_test(int argc, char **argv)
{	
	char *ddns_server = "dvrlink.net";
	char *hostname = "testhost";
	char *username = "testuser";
	char *passwd = "testpass";
	int ret;
		
	NF_DDNS_COMMON_REG_PARAM param;
	
	if(argc < 2){
		printf("%s\n",net_ddns_test_help);
		return -1;
	}							

	if( argc>1 ) ddns_server = argv[1];
	if( argc>2 ) hostname = argv[2];
	if( argc>3 ) username = argv[3];
	if( argc>4 ) passwd = argv[4];
	
	memset( &param, 0x00, sizeof(NF_DDNS_COMMON_REG_PARAM));

	strncpy( param.ddns_server, ddns_server, sizeof(param.ddns_server)-1);
	strncpy( param.hostname, hostname, sizeof(param.hostname)-1);
	strncpy( param.username, username, sizeof(param.username)-1);
	strncpy( param.passwd, passwd, sizeof(param.passwd)-1);
		
	ddns_common_force_register( &param);
	
	return 0;
}
__commandlist(net_ddns_test,"net_ddns_test", net_ddns_test_help, net_ddns_test_help);

#endif

// FIXME for IPX VE hw test firmware..
static void network_reset_func (void)
{
	gchar cmd[256] = {0,};

	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_network == NULL ) g_usleep(10*1000);

	sleep(3600); // 1hour

	while(1)
	{
		g_message("%s dtd start~~~~~!!!!!", __FUNCTION__);

		snprintf(cmd, sizeof(cmd), "ifconfig %s down", _eth_dev);
		proxy_system(cmd, 1, 3);

		g_usleep(1000 * 1000);

		snprintf(cmd, sizeof(cmd), "ifconfig %s up", _eth_dev);
		proxy_system(cmd, 1, 3);

		sleep(3600);	// 1hour 
	}
	g_message("%s end", __FUNCTION__);
}

/******************************************************************************/
// NETWORK SCENARIO START

typedef struct _CONFLICT_STATE{
	int conflict;
	NF_NETIF_MAC conflict_mac;
} CONFLICT_STATE;

typedef struct _NETWORK_STATE{
	CONFLICT_STATE cfl_stat[34];
	int seq;
} NETWORK_STATE;

static NETWORK_STATE net_stat;

static gboolean _dhcp_renew = FALSE;

static int _convert_ip_str_to_val(gchar *ip_str, NF_NETIF_IP *ip_val)
{	
	int i;
	char *ip_p;
	char ip_buf[100] = {0, };

	if(ip_str == NULL)
	{
		g_message("%s ERROR => ip_str is NULL", __FUNCTION__);		
		return -1;
	}

	if(ip_val == NULL)
	{
		g_message("%s ERROR => ip_val is NULL", __FUNCTION__);		
		return -1;
	}

	if( strlen(ip_str) < 7 )
	{		
		g_message("%s ERROR => ip_str:%s", __FUNCTION__, ip_str);
		return -1;
	}

	memset(ip_val, 0x0, sizeof(NF_NETIF_IP));

	if(inet_addr(ip_str) == 0xffffffff){
		nf_openmode_dns_lookup(ip_str, ip_buf);
		ip_p = ip_buf;
	}else{
		ip_p = ip_str;
	}

	for(i=0; i < 4; i++)
	{
		ip_val->ip_addr[i] = strtol(ip_p, &ip_p, 10);

		if(*ip_p == '.'){
			ip_p++;
		}
		else
		{
			if(i != 3)
			{
				g_message("%s ERROR => ip_str:%s", __FUNCTION__, ip_str);				
				return -1;
			}
		}
	}

	return 0;
}

static void _convert_ip_val_to_str(NF_NETIF_IP ip_val, gchar *ip_str, int ip_str_size)
{
	snprintf(ip_str, ip_str_size, "%d.%d.%d.%d", 
		ip_val.ip_addr[0] & 0xff, 
		ip_val.ip_addr[1] & 0xff, 
		ip_val.ip_addr[2] & 0xff, 
		ip_val.ip_addr[3] & 0xff);
}

static void _convert_mac_str_to_val(gchar *mac_str, NF_NETIF_MAC *mac_val)
{
	int i;
	char *mac_p;

	memset(mac_val, 0x0, sizeof(NF_NETIF_MAC));

	mac_p = mac_str;

	for(i=0; i < 6; i++)
	{
		mac_val->mac_addr[i] = strtol(mac_p, &mac_p, 16);

		if(*mac_p == ':'){
			mac_p++;
		}
	}
}

static void _convert_mac_val_to_str(NF_NETIF_MAC mac_val, gchar *mac_str, int mac_str_size)
{
	snprintf(mac_str, mac_str_size, "%02x:%02x:%02x:%02x:%02x:%02x",	
		mac_val.mac_addr[0] & 0xff,
		mac_val.mac_addr[1] & 0xff,
		mac_val.mac_addr[2] & 0xff,
		mac_val.mac_addr[3] & 0xff,
		mac_val.mac_addr[4] & 0xff,
		mac_val.mac_addr[5] & 0xff);
}

/*
static gboolean _dhcp_renew()
{
#if 0
	gboolean ret = FALSE;
	gchar  *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;

	g_message("%s - DHCP RENEW DO", __FUNCTION__);
	
	if(access(DHCP_PID_FILE, F_OK) == 0)
	{
		if (!g_file_get_contents (DHCP_PID_FILE, &contents, &length, &error))
		{	
			g_warning("%s\n", error->message);
			g_error_free (error);

			if(contents)
				g_free(contents);		
		}
		else
		{
			if(contents)
			{
				if( length > 0 )
				{
					int kill_ret;
					int pid = 0;
					gchar *ptr;

					pid = strtol( contents, &ptr, 10);

					if( (pid > 0) && (contents != ptr) )
					{
						kill_ret = kill(pid, SIGUSR1);
					//	kill -USR1 $(cat tmp/pids/mongrel.pid)

						if( kill_ret == 0 )
							ret = TRUE;
					}
				}

				g_free(contents);
			}
		}
	}
#else
	gint is_dhcp=0;
	
	is_dhcp = nf_sysdb_get_bool( "net.proto.dhcpon");

	if(is_dhcp)
		nf_netif_dhcp_restart();
#endif
	return ret;
}
*/

gboolean nf_network_dhcp_renew(void)
{
	g_message("%s - %d", __FUNCTION__, __LINE__);
	
	_dhcp_renew = TRUE;

	return TRUE;
}

#define DHCP_DECFG	"/tmp/dhcp.proc.decfg"
#define DHCP_RENEW	"/tmp/dhcp.proc.renew"

static void _network_dhcp_monitor_thread()
{
	char ip_str[64];
	int route_cnt = 0;
	
	while(1)
	{
		route_cnt++;
		
		if(access(DHCP_DECFG, F_OK) == 0)
		{	
		    g_message( "%s - DHCP DECONFIG", __FUNCTION__);
			
			remove(DHCP_DECFG);
			
			nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 0, LP2_NETWORK_DHCP_FAIL, NULL);	
		}

		if(access(DHCP_RENEW, F_OK) == 0)
		{
		    g_message( "%s - DHCP RENEW", __FUNCTION__);
			
			NF_NETIF_IP ret_ip;
			
			remove(DHCP_RENEW);

			nf_netif_get_ip(&ret_ip);

			snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ret_ip.ip_addr[0], ret_ip.ip_addr[1], ret_ip.ip_addr[2], ret_ip.ip_addr[3]);
			
			nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 1, LP2_NETWORK_DHCP_OK, ip_str);		
		}

		if( _dhcp_renew )
		{	
//		    g_message( "%s - DHCP FLAG SETTING", __FUNCTION__);
			
			_dhcp_renew = FALSE;
			nf_netif_dhcp_restart();
		}

		usleep( 100 * 1000 );
	}
}

static int _convert_macstr_to_netifmac(char *mac_str, NF_NETIF_MAC *ret_mac)
{
	gchar des[12];
	char macchar;
	int i;

	if( mac_str == NULL )
	{
		g_message("%s ERROR => mac_str is NULL", __FUNCTION__);		
		return -1;
	}
	if( ret_mac == NULL )
	{
		g_message("%s ERROR => ret_mac is NULL", __FUNCTION__);		
		return -1;
	}

	if( strlen(mac_str) != 12 )
	{
		g_message("%s ERROR => mac_str length is not 12, wrong mac : %s", __FUNCTION__, mac_str);
		return -1;
	}
	
	for(i = 0; i < 12; i++)
	{
		macchar = mac_str[i];

		if (macchar == 'a')
		{
			des[i] = 10;
		}
		else if (macchar == 'b')
		{
			des[i] = 11;
		}
		else if (macchar == 'c')
		{
			des[i] = 12;
		}
		else if (macchar == 'd')
		{
			des[i] = 13;
		}
		else if (macchar == 'e')
		{
			des[i] = 14;
		}
		else if (macchar == 'f')
		{
			des[i] = 15;
		}
		else
		{
			char temp[2] = {0,};
			temp[0] = macchar;
			temp[1] = '\0';
			des[i] = atoi(temp); //des[i] = atoi(&macchar); // overflow
		}
	}

	for (i = 0; i < 12; i+=2)
	{
		ret_mac->mac_addr[i/2] = 0xff & ((des[i] << 4) | des[i+1]);
	}	

/*	
	g_message("%s - MAC => %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__,
		ret_mac->mac_addr[0],ret_mac->mac_addr[1],ret_mac->mac_addr[2],ret_mac->mac_addr[3],ret_mac->mac_addr[4],ret_mac->mac_addr[5]);
*/

	return 0;
}

static int _get_cam_ip_mac_by_channel(int ch, NF_NETIF_IP *ret_ip, NF_NETIF_MAC *ret_mac)
{

#if 1
	gchar buff[256];
	gchar *db_ip;
	gchar *db_mac;

	snprintf( buff, sizeof(buff), "cam.logininfo.L%d.hostname", ch);

	db_ip = nf_sysdb_get_str_nocopy(buff);
	if (!db_ip || strlen(db_ip) < 7){
		return -1;
	}

	if( _convert_ip_str_to_val(db_ip, ret_ip) < 0 )
	{		
		g_warning("%s - _convert_ip_str_to_val failed for channel %d db_ip:%s len:%d", __FUNCTION__, ch, db_ip, strlen(db_ip));
		return -1;
	}

	snprintf( buff, sizeof(buff), "cam.logininfo.L%d.mac", ch);

	db_mac = nf_sysdb_get_str_nocopy(buff);
	
	if( _convert_macstr_to_netifmac(db_mac, ret_mac) < 0 )
	{
		g_warning("%s - _convert_macstr_to_netifmac failed for channel %d", __FUNCTION__, ch);
		return -1;
	}

	return 0;	
#else
	return -1;
#endif

}

static int _is_invalid_ip(NF_NETIF_IP *ip)
{
	if( (ip->ip_addr[0] & 0xff) == 0 || (ip->ip_addr[0] & 0x0) )
		return TRUE;
	else
		return FALSE;
}

#define CONFLICT_MARK_START	"Received"
//#define CONFLICT_MARK_END	"reply"

int _network_ip_cam_conflict_test(NF_NETIF_IP ip, NF_NETIF_MAC mac, NF_NETIF_MAC *conflict_mac)
{
	gchar  *contents = NULL;	
	GError *error = NULL;	
	gsize  length = 0;
	int ret = -1;

	gchar *mark_start;
//	gchar *mark_end;

	gchar cmd[1024];
	gchar tmp_file[256];

	gchar ip_str[32];

	_convert_ip_val_to_str(ip, ip_str, sizeof(ip_str));

	memset(conflict_mac, 0x0, sizeof(NF_NETIF_MAC));

	memset(cmd, 0x0, sizeof(cmd));
	memset(tmp_file, 0x0, sizeof(tmp_file));

	snprintf( tmp_file, sizeof(tmp_file), "/tmp/cam_ip_test.tmp");

	snprintf(cmd, sizeof(cmd), "arping %s -c1 -w1 -I %s > %s", ip_str, _eth_dev, tmp_file);

	proxy_system( cmd, 1, 3);

	if (!g_file_get_contents (tmp_file, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		ret = -1;
	}
	else
	{
		if(contents)
		{
			mark_start = strstr( contents, CONFLICT_MARK_START);
//			mark_end = strstr( contents, CONFLICT_MARK_END);

//			if( mark_start == NULL || mark_end == NULL )
			if( mark_start == NULL )
			{
				ret = -1;
			}
/*			
			else if( mark_start > mark_end )
			{
				ret = -1;
			}
*/			
			else
			{
				gchar *tmp_str;
				int conflict;

//				tmp_str = mark_end - 1;
				tmp_str = mark_start + strlen(CONFLICT_MARK_START);
				
				while(isspace(*tmp_str))
					tmp_str = tmp_str + 1;

/*
				while(isdigit(*tmp_str))
					tmp_str = tmp_str - 1;

				tmp_str = tmp_str + 1;
*/				
				conflict = strtol(tmp_str, NULL, 10);

				if( conflict > 1 )
				{
					int i;
					gboolean find_mac = FALSE;
					gchar *start_p = contents;
					
					for(i=0; i<conflict; i++)
					{
						gchar *mac_sp = NULL;
						gchar *mac_ep = NULL;
						mac_sp = strchr( start_p, '[');
						if( mac_sp )
						{
							start_p = mac_sp + 1;
							mac_ep = strchr( start_p, ']');

							if( mac_ep )
							{
								char *mac_p;
								NF_NETIF_MAC mac_tmp;
								
								start_p = mac_ep + 1;

								mac_p = mac_sp + 1;
								_convert_mac_str_to_val(mac_p, &mac_tmp);

								if( memcmp(&mac, &mac_tmp, sizeof(NF_NETIF_MAC)) )
								{
									memcpy(conflict_mac, &mac_tmp, sizeof(NF_NETIF_MAC));
									find_mac = TRUE;
									break;
								}
							}
							else
							{
								break;
							}	
						}
						else
						{
							break;
						}
					}

					if( find_mac == FALSE )
					{
						conflict = -1;
					}
				}

				ret = conflict;
			}

			g_free(contents);
		}
		else
		{
			ret = -1;
		}
	}

	return ret;
}

static char ip_conflict_test_help[] = "ip_conflict_test [idx]";
static int ip_conflict_test(int argc, char **argv)
{
	NF_NETIF_IP ret_ip;
	NF_NETIF_MAC conflict_mac;
	gchar ip_buff[32];	
	gchar mac_buff[32];
	
	int ret;
	int idx;

	if(argc < 2){
		printf("%s\n", ip_conflict_test_help);
		return -1;
	}

	idx = atoi(argv[1]);

	if( idx < 0 || idx > NUM_ACTIVE_CH ){
		printf("%s - %s\n", __FUNCTION__, "invalid param");		
		return -1;
	}

	if( idx == 0 )
	{
		nf_netif_get_ip(&ret_ip);

		_convert_ip_val_to_str( ret_ip, ip_buff, sizeof(ip_buff) );

		g_printf("%s SET => ip_str[%s]\n", __FUNCTION__, ip_buff);
		
		if( _is_invalid_ip(&ret_ip) == TRUE )
		{			
			g_printf("%s SET => invalid ip\n", __FUNCTION__);
		}
		else
		{
			gchar mac_str[32];
			ret = _network_ip_set_conflict_test(ret_ip, &conflict_mac);
	
			g_printf("%s SET => RET:%d\n", __FUNCTION__, ret);	

			if( ret > 0 )
			{
				_convert_mac_val_to_str(conflict_mac, mac_str, sizeof(mac_str));
				g_printf("%s SET => conflict_mac[%s]\n", __FUNCTION__, mac_str);			
			}	
		}
	}
	else
	{
		int ch;
		NF_NETIF_MAC ret_mac;

		ch = idx - 1;

		if( _get_cam_ip_mac_by_channel(ch, &ret_ip, &ret_mac) == 0 )
		{
			_convert_ip_val_to_str( ret_ip, ip_buff, sizeof(ip_buff) );			
			_convert_mac_val_to_str(ret_mac, mac_buff, sizeof(mac_buff));
			
			g_printf("%s CAM%d => ip_str[%s], mac[%s]\n", __FUNCTION__, ch, ip_buff, mac_buff);
			
			ret = _network_ip_cam_conflict_test(ret_ip, ret_mac, &conflict_mac);

			g_printf("%s CAM%d => RET:%d\n", __FUNCTION__, ch, ret);			

			if( ret > 1 )
			{
				gchar mac_str[32];
				
				_convert_mac_val_to_str(conflict_mac, mac_str, sizeof(mac_str));
				g_printf("%s CAM%d => conflict_mac[%s]\n", __FUNCTION__, ch, mac_str);				
			}
		}
		else
		{
			g_printf("%s CAM%d => Not connect\n", __FUNCTION__, ch);
		}
	}
			
	return 0;
}
__commandlist(ip_conflict_test,"ip_conflict_test", ip_conflict_test_help, ip_conflict_test_help);

int _network_ip_set_conflict_test(NF_NETIF_IP ip, NF_NETIF_MAC *conflict_mac)
{
	gchar  *contents = NULL;	
	GError *error = NULL;	
	gsize  length = 0;
	int ret = -1;

	gchar *mark_start;
//	gchar *mark_end;

	gchar cmd[1024];
	gchar tmp_file[256];

	gchar ip_str[32];

	_convert_ip_val_to_str(ip, ip_str, sizeof(ip_str));

	memset(conflict_mac, 0x0, sizeof(NF_NETIF_MAC));

	memset(cmd, 0x0, sizeof(cmd));
	memset(tmp_file, 0x0, sizeof(tmp_file));

	snprintf( tmp_file, sizeof(tmp_file), "/tmp/dvr_ip_test.tmp");

	snprintf(cmd, sizeof(cmd), "arping -D %s -w1 -I %s > %s", ip_str, _eth_dev, tmp_file);

	proxy_system( cmd, 1, 3);

	if (!g_file_get_contents (tmp_file, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		ret = -1;
	}
	else
	{
		if(contents)
		{
			mark_start = strstr( contents, CONFLICT_MARK_START);
//			mark_end = strstr( contents, CONFLICT_MARK_END);

//			if( mark_start == NULL || mark_end == NULL )
			if( mark_start == NULL )
			{
				ret = -1;
			}
/*			
			else if( mark_start > mark_end )
			{
				ret = -1;
			}
*/			
			else
			{
				gchar *tmp_str;
				int conflict;

//				tmp_str = mark_end - 1;
				tmp_str = mark_start + strlen(CONFLICT_MARK_START);
				
				while(isspace(*tmp_str))
					tmp_str = tmp_str + 1;

/*
				while(isdigit(*tmp_str))
					tmp_str = tmp_str - 1;

				tmp_str = tmp_str + 1;
*/				
				conflict = strtol(tmp_str, NULL, 10);

				if( conflict > 0 )
				{
					gchar *mac_sp = NULL;
					gchar *mac_ep = NULL;					
					gboolean find_mac = FALSE;
					gchar *start_p = contents;
					
					mac_sp = strchr( start_p, '[');
					
					if( mac_sp )
					{
						start_p = mac_sp + 1;
						mac_ep = strchr( start_p, ']');

						if( mac_ep )
						{
							char *mac_p = mac_sp + 1;
							
							_convert_mac_str_to_val(mac_p, conflict_mac);
							find_mac = TRUE;
						}
					}

					if( find_mac == FALSE )
					{
						conflict = -1;
					}
				}

				ret = conflict;
			}

			g_free(contents);
		}
		else
		{
			ret = -1;
		}
	}

	return ret;
}

static int _network_host_ip_conflict(NF_NETIF_IP *ret_ip)
{
	int ret = 0;
	char ip_str[64];
	NF_NETIF_MAC conflict_mac;			
	CONFLICT_STATE *cfl_stat;

	if(ret_ip == NULL)
		return -1;

	cfl_stat = &(net_stat.cfl_stat[net_stat.seq]);

	if( _is_invalid_ip(ret_ip) == TRUE )
	{
		g_printf("[IP_TEST] SET => invalid ip\n");
		
		if( cfl_stat->conflict != 0 )
		{
			cfl_stat->conflict = 0;
			memset( &(cfl_stat->conflict_mac), 0x0, sizeof(NF_NETIF_MAC));

			nf_notify_fire_params("set_ip_conflict", 0, 0, 0, 0);
		}

		return -1;
	}
	else
	{
		gchar mac_str[32];
		
		ret = _network_ip_set_conflict_test(*ret_ip, &conflict_mac);

		g_printf("[IP_TEST] SET => ret:%d\n", ret);

		if( ret >= 0 )
		{
			if( ret == 0 )
			{						
				if(cfl_stat->conflict != 0)
				{							
					cfl_stat->conflict = 0;
					memset( &(cfl_stat->conflict_mac), 0x0, sizeof(NF_NETIF_MAC));
					
					nf_notify_fire_params("set_ip_conflict", 0, 0, 0, 0);							
				}
			}
			else
			{
				_convert_mac_val_to_str(conflict_mac, mac_str, sizeof(mac_str));
										
				if(cfl_stat->conflict == 0)
				{
					cfl_stat->conflict = 1;							
					cfl_stat->conflict_mac = conflict_mac;						

					nf_notify_fire_params("set_ip_conflict", 1, 0, 0, 0);
					nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 1, LP2_NETWORK_IP_CONFILICT_DETECTED, mac_str);							
				
				}
				else
				{							
//					if( memcmp( &(cfl_stat->conflict_mac), &conflict_mac, sizeof(NF_NETIF_MAC)) )
//					{								
						cfl_stat->conflict_mac = conflict_mac;

						nf_notify_fire_params("set_ip_conflict", 1, 0, 0, 0);									
						nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 1, LP2_NETWORK_IP_CONFILICT_DETECTED, mac_str);								
//					}
				}
			}	
		}	
	}

	return 0;
}

static int ip_conflict_func = 1;

static void _network_conflict_monitor_thread()
{
	gboolean cam_mode_is_open;	

	memset( &net_stat, 0x0, sizeof(NETWORK_STATE) );

	cam_mode_is_open = nf_sysdb_get_bool("cam.install.mode");
	
	while(1)
	{
		if(ip_conflict_func == 1)
		{	
			if( net_stat.seq == 0 )
			{
				NF_NETIF_IP ret_ip;

				nf_netif_get_ip(&ret_ip);

				if(_network_host_ip_conflict(&ret_ip) < 0)
				{
					sleep(1);
				}
			}
			else if( net_stat.seq == 1 )
			{
				char link_lo_dev[32] = {0,};
				NF_NETIF_IP ret_ip;

				snprintf(link_lo_dev, sizeof(link_lo_dev), "%s:0", _eth_dev);

				nf_netif_get_ip_dev(link_lo_dev, &ret_ip);

				if(_network_host_ip_conflict(&ret_ip) < 0)
				{
					sleep(1);
				}
			}
			else
			{			
				if( (net_stat.seq - 2) < NUM_ACTIVE_CH )
				{
					if( cam_mode_is_open == TRUE )
					{				
						int ch;
						NF_NETIF_IP ret_ip;
						NF_NETIF_MAC ret_mac;
						NF_NETIF_MAC conflict_mac;

						CONFLICT_STATE *cfl_stat;

						ch = net_stat.seq - 2;

						cfl_stat = &(net_stat.cfl_stat[net_stat.seq]);

						if( _get_cam_ip_mac_by_channel(ch, &ret_ip, &ret_mac) == 0 )
						{					
							if( _is_invalid_ip(&ret_ip) == TRUE )
							{
								g_printf("[IP_TEST] CAM%d => invalid ip\n", ch);
								
								if( cfl_stat->conflict != 0 )
								{
									cfl_stat->conflict = 0;
									memset( &(cfl_stat->conflict_mac), 0x0, sizeof(NF_NETIF_MAC));
									
									nf_notify_fire_params("cam_ip_conflict", 0, ch, 0, 0);
								}

								sleep(1);
							}
							else 
							{
								int ret = 0;
								gchar mac_str[32];
								gchar ch_mac[64];
								
								ret = _network_ip_cam_conflict_test(ret_ip, ret_mac, &conflict_mac);
								g_printf("[IP_TEST] CAM%d => ret:%d\n", ch, ret);						

								if( ret >= 0 )
								{
									if( ret > 1 )
									{
										_convert_mac_val_to_str(conflict_mac, mac_str, sizeof(mac_str));

										g_printf("[IP_TEST] CAM%d => conflict mac:%s\n", ch, mac_str);							
										
										if(cfl_stat->conflict == 0)
										{
											cfl_stat->conflict_mac = conflict_mac;
											
											cfl_stat->conflict = 1;
											snprintf(ch_mac, sizeof(ch_mac), "%d,%s", ch, mac_str);

											nf_notify_fire_params("cam_ip_conflict", 1, ch, 0, 0);											
											nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 1, LP2_NETWORK_IPCAM_IP_CONFILICT_DETECTED, ch_mac);						
										}
										else
										{
//											if( memcmp( &(cfl_stat->conflict_mac), &conflict_mac, sizeof(NF_NETIF_MAC)) )
//											{
												cfl_stat->conflict_mac = conflict_mac;				
												snprintf(ch_mac, sizeof(ch_mac), "%d,%s", ch, mac_str);

												nf_notify_fire_params("cam_ip_conflict", 1, ch, 0, 0);												
												nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 1, LP2_NETWORK_IPCAM_IP_CONFILICT_DETECTED, ch_mac);							
//											}
										}
									}
									else
									{							
										if(cfl_stat->conflict != 0)
										{							
											cfl_stat->conflict = 0;
											nf_notify_fire_params("cam_ip_conflict", 0, ch, 0, 0);
										}
									}
								}
							}
						}
						else
						{
							g_printf("[IP_TEST] CAM%d => Not connect\n", ch);
							sleep(1);
						}
					}
					else
					{
						sleep(1);
					}	
				}
				else
				{
					sleep(1);
				}
			}

			(net_stat.seq)++;

			if( net_stat.seq == (NUM_ACTIVE_CH + 1 + 1) ) // cam 16, host 1, link-local 1
			{
				net_stat.seq = 0;
			}

			sleep(1);
		}
		else
			sleep(1);			
	}
}

gboolean nf_network_monitor_init()
{
    pthread_t tid = 0;
	
    if ( pthread_create(&tid, NULL, (void*)_network_dhcp_monitor_thread, NULL) != 0)
    {
        return FALSE;
    }

    pthread_detach(tid);

    if ( pthread_create(&tid, NULL, (void*)_network_conflict_monitor_thread, NULL) != 0)
    {
        return FALSE;
    }	

    pthread_detach(tid);

	return TRUE;	
}

gboolean nf_network_get_mac_conflict_ipset(NF_NETIF_MAC *ret_mac)
{
	CONFLICT_STATE *cfl_stat;

	memset(ret_mac, 0x0, sizeof(NF_NETIF_MAC));

	cfl_stat = &(net_stat.cfl_stat[0]);
	
	if(cfl_stat->conflict)
	{
		*ret_mac = cfl_stat->conflict_mac;

		return TRUE;
	}
	else
		return FALSE;
}

gboolean nf_network_get_mac_conflict_ipcam(guint ch, NF_NETIF_MAC *ret_mac)
{
	CONFLICT_STATE *cfl_stat;

	memset(ret_mac, 0x0, sizeof(NF_NETIF_MAC));	

	if(ch >= NUM_ACTIVE_CH)
		return FALSE;
	
	cfl_stat = &(net_stat.cfl_stat[ch+1]);
	
	if(cfl_stat->conflict)
	{
		*ret_mac = cfl_stat->conflict_mac;

		return TRUE;
	}
	else
		return FALSE;
}

static char ip_conflict_state_help[] = "ip_conflict_state [idx]";
static int ip_conflict_state(int argc, char **argv)
{
	gchar mac_buff[32];
	int idx;
	int ret;
	CONFLICT_STATE *cfl_stat;

	if(argc < 2){
		printf("%s\n",ip_conflict_state_help);
		return -1;
	}

	memset(mac_buff, 0x0, sizeof(mac_buff));

	idx = atoi(argv[1]);

	if( idx < 0 || idx > NUM_ACTIVE_CH ){
		printf("%s - %s\n", __FUNCTION__, "invalid param");		
		return -1;
	}

	cfl_stat = &(net_stat.cfl_stat[idx]);	

	if( cfl_stat->conflict )
	{
		_convert_mac_val_to_str(cfl_stat->conflict_mac, mac_buff, sizeof(mac_buff));
		
		g_message("%s - IDX:%d => conflict:1, conflict_mac:%s", __FUNCTION__, idx, mac_buff);
	}
	else
	{
		g_message("%s - IDX:%d => conflict:0, ", __FUNCTION__, idx);		
	}
		
	return 0;
}
__commandlist(ip_conflict_state,"ip_conflict_state", ip_conflict_state_help, ip_conflict_state_help);

static char ip_conflict_onoff_help[] = "ip_conflict_onoff [off(0):on(1)]";
static int ip_conflict_onoff(int argc, char **argv)
{
	int onoff;

	if(argc < 2){
		printf("%s\n",ip_conflict_state_help);
		return -1;
	}

	onoff = atoi(argv[1]);

	g_message("%s - ONOFF => %d", __FUNCTION__, onoff);

	ip_conflict_func = onoff;
		
	return 0;
}
__commandlist(ip_conflict_onoff,"ip_conflict_onoff", ip_conflict_onoff_help, ip_conflict_onoff_help);

// NETWORK SCENARIO END
/******************************************************************************/

gboolean nf_network_onestop_test()
{
	return nf_pds_onestop_test();
}

gboolean nf_network_is_connected_internet(void)
{
	gchar  tmp[256];
	gchar  buff[256];
	gboolean ret = FALSE;
	FILE *fp;
	gint link_status = 0;

	GTimeVal tval;
	gchar result_file[128];

	nf_netif_get_link_status(HOST_ETH_DEV, &link_status );
	if (!link_status) {
		g_message("netif get link : return false\n");
		return FALSE;
	}

	gettimeofday(&tval, NULL);
	snprintf(result_file, sizeof(result_file), "/tmp/wan_ping_%ld_%06ld", tval.tv_sec, tval.tv_usec);

	g_message("ping to GOOGLE DNS\n");
	snprintf( tmp, sizeof(tmp), "ping %s -c 1 -W 1 | grep \"1 packets received\" | wc -l >& %s", "8.8.8.8", result_file);

	proxy_system(tmp, 1, 3);
	fp = fopen( result_file, "r");
	if( fp != NULL)
	{
		if( fgets( buff, sizeof(buff), fp) )
		{
			if ( strchr( buff, '1') != NULL )
				ret = TRUE;
		}

		fclose( fp);
	}

	if (ret) {
		g_message("ping return TRUE\n");

		if (access(result_file, F_OK) == 0)
			remove(result_file);

		return TRUE;
	}

	g_message("ping to GOOGLE DNS\n");
	snprintf( tmp, sizeof(tmp), "ping %s -c 1 -W 1 | grep \"1 packets received\" | wc -l >& %s", "168.126.63.1", result_file);

	proxy_system(tmp, 1, 3);
	fp = fopen( result_file, "r");
	if( fp != NULL)
	{
		if( fgets( buff, sizeof(buff), fp) )
		{
			if ( strchr( buff, '1') != NULL )
				ret = TRUE;
		}

		fclose( fp);
	}

	g_message("connected internet returned, [%d]\n", ret);

	if (access(result_file, F_OK) == 0)
		remove(result_file);

	return ret;
}


static char unimo_check_send_help[] = "unimo_check_send";
static int unimo_check_send(int argc, char **argv)
{
	int ret;

	ret = _unimo_check_send_packet(NULL,0);

	printf("[%s] RET[%d]\n", __FUNCTION__, ret);
		
	return 0;
}
__commandlist(unimo_check_send,"unimo_check_send", unimo_check_send_help, unimo_check_send_help);
