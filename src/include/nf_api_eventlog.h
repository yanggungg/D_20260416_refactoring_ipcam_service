/*
2008-07-29 오전 11:27:56 choissi
	NF_LOG_PARAM 필드 추가
		GTimeVal			time_search;
*/

#ifndef __NF_API_EVENTLOG_H__
#define __NF_API_EVENTLOG_H__

#include <glib.h>

#include "nf_common_util.h"
#include "nf_logevtdef.h"

#define LOG_P1_MASK_SIZE_TMP  	LOG_P1_MASK_SIZE

typedef enum _NF_LOG_PARAM_DIR_E {
	NF_LOG_PARAM_DIR_FORWARD	= 0,
	NF_LOG_PARAM_DIR_BACKWARD	= 1
}NF_LOG_PARAM_DIR_E;

typedef enum _NF_LOG_PARAM_MODE_E {
	NF_LOG_PARAM_MODE_TIME	= 0,
	NF_LOG_PARAM_MODE_LOGID	= 1
}NF_LOG_PARAM_MODE_E;

typedef enum _NF_LOG_PARAM_HIDE_E {
	NF_LOG_PARAM_HIDE_OFF   = 0,
	NF_LOG_PARAM_HIDE_ON	= 1
}NF_LOG_PARAM_HIDE_E;

// Get Log
typedef struct _NF_LOG_PARAM_T {
	GTimeVal			time_begin;			
	GTimeVal			time_end;
	GTimeVal			time_search;

	NF_LOG_PARAM_DIR_E	direction;
	NF_LOG_PARAM_MODE_E	mode;
	NF_LOG_PARAM_HIDE_E hide;
	
	guint64				log_id;
	guint				request_count;				/* Requesting Log Count. */
	guint				type_mask;					/* Type Masking Bit. */
	guint				channelMask;				/* 각 channel Mask값 */
	gchar				param1_mask[LOG_P1_MASK_SIZE];	/* Parameter1 mask array. */
} NF_LOG_PARAM __attribute__ ((aligned(4))) ;

typedef struct _NF_LOG_RESULT_HEADER_T {
	guint	request_count;
	guint	result_count;
	/*
	 * Followed LogCmdData_t type array,
	 * array size = uRequestCount,
	 * data size = uRequestCount * sizeof(LogCmdData_t)
	 */
} NF_LOG_RESULT_HEADER;

typedef struct _NF_LOG_DATA_T {
	guint64		timestamp;
	guchar 		type;
	guchar 		param1;
	guchar 		param2;
	guchar 		text_len;
	guchar 		reserved[4];
	guint64		log_id;
	gchar		text[256];
} NF_LOG_DATA;

typedef struct  _NF_LOG_DATA_OLD_T{
	guint	timestamp;		// sec
	guchar	timestampl;		// 5m second
	guchar	type;			// log_evt_type_e
	guchar	param1;
	guchar	param2;
	guint64	log_id;
	gchar	text[256];		// string
} NF_LOG_DATA_OLD;

typedef struct _NF_LOG_INFO_T{
	guint ch;
	guint64 timestamp;
	guint timestampl;
}NF_LOG_INFO;

gboolean nf_meta_make_base64json(int type, NF_LOG_INFO info, void *data, char *encode_data, int *ret_len, char *keyword, char *topic);


/**
	@brief				이벤트 로그 조회
	@param[in]	param	이벤트 로그 조회 파라메터
	@param[out]	header	이벤트 로그 조회 결과 헤더
	@param[out]	elem	이벤트 로그 조회 결과 리턴할 곳
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_get( NF_LOG_PARAM *param, NF_LOG_RESULT_HEADER *header, 
								NF_LOG_DATA *elem, GError **error);		

/**
	@brief				이벤트 로그 발생
	@param[in]	tv		발생시간 NULL, 입력시 현재시간
	@param[in]	type	로그 type
	@param[in]	param1	로그 param1
	@param[in]	param2	로그 param2
	@param[in]	text	로그 text
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_put_param( GTimeVal *tv, gint type, gint param1, gint param2, const gchar *text );

/**
	@brief				이벤트 로그 발생
	@param[in]	*log_data 로그 데이터
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_put( NF_LOG_DATA *log_data, GError **error);

/**
	@brief				이벤트 로그 포맷 변환 NEW --> OLD
	@param[in]	*log_data 로그 데이터
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_convert( NF_LOG_DATA *log_data, NF_LOG_DATA_OLD *old_data);


/* NAND ERROR LOG */
typedef enum _NF_ERR_NAND_LOG_E {
	NAND_ERR_NA = 0,
	NAND_ERR_RECOVERY_ERROR,
	NAND_ERR_DISK_ERROR,
	NAND_ERR_SMART_ERROR,
	NAND_ERR_ALL,
	NAND_ERR_NR
}NF_ERR_NAND_LOG_E;

typedef struct _NF_ERR_NAND_LOG_DATA_VAL_T {
	guchar 		err_cnt;
	guchar 		err_disk_pdid;
	guchar		reserved[2];
	guchar		serial[28];
} NF_ERR_NAND_LOG_DATA_VAL;

typedef struct _NF_ERR_NAND_LOG_DATA_T {
	NF_ERR_NAND_LOG_DATA_VAL nand_recovery_err;
	NF_ERR_NAND_LOG_DATA_VAL nand_disk_err;
	NF_ERR_NAND_LOG_DATA_VAL nand_smart_err;
	NF_ERR_NAND_LOG_DATA_VAL reserved;
} NF_ERR_NAND_LOG_DATA;


/**
	@brief		nana에 기록하는 error의 count
	@param[in]	type[NF_ERR_NAND_LOG_E], count
	@param[out]	count(in nf_event_read_nand_err_cnt]
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_event_clear_nand_err_cnt(gint type);
gboolean nf_event_read_nand_err_cnt(gint type, gint *count ,guchar* pdid);
gboolean nf_event_inc_nand_err_cnt(gint type, guchar pdid);
gboolean nf_event_set_nand_err_cnt(gint type, gint cnt);
#endif
