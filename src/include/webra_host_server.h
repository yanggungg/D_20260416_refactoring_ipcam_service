#ifndef __WEBRA_HOST_SERVER_H__
#define __WEBRA_HOST_SERVER_H__

#include "webra_host.h"

gint webra_host_server_start(void);
gint webra_host_server_stop(void);
gint webra_host_server_restart(void);

int get_server_started();
gint webra_host_server_restart(void);
gint webra_host_server_htpasswd_generator(void);
#endif // __WEBRA_HOST_SERVER_H__