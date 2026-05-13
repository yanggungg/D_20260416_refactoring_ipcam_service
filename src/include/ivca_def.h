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
2013/02/02 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  ivca_def.h
 * @brief  This file contains stuctures and definitions for the itx vca
 *  protocol & metadata.
 * @author  Jongbin Yim.
 * @date  2013/02/02
 */

#ifndef	_IVCA_DEF_H_
#define	_IVCA_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define EN_REVISED_CFILTER 1

/* Tracking reference mode. */
#define	IVCA_TREF_CENT			0	/**< Centroid. */
#define	IVCA_TREF_GNDP			1	/**< Ground point. */

/* Limit about zones and counters. */
#define	IVCA_MAX_ZONES			16	/**< Maximum number of zones. */
#define	IVCA_MAX_CNTRS			16	/**< Maximum number of counters. */
#define	IVCA_MAX_PTSPERZONE		16	/**< Maximum number of points in a zone. */
#define IVCA_MAX_EVENTS         8

#define	IVCA_MAX_NAME_LEN		32	/**< Maximum number of rule name string
									  * including NULL terminator. */

/* Limit about rule evnent. */
#define	IVCA_MAX_EVENT_CNT		32	/**< Maximum number of event/frame. */

/* Rule types. */
#define	IVCA_RT_LINE			0	/**< Line. */
#define	IVCA_RT_AREA			1	/**< Area. */
#define	IVCA_RT_CNTR			2	/**< Counter. */

/* Event types. */
#define	IVCA_ET_DIR_POS			0x00000001	/**< Crossed positive direction. */
#define	IVCA_ET_DIR_NEG			0x00000002	/**< Crossed negative direction. */
#define	IVCA_ET_ENTER			0x00000010	/**< Entered. */
#define	IVCA_ET_EXIT			0x00000020	/**< Exited. */
#define	IVCA_ET_STOPPED			0x00000040	/**< Stopped. */
#define	IVCA_ET_ABANDONED		0x00000080	/**< Abandoned. */
#define	IVCA_ET_REMOVED			0x00000100	/**< Removed. */
#define	IVCA_ET_LOITERED		0x00000200	/**< Loitered. */
#define	IVCA_ET_FALL			0x00000400	/**< Fall. */
#define	IVCA_ET_COUNTER			0x00004000	/**< Counter value exceeded. */
#define	IVCA_ET_TAMPER			0x00008000	/**< Camera tamper detected. */
#define	IVCA_ET_COLOR			0x00010000	/**< Color filter. */
#define	IVCA_ET_SIZE			0x00020000	/**< Size filter. */
#define	IVCA_ET_CLASS			0x00040000	/**< Class filter. */
#define	IVCA_ET_SPEED			0x00080000	/**< Speed filter. */
#define	IVCA_ET_INTRUSION		0x00100000	/**< Intrusion. */
#define IVCA_ET_FR              0x00200000  /**< Face Reco. */
#define IVCA_ET_LPR             0x00400000  /**< LPR. */
#define IVCA_ET_GENERIC         0x10000000  /**< Generic. */


/* Timeout in seconds: for stopped, abandoned, removed, loitering detection. */
#define	IVCA_ETIMEOUT_MIN		5	/**< Minimum timeout in seconds. */
#define	IVCA_ETIMEOUT_MAX		600	/**< Maximum timeout in seconds. */

#if EN_REVISED_CFILTER == 1
#define	IVCA_ECOLOR_SENS_MAX	2	/**< Maximum color filter sensitivity. */
#else
#define	IVCA_ECOLOR_SENS_MAX	10	/**< Maximum color filter sensitivity. */
#endif

/* Object size. */
#define	IVCA_WIDTH3D_MAX		65535	/**< Maximum object width in the 3D
										  * space, in mm. (=infinity) */
#define	IVCA_HEIGHT3D_MAX		65535	/**< Maximum object height in the 3D
										  * space, in mm. (=infinity) */

/* Object classes. */
#define	IVCA_CLASS_UNKNOWN		0	/**< Unknown. */
#define	IVCA_CLASS_PERSON		1	/**< Person. */
#define	IVCA_CLASS_VEHICLE		2	/**< Vehicle. */
#define	IVCA_CLASS_PEOPLE		3	/**< Group of people. */
#define	IVCA_CLASS_NA			0xFF	/**< Not Applicable. */

/* Object speed. */
#define	IVCA_SPEED3D_MAX		255		/**< Maximum object speed in the 3D
										  * space, in km/h */

/* Static Region types. */
#define	IVCA_SR_UNKNOWN			0	/**< Unknown. */
#define	IVCA_SR_STOPPED			1	/**< Stopped region. */
#define	IVCA_SR_REMOVED			2	/**< Removed region. */

/*
 * About object color histogram.
 * #IVCA_NRBIN_HSVHIST = #IVCA_NRBIN_HSVHIST_C + #IVCA_NRBIN_HSVHIST_G.
 */
#if EN_REVISED_CFILTER == 1
#define	IVCA_NRBIN_HSVHIST_C	6	/**< Number of bins for color in HSV hist.*/
#define	IVCA_NRBIN_HSVHIST_G	2	/**< Number of bins for gray in HSV hist. */
#define	IVCA_NRBIN_HSVHIST		8	/**< Number of bins in the HSV histogram. */
#else
#define	IVCA_NRBIN_HSVHIST_C	24/**< Number of bins for color in HSV hist.*/
#define	IVCA_NRBIN_HSVHIST_G	4	/**< Number of bins for gray in HSV hist. */
#define	IVCA_NRBIN_HSVHIST		28	/**< Number of bins in the HSV histogram. */
#endif

/*
 * About camera calibration.
 */
#define	IVCA_MAX_CALIB_TARGETS	32	/**< Maximum number of
									  * calibration targets. */
#define	IVCA_MIN_CALIB_HEIGHT	50	/**< Minimum height of targets in cm. */
#define	IVCA_MAX_CALIB_HEIGHT	999	/**< Maximum height of targets in cm. */

/*
 * Maximum values of minimum object size in the 3D space.
 */
#define	IVCA_MAX_MINW3D			10000	/**< Maximum of minimum object width
										  * in the 3D space, in mm. */
#define	IVCA_MAX_MINH3D			10000	/**< Maximum of minimum object height
										  * in the 3D space, in mm. */

/* Type of camera change. */
#define IVCA_CAM_AE				0 		/**< Auto Expose. */
#define IVCA_CAM_WB				1		/**< White balnace. */
#define IVCA_CAM_TAMPER			2		/**< Tamper detection. */
#define IVCA_CAM_ZOOM			3		/**< Control zoom. */
#define IVCA_CAM_FOCUS			4		/**< Control focus. */
#define IVCA_CAM_IRIS			5		/**< Control IRIS. */
#define IVCA_CAM_DNN		6		/**< Day and Night change */
#define IVCA_CAM_USER		7		/**< Control image by user. */

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

#pragma	pack(push, 1)

/* Basic types. */
typedef char				ivca_s08_t;	/**< Signed 8bit. */
typedef unsigned char		ivca_u08_t;	/**< Unsigned 8bit. */
typedef short				ivca_s16_t;	/**< Signed 16bit. */
typedef unsigned short		ivca_u16_t;	/**< Unsigned 16bit. */
typedef int					ivca_s32_t;	/**< Signed 32bit. */
typedef unsigned int		ivca_u32_t;	/**< Unsigned 32bit. */
typedef long long			ivca_s64_t;	/**< Signed 64bit. */
typedef unsigned long long	ivca_u64_t;	/**< Unsigned 64bit. */
typedef float				ivca_f32_t;	/**< Float 32bit. */
typedef double				ivca_f64_t;	/**< Double 64bit. */
typedef void *				ivca_ptr_t;	/**< 32bit pointer. */

/**
 * @brief  Point descriptor.
 */
typedef struct _ivca_point_t {
	ivca_s16_t x;			/**< x coordinate. */
	ivca_s16_t y;			/**< y coordinate. */
} ivca_point_t;

/**
 * @brief  Rectangle descriptor.
 */
typedef struct _ivca_rect_t {
	ivca_s16_t x;			/**< Left most x coordinate of the rectangle. */
	ivca_s16_t y;			/**< Top most y coordinate of the rectangle. */
	ivca_s16_t w;			/**< Width of the rectangle. */
	ivca_s16_t h;			/**< Height of the rectangle. */
} ivca_rect_t;

/**
 * @brief  Option descriptor.
 */
typedef struct _ivca_option_t {
	/* Algorithm options. */
	ivca_u08_t en_shadowrm;	/**< Enables(1) or disables(0) shadow removal. */
	ivca_u08_t en_prediction;	/**< Enables(1) or disables(0) prediction. */
	ivca_u08_t en_roi;		/**< Enables(1) or disables(0) ROI. */
	ivca_u08_t en_tamper;	/**< Enables(1) or disables(0) tamper detection. */
	ivca_u08_t en_snapshot;	/**< Enables(1) or disables(0) event snapshot. */
	ivca_u08_t track_ref;	/**< Tracking reference mode: IVCA_TREF_XXXX. */
	ivca_u08_t en_privacy;	/**< Enables(1) or disables(0) privacy masking. */
	ivca_u08_t en_usecalib;	/**< Enables(1) or disables(0) use of calibration
							  * data. (3D information) */
	ivca_u08_t en_night;	/**< Enables(1) or disables(0) use of night mode. */
	ivca_u16_t min_width3d;	/**< Minimum object width in the 3D space, in mm. */
	ivca_u16_t min_height3d;/**< Minimum object height in the 3D space, in mm.*/
	ivca_u08_t en_static_filter;    /** < Enables(1) or disables(0) use of static filter. */
	ivca_u08_t static_filter_sense;	/** < 2 : high 1: mid 0 : low. */

	/* ROI information. */
	ivca_rect_t roi;		/**< ROI rectangle. */

	/* Display options. */
	ivca_u08_t sw_obj_bb;	/**< Show(1) or hide(0) object bounding box. */
	ivca_u08_t sw_obj_id;	/**< Show(1) or hide(0) object ID. */
	ivca_u08_t sw_obj_ar;	/**< Show(1) or hide(0) object area. */
	ivca_u08_t sw_obj_tm;	/**< Show(1) or hide(0) object track time. */
	ivca_u08_t sw_obj_cl;	/**< Show(1) or hide(0) object color information. */
	ivca_u08_t sw_obj_tr;	/**< Show(1) or hide(0) object trajectory. */
	ivca_u08_t sw_obj_w3d;	/**< Show(1) or hide(0) object width in the
							  * 3D space. */
	ivca_u08_t sw_obj_h3d;	/**< Show(1) or hide(0) maximum possible object
							  * height in the 3D space. */
	ivca_u08_t sw_obj_s3d;	/**< Show(1) or hide(0) object speed in the
							  * 3D space. */
	ivca_u08_t sw_rule;		/**< Show(1) or hide(0) rule. */
	ivca_u08_t sw_rule_name;/**< Show(1) or hide(0) rule name. */
	ivca_u08_t sw_roi;		/**< Show(1) or hide(0) ROI. */

	/* Debug options. */
	ivca_u08_t sw_dbg_fg;	/**< Show(1) or hide(0) foreground mask. */
	ivca_u08_t sw_dbg_sh;	/**< Show(1) or hide(0) shadow mask. */
	ivca_u08_t sw_dbg_info;	/**< Show(1) or hide(0) debug information. */

	ivca_u08_t reserved2;	/**< Reserved. Should be set to 0. */
} ivca_option_t;

/**
 * @brief  Zone descriptor.
 */
typedef struct _ivca_zone_t {
	ivca_s16_t id;			/**< Zone Id: 0 ~ (#IVCA_MAX_ZONES - 1). */
	ivca_u08_t type;		/**< Rule type: #IVCA_RT_LINE or #IVCA_RT_AREA. */
	ivca_u08_t active;		/**< Activation state. 0: inactive, 1: active. */
	ivca_u32_t enabled;		/**< Bitwise flag for enabled events. */
	ivca_u16_t stop_time;	/**< Timeout for stopped object detection. */
	ivca_u16_t abandon_time;/**< Timeout for abandoned object detection. */
	ivca_u16_t remove_time;	/**< Timeout for removed object detection. */
	ivca_u16_t loiter_time;	/**< Timeout for loitering object detection. */
	ivca_u08_t ecolor[3];	/**< Color value for the color filter, RGB. */
	ivca_u08_t ecolor_sens;	/**< Sensitivity for the color filter. 0 ~ 2 */
	ivca_u16_t size_min[2];	/**< Min for size filter. [0]:width, [1]:height. */
	ivca_u16_t size_max[2];	/**< Max for size filter. [0]:width, [1]:height. */
	ivca_u16_t speed_min;	/**< Min for speed filter. */
	ivca_u16_t speed_max;	/**< Max for speed filter. */
	ivca_u08_t eclass;		/**< Class type for class filter. */
	ivca_u08_t sensitivity;	/**< Sensitivity level 0 ~ 4. */
	ivca_u16_t fall_time;	/**< Timeout for fallen object detection. */
	ivca_u32_t all_detect_obj ;	/**< all class detect on/off */
	ivca_u32_t c_threshold;		/**< confidence threshold */
	ivca_u08_t color[3];	/**< Display color, RGB respectively. */
	ivca_u08_t npts;		/**< Number of points. (in the polygon) */
	ivca_point_t pt[IVCA_MAX_PTSPERZONE];	/**< Points in the zone. */
	ivca_s08_t name[IVCA_MAX_NAME_LEN];	/**< Zone name,*/
	ivca_s08_t event_audio[IVCA_MAX_EVENTS][256];  
										  /* NULL terminated string. */
} ivca_zone_t;

/**
 * @brief  Counter descriptor.
 */
typedef struct _ivca_cntr_t {
	ivca_s16_t id;			/**< Counter Id: 0 ~ (#IVCA_MAX_CNTRS - 1). */
	ivca_u08_t type;		/**< Rule type: #IVCA_RT_CNTR. */
	ivca_u08_t active;		/**< Activation state. 0: inactive, 1: active. */
	ivca_u32_t enabled;		/**< Bitwise flag for enabled events. */
	/* Counting sources, set to -1 if counting is disabled. */
	ivca_s16_t zid_up;		/**< Zone id for counting up. */
	ivca_s16_t zid_dn;		/**< Zone id for counting down. */
	ivca_s32_t value;		/**< Counter value. */
	ivca_s32_t evalue;		/**< Event value. */
	ivca_u08_t resetalert;	/**< Reset after alert. */
	ivca_u08_t reserved[3];	/**< Reserved. Should be set to 0. */
	ivca_u32_t reserved2;	/**< Reserved. Should be set to 0. */
	ivca_u08_t color[3];	/**< Display color, RGB respectively. */
	ivca_u08_t reserved3;	/**< Reserved. Should be set to 0. */
	ivca_point_t pt[4];		/**< Rectangle for the counter. */
	ivca_s08_t name[IVCA_MAX_NAME_LEN];	/**< Counter name,
										  * NULL terminated string. */
} ivca_cntr_t;

/**
 * @brief  Rule descriptor.
 */
typedef struct _ivca_rule_t {
	ivca_u32_t version;		/**< Version of the rule structure. */
	ivca_u16_t n_width;		/**< Width of the normalized coordinate. */
	ivca_u16_t n_height;	/**< Height of the normalized coordinate. */
	ivca_u16_t nzones;		/**< Number of valid zones in zonelist. */
	ivca_u16_t ncntrs;		/**< Number of valid counters in cntrlist. */
	ivca_u32_t reserved;	/**< Reserved. Should be set to 0. */
	ivca_zone_t zonelist[IVCA_MAX_ZONES];	/**< List of all zones. */
	ivca_cntr_t cntrlist[IVCA_MAX_CNTRS];	/**< List of all counters. */
} ivca_rule_t;

/**
 * @brief  Rule event descriptor.
 */
typedef struct _ivca_rule_event_t {
	ivca_u32_t type;		/**< Type of this event: IVCA_ET_XXXX. */
	ivca_s32_t object_id;	/**< Object id for this event. */
	ivca_s16_t rule_id;		/**< Rule id for this event. */
	ivca_u08_t object_class;/**< Object class: IVCA_CLASS_XXXX. */
	ivca_u08_t ch;			/**< Channel number for this event. */
	ivca_rect_t rc;			/**< Rectangle(bounding box) of the event region. */
	ivca_u32_t timestamp;	/**< Timestamp of this event. */
	ivca_u32_t snap_size;	/**< Size of snapshot image. (in bytes) */
	ivca_ptr_t snapshot;	/**< Extra pointer for snapshot image. */
} ivca_rule_event_t;

/**
 * @brief  Calibration target descriptor.
 */
typedef struct _ivca_calib_target_t {
	ivca_point_t pt[2];		/**< Points of head:[0] and foot:[1]. */
	ivca_s32_t height;		/**< Height of the target in cm. */
} ivca_calib_target_t;

/**
 * @brief  Camera calibration data descriptor.
 */
typedef struct _ivca_calib_t {
	ivca_u16_t p_width;		/**< Width of the video. */
	ivca_u16_t p_height;	/**< Height of the video. */
	ivca_f32_t height;		/**< Camera height, in meters. */
	ivca_f32_t tilt;		/**< Tilt angle, in degrees. */
	ivca_f32_t focal;		/**< Focal length, in pixels. */
	ivca_u16_t paramvalid;	/**< Flag to indicate validity of parameters. */
	ivca_u16_t ntargets;	/**< Number of calibration targets. */
	ivca_calib_target_t targetlist[IVCA_MAX_CALIB_TARGETS];	/**< List of
															  * targets. */
} ivca_calib_t;

/*
 * VCA meta data structure
 * -
 * +----------+--------+---------+--------+---------+-----+\n
 * | meta hdr | ck hdr | ck body | ck hdr | ck body | ... |\n
 * +----------+--------+---------+--------+---------+-----+
 */

/**
 * @brief  Meta data header.
 */
typedef struct _ivca_meta_hdr_t {
	ivca_u32_t signature;	/**< Meta data signature, should be 'META'. */
	ivca_u32_t framenum;	/**< Frame number of processed frame. */
	ivca_u32_t size;		/**< Size of meta data including header. */
	ivca_u32_t rulechange_num;	/**< Number of changing rule.
								 * (Increase number from 0 by 1) */
	ivca_u08_t chid;		/**< Channel id. */
	ivca_u08_t track_ref;	/**< Tracking reference mode: IVCA_TREF_XXXX. */
	ivca_u08_t ckcount;		/**< Number of meta data chunks. */
	ivca_u08_t framerate;	/**< Frame rate of the algorithm. */
	ivca_u16_t n_width;		/**< Width of the normalized coordinate. */
	ivca_u16_t n_height;	/**< Height of the normalized coordinate. */
	ivca_u16_t p_width;		/**< Processed width of the frame. */
	ivca_u16_t p_height;	/**< Processed height of the frame. */
} ivca_meta_hdr_t;

/* Meta data Chunk types. */
#define	IVCA_META_CKTYPE_JNK	0	/**< Junk chunk. (dummy) */
#define	IVCA_META_CKTYPE_OBJ	1	/**< Object chunk. */
#define	IVCA_META_CKTYPE_CNT	2	/**< Counter value chunk. */
#define	IVCA_META_CKTYPE_FGM	3	/**< Foreground mask chunk. */
#define	IVCA_META_CKTYPE_SHM	4	/**< Shadow mask chunk. */
#define	IVCA_META_CKTYPE_DBG	5	/**< Debug chunk. */

/**
 * @brief  Chunk header for meta data.
 */
typedef struct _ivca_meta_ckh_t {
	ivca_u32_t size;		/**< Size of chunk excluding header. */
	ivca_u16_t type;		/**< Meta data chunk type. IVCA_META_CKTYPE_XXXX. */
	ivca_u16_t priv;		/**< Private field, depend on chunk type:
							  * - #IVCA_META_CKTYPE_JNK: Not used.
							  * - #IVCA_META_CKTYPE_OBJ: Number of objects.
							  * - #IVCA_META_CKTYPE_CNT: Number of counters.
							  * - #IVCA_META_CKTYPE_FGM: Not used.
							  * - #IVCA_META_CKTYPE_SHM: Not used.
							  * - #IVCA_META_CKTYPE_DBG: Not used.
							  */
} ivca_meta_ckh_t;


/**
 * @brief  Descriptor for each object in OBJ chunk.
 *  (#IVCA_META_CKTYPE_OBJ)
 *
 * @remark  If the hvalid is set, then HSV histogram is immediately follows the
 *  ivca_meta_obj_t structure. (#IVCA_NRBIN_HSVHIST * sizeof(ivca_u16_t))
 *  -
 *  --+-----------------+--------------+-----------------+-----------------+--\n
 *  ..| obj[A],hvalid=1 | histogram[A] | obj[B],hvalid=0 | obj[C],hvalid=0 |..\n
 *  --+-----------------+--------------+-----------------+-----------------+--\n
 */
typedef struct _ivca_meta_obj_t {
	ivca_u32_t id;			/**< Object id. */
	ivca_u16_t nevents;		/**< Number of triggered events. */
	ivca_u08_t oclass;		/**< Object class. */
	ivca_u08_t hvalid;		/**< Flag to indicate histogram is valid or not. */
	ivca_u32_t area;		/**< Object area in the image plane. */
	ivca_u32_t tframes;		/**< Number of tracked frames. */
	ivca_u08_t isstatic;	/**< Flag to indicate whether the object is static
							  * or not. If the flag is zero, the object is not
							  * a static, otherwise the object is static.
							  * (#IVCA_SR_STOPPED or #IVCA_SR_REMOVED) */
	ivca_u08_t valid3d;		/**< Flag to indicate 3D information fields are
							  * valid or not. 3D information is valid if
							  * the camera is calibrated. */
	ivca_u16_t width3d;		/**< Object width in the 3D space, in meters,
							  * expressed as Q.8 format. */
	ivca_u16_t height3d;	/**< Maximum possible object height in the 3D space,
							  * in meters, expressed as Q.8 format. */
	ivca_u16_t speed3d;		/**< Object speed in the 3D space, in km/h,
							  * expressed as Q.8 format. */
	ivca_s16_t vx3d;		/**< x-directional object velocity in the 3D space,
							  * in m/s, expressed as Q.8 format. */
	ivca_s16_t vy3d;		/**< y-directional object velocity in the 3D space,
							  * in m/s, expressed as Q.8 format. Direction
							  * can be determined by atan(vy3d/vx3d) and
							  * sign(vx3d). */
	ivca_u32_t fn_srdet;	/**< Detected frame number of the static region.
							  * Valid only if isstatic field is nonzero. */
	ivca_rect_t rc;			/**< Rectangle(bounding box) of the object. */
} ivca_meta_obj_t;

/**
 * @brief  Structure of the HSV histogram for objects.
 *
 * @remark  The histogram is consist of bins for object color.
 *  Bins from bin[0] to bin[#IVCA_NRBIN_HSVHIST_C - 1] are used for color, and
 *  bins from bin[#IVCA_NRBIN_HSVHIST_C] to bin[#IVCA_NRBIN_HSVHIST - 1] are
 *  used for gray.
 * @remark  The histogram bins are unsigned Q.14 format (lower 14bits are used
 *  for fractional part, and upper 2bits are used for integer part) and
 *  normalized. Therefore sum of all bins should be 2^14 = 16384.
 * @remark  The colors represented by the color bins are fully saturated and
 *  have highest intensity, which means S=255 and V=255 in HSV color space,
 *  thus only Hue component varies between bins. The colors represented by the
 *  gray bins have just intensity, so S=0 and H=0. (H=0 is meaningless since
 *  Hue is undefined.)
 * @remark  The interval of H in color bins are 360/#IVCA_NRBIN_HSVHIST_C,
 *  and the interval of V in gray bins are 255/#IVCA_NRBIN_HSVHIST_G.
 *  If #IVCA_NRBIN_HSVHIST_C = 24 and #IVCA_NRBIN_HSVHIST_G = 4, then the
 *  intervals for color and gray bins are 15 and 64 respectively. For color
 *  bins the range of bins are shifted by the half of interval
 *  (360/#IVCA_NRBIN_HSVHIST_C/2) for centering. So the range of color bin N is
 *  360/#IVCA_NRBIN_HSVHIST_C * [(N - 1/2), (N + 1/2)]: if #IVCA_NRBIN_HSVHIST_C
 *  is 24, the range is 15 * [(N - 1/2), N + 1/2] = [15*N - 7.5, 15*N + 7.5].
 *  The range of gray bin N is 255/#IVCA_NRBIN_HSVHIST_G * [N, N+1]: if
 *  #IVCA_NRBIN_HSVHIST_G is 4, the range is 63.75 * [N, N+1].
 * @remark  Below is an example of bin range for index N when number of bins
 *  are 24 and 4 for color and gray respectively.
 *  -
 *  +-----+-------------------+-----+--------------+\n
 *  | N   | H                 | S   | V            |\n
 *  +-----+-------------------+-----+--------------|\n
 *  | 0   | -7.5(352.5) ~ 7.5 | 255 | 255          |\n
 *  | 1   | 7.5 ~ 22.5        | 255 | 255          |\n
 *  | ... | ...               | ... | ...          |\n
 *  | 23  | 337.5 ~ 352.5     | 255 | 255          |\n
 *  | 24  | 0                 | 0   | 0 ~ 63.75    |\n
 *  | ... | ...               | ... | ...          |\n
 *  | 27  | 0                 | 0   | 191.25 ~ 255 |\n
 *  +-----+-------------------+-----+--------------+
 */
typedef struct _ivca_meta_objhist_t {
	ivca_u16_t hdata[IVCA_NRBIN_HSVHIST];	/**< Histogram data. */
} ivca_meta_objhist_t;

/**
 * @brief  Descriptor for each counter value in CNT chunk.
 *  (#IVCA_META_CKTYPE_CNT)
 */
typedef struct _ivca_meta_cnt_t {
	ivca_s16_t id;			/**< Counter id. */
	ivca_u16_t reserved;	/**< Reserved. Should be set to 0. */
	ivca_s32_t value;		/**< Counter value. */
} ivca_meta_cnt_t;

/**
 * @brief  Descriptor for DBG chunk.
 *  (#IVCA_META_CKTYPE_DBG)
 */
typedef struct _ivca_meta_dbg_t {
	ivca_u32_t fg_pixels;	/**< Number of fg pixels in the current frame. */
	ivca_u16_t fg_objects;	/**< Number of fg objects in the current frame. */
	ivca_u16_t fg_runs;		/**< Number of runs in the current frame. */
	ivca_u16_t trk_count;	/**< Number of objects being tracked. */
	ivca_u16_t cpu_usage;	/**< CPU usage of the algorithm, in 0.1% unit. */
} ivca_meta_dbg_t;

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

#ifdef __cplusplus
}
#endif

#endif	/* _IVCA_DEF_H_ */

