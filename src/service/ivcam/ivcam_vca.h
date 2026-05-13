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
2013/02/18 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  ivcam_vca.h
 * @brief  This file contains stuctures and definitions for
 *  the Video Contents Analysis for meta data.
 * @author  Jongbin Yim.
 * @date  2013/02/18
 */

#ifndef	_IVCAM_VCA_H_
#define	_IVCAM_VCA_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "libivcam.h"
#include "ivcam_tracker.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/**
 * @brief  VCA model for meta data.
 */
typedef struct _ivcam_vca_t {
	ivcam_tracker_t *tracker;	/**< Tracker for the VCA model. */
	ivca_rule_t *rules;		/**< Associated rules. */
	ivca_s32_t nmaxevents;	/**< Maximum number of output events in a frame. */
	ivca_s32_t nevents;		/**< Number of detected events in
							  * the current frame. */
	ivca_s32_t nstatics;
	ivca_u32_t ntotevents;	/**< Total number of detected events. */
	ivca_rule_event_t *eventlist;	/**< Buffer for detected rule events. */
} ivcam_vca_t;

typedef struct _ai_ivcam_vca_t {
	ai_ivcam_tracker_t *tracker;	/**< Tracker for the VCA model. */
	ivca_rule_t *rules;		/**< Associated rules. */
	ivca_calib_t calib;			/**< Associated camera calibration data. */
	ivca_option_t opt;			/**< Associated camera calibration data. */
	ivca_s32_t nmaxevents;	/**< Maximum number of output events in a frame. */
	ivca_s32_t nevents;		/**< Number of detected events in
							  * the current frame. */
	ivca_s32_t nstatics;
	ivca_u32_t ntotevents;	/**< Total number of detected events. */
	ai_rule_event_t *eventlist;	/**< Buffer for detected rule events. */
	ivca_s08_t object_filter[IVCA_MAX_ZONES][256];
} ai_ivcam_vca_t;

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
 * function prototypes                                                         *
 ******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* _IVCAM_VCA_H_ */

