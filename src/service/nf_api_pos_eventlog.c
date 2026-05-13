#include "nf_sysdb.h"
#include "nf_api_pos_eventlog.h"
#include "jbshell.h"


#define MAX_LOG_REQUEST	30

extern int KLOG_LV;

static void _init_pos_eventlog_param(NF_POS_LOG_PARAM *param)
{
	memset(&param->log_param, 0, sizeof(NF_LOG_PARAM));
	memset(param->log_param.param1_mask, 0xff, sizeof(param->log_param.param1_mask));

	param->log_param.type_mask = LT_MASK_SYSTEM_POS;
}

static void _print_pos_eventlog(NF_POS_LOG_DATA *pos_log)
{	
	GTimeVal tv;
	struct tm stime;
	gchar strTime[64] = {0};

	GUINT64_TO_GTIMEVAL(pos_log->timestamp, tv);
	localtime_r(&tv.tv_sec, &stime);
	strftime(strTime, sizeof(stime), "%y/%m/%d - %H:%M:%S", &stime); 	
	
	g_print("[pos_log] %s, ch: %d, addr: %d, log_type: %d\n", strTime, pos_log->chan, pos_log->addr, pos_log->pos_log_type);
	if (pos_log->pos_log_type == NF_POS_LOG_TYPE_ITEM) {
		g_print("[Item] name: %s, price: %05.02f, qty: %02d, sum: %08.02f\n", 
			pos_log->Item.name, pos_log->Item.price/100.0f, pos_log->Item.qty, pos_log->Item.sum/100.0f);
	}
	else if (pos_log->pos_log_type == NF_POS_LOG_TYPE_TOTAL) {
		g_print("[Total] total: %010.02f\n", pos_log->Total.total/100.0f);
		g_print("----------------------\n");
	}
	else {
		g_print("<%s:%d> ERROR - invalid pos log type : %d\n",__FUNCTION__,__LINE__,pos_log->pos_log_type);
	}
	g_print("\n");
}

/**
	@brief		POS 이벤트 로그 조회된 결과에서 조건검색에 따라 필터링
	@param[in]	param	이벤트 로그 조회 파라메터
	@param[in]	pos_log	이벤트 로그 조회할 데이터
	@return gboolean %TRUE on filtered, %FALSE if on not filtered
*/
static gboolean _pos_eventlog_filter(NF_POS_LOG_PARAM *param, NF_POS_LOG_DATA *pos_log)
{
	gchar *cptr[MAX_STR_NUM_CNT] = {NULL, };
	int i;
	int str_cnt = 0;
	
	g_return_val_if_fail(param != NULL, FALSE);
	g_return_val_if_fail(pos_log != NULL, FALSE);

	
	/* 1. Condition Search */
	if (param->search_cond != NF_POS_LOG_SEARCH_COND_NA && 
//		((param->range_from != 0 && param->range_to != 0) || (param->range_from == 0 && param->range_to > 0)) &&
		(param->range_from <= param->range_to))
	{
		if (pos_log->pos_log_type == NF_POS_LOG_TYPE_ITEM) {
			switch (param->search_cond) {
				case NF_POS_LOG_SEARCH_COND_PRICE:
					if (pos_log->Item.price < param->range_from || pos_log->Item.price > param->range_to)
						return FALSE;
					break;
				case NF_POS_LOG_SEARCH_COND_QTY:
					if (pos_log->Item.qty < param->range_from || pos_log->Item.qty > param->range_to)
						return FALSE;
					break;
				case NF_POS_LOG_SEARCH_COND_SUM:
					if (pos_log->Item.sum < param->range_from || pos_log->Item.sum > param->range_to)
						return FALSE;
					break;
				case NF_POS_LOG_SEARCH_COND_TOTAL:
					return FALSE;
					break;
				default:
					g_print("[%s] error - invalid search_cond: %d\n", __FUNCTION__, param->search_cond);
					return FALSE;
					break;
			}
		}		
		else if (pos_log->pos_log_type == NF_POS_LOG_TYPE_TOTAL) {
			if (param->search_cond == NF_POS_LOG_SEARCH_COND_TOTAL) {
				if (pos_log->Total.total < param->range_from || pos_log->Total.total > param->range_to)
					return FALSE;
			}
			else return FALSE;
		}
		else {
			g_print("[%s] error - invalid pos log type: %d\n", __FUNCTION__, pos_log->pos_log_type);
			return FALSE;
		}
	}

	/* 2. Keyword Search */

	if(!param->search_str_len[0] || !param->search_str[0][0]) return TRUE;

	if (pos_log->pos_log_type == NF_POS_LOG_TYPE_TOTAL) return FALSE;	// keyword search에서 total은 제외

	for(i=0; i< MAX_STR_NUM_CNT; i++)
	{	
		if( param->search_str_len[i] > 0 )
		{
			str_cnt++;
			
			if (param->match_case) cptr[i] = strstr(pos_log->Item.name, param->search_str[i]);
			else cptr[i] = strcasestr(pos_log->Item.name, param->search_str[i]);

			if( cptr[i] )
			{
				if (param->match_whole)
				{
					gboolean not_match = FALSE;
					
					/* cptr 앞뒤로 알파벳 있으면 not match */
					if (cptr[i]-pos_log->Item.name > 0) {
						if (g_ascii_isalpha(*(cptr[i]-1)) || g_ascii_isdigit(*(cptr[i]-1)))
							not_match = TRUE;
					}

					if( not_match == FALSE ){						
						if ( cptr[i] + (param->search_str_len[i] ) != NULL ){
							if (g_ascii_isalpha(*(cptr[i]+param->search_str_len[i])) || g_ascii_isdigit(*(cptr[i]+param->search_str_len[i])))
								not_match = TRUE;
						}						
					}

					if( not_match == TRUE )
						cptr[i] = NULL;
				}
			}
		}
		else
			break;
	}

	if( str_cnt == 1 )
	{
		if( cptr[0] )
			return TRUE;
		else
			return FALSE;
	}
	else if( str_cnt == 2 )
	{
		if( param->operation[0] == 0 ){
			if( cptr[0] && cptr[1] )
				return TRUE;
			else
				return FALSE;
		}
		else{
			if( cptr[0] || cptr[1] )
				return TRUE;
			else
				return FALSE;
		}
	}
	else if( str_cnt == 3 )
	{
		if( param->operation[0] == 0 && param->operation[1] == 0 ){
			if( cptr[0] && cptr[1] && cptr[2] )
				return TRUE;
			else
				return FALSE;
		}
		else if( param->operation[0] == 0 && param->operation[1] == 1 ){
			if( ( cptr[0] && cptr[1] ) || cptr[2] )
				return TRUE;
			else
				return FALSE;
		}
		else if( param->operation[0] == 1 && param->operation[1] == 0 ){
			if( ( cptr[0] || cptr[1] ) && cptr[2] )
				return TRUE;
			else
				return FALSE;
		}
		else{ // ( param->operation[0] == 1 && param->operation[1] == 1 )
			if( cptr[0] || cptr[1] || cptr[2] )
				return TRUE;
			else
				return FALSE;
		}
	}

	return TRUE;	
}

/**
	@brief		POS 이벤트 로그 입력
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_pos_eventlog_put_text(gint ch, char *str, char *param1, char *param2) 
{
	
	GTimeVal tv;
	char buff[256];		

	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)buff;

	memset(	log_data, 0x00, sizeof(NF_POS_LOG_DATA));
			
	g_get_current_time( &tv);
		
	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = ch;
	log_data->addr = ch;
	log_data->pos_log_type = NF_POS_LOG_TYPE_TEXT;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;
	
	snprintf( log_data->Text.text, sizeof(log_data->Text.text)-1, "%s", str );
	snprintf( log_data->Text.param1, sizeof(log_data->Text.param1), "%s", param1 );
	snprintf( log_data->Text.param2, sizeof(log_data->Text.param2), "%s", param2 );
	
	return nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, buff);
}

#if 0
static int _pos_logput_text( gint ch, char *str, char *param1, char *param2) 
{
	
	GTimeVal tv;
	char buff[256];		
	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)buff;

#ifdef DEBUG_POS_PUTLOG
	g_message("@@@@@@@  ch[%2d] text[%s]  [%s][%s]", ch, str, param1, param2);
#endif	
	memset(	log_data, 0x00, sizeof(NF_POS_LOG_DATA));
			
	g_get_current_time( &tv);
		
	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = ch;
	log_data->addr = ch;
	log_data->pos_log_type = NF_POS_LOG_TYPE_TEXT;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;
	
	snprintf( log_data->Text.text, sizeof(log_data->Text.text)-1, "%s", str );
	snprintf( log_data->Text.param1, sizeof(log_data->Text.param1), "%s", param1 );
	snprintf( log_data->Text.param2, sizeof(log_data->Text.param2), "%s", param2 );

#ifndef DEBUG_POS_DONTPUT_LOG		
	nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, buff);
#endif	
	
	return 1;
}
		
static int _pos_logput_item( gint ch, char *item, gfloat price, gfloat qty, gfloat sum) 
{
	
	GTimeVal tv;	
	char buff[256];		
	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)buff;
#ifdef DEBUG_POS_PUTLOG	
	g_message("@@@@@@@  ch[%2d] item[%s] price[%.2f] qty[%.2f] sum[%.2f]", ch, item, price, qty, sum);
#endif	
	memset(	log_data, 0x00, sizeof(NF_POS_LOG_DATA));
			
	g_get_current_time( &tv);
		
	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = ch;
	log_data->addr = ch;
	log_data->pos_log_type = NF_POS_LOG_TYPE_ITEM;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;

	snprintf( log_data->Item.name, sizeof(log_data->Item.name)-1, "%s", item );
		
	log_data->Item.price = (gint)(price*100.0);
	log_data->Item.qty = (gint)(qty*100.0);
	log_data->Item.sum = (gint)(sum*100.0);

#ifndef DEBUG_POS_DONTPUT_LOG	
	nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, buff);	
#endif 	
	
	return 1;
}


static int _pos_logput_total( gint ch, gfloat sum) 
{
	
	GTimeVal tv;	
	char buff[256];		
	NF_POS_LOG_DATA *log_data = (NF_POS_LOG_DATA *)buff;
#ifdef DEBUG_POS_PUTLOG	
	g_message("@@@@@@@  ch[%2d] TOTAL sum[%.2f]", ch, sum);
#endif	
	memset(	log_data, 0x00, sizeof(NF_POS_LOG_DATA));
			
	g_get_current_time( &tv);
		
	log_data->timestamp = GTIMEVAL_TO_GUINT64(tv);
	log_data->chan = ch;
	log_data->addr = ch;
	log_data->pos_log_type = NF_POS_LOG_TYPE_TOTAL;
	log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;
			
	log_data->Total.total = (gint)(sum*100.0);

#ifndef DEBUG_POS_DONTPUT_LOG
	nf_eventlog_put_param(&tv, LT_SYSTEM_POS, ch, 0, buff);	
#endif
	
	return 1;
}

#endif
/**
	@brief		POS 이벤트 로그 조회
	@param[in]	param	이벤트 로그 조회 파라메터
	@param[out]	header	이벤트 로그 조회 결과 헤더
	@param[out[	pos_log	이벤트 로그 조회 결과 리턴할 곳
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_pos_eventlog_get(NF_POS_LOG_PARAM *param, NF_POS_LOG_RESULT_HEADER *header, NF_POS_LOG_DATA *pos_log)
{
	g_return_val_if_fail(param != NULL, FALSE);
	g_return_val_if_fail(header != NULL, FALSE);
	g_return_val_if_fail(pos_log != NULL, FALSE);

	g_return_val_if_fail(param->log_param.request_count <= MAX_LOG_REQUEST, FALSE);

	int i, ch, curr = 0, search = 0;
	int request_count = param->log_param.request_count;
	gboolean res;
	guint64	log_id_last;

	// for test
	GTimeVal tv;
	struct tm *stime;	

	NF_POS_LOG_DATA *_pos_log;	
	static NF_LOG_DATA log_data[MAX_LOG_REQUEST];

	param->log_param.request_count = MAX_LOG_REQUEST;

	for(i=0; i<MAX_STR_NUM_CNT; i++){
		param->search_str_len[i] = strlen(param->search_str[i]);
	}

	// for debug
	if (KLOG_LV >= 1) {
		g_print("[%s] direction: %s\n", __FUNCTION__, param->log_param.direction == NF_LOG_PARAM_DIR_BACKWARD ? "backward" : "forward");
		g_print("[%s] param->param1_mask: ", __FUNCTION__);
		for (i=0; i < 4; i++) g_print("0x%02x ", (guchar)param->log_param.param1_mask[LOG_P1_MASKOFS_SYSTEM_POS+i]);
		g_print("\n");
		g_print("[%s] param->search_cond: %d\n", __FUNCTION__, param->search_cond);
		g_print("[%s] param->range_from: %d, param->range_to: %d\n", __FUNCTION__, param->range_from, param->range_to);
		for (i=0; i<MAX_STR_NUM_CNT; i++) g_print("[%s] param->search_str[%d]: %s\n", __FUNCTION__, i, param->search_str[i]);
		for (i=0; i<MAX_STR_OPER_CNT; i++) g_print("[%s] param->operation[%d]: %d\n", __FUNCTION__, i, param->operation[i]);
		g_print("[%s] param->log_param.log_id: %u\n", param->log_param.log_id);

		stime = localtime(&param->log_param.time_begin.tv_sec);
		g_print("[%s] param->time_begin :: Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n", __FUNCTION__,
				stime->tm_year+1900, stime->tm_mon+1, stime->tm_mday,
				stime->tm_hour, stime->tm_min, stime->tm_sec);

		stime = localtime(&param->log_param.time_end.tv_sec);
		g_print("[%s] param->time_end :: Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n", __FUNCTION__,
				stime->tm_year+1900, stime->tm_mon+1, stime->tm_mday,
				stime->tm_hour, stime->tm_min, stime->tm_sec);

		stime = localtime(&param->log_param.time_search.tv_sec);
		g_print("[%s] param->time_search :: Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n", __FUNCTION__,
				stime->tm_year+1900, stime->tm_mon+1, stime->tm_mday,
				stime->tm_hour, stime->tm_min, stime->tm_sec);		
	}	
	
repeat:
	res = nf_eventlog_get(&param->log_param, &header->log_result_header, log_data, NULL);

	if (KLOG_LV >= 1) {
		g_print("[%s] request_count: %d, result_count: %d\n", 
			__FUNCTION__, param->log_param.request_count, header->log_result_header.result_count);
	}

	if (!res) {
		if (KLOG_LV >= 1) g_print("[%s] res is false !!!, goto exit\n", __FUNCTION__);
		goto exit;
	}
	if (header->log_result_header.result_count <= 0) {		// no more log data
		if (KLOG_LV >= 1) g_print("[%s] result_count: %d, goto exit\n", __FUNCTION__, header->log_result_header.result_count);
		goto exit;
	}

	if (!search) {		
		header->log_id_first = log_data[0].log_id;
		search = 1;
	}

	for (i=0; i < header->log_result_header.result_count; i++) {
		ch = log_data[i].param1;
		_pos_log = (NF_POS_LOG_DATA *)log_data[i].text;
		_pos_log->timestamp = log_data[i].timestamp;		// log timestamp != pos log timestamp
		_pos_log->log_id = log_data[i].log_id;

		if (!_pos_eventlog_filter(param, _pos_log)) continue;

		memcpy(&pos_log[curr++], _pos_log, sizeof(NF_POS_LOG_DATA));

		if (KLOG_LV >= 1) {
#if 1
			GUINT64_TO_GTIMEVAL(log_data[i].timestamp, tv);
			stime = localtime(&tv.tv_sec);

			g_print("log_data[%d].type: %d\n", i, log_data[i].type);
			g_print("log_data[%d].timestamp: %lld\n", i, log_data[i].timestamp);
			g_print("Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n",
					stime->tm_year+1900, stime->tm_mon+1, stime->tm_mday,
					stime->tm_hour, stime->tm_min, stime->tm_sec);		
			g_print("log_data[%d].text: %s\n", i, log_data[i].text);
			g_print("----------------------------------\n");
#endif					
			g_print("[%s]  pos log found - curr: %d, request_count: %d\n", __FUNCTION__, curr, request_count);
		}

		if (curr == request_count) {
			header->log_id_last = log_data[i].log_id;
			header->log_id_last_prev = log_id_last;
			if (KLOG_LV >= 1) g_print("[%s] curr: %d, request_count: %d, goto exit\n", __FUNCTION__, curr, request_count);
			goto exit;
		}
		else {
			log_id_last = log_data[i].log_id;
		}
	}

	if (curr < request_count) {
		param->log_param.mode = NF_LOG_PARAM_MODE_LOGID;
		if (KLOG_LV >= 1) g_print("### header.result_count: %d, curr: %d\n", header->log_result_header.result_count, curr);
		param->log_param.log_id = log_data[header->log_result_header.result_count-1].log_id;
		goto repeat;
	}	
	
exit:
	param->log_param.request_count = request_count;
	header->log_result_header.result_count = curr;
	if (KLOG_LV >= 1) {
		g_print("[%s] param->log_param.request_count: %d\n",__FUNCTION__,request_count);
		g_print("[%s] param->log_result_header.result_count: %d\n",__FUNCTION__,curr);
	}
	return TRUE;
}

static char pos_debug_help[] = "pos_debug";
static int pos_debug(int argc, char **argv)
{
	if (argc < 2) return 0;

	KLOG_LV = atoi(argv[1]);

	g_print("[POS] debug level: %d\n", KLOG_LV);
	
	return 0;
}
__commandlist(pos_debug, "pos_debug", pos_debug_help, pos_debug_help);

static char pos_log_test_help[] = "pos_log_test";
static int pos_log_test(int argc, char **argv)
{	
	int test = 0;
	if (argc > 1) {
		test = atoi(argv[1]);
	}

	NF_POS_LOG_DATA *pos_log = NULL;
	pos_log = g_malloc0(sizeof(NF_POS_LOG_DATA)*MAX_LOG_REQUEST);

	NF_POS_LOG_PARAM param;
	NF_POS_LOG_RESULT_HEADER header;

	memset(&param, 0, sizeof(NF_POS_LOG_PARAM));

	_init_pos_eventlog_param(&param);

	g_get_current_time(&param.log_param.time_end);
	param.log_param.time_search = param.log_param.time_end;
	param.log_param.direction = NF_LOG_PARAM_DIR_BACKWARD;

	param.log_param.mode = NF_LOG_PARAM_MODE_TIME;
	param.log_param.log_id = -1;
	param.log_param.request_count = 14;

	if (test == 1) {
		/* page down/up test */
		int i, r;
		for (r=0; r < 3; r++) {
			nf_pos_eventlog_get(&param, &header, pos_log);
			g_print("[pos_log_test] header.log_result_header.result_count: %d\n", header.log_result_header.result_count);

			for (i=0; i < header.log_result_header.result_count; i++) {
				_print_pos_eventlog(&pos_log[i]);
			}

			param.log_param.mode = NF_LOG_PARAM_MODE_LOGID;
			param.log_param.log_id = header.log_id_last;

			g_print("\n--- page down ---\n");
		}
#if 1
		for (r=0; r < 2; r++) {		
			g_print("\n--- page up ---\n");

			param.log_param.direction = NF_LOG_PARAM_DIR_FORWARD;
			param.log_param.mode = NF_LOG_PARAM_MODE_LOGID;
			if (r == 0) param.log_param.log_id = header.log_id_first;
			else param.log_param.log_id = header.log_id_last;

			nf_pos_eventlog_get(&param, &header, pos_log);
			g_print("[pos_log_test] header.log_result_header.result_count: %d\n", header.log_result_header.result_count);

			for (i=header.log_result_header.result_count-1; i >= 0; i--) {
				_print_pos_eventlog(&pos_log[i]);
			}		
		}
#endif
		return 0;
#if 0		
		param.log_param.time_search = param.log_param.time_begin;
		param.log_param.direction = NF_LOG_PARAM_DIR_FORWARD;
#endif		
	}
	else if (test == 2) {
		param.search_cond = NF_POS_LOG_SEARCH_COND_PRICE;
		param.range_from = 0;
		param.range_to = 2*100;
	}
	else if (test == 3) {
		param.search_cond = NF_POS_LOG_SEARCH_COND_NA;
		strcpy(param.search_str, "INU");
		param.match_case = 0;
		param.match_whole = 0;
	}
	else if (test == 4) {
		nf_sysdb_lock(NF_SYSDB_CATE_SYS);
		
		GValue set_value = {0,};
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, 2400);
		nf_sysdb_set_key0("sys.pos.baud", &set_value, NULL);
		g_value_unset(&set_value);

		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		return 0;
	}
	else if (test == 5) {
		nf_sysdb_lock(NF_SYSDB_CATE_SYS);
		
		GValue set_value = {0,};
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, 9600);
		nf_sysdb_set_key0("sys.pos.baud", &set_value, NULL);
		g_value_unset(&set_value);

		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		return 0;
	}

	nf_pos_eventlog_get(&param, &header, pos_log);

	g_print("[pos_log_test] header.log_result_header.result_count: %d\n", header.log_result_header.result_count);

#if 1
	int i;
	for (i=0; i < header.log_result_header.result_count; i++) {
		_print_pos_eventlog(&pos_log[i]);
	}

#else	/* obsolete */
	int i;
	for (i=0; i < header.result_count; i++) {
		if (log_data[i].type == LT_SYSTEM_POS) {
			int ch = log_data[i].param2;
			NF_POS_LOG_DATA *pos_log = g_malloc0(sizeof(NF_POS_LOG_DATA));
			nf_pos_text2log(log_data[i].text, pos_log);			

			//g_print("----------------------\n");
			g_print("pos ch: %d\n", ch);
			g_print("pos log_type: %d\n", pos_log->pos_log_type);
			if (pos_log->pos_log_type == NF_POS_LOG_TYPE_ITEM) {
				g_print("[Item] name: %s, price: %u, qty: %u, sum: %u\n", 
					pos_log->Item.name, pos_log->Item.price, pos_log->Item.qty, pos_log->Item.sum);				
			}
			else if (pos_log->pos_log_type == NF_POS_LOG_TYPE_TOTAL) {
				g_print("[Total] %u\n", pos_log->Total.total);
			}
			g_print("----------------------\n");

			g_free(pos_log);
		}
	}
#endif

	g_free(pos_log);
	
	return 0;
}
__commandlist(pos_log_test, "pos_log_test", pos_log_test_help, pos_log_test_help);



/* obsolete */
/* NF_POS_LOG_DATA -> text 변환 */
gboolean nf_pos_log2text(NF_POS_LOG_DATA *log_data, char *text)
{
	g_return_val_if_fail(log_data != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	int ret = 0;

	if (log_data->pos_log_type == NF_POS_LOG_TYPE_ITEM) {
		ret = sprintf(text, "%s;%u;%u;%u", 
			log_data->Item.name, log_data->Item.price, log_data->Item.qty, log_data->Item.sum);
	}
	else if (log_data->pos_log_type == NF_POS_LOG_TYPE_TOTAL) {
		ret = sprintf(text, "Total;%u", log_data->Total.total);
	}
	else if (log_data->pos_log_type == NF_POS_LOG_TYPE_TEXT) {
		ret = sprintf(text, "%s", log_data->Text.text);	
	} else {
		g_warning("Error, invalid pos log type: %d\n", log_data->pos_log_type);
		return FALSE;
	}
		
	//printf("<%s> %s\n", __FUNCTION__, text);		
	return TRUE;
}

/* obsolete */
/* text -> NF_POS_LOG_DATA 변환 */
/* 주의 - thread safe 하지않음 */
gboolean nf_pos_text2log(char *text, NF_POS_LOG_DATA *log_data)
{
	char *rbuf;
	g_return_val_if_fail(log_data != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	char seps[] = ";", *token;
	char buf[256];

	memset(buf, 0, sizeof(buf));
	memcpy(buf, text, sizeof(buf));

	char *cptr = strstr(buf, "Total");
	
	if (!cptr ) {	// ITEM
		log_data->pos_log_type = NF_POS_LOG_TYPE_ITEM;
		log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;	// FIXME
		
		token = strtok_r(buf, seps, &rbuf);
		if (token == NULL) goto error;
		strcpy(log_data->Item.name, token);
		
		token = strtok_r(NULL, seps, &rbuf);
		if (token == NULL) goto error;
		log_data->Item.price = atoi(token);

		token = strtok_r(NULL, seps, &rbuf);
		if (token == NULL) goto error;
		log_data->Item.qty = atoi(token);

		token = strtok_r(NULL, seps, &rbuf);
		if (token == NULL) goto error;
		log_data->Item.sum = atoi(token);	
	}
	else {	// TOTAL
		log_data->pos_log_type = NF_POS_LOG_TYPE_TOTAL;
		log_data->pos_log_money = NF_POS_LOG_MONEY_DOLLAR;	// FIXME
	
		token = strtok_r(buf, seps, &rbuf);
		if (token == NULL) goto error;

		token = strtok_r(NULL, seps, &rbuf);
		if (token == NULL) goto error;
		log_data->Total.total = atoi(token);
	}
#if 0
	if (!cptr) {
		printf("[Item] name: %s, price: %u, qty: %u, sum: %u\n", 
			log_data->Item.name, log_data->Item.price, log_data->Item.qty, log_data->Item.sum);
	}
	else {
		printf("[Total] %u\n", log_data->Total.total);
	}
#endif
	return TRUE;	

error:
	g_print("<%s:%d> Error, can't convert text to pos_log (text: %s)\n", __FUNCTION__, __LINE__, text);
	return FALSE;
}


