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
 * @file  ivcam_rule.h
 * @brief  This file contains stuctures and definitions for rule detection
 *  from meta data.
 * @author  Jongbin Yim.
 * @date  2013/02/18
 */

#ifndef	_IVCAM_RULE_H_
#define	_IVCAM_RULE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "ivca_def.h"
#include "ivcam_tracker.h"

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
 * function prototypes                                                         *
 ******************************************************************************/


ivca_s32_t ai_ivcam_rule_detect(ai_ivcam_tracker_t *tracker, ivca_rule_t *rules,
		ivca_s32_t nmaxevents, ai_rule_event_t *eventlist,
		ai_meta_hdr_t *pMTH, ivca_s08_t* object_filter);
/**
 * @brief  Detects rule events for meta data.
 *
 * @param[in] tracker  Pointer to the meta data tracker.
 * @param[in] rules  Pointer to the rule for detection.
 * @param[in] nmaxevents  Maximum number of events.
 * @param[out] eventlist  Output buffer for rule event list.
 *  Caller must provide this buffer.
 * @param[in] pMTH  Pointer to the meta data header.
 *
 * @return  Non negative detected number of events. 0 if there is no event.
 *  If detected > nmaxevents, this function will return nmaxevents.
 */
ivca_s32_t ivcam_rule_detect(ivcam_tracker_t *tracker, ivca_rule_t *rules,
		ivca_s32_t nmaxevents, ivca_rule_event_t *eventlist,
		ivca_meta_hdr_t *pMTH);

#ifdef __cplusplus
}
#endif

#endif	/* _IVCAM_RULE_H_ */

