#include "sqlite3.h"

#define SQL_QUERY_BUF_LEN (256)
#define NUM_FACE_GROUP_MASK	(0xFFFF)
#define SQL_DB_PATH "/NFDVR/data/facedata.db"
#define FACE_MOUNT_PATH	"/mnt/fwup_hdd/"
#define SQL_DEBUG	(0) 

enum __SQL_SETUP_RTN_E_
{
	SQL_SET_ERROR,
	SQL_SET_OK
};

typedef struct __SQL_FACE_DATA_INFO__
{
	int face_id;
	char name[128];
	char memo[128];
	char filepath[128];
	unsigned int group_num;

}SQL_FACE_INFO;

typedef struct __SQL_LPR_DATA_INFO__
{
	int lpr_id;
	char key_string[32];
	unsigned int group_mask;
	char lp_text[128]; 
	char country[64];

}SQL_LPR_INFO;


//face recognition
//current max facd
int nf_sql_get_current_max_face_id();
int nf_sql_drop_facedata_table();
int nf_sql_create_facedata_table();
int nf_sql_insert_facedata(SQL_FACE_INFO* p_info, char* p_file_buffer);
int nf_sql_delete_facedata(int p_face_id);
//all column delete in table && all .jpg delete
int nf_sql_delete_facedata_all();
//if p_page_index == 0 is search all data, p_page_index is page index p_page_size is size per page(table linear)
SQL_FACE_INFO* search_facedata_from_group(int p_group_id, int *p_total_count, int p_page_index, int p_page_size);
//p_info->face_id is primary key you want to change row, other param is value you want to replace value 
int nf_sql_update_facedata(SQL_FACE_INFO* p_info);

//lpr
int nf_sql_get_current_max_lpr_id();
int nf_sql_create_lpr_table();
int nf_sql_delete_lpr_data(int p_lpr_id);
int nf_sql_insert_lpr_data(SQL_LPR_INFO* p_info);
int nf_sql_delete_lpr_data_all();
int nf_sql_search_lpr_data_from_id(int p_lpr_id, SQL_LPR_INFO* p_info);
int nf_sql_search_lpr_data_from_text(char* p_lp_text, SQL_LPR_INFO* p_info);
int nf_sql_search_lpr_data_from_like_text(char* p_lp_text, SQL_LPR_INFO* p_info);
int nf_sql_update_lpr_data_from_id(SQL_LPR_INFO* p_info);
int nf_sql_drop_lpr_data_table();
int nf_sql_create_default_lpr_group_table();
int nf_sql_update_lpr_group_name_from_id(int p_group_id, char *p_group_name);
int nf_sql_update_lpr_group_name_from_array(int p_max_index, int p_buf_len, char (*p_group_name)[p_buf_len]);
int nf_sql_delete_lpr_group_all();
int nf_sql_search_lpr_group_from_id(int p_group_id, char *p_group_text, int p_buf_len);
int nf_sql_drop_lpr_group_table();
int nf_sql_find_matching_group_mask(int* p_group_mask, char* p_lp_text);
