#include "nf_common.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/poll.h>

#if HAVE_OPENSSL_EXT
#include <openssl/ssl.h>
#endif
	
#define DEBUG_FTP_CLIENT_JBSHELL
#define	DEBUG_NET_SEND_JBSHELL
#define DEBUG_NET_SEND_LOG

#ifdef DEBUG_FTP_CLIENT_JBSHELL
	#include "jbshell.h"
#endif
	
#include "nf_network.h"		// for connect_timeout()
#include "nf_util_ftp.h"
#include "unp.h"

#include "nf_record.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "api_ftp"

static GQuark 
_nf_api_ftp_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_API_FTP_ERROR 	_nf_api_ftp_error_quark()

#define	FTP_DEFAULT_TIMEOUT		90
#define FTP_DEFAULT_AUTOSEEK 	1

#define RET_FTP_FAILED			0
#define RET_FTP_FINISHED		1
#define RET_FTP_MOREDATA		2

/* XXX this should be configurable at runtime XXX */
#define	FTP_BUFSIZE	4096

typedef enum ftptype {
	FTPTYPE_ASCII=1,
	FTPTYPE_IMAGE
} ftptype_t;

typedef struct sockaddr_storage _sockaddr_storage;
typedef struct pollfd POLLFD;
 
typedef struct databuf
{
	int		listener;		/* listener socket */
	int		fd;			/* data connection */
	ftptype_t	type;			/* transfer type */
	char		buf[FTP_BUFSIZE];	/* data buffer */
#if HAVE_OPENSSL_EXT
	SSL		*ssl_handle;	/* ssl handle */
	int		ssl_active;		/* flag if ssl is active or not */
#endif
} databuf_t;

typedef struct ftpbuf
{
	int		fd;			/* control connection */
	_sockaddr_storage	localaddr;	/* local address */
	int		resp;			/* last response code */
	char		inbuf[FTP_BUFSIZE];	/* last response text */
	char		*extra;			/* extra characters */
	int		extralen;		/* number of extra chars */
	char		outbuf[FTP_BUFSIZE];	/* command output buffer */
	char		*pwd;			/* cached pwd */
	char		*syst;			/* cached system type */
	ftptype_t	type;			/* current transfer type */
	int		pasv;			/* 0=off; 1=pasv; 2=ready */
	_sockaddr_storage	pasvaddr;	/* passive mode address */
	long	timeout_sec;	/* User configureable timeout (seconds) */
	int			autoseek;	/* User configureable autoseek flag */

	int				nb;		/* "nonblocking" transfer in progress */
	databuf_t		*data;	/* Data connection for "nonblocking" transfers */
	FILE		*stream; /* output stream for "nonblocking" transfers */
	int				lastch;		/* last char of previous call */
	int				direction;	/* recv = 0 / send = 1 */
	int				closestream;/* close or not close stream */
#if HAVE_OPENSSL_EXT
	int				use_ssl; /* enable(1) or disable(0) ssl */
	int				use_ssl_for_data; /* en/disable ssl for the dataconnection */
	int				old_ssl;	/* old mode = forced data encryption */
	SSL				*ssl_handle;      /* handle for control connection */
	int				ssl_active;		  /* ssl active on control conn */
#endif

} ftpbuf_t;

/* open a FTP connection, returns ftpbuf (NULL on error)
 * port is the ftp port in network byte order, or 0 for the default
 */
static ftpbuf_t*	ftp_open(const char *host, unsigned short port, long timeout_sec );

/* quits from the ftp session (it still needs to be closed)
 * return true on success, false on error
 */
static int		ftp_quit(ftpbuf_t *ftp);

/* frees up any cached data held in the ftp buffer */
static void		ftp_gc(ftpbuf_t *ftp);

/* close the FTP connection and return NULL */
static ftpbuf_t*	ftp_close(ftpbuf_t *ftp);

/* logs into the FTP server, returns true on success, false on error */
static int		ftp_login(ftpbuf_t *ftp, const char *user, const char *pass );

/* reinitializes the connection, returns true on success, false on error */
static int		ftp_reinit(ftpbuf_t *ftp);

/* returns the remote system type (NULL on error) */
static const char*	ftp_syst(ftpbuf_t *ftp);

/* returns the present working directory (NULL on error) */
static const char*	ftp_pwd(ftpbuf_t *ftp);

/* exec a command [special features], return true on success, false on error */
static int 	ftp_exec(ftpbuf_t *ftp, const char *cmd);

#if 0
/* send a raw ftp command, return response as a hashtable, NULL on error */
static void	ftp_raw(ftpbuf_t *ftp, const char *cmd, zval *return_value);
#endif

/* changes directories, return true on success, false on error */
static int		ftp_chdir(ftpbuf_t *ftp, const char *dir);

/* changes to parent directory, return true on success, false on error */
static int		ftp_cdup(ftpbuf_t *ftp);

/* creates a directory, return the directory name on success, NULL on error.
 * the return value must be freed
 */
static char*		ftp_mkdir(ftpbuf_t *ftp, const char *dir);

/* removes a directory, return true on success, false on error */
static int		ftp_rmdir(ftpbuf_t *ftp, const char *dir);

/* Set permissions on a file */
static int		ftp_chmod(ftpbuf_t *ftp, const int mode, const char *filename, const int filename_len);

/* Allocate space on remote server with ALLO command
 * Many servers will respond with 202 Allocation not necessary,
 * however some servers will not accept STOR or APPE until ALLO is confirmed. 
 * If response is passed, it is estrdup()ed from ftp->inbuf and must be freed
 * or assigned to a zval returned to the user */
static int		ftp_alloc(ftpbuf_t *ftp, const int size, char **response);

/* returns a NULL-terminated array of filenames in the given path
 * or NULL on error.  the return array must be freed (but don't
 * free the array elements)
 */
static char**		ftp_nlist(ftpbuf_t *ftp, const char *path );

/* returns a NULL-terminated array of lines returned by the ftp
 * LIST command for the given path or NULL on error.  the return
 * array must be freed (but don't
 * free the array elements)
 */
static char**		ftp_list(ftpbuf_t *ftp, const char *path, int recursive );

/* switches passive mode on or off
 * returns true on success, false on error
 */
static int		ftp_pasv(ftpbuf_t *ftp, int pasv);

/* retrieves a file and saves its contents to outfp
 * returns true on success, false on error
 */
static int		ftp_get(ftpbuf_t *ftp, FILE *outstream, const char *path, ftptype_t type, int resumepos );

/* stores the data from a file, socket, or process as a file on the remote server
 * returns true on success, false on error
 */
static int		ftp_put(ftpbuf_t *ftp, const char *path, FILE *instream, ftptype_t type, int startpos );

/* returns the size of the given file, or -1 on error */
static int		ftp_size(ftpbuf_t *ftp, const char *path);

/* returns the last modified time of the given file, or -1 on error */
static time_t		ftp_mdtm(ftpbuf_t *ftp, const char *path);

/* renames a file on the server */
static int		ftp_rename(ftpbuf_t *ftp, const char *src, const char *dest);

/* deletes the file from the server */
static int		ftp_delete(ftpbuf_t *ftp, const char *path);

/* sends a SITE command to the server */
static int		ftp_site(ftpbuf_t *ftp, const char *cmd);

/* retrieves part of a file and saves its contents to outfp
 * returns true on success, false on error
 */
static int		ftp_nb_get(ftpbuf_t *ftp, FILE *outstream, const char *path, ftptype_t type, int resumepos );

/* stores the data from a file, socket, or process as a file on the remote server
 * returns true on success, false on error
 */
static int		ftp_nb_put(ftpbuf_t *ftp, const char *path, FILE *instream, ftptype_t type, int startpos );

/* continues a previous nb_(f)get command
 */
static int		ftp_nb_continue_read(ftpbuf_t *ftp );

/* continues a previous nb_(f)put command
 */
static int		ftp_nb_continue_write(ftpbuf_t *ftp );


/* sends an ftp command, returns true on success, false on error.
 * it sends the string "cmd args\r\n" if args is non-null, or
 * "cmd\r\n" if args is null
 */
static int		ftp_putcmd(	ftpbuf_t *ftp,
					const char *cmd,
					const char *args);

/* wrapper around send/recv to handle timeouts */
static int		my_send(ftpbuf_t *ftp, int s, void *buf, size_t len);
static int		my_recv(ftpbuf_t *ftp, int s, void *buf, size_t len);
static int		my_accept(ftpbuf_t *ftp, int s, struct sockaddr *addr, socklen_t *addrlen);

/* reads a line the socket , returns true on success, false on error */
static int		ftp_readline(ftpbuf_t *ftp);

/* reads an ftp response, returns true on success, false on error */
static int		ftp_getresp(ftpbuf_t *ftp);

/* sets the ftp transfer type */
static int		ftp_type(ftpbuf_t *ftp, ftptype_t type);

/* opens up a data stream */
static databuf_t*	ftp_getdata(ftpbuf_t *ftp );

/* accepts the data connection, returns updated data buffer */
static databuf_t*	data_accept(databuf_t *data, ftpbuf_t *ftp );

/* closes the data connection, returns NULL */
static databuf_t*	data_close(ftpbuf_t *ftp, databuf_t *data);

/* generic file lister */
static char**		ftp_genlist(ftpbuf_t *ftp, const char *cmd, const char *path );

/* IP and port conversion box */
union ipbox {
	struct in_addr	ia[2];
	unsigned short	s[4];
	unsigned char	c[8];
};


/* {{{ php_sockaddr_size
 * Returns the size of struct sockaddr_xx for the family
 */
static int _sockaddr_size(_sockaddr_storage *addr)
{
	switch (((struct sockaddr *)addr)->sa_family) {
	case AF_INET:
		return sizeof(struct sockaddr_in);
#if HAVE_IPV6
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
#endif
#ifdef AF_UNIX
	case AF_UNIX:
		return sizeof(struct sockaddr_un);
#endif
	default:
		return 0;
	}
}

/* {{{ php_any_addr
 * Fills the any (wildcard) address into php_sockaddr_storage
 */
static void _any_addr(int family, _sockaddr_storage *addr, unsigned short port)
{
	memset(addr, 0, sizeof(_sockaddr_storage));
	switch (family) {
#if HAVE_IPV6
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) addr;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = htons(port);
		sin6->sin6_addr = in6addr_any;
		break;
	}
#endif
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *) addr;
		sin->sin_family = AF_INET;
		sin->sin_port = htons(port);
		sin->sin_addr.s_addr = htonl(INADDR_ANY);
		break;
	}
	}
}
/* }}} */ 

static inline int _pollfd_for_ms(int fd, int events, int timeout)
{
	POLLFD p;
	int n;

	p.fd = fd;
	p.events = events;
	p.revents = 0;

	n = Poll(&p, 1, timeout);

	if (n > 0) {
		return p.revents;
	}

	return n;
}


static inline size_t _stream_read( FILE *fp, void *ptr, size_t size)
{
	return (fread( ptr, size, 1, fp) == 1) ? size : 0;	
}
static inline size_t _stream_write( FILE *fp, const void *ptr, size_t size)
{
	return (fwrite( ptr, size, 1, fp) == 1) ? size : 0;
}
static inline int _stream_getc ( FILE *fp )
{
	return fgetc(fp);
}
static inline int _stream_putc ( FILE *fp, int c )
{
	return fputc(c, fp);
}
static inline void _stream_rewind ( FILE *fp)
{
	rewind(fp);
}
static inline int _stream_eof ( FILE *fp)
{
	return feof(fp);
}
static inline int _stream_close ( FILE *fp)
{
	return fclose(fp);
}

/* {{{ ftp_open
 */
ftpbuf_t*
ftp_open(const char *host, unsigned short port, long timeout_sec )
{
	ftpbuf_t		*ftp;
	socklen_t 		 size;
	struct timeval tv;


	/* alloc the ftp structure */
	ftp = calloc(1, sizeof(*ftp));

	g_return_val_if_fail( ftp != NULL, NULL);
	
	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

#if 1
	ftp->fd = connect_timeout_hostname( host,
				(unsigned short) (port ? port : 21), 
				timeout_sec*1000000 );
#else
	ftp->fd = php_network_connect_socket_to_host(host,
			(unsigned short) (port ? port : 21), SOCK_STREAM,
			0, &tv, NULL, NULL, NULL, 0 );
#endif					

	if (ftp->fd < 0) {
		g_warning("%s : connect failed ret[%d]", __FUNCTION__, ftp->fd);
		goto bail;
	}

	
	/* Default Settings */
	ftp->timeout_sec = timeout_sec;
	ftp->nb = 0;

	size = sizeof(ftp->localaddr);
	memset(&ftp->localaddr, 0x00, size);

	//if (getsockname(ftp->fd, (struct sockaddr*) &ftp->localaddr, &size) == -1) {
	if (getsockname(ftp->fd,  &ftp->localaddr, &size) == -1) {
		g_warning( "getsockname failed: %s (%d)", strerror(errno), errno);
		goto bail;
	}

	if (!ftp_getresp(ftp) || ftp->resp != 220) {
		goto bail;
	}

	return ftp;

bail:
	if (ftp->fd != -1) {
		Close(ftp->fd);
	}
	free(ftp);
	return NULL;
}
/* }}} */

/* {{{ ftp_close
 */
ftpbuf_t*
ftp_close(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return NULL;
	}
	if (ftp->data) {
		data_close(ftp, ftp->data);
	}
	if (ftp->fd != -1) {
#if HAVE_OPENSSL_EXT
		if (ftp->ssl_active) {
			SSL_shutdown(ftp->ssl_handle);
		}
#endif		
		Close(ftp->fd);
	}	
	ftp_gc(ftp);
	free(ftp);
	return NULL;
}
/* }}} */

/* {{{ ftp_gc
 */
void
ftp_gc(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return;
	}
	if (ftp->pwd) {
		free(ftp->pwd);
		ftp->pwd = NULL;
	}
	if (ftp->syst) {
		free(ftp->syst);
		ftp->syst = NULL;
	}
}
/* }}} */

/* {{{ ftp_quit
 */
int
ftp_quit(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return 0;
	}

	if (!ftp_putcmd(ftp, "QUIT", NULL)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 221) {
		return 0;
	}

	if (ftp->pwd) {
		free(ftp->pwd);
		ftp->pwd = NULL;
	}

	return 1;
}
/* }}} */

/* {{{ ftp_login
 */
int
ftp_login(ftpbuf_t *ftp, const char *user, const char *pass )
{
#if HAVE_OPENSSL_EXT
	SSL_CTX	*ctx = NULL;
#endif
	if (ftp == NULL) {
		return 0;
	}

#if HAVE_OPENSSL_EXT
	if (ftp->use_ssl && !ftp->ssl_active) {
		if (!ftp_putcmd(ftp, "AUTH", "TLS")) {
			return 0;
		}
		if (!ftp_getresp(ftp)) {
			return 0;
		}
			
		if (ftp->resp != 234) {
			if (!ftp_putcmd(ftp, "AUTH", "SSL")) {
				return 0;
			}
			if (!ftp_getresp(ftp)) {
				return 0;
			}
				
			if (ftp->resp != 334) {
				return 0;
			} else {
				ftp->old_ssl = 1;
				ftp->use_ssl_for_data = 1;
			}
		}
		
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL) {
			g_warning( "failed to create the SSL context");
			return 0;
		}

		SSL_CTX_set_options(ctx, SSL_OP_ALL);

		ftp->ssl_handle = SSL_new(ctx);
		if (ftp->ssl_handle == NULL) {
			g_warning( "failed to create the SSL handle");
			SSL_CTX_free(ctx);
			return 0;
		}

		SSL_set_fd(ftp->ssl_handle, ftp->fd);

		if (SSL_connect(ftp->ssl_handle) <= 0) {
			g_warning( "SSL/TLS handshake failed");
			SSL_shutdown(ftp->ssl_handle);
			return 0;
		}

		ftp->ssl_active = 1;

		if (!ftp->old_ssl) {

			/* set protection buffersize to zero */
			if (!ftp_putcmd(ftp, "PBSZ", "0")) {
				return 0;
			}
			if (!ftp_getresp(ftp)) {
				return 0;
			}

			/* enable data conn encryption */
			if (!ftp_putcmd(ftp, "PROT", "P")) {
				return 0;
			}
			if (!ftp_getresp(ftp)) {
				return 0;
			}
			
			ftp->use_ssl_for_data = (ftp->resp >= 200 && ftp->resp <=299);		
		}
	}
#endif

	if (!ftp_putcmd(ftp, "USER", user)) {
		return 0;
	}
	if (!ftp_getresp(ftp)) {
		return 0;
	}
	if (ftp->resp == 230) {
		return 1;
	}
	if (ftp->resp != 331) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "PASS", pass)) {
		return 0;
	}
	if (!ftp_getresp(ftp)) {
		return 0;
	}
	return (ftp->resp == 230);
}
/* }}} */

/* {{{ ftp_reinit
 */
int
ftp_reinit(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return 0;
	}	

	ftp_gc(ftp);

	ftp->nb = 0;

	if (!ftp_putcmd(ftp, "REIN", NULL)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 220) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ ftp_syst
 */
const char*
ftp_syst(ftpbuf_t *ftp)
{
	char *syst, *end;

	if (ftp == NULL) {
		return NULL;
	}

	/* default to cached value */
	if (ftp->syst) {
		return ftp->syst;
	}
	if (!ftp_putcmd(ftp, "SYST", NULL)) {
		return NULL;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 215) { 
		return NULL;
	}
	syst = ftp->inbuf;
	while (*syst == ' ') {
		syst++;
	}
	if ((end = strchr(syst, ' '))) {
		*end = 0;
	}
	ftp->syst = strdup(syst);
	if (end) {
		*end = ' ';
	}
	return ftp->syst;
}
/* }}} */

/* {{{ ftp_pwd
 */
const char*
ftp_pwd(ftpbuf_t *ftp)
{
	char *pwd, *end;

	if (ftp == NULL) {
		return NULL;
	}

	/* default to cached value */
	if (ftp->pwd) {
		return ftp->pwd;
	}
	if (!ftp_putcmd(ftp, "PWD", NULL)) {
		return NULL;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 257) { 
		return NULL;
	}
	/* copy out the pwd from response */
	if ((pwd = strchr(ftp->inbuf, '"')) == NULL) { 
		return NULL;
	}
	if ((end = strrchr(++pwd, '"')) == NULL) { 
		return NULL;
	}
	ftp->pwd = strndup(pwd, end - pwd);

	return ftp->pwd;
}
/* }}} */

/* {{{ ftp_exec
 */
int
ftp_exec(ftpbuf_t *ftp, const char *cmd)
{
	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "SITE EXEC", cmd)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 200) {
		return 0;
	}

	return 1;
}
/* }}} */

#if 0
/* {{{ ftp_raw
 */
void
ftp_raw(ftpbuf_t *ftp, const char *cmd, zval *return_value)
{
	if (ftp == NULL || cmd == NULL) {
		RETURN_NULL();
	}
	if (!ftp_putcmd(ftp, cmd, NULL)) {
		RETURN_NULL();
	}
	array_init(return_value);
	while (ftp_readline(ftp)) {
		add_next_index_string(return_value, ftp->inbuf, 1);
		if (isdigit(ftp->inbuf[0]) && isdigit(ftp->inbuf[1]) && isdigit(ftp->inbuf[2]) && ftp->inbuf[3] == ' ') {
			return;
		}
	}
}
/* }}} */
#endif


/* {{{ ftp_chdir
 */
int
ftp_chdir(ftpbuf_t *ftp, const char *dir)
{
	if (ftp == NULL) {
		return 0;
	}

	if (ftp->pwd) {
		free(ftp->pwd);
		ftp->pwd = NULL;
	}

	if (!ftp_putcmd(ftp, "CWD", dir)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 250) {
		return 0;
	}
	return 1;
}
/* }}} */

/* {{{ ftp_cdup
 */
int
ftp_cdup(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return 0;
	}

	if (ftp->pwd) {
		free(ftp->pwd);
		ftp->pwd = NULL;
	}

	if (!ftp_putcmd(ftp, "CDUP", NULL)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 250 && ftp->resp != 200) ) {
		return 0;
	}
	return 1;
}
/* }}} */

/* {{{ ftp_mkdir
 */
char*
ftp_mkdir(ftpbuf_t *ftp, const char *dir)
{
	char *mkd, *end;

	if (ftp == NULL) {
		return NULL;
	}
	if (!ftp_putcmd(ftp, "MKD", dir)) {
		return NULL;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 257) {
		return NULL;
	}
	/* copy out the dir from response */
	if ((mkd = strchr(ftp->inbuf, '"')) == NULL) {
		mkd = strdup(dir);
		return mkd;
	}
	if ((end = strrchr(++mkd, '"')) == NULL) {
		return NULL;
	}
	*end = 0;
	mkd = strdup(mkd);
	*end = '"';

	return mkd;
}
/* }}} */

/* {{{ ftp_rmdir
 */
int
ftp_rmdir(ftpbuf_t *ftp, const char *dir)
{
	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "RMD", dir)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 250) {
		return 0;
	}
	return 1;
}
/* }}} */

/* {{{ ftp_chmod
 */
int
ftp_chmod(ftpbuf_t *ftp, const int mode, const char *filename, const int filename_len)
{
	char buffer[1024];

	if (ftp == NULL || filename_len <= 0) {
		return 0;
	}

	snprintf(buffer, sizeof(buffer)-1, "CHMOD %o %s", mode, filename);

	if (!ftp_putcmd(ftp, "SITE", buffer)) {
		return 0;
	}	

	if (!ftp_getresp(ftp) || ftp->resp != 200) {
		return 0;
	}
	
	return 1;
}
/* }}} */

/* {{{ ftp_alloc
 */
int
ftp_alloc(ftpbuf_t *ftp, const int size, char **response)
{
	char buffer[64];

	if (ftp == NULL || size <= 0) {
		return 0;
	}

	snprintf(buffer, sizeof(buffer) - 1, "%d", size);

	if (!ftp_putcmd(ftp, "ALLO", buffer)) {
		return 0;
	}

	if (!ftp_getresp(ftp)) {
		return 0;
	}

	if (response && ftp->inbuf) {
		*response = strdup(ftp->inbuf);
	}

	if (ftp->resp < 200 || ftp->resp >= 300) {
		return 0;
	}

	return 1;	
}
/* }}} */

/* {{{ ftp_nlist
 */
char**
ftp_nlist(ftpbuf_t *ftp, const char *path )
{
	return ftp_genlist(ftp, "NLST", path );
}
/* }}} */

/* {{{ ftp_list
 */
char**
ftp_list(ftpbuf_t *ftp, const char *path, int recursive )
{
	return ftp_genlist(ftp, ((recursive) ? "LIST -R" : "LIST"), path );
}
/* }}} */

/* {{{ ftp_type
 */
int
ftp_type(ftpbuf_t *ftp, ftptype_t type)
{
	char typechar[2] = "?";

	if (ftp == NULL) {
		return 0;
	}
	if (type == ftp->type) { 
		return 1;
	}
	if (type == FTPTYPE_ASCII) {
		typechar[0] = 'A';
	} else if (type == FTPTYPE_IMAGE) {
		typechar[0] = 'I';
	} else {
		return 0;
	}
	if (!ftp_putcmd(ftp, "TYPE", typechar)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 200) {
		return 0;
	}
	ftp->type = type;

	return 1;
}
/* }}} */

/* {{{ ftp_pasv
 */
int
ftp_pasv(ftpbuf_t *ftp, int pasv)
{
	char			*ptr;
	union ipbox		ipbox;
	unsigned long		b[6];
	socklen_t			n;
	struct sockaddr *sa;
	struct sockaddr_in *sin;

	if (ftp == NULL) {
		return 0;
	}
	if (pasv && ftp->pasv == 2) {
		return 1;
	}
	ftp->pasv = 0;
	if (!pasv) {
		return 1;
	}
	n = sizeof(ftp->pasvaddr);
	memset(&ftp->pasvaddr, 0, n);
	sa = (struct sockaddr *) &ftp->pasvaddr;

#if HAVE_IPV6
	if (getpeername(ftp->fd, sa, &n) < 0) {
		return 0;
	}
	if (sa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;
		char *endptr, delimiter;

		/* try EPSV first */
		if (!ftp_putcmd(ftp, "EPSV", NULL)) {
			return 0;
		}
		if (!ftp_getresp(ftp)) {
			return 0;
		}
		if (ftp->resp == 229) {
			/* parse out the port */
			for (ptr = ftp->inbuf; *ptr && *ptr != '('; ptr++);
			if (!*ptr) {
				return 0;
			}
			delimiter = *++ptr;
			for (n = 0; *ptr && n < 3; ptr++) {
				if (*ptr == delimiter) {
					n++;
				}
			}

			sin6->sin6_port = htons((unsigned short) strtoul(ptr, &endptr, 10));
			if (ptr == endptr || *endptr != delimiter) {
				return 0;
			}
			ftp->pasv = 2;
			return 1;
		}
	}

	/* fall back to PASV */
#endif

	if (!ftp_putcmd(ftp, "PASV", NULL)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 227) { 
		return 0;
	}
	/* parse out the IP and port */
	for (ptr = ftp->inbuf; *ptr && !isdigit(*ptr); ptr++);
	n = sscanf(ptr, "%lu,%lu,%lu,%lu,%lu,%lu", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
	if (n != 6) {
		return 0;
	}
	for (n = 0; n < 6; n++) {
		ipbox.c[n] = (unsigned char) b[n];
	}
	sin = (struct sockaddr_in *) sa;
	sin->sin_family = AF_INET;
	sin->sin_addr = ipbox.ia[0];
	sin->sin_port = ipbox.s[2];

	ftp->pasv = 2;

	return 1;
}
/* }}} */

/* {{{ ftp_get
 */
int
ftp_get(ftpbuf_t *ftp, FILE *outstream, const char *path, ftptype_t type, int resumepos )
{
	databuf_t		*data = NULL;
	int				lastch, ret;
	size_t			rcvd;	
	char			arg[11];

	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_type(ftp, type)) {
		goto bail;
	}

	if ((data = ftp_getdata(ftp )) == NULL) {
		goto bail;
	}
	
	ftp->data = data;

	if (resumepos > 0) {
		if (resumepos > 2147483647) {
			g_warning( "PHP cannot handle files greater than 2147483647 bytes.");
			goto bail;
		}
		snprintf(arg, sizeof(arg), "%u", resumepos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "RETR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}

	if ((data = data_accept(data, ftp )) == NULL) {
		goto bail;
	}

	lastch = 0;
	while ((rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE))) {
		
		//g_message("%s : rcvd[%d]",__FUNCTION__, rcvd);
		//g_print("");
		
		if (rcvd == -1) {		
			g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);		
			goto bail;
		}

		if (type == FTPTYPE_ASCII) {
#ifndef PHP_WIN32
			char *s;
#endif
			char *ptr = data->buf;
			char *e = ptr + rcvd;
			/* logic depends on the OS EOL
			 * Win32 -> \r\n
			 * Everything Else \n
			 */
#ifdef PHP_WIN32
			_stream_write(outstream, ptr, (e - ptr));
			ptr = e;
#else
			while (e > ptr && (s = memchr(ptr, '\r', (e - ptr)))) {
				_stream_write(outstream, ptr, (s - ptr));
				if (*(s + 1) == '\n') {
					s++;
					_stream_putc(outstream, '\n');
				}
				ptr = s + 1;
			}
#endif
			if (ptr < e) {
				_stream_write(outstream, ptr, (e - ptr));
			}
		} else if (rcvd != (ret = _stream_write(outstream, data->buf, rcvd)) ) {
			
			g_warning("%s %d !!!!!!!!!!!! recv[%d] ret[%d]", __FUNCTION__,__LINE__, rcvd, ret);		
			goto bail;
		}
	}

	ftp->data = data = data_close(ftp, data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}

	return 1;
bail:
	ftp->data = data_close(ftp, data);
	return 0;
}
/* }}} */

/* {{{ ftp_put
 */
int
ftp_put(ftpbuf_t *ftp, const char *path, FILE *instream, ftptype_t type, int startpos )
{
	databuf_t		*data = NULL;
	int			size;
	char			*ptr;
	int			ch;
	char			arg[11];

	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_type(ftp, type)) {
		goto bail;
	}
	if ((data = ftp_getdata(ftp )) == NULL) {
		goto bail;
	}
	ftp->data = data;	

	if (startpos > 0) { 
		if (startpos > 2147483647) {
			g_warning( "PHP cannot handle files with a size greater than 2147483647 bytes.");
			goto bail;
		}
		snprintf(arg, sizeof(arg), "%u", startpos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "STOR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}
	if ((data = data_accept(data, ftp )) == NULL) {
		goto bail;
	}

	size = 0;
	ptr = data->buf;
	while (!_stream_eof(instream) && (ch = _stream_getc(instream))!=EOF) {
		/* flush if necessary */
		if (FTP_BUFSIZE - size < 2) {
			if (my_send(ftp, data->fd, data->buf, size) != size) {
				goto bail;
			}
			ptr = data->buf;
			size = 0;
		}

		if (ch == '\n' && type == FTPTYPE_ASCII) {
			*ptr++ = '\r';
			size++;
		}

		*ptr++ = ch;
		size++;
	}

	if (size && my_send(ftp, data->fd, data->buf, size) != size) {
		goto bail;
	}
	ftp->data = data = data_close(ftp, data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}
	return 1;
bail:
	ftp->data = data_close(ftp, data);
	return 0;
}
/* }}} */

/* {{{ ftp_size
 */
int
ftp_size(ftpbuf_t *ftp, const char *path)
{
	if (ftp == NULL) {
		return -1;
	}
	if (!ftp_type(ftp, FTPTYPE_IMAGE)) {
		return -1;
	}
	if (!ftp_putcmd(ftp, "SIZE", path)) {
		return -1;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 213) {
		return -1;
	}
	return atoi(ftp->inbuf);
}
/* }}} */

/* {{{ ftp_mdtm
 */
time_t
ftp_mdtm(ftpbuf_t *ftp, const char *path)
{
	time_t		stamp;
	struct tm	*gmt, tmbuf;
	struct tm	tm;
	char		*ptr;
	int		n;

	if (ftp == NULL) {
		return -1;
	}
	if (!ftp_putcmd(ftp, "MDTM", path)) {
		return -1;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 213) {
		return -1;
	}
	/* parse out the timestamp */
	for (ptr = ftp->inbuf; *ptr && !isdigit(*ptr); ptr++);
	n = sscanf(ptr, "%4u%2u%2u%2u%2u%2u", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	if (n != 6) {
		return -1;
	}
	tm.tm_year -= 1900;
	tm.tm_mon--;
	tm.tm_isdst = -1;

	/* figure out the GMT offset */
	stamp = time(NULL);
	gmt = gmtime_r(&stamp, &tmbuf);
	if (!gmt) {
		return -1;
	}
	gmt->tm_isdst = -1;

	/* apply the GMT offset */
	tm.tm_sec += stamp - mktime(gmt);
	tm.tm_isdst = gmt->tm_isdst;

	stamp = mktime(&tm);

	return stamp;
}
/* }}} */

/* {{{ ftp_delete
 */
int
ftp_delete(ftpbuf_t *ftp, const char *path)
{
	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "DELE", path)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 250) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ ftp_rename
 */
int
ftp_rename(ftpbuf_t *ftp, const char *src, const char *dest)
{
	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "RNFR", src)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 350) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "RNTO", dest)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 250) {
		return 0;
	}
	return 1;
}
/* }}} */

/* {{{ ftp_site
 */
int
ftp_site(ftpbuf_t *ftp, const char *cmd)
{
	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "SITE", cmd)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp < 200 || ftp->resp >= 300) {
		return 0;
	}

	return 1;
}
/* }}} */

/* static functions */

/* {{{ ftp_putcmd
 */
int
ftp_putcmd(ftpbuf_t *ftp, const char *cmd, const char *args)
{
	int		size;
	char		*data;
	
	if (strpbrk(cmd, "\r\n")) {
		return 0;
	} 
	/* build the output buffer */
	if (args && args[0]) {
		/* "cmd args\r\n\0" */
		if (strlen(cmd) + strlen(args) + 4 > FTP_BUFSIZE) {
			return 0;
		}
		if (strpbrk(args, "\r\n")) {
			return 0;
		}
		size = snprintf(ftp->outbuf, sizeof(ftp->outbuf)-1, "%s %s\r\n", cmd, args);
	} else {
		/* "cmd\r\n\0" */
		if (strlen(cmd) + 3 > FTP_BUFSIZE) {
			return 0;
		}
		size = snprintf(ftp->outbuf, sizeof(ftp->outbuf)-1, "%s\r\n", cmd);
	}

	data = ftp->outbuf;

	if (my_send(ftp, ftp->fd, data, size) != size) {
		g_warning("%s : cmd[%s]", __FUNCTION__, data);
		return 0;
	}
	//g_message("%s : cmd[%s]", __FUNCTION__, data);
	return 1;
}
/* }}} */

/* {{{ ftp_readline
 */
int
ftp_readline(ftpbuf_t *ftp)
{
	int		size, rcvd;
	char		*data, *eol;

	/* shift the extra to the front */
	size = FTP_BUFSIZE;
	rcvd = 0;
	if (ftp->extra) {
		memmove(ftp->inbuf, ftp->extra, ftp->extralen);
		rcvd = ftp->extralen;
	}

	data = ftp->inbuf;

	do {
		size -= rcvd;
		for (eol = data; rcvd; rcvd--, eol++) {
			if (*eol == '\r') {
				*eol = 0;
				ftp->extra = eol + 1;
				if (rcvd > 1 && *(eol + 1) == '\n') {
					ftp->extra++;
					rcvd--;
				}
				if ((ftp->extralen = --rcvd) == 0) {
					ftp->extra = NULL;
				}
				return 1;
			} else if (*eol == '\n') {
				*eol = 0;
				ftp->extra = eol + 1;
				if ((ftp->extralen = --rcvd) == 0) {
					ftp->extra = NULL;
				}
				return 1;
			}
		}

		data = eol;
		if ((rcvd = my_recv(ftp, ftp->fd, data, size)) < 1) {
			return 0;
		}
	} while (size);

	return 0;
}
/* }}} */

char * ftp_getresp_code2str( int code )
{	
	switch(code)
	{
		case 100: return "The requested action is being initiated";
		case 110: return "Restart marker replay.";
		case 120: return "Service ready in nnn minutes.";
		case 125: return "Data connection already open; transfer starting.";
		case 150: return "File status okay; about to open data connection.";
		case 200: return "Command okay.";
		case 202: return "Command not implemented, superfluous at this site.";
		case 211: return "System status, or system help reply.";
		case 212: return "Directory status.";
		case 213: return "File status.";
		case 214: return "Help message.";
		case 215: return "NAME system type.";
		case 220: return "Service ready for new user.";
		case 221: return "Service closing control connection.";
		case 225: return "Data connection open; no transfer in progress.";
		case 226: return "Closing data connection. Requested file action successful";
		case 227: return "Entering Passive Mode (h1,h2,h3,h4,p1,p2).";
		case 228: return "Entering Long Passive Mode (long address, port).";
		case 229: return "Entering Extended Passive Mode (|||port|).";
		case 230: return "User logged in, proceed. Logged out if appropriate.";
		case 231: return "User logged out; service terminated.";
		case 232: return "Logout command noted, will complete when transfer done.";
		case 250: return "Requested file action okay, completed.";
		case 257: return "PATHNAME created.";
		case 331: return "User name okay, need password.";
		case 332: return "Need account for login.";
		case 350: return "Requested file action pending further information";
		case 421: return "Service not available, closing control connection.";
		case 425: return "Can`t open data connection.";
		case 426: return "Connection closed; transfer aborted.";
		case 434: return "Requested host unavailable.";
		case 450: return "Requested file action not taken.";
		case 451: return "Requested action aborted. Local error in processing.";
		case 452: return "Requested action not taken. Insufficient storage space in system.";
		case 500: return "Syntax error, command unrecognized.";
		case 501: return "Syntax error in parameters or arguments.";
		case 502: return "Command not implemented.";
		case 503: return "Bad sequence of commands.";
		case 504: return "Command not implemented for that parameter.";
		case 530: return "Not logged in.";
		case 532: return "Need account for storing files.";
		case 550: return "Requested action not taken. File unavailable";
		case 551: return "Requested action aborted. Page type unknown.";
		case 552: return "Requested file action aborted. Exceeded storage allocation";
		case 553: return "Requested action not taken. File name not allowed.";
		default :
			return "unknown";
	}
	
}	
/* {{{ ftp_getresp
 */
int
ftp_getresp(ftpbuf_t *ftp)
{
	char *buf;

	if (ftp == NULL) {
		return 0;
	}
	buf = ftp->inbuf;
	ftp->resp = 0;

	while (1) {

		if (!ftp_readline(ftp)) {
			return 0;
		}

		/* Break out when the end-tag is found */
		if (isdigit(ftp->inbuf[0]) && isdigit(ftp->inbuf[1]) && isdigit(ftp->inbuf[2]) && ftp->inbuf[3] == ' ') {
			break;
		}
	}

	/* translate the tag */
	if (!isdigit(ftp->inbuf[0]) || !isdigit(ftp->inbuf[1]) || !isdigit(ftp->inbuf[2])) {
		return 0;
	}

	ftp->resp = 100 * (ftp->inbuf[0] - '0') + 10 * (ftp->inbuf[1] - '0') + (ftp->inbuf[2] - '0');

	//g_message("%s resp[%d][%s]", __FUNCTION__, ftp->resp, ftp_getresp_code2str( ftp->resp ) );
	
	memmove(ftp->inbuf, ftp->inbuf + 4, FTP_BUFSIZE - 4);

	if (ftp->extra) {
		ftp->extra -= 4;
	}
	return 1;
}
/* }}} */

/* {{{ my_send
 */
int
my_send(ftpbuf_t *ftp, int s, void *buf, size_t len)
{
	int		n, size, sent;

	size = len;
	while (size) {
		n = _pollfd_for_ms(s, POLLOUT, ftp->timeout_sec * 1000);

		if (n < 1) {

#if !defined(PHP_WIN32) && !(defined(NETWARE) && defined(USE_WINSOCK))
			if (n == 0) {
				errno = ETIMEDOUT;
			}
#endif
			return -1;
		}

#if HAVE_OPENSSL_EXT
		if (ftp->use_ssl && ftp->fd == s && ftp->ssl_active) {
			sent = SSL_write(ftp->ssl_handle, buf, size);
		} else if (ftp->use_ssl && ftp->fd != s && ftp->use_ssl_for_data && ftp->data->ssl_active) {	
			sent = SSL_write(ftp->data->ssl_handle, buf, size);
		} else {
#endif
			sent = send(s, buf, size, 0);
#if HAVE_OPENSSL_EXT
		}
#endif
		if (sent == -1) {
			return -1;
		}

		buf = (char*) buf + sent;
		size -= sent;
	}

	return len;
}
/* }}} */

/* {{{ my_recv
 */
int
my_recv(ftpbuf_t *ftp, int s, void *buf, size_t len)
{
	int		n, nr_bytes;

	n = _pollfd_for_ms(s, POLLIN, ftp->timeout_sec * 1000);
	if (n < 1) {
		return -1;
	}

#if HAVE_OPENSSL_EXT
	if (ftp->use_ssl && ftp->fd == s && ftp->ssl_active) {
		nr_bytes = SSL_read(ftp->ssl_handle, buf, len);
	} else if (ftp->use_ssl && ftp->fd != s && ftp->use_ssl_for_data && ftp->data->ssl_active) {	
		nr_bytes = SSL_read(ftp->data->ssl_handle, buf, len);
	} else {
#endif
		nr_bytes = recv(s, buf, len, 0);
#if HAVE_OPENSSL_EXT
	}
#endif	
	return (nr_bytes);
}
/* }}} */

/* {{{ data_available
 */
int
data_available(ftpbuf_t *ftp, int s)
{
	int		n;

	n = _pollfd_for_ms(s, POLLIN, 1000);
	if (n < 1) {
#if !defined(PHP_WIN32) && !(defined(NETWARE) && defined(USE_WINSOCK))
		if (n == 0) {
			errno = ETIMEDOUT;
		}
#endif
		return 0;
	}

	return 1;
}
/* }}} */
/* {{{ data_writeable
 */
int
data_writeable(ftpbuf_t *ftp, int s)
{
	int		n;

	n = _pollfd_for_ms(s, POLLOUT, 1000);
	if (n < 1) {
#ifndef PHP_WIN32
		if (n == 0) {
			errno = ETIMEDOUT;
		}
#endif
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ my_accept
 */
int
my_accept(ftpbuf_t *ftp, int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int		n;

	n = _pollfd_for_ms(s, POLLIN, ftp->timeout_sec * 1000);
	if (n < 1) {
#if !defined(PHP_WIN32) && !(defined(NETWARE) && defined(USE_WINSOCK))
		if (n == 0) {
			errno = ETIMEDOUT;
		}
#endif
		return -1;
	}

	return accept(s, addr, addrlen);
}
/* }}} */

/* {{{ ftp_getdata
 */
databuf_t*
ftp_getdata(ftpbuf_t *ftp )
{
	int			fd = -1;
	databuf_t		*data;
	_sockaddr_storage addr;
	struct sockaddr *sa;
	socklen_t 		size;
	union ipbox		ipbox;
	char			arg[sizeof("255, 255, 255, 255, 255, 255")];
	struct timeval	tv;


	/* ask for a passive connection if we need one */
	if (ftp->pasv && !ftp_pasv(ftp, 1)) {
		return NULL;
	}
	/* alloc the data structure */
	data = calloc(1, sizeof(*data));
	data->listener = -1;
	data->fd = -1;
	data->type = ftp->type;

	sa = (struct sockaddr *) &ftp->localaddr;

	/* passive connection handler */
	if (ftp->pasv) {
		/* clear the ready status */
		ftp->pasv = 1;

		/* connect */
		/* Win 95/98 seems not to like size > sizeof(sockaddr_in) */
		size = _sockaddr_size(&ftp->pasvaddr);
		tv.tv_sec = ftp->timeout_sec;
		tv.tv_usec = 0;
#if 1
		if ( (fd = connect_timeout((struct sockaddr*) &ftp->pasvaddr, size, 
			ftp->timeout_sec * 1000000 )) <0 ) {
			g_warning("%s : connect failed ret[%d]", __FUNCTION__, fd);
			goto bail;
		}
#else
		if (php_connect_nonb(fd, (struct sockaddr*) &ftp->pasvaddr, size, &tv) == -1) {
			g_warning( "php_connect_nonb() failed: %s (%d)", strerror(errno), errno);
			goto bail;
		}
#endif
		data->fd = fd;

		ftp->data = data;
		return data;
	
	}else{
		
		/* bind/listen */
		if ((fd = socket(sa->sa_family, SOCK_STREAM, 0)) == -1) {
			g_warning( "socket() failed: %s (%d)", strerror(errno), errno);
			goto bail;
		}
		
	}


	/* active (normal) connection */

	/* bind to a local address */
	_any_addr(sa->sa_family, &addr, 0);
	size = _sockaddr_size(&addr);

	if (bind(fd, (struct sockaddr*) &addr, size) == -1) {
		g_warning( "bind() failed: %s (%d)", strerror(errno), errno);
		goto bail;
	}

#if 1
	if (getsockname(fd, (struct sockaddr*) &addr, &size) == -1) {
		g_warning( "getsockname failed: %s (%d)", strerror(errno), errno);
		goto bail;
	}
#endif

	if (listen(fd, 5) == -1) {
		g_warning( "listen() failed: %s (%d)", strerror(errno), errno);
		goto bail;
	}

	data->listener = fd;

#if HAVE_IPV6 && HAVE_INET_NTOP
	if (sa->sa_family == AF_INET6) {
		/* need to use EPRT */
		char eprtarg[INET6_ADDRSTRLEN + sizeof("|x||xxxxx|")];
		char out[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &((struct sockaddr_in6*) sa)->sin6_addr, out, sizeof(out));
		snprintf(eprtarg, sizeof(eprtarg), "|2|%s|%hu|", out, ntohs(((struct sockaddr_in6 *) &addr)->sin6_port));

		if (!ftp_putcmd(ftp, "EPRT", eprtarg)) {
			goto bail;
		}

		if (!ftp_getresp(ftp) || ftp->resp != 200) {
			goto bail;
		}

		ftp->data = data;
		return data;
	}
#endif

	/* send the PORT */
	ipbox.ia[0] = ((struct sockaddr_in*) sa)->sin_addr;
	ipbox.s[2] = ((struct sockaddr_in*) &addr)->sin_port;
	snprintf(arg, sizeof(arg), "%u,%u,%u,%u,%u,%u", ipbox.c[0], ipbox.c[1], ipbox.c[2], ipbox.c[3], ipbox.c[4], ipbox.c[5]);

	if (!ftp_putcmd(ftp, "PORT", arg)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 200) {
		goto bail;
	}

	ftp->data = data;
	return data;

bail:
	if (fd != -1) {
		Close(fd);
	}
	free(data);
	return NULL;
}
/* }}} */

/* {{{ data_accept
 */
databuf_t*
data_accept(databuf_t *data, ftpbuf_t *ftp )
{
	_sockaddr_storage addr;
	socklen_t			size;

#if HAVE_OPENSSL_EXT
	SSL_CTX		*ctx;
#endif

	if (data->fd != -1) {
		goto data_accepted;
	}
	size = sizeof(addr);
	data->fd = my_accept(ftp, data->listener, (struct sockaddr*) &addr, &size);
	Close(data->listener);
	data->listener = -1;

	if (data->fd == -1) {
		free(data);
		return NULL;
	}

data_accepted:
#if HAVE_OPENSSL_EXT
	
	/* now enable ssl if we need to */
	if (ftp->use_ssl && ftp->use_ssl_for_data) {
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL) {
			g_warning( "data_accept: failed to create the SSL context");
			return 0;
		}

		SSL_CTX_set_options(ctx, SSL_OP_ALL);

		data->ssl_handle = SSL_new(ctx);
		if (data->ssl_handle == NULL) {
			g_warning( "data_accept: failed to create the SSL handle");
			SSL_CTX_free(ctx);
			return 0;
		}
			
		
		SSL_set_fd(data->ssl_handle, data->fd);

		if (ftp->old_ssl) {
			SSL_copy_session_id(data->ssl_handle, ftp->ssl_handle);
		}
			
		if (SSL_connect(data->ssl_handle) <= 0) {
			g_warning( "data_accept: SSL/TLS handshake failed");
			SSL_shutdown(data->ssl_handle);
			return 0;
		}
			
		data->ssl_active = 1;
	}	

#endif

	return data;
}
/* }}} */

/* {{{ data_close
 */
databuf_t*
data_close(ftpbuf_t *ftp, databuf_t *data)
{
	if (data == NULL) {
		return NULL;
	}
	if (data->listener != -1) {
#if HAVE_OPENSSL_EXT
		if (data->ssl_active) {
			SSL_shutdown(data->ssl_handle);
			data->ssl_active = 0;
		}
#endif				
		Close(data->listener);
	}	
	if (data->fd != -1) {
#if HAVE_OPENSSL_EXT
		if (data->ssl_active) {
			SSL_shutdown(data->ssl_handle);
			data->ssl_active = 0;
		}
#endif				
		Close(data->fd);
	}	
	if (ftp) {
		ftp->data = NULL;
	}
	free(data);
	return NULL;
}
/* }}} */


/* {{{ ftp_genlist
 */
char**
ftp_genlist(ftpbuf_t *ftp, const char *cmd, const char *path )
{
	FILE *tmpstream = NULL;
	databuf_t	*data = NULL;
	char		*ptr;
	int		ch, lastch;
	int		size, rcvd;
	int		lines;
	char		**ret = NULL;
	char		**entry;
	char		*text;


	if ((tmpstream = tmpfile()) == NULL) {
		g_warning( "Unable to create temporary file.  Check permissions in temporary files directory.");
		return NULL;
	}

	if (!ftp_type(ftp, FTPTYPE_ASCII)) {
		//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
		goto bail;
	}

	if ((data = ftp_getdata(ftp )) == NULL) {
		goto bail;
	}
	ftp->data = data;	

	if (!ftp_putcmd(ftp, cmd, path)) {
		//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125 && ftp->resp != 226)) {
		//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
		goto bail;
	}

	/* some servers don't open a ftp-data connection if the directory is empty */
	if (ftp->resp == 226) {
		
		//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
		
		ftp->data = data_close(ftp, data);
		_stream_close(tmpstream);
		return calloc(1, sizeof(char**));
	}

	/* pull data buffer into tmpfile */
	if ((data = data_accept(data, ftp )) == NULL) {
		
		//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
		goto bail;
	}
	size = lines = lastch = 0;
	while ((rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE))) {
		if (rcvd == -1) {
			g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
			goto bail;
		}

		_stream_write(tmpstream, data->buf, rcvd);

		size += rcvd;
		for (ptr = data->buf; rcvd; rcvd--, ptr++) {
			if (*ptr == '\n' && lastch == '\r') {
				lines++;
			} else {
				size++;
			}
			lastch = *ptr;
		}
	}

	ftp->data = data = data_close(ftp, data);

	_stream_rewind(tmpstream);

	ret = calloc( 1, (lines + 1)*sizeof(char**) + size*sizeof(char*) );

	entry = ret;
	text = (char*) (ret + lines + 1);
	*entry = text;
	lastch = 0;
	while ((ch = _stream_getc(tmpstream)) != EOF) {
		if (ch == '\n' && lastch == '\r') {
			*(text - 1) = 0;
			*++entry = text;
		} else {
			*text++ = ch;
		}
		lastch = ch;
	}
	*entry = NULL;

	_stream_close(tmpstream);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		
		//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
		free(ret);
		return NULL;
	}

	return ret;
bail:
	
	//g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);
	
	ftp->data = data_close(ftp, data);
	_stream_close(tmpstream);
	if (ret)
		free(ret);
	return NULL;
}
/* }}} */

/* {{{ ftp_nb_get
 */
int
ftp_nb_get(ftpbuf_t *ftp, FILE *outstream, const char *path, ftptype_t type, int resumepos )
{
	databuf_t		*data = NULL;
	char			arg[11];

	if (ftp == NULL) {
		goto bail;
	}

	if (!ftp_type(ftp, type)) {
		goto bail;
	}

	if ((data = ftp_getdata(ftp )) == NULL) {
		goto bail;
	}

	if (resumepos>0) {
		/* We are working on an architecture that supports 64-bit integers
		 * since php is 32 bit by design, we bail out with warning
		 */
		if (resumepos > 2147483647) {
			g_warning( "PHP cannot handle files greater than 2147483648 bytes.");
			goto bail;
		}
		snprintf(arg, sizeof(arg), "%u", resumepos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "RETR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}

	if ((data = data_accept(data, ftp )) == NULL) {
		goto bail;
	}

	ftp->data = data;
	ftp->stream = outstream;
	ftp->lastch = 0;
	ftp->nb = 1;

	return (ftp_nb_continue_read(ftp ));

bail:
	ftp->data = data_close(ftp, data);
	return RET_FTP_FAILED;
}
/* }}} */

/* {{{ ftp_nb_continue_read
 */
int
ftp_nb_continue_read(ftpbuf_t *ftp )
{
	databuf_t	*data = NULL;
	char		*ptr;
	int		lastch, ret;
	size_t		rcvd;
	ftptype_t	type;

	data = ftp->data;

	/* check if there is already more data */
	if (!data_available(ftp, data->fd)) {
		return RET_FTP_MOREDATA;
	}

	type = ftp->type;

	lastch = ftp->lastch;
	if ((rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE))) {
		if (rcvd == -1) {
			g_warning("%s %d !!!!!!!!!!!!", __FUNCTION__,__LINE__);		
			goto bail;
		}

		if (type == FTPTYPE_ASCII) {
			for (ptr = data->buf; rcvd; rcvd--, ptr++) {
				if (lastch == '\r' && *ptr != '\n') {
					_stream_putc(ftp->stream, '\r');
				}
				if (*ptr != '\r') {
					_stream_putc(ftp->stream, *ptr);
				}
				lastch = *ptr;
			}
		} else if (rcvd != (ret = _stream_write(ftp->stream, data->buf, rcvd)) ) {
			g_warning("%s %d !!!!!!!!!!!! recv[%d] ret[%d]", __FUNCTION__,__LINE__, rcvd, ret);		
			goto bail;
		}

		ftp->lastch = lastch;
		return RET_FTP_MOREDATA;
	}

	if (type == FTPTYPE_ASCII && lastch == '\r') {
		_stream_putc(ftp->stream, '\r');
	}

	ftp->data = data = data_close(ftp, data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}

	ftp->nb = 0;
	return RET_FTP_FINISHED;
bail:
	ftp->nb = 0;
	ftp->data = data_close(ftp, data);
	return RET_FTP_FAILED;
}
/* }}} */

/* {{{ ftp_nb_put
 */
int
ftp_nb_put(ftpbuf_t *ftp, const char *path, FILE *instream, ftptype_t type, int startpos )
{
	databuf_t		*data = NULL;
	char			arg[11];

	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_type(ftp, type)) {
		goto bail;
	}
	if ((data = ftp_getdata(ftp )) == NULL) {
		goto bail;
	}
	if (startpos > 0) {
		if (startpos > 2147483647) {
			g_warning( "PHP cannot handle files with a size greater than 2147483647 bytes.");
			goto bail;
		}
		snprintf(arg, sizeof(arg), "%u", startpos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "STOR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}
	if ((data = data_accept(data, ftp )) == NULL) { 
		goto bail;
	}
	ftp->data = data;
	ftp->stream = instream;
	ftp->lastch = 0;
	ftp->nb = 1;

	return (ftp_nb_continue_write(ftp ));

bail:
	ftp->data = data_close(ftp, data);
	return RET_FTP_FAILED;
}
/* }}} */


/* {{{ ftp_nb_continue_write
 */
int
ftp_nb_continue_write(ftpbuf_t *ftp )
{
	int			size;
	char			*ptr;
	int 			ch;

	/* check if we can write more data */
	if (!data_writeable(ftp, ftp->data->fd)) {
		return RET_FTP_MOREDATA;
	}

	size = 0;
	ptr = ftp->data->buf;
	while (!_stream_eof(ftp->stream) && (ch = _stream_getc(ftp->stream)) != EOF) {

		if (ch == '\n' && ftp->type == FTPTYPE_ASCII) {
			*ptr++ = '\r';
			size++;
		}

		*ptr++ = ch;
		size++;

		/* flush if necessary */
		if (FTP_BUFSIZE - size < 2) {
			if (my_send(ftp, ftp->data->fd, ftp->data->buf, size) != size) {
				goto bail;
			}
			return RET_FTP_MOREDATA;
		}
	}

	if (size && my_send(ftp, ftp->data->fd, ftp->data->buf, size) != size) {
		goto bail;
	}
	ftp->data = data_close(ftp, ftp->data);
 
	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}
	ftp->nb = 0;
	return RET_FTP_FINISHED;
bail:
	ftp->data = data_close(ftp, ftp->data);
	ftp->nb = 0;
	return RET_FTP_FAILED;
}
/* }}} */


#ifdef DEBUG_FTP_CLIENT_JBSHELL

static ftpbuf_t *_jbshell_ftp = NULL;

static char ftp_open_help[] = "ftp_open [host] [port:21] [timeout:3]";
static int jbshell_ftp_open(int argc, char **argv)
{	
	gint port = 21;
	gint tout = 3;
	char *host;
	gint is_test = 0;
	
	g_return_val_if_fail ( _jbshell_ftp == NULL, 0 );
			
	if(argc < 2){
		printf("%s\n",ftp_open_help);
		return -1;
	}

	if( strncmp(argv[1],"choissi",7) == 0 )
	{
		host = "192.168.100.71";
		is_test = 1;
	}else{
		host = argv[1];
	}
				
	if(argc >2)	port = strtoul(argv[2],NULL,0);
	if(argc >3)	tout = strtoul(argv[3],NULL,0);
	
	_jbshell_ftp = ftp_open( host, (unsigned short)port, tout);
		
	if( is_test )
	{
		//my_command("ftp_login ftp choissi@ggg.com");
		my_command("ftp_login 1 1");
		my_command("ftp_pasv 1");
	}
		
	return 0;
}
__commandlist(jbshell_ftp_open,"ftp_open", ftp_open_help, ftp_open_help);


static char ftp_close_help[] = "ftp_close";
static int jbshell_ftp_close(int argc, char **argv)
{	

	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );
				
	_jbshell_ftp = ftp_close( _jbshell_ftp );
	
		
	return 0;
}
__commandlist(jbshell_ftp_close,"ftp_close", ftp_close_help, ftp_close_help);


static char ftp_login_help[] = "ftp_login [id] [passwd]";
static int jbshell_ftp_login(int argc, char **argv)
{	

	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );
							
	if(argc < 3){
		printf("%s\n",ftp_login_help);
		return -1;
	}		
	
	ftp_login( _jbshell_ftp, argv[1], argv[2]);
	
	return 0;
}
__commandlist(jbshell_ftp_login,"ftp_login", ftp_login_help, ftp_login_help);


static char ftp_quit_help[] = "ftp_quit";
static int jbshell_ftp_quit(int argc, char **argv)
{	

	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );
				
	ftp_quit(_jbshell_ftp);
		
	return 0;
}
__commandlist(jbshell_ftp_quit,"ftp_quit", ftp_quit_help, ftp_quit_help);


static char ftp_pasv_help[] = "ftp_pasv [is_pasv:1]";
static int jbshell_ftp_pasv(int argc, char **argv)
{	
	gint is_pasv = 1;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc >1)	is_pasv = strtoul(argv[1],NULL,0); 
				
	ftp_pasv( _jbshell_ftp, is_pasv );

	return 0;
}
__commandlist(jbshell_ftp_pasv,"ftp_pasv", ftp_pasv_help, ftp_pasv_help);


static char ftp_delete_help[] = "ftp_delete [filename]";
static int jbshell_ftp_delete(int argc, char **argv)
{	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_delete_help);
		return -1;
	}
									
	ftp_delete( _jbshell_ftp, argv[1] );

	return 0;
}
__commandlist(jbshell_ftp_delete,"ftp_delete", ftp_delete_help, ftp_delete_help);



static char ftp_size_help[] = "ftp_size [filename]";
static int jbshell_ftp_size(int argc, char **argv)
{	
	int size = 0;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_size_help);
		return -1;
	}
									
	size = ftp_size( _jbshell_ftp, argv[1] );	
	g_print("filename[%s] size[%d]\n", argv[1], size);
	
	return 0;
}
__commandlist(jbshell_ftp_size,"ftp_size", ftp_size_help, ftp_size_help);


static char ftp_chdir_help[] = "ftp_chdir [dirname]";
static int jbshell_ftp_chdir(int argc, char **argv)
{	
		
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_chdir_help);
		return -1;
	}
									
	ftp_chdir( _jbshell_ftp, argv[1] );	
	
	
	return 0;
}
__commandlist(jbshell_ftp_chdir,"ftp_chdir", ftp_chdir_help, ftp_chdir_help);


static char ftp_mkdir_help[] = "ftp_mkdir [dirname]";
static int jbshell_ftp_mkdir(int argc, char **argv)
{	
	char *ret_dir = NULL;
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_mkdir_help);
		return -1;
	}
									
	ret_dir = ftp_mkdir( _jbshell_ftp, argv[1] );	

	if(ret_dir)
		free(ret_dir);
	else
		printf("mkdir failed!!\n");
		
			
	return 0;
}
__commandlist(jbshell_ftp_mkdir,"ftp_mkdir", ftp_mkdir_help, ftp_mkdir_help);


static char ftp_rmdir_help[] = "ftp_rmdir [dirname]";
static int jbshell_ftp_rmdir(int argc, char **argv)
{	
		
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_rmdir_help);
		return -1;
	}
									
	ftp_rmdir( _jbshell_ftp, argv[1] );	
	
	
	return 0;
}
__commandlist(jbshell_ftp_rmdir,"ftp_rmdir", ftp_rmdir_help, ftp_rmdir_help);


static char ftp_list_help[] = "ftp_list [dirname]";
static int jbshell_ftp_list(int argc, char **argv)
{	
	char		**llist = NULL, **ptr = NULL;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_list_help);
		return -1;
	}
									
	if (NULL == (llist = ftp_list(_jbshell_ftp, argv[1], 0))) {
		return -2;
	} 									
		
	for (ptr = llist; *ptr; ptr++) {
		g_print("list [%s]\n", *ptr);
	} 		

	free(llist);
		
	return 0;
}
__commandlist(jbshell_ftp_list,"ftp_list", ftp_list_help, ftp_list_help);


static char ftp_get_help[] = "ftp_get [remotefile] [savefile]";
static int jbshell_ftp_get(int argc, char **argv)
{			
	char *remote, *local;
	char filename[256];
	int i, ret;
	
	FILE *fp = NULL;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 3){
		printf("%s\n",ftp_get_help);
		return -1;
	}	
	remote = argv[1]; local = argv[2];
	
	// remove '/'
	for(i=0; local[i];++i)
	{
		if( local[i] == '/' )
			local[i] = '_';
	}							
	
	snprintf(filename, sizeof(filename)-1, "/tmp/%s", local);
	fp = fopen(filename, "w");
	
	g_return_val_if_fail( fp != NULL, -1);
	
	ret = ftp_get( _jbshell_ftp, fp, remote, FTPTYPE_IMAGE, 0);
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}
__commandlist(jbshell_ftp_get,"ftp_get", ftp_get_help, ftp_get_help);


static char ftp_nget_help[] = "ftp_nget [remotefile] [savefile]";
static int jbshell_ftp_nget(int argc, char **argv)
{			
	char *remote, *local;
	char filename[256];
	int i, ret;
	
	FILE *fp = NULL;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 3){
		printf("%s\n",ftp_nget_help);
		return -1;
	}	
	remote = argv[1]; local = argv[2];
	
	// remove '/'
	for(i=0; local[i];++i)
	{
		if( local[i] == '/' )
			local[i] = '_';
	}							
	
	snprintf(filename, sizeof(filename)-1, "/tmp/%s", local);
	fp = fopen(filename, "w");
	
	g_return_val_if_fail( fp != NULL, -1);
	
	ret = ftp_nb_get( _jbshell_ftp, fp, remote, FTPTYPE_IMAGE, 0);
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}
__commandlist(jbshell_ftp_nget,"ftp_nget", ftp_nget_help, ftp_nget_help);



static char ftp_put_help[] = "ftp_put [localfile]";
static int jbshell_ftp_put(int argc, char **argv)
{			
	char *remote, *local;
	char filename[256];
	int i, ret;
	
	FILE *fp = NULL;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_put_help);
		return -1;
	}	
	
	local = argv[1];
	
	// remove '/'
	for(i=0; local[i];++i)
	{
		if( local[i] == '/' )
			local[i] = '_';
	}							
	
	snprintf(filename, sizeof(filename)-1, "/tmp/%s", local);
	fp = fopen(filename, "rb");
	
	g_return_val_if_fail( fp != NULL, -1);
	
	ret = ftp_put( _jbshell_ftp, local, fp,  FTPTYPE_IMAGE, 0);		
	fclose(fp);		
	
	return 0;
}
__commandlist(jbshell_ftp_put,"ftp_put", ftp_put_help, ftp_put_help);

static char ftp_nput_help[] = "ftp_nput [localfile]";
static int jbshell_ftp_nput(int argc, char **argv)
{			
	char *remote, *local;
	char filename[256];
	int i, ret;
	
	FILE *fp = NULL;
	
	g_return_val_if_fail ( _jbshell_ftp != NULL, 0 );

	if(argc < 2){
		printf("%s\n",ftp_nput_help);
		return -1;
	}	
	local = argv[1];
	
	// remove '/'
	for(i=0; local[i];++i)
	{
		if( local[i] == '/' )
			local[i] = '_';
	}							
	
	snprintf(filename, sizeof(filename)-1, "/tmp/%s", local);
	fp = fopen(filename, "rb");
	
	g_return_val_if_fail( fp != NULL, -1);
	
	ret = ftp_nb_put( _jbshell_ftp, local, fp, FTPTYPE_IMAGE, 0);
	
	fclose(fp);		
	
	return 0;
}
__commandlist(jbshell_ftp_nput,"ftp_nput", ftp_nput_help, ftp_nput_help);


#endif


gboolean nf_ftp_client_connect_test( NF_FTP_CLIENT_REQ *req, GError **error )
{
	ftpbuf_t *h_ftp = NULL;
	gint ret = 0;
	
	if( req == NULL ) goto err_param;
	if( req->server[0]  == 0 ) goto err_param;
			
	if( !req->is_anon )
	{
		if( req->user[0]  == 0 ) goto err_param;
		if( req->passwd[0]  == 0 ) goto err_param;
	
	}else{
		strcpy( req->user, "anonymous");
		strcpy( req->passwd, "ftp@dvr.net");
	}
	
	req->timeout_connent_sec = 5;
			
	h_ftp = ftp_open( req->server,
						(unsigned short)req->port,
						(long) req->timeout_connent_sec);
	if( h_ftp  == NULL)
	{
		g_set_error( error, NF_API_FTP_ERROR, NF_FTP_CLIENT_ERROR_CONN, "CONN" );
		return 0;
	}

	ret = ftp_login( h_ftp, req->user, req->passwd);
	ftp_quit( h_ftp ); ftp_close( h_ftp );
		
	if( ret != 1)
	{
		g_set_error( error, NF_API_FTP_ERROR, NF_FTP_CLIENT_ERROR_AUTH, "AUTH" );
		return 0;		
	}
	
	return 1;
	
err_param:
	g_set_error( error, NF_API_FTP_ERROR, NF_FTP_CLIENT_ERROR_PARAMETER, "PARAM" );
	return 0;
		
}

/******************************************************************************/


/******************************************************************************/


typedef struct _NF_NET_SEND_INTERNAL_T	{
#if 0
	GTimeVal	enque_tv;	
	GTimeVal	process_tv;
	gint		retry_cnt;
#endif			
	NF_NET_SEND_SERVER		server;
	NF_NET_SEND_CONTENT		content;
					
} NF_NET_SEND_INTERNAL;


static GQuark 
_nf_net_send_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_NET_SEND_ERROR 	_nf_net_send_error_quark()

#ifdef DEBUG_NET_SEND_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_NET_SEND_IDX_E
{ 
	DEBUG_NET_SEND_IDX_INIT				= 0,
	DEBUG_NET_SEND_IDX_LIBESMTP			= 1,
	DEBUG_NET_SEND_IDX_QUEUE			= 2,
	DEBUG_NET_SEND_IDX_REQUEST			= 3,

	DEBUG_NET_SEND_IDX_TEST				= 4,	
	DEBUG_NET_SEND_IDX_REQUEST_RAW		= 5,	
	DEBUG_NET_SEND_IDX_NR				= 6	
}DEBUG_NET_SEND_IDX_E;

static const char *_DEBUG_NET_SEND_str[32] =
{
	"NET_SEND_IDX_INIT",
	"NET_SEND_IDX_LIBESMTP",
	"NET_SEND_IDX_QUEUE",	
	"NET_SEND_IDX_REQUEST",
	
	"NET_SEND_IDX_TEST",		
	"NET_SEND_IDX_REQUEST_RAW",		
	"NET_SEND_IDX_NR"
};

static gint _DEBUG_NET_SEND_log[32] = 
{
	1,0,0,1, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 
};

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

static void nf_net_send_class_init (NfNetSendClass * klass);
static void nf_net_send_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_net_send_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_net_send_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_net_send_dispose (GObject * object);
static void nf_net_send_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfNetSend	*_nf_net_send = NULL;

static void net_send_thread_func (NfNetSend * self);
static int _net_send_process(NF_NET_SEND_INTERNAL* info);

static gboolean _sysdb_get_ftp_info( NF_NET_SEND_SERVER *svr);

static void _free_ninfo( NF_NET_SEND_INTERNAL *ninfo);
static void _dump_ninfo( NF_NET_SEND_INTERNAL *ninfo);
static void _dump_svr( NF_NET_SEND_SERVER *server );
static void _dump_cont( NF_NET_SEND_CONTENT *content );

static int _check_param_content(NF_NET_SEND_CONTENT *cont);
static int _check_param_server(NF_NET_SEND_SERVER *svr);

GType
nf_net_send_get_type (void)
{
	static GType nf_net_send_type = 0;

	if (G_UNLIKELY (nf_net_send_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfNetSendClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_net_send_class_init,
			NULL,
			NULL,
			sizeof (NfNetSend),
			0,
			(GInstanceInitFunc) nf_net_send_instance_init,
			NULL
		};

		nf_net_send_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfNetSend", &object_info, 0);
	}
	
	return nf_net_send_type;
}

static void
nf_net_send_class_init (NfNetSendClass * klass)
{	
	GObjectClass *gobject_class;
	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_net_send_set_property;
	gobject_class->get_property = nf_net_send_get_property;
			
	gobject_class->dispose = nf_net_send_dispose;
	gobject_class->finalize = nf_net_send_finalize;

}

static void
nf_net_send_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfNetSend *self = NF_NET_SEND (instance);
				
	self->init_done	= 0;
	
	// queue 积己
	self->queue = g_async_queue_new();
 		 
	// notification signal emit侩 thread 积己
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)net_send_thread_func, 
									self, FALSE, NULL);
																		
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_net_send_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_net_send_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_net_send_set_property (GObject * object, guint prop_id,
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
nf_net_send_get_property (GObject * object, guint prop_id,
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

static int _ftp_upload( ftpbuf_t *h_ftp, char *in_file, char *in_ext, char *buf, int len)
{
	FILE *fp = NULL;
	char fname[256], uname[256];	
	int ret = 0;

	// mktemp
	if(in_ext){
		snprintf(fname, sizeof(fname)-1, "/tmp/%s.%s", in_file, in_ext);		
		snprintf(uname, sizeof(uname)-1, "%s.%s", in_file, in_ext);
	}else{
		snprintf(fname, sizeof(fname)-1, "/tmp/%s", in_file);
		snprintf(uname, sizeof(uname)-1, "%s", in_file);
	}
		
	fp = fopen( fname  , "wb") ;
	if(fp){
		char *pos = buf;
		int size = len, wr_size = 0;

		while( size >0 ){
			
			if(size >= 2048) wr_size = 2048;
			else wr_size = size;

			ret = fwrite( pos , wr_size , 1, fp);	
			g_message("%s file[%s] len[%d] ret[%d]",__FUNCTION__, in_file, wr_size, ret);						
			size -= wr_size;
			pos += wr_size;
		}

		fflush(fp);
		fclose(fp);
	}else{	
		return 0;
	}	
	
	// remove
	ftp_delete(h_ftp, uname);	
	
	// file upload	
	fp = fopen(fname, "rb");
	if(fp){
		ret = ftp_put( h_ftp, uname, fp, FTPTYPE_IMAGE, 0);	
		fclose(fp);
	}			
	
	remove( fname );
	return ret;	
}

static int _ftp_upload_file( ftpbuf_t *h_ftp, char *in_file, char *in_ext, char *filename)
{
	FILE *fp = NULL;
	char uname[256];	
	int ret = 0;

	// mktemp
	if(in_ext){
		snprintf(uname, sizeof(uname)-1, "%s.%s", in_file, in_ext);
	}else{
		snprintf(uname, sizeof(uname)-1, "%s", in_file);
	}
	
	// remove
	ftp_delete(h_ftp, uname);	
	
	// file upload	
	fp = fopen(filename, "rb");
	if(fp){
		ret = ftp_put( h_ftp, uname, fp, FTPTYPE_IMAGE, 0);
		fclose(fp);
	}			
	
	return ret;	
}

static void 
_net_ftp_send_process(NF_NET_SEND_INTERNAL *ninfo)
{	

	ftpbuf_t *h_ftp = NULL; 
	FILE *fp = NULL;
	int i, ret;
									
	h_ftp = ftp_open( ninfo->server.server, 
						(unsigned short)ninfo->server.port , 3); 
	
	g_return_if_fail(h_ftp != NULL);	
	
	ret = ftp_login( h_ftp, ninfo->server.user , ninfo->server.passwd);		
	if( ret != 1){
		g_warning("%s login failed[%s][%s]",__FUNCTION__, ninfo->server.user , ninfo->server.passwd);
		goto bail;
	}
	
	// enter pasv
	ftp_pasv( h_ftp, 1);	
		
	// enter directory
	if( ninfo->content.ftp_dir[0] )
	{
		ret = ftp_chdir( h_ftp, ninfo->content.ftp_dir );
		if( ret != 1)
		{
			char *ret_mkdir = NULL;
			
			// MUST mem Free
			ret_mkdir = ftp_mkdir( h_ftp, ninfo->content.ftp_dir );	
			if(ret_mkdir){
				free(ret_mkdir);
				//g_usleep(500000);
				ret = ftp_chdir( h_ftp, ninfo->content.ftp_dir );
				if( ret != 1)
				{
					g_warning("%s chdir failed!![%s]", __FUNCTION__, ninfo->content.ftp_dir);
					goto bail;													
				}				
			}else {
				g_warning("%s directory create failed!![%s]", __FUNCTION__, ninfo->content.ftp_dir);
				goto bail;			
				
			}
		}				
	}
	
	// for contents
	if( ninfo->content.contents[0] )
		_ftp_upload( h_ftp, ninfo->content.subject, "txt", 	
						ninfo->content.contents, 
						strlen(ninfo->content.contents) );

	// for link
	if( ninfo->content.webra_link[0] )
		_ftp_upload( h_ftp, ninfo->content.subject, "url", 	
						ninfo->content.webra_link, 
						strlen(ninfo->content.webra_link) );
						
	// for jpeg
	if( ninfo->content.image_size )
	{
		_ftp_upload( h_ftp, ninfo->content.image_name, NULL, 	
						ninfo->content.image_data, 						
						ninfo->content.image_size );		
	}

	if( ninfo->content.video_start_time != 0 && ninfo->content.video_end_time != 0 )
	{
		GTimeVal start_time;
		GTimeVal end_time;		
		char mp4_file[256];
		NF_RECORD_MP4_LIST mp4_list;
		int ret;

		GUINT64_TO_GTIMEVAL(ninfo->content.video_start_time, start_time);
		GUINT64_TO_GTIMEVAL(ninfo->content.video_end_time, end_time);

		snprintf(mp4_file, sizeof(mp4_file), "/tmp/%s", ninfo->content.video_name);

		ret = _make_mp4_by_time(ninfo->content.video_ch, start_time, end_time, mp4_file, &mp4_list);
		if(ret == 0)
		{
			int i;
			
			if( mp4_list.num > 0 )
			{
				for(i=0; i < mp4_list.num; i++)
				{
					g_message("%s - IDX:%d, mp4_name:%s", __FUNCTION__, i, mp4_list.mp4_name[i]);
				}
				
				for(i=0; i < mp4_list.num; i++)
				{
					_ftp_upload_file( h_ftp, ninfo->content.video_name, NULL, mp4_list.mp4_name[i]);
				}

				_remove_mp4_list(&mp4_list);
			}
		}
	}
								
bail:	
	ftp_quit( h_ftp ); ftp_close( h_ftp );
	return;		
}

//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	
static void
net_send_thread_func (NfNetSend * self)
{
	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_net_send == NULL ) g_usleep(10*1000);
		
	self->init_done = 1;
	
	while(self->thread_run)
	{
		NF_NET_SEND_INTERNAL *ninfo  = g_async_queue_pop( self->queue);
		if(ninfo)
		{

#ifdef DEBUG_NET_SEND_LOG
			if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_QUEUE] )
				_dump_ninfo(ninfo);
#endif
			if(ninfo->server.type == NF_NET_SEND_SERVER_TYPE_FTP)
				_net_ftp_send_process(ninfo);
			else
				g_warning("%s unknown server type[%d]", ninfo->server.type);
				
			_free_ninfo(ninfo);
			g_usleep(10*1000);			
		}						
	}
	g_message("%s end", __FUNCTION__);
}


/**
	@brief				net_send 檬扁拳
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_net_send_init( int wait )
{
	gboolean ret = TRUE;
	gint r = 0;
		
	g_return_val_if_fail (_nf_net_send == NULL, FALSE);	
	
	_nf_net_send = g_object_new ( NF_TYPE_NET_SEND , NULL);

	nf_debug_category_add( "net", _DEBUG_NET_SEND_str, _DEBUG_NET_SEND_log, DEBUG_NET_SEND_IDX_NR);
																																
	if( wait )
	{
		while( _nf_net_send->init_done != 1)
			g_usleep(10*1000);
	}
		
	return ret;
}


static gboolean
_sysdb_get_ftp_info( NF_NET_SEND_SERVER *svr )
{
	g_return_val_if_fail (svr, 0);	
#if 0	
	char *server = nf_sysdb_get_str_nocopy( "net.ftpserv.host");
	char *user   = nf_sysdb_get_str_nocopy( "net.ftpserv.username");
	char *passwd = nf_sysdb_get_str_nocopy( "net.ftpserv.passwd");
	
	int port =  nf_sysdb_get_uint( "net.ftpserv.port");
	int security =  0; //nf_sysdb_get_bool( "net.email.ssl");
#else
/*
<item key="act.event.ftp.host" type="STRING" min="0" max="32" val="" />
<item key="act.event.ftp.port" type="UINT" min="1" max="65535" val="21" />
<item key="act.event.ftp.username" type="STRING" min="0" max="64" val="" />
<item key="act.event.ftp.passwd" type="STRING" min="0" max="16" val="" />
*/
	char *server = nf_sysdb_get_str_nocopy( "act.event.ftp.host");
	char *user   = nf_sysdb_get_str_nocopy( "act.event.ftp.username");
	char *passwd = nf_sysdb_get_str_nocopy( "act.event.ftp.passwd");	
	int port =  nf_sysdb_get_uint( "act.event.ftp.port");	
	int security =  0;	
#endif 

	g_return_val_if_fail( server, 0);
	g_return_val_if_fail( user, 0);
	g_return_val_if_fail( passwd, 0);
				
	memset( svr, 0x00, sizeof(NF_NET_SEND_SERVER));

	strncpy( svr->server, server,	NF_NET_SEND_MAX_SERVER-1 );
	strncpy( svr->user,   user,		NF_NET_SEND_MAX_USER-1 );
	strncpy( svr->passwd, passwd, 	NF_NET_SEND_MAX_PASSWD-1 );

	svr->port = port;
	svr->security = security;

#ifdef DEBUG_NET_SEND_LOG
	if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_REQUEST] )
		nf_debug_hexdump(  svr, sizeof(NF_NET_SEND_SERVER));
#endif

	return 1;
				
}

static int 
_check_param_server(NF_NET_SEND_SERVER *svr)
{	
	int len = 0, user_len = 0, pass_len = 0;

#ifdef DEBUG_NET_SEND_LOG
	if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_REQUEST_RAW] )
		nf_debug_hexdump(  svr, sizeof(NF_NET_SEND_SERVER));

	if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_REQUEST] )
		_dump_svr( svr);				
#endif
	
	len = strnlen(svr->server, NF_NET_SEND_MAX_SERVER);
	g_return_val_if_fail( len >0 && len < NF_NET_SEND_MAX_SERVER , NF_NET_SEND_ERROR_SERVER);
	
	user_len =  strnlen(svr->user, NF_NET_SEND_MAX_USER);
	pass_len =  strnlen(svr->passwd, NF_NET_SEND_MAX_PASSWD);
	
	g_return_val_if_fail( user_len < NF_NET_SEND_MAX_USER, NF_NET_SEND_ERROR_USERNAME);
	g_return_val_if_fail( pass_len < NF_NET_SEND_MAX_PASSWD, NF_NET_SEND_ERROR_PASSWORD);
	
	g_return_val_if_fail( svr->port>0 && svr->port<0xffff, NF_NET_SEND_ERROR_PORT);

	
	return NF_NET_SEND_ERROR_SUCCESS;
}

static int 
_check_param_content(NF_NET_SEND_CONTENT *cont)
{

#ifdef DEBUG_NET_SEND_LOG
	if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_REQUEST_RAW] )
		nf_debug_hexdump(  cont, sizeof(NF_NET_SEND_CONTENT));
		
	if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_REQUEST] )
		_dump_cont( cont );
				
#endif
	
	g_return_val_if_fail( cont->subject[0] != NULL , NF_NET_SEND_ERROR_SUBJECT);
		
	g_return_val_if_fail( strnlen(cont->subject, NF_NET_SEND_MAX_SUBJECT) < NF_NET_SEND_MAX_SUBJECT , NF_NET_SEND_ERROR_SUBJECT);
	g_return_val_if_fail( strnlen(cont->contents, NF_NET_SEND_MAX_CONTENTS) < NF_NET_SEND_MAX_CONTENTS , NF_NET_SEND_ERROR_CONTENTS);
	g_return_val_if_fail( strnlen(cont->image_name, NF_NET_SEND_MAX_IMAGE_NAME) < NF_NET_SEND_MAX_IMAGE_NAME , NF_NET_SEND_ERROR_IMAGE_NAME);
	
	if( cont->image_size == 0 )
		g_return_val_if_fail( cont->image_data == NULL, NF_NET_SEND_ERROR_IMAGE_DATA);	
	else				
		g_return_val_if_fail( cont->image_data != NULL, NF_NET_SEND_ERROR_IMAGE_DATA);
		
	g_return_val_if_fail( cont->image_size < NF_NET_SEND_MAX_IMAGE_SIZE, NF_NET_SEND_ERROR_IMAGE_SIZE);
	
	return NF_NET_SEND_ERROR_SUCCESS;
}


static void 
_free_ninfo( NF_NET_SEND_INTERNAL *ninfo )
{
	g_return_if_fail (ninfo);
	
	if( ninfo->content.image_data) 
		g_free( ninfo->content.image_data);	
		
	g_free( ninfo);
				
}

static void 
_dump_svr( NF_NET_SEND_SERVER *server )
{
	int user_count;
	
	g_return_if_fail (server);

	g_message("type         [%d]", server->type );
	g_message("server       [%s]", server->server	);
	g_message("user         [%s]", server->user	);
	g_message("passwd       [%s]", server->passwd	);
	g_message("port         [%d] ssl[%d]", server->port, server->security );

}

static void 
_dump_cont( NF_NET_SEND_CONTENT *content )
{
	int user_count;
	
	g_return_if_fail (content);
						                            
	g_message("type         [%d]", content->type  		);
	g_message("from         [%s]", content->from		);

	g_message("subject      [%s]", content->subject		);
	g_message("image_size   [%d]", content->image_size	);
	g_message("image_name   [%s]", content->image_name	);
	g_message("cb_func      [0x%08x]", content->cb_func	);
	g_message("cb_arg       [0x%08x]", content->cb_arg	);
	g_message("is_dvr_event [%d]", content->is_dvr_event  );


	g_message("contents     [%s]", content->contents		);
	g_message("webra_link   [%s]", content->webra_link		);

}

static void 
_dump_ninfo( NF_NET_SEND_INTERNAL *ninfo )
{
	int user_count;
	
	g_return_if_fail (ninfo);
	
	_dump_svr(&ninfo->server);
	_dump_cont(&ninfo->content);
	
}

static 
NF_NET_SEND_INTERNAL *_make_ninfo( NF_NET_SEND_SERVER *svr, NF_NET_SEND_CONTENT *cont )
{
	NF_NET_SEND_INTERNAL	*ninfo = NULL;
	gpointer image_data = NULL;
	int i;
	char server[256], domain[256];
	
	g_return_val_if_fail ( svr != NULL, NULL);
	g_return_val_if_fail ( cont != NULL, NULL);
	 	
	if( cont->image_size > 0)
	{
		image_data = g_malloc( cont->image_size );
		g_return_val_if_fail (image_data, 0);
	}

	ninfo = g_malloc0( sizeof(NF_NET_SEND_INTERNAL) );
	if(!ninfo)
	{
		if(image_data) g_free(image_data);
		g_return_val_if_fail( ninfo, 0);			
	}
	
	strncpy( ninfo->server.server, svr->server, 	NF_NET_SEND_MAX_SERVER-1 );	
	strncpy( ninfo->server.user, svr->user, 		NF_NET_SEND_MAX_USER-1 );	
	strncpy( ninfo->server.passwd, svr->passwd, 	NF_NET_SEND_MAX_PASSWD-1 );
		
	ninfo->server.port = svr->port;
	ninfo->server.security = svr->security;

	strncpy( ninfo->content.from, cont->from, 	NF_NET_SEND_MAX_FROM-1 );
	strncpy( ninfo->content.subject, cont->subject, 	NF_NET_SEND_MAX_SUBJECT-1 );	
	strncpy( ninfo->content.contents, cont->contents, 	NF_NET_SEND_MAX_CONTENTS-1 );	

	strncpy( ninfo->content.webra_link, cont->webra_link, 	NF_NET_SEND_MAX_WEBRA_LINK-1 );	
		
	ninfo->content.image_size =  cont->image_size;
	strncpy( ninfo->content.image_name, cont->image_name, 	NF_NET_SEND_MAX_IMAGE_NAME-1 );		
	for(i=0; ninfo->content.image_name[i] && i< NF_NET_SEND_MAX_IMAGE_NAME ;++i) 
	{
		// remove '/'
		if( ninfo->content.image_name[i] == '/' )
			ninfo->content.image_name[i] = '_';
	}

	if( cont->image_size > 0)
	{
		memcpy(image_data, cont->image_data, cont->image_size );
		ninfo->content.image_data = image_data;
	}

	ninfo->content.video_ch = cont->video_ch;
	ninfo->content.video_start_time = cont->video_start_time;
	ninfo->content.video_end_time = cont->video_end_time;	
	strncpy( ninfo->content.video_name, cont->video_name, 	NF_NET_SEND_MAX_IMAGE_NAME-1 );	
		
	ninfo->content.cb_func = cont->cb_func;
	ninfo->content.cb_arg = cont->cb_arg;

	// FOR FTP
	strncpy( ninfo->content.ftp_dir, cont->ftp_dir, 	NF_NET_SEND_MAX_DIR-1 );	
	for(i=0; ninfo->content.ftp_dir[i] && i<NF_NET_SEND_MAX_DIR ;++i)
	{
		 // remove '/'
		if( ninfo->content.ftp_dir[i] == '/' )
			ninfo->content.ftp_dir[i] = '_';
	}			
				
	return ninfo;
}

GStaticMutex _net_send_mutex = G_STATIC_MUTEX_INIT;
 
static gint _send_request(NF_NET_SEND_SERVER *svr, NF_NET_SEND_CONTENT *cont)
{
	gint q_len = 0;
	gint i = 0, ret = 0;
	
	NF_NET_SEND_INTERNAL	*ninfo = NULL;
	gpointer image_data = NULL;
		
	g_return_val_if_fail (_nf_net_send != NULL, NF_NET_SEND_ERROR_INIT);	
	
	g_return_val_if_fail (cont, NF_NET_SEND_ERROR_FAILED);	
	g_return_val_if_fail (svr, NF_NET_SEND_ERROR_FAILED);	
	
	ret = _check_param_content(cont);
	g_return_val_if_fail ( ret == 0 , ret );
	
	ret = _check_param_server(svr);
	g_return_val_if_fail ( ret == 0 , ret);	
	
	ninfo = _make_ninfo(svr, cont);
	g_return_val_if_fail( ninfo , NF_NET_SEND_ERROR_MALLOC);

#ifdef DEBUG_NET_SEND_LOG
	if( _DEBUG_NET_SEND_log[DEBUG_NET_SEND_IDX_TEST] )
		_dump_ninfo(ninfo);
#endif
				
	q_len = g_async_queue_length(_nf_net_send->queue);
	if(q_len > NF_NET_SEND_MAX_QUEUE_LEN)
	{
		g_warning("%s nf_net_send->queue FULL[%d]",__FUNCTION__, q_len);		
		_free_ninfo(ninfo);
		return NF_NET_SEND_ERROR_MAX_QUE_LEN;
	}

#ifdef DEBUG_DIRECT_NET_SEND
	g_static_mutex_lock(&_net_send_mutex);
	_net_send_request(ninfo);
	_free_ninfo(ninfo);
	g_static_mutex_unlock(&_net_send_mutex);
#else
	g_async_queue_push( _nf_net_send->queue, ninfo);
#endif

	g_message("%s queue_push [%d]", __FUNCTION__, q_len +1);

	return NF_NET_SEND_ERROR_SUCCESS; // 0
}
 
gboolean 
nf_net_send_request(NF_NET_SEND_CONTENT *cont, GError **error)
{		
	gint err_code = NF_NET_SEND_ERROR_SUCCESS;		
	NF_NET_SEND_SERVER server;
		
	g_return_val_if_fail ( cont != NULL, NULL);
	
	if(cont->type == NF_NET_SEND_SERVER_TYPE_FTP ){
			
		if( _sysdb_get_ftp_info( &server ) != 1 )	// error
		{
			err_code = NF_NET_SEND_ERROR_SERVER_CONF;
			goto ret_error;
		}

	}else{
		g_warning("%s wrong type[%d]", __FUNCTION__, cont->type);
		err_code = NF_NET_SEND_ERROR_SERVER_CONF;
		goto ret_error;				
	}
	
	err_code = _send_request( &server, cont);
	if(err_code != NF_NET_SEND_ERROR_SUCCESS)
		goto ret_error;
	
	return 1;
	
ret_error:
			
	g_warning("%s failed ret[%d]",__FUNCTION__, err_code);	
	g_set_error (error,
					NF_NET_SEND_ERROR ,	/* error domain */
					err_code ,					/* error code */
					"send net error code[%d]",	/* error message format string */
					err_code);
										
	return 0;
}

gint
nf_net_send_get_queue_length()
{
	g_return_val_if_fail (_nf_net_send != NULL, 0);	
	
	return g_async_queue_length(_nf_net_send->queue);
}


#ifdef DEBUG_NET_SEND_JBSHELL

static NF_NET_SEND_INTERNAL _conf_ftp = {
							{   0,
/* smtp_server[256];*/		"192.168.100.71",
/* smtp_port;		*/		21,
/* smtp_security;   */		0,
/* smtp_user[256];  */		"1",
/* smtp_passwd[256];*/		"1"
						},

						{
							0,
/* from[256];	    */		"test@dvrlink.net",
/* subject[256];    */		"FTP test subject",
/* contents[4096];  */		"FTP test contents",
/*	link	*/				"[InternetShortcut]\r\nURL=http://192.168.100.73:8080/html/playback.htm?start_time=\r\n",
/* image_name[256]; */		"",
/* image_size;      */		0,
/* *image_data;     */		NULL,

/* video_name[256]  */		"",
/* video_ch			*/		0,
/* video_start_time	*/		0,
/* video_end_time	*/		0,							

/* callback func 	*/		NULL,
/* callback arg		*/		NULL,
/* is_dvr_event		*/		0,

// for ftp
							"TEST_DIR"
						}
};


static char *_send_status_string[] = {
	"SUCCESS",
	"FAILED",
	"START"
};

void _net_send_test_cb( gpointer user_data, 
							NF_NET_SEND_STATUS status, 
							const char* svr_msg)
{
	NF_NET_SEND_CONTENT *pCont = (NF_NET_SEND_CONTENT *)user_data;
	
	//g_return_if_fail(pCont);		
	g_message("%s status[%s] svr_msg[%s]", __FUNCTION__, 
			(status <= NF_NET_SEND_STATUS_START) ? _send_status_string[status] : "UNKNOWN", 
			svr_msg);	
}


static char ftp_send_help[] = "ftp_send [subject] [link]";
static int ftp_send(int argc, char **argv)
{		

	if( argc>1)
		snprintf( _conf_ftp.content.subject, 
					sizeof(_conf_ftp.content.subject), 
					argv[1] );
					
	if( argc>2)
		snprintf( _conf_ftp.content.webra_link, 
					sizeof(_conf_ftp.content.webra_link), 
					argv[2] );			

	_dump_ninfo( &_conf_ftp );	
	_net_ftp_send_process(&_conf_ftp);
	return 0;
}
__commandlist(ftp_send, "ftp_send",  ftp_send_help, ftp_send_help);


static char ftp_send_test_help[] = "ftp_send_test [cnt] [usleep]";
static int ftp_send_test(int argc, char **argv)
{	
	gint i;		
	gint cnt = 100, usleep = 1000;


	if( argc>1)
		cnt = (guint)strtol(argv[1],NULL,0);
					
	if( argc>2)
		usleep = (guint)strtol(argv[2],NULL,0);
			
	for(i=0; i<cnt; ++i) {
		
		snprintf( _conf_ftp.content.subject, 
					sizeof(_conf_ftp.content.subject), "ftp_send_test %d", i);
					
		_net_ftp_send_process(&_conf_ftp);
	}
	
	return 0;
}
__commandlist(ftp_send_test, "ftp_send_test",  ftp_send_test_help, ftp_send_test_help);


#endif
