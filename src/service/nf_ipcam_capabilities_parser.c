#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <pthread.h>
#include <glib.h>
#include "nf_ipcam_utils.h"
#include "nf_ipcam_capabilities_parser.h"
#include "nf_ipcam_defs.h"
#include "nf_ipcam_driver_itx.h"

#define NF_API_JSON_PARSER_DBG (0)

#ifndef TRUE
    #define TRUE            (1)
#endif
#ifndef FALSE
    #define FALSE           (0)
#endif

typedef struct
{
	size_t  obj_alloc_cnt;
	size_t  obj_free_cnt;
	size_t  str_alloc_cnt;
	size_t  str_free_cnt;
} NF_JSON_DBG;

static pthread_mutex_t  _dbg_mtx    = PTHREAD_MUTEX_INITIALIZER;
static NF_JSON_DBG      _json_dbg   = { 0, 0, 0, 0 };

char* parser_json_get_string(json_t * p_value);

static const char *str_null_to_blank(const char *str)
{
	static const char blank[2] = {0, };
	if(str) return str;
	return blank;
}

static void _json_dbg_count(int obj_alloc, int obj_free, int str_alloc, int str_free, int print_flag)
{
	pthread_mutex_lock(&_dbg_mtx);

	_json_dbg.obj_alloc_cnt += obj_alloc;
	_json_dbg.obj_free_cnt  += obj_free;
	_json_dbg.str_alloc_cnt += str_alloc;
	_json_dbg.str_free_cnt  += str_free;

	if(print_flag)
	{
		printf("[DEBUG] obj_alloc_cnt:%d, obj_free_cnt:%d, str_alloc_cnt:%d, str_free_cnt:%d\n",
				_json_dbg.obj_alloc_cnt, _json_dbg.obj_free_cnt,
				_json_dbg.str_alloc_cnt, _json_dbg.str_free_cnt);
	}

	pthread_mutex_unlock(&_dbg_mtx);
}

char *readFile(char *p_fileName, int *p_readSize)
{
	int size = 0;
	FILE *fp = fopen(p_fileName, "rb");
	char* buffer = NULL;

	if (fp == NULL)
		return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = malloc(size + 1);
	memset(buffer, 0, size + 1);

	if (fread(buffer, size, 1, fp) < 1)
	{
		*p_readSize = 0;
		free(buffer);
		fclose(fp);
		return NULL;
	}
	*p_readSize = size;
	fclose(fp); 

	return buffer;
}

json_t * parser_load_json(const char *text) {
    json_t *root;
    json_error_t error;

    root = json_loads(text, 0, &error);

    if (root) {
		_json_dbg_count(1, 0, 0, 0, FALSE);
        return root;
    } else {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return (json_t *)0;
    }
}

void parser_free_json(json_t* p_text)
{
	if(p_text)
	{
		json_decref(p_text);
		_json_dbg_count(0, 1, 0, 0, FALSE);
	}
}

json_t* get_node_find_str(json_t *p_node, char *p_str)
{
	json_t *child_node = NULL;

	const char *key;
	json_t *value;

    json_object_foreach(p_node, key, value) {

		if(strstr(key, p_str))
		{
			child_node = value;
			break;
		}
	}

	return child_node;
}

const char *json_plural(int count) {
    return count == 1 ? "" : "s";
}

char* parser_json_get_string(json_t * p_value)
{
	char *val = NULL;

	if(p_value != NULL)
	{
		val = (char*)json_string_value(p_value);
	}

	return val;
}

int parser_json_get_integer(json_t * p_value)
{
	int val = 0;
	const char *string_value = NULL;
	if(p_value == NULL) return val;

	switch(json_typeof(p_value)){
		case JSON_STRING:
			string_value = json_string_value(p_value);
			if(string_value) val = atoi(string_value);
			break;
		case JSON_INTEGER:
			val = (int)json_integer_value(p_value);
			break;
	}

	return val;
}

json_t* parser_category_string(json_t *p_node, char* p_str)
{
	json_t* child_node = NULL;
	json_t* tmp_node = NULL;
	const char* key = NULL;
	json_t* value = NULL;

	int stop_flag = 0;

    if(p_node == NULL || p_str == NULL)
        return NULL;

	tmp_node = get_node_find_str(p_node, "message");
	if(tmp_node == NULL)
		return NULL;

	key = json_string_value(tmp_node);
	if(!strstr(key, "Success"))
		return NULL;

    child_node = json_object_get(json_object_get(json_object_get(p_node, "data"), "api"), p_str);

    /*
	json_object_foreach(p_node, key, value)
	{
		if(strstr(key, "data"))
		{
			json_object_foreach(value, key, tmp_node)
			{
				if(strstr(key, "api"))
				{
					json_object_foreach(tmp_node, key, value)
					{
						if(strstr(key, p_str))
						{
							stop_flag = 1;
							//printf("\e[31m >> FIND [%s] PROFILES \e[0m\n", p_str);
							child_node = value;
							_json_dbg_count(1, 0, 0, 0, FALSE);
							break;
						}
					}
				}
				if(stop_flag)
					break;
			}
		}
		if(stop_flag)
			break;
	}
    */

	//json_decref(value);
	//json_decref(tmp_node);

	return child_node;
}

//////////////////////////////////////////////	IMAGE PROFILE	//////////////////////////////////////////////
char *NF_IPCAM_IMAGE_DEPENDENCY_TABLE[] = {
	"DW-DI-V001",	//NCX3
	"DW-NA-V001",	//NSB-HE
	"TW-PI-V001",	//INBH-2007PRT
	"TW-DI-V001",	//INX-5007
	"TW-NA-V001",	//NMDi-3007PRT
};	//version V001 table

typedef values_t* (*simple_type_func)(json_t*);
typedef modes* (*option_type_func)(json_t*);

int check_profile(json_t *p_node);

void brightness_setting(image_capa_t *p_capa, json_t *element);
void color_setting(image_capa_t *p_capa, json_t *element);
void contrast_setting(image_capa_t *p_capa, json_t *element);
void sharpness_setting(image_capa_t *p_capa, json_t *element);

void awb_mode_setting(image_capa_t *p_capa, json_t *element);
void ae_mode_setting(image_capa_t *p_capa, json_t *element);

typedef void (*img_setup_func_ptr)(image_capa_t *, json_t *element);
static void image_param_setting_func (image_capa_t* p_capa, json_t *element,  img_setup_func_ptr func);
void setting_image_profile(image_capa_t * p_capa, img_capa_type_e p_type, json_t *element);

int setting_parsed_image_profile(json_t *root, image_capa_t* p_capa)
{
	json_t *tmp_node = NULL;
	int rtn = 0;
	int i = 0;
	int image_capa_parse_flag = 0;

	if(root == NULL)
		return CAPA_P_ERROR;

	for(i = 0; i < IMAGE_TYPE_MAX; i++)
	{
		tmp_node = get_node_find_str(root, capabilities[i].key_str);
		if(tmp_node != NULL)
		{
			if( i == 0 )	//known | unknown profile check
			{
				rtn = check_profile(tmp_node);
				if(rtn < NF_IPCAM_DEPENDENCY_TABLE_MAX){
					//printf("\e[95m Found Known IMAGE PROFILE (num : %d) \e[0m\n", rtn);
					p_capa->profile_num = rtn;
					//break;
				}
				else{
					//printf("\e[95m Not Found IMAGE PROFILE (num : %d) \e[0m\n", rtn);
					p_capa->profile_num = rtn;
					//break;
				}
			}
			else
			{
				setting_image_profile(p_capa, capabilities[i].eType, tmp_node); 
				image_capa_parse_flag += 1;	
			}
		}
	}

	return image_capa_parse_flag;
}

int check_profile(json_t *p_node)
{
	const char *value;
	int i = 0;
	
	value = json_string_value(p_node);
	//printf("\e[95m Profile Type : (%s)\e[0m\n", value);
	
	for(i = 0; i < NF_IPCAM_DEPENDENCY_TABLE_MAX; i++)
	{
		if(strstr(value, NF_IPCAM_IMAGE_DEPENDENCY_TABLE[i]))
		{
			//printf("\e[31m FIND IMG PROFILE : (%d)(%s)  \e[0m\n", i, NF_IPCAM_IMAGE_DEPENDENCY_TABLE[i]);
			break;
		}
	}

	return i;
}

static void image_param_setting_func (image_capa_t* p_capa, json_t *element,  img_setup_func_ptr func)
{
	  func (p_capa, element);
}

void setting_image_profile(image_capa_t * p_capa, img_capa_type_e p_type, json_t *element)
{
	img_setup_func_ptr func = NULL;
	
	switch(p_type)
	{
		//supported |= needed uint64_t value
		case BRIGHTNESS:
			p_capa->supported |= NF_IPCAM_IMAGE_BRIGHTNESS;
			func = brightness_setting;
			break;
		case COLOR:
			p_capa->supported |= NF_IPCAM_IMAGE_COLOR;
			func = color_setting;
			break;
		case CONTRAST:
			p_capa->supported |= NF_IPCAM_IMAGE_CONTRAST;
			func = contrast_setting;
			break;
		case SHARPNESS:
			p_capa->supported |= NF_IPCAM_IMAGE_SHARPNESS;
			func = sharpness_setting;
			break;
		case AWB_MODE:
			p_capa->supported |= NF_IPCAM_IMAGE_WB;
			func = awb_mode_setting;
			break;
		case MWB_MODE:
			p_capa->supported |= NF_IPCAM_IMAGE_MWB;
			break;
		case BLC_CTRL:
			p_capa->supported |= NF_IPCAM_IMAGE_BLC;
			break;
		case WDR_CTRL:
			p_capa->supported |= NF_IPCAM_IMAGE_WDR; 
			break;
		case AE_MODE:
			p_capa->supported |= NF_IPCAM_IMAGE_EXPOSURE;
			func = ae_mode_setting;
			break;
		case ME_AGC:
			p_capa->supported |= NF_IPCAM_IMAGE_AGC; 
			break;
		case ME_SHUTTER:
			p_capa->supported |= NF_IPCAM_IMAGE_ESHUTTER;
			break;
		case DNN_MODE:
			p_capa->supported |= NF_IPCAM_IMAGE_DNN;
			break;
		case COLORVU:
			if(parser_json_get_integer(element)){
				p_capa->supported |= NF_IPCAM_IMAGE_COLORVU;
			}
			break;
	}

	if(func != NULL)
		image_param_setting_func(p_capa, element, func);
}

void brightness_setting(image_capa_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "min"))
			p_capa->brightness.min = json_integer_value(value); 
		else if(strstr(key, "max"))
			p_capa->brightness.max = json_integer_value(value); 
		else if (strstr(key, "value"))
			p_capa->brightness.value = json_integer_value(value); 
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[33m brightness.min(%d) max(%d) value(%d) \e[0m\n", 
			p_capa->brightness.min, 
			p_capa->brightness.max, 
			p_capa->brightness.value); 
#endif
}

void color_setting(image_capa_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "min"))
			p_capa->color.min = json_integer_value(value); 
		else if(strstr(key, "max"))
			p_capa->color.max = json_integer_value(value); 
		else if (strstr(key, "value"))
			p_capa->color.value = json_integer_value(value); 
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[33m color.min(%d) max(%d) value(%d) \e[0m\n", 
			p_capa->color.min, 
			p_capa->color.max, 
			p_capa->color.value); 
#endif
}

void contrast_setting(image_capa_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "min"))
			p_capa->contrast.min = json_integer_value(value); 
		else if(strstr(key, "max"))
			p_capa->contrast.max = json_integer_value(value); 
		else if (strstr(key, "value"))
			p_capa->contrast.value = json_integer_value(value); 
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[33m contrast.min(%d) max(%d) value(%d) \e[0m\n", 
			p_capa->contrast.min, 
			p_capa->contrast.max, 
			p_capa->contrast.value); 
#endif
}

void sharpness_setting(image_capa_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "min"))
			p_capa->sharpness.min = json_integer_value(value); 
		else if(strstr(key, "max"))
			p_capa->sharpness.max = json_integer_value(value); 
		else if (strstr(key, "value"))
			p_capa->sharpness.value = json_integer_value(value); 
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[33m sharpness.min(%d) max(%d) value(%d) \e[0m\n", 
			p_capa->sharpness.min, 
			p_capa->sharpness.max, 
			p_capa->sharpness.value); 
#endif
}

void awb_mode_setting(image_capa_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				//supported matching needed
				if(strstr(child_key, "Manual")){
					p_capa->wb.support |= NF_IPCAM_IMAGE_WB_MODE_MANUAL;
				}
				else if(strstr(child_key, "Auto")){
					p_capa->wb.support |= NF_IPCAM_IMAGE_WB_MODE_AUTO; 
				}
				else if(strstr(child_key, "Auto(wide)")){
					p_capa->wb.support |= NF_IPCAM_IMAGE_WB_MODE_AUTO_W; 
				}	
			}
		}
		else if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

			if(strstr(diff_str, "manual")){
				p_capa->wb.value = NF_IPCAM_IMAGE_WB_MODE_MANUAL;
			}
			else if(strstr(diff_str, "auto")){
				p_capa->wb.value = NF_IPCAM_IMAGE_WB_MODE_AUTO; 
			}
			else if(strstr(diff_str, "w_auto")){
				p_capa->wb.value = NF_IPCAM_IMAGE_WB_MODE_AUTO_W; 
			}	

		}
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[33m awe_mode.supported(%08x) value(%08x) \e[0m\n", 
			p_capa->wb.support, 
			p_capa->wb.value); 
#endif
}

void ae_mode_setting(image_capa_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				//supported matching needed
				if(strstr(child_key, "Manual")){
					p_capa->exposure.support |= NF_IPCAM_IMAGE_EXP_MODE_MANUAL_INX;
				}
				else if(strstr(child_key, "Auto")){
					p_capa->exposure.support |= NF_IPCAM_IMAGE_EXP_MODE_AUTO_INX; 
				}
				else if(strstr(child_key, "Auto(Motion Priority)")){
					p_capa->exposure.support |= NF_IPCAM_IMAGE_EXP_MODE_HI_AUTO_MOTION; 
				}	
			}
		}
		else if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

			if(strstr(diff_str, "manual")){
				p_capa->exposure.value = NF_IPCAM_IMAGE_EXP_MODE_MANUAL_INX;
			}
			else if(strstr(diff_str, "auto")){
				p_capa->exposure.value = NF_IPCAM_IMAGE_EXP_MODE_AUTO_INX; 
			}
			else if(strstr(diff_str, "auto_m")){
				p_capa->exposure.value = NF_IPCAM_IMAGE_EXP_MODE_HI_AUTO_MOTION; 
			}	
		}
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[33m exposure.supported(%08x) value(%08x) \e[0m\n", 
			p_capa->exposure.support, 
			p_capa->exposure.value); 
#endif
}
//////////////////////////////////////////////	IMAGE PROFILE END	//////////////////////////////////////////////
//////////////////////////////////////////////	VIDEO PROFILE START	//////////////////////////////////////////////

typedef void (*video_setup_func_ptr)(video_t *, encoder_t *, json_t *element);
static void video_param_setting_func (video_t* p_video, encoder_t* p_encoder, json_t *element,  video_setup_func_ptr func);
void setting_video_profile(video_t *p_video, encoder_t *p_encoder, video_capa_type_e p_type, json_t *element);
void find_matched_resolution(video_t* p_video, encoder_t* p_encoder, const char* p_resol_str, int p_stream);
void codec_0_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void codec_1_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void resolution_0_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void resolution_1_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void capture_mode_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void corridor_mode_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void dummy_profile_setting(video_t* p_video, encoder_t* p_encoder);
void bitctrl_0_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);
void bitctrl_1_setting(video_t* p_video, encoder_t* p_encoder, json_t *element);

static const video_t model_video_dummy = 
{
	/* supported */
	VIDEO_SETUP_FPS|VIDEO_SETUP_BITRATE|VIDEO_SETUP_ANTIFLICKER|VIDEO_SETUP_MIRROR | VIDEO_SETUP_CAPTURE_MODE | VIDEO_SETUP_BITRATE_CONTROL,
	/* on the fly changable */
	VIDEO_SETUP_FPS|VIDEO_SETUP_BITRATE|VIDEO_SETUP_MIRROR,
	/* stream_cnt */
	2,
	/* resolution */
	{ 0, 0, 0 },
	{
		0,
		{ 0, 0, 0 }
	},
	/* fps */
	{
		{	// ntsc
			{
				NF_IPCAM_FPS_300|NF_IPCAM_FPS_150|NF_IPCAM_FPS_70|NF_IPCAM_FPS_30|NF_IPCAM_FPS_20|NF_IPCAM_FPS_10,
				NF_IPCAM_FPS_300
			},
			{
				NF_IPCAM_FPS_300|NF_IPCAM_FPS_150|NF_IPCAM_FPS_70|NF_IPCAM_FPS_30|NF_IPCAM_FPS_20|NF_IPCAM_FPS_10,
				NF_IPCAM_FPS_300
			},
			{
				0, 0
			}
		},
		{	// pal
			{
				NF_IPCAM_FPS_250|NF_IPCAM_FPS_120|NF_IPCAM_FPS_60|NF_IPCAM_FPS_30|NF_IPCAM_FPS_20|NF_IPCAM_FPS_10,
				NF_IPCAM_FPS_250
			},
			{
				NF_IPCAM_FPS_250|NF_IPCAM_FPS_120|NF_IPCAM_FPS_60|NF_IPCAM_FPS_30|NF_IPCAM_FPS_20|NF_IPCAM_FPS_10,
				NF_IPCAM_FPS_250
			},
			{
				0, 0
			}
		}
	},
#if defined(_IPX_1648P)||defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_0824ECO)||defined(_IPX_0412ECO) 
	/* bitrate */
	{
		{ 2000, 4000, 4000 },
		{ 800, 1500, 1500 },
		{ 0, 0, 0 }
	},
	/* quality */
	{
#if defined(_IPX_MULTI_SWITCH)
		{ 1500, 2000, 2200, 3000, 4000 },
#else
		{ 2500, 3000, 3500, 3800, 4000 },
#endif
		{ 200, 500, 800, 1000, 1200 },
		{ 0, 0, 0, 0, 0 }
	},
#elif defined(_IPX_1648VE3) || defined(_IPX_1648P3) || defined(_IPX_1648P3ECO)
	/* bitrate */
	{
		{ 2000, 6000, 4000 },
		{ 800, 1500, 1500 },
		{ 0, 0, 0 }
	},
	/* quality */
	{
#if defined(_IPX_MULTI_SWITCH)
		{ 1500, 2000, 2200, 3000, 4000 },
#else
		{ 2500, 3000, 3500, 3800, 4000 },
#endif
		{ 200, 500, 800, 1000, 1200 },
		{ 0, 0, 0, 0, 0 }
	},
#else
	/* bitrate */
	{
		{ 2000, 8000, 8000 },
		{ 800, 1500, 1500 },
		{ 0, 0, 0 }
	},
	/* quality */
	{
		{ 3000, 4400, 5600, 6800, 8000 },
		{ 200, 500, 800, 1200, 1500 },
		{ 0, 0, 0, 0, 0 }
	},
#endif
	/* anti-flicket */
	{ NF_IPCAM_AF_NTSC | NF_IPCAM_AF_PAL, NF_IPCAM_AF_NTSC },
	/* mirror */
	{ NF_IPCAM_MIRROR_NONE | NF_IPCAM_MIRROR_FLIP | 
		NF_IPCAM_MIRROR_HORIZONTAL | NF_IPCAM_MIRROR_VERTICAL,
		NF_IPCAM_MIRROR_NONE },
	/* capture mode */
	{ 0, 0 },
	/* bitrate control */
	{ 0, 0, 0 }
};

static const encoder_t model_encoder_dummy = 
	{
		{
			0, 0, 0
		},
		{ 30, 30, 0 },
		{ 2000, 800, 0 },
#if defined(_IPX_1648P)||defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_ECO) 
		{ 4000, 1500, 0 },
#elif defined(_IPX_1648VE3) || defined(_IPX_1648P3) || defined(_IPX_1648P3ECO)
		{ 6000, 1500, 0 },
#else
		{ 8000, 1500, 0 },
#endif
		{ 30, 30, 0 },
		{
			0, 0, 0
		},
		{
			0, 0, 0
		}
	};

int setting_parsed_video_profile(json_t *root, video_t* p_video, encoder_t *p_encoder)
{
	json_t *tmp_node = NULL;
	int rtn = 0;
	int i = 0;

	if(root == NULL)
		return CAPA_P_ERROR;

	dummy_profile_setting(p_video, p_encoder);

	for(i = 0; i < VIDEO_TYPE_MAX; i++)
	{
		tmp_node = get_node_find_str(root, video_capabilities[i].key_str);
		if(tmp_node != NULL)
		{
			setting_video_profile(p_video, p_encoder, i, tmp_node);
		}
	}

	//json_decref(tmp_node);

	return CAPA_P_OK;
}

void setting_video_profile(video_t *p_video, encoder_t *p_encoder, video_capa_type_e p_type, json_t *element)
{
	video_setup_func_ptr func = NULL;
	
	switch(p_type)
	{
		case CODEC_0:
			func = codec_0_setting;
			break;
		case CODEC_1:
			func = codec_1_setting;
			break;
		case RESOLUTION_0:
			func = resolution_0_setting;
			break;
		case RESOLUTION_1:
			func = resolution_1_setting;
			break;
		case FPS_0:
			break;
		case FPS_1:
			break;
		case GOP_SIZE_0:
			break;
		case GOP_SIZE_1:
			break;
		case BIT_AVERAGE_0:
			break;
		case BIT_AVERAGE_1:
			break;
		case BIT_CTRL_0:
			func = bitctrl_0_setting;
			break;
		case BIT_CTRL_1:
			func = bitctrl_1_setting;
			break;
		case JPEG_QUALITY:
			break;
		case CAPTURE_MODE:
			func = capture_mode_setting;
			break;
		case CORRIDOR_MODE:
			func = corridor_mode_setting;
			break;
	}

	if(func != NULL)
		video_param_setting_func(p_video, p_encoder, element, func);
}

static void video_param_setting_func (video_t* p_video, encoder_t* p_encoder,  json_t *element,  video_setup_func_ptr func)
{
	  func (p_video, p_encoder, element);
}

void codec_0_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				if(strstr(child_key, "H265"))
				{
					p_video->vcodec[0] = NF_IPCAM_VCODEC_H265;
					p_encoder->vcodec[0] |= NF_IPCAM_VCODEC_H265;
				}
				else if(strstr(child_key, "H264"))
				{	
					if(!(p_video->vcodec[0] & NF_IPCAM_VCODEC_H265))
						p_video->vcodec[0] = NF_IPCAM_VCODEC_H264;

					p_encoder->vcodec[0] |= NF_IPCAM_VCODEC_H264;
				}
				else if(strstr(child_key, "MJPEG"))
				{
					//no action
				}	
			}
		}
		else if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

			if(strstr(diff_str, "H265")){
				//no action
			}
			else if(strstr(diff_str, "H264")){
				//no action
			}
			else if(strstr(diff_str, "MJPEG")){
				//no action
			}

		}
	}

#if NF_API_JSON_PARSER_DBG

	printf("\e[95m[%s][%d] video.vcodec[0] : (%d) encoder.vcodec[0] :(%d)\e[0m\n", __FUNCTION__, __LINE__,
			p_video->vcodec[0], p_encoder->vcodec[0]);

#endif
}

void codec_1_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				if(strstr(child_key, "H265")){
					p_video->vcodec[1] = NF_IPCAM_VCODEC_H265;
					p_encoder->vcodec[1] |= NF_IPCAM_VCODEC_H265;
				}
				else if(strstr(child_key, "H264")){	//S1 default scenario
					if(!(p_video->vcodec[1] & NF_IPCAM_VCODEC_H265))
						p_video->vcodec[1] = NF_IPCAM_VCODEC_H264;

					p_encoder->vcodec[1] |= NF_IPCAM_VCODEC_H264;
				}
				else if(strstr(child_key, "MJPEG")){
					//no action
				}	
			}
		}
		else if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

			if(strstr(diff_str, "H265")){
				//no action
			}
			else if(strstr(diff_str, "H264")){
				//no action
			}
			else if(strstr(diff_str, "MJPEG")){
				//no action
			}	

		}
	}

#if NF_API_JSON_PARSER_DBG
	printf("\e[95m[%s][%d] video.vcodec[1] : (%d) encoder.vcodec[1] :(%d)\e[0m\n", __FUNCTION__, __LINE__,
			p_video->vcodec[1], p_encoder->vcodec[1]);
#endif
}

void resolution_0_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str = NULL;
	int i = 0;
	int main_resol_start = 0;

	json_object_foreach(element, key, value){

		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				find_matched_resolution(p_video, p_encoder, child_key, 0);
			}
		}

		if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);
			if(diff_str)
			{
				for(i = main_resol_start; i < MAIN_RESOL_LENGTH; i++)
				{
					if(strstr(diff_str, res_table[i].key))
					{
						printf("\e[95m [%s][%d] I:%d res(%s) \e[0m\n", __FUNCTION__, __LINE__, i, diff_str);
						p_video->resolution.resolution[0] = res_table[i].ipcam_res;
						break;
					}
				}
// #if defined(_IPX_1648M4CE) || defined(_IPX_0824M4CE) || defined(_IPX_0412M4CE) || defined(_IPX_1648P4CE)
// 				if(p_video->resolution.resolution[0] == NF__IPCAM_RES_3000x3000)
// 				{
// 					p_video->resolution.resolution[0] = NF_IPCAM_RES_2048x2048;
// 				}
// #endif
			}
		}

	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	for(i = 0; i < MAIN_RESOL_LENGTH; i++)
	{
		if(p_video->resolution.resolution[0] & res_table[i].ipcam_res)
			printf("\e[31m[%s][%d] 1st stream :current res(%s) \e[0m\n", __FUNCTION__, __LINE__, res_table[i].key);
	}

#endif
}

void resolution_1_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str = NULL;
	int i = 0;

	json_object_foreach(element, key, value){

		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				find_matched_resolution(p_video, p_encoder, child_key, 1);
			}
		}

		if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);
			if(diff_str)
			{
				for(i = SECOND_RESOL_LENGTH; i < MAX_RESOL_LENGTH; i++)
				{
					if(strstr(diff_str, res_table[i].key))
					{
						printf("\e[99m [%s][%d] I:%d \e[0m\n", __FUNCTION__, __LINE__, i);
						p_video->resolution.resolution[1] = res_table[i].ipcam_res;	
						break;
					}
				}
			}
		}
	}

#if NF_API_JSON_PARSER_DBG
	//debug 

	for(i = 0; i < MAX_RESOL_LENGTH; i++)
	{
		if(p_video->resolution.supported & res_table[i].ipcam_res)
			printf("\e[95m[%s][%d] total : supported res(%s) \e[0m\n", __FUNCTION__, __LINE__, res_table[i].key);

		if(p_video->resolution.resolution[1] & res_table[i].ipcam_res)
			printf("\e[31m[%s][%d] 2nd stream :current res(%s) \e[0m\n", __FUNCTION__, __LINE__, res_table[i].key);
	}
#endif
}

void find_matched_resolution(video_t* p_video, encoder_t* p_encoder, const char* p_resol_str, int p_stream)
{
	int i = 0;

	if(p_stream == 0)
	{
		for(i = 0; i < MAIN_RESOL_LENGTH; i++)
		{
			if(strstr(p_resol_str, res_table[i].key) != NULL)
			{
				p_video->resolution.supported |= res_table[i].ipcam_res;	
				break;
			}
		}

		for(i = 0; i < MAIN_RESOL_LENGTH; i++)
		{
			if(strstr(p_resol_str, res_table[i].key) != NULL)
			{
				p_encoder->res_support[p_stream] |= res_table[i].ipcam_res;	
			}
		}
// #if defined(_IPX_1648M4CE) || defined(_IPX_0824M4CE) || defined(_IPX_0412M4CE) || defined(_IPX_1648P4CE)
// 		if(p_video->resolution.supported & NF_IPCAM_RES_3000x3000){
// 			p_video->resolution.supported &= ~(NF_IPCAM_RES_3000x3000);
// 		}
// 		if(p_encoder->res_support[p_stream] & NF_IPCAM_RES_3000x3000){
// 			p_encoder->res_support[p_stream] &= ~(NF_IPCAM_RES_3000x3000);
// 		}
// #endif
	}
	else
	{

		for(i = SECOND_RESOL_LENGTH; i < MAX_RESOL_LENGTH; i++)
		{
			if(strstr(p_resol_str, res_table[i].key) != NULL)
			{
				p_video->resolution.supported |= res_table[i].ipcam_res;	
				break;
			}
		}

		for(i = SECOND_RESOL_LENGTH; i < MAX_RESOL_LENGTH; i++)
		{
			if(strstr(p_resol_str, res_table[i].key) != NULL)
			{
				p_encoder->res_support[p_stream] |= res_table[i].ipcam_res;	
			}
		}
	}
#if 0 
	for(i = 0; i < MAX_RESOL_LENGTH; i++)
	{
		if(strstr(p_resol_str, res_table[i].key) != NULL)
		{
			p_video->resolution.supported |= res_table[i].ipcam_res;	
			break;
		}
	}

	for(i = 0; i < MAX_RESOL_LENGTH; i++)
	{
		if(strstr(p_resol_str, res_table[i].key) != NULL)
		{
			p_encoder->res_support[p_stream] |= res_table[i].ipcam_res;	
		}
	}
#endif
}

void bitctrl_0_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				if(strstr(child_key, "VBR+"))
				{
					p_encoder->bitctrl[0] |= NF_IPCAM_BITRATE_CONTROL_VBR_PLUS;
				}
                else if(strstr(child_key, "VBR"))
				{
					p_encoder->bitctrl[0] |= NF_IPCAM_BITRATE_CONTROL_CBR;
				}
				else if(strstr(child_key, "CBR"))
				{
					p_encoder->bitctrl[0] |= NF_IPCAM_BITRATE_CONTROL_VBR;
				}
			}
		}
		else if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

            if(strstr(diff_str, "IDNR"))
            {
				p_video->bitctrl[0] = NF_IPCAM_BITRATE_CONTROL_VBR_PLUS;
            }
			else if(strstr(diff_str, "CBR"))
			{
				p_video->bitctrl[0] = NF_IPCAM_BITRATE_CONTROL_CBR;
			}
			else if(strstr(diff_str, "VBR"))
			{
				p_video->bitctrl[0] = NF_IPCAM_BITRATE_CONTROL_VBR;
			}
		}
	}

#if NF_API_JSON_PARSER_DBG
	printf("\e[95m current bitctrl[1st stream] : %d \e[0m\n", p_video->bitctrl[0]);
	printf("\e[95m support bitctrl[1st stream] : %08x \e[0m\n", p_encoder->bitctrl[0]);
#endif
}

void bitctrl_1_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				if(strstr(child_key, "VBR+"))
				{
					p_encoder->bitctrl[1] |= NF_IPCAM_BITRATE_CONTROL_VBR_PLUS;
				}
                else if(strstr(child_key, "VBR"))
				{
					p_encoder->bitctrl[1] |= NF_IPCAM_BITRATE_CONTROL_CBR;
				}
				else if(strstr(child_key, "CBR"))
				{
					p_encoder->bitctrl[1] |= NF_IPCAM_BITRATE_CONTROL_VBR;
				}
			}
		}
		else if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

            if(strstr(diff_str, "IDNR"))
            {
				p_video->bitctrl[1] = NF_IPCAM_BITRATE_CONTROL_VBR_PLUS;
            }
            else if(strstr(diff_str, "CBR"))
			{
				p_video->bitctrl[1] = NF_IPCAM_BITRATE_CONTROL_CBR;
			}
			else if(strstr(diff_str, "VBR"))
			{
				p_video->bitctrl[1] = NF_IPCAM_BITRATE_CONTROL_VBR;
			}
		}
	}

#if NF_API_JSON_PARSER_DBG
	printf("\e[95m current bitctrl[1st stream] : %d \e[0m\n", p_video->bitctrl[1]);
	printf("\e[95m support bitctrl[1st stream] : %d \e[0m\n", p_encoder->bitctrl[1]);
#endif
}

void capture_mode_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	json_t *child_value;

	char* diff_str = NULL;
	int value_cnt = 0;

	json_object_foreach(element, key, value){

		if(strstr(key, "options"))
		{
			json_object_foreach(value, child_key, child_value)
			{
				if(strstr(child_key, "2592x1944_30Fps") != NULL)
					p_video->capture.support |= NF_IPCAM_CAPTURE_5M_30FPS;
				if(strstr(child_key, "1920x1080_60Fps") != NULL)
					p_video->capture.support |= NF_IPCAM_CAPTURE_60FPS;
				if(strstr(child_key, "2048x1536_30Fps") != NULL)
					p_video->capture.support |= NF_IPCAM_CAPTURE_3M_30FPS_1;
				if(strstr(child_key, "2304x1296_30Fps") != NULL)
					p_video->capture.support |= NF_IPCAM_CAPTURE_3M_30FPS_2;
				if(strstr(child_key, "1920x1080_30Fps") != NULL)
					p_video->capture.support |= NF_IPCAM_CAPTURE_2M_30FPS;
			}

		}
		
		if(strstr(key, "value"))
		{
			diff_str = 	parser_json_get_string(value);

			if(strstr(diff_str, "0") != NULL)
				p_video->capture.value = NF_IPCAM_CAPTURE_5M_30FPS;
			if(strstr(diff_str, "1") != NULL)
				p_video->capture.value = NF_IPCAM_CAPTURE_60FPS;
			if(strstr(diff_str, "2") != NULL)
				p_video->capture.value = NF_IPCAM_CAPTURE_3M_30FPS_1;
			if(strstr(diff_str, "3") != NULL)
				p_video->capture.value = NF_IPCAM_CAPTURE_3M_30FPS_2;
			if(strstr(diff_str, "4") != NULL)
				p_video->capture.value = NF_IPCAM_CAPTURE_2M_30FPS;

		}
	}

#if NF_API_JSON_PARSER_DBG
	//debug 
	printf("\e[95m[%s][%d] capture_mode.support(%08x) current.capruemode(%08x) \e[0m\n", __FUNCTION__, __LINE__,
			p_video->capture.support, p_video->capture.value);
	
#endif
}

void corridor_mode_setting(video_t* p_video, encoder_t* p_encoder, json_t *element)
{
	const char *key;
	json_t *value;
	const char *child_key;
	const int int_value;
	json_t *child_value;

	char* diff_str = NULL;
	int diff_int = 0;

	json_object_foreach(element, key, value){
		if(strstr(key, "min"))
		{
			diff_int = parser_json_get_integer(value);
			p_video->corridor_mode_min = diff_int;
			/*
			if(strstr(diff_str, "0") != NULL)
			{
				p_video->corridor_mode_min = 0;
			}
			if(strstr(diff_str, "1") != NULL)
			{
				p_video->corridor_mode_min = 1;
			}
			if(strstr(diff_str, "2") != NULL)
			{
				p_video->corridor_mode_min = 2;
			}
			*/
		}
		if(strstr(key, "max"))
		{
			diff_int = parser_json_get_integer(value);
			p_video->corridor_mode_max = diff_int;
			/*
			if(strstr(diff_str, "0") != NULL)
			{
				p_video->corridor_mode_max = 0;
			}
			if(strstr(diff_str, "1") != NULL)
			{
				p_video->corridor_mode_max = 1;
			}
			if(strstr(diff_str, "2") != NULL)
			{
				p_video->corridor_mode_max = 2;
			}
			*/

		}
		if(strstr(key, "value"))
		{
			diff_int = parser_json_get_integer(value);
			p_video->corridor_mode_value = diff_int;
/*
			if(strstr(diff_str, "0") != NULL)
			{
				p_video->corridor_mode_value = 0;
			}
			if(strstr(diff_str, "1") != NULL)
			{
				p_video->corridor_mode_value = 1;
			}
			if(strstr(diff_str, "2") != NULL)
			{
				p_video->corridor_mode_value = 2;
			}
*/
		}

		if(strstr(key, "corridor_support"))
		{
			diff_int = parser_json_get_integer(value);
			p_video->corridor_support = diff_int;
		}
	}
	//printf("[khkh] corridor_mode set end \n");
	//printf("\e[95m[%s][%d] corridor_mode.min(%d) corridor_mode.max(%d) current.corridor_mode(%d) \e[0m\n", __FUNCTION__, __LINE__, p_video->corridor_mode_min, p_video->corridor_mode_max, p_video->corridor_mode_value);
}


void dummy_profile_setting(video_t* p_video, encoder_t* p_encoder)
{
	//dummy profile forced copy
	memcpy(p_video, &model_video_dummy, sizeof(video_t));
	memcpy(p_encoder, &model_encoder_dummy, sizeof(encoder_t));
}

//////////////////////////////////////////////	VIDEO PROFILE END	//////////////////////////////////////////////
//////////////////////////////////////////////	PMASK PROFILE START	//////////////////////////////////////////////

int setting_parsed_privacy_profile(json_t *root, pmask_t* p_pmask)
{
	const char* key;
	json_t *value;
	int i = 0;

	if(root == NULL)
		return CAPA_P_ERROR;

	json_object_foreach(root, key, value)
	{
		if(strstr(key, "height"))
			p_pmask->block_height = json_integer_value(value);
		else if(strstr(key, "width"))
			p_pmask->block_width = json_integer_value(value);
		else if(strstr(key, "area_num"))
			p_pmask->max_rect = json_integer_value(value);
	}

	//json_decref(value);

#if NF_API_JSON_PARSER_DBG
	
	printf("\e[34m[%s][%d] pmask height(%d) width(%d) rect(%d) \e[0m\n", __FUNCTION__, __LINE__,
			p_pmask->block_height, p_pmask->block_width, p_pmask->max_rect);

#endif

	return CAPA_P_OK;
}

//////////////////////////////////////////////	PMASK PROFILE END	//////////////////////////////////////////////
//////////////////////////////////////////////	MOTION PROFILE START	//////////////////////////////////////////////
void setting_motion_sensitivity(values_t* p_value, json_t *element);

static int _enable_smart_options(motion_t* p_motion, const char *str)
{
	int i;
	if(p_motion == NULL || str == NULL) return -1;
	
	for(i=0; i<p_motion->smart_motion_option_size; i++)
	{
		if(strncmp(p_motion->smart_motion_options[i].name, str, strlen(p_motion->smart_motion_options[i].name)) == 0)
		{
			p_motion->smart_motion_options[i].enable = 1;
			return 1;
		}
	}
	return 0;
}

static int _get_smart_motion_threshold(json_t *root, const char *key)
{
	char key_buffer[128];
	snprintf(key_buffer, sizeof(key_buffer), "%s_threshold", str_null_to_blank(key));
	return parser_json_get_integer(json_object_get(json_object_get(root, key_buffer), "value"));
}

int setting_parsed_smart_motion_profile(json_t *root, motion_t* p_motion)
{
	char* rbuf;
	const char* key;
	json_t *value;
	json_t *p_json = NULL;

	int max_opt_num = 3;
	int i = 0;

	char object_str[512] = {0,};
	char *p_str = NULL;

	NFIPCamMotionSmartOption *option = NULL;

	if(root==NULL){
		return CAPA_P_ERROR;
	}
	p_json = json_object_get(root, "smart_motion_support");
	if(p_json == NULL){
		return CAPA_P_ERROR;
	}

	p_motion->smart_motion_enable = 0;
	p_motion->ai_alarm_event = 0;
	memset(p_motion->smart_motion_options, 0x00, sizeof(p_motion->smart_motion_options));
	p_motion->smart_motion_option_size = 0;

	//smart_motion_support
	p_motion->smart_motion_support = parser_json_get_integer(p_json);
	if(p_motion->smart_motion_support == 0){
		char *debug = json_dumps(root, JSON_ENCODE_ANY);
		printf("[%s:%d] smart motion not support [%s]\n", __func__, __LINE__, debug);
		free(debug);
		return CAPA_P_ERROR;
	}

	//smart_motion_enable
	p_json = json_object_get(json_object_get(root, "enable"), "value");
	if(p_json){
		if(strncmp(parser_json_get_string(p_json), "on", 2)==0){
			p_motion->smart_motion_enable = 1;
		}else{
			p_motion->smart_motion_enable = 0;
		}
	}else{
		char *debug = json_dumps(root, JSON_ENCODE_ANY);
		printf("[%s:%d] smart motion enable key not found [%s]\n", __func__, __LINE__, debug);
		free(debug);
	}

	// ai_alarm_event
	p_json = json_object_get(json_object_get(root, "alarmevt"), "value");
	if(p_json){
		if(strncmp(parser_json_get_string(p_json), "on", 2)==0){
			p_motion->ai_alarm_event = 1;
		}else{
			p_motion->ai_alarm_event = 0;
		}
	}else{
		char *debug = json_dumps(root, JSON_ENCODE_ANY);
		printf("[%s:%d] smart motion alarmevt key not found[%s] \n", __func__, __LINE__, debug);
		free(debug);
	}

	//smart_motion_options
	i = 0;
	json_object_foreach(json_object_get(json_object_get(root, "smartmotion_class"), "options"),key,value)
	{
		option = &(p_motion->smart_motion_options[i]);
		snprintf(option->name, sizeof(option->name), "%s", str_null_to_blank(json_string_value(value)));
		option->threshold = _get_smart_motion_threshold(root, option->name);
		p_motion->smart_motion_option_size++;
		i++;
	}
	p_motion->smart_motion_option_size = i;

	//smart motion values
	p_str = parser_json_get_string(json_object_get(json_object_get(root, "smartmotion_class"), "value"));
	if(p_str)
	{
		if(strlen(p_str)){
			strncpy(object_str, p_str, strlen(p_str));
			for(p_str = strtok_r(object_str, ",", &rbuf); p_str != NULL; p_str = strtok_r(NULL, ",", &rbuf)){
				_enable_smart_options(p_motion, p_str);
			}
		}
	}else{
	}

	return CAPA_P_OK;
}

int setting_parsed_motion_profile(json_t *root, motion_t* p_motion)
{
	const char* key;
	json_t *value;

	int max_opt_num = 3;
	int i = 0;

	if(root == NULL)
		return CAPA_P_ERROR;

	json_object_foreach(root, key, value)
	{
		if(strstr(key, "height"))
			p_motion->block_height = json_integer_value(value);
		else if(strstr(key, "width"))
			p_motion->block_width = json_integer_value(value);
		else if(strstr(key, "area_num"))
			p_motion->max_rect = json_integer_value(value);
		else if(strstr(key, "sensitivity0"))
			setting_motion_sensitivity(&p_motion->sensitivity, value);
		else if(strstr(key, "smart_motion"))
			p_motion->smart_motion_support = json_integer_value(value);
	}

	p_motion->min_block = 1;
	p_motion->num_block = (p_motion->block_height)*(p_motion->block_width);

	//json_decref(value);

#if NF_API_JSON_PARSER_DBG
	
	printf("\e[34m[%s][%d] motion height(%d) width(%d) num_block(%d) rect(%d) \e[0m\n", __FUNCTION__, __LINE__,
			p_motion->block_height, p_motion->block_width, p_motion->num_block, p_motion->max_rect);

#endif

	return CAPA_P_OK;
}

void setting_motion_sensitivity(values_t* p_value, json_t *element)
{
	const char* key;
	json_t *value;

	json_object_foreach(element, key, value)
	{
		if(strstr(key, "min"))
			p_value->min = 1;//json_integer_value(value);

		if(strstr(key, "max"))
			p_value->max = 10;//json_integer_value(value);

		if(strstr(key, "value"))
			p_value->value = 10;//json_integer_value(value);
	}
	//json_decref(value);
#if NF_API_JSON_PARSER_DBG
	
	printf("\e[34m[%s][%d] motion sense min(%d) max(%d) value(%d) \e[0m\n", __FUNCTION__, __LINE__,
			p_value->min, p_value->max, p_value->value);

#endif
	
}
//////////////////////////////////////////////	MOTION PROFILE END	//////////////////////////////////////////////
//////////////////////////////////////////////	AUDIO PROFILE START	//////////////////////////////////////////////

void volume_details_setting(values_t *p_capa, json_t *element);

int setting_parsed_audio_profile(json_t *root, audio_t* p_audio)
{
	const char* key;
	json_t *value;
	int i = 0;

	if(root == NULL)
		return CAPA_P_ERROR;

	json_object_foreach(root, key, value)
	{
		if(strstr(key, "audio_in_support"))
			p_audio->audio_rx = json_integer_value(value);
		else if(strstr(key, "audio_out_support"))
			p_audio->audio_tx = json_integer_value(value);
		else if(strstr(key, "audioon"))
		{
			p_audio->acodec.support |= NF_IPCAM_ACODEC_G711_ULAW; 
			p_audio->acodec.value = NF_IPCAM_ACODEC_G711_ULAW; 
		}
		else if(strstr(key, "mic_volume"))
			volume_details_setting(&p_audio->mic_volume, value);
		else if(strstr(key, "spk_volume"))
			volume_details_setting(&p_audio->speaker_volume, value);
	}
	
	//json_decref(value);

#if NF_API_JSON_PARSER_DBG
	printf("\e[33m [%s] audio in(%d) audio out(%d) \e[0m\n", __FUNCTION__, 
			p_audio->audio_rx,
			p_audio->audio_tx);
#endif


	return CAPA_P_OK;
}

void volume_details_setting(values_t *p_capa, json_t *element)
{
	const char *key;
	json_t *value;

	json_object_foreach(element, key, value){
	
		if(strstr(key, "min"))
			p_capa->min = json_integer_value(value); 
		else if(strstr(key, "max"))
			p_capa->max = json_integer_value(value); 
		else if (strstr(key, "value"))
			p_capa->value = json_integer_value(value); 
	}

	//json_decref(value);

#if NF_API_JSON_PARSER_DBG
	printf("\e[33m [%s] min(%d) max(%d) value(%d) \e[0m\n", __FUNCTION__, 
			p_capa->min, 
			p_capa->max, 
			p_capa->value); 
#endif
}
//////////////////////////////////////////////	AUDIO PROFILE END	//////////////////////////////////////////////
//////////////////////////////////////////////	ALARM PROFILE START	//////////////////////////////////////////////

alarm_t dummy_alarm_profile =
{
	0,
	{ NF_IPCAM_ALARM_TYPE_NO | NF_IPCAM_ALARM_TYPE_NC, NF_IPCAM_ALARM_TYPE_NO },
	0,
	{ 0, 0 }
};

int setting_parsed_alarm_profile(json_t *root, alarm_t* p_alarm)
{
	const char* key;
	json_t *value;
	int flag = 0;

	if(root == NULL)
		return CAPA_P_ERROR;

	json_object_foreach(root, key, value)
	{
		if(strstr(key, "alarm_in_support"))
		{
			//not support NVR
			flag++;
		}
		else if(strstr(key, "alarm_in_cnt"))
		{
			//not support NVR	
			flag++;
		}
		else if(strstr(key, "alarm_out_support"))
		{
			//not support NVR	
		}
		else if(strstr(key, "alarm_out_cnt"))
		{
			//not support NVR	
		}
	}
	
	//json_decref(value);

	if(flag > 0)
	{
		memcpy(p_alarm, &dummy_alarm_profile, sizeof(alarm_t));	
	}

#if NF_API_JSON_PARSER_DBG
#endif

	return CAPA_P_OK;
}
//////////////////////////////////////////////	ALARM PROFILE END	//////////////////////////////////////////////


int parse_license_list(json_t *list, ai_license_data *license)
{
	const char *key;
	json_t *value;
    int index = 0;

    const char *license_name;
    int required;
    int installed;

    json_t *json_algorithm;
	const char *algorithm_key;
	json_t *algorithm_value;

    const char *string_name;
    const char *string_value;
    const char *string_algo_type;

	json_object_foreach(list, key, value)
	{
        license_name = key;
        required = json_is_true(json_object_get(value, "required"));
        installed = json_is_true(json_object_get(value, "installed"));
        json_algorithm =  json_object_get(value, "algorithm");


        json_object_foreach(json_algorithm, algorithm_key, algorithm_value)
        {
            string_value = parser_json_get_string(json_object_get(algorithm_value, "value"));
            string_name = parser_json_get_string(json_object_get(algorithm_value, "name"));
            string_algo_type = parser_json_get_string(json_object_get(algorithm_value, "algo_type"));

            /*
            if(string_algo_type != NULL &&
               string_algo_type[0] != '\0' &&
               strstr(string_algo_type, "mot") == NULL){
                continue;
            }*/

            snprintf(license[index].license_name, sizeof(license[index].license_name), license_name);
            license[index].license_required = required;
            license[index].license_installed = installed;

            snprintf(license[index].value, sizeof(license[index].value), str_null_to_blank(string_value));
            snprintf(license[index].name, sizeof(license[index].name), str_null_to_blank(string_name));
            snprintf(license[index].algo_type, sizeof(license[index].algo_type), str_null_to_blank(string_algo_type));

            index++;
            if(index >= IPX_LICENSE_KEY_MAX){
                goto endl;
            }
        }
    }

endl:
    printf("[%s:%d] algorithm_index[%d]\n", __func__, __LINE__, index);
    return index;
}

static int _aicam_license_check(ai_t* ai)
{
    int i;
    for(i = 0; i < ai->license_length; i++){
        /*
        printf("[%s:%d] debug i[%02d] | license[%s] req[%d]ins[%d] value[%s]name[%s]\n", 
                __func__, __LINE__, i,
                ai->license[i].license_name,
                ai->license[i].license_required,
                ai->license[i].license_installed,
                ai->license[i].value,
                ai->license[i].name
              );
              */

        if(ai->license[i].license_required == 0) return 1;
        if(ai->license[i].license_required * ai->license[i].license_installed) return 1;
    }

    return 0;
}

int setting_parsed_ai_profile(json_t *root, ai_t* p_ai)
{
	const char* key;
	json_t *value;
	int flag = 0;
	const char * version_str = NULL;

	if(root == NULL)
		return CAPA_P_ERROR;

	value = json_object_get(root, "version");
	if(value){
        version_str = json_string_value(value);
        if(version_str)
            sscanf(version_str, "%d.%d.%d", &p_ai->version.major, &p_ai->version.minor, &p_ai->version.sub);
    }

    value = json_object_get(root, "License");
    if(value){
        p_ai->license_length = parse_license_list(value, p_ai->license);

        if(_aicam_license_check(p_ai)){
            p_ai->model_type_support |= NF_AI_MODEL_AICAM;
            p_ai->model_type_value = NF_AI_MODEL_AICAM;
        }
    }

    value = json_object_get(root, "ai_module");
    if(value){
        if(json_integer_value(value)){
            p_ai->model_type_support |= NF_AI_MODEL_AICAM_PRO;
            p_ai->model_type_value = NF_AI_MODEL_AICAM_PRO;
        }
    }

	value = json_object_get(root, "ai_rule_engine");
	if(value)
	{
		p_ai->is_rule_engine = 1;
	}

	value = json_object_get(root, "ai_support");
	if(value)
	{
		p_ai->ai_support = 1;
	}
	else
		p_ai->ai_support = 0;

	return CAPA_P_OK;
}

int setting_parsed_vca_profile(json_t *root, vca_t* p_vca)
{
	const char* key;
	json_t *value;
	int flag = 0;
	char * version_str = NULL;

	if(root == NULL)
		return CAPA_P_ERROR;

	json_object_foreach(root, key, value)
	{
		if(strstr(key, "va_support"))
			p_vca->support = json_integer_value(value);
		if(strstr(key, "version"))
		{
			version_str = parser_json_get_string(value);
			printf("\e[33m [%s][%d] version_str = (%s) \e[0m\n", __FUNCTION__, __LINE__, version_str);
			sscanf(version_str, "%d.%d.%d", &p_vca->version.major, &p_vca->version.minor, &p_vca->version.sub);
		}
	}
#if 0	
	printf("\e[31m >>> va_support : (%d) / version : (%d.%d.%d) \e[0m\n", p_vca->support, 
			p_vca->version.major,
			p_vca->version.minor,
			p_vca->version.sub);
#endif
	return CAPA_P_OK;
}

int setting_parsed_roi_profile(json_t *root, roi_t* p_roi)
{
	const char* key = NULL;
	json_t* value = NULL;

	/*	support check	*/
	if(json_integer_value(json_object_get(root, "roi_support")) == 0L){
		p_roi->chipset = -1;
		return CAPA_P_ERROR;
	}

	/* default value(max_rect/chipset) */
	p_roi->max_rect		= 8;
	p_roi->block_width	= 16;
	p_roi->block_height	= 9;

	json_object_foreach(json_object_get(root, "mode"), key, value)
	{
		/* area check */
		if(strstr(key, "areabx")){
			p_roi->block_width = (unsigned int)parser_json_get_integer(json_object_get(value, "max"));
		}else if(strstr(key, "areaby")){
			p_roi->block_height = (unsigned int)parser_json_get_integer(json_object_get(value, "max"));
		/* option check  */
		}else if(strstr(key, "options")){
			if(json_object_get(value, "manual") != NULL){
				p_roi->chipset = NF_IPCAM_HISILICON_CHIPSET_3516C;
				p_roi->options |= ROI_MANUAL;
			}
			if(json_object_get(value, "auto") != NULL){
				p_roi->chipset = NF_IPCAM_HISILICON_CHIPSET_3516D;
				p_roi->options |= ROI_AUTO;
			}
			if(json_object_get(value, "off") != NULL){
				p_roi->options |= ROI_OFF;
			}
		}
	}

	return CAPA_P_OK;
}

static const gpointer dummy_model_func[NF_IPCAM_TYPE_MAX] = 
	{
/* init       */		NULL, &cam_set_osd_off, NULL, NULL, NULL,
/* reboot     */		NULL,
/* factory    */		NULL,
/* vcodec     */		&cam_set_vcodec,
/* acodec     */		NULL, //&cam_set_acodec,
/* image      */		&cam_set_image_nmx,
/* alarm      */		NULL, //&cam_set_alarm,
/* motion     */		NULL,
/* pmask      */		NULL, //&cam_set_pmask,
/* lens       */		&cam_get_af_capa,
/* orig       */		&cam_set_origin,
/* afmode     */		NULL,
/* armode     */		NULL,
/* pt         */		NULL,
/* zoom       */		&cam_set_zoom,
/* focus      */		&cam_set_focus,
/* iris       */		NULL,
/* onepush    */		&cam_set_onepush,
/* get pan    */		NULL,
/* get tilt   */		NULL,
/* get zoom   */		&cam_get_zoom,
/* get focus  */		&cam_get_focus,
/* get iris   */		NULL,
/* stop       */		&cam_set_ptz_stop,
/* set preset */		NULL,
/* clr preset */		NULL,
/* go preset  */		NULL,
/* poll alarm */		NULL,
/* img_onvif  */        NULL,
/* exp_onvif  */        NULL,
/* fcs_onvif  */        NULL,
/* enable va  */		NULL,
/* reset va   */		NULL,
/* va config  */		NULL,
/* va option  */		NULL,
/* set mirror */		NULL,
/* set dton   */		NULL, //&cam_set_dnn_adjust_d2n,
/* set ntod   */		NULL, //&cam_set_dnn_adjust_n2d,
/* focus comp */		&cam_set_focus_comp,
/* dc_iris_cal*/		NULL,
	};

static void parse_max_resol_support(video_t *p_video, uint64_t* p_supported, int p_stream_num)
{
	int i = 0;
	int resol_count = 0;
	uint64_t temp_resol = 0;
// #if defined(_IPX_0824M4) || defined(_IPX_0412M4) ||  defined(_IPX_0824P4E) || \
// 	defined(_IPX_0824M4E) || defined(_IPX_0412M4E)
// 	i = 1;
// #else 
	i = 0;
// #endif 
	if(p_stream_num == 0)
	{
		for(i; i < MAIN_RESOL_LENGTH; i++)
		{
			if(p_video->resolution.resolution[0] & res_table[i].ipcam_res)
			{
				printf("\e[33m [%s][%d] main: i(%d) \e[0m\n", __func__, __LINE__, i);
				temp_resol |= res_table[i].ipcam_res; 
				resol_count++;
			}

			if(resol_count > 0)
				break;
		}
	}
	else
	{
		for(i = SECOND_RESOL_LENGTH; i < MAX_RESOL_LENGTH; i++)
		{
			if(p_video->resolution.resolution[1] & res_table[i].ipcam_res)
			{
				printf("\e[33m [%s][%d] second: i(%d) \e[0m\n", __func__, __LINE__, i);
				temp_resol |= res_table[i].ipcam_res;
				resol_count++;
			}

			if(resol_count > 0)
				break;
		}
	}

	*p_supported |= temp_resol;

	printf("\e[31m[%s][%d] sjlim87 temp_resol(%lld) \e[0m\n", __FUNCTION__, __LINE__, temp_resol);
}

static void switching_video_supported(video_t *p_video, int max_stream)
{
	int i = 0;
	uint64_t temp_support = 0;

	for(i = 0; i < max_stream; i++)
	{
		parse_max_resol_support(p_video, &temp_support, i);
	}

	p_video->resolution.supported = 0;
	p_video->resolution.supported = temp_support;
	printf("\e[31m[%s][%d] sjlim87 temp_resol(%lld) \e[0m\n", __FUNCTION__, __LINE__, temp_support);
}


static int setup_profiles_parsed_all_kind_of_capabilities(int ch, char *p_buffer)
{
	int rtn = CAPA_P_ERROR;
	int image_rtn = 0;

	json_t* root = NULL;
	json_t* result_node = NULL;
	const char* key = NULL;
	json_t* value = NULL;

	int max_stream_support = 2;

	mtable *runtime = get_runtime();

	if(p_buffer == NULL)
		return rtn;

	if(runtime == NULL)
		return rtn;

	//declaration
	image_capa_t tmp_image_profile;
	video_t 	tmp_video_profile;
	encoder_t 	tmp_encoder_profile;
	motion_t 	tmp_motion_profile;
	pmask_t 	tmp_privacy_profile;
	audio_t 	tmp_audio_profile;
	alarm_t 	tmp_alarm_profile;
	vca_t 		tmp_vca_profile;
	ai_t 		tmp_ai_profile;
	roi_t 		tmp_roi_profile;
	
	//init
	memset(&tmp_image_profile, 0x00, sizeof(image_capa_t));
	memset(&tmp_video_profile, 0x00, sizeof(video_t));
	memset(&tmp_encoder_profile, 0x00, sizeof(encoder_t));
	memset(&tmp_motion_profile, 0x00, sizeof(motion_t));
	memset(&tmp_privacy_profile, 0x00, sizeof(pmask_t));
	memset(&tmp_audio_profile, 0x00, sizeof(audio_t));
	memset(&tmp_alarm_profile, 0x00, sizeof(alarm_t));
	memset(&tmp_vca_profile, 0x00, sizeof(vca_t));
	memset(&tmp_ai_profile, 0x00, sizeof(ai_t));
	memset(&tmp_roi_profile, 0x00, sizeof(roi_t));

	//parsed json data to tmp_profiles
	{
		root = parser_load_json(p_buffer);
		if(root == NULL)
		{
			//debug
			printf("\e[33m [%s][%d] sjlim87 \e[0m\n", __FUNCTION__, __LINE__);
			return rtn;
		}

		result_node = parser_category_string(root, "Image");	
		if(result_node)
			image_rtn = setting_parsed_image_profile(result_node, &tmp_image_profile);

		result_node = parser_category_string(root, "Video");
		if(result_node)
			rtn = setting_parsed_video_profile(result_node, &tmp_video_profile, &tmp_encoder_profile);

		//not supported resolution //ksi_test _ti368_set_vcodec() resolution 0으로 죽는 문제
		if(rtn < 0)
		{
			printf("[%s][%d] CH(%d) Resolution is not supported..\n", __FUNCTION__, __LINE__, ch); 
			parser_free_json(root);
			return rtn;
		}

		result_node = parser_category_string(root, "Motion");
		if(result_node)
			rtn = setting_parsed_motion_profile(result_node, &tmp_motion_profile);

		result_node = parser_category_string(root, "PrivacyMask");
		if(result_node)
			rtn = setting_parsed_privacy_profile(result_node, &tmp_privacy_profile);

		result_node = parser_category_string(root, "Audio");	
		if(result_node)
			rtn = setting_parsed_audio_profile(result_node, &tmp_audio_profile);

		result_node = parser_category_string(root, "Alarm");	
		if(result_node)
			rtn = setting_parsed_alarm_profile(result_node, &tmp_alarm_profile);

		result_node = parser_category_string(root, "VA");	
		if(result_node)
			rtn = setting_parsed_vca_profile(result_node, &tmp_vca_profile);

		result_node = parser_category_string(root, "AI");	
		if(result_node)
			rtn = setting_parsed_ai_profile(result_node, &tmp_ai_profile);

		result_node = parser_category_string(root, "ROI");	
		if(result_node)
			rtn = setting_parsed_roi_profile(result_node, &tmp_roi_profile);

		result_node = parser_category_string(root, "Smart_Motion");
		if(result_node)
			rtn = setting_parsed_smart_motion_profile(result_node, &tmp_motion_profile);

		parser_free_json(root);
	}

	if(tmp_image_profile.profile_num < NF_IPCAM_DEPENDENCY_TABLE_MAX)
	{
		//Known Profile
		int found = tmp_image_profile.profile_num;

		image_t tmp_image_prof;
		memset(&tmp_image_prof, 0x00, sizeof(image_t));
		get_models_image_profile(found, &tmp_image_prof);
		memcpy(&runtime[ch].image, &tmp_image_prof, sizeof(image_t));
	}
	else if(tmp_image_profile.profile_num >= NF_IPCAM_DEPENDENCY_TABLE_MAX && image_rtn == 0)
	{
		//Unown Profile && camera no send camera capability table
		//Known Profile
		int found = tmp_image_profile.profile_num;

		image_t tmp_image_prof;
		memset(&tmp_image_prof, 0x00, sizeof(image_t));
		get_models_image_profile(found, &tmp_image_prof);
		memcpy(&runtime[ch].image, &tmp_image_prof, sizeof(image_t));
	}
	else
	{
		//memcpy tmp_profile to runtime profile
		runtime[ch].image.supported = tmp_image_profile.supported;
		runtime[ch].image.onthefly = tmp_image_profile.supported;

		memcpy(&runtime[ch].image.brightness, &tmp_image_profile.brightness, sizeof(values_t));
		memcpy(&runtime[ch].image.color, &tmp_image_profile.color, sizeof(values_t));
		memcpy(&runtime[ch].image.contrast, &tmp_image_profile.contrast, sizeof(values_t));
		memcpy(&runtime[ch].image.sharpness, &tmp_image_profile.sharpness, sizeof(values_t));
		
		memcpy(&runtime[ch].image.wb, &tmp_image_profile.wb, sizeof(modes));
		memcpy(&runtime[ch].image.mwb, &tmp_image_profile.mwb, sizeof(modes));
		memcpy(&runtime[ch].image.blc, &tmp_image_profile.blc, sizeof(modes));
		memcpy(&runtime[ch].image.wd, &tmp_image_profile.wdr, sizeof(modes));
		memcpy(&runtime[ch].image.exposure, &tmp_image_profile.exposure, sizeof(modes));

		memcpy(&runtime[ch].image.max_agc, &tmp_image_profile.max_agc, sizeof(modes));

		memcpy(&runtime[ch].image.agc, &tmp_image_profile.agc, sizeof(values_t));
		memcpy(&runtime[ch].image.eshutter_speed, &tmp_image_profile.eshutter_speed, sizeof(values_t));
		memcpy(&runtime[ch].image.dnn_sense_ntod, &tmp_image_profile.dnn_sense_ntod, sizeof(values_t));
		memcpy(&runtime[ch].image.dnn_sense_dton, &tmp_image_profile.dnn_sense_dton, sizeof(values_t));
	}

	if(tmp_image_profile.supported & NF_IPCAM_IMAGE_COLORVU){
		runtime[ch].image.supported |= NF_IPCAM_IMAGE_COLORVU;
		runtime[ch].image.colorvu_ctrl.support = (int)(1 << NF_IPCAM_COLORVU_CTRL_NR) - 1;
	}

	switching_video_supported(&tmp_video_profile, max_stream_support);

#if defined(_IPX_0824M4) || defined(_IPX_0412M4) ||  defined(_IPX_0824P4E) || \
	defined(_IPX_0824M4E) || defined(_IPX_0412M4E)
	if(tmp_video_profile.resolution.resolution[0] == NF_IPCAM_RES_3000x3000)
		tmp_video_profile.resolution.resolution[0] = NF_IPCAM_RES_2048x2048;

	if(tmp_video_profile.resolution.supported & NF_IPCAM_RES_3000x3000)
		tmp_video_profile.resolution.supported &= ~(NF_IPCAM_RES_3000x3000);

	if(tmp_encoder_profile.res_support[0] & NF_IPCAM_RES_3000x3000)
		tmp_encoder_profile.res_support[0] &= ~(NF_IPCAM_RES_3000x3000);

#endif


	memcpy(&runtime[ch].video,	   &tmp_video_profile,  sizeof(video_t));
	memcpy(&runtime[ch].encoder,   &tmp_encoder_profile,sizeof(encoder_t));

	memcpy(&runtime[ch].funcs, &dummy_model_func, sizeof(gpointer)*NF_IPCAM_TYPE_MAX);
	runtime[ch].recv_handler = &itx_recv_buf_handler;

	if(runtime[ch].image.supported & NF_IPCAM_IMAGE_DC_IRIS_CAL)
	{
		runtime[ch].funcs[NF_IPCAM_TYPE_SET_DC_IRIS_CAL] = (gpointer)&cam_set_dc_iris_cal;
	}

	if(tmp_motion_profile.num_block != 0)
	{
		runtime[ch].func |= NF_IPCAM_FUNC_MOTION;
		memcpy(&runtime[ch].motion,	&tmp_motion_profile,sizeof(motion_t));
		//FIXME
		runtime[ch].motion.method = MAM_RAW_STREAM;

		if(tmp_motion_profile.smart_motion_support)
		{	/* support smart motion */
			runtime[ch].func |= NF_IPCAM_FUNC_SMART_MOTION;
			runtime[ch].funcs[NF_IPCAM_TYPE_SET_SMART_MOTION] = (gpointer)&cam_set_motion_smart;
			runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOTION] = (gpointer)&cam_set_motion_area;

			//초기값
			runtime[ch].motion.smart_motion_enable = 1;
			runtime[ch].motion.ai_alarm_event = 1;
		}
	}

	if(tmp_privacy_profile.max_rect != 0)
	{
		runtime[ch].func |= NF_IPCAM_FUNC_PRIVACY_MASK;
		runtime[ch].funcs[NF_IPCAM_TYPE_SET_PMASK] = (gpointer)&cam_set_pmask;
		memcpy(&runtime[ch].privacymask, &tmp_privacy_profile, sizeof(pmask_t));
	}

	if(tmp_alarm_profile.alarm_in_type.support & NF_IPCAM_ALARM_TYPE_NO)
	{
		runtime[ch].func |= NF_IPCAM_FUNC_ALARM_IN | NF_IPCAM_FUNC_ALARM_OUT;
		runtime[ch].funcs[NF_IPCAM_TYPE_SET_ALARM] = (gpointer)&cam_set_alarm;
		memcpy(&runtime[ch].privacymask, &tmp_privacy_profile, sizeof(pmask_t));
	}

	if(tmp_audio_profile.audio_rx != 0 || tmp_audio_profile.audio_tx != 0)
	{
		if(tmp_audio_profile.audio_rx != 0)
		{
			runtime[ch].func |= NF_IPCAM_FUNC_AUDIO_RX;
		}
		if(tmp_audio_profile.audio_tx != 0)
		{
			runtime[ch].func |= NF_IPCAM_FUNC_AUDIO_TX;
		}

		runtime[ch].funcs[NF_IPCAM_TYPE_SET_ACODEC] = (gpointer)&cam_set_acodec;
		//runtime[ch].funcs[NF_IPCAM_TYPE_SET_ACODEC] = NULL;
		memcpy(&runtime[ch].audio,	   &tmp_audio_profile,  sizeof(audio_t));
	}
	
	if(tmp_vca_profile.support != 0)
	{
		if(tmp_vca_profile.version.major >= 1)
		{
			memcpy(&runtime[ch].vca, &tmp_vca_profile, sizeof(vca_t));
			// VA FUNCTION
			runtime[ch].func |= NF_IPCAM_FUNC_VA;
			runtime[ch].funcs[NF_IPCAM_TYPE_SET_RESET_VA] = (gpointer)&cam_set_reset_va;
			runtime[ch].funcs[NF_IPCAM_TYPE_SET_VA_CONFIG] = (gpointer)&cam_set_va_config;
		}
	}

	if(tmp_ai_profile.version.major >= 1)
	{
        printf("[%s:%d] AI Version %d.%d.%d\n", __func__, __LINE__, 
                tmp_ai_profile.version.major,
                tmp_ai_profile.version.minor,
                tmp_ai_profile.version.sub);

        memcpy(&runtime[ch].ai, &tmp_ai_profile, sizeof(ai_t));
	}

	if(tmp_ai_profile.is_rule_engine)
	{
		runtime[ch].rule_engine.have_ai_engine = 1;
	}
	else
		runtime[ch].rule_engine.have_ai_engine = 0;

	if(tmp_roi_profile.chipset > 0)
	{
		runtime[ch].func |= NF_IPCAM_FUNC_ROI;
		memcpy(&runtime[ch].roi_area, &tmp_roi_profile, sizeof(roi_t));
		runtime[ch].funcs[NF_IPCAM_TYPE_SET_ROI] = (gpointer)&cam_set_roi_area;
	}

	// ADD corridor Mode
	runtime[ch].func |= NF_IPCAM_FUNC_CORRIDOR_MODE;
	runtime[ch].funcs[NF_IPCAM_TYPE_SET_CORRIDOR_MODE] = (gpointer)&cam_set_corridor_mode;


	rtn = CAPA_P_OK;
	return rtn;
}

extern int nf_ipcam_get_json_capabilities(int ch)
{
	int rtn;
	const char *path = "/cgi-bin/webra.fcgi";
	const char *query = "openapi/get_capability";

	const unsigned int buffer_size = 1024 * 30;
	char *buffer = (char*)malloc(buffer_size);

	icm_http http;
	icm_http_ch_init(&http, ch);

	memset(buffer, 0x00, buffer_size);
	rtn = icm_http_get_request(&http, path, query, buffer, buffer_size, "ITXRecorder-1.0");

	//debug
	//printf("\e[31m [%s][%d] buffer : %s \e[0m\n", __FUNCTION__, __LINE__, buffer);

	rtn = setup_profiles_parsed_all_kind_of_capabilities(ch, buffer);

	free(buffer);

	return rtn;
}


