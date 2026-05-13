#ifndef CURL_MANAGER_H
#define CURL_MANAGER_H

#include <stddef.h>
#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

void initialize_global_curl(void);
void cleanup_curl(void);

int debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr);

#ifdef __cplusplus
}
#endif

#endif /* CURL_MANAGER_H */
