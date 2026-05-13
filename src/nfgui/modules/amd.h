/*
 * amd.h
 * 	- media manager for viewer
 * 	- high level module over the scm
 *	- dependency :
 *		acp
 *		scm
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 18, 2011
 *
 */

#ifndef __AMD_H
#define __AMD_H

#include "mda.h"
#include "acp.h"

////////////////////////////////////////////////////////////
//
// public data type 
//

#if defined(__ITXGUI64)
typedef unsigned long int ACPCTX;
#else 
typedef unsigned int ACPCTX;
#endif
////////////////////////////////////////////////////////////
//
// public interfaces
//

int amd_init(const MEDIA_INFO_T *media_info);
int amd_cleanup();
int amd_remove_all_amda();
ACPCTX amd_get_acp(char *title);
AFILEID amd_find_start_afile(ACPCTX acp);
AFILEID amd_find_end_afile(ACPCTX acp);
int amd_move_to_prev_page(ACPCTX acp);
int amd_move_to_next_page(ACPCTX acp);
AFILEID amd_get_selected_afile(ACPCTX acp, int offset);

int amd_get_afile_count(ACPCTX acp);
AFILE_INFO_T *amd_new_afile_list(ACPCTX acp, AFILEID start, AFILEID end);
int amd_free_afile_list(AFILE_INFO_T *afile_list);
int amd_set_play_afile(ACPCTX acp, AFILEID id);

#endif

