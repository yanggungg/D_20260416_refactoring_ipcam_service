#ifndef __NF_DEBUG_H__
#define __NF_DEBUG_H__

#include <glib.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <execinfo.h>

#define NF_DEBUG_IDENTITY        		0x49545800
#define NF_DEBUG_MAX_PACKET_DATA		(1024*8)
#define NF_DEBUG_MAX_TAG_DATA			(256)
#define NF_DEBUG_SERVER_IP				"192.168.100.10"
#define NF_DEBUG_SERVER_PORT			4000

typedef enum _NF_DEBUG_PACKET_TYPE_E
{
	NF_DEBUG_PACKET_TYPE_CMEM 		= 0,		
	NF_DEBUG_PACKET_TYPE_IFS 		= 1,
	NF_DEBUG_PACKET_TYPE_MEMINFO 	= 2,
	NF_DEBUG_PACKET_TYPE_LOGMSG 	= 3,
	
	NF_DEBUG_PACKET_TYPE_MSG_ERROR    = 0x10,
	NF_DEBUG_PACKET_TYPE_MSG_CRITICAL = 0x11,
	NF_DEBUG_PACKET_TYPE_MSG_WARNING  = 0x12,
	NF_DEBUG_PACKET_TYPE_MSG_MESSAGE  = 0x13,
	NF_DEBUG_PACKET_TYPE_MSG_INFO     = 0x14,
	NF_DEBUG_PACKET_TYPE_MSG_DEBUG    = 0x15,
		
} NF_DEBUG_PACKET_TYPE_E;

#if 0
typedef enum{  
	/* log flags */ 
	G_LOG_FLAG_RECURSION	= 1 << 0,
	G_LOG_FLAG_FATAL		= 1 << 1,

	/* GLib log levels */ 
   	G_LOG_LEVEL_ERROR		= 1 << 2,
	
	/* always fatal */ 
	G_LOG_LEVEL_CRITICAL	= 1 << 3,
	G_LOG_LEVEL_WARNING		= 1 << 4,
	G_LOG_LEVEL_MESSAGE		= 1 << 5,
	G_LOG_LEVEL_INFO		= 1 << 6,
	G_LOG_LEVEL_DEBUG		= 1 << 7,
	G_LOG_LEVEL_MASK		= ~(G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL)
} GLogLevelFlags;
#endif

typedef struct _NF_DEBUG_PACKET_T
{
	guint identity;
	guint size;
	guint type;
	guint ipaddr;
	
	GTimeVal time_val;
	gchar tag[NF_DEBUG_MAX_TAG_DATA];
	gchar data[NF_DEBUG_MAX_PACKET_DATA];
} NF_DEBUG_PACKET;

#define NF_DEBUG_PACKET_PRE_SIZE (sizeof( NF_DEBUG_PACKET) - NF_DEBUG_MAX_PACKET_DATA)

void nf_debug_test(void);

gboolean nf_debug_net_send_file(gint type, const gchar *filename, const gchar *tag);
gboolean nf_debug_net_send_msg(gint type, const gchar *msg, const gchar *tag);
gboolean nf_debug_mem_chkpoint(const gchar *tag);

gboolean nf_debug_category_add(const gchar *cate, 
							const gchar *log_str,  guint *log_arr, 
							guint log_max_arr_idx);							

gboolean nf_debug_dump(const gchar *cate);
gboolean nf_debug_set(const gchar *cate, guint idx, guint value );

gboolean nf_debug_hexdump(gpointer data, int len);
void nf_debug_backtrace (void);

#if defined (_IPX_0412P4) || defined(_IPX_0824P4) || defined(_IPX_1648P4) \
 || defined (_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
 || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)
//IPXP4 HWT??
#else
void nf_debug_backtrace_signal(int sig, struct siginfo *info, struct ucontext *ctx);
#endif


#endif

