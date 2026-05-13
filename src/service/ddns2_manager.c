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
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <glib.h> 
#include <glib/gprintf.h>
#include "unp.h"
#include "unpthread.h"

#include "nf_util_netif.h"
#include "ddns2_md5.h"
#include "ddns2_manager.h"
#include "nf_api_disk.h"

#define DDNS2_PORT 3000
#define DYNDNS_PORT 80

#define DDNS2_IP "211.192.36.88"
//#define DDNS2_ITX_GET_MY_IP "222.122.199.213"
#define DDNS2_ITX_GET_MY_IP "222.122.199.211"
//#define DDNS2_DYN_GET_MY_IP "91.198.22.71"
#define DDNS2_DYN_GET_MY_IP "91.198.22.70"

#define DDNS2_HOST_NAME "dvrlink.net"

#define ITX_CHECKIP_NAME "myip.dvrlink.net"
#define DYNDNS_CHECKIP_NAME "checkip.dyndns.com"

#define DDNS2_RET_CHECK "HTTP/1.1"
#define GET_MYIP_CHECK "Current IP Address"

#define DDNS2_KEY "itxchlrhdi"

/******************************************************************************* 
* Function  :
* Prototype : 
* Arguments : 
* Return : 
******************************************************************************/ 
static void sleep_ex(long usec)
{
	struct timespec req, rem;

	if (usec < 0)
		return;

	req.tv_sec  = (usec / 1000000);
	req.tv_nsec = (usec % 1000000) * 1000;

__do_sleep__:
	if (nanosleep(&req, &rem) < 0) {
		if (errno == EINTR) {
			req = rem;
			goto __do_sleep__;
		}
	}

	return;
}


LPCSTR base16(void *s_, UINT n)
{
	static char buf[256];
	char *d = buf;
	char *s = (char *)s_;
	UINT i;
	//	g_assert(2*n < sizeof(buf));
	for (i = 0; i < n; i++) {
		static LPCSTR cs = "0123456789abcdef";
		*d++ = cs[(*s >> 4) & 0x0F];
		*d++ = cs[(*s++) & 0x0F];
	}
	*d++ = 0;
	return buf;
}

int get_myIP_connect(const SA *saptr, socklen_t salen, int msec, int sockfd)
{
	int				flags, n, ret, try, res;
	fd_set 			myset; 
	struct 			timeval tv; 
	int 			valopt; 
	socklen_t 		lon; 
	
	if(flags = Fcntl(sockfd, F_GETFL, 0) < 0)
	{
		g_printf("Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		return -2;
	}
	if(Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		g_printf("Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		return -2;
	}

	if(msec > 10)
		try = msec/10; // 500msec
	else
		try = 20;		
	//		try = 50;

	while(try--) {	
		if ( (n = connect(sockfd, (struct sockaddr *)saptr, salen)) < 0)
		{	
			g_printf("%s : connect errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));

			if(errno == EINPROGRESS) 
			{
				tv.tv_sec = 20; 
				tv.tv_usec = 0; 
				FD_ZERO(&myset); 
				FD_SET(sockfd, &myset); 
				res = select(sockfd+1, NULL, &myset, NULL, &tv); 

				if (res < 0 && errno != EINTR) 
				{ 
					g_printf("Error connecting %d - %s\n", errno, strerror(errno)); 
					ret = -1;
					goto done;
				} 
				else if (res > 0) 
				{ 
					// Socket selected for write 
					lon = sizeof(int); 
					if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) 
					{ 
						g_printf("Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
						ret = -1;
						goto done;
					} 
					// Check the value returned... 
					if (valopt) 
					{ 
						g_printf("Error in delayed connection() %d - %s\n", valopt, strerror(valopt)); 
						ret = -1;
						goto done;
					} 
					ret = 1;
					goto done;
				} 
				else 
				{ 
					g_printf("Timeout in select() - Cancelling!\n"); 
					ret = -1;
					goto done;
				} 
			}
			else if(errno == EALREADY)
			{
				g_printf("Error connecting %d - %s\n", errno, strerror(errno)); 
			}
			else
			{
				ret = -2; 				
				g_printf("%s : connect errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
				goto done;
			}
		}
		else
		{
			ret = 1;
			goto done;
		}		
		g_printf("connect_test: connect fail... sleep 2 SEC~~!!!\n");
		sleep_ex(2000000); //2000msec
	}
	ret = -3; //timeout

done:	
	return ret;
} 

int ddns2_connect(const SA *saptr, socklen_t salen, int msec, int sockfd)
{
	int				flags, n, ret, try;


	flags = Fcntl(sockfd, F_GETFL, 0);
	Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	if(msec > 10)
		try = msec/10; // 500msec
	else
		try = 20;		

	while(try--) {	
		if ( (n = connect(sockfd, (struct sockaddr *)saptr, salen)) < 0)
		{	
			if ( !(errno == EINPROGRESS || errno == EALREADY ) )
			{
				//#define EALREADY    114 /* Operation already in progress */
				//#define EINPROGRESS 115 /* Operation now in progress */				
				ret = -2; 				
				g_printf("%s : connect errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
				goto done;
			}			
			else
			{
				g_printf("%s : errno == EINPROGRESS or EALREADY.  errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
			}
		}
		else
		{
			ret = 1;
			//printf("try[%d] n[%d]\n", try, n);
			goto done;
		}		
		//printf("[%d]\n",try);				
		g_printf("connect_test: connect fail... sleep 2 SEC~~!!!\n");
		sleep_ex(2000000); //2000msec
		//		sleep_ex(500000); //10msec
	}
	ret = -3; //timeout

done:	
	//	Close(sockfd);	
	return ret;	// ok
}




void ddns2_hmac_md5(void *dest, char *key, char *msg)
{
#define M	MD5_BLOCK_SIZE
#define N	MD5_DEST_SIZE
	char key_[N];
	UINT keyLen = strlen(key);
	UINT i;
	if (keyLen > M) {
		Md5(key, key_, keyLen);
		key = key_;
		keyLen = N;
	}


	UINT msgLen = strlen(msg);

	char *ipad = (char *)malloc(M + msgLen);
	for (i = 0; i < M; i++) {
		ipad[i] = 0x36 ^ (i < keyLen ? key[i] : 0);
	}
	memcpy(ipad + M, msg, msgLen);
	Md5(dest, ipad, M + msgLen);
	free(ipad);

	char opad[M + N];
	for (i = 0; i < M; i++) {
		opad[i] = 0x5c ^ (i < keyLen ? key[i] : 0);
	}
	memcpy(opad + M, dest, N);
	Md5(dest, opad, M + N);
#undef M
#undef N
}

#if 0
static void md5_hash( char **v, int count, char *hash )
{
	struct MD5Context md5;
	int i;
	unsigned char bin[16];

	MD5Init( &md5 );
	for( i = 0; i < count; ++i )
	{
		if( i > 0 ) MD5Update( &md5, (unsigned char *)":", 1 );
		MD5Update( &md5, (unsigned char *)v[i], strlen( v[i] ) );
	}
	MD5Final( bin, &md5 );
	for( i = 0; i < 16; ++i ) sprintf( hash + (i<<1), "%02x", bin[i] );
	hash[32] = 0;
}
#endif

int html_wait_read(int fd, int timeout_sec)
{	
	fd_set rxset, wkset;
	int mx, n;
	struct timeval tv;

	FD_ZERO(&rxset); FD_ZERO(&wkset);

	FD_SET(fd, &rxset);
	FD_SET(fd, &rxset);

	mx = fd ;

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

		if(FD_ISSET(fd, &wkset))
			return 1;
		else if(FD_ISSET(fd, &wkset))
			return 2;
	}	
	return 0;
}

void convert_mac_str( NF_NETIF_MAC *mac_addr, char *ret_str)
{
#define COVERT_CHAR(hex_val) (( hex_val >= 10 ) ? (hex_val - 10) + 'A' : hex_val + '0')

	char string_mac_addr[13];

	unsigned int hex_val = 0;

	hex_val = (unsigned int)((mac_addr->mac_addr[0] & 0xf0) >> 4);
	string_mac_addr[0] = COVERT_CHAR(hex_val);
	hex_val = (unsigned int)(mac_addr->mac_addr[0] & 0x0f);
	string_mac_addr[1] = COVERT_CHAR(hex_val);

	hex_val = (unsigned int)((mac_addr->mac_addr[1] & 0xf0) >> 4);
	string_mac_addr[2] = COVERT_CHAR(hex_val);
	hex_val = (unsigned int)(mac_addr->mac_addr[1] & 0x0f);
	string_mac_addr[3] = COVERT_CHAR(hex_val);

	hex_val = (unsigned int)((mac_addr->mac_addr[2] & 0xf0) >> 4);
	string_mac_addr[4] = COVERT_CHAR(hex_val);
	hex_val = (unsigned int)(mac_addr->mac_addr[2] & 0x0f);
	string_mac_addr[5] = COVERT_CHAR(hex_val);

	hex_val = (unsigned int)((mac_addr->mac_addr[3] & 0xf0) >> 4);
	string_mac_addr[6] = COVERT_CHAR(hex_val);
	hex_val = (unsigned int)(mac_addr->mac_addr[3] & 0x0f);
	string_mac_addr[7] = COVERT_CHAR(hex_val);

	hex_val = (unsigned int)((mac_addr->mac_addr[4] & 0xf0) >> 4);
	string_mac_addr[8] = COVERT_CHAR(hex_val);
	hex_val = (unsigned int)(mac_addr->mac_addr[4] & 0x0f);
	string_mac_addr[9] = COVERT_CHAR(hex_val);

	hex_val = (unsigned int)((mac_addr->mac_addr[5] & 0xf0) >> 4);
	string_mac_addr[10] = COVERT_CHAR(hex_val);
	hex_val = (unsigned int)(mac_addr->mac_addr[5] & 0x0f);
	string_mac_addr[11] = COVERT_CHAR(hex_val);

	string_mac_addr[12] = 0;

	// g_printf("convert_mac_str: source_str[%s],  dest_str[%s] \n",mac_addr->mac_addr, string_mac_addr);

	strncpy(ret_str, string_mac_addr, 12);
}

void ddns2_send_message_make(char *send_message, DDNS2_PARRAM *send_par)
{
	NF_NETIF_MAC mac_addr;
	char temp_char[128];
	char mac_str[16] = {0,};

	if(strlen(send_par->owner_name) != 0)
	{
		memset(temp_char, 0x00, sizeof(temp_char));
		sprintf(temp_char,"obj%%5bowner_name%%5d=%s&",send_par->owner_name);
		strcat(send_message, temp_char);
	}

	if (nf_netif_get_mac(&mac_addr) == TRUE)
	{
		memset(temp_char, 0x00, sizeof(temp_char));

		convert_mac_str(&mac_addr, mac_str);    
		sprintf(temp_char,"obj%%5bmac_addr%%5d=%s&",mac_str); 
		//    sprintf(temp_char,"obj%5bmac_addr%%5d=%s&","00115f000167"); 

		strcat(send_message, temp_char);
	}

	if(strlen(send_par->host_name) != 0)
	{
		memset(temp_char, 0x00, sizeof(temp_char));
		sprintf(temp_char,"obj%%5bname%%5d=%s&",send_par->host_name);
		strcat(send_message, temp_char);
	}

	if(send_par->video_type == 0 || send_par->video_type == 1)
	{
		memset(temp_char, 0x00, sizeof(temp_char));
		sprintf(temp_char,"obj%%5bvideo_type%%5d=%d&",send_par->video_type);
		strcat(send_message, temp_char);
	}

	if(send_par->disk_count > 0)
	{
		memset(temp_char, 0x00, sizeof(temp_char));
		sprintf(temp_char,"obj%%5bn_disks%%5d=%d&",send_par->disk_count);
		strcat(send_message, temp_char);
	}

	if(send_par->disk_size > 0)
	{
		memset(temp_char, 0x00, sizeof(temp_char));
		sprintf(temp_char,"obj%%5bdisks_size%%5d=%d&",send_par->disk_size);
		strcat(send_message, temp_char);
	}

	// if(send_par->disk_filled > 0)
	{
		memset(temp_char, 0x00, sizeof(temp_char));
		sprintf(temp_char,"obj%%5bdisks_filled%%5d=%d&",send_par->disk_filled);
		strcat(send_message, temp_char);
	}

  {
    memset(temp_char, 0x00, sizeof(temp_char)); // todo stream port
    sprintf(temp_char,"obj%%5bstream_port%%5d=%d&",nf_sysdb_get_uint("net.proto.clientport"));
    strcat(send_message, temp_char);
  }

  {
    memset(temp_char, 0x00, sizeof(temp_char)); // todo stream port  
    sprintf(temp_char,"obj%%5bweb_port%%5d=%d&",nf_sysdb_get_uint("net.proto.webport"));
    strcat(send_message, temp_char);
  }

  {
    memset(temp_char, 0x00, sizeof(temp_char)); 
    sprintf(temp_char,"obj%%5btime_zone%%5d=%d&",nf_sysdb_get_uint("sys.date.tz_index"));
    strcat(send_message, temp_char);
  }
  
  {
    memset(temp_char, 0x00, sizeof(temp_char)); 
    sprintf(temp_char,"obj%%5bversion%%5d=%s&",nf_sysdb_get_str_nocopy("sys.info.swver"));
    strcat(send_message, temp_char);
  }

  if(strlen(send_par->lang) != 0)
  {
    memset(temp_char, 0x00, sizeof(temp_char));
    sprintf(temp_char,"obj%%5blang%%5d=%s",send_par->lang);
    strcat(send_message, temp_char);
  }
}

int nf_ddns_itx_get_myip(char *get_myIp_str)
{
	int res, ret= 0;
	int fd, i, j;
	char send_data[4096];

	char rec_buff[4096] = {0,};
	int  ip_num[4] = {0,};
	char *tmp_pchar;
	int send_len; 
	struct sockaddr_in addr;            

	bzero(&addr, sizeof(struct sockaddr_in));
	memset(&addr, 0, sizeof(addr)); // Zero out structure

	ret = my_gethostbyname(ITX_CHECKIP_NAME , &addr);
	if(ret != 1)
	{
		g_printf("%s : my_gethostbyname fail ddns[%s]\n", __FUNCTION__, DDNS2_HOST_NAME);
		return DDNS2_RES_GET_DNSHOST_ERR;	
	} 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80 /*DDNS2_PORT*/);

	fd = Socket(AF_INET, SOCK_STREAM, 0); 
	if(fd < 0)
	{
		g_printf("socket make ok .. fd[%d]\n",fd);
		return	DDNS2_RES_SOCKET_CREATE_ERR;
	}

	ret = get_myIP_connect( (SA *)&addr, sizeof(struct sockaddr_in), 3, fd);
	if(ret != 1)
	{
		g_printf("%s : get_myIP_connect server dont connect. ret[%d]\n",  __FUNCTION__, ret);
		close(fd);
		return DDNS2_RES_SERVER_CONNECT_ERR;
	}
	
	memset(send_data, 0x00, sizeof(send_data));
	sprintf(send_data,"GET /get_address.php HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; InfoPath.2; .NET CLR 2.0.50727)\r\n"
	//"Accept-Encoding: gzip, deflate\r\n"		// choissi disable
												// If you set this params, 
												// the server sends you compressed the result.
	"Host: checkip.dyndns.com\r\n"	
	"Connection: Keep-Alive\r\n\r\n");  

	send_len = strlen(send_data);
	if( (res = send(fd, send_data, send_len, 0)) == send_len )
	{
		char *str_start = NULL;

		ret = html_wait_read(fd, 5);
		if(ret == 1)
		{
			if( (res = recv(fd, (char*)rec_buff, sizeof(rec_buff)-1, 0)) <= 0)
			{
				// some error
				g_printf("%s : recv error ......\n", __FUNCTION__ );
				Close( fd  );	
				return DDNS2_RES_RECV_ERR;
			}
			else
			{
				str_start = strstr(rec_buff, GET_MYIP_CHECK);
				if(str_start == NULL)
				{
					g_printf("%s : recv error ......00[%s]\n", __FUNCTION__, rec_buff );
					Close( fd  );	
					return DDNS2_RES_RECV_ERR;
				}
				else
				{																				
					if(!strncmp(str_start, GET_MYIP_CHECK, strlen(GET_MYIP_CHECK)))
					{
						str_start = str_start + strlen(GET_MYIP_CHECK) + 2;						
						for(i = 0; i < 4; i++)
						{
							if(str_start == NULL)
							{
								Close( fd  );									
								return DDNS2_RES_RECV_ERR;
							}
														
							ip_num[i] = strtol( str_start, &tmp_pchar, 10 );
							if(ip_num[i] < 0 || ip_num[i] > 255)
							{								
								Close( fd  );	
								return DDNS2_RES_RECV_IP_ERR;
							}
							
							for(j = 0; j < 3; j++)
							{
								str_start++;								
								if(str_start == NULL)
								{
									Close( fd  );									
									return DDNS2_RES_RECV_ERR;
								}

								if(*str_start == '.')
								{
									str_start++;
									break;
								}
							}

							if(i < 3 && j == 3)
							{
								Close( fd  );									
								return DDNS2_RES_RECV_ERR;
							}
							
						}
					}
					else
					{
						Close( fd  );							
						return DDNS2_RES_RECV_ERR;
					}
				}
			}
		}
		else
		{
			close(fd);
			return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
		}
	}
	else
	{
		close(fd);
		return DDNS2_RES_DATA_SEND_ERR;
	}

	_done:
		
	sprintf(get_myIp_str,"%d.%d.%d.%d",ip_num[0],ip_num[1],ip_num[2],ip_num[3]);
	g_message("%s ret[%s]",__FUNCTION__, get_myIp_str);		
	
	Close( fd );	
	return 0;
}

int nf_ddns_itx_get_myip2(char *get_myIp_str)
{
	int res, ret= 0;
	int fd, i, j;
	char send_data[4096];

	char rec_buff[4096] = {0,};
	int  ip_num[4] = {0,};
	char *tmp_pchar;
	int send_len;
	struct sockaddr_in addr;

	bzero(&addr, sizeof(struct sockaddr_in));
	memset(&addr, 0, sizeof(addr)); // Zero out structure

	ret = my_gethostbyname(ITX_CHECKIP_NAME , &addr);
	if(ret != 1)
	{
		g_printf("%s : my_gethostbyname fail ddns[%s]\n", __FUNCTION__, DDNS2_HOST_NAME);
		if(inet_aton(DDNS2_ITX_GET_MY_IP, &addr.sin_addr) == 0)
		{
			g_printf("%s : inet_aton fail ip[%s]\n", __FUNCTION__, DDNS2_ITX_GET_MY_IP);
			return DDNS2_RES_GET_DNSHOST_ERR;
		}
		//return DDNS2_RES_GET_DNSHOST_ERR;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80 /*DDNS2_PORT*/);

	fd = Socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{
		g_printf("socket make ok .. fd[%d]\n",fd);
		return	DDNS2_RES_SOCKET_CREATE_ERR;
	}

	ret = get_myIP_connect( (SA *)&addr, sizeof(struct sockaddr_in), 3, fd);
	if(ret != 1)
	{
		g_printf("%s : get_myIP_connect server dont connect. ret[%d]\n",  __FUNCTION__, ret);
		close(fd);
		return DDNS2_RES_SERVER_CONNECT_ERR;
	}

	memset(send_data, 0x00, sizeof(send_data));
	sprintf(send_data,"GET /get_address.php HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; InfoPath.2; .NET CLR 2.0.50727)\r\n"
	//"Accept-Encoding: gzip, deflate\r\n"		// choissi disable
												// If you set this params,
												// the server sends you compressed the result.
	"Host: checkip.dyndns.com\r\n"
	"Connection: Keep-Alive\r\n\r\n");

	send_len = strlen(send_data);
	if( (res = send(fd, send_data, send_len, 0)) == send_len )
	{
		char *str_start = NULL;

		ret = html_wait_read(fd, 5);
		if(ret == 1)
		{
			if( (res = recv(fd, (char*)rec_buff, sizeof(rec_buff)-1, 0)) <= 0)
			{
				// some error
				g_printf("%s : recv error ......\n", __FUNCTION__ );
				Close( fd  );
				return DDNS2_RES_RECV_ERR;
			}
			else
			{
				str_start = strstr(rec_buff, GET_MYIP_CHECK);
				if(str_start == NULL)
				{
					g_printf("%s : recv error ......00[%s]\n", __FUNCTION__, rec_buff );
					Close( fd  );
					return DDNS2_RES_RECV_ERR;
				}
				else
				{
					if(!strncmp(str_start, GET_MYIP_CHECK, strlen(GET_MYIP_CHECK)))
					{
						str_start = str_start + strlen(GET_MYIP_CHECK) + 2;
						for(i = 0; i < 4; i++)
						{
							if(str_start == NULL)
							{
								Close( fd  );
								return DDNS2_RES_RECV_ERR;
							}

							ip_num[i] = strtol( str_start, &tmp_pchar, 10 );
							if(ip_num[i] < 0 || ip_num[i] > 255)
							{
								Close( fd  );
								return DDNS2_RES_RECV_IP_ERR;
							}

							for(j = 0; j < 3; j++)
							{
								str_start++;
								if(str_start == NULL)
								{
									Close( fd  );
									return DDNS2_RES_RECV_ERR;
								}

								if(*str_start == '.')
								{
									str_start++;
									break;
								}
							}

							if(i < 3 && j == 3)
							{
								Close( fd  );
								return DDNS2_RES_RECV_ERR;
							}

						}
					}
					else
					{
						Close( fd  );
						return DDNS2_RES_RECV_ERR;
					}
				}
			}
		}
		else
		{
			close(fd);
			return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
		}
	}
	else
	{
		close(fd);
		return DDNS2_RES_DATA_SEND_ERR;
	}

	_done:

	sprintf(get_myIp_str,"%d.%d.%d.%d",ip_num[0],ip_num[1],ip_num[2],ip_num[3]);
	g_message("%s ret[%s]",__FUNCTION__, get_myIp_str);

	Close( fd );
	return 0;
}

int nf_ddns_dyn_get_myip(char *get_myIp_str)
{
	int res, ret= 0;
	int fd, i, j;
	char send_data[4096];

	char rec_buff[4096] = {0,};
	int  ip_num[4] = {0,};
	char *tmp_pchar;
	int send_len; 
	int dyn_checkip_count = 0;
	struct sockaddr_in addr;            

	bzero(&addr, sizeof(struct sockaddr_in));
	memset(&addr, 0, sizeof(addr)); // Zero out structure

	while(dyn_checkip_count < 2)
	{
		ret = my_gethostbyname(DYNDNS_CHECKIP_NAME , &addr);
		if(ret != 1)
		{
			g_printf("%s : my_gethostbyname fail ddns checkip.dyndns.com\n", __FUNCTION__);
			return DDNS2_RES_GET_DNSHOST_ERR;
		} 

		addr.sin_family = AF_INET;
		addr.sin_port = htons(DYNDNS_PORT); // 8245 or 80
		fd = Socket(AF_INET, SOCK_STREAM, 0); 
		if(fd < 0)
		{
			g_printf("socket make ok .. fd[%d]\n",fd);
			return	DDNS2_RES_SOCKET_CREATE_ERR;
		}

		ret = get_myIP_connect( (SA *)&addr, sizeof(struct sockaddr_in), 3, fd);
		if(ret != 1)
		{
			g_printf("%s : get_myIP_connect server dont connect. ret[%d]\n",  __FUNCTION__, ret);
			close(fd);
			sleep(5);
		}
		else
		{
			break;
		}
		dyn_checkip_count++;
	}

	if(dyn_checkip_count == 2)
	{
		return DDNS2_RES_SOCKET_CREATE_ERR;
	}

	memset(send_data, 0x00, sizeof(send_data));
	
	sprintf(send_data,"GET / HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; InfoPath.2; .NET CLR 2.0.50727)\r\n"
	//"Accept-Encoding: gzip, deflate\r\n"		// choissi disable
												// If you set this params, 
												// the server sends you compressed the result.
	"Host: checkip.dyndns.com\r\n"	
	"Connection: Keep-Alive\r\n\r\n");  

	send_len = strlen(send_data);
	if( (res = send(fd, send_data, send_len, 0)) == send_len )
	{
		char *str_start = NULL;

		ret = html_wait_read(fd, 5);
		if(ret == 1)
		{
			if( (res = recv(fd, (char*)rec_buff, sizeof( rec_buff ), 0)) <= 0)
			{
				// some error
				g_printf("send_SS_ddns_packet: recv error ...... \n");
				Close( fd  );	
				return DDNS2_RES_RECV_ERR;
			}
			else
			{
				str_start = strstr(rec_buff, GET_MYIP_CHECK);
				if(str_start == NULL)
				{
					g_printf("%s : recv error ...... 000[%s]\n", __FUNCTION__, rec_buff );
					Close( fd  );	
					return DDNS2_RES_RECV_ERR;
				}else{
					if(!strncmp(str_start, GET_MYIP_CHECK, strlen(GET_MYIP_CHECK)))
					{
						str_start = str_start + strlen(GET_MYIP_CHECK) + 2;
						
						for(i = 0; i < 4; i++)
						{
							if(str_start == NULL)
							{
								Close( fd  );	
								return DDNS2_RES_RECV_ERR;
							}
							
							ip_num[i] = strtol( str_start, &tmp_pchar, 10 );
							if(ip_num[i] < 0 || ip_num[i] > 255)
							{
								Close( fd  );	
								return DDNS2_RES_RECV_IP_ERR;
							}
							
							for(j = 0; j < 3; j++)
							{
								str_start++;
								if(str_start == NULL)
								{
									Close( fd  );	
									return DDNS2_RES_RECV_ERR;
								}

								if(*str_start == '.')
								{
									str_start++;
									break;
								}
							}

							if(i < 3 && j == 3)
							{
								Close( fd  );	
								return DDNS2_RES_RECV_ERR;
							}
							
						}
					}else{
						Close( fd  );	
						return DDNS2_RES_RECV_ERR;
					}
				}
			}
		}
		else
		{
			close(fd);
			return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
		}
	}
	else
	{
		close(fd);
		return DDNS2_RES_DATA_SEND_ERR;
	}

	_done:

	sprintf(get_myIp_str,"%d.%d.%d.%d",ip_num[0],ip_num[1],ip_num[2],ip_num[3]);
	g_message("%s ret[%s]",__FUNCTION__, get_myIp_str);	

	Close( fd );	
	return 0;
}

int nf_ddns_dyn_get_myip2(char *get_myIp_str)
{
	int res, ret= 0;
	int fd, i, j;
	char send_data[4096];

	char rec_buff[4096] = {0,};
	int  ip_num[4] = {0,};
	char *tmp_pchar;
	int send_len;
	int dyn_checkip_count = 0;
	struct sockaddr_in addr;

	bzero(&addr, sizeof(struct sockaddr_in));
	memset(&addr, 0, sizeof(addr)); // Zero out structure

	while(dyn_checkip_count < 2)
	{
		ret = my_gethostbyname(DYNDNS_CHECKIP_NAME , &addr);
		if(ret != 1)
		{
			g_printf("%s : my_gethostbyname fail ddns checkip.dyndns.com\n", __FUNCTION__);
			if(inet_aton(DDNS2_DYN_GET_MY_IP, &addr.sin_addr) == 0)
			{
				g_printf("%s : inet_aton fail ip[%s]\n", __FUNCTION__, DDNS2_DYN_GET_MY_IP);
				return DDNS2_RES_GET_DNSHOST_ERR;
			}
			//return DDNS2_RES_GET_DNSHOST_ERR;
		}

		addr.sin_family = AF_INET;
		addr.sin_port = htons(DYNDNS_PORT); // 8245 or 80
		fd = Socket(AF_INET, SOCK_STREAM, 0);
		if(fd < 0)
		{
			g_printf("socket make ok .. fd[%d]\n",fd);
			return	DDNS2_RES_SOCKET_CREATE_ERR;
		}

		ret = get_myIP_connect( (SA *)&addr, sizeof(struct sockaddr_in), 3, fd);
		if(ret != 1)
		{
			g_printf("%s : get_myIP_connect server dont connect. ret[%d]\n",  __FUNCTION__, ret);
			close(fd);
			sleep(5);
		}
		else
		{
			break;
		}
		dyn_checkip_count++;
	}

	if(dyn_checkip_count == 2)
	{
		return DDNS2_RES_SOCKET_CREATE_ERR;
	}

	memset(send_data, 0x00, sizeof(send_data));

	sprintf(send_data,"GET / HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; InfoPath.2; .NET CLR 2.0.50727)\r\n"
	//"Accept-Encoding: gzip, deflate\r\n"		// choissi disable
												// If you set this params,
												// the server sends you compressed the result.
	"Host: checkip.dyndns.com\r\n"
	"Connection: Keep-Alive\r\n\r\n");

	send_len = strlen(send_data);
	if( (res = send(fd, send_data, send_len, 0)) == send_len )
	{
		char *str_start = NULL;

		ret = html_wait_read(fd, 5);
		if(ret == 1)
		{
			if( (res = recv(fd, (char*)rec_buff, sizeof( rec_buff ), 0)) <= 0)
			{
				// some error
				g_printf("send_SS_ddns_packet: recv error ...... \n");
				Close( fd  );
				return DDNS2_RES_RECV_ERR;
			}
			else
			{
				str_start = strstr(rec_buff, GET_MYIP_CHECK);
				if(str_start == NULL)
				{
					g_printf("%s : recv error ...... 000[%s]\n", __FUNCTION__, rec_buff );
					Close( fd  );
					return DDNS2_RES_RECV_ERR;
				}else{
					if(!strncmp(str_start, GET_MYIP_CHECK, strlen(GET_MYIP_CHECK)))
					{
						str_start = str_start + strlen(GET_MYIP_CHECK) + 2;

						for(i = 0; i < 4; i++)
						{
							if(str_start == NULL)
							{
								Close( fd  );
								return DDNS2_RES_RECV_ERR;
							}

							ip_num[i] = strtol( str_start, &tmp_pchar, 10 );
							if(ip_num[i] < 0 || ip_num[i] > 255)
							{
								Close( fd  );
								return DDNS2_RES_RECV_IP_ERR;
							}

							for(j = 0; j < 3; j++)
							{
								str_start++;
								if(str_start == NULL)
								{
									Close( fd  );
									return DDNS2_RES_RECV_ERR;
								}

								if(*str_start == '.')
								{
									str_start++;
									break;
								}
							}

							if(i < 3 && j == 3)
							{
								Close( fd  );
								return DDNS2_RES_RECV_ERR;
							}

						}
					}else{
						Close( fd  );
						return DDNS2_RES_RECV_ERR;
					}
				}
			}
		}
		else
		{
			close(fd);
			return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
		}
	}
	else
	{
		close(fd);
		return DDNS2_RES_DATA_SEND_ERR;
	}

	_done:

	sprintf(get_myIp_str,"%d.%d.%d.%d",ip_num[0],ip_num[1],ip_num[2],ip_num[3]);
	g_message("%s ret[%s]",__FUNCTION__, get_myIp_str);

	Close( fd );
	return 0;
}

int nf_ddns2_register(DDNS2_PARRAM *send_par)
{
	int res, ret= 0;
	int fd;
	char send_data[4096];
	char content_char[4096];
	char key_code[16] = {0,};

	char hash_char[256] = {0,};
	char hash_buff[256] = {0,};

	char rec_buff[4096] = {0,};
	char *tmp_pchar;
	int send_len; 
  
  	struct sockaddr_in addr;            
  	    
	bzero(&addr, sizeof(struct sockaddr_in));   
	memset(&addr, 0, sizeof(addr)); // Zero out structure
	ret = my_gethostbyname(DDNS2_HOST_NAME , &addr);
	if(ret != 1)
	{
		g_printf("%s : my_gethostbyname fail ddns[%s]\n", __FUNCTION__, DDNS2_HOST_NAME);
		ddns_set_status(DDNS2_RES_GET_DNSHOST_ERR);
		return DDNS2_RES_GET_DNSHOST_ERR;	
	} 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DDNS2_PORT);
//  addr.sin_addr.s_addr = inet_addr(DDNS2_IP);  
	fd = Socket(AF_INET, SOCK_STREAM, 0);  
	if(fd < 0)
	{
		g_print("%s : Socket ret[%d]\n",  __FUNCTION__, fd);
		ddns_set_status(DDNS2_RES_SOCKET_CREATE_ERR);		
		return	DDNS2_RES_SOCKET_CREATE_ERR;
	}
  
	ret = ddns2_connect( (SA *)&addr, sizeof(struct sockaddr_in), 3, fd);  
  	if(ret != 1)
	{
	    g_print("%s : ddns2_connect ret[%d]\n",  __FUNCTION__, ret);
		ret = DDNS2_RES_SERVER_CONNECT_ERR;
		goto _done;
	}

	memset(send_data, 0x00, sizeof(send_data));
	memset(content_char, 0x00, sizeof(content_char));

	strcpy(key_code, DDNS2_KEY);
	ddns2_send_message_make(content_char, send_par);

	ddns2_hmac_md5(hash_char, key_code, content_char);

	strcpy(hash_buff , base16(hash_char, 16));
	strcat(content_char, "&md=");
	strcat(content_char, hash_buff);

	sprintf(send_data,"POST /host/update_by_host HTTP/1.1\r\n"
	                "Accept: */*\r\n"
	                "Content-Type: application/x-www-form-urlencoded\r\n"
	                "Content-Length:%d\r\n"
	                "Host: localhost\r\n\r\n"
	                "%s",strlen(content_char),content_char);  

	//g_message("%s send_data[%s]",__FUNCTION__, send_data);	
	send_len = strlen(send_data);		
	if( (res = send(fd, send_data, send_len, 0)) == send_len )
	{
		char *str_start = NULL;
		ret = html_wait_read(fd, 5);
		if(ret == 1)
		{
			if( (res = recv(fd, (char*)rec_buff, sizeof(rec_buff)-1, 0)) <= 0)
			{
				// some error
				g_printf("%s : recv error ......\n", __FUNCTION__);
				ret = DDNS2_RES_RECV_ERR;
				goto _done;
			}else{
												
				str_start = strstr(rec_buff, DDNS2_RET_CHECK);
				if(str_start == NULL)
				{
					g_printf("%s : recv error ......00\n", __FUNCTION__, rec_buff);
					ret = DDNS2_RES_RECV_ERR;
					goto _done;
				}else{
					if(!strncmp(str_start, DDNS2_RET_CHECK, strlen(DDNS2_RET_CHECK)))
					{
						ret = strtol( str_start + 9, &tmp_pchar, 10 );
						if(ret == DDNS2_RES_OK_GOOD)
							ret = 1;
						
					}else{						
						g_printf("%s : return msg [err]\n", __FUNCTION__, rec_buff);
						ret = DDNS2_RES_RECV_ERR;						
						goto _done;
					}
				}
			}
		}
		else
		{
			ret = DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
			goto _done;
		}
	}
	else
	{
		ret = DDNS2_RES_DATA_SEND_ERR;
		goto _done;
	}

_done:
	g_printf("%s : ret[%d] recv[%s]\n", __FUNCTION__, ret, rec_buff);
	ddns_set_status(ret);
	
	Close( fd );	
	return ret;
}


void ddns2_set_param(DDNS2_PARRAM *send_par)
{
	gchar *strTemp;
	NF_DISK_INFO *disk_info;
	guint dfillSize = 0, tfillSize = 0, total_used_amount = 0;
	guint64 dSize = 0;
	guint j;
	gint ret = 0;
	gint ddns2_ret = 0;
    int disk_check = 0;

	g_sprintf(send_par->owner_name, "%s", "c");
	g_sprintf(send_par->host_name, "%s", nf_sysdb_get_str_nocopy("net.ddns.hostname"));

	gint sig_t = 0;
	nf_sig_type_get(&sig_t);
	if(sig_t)
	{
		send_par->video_type = 1;  //PAL		
	}
	else 
	{
		send_par->video_type = 0;  //NTSC		
	}    
	send_par->disk_count = nf_get_ddns_disk_count();
	disk_info = g_malloc0(sizeof(NF_DISK_INFO));
	if(!disk_info)
		g_warning("%s : disk info alloc error\n", __FUNCTION__);

	disk_check = nf_disk_get_info(disk_info, NULL);
    if(disk_check == 1)
    {
    	for(j=0; j<16; j++) {
    		if(disk_info->disk_state[0][j] & 0x01)
    			dSize+=disk_info->disk_size[0][j];
    	}    
    	send_par->disk_size = dSize/1000; //MB단위
    	
    	for(j=0; j<16; j++) {
    		if(disk_info->disk_state[0][j] & 0x01)
    			ret = nf_disk_get_usage(0, j, &dfillSize, &tfillSize, NULL);
    		total_used_amount += dfillSize;
    	}    

    	if(ret == TRUE)
    	{
    		send_par->disk_filled = total_used_amount/1000;    		
    	}
    	else
    	{
    		send_par->disk_filled = 0;
    	}
    }
    else
    {
    	send_par->disk_size = 0; //MB단위
        send_par->disk_filled = 0;
    }
        
	g_sprintf(send_par->lang, "%s", nf_sysdb_get_str_nocopy("disp.osd.lang"));
	
	if(disk_info) g_free(disk_info);	
	disk_info = NULL;
	
}



int send_ddns2_packet(DDNS2_PARRAM *send_par)
{
	int res, ret= 0;
	int fd;
	char send_data[4096];
	char content_char[4096];
	char key_code[16] = {0,};
	//	char *v[1];
	char hash_char[256] = {0,};
	char hash_buff[256] = {0,};

	char rec_buff[4096] = {0,};
	char *tmp_pchar;
	int send_len; 

	struct sockaddr_in addr;            

	bzero(&addr, sizeof(struct sockaddr_in));


	memset(&addr, 0, sizeof(addr)); // Zero out structure
	ret = my_gethostbyname(DDNS2_HOST_NAME , &addr);
	if(ret != 1)
	{
		g_printf("%s : my_gethostbyname fail ddns[%s]\n", __FUNCTION__, DDNS2_HOST_NAME);
		return DDNS2_RES_GET_DNSHOST_ERR;
	} 
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DDNS2_PORT);
	//  addr.sin_addr.s_addr = inet_addr(DDNS2_IP);
	fd = Socket(AF_INET, SOCK_STREAM, 0); 
	if(fd < 0){
		g_print("%s : socket ret[%d]\n",  __FUNCTION__, fd);
		return	DDNS2_RES_SOCKET_CREATE_ERR;
	}

	ret = ddns2_connect( (SA *)&addr, sizeof(struct sockaddr_in), 3, fd);
	if(ret != 1)
	{
		g_print("%s : ddns2_connect ret[%d]\n",  __FUNCTION__, ret);
		close(fd);
		return DDNS2_RES_SERVER_CONNECT_ERR;
	}

	memset(send_data, 0x00, sizeof(send_data));
	memset(content_char, 0x00, sizeof(content_char));


	strcpy(key_code, DDNS2_KEY);
	ddns2_send_message_make(content_char, send_par);	
	ddns2_hmac_md5(hash_char, key_code, content_char);
	strcpy(hash_buff , base16(hash_char, 16));
	strcat(content_char, "&md=");
	strcat(content_char, hash_buff);
	sprintf(send_data,"POST /host/update_by_host HTTP/1.1\r\n"
	                "Accept: */*\r\n"
	                "Content-Type: application/x-www-form-urlencoded\r\n"
	                "Content-Length:%d\r\n"
	                "Host: localhost\r\n\r\n"
	                "%s",strlen(content_char),content_char);  

	send_len = strlen(send_data);
	if( (res = send(fd, send_data, send_len, 0)) == send_len )
	{
		char *str_start = NULL;    
	    ret = html_wait_read(fd, 5);
	    if(ret == 1)
	    {
	  		if( (res = recv(fd, (char*)rec_buff, sizeof(rec_buff)-1, 0)) <= 0)
	  		{
	  			// some error
		        g_printf("%s : recv error ...... \n", __FUNCTION__);
		        Close( fd  );	
	  			return DDNS2_RES_RECV_ERR;
	  		}
	  		else
	  		{
				str_start = strstr(rec_buff, DDNS2_RET_CHECK);
				if(str_start == NULL)
				{
			        g_printf("%s : recv error ......00[%s]\n", __FUNCTION__, rec_buff);
			        Close( fd  );	
		  			return DDNS2_RES_RECV_ERR;
				}
				else
				{
					if(!strncmp(str_start, DDNS2_RET_CHECK, strlen(DDNS2_RET_CHECK)))
					{
						ret = strtol( str_start + 9, &tmp_pchar, 10 );
						if(ret == DDNS2_RES_OK_GOOD)
							ret = 1;
							
					} else {
						g_printf("%s : return msg [err][%s]\n", __FUNCTION__, rec_buff);
				        Close( fd  );
			  			return DDNS2_RES_RECV_ERR;
					}
				}	        
	  		}
	    }else{
	      close(fd);
	      return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
	    }
	}
	else
	{
		close(fd);
		return DDNS2_RES_DATA_SEND_ERR;
	}

_done:
  	g_printf("%s : ret[%d] recv[%s]\n", __FUNCTION__, ret, rec_buff);
	Close( fd );	
	return ret;
}

