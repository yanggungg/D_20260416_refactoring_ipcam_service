#include "nf_pos.h"
#include "nf_api_pos_eventlog.h"
#include "nf_logevtdef.h"
#include "jbshell.h"
#include <sys/ioctl.h>

#ifndef TTYS1_DEV_ACT
	#include "fpga_rs485_ioctl.h"
#endif

#if defined (_ANF_1648) || defined(_ANF_0824)
	#include "fpga_rs485_uart2_ioctl.h"
#endif

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "pos"

#define	POS_DEBUG	0
#define	POS_DUMP	0	// write pos data in file 

static guint64 log_count = 0;

int KLOG_LV = 0;

#define __KLOG(LEVEL, ARGS) \
	if (KLOG_LV >= LEVEL) { \
		g_print("[LOG_POS_AVE] "); g_print ARGS; }

#if 0

#define MAX_AVE_BUFFER		(1024*8)
#define AVE_ESC				(0x1B)
#define AVE_LF				(0x0A)
#define AVE_ITEM			(0x40)	// '@'
#define AVE_TOTAL			(0x30)	// '0'


typedef struct _NF_POS_AVE_ITEM_T {
	char	name[32];
	int 	price;
	int 	qty;
	int 	sum;	// price*qty
} NF_POS_AVE_ITEM;

typedef struct _NF_POS_AVE_COMMAND_T {
	int				chan;
	int				addr;
	struct tm 		datetime;
	int 				num_items;
	NF_POS_AVE_ITEM	items[64];	// item 갯수 제한
	int 				total;
} NF_POS_AVE_COMMAND;

typedef enum _NF_POS_AVE_STEP_E { 
	NF_POS_AVE_STEP_DATETIME	= 0, 
	NF_POS_AVE_STEP_ITEM			= 1, 
	NF_POS_AVE_STEP_ITEMINFO		= 2, 
	NF_POS_AVE_STEP_ITEMFINISH	= 3, 
	NF_POS_AVE_STEP_TOTAL		= 4, 
	NF_POS_AVE_STEP_SPLITLINE	= 5, 
	NF_POS_AVE_STEP_NR			= 6 
} NF_POS_AVE_STEP_E;


/* POS AVE Thread Context -> 모든 채널 포함 */
typedef struct _NF_POS_AVE_HANDLER_T {
	unsigned char				buffer[MAX_AVE_BUFFER];	// 파싱처리 전 임시 버퍼
	int						buffer_len; 
	NF_POS_AVE_COMMAND	command[NUM_CHANNEL];			// 파싱처리된 데이터
	NF_POS_AVE_STEP_E		step[NUM_CHANNEL];				// 파싱스텝 FIXME
	int						init;						// 초기화
} NF_POS_AVE_HANDLER;

static NF_POS_AVE_HANDLER	_pos_ave_handler;

//#define TEST_POS_DEV

/*
AVE POS Networking Protocol
(VSI-ADD Protocol)
The AVE VSI-Pro, IF ECR Interface Cards and other POS Adapters export the
POS data in formatted ASCII text via RS-232 directly. This data can be read
by any DVR Com Port for recording as a separate data file appended to the
Audio/Video file.
For multiple POS on one DVR you could use multiple RS-232 ports but this is
cumbersome and the length of runs is limited.
The AVE “Regcom” is a device that connects directly to any of the AVE POS
interfaces listed above or any RS-232 source. This buffered device takes the
POS data and converts it to an RS-485 network.
The “Networker” connects only to the VSI-Pro and supports the same RS-485
network. This network is terminated by a “Hydra” which decodes the RS-485
network and sends all the data to one RS-232 Com Port on the DVR.
The data from Hydra to the DVR Com Port has the following data format:
<ESC> ADDR {TEXT TO BE DISPLAYED}
Where ESC is 1BH; ADDR is one byte binary code. The valid address is 1
through 16.
The DVR will then read this protocol and decode the proper POS interface
data to be associated with the respective camera.
*/

static void _dump_buffer(char *prefix, char *buffer, int len)
{
#if !POS_DEBUG
	return;
#endif
	if (len <= 0) return;

	int i;
	g_print("dump_buffer[%s,len:%d] :\n", prefix, len);
	for (i=0; i < len; i++) printf("%c", (char)buffer[i]);
	g_print("\n");

	int line = 0;
	g_print("dump_buffer_hex[%s] : \n", prefix);
	for (i=0; i < len; i++)  {
		printf("0x%02x ", (char)buffer[i]);
		if (++line >= 10) {
			g_print("\n");
			line = 0;
		}
	}
	g_print("\n");
}

static int _find_string(char *str, int len, char *substr)
{
	int i, j, substr_len = strlen(substr);

	if (len <= 0 || substr_len > len) return -1;
	
	for (i=0; i <= len-substr_len; i++) {
		for (j=0; j < substr_len; j++) { 
			if (substr[j] != str[i+j]) break;
		}
		if (j == substr_len) return i;
	}

	return -1;
}

static void _pos_ave_cmd_print(NF_POS_AVE_COMMAND *command)
{
	g_print("ave command[chan:%d, addr:%d]:\n", command->chan, command->addr);
	
	g_print("Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n",
		command->datetime.tm_year, command->datetime.tm_mon, command->datetime.tm_mday,
		command->datetime.tm_hour, command->datetime.tm_min, command->datetime.tm_sec);

	int name_len;
	int i, j, position = 13;
	for (i=0; i < command->num_items; i++) {
		NF_POS_AVE_ITEM *item = &command->items[i];
		g_print("%s", item->name);
		name_len = strlen(item->name);

		for (j=0; j < position-name_len; j++) g_print(" ");

		g_print(" $%05.02f %02d  - %08.02f dollar\n", item->price/100.0f, item->qty, item->sum/100.0f);
	}

	g_print("Total                    %010.02f dollar\n", command->total/100.0f);
	g_print("---------------------------\n");
}

#define POS_DISP	0
#if POS_DISP
extern void live_win_disp_pos(char *str);
static void _pos_ave_cmd_display(NF_POS_AVE_COMMAND *command)
{
	char disp_str[1024] = {0}, *cptr;
	int	len;
	int	name_len;
	int	i, j, position = 13;
	
	cptr = disp_str;

	len = sprintf(cptr, "Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n",
		command->datetime.tm_year, command->datetime.tm_mon, command->datetime.tm_mday,
		command->datetime.tm_hour, command->datetime.tm_min, command->datetime.tm_sec);
	cptr += len;

	for (i=0; i < command->num_items; i++) {
		NF_POS_AVE_ITEM *item = &command->items[i];
		len = sprintf(cptr, "%s", item->name);
		cptr += len;
		
		name_len = strlen(item->name);

		for (j=0; j < position-name_len; j++) {
			len = sprintf(cptr, " ");
			cptr += len;
		}

		len = sprintf(cptr, " $%05.02f %02d  - %08.02f dollar\n", item->price/100.0f, item->qty, item->sum/100.0f);
		cptr += len;
	}

	len = sprintf(cptr, "Total                    %010.02f dollar\n", command->total/100.0f);
	cptr += len;
	sprintf(cptr, "---------------------------\n");
	
	live_win_disp_pos(disp_str);
}
#endif

#if 1
static gboolean _pos_ave_put_eventlog(NF_POS_AVE_COMMAND *command)
{
	command->datetime.tm_year += 100; //= command->datetime.tm_year+2000-1900;
	command->datetime.tm_mon -= 1;

	time_t ttime = mktime(&command->datetime);	
	GTimeVal tv;
	struct tm sttime;

	gchar text[256];
	memset(text, 0x00, sizeof(text));

	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)text;
	
#if 1	// DVR system time -> log timestamp
	gettimeofday(&tv, NULL);
#else	// POS time -> log timestamp
	tv.tv_sec = ttime;
	tv.tv_usec = 0;
#endif
#if !POS_TEST
	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = command->chan;
	log_data->addr = command->addr;
	log_data->pos_log_type = NF_POS_LOG_TYPE_TOTAL;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;	// FIXME

	log_data->Total.total = command->total;

	nf_eventlog_put_param(&tv, LT_SYSTEM_POS, command->chan, 0, text);
	
	int i;
	for (i=command->num_items-1; i >= 0; i--) {
		memset(text, 0x00, sizeof(text));

		log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
		log_data->chan = command->chan;
		log_data->addr = command->addr;
		log_data->pos_log_type = NF_POS_LOG_TYPE_ITEM;
		log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;	// FIXME
		
		strcpy(log_data->Item.name, command->items[i].name);
		log_data->Item.price = command->items[i].price;
		log_data->Item.qty = command->items[i].qty;
		log_data->Item.sum = command->items[i].sum;

		nf_eventlog_put_param(&tv, LT_SYSTEM_POS, command->chan, 0, text);
	}
#endif
	sttime = *localtime(&tv.tv_sec);
	log_count++;
	__KLOG(0, ("[%04d/%02d/%02d - %02d:%02d:%02d] log count: %lld\n", 
		sttime.tm_year+1900, sttime.tm_mon+1, sttime.tm_mday,
		sttime.tm_hour, sttime.tm_min, sttime.tm_sec,
		log_count));
	if (KLOG_LV >= 1) {
		_pos_ave_cmd_print(command);
	}	

#if POS_DISP
	_pos_ave_cmd_display(command);
#endif

	return TRUE;
}
#else	/* obsolete */
gboolean _pos_ave_put_eventlog(POS_AVE_COMMAND *command)
{
#if 0	
	command->datetime.tm_year = command->datetime.tm_year+2000-1900;
	command->datetime.tm_mon -= 1;
	
	time_t ttime = mktime(&command->datetime);	
	GTimeVal tv;
	tv.tv_sec = ttime;
	tv.tv_usec = 0;
g_print("<%s> tv.tv_sec: %ld\n", __FUNCTION__, tv.tv_sec);
#endif
	static gchar text[256];

	static NF_POS_LOG_DATA *log_data = NULL;
	if (log_data == NULL) log_data = malloc(sizeof(NF_POS_LOG_DATA));

	memset(log_data, 0, sizeof(log_data));
	
	log_data->pos_log_type = NF_POS_LOG_TYPE_TOTAL;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;	// FIXME

	log_data->Total.total = command->total;

	memset(text, 0, sizeof(text));
	nf_pos_log2text(log_data, text);
#if 0
	nf_eventlog_put_param(&tv, LT_SYSTEM_POS, command->chan, 0, text);
#else
	nf_eventlog_put_param(NULL, LT_SYSTEM_POS, command->chan, 0, text);
#endif
	
	int i;
	for (i=command->num_items-1; i >= 0; i--) {
		memset(log_data, 0, sizeof(log_data));		
		
		log_data->pos_log_type = NF_POS_LOG_TYPE_ITEM;
		log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;	// FIXME
		
		strcpy(log_data->Item.name, command->items[i].name);
		log_data->Item.price = command->items[i].price;
		log_data->Item.qty = command->items[i].qty;
		log_data->Item.sum = command->items[i].sum;

		memset(text, 0, sizeof(text));
		nf_pos_log2text(log_data, text);
#if 0					
		nf_eventlog_put_param(&tv, LT_SYSTEM_POS, command->chan, 0, text);
#else
		nf_eventlog_put_param(NULL, LT_SYSTEM_POS, command->chan, 0, text);
#endif
	}
	
	return TRUE;
}
#endif

static void _pos_ave_handler_init(NF_POS_AVE_HANDLER *handler, NF_POS_SYSDB *sysdb_pos)
{	
	int i;

	g_return_if_fail(handler != NULL);
	g_return_if_fail(sysdb_pos != NULL);	
	g_return_if_fail(handler->init == 0);	
	
	for (i=0; i < NUM_CHANNEL; i++) {
		handler->command[i].chan = i;
		handler->command[i].addr = sysdb_pos->addr[i];
		handler->step[i] = NF_POS_AVE_STEP_DATETIME;
	}		
	
	handler->init = 1;

	g_message("%s", __FUNCTION__);
}

static void _pos_ave_command_init(NF_POS_AVE_COMMAND *command, NF_POS_AVE_STEP_E *step)
{
	int chan = command->chan;
	int addr = command->addr;
	
	memset(command, 0, sizeof(command));
	command->chan = chan;
	command->addr = addr;
	command->num_items = 0;
	*step = NF_POS_AVE_STEP_DATETIME;	
}

static int _pos_ave_cmd_analyze_datetime(char *cmd_buffer, int cmd_len, NF_POS_AVE_COMMAND *command)
{
	/* |0x00|Date - 10/02/08 Time - 20:36:43|0x0A| */

#if 1
	struct tm datetime;
	int res;

	if (cmd_buffer[0] == 0x00) {
		res = sscanf(&cmd_buffer[1], "Date - %02d/%02d/%02d Time - %02d:%02d:%02d", 
			&datetime.tm_year, &datetime.tm_mon, &datetime.tm_mday,
			&datetime.tm_hour, &datetime.tm_min, &datetime.tm_sec);
	}
	else {
		res = sscanf(cmd_buffer, "Date - %02d/%02d/%02d Time - %02d:%02d:%02d", 
			&datetime.tm_year, &datetime.tm_mon, &datetime.tm_mday,
			&datetime.tm_hour, &datetime.tm_min, &datetime.tm_sec);		
	}

	if (res != 6) {
		__KLOG(1, ("<%s:%d> error - res :%d, Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n", __FUNCTION__, __LINE__, res,
			datetime.tm_year, datetime.tm_mon, datetime.tm_mday,
			datetime.tm_hour, datetime.tm_min, datetime.tm_sec));
		return 0;			
	}
#else
	/* yy/mm/dd  HH:MM:SS 이면 파싱가능 */
	struct tm datetime;
	int index = 0;
	char tmp[3] = {0};

	/* parsing - yy/mm/dd */
	index = _find_string(cmd_buffer, cmd_len, "/");

	if (index < 2) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	index -= 2;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	datetime.tm_year = atoi(tmp);

	index += 2;
	if (index+2 >= cmd_len || cmd_buffer[index] != '/') {
		__KLOG(1, ("<%s:%d> error -index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	index++;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	datetime.tm_mon = atoi(tmp);

	index += 2;
	if (index+2 >= cmd_len || cmd_buffer[index] != '/') {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	index++;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	datetime.tm_mday = atoi(tmp);

	index += 2;
	if (index+2 >= cmd_len) {
		__KLOG(1, ("<%s:%d> error -index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}

	/* parsing - hh:mm:ss */
	index += _find_string(&cmd_buffer[index], cmd_len-index, ":");
	if (index < 2) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	index -= 2;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error -index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	datetime.tm_hour = atoi(tmp);

	index += 2;
	if (index+2 >= cmd_len || cmd_buffer[index] != ':') {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	index++;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	datetime.tm_min = atoi(tmp);

	index += 2;
	if (index+2 >= cmd_len || cmd_buffer[index] != ':') {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	index++;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	datetime.tm_sec = atoi(tmp);
#endif

	/* parsing finished */
	command->datetime.tm_year = datetime.tm_year;
	command->datetime.tm_mon = datetime.tm_mon;
	command->datetime.tm_mday = datetime.tm_mday;
	command->datetime.tm_hour = datetime.tm_hour;
	command->datetime.tm_min = datetime.tm_min;
	command->datetime.tm_sec = datetime.tm_sec;	

	return 1;
}

static int _pos_ave_cmd_analyze_item(char *cmd_buffer, int cmd_len, NF_POS_AVE_COMMAND *command)
{
	/* @Apple    */

	/* '@'으로 시작하고 item name 길이가 1 이상이면 파싱가능 */
	/* item name 뒤에오는 space(0x20)는 item name에 포함하지 않음(item name 중간의 space는 item name에 포함) */

	char name[32] = {0};
	int res, count;
	NF_POS_AVE_ITEM *item;

	if (cmd_buffer[0] == 0x00) {
		res = sscanf(&cmd_buffer[1], "@%s", name);
	}
	else {
		res = sscanf(cmd_buffer, "@%s", name);
	}

	if (res != 1) {
		__KLOG(1, ("<%s:%d> error - res: %d\n", __FUNCTION__, __LINE__, res));
		return 0;
	}

	count = command->num_items;
	item = &command->items[count];
	memset(item->name, 0, sizeof(item->name));	

	strcpy(item->name, name);
	
	return 1;
}

static int _pos_ave_cmd_analyze_iteminfo(char *cmd_buffer, int cmd_len, NF_POS_AVE_COMMAND *command)
{
	/* |0x00| $01 11  - 00011 dollar|0x0A| */

	/* |$|dd -> price, dd -> quantity, ddddd -> sum */
	/* price*quantity != sum 이면 error */

	int price, qty, sum;
	float fprice, fsum;
	int i, index = 0, res, count;
	NF_POS_AVE_ITEM *item;

	/* find price */
	for (i=0; i < cmd_len; i++) {
		if (cmd_buffer[i] == '$') break;
	}
	index = i;

#if 1
	if (i == cmd_len) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}

	/* 향후 format에 따라 변경 */
	res = sscanf(&cmd_buffer[i], "$%02f %02d  - %05f", &fprice, &qty, &fsum);

	if (res != 3) {
		__KLOG(1, ("<%s:%d> error - res: %d, price: %05.2f, qty: %02d, sum: %05.2f\n", __FUNCTION__, __LINE__, res, fprice, qty, fsum));
		return 0;
	}

	/* float -> int convert */
	fprice *= 100.0f;
	fsum *=100.0f;
	price = (int)fprice;
	sum = (int)fsum;
#else
	char tmp[6] = {0};

	if (index+2 >= cmd_len) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__,index,cmd_len));
		return 0;
	}
	index++;

	if (!isdigit(cmd_buffer[index]) || !isdigit(cmd_buffer[index+1])) {
		__KLOG(1, ("<%s:%d> error - no digit, index: %d\n", __FUNCTION__, __LINE__,index));
		return 0;
	}
	
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	price = atoi(tmp);

	index += 2;
	if (index+2 >= cmd_len) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}

	/* find quantity */
	for (i=index; i < cmd_len-1; i++) {
		if (isdigit(cmd_buffer[i]) && isdigit(cmd_buffer[i+1])) break;
	}
	index = i;

	if (index >= cmd_len-1) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}

	memset(tmp, 0, sizeof(tmp));
	tmp[0] = cmd_buffer[index];
	tmp[1] = cmd_buffer[index+1];
	qty = atoi(tmp);

	index += 2;
	if (index+5 >= cmd_len) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}

	/* find sum */
	int j;
	for (i=index; i < cmd_len-5; i++) {
		for (j=0; j < 5; j++) {
			if (!isdigit(cmd_buffer[i+j])) break;
		}
		if (j == 5) break;		// found 5 digit
	}
	index = i;

	if (index+5 >= cmd_len) {
		__KLOG(1, ("<%s:%d> error - index: %d, cmd_len: %d\n", __FUNCTION__, __LINE__, index, cmd_len));
		return 0;
	}

	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, &cmd_buffer[index], 5);
	sum = atoi(tmp);
#endif
	count = command->num_items;
	item = &command->items[count];

	if (sum != price*qty) {
		__KLOG(1, ("<%s:%d> error - item: %s, sum != price*qty, sum: %d, price: %d, qty: %d\n", 
			__FUNCTION__, __LINE__, item->name, sum, price, qty));
		return 0;
	}

	item->price = price;
	item->qty = qty;
	item->sum = sum;

	command->num_items++;
	
	return 1;
}

static int _pos_ave_cmd_analyze_total(char *cmd_buffer, int cmd_len, NF_POS_AVE_COMMAND *command)
{
	/* 0Total             0000023 dollar|0x0A| */

	/* "Total"이후 digit sequence -> total price */
	/* items sum != total price 이면 error */
	int i, index = 0, res, total, sum = 0;
	float ftotal;

	index = _find_string(cmd_buffer, cmd_len, "Total");
	if (index < 0) {
		__KLOG(1, ("<%s:%d> error - no 'Total' string\n", __FUNCTION__, __LINE__));
		return 0;
	}
	index += strlen("Total");

	/* 향후 format에 따라 변경 */
	res = sscanf(&cmd_buffer[index], "%07f", &ftotal);

	if (res != 1) {
		__KLOG(1, ("<%s:%d> error - res: %d, total: %10.2f\n", __FUNCTION__, __LINE__, res, ftotal));
		return 0;
	}

	ftotal *= 100.0f;
	total = (int)ftotal;

	for (i=0; i < command->num_items; i++) {
		sum += command->items[i].sum;
	}

	if (total != sum) {
		__KLOG(1, ("<%s:%d> error - total != sum, total: %d, sum: %d, num_items: %d\n", __FUNCTION__, __LINE__, total, sum, command->num_items));
		return 0;
	}

	command->total = total;
	
	return 1;
}

static int _pos_ave_cmd_analyze_splitline(char *cmd_buffer, int cmd_len, NF_POS_AVE_COMMAND *command)
{
	/* |0x00|------------------------------|0x0A| */
	return 1;
}

static NF_POS_AVE_STEP_E _pos_ave_cmd_analyze(unsigned char *cmd_buffer, int cmd_len, NF_POS_AVE_STEP_E step, NF_POS_AVE_COMMAND *command)
{
	switch (step) {
		case NF_POS_AVE_STEP_DATETIME: 
			if (_pos_ave_cmd_analyze_datetime(cmd_buffer, cmd_len, command)) {
				__KLOG(2, ("analyze[datetime] Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n", 
					command->datetime.tm_mday, command->datetime.tm_mon, command->datetime.tm_year,
					command->datetime.tm_hour, command->datetime.tm_min, command->datetime.tm_sec));				
				step = step+1;
			}
			else {
				step = NF_POS_AVE_STEP_DATETIME;
				__KLOG(1, ("<%s> parsing error - date time\n", __FUNCTION__));
				_dump_buffer("datetime", cmd_buffer, cmd_len);
				//if (POS_DEBUG) KLOG_LV = 3;
			}
			break;
		case NF_POS_AVE_STEP_ITEM:
			if (_pos_ave_cmd_analyze_item(cmd_buffer, cmd_len, command)) {
				__KLOG(2, ("analyze[item] %s\n", command->items[command->num_items].name));
				step = step+1;
			}
			else {
				step = NF_POS_AVE_STEP_DATETIME;
				__KLOG(1, ("<%s> parsing error - item, name: %s\n", 
					__FUNCTION__, command->items[command->num_items].name));
				_dump_buffer("item", cmd_buffer, cmd_len);
				//if (POS_DEBUG) KLOG_LV = 3;
			}
			break;
		case NF_POS_AVE_STEP_ITEMINFO:
			if (_pos_ave_cmd_analyze_iteminfo(cmd_buffer, cmd_len, command)) {
				__KLOG(2, ("analyze[iteminfo] price: %05.02f, qty: %02d, sum: %08.02f\n",
					command->items[command->num_items-1].price/100.0f, 
					command->items[command->num_items-1].qty, 
					command->items[command->num_items-1].sum/100.0f));
				step = step+1;				
			}
			else {
				step = NF_POS_AVE_STEP_DATETIME;
				__KLOG(1, ("<%s> parsing error - item info, name: %s\n", 
					__FUNCTION__, command->items[command->num_items-1].name));
				_dump_buffer("iteminfo", cmd_buffer, cmd_len);
				//if (POS_DEBUG) KLOG_LV = 3;
			}
			break;
		case NF_POS_AVE_STEP_ITEMFINISH:
			step = NF_POS_AVE_STEP_DATETIME;
			__KLOG(1, ("<%s:%d> error - Invalid step: %d !!!\n",__FUNCTION__,__LINE__,step));
			_dump_buffer("itemfinish", cmd_buffer, cmd_len);
			//if (POS_DEBUG) KLOG_LV = 3;
//			step = step+1;
			break; 
		case NF_POS_AVE_STEP_TOTAL:
			if (_pos_ave_cmd_analyze_total(cmd_buffer, cmd_len, command)) {
				__KLOG(2, ("analyze[total] %010.02f\n", command->total/100.0f));
				step = step+1;
			}
			else {
				step = NF_POS_AVE_STEP_DATETIME;
				__KLOG(1, ("<%s> parsing error - total\n", __FUNCTION__));
				_dump_buffer("total", cmd_buffer, cmd_len);
				//if (POS_DEBUG) KLOG_LV = 3;
			}
			break;
		case NF_POS_AVE_STEP_SPLITLINE:
			if (_pos_ave_cmd_analyze_splitline(cmd_buffer, cmd_len, command)) {
				__KLOG(2, ("analyze[splitline] %s\n", cmd_buffer));
				step = step+1;
			}
			else {
				step = NF_POS_AVE_STEP_DATETIME;
				__KLOG(1, ("<%s> parsing error - splitline\n", __FUNCTION__));
				_dump_buffer("splitline", cmd_buffer, cmd_len);
				//if (POS_DEBUG) KLOG_LV = 3;
			}
			break;
		default:
			__KLOG(1, ("<%s:%d> error - Invalid step: %d !!!\n",__FUNCTION__,__LINE__,step));
			_dump_buffer("default", cmd_buffer, cmd_len);
			//if (POS_DEBUG) KLOG_LV = 3;
			break;
	}

	return step;
}

/* buffer에서 한 command를 검출한다 */
/* return value - command length
 0 : need more data
-1 : error */
static int _pos_ave_cmd_token(unsigned char *buffer, int buffer_len, int buffer_index, NF_POS_AVE_STEP_E *step)
{
	int i, len = 0;
	gboolean finish = FALSE;

	for (i=buffer_index; i < buffer_len; i++) {
		switch (*step) {
			case NF_POS_AVE_STEP_DATETIME: {	// |0x00|Date - 10/02/08 Time - 20:36:43|0x0A|
				if (buffer[i] == AVE_LF) {
					len = i-buffer_index+1;
					finish = TRUE;	
				}
				else if (buffer[i] == AVE_ESC) {	// no '0x0A'
					len = i-buffer_index;
					finish = TRUE;
				}
			} break;
			case NF_POS_AVE_STEP_ITEM: {		// @Apple    |0x1B|
				if (buffer[i] == AVE_ESC) {
					len = i-buffer_index;
					finish = TRUE;
				}
			} break;
			case NF_POS_AVE_STEP_ITEMINFO: {	// |0x00| $01 11  - 00011 dollar|0x0A|
				if (buffer[i] == AVE_LF) {
					len = i-buffer_index+1;
					finish = TRUE;
				}
				else if (buffer[i] == AVE_ESC) {	// no '0x0A'
					len = i-buffer_index;
					finish = TRUE;
				}
			} break;
			case NF_POS_AVE_STEP_ITEMFINISH: {	// move to item or total step
				if (buffer[i] == AVE_ITEM) {	// '@'
					__KLOG(2, ("<%s> STEP_ITEMFINISH -> STEP_ITEM\n", __FUNCTION__));
					*step = NF_POS_AVE_STEP_ITEM;
				}
				else if (buffer[i] == AVE_TOTAL) {	// '0'
					__KLOG(2, ("<%s> STEP_ITEMFINISH -> STEP_TOTAL...1\n", __FUNCTION__));
					*step = NF_POS_AVE_STEP_TOTAL;
				}
				else if (_find_string(&buffer[i], buffer_len-i, "Total") >= 0) {	// "Total"
					__KLOG(2, ("<%s> STEP_ITEMFINISH -> STEP_TOTAL...2\n", __FUNCTION__));
					*step = NF_POS_AVE_STEP_TOTAL;
				}
			} break;
			case NF_POS_AVE_STEP_TOTAL: {		// 0Total             0000023 dollar|0x0A|
				if (buffer[i] == AVE_LF) {
					len = i-buffer_index+1;
					finish = TRUE;
				}
				else if (buffer[i] == AVE_ESC) {	// no '0x0A'
					len = i-buffer_index;
					finish = TRUE;
				}
			} break;
			case NF_POS_AVE_STEP_SPLITLINE: {	// |0x00|------------------------------|0x0A|
				if (buffer[i] == AVE_LF) {
					len = i-buffer_index+1;
					finish = TRUE;
				}
				else if (buffer[i] == AVE_ESC) {	// no '0x0A'
					len = i-buffer_index;
					finish = TRUE;
				}
			} break;
			default: {
				__KLOG(1, ("<%s> error - Invalid parsing step: %d\n", __FUNCTION__, *step));
			} break;
		}
		
		if (finish) break;			
	}

	if (!finish) {	// still need more character
		return 0;
	}

	if (i == buffer_index) {	// error - distorted data
		__KLOG(1, ("<%s:%d> error - step: %d, i: %d, buffer_index: %d, len: %d, buffer_len: %d\n",
			__FUNCTION__,__LINE__,*step,i,buffer_index,len,buffer_len));
		_dump_buffer("buffer", buffer, buffer_len);
		return -1;
	}

	return len;
}

static inline int _pos_ave_get_chan(NF_POS_AVE_HANDLER *handler, unsigned char addr, NF_POS_SYSDB *sysdb_pos)
{
	int i;

	/* address 갱신 */
	for (i=0; i < NUM_CHANNEL; i++) 
		handler->command[i].addr = sysdb_pos->addr[i];
		
	for (i=0; i < NUM_CHANNEL; i++) {
		if (handler->command[i].addr == addr)
			break;
	}

	if (i == NUM_CHANNEL) return -1;

	return handler->command[i].chan;
}

static guint _pos_ave_parse_command(NF_POS_AVE_HANDLER *handler, char *rbuf, int rbuf_len, NF_POS_SYSDB *sysdb_pos)
{
	/* 데이터는 한 command(0x1B|ADDR|{TEXT})가 끝날때까지 같은 POS 장비(같은 address)에서 와야한다 */
	/* 아래 형식으로 데이터가 오지만 중간중간 서로다른 address의 command가 들어올 수 있다 */ 
	/* 수신 데이터에 오류가  있을경우 올바른 command가 올때까지 skip */
	
	/* AVE Packet Format Example
	0x1B|0x01|0x00|Date - 10/02/08 Time - 20:36:43|0x0A|
	0x1B|0x01|@Apple    |0x1B|0x01|0x00| $01 11  - 00011 dollar|0x0A|
	0x1B|0x01|@Snack    |0x1B|0x01|0x00| $01 12  - 00012 dollar|0x0A|
	0x1B|0x01|0Total             0000023 dollar|0x0A|
	0x1B|0x01|0x00|------------------------------|0x0A|
	*/	

	int buffer_index = 0, len, i;
	unsigned char addr = 0xff;			// 현재 처리중인 address
	int			chan;				// addr에 대응되는 channel
	unsigned char cmd_buffer[MAX_AVE_BUFFER];		// command buffer -> 한 command가 다 차면 파싱

	if (!handler->init) {
		_pos_ave_handler_init(handler, sysdb_pos);
	}

	if (handler->buffer_len+rbuf_len >= sizeof(handler->buffer)) {
		__KLOG(0, ("<%s:%d> error - buffer overflow !!!, buffer_len: %d, rbuf_len: %d\n",__FUNCTION__,__LINE__,
			handler->buffer_len,rbuf_len));
	
		_dump_buffer("buffer", handler->buffer, handler->buffer_len);
		
		handler->buffer_len = 0;
		return 0;
	}
	
	memcpy(&handler->buffer[handler->buffer_len], rbuf, rbuf_len);
	handler->buffer_len += rbuf_len;
	
repeat:
	
	if (KLOG_LV >= 3) {
		g_print("\n");	
		_dump_buffer("buffer", handler->buffer, handler->buffer_len);
	}

	__KLOG(3, ("<%s:%d> buffer_index: %d, buffer_len: %d\n",__FUNCTION__,__LINE__,buffer_index,handler->buffer_len));
		
	/* 1. obtain address - |0x1B|addr| */
	addr = 0xff;
	for (i=buffer_index; i < handler->buffer_len; i++) {
		if (handler->buffer[i] == AVE_ESC) {
			if (i+1 < handler->buffer_len) {
				addr = handler->buffer[i+1];
				buffer_index = i+2;
				break;
			}
			else {	// found ESC but no address
				return 0;	// need more data
			}
		}
	}
//	__KLOG(3, ("<%s:%d> addr: %d, buffer_index: %d\n",__FUNCTION__,__LINE__,
//		addr,handler->buffer_index));
	
	if (addr == 0xff) {	// error 
		__KLOG(0, ("<%s:%d> error - addr = 0xff !!!, buffer_index: %d, buffer_len: %d\n",__FUNCTION__,__LINE__,
			buffer_index,handler->buffer_len));
		_dump_buffer("buffer", handler->buffer, handler->buffer_len);
		return 0;
	}

	/* 2. get channel corresponding to addr  */
	chan = _pos_ave_get_chan(handler, addr, sysdb_pos);	// get channel corresponding to addr
	if (chan < 0) {	// error - throw out buffer
		__KLOG(0, ("<%s:%d> error - can't find channel !!! (addr: %d)\n",__FUNCTION__,__LINE__,addr));
		handler->buffer_len = 0;
		return 0;
	}

	if (buffer_index+1 >= handler->buffer_len) {	// need more data to parse
		return 0;
	}
	
	/* 3. make one command */
	len = _pos_ave_cmd_token(handler->buffer, handler->buffer_len, buffer_index, &handler->step[chan]);

	if (len == 0) {	// need more data
		return 0;
	}
	else if (len < 0) {	// error command
		__KLOG(0, ("<%s:%d> error - can't find command token\n",__FUNCTION__,__LINE__));
		_dump_buffer("buffer", handler->buffer, handler->buffer_len);
		goto copy;
	}
	else if (len >= sizeof(cmd_buffer)) {
		__KLOG(0, ("<%s:%d> error - cmd_buffer overflow !!!\n",__FUNCTION__,__LINE__));
		_dump_buffer("cmd_buffer", cmd_buffer, sizeof(cmd_buffer));		
	}

	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	memcpy(cmd_buffer, &handler->buffer[buffer_index], len);
	buffer_index += len;

	if (KLOG_LV >= 3) {
		g_print("chan: %d, addr: %d, buffer_index: %d, ", chan, addr, buffer_index);	
		_dump_buffer("command", cmd_buffer, len);
	}

	/* 4. lexical analyzing cmd buffer */
	handler->step[chan] = _pos_ave_cmd_analyze(cmd_buffer, len, handler->step[chan], &handler->command[chan]);

	if (handler->step[chan] == NF_POS_AVE_STEP_DATETIME) {	// analyze error
		__KLOG(0, ("<%s:%d> analyze error !!!, buffer_index: %d, len: %d\n",__FUNCTION__,__LINE__,buffer_index, len));
		if (KLOG_LV >= 1)
			_pos_ave_cmd_print(&handler->command[chan]);
		
		_pos_ave_command_init(&handler->command[chan], &handler->step[chan]);
	}

	if (handler->step[chan] == NF_POS_AVE_STEP_NR) {	// parsing complete
		_pos_ave_put_eventlog(&handler->command[chan]);		// record pos eventlog

		handler->command[chan].addr = sysdb_pos->addr[chan];		// pos event 처리 끝날때마다 address 갱신
		_pos_ave_command_init(&handler->command[chan], &handler->step[chan]);
		
//		KLOG_LV = 1;	
	}

copy:
	/* 5. copy buffer */
	len = handler->buffer_len-buffer_index;
	if (len > 0) memcpy(handler->buffer, &handler->buffer[buffer_index], len);

	handler->buffer_len = len;
	buffer_index = 0;

	if (buffer_index < handler->buffer_len) {
		__KLOG(3, ("<%s:%d> goto repeat, buffer_index: %d, buffer_len: %d\n", __FUNCTION__,__LINE__,
			buffer_index,handler->buffer_len));
		goto repeat;
	}

	return 1;
}

extern int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

static int pos_rec = 0;

static guint _pos_ave_process_command(NfPos *self)
{		
	static int fd = -1;
	int ret = 0, get_baudrate = 0;
	static guint baudrate = 0;

	fd_set rxset;
	int mx, n;
	struct timeval tv;
	int timeout_sec = 2;

	NF_POS_SYSDB		*sysdb_pos = &self->sysdb_pos;

	guchar buf[MAX_AVE_BUFFER];

#if 1	/* log test */
	if (pos_rec) {
		guchar buf2[] = 
{
0x1b, 0x01, 0x00, 0x44, 0x61, 0x74, 0x65, 0x20, 0x2d, 0x20, 0x31, 0x30, 0x2f, 0x30, 0x35, 0x2f, 0x31, 0x30, 0x20, 0x54, 0x69, 0x6d, 0x65, 0x20, 0x2d, 0x20, 0x31, 0x31, 0x3a, 0x35, 0x31, 0x3a, 0x31, 0x30, 0x0a, 
0x1b, 0x01, 0x40, 0x43, 0x61, 0x6d, 0x65, 0x72, 0x61, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x37, 0x20, 0x30, 0x39, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x36, 0x33, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x42, 0x65, 0x65, 0x72, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x33, 0x20, 0x31, 0x30, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x33, 0x30, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x4e, 0x6f, 0x6f, 0x64, 0x6c, 0x65, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x31, 0x20, 0x30, 0x34, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x30, 0x34, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x4c, 0x69, 0x6e, 0x75, 0x78, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x30, 0x20, 0x31, 0x34, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x44, 0x72, 0x61, 0x67, 0x6f, 0x6e, 0x66, 0x6c, 0x79, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x33, 0x31, 0x20, 0x30, 0x38, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x32, 0x34, 0x38, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x44, 0x56, 0x52, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x31, 0x32, 0x20, 0x31, 0x31, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x31, 0x33, 0x32, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x43, 0x6f, 0x6b, 0x65, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x32, 0x20, 0x30, 0x32, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x30, 0x34, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x55, 0x53, 0x42, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x39, 0x20, 0x31, 0x31, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x39, 0x39, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x40, 0x48, 0x61, 0x6d, 0x62, 0x75, 0x72, 0x67, 0x65, 0x72, 0x20, 
0x1b, 0x01, 0x00, 0x20, 0x24, 0x30, 0x34, 0x20, 0x31, 0x32, 0x20, 0x20, 0x2d, 0x20, 0x30, 0x30, 0x30, 0x34, 0x38, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x30, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x30, 0x30, 0x30, 0x36, 0x32, 0x38, 0x20, 0x64, 0x6f, 0x6c, 0x6c, 0x61, 0x72, 0x0a, 
0x1b, 0x01, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 
};	
/*
Date - 10/05/10 Time - 11:51:10
@Camera     $07 09  - 00063 dollar
@Beer       $03 10  - 00030 dollar
@Noodle     $01 04  - 00004 dollar
@Linux      $00 14  - 00000 dollar
@Dragonfly  $31 08  - 00248 dollar
@DVR        $12 11  - 00132 dollar
@Coke       $02 02  - 00004 dollar
@USB        $09 11  - 00099 dollar
@Hamburger  $04 12  - 00048 dollar
0Total              0000628 dollar
---------------------------
*/
		ret = sizeof(buf2);
		_pos_ave_parse_command(&_pos_ave_handler, buf2, ret, sysdb_pos);

		usleep(pos_rec*1000);

		return ret;
	}
#endif

#if POS_DUMP
	static FILE *fp = NULL;
	if (fp == NULL) {
		fp = fopen("pos_log.txt", "w");
		if (fp == NULL) g_print("error - pos_log.txt open fail\n");
		else g_print("pos_log.txt opened\n");
	}
#endif

#if defined (_ANF_1648) || defined(_ANF_0824)
	if (fd < 0) fd = open(NF_POS_UART2_DEV_NAME, O_RDWR);
#else
	if (fd < 0) 
	{
		fd = open(NF_POS_DEV_NAME, O_RDWR);

#ifdef TTYS1_DEV_ACT
		termios_act_init(fd, 9600, 8, 0, 1);
#endif
	}
#endif

	if (fd < 0)
	{
#if defined (_ANF_1648) || defined(_ANF_0824)
		g_warning("<%s> error open [%s] ret[%d]",__FUNCTION__, 
					NF_POS_UART2_DEV_NAME, fd);
#else
		g_warning("<%s> error open [%s] ret[%d]",__FUNCTION__, 
					NF_POS_DEV_NAME, fd);
#endif
		return 0;
	}

#ifdef TTYS1_DEV_ACT
	//termios_act_init(fd, sysdb_pos->baud, 8, 0, 1);
	//termios_act_init(fd, 9600, 8, 0, 1);
#else
	#if defined (_ANF_1648) || defined(_ANF_0824)
	if (!baudrate) {
		ret = ioctl(fd, FPGA_RS485_UART2_IOCRL_GET_BAUDRATE, &baudrate);
		get_baudrate = 1;
	}
	#else
	if (!baudrate) {
		ret = ioctl(fd, FPGA_RS485_IOCRL_GET_BAUDRATE, &baudrate);
		get_baudrate = 1;
	}
	#endif
	
	if (ret == -1)
	{
		g_warning("GET_BAUDRATE Ioctl Error");
		
		baudrate = 0;
		close(fd);
		fd = -1;
		
		return 0; 
	}

	if (get_baudrate)
	{
		 switch( baudrate )
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
				g_warning("<%s> baudrate range over baudrate[%d]", __FUNCTION__, baudrate);
				baudrate  = FPGA_RS485_BAUDRATE_9600;
				break;
		}
	}

	if (baudrate != sysdb_pos->baud)
	{
		__KLOG(0, ("<%s:%d> baudrate changing !!!\n",__FUNCTION__,__LINE__));
		
	#if defined (_ANF_1648) || defined(_ANF_0824)
		ret = ioctl(fd, FPGA_RS485_UART2_IOCRL_REG_INIT);
	#else
		ret = ioctl(fd, FPGA_RS485_IOCRL_REG_INIT);
	#endif
		if (ret == -1)
		{
			g_warning("REG_INIT Ioctl Error");
			close(fd);
			return 0; 
		}
		baudrate = sysdb_pos->baud;
		
		if ( baudrate < 1 || baudrate > 8 )
		{
			g_warning("<%s> baudrate rate over baudrate[%d]", __FUNCTION__, baudrate );
			close(fd);
			fd = -1;
			baudrate = 0;
			self->sysdb_reload = 1;
			return 0;
		}
	#if defined (_ANF_1648) || defined(_ANF_0824)
		ret = ioctl(fd, FPGA_RS485_UART2_IOCRL_SET_BAUDRATE, &baudrate);
	#else
		ret = ioctl(fd, FPGA_RS485_IOCRL_SET_BAUDRATE, &baudrate);
	#endif

		if (ret == -1)
		{
			g_warning("SET_BAUDRATE Ioctl Error");
			baudrate = 0;
			close(fd);
			fd = -1;
			return 0; 
		}
	}
#endif

	/* select && read */

	FD_ZERO(&rxset);

	FD_SET(fd, &rxset);

	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0; 

	mx = fd;

	n = Select(mx+1 , &rxset, 0, 0, &tv);

	if (n == -1)
	{
		g_warning("<%s> select() error", __FUNCTION__);
		close(fd);
		fd = -1;
		return 0;
	}
	else if(n == 0)
	{
//		close(fd);
		return 0;
	}

	if (FD_ISSET(fd, &rxset))
	{

		ret = read(fd, buf, sizeof(buf));

		if (ret > 0)
		{
#if POS_DUMP
			fwrite(buf, ret, 1, fp);
			fprintf(fp, "\n+++++++++++++++++++++++++++++++++++++\n");
			fflush(fp);
#endif
			if (KLOG_LV >= 3) {	/* dump buffer for debug */			
				int i;				
				g_print("\npos read[%d] :\n", ret);
				for (i=0; i < ret; i++) g_print("%c", (unsigned char)buf[i]);			
				#if 1
				g_print("\nhex :\n");
				
				for (i=0; i < ret; i++) {
					g_print("0x%02x ", buf[i]);
					if ((i+1)%10 == 0) g_print("\n");				
				}
				g_print("\n");
				#endif				
			}			

			if (!self->pos_disable)
				_pos_ave_parse_command(&_pos_ave_handler, buf, ret, sysdb_pos);
			else		// FIXME !!!	---kimdh
				g_usleep(1000000);

		}
	}

//	close(fd);

	return ret;	
}

NF_POS_DECODE	_nf_pos_proto_ave = {
	.idx = 0,
	.proto_name = "AVE",
	.func_process_command = _pos_ave_process_command
};

static char pos_test_rec_help[] = "pos_test_rec";
static int pos_test_rec(int argc, char **argv)
{
	if (argc < 2) return 0;

	pos_rec = atoi(argv[1]);
	
	return 0;
}
__commandlist(pos_test_rec, "pos_test_rec", pos_test_rec_help, pos_test_rec_help);

#endif
