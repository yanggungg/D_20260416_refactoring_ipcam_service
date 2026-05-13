#include <stdio.h>      
#include <stdlib.h>
#include <string.h>
#include "nf_ipcam_sql_utils.h"
#include "nf_util_fw.h"

//facedata prime key id
int g_prime_id;
int g_lpr_maxid;

int nf_sql_get_current_max_face_id()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	sqlite3_stmt *stmt;   

	int row_id = 0;

	char* query = "SELECT * FROM basic_facedata order by id desc limit 1;";

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  /* 1 */

	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              /* 2 */
			row_id = sqlite3_column_int(stmt, 0);

#if 0 
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  /* 3 */
#endif
	}

	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return row_id;
}

int nf_sql_get_current_max_lpr_id()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	sqlite3_stmt *stmt;   

	int row_id = 0;

	char* query = "SELECT * FROM lpr_data order by id desc limit 1;";

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  /* 1 */

	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              /* 2 */
			row_id = sqlite3_column_int(stmt, 0);

#if 0 
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  /* 3 */
#endif
	}

	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return row_id;
}

int nf_sql_drop_facedata_table()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   
	int ret = SQL_SET_ERROR;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	strncpy(query, "DROP TABLE basic_facedata", 31);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	strncpy(query, "DROP TABLE group_facedata", 31);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("facedata table deleted.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_create_facedata_table()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	sqlite3_stmt *stmt;   

	char* basic_facedata_query = "CREATE TABLE basic_facedata (id INTEGER PRIMARY KEY, name TEXT CHECK(typeof(name)='text'), memo TEXT, path TEXT CHECK(typeof(path)='text'));";
	char* group_facedata_query = "CREATE TABLE group_facedata (id INTEGER PRIMARY KEY, face_id INTEGER CHECK(typeof(face_id)='integer'), group_id INTEGER CHECK(typeof(face_id)='integer'));";

	//mnt path init check
	int ret = 0;
	nf_fw_network_upgrade_hdd_mount_state(1,&ret);

	if(ret < 1)
	{
		printf("[%s] ERROR : undefine facedata mount path \n", __func__);
		goto out;
	}
	
	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	//create basic_facedata
	sqlite3_prepare_v2(db, basic_facedata_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);                                                                    /* 3 */
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR create table: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	//create group_facedata
	sqlite3_prepare_v2(db, group_facedata_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);                                                                    /* 3 */
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR create table: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	//init
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("facedata table created...\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_insert_facedata(SQL_FACE_INFO* p_info, char* p_file_buffer)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	int i = 0;
	int file_number = 0;
	char file_path[64] = {0,};
	unsigned int mask = p_info->group_num & NUM_FACE_GROUP_MASK;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	file_number = nf_sql_get_current_max_face_id();
	if(file_number < 0)
		goto out;

	file_number++;

	sprintf(file_path, "%s%d.jpg", FACE_MOUNT_PATH, file_number);

	int ret = 0;
	nf_fw_network_upgrade_hdd_mount_state(1,&ret);
	if(ret < 1)
	{
		printf("[%s] ERROR : facedata mount path not found\n", __func__);
		goto out;
	}
	
	FILE *fp = NULL;
	fp = fopen(file_path, "wb");
	if (fp == NULL) {
		printf("\e[32m [%s] %s ERROR: \e[0m\n", __func__, file_path);
		goto out;
	}

	fwrite(p_file_buffer, strlen(p_file_buffer), 1, fp);
	fclose(fp);

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), "insert into basic_facedata (name, memo, path) values ('%s', '%s', '%s');", p_info->name, p_info->memo, file_path);
	//printf("\e[33m query(%s) \e[0m\n", query);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	if(stmt)
		sqlite3_finalize(stmt);
	
	int prime_id = 0;
	prime_id = sqlite3_last_insert_rowid(db);

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	for(i = 0; i < NUM_ACTIVE_CH; i ++)
	{
		if(mask & (1 << i))
		{
			sprintf(query, "insert into group_facedata (face_id, group_id) values (%d, %d);", prime_id, i);
			sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              
			rc = sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	sqlite3_close(db);
	printf("facedata insert success.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_delete_facedata(int p_face_id)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	//need to delete file
	
	//basic table row delete
	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), "delete from basic_facedata where id=%d", p_face_id);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}


	//group table delete
	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), "delete from group_facedata where face_id=%d", p_face_id);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              
	
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}
	
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("facedata delete(id=%d) success.\n", p_face_id);

	return SQL_SET_OK;
	
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_delete_facedata_all()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	char cmd[64] = {0,};

	char* delete_basic_query = "delete from basic_facedata";
	char* delete_group_query = "delete from group_facedata";

	snprintf(cmd, 631, "rm -rf %s *.jpg", FACE_MOUNT_PATH);
	proxy_system(cmd ,1,5);

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_prepare_v2(db, delete_basic_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s]ERROR deleteing data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_prepare_v2(db, delete_group_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s]ERROR deleting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}
	
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
	
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

SQL_FACE_INFO* search_facedata_from_group(int p_group_id, int *p_total_count, int p_page_index, int p_page_size)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	SQL_FACE_INFO *p_info = NULL;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		return NULL;
	}

	char *test = "SELECT COUNT(*) FROM "
				"(SELECT * FROM group_facedata INNER JOIN basic_facedata ON basic_facedata.id=group_facedata.face_id"
				" WHERE group_facedata.group_id=%d);";
	snprintf(query, SQL_QUERY_BUF_LEN, test, p_group_id);
	rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	rc = sqlite3_step(stmt);

	*p_total_count = sqlite3_column_int(stmt, 0);
	//printf("\e[31m rowcount (%d)\e[0m\n", *p_total_count);
	sqlite3_finalize(stmt);

	if(p_page_index > 0) 
	{
		//page list
		p_info = (SQL_FACE_INFO*)malloc(sizeof(SQL_FACE_INFO) *(p_page_size));
		memset(p_info, 0x00, sizeof(SQL_FACE_INFO)*(p_page_size));
	}
	else
	{
		//total list
		p_info = (SQL_FACE_INFO*)malloc(sizeof(SQL_FACE_INFO) *(*p_total_count));
		memset(p_info, 0x00, sizeof(SQL_FACE_INFO)*(*p_total_count));
	}

	//*p_total_count / p_page_size + (*p_total_count % p_page_size) != 0;

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	char *str = "select * from group_facedata "
				"inner join basic_facedata on basic_facedata.id=group_facedata.face_id where group_facedata.group_id=%d"
				" %s;";
	if(p_page_index > 0)
	{
		char *tmp_str = "order by id limit %d offset %d";
		char tmp_arr[128] = {0,};
		snprintf(tmp_arr, SQL_QUERY_BUF_LEN, tmp_str, p_page_size, ((p_page_index - 1) * p_page_size));
		snprintf(query, SQL_QUERY_BUF_LEN, str, p_group_id, tmp_arr);
	}
	else
	{
		snprintf(query, SQL_QUERY_BUF_LEN, str, p_group_id, "");
	}

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  /* 1 */

	int i = 0;
	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              /* 2 */
			p_info[i].face_id = sqlite3_column_int(stmt, 3);
			strncpy(p_info[i].name, sqlite3_column_text(stmt, 4), 127); 
			strncpy(p_info[i].memo, sqlite3_column_text(stmt, 5), 127); 
			strncpy(p_info[i].filepath, sqlite3_column_text(stmt, 6), 127); 
			i++;

#if 0
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  /* 3 */
#endif
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return p_info;
}

int nf_sql_update_facedata(SQL_FACE_INFO* p_info)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   
	unsigned int mask = p_info->group_num & NUM_FACE_GROUP_MASK;
	int i = 0;

	char *src = "UPDATE basic_facedata SET name='%s', memo='%s' WHERE id=%d;";

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), src, p_info->name, p_info->memo, p_info->face_id);

	//printf("\e[95m %s \e[0m\n", query);

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              
	
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}
	sqlite3_finalize(stmt);

	//group table delete
	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), "delete from group_facedata where face_id=%d", p_info->face_id);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	for(i = 0; i < NUM_ACTIVE_CH; i ++)
	{
		if(mask & (1 << i))
		{
			sprintf(query, "insert into group_facedata (face_id, group_id) values (%d, %d);", p_info->face_id, i);
			sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              
			rc = sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	sqlite3_close(db);
	printf("facedata update success. (id=%d)\n", p_info->face_id);

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int _nf_sql_get_table_row_column(char *p_db_name, char *p_table_name)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	int rowcount = -1;

	rc = sqlite3_open(p_db_name, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		//goto out;
	}

	char *str = "select count(*) from %s;";

	snprintf(query, SQL_QUERY_BUF_LEN, str, p_table_name);

	rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		goto out;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) {
		goto out;
	}
	rowcount = sqlite3_column_int(stmt, 0);
	//printf("\e[31m rowcount (%d)\e[0m\n", rowcount);

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return rowcount;
}

//lpr data
int nf_sql_create_lpr_table()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	sqlite3_stmt *stmt;   

	char* lpr_data_query = "CREATE TABLE lpr_data (id INTEGER PRIMARY KEY, "
						   "key_string TEXT CHECK(typeof(key_string)='text'), "
						   "group_mask INTEGER CHECK(typeof(group_mask)='integer'), "
						   "lp_text TEXT CHECK(typeof(lp_text)='text'), country TEXT CHECK(typeof(country)='text'));";
	
	//mnt path init check
#if 0
	int ret = 0;
	//nf_fw_network_upgrade_hdd_mount_state(1,&ret);

	if(ret < 1)
	{
		printf("[%s] ERROR : undefine facedata mount path \n", __func__);
		goto out;
	}
#endif
	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	//create basic_facedata
	sqlite3_prepare_v2(db, lpr_data_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);                                                                    
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR create table: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("data table created.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_delete_lpr_data(int p_lpr_id)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	//who's role ? need to delete file
	//basic table row delete
	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), "delete from lpr_data where id=%d", p_lpr_id);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("delete(id=%d) success.\n", p_lpr_id);

	return SQL_SET_OK;
	
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_insert_lpr_data(SQL_LPR_INFO* p_info)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	int i = 0;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), "insert into lpr_data (key_string, group_mask, lp_text, country) values ('%s', %d, '%s', '%s');", 
			p_info->key_string, p_info->group_mask, p_info->lp_text, p_info->country);
	//printf("\e[33m query(%s) \e[0m\n", query);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		goto out;
	}

	if(stmt)
		sqlite3_finalize(stmt);
	
	//row max id 
	//g_lpr_maxid = sqlite3_last_insert_rowid(db);
	//printf("\e[31m [%s] g_lpr_maxid(%d) \e[0m\n", __func__,  g_lpr_maxid);

	sqlite3_close(db);
	printf("insert success.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_delete_lpr_data_all()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	char* delete_basic_query = "delete from lpr_data";

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_prepare_v2(db, delete_basic_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s]ERROR deleteing data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
	
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_delete_lpr_group_all()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	char* delete_basic_query = "delete from lpr_group";

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_prepare_v2(db, delete_basic_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s]ERROR deleteing data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
	
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_search_lpr_data_from_id(int p_lpr_id, SQL_LPR_INFO* p_info)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	char *str = "select * from lpr_data where id=%d";
	snprintf(query, SQL_QUERY_BUF_LEN, str, p_lpr_id);

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  /* 1 */

	//int i = 0;
	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              /* 2 */
			p_info->lpr_id = sqlite3_column_int(stmt, 0);
			strncpy(p_info->key_string, sqlite3_column_text(stmt, 1), 31); 
			p_info->group_mask = sqlite3_column_int(stmt, 2);
			strncpy(p_info->lp_text, sqlite3_column_text(stmt, 3), 127); 
			strncpy(p_info->country, sqlite3_column_text(stmt, 4), 63); 

#if 0
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  /* 3 */
#endif			
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_search_lpr_data_from_text(char* p_lp_text, SQL_LPR_INFO* p_info)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	char *str = "select * from lpr_data where lp_text='%s'";
	snprintf(query, SQL_QUERY_BUF_LEN, str, p_lp_text);

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  

	//int i = 0;
	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              
			p_info->lpr_id = sqlite3_column_int(stmt, 0);
			strncpy(p_info->key_string, sqlite3_column_text(stmt, 1), 31); 
			p_info->group_mask = sqlite3_column_int(stmt, 2);
			strncpy(p_info->lp_text, sqlite3_column_text(stmt, 3), 127); 
			strncpy(p_info->country, sqlite3_column_text(stmt, 4), 63); 

#if 0
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  
#endif
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_search_lpr_data_from_like_text(char* p_lp_text, SQL_LPR_INFO* p_info)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	char *str = "select * from lpr_data where lp_text like '%%\%s\%';";
	snprintf(query, SQL_QUERY_BUF_LEN, str, p_lp_text);

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  

	//int i = 0;
	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              
			p_info->lpr_id = sqlite3_column_int(stmt, 0);
			strncpy(p_info->key_string, sqlite3_column_text(stmt, 1), 31); 
			p_info->group_mask = sqlite3_column_int(stmt, 2);
			strncpy(p_info->lp_text, sqlite3_column_text(stmt, 3), 127); 
			strncpy(p_info->country, sqlite3_column_text(stmt, 4), 63); 

#if 0
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  
#endif
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_update_lpr_data_from_id(SQL_LPR_INFO* p_info)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   

	char *src = "UPDATE lpr_data SET key_string='%s', group_mask=%d, lp_text='%s', country='%s' WHERE id=%d;"; 

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), src, p_info->key_string, p_info->group_mask, p_info->lp_text, p_info->country, p_info->lpr_id);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              
	
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("update success. (id=%d)\n", p_info->lpr_id);

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_drop_lpr_data_table()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   
	int ret = SQL_SET_ERROR;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	strncpy(query, "DROP TABLE lpr_data", 31);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("drop lpr table.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_drop_lpr_group_table()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   
	int ret = SQL_SET_ERROR;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	strncpy(query, "DROP TABLE lpr_group", 31);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("drop lpr table.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_find_matching_group_mask(int* p_group_mask, char* p_lp_text)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   
	char test_str[128];

	*p_group_mask = 0;

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	char *str = "select * from lpr_data";
	int tmp_mask = 0;
	sqlite3_prepare_v2(db, str, -1, &stmt, NULL);                              
	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              
		
		if(sqlite3_column_text(stmt, 3) != NULL)
		{
			if(strstr(p_lp_text, sqlite3_column_text(stmt, 3)) != NULL)
			{
				tmp_mask = sqlite3_column_int(stmt, 2);
				tmp_mask &= NUM_FACE_GROUP_MASK;
				*p_group_mask |= tmp_mask;
			}
		}

		printf("%s | %s | %d | %s |%s |%s |%s\n", 
				sqlite3_column_text(stmt, 0), 
				sqlite3_column_text(stmt, 1), 
				sqlite3_column_int(stmt, 2), 
				sqlite3_column_text(stmt, 3), 
				sqlite3_column_text(stmt, 4), 
				sqlite3_column_text(stmt, 5), 
				sqlite3_column_text(stmt, 6));  
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_create_default_lpr_group_table()
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	sqlite3_stmt *stmt;   
	char query[SQL_QUERY_BUF_LEN] = {0,};
	char group_name[64] = {0,};

	char* lpr_data_query = "CREATE TABLE lpr_group (id INTEGER PRIMARY KEY, group_num INTEGER,"
						   "group_name TEXT CHECK(typeof(group_name)='text'), UNIQUE(group_num));";

	//mnt path init check
#if 0
	int ret = 0;
	//nf_fw_network_upgrade_hdd_mount_state(1,&ret);

	if(ret < 1)
	{
		printf("[%s] ERROR : undefine facedata mount path \n", __func__);
		goto out;
	}
#endif
	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	//create basic_facedata
	sqlite3_prepare_v2(db, lpr_data_query, -1, &stmt, NULL);     
	rc = sqlite3_step(stmt);                                                                    
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR create table: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	int i = 0;
	for(i = 0; i < 8; i++)
	{
		memset(query, 0x00, SQL_QUERY_BUF_LEN);
		memset(group_name, 0x00, 64);
		snprintf(group_name, 63, "GROUP%d", i+1);

		snprintf(query, sizeof(query), "INSERT INTO lpr_group(group_num, group_name) values (%d, '%s');", 
				i+1, group_name);
		sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE) {
			printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
		}
	}


	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("group table created.\n");

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_update_lpr_group_name_from_id(int p_group_id, char *p_group_name)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   

	char *src = "UPDATE lpr_group SET group_num=%d, group_name='%s' WHERE id=%d;"; 

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query), src, p_group_id, p_group_name, p_group_id);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              
	
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	printf("update success. (id=%d)\n", p_group_id);

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_update_lpr_group_name_from_array(int p_max_index, int p_buf_len, char (*p_group_name)[p_buf_len])
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[p_buf_len];
	char total_query[p_buf_len * p_max_index];

	char *begin_str = "BEGIN IMMEDIATE TRANSACTION;";
	char *commit_str = "END TRANSACTION;";
	char *src = "UPDATE lpr_group SET group_num=%d, group_name='%s' WHERE id=%d;";

	if(strlen(src) + 1 >=  p_buf_len)
	{
		printf("[%s] ERROR p_buf_len too short (must be over %d, current : %d )\n", __func__, strlen(src) + 1, p_buf_len);
		goto out;
	}

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	rc = sqlite3_exec(db, begin_str, NULL, 0, NULL);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR transaction data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}
	
	int i = 0;
	for(i = 0; i < p_max_index; i++)
	{
		memset(query, 0x00, p_buf_len);
		snprintf(query, p_buf_len - 1, src, i+1, p_group_name[i], i+1);
		rc = sqlite3_exec(db, query, 0, 0, NULL);
		if (rc != SQLITE_OK) {
			printf("[%s] ERROR transaction data: %s\n", __func__, sqlite3_errmsg(db));
			goto out;
		}
	}

	rc = sqlite3_exec(db, commit_str, NULL, 0, NULL);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR transaction data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_close(db);
	printf("group_table update success.\n");

	return SQL_SET_OK;

out:
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int nf_sql_search_lpr_group_from_id(int p_group_id, char *p_group_text, int p_buf_len)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN] = {0,};
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(SQL_DB_PATH, &db);
	if (rc != SQLITE_OK) {
		printf("ERROR opening SQLite DB : %s\n", sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	char *str = "select * from lpr_group where id=%d";
	snprintf(query, SQL_QUERY_BUF_LEN, str, p_group_id);

	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, 16);                                                                  

	while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {                                              
			strncpy(p_group_text, sqlite3_column_text(stmt, 2), p_buf_len -1 ); 

#if 0
		    printf("%s | %s | %s | %s |%s |%s |%s\n", 
					sqlite3_column_text(stmt, 0), 
					sqlite3_column_text(stmt, 1), 
					sqlite3_column_text(stmt, 2), 
					sqlite3_column_text(stmt, 3), 
					sqlite3_column_text(stmt, 4), 
					sqlite3_column_text(stmt, 5), 
					sqlite3_column_text(stmt, 6));  
#endif
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return SQL_SET_OK;
out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int _nf_sql_create_db(char *p_db_name)
{
	sqlite3 *db;        // database connection
	int rc;             // return code

	rc = sqlite3_open(p_db_name, &db);
	if (rc != SQLITE_OK) {
		printf("[%s] ERROR create SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

#if SQL_DEBUG
	printf("db(%s) created.\n", p_db_name);
#endif

	return SQL_SET_OK;

out:
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}

int _nf_sql_drop_table(char* p_db_name, char *p_table_name)
{
	char query[SQL_QUERY_BUF_LEN] = {0,};
	snprintf(query, sizeof(query), "DROP TABLE %s", p_table_name);

	return _nf_sql_send_query(p_db_name, query);
}

int _nf_sql_send_query(char* p_db_name, char* p_query)
{
	sqlite3 *db;        // database connection
	int rc;             // return code
	char query[SQL_QUERY_BUF_LEN];
	sqlite3_stmt *stmt;   

	rc = sqlite3_open(p_db_name, &db);
	if (rc != SQLITE_OK) {
		printf("[%s]ERROR opening SQLite DB : %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	memset(query, 0x00, SQL_QUERY_BUF_LEN);
	snprintf(query, sizeof(query) -1 , p_query);
	sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);                              

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		printf("[%s] ERROR inserting data: %s\n", __func__, sqlite3_errmsg(db));
		goto out;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
#if SQL_DEBUG
	printf("db(%s) insert data.\n", p_db_name);
#endif

	return SQL_SET_OK;

out:
	if(stmt)
		sqlite3_finalize(stmt);
	if(db)
		sqlite3_close(db);

	return SQL_SET_ERROR;
}






//test
#if 0 
int nf_face_test()
{
	int ret;
	int count;

	ret = nf_sql_drop_facedata_table();

	ret = nf_sql_create_facedata_table();

	ret = nf_sql_insert_facedata("sjlim87", 0xF, NULL, "test.jpg");
	ret = nf_sql_insert_facedata("yys630", 0xF, NULL, "test.jpg");
	ret = nf_sql_insert_facedata("jhyoo", 0xF, NULL, "test.jpg");

	ret = nf_sql_delete_facedata(1);

	SQL_FACE_INFO* info = search_facedata_from_group(2, &count);
	printf("\e[104m%d \e[0m\n", info[0].face_id);
	printf("\e[104m%s \e[0m\n", info[0].name);
	printf("\e[104m%d \e[0m\n", info[1].face_id);
	printf("\e[104m%s \e[0m\n", info[1].name);
	if(info)
		free(info);
	
	printf("\e[31m g_prime_id(%d) \e[0m\n", g_prime_id);
	ret = nf_sql_delete_facedata(3);
	ret = nf_sql_insert_facedata("junbuck", 0xF, NULL, "test.jpg");
	printf("\e[31m g_prime_id(%d) \e[0m\n", g_prime_id);
	
	ret = nf_sql_update_facedata_from_id(2, "sjlim", "tt", 7, "test.jpg");

	ret = nf_sql_delete_facedata_all();

	return SQL_SET_OK;
}
#endif
