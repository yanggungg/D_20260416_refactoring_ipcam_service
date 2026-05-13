#ifndef _ONVIF_PACKET_H_
#define _ONVIF_PACKET_H_

#if defined (_ANF5HG_1648) || defined (_ANF5HG_0824D) \
 || defined (_UTM5HGA_1648D) || defined (_UTM5HGA_0824D) || defined (_UTM5HGA_0412D) \
 || defined (_UTM5X_1648D)  || defined (_UTM5X_0824D) || defined (_UTM5X_0412D) \
 || defined (_UTM5HG_1648D) || defined (_UTM5HG_0824D) || defined (_UTM5HG_0412D) \
 || defined (_UTM6GB_1648D) || defined (_UTM6GB_0824D) || defined (_UTM6GB_0412D) \
 || defined (_UTM7G_1648D) || defined (_UTM7G_0824D) || defined (_UTM7G_0412D) \
 || defined (_ANF8G_1648D) || defined (_ANF8G_3296D)
#define DVR
#else
#define NVR
#endif

#include "feature_def.h"
/////////////////////////////////////////////////////////////////
//  Dont't Modify //
#if defined(WIN32)
int onvif_process(int cmd, void* tmp, int size);



#else
#include <time.h>
#endif

#include "glib.h"
#define SIMULATOR_IP "127.0.0.1"
#define VIRTUAL_DATA_TIME_START 1427778559
#define VIRTUAL_DATA_TIME_END    1427788559

/* common value */
#ifdef TRUE
//junsun.2010.11.24.meaningless #warning TRUE already defined
#else
#define TRUE 1
#endif

#ifdef FALSE
//junsun.2010.11.24.meaningless #warning FALSE already defined
#else
#define FALSE 0
#endif

#ifdef true
//junsun.2010.11.24.meaningless #warning true already defined
#else
#define true 1
#endif

#ifdef false
//junsun.2010.11.24.meaningless #warning false already defined
#else
#define false 0
#endif

#ifdef MAX
#undef MAX
#endif
#define  MAX( x, y ) 		( ((x) > (y)) ? (x) : (y) )

#ifdef MIN
#undef MIN
#endif
#define  MIN( x, y ) 		( ((x) < (y)) ? (x) : (y) )

#define BROADCAST_SSID	0	
#define START_SSID		1	//(valid ssid range : 0x00000001 ~ 0xFFFFFFFE)
#define INVALID_SSID	0xFFFFFFFF

#define	SUB_ERROR			(-1)
#define SUB_BUF_IS_FULL		(-2)
#define SSID_IS_DUPLICATED	(-3)

#define ONVIF_MAX_LOG_CNT 128
#define ONVIF_MAX_LOG_DATA 128

#define IRIS_OP_OFF		(0)
#define IRIS_OP_ON		(1)
#define IRIS_OP_DC		(2)
#define IRIS_OP_PWM		(3)
#define IRIS_OP_AUTO	(4)

#define FOCUS_ABSOLUTE		(0)
#define FOCUS_RELATIVE		(1)
#define FOCUS_CONTINUOUS	(2)

#define _false	xsd__boolean__false_	
#define _true 	xsd__boolean__true_		
#define HTTP_NONE_PASS_NAME	"need_to_be_check"
#define HTTP_FAIL_PASS_NAME	"http_auth_fail"


//#define	ONVIF_PACKET_DUMP

#if defined(_NVS2G16)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(16)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (2)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH)
#define ONVIF_RELAY_CNT     (4)
#define ONVIF_ALARMIN_CNT     (16)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (30)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#define PTZ_TOUR_MAX 		(4)
#define PTZ_TOUR_PAR_MAX	(24)
#define PTZ_TOUR_DWELL_SEC_MAX (180)

#define PTZ_FOCUS_SPEED_MIN (-1.0)
#define PTZ_FOCUS_SPEED_MAX (1.0)

#elif defined(_NVS2G08)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(8)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (3)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH * ONVIF_RECORDING_CNT)
#define ONVIF_RELAY_CNT     (2)
#define ONVIF_ALARMIN_CNT     (8)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (30)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)
#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#elif defined(_NVS2G04)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(4)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
//#define ONVIF_RECORDING_CNT 3
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (3)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH * ONVIF_RECORDING_CNT)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (4)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (30)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)
#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#elif defined(_NVS2G01)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(1)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (3)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH * ONVIF_RECORDING_CNT)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (1)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (30)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#elif defined(_NVS2G01H)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(1)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (3)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH * ONVIF_RECORDING_CNT)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (1)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (30)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#elif defined(_NVS2G01I)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(1)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (0)
#define ONVIF_AENCODER_CNT      (0)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (3)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH * ONVIF_RECORDING_CNT)
#define ONVIF_RELAY_CNT     (0)
#define ONVIF_ALARMIN_CNT     (0)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (30)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#elif defined (_IPX_32M4E) || defined (_IPX_32P4E) || defined (_IPX_32P5) || defined (_ANF8G_3296D)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(32)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */ // tmp change
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (1)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (2)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH)
#define ONVIF_RELAY_CNT     (4)
#define ONVIF_ALARMIN_CNT     (16)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_JPEG_MAX_FPS (3)
#define ONVIF_VENCODER_MIN_FPS      (1)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#define PTZ_TOUR_MAX 		(4)
#define PTZ_TOUR_PAR_MAX	(24)
#define PTZ_TOUR_DWELL_SEC_MAX (180)

#define PTZ_FOCUS_SPEED_MIN (-1.0)
#define PTZ_FOCUS_SPEED_MAX (1.0)

#elif defined (_IPX_1648P3ECO) || defined (_IPX_1648VE3) || defined (_IPX_1648P3) || defined (_UTM_1648D) || defined (_IPX_1648M4) || defined (_ANF5HG_1648D) || defined (_UTM5HG_1648D) || defined (_UTM5HGA_1648D) || defined (_UTM5X_1648D) || defined (_UTM6GB_1648D) || defined (_UTM7G_1648D)  \
|| defined (_IPX_1648P4) || defined (_IPX_1648L4) || defined (_IPX_1648M4E) || defined (_IPX_1648P4E) || defined (_IPX_1648M4CE) || defined (_IPX_1648P4CE) || defined (_ANF8G_1648D)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(16)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (1)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_COM_PROFILE_CNT	(ONVIF_CH + ONVIF_CH + ONVIF_CH)
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (2)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (16)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (90)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_JPEG_MAX_FPS (3)
#define ONVIF_VENCODER_MIN_FPS      (1)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (1)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (100)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#define PTZ_TOUR_MAX 		(4)
#define PTZ_TOUR_PAR_MAX	(24)
#define PTZ_TOUR_DWELL_SEC_MAX (180)

#define PTZ_FOCUS_SPEED_MIN (-1.0)
#define PTZ_FOCUS_SPEED_MAX (1.0)

#elif defined (_IPX_0824VE3) || defined (_IPX_0824P3) || defined (_IPX_0824P3ECO) || defined (_IPX_0824P4) || defined (_IPX_0824M4) || defined (_ANF5HG_0824D) || defined (_UTM5HG_0824D) || defined (_UTM5HGA_0824D) || defined (_UTM5X_0824D) || defined (_UTM5HG_0824D) || defined (_UTM6GB_0824D) || defined (_UTM7G_0824D) \
|| defined (_IPX_0824L4) || defined (_IPX_0824M4E) || defined (_IPX_0824P4E) || defined (_IPX_0824M4CE) || defined (_QPX_0824M5CE)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(8)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (2)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH)
#define ONVIF_RELAY_CNT     (4)
#define ONVIF_ALARMIN_CNT     (16)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (1)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (10)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#define PTZ_TOUR_MAX 		(4)
#define PTZ_TOUR_PAR_MAX	(24)
#define PTZ_TOUR_DWELL_SEC_MAX (180)

#define PTZ_FOCUS_SPEED_MIN (-1.0)
#define PTZ_FOCUS_SPEED_MAX (1.0)

#elif defined (_IPX_0412VE3) || defined (_IPX_0412L4) || defined (_IPXM_0412) || defined (_IPX_0412M4) || defined (_UTM5HG_0412D) || defined (_UTM5HGA_0412D) || defined (_UTM5X_0412D) || defined (_UTM6GB_0412D) || defined (_UTM7G_0412D) \
|| defined (_IPX_0412M4E) || defined (_IPX_0412M4CE) || defined (_QPX_0412M5CE)

#define ONVIF_FILE_SERVER   "/tmp/.onvif_udsfile"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#define ONVIF_CH	(4)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (1)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_COM_PROFILE_CNT	(ONVIF_CH + ONVIF_CH + ONVIF_CH)
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (2)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (4)
#define ONVIF_JOBSOURCE_CNT (3)

#define ONVIF_VENCODER_MAX_QUALITY  (90)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_JPEG_MAX_FPS (3)
#define ONVIF_VENCODER_MIN_FPS      (1)
#define ONVIF_VENCODER_MAX_FPS_PAL  (25)
#define ONVIF_VENCODER_MIN_FPS_PAL  (25)
#define ONVIF_VENCODER_MAX_INTERVAL (1)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)
#define ONVIF_VENCODER_H264_PROFILE_CNT (3)

#define MAX_SUBSCRIPTION_CNT    (100)

#define ONVIF_BRIGHT_MAX (100)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (100)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (100)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (100)
#define ONVIF_SHARPNESS_MIN (0)

#define PTZ_TOUR_MAX 		(4)
#define PTZ_TOUR_PAR_MAX	(24)
#define PTZ_TOUR_DWELL_SEC_MAX (180)

#define PTZ_FOCUS_SPEED_MIN (-1.0)
#define PTZ_FOCUS_SPEED_MAX (1.0)

#else

#define ONVIF_FILE_SERVER   "/tmp/onvif_streamer.sock"
#define ONVIF_PASS_FILE "/tmp/.passwd_onvif"

#if 0
#define ONVIF_CH	(4)
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2) /* main, second stream */
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH+ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH+ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH+ONVIF_CH)
#define MAX_PROFILE (ONVIF_CH*ONVIF_VENCODER_CNT*2)	/* max profile size */
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
#define ONVIF_RECORDING_CNT (0)
#define ONVIF_TRACK_CNT (0)
#define ONVIF_MAX_RECORDING_CNT   (0)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (1)
#define ONVIF_JOBSOURCE_CNT (0)
#else
#define ONVIF_CH	(4)
#define MAX_PROFILE (ONVIF_CH + ONVIF_CH + ONVIF_CH) 			/* max profile size */
#define MAX_METADATA (ONVIF_CH) 			/* max profile size */
#define ONVIF_VSOURCE_CNT       (ONVIF_CH)
//#define ONVIF_PRESET_CNT       (ONVIF_CH)
#define ONVIF_VENCODER_CNT      (ONVIF_CH*2)
#define ONVIF_ASOURCE_CNT       (ONVIF_CH)
#define ONVIF_AENCODER_CNT      (ONVIF_CH)
#define ONVIF_METADATA_CNT      (ONVIF_CH)
#define ONVIF_AUDIOOUTPUT_CNT      (ONVIF_CH)
#define ONVIF_PTZ_CNT      (ONVIF_CH)
#define ONVIF_FIXED_PROFILE_CNT (ONVIF_CH*2)
#define ONVIF_MAX_PROFILE_CNT   MAX_PROFILE
//#define ONVIF_RECORDING_CNT 3
#define ONVIF_RECORDING_CNT (3)
#define ONVIF_TRACK_CNT (3)
#define ONVIF_MAX_RECORDING_CNT   (ONVIF_CH)
#define ONVIF_RELAY_CNT     (1)
#define ONVIF_ALARMIN_CNT     (4)
#define ONVIF_JOBSOURCE_CNT (3)
#endif

#define ONVIF_VENCODER_MAX_QUALITY  (100)
#define ONVIF_VENCODER_MIN_QUALITY  (1)
#define ONVIF_VENCODER_MAX_FPS      (30)
#define ONVIF_VENCODER_MIN_FPS      (1)
#define ONVIF_VENCODER_MAX_FPS_PAL  (1)
#define ONVIF_VENCODER_MIN_FPS_PAL  (1)
#define ONVIF_VENCODER_MAX_INTERVAL (1)
#define ONVIF_VENCODER_MIN_INTERVAL (1)
#define ONVIF_VENCODER_MAX_GOV      (60)
#define ONVIF_VENCODER_MIN_GOV      (1)  // 0 means infinition
#define ONVIF_VENCODER_H264_PROFILE_CNT (1)

#define MAX_SUBSCRIPTION_CNT    (10)

#define ONVIF_BRIGHT_MAX (30)
#define ONVIF_BRIGHT_MIN (0)
#define ONVIF_COLOR_MAX (30)
#define ONVIF_COLOR_MIN (0)
#define ONVIF_CONTRAST_MAX (30)
#define ONVIF_CONTRAST_MIN (0)
#define ONVIF_SHARPNESS_MAX (15)
#define ONVIF_SHARPNESS_MIN (1)

#define PTZ_TOUR_MAX 		(4)
#define PTZ_TOUR_PAR_MAX	(24)
#define PTZ_TOUR_DWELL_SEC_MAX (180)

#define PTZ_FOCUS_SPEED_MIN (-1.0)
#define PTZ_FOCUS_SPEED_MAX (1.0)
#endif





//#define PACK_BUFF_SIZE (1024*4)
//#define PACK_BUFF_SIZE (1024*12)
//#define ONVIF_RECORDING_CNT 3
#define ONVIF_PRESET_CNT       (16)
#define PACK_BUFF_SIZE (65536)
#define COMMON_SIZE (64)
#define MAX_IF (48)
#define HALF_COMMON_SIZE (32)
#define RESOLUTION_CNT (16)
#define MAX_SCOPE (20)
#define MAX_MSG_FILTER_CNT (8)
#define ONVIF_RELAY_DELAY_CNT	(10)
#define ONVIF_MAX_TRACK (10)
#define FAULT  -1
#define SUCCESS 1

#define ONVIF_ROI_W 16
#define ONVIF_ROI_H 9
#define ONVIF_AREA ONVIF_ROI_W * ONVIF_ROI_H
#define Translation_X -1
#define Translation_Y 1

// service name. refer the mod_auth.c in lighttpd
enum {
	ONVIF_ALL_SERVICE = 0,
	ONVIF_ANALYTICS_SERVICE,
	ONVIF_DEVICE_SERVICE,
	ONVIF_EVENT_SERVICE	,	
	ONVIF_IMAGING_SERVICE,
	ONVIF_DEVICEIO_SERVICE,	
	ONVIF_PTZ_SERVICE,		
	ONVIF_MEDIA_SERVICE,	
	ONVIF_RECORDING_SERVICE,
	ONVIF_SEARCH_SERVICE,	
	ONVIF_REPLAY_SERVICE,	
	ONVIF_RECEIVER_SERVICE,	
	ONVIF_DISPLAY_SERVICE,	
	ONVIF_ACTION_SERVICE,
	ONVIF_SUBSCRIPTION,
	ONVIF_SECURITY,
	SIZE_OF_SERVICES,
};

#define ONVIF_SUPPORT_ANALYTICS	(1<<ONVIF_ANALYTICS_SERVICE)
#define ONVIF_SUPPORT_DEVICE		(1<<ONVIF_DEVICE_SERVICE)
#define ONVIF_SUPPORT_EVENT		(1<<ONVIF_EVENT_SERVICE)
#define ONVIF_SUPPORT_IMAGING		(1<<ONVIF_IMAGING_SERVICE)
#define ONVIF_SUPPORT_DEVICEIO	(1<<ONVIF_DEVICEIO_SERVICE)
#define ONVIF_SUPPORT_PTZ			(1<<ONVIF_PTZ_SERVICE)
#define ONVIF_SUPPORT_MEDIA		(1<<ONVIF_MEDIA_SERVICE)
#define ONVIF_SUPPORT_RECORDING	(1<<ONVIF_RECORDING_SERVICE)
#define ONVIF_SUPPORT_SEARCH		(1<<ONVIF_SEARCH_SERVICE)
#define ONVIF_SUPPORT_REPLAY		(1<<ONVIF_REPLAY_SERVICE)
#define ONVIF_SUPPORT_RECEIVER	(1<<ONVIF_RECEIVER_SERVICE)
#define ONVIF_SUPPORT_DISPLAY		(1<<ONVIF_DISPLAY_SERVICE)
#define ONVIF_SUPPORT_ACTION		(1<<ONVIF_ACTION_SERVICE)
#define ONVIF_SUPPORT_SECURITY		(1<<ONVIF_SECURITY)


#define ONVIF_ANALYTICS_SERVICE_URL	"onvif/analytics_service"
#define ONVIF_DEVICE_SERVICE_URL	"onvif/device_service"
#define ONVIF_EVENT_SERVICE_URL	"onvif/event_service"
#define ONVIF_IMAGING_SERVICE_URL	"onvif/imaging_service"
#define ONVIF_DEVICEIO_SERVICE_URL	"onvif/deviceio_service"
#define ONVIF_PTZ_SERVICE_URL		"onvif/ptz_service"
#define ONVIF_MEDIA_SERVICE_URL	"onvif/media_service"
#define ONVIF_RECORDING_SERVICE_URL	"onvif/recording_control"
#define ONVIF_SEARCH_SERVICE_URL	"onvif/recording_search"
#define ONVIF_REPLAY_SERVICE_URL	"onvif/replay_control"
#define ONVIF_RECEIVER_SERVICE_URL	"onvif/receiver_service"
#define ONVIF_DISPLAY_SERVICE_URL	"onvif/display_service"
#define ONVIF_ACTION_SERVICE_URL	"onvif/action_service"
#define ONVIF_SUBSCRIPTION_URL		"onvif/subscription"
#define ONVIF_SECURITY_URL		"onvif/advancedsecurity"



#define ONVIF_MAX_RECORDINGJOBS	(ONVIF_MAX_RECORDING_CNT)
#define ONVIF_MAX_RECORDING_RATE		(2000)
#define ONVIF_MAX_TOTAL_RECORDING_RATE	(2000 * ONVIF_MAX_RECORDING_CNT )

#define ONVIF_DEVICE_MNG_VERSION_MAJOR	(1)
#define ONVIF_DEVICE_MNG_VERSION_MINOR	(7)

#define ONVIF_DEVICE_IO_VERSION_MAJOR	(1)
#define ONVIF_DEVICE_IO_VERSION_MINOR	(4)

#define ONVIF_IMAGING_VERSION_MAJOR	(2)
#define ONVIF_IMAGING_VERSION_MINOR	(3)

#define ONVIF_MEDIA_VERSION_MAJOR	(1)
#define ONVIF_MEDIA_VERSION_MINOR	(7)

#define ONVIF_PTZ_VERSION_MAJOR	(2)
#define ONVIF_PTZ_VERSION_MINOR	(5)

#define ONVIF_REC_CONTROL_VERSION_MAJOR	(1)
#define ONVIF_REC_CONTROL_VERSION_MINOR	(7)

#define ONVIF_REC_SEARCH_VERSION_MAJOR	(1)
#define ONVIF_REC_SEARCH_VERSION_MINOR	(7)

#define ONVIF_REPLAY_VERSION_MAJOR	(1)
#define ONVIF_REPLAY_VERSION_MINOR	(5)

#define ONVIF_ANALYTICS_VERSION_MAJOR	(2)
#define ONVIF_ANALYTICS_VERSION_MINOR	(4)

#define ONVIF_EVENT_VERSION_MAJOR		(1)
#define ONVIF_EVENT_VERSION_MINOR		(7)

#define ONVIF_ACTION_VERSION_MAJOR	(1)
#define ONVIF_ACTION_VERSION_MINOR	(0)

#define ONVIF_DEVICEIO_VERSION_MAJOR	(1)
#define ONVIF_DEVICEIO_VERSION_MINOR	(4)

#define ONVIF_SECURITY_VERSION_MAJOR	(1)
#define ONVIF_SECURITY_VERSION_MINOR	(4)


#define ONVIF_METADATA_PTYPE (126)
#define METADATA_INTERVAL_MSEC (1000)

#define ONVIF_Monostable    (0)
#define ONVIF_Bistable      (1)
#define ONVIF_closed        (0)
#define ONVIF_open          (1)
#define ONVIF_active        (0)
#define ONVIF_inactive      (1)

#define RENEW_ABSOLUTE  (0)
#define RENEW_RELATIVE  (1)


/************ MODEL DEPENDANT  *******************/

#define ONVIF_VSOURCE_WIDTH_MIN     (3840)
#define ONVIF_VSOURCE_WIDTH_MAX     (3840)
#define ONVIF_VSOURCE_HEIGHT_MIN    (2160)
#define ONVIF_VSOURCE_HEIGHT_MAX    (2160)
#define ONVIF_VSOURCE_X_MIN     (-1)
#define ONVIF_VSOURCE_X_MAX     (1)
#define ONVIF_VSOURCE_Y_MIN    (-1)
#define ONVIF_VSOURCE_Y_MAX    (1)

#define ONVIF_MCAST_MAIN            (0)
#define ONVIF_MCAST_SECOND          (1)
#define ONVIF_MCAST_VPORT_TYPE      (0)
#define ONVIF_MCAST_APORT_TYPE      (1)
#define ONVIF_CHECK_CONFIG_TOKEN    (1<<3)
#define ONVIF_CHECK_PROFILE_TOKEN   (1<<2)
#define ONVIF_CHECK_CONFIG_TOKEN_OK (1<<1)
#define ONVIF_CHECK_PROFILE_TOKEN_OK    (1<<0)
#define ONVIF_MOV_DISTANCE_MAX (10.0)
#define ONVIF_MOV_DISTANCE_MIN (-10.0)
#define ONVIF_FOCUS_CON_SPEED_MAX (100)
#define ONVIF_FOCUS_CON_SPEED_MIN (1)
#define ONVIF_TOKEN_SIZE (16)
#define ONVIF_MAX_BITRATE_K (20000)
#define ONVIF_MIN_BITRATE_K (512)
#define MAX_RESOLUTION_CNT (60)

#define MAX_FPS (7)
#define MAX_QUALITY (6)



#define ALL_RES_CAPA "WRQZONVQabMYCKLGDHJXUTSFB"

#define NVS_RES_CAPA "KDCB"

#define NVS_RES_CAPA_NTSC "aDBC" //"KDCB"

#define NVS_RES_CAPA_PAL "HGFb" //"LHGF"


#define ONVIF_PTZ_TOKEN_PREFIX "pc"

#define VIDEO_TRACK "tv"
#define AUDIO_TRACK "ta"
#define METADATA_TRACK "tm"

#define DIGITAL_INPUT_TOKEN_PREFIX	"di"
#define RELAY_TOKEN_PREFIX				"relay"


#define CONTINUOUS_PTV_SPACE "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace"
#define CONTINUOUS_ZV_SPACE "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace"

#define ABSOLUTE_PTV_SPACE "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace"
#define ABSOLUTE_ZV_SPACE "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace"

#define RELATIVE_PTV_SPACE "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace"
#define RELATIVE_ZV_SPACE "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace"

#define PT_SPEED_SPACE "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace"
#define Z_SPEED_SPACE "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace"

/************ RESULT FAULT CODE  *******************/

#define PRINT_IP(x)     (((x)&0xFF000000)>>24),(((x)&0x00FF0000)>>16),(((x)&0x0000FF00)>>8),(((x)&0x000000FF))

enum DeviceStatus {
	STATUS_DEVICE_NOT_READY = 0,
	STATUS_DEVICE_READY
};

enum DateTimeType {
    DateTimeType__Manual = 0, 
    DateTimeType__NTP
};

enum mcast_type {
    VIDEO_MCAST_TYPE = 0,
    AUDIO_MCAST_TYPE,
    METADATA_MCAST_TYPE
};

enum AutoFocusMode 
{
	AutoFocusMode__AUTO = 0, 
	AutoFocusMode__MANUAL = 1
};

typedef enum _ip_type
{
	ONVIF_IPV4, 				// 0
	ONVIF_IPV6			    // 1
}ip_type;

typedef enum _onvif_cmd
{
	CMD_GetHostName, 				// 0
	CMD_SetHostName,			    // 1
	CMD_GetDeviceInformation,		// 2
	CMD_GetDNS, 					// 3
	CMD_SetDNS,						// 4	
	CMD_GetNTP,						// 5	
	CMD_SetNTP,						// 6	
	CMD_SetDateTime,				// 7
	CMD_GetDateTime,				// 8
	CMD_GetScopes,					// 9
	CMD_SetScopes,					// 10
	CMD_AddScopes,					// 11
	CMD_RemoveScopes,				// 12
	CMD_GetProfilesDB,              // 13
	CMD_GetProfileDB,               // 14
	CMD_GetVideoEncoder,			// 15
	CMD_GetVideoEncoders,			// 16
	CMD_SetVideoEncoder,            // 17
	CMD_FactoryDefault, 	 	    // 18
	CMD_SystemReboot,               // 19
	CMD_GetStreamUri, 				// 20
	CMD_GetVideoEncoderOption,		// 21
	CMD_GetImagingOption,   		// 22
	CMD_SetImagingOption,           // 23
	CMD_GetNetworkInfo,             // 24
	CMD_SetNetworkInfo,             // 25
	CMD_GetDefaultGateway,             // 26
	CMD_SetDefaultGateway,             // 27
	CMD_GetNetworkPort, 			// 28
	CMD_SetNetworkPort,             // 29
	CMD_SetVideoSource,				// 30
	CMD_GetDiscovery,
	CMD_SetDiscovery,
	CMD_CreateProfile,
	CMD_AddVideoSource,
	CMD_AddVideoEncoder,
	CMD_RemoveVideoEncoder,
	CMD_RemoveVideoSource,
	CMD_DeleteProfile,
	CMD_GetVideoSourceConfiguration, 
	CMD_GetVideoSourceConfigurations, 		// 40
	CMD_CheckTokens,
	CMD_AddAudioSource,
	CMD_AddAudioEncoder,
	CMD_RemoveAudioEncoder,
	CMD_RemoveAudioSource,
	CMD_GetAudioSourceConfiguration, 
	CMD_GetAudioSourceConfigurations, 
	CMD_GetAudioSourceOption,
	CMD_GetAudioEncoderConfiguration, 
	CMD_GetAudioEncoderConfigurations, 		// 50
	CMD_GetCompatibleAudioEncoder,
	CMD_GetCompatibleAudioSource,
	CMD_SetAudioEncoder, 
	CMD_SetAudioSource, 
	CMD_GetCompatibleVideoSource,
	CMD_GetVideoSourceOption,
	CMD_GetVideoSources,
	CMD_GetAudioSources,	
	CMD_GetCompatibleVideoEncoder,
	CMD_GetMetadataConfigurations,			// 60
	CMD_GetCompatibleMetadataConfigurations,
	CMD_AddMetadataConfiguration,
	CMD_RemoveMetadataConfiguration,
	CMD_SetMetadataConfiguration,
	CMD_GetMetadataConfiguration,
	CMD_GetUser,
	CMD_SetUser,
	CMD_CreateUsers,
	CMD_DeleteUsers,
	CMD_GetImagingSettings,					// 70
	CMD_SetImagingSettings,
	CMD_GetOptions,
	CMD_Move,
	CMD_FocusStop,
	CMD_GetStatus,
	CMD_GetCapa,
	CMD_GetRelays,
	CMD_SetRelaySettings,
	CMD_SetRelayState,
	CMD_GotoHomePosition,					// 80
	CMD_AbsoluteMove,
	CMD_ContinuousMove,
	CMD_CreatePresetTour,
	CMD_GetPTZNodes,
	CMD_GetPTZNode,
	CMD_GetPTZConfiguration,
	CMD_GetPTZConfigurationOptions,
	CMD_GetConfigurations,
	CMD_GetNode,
	CMD_GetNodes,							// 90
	CMD_GetPresets,
	CMD_GetPresetTour,
	CMD_GetPresetTourOptions,
	CMD_GetPresetTours,
	CMD_GetServiceCapabilities,
	CMD_PTZGetStatus,
	CMD_ModifyPresetTour,
	CMD_OperatePresetTour,
	CMD_RelativeMove,
	CMD_RemovePreset,						// 100
	CMD_RemovePresetTour,
	CMD_SendAuxiliaryCommand,
	CMD_SetConfiguration,
	CMD_SetHomePosition,
	CMD_SetPreset,
	CMD_GotoPreset,
	CMD_Stop,
	CMD_GetEventCapa,
	CMD_GetPTZConfigurations,
	CMD_SetPTZConfiguration,				// 110
	CMD_AddPTZConfiguration,
	CMD_RemovePTZConfiguration,
	CMD_CreateRecording,
	CMD_CreateRecordingJob,
	CMD_CreateTrack,
	CMD_DeleteRecording,
	CMD_DeleteRecordingJob,
	CMD_DeleteTrack,
	CMD_GetRecordingConfiguration,
	CMD_GetRecordingJobConfiguration,		// 120
	CMD_GetRecordingJobs,
	CMD_GetRecordingJobState,
	CMD_GetRecordingOptions,
	CMD_GetRecordings,
	CMD_GetTrackConfiguration,
	CMD_SetRecordingConfiguration,
	CMD_SetRecordingJobConfiguration,
	CMD_SetRecordingJobMode,
	CMD_SetTrackConfiguration,
	CMD_EndSearch,							// 130
	CMD_FindEvent,
	CMD_FindMetadata,
	CMD_FindPTZPosition,
	CMD_FindRecordings,
	CMD_GetEventSearchResults,
	CMD_GetMediaAttributes,
	CMD_GetMetadataSearchResults,
	CMD_GetRecordingInformation,
	CMD_GetRecordingSearchResults,
	CMD_GetRecordingSummary,				// 140
	CMD_GetSearchState,
	CMD_GetRecordingServiceCapabilities,
	CMD_GetReplayConfiguration,
	CMD_GetReplayUri,
	CMD_SetReplayConfiguration,
	CMD_GetRecordingJobStateChangeEvent,
	CMD_GetEventProperties,	
	CMD_CreatePullPointSubscription,	
	CMD_PullMessages,
	CMD_Subscribe,							// 150
	CMD_Notify,
	CMD_SetSynchronizationPoint,	
	CMD_Unsubscribe,	
	CMD_Renew,
	CMD_GetRelayOptions,
	CMD_GetDigitalInputs,
	CMD_CheckDeviceReady,
	CMD_GetServices,
	CMD_GetGuaranteedNumberOfVideoEncoderInstances,
	CMD_GetSnapshotUri,	
	CMD_GetAnalyticsModules,			// yys onvif
	CMD_ModifyAnalyticsModules,			// 160
	CMD_GetRules,
	CMD_ModifyRules,
	CMD_GetVideoAnalyticsConfiguration,
	CMD_GetVideoAnalyticsConfigurations,
	CMD_SetVideoAnalyticsConfiguration,
	CMD_AddVideoAnalyticsConfiguration,
	CMD_RemoveVideoAnalyticsConfiguration,
	CMD_VAnalytics_Check_Token,
	CMD_GetMetadataConfigurationOptions,
	CMD_SetSynchronizationPoint_trt,		// 170
	
	CMD_GetProfilesDB2,
	CMD_GetStreamUri2,
	CMD_GetVideoEncoderOption2,
	CMD_GetVideoEncoders2,
	CMD_RemoveConfiguration,	
	CMD_AddConfiguration,
	CMD_CreateProfile2,
	CMD_GetAudioEncoderConfiguration2,
	CMD_GetAnalyticsConfigurations2,
	CMD_GetMetadataConfigurations2,			// 180
	CMD_GetAudioOutput,
	CMD_GetAudioOutputConfiguration,
	CMD_GetVideoSourceConfigurations2,
	CMD_GetAudioSourceConfigurations2,
	CMD_GetAudioEncoderConfigurations2,
	
	CMD_AddAudioOutputConfiguration,
	CMD_RemoveAudioOutputConfiguration,
	CMD_GetCompatibleAudioDecoder,
	CMD_AddAudioDecoderConfiguration,
	CMD_RemoveAudioDecoderConfiguration,		// 190
	CMD_SetAudioOutput,
	CMD_SetAudioDecoder,
	CMD_GetAudioOutputConfigurations,
	CMD_GetCompatibleAudioOutput,
	CMD_GetAudioEncoderConfigurationOptions,
	CMD_GetAudioOutput2,
	CMD_GetAudioDecoder2,
	CMD_StartMulticastStreaming,
	CMD_StopMulticastStreaming,
	CMD_GetCompatibleConfigurations,		// 200
	CMD_StartFirmwareUpgrade,
	CMD_CreateOSD,
	CMD_GetOSD,
	CMD_SetOSD,
	CMD_DeleteOSD,
	CMD_GetHttpAuthMethod = 254,
	CMD_RTP_HTTP = 255
}onvif_cmd;

typedef struct _onvif_packet_header
{
    	onvif_cmd cmd;
       unsigned int dlen;
} onvif_packet_header;

typedef struct _onvif_packet
{
	onvif_packet_header header;
	char data[PACK_BUFF_SIZE];
}onvif_packet;


//  Dont't Modify //
/////////////////////////////////////////////////////////////////


// COPY TO FUNCTION
void onvif_thread();
void onvif_discovery_stop();
//junsun.add.2010.11.30
#ifdef JS_FEATURE_GENETEC_SDK
void genetec_discovery_stop();
#endif

#if 1
typedef struct _arg_GetCapa
{
	int ch;
	char token[COMMON_SIZE];
	int is_audio[ONVIF_CH];
	int is_focus[ONVIF_CH];
	int is_zoom[ONVIF_CH];
	int is_motion[ONVIF_CH];
	int is_ptz[ONVIF_CH];
	int alarmin[ONVIF_CH];
	int relay[ONVIF_CH];

	//image service
	int audioSupport[ONVIF_CH];
	int motionSupport[ONVIF_CH];
	int alarmCnt[ONVIF_CH];
	int relayCnt[ONVIF_CH];

	int brightnessSupport[ONVIF_CH];
	int brightnessMin[ONVIF_CH];
	int brightnessMax[ONVIF_CH];

	int colorSupport[ONVIF_CH];
	int colorMin[ONVIF_CH];
	int colorMax[ONVIF_CH];

	int contrastSupport[ONVIF_CH];
	int contrastMin[ONVIF_CH];
	int contrastMax[ONVIF_CH];

	int sharpnessSupport[ONVIF_CH];
	int sharpnessMin[ONVIF_CH];
	int sharpnessMax[ONVIF_CH];

	int blcSupport[ONVIF_CH];
	int wdrSupport[ONVIF_CH];
	int dnnSupport[ONVIF_CH];
	int wbSupport[ONVIF_CH]; 

	int exposureSupport[ONVIF_CH];
	int gainSupport[ONVIF_CH];
	int gainMin[ONVIF_CH];
	int gainMax[ONVIF_CH];
	int irisSupport[ONVIF_CH];
	int irisMin[ONVIF_CH];
	int irisMax[ONVIF_CH];

	int focusSupport[ONVIF_CH];
	int focusAbsoluteMoveSupport[ONVIF_CH];
	int focusAbsoluteRangeMin[ONVIF_CH];
	int focusAbsoluteRangeMax[ONVIF_CH];
	
	int focusRelativeMoveSupport[ONVIF_CH];
	int focusRelativeRangeMin[ONVIF_CH];
	int focusRelativeRangeMax[ONVIF_CH];
	
	int focusContinuousMoveSupport[ONVIF_CH];
	int focusContinuousRangeMin[ONVIF_CH];
	int focusContinuousRangeMax[ONVIF_CH];
	
	int focusAutoModeSupport[ONVIF_CH];
	int focusNearLimitSupport[ONVIF_CH];
	int focusFarLimitSupport[ONVIF_CH];

	//ptz service
	int ptSupport[ONVIF_CH];
	int ptAbsoluteMoveSupport[ONVIF_CH];
	int panAbsoluteRangeMin[ONVIF_CH];
	int panAbsoluteRangeMax[ONVIF_CH];
	int tiltAbsoluteRangeMin[ONVIF_CH];
	int tiltAbsoluteRangeMax[ONVIF_CH];
	
	int ptRelativeMoveSupport[ONVIF_CH];
	int panRelativeRangeMin[ONVIF_CH];
	int panRelativeRangeMax[ONVIF_CH];
	int tiltRelativeRangeMin[ONVIF_CH];
	int tiltRelativeRangeMax[ONVIF_CH];
	
	int ptContinuousMoveSupport[ONVIF_CH];
	int panContinuousRangeMin[ONVIF_CH];
	int panContinuousRangeMax[ONVIF_CH];
	int tiltContinuousRangeMin[ONVIF_CH];
	int tiltContinuousRangeMax[ONVIF_CH];

	int zoomSupport[ONVIF_CH];
	int zoomAbsoluteMoveSupport[ONVIF_CH];
	int zoomAbsoluteRangeMin[ONVIF_CH];
	int zoomAbsoluteRangeMax[ONVIF_CH];

	int zoomRelativeMoveSupport[ONVIF_CH];
	int zoomRelativeRangeMin[ONVIF_CH];
	int zoomRelativeRangeMax[ONVIF_CH];

	int zoomContinuousMoveSupport[ONVIF_CH];
	int zoomContinuousRangeMin[ONVIF_CH];
	int zoomContinuousRangeMax[ONVIF_CH];

	int ptAbsoluteSpeedSupport[ONVIF_CH];
	int ptAbsoluteSpeedDefault[ONVIF_CH];
	int ptAbsoluteSpeedMin[ONVIF_CH];
	int ptAbsoluteSpeedMax[ONVIF_CH];

	int zoomAbsoluteSpeedDefault[ONVIF_CH];
	int zoomAbsoluteSpeedSupport[ONVIF_CH];
	int zoomAbsoluteSpeedMin[ONVIF_CH];
	int zoomAbsoluteSpeedMax[ONVIF_CH];

	int ptzContinuousTimeOutDefault[ONVIF_CH]; // set -1 if not supported
	int ptzContinuousTimeOutSupport[ONVIF_CH];
	int ptzContinuousTimeOutMin[ONVIF_CH];
	int ptzContinuousTimeOutMax[ONVIF_CH];

	int homePositionSupport[ONVIF_CH];
	int presetSupport[ONVIF_CH];
	int presetMaxCnt[ONVIF_CH];
	
	int presetTourSupport[ONVIF_CH];
	int presetTourMaxCnt[ONVIF_CH];
	int presetTourSeqMaxCnt[ONVIF_CH];
} arg_GetCapa;
#else
typedef struct _arg_GetCapa
{
	int is_audio;
	int is_focus;
	int is_zoom;
    	int is_motion;		
    	int alarmin;
    	int relay;

	int is_focus_absolute;
	int is_focus_relative;
	int is_focus_continuous;

	int is_auto_focus_auto;
	int is_auto_focus_manual;	// oneshot

	int focus_near_min;
	int focus_near_max;	
	int focus_far_min;
	int focus_far_max;
} arg_GetCapa;
#endif
typedef struct _arg_Motion
{
	int is_change_sensitivity[4];
	int sensitivity[4];
	char VAnalytics_Token[COMMON_SIZE];
} arg_Motion; 

typedef struct _arg_Active_Cell
{
	int is_active_cells[4];
	char Acitve_Cells[4][100];
	char VAnalytics_Token[COMMON_SIZE];
	_Bool Is_Motion;
}arg_Active_Cell;

typedef struct _arg_VanalyticsConfig
{
	char token[COMMON_SIZE];
	char name[COMMON_SIZE];
	int is_motion;
	int use_count;
	arg_Motion module;
	arg_Active_Cell rule;
}arg_VanalyticsConfig;

typedef struct _arg_VanalyticsConfigs
{
	arg_VanalyticsConfig config[ONVIF_CH];
}arg_VanalyticsConfigs;

typedef struct _arg_AnalyticsConfigs2
{
    char conf_token[COMMON_SIZE];
    char profile_token[COMMON_SIZE];
	arg_VanalyticsConfig config[ONVIF_CH];
}arg_AnalyticsConfigs2;


typedef struct _arg_IPAddress
{
	int type;
	char ipaddress[16];
} arg_IPAddress;

typedef struct _arg_Multicast
{
	arg_IPAddress address;
	int port;
	int ttl;
	int auto_start;
} arg_Multicast;

typedef struct _arg_Tokens
{
	char config_token[COMMON_SIZE];
	char profile_token[COMMON_SIZE];
	int result;
}arg_Tokens;

typedef struct _arg_McastAddress
{
	int		auto_start;
	int		iptype;
	unsigned int	ipaddr;
	char    ipaddrv4[COMMON_SIZE];
    char    ipaddrv6[COMMON_SIZE];
	unsigned int		port;
	unsigned int		ttl;
}arg_McastAddress;

typedef struct _arg_AudioEncoder
{
	int enable;
	char conf_token[COMMON_SIZE];
	char token[COMMON_SIZE];
	char name[COMMON_SIZE];	
	int use_count;
	int encoding;
	int bitrate;
	int s_rate;
	int result;
    	int timeout;
	int save_flag;
	arg_McastAddress mcast;
}arg_AudioEncoder;

typedef struct _arg_AudioEncoders
{
	char token[COMMON_SIZE];
	int aencoder_cnt;
	int result;
	arg_AudioEncoder aencoder[ONVIF_AENCODER_CNT];
}arg_AudioEncoders;

typedef struct _arg_AudioSource
{
	char token[COMMON_SIZE];
	char s_token[COMMON_SIZE];
	char name[COMMON_SIZE];
	int use_count;
	int result;
	int save_flag;
}arg_AudioSource;

typedef struct _arg_AudioOutput
{
    int enable;
    char token[COMMON_SIZE];
    char conf_token[COMMON_SIZE];
    char name[COMMON_SIZE];
    int use_count;
    char send_primacy[COMMON_SIZE];
    int output_level;
}arg_AudioOutput;

typedef struct _arg_AudioDecoder
{
    int enable;
    char token[COMMON_SIZE];
    char conf_token[COMMON_SIZE];
    char name[COMMON_SIZE];
    int use_count;
}arg_AudioDecoder;

typedef struct _arg_AudioSources
{
	char token[COMMON_SIZE];
	/* onvif doesn't support audio source count */
	int asource_cnt;
	int result;
	arg_AudioSource asource[ONVIF_ASOURCE_CNT];
}arg_AudioSources;

typedef struct _arg_RemoveConfig
{
	char profile_token[COMMON_SIZE];
    char config_token[10][COMMON_SIZE]; // max = 10
    char config_type[10][COMMON_SIZE];  // max = 10
    int config_size;
	int result;
}arg_RemoveConfig;

typedef struct _arg_AddConfig
{
	char config_token[COMMON_SIZE];
	char profile_token[COMMON_SIZE];
	int result;
}arg_AddConfig;

typedef struct _arg_AddConfig2
{
    char profile_token[COMMON_SIZE];
    char profile_name[COMMON_SIZE];
    char config_token[10][COMMON_SIZE]; // max = 10
    char config_type[10][COMMON_SIZE];  // max = 10
    int config_size;
    int result;
}arg_AddConfig2;

typedef struct _arg_Metadata
{
	char token[COMMON_SIZE];
	char name[COMMON_SIZE];
	int event_filter_type;
	char event_filter[COMMON_SIZE];
	char event_policy[COMMON_SIZE];
	int use_count;
	int ptz_status;
	int ptz_position;
	int analytics;
	arg_McastAddress mcast;
	int save_flag;
	int result;
	int timeout;
	arg_Motion motion;
} arg_Metadata;

typedef struct _arg_Metadatas
{
	int metadata_cnt;
	char token[COMMON_SIZE];
	arg_Metadata metadata[ONVIF_METADATA_CNT];
	int result;

} arg_Metadatas;

typedef struct _arg_Metadatas2
{
	int metadata_cnt;
	char token[COMMON_SIZE];
    char conf_token[COMMON_SIZE];
	arg_Metadata metadata[ONVIF_METADATA_CNT];
	int result;

} arg_Metadatas2;


typedef struct _arg_PTZConfig
{
	char token[COMMON_SIZE];
	char name[COMMON_SIZE];
	char node_token[COMMON_SIZE];	
	int use_count;
	char def_absolute_PTSpace[COMMON_SIZE*2];
	char def_absolute_ZSpace[COMMON_SIZE*2];	
	char def_relative_PTSpace[COMMON_SIZE*2];
	char def_relative_ZSpace[COMMON_SIZE*2];
	char def_continous_PTSpace[COMMON_SIZE*2];
	char def_continous_ZSpace[COMMON_SIZE*2];

	int save_flag;
	int result;
	int timeout;
	int use_ptz;

	int ch;
} arg_PTZConfig;

typedef struct _arg_PTZConfigs
{
	int cnt;
	arg_PTZConfig conf[ONVIF_PTZ_CNT];
} arg_PTZConfigs;

typedef struct _arg_ImagingOption
{
	char token[COMMON_SIZE];

	int brightness;
	int color;
	int contrast;
	int sharpness;

	int exposureMode; //tt__ExposureMode__AUTO = 0, tt__ExposureMode__MANUAL = 1
	int gain;
	int iris;

	int focusMode;	//tt__AutoFocusMode__AUTO = 0, tt__AutoFocusMode__MANUAL = 1
	int focusNearLimit;
	int focusFarLimit;
	
	int blcMode;	//tt__BacklightCompensationMode__OFF = 0, tt__BacklightCompensationMode__ON = 1
	int dnnMode; 	//tt__IrCutFilterMode__ON    = 0, 	tt__IrCutFilterMode__OFF     = 1, 	tt__IrCutFilterMode__AUTO = 2
	int wdrMode;	//tt__WideDynamicMode__OFF   = 0, 	tt__WideDynamicMode__ON      = 1
	int wbMode;		//tt__WhiteBalanceMode__AUTO = 0, 	tt__WhiteBalanceMode__MANUAL = 1 
	int defogMode;
	float defogLevel;
	
	int blc_mode;
	int e_mode;
	int e_gain;
	int e_time;
	int e_iris;
	int ircutfilter;
	int save_flag;
	int result;
}arg_ImagingOption;

typedef struct _arg_MoveOption
{
	char token[COMMON_SIZE];
	int move_type;
	int position;
	float speed;
	int result;
}arg_MoveOption;

typedef struct _arg_MoveStatus
{
	char token[COMMON_SIZE];
	int position;
	int result;
}arg_MoveStatus;
typedef struct _arg_PTZNode
{
	char token[COMMON_SIZE];
	char name[COMMON_SIZE];
	char node_token[COMMON_SIZE];
	char default_continuous_zoom_velocity_space[COMMON_SIZE * 2];
	char default_continuous_pant_tilt_velocity_space[COMMON_SIZE * 2];
	int use_count;
	int timeout;
	int max_num_preset;
	int home_supported;
	int fixed_home_position;
	int result;
	int ch;
}arg_PTZNode;

typedef struct _arg_PTZStatus
{
	char profileToken[COMMON_SIZE];
	float pan_pos;
	float tilt_pos;
	float zoom_pos;
	int pt_status;
	int zoom_status;
}arg_PTZStatus;

typedef struct _arg_PTZNodes
{
	int ptz_cnt;
	arg_PTZNode ptz[ONVIF_PTZ_CNT];
}arg_PTZNodes;


typedef struct _arg_PresetTourDetail {
	char PresetToken[ONVIF_TOKEN_SIZE];
	int StayTime;
} arg_PresetTourDetail;

typedef struct _arg_PresetTour {
	char ProfileToken[ONVIF_TOKEN_SIZE];
	char PresetTourToken[ONVIF_TOKEN_SIZE];
	int AutoStart;
	int SpotCount;
	int State;
	arg_PresetTourDetail TourSpot[PTZ_TOUR_PAR_MAX];
} arg_PresetTour;

typedef struct _arg_PresetTours {
	char ProfileToken[ONVIF_TOKEN_SIZE];
	arg_PresetTour Tour[PTZ_TOUR_MAX];
} arg_PresetTours;

typedef struct _arg_PTZPresetNode
{
	int result;
	char profileToken[COMMON_SIZE];
	char presetToken[ONVIF_TOKEN_SIZE];
	char presetName[COMMON_SIZE];
	float pan_pos;
	float tilt_pos;
	float zoom_pos;
}arg_PTZPresetNode;
typedef struct _arg_PTZPresetNodes
{
	int result;
	int preset_cnt;
	char profileToken[COMMON_SIZE];
	arg_PTZPresetNode preset[ONVIF_PRESET_CNT];
}arg_PTZPresetNodes;

typedef struct _arg_PresetTourOptions {
	char ProfileToken[ONVIF_TOKEN_SIZE];
	int preset_cnt;
	arg_PTZPresetNode preset[ONVIF_PRESET_CNT];
} arg_PresetTourOptions;

typedef struct _arg_OperatePresetTour {
	char ProfileToken[ONVIF_TOKEN_SIZE];
	char PresetTourToken[ONVIF_TOKEN_SIZE];
	int Operation;
} arg_OperatePresetTour;

typedef struct _arg_resolution
{
	int width;
	int height;
	char resolutionString;
}arg_resolution;

//for ncx3
typedef struct _arg_2nd_resolution
{
	int width_1st;
	int height_1st;
	char *res_2nd;
}arg_2nd_resolution;

typedef struct _arg_fps
{
	char fpsCh;
	int fps;
}arg_fps;
typedef struct _arg_qual
{
	char qualCh;
	int qual;
}arg_qual;

typedef struct _arg_HostName
{
	int FromDHCP;
	char Name[COMMON_SIZE];
}arg_HostName;

typedef struct _arg_Relay
{
	char token[COMMON_SIZE];
	int mode;
	int duration;
	int idle_state;
	int result;
	int curr_state;
}arg_Relay;

typedef struct _arg_Relays
{
	int relay_cnt;
	arg_Relay relay[ONVIF_RELAY_CNT];
}arg_Relays;

enum {
	Mode_option_only_mono = 0,
	Mode_option_only_bi,	
	Mode_option_both,
};

typedef struct _arg_FirmwareUpgrade
{
	char UploadUri[COMMON_SIZE];
	int UploadDelay;
	int ExpectedDownTime;
}arg_FirmwareUpgrade;

typedef struct _arg_GetDeviceInformation
{
	char Manufacturer[COMMON_SIZE];
	char Model[COMMON_SIZE];
	char FirmwareVersion[COMMON_SIZE];
	char SerialNumber[COMMON_SIZE];
	char HardwareId[COMMON_SIZE];
}arg_GetDeviceInformation;


typedef struct _arg_GetWsdlUrl
{
	char WsdlUrl[COMMON_SIZE];
}arg_GetWsdlUrl;


typedef struct _arg_DNS
{
	int	FromDHCP;
	char IPv4Address[2][COMMON_SIZE];
	char SearchDomain[COMMON_SIZE];
}arg_DNS;


typedef struct _arg_NTP
{
	int FromDHCP;
	char NTPname[COMMON_SIZE];
	int is_domain;
	int result;
}arg_NTP;


typedef struct _arg_DateTime
{
	int	Manual_NTP;	
	int DST;
	char TimeZone[COMMON_SIZE];
	// Type : UTC
	int Hour;
	int Minute;
	int Second;	
	int Year;
	int Month;
	int Day;
	// Type : Local
	int loHour;
	int loMinute;
	int loSecond;	
	int loYear;
	int loMonth;
	int loDay;
       int result;
}arg_DateTime;

typedef struct _arg_HttpAuth
{
	int http_auth;
}arg_HttpAuth;

enum HTTP_AUTH {
	HTTP_AUTH_BASIC = 0,
	HTTP_AUTH_DIGEST
};

#if 0
typedef enum{
	JS_ONVIF_VSOURCES_0_2560x1440 = 0,
	JS_ONVIF_VSOURCES_1_1920x1080,
	JS_ONVIF_VSOURCES_2_1280x1024,
	JS_ONVIF_VSOURCES_3_2048x1536,
	JS_ONVIF_VSOURCES_4_2112x1728,
	JS_ONVIF_VSOURCES_5_2112x1440,
	JS_ONVIF_VSOURCES_SIZE ,
	JS_ONVIF_VSOURCES_ENUM_PAD = 0xFF,
}js_onvif_vsources_enum_type;


typedef enum{
	JS_ONVIF_PROFILE_1_SECOND = 0,	/*default profile, second stream MJPEG, default profile must be MJPEG, because the conformance tool need MJPEG */
	JS_ONVIF_PROFILE_2_FIRST = 1,		/* h.264 first stream profile */
//	JS_ONVIF_PROFILE_3_RESERVED,		/* only 1 reserved profile. Currently, just for ONVIF conformance tool. junsun.fixme.aaaaa */
	JS_ONVIF_PROFILE_MAX = 3,			/* max profile size */
	JS_ONVIF_PROFILE_ENUM_PAD = 0xFF,	/* profile enum pad */
}js_onvif_profile_cnt_enum_type;
#endif

enum ONVIF_VIDEO_ENCODING {
	ONVIF_VIDEO_CODEC_JPEG = 0, 
	ONVIF_VIDEO_CODEC_MPEG4 = 1, 
	ONVIF_VIDEO_CODEC_H264 = 2,
	ONVIF_VIDEO_CODEC_H265 = 3,
};

typedef struct _arg_VideoEncoder
{
	char token[COMMON_SIZE];	// video encoder token
	int codec;		        	// JPEG, MPEG4, H.264, H.265
	int width;              	// video width size
	int height;             	// video height size
	char name[COMMON_SIZE];	// video encoder token
	int fps_limit; 
	int fps_interval; 
	arg_McastAddress mcast;
	int use_count;
	int bitrate;          // video bitrate
	int quality;                // video quality
	int h264_profile;
	int govlength; 
	int save_flag;
	int timeout;
	int result;
	int bitrate_control;
	int is_media2;
}arg_VideoEncoder;

typedef struct _arg_VideoEncoders
{
	int vencoder_cnt;
	char token[COMMON_SIZE];
	char conf_token[COMMON_SIZE];
	int result;
	arg_VideoEncoder vencoder[ONVIF_VENCODER_CNT];
}arg_VideoEncoders;

typedef struct _arg_VideoSource
{
	char token[COMMON_SIZE];
	int width;
	int height;
	int save_flag;
	char s_token[COMMON_SIZE];
	char name[COMMON_SIZE];
	float fps;
	int use_count;
	int x;
	int y;
	arg_ImagingOption imaging;
	int result;
}arg_VideoSource;

typedef struct _arg_PTZPantilt
{
	int isPantilt;
	float x;	/* required attribute of type xsd:float */
	float y;	/* required attribute of type xsd:float */
}arg_PTZPantilt;
typedef struct _arg_PTZZoom
{
	int isZoom;
	float x;	/* required attribute of type xsd:float */
}arg_PTZZoom;

typedef struct _arg_PTZOperation
{
	char ProfileToken[COMMON_SIZE];
	char PresetName[COMMON_SIZE];
	char PresetToken[COMMON_SIZE];
	arg_PTZPantilt PanTilt;
	arg_PTZZoom Zoom;
	int timeout;
	int result;
	arg_PTZPantilt PTSpeed;
}arg_PTZOperation;
typedef struct _arg_Stop
{
	char ProfileToken[COMMON_SIZE];
	int PanTilt;
	int Zoom;
	int result;
}arg_Stop;
typedef struct _arg_FocusStop
{
	char VideoSourceToken[COMMON_SIZE];
}arg_FocusStop;
typedef struct _arg_VideoSources
{
	int vsource_cnt;
	arg_VideoSource vsource[MAX_PROFILE];
	char token[COMMON_SIZE];
	int result;
	int timeout;
}arg_VideoSources;

typedef struct _arg_ProfileDB
{
	char token[COMMON_SIZE];		// profile token
	char name[COMMON_SIZE];		// profile token
	arg_Metadata  metadata;		
	arg_AudioEncoder aencoder;	
	arg_AudioSource  asource;
    arg_AudioOutput aoutput;
    arg_AudioDecoder adecoder;
	arg_VideoEncoder vencoder;		// video encoder
	arg_VideoSource  vsource;		// video source	
	arg_PTZConfig ptz;
	arg_VanalyticsConfig vanalytics;
	int audio_enable;
	int fixed;
	int result;
}arg_ProfileDB;


typedef struct _arg_ProfilesDB
{
	char prof_token[COMMON_SIZE];
	int profile_cnt;
	arg_ProfileDB profile[ONVIF_MAX_PROFILE_CNT];
}arg_ProfilesDB;

typedef struct _arg_ProfileDB2
{
    char config_type[COMMON_SIZE][50];
	char config_token[COMMON_SIZE][50];
	char profile_name[COMMON_SIZE];
	int fixed;
	int configSize;
	int result;
}arg_ProfileDB2;


typedef struct _arg_FactoryDefault
{
	int soft;
}arg_FactoryDefault;

typedef struct _arg_SystemReboot
{
	char Message[COMMON_SIZE];
}arg_SystemReboot;

typedef struct _arg_StreamUri
{
	char token[COMMON_SIZE];
	int stream_type;
	int protocol;
	char uri[COMMON_SIZE];
	char uri_http[COMMON_SIZE];	
	int result;
}arg_StreamUri;

typedef struct _arg_StreamUri2
{
	char token[COMMON_SIZE];
	char protocol[COMMON_SIZE];
	char uri[COMMON_SIZE];
	char uri_http[COMMON_SIZE];
}arg_StreamUri2;

typedef struct _arg_SnapshotUri
{
	char token[COMMON_SIZE];
	char uri[COMMON_SIZE*2];
	int result;
}arg_SnapshotUri;


typedef struct _arg_voption
{
	char vencoder_token[COMMON_SIZE];
	int resolution_cnt;
	int h264_profile_cnt;	
	int h264_profile[ONVIF_VENCODER_H264_PROFILE_CNT];
	int codec;
	int max_fps;
	int min_fps;
	int max_fps_interval;
	int min_fps_interval;
	int max_gop;
	int min_gop;	
	arg_resolution resolution[MAX_RESOLUTION_CNT];
	int max_bitrate;
	int min_bitrate;
}arg_voption;
	
typedef struct _arg_VideoEncoderOption
{
	arg_voption voption[2];
	int option_cnt;
	int result;
	int vs_type;
	char profile_token[COMMON_SIZE];
	char encoder_token[COMMON_SIZE];
}arg_VideoEncoderOption;

typedef struct _arg_VideoEncoderOption2
{
	arg_voption voption[3];
	int option_cnt;
	int result;
	int vs_type;
	char profile_token[COMMON_SIZE];
	char encoder_token[COMMON_SIZE];
}arg_VideoEncoderOption2;

typedef struct _arg_VideoSourceOption
{
        /* input */
	char profile_token[COMMON_SIZE];
	char config_token[COMMON_SIZE];

        /* output */
    	char vstoken[ONVIF_VSOURCE_CNT][COMMON_SIZE];
	int vstoken_cnt;
	int x_min;
	int x_max;
	int y_min;
	int y_max;
	int width_min;
	int width_max;
	int height_min;
	int height_max;    
    
	int result;
}arg_VideoSourceOption;

typedef struct _arg_AudioSourceOption
{
        /* input */
	char profile_token[COMMON_SIZE];
	char config_token[COMMON_SIZE];

        /* output */
    	char astoken[ONVIF_ASOURCE_CNT][COMMON_SIZE];
	int astoken_cnt;
	int result;
}arg_AudioSourceOption;


typedef struct _arg_GetOption
{
	char token[COMMON_SIZE];
	int bright_min;
	int bright_max;
	int color_min;
	int color_max;
	int contrast_min;
	int contrast_max;
	int sharp_min;
	int sharp_max;
	int gain_min;
	int gain_max;
	int exposure_time_max;
	int exposure_time_min;
	int op_af;
	int op_heater;
	int op_iris;
	int iris_min;
	int iris_max;
	int focus_near_min;
	int focus_near_max;
	int focus_far_min;
	int focus_far_max;
	int focus_continous_speed_max;
	int focus_continous_speed_min;
	int focus_relative_speed_max;
	int focus_relative_speed_min;
	int focus_absolute_speed_max;
	int focus_absolute_speed_min;
	int result;
}arg_GetOption;

typedef struct _arg__PanTile{
	float x;	/* required attribute of type xsd:float */
	float y;	/* required attribute of type xsd:float */
	char *space;	/* optional attribute of type xsd:anyURI */
} arg__PanTile;

typedef struct _arg__Zoom
{
	float x;	/* required attribute of type xsd:float */
	char *space;	/* optional attribute of type xsd:anyURI */
} arg__Zoom;


typedef struct _arg_preset {
	char presetToken[ONVIF_PRESET_CNT][ONVIF_TOKEN_SIZE];
	char presetName[ONVIF_PRESET_CNT][COMMON_SIZE];
}arg_preset;

typedef struct _arg_DefaultGateway
{
	int dhcp_flag;
	char gateway[32];
	int result;
}arg_DefaultGateway;

typedef struct _arg_NetworkInfo
{
	int dhcp_flag;
	int prefix_length;
	char ipaddr[32];
//junsun.add.2011.01.11 for ONVIF102
//JS_FEATURE_ONVIF102_6P2P26_SET_NETWORK_DEFAULT_GATEWAY_CONFIGURATION_IPV4
	char gateway[32];
	char macaddr[32];
	int result;
}arg_NetworkInfo;


//JS_FEATURE_ONVIF102_6P2P23_SET_NETWORK_PROTOCOLS_CONFIGURATION
typedef struct _arg_NetworkPort
{
	int http_port;
	int rtsp_port;
	int https_port;
	int http_enable;
	int rtsp_enable;
	int https_enable;	
	int is_http;
	int is_rtsp;	
	int is_https;
	int result;
}arg_NetworkPort;

typedef struct _arg_Scope
{
	char scope_cnt;
	char scope[MAX_SCOPE][COMMON_SIZE*2];
	int  scope_fixed[MAX_SCOPE];
	int  result;
}arg_Scope;
	

typedef struct _arg_Discovery
{
	int discovery;
}arg_Discovery;


#ifndef USER_SIZE
#define USER_SIZE		8
#define FIXED_USER_SIZE	8
#else
#error dd is already defined
#endif

 typedef struct _arg_UserType
{
	char	name[USER_SIZE][32];
	char	pass[USER_SIZE][32];
	char	grpname[USER_SIZE][16];
	int		user_cnt;
	int		result;
} arg_UserType;

 typedef struct _arg_PrintfType
{
	char	disp[256];
}arg_PrintfType;


//baek.add.start.2011.02.21 for PullMessage
 typedef struct _arg_GetEventStatus
{
	int motion_flags;
	int alarm_flags;
	int utc_time_t;

}arg_GetEventStatus;

typedef struct _arg_RecordingSourceInformation {
	char SourceId[COMMON_SIZE*2];
	char Name[COMMON_SIZE];
	char Location[COMMON_SIZE];
	char Description[COMMON_SIZE];
	char Address[COMMON_SIZE*2];
} arg_RecordingSourceInformation;

typedef struct _arg_GetRecordingsConfiguration {
	arg_RecordingSourceInformation Source;
	char Content[COMMON_SIZE];
	int MaximumRetentionTime;
} arg_GetRecordingsConfiguration;

typedef struct _arg_TrackConfiguration {
	int TrackType;
	char Description[COMMON_SIZE];
} arg_TrackConfiguration;


typedef struct _arg_GetTracksResponseItem {
	char TrackToken[COMMON_SIZE];
	int to;
	int From;
	arg_TrackConfiguration trackconfiguration;
} arg_GetTracksResponseItem;

typedef struct _arg_GetTracksConfiguration {
	char RecordingToken[COMMON_SIZE];
	char TrackToken[COMMON_SIZE];
	arg_TrackConfiguration trackconfiguration;
} arg_GetTracksConfiguration;

typedef struct _arg_TrackAttributes {
	int type;
	int Bitrate;
	int Width;
	int Height;
	int Encoding;
	int Framerate;
	int Samplerate;
	int CanContainPTZ;
	int CanContainAnalytics;
	int CanContainNotifications;
} arg_TrackAttributes;

typedef struct _arg_RecordingOptionsItem {
	char RecordingToken[COMMON_SIZE];
	char CompatibleSources[COMMON_SIZE];
	int JobSpare;
	int TrackSpareTotal;
	int TrackSpareVideo;
	int TrackSpareAudio;
	int TrackSpareMetadata;

} arg_RecordingOptionsItem;


typedef struct _arg_RecordingOptions {
	char RecordingToken[COMMON_SIZE];
	char CompatibleSources[COMMON_SIZE];
	int JobSpare;
	int TrackSpareTotal;
	int TrackSpareVideo;
	int TrackSpareAudio;
	int TrackSpareMetadata;

} arg_RecordingOptions;
typedef struct _arg_RecordingConfiguration {
	char RecordingToken[COMMON_SIZE];
	arg_GetRecordingsConfiguration recordingConf;
} arg_RecordingConfiguration;

typedef struct _arg_GetRecordingsResponseItem {
	char RecordingToken[ONVIF_MAX_RECORDING_CNT][COMMON_SIZE];
	int EarliestRecording;
	int LatestRecording;
	int RecordingStatus;

	int Until;
	int sizeTrack;
	int sizeRecordingConf;	
	int sizeTrackAttribute;

	arg_GetRecordingsConfiguration recordingConf[ONVIF_MAX_RECORDING_CNT];
	arg_GetTracksResponseItem track[ONVIF_MAX_RECORDING_CNT * ONVIF_TRACK_CNT];
	arg_TrackAttributes trackAttributes[ONVIF_MAX_RECORDING_CNT * ONVIF_TRACK_CNT];
	arg_RecordingOptions options;
} arg_GetRecordingsResponseItem;

typedef struct _arg_RecordingJobStateTracks {
	char SourceTag[COMMON_SIZE];
	char Destination[COMMON_SIZE];
	char state[COMMON_SIZE];
	char error[COMMON_SIZE];

} arg_RecordingJobStateTracks;

typedef struct _arg_RecordingJobStateSource {
	char SourceToken[COMMON_SIZE];
	char SourceType[COMMON_SIZE];
	char state[COMMON_SIZE];
	arg_RecordingJobStateTracks track;

} arg_RecordingJobStateSource;

typedef struct _arg_RecordingJobTrack {
	char SourceTag[COMMON_SIZE];
	char Destination[COMMON_SIZE];
	char state[COMMON_SIZE];
} arg_RecordingJobTrack;

typedef struct _arg_RecordingJobSource {
	char SourceToken[COMMON_SIZE];
	char SourceTokenType[COMMON_SIZE];
	char  state[COMMON_SIZE];
	int AutoCreateReceiver;
	arg_RecordingJobTrack track[3];
} arg_RecordingJobSource;

typedef struct _arg_RecordingJobConfiguration {
	
	char RecordingToken[COMMON_SIZE];
	char RecordingJobMode[COMMON_SIZE];
	
	int Priority;
	int  state;
	int sourceSize;
	arg_RecordingJobStateSource stateSource[ONVIF_JOBSOURCE_CNT];
	arg_RecordingJobSource source[ONVIF_JOBSOURCE_CNT];
} arg_RecordingJobConfiguration;

typedef struct _arg_JobItem {
	char JobToken[COMMON_SIZE];
	arg_RecordingJobConfiguration conf;
}arg_JobItem;

typedef struct _arg_getRecordingJobsResponseItem
{
	arg_JobItem job[ONVIF_MAX_RECORDING_CNT];
}arg_getRecordingJobsResponseItem;

typedef struct _arg_GetServiceCapabilities {
	int DynamicRecordings;
	int DynamicTracks;
	int Encoding;
	float MaxRate;
	float MaxTotalRate;
	float MaxRecordings;
	int MaxRecordingJobs;
	int Options;
} arg_GetServiceCapabilities;

typedef struct _arg_IncludedSource {
	char token[COMMON_SIZE];
	int type;
} arg_IncludedSource;

typedef struct arg_FindSearchResultList {
	char RecordingToken[COMMON_SIZE];
	char TrackToken[COMMON_SIZE];
	int Time;
	int Event;
	int StartStateEvent;
	int Samplerate;
	int Position;
} arg_FindSearchResultList;

typedef struct _arg_GetJobToken {
	char SearchToken[COMMON_SIZE];
	int startPoint;
	int endPoint;
	arg_IncludedSource IncludedSource[COMMON_SIZE];
	char SearchScopeIncludedRecording[COMMON_SIZE][COMMON_SIZE];
	int RecordingInformationFilter;
	int SearchFilter;
	int MaxMatches;
	int IncludeStartState;
	int KeepAliveTime;
	int MinResult;
	int MaxResult;
	int WaitTime;
	arg_FindSearchResultList ResultList[COMMON_SIZE];
	int SearchState;
} arg_GetJobToken;

typedef struct arg_RecordingSummary {
	int DataFrom;
	int DataUntil;
	int NumberRecordings;
} arg_RecordingSummary;
typedef struct _arg_RecordingCapabilities {
	int DynamicRecordings;
	int DynamicTracks;
	int Encoding;
	int MaxTotalRate;
	int MaxRate;
	int MaxRecordings;
	int MaxRecordingJobs;
	int Options;
	int MetadataSearch;
	int GeneralStartEvents;
	int ReversePlayback;
	int SessionTimeoutRange[2];
	int RTP_RTSP_TCP;
} arg_RecordingCapabilities;

typedef struct _arg_ReplayConfiguration {
	int SessionTimeout;
} arg_ReplayConfiguration;
typedef struct _arg_StreamSetup {
	int Stream;
	int TransportProtocol;
	int TransportTunnelProtocol;
	int TransportTunnel;
	char RecordingToken[COMMON_SIZE];
	char uri[COMMON_SIZE];
	char uri_http[COMMON_SIZE];
} arg_StreamSetup;

typedef struct _arg_GetRecordingJobsResponseItem {
	arg_RecordingJobConfiguration configuration;
	char JobToken[COMMON_SIZE];
} arg_GetRecordingJobsResponseItem;


/* Structures of Search Service */
typedef struct _arg_SearchScope {
	int sizeSource;
	int sizeRecording;
	int Recording;
	char Filter[COMMON_SIZE*2];
	arg_IncludedSource Source[COMMON_SIZE];
} arg_SearchScope;


typedef enum { 
	FIND_RECORDING = 0,   // reserved
	FIND_EVENT,
	FIND_METADATA
} REC_FIND_TYPE;

typedef enum {
	ONVIF_PROCESS_RET_NONE	 = 0,
	ONVIF_PROCESS_RET_ERROR,
} ONVIF_PROCESS_RET;

typedef struct _arg_FindRecordings {
	REC_FIND_TYPE FindType; // 0:recording, 1:event, 2:metadata
	time_t StartPoint;
	time_t EndPoint;
	int MaxMatches;
	int KeepAliveTime;
	char SearchToken[COMMON_SIZE];	
	arg_SearchScope Scope;
	int SearchState;
} arg_FindRecordings;


typedef struct _arg_TrackInformation {
	int type;
	int StartTime;
	int EndTime;	
	char token[COMMON_SIZE];
	char Description[COMMON_SIZE];	
	char Content[COMMON_SIZE];	
} arg_TrackInformation;

typedef struct _arg_RecordingInformation {
	int StartTime;
	int EndTime;	
	int Status;
	char token[COMMON_SIZE];
	char Content[COMMON_SIZE];		
	arg_RecordingSourceInformation	Source;
	arg_TrackInformation TrackInfo[ONVIF_TRACK_CNT];
} arg_RecordingInformation;

typedef struct _arg_ResultList {
	int SearchState;
	arg_RecordingInformation RecordInfo;
} arg_ResultList;

typedef struct _arg_RecordingSearchResults {
	arg_ResultList Result;
} arg_RecordingSearchResults;

typedef struct _arg_GuaranteedNumVideoEncoder {
	char token[COMMON_SIZE];
	int total;
	int H264;
	int MJPEG;
	int MPEG;
} arg_GuaranteedNumVideoEncoder;
typedef struct _arg_AuxiliaryCommand{
    char profile_token[COMMON_SIZE];
    char command[COMMON_SIZE];
}arg_AuxiliaryCommand;

typedef struct _arg_OSDConfiguration {
	int type;
	int position_type;
	int text_type;
	int result;
	char video_source_token[COMMON_SIZE];
	char osd_token[COMMON_SIZE];
	char plain_text[COMMON_SIZE];
}arg_OSDConfiguration;

typedef struct _arg_OSDConfigurations {
	arg_OSDConfiguration osd_configs[16];
	char video_source_token[COMMON_SIZE];
	char osd_token[COMMON_SIZE];
	int osd_cnt;
	int result;
}arg_OSDConfigurations;


typedef enum _OSD_position{
	UpperLeft,
	UpperRight,
	LowerLeft,
	LowerRight,
	CustomPosition,
} OSD_position;

typedef enum _OSD_text_type{
	PlainText,
	DateText,
	TimeText,
	DateAndTimeText,
} OSD_text_type;

typedef enum _OSD_type{
	tt__OSDType__Text,	///< xs:string value="Text"
	tt__OSDType__Image,	///< xs:string value="Image"
	tt__OSDType__Extended,	///< xs:string value="Extended"
} OSD_type;


//ONVIF Evnet Key : http://222.112.8.34:8090/x/SgNh

#if defined(_IPX_32P5)
#define MAX_MSG_BUF (400)
#else
#define MAX_MSG_BUF (200)
#endif

typedef enum { 
	START_OF_E_KEY = 0,   // reserved
		
	E_KEY_SENSOR,
	E_KEY_ITX_MOTION,
	E_KEY_VIDEOANALYTICS_MOTION0,
	E_KEY_VIDEOANALYTICS_MOTION1,
	E_KEY_VIDEOANALYTICS_MOTION2,
	E_KEY_VIDEOANALYTICS_MOTION3,
	E_KEY_MOTION_ALARM,
	E_KEY_DIGITAL_INPUT,
	E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE,
	E_KEY_VIDEO_SIGNAL_LOSS,
	E_KEY_ADVANCED_SENSOR,
	E_KEY_RELAY,
	E_KEY_CHANGE_PROFILE,
    E_KEY_CHANGE_CONFIGURATION,
	E_KEY_IMAGE_TOO_BLURRY,
	E_KEY_IMAGE_TOO_DARK,
	E_KEY_IMAGE_TOO_BRIGHT,
	E_KEY_IMAGE_GLOBAL,
	E_KEY_RECORDING_JOB_STATE_CHANGE,
	E_KEY_CONFIGURATION_CHANGE,
	E_KEY_RECORDINGHISTORY_RECORDING_STATE,
	E_KEY_RECORDINGCONFIG_TRACK_CONFIGURATION,
	
	E_KEY_DATA_DELETION,
	E_KEY_EVENT_CONFIGURATION_CHANGE,
	E_KEY_RECORDING_AND_TRACK,
	E_KEY_DVA,
	E_KEY_DVA_EVENT,
	E_KEY_VA_DATA,
	E_KEY_VA_EVENT,
	E_KEY_AI_EVENT,
	
	// ONVIF_NEW_EVENT
	//E_KEY_ONVIF_NEW_EVENT,
	
	END_OF_E_KEY,
} ONVIF_EVENT_KEY;

#define	NOT_SUPPORT_EVENTS (END_OF_E_KEY-E_KEY_RECORDING_JOB_STATE_CHANGE)
#define	NUMBER_OF_EVENTS (END_OF_E_KEY-1)

typedef enum { 
	E_Initialized = 0,
	E_Changed,	
} ONVIF_E_STATE;

typedef enum _ONVIF_FILTER_TYPE {
	TOPIC_EXPRESSION_TYPE = 0,
	MESSAGE_CONTENTS_TYPE,
} ONVIF_FILTER_TYPE;

// ONVIF_NEW_EVENT
#if 0
struct e_new_type {
	unsigned int ch; // channel
	unsigned int value;
};

struct e_new_type_filter {
	int is_ch; // 0: any, -1: not included, 1: included
	unsigned int ch; // channel
	int is_value; // 0: any, -1: not included, 1: included	
	unsigned int value;
};
#endif

typedef enum _onvif_e_event_config_cate {
	MOTION_DETECTION_ON_OFF_CHANGED = 0,
	TAMPER_DETECTION_ON_OFF_CHANGED,
	AUDIO_ON_OFF_CHANGED,
	PTZ_ON_OFF_CHANGED,
	NUM_OF_EVENT_CONF,
} onvif_e_event_config_cate;

struct e_event_config_type {
	onvif_e_event_config_cate category;
	unsigned int prev;
	unsigned int next;	
};

struct e_event_config_type_filter {
	// 0 means it can be used, 1 means it should be filtered.
	onvif_e_event_config_cate cate[NUM_OF_EVENT_CONF];
	char value[16]; // "0x0000" or "0x00AF" ...
};

struct e_image_type {
	unsigned int ch; // channel
	unsigned int value;
};

struct e_image_type_filter {
	// 0 means it can be used, 1 means it should be filtered.
	char	ch[ONVIF_CH]; //  { ch0, ch1, ch2, ... }
	char	value[2];	 // { false, true } 
};

struct e_motion_type {
	unsigned int ch; // channel
	unsigned int value;
};

struct e_motion_type_filter {
	// 0 means it can be used, 1 means it should be filtered.
	char	ch[ONVIF_CH]; //  { ch0, ch1, ch2, ... }
	char	value[2];	 // { false, true } 
};


typedef enum _onvif_e_ad_alarm_state {
	SENSOR_ACTIVE = 0,
	SENSOR_INACTIVE = 1,
	SENSOR_SABOTAGE = 2
} onvif_e_ad_alarm_state;

struct e_ad_alarm_type {
	unsigned int ch; // channel
	onvif_e_ad_alarm_state state;
};

struct e_ad_alarm_type_filter {
	// 0 means it can be used, 1 means it should be filtered.
	char	ch[ONVIF_ALARMIN_CNT]; //  { ch0, ch1, ch2, ... }
	char	value[3];	 // { open, close } 
};

struct e_alarm_type {
	unsigned int ch; // channel
	unsigned int value;
};

typedef enum {
	RELAY_MODE_MONOSTABLE, // auto
    RELAY_MODE_BISTABLE,   // toggle
} relay_mode_t;

// 릴레이 상태
typedef enum {
    RELAY_STATE_ACTIVE = 0,
    RELAY_STATE_INACTIVE   = 1
} relay_state_t;

struct e_relay_type {
    unsigned int ch;        // ch
    relay_mode_t mode;      // mode
    unsigned int duration;  // duration time
    relay_state_t idle;     // idle state
	relay_state_t active;
};

struct e_recording_type {
	unsigned int token; // channel
	unsigned int recordingToken;
	unsigned int value;
	char sourceId[COMMON_SIZE*2];
	char name[COMMON_SIZE];
	char location[COMMON_SIZE];
	char description[COMMON_SIZE];
	char address[COMMON_SIZE*2];
	char content[COMMON_SIZE];

};

struct e_alarm_type_filter {
	// 0 means it can be used, 1 means it should be filtered.
	char	ch[ONVIF_ALARMIN_CNT]; //  { ch0, ch1, ch2, ... }
	char	value[2];	 // { open, close } 
};

struct e_profile_change_type {
    unsigned int    ch;
    unsigned int    value;
    char            token[32];
    char            type[32];
};

struct e_configuration_change_type {
    unsigned int    ch;
    unsigned int    value;
    char            token[32];
    char            type[32];
};


struct e_rec_del_type {
	unsigned int rec_num;
	unsigned int track_num;
	unsigned int start; 
	unsigned int end; 	
};

struct e_rec_del_type_filter {
	// 0 means it can be used, 1 means it should be filtered.
	char	rec_num[ONVIF_CH];  //  { ch0, ch1, ch2, ... }
	char	track_num[ONVIF_MAX_RECORDING_CNT * ONVIF_TRACK_CNT];  //  { tr0, tr1, tr2, ... }
};

struct e_msg{
	ONVIF_EVENT_KEY key;
	ONVIF_E_STATE state;
	unsigned int  time_tick; 
	long    uptime_tick;

	union {
		struct e_alarm_type	alarm;
		struct e_motion_type	motion;
		struct e_rec_del_type	rec_del;	
		struct e_image_type	image;
		struct e_recording_type	recording;
		struct e_event_config_type	event_conf;
		struct e_ad_alarm_type	ad_alarm;
		struct e_relay_type	relay;		

		// ONVIF_NEW_EVENT
		// struct e_new_type	new;		
	};

	struct e_profile_change_type prof_change;
	struct e_configuration_change_type config_change;

	// If this value is not 0, this message must be used for only the subscribtion.
	int  ss_id;
	int	 sent;
};

typedef struct _ONVIF_EVENT_MGSBUF{
	struct e_msg    msg_buf[MAX_MSG_BUF];
	unsigned int    tail;
	unsigned int    end;		// not used
} ONVIF_EVENT_MSGBUF;

typedef enum _filter_ornot {
	ONVIF_NOT_FILTERED = 0,
	ONVIF_FILTERED,		
} ONVIF_FILTER_USE;

struct e_topic_filter{
	// 0 means it can be used, 1 means it should be filtered.
     char topic_filter[NUMBER_OF_EVENTS]; // {START_OF_E_KEY, E_KEY_SENSOR, E_KEY_ITX_MOTION ... }
};

struct e_msg_filter{
	struct e_alarm_type_filter	sensor;
	struct e_motion_type_filter	itx_motion;
	struct e_motion_type_filter	motion_alarm;
	struct e_rec_del_type_filter	record_job_state_change;
	struct e_rec_del_type_filter	configuration_change;
	struct e_rec_del_type_filter	data_deletion;
	struct e_rec_del_type_filter	recording_and_track;
	struct e_alarm_type_filter	digital_input;
	struct e_alarm_type_filter	relay;
	struct e_image_type_filter	global_scene_change;
	struct e_image_type_filter	vloss;	
//	struct e_event_config_type	event_conf;		// not used
	// ONVIF_NEW_EVENT
	// struct e_new_type_filter	new;
};

typedef enum {
    OP_NONE = 0,    // 단일 조건
    OP_AND,         // A and B
    OP_OR,          // A or B
    OP_AND_NOT,     // A and not B
    OP_OR_NOT       // A or not B
} OperatorType;

struct e_message_content {
	char name[COMMON_SIZE];
	char value[COMMON_SIZE];
	OperatorType op_type;
};

struct event_subscription{
	int	ss_id; 
	struct e_topic_filter tf;
	struct e_msg_filter mf;
	struct e_message_content content;
	time_t c_time; // subscription start time
	time_t t_time;
	long    u_time;
	unsigned int old_tail_idx;
	unsigned int mbuf_end;
	int is_first_req;
};

#define MAX_FILTER_DATA_SIZE (COMMON_SIZE*COMMON_SIZE)

struct onvif_filter {
	ONVIF_FILTER_TYPE type;
	char data[MAX_FILTER_DATA_SIZE];
};

struct onvif_filters {
	unsigned int cnt;
	struct onvif_filter	unit[MAX_MSG_FILTER_CNT];
};

typedef struct _arg_CreatePullPoint {
	unsigned int current_time;
	unsigned int termination_time;
	struct onvif_filters filters;
	char ss_address[COMMON_SIZE*4];
	int				err_code;
} arg_CreatePullPoint;

typedef struct _arg_PullMessages {
	int	ss_id;
	unsigned int	timeout;
	unsigned int	limit;
	unsigned int	current_time;
	unsigned int	termination_time;
	unsigned int	msgcnt;
	struct e_msg	msgs[COMMON_SIZE];
} arg_PullMessages;

typedef struct _arg_Renew {
	int	ss_id;
	unsigned int	termination_time;
	unsigned int	is_relative;	
	unsigned int	current_time;	
	int				err_code;
} arg_Renew;

typedef struct _arg_Unsubscribe {
	int	ss_id;
	int err_code;
} arg_Unsubscribe;

typedef struct _arg_Subscribe {
	unsigned int current_time;
	unsigned int termination_time;
	struct onvif_filters filters;
	char dest_address[COMMON_SIZE*4];	
	char ss_address[COMMON_SIZE*4];
} arg_Subscribe;

typedef struct _arg_SetSync {
	int	ss_id;
} arg_SetSync;

typedef struct _arg_RelayOption
{
	char token[COMMON_SIZE];
	int mode;	// 0 : Monostable and Bistable, 1: Only Monostable, 2: Only Bistable
	int delay_msec[ONVIF_RELAY_DELAY_CNT];
	int is_delay_discrete;	   // if this is ture, delay_msec[0] is maxium and delay_msec[1] is minimum.
}arg_RelayOption;

typedef struct _arg_RelayOptions
{
	int relay_op_cnt;
	int token_index;
	char relay_token[COMMON_SIZE];
	arg_RelayOption relay_op[ONVIF_RELAY_CNT];
}arg_RelayOptions;

typedef struct _arg_DigitalInput
{
	char token[COMMON_SIZE];
	int idle_state;
}arg_DigitalInput;

typedef struct _arg_DigitalInputs
{
	int cnt;
	arg_DigitalInput digital_input[ONVIF_ALARMIN_CNT];
}arg_DigitalInputs;

typedef struct _arg_DeviceReady
{
	int result;
}arg_DeviceReady;

typedef struct _arg_GetServices
{
	int	service_flag;
	int	service_cnt;
	char	service_url[SIZE_OF_SERVICES][COMMON_SIZE];
	int	vs_cnt;
	int	as_cnt;	
	int	alarmin_cnt;	
	int	relay_cnt;
	int	ntp;	
	int	snapshot;
	int	max_profiles;
	int	rtp_multicast;
	int	rtp_tcp;
	int	rtp_rtsp_tcp;
	int websocket;
	_Bool RuleSupport;
	_Bool AnalyticsModuleSupport;
	_Bool CellBasedSceneDescriptionSupported;
	char media2_configuration[COMMON_SIZE*2];
}arg_GetServices;

/////////////////////////////////////////////////////////////////////////////////


#define DEVICE_INFO_SHM_ID	(1212)
#define DEVICE_INFO_SEM_ID	(DEVICE_INFO_SHM_ID + 1)

 
#define EVENT_SHM_ID    (3083)
#define EVENT_SEM_ID    (EVENT_SHM_ID+1)
#define EVENT_BN_PROC_SHM_ID    (2848)
#define EVENT_PULL_PROC_SHM_ID  (5294)
#define EVENT_BN_PROC_SEM_ID    (EVENT_BN_PROC_SHM_ID+1)
#define EVENT_PULL_PROC_SEM_ID  (EVENT_PULL_PROC_SHM_ID+1)
#define SEARCH_REC_SHM_ID    (6648)
#define SEARCH_REC_SEM_ID    (SEARCH_REC_SHM_ID+1)



#define MOTION_FLAGS_CHANGED    (1<<0)
#define ALARM_FLAGS_CHANGED     (1<<1)
#define TIME_TICK_CHANGED       (1<<2)
/* event difinition */

typedef struct _arg_GetRecordingJobStateChangeEvent {
	int JobTokenRef;
	char Topic[COMMON_SIZE];
	char JobToken[COMMON_SIZE];
	char state[COMMON_SIZE];
	arg_RecordingJobConfiguration conf;	
} arg_GetRecordingJobStateChangeEvent;


/* connect nf_host to onvif-enent.fcgi */
enum {
	ONVIF_SEARCH_RECORDING = 0,
	ONVIF_SEARCH_EVENT,
	ONVIF_SEARCH_PTZ,
	ONVIF_SEARCH_METADATA
};

enum OnvifEventKeys
{
	E_KEY_MOTION = 0,
	E_KEY_ALARMIN,
	E_KEY_MOTION_INIT,
	E_KEY_ALARMIN_INIT,
	E_KEY_G_MOTION,
	E_KEY_G_MOTION_INIT
};

enum {
	OV_PROP_SENSOR = 100,
	OV_PROP_MOTION,
	OV_PROP_G_MOTION,
	OV_PROP_RECORDING_JOB_STATE_CHANGE,
	OV_PROP_CONFIGURATION_CHANGE,
	OV_PROP_DATA_DELETION,
	OV_PROP_RECORDING_AND_TRACK,
	
	
	EVENT_SHM_MOTION_INIT = 200,
	EVENT_SHM_ALARMIN_INIT,
	EVENT_SHM_G_MOTION_INIT,
	EVENT_SHM_RECORDING_JOB_STATE_CHANGE,
    EVENT_SHM_CONFIGURATION_CHANGE,
    EVENT_SHM_DATA_DELETION,
	EVENT_SHM_RECORDING_AND_TRACK,

};

#define ADD_SHM_BN_PROC     (2)

struct event_proc{
	int     id, filter_flags;
	time_t  t_time, s_time;
	long    u_time;
	unsigned int old_start_idx;
	unsigned int mbuf_end;	
};

#define EVENT_MSGBUF_SHM_ID (5678)
#define EVENT_MSGBUF_SEM_ID (EVENT_MSGBUF_SHM_ID+1)


#define MAX_REC_SEARCH_SESSION (10)

struct event_msg{
	unsigned int key; // what event??
	unsigned int ch; // channel
	unsigned int value; // value
	time_t  time_tick; 
	long    uptime_tick;
	int ss_id;
};

typedef struct _EVENT_MGSBUF_SHM{
	struct event_msg    msg_buf[MAX_MSG_BUF];
	unsigned int    start;
	unsigned int    end;
} EVENT_MSGBUF_SHM;


extern int event_msgbuf_shm_append(unsigned int event_id, unsigned int flags, unsigned int ch);

typedef struct _EVENT_STATUS_SHM{
	unsigned int motion_flags;
	unsigned int alarm_flags;
	unsigned int recording_job_state_chage;
	unsigned int recording_configuration_change;
	unsigned int recording_data_deletion;
	unsigned int recording_recording_and_track;
	
} EVENT_STATUS_SHM;

extern int  event_status_shm_write(int event_id, unsigned int event_status_flag);

typedef enum _EVENT_MSG_FILTER {
	ONVIF_F_MANDATORY           = 1<<0,
	ONVIF_F_MOTION              = 1<<1,
	ONVIF_F_ALARM               = 1<<2,
	ONVIF_F_MOTION_ON           = 1<<3,
	ONVIF_F_MOTION_OFF          = 1<<4,
	ONVIF_F_ALARM_OPEN          = 1<<5,
	ONVIF_F_ALARM_CLOSE         = 1<<6,
	ONVIF_F_PROPERTY_OP_INIT    = 1<<7,
	ONVIF_F_PROPERTY_OP_CHANGE  = 1<<8,
	ONVIF_F_G_MOTION            = 1<<9,
	ONVIF_F_G_MOTION_ON         = 1<<10,
	ONVIF_F_G_MOTION_OFF        = 1<<11,

	ONVIF_F_NOTING              = 1<<31
}EVENT_MSG_FILTER;

int g_event_status_init(void);
int g_event_status_lock(void);
int g_event_status_trylock(void);
int g_event_status_unlock(void);


#define  ONVIF_PREFIX_SEARCH_REC_TOKEN	"srt"
#define  ONVIF_PREFIX_REC_TOKEN	"rs"
#define  STR2INT_SEARCH_TOKEN(t) atoi(t + strlen(ONVIF_PREFIX_SEARCH_REC_TOKEN))

#if 0 //[[ Jeonghun_0130914_BEGIN -- del
typedef struct _search_filter 
{
	int 			type; /* Recordings, Events, Metadata, PTZs  */
	unsigned int	timeout;
	char 		search_token[COMMON_SIZE];
	NF_LOG_PARAM param;
} search_filter;
#endif //]] Jeonghun_0130914_END -- del

enum {
	TYPE_TRACK_VIDEO = 0,
	TYPE_TRACK_AUDIO,
	TYPE_TRACK_METADATA
};
enum {
	TYPE_RECORDING = 0,
	TYPE_TRACK,
	TYPE_METADATA,	
};
typedef struct _track_info {
	guint64   start_time; 
	guint64   end_time;
	int tracktype;
} track_info;
#define MAX_RECORDING_EVENT 128
typedef struct _recording_info {
	guint64   start_time; 
	guint64   end_time;
	char ch;
	char track_cnt;
	track_info   track[MAX_RECORDING_EVENT];
} recording_info;





typedef struct _recordingEvent_info {
	char ch;
	time_t  time; 
	unsigned short eventType; // event type �� Recording , Track�� ���� union �� �°� ���� ��.
	char trackToken[COMMON_SIZE];
	union {
		gboolean is_recording;
		gboolean  is_data_present;
	};

} recordingEvent_info;
typedef struct _arg_SearchResult {
	REC_FIND_TYPE FindType;
	char SearchToken[COMMON_SIZE];
	int MinResult;
	int MaxResult;
	int WaitTime;
	int SearchState;
	int RecInfoCnt;
	recording_info RecInfo[ONVIF_CH];
} arg_SearchResult;

typedef struct _arg_EventSearchResult {
	char SearchToken[COMMON_SIZE];
	int MinResult;
	int MaxResult;
	int WaitTime;
	int SearchState;
	int RecInfoCnt;
	recordingEvent_info eventInfo[MAX_RECORDING_EVENT];
} arg_EventSearchResult;
struct search_session{
	REC_FIND_TYPE FindType;
	time_t StartPoint;
	time_t EndPoint;
	
	char is_using;
	char search_state;
	unsigned int token;	// 0 is "srt0"
	int wp;
	int rp;
	recording_info rec_info[MAX_MSG_BUF];	
};

typedef struct _SEARCH_REC_SHM{
	struct search_session sess[MAX_REC_SEARCH_SESSION];	
} SEARCH_REC_SHM;


enum {
	SEARCH_STATE_QUEUED = 0,
	SEARCH_STATE_SEARCHING,
	SEARCH_STATE_COMPLETED,
	SEARCH_STATE_UNKNOWN
};


int push_rec_info_shm(unsigned int token, recording_info *rec_info);

//baek.add.end.2011.02.21 for PullMessage

/* Audio DB Not Supported 1.02 version 
typedef struct _arg_AudioSource
{
	char token[COMMON_SIZE];
	int audio_type;
}arg_AudioSource;

typedef struct _arg_AudioSources
{
	int audio_cnt;
	arg_AudioSource source[MAX_PROFILE];
}arg_AudioSources;
*/

void onvif_event_init(void);

#if 0 //[[ Jeonghun_0130801_BEGIN -- del
// fuction namespace ....
void onvif_GetScope(arg_Scope *tmp);
void onvif_GetNetworkInfo(arg_NetworkInfo *tmp);
void onvif_SetNetworkInfo(arg_NetworkInfo *tmp);
void onvif_SystemReboot(arg_SystemReboot *tmp);
void onvif_GetDeviceInformation(arg_GetDeviceInformation *tmp);
void onvif_GetNTP(arg_NTP *tmp);
void onvif_SetNTP(arg_NTP *tmp);
void onvif_GetNetworkPort(arg_NetworkPort *tmp);
void onvif_SetNetworkPort(arg_NetworkPort *tmp);
void onvif_GetDateTime(arg_DateTime *tmp);
void onvif_SetDateTime(arg_DateTime *tmp);
void onvif_FactoryDefault(arg_FactoryDefault *tmp);
void onvif_GetScope(arg_Scope *tmp);
void onvif_SetScope(arg_Scope *tmp);
void onvif_AddScope(arg_Scope *tmp);
void onvif_RemoveScope(arg_Scope *tmp);
void onvif_GetDiscovery(arg_Discovery *tmp);
void onvif_SetDiscovery(arg_Discovery *tmp);
void onvif_GetRelays(arg_Relays *tmp);
void onvif_SetRelaySettings(arg_Relay *tmp);
void onvif_SetRelayState(arg_Relay *tmp);
void onvif_GetDNS(arg_DNS *tmp);
void onvif_SetDNS(arg_DNS *tmp);
void onvif_GetHostName(arg_HostName *tmp);		
void onvif_SetHostName(arg_HostName *tmp);	
void onvif_CreateProfile(arg_ProfileDB *tmp);
void onvif_GetProfileDB(arg_ProfileDB *tmp);
void onvif_GetProfilesDB(arg_ProfilesDB *tmp);
void onvif_AddVideoSource(arg_AddConfig *tmp);
void onvif_AddVideoEncoder(arg_AddConfig *tmp);
void onvif_AddAudioSource(arg_AddConfig *tmp);
void onvif_AddAudioEncoder(arg_AddConfig *tmp);
void onvif_AddMetadataConfiguration(arg_AddConfig *tmp);
void onvif_RemoveAudioEncoder(arg_RemoveConfig *tmp);
void onvif_RemoveAudioSource(arg_RemoveConfig *tmp);
void onvif_RemoveVideoEncoder(arg_RemoveConfig *tmp);
void onvif_RemoveVideoSource(arg_RemoveConfig *tmp);
void onvif_RemoveMetadataConfiguration(arg_RemoveConfig *tmp);
void onvif_DeleteProfile(arg_ProfileDB *tmp);
void onvif_GetImagingOption(arg_ImagingOption *tmp);	
void onvif_GetImagingSettings(arg_ImagingOption *tmp);
void onvif_SetImagingOption(arg_ImagingOption *tmp);
//void onvif_GetVideoSource(arg_VideoSource *tmp); 
void onvif_GetVideoSources(arg_VideoSources *tmp); 
void onvif_GetAudioSources(arg_AudioSources *tmp); 
void onvif_GetVideoSourceConfiguration(arg_VideoSource *tmp);
void onvif_GetVideoSourceConfigurations(arg_VideoSources *tmp);
void onvif_GetCompatibleVideoSource(arg_VideoSources *tmp);
void onvif_GetVideoSourceOption(arg_VideoSourceOption *tmp);
void onvif_GetVideoEncoder(arg_VideoEncoder *tmp);		
void onvif_GetVideoEncoders(arg_VideoEncoders *tmp);		
void onvif_GetVideoEncoderOption(arg_VideoEncoderOption *tmp); 
void onvif_GetCompatibleVideoEncoder(arg_VideoEncoders *tmp);
void onvif_GetStreamUri(arg_StreamUri *tmp); 		
void onvif_SetVideoEncoder(arg_VideoEncoder *tmp);
void onvif_GetAudioSourceConfiguration(arg_AudioSource *tmp);
void onvif_GetAudioSourceConfigurations(arg_AudioSources *tmp);
void onvif_GetAudioSourceOption(arg_AudioSourceOption *tmp);
void onvif_GetCompatibleAudioSource(arg_AudioSources *tmp);
void onvif_GetCompatibleAudioEncoder(arg_AudioEncoders *tmp);
void onvif_CheckTokens(arg_Tokens *tmp);
void onvif_SetAudioEncoder(arg_AudioEncoder *tmp);
void onvif_SetAudioSource(arg_AudioSource *tmp);
void onvif_GetAudioEncoderConfiguration(arg_AudioEncoder *tmp);
void onvif_GetAudioEncoderConfigurations(arg_AudioEncoders *tmp);
void onvif_GetCompatibleMetadataConfigurations(arg_Metadatas *tmp);
void onvif_GetMetadataConfigurations(arg_Metadatas *tmp);
void onvif_SetMetadataConfiguration(arg_Metadata *tmp);
void onvif_GetMetadataConfiguration(arg_Metadata *tmp);
void onvif_SetVideoSource(arg_VideoSource *tmp);
void onvif_GetUser(arg_UserType *tmp);
void onvif_SetUser(arg_UserType *tmp);
void onvif_CreateUser(arg_UserType *tmp);
void onvif_DeleteUser(arg_UserType *tmp);
void onvif_SetImagingSettings(arg_ImagingOption *tmp);
void onvif_GetOptions(arg_GetOption *tmp);
void onvif_Move(arg_MoveOption *tmp);
void onvif_GetStatus(arg_MoveStatus *tmp);
void onvif_GetCapability(arg_GetCapa *tmp);
void onvif_Passfd(#define data, int pass_fd);							
void onvif_GotoHomePosition(arg_PTZOperation *tmp);
void onvif_AbsoluteMove(arg_PTZOperation *tmp);
void onvif_ContinuousMove(arg_PTZOperation *tmp);
void onvif_SetImagingOption(arg_ImagingOption *tmp);
void onvif_CreatePresetTour(arg_PTZOperation *tmp);
void onvif_GetConfiguration(arg_PTZOperation *tmp);
void onvif_GetConfigurationOptions(arg_PTZOperation *tmp);
void onvif_GetPTZConfigurations(arg_PTZNodes *tmp);
void onvif_GetPTZConfiguration(arg_PTZNode *tmp);
void onvif_AddPTZConfiguration(arg_PTZNode *tmp);
void onvif_SetPTZConfiguration(arg_PTZNode *tmp);
void onvif_GetNode(arg_PTZNode *tmp);
void onvif_GetNodes(arg_PTZNodes *tmp);
void onvif_GetPresets(arg_PTZPresetNodes *tmp);
void onvif_GetPresetTour(arg_PTZOperation *tmp);
void onvif_GetPresetTourOptions(arg_PTZOperation *tmp);
void onvif_GetPresetTours(arg_PTZOperation *tmp);
void onvif_GetServiceCapabilities(arg_PTZOperation *tmp);
void onvif_PTZGetStatus(arg_PTZOperation *tmp);
void onvif_GotoPreset(arg_PTZPresetNode *tmp);
void onvif_ModifyPresetTour(arg_PTZOperation *tmp);
void onvif_OperatePresetTour(arg_PTZOperation *tmp);
void onvif_RelativeMove(arg_PTZOperation *tmp);
void onvif_RemovePreset(arg_PTZPresetNode *tmp);
void onvif_RemovePresetTour(arg_PTZOperation *tmp);
void onvif_SendAuxiliaryCommand(arg_PTZOperation *tmp);
void onvif_SetConfiguration(arg_PTZOperation *tmp);
void onvif_SetHomePosition(arg_PTZOperation *tmp);
void onvif_SetPreset(arg_PTZPresetNode *tmp);
void onvif_Stop(arg_Stop *tmp);
void onvif_FocusStop(arg_FocusStop *tmp);
#endif //]] Jeonghun_0130801_END -- del



	/* 25 */
#if defined(WIN32)
#define _TTY_LOG_ONVIF_DEBUG printf
#define _TTY_LOG_ONVIF printf
#define OV_DEBUG printf
int nf_sysdb_set_str(char * key, char* value);
gchar* nf_sysdb_get_str_nocopy(gchar* key);
int nf_sysdb_get_uint(gchar* key);
gboolean nf_sysdb_set_uint(gchar* key, int value);
int nf_sysdb_get_int(gchar* key);
gboolean nf_sysdb_set_int(gchar* key, int value);
gboolean nf_sysdb_get_bool(gchar* key);
gboolean nf_sysdb_set_bool(gchar* key, int value);



#else
			#if 0
			void onvif_NCX_GetWsdlUrl(arg_GetWsdlUrl *tmp);									/* 10 */
			void onvif_NCX_SetImagingOption(arg_ImagingOption *tmp);						/* 21 */


			void onvif_NCX_Printf(arg_PrintfType *tmp);
			void onvif_NCX_GetEventStatus(arg_GetEventStatus *tmp);
			#endif


			#if 1
			#define _LOG_CON(fmt, args...)  {   \
						FILE *pts0;                         \
						pts0 = fopen("/dev/console", "a+");    \
						if(pts0 != NULL) {              \
										fprintf(pts0, fmt, ##args); \
										fclose(pts0);                           \
									} \
			}
			#else
			#define _LOG_CON(fmt, args...)
			#endif

			#if 1
			#define _LOG_TEL(fmt, args...)  {   \
						FILE *pts0;                         \
						pts0 = fopen("/dev/pts/0", "a+");    \
						if(pts0 != NULL) {              \
										fprintf(pts0, fmt, ##args); \
										fclose(pts0);                           \
									} \
			}
			#else
			#define _LOG_TEL(fmt, args...)
			#endif

			#define OV_DEBUG(fmt, args...)	{_LOG_CON("[ONVIF:%s-%d]", __FUNCTION__, __LINE__);_LOG_CON(fmt, ##args);_LOG_CON("\n"); }
			#define OV_DEBUG_RAW(fmt, args...)	{_LOG_CON(fmt, ##args);}

			#if 0
			#define _LOG_PTS(fmt, args...)  {   \
						FILE *pts0;                         \
						time_t now_debug;    \
						time(&now_debug);\
						char *time_s = ctime(&now_debug); \
						pts0 = fopen("/dev/pts/0", "a+");    \
						if(pts0 != NULL) {              \
										fprintf(pts0, "["); \
										while( *time_s != '\n' ){ \
															fprintf(pts0, "%c", *time_s);\
															time_s++; \
														} \
										fprintf(pts0, "]: "); \
										fprintf(pts0, fmt, ##args); \
										fprintf(pts0, "\n"); \
										fclose(pts0);                           \
									} \
			}
			#else
			#define _LOG_PTS(fmt, args...)
			#endif


			#if 0
					#define ONVIF_DEBUG_MODE 1
					#define _TTY_LOG_ONVIF_DEBUG(fmt, args...) if(ONVIF_DEBUG_MODE) _TTY_LOG_ONVIF(fmt, ##args)
					#define _TTY_LOG_ONVIF(fmt, args...)  {   \
								FILE *pts0;                         \
								time_t now_debug;    \
								time(&now_debug);\
								char *time_s = ctime(&now_debug); \
								pts0 = fopen("/dev/pts/0", "a+");    \
								if(pts0 != NULL) {              \
												fprintf(pts0, "["); \
												while( *time_s != '\n' ){ \
																	fprintf(pts0, "%c", *time_s);\
																	time_s++; \
																} \
												fprintf(pts0, "]: "); \
												fprintf(pts0, fmt, ##args); \
												fprintf(pts0, "\n"); \
												fclose(pts0);                           \
											} \
					}
					#else
					#define _TTY_LOG_ONVIF(fmt, args...)
					#define _TTY_LOG_ONVIF_DEBUG(fmt, args...)
					#endif

					#if 0
					#define _TTY_LOG_TMP(fmt, args...)  {   \
								FILE *pts0;                         \
								time_t now_debug;    \
								time(&now_debug);\
								char *time_s = ctime(&now_debug); \
								pts0 = fopen("/tmp/baek.log", "a+");    \
								if(pts0 != NULL) {              \
												fprintf(pts0, "["); \
												while( *time_s != '\n' ){ \
																	fprintf(pts0, "%c", *time_s);\
																	time_s++; \
																} \
												fprintf(pts0, "]: "); \
												fprintf(pts0, fmt, ##args); \
												fprintf(pts0, "\n"); \
												fclose(pts0);                           \
											} \
					}
					#else
					#define _TTY_LOG_TMP(fmt, args...)
					#endif

					#define LOCK_SEM(semid) { \
							wait_and_lock( semid ); \
					}

					#define UNLOCK_SEM(semid) { \
							signal_and_unlock( semid ); \
					}

#endif

typedef enum _ONVIF_PROTOCOL_E {
    //--- predefined packet type     1 ~ 32
    ONVIF_PROTOCOL_REQUEST          = 1,    // request
    ONVIF_PROTOCOL_REPLY            = 2,    // Rsponse


    //--- predefined request codes   33 ~ 64
    ONVIF_PROTOCOL_CONNECT          = 33,
    ONVIF_PROTOCOL_DISCONNECT       = 34,


    //--- basic error code      // 200 ~ 232
    ONVIF_PROTOCOL_SUCCESS          = 200,
    ONVIF_PROTOCOL_PROTOCOL     = 201,  // protocol error
    ONVIF_PROTOCOL_UNKNOWNREQUEST   = 202,
    ONVIF_PROTOCOL_FAULT            = 203,  // invalid parameters/data
    ONVIF_PROTOCOL_FAIL         = 204,  // request failed
    ONVIF_PROTOCOL_INTERNAL     = 205,  // internal implementation error
    ONVIF_PROTOCOL_NETWORK          = 206,
    ONVIF_PROTOCOL_ERROR_PRINT      = 207,

}ONVIF_PROTOCOL_E;


typedef enum _ONVIF_ERR_RET_E {
	ONVIF_ERR_RET_SUCCESS           = 0,        // choissi FIXME

	ONVIF_ERR_RET_SOCKET            = 1,
	ONVIF_ERR_RET_INTERNAL          = 2,
	ONVIF_ERR_RET_PARAMETER     = 3,
	ONVIF_ERR_RET_AUTH              = 4,
	ONVIF_ERR_RET_FW_UP         = 5,        // YOON
	ONVIF_ERR_UNKNOWN_ID		= 6,

	ONVIF_ERR_RET_SEND              = ONVIF_ERR_RET_INTERNAL,
	ONVIF_ERR_RET_RECV              = ONVIF_ERR_RET_INTERNAL,

	ONVIF_R_ERR	= 100,
	ONVIF_R_OK,		
	ONVIF_R_ERR_USER_NOT_FOUND,
	ONVIF_R_ERR_FIXED_USER,
	ONVIF_R_ERR_INVALID_PARAM,
	ONVIF_R_ERR_NO_CONFIG,
	ONVIF_R_ERR_PASS_TOO_WEAK,
	ONVIF_R_ERR_USERNAME_CONFLICT,
	ONVIF_R_ERR_TOO_MANY_USER,
	ONVIF_R_ERR_CONFIG_MODIFY,
	ONVIF_R_ERR_NO_ENTITY,
	ONVIF_R_ERR_INVALID_ARG_VAL,
	ONVIF_R_ERR_NO_PROFILE,
	ONVIF_R_ERR_NO_SOURCE,
	ONVIF_R_ERR_SETTING_INVALID,
	ONVIF_R_ERR_MAX_NVT_PROFILES,
	ONVIF_R_ERR_PROFILE_EXIST,
	ONVIF_R_ERR_TOO_MANY_PRESET,
	ONVIF_R_ERR_NO_RECORDING,
	ONVIF_R_ERR_NO_RECORDINGJOBS,
	ONVIF_R_ERR_NOT_SUPPORT,
	ONVIF_R_ERR_NO_PRESET,


}ONVIF_ERR_RET_E;

#define MAX_FILTER_CNT  (10)

#endif





//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh


#if defined(_IPX_32P5)
#define MAX_MSG_BUF (400)
#else
#define MAX_MSG_BUF (200)
#endif

#define str_tns1_tnsitx_conf_change "tns1:RecordingConfig/RecordingConfiguration\0"
#define str_tns1_tnsitx_data_deletion "tns1:RecordingConfig/DeleteTrackData\0"
#define str_tns1_recording_and_track "tns1:RecordingConfig/CreateRecording\0"
#define str_tns1_tnsitx_Motion "tns1:VideoAnalytics/tnsitx:MotionDetection\0"
#define str_tns1_VideoAnalytics_Motion "tns1:RuleEngine/CellMotionDetector/Motion\0"
#define str_tns1_tnsitx_Alarm "tns1:UserAlarm/AlarmIN\0"
#define str_tns1_General_Motion "tns1:VideoSource/MotionAlarm\0"
#define str_tns1_digital_input "tns1:Device/Trigger/DigitalInput\0"
#define str_tns1_relay "tns1:Device/Trigger/Relay\0"
#define str_tns1_GlobalSceneChange "tns1:VideoSource/GlobalSceneChange/AnalyticsService\0"
#define str_tns1_VideoSignalLoss "tns1:VideoSource/SignalLoss\0"
#define str_tns1_ConfigurationVideoSourece "tns1:Configuration/VideoSourceConfiguration/DeviceIOService\0"
#define str_tns1_tnsitx_AdvancedSensor "tns1:UserAlarm/tnsitx:AdvancedSensor\0"
#define str_tns1_tnsitx_EventConfChange "tns1:UserAlarm/tnsitx:EventConfChange\0"
#define str_tns1_tnsitx_RecordingHistoryRecordingState"tns1:UserAlarm/tnsitx:EventConfChange\0"
#define str_tns1_tnsitx_RecordingConfigTrackConfig "tns1:RecordingConfig/TrackConfiguration\0"
#define str_tns1_tnsitx_jobState_change "tns1:RecordingConfig/JobState\0"
#define str_tns1_ProfileChange "tns1:Media/ProfileChanged\0"
#define str_tns1_ConfigurationChange "tns1:Media/ConfigurationChanged\0"
#define str_tns1_ImageBlur "tns1:VideoSource/ImageTooBlurry/ImagingService\0"
// #define str_tns1_ImageDark "tns1:VideoSource/ImageTooDark/ImagingService\0"
// #define str_tns1_ImageBright "tns1:VideoSource/ImageTooBright/ImagingService\0"
// #define str_tns1_ImageGlobalSceneChange "tns1:VideoSource/GlobalSceneChange/ImagingService\0"
#define str_tns1_dummy "tns1:dummy\0"
//ONVIF_NEW_EVENT
//#define str_new_topic  "tns1:RecordingConfig/JobState\0"

#define str_JobStateChangeToken  "RecordingJobToken\0"
#define str_confChangeToken  "RecordingJobToken\0"
#define str_dataDeletionToken  "RecordingJobToken\0"
#define str_RecordingAndTrack  "RecordingJobToken\0"
#define str_rs0  "rs0\0"
#define str_rs1  "rs1\0"
#define str_rs2  "rs2\0"
#define str_rs3  "rs3\0"
#define str_rs4  "rs4\0"
#define str_rs5  "rs5\0"
#define str_rs6  "rs6\0"
#define str_rs7  "rs7\0"
#define str_rs8  "rs8\0"
#define str_rs9  "rs9\0"
#define str_rs10  "rs10\0"
#define str_rs11  "rs11\0"
#define str_VideoSourceConfigurationToken  "VideoSourceConfigurationToken\0"
#define str_general_vsource  "general_vsource\0"
#define str_vs0  "vs0\0"
#define str_vs1  "vs1\0"
#define str_vs2  "vs2\0"
#define str_vs3  "vs3\0"
#define str_vs4  "vs4\0"
#define str_vs5  "vs5\0"
#define str_vs6  "vs6\0"
#define str_vs7  "vs7\0"
#define str_vs8  "vs8\0"
#define str_vs9  "vs9\0"
#define str_vs10  "vs10\0"
#define str_vs11  "vs11\0"
#define str_vs12  "vs12\0"
#define str_vs13  "vs13\0"
#define str_vs14  "vs14\0"
#define str_vs15  "vs15\0"
#define str_MotionActive  "MotionActive\0"
#define str_true  "true\0"
#define str_false  "false\0"
#define str_Port  "Port\0"
#define str_0  "0\0"
#define str_1  "1\0"
#define str_2  "2\0"
#define str_3  "3\0"
#define str_4  "4\0"
#define str_5  "5\0"
#define str_6  "6\0"
#define str_7  "7\0"
#define str_8  "8\0"
#define str_9  "9\0"
#define str_10  "10\0"
#define str_11  "11\0"
#define str_12  "12\0"
#define str_13  "13\0"
#define str_14  "14\0"
#define str_15  "15\0"
#define str_16  "16\0"
#define str_99  "99\0"
#define str_State  "State\0"
#define str_open  "open\0"
#define str_close  "close\0"
#define str_Source  "Source\0"
#define str_Data  "Data\0"
#define str_Initialized  "Initialized\0"
#define str_Changed  "Changed\0"
#define str_InputToken  "InputToken\0"
#define str_RelayToken  "RelayToken\0"
#define str_LogicalState  "LogicalState\0"
#define str_Inactive  "inactive\0"
#define str_Active  "active\0"
#define str_Sabotage  "Sabotage\0"
