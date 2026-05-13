#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

#include "nf_common.h"

#include <endian.h>
#include "nf_util_netif.h"
#include "nf_issm_ctl.h"
#include "nf_network.h"

typedef struct _NF_UPNP_STATUS_T {
	gint 		link_status;
	gboolean	refresh;
	guint		status;
	guint		save_ip;	
} NF_UPNP_STATUS;

static NF_UPNP_STATUS _upnp_status;

gboolean upnp_is_conflict()
{
	struct stat buf;
		
	return ( stat( "/NFDVR/data/upnp_conflict", &buf ) == 0) ? 1:0;
}

static void _upnp_conflict_set( gboolean is_set )
{
	gboolean conflict;	
	
	conflict = upnp_is_conflict();
	
	if(is_set)
	{
		if( !conflict )
		{
			proxy_system("/bin/touch /NFDVR/data/upnp_conflict",1,3);
			proxy_system("/bin/sync",1,3);
		}
	}
	else
	{
		if( conflict )
		{		
			proxy_system("/bin/rm -f /NFDVR/data/upnp_conflict",1,3);
			proxy_system("/bin/sync",1,3);
		}
	}	
}

void nf_upnp_reset_conflict(void)
{
	_upnp_conflict_set(FALSE);
}
	
int upnp_set_status(NF_UPNP_INFO set)
{
	g_message("UPNP - %s - set:%d", __FUNCTION__, set);
	
	_upnp_status.status = set;

	return 0;
}

NF_UPNP_INFO nf_upnp_get_status(void)
{
	return _upnp_status.status;
}

int _update_internal_ip()
{
	NF_NETIF_GET_INFO ret_net_info;
	unsigned int cur_ip = 0;
	int ret = 0;

    memset(&ret_net_info, 0x00, sizeof(ret_net_info));
    nf_netif_get_info(&ret_net_info);
                
    cur_ip = ret_net_info.ipaddr;
	
	if( _upnp_status.save_ip != cur_ip )
	{
		_upnp_status.save_ip = cur_ip;

		return 1;
	}
	else
		return 0;
}

gboolean _is_support_upnp(void)
{
	guint val;

	val = nf_sysdb_get_uint("net.proto.autoport");

	if( val == 1 )
		return TRUE;
	else
		return FALSE;
}

int _upnp_update(gboolean *is_conflict)
{
	guint port = 0;
	int ret = 0;
	int n =0;
	char buf[256];
	
	memset(buf, 0x00, sizeof(buf));

	*is_conflict = FALSE;

	port = nf_sysdb_get_uint("net.rtp.rtspport");
	n = nf_upnp_port_forwording(port, 0, buf);

	if( n == RES_UPNP_SUCCESS_FORCE)
	{
		*is_conflict = TRUE;
		n = RES_UPNP_SUCCESS;
	}
	
	if( n != RES_UPNP_SUCCESS)
	{
		if( (n == RES_UPNP_NOT_IGD) || (n == RES_UPNP_NOT_DEV) )
			ret = -1;
		else
			ret = -2;

		return ret;
	}
/*	
	ret = nf_upnp_get_status_port(port, 0);
	
	if(ret == PORT_ERROR)
		return -1;
	else if( ret != PORT_USE_ME )
	{
		int n = 0;
		
		n = nf_upnp_port_forwording(port, 0, buf);
		if( n != RES_UPNP_SUCCESS)
			return -2;
	}
*/
	sleep(2);

	memset(buf, 0x00, sizeof(buf));

	port = nf_sysdb_get_uint("net.proto.webport");	
	n = nf_upnp_port_forwording(port, 1, buf);

	if( n == RES_UPNP_SUCCESS_FORCE)
	{
		*is_conflict = TRUE;
		n = RES_UPNP_SUCCESS;
	}
	
	if( n != RES_UPNP_SUCCESS && n != RES_UPNP_SUCCESS_FORCE)
	{
		if( (n == RES_UPNP_NOT_IGD) || (n == RES_UPNP_NOT_DEV) )
			ret = -1;
		else
			ret = -2;

		return ret;
	}
/*
	ret = nf_upnp_get_status_port(port, 1);

	if(ret == PORT_ERROR)
		return -1;
	else if( ret != PORT_USE_ME )
	{
		int n = 0;
		
		n = nf_upnp_port_forwording(port, 1, buf);	
		if( n != RES_UPNP_SUCCESS)
			return -2;	
	}
*/

	return 0;
}

NF_UPNP_INFO upnp_force_register(void) {
	int ret = UPNP_GOOD;

	if( _upnp_status.link_status )
	{
		int send_ret;
		gboolean is_conflict;
		
		send_ret = _upnp_update(&is_conflict);

		if(is_conflict)
			ret = UPNP_FORCE;
		else
		{		
			if( send_ret == 0 )
				ret = UPNP_GOOD;		
			else if( send_ret == -1 )
				ret = UPNP_ERR_IGD;
			else if( send_ret == -2 )		
				ret = UPNP_ERR_UPDATE;
		}
	}
	else
	{
		ret = UPNP_ERR_NET;
	}

	upnp_set_status(ret);

	return ret;
}

#define S1_UPNP_LOOP_TIME	60

void upnp_func(void *arg)
{
	guint loop_timer = S1_UPNP_LOOP_TIME;
	
	while(1)
	{		
		if( (loop_timer >= S1_UPNP_LOOP_TIME) || (_upnp_status.refresh == 1) )
		{
			g_message("UPNP - %s - loop_timer", __FUNCTION__);
			
			loop_timer = 0;
			_upnp_status.refresh = 0;

			if( _is_support_upnp() == TRUE )
			{				
				if( _upnp_status.link_status )
				{
					int send_ret;
					gboolean is_conflict;

					send_ret = _upnp_update(&is_conflict);
					if(is_conflict)
						_upnp_conflict_set(TRUE);

					if( send_ret == 0 )
						upnp_set_status(UPNP_GOOD);					
					else if( send_ret == -1 )
						upnp_set_status(UPNP_ERR_IGD);
					else if( send_ret == -2 )		
						upnp_set_status(UPNP_ERR_UPDATE);				
				}
				else
				{	
					upnp_set_status(UPNP_ERR_NET);
				}
			}
			else
			{
				upnp_set_status(UPNP_STOP);				
			}
		}
		else
		{
			g_message("UPNP - %s - sleep", __FUNCTION__);
			loop_timer++;
			sleep(1);
		}
	}
}

static void _upnp_net_rxtx_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{	
	g_return_if_fail(pinfo != NULL);

	if( _upnp_status.link_status !=  pinfo->d.params[3] ){				
		if( pinfo->d.params[3] ){
			
			_upnp_status.refresh = 1;
		}
		
		_upnp_status.link_status = pinfo->d.params[3];				
	}
	
}

static void _upnp_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{	
	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_NET){
		g_message("%s called", __FUNCTION__);
		_upnp_status.refresh = 1;
	}
}

int create_upnp(void)
{
	pthread_t tid;	
	gulong cb_handle = 0;

	memset( &_upnp_status, 0x00, sizeof( NF_UPNP_STATUS ));	
	
	// or notify_get_param_3
	nf_netif_get_link_status(nf_netif_get_eth_str(), &_upnp_status.link_status );
		
	cb_handle= nf_notify_connect_cb( "net_rxtx", _upnp_net_rxtx_cb_func , (gpointer)NULL );
	g_message("%s net_rxtx connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);	
		
	cb_handle= nf_notify_connect_cb( "sysdb_change", _upnp_sysdb_reload_cb_func , (gpointer)NULL );
	g_message("%s sysdb_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);	

	if(Pthread_create(&tid, NULL, upnp_func, NULL)) {	
		g_warning("%s : thread create error", __FUNCTION__);
		return -1;		  
	}		 

	Pthread_detach(tid);

	return 0;
}
