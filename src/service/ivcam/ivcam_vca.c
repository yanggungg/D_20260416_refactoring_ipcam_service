// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2013 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2013/02/25 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  ivcam_vca.c
 * @brief  This file contains implementation of the
 *  Video Contents Analysis for meta data APIs.
 * @author  Jongbin Yim.
 * @data  2013/02/25
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "libivcam.h"
#include "ivcam_tracker.h"
#include "ivcam_rule.h"
#include "ivcam_vca.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

ivca_ptr_t 
ai_ivcam_vca_create(ivca_s32_t id, ivca_s32_t nmaxobjects,
		ivca_s32_t nmaxevents, ivca_rule_t *rules)
{
	ai_ivcam_vca_t *vca;

	vca = (ai_ivcam_vca_t *)calloc(1, sizeof(*vca));
	if ( !vca )
		goto failed;

	vca->tracker = ai_ivcam_tracker_create(id, nmaxobjects);
	if ( !vca->tracker )
		goto failed;

	if ( nmaxevents ) {
		vca->eventlist = (ai_rule_event_t *)calloc(nmaxevents,
				sizeof(ai_rule_event_t));
		if ( !vca->eventlist )
			goto failed;
	}

	vca->nmaxevents = nmaxevents;
	vca->rules = rules;

	return vca;

failed:
	if ( vca )
		ai_ivcam_vca_release(vca);
	return NULL;
}

void
ai_ivcam_vca_release(ivca_ptr_t hVca)
{
	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;

	if ( vca->tracker )
		ai_ivcam_tracker_release(vca->tracker);
	if ( vca->eventlist )
		free(vca->eventlist);
	free(vca);
}	

ivca_s32_t
ai_ivcam_vca_process(ivca_ptr_t hVca, ai_meta_hdr_t *pMTH, ai_meta_ckh_t *pCKH)
{
	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;

	if ( ai_ivcam_tracker_process(vca->tracker, pMTH, pCKH,&(vca->opt)) < 0 )
		return -1;

	if ( vca->rules && vca->eventlist ) {
		vca->nevents = ai_ivcam_rule_detect(vca->tracker, vca->rules,
				vca->nmaxevents, vca->eventlist, pMTH,vca->object_filter);
		vca->ntotevents += vca->nevents;
	}
	return 0;
}	/* ivcam_vca_process(... */

ivca_s32_t
ai_ivcam_vca_get_trackinfo(ivca_ptr_t hVca, ivca_s32_t nmax, ai_obj_t *pObjects)
{
	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;
	ivca_s32_t i, n, count = 0;
	ai_ivcam_track_obj_t *obj;

	if ( !vca->tracker || vca->tracker->size != sizeof(*vca->tracker) )
		return -1;
	
	list_for_each_entry(obj, &vca->tracker->tlist, ai_ivcam_track_obj_t, list) {
		if(obj->match == 0 && obj->removed == 0)
			continue;
		for (n = 0, i = obj->t_head;
				i != obj->t_tail && n < IVCAM_MAX_PTS_PER_OBJ; n++) {
			//captainnn		
			//pObjects[count].traj[n] = obj->traj[i];
			pObjects[count].bbx_traj[n][0] = (((double)obj->traj[i].x)/3840);
			pObjects[count].bbx_traj[n][1] = (((double)obj->traj[i].y)/2160);
			i = (i + 1) % NR_REFP_HIST;
		}
		pObjects[count].npts = n;
		pObjects[count].hvalid = obj->hvalid;
		if ( obj->hvalid )
			memcpy(pObjects[count].hdata, obj->hdata, sizeof(obj->hdata));
		pObjects[count].mobj = obj->mobj;
		if ( vca->rules )
			pObjects[count].mobj.nevents = obj->zevent_count;
		if ( ++count >= nmax )
			break;
	}

	return count;
}	

ivca_s32_t
ai_ivcam_vca_get_events(ivca_ptr_t hVca,
		ivca_s32_t *pcount, ai_rule_event_t **pevents)
{
	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;

	if ( !pcount || !pevents || !vca->eventlist )
		return -1;

	*pcount = vca->nevents;
	*pevents = vca->eventlist;
	return *pcount;
}	/* ivcam_vca_get_events(... */


ivca_s32_t 
ai_ivcam_vca_set_calib(ivca_ptr_t hVca, ivca_calib_t *calib)
{

	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;
	
	if ( calib )
		memcpy(&vca->calib, calib, sizeof(vca->calib));
	else
		memset(&vca->calib, 0, sizeof(vca->calib));
	ai_tracker_set_homography(vca->tracker, calib);
	return 0;
}

ivca_s32_t 
ai_ivcam_vca_set_opt(ivca_ptr_t hVca, ivca_option_t *opt)
{

	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;
	
	if ( opt )
		memcpy(&vca->opt, opt, sizeof(vca->opt));
	else
		memset(&vca->opt, 0, sizeof(vca->opt));
	return 0;
}


ivca_s32_t 
ai_ivcam_vca_set_rules_filter(ivca_ptr_t hVca, char *object_filter)
{

	ai_ivcam_vca_t *vca = (ai_ivcam_vca_t *)hVca;

	memcpy(vca->object_filter, object_filter, sizeof(vca->object_filter));
}


/**
 * @brief  Creates a VCA model for meta data.
 *
 * @param[in] id  VCA model id. (i.e. channel number)
 * @param[in] nmaxobjects  Maximum number of objects in a frame.
 * @param[in] nmaxevents  Maximum number of events in a frame.
 * @param[in] rules  Pointer to rules.
 *
 * @return
 *  - Pointer to the created VCA model for meta data if successful.
 *  - NULL if failed.
 *
 * @remark  If the rule is not specified(NULL), the vca behaves as a simple
 *  tracker and do nothing on ivca_meta_obj_t. On the other hand, if the rule
 *  is specified, the vca detects events based on the rule and the results are
 *  reflected in the trackinfo. (ivca_meta_obj_t::nevents)
 *  The rule can be specified in runtime via ivcam_vca_set_rules().
 *
 * @sa  ivcam_vca_release(), ivcam_vca_process()
 */
ivca_ptr_t
ivcam_vca_create(ivca_s32_t id, ivca_s32_t nmaxobjects,
		ivca_s32_t nmaxevents, ivca_rule_t *rules)
{
	ivcam_vca_t *vca;

	vca = (ivcam_vca_t *)calloc(1, sizeof(*vca));
	if ( !vca )
		goto failed;

	vca->tracker = ivcam_tracker_create(id, nmaxobjects);
	if ( !vca->tracker )
		goto failed;

	if ( nmaxevents ) {
		vca->eventlist = (ivca_rule_event_t *)calloc(nmaxevents,
				sizeof(ivca_rule_event_t));
		if ( !vca->eventlist )
			goto failed;
	}

	vca->nmaxevents = nmaxevents;
	vca->rules = rules;

	return vca;

failed:
	if ( vca )
		ivcam_vca_release(vca);
	return NULL;
}	/* ivcam_vca_create(... */

/**
 * @brief  Releases a VCA model for meta data.
 *
 * @param[in] hVca  Pointer to the VCA model for meta data to release.
 *
 * @sa  ivcam_vca_create(), ivcam_vca_process()
 */
void
ivcam_vca_release(ivca_ptr_t hVca)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	if ( vca->tracker )
		ivcam_tracker_release(vca->tracker);
	if ( vca->eventlist )
		free(vca->eventlist);
	free(vca);
}	/* ivcam_vca_release(... */

/**
 * @brief  Processes meta data for a frame.
 *
 * @param[in] hVca  Pointer to the VCA model for meta data.
 * @param[in] pMTH  Pointer to the meta data header.
 * @param[in] pCKH  Pointer to the object chunk header for the current frame.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 *
 * @sa  itxv_vca_create(), itxv_vca_release()
 */
ivca_s32_t
ivcam_vca_process(ivca_ptr_t hVca, ivca_meta_hdr_t *pMTH, ivca_meta_ckh_t *pCKH)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	if ( ivcam_tracker_process(vca->tracker, pMTH, pCKH) < 0 )
		return -1;

	if ( vca->rules && vca->eventlist ) {
		vca->nevents = ivcam_rule_detect(vca->tracker, vca->rules,
				vca->nmaxevents, vca->eventlist, pMTH);
		vca->ntotevents += vca->nevents;
	}
	return 0;
}	/* ivcam_vca_process(... */

/**
 * @brief  Processes meta data for a frame for fast smart search.
 *
 * @param[in] hVca  Pointer to the VCA model for meta data.
 * @param[in] pMTH  Pointer to the meta data header.
 * @param[in] pCKH  Pointer to the object chunk header for the current frame.
 * @param[out] pcount  Pointer to store the number of events.
 * @param[out] pevents  Pointer to store the event list pointer.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 *
 * @sa  itxv_vca_create(), itxv_vca_release()
 */
ivca_s32_t ivcam_vca_process_for_smart_search(ivca_ptr_t hVca,
		ivca_meta_hdr_t *pMTH, ivca_meta_ckh_t *pCKH,
		ivca_rule_event_t **pevents)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	if ( ivcam_tracker_process(vca->tracker, pMTH, pCKH) < 0 )
		return -1;

	if ( vca->rules && vca->eventlist ) {
		vca->nevents = ivcam_rule_detect(vca->tracker, vca->rules,
				vca->nmaxevents, vca->eventlist, pMTH);
		vca->ntotevents += vca->nevents;
	}

	if (!pevents || !vca->eventlist )
		return -2;

	*pevents = vca->eventlist;
	return vca->nevents;
}

/**
 * @brief  Sets the VCA rules for meta data.
 *
 * @param[in] vca  Pointer to the VCA model for meta data.
 * @param[in] rules  Pointer to rules.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 */
ivca_s32_t
ivcam_vca_set_rules(ivca_ptr_t hVca, ivca_rule_t *rules)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	vca->rules = rules;
	return 0;
}	/* ivcam_vca_set_rules(... */

/**
 * @brief  Gets tracking information of a VCA model for meta data.
 *
 * @param[in] hVca  Pointer to the VCA model for meta data.
 * @param[in] nmax  Maximum number of objects.
 * @param[out] pObjects  Output buffer for object list.
 *  Caller must provide this buffer. (nmax * sizeof(ivcam_obj_t))
 *
 * @return
 *  - Number of tracking objects if successful.
 *  - -1 if failed.
 */
ivca_s32_t
ivcam_vca_get_trackinfo(ivca_ptr_t hVca, ivca_s32_t nmax, ivcam_obj_t *pObjects)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;
	ivca_s32_t i, n, count = 0;
	ivcam_track_obj_t *obj;

	if ( !vca->tracker || vca->tracker->size != sizeof(*vca->tracker) )
		return -1;

	list_for_each_entry(obj, &vca->tracker->tlist, ivcam_track_obj_t, list) {
		for (n = 0, i = obj->t_head;
				i != obj->t_tail && n < IVCAM_MAX_PTS_PER_OBJ; n++) {
			pObjects[count].traj[n] = obj->traj[i];
			i = (i + 1) % NR_REFP_HIST;
		}
		pObjects[count].npts = n;
		pObjects[count].hvalid = obj->hvalid;
		if ( obj->hvalid )
			memcpy(pObjects[count].hdata, obj->hdata, sizeof(obj->hdata));
		pObjects[count].mobj = obj->mobj;
		if ( vca->rules )
			pObjects[count].mobj.nevents = obj->zevent_count;
		if ( ++count >= nmax )
			break;
	}

	return count;
}	/* ivcam_vca_get_trackinfo(... */

/**
 * @brief  Gets event data of a VCA model for meta data.
 *
 * @param[in] hVca  Pointer to the VCA model for meta data.
 * @param[out] pcount  Pointer to store the number of events.
 * @param[out] pevents  Pointer to store the event list pointer.
 *  Caller must provide this buffer. (nmax * sizeof(ivca_rule_event_t))
 *
 * @return
 *  - Number of events if successful.
 *  - -1 if failed.
 */
ivca_s32_t
ivcam_vca_get_events(ivca_ptr_t hVca,
		ivca_s32_t *pcount, ivca_rule_event_t **pevents)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	if ( !pcount || !pevents || !vca->eventlist )
		return -1;

	*pcount = vca->nevents;
	*pevents = vca->eventlist;
	return *pcount;
}	/* ivcam_vca_get_events(... */

/**
 * @brief  Gets the total number of tracked objects of a VCA model for
 *  meta data.
 *
 * @param[in] vca  Pointer to the VCA model for meta data.
 *
 * @return
 *  - Total number of tracked object until now if successful.
 *  - -1 if failed.
 */
ivca_s32_t
ivcam_vca_get_trackedcount(ivca_ptr_t hVca)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	if ( !vca->tracker || vca->tracker->size != sizeof(*vca->tracker) )
		return -1;
	return vca->tracker->totcount;
}	/* ivcam_vca_get_trackedcount(... */

/**
 * @brief  Gets the total number of detected events of a VCA model for
 *  meta data.
 *
 * @param[in] vca  Pointer to the VCA model for meta data.
 *
 * @return
 *  - Total number of detected events until now if successful.
 *  - -1 if failed.
 */
ivca_u32_t
ivcam_vca_get_toteventcount(ivca_ptr_t hVca)
{
	ivcam_vca_t *vca = (ivcam_vca_t *)hVca;

	return vca->ntotevents;
}	/* ivcam_vca_get_toteventcount(... */

