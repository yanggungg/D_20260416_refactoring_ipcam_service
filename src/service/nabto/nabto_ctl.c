#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <modules/iam/nm_iam_serializer.h>

#include <jansson.h>
#include <glib.h>
#include "random_string.h"
#include "nabto_ctl.h"
#include "curl_manager.h"
#include "nf_notify.h"
#include "nf_sysdb.h"
#include "../admin_server_api.h"
#include "../nf_util_fw_single.h"

// =================================================================
// Configuration
// =================================================================
#define NABTO_DIR "/NFDVR/data/nabto"
#define KEY_FILE_PATH    NABTO_DIR "/device_key.pem" 
#define IAM_STATE_FILE_PATH NABTO_DIR "/iam_state.json"
#define CA_CERT_PATH "/usr/ssl/certs/cacert.pem"
#define LOCAL_HOST "127.0.0.1"

// =================================================================
// Data Structures
// =================================================================

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;

static nabto_log_level g_nabto_log_level = NABTO_LOG_LEVEL_TRACE;

void nabto_log_set_level(nabto_log_level level) {
    g_nabto_log_level = level;
}

void nabto_log(nabto_log_level level, const char* file, int line, const char* format, ...) {
    if (level > g_nabto_log_level) return;

    const char* level_str = "UNKNOWN";
    switch (level) {
        case NABTO_LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case NABTO_LOG_LEVEL_WARN:  level_str = "WARN "; break;
        case NABTO_LOG_LEVEL_INFO:  level_str = "INFO "; break;
        case NABTO_LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case NABTO_LOG_LEVEL_TRACE: level_str = "TRACE"; break;
        default: break;
    }

    char timestamp[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    va_list args;
    va_start(args, format);
    printf("[%s][%s][%s:%d] ", timestamp, level_str, file, line);
    vprintf(format, args);
    va_end(args);
}

struct device_app {
    NabtoDevice* device;
    struct device_iam iam;
    char product_id[64];
    char device_id[128];
    char* private_key_pem;
    char* sct_token;
};

struct network_config {
    uint rtsp_port;
    uint http_port;
};

uint g_web_port = 8080;
uint g_rtsp_port = 5554;

bool g_has_pending_req = false;
static struct network_config g_pending_req;

static bool g_is_device_started = false;

// =================================================================
// IAM
// =================================================================

static bool iam_get_effective_credentials(struct device_iam* deviceIam, char** out_pwd, char** out_sct)
{
    if (out_pwd) *out_pwd = NULL;
    if (out_sct) *out_sct = NULL;

    struct nm_iam_state* cur = nm_iam_dump_state(&deviceIam->iam);
    if (!cur) return false;

    const char* pwd = (cur->passwordOpenPassword && cur->passwordOpenPassword[0])
                        ? cur->passwordOpenPassword : NULL;
    const char* sct = (cur->passwordOpenSct && cur->passwordOpenSct[0])
                        ? cur->passwordOpenSct : NULL;

    bool ok = (pwd != NULL) || (sct != NULL);

    if (out_pwd && pwd) *out_pwd = strdup(pwd);
    if (out_sct && sct) *out_sct = strdup(sct);

    nm_iam_state_free(cur);
    return ok;
}

bool save_iam_state_to_file(const char* filename, struct nm_iam_state* state) {
    char *str = NULL; 
    if (!nm_iam_serializer_state_dump_json(state, &str)) return false;

    char tmp[512]; 
    snprintf(tmp, sizeof(tmp), "%s.tmp", filename);

    FILE* f = fopen(tmp, "wb");
    bool ok = false;

    if (f) { 
        fwrite(str, 1, strlen(str), f); fclose(f); 
        ok = (rename(tmp, filename) == 0); 
    }

    if (!ok) remove(tmp);
    nm_iam_serializer_string_free(str);
    return ok;
}

void device_iam_state_changed(struct nm_iam* iam, void* userData) {
    struct device_iam* deviceIam = userData;
    struct nm_iam_state* state = nm_iam_dump_state(&deviceIam->iam);
    if (state == NULL) return;

    if (!save_iam_state_to_file(deviceIam->iamStateFile, state)) {
        NABTO_LOG_W("Failed to persist IAM state on change callback.\n");
    }
    
    nm_iam_state_free(state);
}

void device_iam_create_default_state(struct device_iam* deviceIam, char* open_password, char* sct) {
    struct nm_iam_state* state = nm_iam_state_new();
    nm_iam_state_set_friendly_name(state, "My Device");
    nm_iam_state_set_open_pairing_role(state, "Administrator"); 
    nm_iam_state_set_local_open_pairing(state, true);
    nm_iam_state_set_password_open_pairing(state, true);
    nm_iam_state_set_password_open_password(state, open_password); 

    if (sct == NULL) {
        NABTO_LOG_E("Failed to create server connect token.\n");
    } else {
        nm_iam_state_set_password_open_sct(state, sct);
    }

    save_iam_state_to_file(deviceIam->iamStateFile, state);

    nm_iam_state_free(state);
}

static bool load_iam_config(struct device_iam* deviceIam) {
    struct nm_iam_configuration* conf = nm_iam_configuration_new();

    {
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("Pairing");
        struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
        nm_iam_configuration_statement_add_action(s, "IAM:GetPairing");
        nm_iam_configuration_statement_add_action(s, "IAM:PairingPasswordOpen");
        nm_iam_configuration_statement_add_action(s, "IAM:PairingLocalOpen");
        nm_iam_configuration_add_policy(conf, p);
    }
    {
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("DeviceControl");
        struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
        nm_iam_configuration_statement_add_action(s, "Device:GetLiveStream");
        nm_iam_configuration_statement_add_action(s, "Device:PtzControl");
        nm_iam_configuration_statement_add_action(s, "TcpTunnel:GetService"); 
        nm_iam_configuration_statement_add_action(s, "TcpTunnel:Connect");
        nm_iam_configuration_add_policy(conf, p);
    }
    {
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("ManageIam");
        struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
        nm_iam_configuration_statement_add_action(s, "IAM:ListUsers");
        nm_iam_configuration_statement_add_action(s, "IAM:GetUser");
        nm_iam_configuration_statement_add_action(s, "IAM:DeleteUser");
        nm_iam_configuration_statement_add_action(s, "IAM:SetUserRole");
        nm_iam_configuration_add_policy(conf, p);
    }
    {
        struct nm_iam_role* r = nm_iam_configuration_role_new("Unpaired");
        nm_iam_configuration_role_add_policy(r, "Pairing");
        nm_iam_configuration_add_role(conf,r);
    }
    {
        struct nm_iam_role* r = nm_iam_configuration_role_new("Administrator");
        nm_iam_configuration_role_add_policy(r, "Pairing");
        nm_iam_configuration_role_add_policy(r, "ManageIam");
        nm_iam_configuration_role_add_policy(r, "DeviceControl");
        nm_iam_configuration_add_role(conf, r);
    }

    nm_iam_configuration_set_unpaired_role(conf, "Unpaired");
    bool ok = nm_iam_load_configuration(&deviceIam->iam, conf);
    return ok;
}

bool device_iam_init(struct device_iam* deviceIam, NabtoDevice* device, const char* iamStateFile) {
    deviceIam->iamStateFile = strdup(iamStateFile);
    if (!deviceIam->iamStateFile) { 
        NABTO_LOG_E("Could not allocate memory for IAM state file path.\n");
        return false;
     }

    nm_iam_init(&deviceIam->iam, device, NULL);
    bool okCfg = load_iam_config(deviceIam);
    if (!okCfg) { NABTO_LOG_E("[IAM] load_config failed\n"); return false; }
    nm_iam_set_state_changed_callback(&deviceIam->iam, device_iam_state_changed, deviceIam);

    return true;
}

void device_iam_deinit(struct device_iam* deviceIam) {
    nm_iam_deinit(&deviceIam->iam);
    free(deviceIam->iamStateFile);
}

void debug_print_iam_state(const struct nm_iam_state* s)
{
    NABTO_LOG_D("[IAM][%s] friendlyName=%s | openPairingRole=%s | "
                "passwordOpenPairing=%d | localOpenPairing=%d | "
                "passwordOpenPassword=%p'%s' | passwordOpenSct=%p'%s'\n",
                __FUNCTION__,
                s->friendlyName ? s->friendlyName : "(NULL)",
                s->openPairingRole ? s->openPairingRole : "(NULL)",
                s->passwordOpenPairing ? 1 : 0,
                s->localOpenPairing ? 1 : 0,
                (void*)s->passwordOpenPassword,
                s->passwordOpenPassword ? s->passwordOpenPassword : "(NULL)",
                (void*)s->passwordOpenSct,
                s->passwordOpenSct ? s->passwordOpenSct : "(NULL)");
}

bool device_iam_load_state(struct device_iam* deviceIam, NabtoDevice* device, char* open_password, char* sct) {
    FILE* f = fopen(deviceIam->iamStateFile, "rb");

    if (f == NULL) {
        NABTO_LOG_I("IAM state file not found, creating default.\n");
        device_iam_create_default_state(deviceIam, open_password, sct);
        f = fopen(deviceIam->iamStateFile, "rb");
        if (f == NULL) {
            NABTO_LOG_E("Failed to read newly created state file.\n");
            return false;
        }
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    if (length <= 0) { 
        fclose(f); return false; 
    }

    fseek(f, 0, SEEK_SET);
    
    char* buffer = (char*)malloc((size_t)length + 1);
    if (!buffer) { 
        fclose(f); return false; 
    }

    if (fread(buffer, 1, length, f) != (size_t)length) {
        NABTO_LOG_E("Could not read IAM state file.\n");
        fclose(f);
        free(buffer);
        return false;
    }
    buffer[length] = '\0';
    fclose(f);

    struct nm_iam_state* state = nm_iam_state_new();
    bool success = true;

    if (!nm_iam_serializer_state_load_json(state, (char*)buffer, NULL)) {
        NABTO_LOG_E("Could not parse IAM state file.\n");
        success = false;
    } else {
        bool loaded = nm_iam_load_state(&deviceIam->iam, state);
        if (!loaded) {
            NABTO_LOG_E("Failed to load state into IAM module.\n");
            success = false;
        } 
    }

    if (success) {
        struct nm_iam_state* cur = nm_iam_dump_state(&deviceIam->iam);
        if (cur) {
            bool needUpdate = false;

            if (!cur->passwordOpenPairing) {
                nm_iam_state_set_password_open_pairing(cur, true);
                needUpdate = true;
            }

            if (!cur->passwordOpenPassword || cur->passwordOpenPassword[0] == '\0') {
                nm_iam_state_set_password_open_password(cur, open_password); // generated once at first boot
                needUpdate = true;
            }
            
            if (!cur->passwordOpenSct || cur->passwordOpenSct[0] == '\0') {
                // nabto_device_create_server_connect_token(device, &sct);
                if (sct) {
                    nm_iam_state_set_password_open_sct(cur, sct);
                    needUpdate = true;
                } else {
                    NABTO_LOG_W("Could not create SCT for open pairing.\n");
                }
            }

            if (needUpdate) {
                // Apply & persist the completed state
                if (!nm_iam_load_state(&deviceIam->iam, cur)) {
                    NABTO_LOG_E("Failed to re-apply updated IAM state.\n");
                    success = false;
                } else {
                    save_iam_state_to_file(deviceIam->iamStateFile, cur);
                }
            }
        }
        
        nm_iam_state_free(cur); 
    } else {
        nm_iam_state_free(state);
    }

    free(buffer);
    return success;
}


// =================================================================
// Utilities
// =================================================================
static bool load_key(const char* path, char** keyOut) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long length = ftell(f);

    if (length < 0) {
        NABTO_LOG_E("Could not determine size of key file.\n");
        fclose(f);
        return false;
    }

    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc((size_t)length + 1);
    if (!buffer) { 
        fclose(f); return false; 
    }

    if (fread(buffer, 1, length, f) != (size_t)length) {
        NABTO_LOG_E("Failed to read the entire key file.\n");
        free(buffer);
        fclose(f);
        return false;
    }

    buffer[length] = '\0';
    fclose(f);
    *keyOut = buffer;
    return true;
}

static bool save_key(const char* path, const char* key) {
    FILE* f = fopen(path, "w");
    if (!f) return false;
    fputs(key, f);
    fclose(f);
    return true;
}

static void log_callback(NabtoDeviceLogMessage* msg, void* userData) {
    NABTO_LOG_I("Device Log: %s\n", msg->message);
}

char* iam_create_pairing_string(struct nm_iam* iam,
                                const char* productId,
                                const char* deviceId)
{
    // Defensive checks
    if (!iam || !productId || !deviceId) return NULL;

    // Dump current IAM state (caller is responsible for nm_iam_state_free)
    struct nm_iam_state* st = nm_iam_dump_state((struct nm_iam*)iam);
    if (!st) return NULL;

    // Check if password-open pairing is enabled and fields are present
    const bool pw_open_enabled = (st->passwordOpenPairing ? true : false);
    const char* pwd = st->passwordOpenPassword;
    const char* sct = st->passwordOpenSct;


    if (!pw_open_enabled) {
        nm_iam_state_free(st);
        return NULL;
    }

    // For password-open, both pwd and sct must be available
    if (!pwd || !sct || pwd[0] == '\0' || sct[0] == '\0') {
        nm_iam_state_free(st);
        return NULL;
    }

    // Build the string "p=<pid>,d=<did>,pwd=<pwd>,sct=<sct>"
    char* out = (char*)calloc(1, 512);
    if (!out) {
        nm_iam_state_free(st);
        return NULL;
    }

    // snprintf copies the data; it's safe to free `st` afterwards
    snprintf(out, 511, "p=%s,d=%s,pwd=%s,sct=%s", productId, deviceId, pwd, sct);

    nm_iam_state_free(st);
    return out;
}


// =================================================================
// Main Application
// =================================================================
bool nabto_restart_with_new_ports(struct device_app* app, uint port_rtsp, uint port_http)
{
    NabtoDeviceError ec;
    NabtoDeviceFuture* fut = nabto_device_future_new(app->device);
    nabto_device_close(app->device, fut); 
    ec = nabto_device_future_wait(fut);   
    nabto_device_future_free(fut);

    if (ec != NABTO_DEVICE_EC_OK) {
        NABTO_LOG_W("nabto_device_close failed: %d\n", ec);
    }

    nabto_device_stop(app->device); 

    NABTO_LOG_D("Restarting Nabto device with new ports RTSP=%u HTTP=%u\n", port_rtsp, port_http);
    device_iam_deinit(&app->iam);
    nabto_device_free(app->device);

    app->device = nabto_device_new();
    nabto_device_set_log_level(app->device, "info");
    nabto_device_set_log_callback(app->device, log_callback, NULL);

    nabto_device_set_product_id(app->device, app->product_id);
    nabto_device_set_device_id(app->device, app->device_id);
    nabto_device_set_private_key(app->device, app->private_key_pem);

    if (!device_iam_init(&app->iam, app->device, IAM_STATE_FILE_PATH)) return false;
    if (!device_iam_load_state(&app->iam, app->device, NULL, NULL)) {
        NABTO_LOG_E("Failed to load IAM state.\n");
        return false;
    }

    char* pairingString = iam_create_pairing_string(
        &app->iam, nabto_device_get_product_id(app->device),
        nabto_device_get_device_id(app->device));
    
    NABTO_LOG_D("Pairing string: %s\n", pairingString ? pairingString : "(NULL)");
    free(pairingString);

    ec = nabto_device_enable_mdns(app->device);
    if (ec != NABTO_DEVICE_EC_OK) return false;

    if (app->sct_token && app->sct_token[0]) {
        ec = nabto_device_add_server_connect_token(app->device, app->sct_token);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("enable_mdns failed: %d\n", ec);
            return false;
        }
    }

    ec = nabto_device_add_tcp_tunnel_service(app->device, "rtsp","rtsp", LOCAL_HOST, port_rtsp);
    if (ec != NABTO_DEVICE_EC_OK) {
        NABTO_LOG_E("Failed to add TCP tunnel service: %s\n", nabto_device_error_get_message(ec));
        return false;
    }

    ec = nabto_device_add_tcp_tunnel_service(app->device, "http","http", LOCAL_HOST, port_http);
    if (ec != NABTO_DEVICE_EC_OK) {
        NABTO_LOG_E("Failed to add TCP tunnel service: %s\n", nabto_device_error_get_message(ec));
        return false;
    }

    {
        NabtoDeviceFuture* fut = nabto_device_future_new(app->device);
        nabto_device_start(app->device, fut);
        ec = nabto_device_future_wait(fut);
        nabto_device_future_free(fut);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("Failed to start Nabto device: %s\n", nabto_device_error_get_message(ec));
            return false;
        }
    }

    return true;
}

static void* nabto_ctl_thread_func(void *arg) {
    struct device_app app;
    NabtoDeviceError ec;
    char* key = NULL;
    char orgId[64] = {0};
    char productId[64] = {0};
    char* deviceId = NULL;
    char* fingerprint = NULL;
    NabtoDeviceFuture* fut = NULL;
    char* sct = NULL;
    const char* mac = nf_sysdb_get_str_nocopy("sys.info.mac");
    char* open_password = random_password(12);
    char *pwd_final = NULL, *sct_final = NULL;
    bool is_device_started = false;
    bool is_iam_initialized = false;
    bool key_created_by_sdk = false;
    bool already_registered = false;

    struct stat st = {0};
    if (stat(NABTO_DIR, &st) == -1) {
        if (mkdir(NABTO_DIR, 0777) != 0) {
             NABTO_LOG_E("Failed to create directory %s\n", NABTO_DIR);
        } else {
             NABTO_LOG_I("Created directory %s\n", NABTO_DIR);
        }
    }

    while (1) {
        if (!nf_sysdb_get_bool("net.ddns.p2p_enable")) {
            NABTO_LOG_I("P2P is disabled. Waiting...\n");
            sleep(10);
            continue;
        }

        // Reset API cache so we force an update upon (re)start
        admin_server_api_reset_cache();

        g_web_port = nf_sysdb_get_uint("net.proto.webport");
        g_rtsp_port = nf_sysdb_get_uint("net.proto.rtspport");

        // Reset variables for retry
        key = NULL;
        deviceId = NULL;
        fingerprint = NULL;
        sct = NULL;
        fut = NULL;
        pwd_final = NULL;
        sct_final = NULL;
        is_device_started = false;
        g_is_device_started = false;
        is_iam_initialized = false;
        key_created_by_sdk = false;
        memset(&app, 0, sizeof(struct device_app));

        if (get_nabto_init_info(orgId, sizeof(orgId), productId, sizeof(productId)) != 0) {
            NABTO_LOG_E("Failed to get nabto info\n");
            sleep(10);
            continue;
        }

        app.device = nabto_device_new();
        nabto_device_set_product_id(app.device, productId);
        nabto_device_set_log_level(app.device, "info");
        nabto_device_set_log_callback(app.device, log_callback, NULL);

        if (!load_key(KEY_FILE_PATH, &key)) {
            NABTO_LOG_I("Key file not found, creating new key.\n");
            nabto_device_create_private_key(app.device, &key);
            save_key(KEY_FILE_PATH, key);
            key_created_by_sdk = true;
        } else {
            already_registered = true;
        }
        ec = nabto_device_set_private_key(app.device, key);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("Failed to set private key: %s\n", nabto_device_error_get_message(ec));
            goto cleanup;
        }

        nabto_device_get_device_fingerprint(app.device, &fingerprint);
        if (access(IAM_STATE_FILE_PATH, F_OK) != 0) {
            nabto_device_create_server_connect_token(app.device, &sct);
        }

        if(!device_iam_init(&app.iam, app.device, IAM_STATE_FILE_PATH)) goto cleanup;
        is_iam_initialized = true;

        if (!device_iam_load_state(&app.iam, app.device, open_password, sct)) {
            NABTO_LOG_E("Failed to load IAM state.\n");
            goto cleanup;
        }
        
        bool got = iam_get_effective_credentials(&app.iam, &pwd_final, &sct_final);
        if (!got) {
            NABTO_LOG_W("Neither password nor SCT present in current IAM state\n");
        }

        if (!sct_final && sct) sct_final = strdup(sct);

        set_nabto_info(fingerprint, sct_final, pwd_final);

        if (already_registered) {
            deviceId = get_device_id();
        }
        
        if (!deviceId) {
            NABTO_LOG_I("Device ID not found or registration incomplete. Registering via POST.\n");
            admin_server_api_lock_send();
            deviceId = send_device_info("POST");
            admin_server_api_unlock_send();
        }

        NABTO_LOG_D("device_id: %s\n", deviceId ? deviceId : "(null)");

        if (!deviceId) {
            NABTO_LOG_E("Registration failed. Retrying...\n");
            goto cleanup;
        }

        nabto_device_set_device_id(app.device, deviceId);

        char* pairingString = iam_create_pairing_string(
            &app.iam, nabto_device_get_product_id(app.device),
            nabto_device_get_device_id(app.device));
        
        NABTO_LOG_D("Pairing string: %s\n", pairingString ? pairingString : "(NULL)");
        free(pairingString);

        if (sct_final && sct_final[0]) {
        ec = nabto_device_add_server_connect_token(app.device, sct_final);
            if (ec != NABTO_DEVICE_EC_OK) {
                NABTO_LOG_E("add_server_connect_token failed: %s\n", nabto_device_error_get_message(ec));
            }
        }

        ec = nabto_device_enable_mdns(app.device);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("enable_mdns failed: %d\n", ec);
            goto cleanup;
        }

        ec = nabto_device_add_tcp_tunnel_service(app.device, "rtsp", "rtsp", LOCAL_HOST, g_rtsp_port);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("Failed to add TCP tunnel service: %s\n", nabto_device_error_get_message(ec));
            goto cleanup;
        }
        
        ec = nabto_device_add_tcp_tunnel_service(app.device, "http", "http", LOCAL_HOST, g_web_port);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("Failed to add TCP tunnel service: %s\n", nabto_device_error_get_message(ec));
            goto cleanup;
        }
        
        snprintf(app.product_id, sizeof(app.product_id), "%s", productId);
        snprintf(app.device_id, sizeof(app.device_id), "%s", deviceId);
        app.private_key_pem = key; 
        key = NULL;
        app.sct_token = sct_final ? sct_final : NULL; 
        sct_final = NULL;

        fut = nabto_device_future_new(app.device);
        nabto_device_start(app.device, fut);
        ec = nabto_device_future_wait(fut);
        if (ec != NABTO_DEVICE_EC_OK) {
            NABTO_LOG_E("Failed to start Nabto device: %s\n", nabto_device_error_get_message(ec));
            goto cleanup;
        }
        is_device_started = true;
        g_is_device_started = true;

        struct nm_iam_state* st = nm_iam_dump_state(&app.iam);
        debug_print_iam_state(st);
        nm_iam_state_free(st);

        NABTO_LOG_D("nabto fingerprint %s\n", fingerprint);

        NABTO_LOG_D("Nabto device started successfully.\n");

        
        while(1) {
            if (pthread_mutex_lock(&g_mutex) != 0) {
                NABTO_LOG_E("pthread_mutex_lock failed. Exiting thread loop.\n");
                break;
            }

            while (!g_has_pending_req) {
                pthread_cond_wait(&g_cond, &g_mutex);
            }

            if (!nf_sysdb_get_bool("net.ddns.p2p_enable")) {
                NABTO_LOG_I("P2P disabled. Stopping device.\n");
                g_has_pending_req = false;
                pthread_mutex_unlock(&g_mutex);
                goto cleanup;
            }

            struct network_config req = g_pending_req;  
            g_has_pending_req = false;
            
            pthread_mutex_unlock(&g_mutex);

            bool ok = nabto_restart_with_new_ports(&app, req.rtsp_port, req.http_port);
            
            if (!ok) {
                NABTO_LOG_E("[THREAD] Restart failed.\n");
            } else {
                NABTO_LOG_I("[THREAD] Restart success. Updating global ports.\n");
                g_rtsp_port = req.rtsp_port;
                g_web_port  = req.http_port;
            }
        }

    cleanup:
        NABTO_LOG_I("Shutting down Nabto device...\n");

        if (app.device) {
            // 1. Unregister log callback to prevent logs during destruction
            nabto_device_set_log_callback(app.device, NULL, NULL);

            // 2. Stop the device comfortably
            if (is_device_started) {
                NabtoDeviceFuture* stop_fut = nabto_device_future_new(app.device);
                nabto_device_close(app.device, stop_fut);
                nabto_device_future_wait(stop_fut);
                nabto_device_future_free(stop_fut);
                nabto_device_stop(app.device);
            }
        }
        
        g_is_device_started = false;

        // 3. Free IAM
        if (is_iam_initialized) {
            device_iam_deinit(&app.iam);
        }

        // 4. Free Device
        if (app.device) {
             nabto_device_free(app.device);
        }

        // 5. Free other resources
        if (fut) nabto_device_future_free(fut);
        if (deviceId) free(deviceId);
        if (pwd_final) free(pwd_final);
        if (sct_final) free(sct_final);
        if (fingerprint) nabto_device_string_free(fingerprint);
        if (sct) nabto_device_string_free(sct);
        
        if (key) {
            if (key_created_by_sdk) nabto_device_string_free(key);
            else free(key);
        }
        if (app.private_key_pem) {
            if (key_created_by_sdk) nabto_device_string_free(app.private_key_pem);
            else free(app.private_key_pem);
        }

        printf("Nabto shutdown complete. Retrying in 10 seconds...\n");
        sleep(10);
    }
    
    free(open_password);
    return NULL;
}

void on_change_net_config(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    uint new_web_port = 0;
    uint new_rtsp_port = 0;

    gboolean p2p_enable = nf_sysdb_get_bool("net.ddns.p2p_enable");
    NABTO_LOG_I("on_change_net_config called due to sysdb change notification\n");

    if (!p2p_enable) {
        NABTO_LOG_D("p2p is not enabled. Signaling thread to stop.\n");
        pthread_mutex_lock(&g_mutex);
        g_has_pending_req = true;
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    
    if (!pinfo) return;

    NABTO_LOG_D("sysdb change cate=%d\n", pinfo->d.params[0]);
    int cate = pinfo->d.params[0];
    new_web_port = nf_sysdb_get_uint("net.proto.webport");
    new_rtsp_port = nf_sysdb_get_uint("net.proto.rtspport");

    if (cate != NF_SYSDB_CATE_NET) {
        return;
    }

    if (new_web_port == g_web_port && new_rtsp_port == g_rtsp_port) {
        NABTO_LOG_D("No relevant port/DNS changes detected. Ignoring.\n");
        return;
    }

    NABTO_LOG_D("Detected port change: RTSP %u -> %u, HTTP %u -> %u\n",
           g_rtsp_port, new_rtsp_port, g_web_port, new_web_port);

    pthread_mutex_lock(&g_mutex);
    g_pending_req.rtsp_port = new_rtsp_port;
    g_pending_req.http_port = new_web_port;
    
    g_has_pending_req = true;
    pthread_cond_signal(&g_cond);
    pthread_mutex_unlock(&g_mutex);
}

void backup_nabto() {
    char tmp[256] = {0,};
    sprintf(tmp, "cp -rf %s/iam_state.json %s/device_key.pem /common", NABTO_DIR, NABTO_DIR);
    proxy_system(tmp, 1, 3);
}

void restore_nabto() {
    char tmp[256] = {0,};
    sprintf(tmp, "cp -rf /common/iam_state.json /common/device_key.pem %s", NABTO_DIR);
    proxy_system(tmp, 1, 3);
    sprintf(tmp, "rm -rf /common/iam_state.json /common/device_key.pem");
    proxy_system(tmp, 1, 3);
}

int nabto_ctl_init() {
    pthread_t thread_id;
	gulong cb_handle=0;

    if (pthread_create(&thread_id, NULL, nabto_ctl_thread_func, NULL) != 0) {
        NABTO_LOG_E("Failed to create nabto_ctl thread!\n");
        return -1;
    }

    restore_nabto();

    cb_handle = nf_notify_connect_cb("sysdb_change", on_change_net_config, NULL);

    return 0;
}
