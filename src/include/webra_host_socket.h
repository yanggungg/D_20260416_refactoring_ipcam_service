#ifndef __WEBRA_HOST_SOCKET_H__
#define __WEBRA_HOST_SOCKET_H__

//#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

#include "webra_host.h"
#include "webra_host_query.h"

////////////////////////////////////////////////////////////////////////////////
#define WEBRA_SERVER_FILE_SOCKET "/tmp/webra_server.dat"

#define WEBRA_PACKET_SIZE     1024
#define WEBRA_TOTAL_DATA_SIZE (1024 * 128)
#define WEBRA_MAX_CLIENT      8
#define WEBRA_HOST_LENGTH     64
#define WEBRA_PORT_LENGTH     4
#define WEBRA_AUTHID_LENGTH   64
#define WEBRA_AUTHPASS_LENGTH 64
#define WEBRA_SESSION_LENGTH  32

typedef struct _WEBRA_PACKET_HEADER_T {
  guint               length;
  gchar               ipver; // 4 or 6
  gchar               remote_host[WEBRA_HOST_LENGTH+1];
  gchar               remote_port[WEBRA_PORT_LENGTH+1];
  gchar               sessid[WEBRA_SESSION_LENGTH];
  gchar               userid[WEBRA_AUTHID_LENGTH + 1];
  gchar               usrkey[WEBRA_SESSION_LENGTH];
} WEBRA_PACKET_HEADER;

////////////////////////////////////////////////////////////////////////////////

gint webra_host_socket_init(void);
gint webra_host_socket_main(void);
gint webra_host_socket_close(void);

WEBRA_PACKET_HEADER*  webra_host_socket_get_header();
gchar*                webra_host_socket_get_header_remote_host();
gchar*                webra_host_socket_get_header_remote_port();
gchar*                webra_host_socket_get_header_sessid();
gchar*                webra_host_socket_get_header_userid();
gchar*                webra_hostr_socket_get_header_usrkey();
gchar*                webra_host_socket_get_header_sessid();

////////////////////////////////////////////////////////////////////////////////
#endif // __WEBRA_HOST_SOCKET_H__
