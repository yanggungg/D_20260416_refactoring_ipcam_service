#include "nf_common.h"
#include "nf_notify.h"
#include "nf_onvif_server.h"
#include "nf_sysman.h"
#include "glib.h"
#include "nvs_service/nvs_onvif_app_util.h"
#include "nvs_service/nvs_discovery.h"

#include <arpa/inet.h>
#include <sys/un.h> 
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef USE_PROXY_SYSTEM
#include "proxy_cli.h"
#endif

#define MAX_CLNT_CNT 1000
extern void *onvif_discovery(void *arg);

int g_onvif_service_stop = 0;
	
static pthread_t p_thread[2] = {0,};
static int discovery_flag = 0;

#define HOST_ETH_DEV	"eth0"
#define HUB_ETH_DEV		"eth1"
#define BRIDGE_DEV		"br0"

static char *_eth_dev = HOST_ETH_DEV;

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

pthread_mutex_t		g_req_list_mutex;
pthread_mutex_t		g_req_thread_data_mutex;
pthread_mutex_t		g_release_sock_mutex;

void LOCK_onvif_req_list()
{
	pthread_mutex_lock(&g_req_list_mutex);
}

void UNLOCK_onvif_req_list()
{
	pthread_mutex_unlock(&g_req_list_mutex);
}

void LOCK_onvif_req_thread_data()
{
	pthread_mutex_lock(&g_req_thread_data_mutex);
}

void UNLOCK_onvif_req_thread_data()
{
	pthread_mutex_unlock(&g_req_thread_data_mutex);
}

void LOCK_onvif_release_sock()
{
	pthread_mutex_lock(&g_release_sock_mutex);
}

void UNLOCK_onvif_release_sock()
{
	pthread_mutex_unlock(&g_release_sock_mutex);
}

static void nf_onvif_class_init		(NfOnvifClass * klass); 	 
static void nf_onvif_instance_init 	(GTypeInstance * instance, gpointer g_class);
static void nf_onvif_set_property  	(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void nf_onvif_get_property  	(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void nf_onvif_dispose  		(GObject * object);
static void nf_onvif_finalize 		(GObject * object);

//static gboolean _onvif_tmpdir_create(void);
//static void _onvif_live_log_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);



static GObjectClass 					*parent_class = NULL;
static NfOnvif						*_nf_onvif = NULL;
////////////////////////////////////////////////////////////////////////////////
//IP-CAM

static char * _onvif_fileserver = ONVIF_FILE_SERVER;
//static char * _DVRinfoFilePath = "/tmp/info.js";
//static char * _DVRsavingStatusPath = "/tmp/saving.js";
//static char * _WEBinfoFilePath = "/tmp/webinfo.conf"; 
static int ov_change_key = 0;
////////////////////////////////////////////////////////////////////////////////
static void 	_onvif_socket_release(NfOnvif *self, gint socket);
//static void		_onvif_pop_func (NfOnvif *self, gint fd);
static gint 	_onvif_accept(NfOnvif *self);
static void		_onvif_receive (NfOnvif *self);
static gint		_onvif_net_init(NfOnvif *self);
static void		_onvif_select_init (NfOnvif *self);
static void		_onvif_thread_func (NfOnvif *self);

//static gint 	_onvif_queue_error_check(ONVIF_QUEUE *q);
//static gint 	_onvif_queue_init(ONVIF_QUEUE *q);
//static gint 	_onvif_queue_status(ONVIF_QUEUE *q);
//static gint 	_onvif_queue_available_space(ONVIF_QUEUE *q);
//static gint		_onvif_queue_peek(ONVIF_QUEUE *q, char *data, guint length);
//static gint 	_onvif_queue_push(ONVIF_QUEUE *q, char *data, guint length);
//static gint		_onvif_queue_pop(ONVIF_QUEUE *q, char *data , guint length);
////////////////////////////////////////////////////////////////////////////////

extern int nf_onvif_packet_code(NfOnvif *self, int fd ,char *buff_rcv);

GType
nf_onvif_get_type (void)
{
	static GType nf_onvif_type = 0;

	if (G_UNLIKELY (nf_onvif_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfOnvifClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_onvif_class_init,
			NULL,
			NULL,
			sizeof (NfOnvif),
			0,
			(GInstanceInitFunc) nf_onvif_instance_init,
			NULL
		};

		nf_onvif_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfOnvif", &object_info, 0);
	}
	
	return nf_onvif_type;
}

static void
nf_onvif_class_init (NfOnvifClass * klass)
{	
	GObjectClass *gobject_class;
//	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_onvif_set_property;
	gobject_class->get_property = nf_onvif_get_property;
			
	gobject_class->dispose = nf_onvif_dispose;
	gobject_class->finalize = nf_onvif_finalize;
}
#define MAX_LONG_POLLING_THREAD (32)
pthread_t long_polling[MAX_LONG_POLLING_THREAD]={0,}; 

static void
nf_onvif_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfOnvif *self = NF_ONVIF (instance);	

	self->init_done = 0;

	memset(self->wqueue, 0x00, sizeof(self->wqueue));
	
	_onvif_net_init(self);
	_onvif_select_init(self);

	pthread_mutex_init(&g_req_list_mutex, NULL);
	pthread_mutex_init(&g_req_thread_data_mutex, NULL);	
	pthread_mutex_init(&g_release_sock_mutex, NULL);		
	
	memset(long_polling, 0x00, sizeof(pthread_t)*MAX_LONG_POLLING_THREAD);
	
	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)_onvif_thread_func, self, FALSE, NULL);	
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_onvif_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_onvif_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_onvif_set_property (GObject * object, guint prop_id,
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
nf_onvif_get_property (GObject * object, guint prop_id,
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

////////////////////////////////////////////////////////////////////////////////
/* ������  */
////////////////////////////////////////////////////////////////////////////////



static void 
_onvif_clear_fd(NfOnvif *self, gint socket)
{
	LOCK_onvif_release_sock();
	FD_CLR(socket, &self->select_val.rset);
	UNLOCK_onvif_release_sock();	
}


static void 
_onvif_socket_release(NfOnvif *self, gint socket)
{
	LOCK_onvif_release_sock();
	FD_CLR(socket, &self->select_val.rset);
	close(socket);
	if ( self->wqueue[socket] ){
		g_free(self->wqueue[socket]);
		self->wqueue[socket] = NULL;
	}
	UNLOCK_onvif_release_sock();	
}

/*
static void
_onvif_pop_func (NfOnvif *self, gint fd)
{
#if 0 //YOON
	char *data;
#else
	char data[ONVIF_DATA_BUFF];
#endif
	int  ret;

	while ( 1 )
	{
		if( (ret = _onvif_queue_pop( self->wqueue[fd], data, ONVIF_DATA_BUFF)) != QUEUE_SUCCESS ){
				if( ret == QUEUE_RET_QUEUE_EMPTY )
				{
						break;
				}
				else if( ret == -1)
				{
					g_warning("%s_onvif_queue_pop() error!", __FUNCTION__);
					break;
				}
		}
		
		ret = nf_onvif_packet_code(self, fd, data);	

		if ( ret != ONVIF_ERR_RET_SUCCESS )
		{
			g_warning("%s _onvif_packet_code() error!", __FUNCTION__);
			_onvif_socket_release(self, fd);
			break;
		}
	}
}
*/

static gint 
_onvif_accept(NfOnvif *self)
{
	gint  	serv_sock = self->serv_sock;
	struct 	sockaddr_un   client_addr;
	guint   clnt_addr_size;
	gint   	clnt_socket;
	ONVIF_SELECT *selval  = &self->select_val;

	while( 1 )
	{
		clnt_addr_size  = sizeof( client_addr );

		clnt_socket     = accept( serv_sock, (struct sockaddr*)&client_addr, &clnt_addr_size);

		if ( clnt_socket < 0 )
		{
		if ( errno == EINTR )
		{
			continue;
		}
			g_warning( "%s unable to accept control socket: %s ", __FUNCTION__, strerror( errno ) );
			 return ONVIF_ERR_RET_SOCKET;
		}
		else if (clnt_socket >= OV_MAX_CLNT_CNT)
		{
			close(clnt_socket);
			g_warning( "%s client Max Count Over Error!", __FUNCTION__);
			return ONVIF_ERR_RET_SOCKET;
		}
		break;
	}
	LOCK_onvif_release_sock();
	FD_SET(clnt_socket, &selval->rset);	// select�� Ŭ���̾�Ʈ�� fd�� ���ؼ� ���� 

	if ( clnt_socket > selval->clntCnt )
	{
		selval->clntCnt = clnt_socket;
	}

	if (self->wqueue[clnt_socket] )
	{
		g_free(self->wqueue[clnt_socket]);
		self->wqueue[clnt_socket] = NULL;
	}
	self->wqueue[clnt_socket] = g_malloc0( sizeof(ONVIF_QUEUE) );
	UNLOCK_onvif_release_sock();

	if ( self->wqueue[clnt_socket] == NULL )
	{
		g_warning( "%s self->wqueue[%d] memory alloc error!", __FUNCTION__,clnt_socket );
		_onvif_socket_release(self, clnt_socket);
		return ONVIF_ERR_RET_INTERNAL;
	}

//	_onvif_queue_init(self->wqueue[clnt_socket]);
/*
	if ( (_onvif_queue_init(self->wqueue[clnt_socket])) != ONVIF_ERR_RET_SUCCESS)
	{
		g_warning( "%s client socket[%d]_onvif_queue_init() error!", __FUNCTION__,clnt_socket );
		_onvif_socket_release(self, clnt_socket);
		return ONVIF_ERR_RET_INTERNAL;
	}
*/
	return ONVIF_ERR_RET_SUCCESS;

}


struct long_req_data {
	NfOnvif *self;
	int fd;
	ONVIF_PACKET packet;
	pthread_t *th_list;
}; 

enum {
	NORMAL_OK = -2,
	LONG_POLLING_FULL = -1,
	LONG_POLLING_OK = 0
};

int check_if_long_polling_request(pthread_t *long_polling, guchar code)
{
	int i=0;

	// If code is not a longpolling request, return -2.
	// WARNING : When you add a Long polling command, you MUST consider the thread safe.
	if( code != CMD_PullMessages ){
		return NORMAL_OK;
	}

	// If code is a longpolling request, return thread index.
	LOCK_onvif_req_list();
	for(i=0; i<MAX_LONG_POLLING_THREAD; i++){
		if( long_polling[i] == 0 ){
			//LONG_POLLING_OK
			UNLOCK_onvif_req_list();				
			return i;
		}
	}	
	UNLOCK_onvif_req_list();	

	// If thread is full, return -1.	
	return LONG_POLLING_FULL;
}

static void *_nf_onvif_packet_code_thread(void *arg)
{
	struct long_req_data d;	
	int ret = 0, i=0;
	unsigned int target_size = 0;

	memset(&d, 0x00, sizeof(struct long_req_data));
	memcpy(&d, arg, sizeof(struct long_req_data));	
	UNLOCK_onvif_req_thread_data();

	target_size = ntohl(d.packet.header.dlen) + sizeof(ONVIF_HEADER);
	ret = nf_onvif_packet_code(d.self, d.fd, (char*)&d.packet);
	if ( ret != ONVIF_ERR_RET_SUCCESS )
	{
		g_warning("%s _onvif_packet_code() error!", __FUNCTION__);
	}

	_onvif_socket_release(d.self, d.fd);	
	
th_terminate:
	
	LOCK_onvif_req_list();
	for(i=0; i<MAX_LONG_POLLING_THREAD; i++){
		if( d.th_list[i] == pthread_self()){
			d.th_list[i] = 0;
		}
	}
	UNLOCK_onvif_req_list();	
	
	return (void *)0;
}


static void
_onvif_receive (NfOnvif *self)
{
	g_return_if_fail( self != NULL );
//	g_return_if_fail( self->serv_sock > 0 );
	
	ONVIF_SELECT *selval  = &self->select_val;;
	struct timeval timeout;
	gint *clntCnt = &selval->clntCnt;
	gint   serv_sock = self->serv_sock;
	gint 	select_ret;
	fd_set  rsetTmp;
//	fd_set	wsetTmp;


	struct long_req_data d;
		
	gint fd;


	// Select Multiplexing
	while( 1 )
	{
		rsetTmp = selval->rset;
		timeout.tv_sec = selval->tv.tv_sec;
		timeout.tv_usec = selval->tv.tv_usec;
		
		if (_nf_onvif->sysdb_change)
		{
			_nf_onvif->sysdb_change = 0;
		}
		
		select_ret = select(*clntCnt + 1 , &rsetTmp, 0, 0, &timeout );

		if ( select_ret == -1 )
		{
			if ( errno == EINTR )
			{
				continue;
			}
			g_warning("%s select() error! : %s ", __FUNCTION__, strerror( errno ) );
			//FIXME.baek.why does the Select() make a Bad file descriptor???
			_onvif_select_init(self);
			return;
		}
		else if ( select_ret == 0 )
		{
			//g_warning("%s select() timeout! : %d", __FUNCTION__,  timeout.tv_sec);
			return;
		}
		break;
	}
	
	for ( fd = 0; fd < *clntCnt + 1; ++fd)
	{
		if ( FD_ISSET(fd, &rsetTmp) )
		{
			if ( fd == serv_sock ) // ���� ��û 
			{
				if (_onvif_accept(self) != ONVIF_ERR_RET_SUCCESS)
				{
					return;
				}
//				g_message("%s Client connection [%d]", __FUNCTION__, fd);
			}
			else
			{
				ONVIF_HEADER *pHeader = NULL;
				ONVIF_QUEUE  *pQue = NULL;
				
				gint ret = 0;
				gint read_size = 0;
				gint target_size = 0;
					
				pQue = self->wqueue[fd];
				g_assert(pQue);

				//pHeader = pQue->queue;				
				// khj modify (ONVIF_HEADER *)
				pHeader = (ONVIF_HEADER *)pQue->queue;				
				g_assert(pHeader);				
					
				if( pQue->wpos >= sizeof(ONVIF_HEADER) ) 
				{
					target_size = ntohl(pHeader->dlen) + sizeof(ONVIF_HEADER);
				}
				else{
					target_size = sizeof(ONVIF_HEADER);
				}

				read_size = target_size - pQue->wpos;
				if( read_size >0)
				{
					ret = read ( fd, pQue->queue + pQue->wpos , read_size);								
					if ( ret == -1 )
					{
						if ( errno == EINTR )
						{
							continue;
						}else{				
							g_warning("%s read Error! fd[%d] : %d(%s)", __FUNCTION__, fd, errno, strerror(errno) );
							goto error_continue;
						}
					}else if (ret == 0) {			
						//g_warning("%s read close fd[%d] : %d(%s)", __FUNCTION__, fd, errno, strerror(errno) );
						goto error_continue;
					}else{
						pQue->wpos += ret;
					}
				}

				if( pQue->wpos >= sizeof(ONVIF_HEADER) ) 
				{
					target_size = ntohl(pHeader->dlen) + sizeof(ONVIF_HEADER);
					if( target_size == pQue->wpos) // binggo!! make packet
					{
						int req_ret = 0;

						_onvif_clear_fd(self, fd);

						req_ret = check_if_long_polling_request(long_polling, pHeader->code);

						if( req_ret != NORMAL_OK ){
							OV_DEBUG("%s check_long_polling_request code[%d] ret[%d]", __FUNCTION__, pHeader->code, req_ret );
						}
						
						if( req_ret >=  LONG_POLLING_OK ){
							//baek.this wiil be unlock on _nf_miranda_packet_code_thread.							
							LOCK_onvif_req_thread_data();							
							d.self = self;
							memcpy(&d.packet, pHeader, sizeof(ONVIF_HEADER)+ntohl(pHeader->dlen));
							d.fd = fd;
							d.th_list = long_polling;
							
							LOCK_onvif_req_list();
							pthread_create( &long_polling[req_ret], NULL, _nf_onvif_packet_code_thread, &d );
							pthread_detach(long_polling[req_ret]);
							UNLOCK_onvif_req_list();
							usleep(10*1000); // time for copy 'd'
						}
						else if (req_ret == LONG_POLLING_FULL ){
							g_warning("%s LONG_POLLING_FULL!", __FUNCTION__);
							goto error_continue;							
						}
						else if (req_ret == NORMAL_OK ){
							ret = nf_onvif_packet_code(self, fd, (char *)pHeader);
							if ( ret != ONVIF_ERR_RET_SUCCESS )
							{
								g_warning("%s _onvif_packet_code() error!", __FUNCTION__);
								goto error_continue;
							}						
							_onvif_socket_release(self, fd);
						}
					}
				}
//ok_continue:
			continue;					
error_continue:
			_onvif_socket_release(self, fd);
			continue;
				
			}
		}
	}

}

gint nf_onvif_send_fault(gint fd, gint code, char *buff_rcv)
{
	ONVIF_PACKET	   packet;	
	ONVIF_HEADER		header;
	guint packet_len;

       memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));    

	packet_len = sizeof(ONVIF_HEADER) + sizeof(gint);

	// Additional inforamtion
	memset(packet.data, 0x00, sizeof(gint));
	
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_FAULT, code, sizeof(gint)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

gint nf_onvif_send(gint fd, ONVIF_PACKET *packet, guint packet_len, gint type, gint code_tmp, guint dlen)
{
	gint write_len = 0;
	gint write_len_tmp = 0;
	int code;
	code = code_tmp;    

	g_return_val_if_fail ( packet != NULL, 0);
	g_return_val_if_fail ( packet_len >= 0, 0);
	g_return_val_if_fail ( dlen >= 0, 0);
	//g_return_val_if_fail ( code >= CGI_SET_SYSTEM_MANAGE_FW_UPDATING && code < ONVIF_FIELD_NR, 0);
	packet->header.type = type;
	packet->header.code = code;
	packet->header.dlen = htonl(dlen);

	packet_len = sizeof(ONVIF_HEADER) + ntohl(packet->header.dlen);

	while( 1 )
	{
		write_len = write( fd, packet, packet_len ); 
						
		if ( errno == EINTR )
		{
			continue;
		}
		if ( write_len == -1 )
		{
			g_warning("%s fd[%d] write error : %d(%s)", __FUNCTION__, fd, errno, strerror(errno) );
			return ONVIF_ERR_RET_INTERNAL;
		}

		write_len_tmp += write_len;
		
		if (write_len_tmp == packet_len)
		{
			//g_message("%s Success OUT", __FUNCTION__);
			break;
		}
/*
		else
		{
			packet_len -= write_len;
		}
*/
	}

	//g_message("%s OUT", __FUNCTION__);

	return ONVIF_ERR_RET_SUCCESS;

}
//////////////////////////////////////////////////////////////////////////////////////////////
/* Init Function*/
//////////////////////////////////////////////////////////////////////////////////////////////
/*
UDS(Unix Domain Socket) Protocol�� ���� IPC ����� �Ѵ�. 
TCP/IP�� ��� 
*/
static gint
_onvif_net_init(NfOnvif *self)
{
	g_return_val_if_fail(_onvif_fileserver != NULL, -1);

	char *path = _onvif_fileserver;
	
	struct sockaddr_un server_addr;

	int option;
	socklen_t optlen;

	if ( access( path, F_OK) == 0 )
	{
		unlink( path);
	}

	while(1)
	{

		self->serv_sock = socket( PF_FILE, SOCK_STREAM, 0);

		if( self->serv_sock < 0 )
		{
			g_warning(" %s server socket error!", __FUNCTION__);
			sleep(1);
			continue;
		}

		optlen = sizeof(option);
		option = TRUE;
		
		setsockopt(self->serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, optlen );	// ������ Dead�� �ٽ� �츮�� ���� 

		memset( &server_addr, 0, sizeof( server_addr));
		server_addr.sun_family  = AF_UNIX;
		strcpy( server_addr.sun_path, path);

		if( bind( self->serv_sock, (struct sockaddr*)&server_addr, sizeof( server_addr) ) < 0 )
		{
			g_warning( "%s unable to bind control socket: %s ", __FUNCTION__, strerror( errno ) );
			close(self->serv_sock);
			sleep(1);
			continue;
		}

		if( listen(self->serv_sock, OV_MAX_CLNT_CNT) < 0 )
		{
			 g_warning( "%s unable to listen control socket: %s", __FUNCTION__, strerror( errno ) );
			 close(self->serv_sock);
			 sleep(1);
			 continue;
		}
		break;

	}

	return ONVIF_ERR_RET_SUCCESS;

}

static void _onvif_select_init (NfOnvif *self)
{	
	g_return_if_fail(self != NULL);
	
	ONVIF_SELECT *selval  = &self->select_val;

	FD_ZERO(&selval->rset);
	FD_SET(self->serv_sock, &selval->rset);

	selval->tv.tv_sec = 2;
	selval->tv.tv_usec = 0; 

	selval->clntCnt = self->serv_sock;
}

void ov_update_change_key(void)
{
	++ov_change_key;
	ov_change_key &= 0x000000FF;	// modular 256
}

gboolean 
nf_onvif_init(int wait)
{
	gint is_openmode = 0, is_custom = 0;
	gboolean ret = TRUE;			
	
	g_return_val_if_fail (_nf_onvif == NULL, FALSE);	
	
	_nf_onvif = g_object_new ( NF_TYPE_ONVIF , NULL);

	if( wait )
	{
		while( _nf_onvif->init_done != 1)
			g_usleep(1000*10);
	}
	_nf_onvif->live_log_queue = NULL;

	_nf_onvif->sysdb_change = 0;
	_nf_onvif->web_port_change = 0;

#ifdef DUAL_LAN_NETWORK
	is_openmode = nf_sysdb_get_bool("cam.install.mode");
	is_custom = nf_sysdb_get_bool("cam.install.dual_lan");
	if(is_openmode == 1)
	{
		if(is_custom)
			_eth_dev = HOST_ETH_DEV;
		else
			_eth_dev = BRIDGE_DEV;
	}
	else
	{
		_eth_dev = HOST_ETH_DEV;
	}
#else
		_eth_dev = HOST_ETH_DEV;
#endif

	return ret;
}

static void
_onvif_thread_func (NfOnvif *self)
{       
	g_return_if_fail(self != NULL);

	g_message("%s start", __FUNCTION__);
	// wait init complete
	while( _nf_onvif == NULL ) 
	{
		g_usleep(1000*10);
	}
	
	self->init_done = 1;
	
	while(self->thread_run)
	{							
		_onvif_receive(self);
	}
}

void onvif_service_stop(void)
{
	g_onvif_service_stop = 1;
	multi_bye();
}

extern int lock_msgid_shm_and_clear();
extern int unlock_msgid_shm();

void onvif_discovery_start(void)
{
	if(discovery_flag == 0)
	{
		if(pthread_create(&p_thread[1], NULL, onvif_discovery, (void *)NULL) < 0) {
			perror("onvif_discovery create error : ");
			return;
		}
		pthread_detach(p_thread[1]);
		discovery_flag = 1;
	}
}

void onvif_discovery_stop(void)
{
	if(discovery_flag != 0)
	{
		pthread_cancel(p_thread[1]); 
		discovery_flag = 0;
	}
}

/* for ncx3
 *   auth file have to be created here manually on webra2.0
 */
static char * _ONVIFauthFilePath = "/tmp/.passwd_onvif";
static void nf_onvif_create_user_info_file()
{
	FILE *fp = NULL;
	guint i = 0;
	guint usrcnt = nf_sysdb_get_uint("usr.UCNT");
	gchar buff[64] = {0};
	gchar *grpname = NULL;
	
	if ( (fp = fopen(_ONVIFauthFilePath, "w")) == NULL ) {
		g_warning("%s file oepn error", __FUNCTION__);
		return;
	}

	for (i = 0; i < usrcnt; i++) {
		memset(buff, 0x0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.name", i);
		fprintf(fp, "%s:", nf_sysdb_get_str_nocopy(buff));

		memset(buff, 0x0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.grpname", i);
		grpname = nf_sysdb_get_str_nocopy(buff);
		
		if (strncmp(grpname, "ADMIN", sizeof("ADMIN")) == 0){
			fprintf(fp, "ADMIN:");
		}
		else if (strncmp(grpname, "MANAGER", sizeof("MANAGER")) == 0){
			fprintf(fp, "OPERATOR:");
		}
		else if (strncmp(grpname, "USER", sizeof("USER")) == 0){
			fprintf(fp, "USER:");
		}
		else{
			fprintf(fp, "USER:");
		}
		
		memset(buff, 0x0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.pass", i);		
		fprintf(fp, "%s\n", nf_sysdb_get_str_nocopy(buff));
	}
	
	fchmod((intptr_t)fp, S_IEXEC);
	fclose(fp);
}
static void _sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_message("%s param[%d]",__FUNCTION__, pinfo->d.params[0]);
	
	if(pinfo->d.params[0] == NF_SYSDB_CATE_USR)
	{
		nf_onvif_create_user_info_file();
	}
}

void onvif_init(void)
{
	int discovery_mode=0;

	discovery_mode = nf_sysdb_get_uint("onvif.common.discovery");
	_TTY_LOG_ONVIF("onvif discovery[%d]", discovery_mode);

	g_printf("#######################onvif discovery[%d]", discovery_mode);


	nf_onvif_init(1);

	if((discovery_mode == 1) && (discovery_flag == 0))
	{
	g_printf("#######################onvif_init 22222222222222222\n");
		//baek.debug
		_TTY_LOG_ONVIF("discovery th start");
		if(pthread_create(&p_thread[1], NULL, onvif_discovery, (void *)NULL) < 0) {
			perror("onvif_discovery create error : ");
			return;
		}
		pthread_detach(p_thread[1]);
		discovery_flag = 1;
	}

	apply_onvif_relay_from_sysdb();

	/* for ncx3
	 *   auth file have to be created here manually on webra2.0
	 */
	static gint run_once = 0;
	gint cb_handle = 0;

	nf_onvif_create_user_info_file();
	if(!run_once)
	{
		cb_handle = nf_notify_connect_cb( "sysdb_change", _sysdb_reload_cb_func, (gpointer)NULL);
		printf("[%s][%d] sysdb_change connect_cb[%ld]\n", __FUNCTION__, __LINE__, cb_handle);
		g_assert(cb_handle > 0);
		run_once = 1;
	}
}		

static char * _ONVIFserviceFilePath = "/tmp/onvif_webinfo.conf";
static char * _ONVIFCONFFilePath = "/NFDVR/onvif/lighttpd/sbin/make_onvifinfo.sh";
void	onvif_restart()
{
	char cmd_make_info[128]={0,};
	int onvifport = nf_sysdb_get_uint("net.http.onvifport");
	int http_auth = nf_sysdb_get_uint("net.proto.httpauth_method");
	int https_on = is_https_required();
	
    memset(cmd_make_info, 0x00, sizeof(cmd_make_info));
	
	sprintf(cmd_make_info, "/bin/sh %s %d %s %s",
	_ONVIFCONFFilePath, onvifport,
	https_on == 1 ? "enable": "disable",
	http_auth == 1 ? "digest": "basic");
	
	proxy_system(cmd_make_info, 1, 3);

	FILE *fp;
  
	fp = fopen(_ONVIFserviceFilePath, "w");

	if( fp != NULL ) {
#ifdef _ONVIF_STAND_ALONE
		fprintf(fp, "server.port=%d\n", onvifport);
#else
		fprintf(fp, "server.bind = \"/tmp/lighttpd.socket\"\n");
#endif
		fclose(fp);
	}
	g_message("%s ONVIF PORT [%d]", __FUNCTION__, onvifport);

#ifdef ENABLE_ONVIF_DEVICE
	proxy_system("/bin/sh /NFDVR/onvif/lighttpd/sbin/onvif.sh restart o",1,3);
#else
	proxy_system("/bin/sh /NFDVR/onvif/lighttpd/sbin/onvif.sh restart",1,3);
#endif
	
}

extern char* nf_onvif_get_eth_str(void)
{
	return _eth_dev;
}

