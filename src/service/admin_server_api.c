#include "curl_manager.h"
#include "nf_notify.h"
#include "nf_sysdb.h"
#include "nf_sysman.h"
#include <jansson.h>
#include <glib.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/aes.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "admin_server_api.h"
#include <pthread.h>

#define API_KEY "9fb4d71d2285449a9e20e07285a141bcd57231bbbc40a22a95775aca5890439b"
#define API_SERVER "https://admin.seqnex.com/device/api"
#define CA_CERT_PATH "/usr/ssl/certs/cacert.pem"
#define SECRET_KEY "a1b2c3d4e5f67890123456789abcdef0123456789abcdef0123456789abcdef0" 
#define IAM_STATE_FILE_PATH "/NFDVR/data/nabto/iam_state.json"

static char *g_nabto_fingerprint = NULL;
static char *g_nabto_sct = NULL;
static char *g_nabto_password = NULL;
static gboolean g_send_thread_active = FALSE;
static pthread_mutex_t g_send_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *g_last_sent_json = NULL;
static char *g_last_raw_password = NULL;
static pthread_mutex_t g_json_mutex = PTHREAD_MUTEX_INITIALIZER;

// Response data structure for curl
struct response_data {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct response_data *data = (struct response_data *)userp;

    char *ptr = realloc(data->memory, data->size + total_size + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Not enough memory to store response data\n");
        return 0;
    }

    data->memory = ptr;
    memcpy(&(data->memory[data->size]), contents, total_size);
    data->size += total_size;
    data->memory[data->size] = 0;

    return total_size;
}

// Helper: Convert hex string to binary
static int hex2bin(const char *hex, unsigned char *bin, int bin_len) {
    int len = 0;
    const char *p = hex;
    while (*p && len < bin_len) {
        char high = *p++;
        char low = *p++;
        if (!low) break; // Odd length, should not happen for valid hex keys

        unsigned char byte = 0;
        if (high >= '0' && high <= '9') byte = (high - '0') << 4;
        else if (high >= 'a' && high <= 'f') byte = (high - 'a' + 10) << 4;
        else if (high >= 'A' && high <= 'F') byte = (high - 'A' + 10) << 4;

        if (low >= '0' && low <= '9') byte |= (low - '0');
        else if (low >= 'a' && low <= 'f') byte |= (low - 'a' + 10);
        else if (low >= 'A' && low <= 'F') byte |= (low - 'A' + 10);

        bin[len++] = byte;
    }
    return len;
}

// Helper: Base64 encode
static char *base64_encode(const unsigned char *input, int length) {
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No newlines
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;

    BIO_free_all(b64);

    return buff;
}

// Helper: Encrypt password (AES-256-CBC)
// Returns Base64(IV + Ciphertext)
static char *encrypt_password(const char *password, const char *hex_api_key) {
    if (!password || !hex_api_key) return NULL;

    unsigned char key[32];
    unsigned char iv[16];
    unsigned char *ciphertext = NULL;
    int ciphertext_len = 0;
    int len = 0;
    int password_len = strlen(password);
    char *result = NULL;

    // 1. Prepare Key
    memset(key, 0, sizeof(key));
    hex2bin(hex_api_key, key, sizeof(key));

    // 2. Generate random IV
    if (!RAND_bytes(iv, sizeof(iv))) {
        fprintf(stderr, "[API] RAND_bytes failed\n");
        return NULL;
    }

    // 3. Encrypt
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;

    // Max ciphertext len = plaintext len + block size (for padding)
    ciphertext = (unsigned char *)malloc(password_len + AES_BLOCK_SIZE);
    if (!ciphertext) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) goto err;
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char *)password, password_len)) goto err;
    ciphertext_len = len;
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) goto err;
    ciphertext_len += len;

    // 4. Concatenate IV + Ciphertext
    int total_len = sizeof(iv) + ciphertext_len;
    unsigned char *final_buf = (unsigned char *)malloc(total_len);
    if (!final_buf) goto err;

    memcpy(final_buf, iv, sizeof(iv));
    memcpy(final_buf + sizeof(iv), ciphertext, ciphertext_len);

    // 5. Base64 Encode
    result = base64_encode(final_buf, total_len);

    free(final_buf);
    free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);
    return result;

err:
    if (ciphertext) free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);
    return NULL;
}

void set_nabto_info(const char* fingerprint, const char* sct, const char* password) {
    if (g_nabto_fingerprint) free(g_nabto_fingerprint);
    if (g_nabto_sct) free(g_nabto_sct);
    if (g_nabto_password) free(g_nabto_password);

    g_nabto_fingerprint = fingerprint ? strdup(fingerprint) : NULL;
    g_nabto_sct = sct ? strdup(sct) : NULL;
    g_nabto_password = password ? strdup(password) : NULL;
}

static json_t* build_device_info_json() {
    json_t *root = json_object();

    unsigned int ip, tmp[4];
    char ipaddr[16];
    gboolean httpson = nf_sysdb_get_bool("net.proto.httpson");
    char *mac = nf_sysdb_get_str_nocopy("sys.info.mac");
    
    ip = nf_sysdb_get_uint("net.proto.ipaddr");

    tmp[0] = (ip >> 24) & 255;
    tmp[1] = (ip >> 16) & 255;
    tmp[2] = (ip >> 8) & 255;
    tmp[3] = ip & 255;
    
    snprintf(ipaddr, sizeof(ipaddr), "%d.%d.%d.%d", tmp[0], tmp[1], tmp[2], tmp[3]);

    if(nf_sysdb_get_bool("net.proto.ddnson")) {
        char *ddnsSvr = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");
        char ddnsUrl[256];

        snprintf(ddnsUrl, sizeof(ddnsUrl), "%s.%s", mac, ddnsSvr);
        
        json_object_set_new(root, "externalIp", json_string(ddnsUrl));
    } else {
        json_object_set_new(root, "externalIp", json_string(""));
    }

    json_object_set_new(root, "mac", json_string(mac));
    json_object_set_new(root, "internalIp", json_string(ipaddr));
    json_object_set_new(root, "webPort", json_integer(nf_sysdb_get_uint("net.proto.webport")));
    json_object_set_new(root, "rtspPort", json_integer(nf_sysdb_get_uint("net.proto.rtspport")));
    json_object_set_new(root, "https", json_integer(httpson));
    json_object_set_new(root, "loginId", json_string(nf_sysdb_get_str_nocopy("usr.U0.name")));
    
    // Encrypt Password
    const char *raw_pw = nf_sysdb_get_str_nocopy("usr.U0.pass");
    char *enc_pw = encrypt_password(raw_pw, SECRET_KEY);
    if (enc_pw) {
        json_object_set_new(root, "loginPw", json_string(enc_pw));
        free(enc_pw);
    } else {
        fprintf(stderr, "[API] Password encryption failed\n");
        json_object_set_new(root, "loginPw", json_string(""));
    }
    
    const char *model_name_raw = nf_sysdb_get_str_nocopy("sys.info.model");
    char *model_name_final = NULL;

    if (model_name_raw && strncmp(model_name_raw, "IPX", 3) != 0) {
        size_t len = strlen(model_name_raw);
        model_name_final = (char *)malloc(len + 5); 
        if (model_name_final) {
            sprintf(model_name_final, "IPX %s", model_name_raw);
        } else {
            fprintf(stderr, "[API] Failed to allocate memory for model name\n");
            model_name_final = (char *)model_name_raw; 
        }
    } else {
        model_name_final = (char *)model_name_raw; 
    }

    json_object_set_new(root, "modelName", json_string(model_name_final));
    if (model_name_final && model_name_final != model_name_raw) {
        free(model_name_final);
    }

    int max_ch = var_get_ch_count();
    json_object_set_new(root, "maxChannel", json_integer(max_ch));
    json_object_set_new(root, "vender", json_string(nf_sysdb_get_str_nocopy("sys.info.vendor")));
    json_object_set_new(root, "fwVersion", json_string(nf_sysdb_get_str_nocopy("sys.info.swver")));
    json_object_set_new(root, "hwVersion", json_string(nf_sysdb_get_str_nocopy("sys.info.hwver")));

    // Add Nabto info if available
    #if defined(ENABLE_NABTO)
        if (g_nabto_fingerprint && g_nabto_sct && g_nabto_password) {
            json_t *nabto_obj = json_object();
            json_object_set_new(nabto_obj, "fingerprint", json_string(g_nabto_fingerprint));
            json_object_set_new(nabto_obj, "sct", json_string(g_nabto_sct));
            json_object_set_new(nabto_obj, "password", json_string(g_nabto_password));
            json_object_set_new(root, "nabto", nabto_obj);
        }
    #else
        json_object_set_new(root, "nabto", json_string(""));
    #endif

    return root;
}

char* get_device_id() {
    CURL *curl;
    CURLcode res;
    char url[512];
    struct response_data response = {0};
    long http_code = 0;
    char *device_id = NULL;

    char *mac = nf_sysdb_get_str_nocopy("sys.info.mac");
    if (!mac) return NULL;

    initialize_global_curl();
    curl = curl_easy_init();
    if (!curl) return NULL;
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "ApiKey: " API_KEY);
    headers = curl_slist_append(headers, "Accept: application/json");

    snprintf(url, sizeof(url), "%s/info?mac=%s", API_SERVER, mac);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CAINFO, CA_CERT_PATH);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); 
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res == CURLE_OK && http_code == 200) {
        json_error_t jerr;
        // Debug: Print raw JSON
        fprintf(stderr, "[API] GET raw response: %s\n", response.memory ? response.memory : "(null)");

        json_t *root = json_loads(response.memory ? response.memory : "", 0, &jerr);
        if (root) {
            json_t *nabto_obj = json_object_get(root, "nabto");
            if (nabto_obj && json_is_object(nabto_obj)) {
                json_t *dev = json_object_get(nabto_obj, "deviceId");
                if (dev) {
                    if (json_is_string(dev)) {
                        device_id = strdup(json_string_value(dev));
                        fprintf(stderr, "[API] Parsed deviceId: %s\n", device_id);
                    } else if (json_is_null(dev)) {
                        fprintf(stderr, "[API] deviceId field exists but is JSON null\n");
                    } else {
                        fprintf(stderr, "[API] deviceId field exists but is not string (type=%d)\n", json_typeof(dev));
                    }
                } else {
                     fprintf(stderr, "[API] GET info success but deviceId key missing in nabto object\n");
                }
            } else {
                 fprintf(stderr, "[API] GET info success but no nabto object found\n");
            }
            json_decref(root);
        } else {
             fprintf(stderr, "[API] GET info success but failed to parse JSON: %s\n", response.memory);
        }
    } else {
         fprintf(stderr, "[API] GET info failed (res=%d, code=%ld)\n", res, http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (response.memory) free(response.memory);

    return device_id;
}

char* send_device_info(const char *method) {
    CURL *curl;
    CURLcode res;
    json_t *root = NULL;
    char *json_data = NULL;
    char url[512];
    struct response_data response = {0};
    long http_code = 0;
    char *device_id = NULL;

    initialize_global_curl();
    curl = curl_easy_init();
    if (!curl) return NULL;
    
    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "ApiKey: " API_KEY);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    if (strcmp(method, "POST") == 0) {
        snprintf(url, sizeof(url), "%s/regist", API_SERVER);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (strcmp(method, "PUT") == 0) {
        snprintf(url, sizeof(url), "%s/edit", API_SERVER);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    } else {
        fprintf(stderr, "Unsupported HTTP method: %s\n", method);
        free(json_data);
        json_decref(root);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CAINFO, CA_CERT_PATH);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // debugging
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    while (1) {
        if (root) json_decref(root);
        if (json_data) free(json_data);

        root = build_device_info_json();
        json_data = json_dumps(root, JSON_COMPACT);

        if (json_data && strcmp(method, "PUT") == 0) {
            const char *raw_pw = nf_sysdb_get_str_nocopy("usr.U0.pass");
            pthread_mutex_lock(&g_json_mutex);
            
            int skip = 0;
            if (g_last_sent_json && g_last_raw_password && 
                raw_pw && strcmp(g_last_raw_password, raw_pw) == 0) {
                
                json_error_t jerr;
                json_t *temp_root = json_loads(g_last_sent_json, 0, &jerr);
                if (temp_root) {
                    // Temporarily remove loginPw from both for comparison
                    json_object_del(root, "loginPw");
                    json_object_del(temp_root, "loginPw");
                    
                    if (json_equal(root, temp_root)) {
                        skip = 1;
                    }
                    json_decref(temp_root);
                }
            }
            
            pthread_mutex_unlock(&g_json_mutex);

            if (skip) {
                printf("[API] Device info unchanged. Skipping PUT.\n");
                free(json_data);
                json_decref(root);
                if (response.memory) free(response.memory);
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return NULL;
            }
        }

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(json_data));

        if (response.memory) {
            free(response.memory);
            response.memory = NULL;
            response.size = 0;
        }

        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Debug: Print response on failure or 500 error
        if (res != CURLE_OK || http_code >= 400) {
             fprintf(stderr, "[API] Request failed. Code: %ld, Response: %s\n", http_code, response.memory ? response.memory : "(null)");
        }

        if (res == CURLE_OK && http_code >= 200 && http_code < 300) {
            int success = 0;

            #if defined(ENABLE_NABTO)
                if (strcmp(method, "POST") == 0) {
                    json_error_t jerr;
                    json_t *resp_root = json_loads(response.memory ? response.memory : "", 0, &jerr);
                    if (resp_root) {
                        json_t *dev = json_object_get(resp_root, "deviceId");
                        if (!dev) dev = json_object_get(resp_root, "device_id");
                        
                        if (dev && json_is_string(dev)) {
                            if (device_id) free(device_id);
                            device_id = strdup(json_string_value(dev));
                        } else {
                            fprintf(stderr, "[API] POST success but no deviceId found in response: %s\n", response.memory);
                        }
                        success = 1;
                        json_decref(resp_root);
                    } else {
                        fprintf(stderr, "[API] POST success but failed to parse JSON: %s\n", response.memory);
                    }
                } else {
                    // PUT request
                    success = 1;
                }
            #else
                success = 1;
            #endif

            if (success) {
                pthread_mutex_lock(&g_json_mutex);
                if (g_last_sent_json) free(g_last_sent_json);
                g_last_sent_json = strdup(json_data);
                
                if (g_last_raw_password) free(g_last_raw_password);
                const char *raw_pw = nf_sysdb_get_str_nocopy("usr.U0.pass");
                g_last_raw_password = raw_pw ? strdup(raw_pw) : NULL;
                pthread_mutex_unlock(&g_json_mutex);
                break;
            }
        }

        if (strcmp(method, "PUT") == 0 && http_code == 404) {
            fprintf(stderr, "[API] PUT failed with 404. Device not found on server. Switching to POST to register.\n");
            
            // Clean up current handle
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            if (json_data) { free(json_data); json_data = NULL; }
            if (root) { json_decref(root); root = NULL; }

            // Switch to POST
            method = "POST";
            
            // Re-initialize CURL for POST
            curl = curl_easy_init();
            if (!curl) return NULL;
            
            headers = NULL;
            headers = curl_slist_append(headers, "ApiKey: " API_KEY);
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");

            snprintf(url, sizeof(url), "%s/regist", API_SERVER);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_CAINFO, CA_CERT_PATH);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            
            continue; // Retry immediately with POST
        }

        fprintf(stderr, "API request failed or invalid response (res=%d, code=%ld). Retrying in 10s...\n", res, http_code);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
        }
        sleep(10);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_data);
    json_decref(root);
    if (response.memory) free(response.memory);
    
    return device_id;
}

int get_nabto_init_info(char *orgId, size_t org_len, char *productId, size_t prod_len) {
    CURL *curl;
    CURLcode res;
    struct response_data chunk = (struct response_data){0};
    int ret = -1;
    char api_get_info_url[512];
    char errbuf[CURL_ERROR_SIZE];
    errbuf[0] = 0;

    const char* mac = nf_sysdb_get_str_nocopy("sys.info.mac");
    const char* vender = nf_sysdb_get_str_nocopy("sys.info.vendor");
    const char* type = "NVR"; 

    snprintf(api_get_info_url, sizeof(api_get_info_url), "%s/nabto?mac=%s&vender=%s&type=%s", 
             API_SERVER, mac ? mac : "", vender ? vender : "", type ? type : "");

    initialize_global_curl();

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl_easy_init() failed\n");
        return -1;
    }
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "ApiKey: " API_KEY);

    curl_easy_setopt(curl, CURLOPT_URL, api_get_info_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CAINFO, CA_CERT_PATH);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // debugging
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        fprintf(stderr, "Detailed error: %s\n", errbuf);
    } else {
        json_error_t jerr;
        json_t *root = json_loads(chunk.memory ? chunk.memory : "", 0, &jerr);
        if (!root) {
            fprintf(stderr, "jansson parse error at line %d: %s\n", jerr.line, jerr.text);
        } else if (!json_is_object(root)) {
            fprintf(stderr, "Root is not an object. Body: %.*s\n", (int)chunk.size, chunk.memory ? chunk.memory : "");
            json_decref(root);
        } else {
            json_t *org = json_object_get(root, "orgId");
            json_t *prd = json_object_get(root, "productId");
            if (json_is_string(org) && json_is_string(prd)) {
                snprintf(orgId, org_len, "%s", json_string_value(org));
                snprintf(productId, prod_len, "%s", json_string_value(prd));
                ret = 0;
            } else {
                fprintf(stderr, "Missing keys orgId/productId. Body: %.*s\n", (int)chunk.size, chunk.memory ? chunk.memory : "");
            }
            json_decref(root);
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return ret;
}

static void *send_device_info_thread_func(void *arg) {
    char *method = (char *)arg;
    if (method) {
        char *res = send_device_info(method);
        if (res) free(res);
        free(method);
    }
    
    pthread_mutex_lock(&g_send_thread_mutex);
    g_send_thread_active = FALSE;
    pthread_mutex_unlock(&g_send_thread_mutex);
    
    return NULL;
}

void on_change_device_info(NF_NOTIFY_INFO *pinfo, gpointer data) {
    if (!pinfo) return;

    int cate = pinfo->d.params[0];
    char *method = NULL;

    pthread_mutex_lock(&g_send_thread_mutex);
    if (g_send_thread_active) {
        printf("[API] send_device_info thread is already active. Skipping.\n");
        pthread_mutex_unlock(&g_send_thread_mutex);
        return;
    }
    // Mark active BEFORE creating the thread to prevent race condition
    g_send_thread_active = TRUE;
    pthread_mutex_unlock(&g_send_thread_mutex);
    
    if (access(IAM_STATE_FILE_PATH, F_OK) == 0) {
        method = strdup("PUT");
    } else {
        method = strdup("POST");
    }

    if (method) {
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&tid, &attr, send_device_info_thread_func, method) != 0) {
            fprintf(stderr, "[API] Failed to create send_device_info thread\n");
            free(method);
            // Revert active flag since thread creation failed
            pthread_mutex_lock(&g_send_thread_mutex);
            g_send_thread_active = FALSE;
            pthread_mutex_unlock(&g_send_thread_mutex);
        }
        pthread_attr_destroy(&attr);
    } else {
        // Revert active flag since no method was determined
        pthread_mutex_lock(&g_send_thread_mutex);
        g_send_thread_active = FALSE;
        pthread_mutex_unlock(&g_send_thread_mutex);
    }
}

int admin_server_api_init() {
	gulong cb_handle=0;

    cb_handle = nf_notify_connect_cb("sysdb_change", on_change_device_info, NULL);

    return 0;
}

void admin_server_api_reset_cache() {
    pthread_mutex_lock(&g_json_mutex);
    if (g_last_sent_json) {
        free(g_last_sent_json);
        g_last_sent_json = NULL;
    }
    if (g_last_raw_password) {
        free(g_last_raw_password);
        g_last_raw_password = NULL;
    }
    pthread_mutex_unlock(&g_json_mutex);
}

int admin_server_download_cacert(const char *output_path) {
    CURL *curl;
    CURLcode res;
    FILE *fp;
    long http_code = 0;
    char url[512];

    snprintf(url, sizeof(url), "%s/cacert", API_SERVER);

    curl = curl_easy_init();
    if (!curl) return -1;

    fp = fopen(output_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return -1;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "ApiKey: " API_KEY);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DG-ITX");
    
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    fclose(fp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && http_code == 200) {
        return 0;
    }
    return -1;
}

void admin_server_api_lock_send() {
    pthread_mutex_lock(&g_send_thread_mutex);
    g_send_thread_active = TRUE;
    pthread_mutex_unlock(&g_send_thread_mutex);
}

void admin_server_api_unlock_send() {
    pthread_mutex_lock(&g_send_thread_mutex);
    g_send_thread_active = FALSE;
    pthread_mutex_unlock(&g_send_thread_mutex);
}
