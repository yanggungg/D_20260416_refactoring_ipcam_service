/*
 * ssm.h
 *  - session manager
 *
 * written by seongho
 *
 */

#ifndef __SSM_H
#define __SSM_H

typedef enum {
	USR_AUTH_SYS_SETUP = 0,
	USR_AUTH_SEARCH,
	USR_AUTH_ARCHIVE,
	USR_AUTH_RECORD_SETUP,
	USR_AUTH_EVENT,
	USR_AUTH_AUDIO,
	USR_AUTH_MIC,
	USR_AUTH_REMOTE, 
	USR_AUTH_PTZ, 	
	USR_AUTH_SHUTDOWN,
	USR_AUTH_COVERT,
	USR_AUTH_SEQURINET,
}UsrAuth;

enum {
	PASS_SUCCESS                = 0,
	PASS_ERROR_LENGTH           = 1<<0,
//	PASS_ERROR_3CATEGORY        = 1<<1,
	PASS_ERROR_CATEGORY        = 1<<1,
	PASS_ERROR_3REPEAT          = 1<<2,
	PASS_ERROR_SEQUENTIAL       = 1<<3,
	PASS_ERROR_INCLUDE_ID       = 1<<4,	

	PASS_CHECK_RESULT			= 5	
};

int ssm_init();
int ssm_init_auth();
void ssm_update_auth();
gboolean ssm_logon(gchar *id, gchar* password);
gboolean ssm_check_id_passwd(gchar *id, gchar* password);
gboolean ssm_is_cur_user(gchar *id, gchar* password);
gboolean ssm_check_access_auth(UsrAuth auth);
gchar* ssm_get_cur_id(char *buf);
gchar* ssm_get_prev_id(char *buf);
gboolean ssm_is_admin();
gboolean ssm_is_itxdebug_id();
void ssm_get_cur_group(char *buf);
gint ssm_get_cur_grp_idx();
gint ssm_get_cur_idx();
gint ssm_get_usr_idx(char *user_id);
void ssm_logout();
void ssm_shutdown();
gboolean ssm_is_logon_user(char *user_id);
int ssm_get_remain_day_to_expire();
int ssm_get_remain_day_to_expire_user(char *user_id);
guint ssm_get_covert_mask();
gboolean ssm_get_covert_shown_as();
int ssm_init_auto_logout();
int ssm_start_auto_logout();
int ssm_fakestart_auto_logout();
int ssm_reconfig_auto_logout();

gboolean ssm_check_passwd(gchar* password);
guint ssm_check_enhanced_passwd(gchar* id, gchar* password);

// if what you use function is blocking, use this directly,
// but if it is not, use smt module.
int ssm_stop_auto_logout();
int ssm_fakestop_auto_logout();


int ssm_reset_auto_logout_timer();
int ssm_set_new_user(char *id);
int ssm_run_auto_logout();


#endif
