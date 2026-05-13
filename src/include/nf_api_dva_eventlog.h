#ifndef __NF_API_DVA_EVENTLOG_H__
#define __NF_API_DVA_EVENTLOG_H__

#include "nf_api_eventlog.h"
#include "itx_ai_def.h"

#define MAX_STR_OPER_CNT	2
#define MAX_STR_NUM_CNT		(MAX_STR_OPER_CNT + 1)

#define MAX_JPEG_SIZE 800*1024

typedef struct _ai_generic_log_t {
	unsigned int type;						
    	int ch;							/**< Channel number for this event. */
	ai_point_t event_area[2];			 //bbox left_top, right_bottom
	unsigned int timestamp;			/**< Timestamp of this event. */
     	unsigned int timestampl;			/**< Timestamp of this event. */
		
	//event message
	char caption[45];
	char title[84];
	char description[80];
} ai_generic_log_t;	

typedef struct _ai_fr_log_t {
	unsigned int type;					
	int object_id;					/**< Object id for this event. */
	short int rule_id;					/**< Rule id for this event. */	
    	int ch;							/**< Channel number for this event. */
	double bbx_position[4];			/**< Rectangle(bounding box) of the event region. */
	unsigned int timestamp;			/**< Timestamp of this event. */
     	unsigned int timestampl;			/**< Timestamp of this event. */
     	unsigned int process_time;			/**< process_time. */
	double confidence;				/**< confidence. */
	unsigned int face_id;
	double search_score;
	char group_name[32];
	char name[32];
	int age;
	char gender[32];
} ai_fr_log_t;	


typedef struct _ai_lpr_log_t {
	unsigned int type;					
	int object_id;					/**< Object id for this event. */
	short int rule_id;					/**< Rule id for this event. */	
    	int ch;							/**< Channel number for this event. */
	double bbx_position[4];			/**< Rectangle(bounding box) of the event region. */
	unsigned int timestamp;			/**< Timestamp of this event. */
     	unsigned int timestampl;			/**< Timestamp of this event. */
     	unsigned int process_time;			/**< process_time. */
	double confidence;				/**< confidence. */
	char lp_text[64];
	unsigned int group_mask;
} ai_lpr_log_t;	

typedef enum _DVA_ALG_TYPE_E {
	DVA_INTRUSION_DETECTION,
	DVA_ILLEGAL_PARKING,
	DVA_LPR,
	DVA_COUNTER,
	/* AI-CAM,BOX */
	DVA_AI_DETECTION,
	DVA_AI_FR_DETECTION,
	DVA_AI_LPR_DETECTION,
	DVA_AI_GENERIC,

	/* useful to search event by grouping */
	DVA_GRP_INTRUSION_DETECTION=128,
	DVA_GRP_ILLEGAL_PARKING,
	DVA_GRP_LPR,
	DVA_GRP_COUNTER,

	
	DVA_ALG_TYPE_MAX,
} DVA_ALG_TYPE;

enum DVA_IMG_FORMAT {
	DVA_IMG_BMP,
	DVA_IMG_JPG,
};

typedef struct {
	guint format;
	guint width;
	guint height;
	guint size;
	guchar data[MAX_JPEG_SIZE];
} DVA_IMG;

typedef struct {
	gint width;
	gint height;
	gfloat coords[4];
} DVA_BBOX;

typedef struct {
	gchar name[64];
	gfloat confidence;
	DVA_BBOX bbox;
} __attribute__((packed)) DVA_MSG_IDZ;

typedef struct {
	gchar name[64];
	gfloat confidence;
	DVA_BBOX bbox;
} __attribute__((packed)) DVA_MSG_IPZ;

typedef struct {
	gchar name[64];
	gfloat plate_number;
	DVA_BBOX bbox;
} __attribute__((packed)) DVA_MSG_LPR;

typedef struct {
	gchar name[64];
	gfloat confidence;
	DVA_BBOX bbox;
	guint count;
} __attribute__((packed)) DVA_MSG_COUNTER;

typedef struct {
	guchar	type;
	guchar	ch;
	guint64	timestamp;
	guint64	timestampl;
	DVA_IMG snapshot;
	union {
		DVA_MSG_IDZ intrusion_detection;
		DVA_MSG_IPZ illegal_parking;
		DVA_MSG_LPR lpr;
		DVA_MSG_COUNTER counter;
	} __attribute__((packed));
} DVA_MSG;

typedef struct _NF_DVA_LOG_PARAM_T {
	NF_LOG_PARAM	log_param;

	/* �˻����� */
	guchar		dva_type_mask;
	guchar		match_case;		// 0:off, 1:on
	guchar		match_whole;
	guchar		fr_unregistered_enable; // use search unregistered filltering
	guchar		search_str_enable;
	//gchar		search_str[512]; // fr - name , lpr - plate number
	//guchar		search_str_len;	
	guchar		search_str1_enable; // use FR group name
	guchar		fr_unassinged_group_enable; // use search unassigned in any group user filltering
	gchar		search_str1[512];
	guchar		search_str1_len;	
	guchar		search_str2_enable; // use FR gender
	gchar		search_str2[512];
	guchar		search_str2_len;
	guint		event_mask;
	guint		lpr_grp_mask;
	gchar* 		caption_list;
	guchar		caption_list_len;
	guchar		operation[2];		// 0:and, 1:or
	gchar		search_str[MAX_STR_NUM_CNT][512];
	guchar		search_str_len[MAX_STR_NUM_CNT];	// 0 �̸� N/A
} NF_DVA_LOG_PARAM;

typedef struct _NF_DVA_LOG_RESULT_HEADER_T {
	NF_LOG_RESULT_HEADER	log_result_header;
	guint64	log_id_first;		// for page down/up
	guint64	log_id_last;
	guint64	log_id_last_prev;
} NF_DVA_LOG_RESULT_HEADER;

typedef struct {
	union {
		DVA_MSG_IDZ intrusion_detection;
		DVA_MSG_IPZ illegal_parking;
		DVA_MSG_LPR lpr;
		DVA_MSG_COUNTER counter;
	} __attribute__((packed));
} NF_DVA_LOG_DATA;

/**
	@brief		POS �̺�Ʈ �α� �Է�
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dva_eventlog_put_text(gint ch, char *str, char *param1, char *param2);

/**
	@brief		POS 	�̺�Ʈ �α� ��ȸ
	@param[in]	param	�̺�Ʈ �α� ��ȸ �Ķ����
	@param[out]	header	�̺�Ʈ �α� ��ȸ ��� ���
	@param[out[	result	�̺�Ʈ �α� ��ȸ ��� ������ ��
	@return gboolean %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dva_eventlog_get(NF_DVA_LOG_PARAM *param, NF_DVA_LOG_RESULT_HEADER *header, NF_LOG_DATA *log_data);

#endif

