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
2013/05/18 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  libicalib.h
 * @brief  This file contains stuctures and definitions for
 *  the camera calibration.
 * @author  Jongbin Yim.
 * @date  2013/05/18
 */

#ifndef	_LIBICALIB_H_
#define	_LIBICALIB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "ivca_def.h"

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

/**
 * @brief  Performs camera calibration.
 *
 * @param[in] calib  Pointer to the camera calibration data.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 */
int icalib_process(ivca_calib_t *calib);

/**
 * @brief  Computes head locations of calibration targets on the image plane.
 *
 * @param[in] calib  Pointer to the camera calibration data.
 * @param[out] headloc  Pointer to the array of head locations.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 */
int icalib_compute_headloc(ivca_calib_t *calib, ivca_point_t *headloc);

/**
 * @brief  Computes and generates top view image.
 *
 * @param[in] calib  Pointer to the camera calibration data.
 * @param[in] in  Pointer to the input image.
 * @param[out] out  Pointer to the output image.
 * @param[in] outw  Width of the output image.
 * @param[in] outh  Height of the output image.
 * @param[out] range  Optional Pointer to store the output range.
 *  (x min, x max, y min, y max)
 * @param[out] gndp  Optional pointer to the array of ground points of
 *  foot locations.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 */
int icalib_compute_topview(ivca_calib_t *calib,
		void *in, void *out, int outw, int outh, float *range,
		ivca_point_t *gndp);

#ifdef __cplusplus
}
#endif

#endif	/* _LIBICALIB_H_ */

