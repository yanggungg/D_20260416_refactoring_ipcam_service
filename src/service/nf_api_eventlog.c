#include <glib.h>
#include <string.h>
#include <jansson.h>

#include "nf_common.h"
#include "nf_notify.h"
#include "nf_api_eventlog.h"
#include "nf_api_dva_eventlog.h"
#include "itx_ai_def.h"
#include "nf_logevtdef.h"
#include "libsst.h"

#include "ivca_def.h"

#define DEBUG_EVENTLOG_JBSHELL
//#define DEBUG_EVENTLOG_PUT
//#define DEBUG_EVENTLOG_GET
//#define DEBUG_NAND_ERR_LOG_JBSHELL

#ifdef DEBUG_EVENTLOG_JBSHELL
	#include "jbshell.h"
#endif

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "api_evt"

static const gchar *_eventlog_str[] = {
			/* Types for SYSTEM_LOGEVT category. */
/* 00 */	"SYSTEM_STARTED",
/* 01 */	"SYSTEM_SHUTDOWN",
/* 02 */	"ABNORMAL_SHUTDOWN_DETECTED",
/* 03 */	"SYSTEM_RECOVERED",
/* 04 */	"SYSTEM_TIME_CHANGED",
/* 05 */	"SYSTEM_FW_UPGRADE",
/* 06 */	"SYSTEM_FORMAT",
/* 07 */	"SYSTEM_CHECKDISK",
/* -- */	/* Types for LOGON_LOGEVT category. */
/* 08 */	"LOCAL_LOG_ON",
/* 09 */	"LOCAL_LOG_OFF",
/* 10 */	"REMOTE_LOG_ON",
/* 11 */	"REMOTE_LOG_OFF",
/* -- */	/* Types for SETUP_LOGEVT category. */
/* 12 */	"RECORD_SETUP_CHANGED",
/* 13 */	"SYSTEM_SETUP_CHANGED",
/* -- */	/* Types for EVENT_LOGEVT category. */
/* 14 */	"SENSOR_INPUT",
/* 15 */	"MOTION_DETECTION",	
/* 16 */	"VIDEO_IN",		
/* 17 */	"VIDEO_LOSS",
/* 18 */	"TAMPER",
/* 19 */	"SMART_WARNING",	

/* 20 */	"HDD_DISK_EVENT",	

/* 21 */	"HDD_FULL",
/* 22 */	"HDD_OW",
/* -- */	/* Types for RECORD_LOGEVT category. */
/* 23 */	"RECORD_STARTED",
/* 24 */	"RECORD_STOPPED",
/* -- */	/* Types for SYSTEM_LOGEVT category. 2009-05-26 ���� 1:18:45 */	
/* 25 */	"SYSTEM_EVENT",
/* 26 */	"SYSTEM_DEBUG",
/* 27 */	"SYSTEM_POS",
/* 28 */	"NETWORK_EVENT",
/* 29 */	"IPCAM",
/* 30 */	"VCA",
/* -- */	/* Must not exceed 31 */
/* 31 */	"NR"
};

/*
int sst_log_get_open(u8 mode, u8 dir, u8 hide, u32 typemask,
		u8 *p1mask, u64 t_beg, u64 t_end, u64 t_search, u64 logid);
int sst_log_get_close(int l_sid);
int sst_log_get(int l_sid, int rcount, struct log_data_t *log_data);
*/

/**
	@brief				�̺�Ʈ �α� ��ȸ
	@param[in]	param	�̺�Ʈ �α� ��ȸ �Ķ����
	@param[out]	header	�̺�Ʈ �α� ��ȸ ��� ���
	@param[out]	elem	�̺�Ʈ �α� ��ȸ ��� ������ ��
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_get( NF_LOG_PARAM *param, NF_LOG_RESULT_HEADER *header, 
								NF_LOG_DATA *elem, GError **error)
{
	gint result = 0, log_sid;
	guint64 t_beg = 0, t_end = 0;
	guint64	t_search = 0;
	
	g_return_val_if_fail (param != NULL, 0);

#ifdef DEBUG_EVENTLOG_GET
	
	g_message("%s begin sec[%ld.%06ld] end sec[%ld.%06ld]", __FUNCTION__,
					param->time_begin.tv_sec, param->time_begin.tv_usec,
					param->time_end.tv_sec, param->time_end.tv_usec );
					
	g_message("%s search sec[%ld.%06ld]", __FUNCTION__,
					param->time_search.tv_sec, param->time_search.tv_usec );
	
	g_message("%s dir[%d] mode[%d] hide[%d] req_cnt[%d] type_mask[0x%08x]", __FUNCTION__,
					param->direction, param->mode, param->hide,
					param->request_count, param->type_mask );

	g_message("%s log_id[%llx] param1_mask[%s]", __FUNCTION__, 
					param->log_id, param->param1_mask); 

#endif	
				
	g_return_val_if_fail (header != NULL, 0);
	g_return_val_if_fail (elem != NULL, 0);	

	t_beg = GTIMEVAL_TO_GUINT64(param->time_begin);
	t_end = GTIMEVAL_TO_GUINT64(param->time_end);
	t_search = GTIMEVAL_TO_GUINT64(param->time_search);
	 
	//memset( param->param1_mask, 0x1, sizeof(512));	
	//nf_debug_hexdump( param->param1_mask, LOG_P1_MASK_SIZE_TMP );
	
	log_sid = result = sst_log_get_open ( param->mode, param->direction, param->hide,
								param->type_mask, param->param1_mask,
								t_beg, t_end, t_search, param->log_id );										
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 
		return 0;
	}

	result = sst_log_get( log_sid, param->request_count, (struct log_data_t *)elem );
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 

		result = sst_log_get_close(log_sid);
		if ( result < 0 ) {		
			g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 
			return 0;
		}
		return 0;
	}

#ifdef DEBUG_EVENTLOG_GET
	g_message( "[%s] result_count[%d]", __FUNCTION__, result);
	{
		int i;
		for(i=0; i<result;i++)
		{
			g_message( "%s [%d] timestamp[%lld][%llx] [%llx][%llx][%llx] type[%d] p1[%d]p2[%d] text[%16.16s]", 
					__FUNCTION__, i,
					elem[i].timestamp, elem[i].log_id,
					elem[i].log_id >> 24, (elem[i].log_id >> 12) & 0xFFF, elem[i].log_id & 0xFFF,	
					elem[i].type, 
					elem[i].param1, elem[i].param2, elem[i].text );
		}
	}
#endif
	
	header->request_count = param->request_count;
	header->result_count = result>0 ? result : 0;
			
	result = sst_log_get_close(log_sid);
	if ( result < 0 ) {		
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		return 0;
	}				

	
	return 1;
}								

/**
	@brief				�̺�Ʈ �α� �߻�
	@param[in]	tv		�߻��ð� NULL, �Է½� ����ð�
	@param[in]	type	�α� type
	@param[in]	param1	�α� param1
	@param[in]	param2	�α� param2
	@param[in]	text	�α� text
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_put_param( GTimeVal *tv, gint type, gint param1, gint param2, const gchar *text )
{
	NF_LOG_DATA log_data;
	GTimeVal	tval;
	
	//g_message( "[%s] called\n", __FUNCTION__ );		
	g_return_val_if_fail ( type >= 0 && type < LT_NR, 0);	

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


	if (type == LT_SYSTEM_POS || type == LT_VCA || type == LT_DVA)
	{	// in case of pos logtype, text field has to be type casted into NF_POS_LOG_DATA
		memcpy(log_data.text, text, sizeof(log_data.text));
	}
	else
	{
		if(text)
		{
			snprintf(log_data.text, sizeof(log_data.text),"%s", text);
		}
	}
	
	return nf_eventlog_put( &log_data, NULL);	
}

								
/**
	@brief				�̺�Ʈ �α� �߻�
	@param[in]	*log_data �α� ������
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_put( NF_LOG_DATA *log_data, GError **error)
{
	gint result = 0;
	
	g_return_val_if_fail ( log_data != NULL, 0);
	g_return_val_if_fail ( log_data->type >= 0 && log_data->type < LT_NR, 0);	
							
#ifdef DEBUG_EVENTLOG_PUT
	g_message("%s timestamp[%lld]", __FUNCTION__, log_data->timestamp );
	g_message("%s type[%d][%s] param1[%d] param2[%d] text[%s]", __FUNCTION__, 
				log_data->type, _eventlog_str[log_data->type],  
				log_data->param1, log_data->param2, 
				log_data->text );
#endif					

#if 1	// choissinf 2009-04-07 ���� 5:16:58
	log_data->text_len = 0;
#else
	log_data->text_len = strnlen(log_data->text, sizeof(log_data->text) );
#endif

	if(log_data->type ==LT_VCA){
		log_data->text_len = sizeof(ivca_rule_event_t);
	}
	else
		log_data->text_len = 0;

	log_data->log_id = 0;

	result = sst_log_put((struct log_data_t *)log_data);

	if ( result < 0 ) {
		if ( result != SST_ERR_WRITE_LOG_TO_RAM )
		{
			if( log_data->type != LT_MOTION_DETECTION )
				g_warning("%s result[%d](%s) type[%d][%s] p1[%d]", 
					__FUNCTION__, result, sst_get_error_string(result), 
					log_data->type, 
					_eventlog_str[log_data->type],
					log_data->param1); 
		}
	}else{
		nf_notify_fire_pointer("log", log_data, sizeof(NF_LOG_DATA));
	}	
	return ( result == 0 ) ? 1:0;	
}

gboolean nf_event_nand_log_flush()
{
	gint result = 0;
	
	result = sst_nand_log_flush();
	
	return ( result == 0 ) ? 1:0;	
}

gboolean nf_event_clear_nand_err_cnt(gint type)
{
	FILE *fp = NULL;
	s8 tg[40];
 	NF_ERR_NAND_LOG_DATA err_nand_log;

	g_return_val_if_fail ( type != 0, 0);

	memset(&err_nand_log, 0, sizeof(NF_ERR_NAND_LOG_DATA));
	
	sprintf(tg, "/NFDVR/log/nand_err.log");
	fp = fopen(tg, "r+b");
	if ( !fp )
	{
		fprintf(stderr, "[NAND]%s open fail(%s)\n", __func__, tg);
		return FALSE;
	}

	fseek(fp, 0, SEEK_SET);

	if ( fread(&err_nand_log, 1, sizeof ( NF_ERR_NAND_LOG_DATA), fp) < sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s fread fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}

	if ( type == NAND_ERR_RECOVERY_ERROR )
	{
		err_nand_log.nand_recovery_err.err_cnt = 0;
		err_nand_log.nand_recovery_err.err_disk_pdid = 0xff;
	}
	else if ( type == NAND_ERR_DISK_ERROR )
	{
		err_nand_log.nand_disk_err.err_cnt = 0; 
	}
	else if ( type == NAND_ERR_SMART_ERROR )
	{
		err_nand_log.nand_smart_err.err_cnt = 0;
	}
	else if ( type == NAND_ERR_ALL )
	{
		err_nand_log.nand_recovery_err.err_cnt = 0;
		err_nand_log.nand_disk_err.err_cnt = 0; 
		err_nand_log.nand_smart_err.err_cnt = 0;
	}

	g_message("======%s=========type = %d, ===> %d, %d, %d", 
			__func__, 
			type, 
			err_nand_log.nand_recovery_err.err_cnt,
			err_nand_log.nand_disk_err.err_cnt, 
			err_nand_log.nand_smart_err.err_cnt);

	fseek(fp, 0, SEEK_SET);
	if ( fwrite(&err_nand_log, 1, sizeof(NF_ERR_NAND_LOG_DATA), fp) 	< sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s write fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}

	sync();
	proxy_system("/bin/sync",1,3);	

	fclose(fp);
	return TRUE;
}

gboolean nf_event_read_nand_err_cnt(gint type, gint *count , guchar *pdid)
{
	FILE *fp = NULL;
	s8 tg[40];
 	NF_ERR_NAND_LOG_DATA err_nand_log;

	g_return_val_if_fail ( type != 0, 0);

	memset(&err_nand_log, 0, sizeof(NF_ERR_NAND_LOG_DATA));
	
	sprintf(tg, "/NFDVR/log/nand_err.log");
	fp = fopen(tg, "r+b");
	if ( !fp )
	{
		fprintf(stderr, "[NAND]%s open fail(%s)\n", __func__, tg);
		return FALSE;
	}

	fseek(fp, 0, SEEK_SET);

	if ( fread(&err_nand_log, 1, sizeof ( NF_ERR_NAND_LOG_DATA), fp) < sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s fread fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}

	if ( type == NAND_ERR_RECOVERY_ERROR )
	{
		if(count != NULL)
		*count = err_nand_log.nand_recovery_err.err_cnt;
		if(pdid != NULL)
			*pdid = err_nand_log.nand_recovery_err.err_disk_pdid;
	}
	else if ( type == NAND_ERR_DISK_ERROR )
	{
		*count = err_nand_log.nand_disk_err.err_cnt;
	}
	else if ( type == NAND_ERR_SMART_ERROR )
	{
		*count = err_nand_log.nand_smart_err.err_cnt;
	}
	else if ( type == NAND_ERR_ALL )
	{
		*count = err_nand_log.nand_recovery_err.err_cnt;
		*count += err_nand_log.nand_disk_err.err_cnt;
		*count += err_nand_log.nand_smart_err.err_cnt;
	}
	
	g_message("======%s=========type = %d, ===> %d, %d, %d", 
			__func__, 
			type, 
			err_nand_log.nand_recovery_err.err_cnt,
			err_nand_log.nand_disk_err.err_cnt, 
			err_nand_log.nand_smart_err.err_cnt);

	fclose(fp);
	return TRUE;
}

gboolean nf_event_inc_nand_err_cnt(gint type , u8 pdid)
{
	FILE *fp = NULL;
	s8 tg[40];
 	NF_ERR_NAND_LOG_DATA err_nand_log;

	g_return_val_if_fail ( type != 0, 0);

	memset(&err_nand_log, 0, sizeof(NF_ERR_NAND_LOG_DATA));
	
	sprintf(tg, "/NFDVR/log/nand_err.log");
	fp = fopen(tg, "r+b");
	if ( !fp )
	{
		fprintf(stderr, "[NAND]%s open fail(%s)\n", __func__, tg);
		return FALSE;
	}

	fseek(fp, 0, SEEK_SET);

	if ( fread(&err_nand_log, 1, sizeof ( NF_ERR_NAND_LOG_DATA), fp) < sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s fread fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}

	if ( type == NAND_ERR_RECOVERY_ERROR )
	{
		err_nand_log.nand_recovery_err.err_cnt++;
		err_nand_log.nand_recovery_err.err_disk_pdid = pdid;
	}
	else if ( type == NAND_ERR_DISK_ERROR )
	{
		err_nand_log.nand_disk_err.err_cnt++; 		
	}
	else if ( type == NAND_ERR_SMART_ERROR )
	{
		err_nand_log.nand_smart_err.err_cnt++;
	}

	g_message("======%s=========type = %d, ===> %d, %d, %d", 
			__func__, 
			type, 
			err_nand_log.nand_recovery_err.err_cnt,
			err_nand_log.nand_disk_err.err_cnt, 
			err_nand_log.nand_smart_err.err_cnt);
	
	fseek(fp, 0, SEEK_SET);
	fclose(fp);

	/* let this code fopen again */
	fp = fopen(tg, "r+b");
	if ( !fp )
	{
		fprintf(stderr, "[NAND]%s open fail(%s)\n", __func__, tg);
		return FALSE;
	}
	
	if ( fwrite(&err_nand_log, 1, sizeof(NF_ERR_NAND_LOG_DATA), fp) < sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s write fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}

	sync();
	proxy_system("/bin/sync",1,3);	

	fclose(fp);
	return TRUE;
}

gboolean nf_event_set_nand_err_cnt(gint type, gint cnt)
{
	FILE *fp = NULL;
	s8 tg[40];
 	NF_ERR_NAND_LOG_DATA err_nand_log;

	g_return_val_if_fail ( type != 0, 0);

	memset(&err_nand_log, 0, sizeof(NF_ERR_NAND_LOG_DATA));
			
	sprintf(tg, "/NFDVR/log/nand_err.log");
	fp = fopen(tg, "r+b");
	if ( !fp )
	{
		fprintf(stderr, "[NAND]%s open fail(%s)\n", __func__, tg);
		return FALSE;
	}

	fseek(fp, 0, SEEK_CUR);

	if ( fread(&err_nand_log, 1, sizeof ( NF_ERR_NAND_LOG_DATA), fp) < sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s fread fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}

	if ( type == NAND_ERR_RECOVERY_ERROR )
	{
		err_nand_log.nand_recovery_err.err_cnt = cnt;
	}
	else if ( type == NAND_ERR_DISK_ERROR )
	{
		err_nand_log.nand_disk_err.err_cnt = cnt; 		
	}
	else if ( type == NAND_ERR_SMART_ERROR )
	{
		err_nand_log.nand_smart_err.err_cnt = cnt;
	}

	g_message("======%s=========type = %d, ===> %d, %d, %d", 
			__func__, 
			type, 
			err_nand_log.nand_recovery_err.err_cnt,
			err_nand_log.nand_disk_err.err_cnt, 
			err_nand_log.nand_smart_err.err_cnt);

	fseek(fp, 0, SEEK_SET);
	if ( fwrite(&err_nand_log, 1, sizeof(NF_ERR_NAND_LOG_DATA), fp) < sizeof(NF_ERR_NAND_LOG_DATA) )
	{
		fprintf(stderr, "[NAND]%s write fail(%s)\n", __func__, tg);
		fclose(fp);
		return FALSE;
	}
	
	sync();
	proxy_system("/bin/sync",1,3);	

	fclose(fp);
	return TRUE;
}
/**
	@brief				�̺�Ʈ �α� ���� ��ȯ NEW --> OLD
	@param[in]	*log_data �α� ������
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_eventlog_convert( NF_LOG_DATA *log_data, NF_LOG_DATA_OLD *old_data)
{
	GTimeVal tval;

	g_return_val_if_fail ( log_data != NULL, 0);
	g_return_val_if_fail ( old_data != NULL, 0);

	GUINT64_TO_GTIMEVAL( log_data->timestamp, tval);
			
	old_data->timestamp		= tval.tv_sec;
	old_data->timestampl	= tval.tv_usec/5000;
	
	old_data->type			= log_data->type;
	old_data->param1		= log_data->param1;
	old_data->param2		= log_data->param2;
	old_data->log_id		= log_data->log_id;
	
	strncpy(old_data->text, log_data->text, sizeof(log_data->text)-1 );
		
	return 1;	
}

json_t *nf_event_data_to_json(ai_generic_log_t *data)
{
    int i;
    int rc;

    double timestamp;
    char buffer[100];
    char topic[128] = {0,};

    json_t *ret = NULL;
	json_error_t error;
    json_t *zone = NULL;
    json_t *json = NULL;

    if(data == NULL) return NULL;
	
    snprintf(buffer, sizeof(buffer), "%u.%u", data->timestamp, data->timestampl);
    sscanf(buffer, "%lf", &timestamp);
	snprintf(topic, sizeof(topic) - 1, "RuleEngine/Event/%s", data->caption);

//  ret = json_pack_ex(&error, 0, "{s:s,s:i,s:f,s:{s:{s:s,s:s,s:s},s:{s:s,s:s,s:[{s:{s:f,s:f,s:f,s:f}}],s:[]}}}", 
	ret = json_pack_ex(&error, 0, "{s:s,s:i,s:f,s:{s:{s:s,s:s,s:s},s:{s:[{s:{s:f,s:f,s:f,s:f}}],s:[]}}}", 
            "topic", topic, 
            "source", data->ch,
            "timestamp", timestamp,
            "eventdata",
                "generic_info",
                    "caption", data->caption,
                    "title", data->title,
                    "description", data->description,
            "trigger_info",
            /*
                "trigger_name", data->trigger_type,
                "trigger_type", data->trigger_name,
            */
                "object_list",
                    "bbox",
                        "x0", data->event_area[0].x,
                        "y0", data->event_area[0].y,
                        "x1", data->event_area[1].x,
                        "y1", data->event_area[1].y,
                "zone");
    if(ret == NULL){
        printf("[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        printf("[%s:%d] ch[%d] timestamp[%f][%s] caption[%s] title[%s] description[%s] x0[%f] y0[%f] x1[%f] y1[%f]\n", __func__, __LINE__,
                data->ch, timestamp, buffer,
                data->caption, data->title, data->description,
                data->event_area[0].x,
                data->event_area[0].y,
                data->event_area[1].x,
                data->event_area[1].y);
        return NULL;
    }
/*
    zone = json_object_get(
            json_object_get(
                json_object_get(ret, "eventdata"),
                "trigger_info"),
            "zone");

    printf("[%s:%d] zone[%p]\n", __func__, __LINE__, zone);

    for(i = 0; i < data->trigger_zone_count; i++){
		json = json_pack_ex(&error, 0, "{s:f,s,f}",
                "x", (double)data->trigger_zone_list[i].x,
                "y", (double)data->trigger_zone_list[i].y);

        if(json == NULL){
            char *debug;
            debug = json_dumps(json, JSON_ENCODE_ANY);
            fprintf(stderr, "[%s:%d] json error on line %d: %s json_zone[%s]\n", __FUNCTION__, __LINE__, error.line, error.text, debug);
            if(debug) free(debug);
            break;
        }

        rc = json_array_append_new(zone, json);
    }
*/
    return ret;
}

gboolean nf_meta_make_base64json(int type, NF_LOG_INFO info, void *data, char *encode_data, int *ret_len, char *keyword, char *topic)
{
	int i = 0, j = 0;
	char event_topic[64];
	gboolean is_inside;
	unsigned int time_interval = 0;
	unsigned int report_time_interval = 0;
	unsigned int reset_time_interval = 0;
	unsigned short stop_time=0, abandon_time=0, remove_time=0, loiter_time=0, fall_time=0;
	double score;
	int event_type = 0;
	int channel = -1;
	unsigned long timestamp;
	unsigned int timestampl;
	int object_id = 0;
	int rule_id = -1;
	
	DVA_MSG_IDZ *dva_data = NULL;
	ivca_rule_event_t *va_data;
	ai_rule_event_t *ai_data;
	ai_fr_log_t *fr_data;
	ai_lpr_log_t *lpr_data;
	ai_generic_log_t *gnr_data = NULL;

	char *str_ptr = NULL;
	char str[4096];
	double r_timestamp = 0.0;
	double bbox[4] = {0,0,};
	char class_name[64];
	
	memset(str, 0x00, sizeof(str));

	char *json_dump_data = NULL;
	char db_field[64];
	json_t* _json_root = json_object();
	json_t* _json_meta = NULL;
	json_t* _json_image = NULL;
	json_t* _json_annot = NULL;
	json_t* _json_bbox = NULL;
	json_t* _json_face = NULL;
	json_t* _json_face_groups = NULL;
	json_t* _json_face_attribute = NULL;
	json_t* _json_gnr = NULL;

	memset(event_topic, 0x00, sizeof(topic));
	memset(db_field, 0x00, sizeof(db_field));

	if ( type == DVA_INTRUSION_DETECTION || type == DVA_ILLEGAL_PARKING )
	{
		if ( type == DVA_INTRUSION_DETECTION )
			event_type = -1;
		if ( type == DVA_ILLEGAL_PARKING )
			event_type = -2;

		dva_data = (DVA_MSG_IDZ *)data;
		score = dva_data->confidence;
		strncpy(class_name, dva_data->name, sizeof(dva_data->name) - 1);
		
		for ( j = 0; j < 4; j++ )
			bbox[j] = dva_data->bbox.coords[j];
	}
	else if ( type == DVA_AI_DETECTION )
	{
		ai_data = (ai_rule_event_t *)data;
		
		for ( i = 0; i < IVCA_MAX_ZONES; i++ )
		{
			snprintf(db_field, 64 - 1, "cam.dvabx.rule.R%d.Z%d.id", ai_data->ch, i);
			if ( nf_sysdb_get_int(db_field) == ai_data->rule_id )
			{
				memset(db_field, 0x00, sizeof(db_field));
				snprintf(db_field, 64 - 1, "cam.dvabx.rule.R%d.Z%d.time_sarlf", ai_data->ch, i);
				str_ptr = nf_sysdb_get_str_nocopy(db_field);
				sscanf(str_ptr, "%hu %hu %hu %hu %hu", &stop_time,&abandon_time, &remove_time, &loiter_time,&fall_time);
			}
		}
		event_type = ai_data->type;
		strncpy(class_name, ai_data->object_class, sizeof(ai_data->object_class) - 1);
		channel = ai_data->ch;
		score = ai_data->confidence;
		object_id = ai_data->object_id;
		rule_id = ai_data->rule_id;
		
		for ( j = 0; j < 4; j++ )
			bbox[j] = ai_data->bbx_position[j];
	}
	else if ( type == DVA_AI_FR_DETECTION )
	{
		fr_data = (ai_fr_log_t *)data;
		event_type = fr_data->type;
		object_id = fr_data->object_id;
		rule_id = fr_data->rule_id;
		
		strncpy(class_name, "face", 4);
		channel = fr_data->ch;
		score = fr_data->confidence;
		
		for ( j = 0; j < 4; j++ )
			bbox[j] = fr_data->bbx_position[j];
	}
	else if ( type == DVA_AI_LPR_DETECTION )
	{
		lpr_data = (ai_lpr_log_t *)data;
		event_type = lpr_data->type;
		object_id = lpr_data->object_id;
		rule_id = lpr_data->rule_id;

		strncpy(class_name, "lp", 4);
		channel = lpr_data->ch;
		score = lpr_data->confidence;

		for ( j = 0; j < 4; j++ )
			bbox[j] = lpr_data->bbx_position[j];
		
	}
	else if ( type == -1 )
	{
		va_data = (ivca_rule_event_t *)data;
		
		for ( i = 0; i < IVCA_MAX_ZONES; i++ )
		{
			snprintf(db_field, 64 - 1, "cam.vca.rule.R%d.Z%d.id", va_data->ch, i);
			if ( nf_sysdb_get_int(db_field) == va_data->rule_id )
			{
				memset(db_field, 0x00, sizeof(db_field));
				snprintf(db_field, 64 - 1, "cam.vca.rule.R%d.Z%d.time_sarlf", va_data->ch, i);
				str_ptr = nf_sysdb_get_str_nocopy(db_field);
				sscanf(str_ptr, "%hu %hu %hu %hu %hu", &stop_time,&abandon_time, &remove_time, &loiter_time,&fall_time);
			}
		}
		event_type = va_data->type;
		object_id = va_data->object_id;
		rule_id = va_data->rule_id;
		
		// event tpye is 0 => live counter value event
		if ( event_type != IVCA_ET_COUNTER && event_type != 0)
		{
			memset(class_name, 0x00, sizeof(class_name));
			if ( va_data->object_class == 1 )
			{
				strncpy(class_name, "person",  sizeof(class_name) - 1);
			}
			else if ( va_data->object_class == 2 )
			{
				strncpy(class_name, "car",  sizeof(class_name) - 1);
			}
			else if ( va_data->object_class == 3 )
			{
				strncpy(class_name, "bus",  sizeof(class_name) - 1);
			}
			else if ( va_data->object_class == 4 )
			{
				strncpy(class_name, "bicycle",  sizeof(class_name) - 1);
			}
			else if ( va_data->object_class == 5 )
			{
				strncpy(class_name, "cat",  sizeof(class_name) - 1);
			}
			else if ( va_data->object_class == 6 )
			{
				strncpy(class_name, "truck",  sizeof(class_name) - 1);
			}
			else if ( va_data->object_class == 7 )
			{
				strncpy(class_name, "dog",  sizeof(class_name) - 1);
			}
			else				
			{
				strncpy(class_name, "others",  sizeof(class_name) - 1);
			}
		}
		
		bbox[0] = va_data->rc.x / 3840.0;
		bbox[1] = va_data->rc.y / 2160.0;
		bbox[2] = bbox[0] + (va_data->rc.w / 3840.0);
		bbox[3] = bbox[1] + (va_data->rc.h / 2160.0);
	}
	else if ( type == DVA_AI_GENERIC )
	{
		gnr_data = (ai_generic_log_t *)data;
		
		_json_gnr = nf_event_data_to_json(gnr_data);
		if ( _json_gnr == NULL )
			return FALSE;
	else
	{
			json_dump_data = json_dumps(_json_gnr, JSON_COMPACT);
			
			base64_encode(json_dump_data, strlen(json_dump_data), encode_data);
			*ret_len = strlen(encode_data);
			
			if ( json_dump_data != NULL)
				free(json_dump_data);
			
			json_decref(_json_gnr);
			return TRUE;
		}
	}
	else
	{
		if ( _json_root != NULL )
			json_decref(_json_root);
		return FALSE;
	}

	channel = info.ch;
	timestamp = info.timestamp;
	timestampl = info.timestampl;

	// event topic
	switch ( event_type )
	{
		case IVCA_ET_DIR_POS :
		{
			strncpy(event_topic, "RuleEngine/LineDetector/Crossed", 64 - 1);
			break;
		}
		case IVCA_ET_DIR_NEG :
		{
			strncpy(event_topic, "RuleEngine/LineDetector/Crossed", 64 - 1);
			break;
		}
		case IVCA_ET_ENTER :
		case IVCA_ET_INTRUSION :
		{
			strncpy(event_topic, "RuleEngine/FieldDetector/ObjectsInside", 64 - 1);
			break;
		}
		case IVCA_ET_EXIT :
		{
			strncpy(event_topic, "RuleEngine/FieldDetector/ObjectsInside", 64 - 1);
			break;
		}	
		case IVCA_ET_STOPPED :
		{
			strncpy(event_topic, "RuleEngine/LoiteringDetector/ObjectIsStopped", 64 - 1);
			break;
		}
		case IVCA_ET_REMOVED :
		{
			strncpy(event_topic, "RuleEngine/LoiteringDetector/ObjectIsRemoved", 64 - 1);
			break;
		}
		case IVCA_ET_LOITERED :
		{
			strncpy(event_topic, "RuleEngine/LoiteringDetector/ObjectIsLoitering", 64 - 1);
			break;
		}
		case IVCA_ET_COUNTER :
		{
			strncpy(event_topic, "RuleEngine/CountAggregation/CounterEvent", 64 - 1);	
			break;
		}
		case IVCA_ET_FR:
		case IVCA_ET_LPR:
		{
			strncpy(event_topic, "Analytics/FieldDetector/ObjectDetected", 64 - 1);
			break;
		}
		case 0:
		{
			strncpy(event_topic, "RuleEngine/CountAggregation/Counter", 64 - 1);	
			break;
		}
		case -1 :		// IDZ
		{
			strncpy(event_topic, "RuleEngine/FieldDetector/IntrusionDetection", 64 - 1);	
			break;
		}
		case -2 :		// IPZ
		{
			strncpy(event_topic, "RuleEngine/FieldDetector/IllegalParking", 64 - 1);	
			break;
		}
		default :
		{
			if ( _json_root != NULL )
				json_decref(_json_root);
			return FALSE;
		}
	}
	
	json_object_set_new(_json_root, "source", json_integer(channel));
	json_object_set_new(_json_root, "topic", json_string(event_topic));
	r_timestamp = timestamp + (double)(timestampl / 1000000.0);
	r_timestamp = floor(r_timestamp * 1000) / 1000;

	json_object_set_new(_json_root, "timestamp", json_real(r_timestamp));
	json_object_set_new(_json_root, "metadata", json_object());
	
	//metadata
	_json_meta = json_object_get(_json_root, "metadata");
	json_object_set_new(_json_meta, "annotations", json_array());
			
	
	//metadata.annotations
	_json_annot = json_object_get(_json_meta, "annotations");
	json_array_append_new(_json_annot, json_object());

	//metadata.annotations[i]
	_json_annot = json_array_get(_json_annot, 0);
	
	if ( event_type != IVCA_ET_COUNTER && event_type != 0)
	{
		json_object_set_new(_json_annot, "track_id", json_integer(object_id));
		json_object_set_new(_json_annot, "class", json_string(class_name));
		
		if ( type != -1 )		// not classic va(dmva)
		{
			json_object_set_new(_json_annot, "score", json_real(score));
		}
	}

	// Classic VA, AI
	if ( type == DVA_AI_DETECTION || type == -1 )
		json_object_set_new(_json_annot, "rule_id", json_integer(rule_id));

	switch ( event_type )
	{
		case IVCA_ET_DIR_POS :
		{
			json_object_set_new(_json_annot, "direction", json_string("Left"));
			break;
		}
		case IVCA_ET_DIR_NEG :
		{
			json_object_set_new(_json_annot, "direction", json_string("Right"));
			break;
		}
		case IVCA_ET_EXIT :
		{
			json_object_set_new(_json_annot, "is_inside", json_integer(0));
			break;
		}
		case IVCA_ET_ENTER :
		{
			json_object_set_new(_json_annot, "is_inside", json_integer(1));
			break;
		}
		case IVCA_ET_INTRUSION :
		{
			json_object_set_new(_json_annot, "is_inside", json_integer(2));
			break;
		}
		case IVCA_ET_STOPPED :
		{
			json_object_set_new(_json_annot, "time_interval", json_integer(0));
			json_object_set_new(_json_annot, "time_threshold", json_integer(stop_time*1000));
			break;
		}
		case IVCA_ET_REMOVED :
		{
			json_object_set_new(_json_annot, "time_interval", json_integer(0));
			json_object_set_new(_json_annot, "time_threshold", json_integer(remove_time*1000));
			break;
		}
		case IVCA_ET_LOITERED :
		{
			json_object_set_new(_json_annot, "time_interval", json_integer(0));
			json_object_set_new(_json_annot, "time_threshold", json_integer(loiter_time*1000));
			break;
		}
		case IVCA_ET_COUNTER :
		{
			json_object_set_new(_json_annot, "report_time_interval", json_integer(0));
			json_object_set_new(_json_annot, "reset_time_interval", json_integer(0));
			break;
		}
		case IVCA_ET_FR :
		{	
			json_object_set_new(_json_annot, "face", json_object());
			_json_face = json_object_get(_json_annot, "face");
			
			json_object_set_new(_json_face, "id", json_integer(fr_data->face_id));
			json_object_set_new(_json_face, "name", json_string(fr_data->name));
			
			json_object_set_new(_json_face, "groups", json_array());
			_json_face_groups = json_object_get(_json_face, "groups");

			json_array_append_new(_json_face_groups, json_string(fr_data->group_name));
			
			json_object_set_new(_json_face, "attribute", json_object());
			_json_face_attribute = json_object_get(_json_face, "attribute");
			json_object_set_new(_json_face_attribute, "age", json_integer(fr_data->age));
			json_object_set_new(_json_face_attribute, "gender", json_string(fr_data->gender));
			json_object_set_new(_json_annot, "search_score", json_real(fr_data->search_score));
			
			break;
		}
		case IVCA_ET_LPR :
		{
			json_object_set_new(_json_annot, "lp_text", json_string(lpr_data->lp_text));
			break;
		}
		case 0:
		{
			json_object_set_new(_json_annot, "count", json_integer(object_id));
			json_object_set_new(_json_annot, "report_time_interval", json_integer(0));
			json_object_set_new(_json_annot, "reset_time_interval", json_integer(0));
			break;
		}
	}

	if ( event_type != IVCA_ET_COUNTER && event_type != 0)
	{
		json_object_set_new(_json_annot, "bbox", json_array());

		//metadata.annotations[i].bbox
		_json_bbox = json_object_get(_json_annot, "bbox");
		json_array_append_new(_json_bbox, json_real(bbox[0]));
		json_array_append_new(_json_bbox, json_real(bbox[1]));
		json_array_append_new(_json_bbox, json_real(bbox[2]));
		json_array_append_new(_json_bbox, json_real(bbox[3]));
		//printf("\e[31m i[%d] class[%d] left[%f] top[%f] right[%f] bottom[%f] \e[0m\n", i, dva_event[i].u32Class, dva_event[i].fBbox[0], dva_event[i].fBbox[1],
		//																		dva_event[i].fBbox[2], dva_event[i].fBbox[3]);
	}
	json_dump_data = json_dumps(_json_root, JSON_COMPACT);
	json_decref(_json_root);
	
	memcpy(str, json_dump_data, strlen(json_dump_data));
	if ( json_dump_data != NULL)
		free(json_dump_data);

	//printf("\e[31m %s \e[0m\n", str);
	base64_encode(str, strlen(str), encode_data);
	//printf("\e[31m %s \e[0m\n", encode_data);

	*ret_len = strlen(encode_data);

	return TRUE;
}


#ifdef DEBUG_EVENTLOG_JBSHELL


static char log_test_help[] = "log_test [cnt] ";
static int log_test(int argc, char **argv)
{	
	int cnt = 0;
	
	if(argc < 2){
		printf("%s\n",log_test_help);
		return -1;
	}
	
	cnt = strtol(argv[1],NULL,0);
	
	while(--cnt)
	{
				
	}	
	return 0;
}
__commandlist(log_test,"log_test",log_test_help, log_test_help);


static char log_put_help[] = "log_put [type] [p1] [p2] [str]";
static int log_put(int argc, char **argv)
{	

	GTimeVal tv;
	gint type;
	gint param1;
	gint param2;
	gchar *text = NULL;

	if(argc < 4){
		printf("%s\n",log_put_help);
		return -1;
	}
	
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	type = atoi(argv[1]);
	param1 = atoi(argv[2]);
	param2 = atoi(argv[3]);
	
	if(argc > 4)
		text = argv[4];
		
		
	nf_eventlog_put_param( NULL, type, param1, param2, text );
	
	return 0;
}
__commandlist(log_put,"log_put",log_put_help, log_put_help);


#define MAX_LOG_DATA	128

static char log_get_help[] = "log_get [mode 0:time, 1:logid] [search_time/log_id]";
static int log_get(int argc, char **argv)
{	
	
	NF_LOG_PARAM 			param;
	NF_LOG_RESULT_HEADER	header;
	NF_LOG_DATA				*elem;

	memset( &param, 0x00, sizeof(NF_LOG_PARAM));
	memset( &header, 0x00, sizeof(NF_LOG_RESULT_HEADER));

	if(argc < 3){
		printf("%s\n",log_get_help);
		return -1;
	}
	
	param.mode = (atoi(argv[1]) == 1) ? 1 : 0;
	
	if(param.mode == NF_LOG_PARAM_MODE_TIME ) //0
		param.time_search.tv_sec = atoi(argv[2]);
	else
		param.log_id = (guint64)atoi(argv[2]);

	param.type_mask = 0xffffffff;
	param.request_count = MAX_LOG_DATA;
	
	memset( param.param1_mask, 0xff, sizeof(param.param1_mask) );
		
	elem = g_malloc0( sizeof(NF_LOG_DATA) * MAX_LOG_DATA );	
	nf_eventlog_get( &param, &header, elem, NULL);

	g_message( "[%s] result_count[%d]", __FUNCTION__, header.result_count);
	{
		guint i;
		for(i=0; i<header.result_count;i++)
		{
			g_message( "%s [%d] timestamp[%lld] type[%d] p1[%d]p2[%d] text[%16.16s]", 
					__FUNCTION__, i,
					elem[i].timestamp, elem[i].type, 
					elem[i].param1, elem[i].param2, elem[i].text );
		}
	}


	g_free(elem);
	
	return 0;
}
__commandlist(log_get,"log_get",log_get_help, log_get_help);


#ifdef DEBUG_NAND_ERR_LOG_JBSHELL

static char clear_nand_err_help[] = "clear_nand_err [type 1:recovery_err 2:disk_err 3:smart_err]";
static int clear_nand_err(int argc, char ** argv)
{
	int type;

	if(argc < 2){
		printf("%s\n",clear_nand_err_help);
		return -1;
	}

	type = atoi(argv[1]);
	if ( type < NAND_ERR_RECOVERY_ERROR || type > NAND_ERR_SMART_ERROR )
	{
		printf("%s\n",clear_nand_err_help);
		return -1;
	}
	nf_event_clear_nand_err_cnt(type);
	return 0;
}
__commandlist(clear_nand_err,"clear_nand_err",clear_nand_err_help, clear_nand_err_help);

static char read_nand_err_help[] = "read_nand_err [type 1:recovery_err 2:disk_err 3:smart_err]";
static int read_nand_err(int argc, char **argv)
{
	int type, cnt;

	if(argc < 2){
		printf("%s\n",read_nand_err_help);
		return -1;
	}
	
	type = atoi(argv[1]);	
	if ( type < NAND_ERR_RECOVERY_ERROR || type > NAND_ERR_SMART_ERROR )
	{
		printf("%s\n",read_nand_err_help);
		return -1;
	}

	nf_event_read_nand_err_cnt(type, &cnt,NULL);

	g_message( "[%s] result_count[%d]", __FUNCTION__, cnt);
	return 0;
}
__commandlist(read_nand_err,"read_nand_err",read_nand_err_help, read_nand_err_help);

static char inc_nand_err_help[] = "inc_nand_err [type 1:recovery_err 2:disk_err 3:smart_err]";
static int inc_nand_err(int argc, char ** argv)
{
	int type;

	if(argc < 2){
		printf("%s\n",inc_nand_err_help);
		return -1;
	}

	type = atoi(argv[1]);
	if ( type < NAND_ERR_RECOVERY_ERROR || type > NAND_ERR_SMART_ERROR )
	{
		printf("%s\n",inc_nand_err_help);
		return -1;
	}
	nf_event_inc_nand_err_cnt(type,NULL);
	return 0;
}
__commandlist(inc_nand_err,"inc_nand_err",inc_nand_err_help, inc_nand_err_help);

static char set_nand_err_help[] = "set_nand_err [type 1:recovery_err 2:disk_err 3:smart_err] [val]";
static int set_nand_err(int argc, char ** argv)
{
	int type, val;
	

	if(argc < 3){
		printf("%s\n",set_nand_err_help);
		return -1;
	}

	type = atoi(argv[1]);
	val = atoi(argv[2]);
	
	if ( type < NAND_ERR_RECOVERY_ERROR || type > NAND_ERR_SMART_ERROR )
	{
		printf("%s\n",set_nand_err_help);
		return -1;
	}
	
	nf_event_set_nand_err_cnt(type, val);
	return 0;
}
__commandlist(set_nand_err,"set_nand_err",set_nand_err_help, set_nand_err_help);

#endif /* DEBUG_NAND_ERR_LOG_JBSHELL */

#endif
