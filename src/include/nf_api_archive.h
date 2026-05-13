#ifndef __NF_API_ARCHIVE_H__
#define __NF_API_ARCHIVE_H__

#include <glib.h>
#include "nf_common_util.h"

#include "nf_util_ftp.h"

#define NF_ARCH_MAX_RCOUNT		(1024)

typedef enum _NF_ARCH_TYPE_E {
	NF_ARCH_TYPE_SNAP				= 0,	/* Snapshot archiving */
	NF_ARCH_TYPE_AVI				= 1,	/* AVI movie archiving */
	NF_ARCH_TYPE_RAW				= 2		/* RAW movie archiving */
} NF_ARCH_TYPE_E;
 
/* Direction for archiving list mangement */
typedef enum _NF_ARCH_DIR_E {
	NF_ARCH_DIR_FORWARD				= 0,
	NF_ARCH_DIR_BACKWARD			= 1
} NF_ARCH_DIR_E;

/* Extra mask for querying */
typedef enum _NF_ARCH_EMASK_E {
	NF_ARCH_EMASK_LOG				= 0x01,
	NF_ARCH_EMASK_TEXT				= 0x02,
	NF_ARCH_EMASK_RI				= 0x04,
	NF_ARCH_EMASK_CODEC				= 0x08,
	NF_ARCH_EMASK_PLAYER			= 0x10,
	NF_ARCH_EMASK_NO_AUDIO_TEXT		= 0x20,		// 2010-02-02 오후 10:41:43 choissi for SME-2220
	NF_ARCH_EMASK_POS_LOG			= 0x40,	// 2010-03-18 오후 2:17:29  pos log 
	NF_ARCH_EMASK_MULTI_ARCH	= 0x80,
} NF_ARCH_EMASK_E;

/* Archiving status */
typedef enum _NF_ARCH_STATUS_E {
	NF_ARCH_STATUS_NOTHING			= 0,
	NF_ARCH_STATUS_QUERYING			= 1,
	NF_ARCH_STATUS_ERASING			= 2,
	NF_ARCH_STATUS_EXTRACTING		= 3,
	NF_ARCH_STATUS_BURNING			= 4,
	NF_ARCH_STATUS_WRITING		= 5,
	NF_ARCH_STATUS_SENDING		= 6
} NF_ARCH_STATUS_E;

/* Error codes */
typedef enum _NF_ARCH_ERROR_E {
	NF_ARCH_ERR_NONE				= 0,
	NF_ARCH_ERR_INVMODE				= 1,
	NF_ARCH_ERR_INVDEV				= 2,
	NF_ARCH_ERR_INVPARAM			= 3,
	NF_ARCH_ERR_INVMEDIA			= 4,
	NF_ARCH_ERR_MEDIA_FULL			= 5,
	NF_ARCH_ERR_MEDIA_NOTERASABLE	= 6,
	NF_ARCH_ERR_FAIL				= 7,
	NF_ARCH_ERR_FAIL_DEVERR			= 8,
	NF_ARCH_ERR_REQMORE				= 9,
	NF_ARCH_ERR_LISTEMPTY			= 10,
	NF_ARCH_ERR_LISTFULL			= 11,
	NF_ARCH_ERR_LISTADDED			= 12,
	NF_ARCH_ERR_CANCELED			= 13,
	NF_ARCH_ERR_DATALOCKED			= 14,
	NF_ARCH_ERR_MAX_AVAILABLE_SIZE	= 15,
	
	//  added by chcha for ftp
	NF_ARCH_ERR_FTP_CONN = 16,
	NF_ARCH_ERR_FTP_AUTH = 17,
	NF_ARCH_ERR_FTP_FAIL = 18,

	// added by yesing for temp multi session error
	NF_ARCH_ERR_MULTISESSION_ERROR = 19,
	NF_ARCH_ERR_DISABLE_MULTISESSION= 20,
	NF_ARCH_ERR_MAX_TIME_ADJUSTED= 21,
	NF_ARCH_ERR_NO_VIDEO_DATA = 22,
	NF_ARCH_ERR_DATA_OR_BUG = 23
	
} NF_ARCH_ERROR_E;

/* Callback function type */
typedef void (*cb_arch_fxn_t)(gint result, gpointer context);

typedef struct _NF_ARCH_SNAP_INFO_T {
	guint16			arch_id;			/* Archive entry id. */
	guchar			channel;			/* Channel number of the image. */
	guchar			reserved;
	guint32			extract_count;		/* Extracted count. */
	guint64			time_arch;			/* Archived time. */
	guint64			time_image;			/* Image time. */
	guint32			total_size;			/* Image size in bytes. */
	gpointer		pimage;				/* Pointer for image buffer. */
	gchar			tag[32];
	gchar			user[32];
	gchar			memo[256];
} __attribute__((packed)) NF_ARCH_SNAP_INFO;

typedef struct _NF_ARCH_AVI_INFO_T {
	guint16			arch_id;			/* Archive entry id. */
	guchar			reserved[2];
	guint32			extract_count;		/* Extracted count. */
	guint64			time_arch;			/* Archived time. */
	guint64			time_beg;			/* AVI start time. */
	guint64			time_end;			/* AVI end time. */
	guint64			channel_mask;		/* Bitwise Video/Audio data channel mask. */
	guchar			extra_mask;			/* Extra mask. ARCH_EMASK_XXX. */
	guchar			reserved2[3];
	guint32			total_size;			/* Total size in Kbytes. */
	guint32			ch_size[64];		/* Sizes of each channel in Kbytes. */
	gchar			tag[32];
	gchar			user[32];
	gchar			memo[256];
} __attribute__((packed)) NF_ARCH_AVI_INFO;

#if 1	// choissinf 2008-10-22 오후 8:16:10 
typedef struct _NF_ARCH_DEV_INFO_T {
	guchar			dev_id;
	guchar			dev_type;			/* SCSI peripheral device type. 00h to 1Fh */
	guchar			media_checked;
	guchar			reserved[5];
	guint64			media_size;			/* Valid only when MediaChecked is set. */
	guint64			media_avail;		/* Available media space */
	gchar			vendor[9];
	gchar			product[17];
	gchar			rev[5];	
	gchar			dev_name[6];		/* /dev/sda1 --> "sda1" */
	guchar			reserved2[2]; 	
} __attribute__((packed)) NF_ARCH_DEV_INFO ;
#else
typedef struct _NF_ARCH_DEV_INFO_T {
	guchar			dev_id;
	guchar			dev_type;			/* SCSI peripheral device type. 00h to 1Fh */
	guchar			media_checked;
	guchar			reserved[5];
	guint64			media_size;			/* Valid only when MediaChecked is set. */
	guint64			media_avail;		/* Available media space */
	gchar			vendor[9];
	gchar			product[17];
	gchar			rev[5];
	guchar			reserved2;
} __attribute__((packed)) NF_ARCH_DEV_INFO ;
#endif

typedef struct _NF_ARCH_PROGRESS_T {
	guchar			status;				/* ARCH_STATUS_XXX */
	guchar			reserved[3];
	guint32			current;			/* %% progress = uCurrent * 100 / uTotal */
	guint32			total;
	guint32			time_elapsed;
	guint32			time_remain;
} __attribute__((packed)) NF_ARCH_PROGRESS;

typedef struct _NF_ARCH_SNAP_PARAM_T {	
	GTimeVal  		snap_time;		// play start time
	guint			ch;
	
	guint			image_size;
	gpointer		image;

	gchar			tag[32];
	gchar			user[32];
	gchar			memo[256];
		
} __attribute__ ((packed)) NF_ARCH_SNAP_PARAM; 

typedef struct _NF_ARCH_AVI_PARAM_T {	
	GTimeVal  		start_time;		// play start time
	GTimeVal 		end_time;		// play end time
		
	guint32			ch_mask;
	guint32			audio_mask;
	
	guchar			inc_log;		
	guchar			inc_text;
	guchar			inc_ri;
	guchar			inc_codec;	
	guchar			inc_player;		
	guchar			no_audio_text;
	guchar			inc_pos_log;
	guchar			reserved[1];
	
	gchar			tag[32];
	gchar			user[32];
	gchar			memo[256];
		
	cb_arch_fxn_t	cb_event;		// (if blocking mode, null)
	gpointer		cb_context;		
} __attribute__ ((packed)) NF_ARCH_AVI_PARAM ; 


typedef struct NF_ARCH_CIPHER_PARAM_T {	
		
	guint32			enable;	
	guint32			algo; 			/* always 0 */
	
	gchar			passwd[32+1];	/* passwd (MAX 16 chars) */
	gchar			iv[16+1];		/* passwd */
	
} __attribute__ ((packed)) NF_ARCH_CIPHER_PARAM ; 

/**
	@brief				아카이브 매니저 시작
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_manager_start();

/**
	@brief				아카이브 매니저 종료
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_manager_stop();


/**
	@brief				스냅샷 쿼리	
	@param[in]	param	파라메터
	@param[out]	info	스냅샷 결과(api 사용자가 미리 buffer 할당해서 호출)	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_query_snap( NF_ARCH_SNAP_PARAM *param, 
						NF_ARCH_SNAP_INFO *info, GError **error);
/**
	@brief				AVI 쿼리	
	@param[in]	param	파라메터
	@param[out]	info	스냅샷 결과(api 사용자가 미리 buffer 할당해서 호출)	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_query_avi( NF_ARCH_AVI_PARAM *param, 
						NF_ARCH_AVI_INFO *info, GError **error);
gboolean 
nf_arch_query_avi_ex( NF_ARCH_AVI_PARAM *param, NF_ARCH_AVI_INFO *info, 
					GError **error, guint limitMB, guint option, guint whole_check );


typedef struct _NF_ARCH_PB_AVI_PARAM_T {	
		
	guint32			ch_mask;
	guint32			audio_mask;
	
	guchar			inc_log;		
	guchar			inc_text;
	guchar			inc_ri;
	guchar			inc_codec;	
	guchar			inc_player;
	guchar			no_audio_text;
	guchar			inc_pos_log;
	guchar			reserved[1];
	
	gchar			tag[32];
	gchar			user[32];
	gchar			memo[256];
		
} __attribute__ ((packed)) NF_ARCH_PB_AVI_PARAM ; 


/**
	@brief				
	@param[in]	param	파라메터	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_start_avi( NF_ARCH_PB_AVI_PARAM *param, GError **error);


/**
	@brief				
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_stop_avi( GError **error  );

/**
	@brief				AVI 쿼리	
	@param[out]	info	스냅샷 결과(api 사용자가 미리 buffer 할당해서 호출)	
	@param[in]	cb_event	callback function
	@param[in]	cb_context	callback context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_query_avi( NF_ARCH_AVI_INFO *info, cb_arch_fxn_t cb_event, gpointer cb_context, GError **error);

gboolean 
nf_arch_pb_query_avi_ex( NF_ARCH_AVI_INFO *info, cb_arch_fxn_t cb_event, gpointer cb_context, GError **error,
						guint limitMB, guint option, guint whole_check );
						
#if 0	// 보류 2008-08-12 오후 3:31:06
	int arch_pb_start_avi(u64 chmask, u8 emask, s8 *tag, s8 *user, s8 *memo);
	int arch_pb_stop_avi(void);
	int arch_pb_query_avi(arch_avi_info_t *info, arch_cb_fxn_t fxn_cb, void *ctx_cb);
#endif


/**
	@brief				직전에 쿼리한 info 수정
	@param[in]	tag		수정할 Tag
	@param[in]	user	수정할 User
	@param[in]	memo	수정할 Memo
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_info_modify(gchar *tag, gchar *user, gchar *memo);

/**
	@brief				직전에 쿼리한 info 해제
	@param[in]	fxn_cb	콜백 function pointer (if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_info_destroy(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

/**
	@brief				직전에 쿼리를 sst에 추가
	@param[in]	fxn_cb	콜백 function pointer (if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_info_add(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);




/**
	@brief				아카이브 리스트 목록의 갯수를 가져온다.
	@param[in]	fxn_cb	콜백 function pointer (if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@return	gint		negative if an error occurred,  
						non negative total entry count
*/
gint
nf_arch_list_get_count(NF_ARCH_TYPE_E type, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb);

/**
	@brief				아카이브 리스트에서 snapshot 목록 가져오기
	@param[in]	dir		조회할 목록의 방향(NF_ARCH_DIR_E)
	@param[in]	arch_id	조회할 아카이브 아이디 (0:처음부터, 0xffff:끝부터)
	@param[in]	rcount	요청한 갯수
	@param[in]	get_image	이미지 데이터 포함
	@param[out]	list	조회 결과물이 담길 버퍼(사용자가 할당해서 제공)
						sizeof(arch_snap_info_t)*rcount
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@return	gint		negative if an error occurred,
						non negative total entry count
*/
gint 
nf_arch_list_get_snap(NF_ARCH_DIR_E dir, guint16 arch_id, guint16 rcount, 
					gboolean get_image, NF_ARCH_SNAP_INFO *list, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb);

/**
	@brief				아카이브 리스트에서 avi 목록 가져오기
	@param[in]	dir		조회할 목록의 방향(NF_ARCH_DIR_E)
	@param[in]	arch_id	조회할 아카이브 아이디 (0:처음부터, 0xffff:끝부터)
	@param[in]	rcount	요청한 갯수
	@param[out]	list	조회 결과물이 담길 버퍼(사용자가 할당해서 제공 
						sizeof(arch_avi_info_t)*rcount
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@return	gint		negative if an error occurred,
						non negative total entry count
*/
gint 
nf_arch_list_get_avi(NF_ARCH_DIR_E dir, guint16 arch_id, guint16 rcount, 
					NF_ARCH_AVI_INFO *list, cb_arch_fxn_t fxn_cb, gpointer ctx_cb);

/**
	@brief				아카이브 리스트에서 entry 삭제
	@param[in]	type	아카이브 타입 (NF_ARCH_TYPE_E)
	@param[in]	arch_id	아카이브 아이디
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_list_delete(NF_ARCH_TYPE_E type, guint16 arch_id, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

/**
	@brief				set FTP inf
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_set_ftp_info( NF_FTP_CLIENT_REQ *req, GError **error );

/*   
	@brief		 ftp connection test = creating directory, changing directory, writing ftp_test file
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_ftp_test(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

/*   
	@brief		set archive data format
	@prarm[in]	data_format	1:multi archiving, 0:avi archiving
	@return	gboolean	always return 0;
*/
gboolean
nf_arch_set_multi(guint data_format);


/**
	@brief				버닝 시작
	@param[in]	type	아카이브 타입 (NF_ARCH_TYPE_E)
	@param[in]	arch_id	아카이브 아이디
	@param[in]	dev_id	아카이빙할 디바이스 아이디
	@param[in]	erase	erase and burn
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_burn_start(NF_ARCH_TYPE_E type, guint16 arch_id, guchar dev_id, 
					gboolean erase,	cb_arch_fxn_t fxn_cb, gpointer ctx_cb, 
					GError **error);

/**
	@brief				버닝 종료
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_burn_end(GError **error);

/**
	@brief				버닝 취소
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_burn_cancel(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);



/**
	@brief				아카이브용 디바이스 목록 얻기 (최대 4개)
	@param[out]	dev_table	드바이스 목록 버퍼(사용자가 할당 arch_dev_info_t[4])
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_get_list(NF_ARCH_DEV_INFO *dev_table, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);		

/**
	@brief				아카이브용 디바이스 미디어 크기 얻기 (최대 4개)
	@param[out]	dev_table	드바이스 목록 버퍼(사용자가 할당 arch_dev_info_t[4])
	@param[out]	dev_cnt		디바이스 갯수
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_get_size(NF_ARCH_DEV_INFO *dev_table, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);
													
/**
	@brief				
	@param[in]	fxn_cb	콜백 function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	콜백 function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_set_notify(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error);

/**
	@brief				
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_unset_notify();



/**		
	@brief				아카이빙 진행 상태 조회
	@param[out]	progress	진행 상태
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_get_progress(NF_ARCH_PROGRESS *progress);


/**
	@brief				
	@return				
*/
const gchar*
nf_arch_get_error_string(gint errorno); 


typedef struct  _error_info_t {
        guint32     ch;
        guint32     time;
        guint32     duration;
} NF_ARCH_ERROR_INFO;

/**		
	@brief					아카이빙 에러 발생 위치
	@param[out]	err_info	내부에서 할당 된 에러정보 구조체 리컨
	@return	int				에러 항목 갯수, 에러이면 음수
*/
int nf_arch_get_error_info( NF_ARCH_ERROR_INFO **err_info);



/**
	@brief				set cipher params
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_set_cipher_param( NF_ARCH_CIPHER_PARAM *param,  GError **error );

#endif
