#ifndef __NF_PTZ_H__
#define __NF_PTZ_H__

#include "nf_object.h"
#include "nf_util_device.h"
#include "nf_sysman.h"
#include "board_pp.h"

#define TTYS1_DEV_ACT
#ifdef TTYS1_DEV_ACT
	#include <termios.h>
	#include <unistd.h>
	// #if defined(_IPX_0412M4) || defined(_IPX_0824M4)
	// 		#define NF_PTZ_DEV_NAME "/dev/ttyAMA0"
	// #elif(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
	// 		#define NF_PTZ_DEV_NAME "/dev/ttyAMA2"
	// #else
	// 		#define NF_PTZ_DEV_NAME "/dev/ttyAMA1"	
	// #endif
#define NF_PTZ_DEV_NAME "/dev/ttyS2"
#else
	#define NF_PTZ_DEV_NAME "/dev/fpga_rs485"
#endif

#define ENABLE_PTZ_KEYCTRL_MUTEX		// pakkhman 20121206

typedef enum _NF_PTZ_CMD_E { 
	
	NF_PTZ_CMD_PAN_LEFT,
	NF_PTZ_CMD_PAN_RIGHT,
	NF_PTZ_CMD_TILT_UP,
	NF_PTZ_CMD_TILT_DOWN,
	NF_PTZ_CMD_ZOOM_WIDE,
	NF_PTZ_CMD_ZOOM_TELE,			// 5
		
	NF_PTZ_CMD_PT_LEFTUP,
	NF_PTZ_CMD_PT_LEFTDOWN,
	NF_PTZ_CMD_PT_RIGHTUP,
	NF_PTZ_CMD_PT_RIGHTDOWN,

	NF_PTZ_CMD_IRIS_OPEN,			// 10
	NF_PTZ_CMD_IRIS_CLOSE,
	NF_PTZ_CMD_FOCUS_NEAR,
	NF_PTZ_CMD_FOCUS_FAR,

	NF_PTZ_CMD_STOP,			// 14

	NF_PTZ_CMD_SET_PRESET,			// params[0] preset #
	NF_PTZ_CMD_CLEAR_PRESET,		// params[0] preset #
	NF_PTZ_CMD_GOTO_PRESET,			// params[0] preset #

	NF_PTZ_CMD_PATTERN_START,		// params[0] from , params[1] to
	NF_PTZ_CMD_PATTERN_STOP,    
	NF_PTZ_CMD_PATTERN_SET,			// params[0] preset #    20
	
	NF_PTZ_CMD_SET_ZOOM_SPEED,		// params[0] 0~100
	NF_PTZ_CMD_SET_FOCUS_SPEED,		// params[0] 0~100
	NF_PTZ_CMD_SET_IRIS_SPEED,		// params[0] 0~100
	NF_PTZ_CMD_SET_PANTILT_SPEED,	// params[0] 0~100
	
	NF_PTZ_CMD_SET_AUTO_FOCUS,		// params[0] 0:off 1:on 2:auto
	NF_PTZ_CMD_SET_AUTO_IRIS,		// params[0] 0:off 1:on 2:auto

	NF_PTZ_CMD_OSD_UP_KEY,
	NF_PTZ_CMD_OSD_DOWN_KEY,
	NF_PTZ_CMD_OSD_LEFT_KEY,
	NF_PTZ_CMD_OSD_RIGHT_KEY,	  // 30
	NF_PTZ_CMD_OSD_ENTER_KEY,
	NF_PTZ_CMD_OSD_STOP_KEY,

	NF_PTZ_CMD_RESERVED0,
	NF_PTZ_CMD_RESERVED1,
	NF_PTZ_CMD_RESERVED2,
	NF_PTZ_CMD_RESERVED3,
	NF_PTZ_CMD_RESERVED4,
	NF_PTZ_CMD_RESERVED5,
	NF_PTZ_CMD_RESERVED6,  
	NF_PTZ_CMD_RESERVED7,	  //40

	NF_PTZ_CMD_PAN_LEFT_CON,
	NF_PTZ_CMD_PAN_RIGHT_CON,
	NF_PTZ_CMD_TILT_UP_CON,
	NF_PTZ_CMD_TILT_DOWN_CON,
	NF_PTZ_CMD_ZOOM_WIDE_CON,
	NF_PTZ_CMD_ZOOM_TELE_CON,

	NF_PTZ_CMD_PT_LEFTUP_CON,
	NF_PTZ_CMD_PT_LEFTDOWN_CON,
	NF_PTZ_CMD_PT_RIGHTUP_CON,   
	NF_PTZ_CMD_PT_RIGHTDOWN_CON,    //50

	NF_PTZ_CMD_IRIS_OPEN_CON,
	NF_PTZ_CMD_IRIS_CLOSE_CON,
	NF_PTZ_CMD_FOCUS_NEAR_CON,
	NF_PTZ_CMD_FOCUS_FAR_CON,
	NF_PTZ_CMD_QC_TEST,     // 55
	NF_PTZ_CMD_QC_TEST_STOP,
	NF_PTZ_CMD_ZOOM_STOP,

	NF_PTZ_CMD_NR,
	
} NF_PTZ_CMD_E;

/* PTZ_ATTR Camera attribute*/
typedef enum _NF_PTZ_ATTR_E { 
	NF_PTZ_ATTR_ALL			= 0xffffffff,
	NF_PTZ_ATTR_PAN			= 0x00000001,
	NF_PTZ_ATTR_TILT		= 0x00000002,
	NF_PTZ_ATTR_ZOOM		= 0x00000004,
	NF_PTZ_ATTR_PT_SPEED	= 0x00000008,
	NF_PTZ_ATTR_Z_SPEED		= 0x00000010,
	NF_PTZ_ATTR_F_SPEED		= 0x00000020,
	NF_PTZ_ATTR_I_SPEED		= 0x00000040,
	NF_PTZ_ATTR_PRESET		= 0x00000080,
	NF_PTZ_ATTR_PATTERN		= 0x00000100,
	NF_PTZ_ATTR_TOUR		= 0x00000200,
	NF_PTZ_ATTR_8DIR		= 0x00000400,
	NF_PTZ_ATTR_USERCMD		= 0x00000800,
	NF_PTZ_ATTR_SHUTTER		= 0x00001000,
	NF_PTZ_ATTR_ALARM		= 0x00002000,
	NF_PTZ_ATTR_ZONE		= 0x00004000,
	NF_PTZ_ATTR_PRE_SPEED	= 0x00008000,
	NF_PTZ_ATTR_AUTO_F		= 0x00010000,
	NF_PTZ_ATTR_AUTO_I		= 0x00020000,
	NF_PTZ_ATTR_SWG_SPEED	= 0x00040000,
	NF_PTZ_ATTR_UCMD01		= 0x00080000,
	NF_PTZ_ATTR_UCMD02		= 0x00100000	
} NF_PTZ_ATTR_E;

typedef enum _NF_PTZ_SPEED_E {
	NF_PTZ_SPEED_PAN,
	NF_PTZ_SPEED_TILT,
	NF_PTZ_SPEED_ZOOM,
	NF_PTZ_SPEED_FOCUS,
	NF_PTZ_SPEED_IRIS,
	NF_PTZ_SPEED_PRESET,	
	NF_PTZ_SPEED_NR
} NF_PTZ_SPEED_E;

typedef struct _NF_PTZ_CMD_T {
	gint 			ch;
	NF_PTZ_CMD_E	cmd;					
	gint			params[4];
	gint			reserved[2];
} NF_PTZ_CMD;

typedef struct _NF_PTZ_PROTOCOL_INFO_T {
	gint			totalcnt;
	gint			idx;
	guint			attr;	// NF_PTZ_ATTR_E	
	gchar			name[64+1];			
} NF_PTZ_PROTOCOL_INFO;

typedef struct _NF_PTZ_SYSDB_CH_T {
	guint			ch;					// key="cam.ptz.P0.channel"			type="UINT" 	
	guint			addr;               // key="cam.ptz.P0.addr"			type="UINT" 	
	guint			protocol;           // key="cam.ptz.P0.protocol"		type="UINT" 	
	guint			baud;               // key="cam.ptz.P0.baud"			type="UINT" 	
	guint 			auto_focus;         // key="cam.ptz.P0.auto_focus"		type="BOOL" 	    
	guint 			auto_iris;          // key="cam.ptz.P0.auto_iris"		type="BOOL" 	    
	guint 			zoom_spd;           // key="cam.ptz.P0.zoom_spd"		type="UINT" 	
	guint 			focus_spd;          // key="cam.ptz.P0.focus_spd"		type="UINT" 	    
	guint 			iris_spd;           // key="cam.ptz.P0.iris_spd"		type="UINT" 	
	guint 			preset_spd;         // key="cam.ptz.P0.preset_spd"		type="UINT" 	    
	guint 			pt_spd;             // key="cam.ptz.P0.pt_spd"			type="UINT" 	    
	guint 			swing_spd;          // key="cam.ptz.P0.swing_spd"		type="UINT" 	    
	gchar 			model[64+1];          // key="cam.ptz.P0.model"			type="STRING"
	guint			is_rs485;
} NF_PTZ_SYSDB_CH;

#define NF_PTZ_SWPAT_SLEEP_SEC  (60)
#define NF_PTZ_SWPAT_STEP_CNT	(16)
#define NF_PTZ_SWPAT_CMD_MASK	(0x33983246)
#define NF_PTZ_SWPAT_CNT		(2)

typedef struct _NF_PTZ_SWPAT_SYSDB_CH_T {
	
	guint	cnt;
	
	guint	preset[NF_PTZ_SWPAT_STEP_CNT];
	guint	dwell[NF_PTZ_SWPAT_STEP_CNT];
	guint	speed[NF_PTZ_SWPAT_STEP_CNT];
			
} NF_PTZ_SWPAT_SYSDB_CH;

typedef struct _NF_PTZ_SWPAT_STATUS_CH_T {

	guint	mode;
	
	guint	step;
	guint	curr_preset;
	
	guint	dwell_remain;
	guint	sleep_remain;		

	guint	user_cmd_cnt;
} NF_PTZ_SWPAT_STATUS_CH;


#if 0 
/*
<item key="cam.ptz.P0.ucmd1_num"		type="UINT" 	min="0" max="" val="" />  
	-> MODE 0: disable, Auto:1 

<item key="cam.ptz.P0.ucmd1_str"		type="STRING" 	min="0" max="32" val="" />	
	-> Preset ch   1:'A'	Disable -> 'Z', ����� 16�� ������
	-> 'ABCJZZZZZZZZZGZZZZZZZZZZZZZZZZZZ'

<item key="cam.ptz.P0.ucmd2_str"		type="STRING" 	min="0" max="32" val="" />	
	-> Dwell Time  1:'A'	MAX 20 sec, ����� 16�� ������
	-> 'ACAEACAEACAEACAEZZZZZZZZZZZZZZZZ'
*/

/*
<item key="cam.ptz.P0.scan.S0.number" type="UINT" min="0" max="255" val="0" />
<item key="cam.ptz.P0.scan.S1.number" type="UINT" min="0" max="255" val="0" />
<item key="cam.ptz.P0.scan.S0.dwell" type="UINT" min="0" max="20" val="5" />
<item key="cam.ptz.P0.scan.S1.dwell" type="UINT" min="0" max="20" val="5" />


<item key="cam.ptz.P0.tour.TCNT" type="UINT" min="0" max="16" val="16" />
<item key="cam.ptz.P0.tour.T0.number" type="UINT" min="0" max="255" val="0" />
<item key="cam.ptz.P0.tour.T0.dwell" type="UINT" min="0" max="20" val="5" />
*/
#endif


typedef struct _NF_PTZ_PROTOCOL_T {
	
	unsigned int idx;
	unsigned int attr;	/* NF_PTZ_ATTR_E */
	
	char	proto_name[64];	
					
	/* standard command set */		
	int (*func_pan_left)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pan_right)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_tilt_up)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_tilt_down)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pt_leftup)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pt_leftdown)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pt_rightup)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pt_rightdown)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_zoom_wide)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_zoom_tele)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);

	int (*func_iris_open)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_iris_close)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_focus_near)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_focus_far)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	
	int (*func_stop)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	
	/* extended command set */
	int (*func_set_preset)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_clear_preset)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_goto_preset)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);

	int (*func_pattern_start)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pattern_stop)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_pattern_set)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_run_pattern)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);

	int (*func_set_zoom_speed)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_set_focus_speed)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_set_iris_speed)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_set_pantilt_speed)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
		
	int (*func_set_auto_focus)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_set_auto_iris)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);

	int (*func_osd_up_key)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_osd_down_key)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_osd_left_key)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_osd_right_key)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_osd_enter_key)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_osd_stop_key)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);

	int (*func_reserved0)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_reserved1)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_reserved2)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_reserved3)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	
	int (*func_reserved4)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_reserved5)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_reserved6)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
	int (*func_reserved7)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);
					
} NF_PTZ_PROTOCOL;

#define MAX_PTZ_PROTOCOL	32

typedef int (*NF_PTZ_FUNC_PTR)(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff);

/* type macro */
#define NF_TYPE_PTZ					(nf_ptz_get_type ())

#define NF_IS_PTZ(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_PTZ))
#define NF_IS_PTZ_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_PTZ))

#define NF_PTZ_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_PTZ, NfPtzClass))
#define NF_PTZ(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_PTZ, NfPtz))
#define NF_PTZ_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_PTZ, NfPtzClass))

#define NF_PTZ_CAST(obj)			((NfPtz*)(obj))
#define NF_PTZ_CLASS_CAST(klass)	((NfPtzClass*)(klass))

typedef struct _NfPtz 		NfPtz;
typedef struct _NfPtzClass 	NfPtzClass;

/**
 * NfPtz:
 *
 * NfDVR ptz class
 */
struct _NfPtz {
	NfObject 	 	object;
	
	/*< public >*/
	gint			init_done;
	
	GAsyncQueue		*queue;
	GThread			*thread;
	
	gint			thread_run;
	gint			thread_status;

	gint			sysdb_reload;
	
	NF_PTZ_SYSDB_CH		sysdb_ptz[NUM_ANALOG_CHANNEL];
	NF_PTZ_PROTOCOL		*protocol[MAX_PTZ_PROTOCOL];
	guint				protocol_cnt;
	
	GThread					*sw_pat_thread;
	GMutex					*sw_pat_lock;

	gint					sw_pat_sysdb_reload;	
	
	gint					sw_pat_sysdb_changed[NUM_ANALOG_CHANNEL];
	gint					sw_pat_sysdb_mode[NUM_ANALOG_CHANNEL];
				
	NF_PTZ_SWPAT_SYSDB_CH	sw_pat_sysdb[NUM_ANALOG_CHANNEL][NF_PTZ_SWPAT_CNT];
	NF_PTZ_SWPAT_STATUS_CH	sw_pat_status[NUM_ANALOG_CHANNEL];
	
	/*< public >*/ /* with LOCK */

	/*< private >*/	
};

struct _NfPtzClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

gboolean 
nf_ptz_init(int wait);

// 2009.2.5 mybusisi -> queue push for repeate command
gboolean nf_ptz_RepeatCmd( NF_PTZ_CMD *pcmd , gint input_cmd);
gboolean nf_ptz_releaseCmd( NF_PTZ_CMD *pcmd );

/**
	@brief				ptz Ŀ�ǵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_cmd( NF_PTZ_CMD *pcmd );


typedef enum _NF_PTZ_SWPAT_MODE_E {
	NF_PTZ_SWPAT_MODE_OFF,
	NF_PTZ_SWPAT_MODE_SCAN,
	NF_PTZ_SWPAT_MODE_TOUR,
	NF_PTZ_SWPAT_MODE_NR
} NF_PTZ_SWPAT_MODE_E;


/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_start( gint ch, gint mode );

/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_stop( gint ch );

/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_state( gint ch,  NF_PTZ_SWPAT_STATUS_CH  *status);


/**
	@brief				ptz ���� preview
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_preview( gint ch, gint mode, NF_PTZ_SWPAT_SYSDB_CH  *preview_data );


/**
	@brief				ptz �����ϴ� �������� ���� ���
	@param[out]			proto_arr
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_protocol_get_info( NF_PTZ_PROTOCOL_INFO *proto_arr );


/**
	@brief				ptz �����ϴ� �������� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gint nf_ptz_protocol_get_cnt( void );

#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
	void nf_ptz_lock(void);
	void nf_ptz_unlock(void);
#endif

#endif

#define NF_PTZ_SET_RS485_RX	0
#define NF_PTZ_SET_RS485_TX	1 

void nf_ptz_set_rs485_rtsn(int is_tx);
gboolean nf_board_pp_set_rs485_rtsn(is_tx);
