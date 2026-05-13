#ifndef __WEBRA_HOST_QUERY_H__
#define __WEBRA_HOST_QUERY_H__

#include "webra_host.h"
#include "webra_host_env.h"

////////////////////////////////////////////////////////////////////////////////

gboolean webra_host_query_init();
gboolean webra_host_query_main(GString* query_string);

////////////////////////////////////////////////////////////////////////////////

//GList*    webra_host_query_parser(GString* query_string_orig);
gboolean  webra_host_query_result(GList* result_list, GString* result_string);

////////////////////////////////////////////////////////////////////////////////
#endif // __WEBRA_HOST_QUERY_H__
