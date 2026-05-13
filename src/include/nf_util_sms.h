#ifndef	__NF_UTIL_SMS_H
#define	__NF_UTIL_SMS_H


#define MAX_TEXT_SIZE 128
#define NF_SMS_SEND_MAX_TO_CNT 64
#define DB_STRING_SIZE 64
#define MAX_SMS_SIZE 128
//#define BIZ_ADDRESS "biz.ppurio.com"
#define BIZ_ADDRESS "211.189.43.25"
#define BIZ_PORT "5000"
#define BIZ_SECURITY_PORT "15200"
#define CLICKATELL_ADDRESS "196.216.236.7"
#define SMS_UNLIMIT (-1)
#define NF_SMS_STRING_LIMIT 340
#define NF_SMS_SPLIT_LIMIT 84
#define NF_SMS_SEND_MAX_QUEUE_LEN 	32


typedef enum _NF_SMS_ERROR_TYPE_E  {


	NF_SMS_FAIL_TO_AUTHENTICATE = -2,

	NF_SMS_CONNECTION_ERROR = -1,
	NF_SMS_OK = 1,
	NF_SMS_NOT_AVAILABLE_TIME,
	NF_SMS_DAILY_RESTRICTION_OVER,
	NF_NOT_CONNECTED,
	NF_FAIL_TO_MAKE_FILE_DIRECTORY,
	NF_FAIL_TO_MAKE_BLACKLIST_DIRECTORY,
	NF_WRONG_CMD_ID,
	NF_FAIL_TO_SET_DEVICE,
	NF_SMS_IS_BLACK_LIST,
	NF_FAIL_TO_AUTHENTICATE,
	NF_ERROR_NOT_DEFINED,	
} NF_SMS_ERROR_TYPE_E;

/*queue type macro */
#define NF_TYPE_SMS_SEND				(nf_sms_send_get_type ())
#define NF_IS_SMS_SEND(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_SMS_SEND))
#define NF_IS_SMS_SEND_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_SMS_SEND))

#define NF_SMS_SEND_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_SMS_SEND, NfSMSSendClass))
#define NF_SMS_SEND(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_SMS_SEND, NfSMSSend))
#define NF_SMS_SEND_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_SMS_SEND, NfSMSSendClass))

#define NF_SMS_SEND_CAST(obj)			((NfSMSSend*)(obj))
#define NF_SMS_SEND_CLASS_CAST(klass)	((NfSMSSendClass*)(klass))

typedef struct _NfSMSSend 		NfSMSSend;
typedef struct _NfSMSSendClass 	NfSMSSendClass;

typedef enum _NF_SMS_SEND_ERROR_E {
	NF_SMS_SEND_ERROR_SUCCESS	= 0,
	NF_SMS_SEND_ERROR_FAILED	= 1,
	NF_SMS_SEND_ERROR_FROM		= 2,
	NF_SMS_SEND_ERROR_TO 		= 3,

	NF_SMS_SEND_ERROR_SUBJECT	= 4,
	NF_SMS_SEND_ERROR_CONTENTS	= 5,
	NF_SMS_SEND_ERROR_IMAGE_NAME	= 6,
	NF_SMS_SEND_ERROR_IMAGE_DATA	= 7,

	NF_SMS_SEND_ERROR_IMAGE_SIZE	= 8,
	NF_SMS_SEND_ERROR_SERVER	= 9,
	NF_SMS_SEND_ERROR_PORT		= 10,
	NF_SMS_SEND_ERROR_USERNAME	= 11,

	NF_SMS_SEND_ERROR_PASSWORD	= 12,
	NF_SMS_SEND_ERROR_SECURITY	= 13,
	NF_SMS_SEND_ERROR_MALLOC	= 14,
	NF_SMS_SEND_ERROR_MAX_QUE_LEN	= 15,

	NF_SMS_SEND_ERROR_NETWORK	= 17,
	NF_SMS_SEND_ERROR_SERVER_CONF	= 18,
	NF_SMS_SEND_ERROR_INIT		= 19,
	NF_SMS_SEND_ERROR_NR,
} NF_SMS_SEND_ERROR_E;


typedef enum _NF_SMS_SEND_STATUS_E {
	NF_SMS_SEND_STATUS_SUCCESS	= 0,
	NF_SMS_SEND_STATUS_FAILED	= 1,
	NF_SMS_SEND_STATUS_START	= 2
} NF_SMS_SEND_STATUS;



// hand off
typedef void (*NF_SMS_SEND_CB) ( int msg_int,
								  const char *svr_msg );
struct _NfSMSSend {
	NfObject             object;
	gint                 init_done;
	GAsyncQueue          *queue;
	gint                 request_cnt;
	gint                 process_cnt;
	gint                 error_cnt;
	GThread              *thread;
	gint                 thread_run;
	gint                 thread_status;
};

struct _NfSMSSendClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

typedef enum _NF_SMS_TYPE_E  {
	NF_SMS_TYPE_BIZ,
	NF_SMS_TYPE_CLICK_A_TELL,
} NF_SMS_TYPE_E;

typedef struct _NF_SMS_INFO_T  {

    char username[MAX_SMS_SIZE];
    char passwd[MAX_SMS_SIZE];
    char app_id[MAX_SMS_SIZE];

    int  start_avail_time;
    int  end_avail_time;
    int  max_send_count;

    int  current_send_count;
    int  current_date;

    gboolean security;

    int  type;

} NF_SMS_INFO;

typedef struct NF_SMS_SEND_INFO_T {

    char receiver[NF_SMS_SEND_MAX_TO_CNT][MAX_TEXT_SIZE];
    int	 receiver_cnt;    
    char text[NF_SMS_STRING_LIMIT]; // 340 byte limit.

    NF_SMS_SEND_CB	cb_func;
} NF_SMS_SEND_DATA;

typedef enum _NF_CLICK_A_TELL_TYPE_E {
	NF_CLICK_SEND_MSG,
	NF_CLICK_BALANCE,
}NF_CLICK_A_TELL_TYPE_E;

gboolean nf_sms_send_init( int wait );
int nf_send_biz_direct(char *ipaddress, NF_SMS_INFO *si, NF_SMS_SEND_DATA* ssi);
int nf_sms_find_s1_password(NF_SMS_SEND_DATA *ssi);
int nf_sms_find_password(NF_SMS_INFO *si, NF_SMS_SEND_DATA *ssi);
int nf_sms_push_event(NF_SMS_INFO *si,NF_SMS_SEND_DATA *ssi);

// vendor string table function
gint nf_sms_vendor_get_count();
gchar *nf_sms_vendor_get_string( gint index);
gint nf_sms_vendor_get_index(gchar *str);

#endif

