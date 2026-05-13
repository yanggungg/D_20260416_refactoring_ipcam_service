/*
 * acp.h
 * 	- archiving player 
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 18, 2011
 *
 */

#ifndef __ACP_H
#define __ACP_H

#include "iux_afx.h"
#include "nf_api_disk.h"
#include "cmm.h"


////////////////////////////////////////////////////////////
//
// public data type 
//


typedef int 	AFILEID;
typedef struct _ACP_T 	ACP_T;

typedef struct _AFILE_INFO_T{
	AFILEID			id;
	gchar 			full_name[1024];	// Full path name
	gboolean		is_mul;				// multi file (.mul)
	guint 			ch_mask;			// ex) 000001 -> 1CH , 000011 -> 1CH & 2CH
	time_t 			movie_start;
	time_t 			movie_end;
	gchar 			user[32];
	guint 			file_size;
} AFILE_INFO_T;

typedef enum _ACP_RESULT_E {
	ACP_SUCCESS		= 0,
	ACP_FAIL		= 0xFF,
} ACP_RESULT_E;

////////////////////////////////////////////////////////////
//
// public interfaces
//

ACP_T *acp_create(int device_id);
int acp_destroy(ACP_T *pacp);
int acp_get_count(ACP_T *pacp);
AFILE_INFO_T *acp_new_afile_list(ACP_T *pacp, AFILEID start, AFILEID end);
int acp_free_afile_list(AFILE_INFO_T *afile_list);
int acp_get_afile_name(ACP_T *pacp, AFILEID id, char *buf, int len);
int acp_get_afile_detail(ACP_T *pacp, AFILEID id, NF_AVI_PLAYER_INFO_IN_JUNK *detail);
int acp_play_afile(ACP_T *pacp, AFILEID id);
int acp_verify_file(ACP_T *pacp, AFILEID id, CMMACK_T *pcmmack);

#endif

