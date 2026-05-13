#include "nf_common.h"
#include "nf_keyctrl.h"
#if !defined(TTYS1_DEV_ACT)
	#include "fpga_rs485_ioctl.h"
#endif
#include <sys/ioctl.h>
#include "nf_ptz.h"
#include "nf_qc.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "keyctrl"

// #define DEBUG_KEYCTRL_CMD
#define DEBUG_JBSHELL_KEYCTRL

#ifdef DEBUG_JBSHELL_KEYCTRL
#include "jbshell.h"
#endif

/* Object signals and args */
enum
{
	LAST_SIGNALTTYS1_DEV_ACT
};
  
enum
{
	PROP_0,
	LAST_PROP
	/* FILL ME */
};

extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_d_max;
extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_ganz;
extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_siemens;
extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_mesa;
extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_wonwoo;
extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_vicon;
extern NF_KEYCTRL_DECODE	_nf_keyctrl_proto_grundig;

static void nf_keyctrl_class_init 	 (NfKeyctrlClass * klass);
static void nf_keyctrl_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_keyctrl_set_property  (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_keyctrl_get_property  (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_keyctrl_dispose  (GObject * object);
static void nf_keyctrl_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfKeyctrl	*_nf_keyctrl = NULL;

static void
keyctrl_thread_func (NfKeyctrl * test) ;
guint _nf_keyctrl_qc_test = 0;

#ifdef TTYS1_DEV_ACT
static void 			nf_termios_act_init(int fd, int baudrate, int databit, int parity, int stopbit);
#endif

static int keyctrl_init_instance(NfKeyctrl *self, const NF_KEYCTRL_SYSDB *sysdb_keyctrl);
static int keyctrl_initialize(NfKeyctrl *self, const NF_KEYCTRL_SYSDB *sysdb_keyctrl);
static int keyctrl_baud_init(NfKeyctrl *self, int fd);
static void keyctrl_destroy(int fd);

NfKeyctrl *copy_self;
#if 0
static const char *_NF_KEYCTRL_PROTOCOL_STR[] = {
/* 00 */	"D-MAX",
/* 01 */	"GANZ",
/* 02 */    	"Siemens",
/* 03 */	"MESA-KB",
/* 04 */	"WonWoo",
};
#endif

static void dvrcmd_num1				(void);
static void dvrcmd_num2				(void);
static void dvrcmd_num3				(void);
static void dvrcmd_num4				(void);
static void dvrcmd_num5				(void);
static void dvrcmd_num6				(void);
static void dvrcmd_num7				(void);
static void dvrcmd_num8				(void);
static void dvrcmd_num9				(void);
static void dvrcmd_num10			(void);
static void dvrcmd_num11			(void);
static void dvrcmd_num12			(void);
static void dvrcmd_num13			(void);
static void dvrcmd_num14			(void);
static void dvrcmd_num15			(void);
static void dvrcmd_num16			(void);
static void dvrcmd_display			(void);
static void dvrcmd_seq				(void);
static void dvrcmd_panic			(void);
static void dvrcmd_zoom				(void);
static void dvrcmd_lock				(void);
static void dvrcmd_archive			(void);
static void dvrcmd_ptz				(void);
static void dvrcmd_setup			(void);
static void dvrcmd_search			(void);
static void dvrcmd_left				(void);
static void dvrcmd_right			(void);
static void dvrcmd_up				(void);
static void dvrcmd_down				(void);
static void dvrcmd_pause			(void);
static void dvrcmd_rw				(void);
static void dvrcmd_fw				(void);
static void dvrcmd_ff				(void);
static void dvrcmd_rf				(void);
static void dvrcmd_enter			(void);
static void dvrcmd_return			(void);
static void dvrcmd_hold				(void);
static void dvrcmd_power			(void);
static void dvrcmd_joystic_left 	(void);
static void dvrcmd_joystic_right	(void);
static void dvrcmd_relay			(void);
static void dvrcmd_logout			(void);
static void dvrcmd_stop				(void);
static void dvrcmd_reserved1		(void);
static void dvrcmd_reserved2		(void);
static void dvrcmd_reserved3		(void);
static void dvrcmd_reserved4		(void);
static void dvrcmd_reserved5		(void);
static void dvrcmd_reserved6		(void);
static void dvrcmd_reserved7		(void);
static void dvrcmd_reserved8		(void);
static void dvrcmd_reserved9		(void);
static void dvrcmd_reserved10		(void);
/////////////////////////////////////////////////////////////////////////////////////////////////////
GType
nf_keyctrl_get_type (void)
{
	static GType nf_keyctrl_type = 0;

	if (G_UNLIKELY (nf_keyctrl_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfKeyctrlClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_keyctrl_class_init,
			NULL,
			NULL,
			sizeof (NfKeyctrl),
			0,
			(GInstanceInitFunc) nf_keyctrl_instance_init,
			NULL
		};

		nf_keyctrl_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfKeyctrl", &object_info, 0);
	}

	return nf_keyctrl_type;
}

static void
nf_keyctrl_class_init (NfKeyctrlClass * klass)
{
	GObjectClass *gobject_class;
	int i;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_keyctrl_set_property;
	gobject_class->get_property = nf_keyctrl_get_property;

	gobject_class->dispose = nf_keyctrl_dispose;
	gobject_class->finalize = nf_keyctrl_finalize;

}

static const char *_conv_cmd_str(gint cmd_idx)
{
	g_return_val_if_fail ( cmd_idx >= 0 , "NULL");
	g_return_val_if_fail ( cmd_idx < NF_KEYCTRL_BUTTON_MAP_NR , "NULL");

	return _NF_KEYCTRL_BUTTON_STR[cmd_idx];
}

#ifdef TTYS1_DEV_ACT
static guint inline
_baud_convertor(guint baud)
{
	return baud;
}
#else
/*
#define		FPGA_RS485_BAUDRATE_2400	1
#define		FPGA_RS485_BAUDRATE_4800	2
#define		FPGA_RS485_BAUDRATE_9600	3
#define		FPGA_RS485_BAUDRATE_14400	4
#define		FPGA_RS485_BAUDRATE_19200	5
#define		FPGA_RS485_BAUDRATE_38400	6
#define		FPGA_RS485_BAUDRATE_57600	7
#define		FPGA_RS485_BAUDRATE_115200	8
*/
static guint
_baud_convertor(guint baud)
{
	guint baudrate = 9600;  

	if( (baud < 2400) && (baud > 115200) )
	{
		baud = baudrate;
	}
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
			baudrate = 3;
			break;
	}
	return baudrate;
}
#endif

static void
_get_keyctrl_data_from_sysdb(NfKeyctrl *self)
{
	gint i;

	g_message("%s",__FUNCTION__);

	self->sysdb_keyctrl.addr       = nf_sysdb_get_uint("sys.ctrl.addr");
	self->sysdb_keyctrl.protocol   = nf_sysdb_get_uint("sys.ctrl.protocol");
	self->sysdb_keyctrl.baud 	   = _baud_convertor( nf_sysdb_get_uint("sys.ctrl.baud") );

#ifdef __SUPPORT_POS__
	self->sysdb_keyctrl.pos_enable	= nf_sysdb_get_uint("sys.pos.enable");
#endif
}

static void _add_keyctrl_protocol(NfKeyctrl *self, NF_KEYCTRL_DECODE* protocol)
{
	gchar buff[128];
	gint idx = 0;

	g_return_if_fail( self != NULL);
	g_return_if_fail( self->protocol_cnt < MAX_KEYCTRL_PROTOCOL );
	g_return_if_fail( protocol != NULL);

	idx = self->protocol_cnt;

	self->protocol_decode[idx] = protocol;
	self->protocol_decode[idx]->idx = idx;

	snprintf(buff, sizeof(buff), "sys.info.keyctrl.K%d.name", idx);
	nf_sysdb_set_str(buff,  protocol->proto_name);
	nf_sysdb_set_uint("sys.info.keyctrl.KCNT",  idx+1 );

	g_message("%-16.16s keyctrl_protocol init[%d]" , protocol->proto_name, idx);
	++self->protocol_cnt;
}

static void
nf_keyctrl_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfKeyctrl *self = NF_KEYCTRL (instance);

	self->init_done = 0;
	self->protocol_cnt = 0;

	_get_keyctrl_data_from_sysdb(self);

	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_d_max);		// 0
	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_ganz);		// 1
	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_siemens);	// 2
	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_mesa);		// 3
	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_wonwoo);	// 4
	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_vicon);		// 5
	_add_keyctrl_protocol( self, &_nf_keyctrl_proto_grundig);	// 6

	g_message("%s proto_cnt[%d]", __FUNCTION__, self->protocol_cnt);

	self->handoff_func = NULL;
	self->handoff_arg = NULL;

	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)keyctrl_thread_func,
									self, FALSE, NULL);
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_keyctrl_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_keyctrl_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_keyctrl_set_property (GObject * object, guint prop_id,
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
nf_keyctrl_get_property (GObject * object, guint prop_id,
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
static gboolean _keyctrl_cmd_process( NfKeyctrl *self )
{
	guint	 				dvrcmd = 0;
	guint 					proto_idx = 0;
	NF_KEYCTRL_DECODE_PTR   keyctrl_func_deconde_ptr = NULL;
	NF_KEYCTRL_RECEIVE_PTR  keyctrl_func_receive_ptr = NULL;
	NF_KEYCTRL_DECODE		*proto = NULL;
	NF_KEYCTRL_SYSDB		*sysdb_keyctrl = &self->sysdb_keyctrl;
	int fd;
	int n;

	fd_set rxset, wkset;    //fd ���� group 
	int mx=0;
	struct timeval tv;
	static int length_check = KEYCTRL_PACKET_EMPTY;

fd_instance:
 
	fd = keyctrl_init_instance(self, sysdb_keyctrl);
	FD_ZERO(&rxset);
	FD_ZERO(&wkset);
	FD_SET(fd, &rxset);
	mx = fd;
	length_check = KEYCTRL_PACKET_EMPTY;

	while(1)
	{
#if defined(__SUPPORT_POS__) && !defined(__KEYCTRL_RS485_COMBINE__)
		/* pos�� ���� device�� ���� */
		if (self->sysdb_keyctrl.pos_enable)
		{
			g_usleep(100000);
		}
		else
		{
			if(self->sysdb_reload)
			{
				_get_keyctrl_data_from_sysdb(self);
				self->sysdb_reload = 0;
			}
		}

#endif	/* __SUPPORT_POS__ */

/*
		if((nf_ptz_get_cmd_queue_length() > 0) && (length_check != KEYCTRL_PACKET_PILE))
		{
			sem_post(&RS485_sem1);
			sem_wait(&RS485_sem2);
		}
*/
		keyctrl_baud_init(self, fd);

packet_pile:
		if(self->sysdb_reload) {
			_get_keyctrl_data_from_sysdb(self);
			self->sysdb_reload = 0;
		}
		
#if !defined(_IPX_0824VE) && !defined(_IPX_0412VE) && !defined(_IPX_0824ECO) && !defined(_IPX_0412ECO) \
	&& !defined(_IPX_1648VE3) && !defined(_IPX_0824VE3) && !defined(_IPX_0412VE3) \
	&& !defined(_IPX_0824P3) && !defined(_IPX_1648P3) && !defined(_IPX_0824P3ECO) && !defined(_IPX_1648P3ECO) \
	&& !defined(_IPX_0412P4) && !defined(_IPX_0824P4) && !defined(_IPX_1648P4) \
	&& !defined(_IPX_0412M4) && !defined(_IPX_0824M4) && !defined(_IPX_1648M4) && !defined(_IPX_0824P4E) && !defined(_IPX_1648P4E) \
	&& !defined(_IPX_0412M4E) && !defined(_IPX_0824M4E) && !defined(_IPX_1648M4E) && ! defined(_IPX_32P4E) &&!  defined(_IPX_32M4E) &&! defined(_IPX_32P5)

		keyctrl_baud_init(self, fd);
#endif
		memcpy(&wkset, &rxset, sizeof(fd_set));    
		
		tv.tv_sec  = 0;
		tv.tv_usec = 250000;  //250000;  // 0.1 sec

		n = Select(mx+1, &wkset, 0, 0, &tv);

#ifdef DEBUG_KEYCTRL_CMD 
		g_message("check_check_%s:%d n[%d] fd[%d] ", __FUNCTION__, __LINE__, n, fd);
#endif
	
		if(n < 0) {
			close(fd);
			goto fd_instance;
		}
		else if(n == 0)
			continue;

		//in playback? 0 : out playback  1 : in playback
		//self->isplayback = 0;

		if(FD_ISSET(fd, &wkset))
		{
			proto_idx = sysdb_keyctrl->protocol;
			g_return_val_if_fail  ( proto_idx < self->protocol_cnt,  FALSE);
#ifdef DEBUG_KEYCTRL_CMD
			g_message("%s protocol[%d], Address[%d], Baudrate[%d]", __FUNCTION__,
			sysdb_keyctrl->protocol, sysdb_keyctrl->addr, sysdb_keyctrl->baud);
#endif
			proto =  self->protocol_decode[proto_idx];
			g_return_val_if_fail  ( proto != NULL,  FALSE);

#ifdef DEBUG_KEYCTRL_CMD
			g_message("%s protocol index : %d    protocol name : %s", __FUNCTION__, proto->idx, proto->proto_name);
			g_message("%s protocol[%d], Address[%d], Baudrate[%d]", __FUNCTION__,
						sysdb_keyctrl->protocol, sysdb_keyctrl->addr, sysdb_keyctrl->baud);
#endif
			keyctrl_func_receive_ptr = proto->func_receive;
			length_check = keyctrl_func_receive_ptr(fd);
			
                     if (nf_sysman_qcmode_is_enable()) {
                          if (length_check == NF_SYSMAN_QC_RS485_OK) { 
                                _nf_keyctrl_qc_test = 1;
                          } else {
                                _nf_keyctrl_qc_test = 0;
                          }
                     }
			if(length_check == KEYCTRL_PACKET_ERR)
				continue;
			else if(length_check == KEYCTRL_PACKET_PILE)
				goto packet_pile;
			keyctrl_func_deconde_ptr = proto->func_read_command;

			// keyctrlroller cmd read
			dvrcmd = keyctrl_func_deconde_ptr(self);
			if(dvrcmd != 0)
			{
				self->handoff_func(self->handoff_arg, dvrcmd);
			}
		}
	}

#ifdef DEBUG_KEYCTRL_CMD
	g_message("%s key controller key value[%d]", __FUNCTION__, dvrcmd);
#endif

	if (dvrcmd != 0)
	{
		if(self->handoff_func)
		{
			self->handoff_func(self->handoff_arg, dvrcmd);
		}

		switch(dvrcmd)
		{
		case	NF_KEYCTRL_BUTTON_MAP_NUM1	        : dvrcmd_num1			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM2	        : dvrcmd_num2			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM3	        : dvrcmd_num3			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM4	        : dvrcmd_num4			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM5	        : dvrcmd_num5			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM6          : dvrcmd_num6			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM7          : dvrcmd_num7			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM8          : dvrcmd_num8			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM9          : dvrcmd_num9			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM10         : dvrcmd_num10			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM11	        : dvrcmd_num11			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM12         : dvrcmd_num12			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM13         : dvrcmd_num13			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM14         : dvrcmd_num14			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM15         : dvrcmd_num15			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_NUM16         : dvrcmd_num16			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_DISPLAY       : dvrcmd_display		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_SEQ           : dvrcmd_seq			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_PANIC         : dvrcmd_panic			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_ZOOM	        : dvrcmd_zoom			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_LOCK          : dvrcmd_lock			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_ARCHIVE       : dvrcmd_archive		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_PTZ           : dvrcmd_ptz			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_SETUP    	    : dvrcmd_setup			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_SEARCH        : dvrcmd_search			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_LEFT          : dvrcmd_left			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RIGHT	        : dvrcmd_right			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_UP            : dvrcmd_up				();                break;
		case	NF_KEYCTRL_BUTTON_MAP_DOWN          : dvrcmd_down			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_PAUSE	        : dvrcmd_pause			();	       		   break;
		case	NF_KEYCTRL_BUTTON_MAP_RW            : dvrcmd_rw				();			       break;
		case	NF_KEYCTRL_BUTTON_MAP_FW            : dvrcmd_fw				(); 		       break;
		case	NF_KEYCTRL_BUTTON_MAP_FF            : dvrcmd_ff				();			       break;
		case	NF_KEYCTRL_BUTTON_MAP_RF       	    : dvrcmd_rf				();			       break;
		case	NF_KEYCTRL_BUTTON_MAP_ENTER         : dvrcmd_enter			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RETURN        : dvrcmd_return			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_HOLD          : dvrcmd_hold			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_POWER         : dvrcmd_power			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_JOYSTIC_LEFT  : dvrcmd_joystic_left 	();                break;
		case	NF_KEYCTRL_BUTTON_MAP_JOYSTIC_RIGHT : dvrcmd_joystic_right	();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RELAY			: dvrcmd_relay			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_LOGOUT		: dvrcmd_logout			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_STOP			: dvrcmd_stop			();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED1     : dvrcmd_reserved1		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED2     : dvrcmd_reserved2		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED3     : dvrcmd_reserved3		();                break;
       	case	NF_KEYCTRL_BUTTON_MAP_RESERVED4     : dvrcmd_reserved4		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED5     : dvrcmd_reserved5		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED6     : dvrcmd_reserved6		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED7     : dvrcmd_reserved7		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED8     : dvrcmd_reserved8		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED9     : dvrcmd_reserved9		();                break;
		case	NF_KEYCTRL_BUTTON_MAP_RESERVED10    : dvrcmd_reserved10		();                break;
		default:
			g_warning("%s no keymapping button [%d]",__FUNCTION__, dvrcmd);
			break;
		}
	}

	return TRUE;
}

void nf_keyctrl_qc_test_check_reset(void)
{
	_nf_keyctrl_qc_test = 0;
}

guint nf_keyctrl_qc_test_check(void)
{	
        return _nf_keyctrl_qc_test;
}

void nf_keyctrl_qc_test_set(guint value)
{
	_nf_keyctrl_qc_test = value;
}

static int keyctrl_baud_init(NfKeyctrl *self, int fd)
{
	int ret;
	guint baudrate = 0;
	NF_KEYCTRL_SYSDB *sysdb_keyctrl = &self->sysdb_keyctrl;

	#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
		#if 0
			g_mutex_lock (g_static_mutex_get_mutex (nf_ptz_get_mutex()));
		#else
			nf_ptz_lock();
		#endif
	#endif
#ifdef TTYS1_DEV_ACT
	termios_act_init(fd, sysdb_keyctrl->baud, 8, 0, 1);
	#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
		nf_ptz_unlock();
	#endif

	return 0;
#else 
	printf("\e[32m 614 line  [%s] baudrate is [%d] \e[0m \n", __FUNCTION__, baudrate); 
	ret = ioctl(fd, FPGA_RS485_IOCRL_GET_BAUDRATE, &baudrate);
	if(ret == -1)
	{
		g_warning("GET_BAUDRATE Ioctl Error");

			goto keyctrl_baud_init_fail;
	}
	
	switch(baudrate)
	{
		case 2400:
			baudrate = FPGA_RS485_BAUDRATE_2400;
			break;
		case 4800:
			baudrate = FPGA_RS485_BAUDRATE_4800;
			break;
		case 9600:
			baudrate = FPGA_RS485_BAUDRATE_9600;
			break;
		case 14400:
			baudrate = FPGA_RS485_BAUDRATE_14400;
			break;
		case 19200:
			baudrate = FPGA_RS485_BAUDRATE_19200;
			break;
		case 38400:
			baudrate = FPGA_RS485_BAUDRATE_38400;
			break;
		case 57600:
			baudrate = FPGA_RS485_BAUDRATE_57600;
			break;
		case 115200:
			baudrate = FPGA_RS485_BAUDRATE_115200;
			break;
		default:
			g_warning("%s baudrate range over baudrate[%d]", __FUNCTION__, baudrate);
			baudrate  = FPGA_RS485_BAUDRATE_9600;
			break;
	}

	if(baudrate != sysdb_keyctrl->baud)
	{
		ret = ioctl(fd, FPGA_RS485_IOCRL_REG_INIT);
		if(ret == -1)
		{
			g_warning("REG_INIT Ioctl Error");
				goto keyctrl_baud_init_fail;
		}

		baudrate = sysdb_keyctrl->baud;
		if(baudrate < 1 || baudrate > 8)
		{
			g_warning("%s baudrate rate over baudrate[%d]", __FUNCTION__, baudrate);
			self->sysdb_reload = 1;
				goto keyctrl_baud_init_fail;
		}

		ret = ioctl(fd, FPGA_RS485_IOCRL_SET_BAUDRATE, &baudrate);
		self->sysdb_reload = 1;
		if (ret == -1)
		{
			g_warning("SET_BAUDRATE Ioctl Error");
				goto keyctrl_baud_init_fail;
		}
	}

		#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
			nf_ptz_unlock();
		#endif
		return 0;

keyctrl_baud_init_fail:
		#if defined(ENABLE_PTZ_KEYCTRL_MUTEX)
			nf_ptz_unlock();
		#endif

		return -1;
#endif
}
static int keyctrl_initialize(NfKeyctrl *self, const NF_KEYCTRL_SYSDB *sysdb_keyctrl)
{ 
	int fd; 
	int ret;
	guint baudrate = 0;

	fd = open(NF_KEYCTRL_DEV_NAME, O_RDONLY);
	if(fd < 0)
	{
		g_warning("%s error open [%s] ret[%d]", __FUNCTION__,
				NF_KEYCTRL_DEV_NAME, fd);
		return -1;
	}

	if(keyctrl_baud_init(self, fd) < 0)
		return -1;

	return fd;
}

static int keyctrl_init_instance(NfKeyctrl *self, const NF_KEYCTRL_SYSDB *sysdb_keyctrl)
{
	int fd;

	while(1)
	{
		fd = keyctrl_initialize(self, sysdb_keyctrl);

		if(fd < 0)
			g_warning("%s file descriptor ERROR", __FUNCTION__);
		else
			break;

		g_usleep(100000);
	}
	return fd;
}

// DVR Ation Function
static void dvrcmd_num1				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[0]);
}
static void dvrcmd_num2				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[1]);
}
static void dvrcmd_num3				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[2]);
}
static void dvrcmd_num4				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[3]);
}
static void dvrcmd_num5				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[4]);
}
static void dvrcmd_num6				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[5]);
}
static void dvrcmd_num7				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[6]);
}
static void dvrcmd_num8				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[7]);
}
static void dvrcmd_num9				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[8]);
}
static void dvrcmd_num10			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[9]);
}
static void dvrcmd_num11			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[10]);
}
static void dvrcmd_num12			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[11]);
}
static void dvrcmd_num13			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[12]);
}
static void dvrcmd_num14			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[13]);
}
static void dvrcmd_num15			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[14]);
}
static void dvrcmd_num16			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[15]);
}
static void dvrcmd_display			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[16]);
}
static void dvrcmd_seq				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[17]);
}
static void dvrcmd_panic			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[18]);
}
static void dvrcmd_zoom				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[19]);
}
static void dvrcmd_lock				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[20]);
}
static void dvrcmd_archive			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[21]);
}
static void dvrcmd_ptz				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[22]);
}
static void dvrcmd_setup			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[23]);
}
static void dvrcmd_search			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[24]);
}
static void dvrcmd_left				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[25]);
}
static void dvrcmd_right			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[26]);
}
static void dvrcmd_up				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[27]);
}
static void dvrcmd_down				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[28]);
}
static void dvrcmd_pause			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[29]);
}
static void dvrcmd_rw				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[30]);
}
static void dvrcmd_fw				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[31]);
}
static void dvrcmd_ff				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[32]);
}
static void dvrcmd_rf				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[33]);
}
static void dvrcmd_enter			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[34]);
}
static void dvrcmd_return			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[35]);
}
static void dvrcmd_hold				(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[36]);
}
static void dvrcmd_power			(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[37]);
}
static void dvrcmd_joystic_left 	(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[38]);
}
static void dvrcmd_joystic_right	(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[39]);
}
static void dvrcmd_relay	(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[40]);
}
static void dvrcmd_logout	(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[41]);
}
static void dvrcmd_stop		(void)
{
	g_message("%s  %s call", __FUNCTION__, _NF_KEYCTRL_BUTTON_STR[41]);
}
static void dvrcmd_reserved1		(void)
{
}
static void dvrcmd_reserved2		(void)
{
}
static void dvrcmd_reserved3		(void)
{
}
static void dvrcmd_reserved4		(void)
{
}
static void dvrcmd_reserved5		(void)
{
}
static void dvrcmd_reserved6		(void)
{
}
static void dvrcmd_reserved7		(void)
{
}
static void dvrcmd_reserved8		(void)
{
}
static void dvrcmd_reserved9		(void)
{
}
static void dvrcmd_reserved10		(void)
{
}

/*
UI API
*/
gboolean
nf_keyctrl_register_handoff(NF_KEYCTRL_HANDOFF_FUNC handoff_func, gpointer handoff_arg )
{
	g_return_val_if_fail( _nf_keyctrl != NULL, 0);

	_nf_keyctrl->handoff_func = handoff_func;
	_nf_keyctrl->handoff_arg = handoff_arg;

	return 1;
}

static void
_keyctrl_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_keyctrl != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS)
		_nf_keyctrl->sysdb_reload = 1;
}

static void
keyctrl_thread_func (NfKeyctrl *self)
{
	guint set_status;

	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_keyctrl == NULL ) g_usleep(10*1000);

	self->init_done = 1;

#if 0
	set_status = nf_notify_get_param0("dvr_status");
	if ( set_status == NF_DVR_STATUS_PTZ )
	{
		self->thread_run = 0;
	}
	else
	{
		self->thread_run = 1;
	}
#endif
	while(self->thread_run)
	{

#ifdef __SUPPORT_POS__
		if(self->sysdb_reload)
		{
			_get_keyctrl_data_from_sysdb(self);
			self->sysdb_reload = 0;
		}

		/* pos�� ���� device�� ���� */
		if (!self->sysdb_keyctrl.pos_enable)
		{
			_keyctrl_cmd_process(self);
		}
		else
		{
			g_usleep(100000);
		}
#else
		if(self->sysdb_reload)
		{
			_get_keyctrl_data_from_sysdb(self);
			self->sysdb_reload = 0;
		}
		_keyctrl_cmd_process(self);
#endif

#if 0
		set_status = nf_notify_get_param0("dvr_status");
		if ( set_status == NF_DVR_STATUS_PTZ )
		{
			self->thread_run = 0;
		}
		else
		{
			self->thread_run = 1;
		}
#endif
	}
	g_message("%s end", __FUNCTION__);
}

/**
	@brief				keyctrl �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_keyctrl_init(int wait)
{
	gboolean ret = TRUE;
	gulong cb_handle = 0;
	g_message("%s end", __FUNCTION__);
	g_return_val_if_fail (_nf_keyctrl == NULL, FALSE);

	_nf_keyctrl = g_object_new ( NF_TYPE_KEYCTRL , NULL);

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _keyctrl_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	if( wait )
	{
		while( _nf_keyctrl->init_done != 1)
			g_usleep(10*1000);
	}

	return ret;
}

/** 
	@brief				keyctrl �����ϴ� �������� ����
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gint nf_keyctrl_protocol_get_cnt( void )
{

	g_return_val_if_fail (_nf_keyctrl != NULL, 0);

	return _nf_keyctrl->protocol_cnt;
}

#ifdef TTYS1_DEV_ACT
void termios_act_init(int fd, int baudrate, int databit, int parity, int stopbit)
{
	struct termios newtio;
	long BAUD = 9600, DATABITS = 8, PARITYON = 0, PARITY = 0, STOPBITS = 1;

	baudrate = _baud_convertor(baudrate);

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
#if 1
	//  tcgetattr(fd,&oldtio); /* ���� ������ oldtio�� ���� */

	  bzero(&newtio, sizeof(newtio));
	 // newtio.c_cflag = BAUD | DATABITS | STOPBITS | PARITYON | PARITY | CREAD;
	  newtio.c_cflag  = BAUD | CRTSCTS | CS8 | CLOCAL | CREAD;
	  newtio.c_iflag  = IGNPAR;
	  newtio.c_oflag  = 0;

	  /* set input mode (non-canonical, no echo,...) */
	  newtio.c_lflag = 0;

	  newtio.c_cc[VTIME]    = 0;   /* ���� ������ timer�� disable */
	//  newtio.c_cc[VMIN]     = 5;   /* �ּ� 5 ���� ���� ������ blocking */
	  newtio.c_cc[VMIN]     = 0;   /* �ּ� 5 ���� ���� ������ blocking */

	  tcflush(fd, TCIFLUSH);
	  tcsetattr(fd,TCSANOW,&newtio);
#if 1
	{
		int modemctlline;
//		g_message("RS485 ::: %s", __FUNCTION__);
		modemctlline = TIOCM_RTS;
		ioctl( fd, TIOCMBIC, &modemctlline );
		g_usleep(10*1000);
	}
#endif
#else
           memset(&newtio, 0, sizeof(struct termios));
           newtio.c_cflag = BAUD | DATABITS | STOPBITS | PARITYON | PARITY;
           tcflush(fd, TCIFLUSH);
  	  tcsetattr(fd, TCSANOW, &newtio);
#endif

}
#endif

#ifdef DEBUG_JBSHELL_KEYCTRL

void keyctrl_handoff_func ( gpointer handoff_arg, guint key_data )
{
	g_message("%s handoff_arg[0x%x] key_data[%d]", __FUNCTION__, handoff_arg ,key_data);
}

static char keyctrl_cmd_help[] = "keyctrl_cmd";
static int keyctrl_cmd(int argc, char **argv)
{
	gpointer handoff_arg = NULL;

	nf_keyctrl_register_handoff(&keyctrl_handoff_func,  handoff_arg);

	return 0;
}
__commandlist(keyctrl_cmd,"keyctrl_cmd", keyctrl_cmd_help, keyctrl_cmd_help);

#endif
