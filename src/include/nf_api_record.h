#ifndef __NF_API_RECORD_H__
#define __NF_API_RECORD_H__

#include <glib.h>

/**
	@brief				패닉 레코드 시작
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_rec_start( GError **error );
/**
	@brief				패닉 레코드 중지
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_rec_stop(  GError **error );


/**
	@brief				레코드 시작
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_rec_start( GError **error );

/**
	@brief				레코드 중지
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_rec_stop(  GError **error );
#endif
