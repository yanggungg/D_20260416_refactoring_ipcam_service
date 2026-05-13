/*

	2008-08-28 ???? 8:37:17 choissi
	NF_FRAME_TYPE, NF_CODEC_TYPE  modify
	
	2008-07-07 ???? 2:26:08	choissi
	
	typedef enum _NF_QUALITY_E  		????
	{
	    NF_QUALITY_HIGHEST	= 3,		// A
	    NF_QUALITY_HIGH		= 2,		// B
	    NF_QUALITY_STANDARD = 1,		// C
	    NF_QUALITY_LOW		= 0			// D
	} NF_QUALITY_E;
	
	2008-04-23 ???? 2:20:50 choissi
	
	NF_RECORD_REASON_TYPE_E -> NF_RECORD_REASON_E ��?? ????		
*/
#ifndef __NF_CODEC_HEADER_H__
#define __NF_CODEC_HEADER_H__

#include <glib.h>

typedef struct _ICODEC_HEADER_T {
	guchar		chan;
	guchar 		codec;
	guchar 		flags;
	guchar		version;
	guint	 	frame_size;			/* Frame size exclude header size */
	guchar 		frame_type;
	guchar 		timestampl;			/* 5 msec sub tick for uTimeStamp */
	guchar		resolution;
	guchar 		frame_rate;
	guint 		timestamp;			/* Second tick since 1970 GMT */
    gpointer    gst_buffer;
    guint       reserved;

//	guint64		timestamp2;			/* Nanosecond tick since 1970 GMT */
}  __attribute((packed))ICODEC_HEADER; 

#define NF_CODEC_HEADER		ICODEC_HEADER;

typedef enum _NF_QUALITY_E
{
    NF_QUALITY_SUPER		= 4,		// A
    NF_QUALITY_HIGHEST		= 3,		// B
    NF_QUALITY_HIGH			= 2,		// C
    NF_QUALITY_STANDARD 	= 1,		// D
    NF_QUALITY_LOW			= 0			// E
} NF_QUALITY_E;

typedef enum _NF_CODEC_VERSION_E  
{
	NF_CODEC_VERSION_1 = 1	
}NF_CODEC_VERSION_E;

typedef enum _NF_FPS_E  
{
    NF_FPS_CR32 			= 32,		// A
    NF_FPS_CR31 			= 31,         
    NF_FPS_CR30 			= 30,         
    NF_FPS_CR29 			= 29,         
    NF_FPS_CR28 			= 28,         
    NF_FPS_CR27 			= 27,         
    NF_FPS_CR26 			= 26,         
    NF_FPS_CR25 			= 25,         
    NF_FPS_CR24 			= 24,     
    NF_FPS_CR23 			= 23,     
    NF_FPS_CR22 			= 22,     
    NF_FPS_CR21 			= 21,     
    NF_FPS_CR20 			= 20,     
    NF_FPS_CR19 			= 19,     
    NF_FPS_CR18 			= 18,     
    NF_FPS_CR17 			= 17,     
    NF_FPS_CR16 			= 16,       // B
    NF_FPS_CR15 			= 15,        
    NF_FPS_CR14 			= 14,        
    NF_FPS_CR13 			= 13,        
    NF_FPS_CR12 			= 12,    
    NF_FPS_CR11 			= 11,        
    NF_FPS_CR10 			= 10,    
    NF_FPS_CR09 			= 9,        
    NF_FPS_CR08 			= 8,        // C
    NF_FPS_CR07 			= 7,        
    NF_FPS_CR06 			= 6,        
    NF_FPS_CR05 			= 5,    
    NF_FPS_CR04 			= 4,        // D
    NF_FPS_CR03 			= 3,    
    NF_FPS_CR02 			= 2,        // E
    NF_FPS_CR01 			= 1,        // F
    NF_FPS_CR00 			= 0         // G
}NF_FPS_E;

/* Resolutions */
typedef enum {
	NF_RES_NTSC_NONE		= 0x00, 	// A	
	NF_RES_NTSC_CIF			= 0x01, 	// B
	NF_RES_NTSC_2CIF		= 0x02, 	// C
	NF_RES_NTSC_4CIF		= 0x04, 	// D	
	NF_RES_NTSC_4CIFP		= 0x84, 	// E	
	NF_RES_PAL_CIF			= 0x11, 	// F
	NF_RES_PAL_2CIF			= 0x12, 	// G
	NF_RES_PAL_4CIF			= 0x14, 	// H
	NF_RES_PAL_4CIFP		= 0x94, 	// I	
	NF_RES_640x480			= 0x85, 	// J
	NF_RES_720x480			= 0x86, 	// K
	NF_RES_720x576			= 0x87, 	// L
	NF_RES_800x600			= 0x88, 	// M
	NF_RES_1024x768			= 0x89, 	// N
	NF_RES_1280x1024		= 0x8A, 	// O
	NF_RES_1600x1200		= 0x8B,  	// P	
	NF_RES_1280x720			= 0x8C,		// Q
	NF_RES_1920x1080		= 0x8D,		// R
	NF_RES_640x352			= 0x8E,		// S
	
	NF_RES_640x360          = 0x8F,  	// T
	NF_RES_640x360I         = 0x90,  	// U
	NF_RES_1280x720I 		= 0x92,  	// V
	NF_RES_1920x1080I 		= 0x93,  	// W

	NF_RES_640x400 		    = 0xA0,  	// X
	NF_RES_800x450 	     	= 0xA1,  	// Y
	NF_RES_1440x900 		= 0xA2,  	// Z
	
	// a,b?? ux????  960h ?????? ????? 
	
	NF_RES_320x180			= 0xA3,  	// c

/*	RESERVED Resolutions

	7bit = progressive
	6bit = 960h
	5bit = none
	4bit = is_pal
*/
	// 2015-07-08 ???? 1:26:12 choissi
	// http://222.112.8.34:8080/browse/SWIPXVETHR-441
	
	NF_RES_2304x1296		= 0xA4,  	// d (3M) 16:9
	NF_RES_2048x1536 		= 0xA5,  	// e (3M) 4:3	QXGA 
	NF_RES_2560x1440		= 0xA6,  	// f (3.6M) 16:9 WQHD

	NF_RES_2688x1520 		= 0xA7,  	// g (4M) 16:9
	NF_RES_2560x1600 		= 0xA8,  	// h (4.1M) 16:10 WQXGA
	
	NF_RES_2560x1920 		= 0xA9,  	// i (5M) 4:3 5M IPCAM ActiveX ???? ???
	NF_RES_2592x1920 		= 0xAA,  	// j (5M) 4:3 ???? ??? / ??? 5M ???? / ???? ???? 5M?? ?????? ?? ????? ??????? ??? ???
	NF_RES_2592x1944  		= 0xAB,  	// k (5M) 4:3
	NF_RES_2992x1680 		= 0xAC,  	// l (5M) 16:9 (bosh)

	// ????? CP8.0???? ???????? ????? ???? ????? ?????? ??? ??????
	NF_RES_2880x1800 		= 0xAD,  	// m (5.2M) 16:10 
	NF_RES_3200x1800		= 0xAE,  	// n (5.7M) 16:9 WQXGA+			

	NF_RES_2880x2160		= 0xAF,  	// o (6M) 4:3
	NF_RES_3072x2048 		= 0xE0,  	// p (6M) 3:2  (hikvision)

	NF_RES_3200x2400 		= 0xE1,  	// q (7.7M) 4:3 QUXGA	 
	NF_RES_3840x2160		= 0xE2,  	// r (8M) 16:9	
	NF_RES_2592x1520 		= 0xE3,  	// s (3.9M)	ITX hisilicon 4M
	NF_RES_1920x1440		= 0xE4,		// t (2.8M) Axis Fisheye camera
	NF_RES_1920x1536		= 0xE5,  	// u (2.9M) hikvision 2.9M

	NF_RES_1344x1520		= 0xE6,  	// v (1M)   // 4M Half	
	NF_RES_1296x1944		= 0xE7,  	// w (1.2M) // 5M Half
	NF_RES_1280x1440		= 0xE8,  	// x	    // 4M QHD Half
	NF_RES_1024x1536		= 0xE9,  	// y	    // 3M Half
	NF_RES_1280x960			= 0xEA,  	// z
	
//  for FISHEYE Camera
    NF_RES_3000x3000                = 0xF1,	// '1'
    NF_RES_2048x2048                = 0xF2,	// '2'
    NF_RES_1280x1280                = 0xF3,	// '3'
    NF_RES_640x640                  = 0xF4,	// '4'
    NF_RES_320x320                  = 0xF5,	// '5'
    
	NF_RES_2560x1944				= 0xF6,	// '6'
	NF_RES_2560x1520				= 0xF7,	// '7'

//  7G
       NF_RES_1280x1520                = 0xF8,	// '0'
       NF_RES_1280x1944                = 0xF9,	// '8'
       NF_RES_1280x2160                = 0xFa,	// '9'		
       NF_RES_1920x2160                = 0xFb,	// ':'
       NF_RES_2560x2160				   = 0xfc,  // '='       

	NF_RES_960H_NTSC_CIF	= 0x41,
	NF_RES_960H_NTSC_2CIF	= 0x42,
	NF_RES_960H_NTSC_4CIF	= 0x44,
	NF_RES_960H_NTSC_4CIFP	= 0xC4,
						 
	NF_RES_960H_PAL_CIF 	= 0x51,
	NF_RES_960H_PAL_2CIF	= 0x52,
	NF_RES_960H_PAL_4CIF	= 0x54,
	NF_RES_960H_PAL_4CIFP	= 0xD4,
			
	/* WARNING : whenever you add enum, tell to SST manager and NMF manager. */		
#if 0
	NF_RES_1CIF				= 0x01,
	NF_RES_2CIF				= 0x02, 	
	NF_RES_4CIF				= 0x04,
	NF_RES_4CIFP			= 0x84
#endif	

	NF_RES_360x640 = 0xB0,
	NF_RES_480x640 = 0xB1,
	NF_RES_480x704 = 0xB2,
	NF_RES_576x704 = 0xB3,
	NF_RES_720x1280 = 0xB4,
	NF_RES_768x1024 = 0xB5,
	NF_RES_1024x1280 = 0xB6,
	NF_RES_1080x1920 = 0xB7,
	NF_RES_1536x2048 = 0xB8,
	NF_RES_1296x2304 = 0xB9,
	NF_RES_1520x2592 = 0xBE,
	NF_RES_1944x2592 = 0xBF,
	NF_RES_2160x3840 = 0xC0,
	
} NF_RESOLUTION_E;
	
/* Frame types */
typedef enum {
	NF_FRAME_TYPE_P			= 0,	/* P frame */
	NF_FRAME_TYPE_I			= 1,	/* I frame */
	NF_FRAME_TYPE_NULL		= 2,	/* Null frame */
	NF_FRAME_TYPE_START		= 3,	/* Start frame */
	NF_FRAME_TYPE_END		= 4,	/* End frame (end of clip) */
	NF_FRAME_TYPE_RI		= 5,	/* Reverse I frame */	
	NF_FRAME_TYPE_AUDIO		= 10,	/* Audio frame */

//yesing 
	NF_FRAME_TYPE_AUDIO_MUTE = 11,	/**< Mute Frame for Multi archiving */
	
// choissi 2008-08-28 ???? 8:36:54
	NF_FRAME_TYPE_ENDDATA	= 8,	/* End of data frame(no more data) */
	NF_FRAME_TYPE_STARTDATA	= 9,	/* Start of data frame(first data of the entire) */	
	NF_FRAME_TYPE_OVERLAP   = 14,    /* overlapped data */
//onvif_porting
	NF_FRAME_TYPE_METADATA	= 20, 	/* Metadata frame */
	NF_FRAME_TYPE_MAX		= 21,
//onvif_porting

	/* WARNING : whenever you add enum, tell to SST manager and NMF manager, and increase MAX value. */	
	
} NF_FRAME_TYPE_E;

/* Codec types */
typedef enum {
	NF_CODEC_TYPE_ICODEC	= 0,	/* ICODEC (traditional) */
	NF_CODEC_TYPE_H264		= 1,	/* H.264 Base Profile, P-ref OTM */		
	NF_CODEC_TYPE_H264K		= 2,	/* H.264 Base Profile, I-ref(key frame mode) , NF,ANF,ATM */
	NF_CODEC_TYPE_JPEG		= 3,	/* Jpeg */
	NF_CODEC_TYPE_MPEG4K	= 4,	/* Mpeg4 I-ref HTM, UTM*/			
	NF_CODEC_TYPE_MPEG4		= 5,	/* Mpeg4 P-ref IPCAM */

	// 2009-11-07 ???? 1:37:19 choissi
	NF_CODEC_TYPE_H264MP	= 6,	/* H.264 Main Profile, P-ref */
	NF_CODEC_TYPE_H264MPK	= 7,	/* H.264 Main Profile, I-ref */
//onvif_porting
	NF_CODEC_TYPE_H264HP	= 8,	/* H.264 High Profile, P-ref */
	NF_CODEC_TYPE_H264HK	= 9,	/* H.264 High Profile, I-ref */
//onvif_porting

	// 2016-03-11 ���� 11:42:07 choissi
 	NF_CODEC_TYPE_H265K   	= 9,	/* H.265 I-ref */
	NF_CODEC_TYPE_H265   	= 10,	/* H.265 P-ref */
	NF_CODEC_TYPE_H265R		= 11,   /* H.265 P-ref for cam reserved ( fixme!! )*/
		
// choissi 2008-08-28 ???? 8:36:54
	NF_CODEC_TYPE_URAW		= 0,	/* If frametype == AUDIO, uraw */
	NF_CODEC_TYPE_ARAW		= 1,	/* If frametype == AUDIO, araw */
	NF_CODEC_TYPE_G723		= 2,	/* If frametype == AUDIO, g723 */
//onvif_porting
	NF_CODEC_TYPE_METADATA	= 10,	/* METADATA */	
	NF_CODEC_TYPE_NONE		= 10,	/* don't encoding or transfer */
//onvif_porting

	/* WARNING : whenever you add enum, tell to SST manager and NMF manager. */	
	
} NF_CODEC_TYPE_E;

/* Record reasons */
typedef enum {
	NF_RECORD_REASON_NOTHING	= 0,	/* None */
	NF_RECORD_REASON_TIMER		= 1,	/* By timer (schedule) */
	NF_RECORD_REASON_ALARM		= 2,	/* By alarm */
	NF_RECORD_REASON_MOTION		= 3,	/* By motion */
	NF_RECORD_REASON_USER		= 4,	/* By user event */
	NF_RECORD_REASON_MANUAL		= 5,	/* By manual (panic) */
	NF_RECORD_REASON_PRE		= 6,	/* Pre recorded */
	NF_REC_RT_REASON_MAX	= 7
	
	/* WARNING : whenever you add enum, tell to SST manager and NMF manager, and increase MAX value. */	
	
} NF_RECORD_REASON_E; 

typedef enum {
	NF_RECORD_REASON_CHAR_NOTHING	= ' ',	/* None */
	NF_RECORD_REASON_CHAR_TIMER		= 'T',	/* By timer (schedule) */
	NF_RECORD_REASON_CHAR_ALARM		= 'A',	/* By alarm */
	NF_RECORD_REASON_CHAR_MOTION	= 'M',	/* By motion */
	NF_RECORD_REASON_CHAR_USER		= 'U',	/* By user event */
	NF_RECORD_REASON_CHAR_MANUAL	= 'P',	/* By manual (panic) */
	NF_RECORD_REASON_CHAR_PRE		= 'p'	/* Pre recorded */
} NF_RECORD_REASON_CHAR_E; 

#define NF_CODEC_FLAG_SORB			0x02

//onvif_porting
typedef enum {
	//NF_VIDEO_CODEC_H264_MAIN = 0,
	//NF_VIDEO_CODEC_H264_BASELINE = 1,
	//NF_VIDEO_CODEC_H264_HIGH = 2,	
	//NF_VIDEO_CODEC_JPEG = 3,
	NF_VIDEO_CODEC_H264 = 1,
	NF_VIDEO_CODEC_JPEG = 2,
	NF_VIDEO_CODEC_H265 = 3,
} NF_VIDEO_CODEC_E; 
//onvif_porting
#endif
