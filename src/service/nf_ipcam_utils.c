#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <curl/curl.h>
#include <zlib.h>
#include <jansson.h>

#include "nf_ipcam_utils.h"
#include "nf_ipcam_defs.h"

#define URL_PATH_LEN (2048)
#define HTTP_API_DEBUG (0)
#define REQUEST_TIME_MESURE (1)
#define HTTP_TIME_OUT_SEC (10)

/* STRUCTURE */
struct http_request_info
{
	const char *host;
	unsigned short port;
	unsigned int ssl;
	int time_out;

	const char *user;
	const char *passwd;

	const char *path;
	const char *query;

	int status;
	char *buffer;
	size_t buffer_size;

	const char *user_agent;
};

struct http_upload_info
{
	char *http_head[ADD_HEADER_MAX];
	int	head_cnt;

	char *form_name;
	char *file_name;
	char *file_buffer;
	size_t file_buffer_size;
};

struct chunk
{
	char *memory;
	size_t size;
};


/* STATIC FUNCTION */
void timespec_diff(const struct timespec *start, const struct timespec *stop, struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
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


// GET or POST
//
static int http_error_check(const char *host, const unsigned short port, const int ret, const int status)
{
	int rtn = 0;
	char error_msg[512] = {0,};

	if(ret == 0 && status == 200)
		rtn = ICM_RTN_OK;
	else if(ret == 0 && status == 401)
		rtn = ICM_RTN_AUTH_FAIL;
	else if(ret == 0 && status == 404)
		rtn = ICM_RTN_NOT_FOUND;
	else if(ret == 0 && status == 405)
		rtn = ICM_RTN_METHOD_NOT_ALLOWED;
	else if(ret == 0 && status == 500)
		rtn = ICM_RTN_SERVER_ERR;
	else if(ret == 7)
		rtn = ICM_RTN_CONNECT_FAIL;
	else if(ret == 28)
		rtn = ICM_RTN_TIMEOUT;
	else if(ret == 35)
		rtn = ICM_RTN_SSL_CONNECT_FAIL;
	else if(ret == 52)
		rtn = ICM_RTN_NOTHING;
	else
		rtn = ICM_RTN_UNKNOWN;

	if(ret != 0)
	{
		snprintf(error_msg,512, "<%s> host(%s:%d) ret(%d)  msg(%s)",__FUNCTION__, host, port, ret, curl_easy_strerror(ret));
		puts(error_msg);
	}

	return rtn;
}

int http_curl_post_json_request(struct chunk *chunk, const struct http_request_info *request)
{
	CURL *curl = NULL;
	CURLcode res = 0;
	int port = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = { "http", "https" };
	struct curl_slist *hs=NULL;


	if(request == NULL)
	{
		return -1;
	}

	curl = curl_easy_init();

	if(request->port == 0)
	{
		port = 80;
	}
	else
	{
		port = request->port;
	}

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		snprintf(url, URL_PATH_LEN, "%s://%s:%d%s", protocol_list[request->ssl], request->host, port, request->path);
#if HTTP_API_DEBUG
		printf("<%s> URL: %s\n", __FUNCTION__, url);
		printf("<%s> QUERY: %s\n", __FUNCTION__, request->query);
#endif

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

		if(request->user_agent != NULL)
			curl_easy_setopt(curl, CURLOPT_USERAGENT, request->user_agent);
		else
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "IPX-NVR");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->time_out); 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

		//header list
		hs = curl_slist_append(hs, "Content-Type: application/json");
		if(hs != NULL){
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
		}else{
			printf("<%s> header list init append error\n", __FUNCTION__);
		}

		if(request->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		if(request->query != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->query);
		}
		if(request->user != NULL && request->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, request->user);
			strcat(idpwd, ":");
			strcat(idpwd, request->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		res = curl_easy_perform(curl);

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &request->status);

		if(hs != NULL) 
			curl_slist_free_all(hs);	
		curl_easy_cleanup(curl);

#if HTTP_API_DEBUG
		if(chunk != NULL)
		{
			printf("<%s> >>> RESPONE(size:%d)\n", __FUNCTION__, chunk->size);
			puts(chunk->memory);
			printf("<%s> <<< RESPONE\n", __FUNCTION__);
		}
#endif
	}
	else
	{
		printf("curl_easy_init fail\n");
	}

	return res;
}

static int http_curl_post_request(struct chunk *chunk, const struct http_request_info *request)
{
	CURL *curl = NULL;
	CURLcode res = 0;
	int port = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = { "http", "https" };


	if(request == NULL)
	{
		return -1;
	}

	curl = curl_easy_init();

	if(request->port == 0)
	{
		port = 80;
	}
	else
	{
		port = request->port;
	}

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		snprintf(url, URL_PATH_LEN, "%s://%s:%d%s", protocol_list[request->ssl], request->host, port, request->path);
#if HTTP_API_DEBUG
		printf("<%s> URL: %s\n", __FUNCTION__, url);
		printf("<%s> QUERY: %s\n", __FUNCTION__, request->query);
#endif

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

		if(request->user_agent != NULL)
			curl_easy_setopt(curl, CURLOPT_USERAGENT, request->user_agent);
		else
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "IPX-NVR");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->time_out); 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

		if(request->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		if(request->query != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->query);
		}
		if(request->user != NULL && request->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, request->user);
			strcat(idpwd, ":");
			strcat(idpwd, request->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		res = curl_easy_perform(curl);

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &request->status);

		curl_easy_cleanup(curl);

#if HTTP_API_DEBUG
		if(chunk != NULL)
		{
			printf("<%s> >>> RESPONE(size:%d)\n", __FUNCTION__, chunk->size);
			puts(chunk->memory);
			printf("<%s> <<< RESPONE\n", __FUNCTION__);
		}
#endif
	}
	else
	{
		printf("curl_easy_init fail\n");
	}

	return res;
}

static int http_curl_get_json_request(struct chunk *chunk, const struct http_request_info *request)
{
	CURL *curl = NULL;
	CURLcode res = 0;;
	int port = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = { "http", "https" };
	struct curl_slist *hs=NULL;


	if(request == NULL)
	{
		return -1;
	}

	curl = curl_easy_init();

	if(request->port == 0)
	{
		port = 80;
	}
	else
	{
		port = request->port;
	}

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		if(request->query != NULL && request->query[0] != '\0'){
			snprintf(url, URL_PATH_LEN, "%s://%s:%d%s?%s", protocol_list[request->ssl], request->host, port, request->path,request->query);
		}else{
			snprintf(url, URL_PATH_LEN, "%s://%s:%d%s", protocol_list[request->ssl], request->host, port, request->path);
		}

#if HTTP_API_DEBUG
		printf("<%s> URL: %s\n", __FUNCTION__, url);
#endif

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		if(request->user_agent != NULL)
			curl_easy_setopt(curl, CURLOPT_USERAGENT, request->user_agent);
		else
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "ITX-IPX");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->time_out); 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

		//header list
		hs = curl_slist_append(hs, "Content-Type: application/json");
		if(hs != NULL){
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
		}else{
			printf("<%s> header list init append error\n", __FUNCTION__);
		}

		if(request->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		if(request->user != NULL && request->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, request->user);
			strcat(idpwd, ":");
			strcat(idpwd, request->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		res = curl_easy_perform(curl);

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &request->status);

		if(hs != NULL) 
			curl_slist_free_all(hs);	
		curl_easy_cleanup(curl);
#if HTTP_API_DEBUG
		if(chunk != NULL)
		{
			printf("<%s> >>> RESPONE(size:%d)\n", __FUNCTION__, chunk->size);
			puts(chunk->memory);
			printf("<%s> <<< RESPONE\n", __FUNCTION__);
		}
#endif
	}
	else
	{
		printf("curl_easy_init fail\n");
	}

	return res;
}

static int http_curl_get_request(struct chunk *chunk, const struct http_request_info *request)
{
	CURL *curl = NULL;
	CURLcode res = 0;;
	int port = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = { "http", "https" };


	if(request == NULL)
	{
		return -1;
	}

	curl = curl_easy_init();

	if(request->port == 0)
	{
		port = 80;
	}
	else
	{
		port = request->port;
	}

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		snprintf(url, URL_PATH_LEN, "%s://%s:%d%s?%s", protocol_list[request->ssl], request->host, port, request->path,request->query);

#if HTTP_API_DEBUG
		printf("<%s> URL: %s\n", __FUNCTION__, url);
#endif

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		if(request->user_agent != NULL)
			curl_easy_setopt(curl, CURLOPT_USERAGENT, request->user_agent);
		else
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "ITX-IPX");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->time_out); 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

		if(request->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		if(request->user != NULL && request->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, request->user);
			strcat(idpwd, ":");
			strcat(idpwd, request->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		res = curl_easy_perform(curl);

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &request->status);

		curl_easy_cleanup(curl);
#if HTTP_API_DEBUG
		if(chunk != NULL)
		{
			printf("<%s> >>> RESPONE(size:%d)\n", __FUNCTION__, chunk->size);
			puts(chunk->memory);
			printf("<%s> <<< RESPONE\n", __FUNCTION__);
		}
#endif
	}
	else
	{
		printf("curl_easy_init fail\n");
	}

	return res;
}

static int http_request(const struct http_request_info *req, char *buffer, const size_t buffer_size, int (*request)(struct chunk *, const struct http_request_info *))
{
	int rtn = -1;
	struct chunk chunk;
	unsigned int copy_size = 0;
#if REQUEST_TIME_MESURE
	struct timespec start_t, end_t;
#endif

	memset(&chunk, 0x00, sizeof(chunk));

	http_chunk_init(&chunk);

#if REQUEST_TIME_MESURE
	clock_gettime(CLOCK_MONOTONIC, &start_t);
#endif

	rtn = request(&chunk, req);

#if REQUEST_TIME_MESURE
	clock_gettime(CLOCK_MONOTONIC, &end_t);
#endif

#if REQUEST_TIME_MESURE
	struct timespec res;
	char ipstr[16] = {0};
	timespec_diff(&start_t, &end_t, &res);
	printf("<%s> (%s) RTN(CURL:%d HTTP:%d) TIME DIFF (%lu.%09lu) Query(%s)\n", __FUNCTION__, req->host, rtn, req->status, res.tv_sec, res.tv_nsec, req->query);
#endif

	if(buffer_size > chunk.size)
		copy_size = chunk.size;
	else
		copy_size = buffer_size - 1;

	if(buffer != NULL && buffer_size > 0)
	{
		memset(buffer, 0x00, buffer_size);
		memcpy(buffer, chunk.memory, copy_size);
	}

	http_chunk_release(&chunk);

	rtn = http_error_check(req->host, req->port, rtn, req->status);

	return rtn;
}

static int http_new_request(const struct http_request_info *req, icm_response *icm_res, int (*request)(struct chunk *, const struct http_request_info *))
{
	int rtn = -1;
	struct chunk chunk;
#if REQUEST_TIME_MESURE
	struct timespec start_t, end_t;
#endif

	memset(&chunk, 0x00, sizeof(chunk));

	http_chunk_init(&chunk);

#if REQUEST_TIME_MESURE
	clock_gettime(CLOCK_MONOTONIC, &start_t);
#endif

	rtn = request(&chunk, req);

#if REQUEST_TIME_MESURE
	clock_gettime(CLOCK_MONOTONIC, &end_t);
#endif

#if REQUEST_TIME_MESURE
	struct timespec res;
	char ipstr[16] = {0};
	timespec_diff(&start_t, &end_t, &res);
	//printf("<%s> (%s) RTN(CURL:%d HTTP:%d) TIME DIFF (%lu.%09lu) Query(%s)\n", __FUNCTION__, req->host, rtn, req->status, res.tv_sec, res.tv_nsec, req->query);
#endif

	if(icm_res != NULL)
	{
		icm_res->msg = chunk.memory;
		icm_res->size = chunk.size;
		chunk.memory = NULL;
		chunk.size = 0;
	}
	else
	{
		http_chunk_release(&chunk);
	}

	rtn = http_error_check(req->host, req->port, rtn, req->status);

	return rtn;
}

static int http_file_upload_from_buffer(const char * const url, const char * const form_name, const char * const file_name, const char * const file_buffer, const int file_size)
{
	CURL *curl;
	CURLcode res;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	struct curl_slist *headerlist=NULL;

	static const char buf[] = "Expect:";

	initialize_global_curl();

	/* Fill in the file upload field */
	curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, form_name,
			CURLFORM_BUFFER, file_name,
			CURLFORM_BUFFERPTR, file_buffer,
			CURLFORM_BUFFERLENGTH, file_size,
			CURLFORM_END);

	curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);

	if(curl)
	{   
		/* what URL that receives this POST */
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the formpost chain */
		curl_formfree(formpost);

		/* free slist */
		curl_slist_free_all(headerlist);
	}   
	return 0;
}

static int http_curl_upload_from_buffer(struct chunk *chunk, const struct http_request_info *request, const struct http_upload_info *upload)
{
	CURL *curl = NULL;
	CURLcode res = 0;;
	int port = 0;
	char url[URL_PATH_LEN] = {0,};
	char protocol_list[][8] = { "http", "https" };
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	int idx = 0;


	if(request == NULL || upload == NULL)
	{
		return -1;
	}

	curl_formadd(&formpost,
				 &lastptr,
				 CURLFORM_COPYNAME,     upload->form_name, 
				 CURLFORM_BUFFER,       upload->file_name, 
				 CURLFORM_BUFFERPTR,    upload->file_buffer,
				 CURLFORM_BUFFERLENGTH, upload->file_buffer_size,
				 CURLFORM_END);

	if(formpost == NULL)
	{
		ICM_UTILS_PRINT("curl_formadd fail\n");
		return -1;
	}

	for(idx = 0; idx < upload->head_cnt; idx++)
	{
		headerlist = curl_slist_append(headerlist, (const char *)(upload->http_head));
	}

	curl = curl_easy_init();

	if(request->port == 0)
	{
		port = 80;
	}
	else
	{
		port = request->port;
	}

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		snprintf(url, URL_PATH_LEN, "%s://%s:%d%s", protocol_list[request->ssl], request->host, port, request->path);

#if HTTP_API_DEBUG
		printf("<%s> URL: %s\n", __FUNCTION__, url);
#endif

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		if(request->user_agent != NULL)
			curl_easy_setopt(curl, CURLOPT_USERAGENT, request->user_agent);
		else
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "ITX-IPX");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_chunk_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->time_out); 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

		if(request->ssl == 1)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		if(request->user != NULL && request->passwd != NULL)
		{
			char idpwd[128] = {0,};
			strcat(idpwd, request->user);
			strcat(idpwd, ":");
			strcat(idpwd, request->passwd);
			curl_easy_setopt(curl, CURLOPT_USERPWD, idpwd);
		}

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		res = curl_easy_perform(curl);

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &request->status);

		curl_easy_cleanup(curl);
#if HTTP_API_DEBUG
		if(chunk != NULL)
		{
			printf("<%s> >>> RESPONE(size:%d)\n", __FUNCTION__, chunk->size);
			puts(chunk->memory);
			printf("<%s> <<< RESPONE\n", __FUNCTION__);
		}
#endif
	}
	else
	{
		printf("curl_easy_init fail\n");
	}

	if(formpost != NULL)
	{
		curl_formfree(formpost);
	}

	if(headerlist != NULL)
	{
		curl_slist_free_all(headerlist);
	}

	return res;
}

static int http_upload_from_buffer(const struct http_request_info *req, const struct http_upload_info *up, icm_response *icm_res)
{
	int rtn = -1;
	struct chunk chunk;
#if REQUEST_TIME_MESURE
	struct timespec start_t, end_t;
#endif

	memset(&chunk, 0x00, sizeof(chunk));

	http_chunk_init(&chunk);

#if REQUEST_TIME_MESURE
	clock_gettime(CLOCK_MONOTONIC, &start_t);
#endif

	rtn = http_curl_upload_from_buffer(&chunk, req, up);

#if REQUEST_TIME_MESURE
	clock_gettime(CLOCK_MONOTONIC, &end_t);
#endif

#if REQUEST_TIME_MESURE
	struct timespec res;
	timespec_diff(&start_t, &end_t, &res);
	printf("<%s> (%s) RTN(CURL:%d HTTP:%d) TIME DIFF (%lu.%09lu) Query(%s)\n", __FUNCTION__, req->host, rtn, req->status, res.tv_sec, res.tv_nsec, req->query);
#endif

	if(icm_res != NULL)
	{
		icm_res->msg = chunk.memory;
		icm_res->size = chunk.size;
		chunk.memory = NULL;
		chunk.size = 0;
	}
	else
	{
		http_chunk_release(&chunk);
	}

	rtn = http_error_check(req->host, req->port, rtn, req->status);

	return rtn;
}


/* EXTERN FUNCTION */

int icm_http_ch_init(icm_http *ctx, const int ch)
{
	int rtn = 0;
	memset(ctx, 0x00, sizeof(icm_http));

	nf_ipcam_get_ipstr(ch, ctx->ipstr);
	nf_ipcam_get_username(ch, ctx->username);
	nf_ipcam_get_password(ch, ctx->password);
	ctx->auth_type = 0;
	ctx->port = nf_ipcam_get_http_port(ch);
	ctx->ssl = nf_ipcam_is_ssl(ch);
	ctx->timeout = 0;

	return rtn;
}

int icm_http_ch_init_tout(icm_http *ctx, const int ch, int timeout)
{
	int rtn = 0;
	memset(ctx, 0x00, sizeof(icm_http));

	nf_ipcam_get_ipstr(ch, ctx->ipstr);
	nf_ipcam_get_username(ch, ctx->username);
	nf_ipcam_get_password(ch, ctx->password);
	ctx->auth_type = 0;
	ctx->port = nf_ipcam_get_http_port(ch);
	ctx->ssl = nf_ipcam_is_ssl(ch);
	ctx->timeout = timeout;

	return rtn;
}

int icm_http_ip_init(icm_http *ctx, const unsigned int ip, const unsigned short port, const char *username, const char *password, const unsigned int ssl)
{
	int rtn = 0;
	char ipstr[16] = {0,};

	memset(ctx, 0x00, sizeof(icm_http));

	snprintf(ipstr, 16, "%d.%d.%d.%d", (ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);

	ctx->port = port;
	ctx->ssl = ssl;
	ctx->auth_type = 0;
	strncpy(ctx->ipstr, ipstr, 16);
	strncpy(ctx->username, username, 64);
	strncpy(ctx->password, password, 64);

	ctx->timeout = 0;

	return rtn;
}

int icm_http_get_request(icm_http *ctx, const char *path, const char *query, char *buffer, const size_t buffer_size, const char* user_agent)
{
	int rtn = 0;
	struct http_request_info req;

	memset(&req, 0x00, sizeof(req));
	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	req.time_out = HTTP_TIME_OUT_SEC;
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;
	req.query = query;
	req.user_agent = user_agent;

	rtn = http_request(&req, buffer, buffer_size, http_curl_get_request);

	ctx->status = req.status;

	return rtn;
}

int icm_http_post_request(icm_http *ctx, const char *path, const char *query, char *buffer, const size_t buffer_size)
{
	int rtn = 0;
	struct http_request_info req;

	memset(&req, 0x00, sizeof(req));

	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	req.time_out = HTTP_TIME_OUT_SEC;
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;
	req.query = query;

	rtn = http_request(&req, buffer, buffer_size, http_curl_post_request);

	ctx->status = req.status;

	return rtn;
}

int icm_http_file_upload_from_buffer(const char * const url, const char * const form_name, const char * const file_name, const char * const file_buffer, const int file_size)
{
	int rtn = 0;

	rtn = http_file_upload_from_buffer(url, form_name, file_name, file_buffer, file_size);

	return rtn;
}

int icm_http_new_get_request_json(icm_http *ctx, const char *path, const char *query, icm_response *res)
{
	int rtn = 0;
	struct http_request_info req;

	if(ctx == NULL || res == NULL)
		return -1;

	memset(&req, 0x00, sizeof(req));
	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	if(ctx->timeout > 0)
	{
		req.time_out = ctx->timeout;
	}
	else
	{
		req.time_out = HTTP_TIME_OUT_SEC;
	}
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;
	req.query = query;

	rtn = http_new_request(&req, res, http_curl_get_json_request);

	ctx->status = req.status;
	res->status = req.status;

	return rtn;
}

int icm_http_new_get_request(icm_http *ctx, const char *path, const char *query, icm_response *res)
{
	int rtn = 0;
	struct http_request_info req;

	if(ctx == NULL || res == NULL)
		return -1;

	memset(&req, 0x00, sizeof(req));
	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	if(ctx->timeout > 0)
	{
		req.time_out = ctx->timeout;
	}
	else
	{
		req.time_out = HTTP_TIME_OUT_SEC;
	}
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;
	req.query = query;

	rtn = http_new_request(&req, res, http_curl_get_request);

	ctx->status = req.status;
	res->status = req.status;

	return rtn;
}


int icm_http_new_post_request_json(icm_http *ctx, const char *path, json_t *json, icm_response *res)
{
	int rtn = 0;
	struct http_request_info req;

	if(ctx == NULL || res == NULL)
		return -1;

	memset(&req, 0x00, sizeof(req));
	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	if(ctx->timeout > 0)
	{
		req.time_out = ctx->timeout;
	}
	else
	{
		req.time_out = HTTP_TIME_OUT_SEC;
	}
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;

	req.query = json_dumps(json, JSON_ENCODE_ANY);

	rtn = http_new_request(&req, res, http_curl_post_json_request);

	ctx->status = req.status;
	res->status = req.status;

	if(req.query != NULL){
		IPCAM_DBG(MINOR, "req.query[%s]\n", req.query);
		free(req.query);
	}
	return rtn;
}

int icm_http_new_post_request(icm_http *ctx, const char *path, const char *query, icm_response *res)
{
	int rtn = 0;
	struct http_request_info req;

	if(ctx == NULL || res == NULL)
		return -1;

	memset(&req, 0x00, sizeof(req));

	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	if(ctx->timeout > 0)
	{
		req.time_out = ctx->timeout;
	}
	else
	{
		req.time_out = HTTP_TIME_OUT_SEC;
	}
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;
	req.query = query;

	rtn = http_new_request(&req, res, http_curl_post_request);

	ctx->status = req.status;
	res->status = req.status;

	return rtn;
}

void icm_http_response_free(icm_response *res)
{
	if(res != NULL)
	{
		if(res->msg != NULL)
		{
		    free(res->msg);
		    res->msg  = NULL;
		    res->size = 0;
		}
	}
}

int icm_http_upload_from_buffer(icm_http *ctx, const char *path, const char *query, icm_http_upload *upload, icm_response *res)
{
	int rtn = 0;
	int idx = 0;
	struct http_request_info req;
	struct http_upload_info up;

	if(ctx == NULL || upload == NULL)
		return -1;

	memset(&req, 0x00, sizeof(req));

	req.host = ctx->ipstr;
	req.port = ctx->port;
	req.ssl = ctx->ssl;
	if(ctx->timeout > 0)
	{
		req.time_out = ctx->timeout;
	}
	else
	{
		req.time_out = HTTP_TIME_OUT_SEC;
	}
	req.user = ctx->username;
	req.passwd = ctx->password;
	req.path = path;
	req.query = query;

	memset(&up, 0x00, sizeof(up));
	up.head_cnt = upload->head_cnt;
	for(idx = 0; idx < up.head_cnt; idx++)
	{
		up.http_head[idx] = upload->http_head[idx];
	}

	up.form_name = upload->form_name;
	up.file_name = upload->file_name;
	up.file_buffer = upload->file_buffer;
	up.file_buffer_size = upload->file_buffer_size;

	rtn = http_upload_from_buffer(&req, &up, res);

	ctx->status = req.status;

	return rtn;
}

int icm_http_add_header(icm_http_upload *upload, const char *header_str)
{
	int rtn = -1;

	if(upload == NULL || header_str == NULL)
		return -1;

	if(upload->head_cnt >= ADD_HEADER_MAX)
	{
		ICM_UTILS_PRINT("upload have max header\n");
		goto ends_label;
	}
	else
	{
		snprintf(upload->http_head[upload->head_cnt], UPLOAD_STR_LEN, header_str);
		upload->head_cnt++;
	}

	rtn = ICM_RTN_OK;

ends_label:

	return rtn;
}

/* OTHER API */

int icm_sock_timeout_set(int sock, struct timeval _tv)
{
	int ret  = -1;
	struct timeval tv;
	tv = _tv;
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if(ret < 0)
	{
		goto end_label;
	}

	tv = _tv;
	ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	if(ret < 0)
	{
		goto end_label;
	}
end_label:
	return ret;
}

// SUCESSES: 0
// FAIL: 1
int icm_find_http_form_data(const char *msg, const char *key, char *value)
{
#define ENTRY_MAX (3)
	int rtn = 1;
	int i = 0;
	char *start = NULL, *end = NULL;
	char *p[ENTRY_MAX];

	/* find Model Name */
	start = strstr(msg, key);
	if (start != NULL)
	{

		start += strlen(key);
		p[0] = strstr(start, "&");
		p[1] = strstr(start, "\r");
		p[2] = strstr(start, "\n");

		end = NULL;
		for(i = 0; i < ENTRY_MAX; i++)
		{
			if(p[i] != NULL)
			{
				if(end > p[i] || end == NULL)
				{
					end = p[i];
				}
			}
		}

		if(end != NULL)
		{
			unsigned int size = (unsigned int)(end - start);
			strncpy(value, start, size);
			rtn = 0;
		}
	}

end_label:
	return rtn;
}


/* ================================================================================ */
// STRING

void icm_str_array_free(icm_str_array str_array, int array_size)
{
	int	 array_idx   = 0;

	if(str_array != NULL)
	{
		for(array_idx = 0; array_idx < array_size; array_idx++)
		{
		    if(str_array[array_idx] != NULL)
		    {
		        free(str_array[array_idx]);
		    }
		}
		free(str_array);
	}
}

icm_str_array icm_str_split(const char *string, const char *delimiter, int max_tokens)
{
	int	     cur_token   = 0;
	int	     rtn_code    = ICM_STR_RTN_SUCC;
	int	     array_size  = 0;
	int	     str_len     = 0;
	int	     delim_len   = 0;
	int	     token_len   = 0;
	char	    **out       = NULL;
	char	    *str_pos    = NULL;
	const char  *str_tmp	= NULL;

	if(string == NULL || delimiter == NULL || max_tokens < 1)
	{
		ICM_UTILS_PRINT("input param error\n");
		return NULL;
	}

	array_size = max_tokens;

	out = (char **)calloc((size_t)array_size, sizeof(char *));
	if(out == NULL)
	{
		ICM_UTILS_PRINT("memory alloc fail\n");
		rtn_code = ICM_STR_RTN_FAIL;
		goto end_label;
	}

	str_len   = (int)strlen(string);
	delim_len = (int)strlen(delimiter);

	str_tmp = string;
	str_pos = strstr(str_tmp, delimiter);
	if(str_pos == NULL)
	{
		// No Split
		if(str_len >= 0)
		{
			out[0] = (char *)malloc((size_t)str_len+1);
			if(out[0] == NULL)
			{
				ICM_UTILS_PRINT("memory alloc fail\n");
				rtn_code = ICM_STR_RTN_FAIL;
				goto end_label;
			}
			snprintf(out[0], (size_t)str_len+1, "%s", string);
			cur_token = 1;
		}
		rtn_code = ICM_STR_RTN_SUCC;
		goto end_label;
	}
	else
	{
		// Yes Split
		str_tmp   = string;
		cur_token = 0;
		while(((str_pos = strstr(str_tmp, delimiter)) != NULL) && cur_token < array_size-1)
		{
		    token_len = (str_pos - str_tmp);
			if(token_len >= 0)
			{
				out[cur_token] = (char *)malloc((size_t)token_len+1);
				if(out[cur_token] == NULL)
				{
					ICM_UTILS_PRINT("memory alloc fail\n");
					rtn_code = ICM_STR_RTN_FAIL;
					goto end_label;
				}
				snprintf(out[cur_token], (size_t)token_len+1, "%s", str_tmp);
			}

		    str_tmp = str_pos + delim_len;
		    cur_token++;
		}

		// last token
		token_len = (string + str_len) - str_tmp;
		if(token_len >= 0)
		{
		    out[cur_token] = (char *)malloc((size_t)token_len+1);
		    if(out[cur_token] == NULL)
		    {
				ICM_UTILS_PRINT("memory alloc fail\n");
		        rtn_code = ICM_STR_RTN_FAIL;
		        goto end_label;
		    }
		    snprintf(out[cur_token], (size_t)token_len+1, "%s", str_tmp);
		    cur_token++;
		}
	}

	rtn_code = ICM_STR_RTN_SUCC;

end_label:

	if(rtn_code == ICM_STR_RTN_SUCC)
	{
		// dummy token set
		if(cur_token < array_size)
		{
			int idx = 0;
			for(idx = cur_token; idx < array_size; idx++)
			{
				out[idx] = (char *)malloc(1);
				if(out[idx] == NULL)
				{
					ICM_UTILS_PRINT("memory alloc fail\n");
					icm_str_array_free(out, array_size);
					return NULL;
				}
				out[idx][0] = '\0';
			}
		}
	}
	else
	{
		if(out != NULL)
		{
			icm_str_array_free(out, array_size);
			out = NULL;
		}
	}

	return out;
}

int icm_str_trim(char *str, size_t str_len)
{
	int rtn = ICM_STR_RTN_FAIL;
	char *l_ptr = NULL; // left
	char *r_ptr = NULL; // right
	char *s_ptr = NULL; // start

	if(str == NULL || str_len == 0)
	{
		ICM_UTILS_PRINT("input param error\n");
	    goto ends_label;
	}

	for(l_ptr = str; *l_ptr != 0 && isspace(*l_ptr); l_ptr++)
	    ;

	for(r_ptr = str+str_len-1; r_ptr != l_ptr && isspace(*r_ptr); r_ptr--)
	    ;

	*(++r_ptr) = 0;

	for(s_ptr = str; l_ptr < r_ptr; s_ptr++, l_ptr++)
	{
	    *s_ptr = *l_ptr;
	}
	*s_ptr = 0;

	rtn = ICM_STR_RTN_SUCC;

ends_label:

	return rtn;
}

/* ================================================================================ */
// ZLIB

#define ZLIB_CHUNK	(1024)

void icm_zlib_stream_free(icm_zlib_stream *strm)
{
	if(strm != NULL)
	{
		if(strm->stream != NULL)
		{
			free(strm->stream);
			strm->stream = NULL;
			strm->memLen = 0;
			strm->useLen = 0;
		}
	}
}

int icm_zlib_deflate_to_gzip(const char *src, size_t srcLen, icm_zlib_stream *out, icm_gzip_header *head)
{
	int rtn = ICM_ZLIB_RTN_FAIL;
	int res = Z_ERRNO;
	size_t flush_cnt = 0;
	uLong destLen = 0;
	Bytef *dest = NULL;
	z_stream z_strm = { 0 };
	gz_header gz_head = { 0 };

	if(src == NULL || out == NULL)
	{
		ICM_UTILS_PRINT("input param error\n");
		goto ends_label;
	}

	memset(&z_strm, 0x00, sizeof(z_strm));
	res = deflateInit2(&z_strm, 
			           Z_DEFAULT_COMPRESSION, 
					   Z_DEFLATED, 
					   MAX_WBITS+16, // 16 is add GZIP windowBits
					   MAX_MEM_LEVEL, 
					   Z_DEFAULT_STRATEGY);
	if(res != Z_OK)
	{
		ICM_UTILS_PRINT("deflateInit2 fail(res=%d)\n", res);
		goto ends_label;
	}
	
	if(head != NULL)
	{
		memset(&gz_head, 0x00, sizeof(gz_head));
		gz_head.text  = head->file_type;
		gz_head.time  = head->make_time;
		gz_head.os    = head->os_code;
		gz_head.name  = (Bytef *)head->name;

		if(head->file_type == ICM_GZIP_BIN)
		{
			z_strm.data_type = Z_BINARY;
		}
		else if(head->file_type == ICM_GZIP_TEXT)
		{
			z_strm.data_type = Z_TEXT;
		}
		else
		{
			ICM_UTILS_PRINT("Unknown file type(%d)\n", head->file_type);
			rtn = ICM_ZLIB_HEAD_ERR;
			goto ends_label;
		}

		res = deflateSetHeader(&z_strm, &gz_head);
		if(res != Z_OK)
		{
			ICM_UTILS_PRINT("deflateSetHeader fail(res=%d)\n", res);
			goto ends_label;
		}
	}

	z_strm.next_in = (Bytef *)src;

	destLen = deflateBound(&z_strm, srcLen);

	if((dest = (Bytef *)calloc(1, sizeof(Bytef)*destLen+1)) == NULL)
	{
		ICM_UTILS_PRINT("memory alloc fail\n");
		goto ends_label;
	}

	z_strm.next_out = dest;

	while(1)
	{
		if(z_strm.total_in >= srcLen || z_strm.total_out >= destLen)
		{
			break;
		}

		if((z_strm.total_in - srcLen) >= ZLIB_CHUNK)
		{
			z_strm.avail_in = srcLen - z_strm.total_in;
		}
		else
		{
			z_strm.avail_in = z_strm.total_in - srcLen;
		}

		z_strm.avail_out = destLen - z_strm.total_out;

		res = deflate(&z_strm, Z_NO_FLUSH);
		if(res != Z_OK)
		{
			ICM_UTILS_PRINT("deflate fail(res=%d)\n", res);
			goto ends_label;
		}
	}

	while(1)
	{
		z_strm.avail_out = destLen - z_strm.total_out;
		res = deflate(&z_strm, Z_FINISH);
		if(res == Z_STREAM_END)
		{
			break;
		}

		if(flush_cnt >= ICM_ZLIB_FLUSH_MAX)
		{
			ICM_UTILS_PRINT("flush count over(%d>=%d)\n", flush_cnt, ICM_ZLIB_FLUSH_MAX);
			goto ends_label;
		}
		else
		{
			flush_cnt++;
		}
	}

	if(out->stream == NULL)
	{
		out->stream = (char *)dest;
		out->memLen = destLen+1;
		out->useLen = z_strm.total_out;
		dest = NULL;
		rtn = ICM_ZLIB_RTN_SUCC;
	}
	else
	{
		rtn = ICM_ZLIB_OUT_ERR;
	}

ends_label:

	if(dest != NULL)
	{
		free(dest);
	}

	deflateEnd(&z_strm);

	return rtn;
}

/* ================================================================================ */

// API TEST CODE
//#define IPCAM_HTTP_API_TEST_MAIN

#ifdef IPCAM_HTTP_API_TEST_MAIN

// Set Runtime Info
static int runtime_setup()
{
	mtable* runtime = get_runtime();


	////////////////////////////////////////////////////////
	strncpy(runtime[0].username, "ADMIN", 64);
	strncpy(runtime[0].password, "~~qqww11", 64);

	inet_aton("172.16.0.67", (struct in_addr *)&runtime[0].sys.ipaddr);

	runtime[0].sys.http_port = 80;
	runtime[0].sys.use_ssl = 0;
	runtime[0].sys.macaddr[0] = 0x00;
	runtime[0].sys.macaddr[1] = 0x11;
	runtime[0].sys.macaddr[2] = 0x5f;
	runtime[0].sys.macaddr[3] = 0xb0;
	runtime[0].sys.macaddr[4] = 0xc2;
	runtime[0].sys.macaddr[5] = 0xa0;

	////////////////////////////////////////////////////////
	strncpy(runtime[1].username, "ADMIN", 64);
	strncpy(runtime[1].password, "~~qqww11", 64);
	runtime[1].sys.http_port = 8082;
	runtime[1].sys.use_ssl = 1;
	inet_aton("172.16.0.54", (struct in_addr *)&runtime[1].sys.ipaddr);

	return 0;
}

// API Call
static void api_request(void)
{
	// ITX API
	const char *path = "/cgi-bin/action.fcgi";
	const char *query1 = "api=get_setup.system.info";
	const char *query2 = "api=get_setup.video.af.capability";
	const char *query3 = "api=get_setup.video.camera";
	const char *query4 = "api=get_setup.live.roi";
	const char *query5 = "api=get_setup.event.motion";
	const char *query6 = "api=get_setup.live.ctrl.privacymask";

	const unsigned int buffer_size = 4096;
	char buffer[buffer_size];
	int rtn = -1;

	icm_http http;
	icm_http_ch_init(&http, 0);

	memset(buffer, 0x00, buffer_size);

	rtn = icm_http_get_request(&http, path, query2, buffer, buffer_size, NULL);
	icm_http_error_check(__FUNCTION__, __LINE__, 0, rtn, http.status);

	icm_http_ch_init(&http, 1);
	rtn = icm_http_get_request(&http, path, query2, buffer, buffer_size, NULL);
	icm_http_error_check(__FUNCTION__, __LINE__, 0, rtn, http.status);
}

#include <pthread.h>
#include <stdlib.h>
extern int nf_openssl_thread_setup(void);
extern int nf_openssl_thread_cleanup(void);
int main(int argc, char **argv)
{
    initialize_global_curl();

	// OpenSSL Thread Safe Set
	// https://curl.haxx.se/libcurl/c/threadsafe.html
	// http://openssl.6102.n7.nabble.com/sha-block-data-order-Crash-td12079.html
	nf_openssl_thread_setup();
	runtime_setup();


	while(1)
	{
		api_request();
	}

	// OpenSSL Thread Safe Cleanup
	nf_openssl_thread_cleanup();


	return 0;
}


#endif
