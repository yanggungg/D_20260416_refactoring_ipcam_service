#ifndef __NF_NETWORK_H__
#define __NF_NETWORK_H__

#include "nf_object.h"

#include "nf_netsvr.h"
#include "nf_netsvr_drdef.h"
#include "unp.h"
#include "nf_util_netif.h"

/******************************************************************************/
// for nf_netsvr_live.c
int _netsvr_get_client_count(void);
void _netsvr_enque_client_entry( SERVER_INFO *pServer_info, CLIENT_INFO *pEntry);
void _netsvr_print_client_entry( SERVER_INFO *pServer_info);
guint _netsvr_get_convert_flag(void);
CLIENT_INFO *_netsvr_find_client_setup( SERVER_INFO *pServer_info);

int _client_find_streamid_by_pclient( CLIENT_INFO *info);
int _client_set_streamid( CLIENT_INFO *info);
void _client_unset_streamid( CLIENT_INFO *info, int playid );

JOB_INFO *_client_new_job(CLIENT_INFO *pClient_info, int msg_id, void *msg, int msg_len );
void _client_free_job(JOB_INFO *pJobEntry, int in_queue);
int _client_enque_job(JOB_INFO *pJobEntry);
JOB_INFO *_client_enque_job_simple(CLIENT_INFO *pClient_info, int msg_id, void *msg, int msg_len );
JOB_INFO *_client_peek_job(CLIENT_INFO *info, int req_msg_id);

char *_str_dr_proto(int  prot);
char *_str_dr_proto_inform( int  prot );

int _sock_set_timeout(int sd, unsigned int sec);
int _send_controlframe(int ds, int type);

int _client_broadcast_msg(char *msg, int size);
void _client_stat_sock_buff(CLIENT_INFO *pClient);
unsigned char _client_get_mode(CLIENT_INFO *pClient);
void _client_set_mode(CLIENT_INFO *pClient, unsigned char mode);
int _client_eventlog_put(CLIENT_INFO *pClient, gint type, gint param1, gint param2, gchar *text);

// for nf_netsvr_live.c
void _livemgr_init_fps_data ( CLIENT_INFO *pClient );
int _livemgr_close();
int _livemgr_init();
int _livemgr_put_frame( gpointer frame);
guint _livemgr_reload_handoff(guint ch_mask);

// for nf_netsvr_websvr.c
int create_webserver(void);


// for nf_netsvr_ddns.c
int create_ddns(void);
int port_test(char *host, unsigned short port);
int port_test_netsvr( int is_ddns );
int port_test_websvr( int is_ddns );

typedef struct _NF_DDNS_STATUS_T {

	GTimeVal	update_tv;
	GTimeVal	update_success_tv;

	guint		status;
	
	gchar		ip_addr[128];	
	gchar		host_addr[128];

	gchar		save_ip[256];	
	gchar		get_server_ip[256];
	gchar		get_dns_ip[256];

	
	// ddns thread internal status	
	guint		server_idx;
		
	guint		thread_state;
	guint		thread_sleep_sec;
	guint		thread_update_time;
	guint		dns_error_count;
	guint		getIP_error_count;
	guint		force_update_count;
	guint		update_error_count;
		
} NF_DDNS_STATUS;

int nf_get_ddns_disk_count(void);
void nf_set_ddns_disk_count(int disk_count);

int ddns_get_hostaddr(  );
int ddns_get_status( NF_DDNS_STATUS *status );
int ddns_force_register();

typedef struct _NF_DDNS_COMMON_REG_PARAM_T {
	char ddns_server[256];			
	
	char hostname[256];		
	char username[256];
	char passwd[256];			
} NF_DDNS_COMMON_REG_PARAM;	

int ddns_common_force_register( NF_DDNS_COMMON_REG_PARAM *param);

int nf_ddns_get_status(void);

/* -------------------------------------------------------------------------- */

char *my_inet_ntoa_r(struct in_addr ina, char *buf, int buf_len);
int my_getaddrinfo(const char *host, struct sockaddr_in *ret_addr, int dump_all);
int my_gethostbyname(const char *host, struct sockaddr_in *ret_addr);
int connect_timeout(const SA *saptr, socklen_t salen, int usec);
int connect_timeout_hostname(const char *host, const int port, const int usec);

/******************************************************************************/

/* type macro */
#define NF_TYPE_NETWORK					(nf_network_get_type ())

#define NF_IS_NETWORK(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_NETWORK))
#define NF_IS_NETWORK_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_NETWORK))

#define NF_NETWORK_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_NETWORK, NfNetworkClass))
#define NF_NETWORK(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_NETWORK, NfNetwork))
#define NF_NETWORK_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_NETWORK, NfNetworkClass))

#define NF_NETWORK_CAST(obj)			((NfNetwork*)(obj))
#define NF_NETWORK_CLASS_CAST(klass)	((NfNetworkClass*)(klass))

typedef struct _NfNetwork 		NfNetwork;
typedef struct _NfNetworkClass 	NfNetworkClass;

/**
 * NfNetwork:
 *
 * NfDVR network class
 */
struct _NfNetwork {
	NfObject 	 	object;
	
	/*< public >*/	
	gint			init_done;
	
	GAsyncQueue		*queue;	

	GThread			*thread;	
	gint			thread_run;
	gint			thread_status;

	// for audio
	GAsyncQueue		*audio_queue;	
	GThread			*audio_thread;	
	gint			audio_thread_run;
	gint			audio_thread_status;

	// client_accept
	GThread			*accept_thread;
	gint			 accept_thread_run;
	gint			 accept_thread_status;
								
	// ddns
	// webserver	

	// per client_thread
										
	/*< public >*/ /* with LOCK */

	/*< private >*/	
};

struct _NfNetworkClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

gboolean 
nf_network_init(int wait);


typedef enum _NF_NETWORK_PORT_TEST_ADDRESS_E {
	NF_NETWORK_PORT_TEST_ADDRESS_LOCAL  = 0,
	NF_NETWORK_PORT_TEST_ADDRESS_DDNS	= 1,
	NF_NETWORK_PORT_TEST_ADDRESS_DDNS_SVR	= 2
} NF_NETWORK_PORT_TEST_ADDRESS_E;

gboolean 
nf_network_port_test_netsvr( NF_NETWORK_PORT_TEST_ADDRESS_E is_ddns, GError **error );

gboolean 
nf_network_port_test_websvr( NF_NETWORK_PORT_TEST_ADDRESS_E is_ddns, GError **error );


typedef enum _NF_DISCONN_REASON_E {

	NF_DISCONN_SVR_IP_CHANGE	= 0x00010001,
	NF_DISCONN_SVR_DISK_FORMAT	= 0x00010002,
	NF_DISCONN_SVR_POWER_OFF	= 0x00010003,
	NF_DISCONN_SVR_TIME_CHANGE	= 0x00010004,
	NF_DISCONN_SVR_FW_UPGRADE	= 0x00010005,
	NF_DISCONN_SVR_FACTORY_DEFAULT	= 0x00010006,
	NF_DISCONN_SVR_DISK_MANAGEMENT	= 0x00010007,  
	NF_DISCONN_SVR_USER_MANAGEMENT	= 0x00010008,
	NF_DISCONN_SVR_SYSDB_LOAD		= 0x00010009,
#if defined(__ARCHIVING_DISCONNETC__)
	NF_DISCONN_SVR_ARCHIVE			= 0x0001000A,
	NF_DISCONN_SVR_NR				= 10, // must update  _dr_disconnect() g_assert
#else
	NF_DISCONN_SVR_NR				= 9, // must update  _dr_disconnect() g_assert
#endif
	NF_DISCONN_CLI_ADMIN 		= 0x00020001,
	NF_DISCONN_CLI_MANAGER 		= 0x00020002,
	NF_DISCONN_CLI_USER 		= 0x00020003,
	NF_DISCONN_CLI_NR			= 3	
} NF_DISCONN_REASON_E;

gboolean 
nf_network_start(void );

gboolean 
nf_network_stop( NF_DISCONN_REASON_E reason );

guint	nf_network_get_stop_reason();
guint nf_network_get_webra_audio_status(void);
void nf_network_set_webra_audio_status(guint val);

int nf_network_notify_net_status(int conn, int live, int play);

gboolean nf_network_onestop_test(void);

typedef enum
{
  RES_UPNP_SUCCESS_FORCE = 100,	
  RES_UPNP_SUCCESS = 101,
  RES_UPNP_IGD_NOT_PORT_FWD = 102,
  RES_UPNP_DEV_NOT_IGD = 103,
  RES_UPNP_EQUAL_PORT = 104,
  RES_UPNP_PORT_DEL_FAIL = 105,
  RES_UPNP_NOT_IGD = 106,
  RES_UPNP_NOT_DEV = 107,
  RES_UPNP_PORT_DEL_ERROR = 108,
  RES_UPNP_RESULT,
}upnp_result_type;

typedef enum _NF_UPNP_INFO{
	UPNP_INIT 		= 0,
	UPNP_GOOD,
	UPNP_FORCE,
	UPNP_ERR_NET,	
	UPNP_ERR_IGD,
	UPNP_ERR_UPDATE,
	UPNP_STOP,	
	UPNP_MAX
} NF_UPNP_INFO;

NF_UPNP_INFO upnp_force_register();

gboolean upnp_is_conflict();	
int upnp_set_status(NF_UPNP_INFO set);
NF_UPNP_INFO nf_upnp_get_status(void);

int create_upnp(void);

int nf_upnp_port_forwording(int int_port, int desc_type, char *conn_addr);
int nf_upnp_port_delete(int int_port, int desc_type);

int nf_upnp_find_igd(void);
int nf_upnp_is_use_port(int int_port);

typedef enum _UPNP_PORT
{
	PORT_ERROR		= -1,
	PORT_USE_NOT	= 0,
	PORT_USE_OTHER,
	PORT_USE_ME,
	PORT_MAX
}UPNP_PORT;

int nf_upnp_get_status_port(int int_port, int type);

gboolean nf_network_dhcp_renew(void);

gboolean nf_network_get_mac_conflict_ipset(NF_NETIF_MAC *ret_mac);
gboolean nf_network_get_mac_conflict_ipcam(guint ch, NF_NETIF_MAC *ret_mac);

#endif
