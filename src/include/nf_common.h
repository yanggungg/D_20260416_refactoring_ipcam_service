/** @file	nf_common.h
	@brief	nf_common ????????
*/

#ifndef _NF_COMMON_H
#define _NF_COMMON_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>

#include "nf_common_util.h"

#include "nf_object.h"
#include "nf_sysdb.h"
#include "nf_notify.h"

#define BASE_IPCAM_CHANNEL      32
#define BASE_IPCAM_2ND_CHANNEL	32
#define BASE_AUDIO_CHANNEL      64

#define ENABLE_CALENDAR_OPTIMIZE
//#define ENABLE_PROJECT_KMW
//#define ENABLE_PROJECT_KUMMI
//#define ENABLE_RTSP_ENCRYPTION		// choissi test

/******************************************************************************/
#if defined(_IPX_1648P4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (8)
	#define NUM_ACTIVE_CH_HUB       (8)
	#define NUM_ACTIVE_CH			(16)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(2)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0824P4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(8)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(2)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_1648L4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (8)
	#define NUM_ACTIVE_CH_HUB       (8)
	#define NUM_ACTIVE_CH			(16)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(2)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0824L4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(8)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(2)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0412L4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(4)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0x000F)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC     (2)
		#define NUM_ALARM_DVR           (2)
		#if 0
			#define NUM_ALARM_IPCAM         (8)
		#else
			#define NUM_ALARM_IPCAM         (16)
		#endif
		#define NUM_ALARM               (NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK        (0x000000FF)

		#define NUM_RELAY_DVR           (1)
		#if 0
			#define NUM_RELAY_IPCAM         (8)
		#else
			#define NUM_RELAY_IPCAM         (16)
		#endif
		#define NUM_RELAY               (NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK        (0x000000FF)
	#else
		#define NUM_ALARM_ARI_PANIC     (2)
		#define NUM_ALARM               (2)
		#define NUM_RELAY               (1)
	#endif

	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_1648M4)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
//	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (8)
	#define NUM_ACTIVE_CH_HUB       (8)
	#define NUM_ACTIVE_CH			(16)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(2)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0824M4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	//#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(8)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(0)
		#define NUM_ALARM_DVR          	(8)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0412M4)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	//#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(4)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0x000F)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC     (0)
		#define NUM_ALARM_DVR           (4)
		#if 0
			#define NUM_ALARM_IPCAM         (8)
		#else
			#define NUM_ALARM_IPCAM         (16)
		#endif
		#define NUM_ALARM               (NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK        (0x000000FF)

		#define NUM_RELAY_DVR           (1)
		#if 0
			#define NUM_RELAY_IPCAM         (8)
		#else
			#define NUM_RELAY_IPCAM         (16)
		#endif
		#define NUM_RELAY               (NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK        (0x000000FF)
	#else
		#define NUM_ALARM_ARI_PANIC     (2)
		#define NUM_ALARM               (2)
		#define NUM_RELAY               (1)
	#endif

	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_32M4E)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
//	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(32)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(32)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (32)
	#define NUM_ACTIVE_CH_HUB       (0)
	#define NUM_ACTIVE_CH			(32)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFFFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(8)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(NUM_ACTIVE_CH)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0xFFFFFFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(NUM_ACTIVE_CH)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0xFFFFFFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(0)
		#define NUM_ALARM				(8)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)


#elif defined(_IPX_1648M4E)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
//	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (16)
	#define NUM_ACTIVE_CH_HUB       (0)
	#define NUM_ACTIVE_CH			(16)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(8)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(0)
		#define NUM_ALARM				(8)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0824M4E)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
		//#define ENABLE_FAN_FAIL_CHECK /** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
			//#define ENABLE_SYSFAN2
			//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/
	
	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(8)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)
	
	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(0)
		#define NUM_ALARM_DVR          	(8)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)
	
		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif
	
	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)
	
	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)
	
	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/
	
	#define NUM_SOLO         		(2)
	
#elif defined(_IPX_0412M4E)
	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
		//#define ENABLE_FAN_FAIL_CHECK /** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
			//#define	ENABLE_SYSFAN2
			//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/
	
	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(4)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0x000F)
	
	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC     (0)
		#define NUM_ALARM_DVR           (4)
		#if 0
			#define NUM_ALARM_IPCAM         (8)
		#else
			#define NUM_ALARM_IPCAM         (16)
		#endif
		#define NUM_ALARM               (NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK        (0x000000FF)
	
		#define NUM_RELAY_DVR           (1)
		#if 0
			#define NUM_RELAY_IPCAM         (8)
		#else
			#define NUM_RELAY_IPCAM         (16)
		#endif
		#define NUM_RELAY               (NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK        (0x000000FF)
	#else
		#define NUM_ALARM_ARI_PANIC     (2)
		#define NUM_ALARM               (2)
		#define NUM_RELAY               (1)
	#endif
	
	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)
	
	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)
	
	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/
	
	#define NUM_SOLO         		(2)
	
#elif defined(_IPX_32P4E)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(32)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(32)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (32)
	#define NUM_ACTIVE_CH_HUB       (0)
	#define NUM_ACTIVE_CH			(32)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFFFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(16)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(NUM_ACTIVE_CH)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0xFFFFFFFF)

		#define NUM_RELAY_DVR			(16)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(NUM_ACTIVE_CH)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0xFFFFFFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define ENABLE_ARI_PANIC
	#if defined(ENABLE_ARI_PANIC)
		#define ARI_PANIC_LOCATION	(16)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)
	
#elif defined(_IPX_1648P4E)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(16)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(16)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (16)
	#define NUM_ACTIVE_CH_HUB       (0)
	#define NUM_ACTIVE_CH			(16)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(16)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(16)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define ENABLE_ARI_PANIC			/*ARI, PANIC ACTIVE__ 200409 add by jr695*/
	#if defined(ENABLE_ARI_PANIC)
		#define ARI_PANIC_LOCATION	(16)
	#endif

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_0824P4E)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
	//#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		//#define	 ENABLE_SYSFAN2
		//#define	 ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(8)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(8)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH			(8)
	#define NUM_ACTIVE_SOPT_CH		(0)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_DVR          	(8)
		#if 1
			#define NUM_ALARM_IPCAM        	(16)
		#else
			#define NUM_ALARM_IPCAM        	(16)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0x0000FFFF)

		#define NUM_RELAY_DVR			(1)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(16)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0x0000FFFF)
	#else
		#define NUM_ALARM				(2)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_ALARM_ARI_PANIC		(2)
	#define ENABLE_ARI_PANIC			/*ARI, PANIC ACTIVE__ 200409 add by jr695*/
	#if defined(ENABLE_ARI_PANIC)
		#define ARI_PANIC_LOCATION		(16)
	#endif

	#define NUM_SPOT            	(0)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

#elif defined(_IPX_32P5)

	#define MODEL_TYPE_NVR
	#define ENABLE_SPOT_VLOSS		/** 090821 add by pakkhman **/
//	#define ENABLE_FAN_FAIL_CHECK	/** 090821 add by pakkhman **/
	#if defined(ENABLE_FAN_FAIL_CHECK)
		#define	ENABLE_SYSFAN1
		#define	ENABLE_SYSFAN2
		//#define ENABLE_CPUTEMP
		#define ENABLE_SYSTEMP
	#endif
	#define ENABLE_POE_CHECK		/** 090821 add by pakkhman **/

	#define NUM_CHANNEL         	(32)
	#define NUM_ANALOG_CHANNEL  	(NUM_CHANNEL)
	#define NUM_NETWORK_CHANNEL   	(32)
	#define NUM_IPCAM_CHANNEL   	(0)
	#define NUM_TOTAL_CHANNEL   	(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH_DVR       (16)
	#define NUM_ACTIVE_CH_HUB       (0)
	#define NUM_ACTIVE_CH			(32)
	#define NUM_ACTIVE_SOPT_CH		(2)
	#define NUM_ACTIVE_CH_MASK  	(0xFFFFFFFF)

	#define ENABLE_RELAY_IPCAM
	#if defined(ENABLE_SENSOR_IPCAM)
		#define NUM_ALARM_ARI_PANIC		(2)
		#define NUM_ALARM_DVR          	(16)
		#if 0
			#define NUM_ALARM_IPCAM        	(8)
		#else
			#define NUM_ALARM_IPCAM        	(NUM_ACTIVE_CH)
		#endif
		#define NUM_ALARM				(NUM_ALARM_DVR+NUM_ALARM_IPCAM)
		#define ALARM_IPCAM_MASK       	(0xFFFFFFFF)

		#define NUM_RELAY_DVR			(16)
		#if 0
			#define NUM_RELAY_IPCAM        	(8)
		#else
			#define NUM_RELAY_IPCAM        	(NUM_ACTIVE_CH)
		#endif
		#define NUM_RELAY				(NUM_RELAY_DVR+NUM_RELAY_IPCAM)
		#define RELAY_IPCAM_MASK       	(0xFFFFFFFF)
	#else
		#define NUM_ALARM_ARI_PANIC		(0)
		#define NUM_ALARM				(8)
		#define NUM_RELAY				(1)
	#endif

	#define NUM_AUDIO_MAX           (32)

	#define NUM_SPOT            	(2)
	#define NUM_AUDIO           	(1)
	#define NUM_AUDIO_INPUT         (NUM_AUDIO)

	#define NUM_DSP             	(0)
	#define NUM_ACTIVE_DSP			(0)

	#define MAX_MD_COL          	(16) /*TBD*/
	#define MAX_MD_ROW          	(12) /*TBD*/

	#define NUM_SOLO         		(2)

	#define ENABLE_ARI_PANIC
	#if defined(ENABLE_ARI_PANIC)
		#define ARI_PANIC_LOCATION	(16)
	#endif
	
#else   /* _NF_1648 _NF_BASE_MODEL  */

	#define NUM_CHANNEL				(16)
	#define NUM_ANALOG_CHANNEL      (NUM_CHANNEL)
	#define NUM_IPCAM_CHANNEL		(4)
	#define NUM_TOTAL_CHANNEL		(NUM_ANALOG_CHANNEL+NUM_IPCAM_CHANNEL)
	#define NUM_ACTIVE_CH           (NUM_CHANNEL) /*FIXME*/
	#define NUM_ACTIVE_CH_MASK		(0xFFFF)

	#define NUM_ALARM               (16)
	#define NUM_RELAY               (16)
	#define NUM_SPOT                (4)
	#define NUM_AUDIO               (4)

	#define NUM_DSP                 (2) /**/
	#define NUM_ACTIVE_DSP			(2)

	#define MAX_MD_COL          (22) /*TBD*/
	#define MAX_MD_ROW          (12) /*TBD*/


#endif  /*_ANF_1648*/

/******************************************************************************/

#define NF_SYSDB_DATA_PATH_STR	"/NFDVR/data/"
#define NF_SYSDB_CONF_FILENAME	"/NFDVR/data/nf_sysdb.conf"
#define NF_SYSDB_CONF_VER_FILENAME	"/NFDVR/data/nf_sysdb_version.conf"
#define NF_SYSDB_CONF_PRIVATE_FILENAME	"/NFDVR/data/nf_sysdb_private.conf"
#define NF_SYSDB_CONF_PATH_STR	"/NFDVR/data/nf_sysdb_%s.conf"
#if defined(SUPPORT_NAND_128M)
	#define NF_SYSDB_CONF_IMPORT_PATH_STR	"%s/nf_sysdb_%s.conf"
#else
	#if defined(CONFIG_FWUPGRADE_SINGLE)
		#define NF_SYSDB_CONF_IMPORT_PATH_STR   "%s/nf_sysdb_%s.conf"
	#else
		#define NF_SYSDB_CONF_IMPORT_PATH_STR	"%s/NFDVR/data/nf_sysdb_%s.conf"
	#endif
#endif
#define NF_SYSDB_CONF_FILE_STR	"nf_sysdb_%s.conf"

// 2008-01-01 00:00:00
#define NF_DATETIME_MIN	(1199145600)

// HDD Group
#define 	NUM_STRG_GROUP		5
//enum _group_id_ {INTERNAL = 0, EXTERNAL, IDE_ARCHIVING, INTERNAL_USB, EXTERNAL_USB};
#define 	NUM_STRG			32
#define 	SIZE_OF_DESCRIPTION	256
#define 	NUM_BUZZER_ACT		128
#define 	NUM_LOG_ACT			128
#define 	NUM_EACH_TEMPLATE	32
#define 	NUM_MDPYSEQ			32
#define 	MAX_LENGTH_SEQ		32

// Setup DB Define ------------------------------------------
#define 	NUM_GROUP		32
#define 	NUM_USER		32

#define		MAX_PTZPROTO	5
#define 	MAX_UEVENTLIST	32
#define		MAX_CMDLEN		16
#define 	MAX_STRLEN		32
#define 	MAX_DESCLEN		64
#define 	MAX_BZREVT		32
#define		MAX_POPEVT		32
#define		MAX_EMAILEVT	32
#define		MAX_CLIEVT		32
#define		MAX_LOGEVT		32
#define		MAX_DISSEQ		32
#define		MAX_DISTEMP		32
#define		MAX_DISUNIT		32
#define		MAX_TEMP		32
#define 	MAX_SCHEPARAM	32
#define		MAX_PTZTOUR		32
#define		MAX_PTZPRESET	32
#define		MAX_PTZPATTERN	32
#define		MAX_PTZRECCMD	32
#define 	MAX_DEV			4
#define 	MAX_UPIMG		32
#define		PTZ_PROTONUM	2

// End of Setup DB Define -----------------------------------

// ON/OFF
#define ON 				1
#define OFF 			0

#define PACKED			1
#define UNPACKED		0

#define FULL			1
#define EMPTY			0

#ifndef	TRUE
	#define	TRUE		1
#endif
#ifndef	FALSE
	#define	FALSE		0
#endif

// DONT_CARE
#define ucDONT_CARE		0xff
#define usDONT_CARE		0xffff
#define uDONT_CARE		0xffffffff
#define sDONT_CARE		0xffffffff
#define ptrDONT_CARE	0xffffffff

#define NODEBUG			0x00
#define DEBUG			0x01

typedef enum _NF_CAM_TYPE_E
{
	NF_CAM_TYPE_ANALOG	= 0,
	NF_CAM_TYPE_IP		= 1
} NF_CAM_TYPE_E;

typedef enum _NF_DATE_FORMAT_E
{
	NF_DATE_YYYYMMDD = 0,
	NF_DATE_MMDDYYYY = 1,
	NF_DATE_DDMMYYYY = 2
}NF_DATE_FORMAT_E;

typedef enum _NF_TIME_FORMAT_E
{
	NF_TIME_24H = 0,
	NF_TIME_12H = 1
}NF_TIME_FORMAT_E;

typedef enum _NF_VIDEO_SIG_E
{
	NF_VIDEO_SIG_NTSC	= 0,
	NF_VIDEO_SIG_PAL	= 1
} NF_VIDEO_SIG_E;

typedef enum _NF_DISPLAY_SIG_E
{
    NF_DISPLAY_SIG_D4    = 0,
    NF_DISPLAY_SIG_D1    = 1,
} NF_DISPLAY_SIG_E;

#define REC_TIME_MARGIN		2
#define MAX_POST_REC_TIME	(180+REC_TIME_MARGIN)
#define MAX_PRE_REC_TIME	(15+REC_TIME_MARGIN)
#define MAX_PRE_REC_TIME_HD_LIMIT	(5+1)

#define MAX_RECCFG_TEMPLATE	32

#define _D1_WIDTH			704
#define _D1_NTSC_HEIGHT		480
#define _D1_PAL_HEIGHT		576

#define _4D1_WIDTH			(_D1_WIDTH*2)
#define _4D1_NTSC_HEIGHT	(_D1_NTSC_HEIGHT*2)
#define _4D1_PAL_HEIGHT		(_D1_PAL_HEIGHT*2)

#if defined(_SNF_MODEL)
#define WIDTH_800			800
#define HEIGHT_600			600

#define WIDTH_1280			1280
#define HEIGHT_1024			1024

#define WIDTH_1920			1920
#define HEIGHT_1080			1080
#endif

/* VIDEO defines */
#define MAX_DISPLAY_WIDTH       _4D1_WIDTH
#define MAX_DISPLAY_HEIGHT      _4D1_PAL_HEIGHT

#define DISPLAY_NTSC_RATE		(29.97)
#define DISPLAY_PAL_RATE		(25.0)

#define DISPLAY_D4_WIDTH        _4D1_WIDTH
#define DISPLAY_D1_WIDTH        _D1_WIDTH

#define DISPLAY_NTSC4_HEIGHT    _4D1_NTSC_HEIGHT
#define DISPLAY_PAL4_HEIGHT     _4D1_PAL_HEIGHT
#define DISPLAY_NTSC_HEIGHT     _D1_NTSC_HEIGHT
#define DISPLAY_PAL_HEIGHT      _D1_PAL_HEIGHT

#define D1_WIDTH                _D1_WIDTH
#define D1_NTSC_HEIGHT          _D1_NTSC_HEIGHT
#define D1_PAL_HEIGHT           _D1_PAL_HEIGHT
#define CIF_WIDTH               (D1_WIDTH/2)
#define CIF_NTSC_HEIGHT         (D1_NTSC_HEIGHT/2)
#define CIF_PAL_HEIGHT          (D1_PAL_HEIGHT/2)

extern gint				nf_display_is_pal();
extern gint				nf_display_is_d1();
extern gfloat 			nf_display_get_rate();
extern gint 				nf_display_get_width();
extern gint 				nf_display_get_height();
extern gint 				nf_display_get_d1_height();
extern gint 				nf_display_get_cif_height();
extern gint 				nf_display_get_active_width();
extern gint 				nf_display_get_active_height();

#define DISPLAY_IS_PAL			(nf_display_is_pal())
#define DISPLAY_IS_D1			(nf_display_is_d1())

#define DISPLAY_RATE			(nf_display_get_rate())
#define DISPLAY_WIDTH			(nf_display_get_width())
#define DISPLAY_HEIGHT			(nf_display_get_height())

#define DISPLAY_D1_HEIGHT		(nf_display_get_d1_height())
#define DISPLAY_CIF_HEIGHT		(nf_display_get_cif_height())

#define DISPLAY_ACTIVE_WIDTH	(nf_display_get_active_width())
#define DISPLAY_ACTIVE_HEIGHT	(nf_display_get_active_height())

#define CIF_HEIGHT				(nf_display_get_cif_height())

#define VIDEO_WIDTH				(nf_video_get_width())
#define VIDEO_HEIGHT			(nf_video_get_height())


typedef enum _NF_DISPLAY_E {
	NF_DISPLAY_FULL 		= 0x00, 	/* 1 */
	NF_DISPLAY_QUAD 		= 0x01,		/* 4 */
	NF_DISPLAY_HEXA_A 		= 0x02, 	/* 6, A-type */
	NF_DISPLAY_HEXA_B 		= 0x03, 	/* 6, B-type */
	NF_DISPLAY_OCTA_A 		= 0x04, 	/* 8, A-type */
	NF_DISPLAY_OCTA_B 		= 0x05, 	/* 8, B-type */
	NF_DISPLAY_NONA 		= 0x06, 	/* 9 */
	NF_DISPLAY_TRIDECA 		= 0x07, 	/* 13 */
	NF_DISPLAY_HEXADECA 	= 0x08,		/* 16 */
	NF_DISPLAY_HEXATRICONTA = 0x09,		/* 36 */
	NF_DISPLAY_SEQ 			= 0x20, 	/* TBD*/
	NF_DISPLAY_SEQ_DIV 		= 0x21, 		/* TBD*/
    NF_DISPLAY_LIVE_ZOOM    =   0x30,
    NF_DISPLAY_PLAYBACK_ZOOM =   0x31,
	NF_DISPLAY_LIVE_VCA		= 0x32,
	NF_DISPLAY_PLAYBACK_SMART	= 0x33,
	NF_DISPLAY_TETRAICOSA     = 0x35,   /* 24 */
	NF_DISPLAY_DOTRIACONTA   = 0x36,  /* 32 */
    NF_DISPLAY_NONE          =   0x50
} NF_DISPLAY_E ;

typedef enum _NF_JPEG_SIZE_E {
    NF_MAIN_SIZE,
    NF_SECOND_SIZE
} NF_JPEG_SIZE_E;

#define MAX_DISP_LAYER 3

typedef struct
{
	gint DispID[MAX_DISP_LAYER]; // 0 : main, 1 : sub, 2 : spot0, 3 : spot1
}NF_MULTI_DISP_DEV_T;

#endif
