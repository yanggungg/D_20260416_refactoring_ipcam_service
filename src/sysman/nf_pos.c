#include "nf_pos.h"
#include "nf_sysman.h"
#include "nf_api_disk.h"

#include "nf_api_pos_eventlog.h"
#include "nf_logevtdef.h"
#include "jbshell.h"
#include <sys/ioctl.h>

//#define TEST_POS_DEV

typedef struct _NF_POS_USB_STAT{
	gchar dev_path[256];
	gchar sys_path[512];
	gint  connect;
	gint  gen_alias;
	gchar physical[256];
	gchar alias[256];
}NF_POS_USB_STAT;

typedef struct _NF_POS_CONF {
	NF_POS_USB_STAT usb_stat[USB_SERIAL_MAX];

    GAsyncQueue     *pos_db_queue;
	gboolean		connect[NUM_ACTIVE_CH];

	gint			fd[NUM_ACTIVE_CH];
	gint			need_refresh[NUM_ACTIVE_CH];
	NF_POS_DB_T		pos_db[NUM_ACTIVE_CH];

	gboolean		watchdog_on[NUM_ACTIVE_CH];
	guint			watchdog_cnt[NUM_ACTIVE_CH];
} NF_POS_CONF;

typedef struct _CAHR_INFO{
	int num;
	char array_char[64];
} CHAR_INFO;

typedef struct _NF_POS_PARSER{
	GList *trs_start;
	GList *trs_end;
	GList *eol;
	GList *ignore;
} NF_POS_PARSER;

typedef struct _NF_POS_DATA{
	char		buffer[MAX_POS_BUFFER];
	int			buffer_len;
} NF_POS_DATA;

typedef struct _POS_TEST_INFO{
	gboolean action;
	int ch;
	cb_pos_test_fxn_t cb_func;
	NF_POS_DB_T test_db;
} POS_TEST_INFO;

typedef struct _POS_POSITION{
	int start;
	int end;
} POS_POSITION;

static NF_POS_DATA _nf_pos_data[NUM_CHANNEL];
static POS_TEST_INFO pos_test_info;
static guint pos_display = 0;
//static guint pos_display = 1;
static NF_POS_CONF	_nf_pos;
static NF_POS_PARSER nf_pos_parser[NUM_CHANNEL];
static int start_test_flag = 0;
static int end_test_flag = 0;

static guint pos_recv[NUM_ACTIVE_CH] = {0, };

static void refresh_pos_device(int ch);
static void _pos_make_parser(int ch);
static NF_POS_PARSER* _pos_get_parser_by_ch(int ch);

#if defined(TEST_POS_DEV)

typedef struct _POS_ITEM {
	char item[64];
	int price;
} POS_ITEM;

typedef struct _POS_DATA
{
	char item[64];
} POS_DATA;

static POS_ITEM item_list[12] = { {"@Linux",0},
				           {"@Milk", 4},
				           {"@Dragonfly", 31},
				           {"@Noodle", 1},
				           {"@Cigarette", 2},
				           {"@Coke", 2},
				           {"@USB", 9},
				           {"@Hamburger", 4},
				           {"@Beer", 3},
				           {"@DVR", 12},
				           {"@Camera", 7},
				           {"@Apple", 6}
};

static int ch_timer[NUM_ACTIVE_CH] = {0, };
static int seed = NULL;
static int pos_rand()
{
	GTimeVal cur;
	int ret;
	g_get_current_time(&cur);
	seed += cur.tv_usec;
	ret = rand_r(&seed);
	seed += ret;
	return ret;
}

static int _random_buf(POS_DATA *pos_data)
{
	int pre_num[12] = {0, };
	int total_n = 0;
	int n = 0;
	int num = 0;
	int i,j;
	int count;
	int total;
	int item_total = 0;

	total_n = pos_rand() % 12;

	for(j=0; j<total_n; j++)
	{
/*
		num = pos_rand() % 12;
*/
		while(1)
		{
			num = pos_rand() % 12;

			for(i = 0; i < n; i++)
			{
				if( pre_num[i] == num )
					break;
			}

			if(i == n)
				break;
			else
				continue;
		}

		pre_num[n] = num;
		count = pos_rand() % 30;

		total = count * item_list[num].price;

		item_total += total;

		sprintf(pos_data[n].item, "%s   $%d %d - %d dollar", item_list[num].item, item_list[num].price, count, total);

		n++;
	}

	sprintf(pos_data[n].item, "@Total         %d dollar", item_total);
	n++;

	return n;
}

static int _pos_event_put(int ch, int num, POS_DATA *pos_data)
{
	int i;

	GTimeVal tv;
	gchar text[256];
	memset(text, 0x00, sizeof(text));

	gettimeofday(&tv, NULL);

	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)text;

	for( i = num-1; i >= 0; i--)
	{
		log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
		log_data->chan = ch;
		log_data->pos_log_type = NF_POS_LOG_TYPE_TEXT;
		sprintf(log_data->Text.text, "%s", pos_data[i].item);

		nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, text);
	}

	if(pos_display)
	{
		printf("\n--- POS DATA -- CH : %d START --\n", ch);
		for( i = num-1; i >= 0; i--)
		{
			printf("%s\n", pos_data[i].item);
		}
		printf("\n--- POS DATA -- CH : %d END --\n", ch);
	}

	return 0;
}
#endif

static void termios_act_init(int fd, int baudrate, int databit, int parity, int stopbit)
{
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
#if 1
	//  tcgetattr(fd,&oldtio); /* 현재 설정을 oldtio에 저장 */

	  bzero(&newtio, sizeof(newtio));
//	  newtio.c_cflag  = BAUD | CRTSCTS | CS8 | CLOCAL | CREAD;
	  newtio.c_cflag  = BAUD | DATABITS | PARITYON | PARITY | STOPBITS | CRTSCTS | CLOCAL | CREAD;

	  newtio.c_iflag  = IGNPAR;
	  newtio.c_oflag  = 0;

	  /* set input mode (non-canonical, no echo,...) */
	  newtio.c_lflag = 0;

	  newtio.c_cc[VTIME]    = 0;   /* 문자 사이의 timer를 disable */
	  //newtio.c_cc[VMIN]     = 10;   /* 최소 5 문자 받을 때까진 blocking */
	  newtio.c_cc[VMIN]     = 0;   /* 최소 5 문자 받을 때까진 blocking */

	  tcflush(fd, TCIFLUSH);
	  tcsetattr(fd,TCSANOW,&newtio);
#else
      memset(&newtio, 0, sizeof(struct termios));
      newtio.c_cflag = BAUD | DATABITS | STOPBITS | PARITYON | PARITY;
      tcflush(fd, TCIFLUSH);
  	  tcsetattr(fd, TCSANOW, &newtio);
#endif

}

static int pos_baud_init(int fd, int baudrate)
{
	int ret;

#ifdef TTYS1_DEV_ACT
	termios_act_init(fd, baudrate, 8, 0, 1);
#else

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

	ret = ioctl(fd, FPGA_RS485_IOCRL_REG_INIT);
	if(ret == -1)
		return -1;

	ret = ioctl(fd, FPGA_RS485_IOCRL_SET_BAUDRATE, &baudrate);
	if (ret == -1)
		return -1;
#endif

	return 0;
}

static void _get_usb_serial_physical(char *sys_path, char *buff, int buff_size)
{
	char tmp[256];
	int ret;
	char *p;

	memset(buff, 0x0, buff_size);

	ret = readlink(sys_path, tmp, sizeof(tmp)-1);
	if(ret > 0)
	{
		tmp[ret] = 0;
		p = strrchr(tmp,':');
		if(p)
		{
			*p = 0;

			p = strrchr(tmp,'/');
			if(p)
			{
				snprintf(buff, buff_size, "%s", p+1);
			}
		}
	}

	g_message("%s : %s => %s", __FUNCTION__, sys_path, buff);
}

static void _make_usb_serial_alias(char *physical, int curr_idx, char *alias, int alias_size)
{
	int len;
	char buff[512];

	memset(alias, 0x0, alias_size);
	memset(buff, 0x0, sizeof(buff));

	len = strlen(physical);

	if(len > 0)
	{
		int i = 0;
		int cnt = 0;
		char *str_p;
		char cnt_tmp[16];

		for(i=0; i < curr_idx; i++)
		{
			if( !strcmp(physical, _nf_pos.usb_stat[i].physical) )
			{
				cnt++;
			}
		}

		if( strlen(physical) == 0 )
			return;

		strcpy(buff, "USB-");

		str_p = physical;

		for(; *str_p != 0; str_p++)
		{
			if(*str_p == '.')
				strcat(buff, "-H-");
			else
			{
				char tmp[16];
				snprintf(tmp, sizeof(tmp), "%c", *str_p);

				strcat(buff, tmp);
			}
		}

		snprintf(cnt_tmp, sizeof(cnt_tmp), ":%d", cnt+1);
		strcat(buff, cnt_tmp);

		snprintf(alias, alias_size, "%s", buff);
	}
}

/*
static void _make_usb_serial_info(char *sys_path, NF_POS_USB_STAT *usb_stat)
{
	char dev_path[256];
	int ret;
	char *p;

	ret = readlink(sys_path, dev_path, sizeof(dev_path)-1);
	if(ret > 0)
	{
		memset(usb_stat->physical, 0x0, sizeof(usb_stat->physical));
		memset(usb_stat->alias, 0x0, sizeof(usb_stat->alias));

		dev_path[ret] = 0;
		p = strrchr(dev_path,':');
		if(p)
		{
			*p = 0;

			p = strrchr(dev_path,'/');
			if(p)
			{
				char *str_p;

				strcpy(usb_stat->physical, p+1);

				str_p = strchr(usb_stat->physical,'.');
				str_p++;

				strcpy(usb_stat->alias, "USB-");

				for(; *str_p != NULL; str_p++)
				{
					if(*str_p == '.')
						strcat(usb_stat->alias, "-H-");
					else
					{
						char tmp[16];
						snprintf(tmp, sizeof(tmp), "%c", *str_p);

						strcat(usb_stat->alias, tmp);
					}
				}
			}
		}
	}
}
*/

static int search_channel_by_alias(char *alias)
{
	int ch;

	for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		if( strcmp(_nf_pos.pos_db[ch].port_alias, alias) == 0 )
			break;
	}

	if( ch == NUM_ACTIVE_CH )
		return -1;
	else
		return ch;
}

static void _load_pos_db(NF_POS_DB_T *pos_db)
{
	int ch, i;
	char key[128];
	gchar *val_str;

	for(ch=0; ch < NUM_ACTIVE_CH; ch++)
	{
		snprintf(key, sizeof(key),"sys.pos.P%d.enable", ch);
		pos_db[ch].act = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.type", ch);
		pos_db[ch].type = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.port", ch);
		val_str = nf_sysdb_get_str_nocopy(key);
		if(val_str)
			snprintf(pos_db[ch].port_alias, "%s", val_str);

		snprintf(key, sizeof(key), "sys.pos.P%d.protocol", ch);
		pos_db[ch].protocol = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.char_set", ch);
		pos_db[ch].char_set = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.baud", ch);
		pos_db[ch].baud = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.parity", ch);
		pos_db[ch].parity = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.databit", ch);
		pos_db[ch].databit = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.stopbit", ch);
		pos_db[ch].stopbit = nf_sysdb_get_uint(key);

		snprintf(key, sizeof(key), "sys.pos.P%d.transact_start", ch);
		val_str = nf_sysdb_get_str_nocopy(key);
		if(val_str)
			snprintf(pos_db[ch].transact_start, "%s", val_str);

		snprintf(key, sizeof(key), "sys.pos.P%d.transact_end", ch);
		val_str = nf_sysdb_get_str_nocopy(key);
		if(val_str)
			snprintf(pos_db[ch].transact_end, "%s", val_str);

		snprintf(key, sizeof(key), "sys.pos.P%d.endofline", ch);
		val_str = nf_sysdb_get_str_nocopy(key);
		if(val_str)
			snprintf(pos_db[ch].endofline, "%s", val_str);

		snprintf(key, sizeof(key), "sys.pos.P%d.ignore", ch);
		val_str = nf_sysdb_get_str_nocopy(key);
		if(val_str)
			snprintf(pos_db[ch].ignore, "%s", val_str);
	}
}

static void _pos_sysdb_change(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS)
	{
		NF_POS_DB_T *pos_db;

		g_message( "%s", __FUNCTION__);

		pos_db = g_malloc0(sizeof(NF_POS_DB_T) * NUM_ACTIVE_CH);

		_load_pos_db(pos_db);

		g_async_queue_push(_nf_pos.pos_db_queue, pos_db);
	}
}

static NF_POS_USB_STAT* _pos_get_usb_stat_data_by_idx(int idx)
{
	NF_POS_USB_STAT *usb_stat;

	usb_stat = &_nf_pos.usb_stat[idx];

	return usb_stat;
}

static void _usb_serial_monitor(void *arg)
{
	int i;
	int ch;
	NF_POS_USB_STAT *usb_stat;

	while(1)
	{
		for(i=0; i < USB_SERIAL_MAX; i++)
		{
			usb_stat = _pos_get_usb_stat_data_by_idx(i);

			if(access(usb_stat->sys_path, F_OK) == 0)
			{
				if( usb_stat->connect != 1 )
				{
					if( usb_stat->gen_alias == 0 )
					{
						_get_usb_serial_physical(usb_stat->sys_path, usb_stat->physical, sizeof(usb_stat->physical));
						_make_usb_serial_alias(usb_stat->physical, i, usb_stat->alias, sizeof(usb_stat->alias));
	//					_make_usb_serial_info(tmp, usb_stat);

						usb_stat->gen_alias = 1;
					}
					else
					{
						if(access(usb_stat->dev_path, F_OK) == 0)
						{
							NF_POS_DEV_NOTI dev_noti;

							memset(&dev_noti, 0x0, sizeof(NF_POS_DEV_NOTI));

							usb_stat->connect = 1;
							dev_noti.connect = 1;

							ch = search_channel_by_alias(usb_stat->alias);

							if( ch >= 0 )
								_nf_pos.need_refresh[ch] = 1;

							snprintf(dev_noti.dev_str, sizeof(dev_noti.dev_str), "%s", usb_stat->alias);

							nf_notify_fire_pointer("pos_dev", &dev_noti, sizeof(NF_POS_DEV_NOTI));
						}
					}
				}
			}
			else
			{
				if( usb_stat->connect != 0 )
				{
					NF_POS_DEV_NOTI dev_noti;

					memset(&dev_noti, 0x0, sizeof(NF_POS_DEV_NOTI));

					dev_noti.connect = 0;
					snprintf(dev_noti.dev_str, sizeof(dev_noti.dev_str), "%s", usb_stat->alias);

					ch = search_channel_by_alias(usb_stat->alias);

					memset(usb_stat->physical, 0x0, sizeof(usb_stat->physical));
					memset(usb_stat->alias, 0x0, sizeof(usb_stat->alias));

					usb_stat->connect = 0;
					usb_stat->gen_alias = 0;

					if( ch >= 0 )
						_nf_pos.need_refresh[ch] = 1;

					nf_notify_fire_pointer("pos_dev", &dev_noti, sizeof(NF_POS_DEV_NOTI));
				}
			}
		}

		usleep(50*1000);
	}
}

static void _pos_device(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NF_POS_DEV_NOTI dev_noti;

	g_return_if_fail(pinfo != NULL);

    memcpy(&dev_noti, pinfo->p.ptr, sizeof(NF_POS_DEV_NOTI));

	g_message("%s - conn:%d - dev:%s", __FUNCTION__, dev_noti.connect, dev_noti.dev_str);
}

static int _get_dev_path_by_alias(char *alias, char *buff, int buff_size)
{
	int ret = 0;
	int i;

	if( strcmp( alias, DEV_POS_RS232_STR) == 0 )
	{
		snprintf(buff, buff_size, "%s", DEV_POS_RS232_NAME);
		ret = 1;
	}
	else if( strcmp( alias, DEV_POS_RS485_STR) == 0 )
	{
		snprintf(buff, buff_size, "%s", DEV_POS_RS485_NAME);
		ret = 1;
	}
	else
	{
		for(i=0; i < USB_SERIAL_MAX; i++)
		{
			if(_nf_pos.usb_stat[i].connect == 1)
			{
				if( strcmp( alias, _nf_pos.usb_stat[i].alias) == 0 )
				{
					snprintf(buff, buff_size, "%s", _nf_pos.usb_stat[i].dev_path);
					ret = 1;
					break;
				}
			}
		}
	}

	return ret;
}

static NF_POS_DATA* _pos_get_channel_buffer(int ch)
{
	NF_POS_DATA *ret;

	ret = &_nf_pos_data[ch];

	return ret;
}

static void reset_buffer(int ch)
{
	NF_POS_DATA *pos_data;

	pos_data = _pos_get_channel_buffer(ch);

	memset(pos_data->buffer, 0x0, sizeof(pos_data->buffer));
	pos_data->buffer_len = 0;
}

static void _pos_reset_fd_by_ch(int ch)
{
	if( _nf_pos.fd[ch] >= 0 )
	{
		close(_nf_pos.fd[ch]);
		_nf_pos.fd[ch] = -1;
		g_message("%s - close pos channel:%d", __FUNCTION__, ch);
	}
}

static gboolean _is_pos_test(void)
{
	return pos_test_info.action;
}

static POS_TEST_INFO* _get_pos_test_info(void)
{
	return &pos_test_info;
}

static NF_POS_DB_T* _pos_get_db_by_ch(int ch)
{
	NF_POS_DB_T *ret;

	if( _is_pos_test() == TRUE )
	{
		POS_TEST_INFO* test_info;
		test_info = _get_pos_test_info();

		if(test_info->ch == ch){
			ret = &(test_info->test_db);
			return ret;
		}
	}

	ret = &_nf_pos.pos_db[ch];

	return ret;
}

static void _close_pos_device(int ch)
{
	_pos_reset_fd_by_ch(ch);
	reset_buffer(ch);
}

static void _pos_test_start(void)
{
	int ch;

	start_test_flag = 0;
	pos_test_info.action = 1;

	for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		_close_pos_device(ch);
	}

	_pos_make_parser(pos_test_info.ch);
	refresh_pos_device(pos_test_info.ch);
}

static void _pos_test_end(void)
{
	int ch;

	end_test_flag = 0;
	pos_test_info.action = 0;

	_pos_make_parser(pos_test_info.ch);

	for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		refresh_pos_device(ch);
	}

	pos_test_info.ch = -1;
	pos_test_info.cb_func = NULL;
	memset( &(pos_test_info.test_db), 0x0, sizeof(NF_POS_DB_T) );
}

static void test_func(char *text)
{
	g_message("%s - %s", __FUNCTION__, text);
}

static void refresh_pos_device(int ch)
{
	int fd = -1;
	NF_POS_DB_T *db;
	char dev_path[256];
	int ret;

#if defined(TEST_POS_DEV)
	_nf_pos.fd[ch] = 1;
	return;
#endif

//	g_message( "%s - %d", __FUNCTION__, __LINE__);

	_close_pos_device(ch);

	db = _pos_get_db_by_ch(ch);

	g_message( "%s - CH:%d - find alias:%s", __FUNCTION__, ch, db->port_alias);

	ret = _get_dev_path_by_alias(db->port_alias, dev_path, sizeof(dev_path));

	if( ret )
	{
//		g_message( "%s - CH:%d", __FUNCTION__, ch, );

		if( db->type == POS_TYPE_SERIAL )
		{
			int retry = 0;

			fd = open(dev_path, O_RDONLY);
/*
			while(1)
			{
//				fd = open(dev_name, O_RDWR);
				fd = open(dev_name, O_RDONLY);

				if( fd >= 0 || retry == 8 )
				{
					break;
				}

				retry++;
				g_usleep(50*1000);
			}
*/
			if( fd < 0 )
			{
				g_message( "%s - dev open err - dev_path:%s => %s", __FUNCTION__, dev_path, strerror(errno));
				_nf_pos.fd[ch] = -1;
			}
			else
			{
				if( !strcmp( dev_path, DEV_POS_RS485_NAME) )
				{
					if( pos_baud_init(fd, db->baud) < 0 )
					{
						g_message( "%s - dev config err - dev_path:%s", __FUNCTION__, dev_path);
						close(fd);
						_nf_pos.fd[ch] = -1;
					}
					else
					{
						g_message( "%s - dev config success - dev_path:%s", __FUNCTION__, dev_path);
						_nf_pos.fd[ch] = fd;
						_nf_pos.watchdog_on[ch] = TRUE;
						_nf_pos.watchdog_cnt[ch] = 0;
					}
				}
				else
				{
					termios_act_init(fd, db->baud, db->databit, db->parity, db->stopbit);

					g_message( "%s - dev config success - dev_path:%s", __FUNCTION__, dev_path);
					_nf_pos.fd[ch] = fd;
					_nf_pos.watchdog_on[ch] = TRUE;
					_nf_pos.watchdog_cnt[ch] = 0;
				}
			}
		}
		else
		{
			g_message( "%s - dev type err", __FUNCTION__);
			_nf_pos.fd[ch] = -1;
		}
	}
	else
	{
		g_message( "%s - not found alias", __FUNCTION__);
		_nf_pos.fd[ch] = -1;
	}
}

static CHAR_INFO* _split_char_append(GList **list)
{
	CHAR_INFO *data = g_malloc0(sizeof(CHAR_INFO));
	*list = g_list_append(*list, data);
	return data;
}

static void _pos_free_split_char(GList **list)
{
	GList *plist;
	for(plist = g_list_first(*list); plist != NULL; plist = g_list_next(plist))
	{
		g_free(plist->data);
	}

	g_list_free(*list);

	*list = NULL;
}

static void _get_split_char(char *src, int *num, char *array_char)
{
	int a = 0;
	int idx = 0;
	char *stop;

	char *ptr = src;

	while( 1 )
	{
		a = strtol(ptr, &stop, 16) & 0xff;

		array_char[idx] = a;

		g_message("%s - split idx:%d char:%c", __FUNCTION__, idx, a);


		idx++;

		if(stop)
		{
			if(*stop == '|')
			{
				g_message("%s - split find |", __FUNCTION__);

				ptr = stop + 1;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	*num = idx;
}

static void _pos_make_split_char(char *src, int src_size, GList **list)
{
	char* rbuf;
	char temp[256];;
	char *ptr;
	CHAR_INFO *data;

	memset(temp, 0x0, sizeof(temp));
	memcpy(temp, src, src_size);

	ptr = strtok_r(temp, ",", &rbuf);

	if(strlen(temp) > 0)
	{
		g_message("%s - %d - ptr = %s", __FUNCTION__, __LINE__, ptr);

		data = _split_char_append(list);
		_get_split_char(ptr, &(data->num), data->array_char);

		while(ptr = strtok_r(NULL, ",", &rbuf))
		{
			g_message("%s - %d - ptr = %s", __FUNCTION__, __LINE__, ptr);

			data = _split_char_append(list);
			_get_split_char(ptr, &(data->num), data->array_char);
		}
	}
}

static void _pos_remove_parser(NF_POS_PARSER *parser)
{
	_pos_free_split_char(&(parser->trs_start));
	_pos_free_split_char(&(parser->trs_end));
	_pos_free_split_char(&(parser->eol));
	_pos_free_split_char(&(parser->ignore));
}

static void _pos_make_parser(int ch)
{
	NF_POS_PARSER *parser;
	NF_POS_DB_T *db;

	parser = _pos_get_parser_by_ch(ch);
	db = _pos_get_db_by_ch(ch);

	_pos_remove_parser(parser);

	_pos_make_split_char(db->transact_start, sizeof(db->transact_start), &(parser->trs_start));
	_pos_make_split_char(db->transact_end, sizeof(db->transact_end), &(parser->trs_end));
	_pos_make_split_char(db->endofline, sizeof(db->endofline), &(parser->eol));
	_pos_make_split_char(db->ignore, sizeof(db->ignore), &(parser->ignore));
}

static int _search_idx(char *src, int src_size, CHAR_INFO *search_info)
{
	int max_idx;
	int i;

	max_idx = src_size - search_info->num;

	if( src_size <= 0 )
		return -1;

	if( max_idx < 0 )
		return -1;

	if( search_info->num == 0 )
		return 0;

	for(i=0; i<= max_idx ; i++)
	{
		if(src[i] == search_info->array_char[0])
		{
			if( memcmp(&src[i], search_info->array_char, search_info->num ) == 0 )
			{
				break;
			}
		}
	}

	if( i > max_idx )
	{
		return -1;
	}
	else
	{
		return i;
	}
}

static POS_POSITION _pos_search_idx(GList *delimeter, char *src, int src_size)
{
	CHAR_INFO *char_info;
	POS_POSITION position;

	int idx = 0;
	GList *plist;

	position.start = -1;
	position.end = -1;

	for(plist = delimeter; plist != NULL; plist = g_list_next(plist))
	{
		char_info = plist->data;

		idx = _search_idx(src, src_size, char_info);
		if(idx >= 0)
		{
			if( position.start == -1 )
			{
				position.start = idx;
				position.end = idx + char_info->num;
			}
			else
			{
				if( position.start > idx )
				{
					position.start = idx;
					position.end = idx + char_info->num;
				}
			}
		}
	}

	return position;
}

static void _hex_debug(char *name, char *buff, int buff_size)
{
	int i;
	g_print("%s:\n", name);
	g_print("\nsize[%d] :\n", buff_size);
	for (i=0; i < buff_size; i++) g_print("%c", (unsigned char)buff[i]);

	g_print("\nhex start:\n");

	for (i=0; i < buff_size; i++) {
		g_print("0x%02x ", buff[i] & 0xff);
		if ((i+1)%10 == 0) g_print("\n");
	}
	g_print("\nhex end:\n");
}

static int _pos_put_livelog( GTimeVal *tv, gint type, gint param1, gint param2, const gchar *text )
{
	NF_LOG_DATA log_data;
	GTimeVal	tval;

	if (type != LT_SYSTEM_POS)
	{
		return -1;
	}

	if( text == NULL )
	{
		return -1;
	}

	memset( &log_data, 0x00, sizeof(NF_LOG_DATA));

	if(tv == NULL)
	{
		//g_get_current_time(&tval);
		gettimeofday(&tval, NULL);
		log_data.timestamp = GTIMEVAL_TO_GUINT64(tval);
	}else{
		log_data.timestamp = GTIMEVAL_TO_GUINT64(*tv);
	}

	log_data.type = type;
	log_data.param1 = param1;
	log_data.param2 = param2;
	log_data.text_len = 0;
	log_data.log_id = 0;

	memcpy(log_data.text, text, sizeof(log_data.text));

	nf_notify_fire_pointer("log", &log_data, sizeof(NF_LOG_DATA));

	return 0;
}

static int nf_pos_eventlog_put( GTimeVal *tv, gint ch, const gchar *text )
{
	POS_TEST_INFO* test_info;
	test_info = _get_pos_test_info();

	if( _is_pos_test() == TRUE && test_info->ch == ch)
	{
		NF_POS_LOG_DATA *log_data = text;

		if( strlen(log_data->Text.text) > 0 )
		{
			if(pos_display)
				g_message("%s - %d", __FUNCTION__, __LINE__);

			test_info->cb_func(log_data->Text.text);
		}
	}
	else
	{
		if(pos_display)
			g_message("%s - %d", __FUNCTION__, __LINE__);

		if( nf_filesystem_is_online() == 0 )
		{
			_pos_put_livelog(tv, LT_SYSTEM_POS, ch, 0, text);
		}
		else
		{
			nf_eventlog_put_param(tv, LT_SYSTEM_POS, ch, 0, text);
		}
	}

	return 0;
}

static void _event_log_put(int ch, GTimeVal tv, char *buff, int size, GList *ignore)
{
	gchar text[256];
	int len;
	int idx = 0;
	POS_POSITION ignore_idx;

	memset(text, 0x00, sizeof(text));

	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)text;

	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = ch;
	log_data->pos_log_type = NF_POS_LOG_TYPE_TEXT;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;

	if(pos_display)
		g_message("%s - %d - STR : %s", __FUNCTION__, __LINE__, buff);

	ignore_idx = _pos_search_idx(ignore, buff, size);

	if( ignore_idx.start >= 0 )
	{
		if(pos_display)
			g_message("%s find ignore string", __FUNCTION__);

		return;
	}

	while(1)
	{
		len = strlen( &(buff[idx]) );

		if( len <= 0 )
		{
//			g_message("%s - %d", __FUNCTION__, __LINE__);
			break;
		}
		else if( len < sizeof(log_data->Text.text) )
		{
//			g_message("%s - %d", __FUNCTION__, __LINE__);
			snprintf(log_data->Text.text, sizeof(log_data->Text.text), "%s", &(buff[idx]));
//			nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, text);
			nf_pos_eventlog_put(&tv, ch, text);
			break;
		}
		else
		{
//			g_message("%s - %d", __FUNCTION__, __LINE__);
			snprintf(log_data->Text.text, sizeof(log_data->Text.text), "%s", &(buff[idx]));
			idx += (sizeof(log_data->Text.text) - 1);
//			nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, text);
			nf_pos_eventlog_put(&tv, ch, text);
		}
	}
}

static gchar* _get_sysdb_pos_text(int ch)
{
	char key[128];

	snprintf(key, sizeof(key),"act.pos.P%d.text", ch);

	return nf_sysdb_get_str_nocopy(key);
}

static void _event_poslog_put(int ch, char *buff, int size, GList *ignore)
{
	gchar text[256];
	int len;
	int idx = 0;
	POS_POSITION ignore_idx;
	gchar *p;
	int max_len;
	int i = 0;

	char *pos_text;

	GTimeVal tv;
	gettimeofday(&tv, NULL);

	if(pos_display)
	{
		_hex_debug(__FUNCTION__, buff, size);
	}

	memset(text, 0x00, sizeof(text));

	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)text;

	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = ch;
	log_data->pos_log_type = NF_POS_LOG_TYPE_TEXT;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;

	pos_text = _get_sysdb_pos_text(ch);

	if( pos_text )
	{
		if( strlen(pos_text) > 0 )
		{
			char *loc;

			loc = strstr(buff, pos_text);

			if( loc )
			{
				NF_POS_TEXT_EVENT pos_event;
				memset( &pos_event , 0x0, sizeof(NF_POS_TEXT_EVENT) );

				pos_event.ch = ch;
				pos_event.timestamp = GTIMEVAL_TO_GUINT64(tv);
				snprintf( pos_event.data, sizeof(pos_event.data), "%s", buff);
				nf_notify_fire_pointer("pos_text_event", &pos_event, sizeof(NF_POS_TEXT_EVENT));
				g_message("POS_TEXT FIRE => %s", pos_event.data);
			}
		}
	}

	ignore_idx = _pos_search_idx(ignore, buff, size);

	if( ignore_idx.start >= 0 )
	{
		if(pos_display)
			g_message("%s find ignore string", __FUNCTION__);

		return;
	}

	max_len = sizeof(log_data->Text.text);

	while(1)
	{
		p = &(buff[idx]);
		len = strlen(p);
		memset(log_data->Text.text, 0x0, max_len);

		if( len <= 0 )
		{
//			g_message("%s - %d", __FUNCTION__, __LINE__);
			break;
		}
		else if( len < max_len )
		{
//			g_message("%s - %d - %s", __FUNCTION__, __LINE__, p);
			snprintf(log_data->Text.text, max_len, "%s", p);
//			nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, text);
			nf_pos_eventlog_put(&tv, ch, text);
			break;
		}
		else
		{
//			g_message("%s - %d - %s", __FUNCTION__, __LINE__, p);
			for(i = max_len - 1; i > 0; i--)
			{
				if( (p[i] & 0xff) == 0x20 )
					break;
			}

			if( i == 0 )
			{
//				g_message("%s - %d", __FUNCTION__, __LINE__);
				memcpy(log_data->Text.text, p, max_len - 1);
				idx += (max_len - 1);
				nf_pos_eventlog_put(&tv, ch, text);
			}
			else
			{
//				g_message("%s - %d = i=> %d", __FUNCTION__, __LINE__, i);
				memcpy(log_data->Text.text, p, i);
				idx += (i + 1);
				nf_pos_eventlog_put(&tv, ch, text);
			}
		}
	}
}

gboolean nf_pos_log_put_manual(int ch, char *buff)
{
	GList *ignore = NULL;
	int size;

	if( nf_filesystem_is_online() == 0 )
	{
		return FALSE;
	}

	size = strlen(buff);

	_event_poslog_put(ch, buff, size, ignore);

	return TRUE;
}

static char pos_put_manual_help[] = "pos_put_manual [CH] [TEXT]";
static int pos_put_manual(int argc, char **argv)
{
	int ch;
	char *str;
	gboolean ret;

	if( argc < 3 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	ch = atoi(argv[1]);
	str = argv[2];

	ret = nf_pos_log_put_manual(ch, str);

	if( ret == FALSE )
		g_message("%s - ERROR", __FUNCTION__);

	return 0;
}
__commandlist(pos_put_manual, "pos_put_manual", pos_put_manual_help, pos_put_manual_help);

static void	_pos_push_data(int ch, char *buff, int len, GList *eol, GList *ignore)
{
	char pos_cmd_data[MAX_POS_BUFFER];
	POS_POSITION eol_idx;

	int cur_idx = 0;
	int remain = 0;

	GTimeVal tv;
	gettimeofday(&tv, NULL);

	remain = len;

	if( pos_display)
	{
		_hex_debug(__FUNCTION__, buff, len);
	}

	if( nf_filesystem_is_online() == 0 )
	{
		g_message("%s - filesystem online fail", __FUNCTION__);
		return;
	}

	while(1)
	{
		eol_idx = _pos_search_idx(eol, &(buff[cur_idx]), remain);

		if( pos_display)
		{
			g_message("%s - %d => start_idx = %d / end_idx = %d", __FUNCTION__, __LINE__, eol_idx.start, eol_idx.end);
		}

		if( eol_idx.start == -1 )
		{
			memset(pos_cmd_data, 0x0, sizeof(pos_cmd_data));
			memcpy(pos_cmd_data, &(buff[cur_idx]), remain);
			_event_log_put(ch, tv, pos_cmd_data, remain, ignore);
			break;
		}
		else
		{
			memset(pos_cmd_data, 0x0, sizeof(pos_cmd_data));
			memcpy(pos_cmd_data, &(buff[cur_idx]), eol_idx.start);
			_event_log_put(ch, tv, pos_cmd_data, eol_idx.start, ignore);
			cur_idx += eol_idx.end;
			remain -= eol_idx.end;
		}
	}
}

static NF_POS_PARSER* _pos_get_parser_by_ch(int ch)
{
	return &nf_pos_parser[ch];
}

static void _pos_data_parse_command(int ch, char *rbuf, int rbuf_len)
{
	NF_POS_DATA *pos_data;
	NF_POS_DB_T* db;
	char pos_cmd_data[MAX_POS_BUFFER];

	int i;

	pos_data = _pos_get_channel_buffer(ch);

	if (pos_data->buffer_len + rbuf_len >= sizeof(pos_data->buffer)) {
		g_message("%s - buffer overflow", __FUNCTION__);
		pos_data->buffer_len = 0;
	}

	memcpy(&(pos_data->buffer[pos_data->buffer_len]), rbuf, rbuf_len);
	pos_data->buffer_len += rbuf_len;

	db = _pos_get_db_by_ch(ch);

	if(pos_display)
	{
		_hex_debug(__FUNCTION__, pos_data->buffer, pos_data->buffer_len);
	}

	if( db->protocol == 0 )
	{
		NF_POS_PARSER *parser;
		CHAR_INFO *char_info;
		GList *plist;
		POS_POSITION start_idx;
		POS_POSITION end_idx;

		POS_POSITION eol_idx;

		int idx = -1;
		int len = 0;

		parser = _pos_get_parser_by_ch(ch);

loop:

		if(parser->trs_start == NULL)
		{
			eol_idx = _pos_search_idx(parser->eol, pos_data->buffer, pos_data->buffer_len);

			if( eol_idx.start == 0 )
			{
				len = pos_data->buffer_len - end_idx.end;
				memcpy(pos_data->buffer, &(pos_data->buffer[eol_idx.end]), len);
				pos_data->buffer_len = len;
				goto loop;
			}
			else if( eol_idx.start > 0 )
			{
				len = eol_idx.start;
				memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);
				memcpy(pos_cmd_data, pos_data->buffer, len);
				_event_poslog_put(ch, pos_cmd_data, len, parser->ignore);

				len = pos_data->buffer_len - eol_idx.end;
				memcpy(pos_data->buffer, &(pos_data->buffer[eol_idx.end]), len);
				pos_data->buffer_len = len;
				goto loop;
			}
		}
		else
		{
			start_idx = _pos_search_idx(parser->trs_start, pos_data->buffer, pos_data->buffer_len);

			if( start_idx.start > 0 )
			{
				len = pos_data->buffer_len - start_idx.start;
				memcpy(pos_data->buffer, &(pos_data->buffer[start_idx.start]), len);

				pos_data->buffer_len = len;
				goto loop;
			}
			else if( start_idx.start == 0 )
			{
				char *trs_buff = &(pos_data->buffer[start_idx.end]);
				int trs_size = pos_data->buffer_len - start_idx.end;

				end_idx = _pos_search_idx(parser->trs_end, trs_buff, trs_size);
				eol_idx = _pos_search_idx(parser->eol, trs_buff, trs_size);

				if( end_idx.start == 0 )
				{
					trs_size -= end_idx.end;
					pos_data->buffer_len = trs_size;

					memcpy(pos_data->buffer, &(trs_buff[end_idx.end]), trs_size);
					goto loop;
				}
				else if( end_idx.start < 0 )
				{
					if( eol_idx.start == 0 )
					{
						trs_size -= eol_idx.end;
						pos_data->buffer_len -= eol_idx.end;

						memcpy(trs_buff, &(trs_buff[eol_idx.end]), trs_size);
						goto loop;
					}
					else if( eol_idx.start > 0 )
					{
						memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);
						memcpy(pos_cmd_data, trs_buff, eol_idx.start);
						_event_poslog_put(ch, pos_cmd_data, eol_idx.start,parser->ignore);

						trs_size -= eol_idx.end;
						pos_data->buffer_len -= eol_idx.end;

						memcpy(trs_buff, &(trs_buff[eol_idx.end]), trs_size);
						goto loop;
					}
				}
				else
				{
					if( eol_idx.start == 0 )
					{
						trs_size -= eol_idx.end;
						pos_data->buffer_len -= eol_idx.end;

						memcpy(trs_buff, &(trs_buff[eol_idx.end]), trs_size);
						goto loop;
					}
					else if( eol_idx.start < 0 )
					{
						memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);
						memcpy(pos_cmd_data, trs_buff, end_idx.start);
						_event_poslog_put(ch, pos_cmd_data, end_idx.start, parser->ignore);

						trs_size -= end_idx.end;
						pos_data->buffer_len = trs_size;

						memcpy(pos_data->buffer, &(trs_buff[end_idx.end]), trs_size);
						goto loop;
					}
					else
					{
						if( end_idx.start <= eol_idx.start )
						{
							memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);
							memcpy(pos_cmd_data, trs_buff, end_idx.start);
							_event_poslog_put(ch, pos_cmd_data, end_idx.start, parser->ignore);

							trs_size -= end_idx.end;
							pos_data->buffer_len = trs_size;

							memcpy(pos_data->buffer, &(trs_buff[end_idx.end]), trs_size);
							goto loop;
						}
						else
						{
							memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);
							memcpy(pos_cmd_data, trs_buff, eol_idx.start);
							_event_poslog_put(ch, pos_cmd_data, eol_idx.start, parser->ignore);

							trs_size -= eol_idx.end;
							pos_data->buffer_len -= eol_idx.end;

							memcpy(trs_buff, &(trs_buff[eol_idx.end]), trs_size);
							goto loop;
						}
					}
				}
			}
		}
	}
	else if( db->protocol == 1 )
	{
		process_epson_protocol(ch, pos_data->buffer, &(pos_data->buffer_len));
	}
}

static gboolean _display_pos_data_len(gpointer data)
{
	int ch = 0;

	g_print("\n================ POS recv len ====================\n");

	for(ch=0; ch < NUM_ACTIVE_CH; ch++)
	{
		if( ch == 8 )
			g_print("\n");

		g_print("CH%d : %u, ", ch, pos_recv[ch]);
		pos_recv[ch] = 0;
	}

	g_print("\n==================================================\n\n");

	return TRUE;
}

static gboolean _pos_watchdog_func(gpointer data)
{
	NF_POS_DB_T *db;
	gboolean status;
	int ch = 0;

	for(ch=0; ch < NUM_ACTIVE_CH; ch++)
	{
		db = _pos_get_db_by_ch(ch);
		status = nf_pos_get_port_status(ch);

		if( db->act && status )
		{
			if( _nf_pos.watchdog_on[ch] == TRUE && _is_pos_test() == FALSE )
			{
				if( _nf_pos.watchdog_cnt[ch] > 60 )
				{
					_nf_pos.watchdog_cnt[ch] = 0;
					_nf_pos.need_refresh[ch] = 1;

					g_message("%s => RESET CH:%d", __FUNCTION__, ch);
				}
				else
				{
					_nf_pos.watchdog_cnt[ch] = _nf_pos.watchdog_cnt[ch] + 1;
				}
			}
		}
	}

	return TRUE;
}

#if 0
static void _pos_data_parse_command(int ch, char *rbuf, int rbuf_len)
{
	NF_POS_DATA *pos_data;
	NF_POS_DB_T* db;
	char pos_cmd_data[MAX_POS_BUFFER];

	int i;

	pos_data = _pos_get_channel_buffer(ch);

	if (pos_data->buffer_len + rbuf_len >= sizeof(pos_data->buffer)) {
		g_message("%s - buffer overflow", __FUNCTION__);
		pos_data->buffer_len = 0;
	}

	memcpy(&(pos_data->buffer[pos_data->buffer_len]), rbuf, rbuf_len);
	pos_data->buffer_len += rbuf_len;

	if(pos_display)
	{
		g_print("\n_pos_data_parse_command start:\n");

		for (i=0; i < pos_data->buffer_len; i++) {
			g_print("0x%02x ", pos_data->buffer[i]);
			if ((i+1)%10 == 0) g_print("\n");
		}
		g_print("\n_pos_data_parse_command end:\n");
	}

	db = _pos_get_db_by_ch(ch);

	if( db->protocol == 0 )
	{
		NF_POS_PARSER *parser;
		CHAR_INFO *char_info;
		GList *plist;
		POS_POSITION start_idx;
		POS_POSITION end_idx;
		int idx = -1;

		parser = _pos_get_parser_by_ch(ch);

loop:
		start_idx = _pos_search_idx(parser->trs_start, pos_data->buffer, pos_data->buffer_len);
		end_idx = _pos_search_idx(parser->trs_end, pos_data->buffer, pos_data->buffer_len);

		if(pos_display)
		{
			g_message("%s - %d => start_idx = %d / end_idx = %d", __FUNCTION__, __LINE__, start_idx.start, end_idx.start);
		}

		if(start_idx.start >= 0 && end_idx.start >= 0)
		{
			if(start_idx.start > end_idx.start)
			{
				int len = pos_data->buffer_len - start_idx.start;

				memcpy(pos_data->buffer, &(pos_data->buffer[start_idx.start]), len);
				pos_data->buffer_len = len;
			}
			else
			{
				int len = end_idx.start - start_idx.end;

				memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);
				memcpy(pos_cmd_data, &(pos_data->buffer[start_idx.end]), len);
				_pos_push_data(ch, pos_cmd_data, len, parser->eol, parser->ignore);

				len = pos_data->buffer_len - end_idx.end;

				memcpy(pos_data->buffer, &(pos_data->buffer[end_idx.end]), len);
				pos_data->buffer_len = len;

				goto loop;
			}
		}
	}
}
#endif

static void _pos_receive_data(void *arg)
{
	static int fd = -1;
	int ret = 0, get_baudrate = 0;
	static guint baudrate = 0;

	struct timeval tv;
	fd_set rxset;
	int mx, n;
	int ch;

	int change_flag;

	void *q_data;

	guchar buf[MAX_POS_BUFFER];

#if defined(TEST_POS_DEV)
	POS_DATA pos_data[13];
#endif

	while(1)
	{
		change_flag = 0;

		q_data = g_async_queue_try_pop( _nf_pos.pos_db_queue );
		if(q_data)
		{
			NF_POS_DB_T *pos_db = (NF_POS_DB_T *)q_data;

			for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
			{
				if( memcmp(&_nf_pos.pos_db[ch], &pos_db[ch], sizeof(NF_POS_DB_T)) )
				{
		    	    memcpy(&_nf_pos.pos_db[ch], &pos_db[ch], sizeof(NF_POS_DB_T));

					_pos_make_parser(ch);
					_nf_pos.need_refresh[ch] = 1;
				}
			}

    	    g_free(q_data);
		}

		if( start_test_flag )
		{
			g_message( "%s - test func start", __FUNCTION__);
			_pos_test_start();
		}

		if( end_test_flag )
		{
			g_message( "%s - test func end", __FUNCTION__);
			_pos_test_end();

			change_flag = 1;
		}

		if( _is_pos_test() == FALSE )
		{
			for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
			{
/*
				if( _is_pos_test() == TRUE )
				{
					POS_TEST_INFO* test_info;
					test_info = _get_pos_test_info();

					if(test_info->ch == ch)
						continue;
				}
*/
				if(_nf_pos.need_refresh[ch])
				{
					_nf_pos.need_refresh[ch] = 0;
					refresh_pos_device(ch);
					change_flag = 1;
				}
			}
		}

		if(change_flag)
		{
			nf_notify_fire_params("sys_pos_changed", 0, 0, 0, 0);
		}

		FD_ZERO(&rxset);
		mx = -1;

		for(ch = 0 ; ch < NUM_ACTIVE_CH; ch++)
		{
			fd = _nf_pos.fd[ch];

			if( fd >= 0 )
			{
				FD_SET(fd, &rxset);

				if( mx < fd)
					mx = fd;
			}
		}

		if( mx >= 0 )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 0;

#if defined(TEST_POS_DEV)
			n = 1;
#else
			n = Select(mx+1 , &rxset, 0, 0, &tv);
#endif
			if( n > 0 )
			{
				for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
				{
					fd = _nf_pos.fd[ch];

					if( fd >= 0)
					{
#if defined(TEST_POS_DEV)
						if(ch_timer[ch] <= 0)
						{
							if( _nf_pos.pos_db[ch].act && nf_filesystem_is_online() )
							{
								n = _random_buf(pos_data);
								_pos_event_put(ch, n, pos_data);
							}

							ch_timer[ch] = (pos_rand() % 30)+ 10;
						}
						else
						{
							ch_timer[ch]--;
						}
#else
						if (FD_ISSET(fd, &rxset))
						{
							ret = read(fd, buf, sizeof(buf));

							if(ret > 0)
							{
								if (pos_display) {
									_hex_debug(__FUNCTION__, buf, ret);
								}

								// workaround POS-device , To Reset
								if( _nf_pos.watchdog_on[ch] == TRUE && ret > 3 )
								{
									_nf_pos.watchdog_on[ch] = FALSE;
									_nf_pos.watchdog_cnt[ch] = 0;
								}

								NF_POS_DB_T *db = _pos_get_db_by_ch(ch);

								if( db->act )
								{
									pos_recv[ch] += ret;
									_pos_data_parse_command(ch, buf, ret);
								}

							}
						}
#endif
					}
				}
			}
		}

		g_usleep(100 * 1000);
	}
}

static void _init_pos_data(void)
{
	int i, ch;

	memset(&_nf_pos, 0x0, sizeof(NF_POS_CONF));

	memset(&pos_test_info, 0x0, sizeof(POS_TEST_INFO));

	memset(nf_pos_parser, 0x0, sizeof(NF_POS_PARSER) * 16);

	_load_pos_db(_nf_pos.pos_db);

	for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		_pos_make_parser(ch);
	}

	_nf_pos.pos_db_queue = g_async_queue_new();

	for(i=0; i<USB_SERIAL_MAX; i++)
	{
		snprintf(_nf_pos.usb_stat[i].dev_path, sizeof(_nf_pos.usb_stat[i].dev_path), "/dev/ttyUSB%d", i);
		snprintf(_nf_pos.usb_stat[i].sys_path, sizeof(_nf_pos.usb_stat[i].sys_path), "/sys/bus/usb-serial/devices/ttyUSB%d", i);
	}

	for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
#if defined(TEST_POS_DEV)
		_nf_pos.fd[ch] = 1;
#else
		_nf_pos.fd[ch] = -1;
#endif
	}

	for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		refresh_pos_device(ch);
	}
}

gboolean nf_pos_init(void)
{
    pthread_t tid = 0;

	_init_pos_data();

    if ( pthread_create(&tid, NULL, (void*)_usb_serial_monitor, NULL) != 0)
    {
        return 0;
    }

    pthread_detach(tid);

    if ( pthread_create(&tid, NULL, (void*)_pos_receive_data, NULL) != 0)
    {
        return 0;
    }

    pthread_detach(tid);

	nf_notify_connect_cb("sysdb_change", _pos_sysdb_change, NULL);

	// for test
//	nf_notify_connect_cb("pos_dev", _pos_device, NULL);

	nf_timer_add( 1000 * 10, _display_pos_data_len, NULL);
	nf_timer_add( 1000, _pos_watchdog_func, NULL);

    return 1;
}

gboolean nf_pos_is_485_use(void)
{
	NF_POS_DB_T		pos_db[NUM_ACTIVE_CH];
	int i;

	_load_pos_db(pos_db);

	for( i=0; i<NUM_ACTIVE_CH; i++ )
	{
		if( pos_db[i].type == POS_TYPE_SERIAL )
		{
			if( !strcmp(pos_db[i].port_alias, DEV_POS_RS485_STR) )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

gboolean nf_pos_test_start(int ch, NF_POS_DB_T* db, cb_pos_test_fxn_t cb_func)
{
	int i;
	NF_POS_DB_T* ori_db;

	if( pos_test_info.action )
		return FALSE;

/*
	if( strcmp(db->port_alias, "NONE") )
	{
		for(i=0; i<NUM_CHANNEL ; i++)
		{
			if( i == ch )
				continue;
			else
			{
				ori_db = _pos_get_db_by_ch(i);

				if( !strcmp(ori_db->port_alias, db->port_alias) )
					return FALSE;
			}
		}
	}
*/
	start_test_flag = 1;

	pos_test_info.ch = ch;
	pos_test_info.cb_func = cb_func;
	memcpy( &(pos_test_info.test_db), db, sizeof(NF_POS_DB_T) );

	return TRUE;
}

gboolean nf_pos_test_end()
{
	if( pos_test_info.action == 0 )
		return FALSE;

	end_test_flag = 1;

	return TRUE;
}

gboolean nf_pos_get_serial_option(char *name, NF_POS_SERIAL_OPTION *option)
{

	gboolean baudrate;
	gboolean databit;
	gboolean parity;
	gboolean stopbit;

	if( !strcmp( name, DEV_POS_RS485_STR) )
	{
		option->baudrate = 1;
		option->databit = 0;
		option->parity = 0;
		option->stopbit = 0;
	}
	else
	{
		option->baudrate = 1;
		option->databit = 1;
		option->parity = 1;
		option->stopbit = 1;
	}

	return TRUE;
}

gboolean nf_pos_get_parse_option(char *protocol, NF_POS_PARSE_OPTION *option)
{
	gboolean ret = FALSE;

	if( !strcmp(protocol,"EPSON") )
	{
		option->transaction_start = 0;
		option->transaction_end = 0;
		option->line_delimiter = 0;
		option->ignore_string = 0;

		ret = TRUE;
	}
	else // ( !strcmp(protocol,"GENERAL") )
	{
		option->transaction_start = 1;
		option->transaction_end = 1;
		option->line_delimiter = 1;
		option->ignore_string = 1;

		ret = TRUE;
	}

	return ret;
}

gboolean nf_pos_get_serial_info(NF_POS_SERIAL_INFO *info, int *num)
{
	int i;
	int count=0;

	*num = 0;
	memset(info, 0x0, sizeof(NF_POS_SERIAL_INFO) * NF_POS_SERIAL_MAX);

	snprintf(info[count].name, sizeof(info[count].name), "NONE");
	count++;

#if defined(_IPX_1648P) || defined(_IPX_0824P) || defined(_SNF_0824) || defined(_SNF_0424) \
	||defined(_IPX_1648VE3)||defined(_IPX_0824VE3)||defined(_IPX_0412VE3) \
	||defined(_IPX_0824P3)||defined(_IPX_1648P3)||defined(_IPX_0824P3ECO)||defined(_IPX_1648P3ECO) \
	||defined(_IPX_0412P4) || defined(_IPX_0824P4)||defined(_IPX_1648P4) \
	||defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_0824P4E) \
	||defined(_IPX_0412M4E) || defined(_IPX_0824M4E)
	snprintf(info[count].name, sizeof(info[count].name), DEV_POS_RS232_STR);
	count++;
#endif
/*
	snprintf(info[count].name, sizeof(info[count].name), DEV_POS_RS485_STR);
	count++;
*/

	for(i=0; i<USB_SERIAL_MAX; i++)
	{
		if( _nf_pos.usb_stat[i].connect == 1 )
		{
			snprintf(info[count].name, sizeof(info[count].name), "%s", _nf_pos.usb_stat[i].alias);
			count++;
		}
	}

	*num = count;

	return TRUE;
}

gboolean nf_pos_get_port_status(int ch)
{
	gboolean ret;

	if( _nf_pos.fd[ch] >= 0 )
		ret = TRUE;
	else
		ret = FALSE;

	if(pos_display)
		g_message("%s - CH:%d => RET:%d", __FUNCTION__, ch, ret);

	return ret;
}
/*
gboolean nf_pos_get_port_status(int ch)
{
	NF_POS_DB_T *db;
	NF_POS_USB_STAT *usb_stat;
	int i;

	db = _pos_get_db_by_ch(ch);

	for(i = 0; i < USB_SERIAL_MAX; i++)
	{
		usb_stat = _pos_get_usb_stat_data_by_idx(i);

		if( !strcmp(db->port_alias, usb_stat->alias) )
			break;
	}

	if( ch == USB_SERIAL_MAX )
		return FALSE;
	else
		return TRUE;
}
*/

int nf_pos_get_protocol_cnt(void)
{
//	return 1;
	return 2;
}

gboolean nf_pos_get_protocol_list(char **protocol)
{
	int i = 0;

	snprintf(protocol[i++], MAX_POS_PROTO_STR_LENGTH, "%s", "GENERAL");
	snprintf(protocol[i++], MAX_POS_PROTO_STR_LENGTH, "%s", "EPSON");

	return TRUE;
}

int nf_pos_get_charset_cnt(void)
{
	return 1;
}

gboolean nf_pos_get_charset_list(char **charset)
{
	int i = 0;

	snprintf(charset[i++], MAX_POS_CHARSET_STR_LENGTH, "%s", "ASCII");

	return TRUE;
}


enum {
	FD = 0,
	USB,
};

static char pos_parser_display_help[] = "pos_parser_display [CH]";
static int pos_parser_display(int argc, char **argv)
{
	int ch = 0;
	int i=0;

	int num = 0;
	NF_POS_PARSER *parser;

	if( argc < 2 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	ch = atoi(argv[1]);

	g_message("%s - parser -CH:%d", __FUNCTION__, ch);

	parser = &nf_pos_parser[ch];

	GList *plist;

	num = g_list_length(parser->trs_start);

	if(num > 0 )
	{
		for(plist = g_list_first(parser->trs_start); plist != NULL; plist = g_list_next(plist))
		{
			CHAR_INFO* char_info =  plist->data;

			if( char_info->num > 0 )
			{
				for(i=0; i < char_info->num ; i++)
				{
					g_message("%s - IDX:%d < %c >", __FUNCTION__, i, char_info->array_char[i]);


				}
			}
		}
	}

	return 0;
}
__commandlist(pos_parser_display, "pos_parser_display", pos_parser_display_help, pos_parser_display_help);

enum {
	TRS_START = 0,
	TRS_END,
	EOL,
	IGNORE,

};

static char pos_parser_set_help[] = "pos_parser_set [CH] [type] [str]";
static int pos_parser_set(int argc, char **argv)
{
	int ch = 0;
	int i=0;
	int type;

	int num = 0;
	NF_POS_PARSER *parser;

	char *str;

	char key[128];


	if( argc < 4 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	ch = atoi(argv[1]);

	type = atoi(argv[2]);

	str = argv[3];

	g_message("%s - parser SET:%d", __FUNCTION__, ch);

	if(type == TRS_START)
		sprintf(key, "sys.pos.P%d.transact_start", ch);
	else if(type == TRS_END)
		sprintf(key, "sys.pos.P%d.transact_end", ch);
	else if(type == EOL)
		sprintf(key, "sys.pos.P%d.endofline", ch);
	else
		sprintf(key, "sys.pos.P%d.ignore", ch);

	nf_sysdb_set_str(key, str);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	return 0;
}
__commandlist(pos_parser_set, "pos_parser_set", pos_parser_set_help, pos_parser_set_help);

static char pos_test_start_help[] = "pos_test_start [CH] [PORT]";
static int pos_test_start(int argc, char **argv)
{
	NF_POS_DB_T		pos_db[NUM_ACTIVE_CH];
	NF_POS_DB_T		tmp;
	int ch;
	char *port;
	gboolean ret;


	ch = atoi(argv[1]);

	_load_pos_db(pos_db);

//	port = argv[2];

	memcpy(&tmp, &(pos_db[ch]), sizeof(NF_POS_DB_T));

//	snprintf(tmp.port_alias, sizeof(tmp.port_alias), "%s", port);

	ret = nf_pos_test_start(ch, &tmp, test_func);

	g_message("%s - RET = %d", __FUNCTION__, ret);

	return 0;
}
__commandlist(pos_test_start, "pos_test_start", pos_test_start_help, pos_test_start_help);

static char pos_test_end_help[] = "pos_test_end";
static int pos_test_end(int argc, char **argv)
{
	gboolean ret;

	ret = nf_pos_test_end();

	g_message("%s - RET = %d", __FUNCTION__, ret);

	return 0;
}
__commandlist(pos_test_end, "pos_test_end", pos_test_end_help, pos_test_end_help);

static char pos_stat_info_help[] = "pos_stat_info [0]";
static int pos_stat_info(int argc, char **argv)
{
	int i;
	int type;

	if( argc < 2 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	type = atoi(argv[1]);

	if( type == FD )
	{
		for(i=0; i<NUM_ACTIVE_CH; i++)
		{
			g_message("%s - CH:%d => %d", __FUNCTION__, i, _nf_pos.fd[i]);
		}
	}

	return 0;
}
__commandlist(pos_stat_info, "pos_stat_info", pos_stat_info_help, pos_stat_info_help);

static char pos_dev_info_help[] = "pos_dev_info";
static int pos_dev_info(int argc, char **argv)
{
	int i;
	gboolean ret;
	int count;
	NF_POS_SERIAL_INFO	info[NF_POS_SERIAL_MAX];

	ret = nf_pos_get_serial_info(info, &count);

	g_message("%s - COUNT:%d", __FUNCTION__, count);

	for(i=0; i<count; i++)
	{
		g_message("%s - %s", __FUNCTION__, info[i].name);
	}
	return 0;
}
__commandlist(pos_dev_info, "pos_dev_info", pos_dev_info_help, pos_dev_info_help);

enum {
	ENABLE = 0,
	PORT,
	BAUD,
	DATABIT,
	PARITY,
	STOPBIT
};

static char pos_change_db_help[] = "pos_change_db [0-5] [0000000000000000]";
static int pos_change_db(int argc, char **argv)
{
	int i;
	int num = 0;
	gchar enable_ch[20];
	gchar key[128];
	int type;
	int ch;


	if( argc < 3 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	type = atoi(argv[1]);

	if( type == ENABLE )
	{
		snprintf(enable_ch, sizeof(enable_ch), "%s", argv[2]);
		num = strlen(enable_ch);
		if( num != NUM_ACTIVE_CH )
		{
			g_message("%s - ARGV NUM Fail", __FUNCTION__);
			return 0;
		}

		g_message("%s - enable_ch : %s", __FUNCTION__, enable_ch);

		for(i=0; i<NUM_ACTIVE_CH;i++)
		{
			sprintf(key, "sys.pos.P%d.enable", i);

			if( (enable_ch[i] & 0xff) == '0' )
			{
				nf_sysdb_set_uint(key, 0);
			}
			else if( (enable_ch[i] & 0xff) == '1' )
			{
				nf_sysdb_set_uint(key, 1);
			}
		}
	}
	else if( type == PORT )
	{
		char *port;

		ch = atoi(argv[2]);
		port = argv[3];

		g_message("%s - port change : %d => %s", __FUNCTION__, ch, port);

		sprintf(key, "sys.pos.P%d.port", ch);

		nf_sysdb_set_str(key, port);
	}
	else if( type = BAUD )
	{
		gint baud;

		ch = atoi(argv[2]);
		baud = atoi(argv[3]);

		g_message("%s - buad change : %d => %d", __FUNCTION__, ch, baud);

		sprintf(key, "sys.pos.P%d.baud", ch);

		nf_sysdb_set_uint(key, baud);
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	return 0;
}
__commandlist(pos_change_db, "pos_change_db", pos_change_db_help, pos_change_db_help);

static char pos_display_onoff_help[] = "pos_display [0:1]";
static int pos_display_onoff(int argc, char **argv)
{
	int display = 0;

	if( argc < 2 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	display = atoi(argv[1]);

	g_message("%s - DISPLAY to %d", __FUNCTION__, display);

	if(display == 1)
	{
		pos_display = 1;
	}
	else
	{
		pos_display = 0;
	}

	return 0;
}
__commandlist(pos_display_onoff, "pos_display_onoff", pos_display_onoff_help, pos_display_onoff_help);


typedef struct _POS_PROTO_ESCP {
	gchar	cmd[10];
	gint	cmd_len;
	gint	data_len;
} POS_PROTO_ESCP;

static POS_PROTO_ESCP pos_gs_cmd[] =
{
	{ { 0x2A,                   }, 1, -1},
	{ { 0x2F,                   }, 1, 3 },
	{ { 0x42,                   }, 1, 3 },
	{ { 0x48,                   }, 1, 3 },
	{ { 0x56,                   }, 1, -1},
	{ { 0x61,                   }, 1, 3 },
	{ { 0x68,                   }, 1, 3 },
	{ { 0x6B,                   }, 1, 0 },
	{ { 0x77,                   }, 1, 3 }
};

static int pos_gs_cmd_count = sizeof(pos_gs_cmd)/sizeof(POS_PROTO_ESCP);

static POS_PROTO_ESCP pos_escp_cmd[] =
{
	{ { 0x0E,                   }, 1, 2 },
	{ { 0x0F,                   }, 1, 2 },
//  ESC/POS (Disable Double Width mode)			// New feature
	{ { 0x14,                   }, 1, 2 },
	{ { 0x23,                   }, 1, 2 },
//	{ { 0x30,                   }, 1, 2 },
//  ESC/POS (Setting Bluetooth parameter)
	{ { 0x30,                   }, 1, -1},
	{ { 0x31,                   }, 1, 2 },
	{ { 0x32,                   }, 1, 2 },
	{ { 0x34,                   }, 1, 2 },
	{ { 0x35,                   }, 1, 2 },
	{ { 0x36,                   }, 1, 2 },
//	{ { 0x37,                   }, 1, 2 },
//  ESC/POS (Setting Control Parameter Command)		// New feature
	{ { 0x37,                   }, 1, 5 },
//	{ { 0x38,                   }, 1, 2 },
//  ESC/POS (Sleep parameter)					// New feature
	{ { 0x38,                   }, 1, 3 },
	{ { 0x39,                   }, 1, 2 },
	{ { 0x3C,                   }, 1, 2 },
	{ { 0x3D,                   }, 1, 2 },
	{ { 0x3E,                   }, 1, 2 },
	{ { 0x40,                   }, 1, 2 },
//	{ { 0x45,                   }, 1, 2 },
//  ESC/POS (Turn emphasized mode on/off)
	{ { 0x45,                   }, 1, 3 },
	{ { 0x46,                   }, 1, 2 },
//	{ { 0x47,                   }, 1, 2 },
//  ESC/POS (Turn double-strike mode on/off)
	{ { 0x47,                   }, 1, 3 },
	{ { 0x48,                   }, 1, 2 },
//	{ { 0x4D,                   }, 1, 2 },
//	ESC/POS (Select character font)
	{ { 0x4D,                   }, 1, 3 },
	{ { 0x4F,                   }, 1, 2 },
	{ { 0x50,                   }, 1, 2 },
	{ { 0x54,                   }, 1, 2 },
	{ { 0x67,                   }, 1, 2 },
	{ { 0x19,                   }, 1, 3 },
	{ { 0x20,                   }, 1, 3 },
	{ { 0x21,                   }, 1, 3 },
	{ { 0x25,                   }, 1, 3 },
	{ { 0x2B,                   }, 1, 3 },
	{ { 0x2D,                   }, 1, 3 },
	{ { 0x2F,                   }, 1, 3 },
	{ { 0x33,                   }, 1, 3 },
	{ { 0x41,                   }, 1, 3 },
	{ { 0x49,                   }, 1, 3 },
	{ { 0x4A,                   }, 1, 3 },
	{ { 0x4E,                   }, 1, 3 },
	{ { 0x51,                   }, 1, 3 },
	{ { 0x52,                   }, 1, 3 },
	{ { 0x53,                   }, 1, 3 },
	{ { 0x55,                   }, 1, 3 },
	{ { 0x57,                   }, 1, 3 },
	{ { 0x61,                   }, 1, 3 },
//	ESC/POS (Print and feed n lines)
	{ { 0x64,                   }, 1, 3 },
// 	{ { 0x69,                   }, 1, 3 },
//	ESC/POS (Cutting Paper ???)
	{ { 0x69,                   }, 1, 2 },
	{ { 0x6A,                   }, 1, 3 },
	{ { 0x6B,                   }, 1, 3 },
	{ { 0x6C,                   }, 1, 3 },
	{ { 0x6D,                   }, 1, 3 },
//	{ { 0x70,                   }, 1, 3 },
//	ESC/POS (Generate pulse)
	{ { 0x70,                   }, 1, 5 },
	{ { 0x71,                   }, 1, 3 },
	{ { 0x72,                   }, 1, 3 },
	{ { 0x73,                   }, 1, 3 },
	{ { 0x74,                   }, 1, 3 },
//	ESC/POS (Transmit peripheral devices status)	// New feature
	{ { 0x75,                   }, 1, 2 },
//  ESC/POS (Transmit paper sensor status)		// New feature
	{ { 0x76,                   }, 1, 3 },
	{ { 0x77,                   }, 1, 3 },
	{ { 0x78,                   }, 1, 3 },
//	ESC/POS (Set/Cancel Character Updown mode)	// New feature
	{ { 0x7B,                   }, 1, 3 },
	{ { 0x24,                   }, 1, 4 },
	{ { 0x3F,                   }, 1, 4 },
	{ { 0x5C,                   }, 1, 4 },
//	ESC/POS (Select paper sensor(s) to output paper-end signal)
//	{ { 0x63, 0x33              }, 2, 4 },
	{ { 0x63,                   }, 1, 4 },
//	{ { 0x65,                   }, 1, 4 },
//	ESC/POS (Print and reverse feed n lines)
	{ { 0x65,                   }, 1, 3 },
	{ { 0x66,                   }, 1, 4 },
	{ { 0x58,                   }, 1, 5 },
//	{ { 0x42,                   }, 1, 0 },
//	ESC/POS (Set left blank char nums)			// New feature
	{ { 0x42,                   }, 1, 3 },
	{ { 0x44,                   }, 1, 0 },
	{ { 0x62,                   }, 1, 0 },
	{ { 0x2A,                   }, 1, -1},
	{ { 0x4B,                   }, 1, -1},
	{ { 0x4C,                   }, 1, -1},
	{ { 0x59,                   }, 1, -1},
	{ { 0x5A,                   }, 1, -1},
	{ { 0x5E,                   }, 1, -1},
	{ { 0x26, 0x00,             }, 2, -1},
//	ESC/POS (Enable/Disable User-defined Characters)	// New feature
	{ { 0x26,                   }, 1, -1},
	{ { 0x2E, 0x02,             }, 2, 8 },
	{ { 0x2E,                   }, 1, -1},
	{ { 0x3A, 0x00,             }, 2, 5 },
	{ { 0x43, 0x00,             }, 2, 4 },
	{ { 0x43,                   }, 1, 3 },
	{ { 0x28, 0x2D,             }, 2, 8 },
	{ { 0x28, 0x43,             }, 2, 7 },
	{ { 0x28, 0x42,             }, 2, 12},
	{ { 0x28, 0x47,             }, 2, 6 },
	{ { 0x28, 0x55,             }, 2, 6 },
	{ { 0x28, 0x56,             }, 2, 7 },
	{ { 0x28, 0x5E,             }, 2, -1},
	{ { 0x28, 0x63,             }, 2, 9 },
	{ { 0x28, 0x74,             }, 2, 8 },
	{ { 0x28, 0x76,             }, 2, 3 },
	{ { 0x28, 0x69, 0x01, 0x00, }, 4, 6 }
};

static int pos_escp_cmd_count = sizeof(pos_escp_cmd)/sizeof(POS_PROTO_ESCP);

static void _debug_escp_cmd(char *cmd, int cmd_len)
{
	int i;

	if (pos_display) {
		printf("POS CMD => [0x1b]");

		for(i=0; i < cmd_len; i++)
		{
			printf("[0x%02x]", cmd[i] & 0xff);
		}

		printf("\n");
	}
}

static void _debug_gs_cmd(char *cmd, int cmd_len)
{
	int i;

	if (pos_display) {
		printf("POS CMD => [0x1d]");

		for(i=0; i < cmd_len; i++)
		{
			printf("[0x%02x]", cmd[i] & 0xff);
		}

		printf("\n");
	}
}

static int _find_gs_cmd(char *start, int remain_size, int *data_len)
{
	char *desc = NULL;
	int i = 0;

	if (pos_display) {
		g_message("%s - start=0x%x, remain_size=%d", __FUNCTION__, start, remain_size);
	}

	for(i=0; i<pos_gs_cmd_count; i++)
	{
		if( remain_size < pos_gs_cmd[i].cmd_len )
		{

			if (pos_display) {
	//			g_message("start=0x%x, remain_size=%dm, ", start, remain_size, );
				g_message("%s - %d", __FUNCTION__, __LINE__);
			}
			return -1;
		}

		if( memcmp( start, pos_gs_cmd[i].cmd, pos_gs_cmd[i].cmd_len ) == 0 )
		{
			_debug_gs_cmd(pos_gs_cmd[i].cmd, pos_gs_cmd[i].cmd_len);

			if( pos_gs_cmd[i].data_len == 0 )
			{
				if( (pos_gs_cmd[i].cmd[0] & 0xff) == 0x6B )
				{
					if( pos_gs_cmd[i].cmd_len + 1 < remain_size )
					{
						int m;

						m = start[pos_gs_cmd[i].cmd_len + 0];

						if( (0 <= m) && (m <= 6) )
						{
							int start_idx, max_idx;
							int j;

							start_idx = pos_gs_cmd[i].cmd_len + 1;
							max_idx = 14;

							for(j=0; j < max_idx; j++)
							{
								if(start_idx + j < remain_size)
								{
									if( start[start_idx + j] == 0 )
									{
										*data_len = start_idx + j + 1;
										return 1;
									}
								}
								else
								{
									if (pos_display) {
										g_message("%s - %d", __FUNCTION__, __LINE__);
									}
									return -1;
								}
							}

							// PROTOCOL ERROR

							*data_len = start_idx + max_idx + 1;
							printf("GS CMD => NULL not found\n");
							return 1;
						}
						else
						{
							if( pos_gs_cmd[i].cmd_len + 2 < remain_size )
							{
								int start_idx, n;

								start_idx = pos_gs_cmd[i].cmd_len + 2;
								n = start[pos_gs_cmd[i].cmd_len + 1];

								*data_len = start_idx + n + 1;
								return 1;
							}
							else
							{
								return -1;
							}
						}
					}
					else
					{
						return -1;
					}
				}
				else
				{
					return 0;
				}
			}
			else if( pos_gs_cmd[i].data_len == -1 )
			{
				if( (pos_gs_cmd[i].cmd[0] & 0xff) == 0x56 )
				{
					if( pos_gs_cmd[i].cmd_len + 1 < remain_size )
					{
						int m;

						m = start[pos_gs_cmd[i].cmd_len + 0];

						if( (m == 0) || (m == 1) || (m == 48) || (m == 49) )
						{
							*data_len = 3;
						}
						else
						{
							*data_len = 4;
						}

						return 1;
					}
					else
					{
						return -1;
					}
				}
				else if( (pos_gs_cmd[i].cmd[0] & 0xff) == 0x2A )
				{
					if( pos_gs_cmd[i].cmd_len + 2 < remain_size )
					{
						int x,y,k;

						x = start[pos_gs_cmd[i].cmd_len + 0];
						y = start[pos_gs_cmd[i].cmd_len + 1];
						k = x * y + 8;

						*data_len = pos_gs_cmd[i].cmd_len + 2 + k + 1;
						return 1;
					}
					else
					{
						return -1;
					}
				}
				else
				{
					return 0;
				}
			}
			else
			{
				*data_len = pos_gs_cmd[i].data_len;
				return 1;
			}
		}
	}

	return 0;
}

static int _find_escp_cmd(char *start, int remain_size, int *data_len)
{
	char *desc = NULL;
	int i = 0;

	if (pos_display) {
		g_message("%s - start=0x%x, remain_size=%d", __FUNCTION__, start, remain_size);
	}

	for(i=0; i<pos_escp_cmd_count; i++)
	{
		if( remain_size < pos_escp_cmd[i].cmd_len )
		{
			if (pos_display) {
		//			g_message("start=0x%x, remain_size=%dm, ", start, remain_size, );
					g_message("%s - remain size too short(cmd_len)", __FUNCTION__);
			}
			return -1;
		}

		if( memcmp( start, pos_escp_cmd[i].cmd, pos_escp_cmd[i].cmd_len ) == 0 )
		{
			_debug_escp_cmd(pos_escp_cmd[i].cmd, pos_escp_cmd[i].cmd_len);

			if( pos_escp_cmd[i].data_len == 0 )
			{
				int start_idx, max_idx = 0;
				int j;

				if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x42 )
				{
					start_idx = pos_escp_cmd[i].cmd_len;
					max_idx = 17;
				}
				else if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x44 )
				{
					start_idx = pos_escp_cmd[i].cmd_len;
					max_idx = 33;
				}
				else // 0x62
				{
					start_idx = pos_escp_cmd[i].cmd_len + 1;
					max_idx = 17;
				}

				for(j=0; j < max_idx; j++)
				{
					if(start_idx + j < remain_size)
					{
						if( start[start_idx + j] == 0 )
						{
							*data_len = start_idx + j + 1;
							return 1;
						}
					}
					else
					{
						if (pos_display) {
							g_message("%s - remain size too short(data_len = 0)", __FUNCTION__);
						}
						return -1;
					}
				}

				// PROTOCOL ERROR

				*data_len = start_idx + max_idx + 1;
				printf("POS CMD => NULL not found\n");
				return 1;
			}
			else if( pos_escp_cmd[i].data_len == -1 )
			{
				if( pos_escp_cmd[i].cmd_len == 2 )
				{
					if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x28 && (pos_escp_cmd[i].cmd[1] & 0xff) == 0x5e )
					{
						if( pos_escp_cmd[i].cmd_len + 2 < remain_size )
						{
							int k, nl, nh;

							nl = start[pos_escp_cmd[i].cmd_len + 0];
							nh = start[pos_escp_cmd[i].cmd_len + 1];

							k = ((nh * 256) + nl);

							*data_len = pos_escp_cmd[i].cmd_len + 2 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - remain size too short(data_len = -1) CMD = 0x28 / 0x5E)", __FUNCTION__);
							}
							return -1;
						}
					}
					else // 0x26 & 0x00  =>  escp protocol 91page
					{
						if( pos_escp_cmd[i].cmd_len + 5 < remain_size )
						{
							int k, n, m, a1, x;

							n = start[pos_escp_cmd[i].cmd_len + 0];
							m = start[pos_escp_cmd[i].cmd_len + 1];
							a1 = start[pos_escp_cmd[i].cmd_len + 3];

							switch(n)
							{
								case 16:
									{
										switch(m)
										{
											case 42:
											case 36:
											case 30:
											case 24:
											case 12:
											case 10:
											case 8:
												x = 2;
												break;
											default:
												printf("POS CMD => 0x26,0x00 / case 16: default => range out[0x%x]\n", m);
												x = 2;
												break;
										}
									}
									break;

								case 24:
									{
										switch(m)
										{
											case 42:
											case 36:
											case 30:
											case 24:
											case 12:
											case 10:
											case 8:
												x = 3;
												break;
											default:
												printf("POS CMD => 0x26,0x00 / case 24: default => range out[0x%x]\n", m);
												x = 3;
												break;
										}
									}
									break;

								default:
									printf("POS CMD => 0x26,0x00 / range out[0x%x]\n", n);
									x = 3;
									break;
							}

							k = a1 * x;

							*data_len = pos_escp_cmd[i].cmd_len + 5 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - remain size too short(data_len = -1) CMD = 0x26 / 0x00)", __FUNCTION__);
							}
							return -1;
						}
					}
				}
				else // pos_escp_cmd[i].cmd_len == 1
				{
					if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x26 )
					{
						if( pos_escp_cmd[i].cmd_len + 4 < remain_size )
						{
							int s,n,w,m,x;

							s = start[pos_escp_cmd[i].cmd_len + 0];
							n = start[pos_escp_cmd[i].cmd_len + 1];
							w = start[pos_escp_cmd[i].cmd_len + 2];
							m = start[pos_escp_cmd[i].cmd_len + 3];

							x = s * w;

							*data_len = pos_escp_cmd[i].cmd_len + 4 + x + 1;
							return 1;
						}
						else
						{
							return -1;
						}
					}
					else if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x2A )
					{
						if( pos_escp_cmd[i].cmd_len + 3 < remain_size )
						{
							int k, m, nl, nh, byte_columns;

							m = start[pos_escp_cmd[i].cmd_len + 0];
							nl = start[pos_escp_cmd[i].cmd_len + 1];
							nh = start[pos_escp_cmd[i].cmd_len + 2];

#if 1	// New feature
							switch ( m ) {
								case 0:
								case 1:
									byte_columns = 1;
									break;

								case 32:
								case 33:
									byte_columns = 3;
									break;

								default:
									printf("POS CMD => 0x2A / range out[0x%x]\n", m);
									byte_columns = 1;
									break;
							}

							k = (nl + (256 * nh)) * byte_columns;
#else
							switch ( m ) {
								case 0:
								case 1:
								case 2:
								case 3:
								case 4:
								case 6:
									byte_columns = 1;
									break;

								case 32:
								case 33:
								case 38:
								case 39:
								case 40:
									byte_columns = 3;
									break;

								case 71:
								case 72:
								case 73:
									byte_columns = 6;
									break;

								default:
									printf("POS CMD => 0x2A / range out[0x%x]\n", m);
									byte_columns = 1;
									break;
							}

							k = ((nh * 256 ) + nl) * byte_columns;
#endif
							*data_len = pos_escp_cmd[i].cmd_len + 3 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - %d", __FUNCTION__, __LINE__);
							}
							return -1;
						}
					}
					else if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x2E )
					{
						if( pos_escp_cmd[i].cmd_len + 6 < remain_size )
						{
							int k, m, nl, nh;

							m = start[pos_escp_cmd[i].cmd_len + 3];
							nl = start[pos_escp_cmd[i].cmd_len + 4];
							nh = start[pos_escp_cmd[i].cmd_len + 5];

							k = m + (((nh*256) + nl + 7) / 8);

							*data_len = pos_escp_cmd[i].cmd_len + 6 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - %d", __FUNCTION__, __LINE__);
							}
							return -1;
						}
					}
					else if( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x30 )		// New feature
					{
						if( pos_escp_cmd[i].cmd_len + 3 < remain_size )
						{
							int n1,n2,n3,k;

							n1 = start[pos_escp_cmd[i].cmd_len + 0];
							n2 = start[pos_escp_cmd[i].cmd_len + 1];
							n3 = start[pos_escp_cmd[i].cmd_len + 2];

							k = n2+n3;

							*data_len = pos_escp_cmd[i].cmd_len + 3 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - %d", __FUNCTION__, __LINE__);
							}
							return -1;
						}
					}
					else if( ((pos_escp_cmd[i].cmd[0] & 0xff) == 0x4B) ||
						((pos_escp_cmd[i].cmd[0] & 0xff) == 0x4C) ||
						((pos_escp_cmd[i].cmd[0] & 0xff) == 0x59) ||
						((pos_escp_cmd[i].cmd[0] & 0xff) == 0x5A))
					{
						if( pos_escp_cmd[i].cmd_len + 2 < remain_size )
						{
							int k, nl, nh;

							nl = pos_escp_cmd[i].cmd[1];
							nh = pos_escp_cmd[i].cmd[2];
							k = ( (nh * 256) + nl );

							*data_len = pos_escp_cmd[i].cmd_len + 2 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - %d", __FUNCTION__, __LINE__);
							}
							return -1;
						}
					}
					else // ( (pos_escp_cmd[i].cmd[0] & 0xff) == 0x5E )
					{
						if( pos_escp_cmd[i].cmd_len + 3 < remain_size )
						{
							int k, m, nl, nh;

							m = pos_escp_cmd[i].cmd[1];
							nl = pos_escp_cmd[i].cmd[2];
							nh = pos_escp_cmd[i].cmd[3];
							k = ( (nh * 256) + nl ) * 2;	// ????

							*data_len = pos_escp_cmd[i].cmd_len + 3 + k + 1;
							return 1;
						}
						else
						{
							if (pos_display) {
								g_message("%s - %d", __FUNCTION__, __LINE__);
							}
							return -1;
						}
					}
				}
			}
			else
			{
				*data_len = pos_escp_cmd[i].data_len;
				return 1;
			}

			break;
		}
	}

	return 0;
}

static int remove_escp_gs_cmd(char *buf, int *buf_size, int *end_idx)
{
	char *p = buf;
	int data_len;
	int remain_size;

	int i = 0;
	int ret;

	while( i < *buf_size )
	{
		int esc;
		esc = buf[i] & 0xff;

		if( esc == 0x1B || esc == 0x1D )
		{
			if( i+1 < *buf_size )
			{
				remain_size = *buf_size - (i+1);

				if( esc == 0x1B )
					ret = _find_escp_cmd( &buf[i+1], remain_size, &data_len);
				else
					ret = _find_gs_cmd( &buf[i+1], remain_size, &data_len);

				if( ret == -1 )
				{
					*end_idx = i;

					if (pos_display) {
						printf("POS CMD => DATA_LEN:-1\n");
					}

					return -1;
				}
				else if( ret == 0 )
				{
					if (pos_display) {
						printf("POS CMD => CMD not found\n");
					}

					memcpy( &buf[i], &buf[i+1], (*buf_size - i) - 1);
					*buf_size = *buf_size - 1;
					continue;
				}
				else if( ret == 1 )
				{
					if( i + data_len <= *buf_size )
					{
						int len;

						if (pos_display) {
							printf("POS CMD => DATA_LEN:%d removed\n", data_len);
						}

						len = (*buf_size - i) - data_len;

						if( len > 0 )
							memcpy( &buf[i], &buf[i+data_len], len);

						*buf_size = *buf_size - data_len;
						continue;
					}
					else
					{
						if (pos_display) {
							printf("POS CMD => DATA_LEN:%d buf not remained\n", data_len);
						}

						*end_idx = i;
						return -1;
					}
				}
			}
			else
			{
				*end_idx = i;
				return -1;
			}
		}

		i++;
	}

	return 0;
}

static int _parse_epson_data(int ch, char *buf, int *buf_size)
{
	POS_POSITION eol_idx;
	char pos_cmd_data[MAX_POS_BUFFER];

	int i = 0, j = 0;
	int len;

	while( i < *buf_size )
	{
		if( (buf[i] & 0xff) == 0x0a )
		{
			if( i == 0 )
			{
				len = *buf_size - 1;
				memcpy(buf, &buf[1], len);
				*buf_size = len;
			}
			else if( (i == 1) && ((buf[0] & 0xff) == 0x0D) )
			{
				len = *buf_size - 2;
				memcpy(buf, &buf[2], len);
				*buf_size = len;
			}
			else
			{
				len = i;
				memset(pos_cmd_data, 0x0, MAX_POS_BUFFER);

				if( (buf[i-1] & 0xff) == 0x0D )
				{
					memcpy(pos_cmd_data, buf, len-1);
				}
				else
				{
					memcpy(pos_cmd_data, buf, len);
				}

				if(pos_cmd_data[0] == 0)	// exception NULL ???
				{
					int p = 0;

					while(p < len)
					{
						if( pos_cmd_data[p] != 0 )
							break;

						p++;
					}

					if( p != len )
					{
						_event_poslog_put(ch, &pos_cmd_data[p], len - p, NULL);
					}
				}
				else
				{
					_event_poslog_put(ch, pos_cmd_data, len, NULL);
				}

				len = *buf_size - (i+1);
				memcpy(buf, &buf[i+1], len);
				*buf_size = len;
			}

			i = 0;
		}
		else
		{
			i++;
		}
	}

	return 0;
}

int process_epson_protocol(int ch, char *ori_buf, int *ori_buf_size)
{
	int removed_size;
	int end_idx = -1;
	int ret;

//	g_message("%s - start=0x%x, remain_size=%d", __FUNCTION__, ori_buf, *ori_buf_size);

	ret = remove_escp_gs_cmd(ori_buf, ori_buf_size, &end_idx);

	if (pos_display) {
		_hex_debug(__FUNCTION__, ori_buf, *ori_buf_size);

		g_message("%s - remove_escp_gs_cmd => ret:%d, buf_size:%d, end_idx:%d", __FUNCTION__, ret, *ori_buf_size, end_idx);
	}

	if( ret < 0 )
	{
		int back_size;
		char *escp_back;

		int buf_size;

		back_size = *ori_buf_size - end_idx;

		escp_back = g_malloc0( back_size );

		memcpy(escp_back, &ori_buf[end_idx], back_size);

		buf_size = *ori_buf_size - back_size;

		_parse_epson_data(ch, ori_buf, &buf_size);

		memcpy(&ori_buf[buf_size], escp_back, back_size);

		*ori_buf_size = buf_size + back_size;

		g_free(escp_back);
	}
	else
	{
		_parse_epson_data(ch, ori_buf, ori_buf_size);
	}

	return 0;
}

/*
static char pos_test_help[] = "pos_test [idx]";
static int pos_test(int argc, char **argv)
{
	int idx, item;
	int i;

	if( argc < 3 )
	{
		g_message("%s - ARGC Fail", __FUNCTION__);
		return 0;
	}

	idx = atoi(argv[1]);

	g_message("[%s] KJH =>  CMD_LEN:%d", __FUNCTION__, pos_escp_cmd[idx].cmd_len);
	g_message("[%s] KJH => DATA_LEN:%d", __FUNCTION__, pos_escp_cmd[idx].data_len);

	g_printf("\n");

	for(i=0; i< pos_escp_cmd[idx].cmd_len; i++)
	{
		g_printf("[0x%x]", pos_escp_cmd[idx].cmd[i]);
	}

	g_printf("\n");


	return 0;
}
__commandlist(pos_test, "pos_test", pos_test_help, pos_test_help);
*/

