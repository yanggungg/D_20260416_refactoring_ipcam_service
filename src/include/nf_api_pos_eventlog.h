#ifndef __NF_API_POS_EVENTLOG_H__
#define __NF_API_POS_EVENTLOG_H__

#include "nf_api_eventlog.h"

#define MAX_STR_OPER_CNT	2
#define MAX_STR_NUM_CNT		(MAX_STR_OPER_CNT + 1)

typedef enum _NF_POS_LOG_SEARCH_COND_E {
	NF_POS_LOG_SEARCH_COND_NA = 0,
	NF_POS_LOG_SEARCH_COND_PRICE,
	NF_POS_LOG_SEARCH_COND_QTY,
	NF_POS_LOG_SEARCH_COND_SUM,
	NF_POS_LOG_SEARCH_COND_TOTAL,
	NF_POS_LOG_SEARCH_COND_NR
} NF_POS_LOG_SEARCH_COND_E;

typedef struct _NF_POS_LOG_PARAM_T {
	NF_LOG_PARAM	log_param;

	/* 검색조건 */
	NF_POS_LOG_SEARCH_COND_E	search_cond;
	guint		range_from;		// 0 ~ 0 이면 N/A
	guint		range_to;
	guchar		match_case;		// 0:off, 1:on
	guchar		match_whole;
	guchar		operation[MAX_STR_OPER_CNT];		// 0:and, 1:or
	gchar		search_str[MAX_STR_NUM_CNT][64];
	guchar		search_str_len[MAX_STR_NUM_CNT];	// 0 이면 N/A
} NF_POS_LOG_PARAM;

typedef struct _NF_POS_LOG_RESULT_HEADER_T {
	NF_LOG_RESULT_HEADER	log_result_header;
	guint64	log_id_first;		// for page down/up
	guint64	log_id_last;
	guint64	log_id_last_prev;
} NF_POS_LOG_RESULT_HEADER;

enum {
	NF_POS_LOG_TYPE_ITEM = 0,
	NF_POS_LOG_TYPE_TOTAL,
	NF_POS_LOG_TYPE_TEXT,
	NF_POS_LOG_TYPE_RAW,
	NF_POS_LOG_TYPE_NR
};

enum {
	NF_POS_LOG_MONEY_WON = 0,
	NF_POS_LOG_MONEY_DOLLAR,
	NF_POS_LOG_MONEY_EURO,
	NF_POS_LOG_MONEY_NR
};

typedef struct _NF_POS_LOG_DATA_T {
	guint64	timestamp;
	guchar	chan;
	guchar	addr;			// POS 장비 address
	guchar	pos_log_type;		// NF_POS_LOG_TYPE_XXX
	guchar	pos_log_money;	// NF_POS_LOG_MONEY_XXX
	union {
		struct {	// NF_POS_LOG_TYPE_ITEM
			gchar	name[42];
			guint	price;			// 마지막 2자리는 소수점 -> *100 한 값 
			guint	qty;
			guint	sum;			//  price*qty (마지막 2자리는 소수점)
		} __attribute__((packed)) Item;
		struct {	// NF_POS_LOG_TYPE_TOTAL
			guint	total;			// 마지막 2자리는 소수점 -> *100 한 값 
			gchar	reserved[50];
		} __attribute__((packed)) Total;
		struct {
			gchar	text[50];
			gchar	param1[2];
			gchar	param2[2];
		} __attribute__((packed)) Text;
		struct {
			gchar	text[54];
 		} __attribute__((packed)) Raw;
		/* 프로토콜에 따라 추가 */
	} __attribute__((packed));

	guint64	log_id;	 // LOG ID
	
} __attribute__((packed)) NF_POS_LOG_DATA ;

/**
	@brief		POS 이벤트 로그 입력
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_pos_eventlog_put_text(gint ch, char *str, char *param1, char *param2);

/**
	@brief		POS 	이벤트 로그 조회
	@param[in]	param	이벤트 로그 조회 파라메터
	@param[out]	header	이벤트 로그 조회 결과 헤더
	@param[out[	result	이벤트 로그 조회 결과 리턴할 곳
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_pos_eventlog_get(NF_POS_LOG_PARAM *param, NF_POS_LOG_RESULT_HEADER *header, NF_POS_LOG_DATA *pos_log);

/* 아래 사용않함 */

/* NF_POS_LOG_DATA -> text 변환 */
gboolean nf_pos_log2text(NF_POS_LOG_DATA *log_data, char *text);

/* text -> NF_POS_LOG_DATA 변환 */
gboolean nf_pos_text2log(char *text, NF_POS_LOG_DATA *log_data);

#endif

