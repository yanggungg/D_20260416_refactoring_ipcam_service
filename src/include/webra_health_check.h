#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

typedef struct _Response {
  char *body;
  size_t body_size;
} Response;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
char *call_http_request(const char *request_url, const char *user_id, const char *password);

