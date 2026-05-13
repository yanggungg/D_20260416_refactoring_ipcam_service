#ifndef __NF_API_DLVA_H__
#define __NF_API_DLVA_H__

#include <glib.h>
#include <jansson.h>
#include "nf_notify.h"

#define DLVA_API_RET_OK                      0
#define DLVA_API_RET_ARGUMENT_ERROR          -1
#define DLVA_API_RET_FAILED_INIT             -2
#define DLVA_API_RET_AIBOX_RETCODE_ERROR     -3
#define DLVA_API_RET_AIBOX_DATA_PARSE_ERROR  -4
#define DLVA_API_RET_AIBOX_PERMISSION_DENIED -5
#define DLVA_API_RET_AIBOX_CONNECTION_FAILED -6
#define DLVA_API_RET_AIBOX_AUTH_FAILED       -7
#define DLVA_API_RET_AIBOX_DATA_NOT_FOUND    -8

//group_id search option (nf_api_aibox_lpr_list_get, nf_api_aibox_fr_face_list_get_paging)
#define AIBOX_GROUP_SERACH_ALL        -1
#define AIBOX_GROUP_SERACH_UNASSIGNED 0

#define MAX_AIBOX_DB_GROUP_SIZE 30
#define MAX_AIBOX_DB_ZONE_SIZE 20

#define AIBOX_MAX_CHANNEL 16

#define RULEDATA_MAX_ZONE_SIZE 128 
#define RULEDATA_LIST_MAX 256

typedef struct _aibox_search_list
{
	GList *list;
	pthread_mutex_t _list_mutex;

}aibox_search_list;

typedef struct _aibox_connection_info
{
	unsigned int ip;
	unsigned int linklocal_ip;
	unsigned int cin_ip;
	unsigned char mac[8];
    int is_dhcp;
    unsigned int transaction;

    //web connection info
    int use_ssl;
    int web_port;

    char model[30];
}aibox_connection_info;

typedef enum NF_AI_MODEL_TYPE_E{
    NF_AI_MODEL_NONE        = 0,
    NF_AI_MODEL_AICAM       = 1<<0,
    NF_AI_MODEL_AICAM_PRO   = 1<<1,  //built in aibox
}NF_AI_MODEL_TYPE_E;

typedef enum _NF_AIBOX_DB_CHANGE_E{
	NF_AIBOX_DB_NONE		= 0,
	NF_AIBOX_DB_RULE		= 1,
}NF_AIBOX_DB_CHANGE_E;

typedef enum _NF_NVR_AIBOX_CONN_STATUS_E{
	NF_AIBOX_CONFIG_OFF         = 0,
	NF_AIBOX_CONFIG_ON          = 1,
	NF_AIBOX_CONNECTING         = 2,
	NF_AIBOX_CONN_SUCCESS       = 3,

    //error
	NF_AIBOX_CONN_FAILED        = 4,
	NF_AIBOX_STREAM_CONN_FAILED = 5,
}NF_NVR_AIBOX_CONN_STATUS_E;

char *_ip_to_str(unsigned int ip, char *buf);

typedef struct _host_inf
{
	unsigned int ipaddr;
	unsigned int netmask;
	char dev[64];
	struct _host_inf *next;
} host_inf;

//host info
host_inf *nf_api_get_host_inf_list(void);
int nf_api_is_nvr_stream(const char *rtsp_url, host_inf *list);
void nf_api_load_host_info_list();

//aibox/camera status
int nf_api_is_ai_camera(int ch);
int nf_api_get_aibox_url(int ch, char *url_buffer);

//aibox search functions
unsigned int nf_api_aibox_search(int timeout);
aibox_search_list *nf_api_aibox_search_list(int timeout);
int nf_api_aibox_set(unsigned int ch, unsigned int aibox_ipaddr, const char *algorithm);
void aibox_configure_status_update_all();

//aibox list functions
unsigned int nf_api_aibox_list_get_size(aibox_search_list *aibox_list);
void nf_api_aibox_list_free(aibox_search_list *aibox_list);

unsigned int nf_api_aibox_list_get_ip(aibox_search_list *aibox_list, int index);
unsigned int nf_api_aibox_connection_info_get_ip(aibox_connection_info *data);
aibox_connection_info *nf_api_aibox_list_get_data(aibox_search_list *aibox_list, int index);

unsigned char *nf_api_aibox_list_get_mac(aibox_search_list *aibox_list, int index);

aibox_connection_info *nf_api_aibox_list_get_from_mac(aibox_search_list *aibox_list, unsigned char *mac);

char *nf_api_get_host_from_rtsp_url(char *url, char *buf);
int nf_api_get_aibox_info(unsigned int ip, int *nvr_ch, int *port, int *use_ssl, char *id, char *pw);

typedef struct _DLVA_POINT_T_
{
    float x;
    float y;
}DLVA_POINT;

typedef struct _aibox_algorithm_info
{
	int disabled;
	char value[50];		//key, id
	char text[100];
    char algo_type[50];
	
	//min resolution
	unsigned int resolution_min_height;
	unsigned int resolution_min_width;
}aibox_algorithm_info;


typedef struct _aibox_algorithm_name
{
	char value[50];		//key, id
	char text[100];
    char algo_type[50];
}aibox_algorithm_name;

typedef struct ai_capa_t{
	char owner[20];
	char mac[8];
	unsigned int aibox_ip;

	int ch;
	aibox_algorithm_info *algorithm_list;
	int algorithm_count;
}ai_capa_t;

typedef struct ai_connection_info
{
	unsigned int aibox_ip;
	unsigned char mac[8];
    char algorithm[50];
}ai_connection_info;

typedef struct _AI_LICENSE_DATA
{
    char license_name[8];
    unsigned int license_required;
    unsigned int license_installed;
    char value[50];
    char name[100];
    char algo_type[50];
}ai_license_data;

typedef struct jpeg_image_data
{
	char *memory;
	size_t size;
}jpeg_image_data;

typedef struct fr_group_info
{
    int id;
    char name[255];
    int face_count;
}fr_group_info;


typedef struct face_info
{
    int id;
    char name[255];
    char image_url[255];
    char memo[255];

    fr_group_info group_list[MAX_AIBOX_DB_GROUP_SIZE];
    int group_length;
}face_info;

//lpr data
typedef struct lpr_group_info
{
    int id;
    char name[255];
    char memo[255];
    //int lp_count;
}lpr_group_info;

typedef struct lp_info
{
    lpr_group_info group_list[MAX_AIBOX_DB_GROUP_SIZE];
    int group_length;

    int id;
    char memo[255];
    char name[255];
    char phone[50];
    char plate[50];
}lp_info;

typedef enum _LPR_TRIGGER_MODE_E
{
    LPR_EVT_MODE_ALL,
    LPR_EVT_MODE_IN_GROUP,
    LPR_EVT_MODE_UNREGISTER
}LPR_TRIGGER_MODE_E;

typedef enum _LPR_TRIGGER_POLICY_E
{
    LPR_EVT_POLICY_VERYHIGH,
    LPR_EVT_POLICY_HIGH,
    LPR_EVT_POLICY_NORMAL
}LPR_TRIGGER_POLICY_E;

typedef enum{
    RULE_TYPE_UNKNOWN,
    RULE_TYPE_LPR,
    RULE_TYPE_FR,
}AIBOX_RULE_TYPE_E;

typedef enum _FR_TRIGGER_GTYPE_E
{
    FR_EVT_MODE_ATTR,
    FR_EVT_MODE_COMPARISON,
    FR_EVT_MODE_UNREGISTER
}FR_TRIGGER_GTYPE_E;

typedef enum _AIBOX_GENDER_E
{
    AIBOX_GENDER_ALL,
    AIBOX_GENDER_MALE,
    AIBOX_GENDER_FEMALE
}AIBOX_GENDER_E;

typedef struct
{
    AIBOX_RULE_TYPE_E type;
    int ch;
    int aibox_ch;
    int trigger_id;
    char trigger_name[250];
    char name[250];
}aibox_rule;

typedef struct lpr_rule
{
    //default rule data
    AIBOX_RULE_TYPE_E type;
    int ch;
    int aibox_ch;
    int trigger_id;
    char trigger_name[250];
    char name[250];

    LPR_TRIGGER_MODE_E rmode;
    LPR_TRIGGER_POLICY_E policy;

    int group_id_list[MAX_AIBOX_DB_GROUP_SIZE];
    size_t group_size;
    
    DLVA_POINT zone[MAX_AIBOX_DB_ZONE_SIZE];
    size_t zone_size;
}lpr_rule;

typedef struct fr_rule
{
    //default rule data
    AIBOX_RULE_TYPE_E type;
    int ch;
    int aibox_ch;
    int trigger_id;
    char trigger_name[250];
    char name[250];

    FR_TRIGGER_GTYPE_E gtype;  //attr comparison unregi

    //Facial Attribute Filtering
    int age_max;            //0 ~ 100
    int age_min;            //0 ~ 100
    AIBOX_GENDER_E gender;    //all male female

    //Comparison
    int search_thr;         //Min Similarity

    //Comparison & Unregistered
    int liveness;           //Liveliness Detection true or false
    int liveness_thr;       //Liveliness Threshold
    int group_id_list[MAX_AIBOX_DB_GROUP_SIZE];
    size_t group_size;
    
    DLVA_POINT zone[MAX_AIBOX_DB_ZONE_SIZE];
    size_t zone_size;
}fr_rule;


//face add bulk

typedef enum __FR_FACE_BULK_UPLOAD_STATE__
{
    FR_BULK_SUCC = 0,
    FR_BULK_ARGUMENT_MISSING,
    FR_BULK_FILE_NOT_FOUND,
    FR_BULK_FILE_AMOUNT_OVER,
    FR_BULK_ERROR
} FR_FACE_BULK_UPLOAD_STATE;

typedef enum __FR_FACE_BULK_UPLOAD_RESULT_TYPE__
{
    FR_BULK_TYPE_UPLOAD = 0,
    FR_BULK_TYPE_CONFIRM
} FR_FACE_BULK_UPLOAD_RESULT_TYPE;

typedef struct 
{
    char file_path[200];
	char name[200];
	char group[200];    //group name
}FR_UPLOAD_FILE_INFO;

typedef struct
{
    FR_FACE_BULK_UPLOAD_RESULT_TYPE type;   //upload    confirm
    int result;

    int index;
    int id;
    char path[200];
}FR_BULK_UPLOAD_CALLBACK_MESSAGE;

typedef struct lpr_db_search_option 
{
    const char *memo;
    const char *name;
    const char *phone;
    const char *plate;
}lpr_db_search_option;

typedef struct fr_db_search_option 
{
    const char *name;
    const char *memo;
}fr_db_search_option;

typedef int (*upload_result_callback)(FR_BULK_UPLOAD_CALLBACK_MESSAGE data);

typedef struct _aibox_rule_data
{
    //int nvr_ch;
    //int aibox_ch;
    char event_type[64];
    char name[64];
    AIBOX_RULE_TYPE_E type;

    size_t zone_size;
    DLVA_POINT zone_list[RULEDATA_MAX_ZONE_SIZE];
}aibox_rule_data;



//url parse
int is_nvr_ip(const char *ip_str);
int is_nvr_stream(const char *rtsp_url);

//http api timeout
void nf_api_aibox_set_http_timeout(int sec);
void nf_api_aibox_set_http_connection_timeout(int sec);

//ch stream info
ai_connection_info *get_ai_connection_info(aibox_search_list *list);
ai_connection_info *nf_api_get_ai_connection_info(aibox_search_list *list);
int nf_api_aibox_connection_is_alive(unsigned int ch, unsigned int aibox_ip);
int nf_api_aibox_server_state_is_alive(unsigned int aibox_ip, int connection_timeout);
int nf_api_selected_aibox_algorithm(int ch, unsigned int aibox_ip, char *value);
int nf_api_selected_aicamera_algorithm(int ch, char *value);
const ai_license_data *nf_api_selected_aicamera_algorithm_data(int ch);

//owner
char *nf_api_get_nvr_owner(char *owner);
char *nf_api_get_aibox_owner(unsigned int aibox_ip, char *owner);
int nf_api_set_aibox_owner(unsigned int aibox_ip);
int nf_api_delete_owner(unsigned int aibox_ip);

int nf_api_set_zmq_uri(unsigned int aibox_ip);

//video stream
int nf_api_aibox_set_video_stream(unsigned int ch, unsigned int aibox_ip, const char *algorithm);
int nf_api_aibox_set_video_streams(unsigned int aibox_ip, aibox_algorithm_name *ch_info, unsigned int ch_mask);
int nf_api_aibox_delete_video_stream(unsigned int ch, unsigned int aibox_ip);

//get stream list
GList *nf_api_get_stream_list(unsigned int aibox_ip);
void nf_api_stream_list_free(GList *list);

//get capability
ai_capa_t *nf_api_get_aicam_capability(int ch);
ai_capa_t *nf_api_get_aibox_capability(unsigned int aibox_ip, int ch);

ai_capa_t *nf_api_get_capability(unsigned int aibox_ip, int ch);
ai_capa_t *nf_api_get_aibox_capabilities(unsigned int aibox_ip);
ai_capa_t *nf_api_get_capabilities(unsigned int aibox_ip);
void nf_api_capability_free(ai_capa_t *capa);
void nf_api_capabilities_free(ai_capa_t *capa);
void nf_api_algorithm_list_free(GList *list);
void aibox_algorithm_info_free(aibox_algorithm_info *info);

aibox_algorithm_name *nf_api_get_aibox_algorithm_names(unsigned int aibox_ip, int *length);
aibox_algorithm_name *nf_api_get_algorithms_all_ch(unsigned int aibox_ip);
aibox_algorithm_name *nf_api_get_algorithm_data();


int nf_api_aibox_update_video_stream_all(unsigned int change_info);
int nf_api_aibox_get_aibox_iplist(unsigned int *list);
void nf_api_aibox_url_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

        //fr - face//
int nf_api_aibox_fr_get(unsigned int aibox_ip, int id, face_info *info, jpeg_image_data *image);
//image get
jpeg_image_data nf_api_fr_get_image(unsigned int aibox_ip, int aibox_face_id, const char *image_name);

//list get
int nf_api_aibox_fr_face_list_get(unsigned int aibox_ip, face_info **list, int *length); //return code
int nf_api_aibox_fr_face_list_get_paging(
		//input
		unsigned int aibox_ip,
		int page_index,
		int page_size,

		//search option
		int group_id,
		const fr_db_search_option *search_option,

		//output
		int *max_page_num,
		face_info **list,
		int *length);
face_info *aibox_fr_face_list_get(unsigned int aibox_ip, int *length);
void free_face_info_list(face_info *list, int length);

//add
int nf_api_fr_face_add(unsigned int aibox_ip, jpeg_image_data image, const char *nvr_id, int *group_id_list, int group_length);

//modify
int nf_api_fr_face_modify(unsigned int aibox_ip, jpeg_image_data image, const char *name, int face_id);
int nf_api_aibox_fr_modify(unsigned int aibox_ip, face_info *info, jpeg_image_data *image);

//int nf_api_fr_face_bulk_upload(unsigned int aibox_ip, const char *directory_path);
//FR_FACE_BULK_UPLOAD_STATE nf_api_fr_face_bulk_upload(unsigned int aibox_ip, const char *directory_path, int max_face_count, FR_UPLOAD_FILE_INFO **list, int *list_len, result_callback functor);
FR_FACE_BULK_UPLOAD_STATE nf_api_fr_face_bulk_upload(
        unsigned int aibox_ip,          //targetaibox ip address
        const char *directory_path,     //upload root
        int max_face_count,             //available count
        FR_UPLOAD_FILE_INFO **list,     // result list
        int *list_len,                  // result list len
        upload_result_callback functor);       //functor (optional)


//delete
int aibox_fr_face_delete(unsigned int aibox_ip, unsigned int face_id);
int nf_api_aibox_fr_face_delete_all(unsigned int aibox_ip); //delete all group & face

        //fr - group//
fr_group_info *nf_api_fr_group_list_get(unsigned int aibox_ip, int *length);
int nf_api_fr_group_add(unsigned int aibox_ip, const char *group_name);
int aibox_fr_group_delete(unsigned int aibox_ip, unsigned int group_id);
int nf_api_fr_face_group_bind(unsigned int aibox_ip, int face_id, int group_id);    //face - group bind
int nf_api_fr_face_group_unbind(unsigned int aibox_ip, int face_id, int group_id);

int nf_api_aibox_login_check(unsigned int aibox_ip, const char *id, const char *pw);

/***************************************************
 * AIBOX LPR Management functions
***************************************************/
/*
 * lpr - face
 */
//list get
int nf_api_aibox_lpr_list_get(
        //input
        unsigned int aibox_ip,  
        int page_index,         //페이지 인덱스
        int page_size,          //한번에 보여줄 페이지 크기

		//search option
		int group_id,
		const lpr_db_search_option *search_option,

        //output
        int *max_page_num,      
        lp_info **list_ret,
        int *length); 

int nf_api_aibox_lpr_get(unsigned int aibox_ip, int id, lp_info *info);

//list free
void free_lp_info_list(lp_info *list, size_t length);
void free_lp_info(lp_info *object);

//add
int nf_api_lpr_add(unsigned int aibox_ip, lp_info *info);

//modify
int nf_api_lpr_modify(unsigned int aibox_ip, lp_info *info);

//delete
int aibox_lpr_delete(unsigned int aibox_ip, int lpr_id);
int nf_api_aibox_lpr_delete_all(unsigned int aibox_ip);//, const char *group_name/* NULL : deelte All */);

//bulk upload
int nf_api_lpr_bulk_upload(unsigned int aibox_ip, const char *file_path);

/*
 * lpr - group
 */
int nf_api_lpr_group_list_get(unsigned int aibox_ip, lpr_group_info **list, int *length);
int nf_api_lpr_group_add(unsigned int aibox_ip, const char *group_name);
int aibox_lpr_group_delete(unsigned int aibox_ip, int group_id);// const char *group_name);

/*****************/
/* ai camera api */
/*****************/
int nf_api_aicam_fr_get(unsigned int channel, int id, face_info *info, jpeg_image_data *image);
//image get
jpeg_image_data nf_api_aicam_fr_get_image(unsigned int channel, int face_id, char *image_name);

//list get
int nf_api_aicam_fr_face_list_get(unsigned int channel, face_info **list, int *length); //return code
//face_info *aicam_fr_face_list_get(unsigned int channel, int *length);

//add
int nf_api_aicam_fr_face_add(unsigned int channel, jpeg_image_data image, const char *nvr_id, int *group_id_list, int group_length);

//modify
int nf_api_aicam_fr_face_modify(unsigned int channel, jpeg_image_data image, const char *name, int face_id);

//delete
int aicam_fr_face_delete(unsigned int channel, unsigned int face_id);
int nf_api_aicam_fr_face_delete_all(unsigned int channel); //delete all group & face

//group
fr_group_info *nf_api_aicam_fr_group_list_get(unsigned int channel, int *length);

//bulk
FR_FACE_BULK_UPLOAD_STATE nf_api_aicam_fr_face_bulk_upload(
        unsigned int channel,               // aicam channel
        const char *directory_path,         //upload root
        int max_face_count,                 //available count
        FR_UPLOAD_FILE_INFO **list_result,  // result list
        int *list_len,                      // result list len
        upload_result_callback functor);           //functor (optional)

//aibox event
int nf_api_get_aibox_rules(int nvr_ch, aibox_rule_data **rules, int *rule_size);

unsigned int get_aibox_ch_mask(unsigned int ip);

int nf_api_aibox_connection_status_update(unsigned int aibox_ip, json_t *vsource);
unsigned int nf_api_get_aibox_connection_status(int ch);
int nf_api_aibox_conn_check();
time_t nf_api_aibox_conn_time_update(int ch, time_t current);

int nf_api_ai_config_update(int ch);

//lpr rules
int nf_api_get_lpr_rule_by_id(unsigned int aibox_ip, unsigned int trigger_id, lpr_rule *rule);
int nf_api_get_lpr_rules(unsigned int aibox_ip, lpr_rule **rule, int *rule_size);

//rule api
int nf_api_get_trigger_id(unsigned int aibox_ip, const char *trigger_name, AIBOX_RULE_TYPE_E type);
int nf_api_add_rule(unsigned int aibox_ip, aibox_rule *rule);
int nf_api_modify_rule(unsigned int aibox_ip, aibox_rule *rule);
int nf_api_delete_rule(unsigned int aibox_ip, int trigger_id, AIBOX_RULE_TYPE_E type);

#endif // __NF_API_DLVA_H__

