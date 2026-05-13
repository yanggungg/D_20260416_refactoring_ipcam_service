#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "jansson.h"
#include "nf_ipcam_defs.h"
#include "nf_api_ipcam.h"

char *readFile(char *p_fileName, int *p_readSize);

json_t * parser_load_json(const char *text); 
void parser_free_json(json_t* p_text);
char* parser_json_get_string(json_t * p_value);
int parser_json_get_integer(json_t * p_value);
void parser_json_string_free(char *p_value);

json_t* get_node_find_str(json_t *p_node, char *p_str);
const char *json_plural(int count) ;
json_t* parser_category_string(json_t *p_node, char* p_str);

#define NF_IPCAM_DEPENDENCY_TABLE_MAX 5 

enum __NF_CAPABILITY_PARSER_E__
{
	CAPA_P_ERROR = -1,
	CAPA_P_OK = 0,

};

typedef enum __IPCAM_IAMGE_CAPA_TYPE_ENUM__
{
	PROFILE = 0,
	COLORVU,
	BRIGHTNESS,
	COLOR,
	CONTRAST,
	SHARPNESS,
	AWB_MODE,
	MWB_MODE,
	BLC_CTRL,
	WDR_CTRL,
	AE_MODE,
	ME_AGC,
	ME_SHUTTER,
	DNN_MODE,
	MAX_AGC,
	IMAGE_TYPE_MAX,

}img_capa_type_e;

typedef struct __IMAGE_CAPA_PARSE_TYPE__
{
	img_capa_type_e eType;
	char* key_str;

} image_capa_type_t;

static image_capa_type_t capabilities[IMAGE_TYPE_MAX] =
{
	{	PROFILE, 		"profile"			},
	{	COLORVU,		"ColorVu"			},
	{	BRIGHTNESS, 	"brightness"		},
	{	COLOR,			"colorsaturation"	},
	{	CONTRAST,		"contrast"			},
	{	SHARPNESS,		"sharpness"			},
	{	AWB_MODE,		"awb_mode"			},
	{	MWB_MODE,		"mwb_mode"			},
	{	BLC_CTRL,		"blc_ctrl"			},
	{	WDR_CTRL,		"wdr_ctrl"			},
	{	AE_MODE,		"ae_mode"			},
	{	ME_AGC,			"me_agc"			},
	{	ME_SHUTTER,		"me_shutter"		},
	{	DNN_MODE,		"dnn_mode"			},
	{	MAX_AGC,		"max_agc"			},
};

typedef struct __NF_IPCAM_IMAGE_CAPABILITIEIS__
{
	int profile_num;
	uint64_t supported;

	values_t brightness;
	values_t color;
	values_t contrast;
	values_t sharpness;

	modes wb; 
    modes mwb;
	modes blc;
	modes wdr;
    modes exposure;

	values_t agc;
	values_t eshutter_speed;

	values_t dnn_sense_ntod;
	values_t dnn_sense_dton;

	modes max_agc;

} image_capa_t;

///////////////////////////////////////////////////////////// image profile
typedef enum __IPCAM_VIDEO_CAPA_TYPE_ENUM__
{
	CODEC_0 = 0,
	CODEC_1,
	RESOLUTION_0,
	RESOLUTION_1,
	FPS_0,
	FPS_1,
	GOP_SIZE_0,
	GOP_SIZE_1,
	BIT_AVERAGE_0,
	BIT_AVERAGE_1,
	BIT_CTRL_0,
	BIT_CTRL_1,
	JPEG_QUALITY, 
	CAPTURE_MODE, 
	CORRIDOR_MODE,

	VIDEO_TYPE_MAX = 15,

}video_capa_type_e;

typedef struct __VIDEO_CAPA_PARSE_TYPE__
{
	video_capa_type_e eType;
	char* key_str;

} video_capa_type_t;

static video_capa_type_t video_capabilities[VIDEO_TYPE_MAX] =
{
	{	CODEC_0, 			"codec0"			},
	{	CODEC_1, 			"codec1"			},
	{	RESOLUTION_0,		"resolution0"		},
	{	RESOLUTION_1,		"resolution1"		},
	{	FPS_0,				"fps0"				},
	{	FPS_1,				"fps1"				},
	{	GOP_SIZE_0,			"gop_size0"			},
	{	GOP_SIZE_1,			"gop_size1"			},
	{	BIT_AVERAGE_0,		"bitavr0"			},
	{	BIT_AVERAGE_1,		"bitavr1"			},
	{	BIT_CTRL_0,			"bitctrl0"			},
	{	BIT_CTRL_1,			"bitctrl1"			},
	{	JPEG_QUALITY,		"jpegqual"			},
	{	CAPTURE_MODE,		"capture_mode"		},
	{	CORRIDOR_MODE,		"corridor_view"		},
};

typedef struct __PARSER_RESOLUTION_TABLE__
{
	uint64_t ipcam_res;
	char* key;
} resol_table_t;

//#define MAX_RESOL_LENGTH		41	
#define MAIN_RESOL_LENGTH   23	
#define SECOND_RESOL_LENGTH	23 //ksi_test 21 -> 23
/*
 ./nf_host() [0x6976f8]:_ti368_set_vcodec at /home/ksi/filesys_p5/NFDVR/src/service/nf_ipcam_driver_itx.c:8033
 ./nf_host(cam_set_vcodec+0x60) [0x685ef4]: [nf_ipcam_set_motion_smart:5111] db value is not changed
*/

static resol_table_t res_table[] = 
{
	{	NF_IPCAM_RES_3000x3000, "3000x3000" },	//0
	{	NF_IPCAM_RES_2048x2048, "2048x2048" },
	{	NF_IPCAM_RES_1280x1280, "1280x1280" },
	{   NF_IPCAM_RES_3840x2160, "3840x2160" },
	{   NF_IPCAM_RES_3200x2400, "3200x2400" },
	{   NF_IPCAM_RES_3072x2048, "3072x2048" },
	{   NF_IPCAM_RES_2880x2160, "2880x2160" },
	{   NF_IPCAM_RES_3200x1800, "3200x1800" },
	{   NF_IPCAM_RES_2880x1800, "2880x1880" },
	{   NF_IPCAM_RES_2592x1944, "2592x1944" },
	{   NF_IPCAM_RES_2992x1680, "2992x1680" },	//10
	{   NF_IPCAM_RES_2592x1920, "2592x1920" },
	{   NF_IPCAM_RES_2560x1920, "2560x1920" },
	{   NF_IPCAM_RES_2560x1600, "2560x1600" },
	{   NF_IPCAM_RES_2688x1520, "2688x1520" },
	{   NF_IPCAM_RES_2592x1520, "2592x1520" },
	{   NF_IPCAM_RES_2560x1440, "2560x1440" },
	{   NF_IPCAM_RES_2048x1536, "2048x1536" },
	{   NF_IPCAM_RES_2304x1296, "2304x1296" },
	{   NF_IPCAM_RES_1920x1080, "1920x1080" },
	{   NF_IPCAM_RES_1600x1200, "1600x1200" },	//20  
	{   NF_IPCAM_RES_1280x1024, "1280x1024" },
	{   NF_IPCAM_RES_1440x900,  "1440x900"  },
	{   NF_IPCAM_RES_1280x720,  "1280x720"  },
	{   NF_IPCAM_RES_1024x768,  "1024x768"  },
	{   NF_IPCAM_RES_800x600,   "800x600"   },
	{   NF_IPCAM_RES_800x450,   "800x450"   },
	{   NF_IPCAM_RES_720x576,   "720x576"   },
	{   NF_IPCAM_RES_720x480,   "720x480"   },
	{   NF_IPCAM_RES_704x576,   "704x576"   },
	{   NF_IPCAM_RES_704x480,   "704x480"   },	//30
	{   NF_IPCAM_RES_640x480,   "640x480"   },
	{   NF_IPCAM_RES_640x400,   "640x400"   },
	{   NF_IPCAM_RES_640x360,   "640x360"   },
	{   NF_IPCAM_RES_640x352,   "640x352"   },
	{   NF_IPCAM_RES_352x288,   "352x288"   },
	{   NF_IPCAM_RES_352x240,   "352x240"   },
	{   NF_IPCAM_RES_320x240,   "320x240"   }, 
	{   NF_IPCAM_RES_320x180,   "320x180"   },
	{   NF_IPCAM_RES_640x640,   "640x640"   },
	{   NF_IPCAM_RES_320x320,   "320x320"   },	//40
};
#define MAX_RESOL_LENGTH		sizeof(res_table)/sizeof(resol_table_t)

///////////////////////////////////////////////////////////// video profile

extern int setting_parsed_image_profile(json_t *root, image_capa_t* p_capa);
extern int setting_parsed_video_profile(json_t *root, video_t* p_video, encoder_t *p_encoder);
extern int setting_parsed_motion_profile(json_t *root, motion_t* p_motion);
extern int setting_parsed_privacy_profile(json_t *root, pmask_t* p_pmask);
extern int setting_parsed_audio_profile(json_t *root, audio_t* p_audio);
extern int setting_parsed_alarm_profile(json_t *root, alarm_t* p_alarm);

