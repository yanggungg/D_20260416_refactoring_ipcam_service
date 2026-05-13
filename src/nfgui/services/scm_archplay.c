/*
 * scm_archplay.c
 * 	- scm archiving play service
 *	- dependencies :
 *		
 *
 * Written by Jungho Kim (kjh81@itxsecurity.com)
 * Copyright (c) ITX security, May 2, 2011
 *
 */

#include "iux_afx.h"

#include "scm.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "scm_internal.h"
#include "acp.h"
#include "nf_api_disk.h"
#include "evt.h"

#define ARCH_DEV_AFILE_FILE_CNT					512
#define AFILE_FILENAME_LEN						1024
#define SETBY_STR_LEN							32
#define MAX_VIDEO_CH							16

#define IS_ACP_SUCCESS()	(result == ACP_SUCCESS)


////////////////////////////////////////////////////////////
//
// private functions
//

static gboolean _check_play_manager(gpointer data)
{
	IMSG ret_msg = (IMSG)data;
	if (!nf_avi_player_check_thread_start()) return TRUE;
	evt_send_to_local(ret_msg, 0, 0, 0);
	return FALSE;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

HANDLER int _scm_on_verify_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	ACP_RESULT_E result = (ACP_RESULT_E)pmsg->param;

	switch (tra) {
	case TRA_VERIFY_ARCH:
		if (IS_ACP_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, ER_VERIFYING);
		break;
	}

	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_open_avi_play_manager(IMSG ret_msg)
{
	if (!nf_avi_player_start()) return -1;
	_scm_add_timeout(&iscm, 100, _check_play_manager, (gpointer)ret_msg);
	return 0;
}

int scm_close_avi_play_manager()
{	
	if (!nf_avi_player_finish()) return -1;
	return 0;
}

int scm_get_afile_name(ACPCTX acpctx, AFILEID id, char *buf, int len)
{
	return acp_get_afile_name(acpctx, id, buf,len); 
}

ACPCTX scm_create_archplay_ctx(int device_id)
{
	ACP_T *ctx;
	ctx = acp_create(device_id);
	return (ACPCTX)ctx;
}

int scm_destroy_archplay_ctx(ACPCTX acpctx)
{
	if (acpctx) acp_destroy(acpctx);
	return 0;
}

int scm_get_afile_count(ACPCTX acpctx)
{
	return acp_get_count(acpctx);
}

AFILE_INFO_T *scm_new_afile_list(ACPCTX acpctx, AFILEID start, AFILEID end)
{
	return acp_new_afile_list(acpctx, start, end);
}

int scm_free_afile_list(AFILE_INFO_T *afile_list)
{
	return acp_free_afile_list(afile_list);
}

int scm_get_afile_detail(ACPCTX acpctx, AFILEID id, NF_AVI_PLAYER_INFO_IN_JUNK *detail)
{	
	return acp_get_afile_detail(acpctx, id, detail);
}

int scm_set_play_afile(ACPCTX acpctx, AFILEID id)
{	
	return acp_play_afile(acpctx, id);
}

int scm_verify_arch_file(ACPCTX acpctx, AFILEID id, IMSG ret_msg)
{
	int ret = 0;
	TRANSACTION_E tra = TRA_VERIFY_ARCH;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_ACP_VERIFY_FILE, (void *)tra };
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	ret = acp_verify_file(acpctx, id, &cmmack);
	return ret;
}

