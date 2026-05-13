#ifndef __NF_IPCAM_DRIVER_UTILS_C__
#define __NF_IPCAM_DRIVER_UTILS_C__

#define ICM_UTILS_PRINT(fmt,...) \
{ \
	if(1) \
	{ \
		fprintf(stdout, "%s(%d) : " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		fflush(stdout); \
	} \
}\

// HTTP REQUEST
#define CURL_CODE(code)	(code&0x0000FFFF)
#define HTTP_CODE(code)	(code>>16 & 0x0000FFFF)
#define ADD_HEADER_MAX	(10)
#define	UPLOAD_STR_LEN	(256)

typedef enum  {
	ICM_RTN_OK,
	ICM_RTN_UNKNOWN,
	ICM_RTN_NOT_FOUND,
	ICM_RTN_METHOD_NOT_ALLOWED,
	ICM_RTN_CONNECT_FAIL,
	ICM_RTN_SSL_CONNECT_FAIL,
	ICM_RTN_TIMEOUT,
	ICM_RTN_NOTHING,
	ICM_RTN_AUTH_FAIL,
	ICM_RTN_SERVER_ERR,
}ICM_RTN_TYPE;

/* FUNCTION */
struct _icm_http
{
	char username[64];
	char password[64];
	char ipstr[16];
	unsigned short port;
	unsigned int ssl;
	int auth_type;
	int status;
	int timeout;
};
typedef struct _icm_http icm_http;

typedef struct
{
	// for http head add
	char http_head[ADD_HEADER_MAX][UPLOAD_STR_LEN];
	int head_cnt;

	// for upload
	char form_name[256];
	char file_name[256];
	char *file_buffer;
	size_t file_buffer_size;
} icm_http_upload;

struct _icm_response
{
	char *msg;
	size_t size;
	int status;
};
typedef struct _icm_response icm_response;

extern int icm_http_ch_init(icm_http *ctx, const int ch);
extern int icm_http_ch_init_tout(icm_http *ctx, const int ch, int timeout);
extern int icm_http_ip_init(icm_http *ctx, const unsigned int ip, const unsigned short port, const char *username, const char *password, const unsigned int ssl);

extern int icm_http_get_request(icm_http *ctx, const char *path, const char *query, char *buffer, const size_t buffer_size, const char* user_agent);
extern int icm_http_post_request(icm_http *ctx, const char *path, const char *query, char *buffer, const size_t buffer_size);


extern int icm_http_new_get_request(icm_http *ctx, const char *path, const char *query, icm_response *res);
extern int icm_http_new_post_request(icm_http *ctx, const char *path, const char *query, icm_response *res);
extern void icm_http_response_free(icm_response *res);
extern int icm_http_upload_from_buffer(icm_http *ctx, const char *path, const char *query, icm_http_upload *upload, icm_response *res);
extern int icm_http_add_header(icm_http_upload *upload, const char *header_str);


extern int icm_sock_timeout_set(int sock, struct timeval _tv);
extern int icm_find_http_form_data(const char *msg, const char *key, char *value);


// STRING
typedef enum
{
	ICM_STR_RTN_SUCC = 0,
	ICM_STR_RTN_FAIL
}ICM_STR_RTN_TYPE;

typedef char** icm_str_array;

extern void icm_str_array_free(icm_str_array str_array, int array_size);
extern icm_str_array icm_str_split(const char *string, const char *delimiter, int max_tokens);
extern int icm_str_trim(char *str, size_t str_len); // change str param

// ZLIB
typedef enum
{
	ICM_ZLIB_RTN_SUCC = 0,
	ICM_ZLIB_RTN_FAIL,
	ICM_ZLIB_HEAD_ERR,
	ICM_ZLIB_OUT_ERR
}ICM_ZLIB_RTN_TYPE;

/* deflate try max count */
#define	ICM_ZLIB_FLUSH_MAX	(100000)

/* gzip org file type */
#define ICM_GZIP_BIN	(0)
#define	ICM_GZIP_TEXT	(1)

/* gzip make os code */
#define	ICM_OS_UNIX		(3)

typedef struct
{
	size_t memLen;
	size_t useLen;
	char *stream;
} icm_zlib_stream;

typedef struct
{
	int file_type;		// 0:binary, 1:text, other:fail
	time_t make_time;	// last modify time
	int os_code;		// gzip os code ( 3:UNIX )
	char name[256];		// real file name
} icm_gzip_header;

extern void icm_zlib_stream_free(icm_zlib_stream *strm);
extern int icm_zlib_deflate_to_gzip(const char *src, size_t srcLen, icm_zlib_stream *out, icm_gzip_header *head);

#endif // __NF_IPCAM_DRIVER_UTILS_C__


