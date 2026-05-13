/*
* nf_webserver.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "nf_network.h"
#include "unp.h"

#ifdef DEBUG_JBSHELL_WEBIF
	#include "jbshell.h" 
#endif

#define		SEPERATOR		"."
#define 	WEB_BUFFSIZE	512
#define 	MAX_STR_LEN		128

//#define LINK_HTM_TEMP 		"<html><head><title>Link</title></head><body><SCRIPT LANGUAGE=\"javascript\">var ip=location.hostname;\nlocation.href=\"http://dvrlink.net/webdvr/webdvr%s.html?\"+ip+\"=\";</SCRIPT></body></html>"
#define LINK_HTM_TEMP 		"<html><head><title>Link</title></head><body><SCRIPT LANGUAGE=\"javascript\">var ip=location.hostname;\nlocation.href=\"/webdvr%s.html?\"+ip+\"=\";</SCRIPT></body></html>"
static char link_htm[4096];

static char index_htm[] = 	"<html><head><title></title></head><frameset rows=\"0%, 100%\" cols=\"1*\" border=\"0\"><frame src=\"main.htm\" noresize scrolling=\"no\" marginwidth=\"10\" marginheight=\"14\"><frame src=\"link.htm\" noresize scrolling=\"no\" marginwidth=\"10\" marginheight=\"14\"><noframes><body></body></noframes></frameset></html>";
static char main_htm[] =	"<html><head><title>NFDVR</title></head><body>NFDVR WEB SERVER</body></html>";

//http://dvrlink.net/webdvr/webdvr10.1.100EN.html?192.168.100.54=

//#define ROOT_PATH "/var/www/html"
#define ROOT_PATH "/NFDVR/data/web"
#define SERVER "webserver/1.0"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define PORT 80

#define _DEBUG_NET

#define DEBUG_FORCE_VERSION

typedef enum
{
  HTTP_OK = 200,
  HTTP_MOVED_TEMPORARILY = 302,
  HTTP_BAD_REQUEST = 400,       /* malformed syntax */
  HTTP_UNAUTHORIZED = 401, /* authentication needed, respond with auth hdr */
  HTTP_NOT_FOUND = 404,
  HTTP_FORBIDDEN = 403,
  HTTP_REQUEST_TIMEOUT = 408,
  HTTP_NOT_IMPLEMENTED = 501,   /* used for unrecognized requests */
  HTTP_INTERNAL_SERVER_ERROR = 500,
#if 0 /* future use */
  HTTP_CONTINUE = 100,
  HTTP_SWITCHING_PROTOCOLS = 101,
  HTTP_CREATED = 201,
  HTTP_ACCEPTED = 202,
  HTTP_NON_AUTHORITATIVE_INFO = 203,
  HTTP_NO_CONTENT = 204,
  HTTP_MULTIPLE_CHOICES = 300,
  HTTP_MOVED_PERMANENTLY = 301,
  HTTP_NOT_MODIFIED = 304,
  HTTP_PAYMENT_REQUIRED = 402,
  HTTP_BAD_GATEWAY = 502,
  HTTP_SERVICE_UNAVAILABLE = 503, /* overload, maintenance */
  HTTP_RESPONSE_SETSIZE=0xffffffff
#endif
} HttpResponseNum;

enum
{
	IDX_HTTP_OK = 0 ,
	IDX_HTTP_MOVED_TEMPORARILY,
	IDX_HTTP_REQUEST_TIMEOUT,
	IDX_HTTP_NOT_IMPLEMENTED,
	IDX_HTTP_UNAUTHORIZED,
	IDX_HTTP_NOT_FOUND,
	IDX_HTTP_BAD_REQUEST,
	IDX_HTTP_FORBIDDEN,
	IDX_HTTP_INTERNAL_SERVER_ERROR
};

typedef struct
{
  HttpResponseNum type;
  const char *name;
  const char *info;
} HttpEnumString;

static const HttpEnumString httpResponseArr[] = {
	
/* 0 */  { HTTP_OK, "OK", NULL }, 
/* 1 */  { HTTP_MOVED_TEMPORARILY, "Found", "Directories must end with a slash." },
/* 2 */  { HTTP_REQUEST_TIMEOUT, "Request Timeout", "No request appeared within a reasonable time period." },
/* 3 */  { HTTP_NOT_IMPLEMENTED, "Not Implemented", "The requested method is not recognized by this server." },
/* 4 */  { HTTP_UNAUTHORIZED, "Unauthorized", "" },
/* 5 */  { HTTP_NOT_FOUND, "Not Found", "The requested URL was not found on this server." },
/* 6 */  { HTTP_BAD_REQUEST, "Bad Request", "Unsupported method." },
/* 7 */  { HTTP_FORBIDDEN, "Forbidden", "" },
/* 8 */  { HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error", "Internal Server Error" },

#if 0                               /* not implemented */
  { HTTP_CREATED, "Created" },
  { HTTP_ACCEPTED, "Accepted" },
  { HTTP_NO_CONTENT, "No Content" },
  { HTTP_MULTIPLE_CHOICES, "Multiple Choices" },
  { HTTP_MOVED_PERMANENTLY, "Moved Permanently" },
  { HTTP_NOT_MODIFIED, "Not Modified" },
  { HTTP_BAD_GATEWAY, "Bad Gateway", "" },
  { HTTP_SERVICE_UNAVAILABLE, "Service Unavailable", "" },
#endif

};

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
char *get_mime_type(char *name)
{
  char *ext = strrchr(name, '.');
  if (!ext) return NULL;
  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
  if (strcmp(ext, ".gif") == 0) return "image/gif";
  if (strcmp(ext, ".png") == 0) return "image/png";
  if (strcmp(ext, ".css") == 0) return "text/css";
  if (strcmp(ext, ".au") == 0) return "audio/basic";
  if (strcmp(ext, ".wav") == 0) return "audio/wav";
  if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
  if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
  if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
  if (strcmp(ext, ".cab") == 0)	return "application/octet-stream";
  if (strcmp(ext, ".msi") == 0)	return "application/octet-stream";
  	
  return NULL;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void send_headers(int fd, HttpEnumString httpResponse, char *extra, char *mime, 
                  int length, time_t date)
{
	char buff[1024];
	unsigned int len; 
	int ret;
	time_t now;
	char timebuf[128];
	struct tm	buff_tm;
	
	
	len = snprintf(buff, sizeof(buff), "%s %d %s\r\nServer: %s\r\n", 
					PROTOCOL, httpResponse.type, httpResponse.name,  SERVER);
	ret = write(fd, buff, len);
	
	now = time(NULL);
	gmtime_r(&now, &buff_tm);
	
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, &buff_tm);
	len = snprintf(buff, sizeof(buff), "Date: %s\r\n", timebuf);
	ret = write(fd, buff, len);
	
	if(extra)
	{
		len = snprintf(buff, sizeof(buff), "%s\r\n", extra);
		ret = write(fd, buff, len);
	}
	if(mime)
	{ 
		len = snprintf(buff, sizeof(buff), "Content-Type: %s\r\n", mime);
		ret = write(fd, buff, len);
	}
	if(length >= 0)
	{
		len = snprintf(buff, sizeof(buff), "Content-Length: %d\r\n", length);
		ret = write(fd, buff, len);
	}
	if(date != -1)
	{
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, &buff_tm);
		len = snprintf(buff, sizeof(buff), "Last-Modified: %s\r\n", timebuf);
		ret = write(fd, buff, len);
	}
	len = snprintf(buff, sizeof(buff), "Connection: close\r\n\r\n");
	ret = write(fd, buff, len);
	
	g_message("%s : HTTP ret code[%d] len[%d]", __FUNCTION__, httpResponse.type, length );
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void send_error(int fd, HttpEnumString httpResponse, char *extra)
{
	char buff[1024];
	unsigned int len;
	int ret;
		
	len = snprintf(buff, sizeof(buff), "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n"
										"<BODY><H4>%d %s</H4>\r\n"
										"%s\r\n</BODY></HTML>\r\n",
					httpResponse.type, httpResponse.name,
					httpResponse.type, httpResponse.name, httpResponse.info);
	
	send_headers(fd, httpResponse, extra, "text/html", len, -1);
	ret = write(fd, buff, len);		
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int send_file(int fd, char *path )
{
	char data[1024];
	int n, ret=0, tot_send=0;
	struct stat stat_buf;

	FILE *fp;

	g_message("%s", __FUNCTION__);
	 
	if( stat(path, &stat_buf) < 0 )
	{
		g_warning("%s httpResponseArr[ IDX_HTTP_NOT_FOUND ]", __FUNCTION__);
		g_warning("path [%s]", path);
		send_error(fd, httpResponseArr[ IDX_HTTP_NOT_FOUND ], NULL);		
		return -404; 
	}
		
	fp = fopen(path, "rb");

	if (!fp)
	{
		g_warning("%s httpResponseArr[ IDX_HTTP_FORBIDDEN ]", __FUNCTION__);
		send_error(fd, httpResponseArr[ IDX_HTTP_FORBIDDEN ], NULL);
		return -403;
	}
					
	send_headers(fd, httpResponseArr[ IDX_HTTP_OK ], NULL, get_mime_type(path), 
				stat_buf.st_size, stat_buf.st_mtime);
		
	// file send
	while ((n = fread(data, 1, sizeof(data), fp)) > 0)
	{
		ret = Writen(fd, data, n);
		if(ret <0)
		{
			fclose(fp);
			return ret;
		}
		tot_send += ret;
	}

	fclose(fp);		

	return ret;
}

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


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int recv_sleep(socket_t sock, char *buf, int n, unsigned int timeout)
{
	int count;
	while(1)
	{
		count = recv(sock, buf, n, MSG_DONTWAIT);
		if(count == -1)
		{
			if(errno != EAGAIN) 
				return -1;
			count = 0;
		}
		if(count == n)
		{
			return n;
		}
		if(timeout--)
		{
			sleep_ex(1000); 	// 1ms
		}
			
		if(timeout==0)
		{
			return -1;
		}
	}
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int recvln(socket_t conn, char *buff, int buffsz)
{
	char    *bp = buff, c;
	int n = 0;
	int max_recv_cnt = 1000;

	while(bp - buff < buffsz &&
		(n = recv_sleep(conn, bp, 1, 1000)) > 0) {
			if (*bp++ == '\n')
				return (bp - buff);
	}

	if (n < 0)
		return -1;

	if (bp - buff == buffsz)
		while (recv_sleep(conn, &c, 1, 1000) > 0 && c != '\n' && (max_recv_cnt--));

	if(max_recv_cnt < 0)
		return -1;
	return (bp - buff);
}


char *remove_space(char *str) 
{
	int i, j;
	
	if(!str)return NULL;
	
	for(j = 0; str[j] == ' ' || str[j] == 9 || str[j] == '\r' || str[j] == '\n'; j++);
	for(i = 0; str[j] != '\0'; i++, j++) str[i] = str[j];
	for(i--; (i >= 0) && (str[i] == ' ' || str[i] == 9 || str[i] == '\r' || str[i] == '\n'); i--);

	str[i+1] = '\0';
	
	return str;
}
 
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void _on_accept(socket_t sock, struct sockaddr_in *psa)
{
	char cmd[16], vers[16], path[1024];
	char web_buff[WEB_BUFFSIZE];
	char *p;
	int n, len, ret;
	unsigned i = 0;
	
	memset( cmd, 0x00, sizeof(cmd));		
	memset( path, 0x00, sizeof(path));	
	memset( vers, 0x00, sizeof(vers));	
	memset( web_buff, 0x00, sizeof(web_buff));	
	
	/* read and parse the request line */
	n = recvln(sock, web_buff, WEB_BUFFSIZE);
	p = web_buff;
	strncpy(cmd, p, 3);
	cmd[3] = '\0';
	if(strncmp(cmd,"GET",3))
		return ;

	p += 4;
	for(i=0 ; i < sizeof(path); i++)
	{
		path[i] = p[i];
		if(p[i] == ' ' || p[i] == '\0' || p[i] == '\r' || p[i] == '\n')
		{
			path[i] = '\0';
			break;
		}
	}
	p += (i+1);
	for(i=0 ; i < sizeof(vers); i++)
	{
		vers[i] = p[i];
		if(p[i] == ' ' || p[i] == '\0'|| p[i] == '\r' || p[i] == '\n')
		{
			vers[i] = '\0';
			break;
		}
	}

	/* skip all headers - read until we get \r\n alone */
	while((n = recvln(sock, web_buff, WEB_BUFFSIZE)) > 0) {
		if (n == 2 && web_buff[0] == '\r' && web_buff[1] == '\n')
			break;
	}
#ifdef _DEBUG_NET
	g_message("%s : web request cmd[%s] ver[%s] path[%s]", __FUNCTION__ , cmd, vers, path);
#endif
				
	/* check for unexpected end of file */
	if (n < 1) {
		g_warning("%s unexpected end of file ",__FUNCTION__);
		return ;
	}
	if (strncmp(cmd, "GET", 16) || (strncmp(vers, "HTTP/1.0", 16) &&
		strncmp(vers, "HTTP/1.1", 16))) {
			g_warning("%s httpResponseArr[ IDX_HTTP_BAD_REQUEST ]",__FUNCTION__);
			send_error(sock, httpResponseArr[ IDX_HTTP_BAD_REQUEST ], NULL);
			return ;
	}

	/* send the requested web page or a "not found" error */
	if (strncmp(path, "/", 64) == 0) {
		
		len = strlen(index_htm);
		g_message("%s httpResponseArr[IDX_HTTP_OK]",__FUNCTION__);
		send_headers(sock, httpResponseArr[IDX_HTTP_OK], NULL,  "text/html", len , -1);
		gsock_write(sock, index_htm, len, 1000);
		
	} else if(strncmp(path, "/link.htm", 64) == 0) {
		
		len = strlen(link_htm);
		g_message("%s httpResponseArr[IDX_HTTP_OK]",__FUNCTION__);
		send_headers(sock, httpResponseArr[IDX_HTTP_OK], NULL, "text/html", len , -1);
		gsock_write(sock, link_htm, len, 1000);
		
	} else if(strncmp(path, "/main.htm", 64) == 0) {
		
		len = strlen(main_htm);				
		g_message("%s httpResponseArr[IDX_HTTP_OK]",__FUNCTION__);
		send_headers(sock, httpResponseArr[IDX_HTTP_OK], NULL,  "text/html", len , -1);
		gsock_write(sock, main_htm, len, 1000);

#ifdef DEBUG_JBSHELL_WEBIF
	} else if(strncmp(path, "/jbshell.htm", 12) == 0) {
	
		char key1[1024];
		char key2[1024];
		char *cmd = NULL;		

		snprintf(key1, sizeof(key1), "key1=%s", nf_sysdb_get_str_nocopy("sys.info.swver") );
		snprintf(key2, sizeof(key2), "key2=%s", nf_sysdb_get_str_nocopy("usr.U0.pass") );
		
		if( strstr( path, key1 ) != NULL
			&& strstr( path, key2 ) != NULL
			&& (cmd = strstr( path, "cmd=") ) != NULL )
		{				
			int i, len=strlen(cmd);
			
			for(i=0; i<len;++i)
				if(cmd[i] == '^') cmd[i] = ' ';	
							
			g_message ("%s cmd[%s]",__FUNCTION__, cmd);
			
			my_command( (cmd + 4) );				
		}

		len = strlen(main_htm);				
		g_message("%s httpResponseArr[IDX_HTTP_OK]",__FUNCTION__);
		send_headers(sock, httpResponseArr[IDX_HTTP_OK], NULL,  "text/html", len , -1);
		gsock_write(sock, main_htm, len, 1000);
						
#endif	

	} else { /* not found */

		char tmpPath[1024], *c, *dupPath;
		char tmp[1024];
		
		strncpy(tmp, path, sizeof(path) );
			
		// 파일 이름만 분리
		c = strrchr( tmp, '?' );
		if( c != NULL ) *c = '\0';

#ifdef _DEBUG_NET
		g_message("%s dupPath [%s]", __FUNCTION__, tmp);	
#endif
		
		// 다른 디렉토리는 못 봄(파일 보안)
		for(c = tmp + strlen(tmp) - 1; c >= tmp && !(*c == '/' || *c == '\\'); c--);
		for(; c >= tmp; c--) *c = ' '; 		
		remove_space(tmp);
	
		snprintf(tmpPath, sizeof(tmpPath), "%s/%s", ROOT_PATH, tmp);					

		ret = send_file(sock, tmpPath);				
		g_message("%s : send_file path[%s] ret[%d]", __FUNCTION__, tmpPath, ret);
		
	}
		
}

unsigned int backdoor_info_final_ip = 0;
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void *webserver_func(void *arg)
{
	unsigned int port; 
	int ret;
	struct sockaddr_in sa;
	char 	tmpver[128], lang[8];
	int		var[8];
	char	tmp[128];

	// skshin 20071010
	FILE *fp;
	
	socket_t sock;
	socket_t gsListener = -1;
	
	if(arg == NULL)
	{
		g_warning("%s : web port is null", __FUNCTION__);
		pthread_exit(NULL);
	}

	port = *(int *)arg;	
	
	memset( tmpver, 0x00, sizeof(tmpver));
	memset( tmp, 0x00, sizeof(tmp));
	memset( var, 0x00, sizeof(var));
	memset( lang, 0x00, sizeof(lang));

	sleep(5);
	
	//strncpy( tmp, get_sysdb_swver(), sizeof(tmp));	
	strncpy( tmp, nf_sysdb_get_str_nocopy("sys.info.swver"), sizeof(tmp));	

#ifdef DEBUG_FORCE_VERSION	 
	strncpy( tmp, "20.1.0.100ML", sizeof(tmp));	
	g_warning("%s DEBUG_FORCE_VERSION ver[%s]",__FUNCTION__, tmp);
#endif	

	ret = sscanf( tmp, "%d.%d.%d.%d%s", &var[0],&var[1],&var[2],&var[3],lang);
	
	fp = fopen("version", "r");
	if (fp == NULL) 
	{
		strcpy(lang, "EN"); 
	}
	else 
	{
		fgets(tmpver, sizeof(tmpver), fp);
		fgets(tmpver, sizeof(tmpver), fp);
		memset(tmpver, 0x00, sizeof(tmpver));
		memset(lang, 0x00, sizeof(lang));

		if (fscanf(fp, "LANGCODE=%s", lang) != 1) 
		{
			strcpy(lang, "EN"); 
		}
		fclose(fp);
	}

#ifdef DEBUG_FORCE_VERSION	
	strncpy( lang, "ML", sizeof(lang));
	g_warning("%s DEBUG_FORCE_VERSION lang[%s]",__FUNCTION__, lang);	
#endif

#ifdef _DEBUG_NET	
	g_message("%s : swver[%s] ret[%d] [%d].[%d].[%d].[%d%s]", __FUNCTION__,
				tmp, ret, var[0],var[1],var[2],var[3], lang);					
#endif

	sprintf(tmpver, "%d.%d.%d%s", var[0],var[1],var[3],lang);			

	sprintf(link_htm, LINK_HTM_TEMP, tmpver);
	//sprintf(link_htm, LINK_HTM_TEMP, "10.1.100EN");

	while(1) {
		
		if( nf_sysdb_get_bool("net.proto.webon")!= 1 ) 
		{
			sleep(1); 
			continue;
		}
		
		port = nf_sysdb_get_uint("net.proto.webport");
		
		gsListener = gsock_create_socket(SOCK_STREAM, 0);
		if(gsListener < 0) 
		{			
			g_warning("%s : gsock_create_socket()", __FUNCTION__ );
			sleep(5); 
			continue;										
		}
				
		ret = gsock_listen(gsListener, INADDR_ANY, port , 1);
		if( !ret ) 
		{			
			g_warning("%s : gsock_listen() port[%d]", __FUNCTION__, port);
			close(gsListener); 
			sleep(5); 
			continue;
		}
				
#ifdef _DEBUG_NET
		g_message("%s : webserver listen port[%d] fd[%d] ver[%s]", __FUNCTION__, 
				port, gsListener, tmpver);
#endif					
		
		for(;;) {
			
			ret = gsock_check_readable(gsListener, 1000);	// 1 sec
			if(ret < 0) break;		// fd err
			if(ret ==0) //timeout
			{
				if( nf_sysdb_get_uint("net.proto.webport") != port
						|| nf_sysdb_get_bool("net.proto.webon") != 1 )				
				{
					break;					
				}
				else
				{
					continue;
				}
			}
						
			sock = gsock_accept2(gsListener, &sa);
			if(sock == -1)
			{
				g_warning("%s gsock_accept2()", __FUNCTION__);
				break;
			}
			backdoor_info_final_ip = sa.sin_addr.s_addr;	//client addr
			_on_accept(sock, &sa);							//client socket , client addr info
			close(sock);
		}	

		close(gsListener);
		sleep(1);
	}

	g_message("%s : thread end", __FUNCTION__);
	pthread_exit(NULL);	
	
	return 0;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int create_webserver(void)
{
   pthread_t tid;
   unsigned int  webport;

   webport = nf_sysdb_get_uint("net.proto.webport");
			 
   if(Pthread_create(&tid, NULL, webserver_func, &webport))
   {          
   			g_warning("%s : thread create error", __FUNCTION__);
            return -1;          
   }          
   gServer_info.websvr_tid = tid;
   Pthread_detach(tid);

#ifdef _DEBUG_NET
   g_message("%s : thread created! [webserver_func]", __FUNCTION__);
#endif              

   return 0;
}

