#include <sys/types.h>   /* basic system data types */
#include <sys/ioctl.h>
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>        /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>       /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>     /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>      /* for Unix domain sockets */

#include <glib.h>

#include "unp.h"
#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_dspcomm_app.h"

#include "nf_dspcomm_ioctl.h"		// from driver
 
//#define DEBUG_DSPCOMM_COMMAND

#if defined(_NF_1648)||defined(_NF_0824)
	#define ANF_RETURN(val)

#else   /**/
	#define ANF_RETURN(val)     return (val);

#endif  /**/


#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "dspcomm"

static int dspcomm_fd[NF_DSPCOMM_MAX_DSP][NF_DSPCOMM_MAX_CHAN] = { {0,}, {0,} };

int nf_dspcomm_open_chan(int dspid, int type)
{
	int devid = 0, fd;
	unsigned char tmp_str[256];
			
	g_return_val_if_fail ( dspid >= 0 && dspid < NUM_ACTIVE_DSP, -1);	
	g_return_val_if_fail ( type >= 0 && type < NF_DSPCOMM_MAX_DEV, -2);	
	g_return_val_if_fail ( dspcomm_fd[dspid][type] == 0, -3);	
			
	devid = (NF_DSPCOMM_MAX_CHAN*dspid) + type;
	
	sprintf(tmp_str, "/dev/dspcomm%d", devid );

	fd = Open(tmp_str, O_RDWR, 0);
	if(fd<0)
	{
		g_warning("%s open failed [%s]",__FUNCTION__, tmp_str) ;// error
	}
	else
	{
		dspcomm_fd[dspid][type] = fd;
		g_message("%s open %s fd[%d]", __FUNCTION__, tmp_str, fd);
	}			
	return fd;					
}

int nf_dspcomm_close_chan(int dspid, int type)
{
	int ret;
		
	g_return_val_if_fail ( dspid >= 0 && dspid < NUM_ACTIVE_DSP, -1);	
	g_return_val_if_fail ( type >= 0 && type < NF_DSPCOMM_MAX_DEV, -2);	
	g_return_val_if_fail ( dspcomm_fd[dspid][type] > 0, -3);

	ret = Close(dspcomm_fd[dspid][type]);
	if(ret<0)
	{
		// error	
	}	
	else
	{
		dspcomm_fd[dspid][type] = 0;
	}											
	return ret;
}

int nf_dspcomm_send_cmd(int dspid, int type, int code, void *data, int len, int nb)
{
	int fd, clen=0, ret;
	char buff[MAX_CMD_BUFF];
	
	DPACKET *pHeader =(DPACKET *)buff;
	char *pData = ((char *)buff) + sizeof(DPACKET);		
	
	int ch = NF_DSPCOMM_CH_COMMAND;

    ANF_RETURN(0)
    
#ifdef DEBUG_DSPCOMM_COMMAND
	g_message("%s dspid[%d] type[%d] code[%d] len[%d]", __FUNCTION__,
					 dspid, type, code, len);
#endif

	g_return_val_if_fail ( dspid >= 0 && dspid < NUM_ACTIVE_DSP, -1);	
	g_return_val_if_fail ( type == DPT_REQUEST, -2);	
	g_return_val_if_fail ( data != NULL, -3);	
	g_return_val_if_fail ( len >0 && len < sizeof(buff), -4);	
			
	pHeader->type = (unsigned char)type;
	pHeader->code = (unsigned char)code;
	pHeader->dlen = (unsigned short)len;		
	memcpy(pData, data, len);

	clen = NF_DSPCOMM_ALIGN_SIZE( (len + sizeof(DPACKET) ), NF_DSPCOMM_READ_SIZE );

	if( dspcomm_fd[dspid][ch] > 0 )					
	{
		fd = dspcomm_fd[dspid][ch];
	}else{
		fd = nf_dspcomm_open_chan(dspid, ch);
	}	

	g_return_val_if_fail ( fd > 0, -99);
	
	ret = Writen(fd, buff, clen);
	if(ret != clen)
		return -1;
	else
		return 	1;
}

int nf_dspcomm_recv_cmd(int dspid, int ch, void *data, int nb)
{
	int fd, clen=0, ret, tot_read = 0;
		
	DPACKET *pHeader =(DPACKET *)data;
	char *pData = data+NF_DSPCOMM_READ_SIZE;		

    ANF_RETURN(0)
    
#ifdef DEBUG_DSPCOMM_COMMAND
	g_message("%s dspid[%d] ch[%d] len[%d]", __FUNCTION__, dspid, ch, len);
#endif
	
	g_return_val_if_fail ( dspid >= 0 && dspid < NUM_ACTIVE_DSP, -1);		
	g_return_val_if_fail ( data != NULL, -3);	

	if( dspcomm_fd[dspid][ch] > 0 )					
	{
		fd = dspcomm_fd[dspid][ch];
	}else{
		fd = nf_dspcomm_open_chan(dspid, ch);
	}	
	
	g_return_val_if_fail ( fd >0 , -99);	

	ret = Readn(fd, data, NF_DSPCOMM_READ_SIZE);	
	if(ret != NF_DSPCOMM_READ_SIZE)
	{
		g_warning("err Readn 1 ret[%d]", ret);
		return -11;
	}	
	tot_read = ret;
		
	clen = pHeader->dlen + sizeof(DPACKET);
	clen = NF_DSPCOMM_ALIGN_SIZE( clen , NF_DSPCOMM_READ_SIZE ) - NF_DSPCOMM_READ_SIZE;
	
	g_return_val_if_fail ( clen < MAX_CMD_BUFF, -98);	
		
	if( clen > 0 )
	{
		ret = Readn(fd, pData, clen);	
		if(ret != clen)
		{
			g_warning("err Readn 2 ret[%d]", ret);
			return -12;
		}
		tot_read += ret;					
	}
	
	return	tot_read;		
}



int nf_dspcomm_recv_cmd_malloc(int dspid, int ch, void **data, int nb)
{
	int fd, clen=0, ret, tot_read = 0;

	char 	tmp[NF_DSPCOMM_READ_SIZE];	
	DPACKET *pHeader =(DPACKET *)tmp;
	char	*pData = NULL, *pBuff = NULL;

    ANF_RETURN(0)
    
#ifdef DEBUG_DSPCOMM_COMMAND
	g_message("%s dspid[%d] ch[%d] len[%d]", __FUNCTION__, dspid, ch, len);
#endif
	
	g_return_val_if_fail ( dspid >= 0 && dspid < NUM_ACTIVE_DSP, -1);		
	g_return_val_if_fail ( data != NULL, -3);	

	if( dspcomm_fd[dspid][ch] > 0 )					
	{
		fd = dspcomm_fd[dspid][ch];
	}else{
		fd = nf_dspcomm_open_chan(dspid, ch);
	}	
	
	g_return_val_if_fail ( fd >0 , -99);	

	memset(tmp, 0x00, NF_DSPCOMM_READ_SIZE);
	
	ret = Readn(fd, pHeader, NF_DSPCOMM_READ_SIZE);	
	if(ret != NF_DSPCOMM_READ_SIZE)
	{
		g_warning("err Readn 1 ret[%d]", ret);
		return -11;
	}	
	tot_read = ret;
		
	clen = pHeader->dlen + sizeof(DPACKET);
	clen = NF_DSPCOMM_ALIGN_SIZE( clen , NF_DSPCOMM_READ_SIZE );
	
	pBuff = g_malloc(clen);
	g_return_val_if_fail (pBuff , -98);	
	
	memcpy(	pBuff, pHeader, NF_DSPCOMM_READ_SIZE );

	pData = pBuff + NF_DSPCOMM_READ_SIZE;	
	clen -= NF_DSPCOMM_READ_SIZE;
	
	if( clen > 0 )
	{
		ret = Readn(fd, pData, clen);	
		if(ret != clen)
		{
			g_warning("err Readn 2 ret[%d]", ret);
			g_free(	pBuff);			
			return -12;
		}
		tot_read += ret;					
	}

	*data = pBuff;			
	return	tot_read;		
}


int nf_dspcomm_ioctl_read( NF_DSPCOMM_IOCTL_COMM *ioctl_buff )
{
	int fd, ret;
			
	g_return_val_if_fail (ioctl_buff, 0);
	
#ifdef DEBUG_DSPCOMM_COMMAND
	g_message("%s dspid[%d] ch[%d] addr[0x%08x] len[%d]", __FUNCTION__, 
			ioctl_buff->dsp_id, 
			ioctl_buff->addr, 
			ioctl_buff->len);
#endif

	g_return_val_if_fail ( ioctl_buff->dsp_id >= 0 && ioctl_buff->dsp_id < NUM_ACTIVE_DSP , -1);		
	g_return_val_if_fail ( ioctl_buff->len < sizeof(ioctl_buff->buff), -2);	
		
	if( dspcomm_fd[ioctl_buff->dsp_id][NF_DSPCOMM_CH_COMMAND] > 0 )
	{
		fd = dspcomm_fd[ioctl_buff->dsp_id][NF_DSPCOMM_CH_COMMAND];
	}else{
		fd = nf_dspcomm_open_chan(ioctl_buff->dsp_id, NF_DSPCOMM_CH_COMMAND);
	}	
	
	g_return_val_if_fail ( fd >0 , -99);
					
	return	Ioctl( fd, DSPCOMM_IOCTL_DEBUG_READ, ioctl_buff);
			
}

static GTimeVal _sync_time = { NF_DATETIME_MIN,0};

GTimeVal nf_dspcomm_get_last_sync_time()
{	
	return _sync_time;
}

GTimeVal nf_dspcomm_get_time( gint dsp_id )
{	
	GTimeVal	tval = {0,0};

#if defined(_ANF_1648) || defined(_ATM_0824) || defined(_ATM_1624)
	g_get_current_time(&tval);
#else
	NF_DSPCOMM_IOCTL_COMM ioctl_comm;
	guint64 	time_msec = 0, time_nsec = 0;
	int r;
	
	g_return_val_if_fail( dsp_id < NUM_ACTIVE_DSP, tval );
	
	memset( ioctl_comm.buff, 0x00, NF_DSPCOMM_TICK_SIZE);		
			
	ioctl_comm.dsp_id = dsp_id;
	ioctl_comm.addr = NF_DSPCOMM_TICK_ADDR;
	ioctl_comm.len = NF_DSPCOMM_TICK_SIZE;

	r = nf_dspcomm_ioctl_read( &ioctl_comm );
	if(!r)
	{			
		time_msec = (*(guint64 *)ioctl_comm.buff);
		time_nsec = time_msec * 1000000;
		
		GUINT64_TO_GTIMEVAL( time_nsec, tval );
	}		
#endif 	
	return tval;	
}

gboolean nf_dspcomm_sync_time()
{	
	guint64	req;
	GTimeVal tval;
	int i;
	
#if defined(_ANF_1648) || defined(_ATM_0824) || defined(_ATM_1624)
	gettimeofday(&tval, NULL);
	_sync_time = tval;
#else			
	memset(&req, 0x00, sizeof(guint64));						
	for(i=0; i<NUM_ACTIVE_DSP ; ++i)
	{
		gettimeofday(&tval, NULL);
		req = GTIMEVAL_TO_GUINT64(tval);					
		nf_dspcomm_send_cmd(i, NF_DSPCOMM_CH_COMMAND, DP_SET_TIME, &req, sizeof(guint64), 0);
	}
		
	_sync_time = tval;
	
	g_message("%s tv_sec[%d]usec[%d]", __FUNCTION__, (int)tval.tv_sec, (int)tval.tv_usec);	
#endif			
	return 1;
}

