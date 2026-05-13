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
 * @file  ivcam_tracker.h
 * @brief  This file contains stuctures and definitions of the meta data
 *  tracker for the itx vca.
 * @author  Jongbin Yim.
 * @date  2013/02/18
 */

#ifndef	_IVCAM_TRACKER_H_
#define	_IVCAM_TRACKER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "ivca_def.h"
#include "itx_ai_def.h"
#include "list.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	NR_REFP_HIST		64

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/
#define	FFRAC(q)		(1 << (q))
#define	FIXED(a, q)		(int)((a) * FFRAC(q))
#define	FMUL(x, y, q)	(((x) * (y)) >> (q))
#define	FCONVD(x, q)	((x) >> (q))
#define M_PI 3.141592653
#define	Q_ST			4
#ifndef	abs
#define	abs(a)			((a) > 0 ? (a) : -(a))
#endif
#define	SPEED3D_MAX			255

#ifndef	min
#define	min(x, y)		((x) < (y) ? (x) : (y))
#endif
#define	FCONVI(x, q)	((x) << (q))

/**
 * @brief  Matrix structure.
 */
typedef struct _ivca_mat_t {
	ivca_s32_t nrows;		/**< Number of rows. */
	ivca_s32_t ncols;		/**< Number of columns. */
	ivca_s32_t **mdata;		/**< Data pointer for each row. */
	ivca_s32_t *rdata;			/**< Raw data. */
} ivca_mat_t;


#define	MAT_SIZE(R, C)	(sizeof(ivca_mat_t) + sizeof(ivca_mat_t *) * (R)\
		+ sizeof(ivca_s32_t) * (R) * (C))

/**
 * @brief  Zone state for an object. (Used by tracker internally)
 */
typedef struct _ivcam_zone_state_t {
	ivca_u32_t state;		/**< On/off for each event type. */
	/* Used for debouncing. */
	ivca_u16_t cnt_on;		/**< Consecutive ON count. */
	ivca_u16_t cnt_off;		/**< Consecutive OFF count. */
} ivcam_zone_state_t;

/**
 * @brief  Descriptor for track objects. (internal)
 */
typedef struct _ivcam_track_obj_t {
	struct list_head list;
	ivca_u16_t t_head;		/**< Head position in traj[]. */
	ivca_u16_t t_tail;		/**< Tail position in traj[]. */
	ivca_point_t traj[NR_REFP_HIST];	/**< Trajectory: circular queue. */
	ivca_u32_t fn_first;	/**< Frame number of the first frame. */
	ivca_u32_t fn_last;		/**< Frame number of the last frame. */
	ivca_u08_t hvalid;		/**< Flag to indicate histogram is valid or not. */
	ivca_u08_t reserved[3];
	ivca_u16_t hdata[IVCA_NRBIN_HSVHIST];		/* HSV histogrm data. */
	ivca_meta_obj_t mobj;	/**< Meta Object. */
	ivcam_zone_state_t zstate[IVCA_MAX_ZONES];	/**< Zone states. */
	ivca_u16_t zevent_count;/**< Count of detected zone events.
							  * Increased for an enabled event. */
	ivca_u08_t npts;		/**< Number of valid points in traj[]. */
	ivca_u08_t match;		/**< Indicates whether the object is matched or
							  * not in the current frame. */
} ivcam_track_obj_t;

/**
 * @brief  Meta data tracker structure.
 */
typedef struct _ivcam_tracker_t {
	ivca_s32_t size;		/**< Size of the tracker. */
	ivca_s32_t id;			/**< Tracker id. */
	ivca_s32_t nmaxobjs;	/**< Maximum number of objects in a frame. */
	ivca_s32_t tcount;		/**< Currently being tracked object count. */
	struct list_head tlist;
	ivca_s32_t totcount;	/**< Total number of tracked objects. */
	ivca_meta_obj_t **mtbl;	/**< A table for matching. Stores pointer of the
							  * metadata of objects for the current frame. */
} ivcam_tracker_t;

typedef struct _ai_ivcam_zone_state_t {
	ivca_u32_t state;		/**< On/off for each event type. */
	/* Used for debouncing. */
	ivca_u16_t cnt_on;		/**< Consecutive ON count. */
	ivca_u16_t cnt_off;		/**< Consecutive OFF count. */
	ivca_u32_t loitering_start;
	ivca_u32_t stopped_start;
	ivca_s16_t stopped_start_x;
	ivca_s16_t stopped_start_y;
}ai_ivcam_zone_state_t;

typedef struct _ai_static_region {
	struct list_head list;
	ivca_s16_t x;			/**< Left most x coordinate of the rectangle. */
	ivca_s16_t y;			/**< Top most y coordinate of the rectangle. */
	ivca_s16_t w;			/**< Width of the rectangle. */
	ivca_s16_t h;			/**< Height of the rectangle. */
}ai_static_region_t;


typedef struct _ai_ivcam_track_obj_t {
	struct list_head list;
	ivca_u16_t t_head;		/**< Head position in traj[]. */
	ivca_u16_t t_tail;		/**< Tail position in traj[]. */
	ivca_point_t traj[NR_REFP_HIST];	/**< Trajectory: circular queue. */
	ivca_u32_t fn_first;	/**< Frame number of the first frame. */
	ivca_u32_t fn_last;		/**< Frame number of the last frame. */
	ivca_u08_t hvalid;		/**< Flag to indicate histogram is valid or not. */
	ivca_u08_t reserved[3];
	ivca_u16_t hdata[IVCA_NRBIN_HSVHIST];		/* HSV histogrm data. */
	ai_meta_obj_t mobj;	/**< Meta Object. */
	ai_ivcam_zone_state_t zstate[IVCA_MAX_ZONES];	/**< Zone states. */
	ivca_u16_t zevent_count;/**< Count of detected zone events.
							  * Increased for an enabled event. */
	ivca_u08_t npts;		/**< Number of valid points in traj[]. */
	ivca_u08_t match;		/**< Indicates whether the object is matched or
							  * not in the current frame. */
			
	ivca_point_t cent;		/**< Center of the bounding rectangle. */	
	ivca_point_t old_cent;		/**< Center of the bounding rectangle. */				  	
	ivca_rect_t rect;		/**< Bounding rectangle of the object. */

	/* Information on the ground plane. */
	ivca_s32_t gx;				/**< Ground point location on the ground plane. */
	ivca_s32_t gy;				/**< in meters, Q.8 format. */
	ivca_s32_t gvx;			/**< Speed on the ground plane. */
	ivca_s32_t gvy;			/**< in meters/sec, Q.8 format. */
	ivca_s32_t gw;				/**< Width on the ground plane,
							  * in meters, Q.8 format. */
	ivca_s32_t gh;				/**< Maximum possible height in the 3D space,
							  * in meters, Q.8 format. */
	ivca_u32_t gspeed;		/**< Speed on the ground plane,
							  * in km/h, Q.8 format. */
	
	ivca_point_t gndp;		/**< Ground point of the bounding rectangle. */
							  
	/* Used for prediction. */
	ivca_s32_t vx;				/**< x-dir. speed estimation for Kalman init. */
	ivca_s32_t vy;				/**< y-dir. speed estimation for Kalman init. */
	ivca_u32_t timestamp;
	ivca_u32_t timestampl;
	ivca_u32_t gt;	//timestamp gap
	ivca_u32_t static_check_time;
	ivca_u08_t static_object;
	ivca_u32_t static_del_time;
	ivca_u08_t removed;
	ivca_u08_t minimum_size;
} ai_ivcam_track_obj_t;

typedef struct _ai_ivcam_tracker_t {
	ivca_s32_t size;		/**< Size of the tracker. */
	ivca_s32_t id;			/**< Tracker id. */
	ivca_s32_t nmaxobjs;	/**< Maximum number of objects in a frame. */
	ivca_s32_t tcount;		/**< Currently being tracked object count. */
	struct list_head tlist;
	ivca_s32_t totcount;	/**< Total number of tracked objects. */
	ai_meta_obj_t **mtbl;	/**< A table for matching. Stores pointer of the
							  * metadata of objects for the current frame. */
	ivca_mat_t *Hg;			/**< Homography from ground to image plane. */
	ivca_mat_t *Hginv;		/**< Homography from image to ground plane. */
	ivca_point_t uv0;		/**< Principal point. */
	ivca_s32_t rho[4];			/**< Precomputed rho vector * rho_s. */
	ivca_s32_t rho_s;			/**< Scale factor for rho. */

	ivca_s32_t rx;			/**< Ratio of normalized/input width. */
	ivca_s32_t ry;			/**< Ratio of normalized/input height. */
	
	/* static object */
	struct list_head list_static;
	ivca_s32_t static_cnt;
} ai_ivcam_tracker_t;

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function prototypes                                                         *
 ******************************************************************************/


ai_ivcam_tracker_t *ai_ivcam_tracker_create(ivca_s32_t id, ivca_s32_t nmaxobjs);
ivca_s32_t ai_ivcam_tracker_release(ai_ivcam_tracker_t *tracker);
ivca_s32_t ai_ivcam_tracker_process(ai_ivcam_tracker_t *tracker,
		ai_meta_hdr_t *pMTH, ai_meta_ckh_t *pCKH, ivca_option_t *opt);
ivca_s32_t ai_tracker_set_homography(ai_ivcam_tracker_t *tracker, ivca_calib_t *calib);


/**
 * @brief  Creates a meta data tracker.
 *
 * @param[in] id  Tracker id. (i.e. channel number)
 * @param[in] nmaxobjs  Maximum number of tracking objects in a frame.
 *
 * @return
 *  - Pointer to the created meta data tracker if successful.
 *  - NULL if failed.
 *
 * @sa  ivcam_tracker_release(), ivcam_tracker_process()
 */
ivcam_tracker_t *ivcam_tracker_create(ivca_s32_t id, ivca_s32_t nmaxobjs);

/**
 * @brief  Releases a meta data tracker.
 *
 * @param[in] tracker  Pointer to the meta data tracker to release.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 *
 * @sa  ivcam_tracker_create(), ivcam_tracker_process()
 */
ivca_s32_t ivcam_tracker_release(ivcam_tracker_t *tracker);

/**
 * @brief  Performs meta data tracking for the current frame.
 *
 * @param[in] tracker  Pointer to the meta data tracker.
 * @param[in] pMTH  Pointer to the meta data header.
 * @param[in] pCKH  Pointer to the object chunk header for the current frame.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 *
 * @sa  ivcam_tracker_create(), ivcam_tracker_release()
 */
ivca_s32_t ivcam_tracker_process(ivcam_tracker_t *tracker,
		ivca_meta_hdr_t *pMTH, ivca_meta_ckh_t *pCKH);

#ifdef __cplusplus
}
#endif

#endif	/* _IVCAM_TRACKER_H_ */

