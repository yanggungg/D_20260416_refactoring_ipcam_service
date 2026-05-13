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
 * @file  libivcam.h
 * @brief  This file contains stuctures and definitions for
 *  ivca meta data library.
 * @author  Jongbin Yim.
 * @date  2013/02/25
 */

#ifndef	_LIBIVCAM_
#define	_LIBIVCAM_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "ivca_def.h"
#include "itx_ai_def.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	IVCAM_MAX_PTS_PER_OBJ	64

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

#pragma	pack(push, 1)

typedef struct _ivcam_obj_t {
	ivca_u16_t npts;		/**< Number of points in traj[]. */
	ivca_u16_t hvalid;		/**< Flag to indicate histogram is valid or not. */
	ivca_point_t traj[IVCAM_MAX_PTS_PER_OBJ];	/**< Trajectory information. */
	ivca_u16_t hdata[IVCA_NRBIN_HSVHIST];		/**< HSV histogram data. */
	ivca_meta_obj_t mobj;	/**< Associated meta object. */
} ivcam_obj_t;

typedef struct _ai_obj_t {
	unsigned short	 npts;		/**< Number of points in traj[]. */
	unsigned short	 hvalid;		/**< Flag to indicate histogram is valid or not. */
	double 			 bbx_traj[IVCAM_MAX_PTS_PER_OBJ][2];	/**< Trajectory information. */
	unsigned short	 hdata[IVCA_NRBIN_HSVHIST];		/**< HSV histogram data. */
	ai_meta_obj_t 	 mobj;	/**< Associated meta object. */
} ai_obj_t;

#pragma	pack(pop)

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function prototypes                                                         *
 ******************************************************************************/
ivca_ptr_t ai_ivcam_vca_create(ivca_s32_t id, ivca_s32_t nmaxobjects,
		ivca_s32_t nmaxevents, ivca_rule_t *rules);
void ai_ivcam_vca_release(ivca_ptr_t hVca);

ivca_s32_t ai_ivcam_vca_process(ivca_ptr_t hVca,
		ai_meta_hdr_t *pAH, ai_meta_ckh_t *pAC);

ivca_s32_t ai_ivcam_vca_set_rules(ivca_ptr_t hVca, ivca_rule_t *rules);

ivca_s32_t ai_ivcam_vca_get_trackinfo(ivca_ptr_t hVca,
		ivca_s32_t nmax, ai_obj_t *pObjects);

ivca_s32_t ai_ivcam_vca_get_events(ivca_ptr_t hVca,
		ivca_s32_t *pcount, ai_rule_event_t **pevents);

ivca_s32_t ai_ivcam_vca_set_calib(ivca_ptr_t hVca, ivca_calib_t *calib);

ivca_s32_t ai_ivcam_vca_set_opt(ivca_ptr_t hVca, ivca_option_t *opt);

ivca_s32_t ai_ivcam_vca_set_rules_filter(ivca_ptr_t hVca, char *object_filter);


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
ivca_ptr_t ivcam_vca_create(ivca_s32_t id, ivca_s32_t nmaxobjects,
		ivca_s32_t nmaxevents, ivca_rule_t *rules);

/**
 * @brief  Releases a VCA model for meta data.
 *
 * @param[in] hVca  Pointer to the VCA model for meta data to release.
 *
 * @sa  ivcam_vca_create(), ivcam_vca_process()
 */
void ivcam_vca_release(ivca_ptr_t hVca);

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
ivca_s32_t ivcam_vca_process(ivca_ptr_t hVca,
		ivca_meta_hdr_t *pMTH, ivca_meta_ckh_t *pCKH);

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
ivca_s32_t ivcam_vca_set_rules(ivca_ptr_t hVca, ivca_rule_t *rules);

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
ivca_s32_t ivcam_vca_get_trackinfo(ivca_ptr_t hVca,
		ivca_s32_t nmax, ivcam_obj_t *pObjects);

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
ivca_s32_t ivcam_vca_get_events(ivca_ptr_t hVca,
		ivca_s32_t *pcount, ivca_rule_event_t **pevents);

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
ivca_s32_t ivcam_vca_get_trackedcount(ivca_ptr_t hVca);

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
ivca_u32_t ivcam_vca_get_toteventcount(ivca_ptr_t hVca);

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
		ivca_rule_event_t **pevents);

#ifdef __cplusplus
}
#endif

#endif	/* _LIBIVCAM_ */

