#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include "nf_sysdb.h"
#include "nf_api_dva_eventlog.h"
#include "jbshell.h"
#include "itx_ai_def.h"


#define MAX_LOG_REQUEST	30

extern int KLOG_LV;

static void _init_dva_eventlog_param(NF_DVA_LOG_PARAM *param)
{
	memset(&param->log_param, 0, sizeof(NF_LOG_PARAM));
	memset(param->log_param.param1_mask, 0xff, sizeof(param->log_param.param1_mask));

	param->log_param.type_mask = LT_MASK_DVA;
}

static void _print_dva_eventlog(NF_DVA_LOG_DATA *pos_log)
{	
#if 0
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
#endif
}

void 
convert_ai_log_generic(ai_rule_event_t* log_text)
{
	ai_generic_log_t ai_generic_log;
	char tmp[256];

	ai_generic_log.type = IVCA_ET_GENERIC;
	ai_generic_log.ch = log_text->ch;
	ai_generic_log.event_area[0].x = log_text->bbx_position[0];
	ai_generic_log.event_area[0].y = log_text->bbx_position[1];
	ai_generic_log.event_area[1].x = log_text->bbx_position[2];
	ai_generic_log.event_area[1].y = log_text->bbx_position[3];
	ai_generic_log.timestamp = log_text->timestamp;
	ai_generic_log.timestampl = log_text->timestampl;

	memset(ai_generic_log.caption,0x00,sizeof(ai_generic_log.caption));
	memset(ai_generic_log.title,0x00,sizeof(ai_generic_log.title));
	memset(ai_generic_log.description,0x00,sizeof(ai_generic_log.description));

	switch(log_text->type){
		case IVCA_ET_DIR_POS:
		case IVCA_ET_DIR_NEG:
			strcpy(ai_generic_log.caption ,  "Line Crossing");
			break;
		case IVCA_ET_ENTER:
		case IVCA_ET_EXIT:
			strcpy(ai_generic_log.caption ,  "Enter/Exit");
			break;
		case IVCA_ET_STOPPED:
			strcpy(ai_generic_log.caption ,  "Stopping");
			break;
		case IVCA_ET_REMOVED:
			strcpy(ai_generic_log.caption ,  "Removed");
			break;
		case IVCA_ET_LOITERED:
			strcpy(ai_generic_log.caption ,  "Loitering");
			break;
		case IVCA_ET_INTRUSION:
			strcpy(ai_generic_log.caption ,  "Intrusion");
			break;
	}
	
	sprintf(tmp, "class : %s",log_text->object_class);
	strcpy(ai_generic_log.title , tmp);

	memcpy(log_text, &ai_generic_log, 256);
}

/**
	@brief		POS �̺�Ʈ �α� ��ȸ�� ������� ���ǰ˻��� ���� ���͸�
	@param[in]	param	�̺�Ʈ �α� ��ȸ �Ķ����
	@param[in]	pos_log	�̺�Ʈ �α� ��ȸ�� ������
	@return gboolean %TRUE on filtered, %FALSE if on not filtered
*/
static gboolean _dva_eventlog_filter(NF_DVA_LOG_PARAM *param, NF_LOG_DATA *log_data)
{
	gchar *obj_str = NULL;
	int i;
	gchar *cptr[MAX_STR_NUM_CNT] = {NULL, };
	int str_cnt = 0;
	gchar tmp[180];
	
	NF_DVA_LOG_DATA *dva_log;
	ai_rule_event_t* ai_log;
	ai_generic_log_t* ai_generic_log;
	
	int dva_type, dva_type_mask;
	
	g_return_val_if_fail(param != NULL, FALSE);
	g_return_val_if_fail(log_data != NULL, FALSE);

	dva_type = log_data->param2;
	dva_type_mask = param->dva_type_mask;
	if (!(dva_type_mask & (1 << dva_type)))
		return FALSE;
	

	 if(dva_type == DVA_AI_DETECTION || dva_type == DVA_AI_GENERIC){
		if(dva_type == DVA_AI_DETECTION){
			convert_ai_log_generic((ai_rule_event_t *)log_data->text);
		}
		
		ai_generic_log = (ai_generic_event_t *)log_data->text;	

		// caption com
		if(param->caption_list_len){
			if(strcasestr(param->caption_list, ai_generic_log->caption) == NULL)
				return FALSE;
		}
		else return FALSE;

		sprintf(tmp,"%s %s",ai_generic_log->title,ai_generic_log->description);

		for(i=0; i< MAX_STR_NUM_CNT; i++)
		{	
			if( param->search_str_len[i] > 0 )
			{
				str_cnt++;
				
				if (param->match_case) cptr[i] = strstr(tmp, param->search_str[i]);
				else cptr[i] = strcasestr(tmp, param->search_str[i]);

				if( cptr[i] )
				{
					if (param->match_whole)
					{
						gboolean not_match = FALSE;
						
						/* cptr �յڷ� ���ĺ� ������ not match */
						if (cptr[i]-tmp > 0) {
							//if (g_ascii_isalpha(*(cptr[i]-1)) || g_ascii_isdigit(*(cptr[i]-1)))
							//	not_match = TRUE;
							if (!isspace(*(cptr[i]-1))) not_match = TRUE;
						}

						if( not_match == FALSE ){						
							if ( cptr[i] + (param->search_str_len[i] ) != NULL ){
								//if (g_ascii_isalpha(*(cptr[i]+param->search_str_len[i])) || g_ascii_isdigit(*(cptr[i]+param->search_str_len[i])))
								//	not_match = TRUE;
								if (!isspace(*(cptr[i]+param->search_str_len[i]))) not_match = TRUE;
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
	else{
		
		/* Keyword Search */
		if(!param->search_str_len[0] || !param->search_str[0][0]) return FALSE;
	
		dva_log = (NF_DVA_LOG_DATA *)log_data->text;	
		switch(dva_type)
		{
		case DVA_INTRUSION_DETECTION:
			obj_str = dva_log->intrusion_detection.name;
			break;
		case DVA_ILLEGAL_PARKING:
			obj_str = dva_log->illegal_parking.name;
			break;
		case DVA_LPR:
			obj_str = dva_log->lpr.name;
			break;
		case DVA_COUNTER:
			obj_str = dva_log->counter.name;
			break;
		default:
			return FALSE;
		}
	}
	
	if (strcasestr(param->search_str[0], obj_str))
		return TRUE;
		
	return FALSE;

#if 0
	for(i=0; i < MAX_STR_NUM_CNT; i++)
	{	
		if( param->search_str_len[i] > 0 )
		{
			if (param->match_case) cptr[i] = strstr(obj_str, param->search_str[i]);
			else cptr[i] = strcasestr(obj_str, param->search_str[i]);

			if( cptr[i] )
			{
				if (param->match_whole)
				{
					gboolean not_match = FALSE;
					
					/* cptr �յڷ� ���ĺ� ������ not match */
					if (cptr[i]-obj_str > 0) {
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


	if( cptr[0] )
		return TRUE;
	else
		return FALSE;
#endif

	return TRUE;	
}

/**
	@brief		POS �̺�Ʈ �α� �Է�
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dva_eventlog_put_text(gint ch, char *str, char *param1, char *param2) 
{
#if 0
	GTimeVal tv;
	char buff[256];		

	NF_DVA_LOG_DATA *log_data = (NF_DVA_LOG_DATA *)buff;

	memset(	log_data, 0x00, sizeof(NF_DVA_LOG_DATA));
			
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
#endif
	return TRUE;
}

#if 0
static int _pos_logput_text( gint ch, char *str, char *param1, char *param2) 
{
	
	GTimeVal tv;
	char buff[256];		
	NF_DVA_LOG_DATA *log_data = (NF_DVA_LOG_DATA *)buff;

#ifdef DEBUG_POS_PUTLOG
	g_message("@@@@@@@  ch[%2d] text[%s]  [%s][%s]", ch, str, param1, param2);
#endif	
	memset(	log_data, 0x00, sizeof(NF_DVA_LOG_DATA));
			
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
	NF_DVA_LOG_DATA *log_data = (NF_DVA_LOG_DATA *)buff;
#ifdef DEBUG_POS_PUTLOG	
	g_message("@@@@@@@  ch[%2d] item[%s] price[%.2f] qty[%.2f] sum[%.2f]", ch, item, price, qty, sum);
#endif	
	memset(	log_data, 0x00, sizeof(NF_DVA_LOG_DATA));
			
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
	NF_DVA_LOG_DATA *log_data = (NF_DVA_LOG_DATA *)buff;
#ifdef DEBUG_POS_PUTLOG	
	g_message("@@@@@@@  ch[%2d] TOTAL sum[%.2f]", ch, sum);
#endif	
	memset(	log_data, 0x00, sizeof(NF_DVA_LOG_DATA));
			
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
	@brief		POS �̺�Ʈ �α� ��ȸ
	@param[in]	param	�̺�Ʈ �α� ��ȸ �Ķ����
	@param[out]	header	�̺�Ʈ �α� ��ȸ ��� ���
	@param[out[	pos_log	�̺�Ʈ �α� ��ȸ ��� ������ ��
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dva_eventlog_get(NF_DVA_LOG_PARAM *param, NF_DVA_LOG_RESULT_HEADER *header, NF_LOG_DATA *log_data)
{
	g_return_val_if_fail(param != NULL, FALSE);
	g_return_val_if_fail(header != NULL, FALSE);

	g_return_val_if_fail(param->log_param.request_count <= MAX_LOG_REQUEST, FALSE);

	int i, ch, curr = 0, search = 0;
	int request_count = param->log_param.request_count;
	gboolean res;
	guint64	log_id_last;

	// for test
	GTimeVal tv;
	struct tm *stime;	

	NF_DVA_LOG_DATA *dva_log;
	static NF_LOG_DATA _log_data[MAX_LOG_REQUEST];

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
		//g_print("[%s] param->search_str: %s\n", __FUNCTION__, param->search_str[i]);
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
	res = nf_eventlog_get(&param->log_param, &header->log_result_header, _log_data, NULL);

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
		header->log_id_first = _log_data[0].log_id;
		search = 1;
	}

	for (i=0; i < header->log_result_header.result_count; i++) {
		//dva_log = (NF_DVA_LOG_DATA *)_log_data[i].text;
		
		if (!_dva_eventlog_filter(param, &_log_data[i]))
			continue;

		//memcpy(&dva_log[curr++], _dva_log, sizeof(NF_DVA_LOG_DATA));
		memcpy(&log_data[curr++], &_log_data[i], sizeof(NF_LOG_DATA));

		if (KLOG_LV >= 1) {
#if 1
			GUINT64_TO_GTIMEVAL(_log_data[i].timestamp, tv);
			stime = localtime(&tv.tv_sec);

			g_print("log_data[%d].type: %d\n", i, _log_data[i].type);
			g_print("log_data[%d].timestamp: %lld\n", i, _log_data[i].timestamp);
			g_print("Date - %02d/%02d/%02d Time - %02d:%02d:%02d\n",
					stime->tm_year+1900, stime->tm_mon+1, stime->tm_mday,
					stime->tm_hour, stime->tm_min, stime->tm_sec);		
			g_print("log_data[%d].text: %s\n", i, _log_data[i].text);
			g_print("----------------------------------\n");
#endif					
			g_print("[%s]  pos log found - curr: %d, request_count: %d\n", __FUNCTION__, curr, request_count);
		}

		if (curr == request_count) {
			header->log_id_last = _log_data[i].log_id;
			header->log_id_last_prev = log_id_last;
			if (KLOG_LV >= 1) g_print("[%s] curr: %d, request_count: %d, goto exit\n", __FUNCTION__, curr, request_count);
			goto exit;
		}
		else {
			log_id_last = _log_data[i].log_id;
		}
	}

	if (curr < request_count) {
		param->log_param.mode = NF_LOG_PARAM_MODE_LOGID;
		if (KLOG_LV >= 1) g_print("### header.result_count: %d, curr: %d\n", header->log_result_header.result_count, curr);
		param->log_param.log_id = _log_data[header->log_result_header.result_count-1].log_id;
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

#if 0
static char dva_debug_help[] = "dva_debug";
static int dva_debug(int argc, char **argv)
{
	if (argc < 2) return 0;

	KLOG_LV = atoi(argv[1]);

	g_print("[POS] debug level: %d\n", KLOG_LV);
	
	return 0;
}
__commandlist(dva_debug, "dva_debug", pos_debug_help, pos_debug_help);
#endif

#if 0
static char dva_log_test_help[] = "dva_log_test";
static int dva_log_test(int argc, char **argv)
{	
	int testcase = 0;
	if (argc > 1) {
		testcase = atoi(argv[1]);
	}

	NF_DVA_LOG_DATA *dva_log = NULL;
	dva_log = g_malloc0(sizeof(NF_DVA_LOG_DATA)*MAX_LOG_REQUEST);

	NF_DVA_LOG_PARAM param;
	NF_DVA_LOG_RESULT_HEADER header;

	memset(&param, 0, sizeof(NF_DVA_LOG_PARAM));

	_init_dva_eventlog_param(&param);

	g_get_current_time(&param.log_param.time_end);
	param.log_param.time_search = param.log_param.time_end;
	param.log_param.direction = NF_LOG_PARAM_DIR_BACKWARD;

	param.log_param.mode = NF_LOG_PARAM_MODE_TIME;
	param.log_param.log_id = -1;
	param.log_param.request_count = 14;

	if (testcase == 1) {
		/* page down/up test */
		int i, r;
		for (r=0; r < 3; r++) {
			nf_dva_eventlog_get(&param, &header, dva_log);
			g_print("[dva_log_test] header.log_result_header.result_count: %d\n", header.log_result_header.result_count);

			for (i=0; i < header.log_result_header.result_count; i++) {
				_print_dva_eventlog(&dva_log[i]);
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

			nf_dva_eventlog_get(&param, &header, dva_log);
			g_print("[dva_log_test] header.log_result_header.result_count: %d\n", header.log_result_header.result_count);

			for (i=header.log_result_header.result_count-1; i >= 0; i--) {
				_print_dva_eventlog(&dva_log[i]);
			}		
		}
#endif
		return 0;
#if 0		
		param.log_param.time_search = param.log_param.time_begin;
		param.log_param.direction = NF_LOG_PARAM_DIR_FORWARD;
#endif		
	}
	nf_pos_eventlog_get(&param, &header, dva_log);

	g_print("[dva_log_test] header.log_result_header.result_count: %d\n", header.log_result_header.result_count);

#if 1
	int i;
	for (i=0; i < header.log_result_header.result_count; i++) {
		_print_dva_eventlog(&dva_log[i]);
	}

#else	/* obsolete */
	int i;
	for (i=0; i < header.result_count; i++) {
		if (log_data[i].type == LT_SYSTEM_POS) {
			int ch = log_data[i].param2;
			NF_DVA_LOG_DATA *dva_log = g_malloc0(sizeof(NF_DVA_LOG_DATA));
			nf_pos_text2log(log_data[i].text, dva_log);			

			//g_print("----------------------\n");
			g_print("pos ch: %d\n", ch);
			g_print("pos log_type: %d\n", dva_log->pos_log_type);
			if (dva_log->pos_log_type == NF_POS_LOG_TYPE_ITEM) {
				g_print("[Item] name: %s, price: %u, qty: %u, sum: %u\n", 
					dva_log->Item.name, dva_log->Item.price, dva_log->Item.qty, dva_log->Item.sum);				
			}
			else if (dva_log->pos_log_type == NF_POS_LOG_TYPE_TOTAL) {
				g_print("[Total] %u\n", dva_log->Total.total);
			}
			g_print("----------------------\n");

			g_free(dva_log);
		}
	}
#endif

	g_free(dva_log);
	
	return 0;
}
__commandlist(dva_log_test, "dva_log_test", dva_log_test_help, dva_log_test_help);
#endif


