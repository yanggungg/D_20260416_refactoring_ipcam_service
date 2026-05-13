#include "nf_common.h"
#include "nf_api_cam.h"

#include "nf_util_device.h"

//#define DEBUG_CAM_API

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "api_cam"


// function definition
static guint _get_cam_brightness(guint);
static guint _get_cam_contrast(guint);
static guint _get_cam_tint(guint);
static guint _get_cam_color(guint);

/**
	@brief						get brightness infomation
	@param[in]	ch				channel number	
	@return		brightness		brightness infomation
*/
static guint _get_cam_brightness(guint ch)
{
	char tmp_key[256];
	
	g_return_val_if_fail ( ch<NUM_CHANNEL, 0 );

	sprintf(tmp_key , "cam.C%d.bright", ch);
	
	return nf_sysdb_get_uint(tmp_key);
}

/**
	@brief						get contrast infomation
	@param[in]	ch				channel number	
	@return		brightness		contrast infomation
*/
static guint _get_cam_contrast(guint ch)
{
	char tmp_key[256];
	
	g_return_val_if_fail ( ch<NUM_CHANNEL, 0 );

	sprintf(tmp_key , "cam.C%d.contrast", ch);
	
	return nf_sysdb_get_uint(tmp_key);
}

/**
	@brief						get tint infomation
	@param[in]	ch				channel number	
	@return		brightness		tint infomation
*/
static guint _get_cam_tint(guint ch)
{
	char tmp_key[256];

	g_return_val_if_fail ( ch<NUM_CHANNEL, 0 );
	
	sprintf(tmp_key , "cam.C%d.tint", ch);
	
	return nf_sysdb_get_uint(tmp_key);
}

/**
	@brief						get color infomation
	@param[in]	ch				channel number	
	@return		brightness		color infomation
*/
static guint _get_cam_color(guint ch)
{
	char tmp_key[256];

	g_return_val_if_fail ( ch<NUM_CHANNEL, 0 );
	
	sprintf(tmp_key , "cam.C%d.color", ch);
	
	return nf_sysdb_get_uint(tmp_key);
}

/**
	@brief						set brightness contrast tint color
	@return		 gboolean   	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_cam_init()
{
	gboolean ret = TRUE; 
	guint ch, brightness, contrast, tint, color;

#ifdef DEBUG_CAM_API
	g_message("%s called!!", __FUNCTION__);
#endif

	for(ch=0; ch < NUM_ACTIVE_CH; ++ch)
	{
		brightness = _get_cam_brightness(ch);
		contrast = _get_cam_contrast(ch);
		tint = _get_cam_tint(ch);
		color = _get_cam_color(ch);

		ret = nf_cam_set_attr(ch,brightness, contrast ,tint, color);
		if(!ret){
			g_warning("%s error!!! ret[%d]",__FUNCTION__, ret);
			return ret;
		}
	}

	return ret;
}

/**
	@brief				카메라 속성 변경
	@param[in]	gint	ch
	@param[in]	guint	brightness
	@param[in]	guint	contrast
	@param[in]	guint	tint
	@param[in]	guint	color	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_cam_set_attr ( guint ch, guint brightness, guint contrast, guint tint, guint color )
{
#ifdef DEBUG_CAM_API
	g_message("%s called!!", __FUNCTION__);
#endif

#ifdef DEBUG_CAM_API
	g_message("%s ch[%d] br[%d] cr[%d] tint[%d] co[%d]",
		   		__FUNCTION__, ch, brightness, contrast, tint, color);
#endif

	g_return_val_if_fail( ch<NUM_CHANNEL, 0);
	g_return_val_if_fail( brightness<=100, 0);
	g_return_val_if_fail( contrast<=100, 0);
	g_return_val_if_fail( tint<=100, 0);
	g_return_val_if_fail( color<=100, 0);

#if defined(USE_DEV_TW2864)
	return nf_dev_tw2864_set_picture( ch, brightness, tint, color, contrast );
#else
	g_message("%s Not Implemented Yet!!", __FUNCTION__);

	return TRUE;
#endif
}

/**
	@brief				비디오 시그널 타입 얻기
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_sig_type_get( gint *sig_type )
{
#ifdef DEBUG_CAM_API
	g_message("%s called!!", __FUNCTION__);
#endif

	*sig_type = DISPLAY_IS_PAL;

	return 1;	
}
