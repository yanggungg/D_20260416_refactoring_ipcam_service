#ifndef __NF_KEYCTRL_H__
#define __NF_KEYCTRL_H__

#include "nf_object.h"
#include "nf_common.h"
#include "nf_notify.h"
#include "nf_sysman.h"

#define TTYS1_DEV_ACT  
#if defined(TTYS1_DEV_ACT)
#include <termios.h>
#include <unistd.h>

#define NF_KEYCTRL_DEV_NAME "/dev/ttyS1"

#else
	#define NF_KEYCTRL_DEV_NAME "/dev/fpga_rs485"
#endif

// DVR key panel keymap
typedef enum _NF_KEYCTRL_BUTTON_MAP_E { 
	
	NF_KEYCTRL_BUTTON_MAP_NUM1 = 1,
	NF_KEYCTRL_BUTTON_MAP_NUM2,
	NF_KEYCTRL_BUTTON_MAP_NUM3,
	NF_KEYCTRL_BUTTON_MAP_NUM4,
	NF_KEYCTRL_BUTTON_MAP_NUM5,
	NF_KEYCTRL_BUTTON_MAP_NUM6,
	NF_KEYCTRL_BUTTON_MAP_NUM7,
	NF_KEYCTRL_BUTTON_MAP_NUM8,
	NF_KEYCTRL_BUTTON_MAP_NUM9,
	NF_KEYCTRL_BUTTON_MAP_NUM10,
	NF_KEYCTRL_BUTTON_MAP_NUM11,
	NF_KEYCTRL_BUTTON_MAP_NUM12,
	NF_KEYCTRL_BUTTON_MAP_NUM13,
	NF_KEYCTRL_BUTTON_MAP_NUM14,
	NF_KEYCTRL_BUTTON_MAP_NUM15,
	NF_KEYCTRL_BUTTON_MAP_NUM16,

	NF_KEYCTRL_BUTTON_MAP_DISPLAY,
	NF_KEYCTRL_BUTTON_MAP_SEQ,
	NF_KEYCTRL_BUTTON_MAP_PANIC,
	NF_KEYCTRL_BUTTON_MAP_ZOOM,
	NF_KEYCTRL_BUTTON_MAP_LOCK,
	NF_KEYCTRL_BUTTON_MAP_ARCHIVE,
	NF_KEYCTRL_BUTTON_MAP_PTZ,
	NF_KEYCTRL_BUTTON_MAP_SETUP,
	NF_KEYCTRL_BUTTON_MAP_SEARCH,
	
	NF_KEYCTRL_BUTTON_MAP_LEFT,
	NF_KEYCTRL_BUTTON_MAP_RIGHT,
	NF_KEYCTRL_BUTTON_MAP_UP,
	NF_KEYCTRL_BUTTON_MAP_DOWN,

	NF_KEYCTRL_BUTTON_MAP_PAUSE,			// ��
	NF_KEYCTRL_BUTTON_MAP_RW,				// ��
	NF_KEYCTRL_BUTTON_MAP_FW, 				// ��
	NF_KEYCTRL_BUTTON_MAP_FF,				// ����
	NF_KEYCTRL_BUTTON_MAP_RF,				// ����
	
	NF_KEYCTRL_BUTTON_MAP_ENTER,
	NF_KEYCTRL_BUTTON_MAP_RETURN,
	
	NF_KEYCTRL_BUTTON_MAP_HOLD,
	NF_KEYCTRL_BUTTON_MAP_POWER,
	
	NF_KEYCTRL_BUTTON_MAP_JOYSTIC_LEFT,
	NF_KEYCTRL_BUTTON_MAP_JOYSTIC_RIGHT,
	NF_KEYCTRL_BUTTON_MAP_JOYSTIC_UP,
	NF_KEYCTRL_BUTTON_MAP_JOYSTIC_DOWN,
	
	NF_KEYCTRL_BUTTON_MAP_TURN_LEFT,
	NF_KEYCTRL_BUTTON_MAP_TURN_RIGHT,
	
	NF_KEYCTRL_BUTTON_MAP_RELAY,
	NF_KEYCTRL_BUTTON_MAP_LOGOUT,
	NF_KEYCTRL_BUTTON_MAP_STOP,
	
	NF_KEYCTRL_BUTTON_MAP_RESERVED1, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED2, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED3, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED4, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED5, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED6, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED7, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED8, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED9, 
	NF_KEYCTRL_BUTTON_MAP_RESERVED10, 
	
	NF_KEYCTRL_BUTTON_MAP_NR
	
} NF_KEYCTRL_BUTTON_MAP_E;

static const char *_NF_KEYCTRL_BUTTON_STR[] = {
	
	"NF_KEYCTRL_BUTTON_MAP_NUM1",
	"NF_KEYCTRL_BUTTON_MAP_NUM2",
	"NF_KEYCTRL_BUTTON_MAP_NUM3",
	"NF_KEYCTRL_BUTTON_MAP_NUM4",
	"NF_KEYCTRL_BUTTON_MAP_NUM5",
	"NF_KEYCTRL_BUTTON_MAP_NUM6",
	"NF_KEYCTRL_BUTTON_MAP_NUM7",
	"NF_KEYCTRL_BUTTON_MAP_NUM8",
	"NF_KEYCTRL_BUTTON_MAP_NUM9",
	"NF_KEYCTRL_BUTTON_MAP_NUM10",
	"NF_KEYCTRL_BUTTON_MAP_NUM11",
	"NF_KEYCTRL_BUTTON_MAP_NUM12",
	"NF_KEYCTRL_BUTTON_MAP_NUM13",
	"NF_KEYCTRL_BUTTON_MAP_NUM14",
	"NF_KEYCTRL_BUTTON_MAP_NUM15",
	"NF_KEYCTRL_BUTTON_MAP_NUM16",
	"NF_KEYCTRL_BUTTON_MAP_DISPLAY",
	"NF_KEYCTRL_BUTTON_MAP_SEQ",
	"NF_KEYCTRL_BUTTON_MAP_PANIC",
	"NF_KEYCTRL_BUTTON_MAP_ZOOM",
	"NF_KEYCTRL_BUTTON_MAP_LOCK",
	"NF_KEYCTRL_BUTTON_MAP_ARCHIVE",
	"NF_KEYCTRL_BUTTON_MAP_PTZ",
	"NF_KEYCTRL_BUTTON_MAP_SETUP",
	"NF_KEYCTRL_BUTTON_MAP_SEARCH",
	"NF_KEYCTRL_BUTTON_MAP_LEFT",
	"NF_KEYCTRL_BUTTON_MAP_RIGHT",
	"NF_KEYCTRL_BUTTON_MAP_UP",
	"NF_KEYCTRL_BUTTON_MAP_DOWN",
	"NF_KEYCTRL_BUTTON_MAP_PAUSE",			// ��
	"NF_KEYCTRL_BUTTON_MAP_RW",				// ��
	"NF_KEYCTRL_BUTTON_MAP_FW", 				// ��
	"NF_KEYCTRL_BUTTON_MAP_FF",				// ����
	"NF_KEYCTRL_BUTTON_MAP_RF",				// ����
	"NF_KEYCTRL_BUTTON_MAP_ENTER",
	"NF_KEYCTRL_BUTTON_MAP_RETURN",
	"NF_KEYCTRL_BUTTON_MAP_HOLD",
	"NF_KEYCTRL_BUTTON_MAP_POWER",
	"NF_KEYCTRL_BUTTON_MAP_JOYSTIC_LEFT",
	"NF_KEYCTRL_BUTTON_MAP_JOYSTIC_RIGHT",
	"NF_KEYCTRL_BUTTON_MAP_RELAY",
	"NF_KEYCTRL_BUTTON_MAP_GOTO_LIVE",
	"NF_KEYCTRL_BUTTON_MAP_RESERVED1", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED2", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED3", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED4", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED5", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED6", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED7", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED8", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED9", 
	"NF_KEYCTRL_BUTTON_MAP_RESERVED10"
	
};

// keyboard sysbd (not exist in sysdb)
typedef struct _NF_KEYCTRL_SYSDB_T {
	guint			addr;               // DVR ID 	
	guint			protocol;           // protocol number (name)
	guint			baud;               // baudrate 	
	gchar 			model[64];          // 				
#ifdef __SUPPORT_POS__	
	guint			pos_enable;		// pos enable�� ��Ÿ���̾����	
#endif	
} NF_KEYCTRL_SYSDB;

#define MAX_KEYCTRL_PROTOCOL	16

/* type macro */
#define NF_TYPE_KEYCTRL					(nf_keyctrl_get_type ())

#define NF_IS_KEYCTRL(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_KEYCTRL))
#define NF_IS_KEYCTRL_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_KEYCTRL))

#define NF_KEYCTRL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_KEYCTRL, NfKeyctrlClass))
#define NF_KEYCTRL(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_KEYCTRL, NfKeyctrl))
#define NF_KEYCTRL_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_KEYCTRL, NfKeyctrlClass))

#define NF_KEYCTRL_CAST(obj)			((NfKeyctrl*)(obj))
#define NF_KEYCTRL_CLASS_CAST(klass)	((NfKeyctrlClass*)(klass))

/*typedef enum _NF_KEYCTRL_TRANS_TYPE_E {   
	NF_KEYCTRL_TRANS_RS485 = 0,
	NF_KEYCTRL_TRANS_UDP = 1,
	NF_KEYCTRL_TRANS_TCP = 2,

} NF_KEYCTRL_TRANS_TYPE_E; */ 

typedef struct _NfKeyctrl 		NfKeyctrl;
typedef struct _NfKeyctrlClass 	NfKeyctrlClass;

typedef struct _NF_KEYCTRL_DECODE_T {  
	
	unsigned int idx;
	char	proto_name[64];	

        int (*func_receive)(const int fd);
 	 guint (*func_read_command)(const NfKeyctrl *self);
	 guint prop;
}NF_KEYCTRL_DECODE;

/*typedef struct _select_t
{
	fd_set rset;
	struct timeval tv;
	int clntCnt;
}select_t;*/

typedef guint (*NF_KEYCTRL_DECODE_PTR)(NfKeyctrl *self);
typedef int (*NF_KEYCTRL_RECEIVE_PTR)(const int fd);
typedef void (*NF_KEYCTRL_HANDOFF_FUNC) ( gpointer handoff_arg, guint key_data ); 

/**
 * Nfkeycont:
 *
 * NfDVR keycontroller class
 */
struct _NfKeyctrl {
	NfObject 	 	object;
	
	/*< public >*/	
	gint			init_done;

	GThread			*thread;
	
	gint			thread_run;
	gint			thread_status;

	gint			sysdb_reload;
	
	NF_KEYCTRL_SYSDB	sysdb_keyctrl;
	NF_KEYCTRL_DECODE   *protocol_decode[MAX_KEYCTRL_PROTOCOL];
	
	guint				protocol_cnt;	

//	gboolean			isplayback;
	
	NF_KEYCTRL_HANDOFF_FUNC handoff_func;
	gpointer		handoff_arg;
			
	/*< public >*/ /* with LOCK */

	/*< private >*/	
};

struct _NfKeyctrlClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};


gboolean 
nf_keyctrl_init( int wait );

gboolean
nf_keyctrl_register_handoff(NF_KEYCTRL_HANDOFF_FUNC handoff_func, gpointer handoff_arg );

void termios_act_init(int fd, int baudrate, int databit, int parity, int stopbit);

// written by SKSHIN
gint nf_keyctrl_protocol_get_cnt( void );

enum {
	KEYCTRL_PACKET_ERR = -1,
	KEYCTRL_PACKET_PILE = 0,
	KEYCTRL_PACKET_EMPTY = 1
};

// guint nf_keyctrl_qc_test_check(void); 
void nf_keyctrl_qc_test_set(guint value);
#endif

