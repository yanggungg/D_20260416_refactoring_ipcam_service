/*
 * rmt.c
 * 	- remote client session modules
 *	- dependencies :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 21, 2011
 *
 */


#include "rmt.h"
#include "iux_afx.h"
#include "issm.h"
#include "nf_issm_ctl.h"
#include "ix_mem.h"
#include "issm.h"

#define DBG_LEVEL		0
#define DBG_MODULE		"RMT"



////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _USER_ID_T {
	char user_id[LEN_USERID];
} USER_ID_T;

typedef enum _RTP_CLIENT_STATUS_E{
	RTP_LIVE_PLAY_U			= 0,
	RTP_PLAYBACK_PLAY_U     = 1,
	RTP_ARCHIVING_PLAY_U    = 2,
	RTP_NOTHING_PLAY_U		= 3
}RTP_CLIENT_STATUS_E;

typedef struct _rtp_live_uinfo{
	unsigned int			start_time;				// LIVE START TIME. PAUSE 했다가 다시 시작하면 리셋하여 표시 됨. 따라서 SESSION OPEN TIME과 다름. icodec header에 시간임.

	unsigned int 			channel_mask;
	unsigned int 			channel_audio_ch;
	unsigned int			client_channel_count;			// client에서 오픈 요청한 채널의 개수

	long long				total_send_size;
}rtp_live_uinfo;

typedef struct _rtp_playback_uinfo{
	unsigned int			start_time;						// UTC time
	unsigned int			end_time;						// UTC time

	unsigned int 			channel_mask;
	unsigned int 			channel_audio_ch;
	unsigned int			client_channel_count;			// client에서 오픈 요청한 채널의 개수
	unsigned char			direction;								// FORWARD 0, BACKWARD 1
	unsigned char			speed;
	unsigned char  			reserved[2];

	long long					total_send_size;
}rtp_playback_uinfo;

typedef struct _rtp_session_uinfo{
	int								session_id;
	char							user_id[128];
	char							addr[64];
	struct timeval		session_open_time;	// gettimeofday 받은 데이타.

	RTP_CLIENT_STATUS_E	client_status;		// RTP_CLIENT_STATUS_E 값을 가짐
	rtp_live_uinfo		live_uinfo;
	rtp_playback_uinfo	playback_uinfo;
} SESSION_INFO;


////////////////////////////////////////////////////////////
//
// private variable
//


////////////////////////////////////////////////////////////
//
// private functions
//

static int _is_match(char *p, char *q)
{
	return (strcmp(p, q) == 0);
}

static int _get_user_count(SESSION_INFO *sinfo)
{
	int ssn_cnt;
	int i, j;
	int cur_cnt = 0;
	USER_ID_T *user_list;
		
	ssn_cnt = nf_issm_ctl_get_session_count();
	if (ssn_cnt <= 0) return -1;

	user_list = (USER_ID_T *)imalloc(sizeof(USER_ID_T) * ssn_cnt);		// work-around for memory crash
	memset(user_list, 0x00, sizeof(USER_ID_T) * ssn_cnt);

	strncpy(user_list[0].user_id, sinfo[0].user_id, LEN_USERID);
	user_list[0].user_id[LEN_USERID] = 0;
	cur_cnt++;

	for (i = 1; i < ssn_cnt; ++i) {
		for (j = 0; j < cur_cnt; ++j) {
			if (_is_match(sinfo[i].user_id, user_list[j].user_id)) break;
		}
		if (j != cur_cnt) continue;

		strncpy(user_list[cur_cnt].user_id, sinfo[i].user_id, LEN_USERID);
		user_list[cur_cnt].user_id[LEN_USERID] = 0;
		cur_cnt++;
	}

	ifree(user_list);
	return cur_cnt;
}

static int _update_session_info(REMOTE_USER_T *rinfo, SESSION_INFO *sinfo)
{
	switch (sinfo->client_status) {
	case RTP_LIVE_PLAY_U:
		rinfo->ssinfo[0].type = sinfo->client_status;
		rinfo->ssinfo[0].id[rinfo->ssinfo[0].cnt] = sinfo->session_id;
		rinfo->ssinfo[0].cnt++;
		break;
	case RTP_PLAYBACK_PLAY_U:
		rinfo->ssinfo[1].type = sinfo->client_status;
		rinfo->ssinfo[1].id[rinfo->ssinfo[1].cnt] = sinfo->session_id;
		rinfo->ssinfo[1].cnt++;
		break;
	case RTP_ARCHIVING_PLAY_U:
		rinfo->ssinfo[2].type = sinfo->client_status;
		rinfo->ssinfo[2].id[rinfo->ssinfo[2].cnt] = sinfo->session_id;
		rinfo->ssinfo[2].cnt++;
		break;
	case RTP_NOTHING_PLAY_U:
		rinfo->ssinfo[3].type = sinfo->client_status;
		rinfo->ssinfo[3].id[rinfo->ssinfo[3].cnt] = sinfo->session_id;
		rinfo->ssinfo[3].cnt++;
		break;
	}

	return 0;
}

static int _add_remote_user_info(REMOTE_USER_T *rinfo, SESSION_INFO *sinfo)
{
	strncpy(rinfo->user_id, sinfo->user_id, LEN_USERID);
	rinfo->user_id[LEN_USERID] = 0;
	strcpy(rinfo->addr, sinfo->addr);
	rinfo->cnt_all++;
	return 0;
}

static REMOTE_USER_T *_make_remote_user_info(SESSION_INFO *sinfo, int ssn_cnt, int usr_cnt)
{
	REMOTE_USER_T *rinfo = 0;
	int i, j;
	int add_cnt = 0;

	rinfo = imalloc(sizeof(REMOTE_USER_T) * usr_cnt);		// work-around for memory crash
	memset(rinfo, 0x00, sizeof(REMOTE_USER_T) * usr_cnt);

	for (i = 0; i < ssn_cnt; ++i) {
		for (j = 0; j < add_cnt; ++j) {
			if (_is_match(sinfo[i].user_id, rinfo[j].user_id)) {
				_update_session_info(&rinfo[j], &sinfo[i]);	
				break;
			}
		}

		if (j == add_cnt) {
			_add_remote_user_info(&rinfo[add_cnt], &sinfo[i]);
			_update_session_info(&rinfo[add_cnt], &sinfo[i]);
			++add_cnt;
		}
	}

	return rinfo;
}

static REMOTE_USER_T *_get_remote_user_info(SESSION_INFO *sinfo, int ssn_cnt, int *ret_cnt)
{
	int usr_cnt = _get_user_count(sinfo);
	if (usr_cnt <= 0) return 0;
	if (ret_cnt) *ret_cnt = usr_cnt;
	return _make_remote_user_info(sinfo, ssn_cnt, usr_cnt);
} 

static SESSION_LIST_T *_get_session_list(SESSION_INFO *sinfo, int ssn_cnt)
{
	SESSION_LIST_T *slist;
	int i;

	slist = imalloc(sizeof(SESSION_LIST_T) * ssn_cnt);	// work-around for memory crash
	memset(slist, 0x00, sizeof(SESSION_LIST_T) * ssn_cnt);

	for (i = 0; i < ssn_cnt; ++i) {
		slist[i].id = sinfo[i].session_id;
		slist[i].type = sinfo[i].client_status;
		strncpy(slist[i].user_id, sinfo[i].user_id, LEN_USERID);
		slist[i].user_id[LEN_USERID] = 0;
		strcpy(slist[i].addr, sinfo[i].addr);
	}

	return slist;
}

static SESSION_INFO *_get_session_info(int *cnt)
{
	SESSION_INFO *ps;
	ISSM_TASK_INFOS *task_infos;
	int ret, i;
	int sscnt = 0;
	int tmcnt = 0;

	DMSG(1, "");
	ret = issm_get_task_infos(&task_infos);
	if (ret != ISSM_OK) return 0;

	DMSG(1, "");
	if (cnt == 0) return 0;
	DMSG(1, "");
	tmcnt = task_infos->task_cnt;
	DMSG(1, "");
	if (tmcnt == 0) {
		DMSG(1, "%d", tmcnt);
		ret = issm_free_task_infos(&task_infos);
		DMSG(1, "free ret = %d", ret);
		return 0;
	}

	DMSG(1, "cnt = %d", *cnt);
	ps = imalloc(sizeof(SESSION_INFO) * (tmcnt));

	DMSG(1, "");
	for (i = 0; i < tmcnt; i++)
	{
		if (task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING)
		{
			ps[sscnt].session_id = task_infos->task_info[i].task_id;
			ps[sscnt].session_open_time.tv_sec = task_infos->task_info[i].session_open_time / 1000LL;
			ps[sscnt].session_open_time.tv_usec = (task_infos->task_info[i].session_open_time % 1000LL) * 1000LL;

			if (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[0])
			{
				ps[sscnt].client_status = RTP_LIVE_PLAY_U;
			  
        sprintf(ps[sscnt].user_id, "%s", task_infos->task_info[i].login_id);
			  sprintf(ps[sscnt].addr, "%s", task_infos->task_info[i].client_ip);

			  DMSG(1, "[%s] client_status     : %d\n", __FUNCTION__, ps[sscnt].client_status);
			  DMSG(1, "[%s] user_id           : %s\n", __FUNCTION__, ps[sscnt].user_id);
			  DMSG(1, "[%s] addr              : %s\n", __FUNCTION__, ps[sscnt].addr);

				++sscnt;		// itx rtsp session only
			}
			else if  (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[1])
			{
				if (task_infos->task_info[i].start_time == 0 || task_infos->task_info[i].end_time == 0)
					ps[sscnt].client_status = RTP_PLAYBACK_PLAY_U;
				else
					ps[sscnt].client_status = RTP_ARCHIVING_PLAY_U;

        sprintf(ps[sscnt].user_id, "%s", task_infos->task_info[i].login_id);
			  sprintf(ps[sscnt].addr, "%s", task_infos->task_info[i].client_ip);

			  DMSG(1, "[%s] client_status     : %d\n", __FUNCTION__, ps[sscnt].client_status);
			  DMSG(1, "[%s] user_id           : %s\n", __FUNCTION__, ps[sscnt].user_id);
			  DMSG(1, "[%s] addr              : %s\n", __FUNCTION__, ps[sscnt].addr);

				++sscnt;		// itx rtsp session only
			}
			else
			{
				ps[sscnt].client_status = RTP_LIVE_PLAY_U;
			}
		}
		else
		{
//			ps[i].client_status = RTP_NOTHING_PLAY_U;
		}

	}

	ret = issm_free_task_infos(&task_infos);
	DMSG(1, "free ret = %d", ret);
	*cnt = sscnt;
	return ps;

}


////////////////////////////////////////////////////////////
//
// public functions
//

REMOTE_USER_T *rmt_new_remote_user_info(int *ret_cnt)
{
	REMOTE_USER_T *ulist;
	SESSION_INFO *sinfo;
	int ssn_cnt;
	int usr_cnt;
	int ret;
	
	DMSG(1, "");
	sinfo = _get_session_info(&ssn_cnt);
	if (!sinfo) return 0;
	
	DMSG(1, "");
	ulist = _get_remote_user_info(sinfo, ssn_cnt, ret_cnt);
	ifree(sinfo);
	return ulist;
}

int rmt_free_remote_user_info(REMOTE_USER_T *remote)
{
	if (remote) ifree(remote);
	return 0;
}

SESSION_LIST_T *rmt_new_session_list(int *ret_cnt)
{
	SESSION_LIST_T *slist;
	SESSION_INFO *sinfo;
	int	ssn_cnt;
	int ret;

	DMSG(1, "");
	sinfo = _get_session_info(&ssn_cnt);
	if (ret_cnt) *ret_cnt = 0;
	if (!sinfo) return 0;

	DMSG(1, "");
	slist = _get_session_list(sinfo, ssn_cnt);
	if (ret_cnt) *ret_cnt = ssn_cnt;
	ifree(sinfo);
	return slist;
}

int rmt_free_session_list(SESSION_LIST_T *session)
{
	if (session) ifree(session);
	return 0;
}

int rmt_disconnect_session(SSID id)
{
	nf_issm_ctl_disconnect_session(id);
	return 0;
}
