#ifndef device_iam_H
#define device_iam_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include <nabto/nabto_device.h>
#include <modules/iam/nm_iam.h>

/**
 * @brief Lightweight IAM wrapper used by the camera application.
 *
 * - Owns an nm_iam instance bound to a NabtoDevice.
 * - Persists IAM state to a JSON file on the device.
 * - Provides helpers for pairing strings and one-time password invite.
 */
struct device_iam {
    // Nabto IAM instance (lifecycle: init → deinit)
    struct nm_iam iam;

    // Path to IAM state JSON file (owned by this struct)
    char* iamStateFile;
};

/**
 * @brief Initialize IAM wrapper and bind to a NabtoDevice.
 *
 * @param deviceIam   Out object to initialize.
 * @param device      Nabto device (must outlive deviceIam).
 * @param iamStateFile Path to JSON state file (will be strdup'ed).
 *
 * Notes:
 * - Does NOT start the device.
 * - Does NOT load state; call device_iam_load_state() after init.
 * - Sets the state changed callback to auto-persist on updates.
 */
bool device_iam_init(struct device_iam* deviceIam,
                     NabtoDevice* device,
                     const char* iamStateFile);

/**
 * @brief Deinitialize IAM wrapper and free owned resources.
 *
 * Notes:
 * - Calls nm_iam_deinit().
 * - Frees iamStateFile.
 * - Does NOT stop/free the NabtoDevice.
 */
void device_iam_deinit(struct device_iam* deviceIam);

/**
 * @brief Create a secure default IAM state and persist to file.
 *
 * Notes:
 * - Industrial defaults: local/open pairing disabled by default.
 * - Pairing role defaults to "Administrator" (adjust to your policy).
 * - Does not start any pairing mode automatically.
 *
 * Call this when the IAM state file does not exist, typically from
 * device_iam_load_state() if fopen() fails.
 */
void device_iam_create_default_state(struct device_iam* deviceIam, char* open_password, char* sct);

/**
 * @brief Load IAM state from JSON file and apply to IAM module.
 *
 * @return true on success, false on parse/apply errors.
 *
 * Notes:
 * - On first boot, will create a default file and then load it.
 * - On success, DO NOT free any internal state pointers yourself
 *   (nm_iam owns its internal state; deallocate via nm_iam_deinit()).
 */
bool device_iam_load_state(struct device_iam* deviceIam, NabtoDevice* device, char* open_password, char* sct);

/**
 * @brief Persist IAM state to a JSON file.
 *
 * @param filename Target path.
 * @param state    State to serialize.
 * @return true on success.
 *
 * Ownership:
 * - Does not take ownership of @p state.
 */
bool save_iam_state_to_file(const char* filename, struct nm_iam_state* state);

/**
 * @brief Create a pairing string using current IAM state.
 *
 * Format: "p=<productId>,d=<deviceId>,pwd=<password>,sct=<sct>"
 *
 * @param iam       IAM instance.
 * @param productId Nabto product id.
 * @param deviceId  Nabto device id.
 * @return Heap-allocated string; caller must free() it. NULL on error.
 *
 * Notes:
 * - Uses nm_iam_dump_state() internally; NULL fields are emitted as empty.
 */
char* iam_create_pairing_string(struct nm_iam* iam,
                                const char* productId,
                                const char* deviceId);

/**
 * @brief Issue a one-time password invite (industrial flow).
 *
 * Behavior:
 * - Enables PasswordInvite pairing mode.
 * - Sets invite username/password and an on-demand SCT.
 * - Applies state to IAM (nm_iam_load_state) and persists to file.
 * - Returns a ready-to-share pairing string: 
 *   "p=<p>,d=<d>,u=<username>,pwd=<pwd>,sct=<sct>"
 *
 * @param deviceIam  IAM wrapper.
 * @param username   Unique username for the invite (validated by caller).
 * @param outPairStr Out heap string; caller must free(). Set to NULL on failure.
 * @return true on success.
 *
 * Security:
 * - Do NOT log the returned string (contains secrets).
 * - Recommended to enforce short TTL at higher layers and revoke after use.
 */
bool device_iam_issue_one_time_invite(struct device_iam* deviceIam,
                                      const char* username,
                                      char** outPairStr);

/**
 * @brief Revoke current invite (disable and clear secrets).
 *
 * Behavior:
 * - Disables PasswordInvite mode.
 * - Clears invite username/password/SCT in state.
 * - Applies to IAM and persists to file.
 *
 * @return true on success.
 */
bool device_iam_revoke_invite(struct device_iam* deviceIam);

/**
 * @brief Optional: State-changed callback to auto-persist state.
 *
 * Signature matches nm_iam_set_state_changed_callback().
 * You may expose and reuse this if you want to hook it from outside,
 * otherwise keep it static in your .c and omit this prototype.
 */
void device_iam_state_changed(struct nm_iam* iam, void* userData);

/**
 * @brief Optional: Debug helper to print critical IAM fields.
 *
 * Implement only in debug builds if needed.
 */
void debug_print_iam_state(const struct nm_iam_state* s);

void backup_nabto();

// Nabto Control Init
int nabto_ctl_init();

// =================================================================
// Logging
// =================================================================
typedef enum {
    NABTO_LOG_LEVEL_NONE = 0,
    NABTO_LOG_LEVEL_ERROR,
    NABTO_LOG_LEVEL_WARN,
    NABTO_LOG_LEVEL_INFO,
    NABTO_LOG_LEVEL_DEBUG,
    NABTO_LOG_LEVEL_TRACE
} nabto_log_level;

void nabto_log_set_level(nabto_log_level level);
void nabto_log(nabto_log_level level, const char* file, int line, const char* format, ...);

#define NABTO_LOG_E(fmt, ...) nabto_log(NABTO_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define NABTO_LOG_W(fmt, ...) nabto_log(NABTO_LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define NABTO_LOG_I(fmt, ...) nabto_log(NABTO_LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define NABTO_LOG_D(fmt, ...) nabto_log(NABTO_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define NABTO_LOG_T(fmt, ...) nabto_log(NABTO_LOG_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // device_iam_H
