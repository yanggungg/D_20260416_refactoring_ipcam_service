#ifndef __NF_POS_H__
#define __NF_POS_H__

#include "nf_object.h"
#include "nf_common.h"
#include <termios.h>
#include <unistd.h>

#define TTYS1_DEV_ACT
#ifdef TTYS1_DEV_ACT 		//device : ttyS1
	#include <termios.h>
	#include <unistd.h>
	#define DEV_POS_RS485_NAME "/dev/ttyAMA1"
#else
	#include "fpga_rs485_ioctl.h"
	#define DEV_POS_RS485_NAME "/dev/fpga_rs485"
#endif

#define DEV_POS_RS232_NAME	"/dev/ttyAMA0"

#define DEV_POS_RS232_STR	"RS232"
#define DEV_POS_RS485_STR	"RS485"

#define MAX_POS_BUFFER		(1024*8)

#define NONE_PORT		1
#define RS232_MAX		1
#define RS485_MAX		1
#define USB_SERIAL_MAX	32

#define NF_POS_SERIAL_MAX	(NONE_PORT + RS232_MAX + RS485_MAX + USB_SERIAL_MAX)

typedef struct _NF_POS_SERIAL_INFO{
	gchar name[128];
} NF_POS_SERIAL_INFO;

typedef struct _NF_POS_SERIAL_OPTION{
	gboolean baudrate;
	gboolean databit;
	gboolean parity;
	gboolean stopbit;
} NF_POS_SERIAL_OPTION;

typedef struct _NF_POS_PARSE_OPTION{
	gboolean transaction_start;
	gboolean transaction_end;
	gboolean line_delimiter;
	gboolean ignore_string;
} NF_POS_PARSE_OPTION;

typedef enum _POS_TYPE_E {
	POS_TYPE_SERIAL = 0,
	POS_TYPE_NETWORK,
	POS_TYPE_MAX
} POS_TYPE_E;

typedef struct _NF_POS_DEV_NOTI {
	gint	connect;
	gint	type;	// not used;
	gchar	dev_str[128];
} NF_POS_DEV_NOTI;

gboolean nf_pos_get_serial_info(NF_POS_SERIAL_INFO *info, int *num);
gboolean nf_pos_get_serial_option(char *name, NF_POS_SERIAL_OPTION *option);
gboolean nf_pos_get_parse_option(char *protocol, NF_POS_PARSE_OPTION *option);

gboolean nf_pos_init(void);

int nf_pos_get_protocol_cnt(void);
gboolean nf_pos_get_protocol_list(char **protocol);
int nf_pos_get_charset_cnt(void);
gboolean nf_pos_get_charset_list(char **charset);

#define MAX_POS_PROTO_STR_LENGTH	32
#define MAX_POS_CHARSET_STR_LENGTH	32

typedef void (*cb_pos_test_fxn_t)(char *text);

typedef struct _NF_POS_DB_T {
	guint	    	act;
	guint			type;
	gchar			port_alias[256];
	guint			protocol;
	guint			char_set;
	guint			baud;
	guint			parity;
	guint			databit;
	guint			stopbit;
	gchar			transact_start[256];
	gchar			transact_end[256];
	gchar			endofline[256];
	gchar			ignore[256];
}NF_POS_DB_T;

gboolean nf_pos_is_485_use(void);

gboolean nf_pos_log_put_manual(int ch, char *buff);

typedef struct _NF_POS_TEXT_EVENT{
	int ch;
	guint64	timestamp;
	char data[128+1];
} NF_POS_TEXT_EVENT;

#endif
