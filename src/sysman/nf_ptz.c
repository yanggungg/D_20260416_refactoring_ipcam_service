#include "nf_common.h"
#include "nf_ptz.h"
#include <sys/ioctl.h>
#include "nf_api_ipcam.h"
#include "nf_util_device.h"
#include "nf_sysman.h"
#include "nf_qc.h"

#if !defined(TTYS1_DEV_ACT)
	#include "fpga_rs485_ioctl.h"
#endif

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

//#define DEBUG_PTZ_CMD
#define DEBUG_JBSHELL_PTZ

#ifdef DEBUG_JBSHELL_PTZ
#include "jbshell.h"
#endif


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

extern NF_PTZ_PROTOCOL	_nf_ptz_proto_pelcoD;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_pelcoP;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_Mesa_Pelco;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_D_Max;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_fastraxII_2;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_Ganz_PT_V3_2;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Scc641;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Scc643a;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Spd3300;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Multix;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Mrx1000;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_WVcs850;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_WVcsr604;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Ptc400c;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Ptc200;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Takex;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_LPT_A100L;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_interM;
extern NF_PTZ_PROTOCOL	_nf_ptz_proto_Vitek;
extern NF_PTZ_PROTOCOL  _nf_ptz_proto_Lilin;
extern qcmode_enable; 
static void ptr_func_write(int fd, const unsigned char *data, unsigned int ptz_cmd_length);
static void general_write(int fd, const unsigned char *data, unsigned int ptz_cmd_length);
extern void takex_write(int fd, const unsigned char *data, unsigned int ptz_cmd_length);



static void nf_ptz_class_init (NfPtzClass * klass);
static void nf_ptz_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_ptz_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_ptz_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_ptz_dispose (GObject * object);
static void nf_ptz_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfPtz	*_nf_ptz = NULL;

static void
ptz_thread_func (NfPtz * test) ;
static void
ptz_sw_pat_thread_func (NfPtz *self);
static int count_temp = 0;	

#ifdef TTYS1_DEV_ACT
static void 			nf_termios_act_init(int fd, int baudrate, int databit, int parity, int stopbit);
#endif
static void 			nf_ptz_send_command(unsigned char *data, guint ptz_cmd_length, NF_PTZ_SYSDB_CH	*sysdb_ch, const char *ptz_name);

static const char *_NF_PTZ_CMD_STR[] = {
/* 00 */	"NF_PTZ_CMD_PAN_LEFT",
/* 01 */	"NF_PTZ_CMD_PAN_RIGHT",
/* 02 */	"NF_PTZ_CMD_TILT_UP",
/* 03 */	"NF_PTZ_CMD_TILT_DOWN",
/* 04 */	"NF_PTZ_CMD_ZOOM_WIDE",

/* 05 */	"NF_PTZ_CMD_ZOOM_TELE",
/* 06 */	"NF_PTZ_CMD_PT_LEFTUP",
/* 07 */	"NF_PTZ_CMD_PT_LEFTDOWN",
/* 08 */	"NF_PTZ_CMD_PT_RIGHTUP",
/* 09 */	"NF_PTZ_CMD_PT_RIGHTDOWN",

/* 10 */	"NF_PTZ_CMD_IRIS_OPEN",
/* 11 */	"NF_PTZ_CMD_IRIS_CLOSE",
/* 12 */	"NF_PTZ_CMD_FOCUS_NEAR",
/* 13 */	"NF_PTZ_CMD_FOCUS_FAR",
/* 14 */	"NF_PTZ_CMD_STOP",

/* 15 */	"NF_PTZ_CMD_SET_PRESET",
/* 16 */	"NF_PTZ_CMD_CLEAR_PRESET",
/* 17 */	"NF_PTZ_CMD_GOTO_PRESET",
/* 18 */	"NF_PTZ_CMD_PATTERN_START",
/* 19 */	"NF_PTZ_CMD_PATTERN_STOP",

/* 20 */	"NF_PTZ_CMD_PATTERN_SET",
/* 21 */	"NF_PTZ_CMD_SET_ZOOM_SPEED",
/* 22 */	"NF_PTZ_CMD_SET_FOCUS_SPEED",
/* 23 */	"NF_PTZ_CMD_SET_IRIS_SPEED",
/* 24 */	"NF_PTZ_CMD_SET_PANTILT_SPEED",

/* 25 */	"NF_PTZ_CMD_SET_AUTO_FOCUS",
/* 26 */	"NF_PTZ_CMD_SET_AUTO_IRIS",
/* 27 */	"NF_PTZ_CMD_RESERVED0",
/* 28 */	"NF_PTZ_CMD_RESERVED1",
/* 29 */	"NF_PTZ_CMD_RESERVED2",

/* 30 */	"NF_PTZ_CMD_OSD_UP_KEY",
/* 31 */	"NF_PTZ_CMD_OSD_DOWN_KEY",
/* 32 */	"NF_PTZ_CMD_OSD_LEFT_KEY",
/* 33 */	"NF_PTZ_CMD_OSD_RIGHT_KEY",
/* 34 */	"NF_PTZ_CMD_OSD_ENTER_KEY",
/* 35 */	"NF_PTZ_CMD_OSD_STOP_KEY",

/* 36 */	"NF_PTZ_CMD_RESERVED3",
/* 37 */	"NF_PTZ_CMD_RESERVED4",
/* 38 */	"NF_PTZ_CMD_RESERVED5",
/* 39 */	"NF_PTZ_CMD_RESERVED6",
/* 40 */	"NF_PTZ_CMD_RESERVED7",

	"NF_PTZ_CMD_PAN_LEFT_CON",
	"NF_PTZ_CMD_PAN_RIGHT_CON",
	"NF_PTZ_CMD_TILT_UP_CON",
	"NF_PTZ_CMD_TILT_DOWN_CON",
	"NF_PTZ_CMD_ZOOM_WIDE_CON",
	"NF_PTZ_CMD_ZOOM_TELE_CON",

	"NF_PTZ_CMD_PT_LEFTUP_CON",
	"NF_PTZ_CMD_PT_LEFTDOWN_CON",
	"NF_PTZ_CMD_PT_RIGHTUP_CON",
	"NF_PTZ_CMD_PT_RIGHTDOWN_CON",

	"NF_PTZ_CMD_IRIS_OPEN_CON",
	"NF_PTZ_CMD_IRIS_CLOSE_CON",
	"NF_PTZ_CMD_FOCUS_NEAR_CON",
	"NF_PTZ_CMD_FOCUS_FAR_CON",

/* 41 */	"NF_PTZ_CMD_NR"
};

GType
nf_ptz_get_type (void)
{
	static GType nf_ptz_type = 0;

	if (G_UNLIKELY (nf_ptz_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfPtzClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_ptz_class_init,
			NULL,
			NULL,
			sizeof (NfPtz),
			0,
			(GInstanceInitFunc) nf_ptz_instance_init,
			NULL
		};

		nf_ptz_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfPtz", &object_info, 0);
	}
	
	return nf_ptz_type;
}

static void
nf_ptz_class_init (NfPtzClass * klass)
{	
	GObjectClass *gobject_class;
	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_ptz_set_property;
	gobject_class->get_property = nf_ptz_get_property;
			
	gobject_class->dispose = nf_ptz_dispose;
	gobject_class->finalize = nf_ptz_finalize;

}

static const char *_conv_cmd_str(gint cmd_idx)
{
	g_return_val_if_fail ( cmd_idx >= 0 , "NULL");		
	g_return_val_if_fail ( cmd_idx < NF_PTZ_CMD_NR , "NULL");
		
	return _NF_PTZ_CMD_STR[cmd_idx];
}

inline static uint
_sysdb_get_uint( gint ch, gchar *item)
{
	gchar buff[128];

	g_return_val_if_fail(ch >=0 && ch <NUM_ANALOG_CHANNEL, FALSE);
	g_return_val_if_fail(item != NULL, FALSE);
	
	snprintf(buff, sizeof(buff), "cam.ptz.P%d.%s",  ch, item);
			
	return nf_sysdb_get_uint( buff );
}

inline static uint
_sysdb_get_bool( gint ch, gchar *item)
{
	gchar buff[128];

	g_return_val_if_fail(ch >=0 && ch <NUM_ANALOG_CHANNEL, FALSE);
	g_return_val_if_fail(item != NULL, FALSE);
	
	snprintf(buff, sizeof(buff), "cam.ptz.P%d.%s",  ch, item);
			
	return nf_sysdb_get_bool( buff );
}

inline static char *
_sysdb_get_str_nocpy( gint ch, gchar *item)
{
	gchar buff[128];

	g_return_val_if_fail(ch >=0 && ch <NUM_ANALOG_CHANNEL, FALSE);
	g_return_val_if_fail(item != NULL, FALSE);
	
	snprintf(buff, sizeof(buff), "cam.ptz.P%d.%s",  ch, item);
			
	return nf_sysdb_get_str_nocopy( buff );
}

inline static gboolean
_sysdb_set_str( gint ch, gchar *item, gchar *value)
{
	gchar buff[128];

	g_return_val_if_fail(ch >=0 && ch <NUM_ANALOG_CHANNEL, FALSE);
	g_return_val_if_fail(item != NULL, FALSE);
	
	snprintf(buff, sizeof(buff), "cam.ptz.P%d.%s",  ch, item);
			
	return nf_sysdb_set_str( buff, value );
}


static void
_get_ptz_data_from_sysdb(NfPtz *self)
{	
	gint i;	

	g_return_if_fail(self != NULL);
	
	g_message("%s",__FUNCTION__);
	
	for(i=0; i<NUM_ANALOG_CHANNEL; i++)
	{		
		self->sysdb_ptz[i].ch         = i;
		self->sysdb_ptz[i].addr       = _sysdb_get_uint( i, "addr" );
		self->sysdb_ptz[i].protocol   = _sysdb_get_uint( i, "protocol" );
		self->sysdb_ptz[i].baud       = _sysdb_get_uint( i, "baud" );
		self->sysdb_ptz[i].auto_focus = _sysdb_get_bool( i, "auto_focus" );
		self->sysdb_ptz[i].auto_iris  = _sysdb_get_bool( i, "auto_iris" );
		self->sysdb_ptz[i].zoom_spd   = _sysdb_get_uint( i, "zoom_spd" );
		self->sysdb_ptz[i].focus_spd  = _sysdb_get_uint( i, "focus_spd" );
		self->sysdb_ptz[i].iris_spd   = _sysdb_get_uint( i, "iris_spd" );
		self->sysdb_ptz[i].preset_spd = _sysdb_get_uint( i, "preset_spd" );
		self->sysdb_ptz[i].pt_spd     = _sysdb_get_uint( i, "pt_spd" );
		self->sysdb_ptz[i].swing_spd  = _sysdb_get_uint( i, "swing_spd" );
		self->sysdb_ptz[i].is_rs485   = _sysdb_get_bool( i, "rs485" );		
	}
}

static void
_get_ptz_data_from_sysdb_sw_pat(NfPtz *self)
{	
	gint i,j,k;	

	g_return_if_fail(self != NULL);
	
	g_message("%s",__FUNCTION__);
	
	memset(self->sw_pat_sysdb, 0x00, 
					sizeof(NF_PTZ_SWPAT_SYSDB_CH) * NUM_ANALOG_CHANNEL * NF_PTZ_SWPAT_CNT );
					
	for(i=0; i< NUM_ANALOG_CHANNEL; ++i)
	{				
		char buff[128];
		
		NF_PTZ_SWPAT_SYSDB_CH  *pscan = &self->sw_pat_sysdb[i][0];
		NF_PTZ_SWPAT_SYSDB_CH  *ptour = &self->sw_pat_sysdb[i][1];

		snprintf( buff, sizeof(buff), "cam.ptz.P%d.mode", i);				
 		self->sw_pat_sysdb_mode[i] = nf_sysdb_get_uint(buff); // 1 scan 2:tour
				
		pscan->cnt = 2;		// the SCAN cat was fixed at 2;
		for(j=0;j<pscan->cnt;++j) 
		{
			snprintf( buff, sizeof(buff), "cam.ptz.P%d.scan.S%d.number", i, j);
			pscan->preset[j] = nf_sysdb_get_uint(buff);
			
			snprintf( buff, sizeof(buff), "cam.ptz.P%d.scan.S%d.dwell", i, j);
			pscan->dwell[j] = nf_sysdb_get_uint(buff);							
		}

		snprintf( buff, sizeof(buff), "cam.ptz.P%d.tour.TCNT", i);
		ptour->cnt = nf_sysdb_get_uint(buff);
		
		for(j=0;j<NF_PTZ_SWPAT_STEP_CNT;++j)
		{
			snprintf( buff, sizeof(buff), "cam.ptz.P%d.tour.T%d.number", i, j);
			ptour->preset[j] = nf_sysdb_get_uint(buff);
			
			snprintf( buff, sizeof(buff), "cam.ptz.P%d.tour.T%d.dwell", i, j);
			ptour->dwell[j] = nf_sysdb_get_uint(buff);							
		}

	}
}

static void _add_ptz_protocol(NfPtz *self, NF_PTZ_PROTOCOL* protocol)
{
	gchar buff[128];
	gint idx = 0;
	
	g_return_if_fail( self != NULL);
	g_return_if_fail( self->protocol_cnt < MAX_PTZ_PROTOCOL );	
	g_return_if_fail( protocol != NULL);
	
	idx = self->protocol_cnt;
	
	self->protocol[idx] = protocol;	
	self->protocol[idx]->idx = idx;	
	
	snprintf(buff, sizeof(buff), "sys.info.ptz.P%d.name", idx);	
	nf_sysdb_set_str(buff,  protocol->proto_name);	
	nf_sysdb_set_uint("sys.info.ptz.PCNT",  idx+1 );		
	
	g_message("%-16.16s ptz_protocol init[%d]" , protocol->proto_name, idx);
	++self->protocol_cnt;		
}

static void
nf_ptz_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfPtz *self = NF_PTZ (instance);	

	self->init_done = 0;
	self->protocol_cnt = 0;
			
	_get_ptz_data_from_sysdb(self);
	_get_ptz_data_from_sysdb_sw_pat(self);	
			
	// ���� ���� �ϸ� �ȵ�!  choissi 2009-06-13 ���� 12:57:26
	_add_ptz_protocol(self, &_nf_ptz_proto_pelcoD);            // 0 
	_add_ptz_protocol(self, &_nf_ptz_proto_pelcoP);            // 1 
	_add_ptz_protocol(self, &_nf_ptz_proto_Mesa_Pelco);        // 2 
	_add_ptz_protocol(self, &_nf_ptz_proto_D_Max);             // 3 
	_add_ptz_protocol(self, &_nf_ptz_proto_fastraxII_2);       // 4 
	_add_ptz_protocol(self, &_nf_ptz_proto_Ganz_PT_V3_2);      // 5 
	_add_ptz_protocol(self, &_nf_ptz_proto_Scc641);            // 6 
	_add_ptz_protocol(self, &_nf_ptz_proto_Scc643a);           // 7 
	_add_ptz_protocol(self, &_nf_ptz_proto_Spd3300);           // 8
	_add_ptz_protocol(self, &_nf_ptz_proto_Multix);            // 9
	_add_ptz_protocol(self, &_nf_ptz_proto_Mrx1000);           // 10
	_add_ptz_protocol(self, &_nf_ptz_proto_WVcs850);           // 11
	_add_ptz_protocol(self, &_nf_ptz_proto_WVcsr604);          // 12
	_add_ptz_protocol(self, &_nf_ptz_proto_Ptc400c);           // 13
	_add_ptz_protocol(self, &_nf_ptz_proto_Ptc200);            // 14
	_add_ptz_protocol(self, &_nf_ptz_proto_Takex);             // 15
	_add_ptz_protocol(self, &_nf_ptz_proto_LPT_A100L);         // 16
	_add_ptz_protocol(self, &_nf_ptz_proto_interM);            // 17
	_add_ptz_protocol(self, &_nf_ptz_proto_Vitek);             // 18
	_add_ptz_protocol(self, &_nf_ptz_proto_Lilin);             // 19

	g_message("%s proto_cnt[%d]", __FUNCTION__, self->protocol_cnt);
			
	// queue ����
	self->queue = g_async_queue_new();
 		 
	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)ptz_thread_func, 
									self, FALSE, NULL);		
									

	self->sw_pat_lock = g_mutex_new();
	self->sw_pat_thread = g_thread_create(	(GThreadFunc)ptz_sw_pat_thread_func, 
									self, FALSE, NULL);		
									
									
																																									
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_ptz_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_ptz_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_ptz_set_property (GObject * object, guint prop_id,
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
nf_ptz_get_property (GObject * object, guint prop_id,
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

static gboolean 
_ptz_cmd_process( NfPtz *self, NF_PTZ_CMD *cmd)
{
	unsigned char		ptz_cmd_buff[256];
	guint				ptz_cmd_length;
	guint 				proto_idx = 0;
	NF_PTZ_FUNC_PTR		ptz_func_ptr = NULL;
	
	NF_PTZ_PROTOCOL		*proto = NULL;	
	NF_PTZ_SYSDB_CH		*sysdb_ch = &self->sysdb_ptz[ cmd->ch ];

	int read_operation = 0;
	gboolean f_support = 0, z_support = 0;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(cmd != NULL, FALSE);
	
	proto_idx = sysdb_ch->protocol;
	g_return_val_if_fail  ( proto_idx < self->protocol_cnt,  FALSE);	
	proto =  self->protocol[proto_idx];
	switch(cmd->cmd)
		{
		case	NF_PTZ_CMD_PAN_LEFT_CON	     : /* _CON command for Vitek only! */
		case	NF_PTZ_CMD_PAN_LEFT          : ptz_func_ptr = proto->func_pan_left          ; break;
		case	NF_PTZ_CMD_PAN_RIGHT_CON     :
		case	NF_PTZ_CMD_PAN_RIGHT         : ptz_func_ptr = proto->func_pan_right         ; break;
		case	NF_PTZ_CMD_TILT_UP_CON       :
		case	NF_PTZ_CMD_TILT_UP           : ptz_func_ptr = proto->func_tilt_up           ; break;
		case	NF_PTZ_CMD_TILT_DOWN_CON     :
		case	NF_PTZ_CMD_TILT_DOWN         : ptz_func_ptr = proto->func_tilt_down         ; break;
		case	NF_PTZ_CMD_PT_LEFTUP_CON     :
		case	NF_PTZ_CMD_PT_LEFTUP         : ptz_func_ptr = proto->func_pt_leftup         ; break;
		case	NF_PTZ_CMD_PT_LEFTDOWN_CON   :
		case	NF_PTZ_CMD_PT_LEFTDOWN       : ptz_func_ptr = proto->func_pt_leftdown       ; break;
		case	NF_PTZ_CMD_PT_RIGHTUP_CON    :
		case	NF_PTZ_CMD_PT_RIGHTUP        : ptz_func_ptr = proto->func_pt_rightup        ; break;
		case	NF_PTZ_CMD_PT_RIGHTDOWN_CON  :
		case	NF_PTZ_CMD_PT_RIGHTDOWN      : ptz_func_ptr = proto->func_pt_rightdown      ; break;
		case	NF_PTZ_CMD_ZOOM_WIDE_CON     :
		case	NF_PTZ_CMD_ZOOM_WIDE         : ptz_func_ptr = proto->func_zoom_wide         ; break;
		case	NF_PTZ_CMD_ZOOM_TELE_CON     :
		case	NF_PTZ_CMD_ZOOM_TELE         : ptz_func_ptr = proto->func_zoom_tele         ; break;
		case	NF_PTZ_CMD_IRIS_OPEN_CON     :
		case	NF_PTZ_CMD_IRIS_OPEN         : ptz_func_ptr = proto->func_iris_open         ; break;
		case	NF_PTZ_CMD_IRIS_CLOSE_CON    :
		case	NF_PTZ_CMD_IRIS_CLOSE        : ptz_func_ptr = proto->func_iris_close        ; break;
		case	NF_PTZ_CMD_FOCUS_NEAR_CON    :
		case	NF_PTZ_CMD_FOCUS_NEAR        : ptz_func_ptr = proto->func_focus_near        ; break;
		case	NF_PTZ_CMD_FOCUS_FAR_CON     :
		case	NF_PTZ_CMD_FOCUS_FAR         : ptz_func_ptr = proto->func_focus_far         ; break;
		case	NF_PTZ_CMD_STOP              : ptz_func_ptr = proto->func_stop              ; break;
		case	NF_PTZ_CMD_SET_PRESET        : ptz_func_ptr = proto->func_set_preset        ; break;
		case	NF_PTZ_CMD_CLEAR_PRESET      : ptz_func_ptr = proto->func_clear_preset      ; break;
		case	NF_PTZ_CMD_GOTO_PRESET       : ptz_func_ptr = proto->func_goto_preset       ; break;
		case	NF_PTZ_CMD_PATTERN_START     : ptz_func_ptr = proto->func_pattern_start 	; break;
		case	NF_PTZ_CMD_PATTERN_STOP      : ptz_func_ptr = proto->func_pattern_stop  	; break;
		case	NF_PTZ_CMD_PATTERN_SET       : ptz_func_ptr = proto->func_pattern_set       ; break;
		case	NF_PTZ_CMD_SET_ZOOM_SPEED    : ptz_func_ptr = proto->func_set_zoom_speed    ; break;
		case	NF_PTZ_CMD_SET_FOCUS_SPEED   : ptz_func_ptr = proto->func_set_focus_speed   ; break;
		case	NF_PTZ_CMD_SET_IRIS_SPEED    : ptz_func_ptr = proto->func_set_iris_speed    ; break;
		case	NF_PTZ_CMD_SET_PANTILT_SPEED : ptz_func_ptr = proto->func_set_pantilt_speed ; break;
		case	NF_PTZ_CMD_SET_AUTO_FOCUS    : ptz_func_ptr = proto->func_set_auto_focus    ; break;
		case	NF_PTZ_CMD_SET_AUTO_IRIS     : ptz_func_ptr = proto->func_set_auto_iris     ; break;
		case	NF_PTZ_CMD_OSD_UP_KEY	     : ptz_func_ptr = proto->func_osd_up_key	    ; break;
		case	NF_PTZ_CMD_OSD_DOWN_KEY      : ptz_func_ptr = proto->func_osd_down_key      ; break;
		case	NF_PTZ_CMD_OSD_LEFT_KEY      : ptz_func_ptr = proto->func_osd_left_key      ; break;
		case	NF_PTZ_CMD_OSD_RIGHT_KEY     : ptz_func_ptr = proto->func_osd_right_key     ; break;
		case	NF_PTZ_CMD_OSD_ENTER_KEY     : ptz_func_ptr = proto->func_osd_enter_key     ; break;
		case	NF_PTZ_CMD_OSD_STOP_KEY      : ptz_func_ptr = proto->func_osd_stop_key      ; break;
		case	NF_PTZ_CMD_RESERVED0         : ptz_func_ptr = proto->func_reserved0         ; break;
		case	NF_PTZ_CMD_RESERVED1         : ptz_func_ptr = proto->func_reserved1         ; break;
		case	NF_PTZ_CMD_RESERVED2         : ptz_func_ptr = proto->func_reserved2         ; break;
		case	NF_PTZ_CMD_RESERVED3         : ptz_func_ptr = proto->func_reserved3         ; break;
		case	NF_PTZ_CMD_RESERVED4         : ptz_func_ptr = proto->func_reserved4         ; break;
		case	NF_PTZ_CMD_RESERVED5         : ptz_func_ptr = proto->func_reserved5         ; break;
		case	NF_PTZ_CMD_RESERVED6         : ptz_func_ptr = proto->func_reserved6         ; break;
		case	NF_PTZ_CMD_RESERVED7	     : ptz_func_ptr = proto->func_reserved7	        ; break;
		case	NF_PTZ_CMD_ZOOM_STOP		 : ptz_func_ptr = proto->func_stop			; break;
		default	:
			g_warning("%s error cmd[%d]",__FUNCTION__, cmd->cmd);
			break;
		}

	g_return_val_if_fail  ( ptz_func_ptr != NULL,  FALSE);

	if(nf_sysman_qcmode_is_enable())
	{
		#if defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E) || defined(_IPX_32P5)
			ptz_cmd_length = ptz_func_ptr( proto, sysdb_ch, cmd, ptz_cmd_buff);
			printf("ptz_cmd_length = [%d] , proto_name is [%s] addr is [%d] \n", ptz_cmd_length, proto->proto_name, proto->attr); 
			
			if(ptz_cmd_length == 0xff)
			{
				// nf_dev_decoder_set_ptz_cmd((guchar)sysdb_ch->ch, ptz_cmd_buff[0], 10, ptz_cmd_buff[1]);
			}
			else 
			{
					nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_TX);
					usleep(1000 * 50);
					nf_ptz_send_command(ptz_cmd_buff, ptz_cmd_length, sysdb_ch, proto->proto_name);
					usleep(1000 * 50);
					nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_RX);
			}	
		#endif 
	}
	
	if( sysdb_ch->is_rs485 ) {	
		nf_ipcam_get_ptz_support(cmd->ch, NF_IPCAM_IMAGE_ZOOM, &z_support);
		nf_ipcam_get_ptz_support(cmd->ch, NF_IPCAM_IMAGE_FOCUS, &f_support);
		if(z_support || f_support)
		{
			// for ipx only
			if( cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE )			nf_ipcam_set_zoom_wide ( cmd->ch, cmd, NULL, NULL, NULL );
			else if( cmd->cmd == NF_PTZ_CMD_ZOOM_TELE )		nf_ipcam_set_zoom_tele ( cmd->ch, cmd, NULL, NULL, NULL );
			else if( cmd->cmd == NF_PTZ_CMD_FOCUS_NEAR )	nf_ipcam_set_focus_near( cmd->ch, cmd->params[0] );
			else if( cmd->cmd == NF_PTZ_CMD_FOCUS_FAR )	    nf_ipcam_set_focus_far ( cmd->ch, cmd->params[0] );	
			else if( cmd->cmd == NF_PTZ_CMD_IRIS_OPEN )     nf_ipcam_set_iris_open ( cmd->ch );
			else if( cmd->cmd == NF_PTZ_CMD_IRIS_CLOSE )    nf_ipcam_set_iris_close( cmd->ch );		
			else if( cmd->cmd == NF_PTZ_CMD_ZOOM_STOP )		nf_ipcam_set_zoom_stop( cmd->ch );
			else if( cmd->cmd == NF_PTZ_CMD_STOP)         
			{
				ptz_cmd_length = ptz_func_ptr( proto, sysdb_ch, cmd, ptz_cmd_buff);
//				printf("1. cmd->cmd == NF_PTZ_CMD_STOP %d line  ptz_cmd_length is [%d] \n", __LINE__, ptz_cmd_length); 
				if (ptz_cmd_length)
				{
					nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_TX);
					usleep(1000 * 100);
					nf_ptz_send_command(ptz_cmd_buff, ptz_cmd_length, sysdb_ch, proto->proto_name);
					usleep(1000 * 100);
					nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_RX);
				}
				nf_ipcam_set_ptz_stop  ( cmd->ch, cmd, NULL, NULL, NULL );
			}
			else{
				ptz_cmd_length = ptz_func_ptr( proto, sysdb_ch, cmd, ptz_cmd_buff);
//				printf("2. cmd->cmd == NF_PTZ_CMD_STOP %d line  ptz_cmd_length is [%d] \n", __LINE__, ptz_cmd_length); 								
				if (ptz_cmd_length)
				{
					nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_TX);
					usleep(1000 * 100);
					nf_ptz_send_command(ptz_cmd_buff, ptz_cmd_length, sysdb_ch, proto->proto_name);
					usleep(1000 * 100);
					nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_RX);
				}			
			}
		}
		else
		{
			ptz_cmd_length = ptz_func_ptr( proto, sysdb_ch, cmd, ptz_cmd_buff);
			nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_TX);
			usleep(1000 * 100);			
			nf_ptz_send_command(ptz_cmd_buff, ptz_cmd_length, sysdb_ch, proto->proto_name);
			usleep(1000 * 100);
			nf_ptz_set_rs485_rtsn(NF_PTZ_SET_RS485_RX);
		}
				
	} else {
		// FIXME speed	
		if( cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE )			nf_ipcam_set_zoom_wide ( cmd->ch, cmd, NULL, NULL, NULL );
		else if( cmd->cmd == NF_PTZ_CMD_ZOOM_TELE )		nf_ipcam_set_zoom_tele ( cmd->ch, cmd, NULL, NULL, NULL );
		else if( cmd->cmd == NF_PTZ_CMD_FOCUS_NEAR )	nf_ipcam_set_focus_near( cmd->ch, cmd->params[0] );
		else if( cmd->cmd == NF_PTZ_CMD_FOCUS_FAR )	    nf_ipcam_set_focus_far ( cmd->ch, cmd->params[0] );	
		else if( cmd->cmd == NF_PTZ_CMD_IRIS_OPEN )     nf_ipcam_set_iris_open ( cmd->ch );
		else if( cmd->cmd == NF_PTZ_CMD_IRIS_CLOSE )    nf_ipcam_set_iris_close( cmd->ch );
		else if( cmd->cmd == NF_PTZ_CMD_STOP)           nf_ipcam_set_ptz_stop  ( cmd->ch, cmd, NULL, NULL, NULL );
		else if( cmd->cmd == NF_PTZ_CMD_ZOOM_STOP )		nf_ipcam_set_zoom_stop( cmd->ch );
		else if( cmd->cmd >= NF_PTZ_CMD_PAN_LEFT && cmd->cmd <= NF_PTZ_CMD_TILT_DOWN ) nf_ipcam_set_pt_move( cmd->ch, cmd, NULL, NULL, NULL );
		else if( cmd->cmd >= NF_PTZ_CMD_PT_LEFTUP && cmd->cmd <= NF_PTZ_CMD_PT_RIGHTDOWN ) nf_ipcam_set_pt_move( cmd->ch, cmd, NULL, NULL, NULL );
		else if( cmd->cmd == NF_PTZ_CMD_GOTO_PRESET && cmd->params[0] == 240 ) {	// MAGIC KEY PRESET choissi
			g_warning("%s NF_PTZ_CMD_GOTO_PRESET ch[%d][%d] -->> set_oneshot", __FUNCTION__ , cmd->ch, cmd->params[0]);
			nf_ipcam_set_oneshot(cmd->ch, NULL, NULL, NULL);					
		} else if( cmd->cmd == NF_PTZ_CMD_GOTO_PRESET && cmd->params[0] == 241 ) {
			g_warning("%s NF_PTZ_CMD_GOTO_PRESET ch[%d][%d] -->> lens_default", __FUNCTION__ , cmd->ch, cmd->params[0]);
			nf_ipcam_set_lens_default(cmd->ch, NULL, NULL, NULL);
		} else if (cmd->cmd == NF_PTZ_CMD_SET_PRESET) {
			nf_ipcam_set_preset(cmd->ch, cmd->params[0], NULL);
		} else if (cmd->cmd == NF_PTZ_CMD_CLEAR_PRESET) {
			nf_ipcam_clear_preset(cmd->ch, cmd->params[0], NULL);
		} else if (cmd->cmd == NF_PTZ_CMD_GOTO_PRESET) {
			nf_ipcam_go_preset(cmd->ch, cmd->params[0], NULL);
		}
	}
//	#endif 
	
	return 1;		
}

static void
_ptz_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_ptz != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM) {	// cam.ptz.Px.*		
		_nf_ptz->sysdb_reload = 1;
		_nf_ptz->sw_pat_sysdb_reload = 1;
	}
}

static void
_ptz_tmp_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_ptz != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_TMP_CHANGE_EVENTID_PTZ) {	// cam.ptz.Px.*				
		_nf_ptz->sw_pat_sysdb_reload = 1;
	}
}

//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	
#define CMDDELAY 1
static void ptz_thread_func (NfPtz *self)
{
	static NF_PTZ_CMD prv_con_cmd, prv_cmd;
	static int con_cmd_flag=0;
	static GTimeVal start_tv, curr_time;

	NF_PTZ_CMD *qitem;
	static int cmdDelay = CMDDELAY;
	g_message("%s start", __FUNCTION__);
	int i = 0; 

	int proto_index_vitek;
	static NF_PTZ_CMD stop_cmd;	
	static gboolean isCmdStop = FALSE;	
	static int sendStopCnt = 0;
	NF_PTZ_SYSDB_CH *sysdb_ch =NULL;
	guint proto_idx = 0;	
	
	// wait init complete
	while( _nf_ptz == NULL ) g_usleep(10*1000);
	
	self->init_done = 1;

	for(proto_index_vitek=0; proto_index_vitek<MAX_PTZ_PROTOCOL; proto_index_vitek++)
	{
		if(!strncmp("VITEK", self->protocol[proto_index_vitek]->proto_name, 5)) break;
	}
	while(self->thread_run)
	{							
		if ( cmdDelay > 0 )
		{
			--cmdDelay;
			g_usleep(10*1000);
			continue;
		}
		else
		{
			cmdDelay = CMDDELAY;
		}
			
		//fprintf(stderr, "@@@@@@@@@@ PTZ QUEUE POP(-) READY \n");

		if(con_cmd_flag)
		{	
			do {
				gettimeofday((struct timeval *)&curr_time, NULL);
				qitem = g_async_queue_try_pop(self->queue);
				if((curr_time.tv_sec - start_tv.tv_sec) > 3)
				{
					prv_con_cmd.cmd = NF_PTZ_CMD_STOP; 	
					qitem = (NF_PTZ_CMD *)g_malloc(sizeof(NF_PTZ_CMD));
					memcpy(qitem, &prv_con_cmd, sizeof(NF_PTZ_CMD));
					con_cmd_flag = 0;
					break;
				}
				if(qitem != NULL)
					start_tv = curr_time;	
			}while(qitem == NULL);
		}
		else
		{
			qitem = g_async_queue_pop(self->queue);
		}
		//fprintf(stderr, "@@@@@@@@@@ PTZ QUEUE POP GO ~~~~~~~~~~~~~~~~~~\n");
		
		if(qitem == NULL)  //If Q is empty
		{	
			g_usleep(10*1000);
			continue;
		}

		if(self->sysdb_reload)
		{
			_get_ptz_data_from_sysdb(self);
			self->sysdb_reload = 0;
		}
// #define DEBUG_PTZ_CMD		
#ifdef DEBUG_PTZ_CMD
		g_print( "%s ch[%d] cmd[%d][%s] params[%d][%d]\n", __FUNCTION__ ,
			qitem->ch, qitem->cmd, _conv_cmd_str((int)qitem->cmd),
			qitem->params[0], qitem->params[1] );
#endif
		if((qitem->cmd >= NF_PTZ_CMD_PAN_LEFT_CON) && (qitem->cmd <= NF_PTZ_CMD_FOCUS_FAR_CON))
		{
			if(self->sysdb_ptz[qitem->ch].protocol != proto_index_vitek)
			{
				qitem->cmd = qitem->cmd - NF_PTZ_CMD_PAN_LEFT_CON;
			}
			prv_con_cmd = *qitem;
			gettimeofday((struct timeval *)&start_tv, NULL);
			con_cmd_flag=1;
		}
		else
		{
			con_cmd_flag=0;
		}

		if(qitem->cmd == NF_PTZ_CMD_QC_TEST)  
		{
			nf_ptz_cmd( qitem );
			qitem->cmd = NF_PTZ_CMD_PAN_LEFT;

			g_usleep(100000);
		}

		if(prv_cmd.ch == qitem->ch)
		{
			if(qitem->cmd == NF_PTZ_CMD_STOP && prv_cmd.cmd == NF_PTZ_CMD_STOP)
			{
				;	
			}
			else
			{
				_ptz_cmd_process( self, qitem);
			}
		}
		else
		{
			_ptz_cmd_process( self, qitem);
		}
	
		prv_cmd.ch = qitem->ch;
		prv_cmd.cmd = qitem->cmd;
		
		if(qitem->cmd == NF_PTZ_CMD_QC_TEST_STOP)
		{
			nf_ptz_releaseCmd(qitem);
		}
		g_free(qitem);
		
	}
	g_message("%s end", __FUNCTION__);
}

static void _sw_ptz_set_user_cmd_cnt( int ch )
{
	g_return_if_fail ( _nf_ptz != NULL);
	g_return_if_fail ( ch >=0 && ch < NUM_ANALOG_CHANNEL);
	
	++_nf_ptz->sw_pat_status[ch].user_cmd_cnt;	
	
	return;
}

static void
ptz_sw_pat_thread_func (NfPtz *self)
{
	int i = 0,j; 
	g_message("%s start", __FUNCTION__);
	NF_PTZ_CMD	cmd;
	
	// wait init complete
	while( _nf_ptz == NULL ) g_usleep(10*1000);
		
	while(self->thread_run)
	{	

		if(self->sw_pat_sysdb_reload)
		{
			g_message("%s sysdb reload init status[%d]", __FUNCTION__, self->sw_pat_sysdb_reload);
			
			self->sw_pat_sysdb_reload = 0;
			_get_ptz_data_from_sysdb_sw_pat(self);	
						
			memset( self->sw_pat_status, 0x00, sizeof(self->sw_pat_status) );
			g_usleep(1000000); // 1 sec
		}
					
		for(i=0;i<NUM_ANALOG_CHANNEL; ++i)
		{
			NF_PTZ_SWPAT_SYSDB_CH  *psysdb = NULL;
			NF_PTZ_SWPAT_STATUS_CH  *pstate = &self->sw_pat_status[i];
			guint	preset, dwell;
			gboolean ret;


			// Mode has been changed
			if( self->sw_pat_sysdb_changed[i] ) {
				memset( &self->sw_pat_status[i], 0x00, sizeof( NF_PTZ_SWPAT_STATUS_CH ) );
				self->sw_pat_sysdb_changed[i] = 0;
			}

			// select sysdb info
			if( self->sw_pat_sysdb_mode[i] == 0 ) 
				continue;
			else if (self->sw_pat_sysdb_mode[i] == NF_PTZ_SWPAT_MODE_SCAN)	// scan mode
				psysdb = &self->sw_pat_sysdb[i][0];
			else  // NF_PTZ_SWPAT_MODE_TOUR
				psysdb = &self->sw_pat_sysdb[i][1];		// tour mode
			
			self->sw_pat_status[i].mode = self->sw_pat_sysdb_mode[i];			
															
			if( psysdb->cnt == 0 ) {
				continue;
			}
																		
			if( pstate->user_cmd_cnt )
			{
				pstate->user_cmd_cnt = 0;
				pstate->sleep_remain = NF_PTZ_SWPAT_SLEEP_SEC;
				pstate->dwell_remain = 0;				
			}
				
			if(	pstate->sleep_remain ) {
				--pstate->sleep_remain;

				if( pstate->sleep_remain > NF_PTZ_SWPAT_SLEEP_SEC)
					pstate->sleep_remain = NF_PTZ_SWPAT_SLEEP_SEC;

				continue;
			}

			if(	pstate->dwell_remain ) {
				--pstate->dwell_remain;
				
				if( pstate->dwell_remain > NF_PTZ_SWPAT_SLEEP_SEC)
					pstate->dwell_remain = NF_PTZ_SWPAT_SLEEP_SEC;
					
				continue;
			}						
			
			preset = psysdb->preset[pstate->step];
			dwell = psysdb->dwell[pstate->step];
		
			if(	preset )
			{
				// preset run;			
				memset( &cmd, 0x00, sizeof( cmd ));
				
				cmd.ch 			= i;
				cmd.cmd 		= NF_PTZ_CMD_GOTO_PRESET;
				cmd.params[0] 	= preset;
				cmd.reserved[0]	= NF_PTZ_SWPAT_CMD_MASK;
				ret = nf_ptz_cmd( &cmd );
				
				g_message("%s goto ch[%d] step[%d] -> preset[%d] ret[%d]", __FUNCTION__, 
							cmd.ch,  pstate->step,  cmd.params[0], ret);
															
				pstate->dwell_remain	= dwell;
				pstate->curr_preset		= preset;

			}
						
			// find next step
			for(j=0;j<NF_PTZ_SWPAT_STEP_CNT;++j)
			{
				guint t_step = 0;
				
				if(psysdb->cnt) 
					 t_step = (pstate->step+j+1)% psysdb->cnt;
				
				if(psysdb->preset[ t_step ] != 0 ){
					pstate->step = t_step ;
					break;
				}
			}			
		}
		
		g_usleep(1000000); // 1 sec
		
	}
	g_message("%s end", __FUNCTION__);
}


/**
	@brief				ptz �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_ptz_init(int wait)
{
	gboolean ret = TRUE;			
	gulong cb_handle = 0;
	
	g_return_val_if_fail (_nf_ptz == NULL, FALSE);	
		
	//_ptz_sysdb_db_fix();
	
	_nf_ptz = g_object_new ( NF_TYPE_PTZ , NULL);

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _ptz_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sysdb_tmp_change", _ptz_tmp_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	if( wait )
	{
		while( _nf_ptz->init_done != 1)
			g_usleep(10*1000);
	}
									
	return ret;
}

/*
	ptz�� hunter �ҽ��� �ִ� ���� ����.
	
	api �׷�
		�ܼ� ptz ����
		���� ����
		���� ������� �ֱ� ( ������ �� ��Ű�� �����尡 �����Ǿ� ����)
		
	ptz�� serial ��Ʈ���� (rs-485)	
	gobject module ��� ����( �������� ���� �������� ���� ����)				
		
	ptz ���������� ��� ����			
		ptz_ganz
		ptz_pelcod
		ptz_pelcop
*/



/**
	@brief				ptz �����ϴ� �������� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gint nf_ptz_protocol_get_cnt( void )
{

	g_return_val_if_fail (_nf_ptz != NULL, 0);	
	
	return _nf_ptz->protocol_cnt;
}

/**
	@brief				ptz �����ϴ� ��� ��� 
	@param[out]	error	
	@return	gboolean	
*/
guint nf_ptz_protocol_get_attr( gint channel )
{
	int 				bitfunc = 0;
	unsigned char		ptz_cmd_buff[256];
	guint				ptz_cmd_length;
	guint 				proto_idx = 0;
	NF_PTZ_CMD 			cmd;
	NF_PTZ_FUNC_PTR		ptz_func_ptr = NULL;
	
	NF_PTZ_PROTOCOL		*proto = NULL;	
	// FIXME channel < NUM_ANALOG_CHANNEL
	NF_PTZ_SYSDB_CH		*sysdb_ch = &_nf_ptz->sysdb_ptz[ channel ];
	
	proto_idx = sysdb_ch->protocol;					
	g_return_val_if_fail  ( proto_idx < _nf_ptz->protocol_cnt,  FALSE);	
	
	proto =  _nf_ptz->protocol[proto_idx];
	return proto->attr;

#if 0	
	// preset 0x02
	ptz_func_ptr = proto->func_set_preset;
		
	g_return_val_if_fail  ( ptz_func_ptr != NULL,  FALSE);

	cmd.cmd = NF_PTZ_CMD_SET_PRESET;
	
	ptz_cmd_length = ptz_func_ptr( proto, sysdb_ch, &cmd, ptz_cmd_buff);

	if (ptz_cmd_length)
	{
		bitfunc |= 0x02;
	}

	// swing 0x01
	ptz_func_ptr = proto->func_pattern_set;
		
	g_return_val_if_fail  ( ptz_func_ptr != NULL,  FALSE);

	cmd.cmd = NF_PTZ_CMD_PATTERN_SET;
	
	ptz_cmd_length = ptz_func_ptr( proto, sysdb_ch, &cmd, ptz_cmd_buff);

	if (ptz_cmd_length)
	{
		bitfunc |= 0x01;
	}
	return bitfunc;
#endif

}

/**
	@brief				ptz �����ϴ� �������� ���� ���
	@param[out]			proto_arr
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_protocol_get_info( NF_PTZ_PROTOCOL_INFO *proto_arr )
{
	guint i=0;
	
	g_return_val_if_fail (_nf_ptz != NULL, 0);	
	g_return_val_if_fail (proto_arr != NULL, 0);

	g_return_val_if_fail (_nf_ptz->protocol_cnt>0, 0);

	for(i=0;i<_nf_ptz->protocol_cnt; i++)
	{
		proto_arr[i].totalcnt = _nf_ptz->protocol_cnt;
		proto_arr[i].idx = _nf_ptz->protocol[i]->idx;
		proto_arr[i].attr = _nf_ptz->protocol[i]->attr;
			
		memcpy(proto_arr[i].name, _nf_ptz->protocol[i]->proto_name, sizeof(proto_arr[i].name));
	}
	
	return 1;	
}

/**
	@brief				ptz Ŀ�ǵ� ����
	@param[in]	pcmd	ptz Ŀ�ǵ� ����ü
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/

gboolean nf_ptz_cmd( NF_PTZ_CMD *pcmd )
{	
	NF_PTZ_CMD *qitem = NULL;
	gint qsize = 0;
	
	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);			
	g_return_val_if_fail ( pcmd != NULL, FALSE);	
	g_return_val_if_fail ( pcmd->ch < NUM_ANALOG_CHANNEL,  FALSE);	
	g_return_val_if_fail ( pcmd->cmd < NF_PTZ_CMD_NR,  FALSE);	

#ifdef DEBUG_PTZ_CMD  
	g_print( "\e[32m [ %s ] ch[%d] cmd[%d][%s] params[%d][%d] \e[0m \n", __FUNCTION__ ,
			pcmd->ch, pcmd->cmd, _conv_cmd_str((int)pcmd->cmd),
			pcmd->params[0], pcmd->params[1] );

#endif

#if 1  		// �ؾ��Ұ� ���� 2008-10-29 ���� 7:44:51 choissinf
     qsize = g_async_queue_length( _nf_ptz->queue );
     if(pcmd->cmd != NF_PTZ_CMD_STOP)
     	    g_return_val_if_fail ( qsize < 128 , FALSE);  
     else
     	    g_return_val_if_fail ( qsize < 256 , FALSE);

#endif
	if( pcmd->reserved[0] != NF_PTZ_SWPAT_CMD_MASK 
			&& pcmd->cmd != NF_PTZ_CMD_STOP
			&& pcmd->cmd != NF_PTZ_CMD_ZOOM_STOP
			&& pcmd->cmd != NF_PTZ_CMD_SET_ZOOM_SPEED
			&& pcmd->cmd != NF_PTZ_CMD_SET_FOCUS_SPEED
			&& pcmd->cmd != NF_PTZ_CMD_SET_IRIS_SPEED
			&& pcmd->cmd != NF_PTZ_CMD_SET_PANTILT_SPEED
			&& pcmd->cmd != NF_PTZ_CMD_SET_AUTO_FOCUS
			&& pcmd->cmd != NF_PTZ_CMD_SET_AUTO_IRIS )
		_sw_ptz_set_user_cmd_cnt ( pcmd->ch );
	
	qitem = g_malloc(sizeof(NF_PTZ_CMD));	
	g_return_val_if_fail ( qitem != NULL, FALSE);		
	
	memcpy( qitem, pcmd, sizeof(NF_PTZ_CMD));
       g_async_queue_push( _nf_ptz->queue, qitem);
	//g_free(qitem);
	return 1;	
}

// 2009.2.5 mybusisi -> queue push for repeate command
// 2009.7.4 khj776 -> queue push update

gboolean nf_ptz_RepeatCmd( NF_PTZ_CMD *pcmd , gint input_cmd)
{	
	NF_PTZ_CMD *qitem = NULL;
	gint qsize = 0;

	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);			
	g_return_val_if_fail ( pcmd != NULL, FALSE);	
	g_return_val_if_fail ( pcmd->ch < NUM_ANALOG_CHANNEL,  FALSE);	
	g_return_val_if_fail ( pcmd->cmd < NF_PTZ_CMD_NR,  FALSE);	

#if 1  		// �ؾ��Ұ� ���� 2008-10-29 ���� 7:44:51 choissinf
	 qsize = g_async_queue_length( _nf_ptz->queue ) ;
     if(pcmd->cmd != NF_PTZ_CMD_STOP)
     {
     	g_return_val_if_fail ( qsize < 128 , FALSE);
     }
     else
     {
     	g_return_val_if_fail ( qsize < 256 , FALSE);
     }
#endif

	qitem = g_malloc(sizeof(NF_PTZ_CMD));	
	g_return_val_if_fail ( qitem != NULL, FALSE);		

	memcpy( qitem, pcmd, sizeof(NF_PTZ_CMD));			
	qitem->cmd = input_cmd;

	g_async_queue_push( _nf_ptz->queue, qitem);

	//	g_free(qitem);
	return 1;	
}

gboolean nf_ptz_releaseCmd( NF_PTZ_CMD *pcmd)
{
	gint tmp_qsize;
	gint qsize = 0;

	NF_PTZ_CMD *qitem = NULL;

	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);			
	g_return_val_if_fail ( pcmd != NULL, FALSE);	
	g_return_val_if_fail ( pcmd->ch < NUM_ANALOG_CHANNEL,  FALSE);	
	g_return_val_if_fail ( pcmd->cmd < NF_PTZ_CMD_NR,  FALSE);	

#if 0
	qsize = g_async_queue_length( _nf_ptz->queue ) ;	
		
	if ( qsize > 0 )
	{
		for (tmp_qsize = 0; tmp_qsize < qsize; ++tmp_qsize )
		{
			g_async_queue_pop( _nf_ptz->queue);				
		}
	}
#else
	while( qitem = g_async_queue_try_pop( _nf_ptz->queue) ) {
		if(qitem) 
			g_free(qitem);
	}
#endif
	return 1;	
}


/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_start( gint ch, gint mode )
{

	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);			
	g_return_val_if_fail ( ch < NUM_ANALOG_CHANNEL,  FALSE);	
	g_return_val_if_fail ( mode < NF_PTZ_SWPAT_MODE_NR,  FALSE);	
	g_return_val_if_fail ( mode > NF_PTZ_SWPAT_MODE_OFF,  FALSE);		// 0

	g_message("%s ch[%d] mode[%d]",__FUNCTION__, ch, mode);
			
	_nf_ptz->sw_pat_sysdb_mode[ch] = mode;
	++_nf_ptz->sw_pat_sysdb_changed[ch];
	
	return 1;	
}


/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_stop( gint ch)
{

	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);				
	g_return_val_if_fail ( ch < NUM_ANALOG_CHANNEL,  FALSE);	
	
	_nf_ptz->sw_pat_sysdb_mode[ch] = NF_PTZ_SWPAT_MODE_OFF;
	++_nf_ptz->sw_pat_sysdb_changed[ch];
			
	return 1;	
}


/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_state( gint ch,  NF_PTZ_SWPAT_STATUS_CH  *status)
{
	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);		
	g_return_val_if_fail ( ch < NUM_ANALOG_CHANNEL,  FALSE);	
	g_return_val_if_fail ( status !=  NULL,  FALSE);	
	
	memcpy( status,  &_nf_ptz->sw_pat_status[ch],  sizeof(NF_PTZ_SWPAT_STATUS_CH));
	
	return 1;
	
}


/**
	@brief				ptz ���� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_ptz_pattern_preview( gint ch, gint mode, NF_PTZ_SWPAT_SYSDB_CH  *preview_data  )
{

	g_return_val_if_fail ( _nf_ptz != NULL, FALSE);			
	g_return_val_if_fail ( ch < NUM_ANALOG_CHANNEL,  FALSE);	
	g_return_val_if_fail ( mode < NF_PTZ_SWPAT_MODE_NR,  FALSE);	
	g_return_val_if_fail ( mode > NF_PTZ_SWPAT_MODE_OFF,  FALSE);		// 0
	
	g_return_val_if_fail ( preview_data != NULL, FALSE);			
		
#if 0	
	if( mode == NF_PTZ_SWPAT_MODE_TOUR ) {
		
		memcpy( &_nf_ptz->sw_pat_sysdb_preview[ch][1],  preview_data, 
				sizeof(NF_PTZ_SWPAT_STATUS_CH));	// tour
		
		_nf_ptz->sw_pat_sysdb_preview_flag[ch] |= (1<<NF_PTZ_SWPAT_MODE_TOUR);
		
	} else if( mode == NF_PTZ_SWPAT_MODE_SCAN ) {
		
		g_return_val_if_fail ( preview_data->cnt != 2 && preview_data->cnt != 0 , FALSE);			
		
		memcpy( &_nf_ptz->sw_pat_sysdb_preview[ch][0],  preview_data, 
				sizeof(NF_PTZ_SWPAT_STATUS_CH));	// scan

		_nf_ptz->sw_pat_sysdb_preview_flag[ch] |= (1<<NF_PTZ_SWPAT_MODE_SCAN);
		
	} else {
		
		g_warning("%s wrong mode[%d]", __FUNCTION__, mode );		
		return 0;
	}
#endif
						
	return 1;	
}


#ifdef TTYS1_DEV_ACT
static void _termios_act_init(int fd, int baudrate, int databit, int parity, int stopbit)
{
	printf("fd is [%d] , baudrate is [%d], databit is [%d] , parity is [%d] stopbit is [%d] \n", fd, baudrate, databit, parity, stopbit); 
	struct termios newtio;
	long BAUD = 9600, DATABITS = 8, PARITYON = 0, PARITY = 0, STOPBITS = 1;

	switch (baudrate) {
		case 230400:
           BAUD = B230400; 
           break;
        case 115200:
           BAUD = B115200; 
           break;
		case 57600:
           BAUD = B57600;
           break;   
		case 38400:
           BAUD = B38400;
           break;   
        case 19200:
           BAUD = B19200;
           break;
        case 9600:
           BAUD = B9600;
           break;
        case 4800:
           BAUD = B4800;
           break;
		case 2400:
           BAUD = B2400;
           break;
        case 1200:
           BAUD = B1200;
           break;
		default:
			BAUD = B9600;
			g_warning("%s baudrate range out [%d]", __FUNCTION__, baudrate);
			break;
   }

   switch (databit) {
        case 8:
           DATABITS = CS8;
           break;
        case 7:
           DATABITS = CS7;
           break;
        case 6:
           DATABITS = CS6;
           break;
        case 5:
	       DATABITS = CS5;
	       break;
		default:
			 DATABITS = CS8;
			g_warning("%s databit range out [%d]", __FUNCTION__, databit);
			break;
   }

   switch (parity) {
        case 0:
           PARITYON = 0;
           PARITY = 0;
           break;
        case 1:
	       PARITYON = PARENB;
	       PARITY = PARODD;
	       break;
        case 2:
           PARITYON = PARENB;
           PARITY = 0;
           break;
		default:
		   PARITYON = 0;
           PARITY = 0;
		   g_warning("%s parity range out [%d]", __FUNCTION__, parity);
		   break;
   }

   switch (stopbit) {
        case 1:
           STOPBITS = 0;
           break;
        case 2:
           STOPBITS = CSTOPB;
           break;
		default:
           STOPBITS = 0;
		   g_warning("%s stopbits range out [%d]", __FUNCTION__, stopbit);
		   break;
   }
   
	memset(&newtio, 0, sizeof(struct termios));
	newtio.c_cflag = BAUD | DATABITS | STOPBITS | PARITYON | PARITY;
//	newtio.c_cflag = BAUD | DATABITS | STOPBITS | PARITYON | PARITY | CREAD;
//	newtio.c_cflag = CS8 | CREAD | CLOCAL;

	tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);
	
}
#endif

static guint 
_baud_convertor(guint baud)
{
	guint baudrate = 9600;
	g_return_val_if_fail( (baud >= 2400) && (baud <= 115200) , 0);

	switch(baud)
	{
		case 2400:
			baudrate = 1;
			break;
		case 4800:
			baudrate = 2;
			break;
		case 9600:
			baudrate = 3;
			break;
		case 14400:
			baudrate = 4;
			break;
		case 19200:
			baudrate = 5;
			break;
		case 38400:
			baudrate = 6;
			break;
		case 57600:
			baudrate = 7;
			break;
		case 115200:
			baudrate = 8;
			break;
		default:
			g_warning("%s baudrate rage over baudrate[%d]", __FUNCTION__, baud);
			break;
	}
	return baudrate;
}


#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
static GStaticMutex ptz_mutex = G_STATIC_MUTEX_INIT;
#if 0
	GStaticMutex *nf_ptz_get_mutex(void)
	{
		return &ptz_mutex;
	}
#endif
#endif

static void	nf_ptz_send_command( unsigned char *data, guint ptz_cmd_length, NF_PTZ_SYSDB_CH	*sysdb_ch, const char *ptz_name)
{
	gint fd=0, ret=0;
	guint baudrate = 0;
	guint baud_store = 0;

	g_return_if_fail(data != NULL);
	g_return_if_fail(ptz_cmd_length > 0);
	g_return_if_fail(sysdb_ch != NULL);

#if 0 
	if (nf_sysman_get_serial_enable())
		fd = open("/dev/ttyAMA1", O_RDWR|O_NDELAY);
	else
		fd = open(NF_PTZ_DEV_NAME, O_RDWR|O_NDELAY);
#endif 

	fd = open(NF_PTZ_DEV_NAME, O_RDWR|O_NDELAY);
	
	g_return_if_fail(fd >= 0);

	#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
		nf_ptz_lock();
	#endif
	
#ifdef TTYS1_DEV_ACT
	_termios_act_init(fd, sysdb_ch->baud, 8, 0, 1);
	//_termios_act_init(fd, 9600, 8, 0, 1);
#else
	ret = ioctl(fd, FPGA_RS485_IOCRL_GET_BAUDRATE, &baudrate);
	if (ret == -1)
	{
		g_warning("ioctl error");
		goto nf_ptz_send_command_fail;
	}
	if (baudrate != sysdb_ch->baud )
	{
		ret = ioctl(fd, FPGA_RS485_IOCRL_REG_INIT);
		if (ret == -1)
		{
			g_warning("ioctl error");
			goto nf_ptz_send_command_fail;
		}
		
		baudrate = _baud_convertor(sysdb_ch->baud);
		
		if ( baudrate < 1 || baudrate > 8 )
		{
			g_warning("%s baudrate rate over baudrate[%d]", __FUNCTION__, baudrate );
			goto nf_ptz_send_command_fail;
		}
	
		ret = ioctl(fd, FPGA_RS485_IOCRL_SET_BAUDRATE, &baudrate);

		if (ret == -1)
		{
			g_warning("ioctl error");
			goto nf_ptz_send_command_fail;
		}
	}
#endif
	if (nf_sysman_qcmode_is_enable()) {    // for RS485 QC Test(ptz <-> keycntl)
		data = NF_SYSMAN_QC_RS485_TEXT;
		ptz_cmd_length = NF_SYSMAN_QC_RS485_TEXT_LEN;

/*		Send Debugging Code 

		int i=0; 
		for(i=0; i<NF_SYSMAN_QC_RS485_TEXT_LEN; i++) 
		{
			printf("check_check fd is [%d] . data[%d] is (0x%x) [%c]  \n", fd, i, data[i], data[i]); 
		}		 */
	}
	ptr_func_write(fd, data, ptz_cmd_length); 
	
#ifdef DEBUG_PTZ_CMD
	HexDump(data, ptz_cmd_length, 0);
#endif

	close(fd);	

	#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
		nf_ptz_unlock();
	#endif

	return TRUE;

nf_ptz_send_command_fail:

	close(fd);

	#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
		nf_ptz_unlock();
	#endif

	return FALSE; 
}

#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
void nf_ptz_lock(void)
{
	g_return_val_if_fail( _nf_ptz != NULL, -1 );
	
	g_static_mutex_lock (&ptz_mutex);
}

void nf_ptz_unlock(void)
{
	g_return_val_if_fail( _nf_ptz != NULL, -1 );

	g_static_mutex_unlock (&ptz_mutex);

}
#endif

#ifdef DEBUG_JBSHELL_PTZ

static char ptz_cmd_help[] = "ptz_cmd [ch] [cmd] [param0:0] [param1:0]";
static int ptz_cmd(int argc, char **argv)
{	

	gint ch, cmd, param0 = 0, param1 = 0;
	NF_PTZ_CMD tmp_cmd;
	
	g_return_val_if_fail( _nf_ptz != NULL, -1 );

	if(argc < 3){
		printf("%s\n",ptz_cmd_help);
		return -1;
	}
		
	ch = strtoul(argv[1],NULL,0);
	cmd = strtoul(argv[2],NULL,0);	
	
	if(argc > 3)
		param0 = strtoul(argv[3],NULL,0);

	if(argc > 4)
		param1 = strtoul(argv[4],NULL,0);
				
	
	memset( &tmp_cmd, 0x00, sizeof(NF_PTZ_CMD)); 
	
	tmp_cmd.ch = ch;
	tmp_cmd.cmd = cmd;
	tmp_cmd.params[0] = param0;
	tmp_cmd.params[1] = param1;
				
	nf_ptz_cmd( &tmp_cmd);
							
	return 0;
}
__commandlist(ptz_cmd,"ptz_cmd", ptz_cmd_help, ptz_cmd_help);



static char ptz_swpat_help[] = "ptz_swpat [ch] [mode]";
static int ptz_swpat(int argc, char **argv)
{	

	gint ch, cmd = 0;

	g_return_val_if_fail( _nf_ptz != NULL, -1 );

	if(argc < 3){
		printf("%s\n",ptz_swpat_help);
		return -1;
	}
		
	ch = strtoul(argv[1],NULL,0);
	cmd = strtoul(argv[2],NULL,0);	
	
	if (cmd ==0)
		nf_ptz_pattern_stop( ch );
	else 		
		nf_ptz_pattern_start( ch, cmd );
								
	return 0;
}
__commandlist(ptz_swpat,"ptz_swpat", ptz_swpat_help, ptz_swpat_help);

#endif




void ptr_func_write(int fd, const unsigned char *data, unsigned int ptz_cmd_length)
{
	void (*pfunc)(int fd, const unsigned char *data, unsigned int ptz_cmd_length);
	// 0 ~ 2 byte TAKENAKA define byte
	if((data[0] == 0xd0) && (data[1] == 0xe0) && (data[2] == 0xf0))
			pfunc = takex_write;
	else
		pfunc = general_write;

	pfunc(fd, data, ptz_cmd_length);
}


static void general_write(int fd, const unsigned char *data, unsigned int ptz_cmd_length)
{
	int ret;

	ret = write(fd, data, ptz_cmd_length);

	if (ret == 0) {
		g_warning("%s Write no data! Write size[%d] ret[%d]", __FUNCTION__, ptz_cmd_length, ret);
	}
	else if (ret == -1) {
		g_warning("%s Write error", __FUNCTION__);
	} 
	else if(ret != ptz_cmd_length) {
		g_warning("%s Write size[%d] ret[%d]", __FUNCTION__, ptz_cmd_length, ret);
	}
}

void nf_ptz_set_rs485_rtsn(int is_tx)
{
	#if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		nf_dev_board_pp_set_gpio(7, 2, is_tx);
	#elif defined(_IPX_0412M4)
		nf_dev_board_pp_set_gpio(12, 2, is_tx);
	#elif defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
		nf_dev_board_pp_set_gpio(7, 3, is_tx); 
	#else
		nf_dev_board_pp_set_gpio(11, 2, is_tx);
	#endif
}
