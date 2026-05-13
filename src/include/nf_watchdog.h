#ifndef __NF_WATCHDOG_H__
#define __NF_WATCHDOG_H__

#include "nf_object.h"

/******************************************************************************/
#define NF_WATCHDOG_KILL_SECOND			60000//6000->60000
#define NF_WATCHDOG_TIME_SEC			150//15->150	
#define NF_WATCHDOG_TIME_SST_SEC 		3000//300->3000
#define NF_WATCHDOG_TIME_SST_PRE_ALARM_SEC 		2500//250->2500

typedef enum _NF_WATCHDOG_MEMBER_E {
	NF_WATCHDOG_ENABLE				= 1,
	NF_WATCHDOG_DISABLE				= 0,
	
	NF_WATCHDOG_MEMBER_RECORD		= 0,
	NF_WATCHDOG_MEMBER_RECORD_AUDIO	= 1,
	NF_WATCHDOG_MEMBER_SST			= 2,
	NF_WATCHDOG_MEMBER_DSP0			= 3,	
	
	NF_WATCHDOG_MEMBER_DSP1			= 4,
	NF_WATCHDOG_MEMBER_GUI			= 5,
	NF_WATCHDOG_MEMBER_MAIN_TIMER	= 6,
	NF_WATCHDOG_MEMBER_NETSVR		= 7,
	
	NF_WATCHDOG_MEMBER_EMAIL_SEND	= 8,
	NF_WATCHDOG_MEMBER_ACTION		= 9,
	NF_WATCHDOG_MEMBER_EVENT		= 10,
	NF_WATCHDOG_MEMBER_PTZ			= 11,
	
	NF_WATCHDOG_MEMBER_KEYCTRL		= 12,
	NF_WATCHDOG_MEMBER_NOTIFY		= 13,		
	NF_WATCHDOG_MEMBER_ENCODE0		= 14,
	NF_WATCHDOG_MEMBER_ENCODE1		= 15,

	NF_WATCHDOG_MEMBER_ENCODEZ		= 16,
	NF_WATCHDOG_MEMBER_M3_VPSS		= 17,
	NF_WATCHDOG_MEMBER_M3_CODEC		= 18,
	
	NF_WATCHDOG_MEMBER_NR			= 19
				
} NF_WATCHDOG_MEMBER_E;

/* type macro */
#define NF_TYPE_WATCHDOG				(nf_watchdog_get_type ())

#define NF_IS_WATCHDOG(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_WATCHDOG))
#define NF_IS_WATCHDOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_WATCHDOG))

#define NF_WATCHDOG_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_WATCHDOG, NfWatchdogClass))
#define NF_WATCHDOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_WATCHDOG, NfWatchdog))
#define NF_WATCHDOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_WATCHDOG, NfWatchdogClass))

#define NF_WATCHDOG_CAST(obj)			((NfWatchdog*)(obj))
#define NF_WATCHDOG_CLASS_CAST(klass)	((NfWatchdogClass*)(klass))

typedef struct _NfWatchdog 		NfWatchdog;
typedef struct _NfWatchdogClass NfWatchdogClass;

/**
 * NfWatchdog:
 *
 * NfDVR notify class
 */
struct _NfWatchdog {
	NfObject 	 	object;

	/*< public >*/
	gint			init_done;

	GAsyncQueue		*queue;			

	GThread			*thread;	
	gint			thread_run;
	gint			thread_status;

	/*< public >*/ /* with LOCK */

	gboolean		is_running;

	guint 			enable_cp[NF_WATCHDOG_MEMBER_NR];
	guint 			max_cp[NF_WATCHDOG_MEMBER_NR];
	guint 			cur_cp[NF_WATCHDOG_MEMBER_NR];	
	GTimeVal		last_kick_tv[NF_WATCHDOG_MEMBER_NR];
			
	/*< private >*/	
};

struct _NfWatchdogClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

gboolean nf_watchdog_init(int wait);

gboolean nf_watchdog_start_run(void);

gboolean nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_E module, guint interval_sec, gboolean enable);
gboolean nf_watchdog_kick( NF_WATCHDOG_MEMBER_E module );
gboolean nf_watchdog_dump(void);
		
#endif

