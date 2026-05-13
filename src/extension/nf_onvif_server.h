#ifndef __NF_ONVIF_SERVER_H__
#define __NF_ONVIF_SERVER_H__

#include "nf_object.h"
#include <sys/time.h>
#include <unistd.h>
#include "nf_notify.h"
#ifdef USE_DVR
#include "nf_logevtdef.h"
#include "nf_api_eventlog.h"
#endif//USE_DVR

#include "feature_def.h"
#include "onvif_common.h"

#define ONVIF_DATA_BUFF		(1024*512)
#define OV_MAX_CLNT_CNT  			1000
#define OV_HEADER_STRLEN			32	
#define OV_MAX_QUEUE_SIZE 			(ONVIF_DATA_BUFF * 4)

#define OV_MAX_LOG_CNT 	128
#define	OV_MAX_LOG_DATA	(sizeof(NF_LOG_DATA_OLD)*OV_MAX_LOG_CNT)
#define OV_SIZE_LOG_BUFF	(sizeof(int)+sizeof(NF_LOG_RESULT_HEADER)+OV_MAX_LOG_DATA)
// khj log define
#define OV_MAX_LIVE_LOG	(64)

typedef enum _ONVIF_LOG_E {
	ONVIF_LOG_SETUP	= 1 << 0,
	ONVIF_LOG_MOTION	= 1 << 1,
	ONVIF_LOG_SMART	= 1 << 2,
	ONVIF_LOG_SYSTEM	= 1 << 3,
	ONVIF_LOG_ALARM	= 1 << 4,
	ONVIF_LOG_VLOSS	= 1 << 5,
	ONVIF_LOG_RECORD	= 1 << 6
}ONVIF_LOG_E;


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////STRUCTURE//////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* Packet Structure*/
////////////////////////////////////////////////////////////////////////////////
typedef struct _ONVIF_HEADER_T {
	guchar							type;
	guchar							code;
	guint							dlen;
	gchar							id[OV_HEADER_STRLEN];
	gchar							pwd[OV_HEADER_STRLEN];
	guchar							ipaddr[OV_HEADER_STRLEN];
	gchar							ipver;
	gchar							pw_expired;
} ONVIF_HEADER;

typedef struct _ONVIF_PACKET_T {
	ONVIF_HEADER 		header;
	gchar 				data[ONVIF_DATA_BUFF];
} ONVIF_PACKET;

typedef struct _ONVIF_QUEUE_T
{
	char queue[OV_MAX_QUEUE_SIZE];	// Queue : 64K * 4
	gint head;
	gint tail;
	gint wpos;
	gint destpos;
}ONVIF_QUEUE;

typedef struct _ONVIF_SELECT_T {
	fd_set 				rset;
	struct 	timeval 	tv;
	gint 				clntCnt;
} ONVIF_SELECT;

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////OBJECT/////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/* type macro */
#define NF_TYPE_ONVIF						(nf_onvif_get_type ())

#define NF_IS_ONVIF(obj)					(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_ONVIF))
#define NF_IS_ONVIF_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_ONVIF))

#define NF_ONVIF_GET_CLASS(obj)			(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_ONVIF, NfOnvifClass))
#define NF_ONVIF(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_ONVIF, NfOnvif))
#define NF_ONVIF_CLASS(klass)				(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_ONVIF, NfOnvifClass))

#define NF_ONVIF_CAST(obj)				((NfOnvif*)(obj))
#define NF_ONVIF_CLASS_CAST(klass)		((NfOnvifClass*)(klass))

typedef struct _NfOnvif 		NfOnvif;
typedef struct _NfOnvifClass 	NfOnvifClass;

struct _NfOnvif {
	NfObject 	 		object;
	
	/*< public >*/	
	gint				init_done;
	GThread *			thread;
	gint				thread_run;
	gint				thread_status;
	gint 				serv_sock;
	ONVIF_SELECT 		select_val;
	gint                sysdb_change;
	//gint              cur_clnt_sock;
	ONVIF_QUEUE *		wqueue[OV_MAX_CLNT_CNT];

	GAsyncQueue			*live_log_queue;

	gint                web_port;
	gint                web_port_change;

	/*< public >*/ /* with LOCK */
	/*< private >*/	
};

struct _NfOnvifClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////function///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//Web FCGI 와 DVR과의 UDS통신을 위한 초기화 
gboolean 	nf_onvif_init( int wait );
gint 		nf_onvif_packet_code( NfOnvif *self, int fd ,char *buff_rcv);
gint 		nf_onvif_send(gint fd, ONVIF_PACKET *packet, guint packet_len, gint type, gint code, guint dlen);
gint		nf_onvif_send_fault(gint fd, gint code, char *buff_rcv);

#endif

