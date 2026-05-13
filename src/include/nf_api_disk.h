#ifndef __NF_API_DISK_H__
#define __NF_API_DISK_H__

#define ENABLE_DISK_PRESERVE	// choissi 2013-02-18 오후 6:00:40 

/* Callback events */
typedef enum _NF_SST_EVT_E {
	NF_SST_EVT_NONE				= 0 ,
	NF_SST_EVT_DISKFULL			= 1 ,
	NF_SST_EVT_OWSTART			= 2 ,
	NF_SST_EVT_OVERLAP			= 3 ,
	NF_SST_EVT_IOERR			= 4 ,
	NF_SST_EVT_FSSTARTBEG		= 16,
	NF_SST_EVT_FSSTARTCMPL		= 17,
	NF_SST_EVT_FORMATBEG		= 18,
	NF_SST_EVT_FORMATCMPL		= 19,
	NF_SST_EVT_RECOVERYBEG		= 20,
	NF_SST_EVT_RECOVERYCMPL		= 21,
	NF_SST_EVT_FSSTOPBEG		= 22,
	NF_SST_EVT_FSSTOPCMPL		= 23,
	NF_SST_EVT_DELDATABEG		= 24,
	NF_SST_EVT_DELDATACMPL		= 25,
	NF_SST_EVT_PRESERVEBEG		= 26,
	NF_SST_EVT_PRESERVECMPL		= 27,
	NF_SST_EVT_DISKCHKBEG		= 28,
	NF_SST_EVT_DISKCHKCMPL		= 29,
	NF_SST_EVT_DISKCHKBAD		= 30,
	NF_SST_EVT_DISKCHKCANCEL	= 31,
	NF_SST_EVT_RTLIMITCOMPL		= 32,
	NF_SST_EVT_GET_ESTG_INFO	= 33,
	NF_SST_EVT_RAIDIOERR		= 34,
	NF_SST_EVT_DISKIOERR		= 35,
	NF_SST_EVT_RTLIMITSTART		= 36,		// choissi 2010-01-25 오후 7:48:38
	NF_SST_EVT_PRIVACYBEG		= 38,
	NF_SST_EVT_PRIVACYCMPL		= 39,
	NF_SST_EVT_MAX				= 39		/* Maximum value (!= count) */
} NF_SST_EVT_E;

/* Progress types */
typedef enum _NF_SST_PRGT_E {
	NF_SST_PRGT_NONE			= 0,
	NF_SST_PRGT_FSSTART			= 1,
	NF_SST_PRGT_FSSTOP			= 2,
	NF_SST_PRGT_FORMAT			= 3,
	NF_SST_PRGT_RECOVERY		= 4,
	NF_SST_PRGT_DELETE			= 5,
	NF_SST_PRGT_PRESERVE		= 6,
	NF_SST_PRGT_CHKDISK			= 7,
	NF_SST_PRGT_RTLIMIT			= 8,
	NF_SST_PRGT_PRIVACY			= 9,
	NF_SST_PRGT_MAX				= 9		/* Maximum value (!= count) */
} NF_SST_PRGT_E;

/* Error codes */
typedef enum _NF_SST_ERR_E{
	NF_SST_ERR_NONE				= 0 ,
	NF_SST_ERR_SEQERR			= 1 ,
	NF_SST_ERR_INVPARAM			= 2 ,
	NF_SST_ERR_INVSTM			= 3 ,
	NF_SST_ERR_STMBSY			= 4 ,
	NF_SST_ERR_STMERR			= 5 ,
	NF_SST_ERR_DATALOCKED		= 6 ,
	NF_SST_ERR_DISKFULL			= 7 ,
	NF_SST_ERR_NODISK			= 8 ,
	NF_SST_ERR_NOMEM			= 9 ,
	NF_SST_ERR_NOSPACE			= 10,
	NF_SST_ERR_IO				= 11,
	NF_SST_ERR_TIMEOUT			= 12,
	NF_SST_ERR_NODATA			= 13,
	NF_SST_ERR_ENDDATA			= 14,
	NF_SST_ERR_EMPTY			= 15,
	NF_SST_ERR_OVERLAP			= 16,	
	NF_SST_ERR_MAX_AVAILABLE_SIZE	= 17,	/* about 20GB*/
	NF_SST_ERR_NO_IFRAME			= 18,	/* There is no iframe in this Macroblock */
	NF_SST_ERR_MAX_TIME_ADJUSTED 	= 19,	/* max time adjusted if fix_time_pos > 0 && size_limit */
	NF_SST_ERR_NO_VIDEO_DATA	 	= 20,	/* there is only log & pos_log data when query is operated.*/
	NF_SST_ERR_WRITE_LOG_TO_RAM	= 21,	/* */
	NF_SST_ERR_NAND_LOG_TO_SST_LIBRARY	= 22,	/* NAND_LOG_TO_SST_LIBRARY(ifsdrv.ko -> libsst.so) */

	//raid
	NF_SST_ERR_RAID_NO_CTRL 		= 23,
	NF_SST_ERR_RAID_NOT_ENOUGH_DISK	= 24,
	NF_SST_ERR_RAID_FW_UP_FAIL		= 25,
	NF_SST_ERR_RAID_NO_REBUILD		= 26,
	NF_SST_ERR_RAID_NO_EVENET		= 27,
	NF_SST_ERR_RAID_RAID_API		= 28,

	NF_SST_ERR_MAX				= 28	/* Maximum value (!= count) */ 
} NF_SST_ERR_E;

typedef void (*cb_event_fxn_t)(NF_SST_EVT_E event_num, gint arg, gpointer context); 


typedef enum  _NF_DISK_WRITE_MODE_E {
	NF_DISK_WRITE_MODE_WRITEONCE = 0,
	NF_DISK_WRITE_MODE_OVERWRITE = 1
} NF_DISK_WRITE_MODE_E;

typedef enum  _NF_DISK_FORMAT_MODE_E {
	NF_DISK_FORMAT_MODE_SYNC = 0,
	NF_DISK_FORMAT_MODE_FORMAT = 1
} NF_DISK_FORMAT_MODE_E;

/**
	@brief				디스크 저장 방법 설정					
	@param[in]	w_mode	디스크 저장 방법 
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_change_wmode( NF_DISK_WRITE_MODE_E w_mode, gint restart_wo, GError **error );

/**
	@brief				디스크 저장 방법 얻기
	
	@param[out]	w_mode	디스크 저장 방법
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_wmode( NF_DISK_WRITE_MODE_E *w_mode, GError **error );


/**
	@brief				레코딩 시간 제한 설정	
	@param[in]	rtlimit	
	@param[in]	d_mode	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_set_rtlimit( guint rtlimit, guchar d_mode, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error );

/**
	@brief				레코딩 시간 제한 얻기	
	@param[in]	rtlimit	
	@param[in]	d_mode	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_rtlimit( guint *rtlimit, guchar *d_mode, GError **error );



/**
	@brief				filesystem 이 온라인 인가?
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_is_online();

typedef struct _NF_SST_PROGRESS_T {
	gchar 	type;
	gchar 	pdid;
	gchar 	reserved[2];
	guint	current;
	guint	total;
	guint	time_elapsed;
	guint	time_remain;
	guint	privates[3];
} NF_SST_PROGRESS __attribute__ ((aligned(4))); 

/**
	@brief				디스크 progress 조회
	@param[in]	type	조회할 progress 종류
	@param[out]	progress 결과값
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_progress(NF_SST_PRGT_E type, NF_SST_PROGRESS  *progress); 

/**
	@brief				파일 시스템 시작 & 이벤트 통지 핸들러 등록
	@param[in]	format_flag	포맷 허가 플래그
	@param[in]	cb_event	이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
	
	nf_disk_get_info로 먼저 디스크 정보를 조회 한 후에 fs start를 할지 결정해야 한다.
	신규로 추가된 disk에 대해서 format을 할지 말지에 대한 여부도 필요.
	nf_filesystem_start가 호출되면 파일 시스템이 리커버리, 포맷등을 수행한다.
	이때 진행 상황(큰 스텝)은 cb_event로 전달 되고 상세 진행 사항은
	별도 api(nf_filesystem_get_progress)를 사용해서 폴링 한다.
*/
gboolean nf_filesystem_start( gint format_flag, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error );

/*
static void
fsstart_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FSSTARTBEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			((int *)context)[2] = 1;
			break;
		case SST_EVT_FSSTARTCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			((int *)context)[2] = 0;
			break;
		case SST_EVT_FORMATBEG:
			printf("%s: %s, disk=%u\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[3] = 1;
			break;
		case SST_EVT_FORMATCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[3] = 0;
			break;
		case SST_EVT_RECOVERYBEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			((int *)context)[4] = 1;
			break;
		case SST_EVT_RECOVERYCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[4] = 0;
			break;
		default:
			break;
	}
}

static char fs_start_help[] = "fs_start [format_flag]";
static int
fs_start(int argc, char **argv)
{
	u8 format_flag;
	int result;
	volatile int context[5];
	struct sst_progress_t progress;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", fs_start_help);
		return -1;
	}
	format_flag = atoi(argv[1]);

	// Do fs start in non-blocking mode. 
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_start(format_flag, 1,
			fsstart_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_fs_start() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(10000);
	while ( !context[0] ) {
		if ( context[3] ) {
			if ( !sst_get_progress(SST_PRGT_FORMAT, &progress) &&
					progress.ucType == SST_PRGT_FORMAT ) {
				printf("\rFormatting: %u%% (%u / %u)",
						progress.uCurrent * 100 / progress.uTotal,
						progress.uCurrent, progress.uTotal);
				fflush(stdout);
			}
		}
		else if ( context[4] ) {
			if ( !sst_get_progress(SST_PRGT_RECOVERY, &progress) &&
					progress.ucType == SST_PRGT_RECOVERY ) {
				printf("\rRecoverying: %u%% (%u / %u)",
						progress.uCurrent * 100 / progress.uTotal,
						progress.uCurrent, progress.uTotal);
				fflush(stdout);
			}
		}
		usleep(100000);
	}
	usleep(10000);
	printf("File system start completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
} 		
*/


/**
	@brief				파일 시스템 종료	
	@param[in]	cb_event	이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_stop( cb_event_fxn_t cb_event , gpointer cb_ctx, GError **error );


/**
	@brief				스토리지에 저장된 마지막 시간
	@param[out]	*time	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_lasttime( GTimeVal *time  );

/**
	@brief				레코딩 데이터 삭제
	@param[in]	utc_time_sec
	@param[in]	cb_event	이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_delete_data( guint utc_time_sec,
									 cb_event_fxn_t cb_event, gpointer cb_ctx, 
									 GError **error );


gboolean nf_filesystem_set_privacy_frame(void);

typedef struct _NF_PRIVACY_TIME_INFO_T {
	guint start_time;
	guint end_time;
} NF_PRIVACY_TIME_INFO;

/**
	@brief		레코딩 데이터 구간 삭제 가능한지 체크
	@param[in]	ch_mask		삭제 할 채널들의 비트 마스크
	@param[in]	*time_info	삭제 할 영상의 시작, 종료 시간
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_check_privacy_section( guint ch_mask,  NF_PRIVACY_TIME_INFO *time_info);

/**
	@brief		레코딩 데이터 구간 삭제
	@param[in]	ch_mask		삭제 할 채널들의 비트 마스크
	@param[in]	*time_info	삭제 할 영상의 시작, 종료 시간
	@param[in]	cb_event		이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_set_privacy_section( guint ch_mask,  NF_PRIVACY_TIME_INFO *time_info,
									 cb_event_fxn_t cb_event, gpointer cb_ctx, 
									 GError **error );

typedef struct _NF_DISK_PRIVACY_INFO_T {
	guint start_time;
	guint end_time;
	guint ch_mask;
	gchar reserved[4];
} NF_DISK_PRIVACY_INFO;

#define NF_DISK_PRIVACY_INFO_MAX_CNT 32

/*
<item key="disk.erase_section.idx" type="UINT" min="0" max="" val="0" />
<item key="disk.erase_section.E0.chmask" type="UINT" min="0" max="" val="0" />
<item key="disk.erase_section.E0.from" type="UINT" min="0" max="" val="0" />
<item key="disk.erase_section.E0.to" type="UINT" min="0" max="" val="0" />
*/

gboolean nf_filesystem_load_privacy_conf();
gboolean nf_filesystem_save_privacy_conf();

typedef struct _NF_DISK_INFO_T {
	guchar total_disks;
	guchar formatted_disks;
	guchar removed_disks;
	guchar added_disks;
	guchar group_disks[2];			// 그룹별 (internal,external) 디스크 갯수
	guchar reserved[2];
	guchar disk_state[2][16];		// 디스크 상태
									//	bit0: Existence flag
									//	bit1: Added flag
									//	bit2: Removed flag
									//	bit3: Formatted flag
									//  bit4: Conflict flag
	gchar  model_num[2][16][32];
	guint64  disk_size[2][16];			// Kbytes
	guchar sSerialNo[2][16][22];
	guchar ucPortNo[2][16];	
} NF_DISK_INFO __attribute__ ((aligned(4))); 

typedef enum _NF_DISK_INFO_FLAG_E {
	NF_DISK_INFO_FLAG_EXIST 	= 0x01,
	NF_DISK_INFO_FLAG_ADDED 	= 0x02,
	NF_DISK_INFO_FLAG_REMOVED 	= 0x04,
	NF_DISK_INFO_FLAG_FORMATTED = 0x08,
	NF_DISK_INFO_FLAG_CONFLICTED = 0x10,
	NF_DISK_INFO_FLAG_SYSTEM_CONFLICTED = 0x20,
	NF_DISK_INFO_FLAG_MIRROR 	= 0x40,
	NF_DISK_INFO_FLAG_SYSTEM_REMOVED 	= 0x80
} NF_DISK_INFO_FLAG_E;

/**
	@brief				디스크 정보 얻기
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_get_info( NF_DISK_INFO *disk_info, GError **error );

#if 1			// choissi 2010-08-11 오후 2:48:04  

/**
	@brief				디스크 포맷하기                         	
	@param[in]	mode	디스크 format 모드
	@param[in]	disk_id	해당 그룹의 디스크 번호 : 0~15
	@param[in]	cb_event	이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_format( gint mode, gint disk_id, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error);

#else

/**
	@brief				디스크 포맷하기                         	
	@param[in]	grp_id	디스크 그룹 아이디 0:내부장치 1:외부장치 0xff:모든 디스크
	@param[in]	disk_id	해당 그룹의 디스크 번호 : 0~15
	@param[in]	cb_event	이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_format( gint grp_id, gint disk_id, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error);

#endif

/**
	@brief				디스크 검사 시작 - 디스크 그룹단위로 동작
	@param[in]	grp_id	디스크 그룹 아이디 0:내부장치 1:외부장치
	@param[in]	cb_event	이벤트 콜백 함수
	@param[in]	cb_ctx		이벤트 콜백 context	
	@param[out]	error	return location for a #GError, or %NULL		
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_check_start(gint grp_id, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error);

/**
	@brief				디스크 검사 중지
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_check_stop( void );

/**
	@brief				디스크 뷰 얻기
	@param[in]	grp_id	디스크 그룹 아이디 0:내부장치 1:외부장치 0xff:모든 디스크
	@param[in]	disk_id	해당 그룹의 디스크 번호 : 0~15
	@param[in]	req_count 	요청한 갯수	
	@param[out]	*elm_size	한 블럭당 사이즈 (단위:kbytes)	
	@param[out]	*view_data  diskview 반환값
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_get_view( gint grp_id, gint disk_id, guint req_count, gint *elm_size, gchar *view_disk, GError **error );

/**
	@brief				하드 디스크 사용량 얻기
	@param[in]	grp_id	디스크 그룹 아이디 0:내부장치 1:외부장치 0xff:모든 디스크
	@param[in]	disk_id	해당 그룹의 디스크 번호 : 0~15
	@param[out]	used	사용한 용량 (단위:MB)
	@param[out]	total	전체 용량 (단위:MB)
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_get_usage( gint grp_id, gint disk_id, guint *used, guint *total, GError **error );

/**
	@brief				디스크 정보 얻기 ( is_full )
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_is_full( GError **error );


typedef struct _NF_DISK_REC_DISK_TIME_T {
	guint				min_time;
	guint				max_time;
	guint				used_mbcnt;
	guchar 				ucPortNo[2][16];
} NF_DISK_REC_DISK_TIME;

/**
	@brief				디스크 별 레코딩 min,max 얻기
	@param[in]	hide	숨김모드 설정 ( timeline hide와 동일)	
	@param[in]	grp_id	disk group_id
	@param[in]	disk_id	disk_id
	@param[out]	data	조회 결과
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_rec_disk_time( gint grp_id, gint disk_id, gint hide, NF_DISK_REC_DISK_TIME *data, GError **error);


typedef struct _NF_DISK_REC_TIME_T {
	guint             min_time;
	guint             min_time_ch[128];
	guint             max_time;
	guint             max_time_ch[128];
	guchar 		ucPortNo[2][16];
} NF_DISK_REC_TIME;

/**
	@brief				디스크 채널별 레코딩 min,max 얻기
	@param[in]	hide	숨김모드 설정 ( timeline hide와 동일)	
	@param[out]	data	조회 결과
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_rec_time( gint hide, NF_DISK_REC_TIME *data, GError **error);


typedef enum _NF_SMART_E {
	NF_SMART_STATUS_OK		=0,
	NF_SMART_STATUS_FAIL	=1,
	NF_SMART_NOT_SUPPORTED	=2,
 	NF_SMART_NO_DISK		=3
} NF_SMART_E;

/**
	@brief	proc 
	@param[out]	proc 데이터
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_proc_all(void);

/**
	@brief				NAND data 데이터 얻기
	@param[out]	data	SMART 상태 원본 데이터
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_nand_log(void);

/**
	@brief				디스크 SMART 데이터 얻기(smartctl)
	@param[out]	data	SMART 상태 원본 데이터
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_smartctl(void);

/**
	@brief	proc 
	@param[out]	get one proc 데이터
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_proc(const gchar *string, const gchar *string2, const gchar *string3);

/**
	@brief				sysdb data 데이터 얻기
*/
gboolean nf_get_sysdb(void);


typedef struct _NF_SMART_STATUS_T
{
	/* SMART status for each disk. */
	gchar	status[2][16];
	/*
	 * -1 if a drive does not support temperature reporting.
	 *  Ignore this field if ucStatus[drive] field is NOT_SUPPORTED or NO_DISK.
	 */
	gint	temperature[2][16];
} NF_SMART_STATUS;


/**
	@brief				디스크 SMART 상태 얻기
	@param[out]	status	SMART 상태 값
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_status (NF_SMART_STATUS *status, GError **error);


typedef struct _NF_SMART_ATTR {
	guint	type;		//1:Pre-fail, 0:Old_age
	guint	flags;
	
	guint	value;
	guint	worst;
	guint	threshold;
	guint	status;		// 0:normal 1:in_the_past 2:check 3:failing_now
	
	guint64 raw;			//meaning = raw_value
	
} NF_SMART_ATTR;

typedef struct _NF_SMART_DISK_INFO {
	
	GTimeVal	update_time;							//sec
	guint disk_status;								// 0:normal 1:in_the_past 2:check 3:failing_now
	
	NF_SMART_ATTR	raw_read_error_rate;			//print value & threshold
	NF_SMART_ATTR	spin_up_time;					//print raw = last time
	NF_SMART_ATTR	reallocated_sector_ct;			//print raw
	NF_SMART_ATTR	seek_error_rate;				//print value & threshold
	
	NF_SMART_ATTR	reallocation_event_ct;
	NF_SMART_ATTR	current_pending_sector;
	NF_SMART_ATTR	offline_uncorrectable;
	
	guint	start_stop_cnt;
	guint	power_on_hours;
	guint	spin_retry_cnt;
	guint	power_cycle_cnt;
	guint	temperature_celsius;					//degree
	guint	temperature_status;
	
	char	raw_text[4096];
	
	guchar 		ucPortNo[2][16];
} NF_SMART_DISK_INFO;

typedef enum _NF_SMART_ID_E {
	RAW_READ_ERROR_RATE 		= 0x01,
	THROUGHPUT_PERFORMANCE 	= 0x02,
	SPIN_UP_TIME				= 0x03,
	START_STOP_COUNT			= 0x04,
	REALLOCATION_SECTOR_COUNT 	= 0x05,
	SEEK_ERROR_RATE				= 0x07,
	SEEK_TIME_PERFORMANCE 		= 0x08,
	POWER_ON_HOURS_COUNT 		= 0x09,
	SPIN_RETRY_COUNT 			= 0x0A,
	POWER_CYCLE_COUNT 			= 0x0C,
	REPORTED_UNCORRECT_COUNT	= 0xBB,
	AIRFLOW_TEMPERATURE		= 0xBE,
	POWER_OFF_RETRACT_COUNT 	= 0xC0,
	LOAD_CYCLE_COUNT 			= 0xC1,
	TEMPERATURE 				= 0xC2,
	REALLOCATED_EVENT_COUNT 	= 0xC4,
	CURRENT_PENDING_SECTOR		= 0xC5,
	OFFLINE_UNCORRECTABLE 		= 0xC6,
	ULTRA_DMA_CRC_ERROR_COUNT	= 0xC7
} NF_SMART_ID_E;

typedef enum _NF_SMART_STATUS_E {
	SMART_NORMAL 		= 0,
	SMART_IN_THE_PAST 	= 1,
	SMART_CHECK		= 2,
	SMART_FAILING_NOW	= 3,
	SMART_UNKNOWN		= 4
} NF_SMART_STATUS_E;

#define SMART_READ_INTERVAL 60

// 100 -> 50     reallocated sector count + current pending sector count + reported_ uncorrect count
#define REALLOCATED_SECTOR_CNT_THRESHOLD 50
#define RAW_READ_ERR_RATE_THRESHOLD 10000

/**
	@brief				디스크 SMART 상태 얻기
	@param[out]	info	SMART 상태 값
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_info ( gint grp_id, gint disk_id, NF_SMART_DISK_INFO *info, GError **error);


/**
	@brief				디스크 SMART 데이터 얻기
	@param[in]	grp_id	디스크 그룹 아이디 0:내부장치 1:외부장치
	@param[in]	disk_id	해당 그룹의 디스크 번호 : 0~15
	@param[out]	data	SMART 상태 원본 데이터
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_data(  gint grp_id, gint disk_id, gchar data[4096], GError **error);


/**
	@brief				디스크 SMART 데이터 체크
	@param[out]	data	SMART 상태 원본 데이터
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_validate_data( gchar data[4096] );

/**
	@brief				디스크 SMART 상태 얻기 (unc)
	@param[out]	status	SMART 상태 값
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_unc ( guint *key, GError **error );


/**
	@brief				디스크 SMART 상태 얻기 (boot unc)
	@param[out]	status	SMART 상태 값
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_boot_unc ( guint *key, GError **error );

/******************************************************************************/


/******************************************************************************/

/* RAID MODE */
typedef enum _NF_ESTG_RAID_MODE_E {
	NF_ESTG_RAID_MODE_MIRROR	= 3,
	NF_ESTG_RAID_MODE_BIG		= 0,
	NF_ESTG_RAID_MODE_JBOD 		= 1
	
} NF_ESTG_RAID_MODE_E; 

/* DISK STATUS */
typedef enum _NF_ESTG_DISK_STATUS_E {
	NF_ESTG_DISK_STATUS_ONLINE		= 0,
	NF_ESTG_DISK_STATUS_REBUILD		= 1,
	NF_ESTG_DISK_STATUS_VERIFY		= 2,
	NF_ESTG_DISK_STATUS_UNPLUGGED	= 3	
	
} NF_ESTG_DISK_STATUS_E; 

/* ERROR CODES */
typedef enum _NF_ESTG_ERR_E {

	NF_ESTG_ERR_NONE		= 0,
	NF_ESTG_ERR_DID			= 1,
	NF_ESTG_ERR_MBOX 		= 2,
	NF_ESTG_ERR_SERIAL 		= 3, 
	NF_ESTG_ERR_SETMODE		= 4,
	NF_ESTG_ERR_REINIT		= 5,
	NF_ESTG_ERR_CABLE		= 6,
	NF_ESTG_ERR_START		= 7,
	NF_ESTG_ERR_STOP		= 8,
	NF_ESTG_ERR_BUZZER		= 9
	
} NF_ESTG_ERR_E; 

#define NF_ESTG_DISK_MAX		(8)	// libsst.h disk_max 8
#define NF_ESTG_RAID_MAX		(4)	// libsst.h raid_max 4

typedef struct _NF_ESTG_DISK_T {
	guchar	model[40];
	guchar	serial[20];
	guint64	capacity;
	
} __attribute__((packed)) NF_ESTG_DISK ; 

typedef struct _NF_ESTG_DISK_INFO_T {

	NF_ESTG_DISK 	disk[NF_ESTG_DISK_MAX];
	
} NF_ESTG_DISK_INFO;


typedef struct _NF_ESTG_RAID_T {

	guchar	mode;
	guchar	pin_conf;	
	guchar	set_result;

} __attribute__((packed)) NF_ESTG_RAID ;	

typedef struct _NF_ESTG_RAID_MODE_T {

	NF_ESTG_RAID	raid[NF_ESTG_RAID_MAX];

} NF_ESTG_RAID_MODE;


typedef struct _NF_ESTG_PROGRESS_T {

	guint64	complete_sector; 
	guint64	remain_sector;
	guint	remain_time;
	guchar	percent;
	
} __attribute__((packed)) NF_ESTG_PROGRESS ;

typedef struct _NF_ESTG_PROGRESS_STATE_T {

	NF_ESTG_PROGRESS	progress[NF_ESTG_RAID_MAX];

} NF_ESTG_PROGRESS_STATE;


typedef struct _NF_ESTG_DISK_STATUS_T {

	guchar	status[NF_ESTG_DISK_MAX];

} NF_ESTG_DISK_STATUS;


typedef struct _NF_ESTG_CABLE_T {

	guchar type;

} NF_ESTG_CABLE; 

 
typedef void (*cb_estg_fxn_t)(int result, gpointer context);

gboolean nf_estg_get_info(int estg_id, NF_ESTG_DISK_INFO *disk_info, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error );
gboolean nf_estg_get_mode(int estg_id, NF_ESTG_RAID_MODE *raid_mode, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
gboolean nf_estg_set_mode(int estg_id, NF_ESTG_RAID_MODE *raid_mode, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
gboolean nf_estg_get_status(int estg_id, NF_ESTG_DISK_STATUS *disk_status, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
gboolean nf_estg_get_progress(int estg_id, NF_ESTG_PROGRESS_STATE *progress_state, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

gboolean nf_estg_reinit(int estg_id, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

#if 0		// do not use
gboolean nf_estg_set_cable(int estg_id, NF_ESTG_CABLE *type, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
gboolean nf_estg_get_cable(int estg_id, NF_ESTG_CABLE *type, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
gboolean nf_estg_set_buzzer(int estg_id, int time, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
#endif


void nf_set_using_usb(gboolean val);

// -----------------------------------------------------------------------------
// 2010-09-15 오후 4:19:38  DVR AVI PLAYER
// -----------------------------------------------------------------------------

#ifdef ENABLE_DVR_AVI_PLAYER

typedef struct  _NF_AVI_PLAYER_DEV_INFO_T {
	guchar	ucDevId;
	guchar	ucDevType;			/* SCSI peripheral device type. 00h to 1Fh */
	guchar	ucMediaChecked;
	guchar	ucReserved[3];
	gushort ucHostNum;
	guint64	ulMediaSize;		/* Valid only when MediaChecked is set. */
	guint64	ulMediaAvail;		/* Available media space */
	guchar	sVendor[9];
	guchar	sProduct[17];
	guchar	sRev[5];
	guchar 	sDev[6];
	guchar	ucReserved2[2];
} __attribute__((packed)) NF_AVI_PLAYER_DEV_INFO ;

/* MUL arch Player */
typedef struct _NF_AVI_PLAYER_LOG_INFO_T
{
	guchar burning_type;			//burning or Erasing &burning
	guchar device[17];			//archiving device name
	guchar reservde[2];
	guint reserved_at;			//reserved time
	guint archived_at;			//archived time
	guint movie_start;			//video start time in this file
	guint movie_end;			//video end time in this file
	guint64 ch_sel;				//selected channel information
	guchar log;					//log select info
	guchar pos_log;				//pos_log select info
	guchar tag[32];				//tag info
	guchar user[32];				//user name info
	guchar memo[256];			//memo info
	guchar dst;					//dst info
	guchar timezone;				//timezone info
	guint extra_mask;			//extra mask filed info(only use in sst)
	guchar created_cnt;			//created file count
	guchar reserved2[2];
	guint file_size;
}NF_AVI_PLAYER_LOG_INFO;

#define MAX_VIDEO_CHAN 32
typedef struct _NF_AVI_PLAYER_INFO_IN_JUNK_T
{
	guint v_frames[MAX_VIDEO_CHAN];	//count of video frame
	guint a_frames[MAX_VIDEO_CHAN];	//count of audio frame
	guint t_frames[MAX_VIDEO_CHAN];	//count of txt frame
	guint v_size[MAX_VIDEO_CHAN];		//size of video data
	guint a_size[MAX_VIDEO_CHAN];		//size of audio data
	guint t_size[MAX_VIDEO_CHAN];		//size of txt data
	guint a_header_size[MAX_VIDEO_CHAN];// size of audio header(only in mul file)
	guint start_time_ch[MAX_VIDEO_CHAN];	//start time for channel
	guint end_time_ch[MAX_VIDEO_CHAN];	//end time for channel
	NF_AVI_PLAYER_LOG_INFO arch_log_info;
}NF_AVI_PLAYER_INFO_IN_JUNK;


/* Callback function type */
typedef void (*avi_player_cb_fxn_t)(gint result, gpointer context);


/**
	@brief				DVR AVI PLAYER START ( 기능 시작시 호출 )	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_start(void);

/**
	@brief				DVR AVI PLAYER STOP ( 기능 종료시 호출 )	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_finish(void);


/**
	@brief				DVR AVI PLAYER 디바이스 콜백 등록
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_set_notify(avi_player_cb_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

/**
	@brief				DVR AVI PLAYER 디바이스 콜백 해제	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_unset_notify(void);

/**
	@brief				DVR AVI PLAYER 디바이스 목록 얻기 (최대 4개)
	@param[out]	dev_table	드바이스 목록 버퍼(사용자가 할당 arch_dev_info_t[4])
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_get_list(NF_AVI_PLAYER_DEV_INFO *dev_table, 
								avi_player_cb_fxn_t fxn_cb, gpointer ctx_cb, GError **error);


/**
	@brief				DVR AVI PLAYER 디바이스 목록 얻기 (최대 4개) with free size
	@param[out]	dev_table	드바이스 목록 버퍼(사용자가 할당 arch_dev_info_t[4])
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_get_size(NF_AVI_PLAYER_DEV_INFO *dev_table, 
								avi_player_cb_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

/**
	@brief				디바이스에 포함된 AVI 파일 갯수 조회	
	@param[in]	dev_id	디바이스 ID	
	@return	gint		AVI 파일 갯수, 음수는 에러 코드
*/
gint nf_avi_player_get_file_num(guint dev_id);


/**
	@brief				디바이스에 포함된 AVI 파일 목록 조회
	@param[in]	dev_id	디바이스 ID	
	@param[out]	avi_list	avi_list ( 사용자가 할당, 512개 x 256자 )
	@return	gint		AVI 파일 갯수, 음수는 에러 코드
*/
gint nf_avi_player_get_file_list(guint dev_id, guchar **avi_file_list);


/**
	@brief				DVR AVI PLAYER가 재생할 파일 지정
	@param[in]	full_path	avi 파일이 마운트 된 full path
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_set_play_filename(gchar *full_path, GError **error);


/**
	@brief				DVR MUL PLAYER가 thread가 모두 생성되었는지 확인
	@return	gboolean	%TRUE on success, %FALSE if threads are not ready
*/
gboolean nf_avi_player_check_thread_start(void);


/**
	@brief				DVR MUL PLAYER할 channel을 선택한다. ch는 31이하여야 한다.
	@return	gboolean	%TRUE on success, %FALSE if channel number is bigger than MAX_VIDEO_CHAN(32)
*/
gboolean nf_avi_player_mul_set_ch(guint ch);


/**
	@brief				DVR MUL PLAYER에서 file의 정보를 얻어 온다.
	@param[in]	full_path	정보를 얻기 원하는 파일의 full path
	@param[out]			파일의 정보 in info_in_junk_t
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_mul_get_info(gchar *full_path, NF_AVI_PLAYER_INFO_IN_JUNK *info, GError **error);


/**
	@brief				DVR AVI PLAYER에서 MD5를 확인 한다.
	@param[in]	full_path	정보를 얻기 원하는 파일의 full path
	@return	gboolean		0 on success, -1 if an error occurred, -2 verify check result is error.
*/
gint nf_avi_player_check_md5(gchar *full_path, GError **error);



/**
	@brief				DVR AVI PLAYER 에서 MD5의 progress를 return 한다.
	@return	gint			MD5 progress percentage && -1 if MD5 progress is completed
*/
gint nf_avi_player_md5_get_progress(void);


/**
	@brief				DVR AVI PLAYER에서 file play가능 유무를 확인 한다.
	@param[in]	full_path	정보를 얻기 원하는 파일의 full path
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gint nf_avi_player_check_file(gchar *full_path, GError **error);

#endif //ENABLE_DVR_AVI_PLAYER


#ifdef ENABLE_DISK_PRESERVE

#define NF_DISK_PRESERVE_PB_MODE_NORMAL     0
#define NF_DISK_PRESERVE_PB_MODE_PRESERVE   1

/**
	@brief			playback 모드를 설정한다.
	@param[in]	    mode   0:normal 1:preserved
	@param[in]		param1 preserved sessid	
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_preserve_pb_mode( guint mode, guint param1);


/**
	@brief			preserved list 수를 조회 한다.
	@return	gint		result count  on success, -1 if an error occurred
*/
gint nf_disk_preserve_list_cnt();

typedef struct _NF_DISK_PRESERVE_INFO_T {
	guint no;
	guint sess_id;
	guint start_timestamp;
	guint end_timestamp;
	guint mb_cnt;
	guint status;
} NF_DISK_PRESERVE_INFO;

/**
	@brief			preserved list를 얻어 온다.
	@param[in]	    start_no   start index ( no )
	@param[in]		count      request count
	@param[in]		pbuff      result buffer   NF_DISK_PRESERVE_INFO * count 
	@return	gint		result count  on success, -1 if an error occurred
*/
gint  nf_disk_preserve_list_get(  guint start_no, guint count, NF_DISK_PRESERVE_INFO *pbuff);
#endif	// ENABLE_DISK_PRESERVE

gboolean nf_set_passwd(unsigned char* passwd , unsigned int size);
gboolean nf_filesystem_set_rtlimit_runtime( guint rtlimit);

#endif

