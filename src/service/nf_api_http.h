#ifndef __NF_API_HTTP_H__
#define __NF_API_HTTP_H__

#include <curl/curl.h>

#include <openssl/engine.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>


struct chunk
{
	char *memory;
	size_t size;
};

typedef struct _HTTP_CTX
{
	/*
	 * request
	 */
	char *host;
	unsigned short port;
	unsigned int ssl;
	int time_out;
	int conn_time_out;

	char *user;
	char *passwd;

	char *method;
	char *path;
	char *query;	//get data
	char *buffer;		//post data
	size_t buffer_size;

    struct curl_slist *header_list;


	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;

	/*
	 * response
	 */
    long auth_type;
    /* auth_type
    #define CURLAUTH_NONE         ((unsigned long)0)
    #define CURLAUTH_BASIC        (((unsigned long)1)<<0)
    #define CURLAUTH_DIGEST       (((unsigned long)1)<<1)
    #define CURLAUTH_NEGOTIATE    (((unsigned long)1)<<2)
    */

	int status;
	char *http_header_date;

	struct chunk res_header;
	struct chunk res_data;

}HTTP_CTX;

typedef enum{
	HTTP_SET_HOST, 
	HTTP_SET_PORT, 
	HTTP_SET_SSL, 
	HTTP_SET_TIME_OUT, 
	HTTP_SET_CONN_TIME_OUT, 
	HTTP_SET_METHOD, 
	HTTP_SET_USER, 
	HTTP_SET_AUTH_TYPE, 
	HTTP_SET_PASSWD, 
	HTTP_SET_PATH, 
	HTTP_SET_QUERY, 
	HTTP_ADD_QUERY, 
	HTTP_ADD_HEADER, 
	HTTP_SET_POST,
	HTTP_ADD_MULTIPART
}HTTP_SET_OPTION;

typedef struct _Token_iterator
{
	//입력값 설정
	char *token;
	char *buffer;

	//계산할때 사용
	char *current;
	char *next;

	//연산에 필요한 값, 최초 init시 설정
	char *end;
	int token_length;
}Token_iterator;



int http_init(HTTP_CTX *http_ctx);
int http_request(HTTP_CTX *http_ctx);
int http_data_set(HTTP_CTX *http_ctx, HTTP_SET_OPTION option, ...);
const char *http_get_res(HTTP_CTX *http_ctx);
void http_release(HTTP_CTX *http_ctx);

//debug print
void http_req_data_print(HTTP_CTX *ctx);
void http_res_data_print(HTTP_CTX *ctx);

void release_token(Token_iterator *iter);
const char *tok_get_next(Token_iterator *iter);
int is_token_char(char c, char *token, int token_lenth);
const char *tok_iterator_init(Token_iterator *iter, const char *str, const char *token);

//hmac
char *http_get_encrypt_hmac(char *key, const unsigned char *data, const EVP_MD *type);

int aibox_http_default_setting_multipart(HTTP_CTX *ctx, unsigned int aibox_ip);
int aicam_http_default_setting(HTTP_CTX *ctx, int ch);
int aibox_http_default_setting(HTTP_CTX *ctx, unsigned int aibox_ip);

#endif	//__NF_API_HTTP_H__

