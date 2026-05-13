/**
 * @file nf_onvif_imaging.c
 * @brief ONVIF Imaging 구현.
 * @author
 * @date 16/07/2012
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_ONVIF_IMAGING_C__
#define __NF_ONVIF_IMAGING_C__

#include <stdsoap2.h>
#include <onvifH.h>

#include <nf_ipcam_defs.h>

extern int SOAP_SSL_CLIENT_CONTEXT(struct soap* soap, unsigned short a, const char* b, const char* c, const char* d, const char* e, const char* f);

/* Static msg exchange funcs */
// GetImagingSettings(auth, endpoint, user, pass, token, image_t_onvif)
ONVIF_MSG _nf_onvif_img_get_imaging_settings(ipcam_onvif_auth_info_t* auth_info, const char* endpoint , const char* token, image_t_onvif* image_t);
// GetMoveOptions(auth, endpoint, user, pass, token, image_t_onvif)
ONVIF_MSG _nf_onvif_img_get_move_options(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, image_t_onvif* image_t);
// GetOptions(auth, endpoint, user, pass, token, image_t_onvif)
ONVIF_MSG _nf_onvif_img_get_options(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, image_t_onvif* image_t);
// GetServiceCapabilities(endpoint, capability) // later use
ONVIF_MSG _nf_onvif_img_get_service_capabilities(const char*, struct imaging__Capabilities*);
// GetStatus(auth, endpoint, user, pass, token, position)
ONVIF_MSG _nf_onvif_img_get_status(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, int* position);
// Move(auth, endpoint, user, pass, token, focus_move_onvif)
ONVIF_MSG _nf_onvif_img_move(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token, focus_move_onvif setter, focus_move_t_onvif *move);
// SetImagingSettings(auth, endpoint, user, pass, token, image_info_onvif, image_t_onvif, forcepersistence)
ONVIF_MSG _nf_onvif_img_set_imaging_settings(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token
		,  image_info_onvif setter,  image_t_onvif image_t, const enum xsd__boolean* ForcePersistence);
// Stop(auth, endpoint, user, pass, token)
ONVIF_MSG _nf_onvif_img_stop(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token);

// convert float-range into value(min, max, multiplier)
static void _nf_set_value_with_mux(const struct tt__FloatRange, values*);



/* -------------   ONVIF related APIs for the IPX scenario   --------------- */


/**
 * @brief ONVIF image의 설정 범위를 조회한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_get_image_t(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;


	NF_ONVIF_DBG(MAJOR, "START(CH %d)\n", ch);

	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(strlen(runtime[ch].onvif.vs_token) > 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_IMAGE), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE] != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_img_get_options(&auth_info, endpoint, runtime[ch].onvif.vs_token, &(runtime[ch].image_onvif));
	if(strcmp(runtime[ch].sys.vendor, "H264") == 0)
	{
		runtime[ch].image_onvif.supported_image &= ~NF_IPCAM_IMAGE_ONVIF_SHARPNESS;
		runtime[ch].image_onvif.supported_exposure &= ~NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE;
	}

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "ch (%d) get imaging option REQ error(%d)\n",
				ch, rtn);
		rtn = 0;

		goto ends_label;
	}

	rtn = _nf_onvif_img_get_move_options(&auth_info, endpoint, runtime[ch].onvif.vs_token, &(runtime[ch].image_onvif));

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "get image moving option REQ fail(%d)\n",
				rtn);
	}

ends_label:
	NF_ONVIF_DBG(MAJOR, "END(CH %d)\n",ch);
	return rtn;

}

/**
 * @brief ONVIF image의 현재 설정값을 조회한다.
 * @param ch 채널 번호.
 * @return 0 - 성공,  기타 - 실패.
 */
ONVIF_API nf_onvif_get_image_t_value(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	int rtn = 0;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START(CH %d)\n", ch);

	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(strlen(runtime[ch].onvif.vs_token) > 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_IMAGE), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE] != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_img_get_imaging_settings(&auth_info, endpoint, runtime[ch].onvif.vs_token, &(runtime[ch].image_onvif));

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "ch(%d) get imaging setting REQ fail(%d)\n",
				ch, rtn);
	}

	NF_ONVIF_DBG(MAJOR, "END(CH %d)\n", ch);
	return rtn;

}

/**
 * @brief ONVIF image중 focus를 움직인다.
 * @param ch 채널 번호.
 * @param setter Focus 움직임 정보 struct.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAIL - 실패.
 */
ONVIF_API nf_onvif_focus_move(int ch, focus_move_onvif setter)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START(CH %d)\n", ch);

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(strlen(runtime[ch].onvif.vs_token) > 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);

	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_img_move(&auth_info, endpoint, runtime[ch].onvif.vs_token, setter, &runtime[ch].image_onvif.move);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "ch(%d) focus move REQ fail(%d)\n",
				ch, rtn);
	}

	//nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_FOCUS_ONVIF, -1);
	//rtn = IPCAM_SETUP_RTN_DONE;

	NF_ONVIF_DBG(MAJOR, "END(CH %d)\n", ch);
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF image 중 현재 focus 상태를 조회한다.(위치 및 이동중 여부)
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAIL - 실패.
 */
ONVIF_API nf_onvif_get_status(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START(CH %d)\n", ch);

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(strlen(runtime[ch].onvif.vs_token) > 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_IMAGE), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);

	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_img_get_status(&auth_info, endpoint, runtime[ch].onvif.vs_token, &runtime[ch].image_onvif.move.abposition.value);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(MAJOR, "ch(%d) get status REQ fail(%d)\n",
				ch, rtn);
	}

	NF_ONVIF_DBG(MAJOR, "END(CH %d)\n", ch);
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF image 중 focus 이동을 멈춘다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAIL - 실패.
 */
ONVIF_API nf_onvif_focus_stop(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START(CH %d)\n", ch);

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(strlen(runtime[ch].onvif.vs_token) > 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_img_stop(
			&auth_info, endpoint, runtime[ch].onvif.vs_token);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "ch(%d) focus stop REQ fail(%d)\n",
				ch, rtn);
	}
	else
	{
		nf_onvif_get_status(ch);
	}

	NF_ONVIF_DBG(MAJOR, "END(CH %d)\n",ch);
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF image 설정을 변경한다.
 * @param ch 채널 번호.
 * @param setter Image 설정 struct.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAIL - 실패.
 */
ONVIF_API nf_onvif_set_image(int ch, image_info_onvif setter)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START(CH %d)\n", ch);

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(strlen(runtime[ch].onvif.vs_token) > 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_IMAGE), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE] != NULL, IPCAM_SETUP_RTN_FAILED);


	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	enum xsd__boolean bl = xsd__boolean__true_;

	rtn = _nf_onvif_img_set_imaging_settings(&auth_info, endpoint, runtime[ch].onvif.vs_token, setter, runtime[ch].image_onvif, &bl);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "ch(%d) set imaging setting REQ fail(%d)\n",
				ch, rtn);
	}

	//nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_IMAGE_ONVIF, -1);

	NF_ONVIF_DBG(MAJOR, "END(CH %d)\n", ch);

	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}



/* -------------   ONVIF MSG exchanging methods - Internal use   --------------- */


ONVIF_MSG _nf_onvif_img_get_imaging_settings(ipcam_onvif_auth_info_t* auth_info, const char* endpoint , const char* token, image_t_onvif* image_t)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__GetImagingSettings req;
	struct _imaging__GetImagingSettingsResponse res;

	char *action = "GetImagingSettings";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _imaging__GetImagingSettings));
	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);

	rtn = soap_call___imaging__GetImagingSettings(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}
	if (res.ImagingSettings == NULL)
	{
		NF_ONVIF_DBG(ERROR, "RPC response MSG wrong\n");
		rtn = (-1);
		goto ends_label;
	}

	struct tt__ImagingSettings20 ImagingSetting = *(res.ImagingSettings);

	// image_t setting
	if(ImagingSetting.Brightness != NULL)
	{
		image_t->brightness.value = roundi(*(ImagingSetting.Brightness) * (float)image_t->brightness.multiplier);
	}
	if(ImagingSetting.ColorSaturation != NULL)
	{
		image_t->color.value = roundi(*(ImagingSetting.ColorSaturation) * (float)image_t->color.multiplier);
	}
	if(ImagingSetting.Contrast != NULL)
	{
		image_t->contrast.value = roundi(*(ImagingSetting.Contrast) * (float)image_t->contrast.multiplier);
	}
	if(ImagingSetting.Sharpness != NULL)
	{
		image_t->sharpness.value = roundi(*(ImagingSetting.Sharpness) * (float)image_t->sharpness.multiplier);
	}
	if(ImagingSetting.BacklightCompensation != NULL)
	{
		switch(ImagingSetting.BacklightCompensation->Mode)
		{
			case tt__BacklightCompensationMode__ON:
				image_t->blcmode.value = NF_IPCAM_BLC_MODE_ONVIF_ON;

				break;
			case tt__BacklightCompensationMode__OFF:
				image_t->blcmode.value = NF_IPCAM_BLC_MODE_ONVIF_OFF;

				break;
		}
		if(ImagingSetting.BacklightCompensation->Level != NULL)
		{
			image_t->blclevel.value = roundi(*(ImagingSetting.BacklightCompensation->Level) * (float)image_t->blclevel.multiplier);
		}

	}
	if(ImagingSetting.Focus != NULL)
	{
		switch(ImagingSetting.Focus->AutoFocusMode)
		{
			case tt__AutoFocusMode__AUTO:
				image_t->focus.mode.value = NF_IPCAM_FOCUS_MODE_ONVIF_AUTO;

				break;

			case tt__AutoFocusMode__MANUAL:
				//image_t->focus.mode.value = NF_IPCAM_FOCUS_MODE_ONVIF_MANUAL;
				if(image_t->focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)
				{
					image_t->focus.mode.value = NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS;
				}
				else if(image_t->focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
				{
					image_t->focus.mode.value = NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE;
				}
				else if(image_t->focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
				{
					image_t->focus.mode.value = NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE;
				}


				break;
		}

		if(ImagingSetting.Focus->DefaultSpeed != NULL)
		{
			image_t->focus.defaultspeed.value = roundi(*(ImagingSetting.Focus->DefaultSpeed) * (float)image_t->focus.defaultspeed.multiplier);
		}
		if(ImagingSetting.Focus->NearLimit != NULL)
		{
			image_t->focus.nearlimit.value = roundi(*(ImagingSetting.Focus->NearLimit) * (float)image_t->focus.nearlimit.multiplier);
		}
		if(ImagingSetting.Focus->FarLimit != NULL)
		{
			image_t->focus.farlimit.value = roundi(*(ImagingSetting.Focus->FarLimit) * (float)image_t->focus.farlimit.multiplier);
		}
	}
	if(ImagingSetting.IrCutFilter != NULL)
	{
		switch(*(ImagingSetting.IrCutFilter))
		{
			case tt__IrCutFilterMode__ON:
				image_t->ircut.value = NF_IPCAM_IRCUT_MODE_ONVIF_ON;

				break;
			case tt__IrCutFilterMode__OFF:
				image_t->ircut.value = NF_IPCAM_IRCUT_MODE_ONVIF_OFF;

				break;
			case tt__IrCutFilterMode__AUTO:
				image_t->ircut.value = NF_IPCAM_IRCUT_MODE_ONVIF_AUTO;

				break;
		}
	}

	if(ImagingSetting.WideDynamicRange != NULL)
	{
		switch(ImagingSetting.WideDynamicRange->Mode)
		{
			case tt__WideDynamicMode__OFF:
				image_t->wdrmode.value = NF_IPCAM_WDR_MODE_ONVIF_OFF;

				break;
			case tt__WideDynamicMode__ON:
				image_t->wdrmode.value = NF_IPCAM_WDR_MODE_ONVIF_ON;

				break;
		}

		if(ImagingSetting.WideDynamicRange->Level != NULL)
		{
			image_t->wdrlevel.value = roundi(*(ImagingSetting.WideDynamicRange->Level) * (float)image_t->wdrlevel.multiplier);
		}
	}

	if(ImagingSetting.WhiteBalance != NULL)
	{
		switch(ImagingSetting.WhiteBalance->Mode)
		{
			case tt__WhiteBalanceMode__AUTO:
				image_t->wb.mode.value = NF_IPCAM_WB_MODE_ONVIF_AUTO;

				break;
			case tt__WhiteBalanceMode__MANUAL:
				image_t->wb.mode.value = NF_IPCAM_WB_MODE_ONVIF_MANUAL;

				break;
		}

		if(ImagingSetting.WhiteBalance->CbGain != NULL)
		{
			image_t->wb.cbgain.value = roundi(*(ImagingSetting.WhiteBalance->CbGain) * (float)image_t->wb.cbgain.multiplier);
		}
		if(ImagingSetting.WhiteBalance->CrGain != NULL)
		{
			image_t->wb.crgain.value = roundi(*(ImagingSetting.WhiteBalance->CrGain) * (float)image_t->wb.crgain.multiplier);
		}
	}

	if(ImagingSetting.Exposure != NULL)
	{
		switch(ImagingSetting.Exposure->Mode)
		{
			case tt__ExposureMode__AUTO:
				image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO;

				break;
			case tt__ExposureMode__MANUAL:
				image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL;

				break;
		}
		if(ImagingSetting.Exposure->Priority != NULL)
		{
			switch(*(ImagingSetting.Exposure->Priority))	// optional
			{
				case tt__ExposurePriority__LowNoise:
					image_t->exposure.priority.value = NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE;

					break;
				case tt__ExposurePriority__FrameRate:
					image_t->exposure.priority.value = NF_IPCAM_PRIORITY_MODE_ONVIF_FRAMERATE;

					break;
			}
		}

		if(ImagingSetting.Exposure->MinExposureTime != NULL)
		{
			image_t->exposure.minetime.value = roundi(*(ImagingSetting.Exposure->MinExposureTime) * (float)image_t->exposure.minetime.multiplier);
		}
		if(ImagingSetting.Exposure->MaxExposureTime != NULL)
		{
			image_t->exposure.maxetime.value = roundi(*(ImagingSetting.Exposure->MaxExposureTime) * (float)image_t->exposure.maxetime.multiplier);
		}
		if(ImagingSetting.Exposure->ExposureTime != NULL)
		{
			image_t->exposure.etime.value = roundi(*(ImagingSetting.Exposure->ExposureTime) * (float)image_t->exposure.etime.multiplier);
		}
		if(ImagingSetting.Exposure->MinGain != NULL)
		{
			image_t->exposure.mingain.value = roundi(*(ImagingSetting.Exposure->MinGain) * (float)image_t->exposure.mingain.multiplier);
		}
		if(ImagingSetting.Exposure->MaxGain != NULL)
		{
			image_t->exposure.maxgain.value = roundi(*(ImagingSetting.Exposure->MaxGain) * (float)image_t->exposure.maxgain.multiplier);
		}
		if(ImagingSetting.Exposure->Gain != NULL)
		{
			image_t->exposure.gain.value = roundi(*(ImagingSetting.Exposure->Gain) * (float)image_t->exposure.gain.multiplier);
		}
		if(ImagingSetting.Exposure->MinIris != NULL)
		{
			image_t->exposure.miniris.value = roundi(*(ImagingSetting.Exposure->MinIris) * (float)image_t->exposure.miniris.multiplier);
		}
		if(ImagingSetting.Exposure->MaxIris != NULL)
		{
			image_t->exposure.maxiris.value = roundi(*(ImagingSetting.Exposure->MaxIris) * (float)image_t->exposure.maxiris.multiplier);
		}
		if(ImagingSetting.Exposure->Iris != NULL)
		{
			image_t->exposure.iris.value = roundi(*(ImagingSetting.Exposure->Iris) * (float)image_t->exposure.iris.multiplier);
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_get_options(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, image_t_onvif* image_t)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__GetOptions req;
	struct _imaging__GetOptionsResponse res;

	char *action = "GetOptions";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _imaging__GetOptions));
	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);

	image_t->supported_image = 0;

	image_t->focus.mode.support = 0;
	image_t->ircut.support = 0;
	image_t->wdrmode.support = 0;
	image_t->wb.mode.support = 0;

	image_t->exposure.mode.support = 0;
	image_t->exposure.priority.support = 0;

	image_t->supported_exposure = 0;

	rtn = soap_call___imaging__GetOptions(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}
	if (res.ImagingOptions == NULL)
	{
		NF_ONVIF_DBG(ERROR, "RPC response MSG wrong\n");
		rtn = (-1);
		goto ends_label;
	}

	struct tt__ImagingOptions20 ImagingOption = *(res.ImagingOptions);

	// image_t setting

	if(ImagingOption.Brightness != NULL)
	{
		_nf_set_value_with_mux(*(ImagingOption.Brightness), &(image_t->brightness));

		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;

	}
	if(ImagingOption.ColorSaturation != NULL)
	{
		_nf_set_value_with_mux(*(ImagingOption.ColorSaturation), &image_t->color);

		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_COLOR;

	}
	if(ImagingOption.Contrast != NULL)
	{
		_nf_set_value_with_mux(*(ImagingOption.Contrast), &image_t->contrast);

		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_CONTRAST;

	}
	if(ImagingOption.Sharpness != NULL)
	{
		_nf_set_value_with_mux(*(ImagingOption.Sharpness), &image_t->sharpness);

		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_SHARPNESS;

	}
	if(ImagingOption.BacklightCompensation != NULL)
	{
		for(i = 0; i < ImagingOption.BacklightCompensation->__sizeMode; i++)
		{
			switch(ImagingOption.BacklightCompensation->Mode[i])
			{
				case tt__BacklightCompensationMode__OFF:
					image_t->blcmode.support |= NF_IPCAM_BLC_MODE_ONVIF_OFF;

					break;

				case tt__BacklightCompensationMode__ON:
					image_t->blcmode.support |= NF_IPCAM_BLC_MODE_ONVIF_ON;

					break;
			}
			image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE;
		}
		if(ImagingOption.BacklightCompensation->Level != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.BacklightCompensation->Level), &image_t->blclevel);

			image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL;
		}
	}

	if(ImagingOption.Focus != NULL)
	{
		for(i = 0; i < ImagingOption.Focus->__sizeAutoFocusModes; i++)
		{
			switch(ImagingOption.Focus->AutoFocusModes[i])
			{
				case tt__AutoFocusMode__AUTO:
					image_t->focus.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_AUTO;

					break;

				case tt__AutoFocusMode__MANUAL:
					//image_t->focus.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_MANUAL;

					break;
			}
			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		if(ImagingOption.Focus->DefaultSpeed != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Focus->DefaultSpeed), &image_t->focus.defaultspeed);

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED;
		}
		if(ImagingOption.Focus->NearLimit != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Focus->NearLimit), &image_t->focus.nearlimit);

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT;
		}
		if(ImagingOption.Focus->FarLimit != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Focus->FarLimit), &image_t->focus.farlimit);

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT;
		}
	}

	if(ImagingOption.__sizeIrCutFilterModes > 0)
	{
		for(i = 0; i < ImagingOption.__sizeIrCutFilterModes; i++)
		{
			switch(ImagingOption.IrCutFilterModes[i])
			{
				case tt__IrCutFilterMode__ON:
					image_t->ircut.support |= NF_IPCAM_IRCUT_MODE_ONVIF_ON;

					break;
				case tt__IrCutFilterMode__OFF:
					image_t->ircut.support |= NF_IPCAM_IRCUT_MODE_ONVIF_OFF;

					break;
				case tt__IrCutFilterMode__AUTO:
					image_t->ircut.support |= NF_IPCAM_IRCUT_MODE_ONVIF_AUTO;

					break;
			}
		}
		image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRCUT;
	}

	if(ImagingOption.WideDynamicRange != NULL)
	{
		for(i = 0; i < ImagingOption.WideDynamicRange->__sizeMode; i++)
		{
			switch(ImagingOption.WideDynamicRange->Mode[i])
			{
				case tt__WideDynamicMode__OFF:
					image_t->wdrmode.support |= NF_IPCAM_WDR_MODE_ONVIF_OFF;

					break;
				case tt__WideDynamicMode__ON:
					image_t->wdrmode.support |= NF_IPCAM_WDR_MODE_ONVIF_ON;

					break;
			}
			image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE;
		}
		if(ImagingOption.WideDynamicRange->Level != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.WideDynamicRange->Level), &image_t->wdrlevel);

			image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL;

		}
	}

	if(ImagingOption.WhiteBalance != NULL)
	{
		for(i = 0; i < ImagingOption.WhiteBalance->__sizeMode; i++)
		{
			switch(ImagingOption.WhiteBalance->Mode[i])
			{
				case tt__WhiteBalanceMode__AUTO:
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_ONVIF_AUTO;

					break;
				case tt__WhiteBalanceMode__MANUAL:
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_ONVIF_MANUAL;

					break;
			}
			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_MODE;
		}
		if(ImagingOption.WhiteBalance->YbGain != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.WhiteBalance->YbGain), &image_t->wb.cbgain);

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN;

		}
		if(ImagingOption.WhiteBalance->YrGain != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.WhiteBalance->YrGain), &image_t->wb.crgain);

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN;

		}
	}
	if(ImagingOption.Exposure != NULL)
	{
		for(i = 0; i < ImagingOption.Exposure->__sizeMode; i++)
		{
			switch(ImagingOption.Exposure->Mode[i])
			{
				case tt__ExposureMode__AUTO:
					image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO;

					break;
				case tt__ExposureMode__MANUAL:
					image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL;

					break;
			}
			image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		for(i = 0; i < ImagingOption.Exposure->__sizePriority; i++)
		{
			switch(ImagingOption.Exposure->Priority[i])
			{
				case tt__ExposurePriority__LowNoise:
					image_t->exposure.priority.support |= NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE;

					break;
				case tt__ExposurePriority__FrameRate:
					image_t->exposure.priority.support |= NF_IPCAM_PRIORITY_MODE_ONVIF_FRAMERATE;

					break;
			}
			image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_PRIORITY;
		}
		if(ImagingOption.Exposure->MinExposureTime != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->MinExposureTime), &image_t->exposure.minetime);

			if(image_t->exposure.minetime.min != image_t->exposure.minetime.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINETIME;
			}
		}
		if(ImagingOption.Exposure->MaxExposureTime != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->MaxExposureTime), &image_t->exposure.maxetime);

			if(image_t->exposure.maxetime.min != image_t->exposure.maxetime.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXETIME;
			}
		}
		if(ImagingOption.Exposure->ExposureTime != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->ExposureTime), &image_t->exposure.etime);

			if(image_t->exposure.etime.min != image_t->exposure.etime.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_ETIME;
			}
		}
		if(ImagingOption.Exposure->MinGain != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->MinGain), &image_t->exposure.mingain);

			if(image_t->exposure.mingain.min != image_t->exposure.mingain.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINGAIN;
			}
		}
		if(ImagingOption.Exposure->MaxGain != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->MaxGain), &image_t->exposure.maxgain);

			if(image_t->exposure.maxgain.min != image_t->exposure.maxgain.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN;
			}
		}
		if(ImagingOption.Exposure->Gain != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->Gain), &image_t->exposure.gain);

			if(image_t->exposure.gain.min != image_t->exposure.gain.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_GAIN;
			}
		}
		if(ImagingOption.Exposure->MinIris != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->MinIris), &image_t->exposure.miniris);

			if(image_t->exposure.miniris.min !=  image_t->exposure.miniris.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINIRIS;
			}
		}
		if(ImagingOption.Exposure->MaxIris != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->MaxIris), &image_t->exposure.maxiris);

			if(image_t->exposure.maxiris.min != image_t->exposure.maxiris.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS;
			}
		}
		if(ImagingOption.Exposure->Iris != NULL)
		{
			_nf_set_value_with_mux(*(ImagingOption.Exposure->Iris), &image_t->exposure.iris);

			if(image_t->exposure.iris.min != image_t->exposure.iris.max)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRIS;
			}
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_get_move_options(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, image_t_onvif* image_t)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__GetMoveOptions req;
	struct _imaging__GetMoveOptionsResponse res;

	char *action = "GetMoveOptions";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _imaging__GetMoveOptions));
	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);
	image_t->move.mode.support = 0;

	rtn = soap_call___imaging__GetMoveOptions(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}
	if (res.MoveOptions == NULL)
	{
		NF_ONVIF_DBG(ERROR, "RPC response MSG wrong\n");
		rtn = (-1);
		goto ends_label;
	}
	
	struct tt__MoveOptions20 MoveOption = *(res.MoveOptions);

	goto skip_relative;

	if(MoveOption.Absolute != NULL)
	{
		if (MoveOption.Absolute->Position != NULL)
		{
			image_t->move.abposition.max = MoveOption.Absolute->Position->Max;
			image_t->move.abposition.min = MoveOption.Absolute->Position->Min;
		}
		else
		{
			goto skip_absolute;
		}
		if(MoveOption.Absolute->Speed != NULL)
		{
			image_t->move.abspeed.max = MoveOption.Absolute->Speed->Max;
			image_t->move.abspeed.min = MoveOption.Absolute->Speed->Min;

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED;
		}
		image_t->move.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		image_t->focus.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE;
	}

skip_absolute:
	if(MoveOption.Relative != NULL)
	{
		if (MoveOption.Relative->Distance != NULL)
		{
			image_t->move.redistance.max = MoveOption.Relative->Distance->Max;
			image_t->move.redistance.min = MoveOption.Relative->Distance->Min;
		}
		else
		{
			goto skip_relative;
		}
		if(MoveOption.Relative->Speed != NULL)
		{
			image_t->move.respeed.max = MoveOption.Relative->Speed->Max;
			image_t->move.respeed.min = MoveOption.Relative->Speed->Min;

			image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED;
		}
		image_t->move.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		image_t->focus.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE;
	}

skip_relative:
	if(MoveOption.Continuous != NULL)
	{
		if (MoveOption.Continuous->Speed != NULL)
		{
			int min, max;
			max = MoveOption.Continuous->Speed->Max;
			min = MoveOption.Continuous->Speed->Min;

			if(max != min || max < min )
			{
				image_t->move.cospeed.max = max;
				image_t->move.cospeed.min = min;
			}
			else
			{
				goto ends_label;
			}
		}
		else
		{
			goto ends_label;
		}

		image_t->move.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED;
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		image_t->focus.mode.support |= NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_get_service_capabilities(const char *endpoint, struct imaging__Capabilities* Capability)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__GetServiceCapabilities req;
	struct _imaging__GetServiceCapabilitiesResponse res;

	char *action = "GetServiceCapabilities";


	NF_ONVIF_DBG(MAJOR, "START\n");

	memset(&req, 0x00, sizeof(struct _imaging__GetServiceCapabilities));
	rtn = soap_call___imaging__GetServiceCapabilities(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}
	if (res.Capabilities == NULL)
	{
		NF_ONVIF_DBG(ERROR, "RPC response MSG wrong\n");
		rtn = (-1);
		goto ends_label;
	}
	{
		NF_ONVIF_DBG(MINOR, "GetCapabilities Success!\n");
	}



ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_get_status(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, int* position)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__GetStatus req;
	struct _imaging__GetStatusResponse res;

	char *action = "GetStatus";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _imaging__GetStatus));
	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);

	rtn = soap_call___imaging__GetStatus(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}
	if (res.Status == NULL)
	{
		NF_ONVIF_DBG(ERROR, "RPC response MSG wrong\n");
		rtn = (-1);
		goto ends_label;
	}

	if(res.Status->FocusStatus20 != NULL)
	{
		*position = roundi(res.Status->FocusStatus20->Position);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_move(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token, focus_move_onvif setter, focus_move_t_onvif *move)
{
	int i = 0, j = 0;
	int rtn = 0;
	float speed;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__Move req;
	struct _imaging__MoveResponse res;

	char *action = "Move";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method ,auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);

	req.Focus = soap_malloc(soap, sizeof(struct tt__FocusMove));
	memset(req.Focus, 0x00, sizeof(struct tt__FocusMove));
	switch(setter.mode)
	{
		case NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE:
			speed = (float)(setter.speed)/ (float)(100 / (float)move->abspeed.max);
			req.Focus->Absolute = soap_malloc(soap, sizeof(struct tt__AbsoluteFocus));
			memset(req.Focus->Absolute, 0x00, sizeof(struct tt__AbsoluteFocus));
			req.Focus->Absolute->Position = setter.position;
			if(setter.speed != 0)
			{
				req.Focus->Absolute->Speed = soap_malloc(soap, sizeof(float));
				*(req.Focus->Absolute->Speed) = setter.speed;
			}
			break;

		case NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE:
			speed = (float)(setter.speed)/ (float)(100 / (float)move->respeed.max);
			req.Focus->Relative = soap_malloc(soap, sizeof(struct tt__RelativeFocus));
			memset(req.Focus->Relative, 0x00, sizeof(struct tt__RelativeFocus));
			req.Focus->Relative->Distance = setter.distance;
			if(setter.speed != 0)
			{
				req.Focus->Relative->Speed = soap_malloc(soap, sizeof(float));
				*(req.Focus->Relative->Speed) = setter.speed;
			}
			break;

		case NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS:
			speed = (float)(setter.speed)/ (float)(100 / (float)move->cospeed.max);
			req.Focus->Continuous = soap_malloc(soap, sizeof(struct tt__ContinuousFocus));
			if(speed < move->cospeed.min || speed > move->cospeed.max)
			{
				speed = move->cospeed.min;
			}
			req.Focus->Continuous->Speed = speed;
			break;

		default:
			NF_ONVIF_DBG(ERROR, "REQ contains no option\n");

			rtn = SOAP_CLI_FAULT;

			goto ends_label;
			break;
	}

	rtn = soap_call___imaging__Move(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_set_imaging_settings(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token
		,  image_info_onvif setter,  image_t_onvif image_t, const enum xsd__boolean* ForcePersistence)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__SetImagingSettings req;
	struct _imaging__SetImagingSettingsResponse res;

	char *action = "SetImagingSettings";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);

	req.ImagingSettings = soap_malloc(soap, sizeof(struct tt__ImagingSettings20));

	struct tt__ImagingSettings20 *iset = req.ImagingSettings;
	memset(iset, 0x00, sizeof(struct tt__ImagingSettings20));
	if(image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
	{
		iset->Brightness = soap_malloc(soap, sizeof(float));
		*(iset->Brightness) = (float)setter.brightness / (float)image_t.brightness.multiplier;
	}

	if(image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
	{
		iset->Contrast = soap_malloc(soap, sizeof(float));
		*(iset->Contrast) = (float)setter.contrast / (float)image_t.contrast.multiplier;
	}

	if(image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
	{
		iset->Sharpness = soap_malloc(soap, sizeof(float));
		*(iset->Sharpness) = (float)setter.sharpness / (float)image_t.sharpness.multiplier;
	}

	if(image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
	{
		iset->ColorSaturation = soap_malloc(soap, sizeof(float));
		*(iset->ColorSaturation) = (float)setter.color / (float)image_t.color.multiplier;
	}

	if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE)
	&& (setter.blcmode & (NF_IPCAM_BLC_MODE_ONVIF_ON | NF_IPCAM_BLC_MODE_ONVIF_OFF)))
	{
		iset->BacklightCompensation = soap_malloc(soap, sizeof(struct tt__BacklightCompensation20));
		memset(iset->BacklightCompensation, 0x00, sizeof(struct tt__BacklightCompensation20));
		iset->BacklightCompensation->Mode = (setter.blcmode == NF_IPCAM_BLC_MODE_ONVIF_ON) ? tt__BacklightCompensationMode__ON : tt__BacklightCompensationMode__OFF;
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
		&& (setter.blcmode == NF_IPCAM_BLC_MODE_ONVIF_ON))
		{
			iset->BacklightCompensation->Level = soap_malloc(soap, sizeof(float));
			*(iset->BacklightCompensation->Level) = (float)setter.blclevel / (float)image_t.blclevel.multiplier;
		}
	}


	if((image_t.supported_image &
	  (NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE | NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED | NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT | NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT))
	&& (setter.focus.mode & (NF_IPCAM_FOCUS_MODE_ONVIF_AUTO | NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE | NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE | NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)))
	{
		iset->Focus = soap_malloc(soap, sizeof(struct tt__FocusConfiguration20));
		memset(iset->Focus, 0x00, sizeof(struct tt__FocusConfiguration20));
		iset->Focus->AutoFocusMode = (setter.focus.mode == NF_IPCAM_FOCUS_MODE_ONVIF_AUTO) ? tt__AutoFocusMode__AUTO  : tt__AutoFocusMode__MANUAL;
		if((image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
		&& (setter.focus.mode == NF_IPCAM_FOCUS_MODE_ONVIF_AUTO))
		{
			iset->Focus->DefaultSpeed = soap_malloc(soap, sizeof(float));
			*(iset->Focus->DefaultSpeed) = (float)setter.focus.defaultspeed / (float)image_t.focus.defaultspeed.multiplier;
		}
		if((image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
		&& (setter.focus.mode == NF_IPCAM_FOCUS_MODE_ONVIF_AUTO))
		{
			iset->Focus->NearLimit = soap_malloc(soap, sizeof(float));
			*(iset->Focus->NearLimit) = (float)setter.focus.nearlimit / (float)image_t.focus.nearlimit.multiplier;
		}
		if((image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
		&& (setter.focus.mode == NF_IPCAM_FOCUS_MODE_ONVIF_AUTO))
		{
			iset->Focus->FarLimit = soap_malloc(soap, sizeof(float));
			*(iset->Focus->FarLimit) = (float)setter.focus.farlimit / (float)image_t.focus.farlimit.multiplier;
		}
	}

	if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
	&& (setter.ircut & (NF_IPCAM_IRCUT_MODE_ONVIF_ON | NF_IPCAM_IRCUT_MODE_ONVIF_OFF | NF_IPCAM_IRCUT_MODE_ONVIF_AUTO)))
	{
		iset->IrCutFilter = soap_malloc(soap, sizeof(enum tt__IrCutFilterMode));
		switch(setter.ircut)
		{
			case NF_IPCAM_IRCUT_MODE_ONVIF_ON:
				*(iset->IrCutFilter) = tt__IrCutFilterMode__ON;

				break;
			case NF_IPCAM_IRCUT_MODE_ONVIF_OFF:
				*(iset->IrCutFilter) = tt__IrCutFilterMode__OFF;

				break;
			case NF_IPCAM_IRCUT_MODE_ONVIF_AUTO:
				*(iset->IrCutFilter) = tt__IrCutFilterMode__AUTO;

				break;
		}
	}

	if((image_t.supported_exposure & (NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE | NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL))
	&& (setter.wdrmode & (NF_IPCAM_WDR_MODE_ONVIF_OFF | NF_IPCAM_WDR_MODE_ONVIF_ON)))
	{
		iset->WideDynamicRange = soap_malloc(soap, sizeof(struct tt__WideDynamicRange20));
		memset(iset->WideDynamicRange, 0x00, sizeof(struct tt__WideDynamicRange20));
		iset->WideDynamicRange->Mode = (setter.wdrmode == NF_IPCAM_WDR_MODE_ONVIF_OFF) ? tt__WideDynamicMode__OFF : tt__WideDynamicMode__ON;
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
		&& (setter.wdrmode == NF_IPCAM_WDR_MODE_ONVIF_ON))
		{
			iset->WideDynamicRange->Level = soap_malloc(soap, sizeof(float));
			*(iset->WideDynamicRange->Level) = (float)setter.wdrlevel / (float)image_t.wdrlevel.multiplier;
		}
	}

	if((image_t.supported_image &
	  (NF_IPCAM_IMAGE_ONVIF_WB_MODE | NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN | NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN))
	&& (setter.wb.mode & (NF_IPCAM_WB_MODE_ONVIF_AUTO | NF_IPCAM_WB_MODE_ONVIF_MANUAL)))
	{
		iset->WhiteBalance = soap_malloc(soap, sizeof(struct tt__WhiteBalance20));
		memset(iset->WhiteBalance, 0x00, sizeof(struct tt__WhiteBalance20));
		iset->WhiteBalance->Mode = (setter.wb.mode == NF_IPCAM_WB_MODE_ONVIF_AUTO) ? tt__WhiteBalanceMode__AUTO : tt__WhiteBalanceMode__MANUAL;
		if((image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
		&& (setter.wb.mode == NF_IPCAM_WB_MODE_ONVIF_MANUAL))

		{
			iset->WhiteBalance->CrGain = soap_malloc(soap, sizeof(float));
			*(iset->WhiteBalance->CrGain) = (float)setter.wb.crgain / (float)image_t.wb.crgain.multiplier;
		}
		if((image_t.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
		&& (setter.wb.mode == NF_IPCAM_WB_MODE_ONVIF_MANUAL))
		{
			iset->WhiteBalance->CbGain = soap_malloc(soap, sizeof(float));
			*(iset->WhiteBalance->CbGain) = (float)setter.wb.cbgain / (float)image_t.wb.crgain.multiplier;
		}
	}

	if((image_t.supported_exposure &
	  (NF_IPCAM_EXPOSURE_ONVIF_MODE | NF_IPCAM_EXPOSURE_ONVIF_PRIORITY | NF_IPCAM_EXPOSURE_ONVIF_MINETIME | NF_IPCAM_EXPOSURE_ONVIF_MAXETIME |
	   NF_IPCAM_EXPOSURE_ONVIF_ETIME | NF_IPCAM_EXPOSURE_ONVIF_MINGAIN | NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN | NF_IPCAM_EXPOSURE_ONVIF_GAIN |
	   NF_IPCAM_EXPOSURE_ONVIF_MINIRIS | NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS | NF_IPCAM_EXPOSURE_ONVIF_IRIS | NF_IPCAM_EXPOSURE_ONVIF_BOTTOM |
	   NF_IPCAM_EXPOSURE_ONVIF_TOP | NF_IPCAM_EXPOSURE_ONVIF_RIGHT | NF_IPCAM_EXPOSURE_ONVIF_LEFT ))
	&& (setter.exposure.mode & (NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO | NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL)))
	{
		iset->Exposure = soap_malloc(soap, sizeof(struct tt__Exposure20));
		memset(iset->Exposure, 0x00, sizeof(struct tt__Exposure20));
		iset->Exposure->Mode = (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO) ? tt__ExposureMode__AUTO : tt__ExposureMode__MANUAL;
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_PRIORITY)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO)
		&& (setter.exposure.priority & (NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE | NF_IPCAM_PRIORITY_MODE_ONVIF_FRAMERATE)))
		{
			iset->Exposure->Priority = soap_malloc(soap, sizeof(enum tt__ExposurePriority));
			*(iset->Exposure->Priority) = (setter.exposure.priority == NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE) ? tt__ExposurePriority__LowNoise : tt__ExposurePriority__FrameRate;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO))
		{
			iset->Exposure->MinExposureTime = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->MinExposureTime) = (float)setter.exposure.minetime / (float)image_t.exposure.minetime.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO))
		{
			iset->Exposure->MaxExposureTime = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->MaxExposureTime) = (float)setter.exposure.maxetime / (float)image_t.exposure.maxetime.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL))
		{
			iset->Exposure->ExposureTime = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->ExposureTime) = (float)setter.exposure.etime / (float)image_t.exposure.etime.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO))
		{
			iset->Exposure->MinGain = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->MinGain) = (float)setter.exposure.mingain / (float)image_t.exposure.mingain.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO))
		{
			iset->Exposure->MaxGain = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->MaxGain) = (float)setter.exposure.maxgain / (float)image_t.exposure.maxgain.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL))
		{
			iset->Exposure->Gain = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->Gain) = (float)setter.exposure.gain / (float)image_t.exposure.gain.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO))
		{
			iset->Exposure->MinIris = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->MinIris) = (float)setter.exposure.miniris / (float)image_t.exposure.miniris.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO))
		{
			iset->Exposure->MaxIris = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->MaxIris) = (float)setter.exposure.maxiris / (float)image_t.exposure.maxiris.multiplier;
		}
		if((image_t.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
		&& (setter.exposure.mode == NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL))
		{
			iset->Exposure->Iris = soap_malloc(soap, sizeof(float));
			*(iset->Exposure->Iris) = (float)setter.exposure.iris / (float)image_t.exposure.iris.multiplier;
		}
	}

	if(ForcePersistence != NULL)	// optional
	{
		req.ForcePersistence = soap_malloc(soap, sizeof(enum xsd__boolean));
		*(req.ForcePersistence) = *ForcePersistence;
	}
	else
	{
		req.ForcePersistence = NULL;
	}

	rtn = soap_call___imaging__SetImagingSettings(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_img_stop(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token)
{
	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _imaging__Stop req;
	struct _imaging__StopResponse res;

	char *action = "Stop";


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _imaging__Stop));
	req.VideoSourceToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.VideoSourceToken, token);

	rtn = soap_call___imaging__Stop(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "RPC returns fail\n");

		SOAP_ERROR_PRINT(soap);
	}
	
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

/**
 * @brief Float 범위값 중 최소값이 0.0xx일 때 int형으로 변환.
 * @param[in] range 변환할 float range.
 * @param[out] value 대상 int range.
 */
static void _nf_set_value_with_mux(const struct tt__FloatRange range, values* value)
{
	int mux = 1;
	if(range.Min > 0 && range.Min < 1)
	{
		float temp = 1 / range.Min;
		while(mux < temp)
		{
			mux *= 10;
		}
	}

	if((range.Min == 0 || (range.Min >= 0 && range.Min < 1)) && range.Max <= 1)
	{
		mux = 100;
	}

	value->multiplier = mux;
	value->min = roundi(range.Min * mux);
	value->max = roundi(range.Max * mux);
}


#endif //__NF_ONVIF_IMAGING_C__

