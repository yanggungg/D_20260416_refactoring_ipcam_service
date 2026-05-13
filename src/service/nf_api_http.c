#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "nf_util_function.h"
#include "nf_api_http.h"
#include "nf_ipcam_defs.h"
#include "nf_api_dlva.h"

#define URL_PATH_LEN (2048)

static size_t http_chunk_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
static int http_chunk_release(struct chunk *chunk);
static int http_chunk_init(struct chunk *chunk);
static pthread_mutex_t mutex_http_request = PTHREAD_MUTEX_INITIALIZER;

const char *tok_iterator_init_(Token_iterator *iter, char *str, const char *token)
{
	int str_len = 0;
	int token_len = 0;

	//argument check
	if(iter == NULL) return NULL;
	memset(iter, 0, sizeof(Token_iterator));

	if(str == NULL) return NULL;
	if(token == NULL) return NULL;

	str_len = strlen(str);
	token_len = strlen(token);
	if(str_len == 0 || token_len == 0) return NULL;

	//init data

    /*
	iter->buffer = (char *)malloc(str_len + 1);
	if(iter->buffer == NULL){
		goto err;
	}
	iter->token = (char *)malloc(token_len + 1);
	if(iter->token == NULL){
		goto err;
	}

	snprintf(iter->buffer, str_len + 1, str);
	snprintf(iter->token, token_len + 1, token);
    */

    iter->buffer = str;
    iter->token = token;

	iter->token_length = token_len;
	iter->end = iter->buffer + str_len;

	iter->next = iter->buffer;
	str = tok_get_next(iter);
	
	if(str == NULL){
		goto err;
	}

	return str;
err:
	return NULL;
}

const char *tok_iterator_init(Token_iterator *iter, const char *str, const char *token)
{
	int str_len = 0;
	int token_len = 0;

	//argument check
	if(iter == NULL) return NULL;
	memset(iter, 0, sizeof(Token_iterator));

	if(str == NULL) return NULL;
	if(token == NULL) return NULL;

	str_len = strlen(str);
	token_len = strlen(token);
	if(str_len == 0 || token_len == 0) return NULL;

	//init data

	iter->buffer = (char *)malloc(str_len + 1);
	if(iter->buffer == NULL){
		goto err;
	}
	iter->token = (char *)malloc(token_len + 1);
	if(iter->token == NULL){
		goto err;
	}

	snprintf(iter->buffer, str_len + 1, str);
	snprintf(iter->token, token_len + 1, token);

	iter->token_length = token_len;
	iter->end = iter->buffer + str_len;

	iter->next = iter->buffer;
	str = tok_get_next(iter);
	
	if(str == NULL){
		goto err;
	}

	return str;
err:
	release_token(iter);
	return NULL;
}

int is_token_char(char c, char *token, int token_length)
{
	int i;
	for(i = 0; i < token_length; i++){
		if(token[i] == c){
			return 1;
		}
	}
	return 0;
}

const char *tok_get_next(Token_iterator *iter)
{
	for(iter->current = iter->next; iter->current < iter->end; iter->current++){
		if(!is_token_char(*(iter->current), iter->token, iter->token_length)){
			break;
		}
	}
	
	if(iter->current == iter->end){
		//release_token(iter);
	   	return NULL;
	}

	for(iter->next = iter->current + 1; iter->next < iter->end; iter->next++){
		if(is_token_char(*(iter->next), iter->token, iter->token_length)){
			*(iter->next++) = '\0';
			break;
		}
	}

	return iter->current;
}

void release_token(Token_iterator *iter)
{
	if(iter == NULL) return;
	if(iter->buffer){
	   	free(iter->buffer);
		iter->buffer = NULL;
	}
	if(iter->token){
	   	free(iter->token);
		iter->token = NULL;
	}
}

int aibox_http_default_setting_multipart(HTTP_CTX *ctx, unsigned int aibox_ip)
{
	char ipbuf[20];

	if(ctx == NULL) return 1;
	if(aibox_ip == 0 || aibox_ip == 0xffffffff) return -1;

	memset(ipbuf, 0, sizeof(ipbuf));
	
	//set default
	http_data_set(ctx, HTTP_SET_HOST			, _ip_to_str(aibox_ip, ipbuf));
	http_data_set(ctx, HTTP_SET_PORT			, 8443);
	http_data_set(ctx, HTTP_SET_SSL				, 1);
	http_data_set(ctx, HTTP_SET_AUTH_TYPE       , CURLAUTH_DIGEST);
	http_data_set(ctx, HTTP_SET_TIME_OUT		, 10);				//default 5
	http_data_set(ctx, HTTP_SET_CONN_TIME_OUT	, 3);				//default 3
	http_data_set(ctx, HTTP_ADD_HEADER 			, "Connection", "Keep-Alive");
	return 0;
}

int aibox_http_default_setting(HTTP_CTX *ctx, unsigned int aibox_ip)
{
	char ipbuf[20];
	int rc;

	int nvr_ch;
	int web_port;
	int use_ssl;
	//char id[200];
	//char pw[200];

	if(ctx == NULL) return 1;
	if(aibox_ip == 0 || aibox_ip == 0xffffffff) return -1;

	memset(ipbuf, 0, sizeof(ipbuf));
	//memset(id, 0, sizeof(id));
	//memset(pw, 0, sizeof(pw));
	
	rc = nf_api_get_aibox_info(aibox_ip, &nvr_ch, &web_port, &use_ssl, NULL /*id*/, NULL /*pw*/);
    if(rc < 0)
    {
        use_ssl = 1;
        web_port = 8443;
        //sprintf(id, "ADMIN");
        //sprintf(pw, "1234");
    }
	
	//set default
	http_data_set(ctx, HTTP_SET_HOST			, _ip_to_str(aibox_ip, ipbuf));
	http_data_set(ctx, HTTP_SET_PORT			, web_port);
	http_data_set(ctx, HTTP_SET_SSL				, use_ssl);
	http_data_set(ctx, HTTP_SET_AUTH_TYPE       , CURLAUTH_DIGEST);
	http_data_set(ctx, HTTP_SET_TIME_OUT		, 10);				//default 5
	http_data_set(ctx, HTTP_SET_CONN_TIME_OUT	, 3);				//default 3
	http_data_set(ctx, HTTP_ADD_HEADER 			, "Connection", "Keep-Alive");
	http_data_set(ctx, HTTP_ADD_HEADER 			, "Content-Type", "application/json");

	return 0;
}

int aicam_http_default_setting(HTTP_CTX *ctx, int ch)
{
	mtable *runtime = get_runtime();
	char ipbuf[20];

	if(ctx == NULL){
		printf("[%s:%d] http ctx is NULL\n", __func__, __LINE__);
		return 1;
	}
	if(runtime == NULL){
		printf("[%s:%d] runtime is NULL\n", __func__, __LINE__);
	   	return 1;
	}

	memset(ipbuf, 0, sizeof(ipbuf));
	
	//set default
	http_data_set(ctx, HTTP_SET_HOST			, _ip_to_str(ntohl(runtime[ch].sys.ipaddr), ipbuf));
	http_data_set(ctx, HTTP_SET_PORT			, (runtime[ch].sys.http_port));
	http_data_set(ctx, HTTP_SET_SSL				, (runtime[ch].sys.use_ssl));
	http_data_set(ctx, HTTP_SET_USER			, runtime[ch].username);
	http_data_set(ctx, HTTP_SET_PASSWD			, runtime[ch].password);
	http_data_set(ctx, HTTP_SET_TIME_OUT		, 10);				//default 5
	http_data_set(ctx, HTTP_SET_CONN_TIME_OUT	, 3);				//default 3

	return 0;
}

/* STATIC HTTP FUNCTION */
static int http_chunk_init(struct chunk *chunk)
{
	int rtn = 1;
	if(chunk != NULL)
	{
		memset(chunk, 0x00, sizeof(struct chunk));
		chunk->memory = calloc(1,1);
		chunk->size = 0;
		rtn = 0;
	}
	return rtn;
}

static int http_chunk_release(struct chunk *chunk)
{
	int rtn = 1;
	if(chunk != NULL)
	{
		if(chunk->memory != NULL)
		{
			free(chunk->memory);
			chunk->memory = NULL;
			chunk->size = 0;
			rtn = 0;
		}
	}
	return rtn;
}

static char *rtrim(char *str)
{
	int len;
	int i;
	if(str == NULL) return 0;

	len = (int)strlen(str);
	for(i = len-1; i >= 0; i--){
		if(str[i] == ' ' ||
				str[i] == '\n' ||
				str[i] == '\r' ||
				str[i] == '\t'){
			str[i] = '\0';
		}else{
			break;
		}
	}
	//return i;
	return str;
}

static char *ltrim(char *str)
{
	while(*str != '\0'){
		if(*str == ' ') str++;
		else break;
	}
	return str;
}

int strnicmp(char *str1, char *str2, int len)
{
	int i;
	int count = 0;
	for(i = 0; i < len; i++){
		if(str1[i] == '\0' && str2[i] == '\0') break;
		if(tolower(str1[i]) != tolower(str2[i])){
			count++;
		}
	}
	return count;
}

static int parseHeader(char *ptr, char *key, char *value)
{
	int len = (int)strlen(ptr);
	int i;

	for(i = 0; i < len; i++){
		if(ptr[i] == ':') break;
	}
	if(i == len) return 0;

	strncpy(key, ptr, (size_t)i);
	key[i] = 0;

	for(i++; ptr[i] != '\0'; i++){
		if(ptr[i] != ' ') break;
	}

	strncpy(value, ltrim(ptr+i), (size_t)(len - i));
	rtrim(value);
	rtrim(key);

	return 1;
}

static size_t http_header_parser(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = size * nmemb;

	char key[512], value[512];
	memset(key, 0, sizeof(key));
	memset(value, 0, sizeof(value));
	
	if(userdata != NULL)
	{
		if(parseHeader(ptr, key, value)){
			if(strnicmp(key, "date", 4) == 0){
				char **date = (char **)userdata;
				size_t len = strlen(value);

				*date = malloc(len+1);
				if(date != NULL){
					strncpy(*date, value, len);
					(*date)[len] = 0;
				}
			}
		}
	}


	return realsize;
}

static size_t http_chunk_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = size * nmemb;
	if(userdata != NULL)
	{
		struct chunk *mem = (struct chunk *)userdata;

		mem->memory = realloc(mem->memory, mem->size + realsize + 1);
		if(mem->memory == NULL) {
			/* out of memory! */ 
			printf("not enough memory (realloc returned NULL)\n");
			return 0;
		}

		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}

	return realsize;
}

int http_init(HTTP_CTX *http_ctx)
{
	if(http_ctx == NULL) return -1;

	memset(http_ctx, 0x00, sizeof(HTTP_CTX));

	http_chunk_init(&(http_ctx->res_header));
	http_chunk_init(&(http_ctx->res_data));


	//set default
	http_data_set(http_ctx, HTTP_SET_PORT    , 80);
	http_data_set(http_ctx, HTTP_SET_TIME_OUT, 5);
	return 0;
}

char *http_strdup(const char *str)
{
	size_t len;
	char *newstr;

	if(!str)
		return (char *)NULL;

	len = strlen(str);
	if(len == 0) return NULL;

	if(len >= ((size_t)-1) / sizeof(char))
		return (char *)NULL;

	newstr = malloc((len + 1)*sizeof(char));
	if(!newstr)
		return (char *)NULL;

	memcpy(newstr, str, (len + 1)*sizeof(char));

	return newstr;
}

int http_set_string(char **dst, char *src)
{
	int len = 0;
	char *buffer = NULL;
	if(src == NULL) return -1;

	len = (int)strlen(src);
	if(len == 0) return -2;

	
	buffer = http_strdup(src);
	if(buffer == NULL) return -4;

	if(*dst != NULL){
		free(*dst);
	}
	*dst = buffer;

	return len;
}

int http_add_query(HTTP_CTX *http_ctx, char *key, char *value)
{
	int current_len = 0;
	int value_len = 0;
	int key_len = 0;
	int alloc_size = 0;
	
	if(key == NULL || value == NULL || http_ctx == NULL){
		printf("[%s:%d] error key[%p] value[%p] http_ctx[%p]\n", __func__, __LINE__, key, value, http_ctx);
		return -1;
	}

	key_len = strlen(key);
	value_len = strlen(value);
	//if(key_len == 0 || value_len == 0){
	if(key_len==0){ // value 공백 허용
		printf("[%s:%d] error key[%d] value[%d]\n", __func__, __LINE__, key_len, value_len);
		return -1;
	}

	//memory alloc
	if(http_ctx->query == NULL){
		alloc_size = value_len + key_len + 3;
		http_ctx->query = malloc(alloc_size);
	}else{
		current_len = strlen(http_ctx->query);
		alloc_size = current_len + value_len + key_len + 3;
		http_ctx->query = realloc(http_ctx->query, alloc_size);
	}

	if(http_ctx->query == NULL){
		printf("[%s:%d] out of memory\n", __func__, __LINE__);
		return -1;
	}

	if(current_len){
		http_ctx->query[current_len++] = '&';
	}
	//http_ctx->query[current_len] = '\0';

	//add string
	snprintf(http_ctx->query + current_len, alloc_size, "%s=%s", key, value);
	
	return 0;
}

int http_add_header(HTTP_CTX *http_ctx, char *key, char *value)
{
	struct curl_slist *new_header_list = NULL;
	char buffer[512];

	if(key == NULL) return -1;
	if(key[0] == '\0') return -2;
	if(value == NULL) return -3;
	if(value[0] == '\0') return -4;

	snprintf(buffer, 512, "%s: %s", key, value);
	//printf("[%s:%d] http_ctx[%p] key[%s] value[%s] header[%s]\n", __FUNCTION__, __LINE__,
	//		http_ctx, key, value, buffer);

	new_header_list = curl_slist_append(http_ctx->header_list, buffer);
	if(new_header_list == NULL) return -5;
	http_ctx->header_list = new_header_list;
	return 0;
}


//#define _HTTP_DEBUG_
int http_request(HTTP_CTX *http_ctx)
{
	static CURL *curl = NULL;
	CURLcode res = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = {"http", "https"};

#ifdef _HTTP_DEBUG_
	http_req_data_print(http_ctx);
#endif

	if(http_ctx == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&mutex_http_request);
	if(curl == NULL){
        curl = curl_easy_init();
    }else{
        curl_easy_reset(curl);
    }

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		if(http_ctx->query != NULL && http_ctx->query[0] != '\0'){
			snprintf(url, URL_PATH_LEN, "%s://%s:%d%s?%s", protocol_list[http_ctx->ssl], http_ctx->host, http_ctx->port, http_ctx->path,http_ctx->query);
		}else{
			snprintf(url, URL_PATH_LEN, "%s://%s:%d%s", protocol_list[http_ctx->ssl], http_ctx->host, http_ctx->port, http_ctx->path);
		}

		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "IPX-NVR");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		//curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION , http_chunk_write_callback);
		//curl_easy_setopt(curl, CURLOPT_HEADERDATA, &(http_ctx->res_header));
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION , http_header_parser);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &(http_ctx->http_header_date));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(http_ctx->res_data));
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)(http_ctx->time_out));
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)(http_ctx->conn_time_out));
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

        if(http_ctx->auth_type){
            //printf("[%s:%d] http_ctx->user[%s] auth[%d] digest\n", __func__, __LINE__, http_ctx->user, http_ctx->auth_type);
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)http_ctx->auth_type);
        }

		if(http_ctx->header_list != NULL){
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_ctx->header_list);
		}

		if(http_ctx->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

        if(http_ctx->formpost){
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, http_ctx->formpost);
        }else
		if(http_ctx->buffer != NULL && http_ctx->buffer_size > 0){
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, http_ctx->buffer);
		}

		if(http_ctx->method != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, http_ctx->method);
		}

		if(http_ctx->user != NULL && http_ctx->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, http_ctx->user);
			strcat(idpwd, ":");
			strcat(idpwd, http_ctx->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		res = curl_easy_perform(curl);
        //printf("[%s:%d] res[%d]\n", __func__, __LINE__, res);

        if(res != CURLE_OK){
            printf("[%s:%d] curl_easy_perform error - [%d][%s]\n", __func__, __LINE__, res, curl_easy_strerror(res));
        }else{
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_ctx->status);
            
            if(http_ctx->status == 401 || http_ctx->status == 500){
                printf("[%s:%d] status[%d] session reset\n", __func__, __LINE__, http_ctx->status);

                /*
                curl_easy_cleanup(curl);
                curl = NULL;

                free(http_ctx->res_data.memory);
                http_ctx->res_data.memory = NULL;
                http_ctx->res_data.size = 0;
                res = http_request(http_ctx);
                printf("[%s:%d] res[%d]\n", __func__, __LINE__, res);
                */
            }
            
        }

        curl_easy_cleanup(curl);
        curl = NULL;

	}
	else
	{
		printf("curl_easy_init fail\n");
	}
#ifdef _HTTP_DEBUG_
	http_res_data_print(http_ctx);
#endif
	pthread_mutex_unlock(&mutex_http_request);
	return res;
}

int http_request_always_init(HTTP_CTX *http_ctx) // bug fix - SWPFOURCE-636
{
	CURL *curl = NULL;
	CURLcode res = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = {"http", "https"};

#ifdef _HTTP_DEBUG_
	http_req_data_print(http_ctx);
#endif

	if(http_ctx == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&mutex_http_request);
	//if(curl == NULL){
	//        curl = curl_easy_init();
	//}
	//else
	//{
	//	curl_easy_reset(curl);
	//}
	curl = curl_easy_init();  // cache,session,cookies.. are always init

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		if(http_ctx->query != NULL && http_ctx->query[0] != '\0'){
			snprintf(url, URL_PATH_LEN, "%s://%s:%d%s?%s", protocol_list[http_ctx->ssl], http_ctx->host, http_ctx->port, http_ctx->path,http_ctx->query);
		}else{
			snprintf(url, URL_PATH_LEN, "%s://%s:%d%s", protocol_list[http_ctx->ssl], http_ctx->host, http_ctx->port, http_ctx->path);
		}

		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "IPX-NVR");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		//curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION , http_chunk_write_callback);
		//curl_easy_setopt(curl, CURLOPT_HEADERDATA, &(http_ctx->res_header));
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION , http_header_parser);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &(http_ctx->http_header_date));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(http_ctx->res_data));
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)(http_ctx->time_out));
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)(http_ctx->conn_time_out));
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

        if(http_ctx->auth_type){
            //printf("[%s:%d] http_ctx->user[%s] auth[%d] digest\n", __func__, __LINE__, http_ctx->user, http_ctx->auth_type);
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)http_ctx->auth_type);
        }

		if(http_ctx->header_list != NULL){
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_ctx->header_list);
		}

		if(http_ctx->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

        if(http_ctx->formpost){
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, http_ctx->formpost);
        }else
		if(http_ctx->buffer != NULL && http_ctx->buffer_size > 0){
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, http_ctx->buffer);
		}

		if(http_ctx->method != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, http_ctx->method);
		}

		if(http_ctx->user != NULL && http_ctx->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, http_ctx->user);
			strcat(idpwd, ":");
			strcat(idpwd, http_ctx->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		res = curl_easy_perform(curl);
        //printf("[%s:%d] res[%d]\n", __func__, __LINE__, res);

        if(res != CURLE_OK){
            printf("[%s:%d] curl_easy_perform error - [%d][%s]\n", __func__, __LINE__, res, curl_easy_strerror(res));
        }else{
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_ctx->status);
            
            if(http_ctx->status == 401 || http_ctx->status == 500){
                printf("[%s:%d] status[%d] session reset\n", __func__, __LINE__, http_ctx->status);

                /*
                curl_easy_cleanup(curl);
                curl = NULL;

                free(http_ctx->res_data.memory);
                http_ctx->res_data.memory = NULL;
                http_ctx->res_data.size = 0;
                res = http_request(http_ctx);
                printf("[%s:%d] res[%d]\n", __func__, __LINE__, res);
                */
            }
        }

		curl_easy_cleanup(curl);
		curl = NULL;
}
	else
	{
		printf("curl_easy_init fail\n");
	}
#ifdef _HTTP_DEBUG_
	http_res_data_print(http_ctx);
#endif
	pthread_mutex_unlock(&mutex_http_request);
	return res;
}

static int http_add_multipart(HTTP_CTX *http_ctx, const char *name, const char *value)
{
    if(http_ctx == NULL || name == NULL || value == NULL){
        printf("[%s:%d] error http_ctx[%p] name[%p] value[%p]\n", __func__, __LINE__, http_ctx, name, value);
        return -1;
    }
    if(strlen(name) <= 0 || strlen(value) <= 0){
        printf("[%s:%d] error name[%s] value[%s]\n", __func__, __LINE__, name, value);
        return -2;
    }

    curl_formadd(&(http_ctx->formpost),
            &(http_ctx->lastptr),
            CURLFORM_COPYNAME, name,
            CURLFORM_COPYCONTENTS, value,
            CURLFORM_END);

    //printf("[%s:%d] name[%s] value[%s]\n", __func__, __LINE__, name, value);
    return 0;
}

int http_data_set(HTTP_CTX *http_ctx, HTTP_SET_OPTION option, ...)
{
	va_list arg;
	int ret = 0;

	if(http_ctx == NULL)
		  return -1;

	va_start(arg, option);

	switch(option)
	{
		case HTTP_SET_HOST:
			if(http_set_string(&(http_ctx->host), (char *)va_arg(arg, char *)) <= 0) ret = -2;
			break;
		case HTTP_SET_PORT: 
			http_ctx->port = (unsigned short)va_arg(arg, int);
			break;
		case HTTP_SET_SSL: 
			http_ctx->ssl = (unsigned int)va_arg(arg, unsigned int);
			break;
		case HTTP_SET_AUTH_TYPE: 
			http_ctx->auth_type = (long)va_arg(arg, long);
			break;
		case HTTP_SET_TIME_OUT: 
			http_ctx->time_out = (int)va_arg(arg, int);
			break;
		case HTTP_SET_CONN_TIME_OUT: 
			http_ctx->conn_time_out = (int)va_arg(arg, int);
			break;
		case HTTP_SET_METHOD: 
			if(http_set_string(&(http_ctx->method), (char *)va_arg(arg, char *)) <= 0) ret = -2;
			break;
		case HTTP_SET_USER: 
			if(http_set_string(&(http_ctx->user), (char *)va_arg(arg, char *)) <= 0) ret = -2;
			break;
		case HTTP_SET_PASSWD: 
			if(http_set_string(&(http_ctx->passwd), (char *)va_arg(arg, char *)) <= 0) ret = -2;
			break;
		case HTTP_SET_PATH: 
			if(http_set_string(&(http_ctx->path), (char *)va_arg(arg, char *)) <= 0) ret = -2;
			break;
		case HTTP_SET_QUERY: 
			if(http_set_string(&(http_ctx->query), (char *)va_arg(arg, char *)) <= 0) ret = -2;
			break;
		case HTTP_ADD_QUERY: 
			//if(http_add_query(http_ctx, (char *)va_arg(arg, char *), (char *)va_arg(arg, char *)) < 0) ret = -2;
            {
                const char *key = (char *)va_arg(arg, char *);
                const char *value = (char *)va_arg(arg, char *);

                if(http_add_query(http_ctx, key, value) < 0) ret = -2;
            }
			break;
		case HTTP_ADD_HEADER:
			{
				char *key = (char *)va_arg(arg, char *);
				char *value = (char *)va_arg(arg, char *);
				if(http_add_header(http_ctx, key, value) < 0) ret = -2;
			}
			break;
		case HTTP_SET_POST:
			{
				int len;
				len = http_set_string(&(http_ctx->buffer), (char *)va_arg(arg, char *));
				if(len <= 0){
					ret = -2;
				}else{
					http_ctx->buffer_size = (size_t)len;
				}
			}
			break;
		case HTTP_ADD_MULTIPART:
			{
				const char *name = (char *)va_arg(arg, char *);
				const char *value = (char *)va_arg(arg, char *);
				ret = http_add_multipart(http_ctx, name, value);
				if(ret < 0){
                    printf("[%s:%d] http_add_multipart[%d]\n", __func__, __LINE__, ret);
                    ret = -2;
                }
			}
			break;
		default:
			ret = -2;
			break;
	}

	va_end(arg);
	return ret;
}

const char *http_get_res(HTTP_CTX *http_ctx)
{
	return (http_ctx->res_data).memory;
}

void http_release(HTTP_CTX *http_ctx)
{
	if(http_ctx == NULL) return;

	if(http_ctx->host){
		free(http_ctx->host);
		http_ctx->host=NULL;
	}
	if(http_ctx->user){
		free(http_ctx->user);
		http_ctx->user=NULL;
	}
	if(http_ctx->passwd){
		free(http_ctx->passwd);
		http_ctx->passwd=NULL;
	}
	if(http_ctx->method){
		free(http_ctx->method);
		http_ctx->method=NULL;
	}
	if(http_ctx->path){
		free(http_ctx->path);
		http_ctx->path=NULL;
	}
	if(http_ctx->query){
		free(http_ctx->query);
		http_ctx->query=NULL;
	}
	if(http_ctx->buffer){
		free(http_ctx->buffer);
		http_ctx->buffer=NULL;
	}
	if(http_ctx->http_header_date){
		free(http_ctx->http_header_date);
		http_ctx->http_header_date=NULL;
	}

	curl_slist_free_all(http_ctx->header_list);

	//response
	http_chunk_release(&(http_ctx->res_header));
	http_chunk_release(&(http_ctx->res_data));
}

void http_res_data_print(HTTP_CTX *ctx)
{
	printf("[%s:%d] [%-10s][%s]\n", __FUNCTION__, __LINE__, "date", ctx->http_header_date);
	printf("[%s:%d] [%-10s][%s]\n", __FUNCTION__, __LINE__, "body", ctx->res_data.memory);
	printf("[%s:%d] [%-10s][%d]\n", __FUNCTION__, __LINE__, "status", ctx->status);
}

void http_req_data_print(HTTP_CTX *ctx)
{
	printf("[%s:%d] [%-13s][%s]\n", __FUNCTION__, __LINE__, "host       ", ctx->host       );
	printf("[%s:%d] [%-13s][%d]\n", __FUNCTION__, __LINE__, "port       ", ctx->port       );
	printf("[%s:%d] [%-13s][%d]\n", __FUNCTION__, __LINE__, "ssl        ", ctx->ssl        );
	printf("[%s:%d] [%-13s][%d]\n", __FUNCTION__, __LINE__, "time_out   ", ctx->time_out   );
	printf("[%s:%d] [%-13s][%s]\n",  __FUNCTION__, __LINE__, "user       ", ctx->user       );
	printf("[%s:%d] [%-13s][%s]\n",  __FUNCTION__, __LINE__, "passwd     ", ctx->passwd     );
	printf("[%s:%d] [%-13s][%s]\n",  __FUNCTION__, __LINE__, "method"     , ctx->method	    );
	printf("[%s:%d] [%-13s][%s]\n",  __FUNCTION__, __LINE__, "path       ", ctx->path       );
	printf("[%s:%d] [%-13s][%s]\n",  __FUNCTION__, __LINE__, "query      ", ctx->query      );
	printf("[%s:%d] [%-13s][%s]\n",  __FUNCTION__, __LINE__, "post       ", ctx->buffer     );
	printf("[%s:%d] [%-13s][%ld]\n", __FUNCTION__, __LINE__, "post       ", ctx->buffer_size);

	int index = 0;
    struct curl_slist *iter;
	for(iter = ctx->header_list, index = 0; iter != NULL; iter = iter->next){
		printf("[%s:%d] [%-13s][%s]\n", __FUNCTION__, __LINE__, "header_list", iter->data);
	}
	
}


/*
static const char MimeBase64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static int DecodeMimeBase64[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 00-0F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 10-1F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  // 20-2F 
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  // 30-3F 
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  // 40-4F 
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  // 50-5F 
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  // 60-6F 
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  // 70-7F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 80-8F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 90-9F 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // A0-AF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // B0-BF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // C0-CF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // D0-DF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // E0-EF 
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   // F0-FF 
};

int base64_encode_len(int numBytes)
{
	int size = 0;

	size = (4 * (numBytes / 3)) + (numBytes % 3? 4 : 0) + 1;

	return size;
}

int base64_encode(char *text, int numBytes, char *encodedText)
{
	unsigned char input[3]  = {0,0,0};
	unsigned char output[4] = {0,0,0,0};
	int   index, i, j;
	char *p, *plen;

	plen = text + numBytes - 1;
	j = 0;

	for  (i = 0, p = text;p <= plen; i++, p++) {
		index = i % 3;
		input[index] = *p;

		if (index == 2 || p == plen) {
			output[0] = ((input[0] & 0xFC) >> 2);
			output[1] = ((input[0] & 0x3) << 4) | ((input[1] & 0xF0) >> 4);
			output[2] = ((input[1] & 0xF) << 2) | ((input[2] & 0xC0) >> 6);
			output[3] = (input[2] & 0x3F);
			encodedText[j++] = MimeBase64[output[0]];
			encodedText[j++] = MimeBase64[output[1]];
			encodedText[j++] = index == 0? '=' : MimeBase64[output[2]];
			encodedText[j++] = index <  2? '=' : MimeBase64[output[3]];
			input[0] = input[1] = input[2] = 0;
		}
	}

	encodedText[j] = '\0';

	return j;
}

int base64_decode(char *text, int numBytes, char *decodedText)
{
	const char* cp;
	int space_idx = 0; 
	int phase = 0;
	int d, prev_d = 0;
	unsigned char c;
	
	for ( cp = text; *cp != '\0'; ++cp ) {
		d = DecodeMimeBase64[(int) *cp];
		if ( d != -1 ) {
			switch ( phase ) {
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( space_idx < numBytes )
						decodedText[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( space_idx < numBytes )
						decodedText[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( space_idx < numBytes )
						decodedText[space_idx++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	
	return space_idx;
}
*/

char *hexdigest(char *out, unsigned char *in, int in_len)
{
	static char buffer[2048];
	int i;

	if(out == NULL) out = buffer;
	

	for(i = 0; i < in_len; i++){
		sprintf(out + (i*2), "%02x", in[i]);
	}

	out[in_len*2] = 0;
	return out;
}

char *http_get_encrypt_hmac(char *key, const unsigned char *data, const EVP_MD *type)
{
	int ret = 0;
	unsigned int enc_len = 512;
	unsigned char *enc_buf = (unsigned char*) malloc(sizeof(char) * enc_len);

	char *hash = NULL;

	if(enc_buf == NULL) goto err;

	HMAC_CTX ctx;

	ENGINE_load_builtin_engines();
	ENGINE_register_all_complete();

	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, (int)strlen(key), type, NULL);
	HMAC_Update(&ctx, data, strlen((char *)data));
	HMAC_Final(&ctx, enc_buf, &enc_len);
	HMAC_CTX_cleanup(&ctx);

	//printf("enc_len[%d] enc_buf[%s]\n", enc_len, enc_buf);
	//printf("enc_len[%d] enc_buf[%s]\n", enc_len, hexdigest(NULL, enc_buf, enc_len));

	hash = (char *)malloc((enc_len*2)+1);
	if(hash == NULL) goto err;

	hexdigest(hash, enc_buf, (int)enc_len);

	free(enc_buf);

	return hash;
/*
	//base64 encoding
	base64_enc_len = base64_encode_len(enc_len);
	base64_enc = malloc(base64_enc_len+1);
	if(base64_enc == NULL){
		goto err;
	}
	base64_encode(enc_buf, enc_len, base64_enc);
	free(enc_buf);
	printf("base64_enc_len[%d] base64_enc[%s]\n", base64_enc_len, base64_enc);
	return base64_enc;
	*/
err:
	if(hash != NULL) free(hash);
	if(enc_buf != NULL) free(enc_buf);
	return NULL;
}




