/*
 * scm_van.c
 * 	- scm video analytics
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jul 7, 2014
 *
 */

#include "scm.h"
#include "scm_internal.h"
#include "iux_afx.h"
#include "nf_common.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_VAN"

////////////////////////////////////////////////////////////
//
// private data type 
//

static int _set_zone_list(ivca_zone_t *zlist, VCAZone *zone)
{
	int i;

	zlist->id			= zone->id;
	zlist->type			= zone->type;
	zlist->active		= zone->active;
	zlist->enabled		= zone->enabled;
	zlist->stop_time	= zone->stop_time;
	zlist->abandon_time = zone->abandon_time;
	zlist->remove_time	= zone->remove_time;
	zlist->loiter_time	= zone->loiter_time;
	*(guint *)zlist->ecolor = 0x00ffffff & zone->ecolor;			// not safe
	zlist->ecolor_sens	= zone->ecolor_sens;
	zlist->size_min[0]	= zone->size_min[0];
	zlist->size_min[1]	= zone->size_min[1];
	zlist->size_max[0]	= zone->size_max[0];
	zlist->size_max[1]	= zone->size_max[1];
	zlist->speed_min	= zone->speed_min;
	zlist->speed_max	= zone->speed_max;
	zlist->eclass		= zone->eclass;
	zlist->sensitivity	= zone->sensitivity;
	zlist->fall_time	= zone->fall_time;
	*(guint *)zlist->color  = 0x00ffffff & zone->color;				// not safe
	zlist->npts			= zone->npts;
	for (i = 0; i < IVCA_MAX_PTSPERZONE; ++i) {
		zlist->pt[i].x = zone->pt[i].x;
		zlist->pt[i].y = zone->pt[i].y;
	}
	strcpy(zlist->name, zone->name);

	return 0;
}

static int _set_cntr_list(ivca_cntr_t *clist, VCACntr *cntr)
{
	int i;

	clist->id			= cntr->id;
	clist->type			= cntr->type;
	clist->active		= cntr->active;
	clist->enabled		= cntr->enabled;
	clist->zid_up		= cntr->zid_up;
	clist->zid_dn		= cntr->zid_dn;
	clist->evalue		= cntr->evalue;
	clist->resetalert	= cntr->resetalert;
	*(guint *)clist->color  = 0x00ffffff & cntr->color;				// not safe
	for (i = 0; i < 4; ++i) {
		clist->pt[i].x = cntr->pt[i].x;
		clist->pt[i].y = cntr->pt[i].y;
	}
	strcpy(clist->name, cntr->name);

	return 0;
}

static int _import_dal_rule(VCARule *rule)
{
	int i;
	ivca_rule_t *pr = &iscm.ssrule;

	memset(&iscm.ssrule, 0x00, sizeof(ivca_rule_t));
	
	pr->n_width = 1920 * 2; 
	pr->n_height = 1080 * 2; 
	pr->nzones = rule->zonelist.cnt;
	pr->ncntrs = rule->cntrlist.cnt;

	for (i = 0; i < pr->nzones; i++)
		_set_zone_list(&pr->zonelist[i], &rule->zonelist.zone[i]);		

	for (i = 0; i < pr->ncntrs; i++)
		_set_cntr_list(&pr->cntrlist[i], &rule->cntrlist.cntr[i]);

	return 0;
}

static int _is_analyzing()
{
	if (iscm.ssrule.n_width == 0) return 0;
	return 1;
}

static int _start_analyzing(time_t from, time_t to)
{
	nf_smart_search_set_rule(NUM_ACTIVE_CH, &iscm.ssrule,from,to);
	nf_meta_data_display_playback_on();
	nf_meta_data_reset_cnt();
	return 0;
}

static int _stop_analyzing()
{
	nf_meta_data_reset_cnt();
	nf_meta_data_display_playback_off();
	nf_smart_search_reset(1, NULL);	
	memset(&iscm.ssrule, 0x00, sizeof(ivca_rule_t));
	return 0;
}

////////////////////////////////////////////////////////////////
//
// protected interfaces
//


////////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_analyze_video_range_ivca(int ch, time_t from, time_t to, ivca_rule_t *rule)
{
	return 0;
}

int scm_analyze_video_range_dal(int ch, time_t from, time_t to, VCARule *rule)
{
	if (_is_analyzing()) return -1;

    g_message("%s called", __FUNCTION__);	
	_import_dal_rule(rule);
	_start_analyzing(from,to);
	return 0;
}

int scm_analyze_video_range_db(int ch, time_t from, time_t to, int idx)
{
	return 0;
}

int scm_analyze_video_file(int ch, char *path) 
{
	return 0;
}

int scm_analyze_video_url(int ch, char *url) 
{
	return 0;
}

int scm_cancel_analyze()
{
	if (!_is_analyzing()) return -1;

    g_message("%s called", __FUNCTION__);	
	_stop_analyzing();
	return 0;
}

