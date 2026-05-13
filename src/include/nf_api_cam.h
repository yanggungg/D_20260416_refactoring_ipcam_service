#ifndef __NF_API_CAM_H__
#define __NF_API_CAM_H__

/**
	@brief				카메라 속성 변경
	@param[in]	gint	ch
	@param[in]	gint	brightness
	@param[in]	gint	contrast
	@param[in]	gint	tint
	@param[in]	gint	color	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_cam_set_attr ( guint ch, guint brightness, guint contrast, guint tint, guint color );

/**
	@brief				비디오 시그널 타입 얻기
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_sig_type_get( gint *sig_type );

gboolean nf_cam_init();
#endif
