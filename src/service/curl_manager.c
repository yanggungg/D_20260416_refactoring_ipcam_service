#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "curl_manager.h"
#include "admin_server_api.h"

#define TARGET_PATH   "/usr/ssl/certs/cacert.pem"
#define TEMP_PATH     "/usr/ssl/certs/cacert.pem.tmp"
#define TARGET_DIR    "/usr/ssl/certs"
#define CA_BUNDLE_UPDATE_INTERVAL 63072000 // 2 years (730 days)

static pthread_mutex_t curl_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static int curl_initialized = 0;

int debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
    const char *text;
    switch (type) {
        case CURLINFO_TEXT:
            text = "== Info";
            break;
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        default:
            return 0;
    }

    char file_name[256];
    fprintf(stderr, "%s, %zu bytes\n", text, size);
    if (type == CURLINFO_DATA_OUT) {
        FILE *file = fopen("/NFDVR/curl_debug.log", "ab");
        if (file) {
            size_t written = fwrite(data, 1, size, file);
            if (written != size) {
                fprintf(stderr, "Short write: %zu/%zu\n", written, size);
            }
            fclose(file);
        } else {
            perror("Failed to open file for writing");
        }
    }
    return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int update_ca_bundle() {
    char mkdir_cmd[256];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", TARGET_DIR);
    system(mkdir_cmd);

    struct stat st;
    if (stat(TARGET_PATH, &st) == 0) {
        time_t now = time(NULL);
        if (difftime(now, st.st_mtime) < CA_BUNDLE_UPDATE_INTERVAL) {
            printf("[Info] CA bundle is up to date (less than 2 years old). Skipping update.\n");
            return 0;
        }
    }

    printf("[Info] Downloading CA bundle from Admin Server ...\n");
    if (admin_server_download_cacert(TEMP_PATH) == 0) {
        printf("[Info] Download success. Replacing old file...\n");
        
        if (rename(TEMP_PATH, TARGET_PATH) == 0) {
            printf("[Success] CA bundle updated: %s\n", TARGET_PATH);
            return 0; 
        } else {
            perror("[Error] Failed to rename file");
            unlink(TEMP_PATH); 
            return -1;
        }
    } else {
        fprintf(stderr, "[Error] Download failed from Admin Server\n");
        unlink(TEMP_PATH);
        return -1;
    }
}

void initialize_global_curl(void) {
    pthread_mutex_lock(&curl_init_mutex);
    if (!curl_initialized) {
        CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (rc == CURLE_OK) {
            curl_initialized = 1;
            if (update_ca_bundle() == 0) {
                printf("System is ready for Secure Connection!\n");
            } else {
                printf("Using old CA bundle or none available.\n");
            }
        } else {
            fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(rc));
        }
    }
    pthread_mutex_unlock(&curl_init_mutex);
}

void cleanup_curl() {
    pthread_mutex_lock(&curl_init_mutex);
    if (curl_initialized) {
        curl_global_cleanup();
        curl_initialized = 0;
        printf("curl_global_cleanup() called\n");
    }
    pthread_mutex_unlock(&curl_init_mutex);
}