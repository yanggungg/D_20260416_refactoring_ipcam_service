#ifndef ADMIN_SERVER_API_H
#define ADMIN_SERVER_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ITX server API module.
 * 
 * Sets up necessary callbacks (e.g., for system DB changes).
 * 
 * @return 0 on success, non-zero on failure.
 */
int admin_server_api_init();

/**
 * @brief Reset the cached last sent JSON and password.
 * 
 * This should be called when the Nabto service is (re)started or P2P is toggled ON,
 * ensuring that the device info is sent at least once even if unchanged.
 */
void admin_server_api_reset_cache();

/**
 * @brief Set the Nabto information to be included in registration/updates.
 * 
 * @param fingerprint The Nabto device fingerprint.
 * @param sct         The Server Connect Token.
 * @param password    The shared password.
 */
void set_nabto_info(const char* fingerprint, const char* sct, const char* password);

/**
 * @brief Perform synchronous device registration.
 * 
 * Sends the full device info + Nabto info to the server via POST /regist.
 * If registration fails, it may attempt to retrieve existing info via GET /device.
 * 
 * @return A newly allocated string containing the deviceId on success, or NULL on failure.
 *         The caller is responsible for freeing the returned string.
 */
/**
 * @brief Send device information to the server.
 * 
 * If method is "POST", it performs registration and returns the deviceId.
 * If method is "PUT", it performs an update and returns NULL (or deviceId if available, but typically ignored).
 * 
 * @param method "POST" for registration, "PUT" for update.
 * @return A newly allocated string containing the deviceId if method is "POST" and success, otherwise NULL.
 *         The caller is responsible for freeing the returned string.
 */
char* send_device_info(const char* method);

/**
 * @brief Retrieve Nabto information (orgId, productId) from the server.
 * 
 * Uses GET /device/api/nabto with mac, vender, and type parameters.
 * 
 * @param orgId      Buffer to store the retrieved Organization ID.
 * @param org_len    Size of the orgId buffer.
 * @param productId  Buffer to store the retrieved Product ID.
 * @param prod_len   Size of the productId buffer.
 * 
 * @return 0 on success, non-zero on failure.
 */
int get_nabto_init_info(char *orgId, size_t org_len, char *productId, size_t prod_len);

/**
 * @brief Retrieve the device ID directly from the server via GET.
 * 
 * Uses GET /device/api/info?mac=<MAC>
 * 
 * @return A newly allocated string containing the deviceId, or NULL if not found/failed.
 *         The caller must free the returned string.
 */
char* get_device_id();

/**
 * @brief Download the CA certificate from the admin server.
 * 
 * Uses GET /device/api/cacert with ApiKey header.
 * 
 * @param output_path Path to save the downloaded file.
 * @return 0 on success, non-zero on failure.
 */
int admin_server_download_cacert(const char *output_path);

/**
 * @brief Acquire the send-thread guard.
 * 
 * Sets g_send_thread_active = TRUE so that on_change_device_info
 * callbacks will skip while a direct send_device_info call is in progress.
 * Must be paired with admin_server_api_unlock_send().
 */
void admin_server_api_lock_send();

/**
 * @brief Release the send-thread guard.
 * 
 * Sets g_send_thread_active = FALSE so that on_change_device_info
 * callbacks can proceed again.
 */
void admin_server_api_unlock_send();

#ifdef __cplusplus
}
#endif

#endif
