/*
 * scm_record.c
 * 	- scm record service
 *	- dependencies :
 *		
 *
 * Written by eddy.kim
 * Copyright (c) ITX security, Mar 5, 2011
 *
 */

#include "vfs.h"

#include "nf_api_disk.h"
#include "scm_internal.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_REC"


////////////////////////////////////////////////////////////
//
// public interfaces
//

gboolean scm_start_panic_record()
{
	return _scm_work_panicrec_start(&iscm);
}

gboolean scm_stop_panic_record()
{
	return _scm_work_panicrec_stop(&iscm);
}

gboolean scm_toggle_panic_record()
{
	return _scm_work_panicrec_toggle(&iscm);
}

gboolean scm_get_panic_record()
{
	return nf_panic_record_is_set(&iscm);
}

gboolean scm_get_record_time(time_t *first, time_t *last)
{
	NF_DISK_REC_TIME rec_t;

	if (first) *first = 0;
	if (last) *last = 0;
	if (!nf_disk_rec_time(0, &rec_t, NULL)) return FALSE;
	if (first) *first = rec_t.min_time;
	if (last) *last = rec_t.max_time;
	
	DMSG(1, "Min:%u, Max:%u\n", rec_t.min_time, rec_t.max_time);
	return TRUE;
}

