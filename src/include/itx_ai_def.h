/*******************************************************************************
*  (c) COPYRIGHT 2019 ITX M2M                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  HAERYONG AHN,  captainnn@itxm2m.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2019/02/20 HaeRyong Ahn   Created.

................................................................................
DESCRIPTION:

................................................................................
*/


#ifndef	_ITX_AI_DEF_H_
#define	_ITX_AI_DEF_H_

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

#pragma	pack(push, 1)

/**
 * @brief  Rule event descriptor.
 */

typedef struct _ai_rule_event_t {
	unsigned int type;					/**< Type of this event: IVCA_ET_XXXX. */
	int object_id;					/**< Object id for this event. */
	short int rule_id;					/**< Rule id for this event. */	
    	int ch;							/**< Channel number for this event. */
	char object_class[64];   			/**< Object class */	
     	char topic[64];   					/**< topic */
	double bbx_position[4];			/**< Rectangle(bounding box) of the event region. */
	unsigned int timestamp;			/**< Timestamp of this event. */
     	unsigned int timestampl;			/**< Timestamp of this event. */
     	unsigned int process_time;			/**< process_time. */
	double confidence;				/**< confidence. */
	int reserved[12];					/**< Reserved */
	unsigned long long vid_ts_us;
} ai_rule_event_t;	// 242 byte

/*
 * AI meta data structure
 * -
 * +----------+--------+---------+--------+---------+-----+\n
 * | meta hdr | ck hdr | ck body | ck hdr | ck body | ... |\n
 * +----------+--------+---------+--------+---------+-----+
 */

/**
 * @brief  Meta data header.
 */
typedef struct _ai_meta_hdr_t {
	unsigned int signature;		/**< Meta data signature, should be 'ITXM'. */
	unsigned int version;			/**< Meta data structure ver. */
	unsigned int size;				/**< Size of meta data including header. */
	unsigned int rulechange_num;	/**< Number of changing rule.
								 * (Increase number from 0 by 1) */
	int ch;						/**< Channel id. */
	unsigned char track_ref;		/**< Tracking reference mode: IVCA_TREF_XXXX. */
	unsigned char ckcount;		/**< Number of meta data chunks. */
	unsigned int n_width;			/**< Width of the normalized coordinate. */
	unsigned int n_height;			/**< Height of the normalized coordinate. */
	unsigned int timestamp;		/**< timestamp */
	unsigned int timestampl;		/**< timestampl */
	unsigned int process_time;		/**< process_time */
	char reserved1[6];
	char topic[64];				/**< ai topic */
	int reserved2[4];
} ai_meta_hdr_t;	// 128 byte


/* Meta data Chunk types. */
#define	AI_META_CKTYPE_JNK	0	/**< Junk chunk. (dummy) */
#define	AI_META_CKTYPE_OBJ	1	/**< Object chunk. */
#define	AI_META_CKTYPE_CNT	2	/**< Counter value chunk. */
#define	AI_META_CKTYPE_LPR	3	/**< License Plate Recognition chunk. */
#define	AI_META_CKTYPE_FR		4	/**< Face Recognition chunk. */

/**
 * @brief  Chunk header for meta data.
 */
typedef struct _ai_meta_ckh_t {
	unsigned int  size;			/**< Size of chunk excluding header. */
	unsigned short  type;		/**< Meta data chunk type. AI_META_CKTYPE_XXXX. */
	unsigned short  priv;		/**< Private field, depend on chunk type:
							  * - #AI_META_CKTYPE_JNK: Not used.
							  * - #AI_META_CKTYPE_OBJ: Number of objects.
							  * - #AI_META_CKTYPE_CNT: Number of counters.
							  */
} ai_meta_ckh_t;	//8 byte


/**
 * @brief  Descriptor for each object in OBJ chunk.
 *  (#AI_META_CKTYPE_OBJ)
 *
 * @remark  If the hvalid is set, then HSV histogram is immediately follows the
 *  ivca_meta_obj_t structure. (#AI_NRBIN_HSVHIST * sizeof(ivca_u16_t))
 *  -
 *  --+-----------------+--------------+-----------------+-----------------+--\n
 *  ..| obj[A],hvalid=1 | histogram[A] | obj[B],hvalid=0 | obj[C],hvalid=0 |..\n
 *  --+-----------------+--------------+-----------------+-----------------+--\n
 */
typedef struct _ai_meta_obj_t {
	unsigned int id;				/**< Object id. */
	unsigned short nevents;		/**< Number of triggered events. */
	unsigned short is_static;		/* static object filter. */
	char object_class[64];		/**< Object class. */
	unsigned char hvalid;			/**< Flag to indicate histogram is valid or not. */
	unsigned int tframes;			/**< Number of tracked frames. */
	unsigned char valid3d;			/**< Flag to indicate 3D information fields are
							  	* valid or not. 3D information is valid if
							  	* the camera is calibrated. */
	unsigned short width3d;		/**< Object width in the 3D space, in meters,
							 	 * expressed as Q.8 format. */
	unsigned short height3d;		/**< Maximum possible object height in the 3D space,
							  	* in meters, expressed as Q.8 format. */
	unsigned short speed3d;		/**< Object speed in the 3D space, in km/h,
							  	* expressed as Q.8 format. */
	short vx3d;					/**< x-directional object velocity in the 3D space,
							  	* in m/s, expressed as Q.8 format. */
	short vy3d;					/**< y-directional object velocity in the 3D space,
							 	 * in m/s, expressed as Q.8 format. Direction
							  	* can be determined by atan(vy3d/vx3d) and
							  	* sign(vx3d). */
	unsigned char isstatic;
	char reserved;	
	double confidence;			/** confidence */
	double bbx_position[4];		/**< Rectangle(bounding box) of the object. */
} ai_meta_obj_t;	// 128 byte

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
typedef struct _ai_meta_objhist_t {
	unsigned short hdata[IVCA_NRBIN_HSVHIST];	/**< Histogram data. */
} ai_meta_objhist_t;	

/**
 * @brief  Descriptor for each counter value in CNT chunk.
 *  (#AI_META_CKTYPE_CNT)
 */
typedef struct _ai_meta_cnt_t {
	short id;					/**< Counter id. */
	unsigned short reserved;	/**< Reserved. Should be set to 0. */
	unsigned int value;		/**< Counter value. */
} ai_meta_cnt_t;	// 8byte

#pragma	pack(pop)

#define MAX_ZONE_LIST  128

typedef struct _ai_point_t_
{
    float x;
    float y;
}ai_point_t;

typedef struct _ai_generic_event_t {
       unsigned int type;                                      /**< Type of this event: IVCA_ET_XXXX. */
       int ch;                                                 /**< Channel number for this event. */
       ai_point_t event_area[2];                        //bbox left_top, right_bottom
       unsigned int timestamp;                 /**< Timestamp of this event. */
       unsigned int timestampl;                        /**< Timestamp of this event. */

       //event message
       char caption[45];
       char title[84];
       char description[100];

       //zone
       char trigger_type[45];
       char trigger_name[45];
       int trigger_zone_count;
       ai_point_t trigger_zone_list[MAX_ZONE_LIST];
} ai_generic_event_t;

#define GENERIC_FILE "/NFDVR/log/g_data.log"

typedef struct _fr_info_t {
       unsigned int face_id;
       double search_score;
       char name[32];
       char group_name[16][32];
       int group_cnt;
       int age;
       char gender[32];
       char headwear[32];
       char glasses[32];
}fr_info_t;

typedef struct _lpr_info_t {
       char number[32];
       char country[16];
       double score;
}lpr_info_t;

typedef struct _ai_fr_obj_t {
       int id;
       char object_class[64];
       double confidence;
       double bbx_position[4];
       int info_cnt;
       fr_info_t info[8];
}ai_fr_obj_t;

typedef struct _ai_lpr_obj_t {
       int id;
       char object_class[64];
       double confidence;
       double bbx_position[4];
       char lp_text[64];
}ai_lpr_obj_t;

typedef struct _fr_lpr_option_t {
       /* Display options. */
       ivca_u08_t sw_obj_bb;   /**< Show(1) or hide(0) object bounding box. */
       ivca_u08_t sw_rule;             /**< Show(1) or hide(0) rule. */
       ivca_u08_t sw_rule_name;/**< Show(1) or hide(0) rule name. */
       ivca_u08_t sw_grp_name;
       ivca_u08_t sw_plate_number;
} fr_lpr_option_t ;

#define GROUP_MASK_LEN         9
#define FR_LPR_MAX_ZONES               4

/**
 * @brief  Zone descriptor.
 */
typedef struct _fr_lpr_zone_t {
       ivca_s16_t id;                  /**< Zone Id: 0 ~ (#IVCA_MAX_ZONES - 1). */
       ivca_u08_t type;                /**< Rule type: #IVCA_RT_LINE or #IVCA_RT_AREA. */
       ivca_u08_t active;              /**< Activation state. 0: inactive, 1: active. */
       ivca_u32_t c_threshold;         /**< confidence threshold */
       ivca_u08_t color[3];    /**< Display color, RGB respectively. */
       ivca_u08_t npts;                /**< Number of points. (in the polygon) */
       ivca_point_t pt[IVCA_MAX_PTSPERZONE];   /**< Points in the zone. */
       ivca_s08_t name[IVCA_MAX_NAME_LEN];     /**< Zone name,
                                                                                 * NULL terminated string. */
       ivca_u32_t grp_mask;                                    /** LPR Group Mask **/
       ivca_s08_t group_filter[1024];                  /** FR Group Mask **/
} fr_lpr_zone_t;

typedef struct _fr_lpr_rule_t {
       ivca_u16_t nzones;              /**< Number of valid zones in zonelist. */
       fr_lpr_zone_t zonelist[FR_LPR_MAX_ZONES];       /**< List of all zones. */
} fr_lpr_rule_t;


typedef struct _ai_fr_event_t {
       unsigned int type;                                      /**< Type of this event: IVCA_ET_XXXX. */
       int object_id;                                  /**< Object id for this event. */
       short int rule_id;                                      /**< Rule id for this event. */
       int ch;                                                 /**< Channel number for this event. */
       char object_class[64];                          /**< Object class */
       char topic[64];                                         /**< topic */
       double bbx_position[4];                 /**< Rectangle(bounding box) of the event region. */
       unsigned int timestamp;                 /**< Timestamp of this event. */
       unsigned int timestampl;                        /**< Timestamp of this event. */
       unsigned int process_time;                      /**< process_time. */
       double confidence;                              /**< confidence. */
       int info_cnt;
       fr_info_t info[8];
} ai_fr_event_t;


typedef struct _ai_lpr_event_t {
       unsigned int type;                                      /**< Type of this event: IVCA_ET_XXXX. */
       int object_id;                                  /**< Object id for this event. */
       short int rule_id;                                      /**< Rule id for this event. */
       int ch;                                                 /**< Channel number for this event. */
       char object_class[64];                          /**< Object class */
       char topic[64];                                         /**< topic */
       double bbx_position[4];                 /**< Rectangle(bounding box) of the event region. */
       unsigned int timestamp;                 /**< Timestamp of this event. */
       unsigned int timestampl;                        /**< Timestamp of this event. */
       unsigned int process_time;                      /**< process_time. */
       double confidence;                              /**< confidence. */
       char lp_text[64];
       unsigned int grp_mask;
} ai_lpr_event_t;



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

#endif	/* _ITX_AI_DEF_H_ */

