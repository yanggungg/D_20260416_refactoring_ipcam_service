#ifndef __WEBRA_HOST_LOG_H__
#define __WEBRA_HOST_LOG_H__


#if 1
// fake declares

#define ZLOG_LEVEL_TRACE 1

#define __FUNC_IN__
#define __FUNC_OUT__
#define __MEMORY__
#define __DEBUG__ zlog_debug(webra_log_get_host_zc(),"[DEBUG] %s in %s:%d", \
                          __func__, __FILE__, __LINE__);

#define zlog_trace(cat, format, ...) \
  zlog(cat, __FILE__, sizeof(__FILE__)-1, \
      __func__, sizeof(__func__)-1, __LINE__, \
      ZLOG_LEVEL_TRACE, format, ## __VA_ARGS__)

#define webra_time_start();
#define webra_time_print(A);

static void webra_log(const gchar *msg, ...);

void websvr_get_localtime(const time_t *ttime, const gint dst, struct tm *stTime);
gboolean _web_datetime_is_dst( time_t tick);

int webra_log_get_host_zc();
void zlog_info();
void zlog();
void zlog_error();
void webra_log_init();
void dzlog();
void zlog_debug();
void zlog_notice();
void dzlog_notice();
void webra_conf_reload();
#else
#include "zlog.h"

//#define __DEBUG_TRACE_WEBRA__
//

extern zlog_category_t  *g_webra_log;
#define ZLOG_CATEGORY   "WEBRA"

#ifdef __DEBUG_TRACE_WEBRA__
#define __FUNC_IN__ zlog_debug(webra_log_get_host_zc(),"[IN] %s in %s:%d", \
                          __func__, __FILE__, __LINE__);

#define __FUNC_OUT__ zlog_debug(webra_log_get_host_zc(),"[OUT] %s in %s:%d", \
                          __func__, __FILE__, __LINE__);


#define __MEMORY__ zlog_debug(webra_log_get_host_zc(),"[MEMORY]  %s in %s:%d", \
                          __func__, __FILE__, __LINE__); \
                    webra_print_memory();
#else
#define __FUNC_IN__
#define __FUNC_OUT__
#define __MEMORY__
#define __DEBUG__ zlog_debug(webra_log_get_host_zc(),"[DEBUG] %s in %s:%d", \
                          __func__, __FILE__, __LINE__);
#endif

#ifdef __WEBRA_TIME_LOG__
void      webra_time_start();
void      webra_time_print(const gchar* tag);
#else
#define webra_time_start();
#define webra_time_print(A);
#endif
void      webra_print_memory();

zlog_category_t* webra_log_get_host_zc();
gboolean webra_log_category_level_enabled(zlog_category_t *zc, int level);

enum {
  ZLOG_LEVEL_TRACE = 10,
  /* must equals conf file setting */
};

#define zlog_trace(cat, format, ...) \
  zlog(cat, __FILE__, sizeof(__FILE__)-1, \
      __func__, sizeof(__func__)-1, __LINE__, \
      ZLOG_LEVEL_TRACE, format, ## __VA_ARGS__)
#endif
#endif //// __WEBRA_HOST_LOG_H__

