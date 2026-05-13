#ifndef __NF_ONVIF_MEDIA2_C__
#define __NF_ONVIF_MEDIA2_C__

#define _GNU_SOURCE 
#include <onvifH.h>
#include <nf_ipcam_defs.h>
#include <string.h>

#define TOKEN_LEN 	(64) // > UUID SIZE 
#define NAME_LEN 	(64) // > UUID SIZE
#define MAX_ATT		(32) // MAX ATTRIBUTE
#define RESOLUTION_CNT_MAX	(32) // MAX ATTRIBUTE
#define FPS_CNT_MAX (60)
#define VEC_CNT_MAX (20)
#define MAX_SIZE	(20)
#define PROFILE_CNT_MAX (4)
#define PROFILE_LEN_MAX (16)
#define MAX_RES_SIZE	(32) // MAX Video Resolution Number
#define URI_LENGTH		(256)
/////////////////////////////////////////////////////////////
extern int _nf_onvif_add_auth2(const int ch, const struct soap *_soap);
extern int _get_fps_num(NF_IPCAM_FPS_E fps);
////////////////////////////////////////////////////////////

struct onvif_profile 
{
	char token[TOKEN_LEN];
	char name[NAME_LEN];
	char vs_token[TOKEN_LEN];	// VideoSourceToken
	char vsc_token[TOKEN_LEN];	// VideoSourceConfigurationToken
	char as_token[TOKEN_LEN];
	char asc_token[TOKEN_LEN];	// AudioSourceConfigurationToken
	char vec_token[TOKEN_LEN];	// VideoEncoderConfigurationToken
	char aec_token[TOKEN_LEN];	// AudioEncoderConfigurationToken
	char aoc_token[TOKEN_LEN];	// AudioOutputConfigurationToken
	char adc_token[TOKEN_LEN];	// AudioDecoderConfigurationToken
	char ptz_token[TOKEN_LEN];	// PTZConfigurationToken
	char vac_token[TOKEN_LEN];	// VideoAnalyticsToken
	char meta_token[TOKEN_LEN];	// MetadataConfigurationToken
	int h264_ProfilesSupported;
	int h265_ProfilesSupported;
	int motion_type;
};

struct onvif_profiles 
{
	int size;
	struct onvif_profile profile[MAX_ATT];
};

struct video_resolution 
{
	int width;
	int height;
};

struct ivalue
{
	int min;
	int max;
	int val;
};

struct fvalue
{
	float min;
	float max;
	float val;
};

struct fps_cap
{
	char fps[60];
	int size;
};

struct res_cap
{
	struct video_resolution resolutions[RESOLUTION_CNT_MAX];
	int size;
};

struct enc_profiles
{
	char profiles[PROFILE_CNT_MAX][PROFILE_LEN_MAX];
	int size;
};

struct video_encoder_option
{
	char codec[32];
	char cbr_supported;
	struct enc_profiles profiles;
	struct res_cap resolution;
	struct fps_cap fps;
	struct ivalue gop;
	struct fvalue quality;
	struct ivalue bitrate;
};

struct video_encoder_options
{
	char token[TOKEN_LEN];
	struct video_encoder_option option[VEC_CNT_MAX];
	int size;
};

struct soap_handle
{
	int ch;
	struct soap *soap;
};

struct video_encoder_setup
{
	char name[128];
	char token[128];
	char encoding[128];
	char profile[64];
	struct video_resolution res;
	float fps;
	int bps;
	int cbr;
	float quality;
	int gov;
};

struct config
{
	char *token;
	char *type;
};

struct configs
{
	int size;
	struct config *configs;
};

struct video_source_config
{
	char token[TOKEN_LEN];
	char source_token[TOKEN_LEN];
};

struct video_source_configs
{
	int size;
	struct video_source_config configs[MAX_SIZE];
};

static int _media2_get_profiles(const struct soap_handle *handle, const char *endpoint, struct onvif_profiles *profiles);
static int _profile_extract(struct media2__MediaProfile *media_profile, struct onvif_profile *out_profile);
static int _profiles_extract(struct _media2__GetProfilesResponse *res, struct onvif_profiles *profiles);

static void soap_handle_init_and_set_auth(const int ch, struct soap_handle *handle)
{
	handle->ch = ch;
	handle->soap = soap_new();
	_nf_onvif_add_auth2(ch, handle->soap);
	handle->soap->connect_timeout = 10;
	handle->soap->send_timeout = 2;
	handle->soap->recv_timeout = 5;
}

static void soap_handle_release(struct soap_handle *handle)
{
	struct soap *soap = handle->soap;
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	handle->soap = NULL;
}


// GetProfiles

static int _profile_extract(struct media2__MediaProfile *media_profile, struct onvif_profile *out_profile)
{
	struct media2__ConfigurationSet *Configurations;

	if(media_profile->Name != NULL)
	{
		strncpy(out_profile->name, media_profile->Name, NAME_LEN);
	}
	if(media_profile->token != NULL)
	{
		strncpy(out_profile->token, media_profile->token, TOKEN_LEN);
	}

	Configurations = media_profile->Configurations;

	if(Configurations != NULL)
	{

		if(Configurations->VideoSource != NULL)
		{
			if(Configurations->VideoSource->token != NULL)
			{
				strncpy(out_profile->vsc_token, Configurations->VideoSource->token, TOKEN_LEN);
			}
			if(Configurations->VideoSource->SourceToken != NULL)
			{
				strncpy(out_profile->vs_token, Configurations->VideoSource->SourceToken, TOKEN_LEN);
			}
		}

		if(Configurations->AudioSource != NULL)
		{
			if(Configurations->AudioSource->token != NULL)
			{
				strncpy(out_profile->asc_token, Configurations->AudioSource->token, TOKEN_LEN);
			}
			if(Configurations->AudioSource->SourceToken != NULL)
			{
				strncpy(out_profile->as_token, Configurations->AudioSource->SourceToken, TOKEN_LEN);
			}
		}


		if(Configurations->VideoEncoder != NULL)
		{
			if(Configurations->VideoEncoder->token != NULL)
			{
				strncpy(out_profile->vec_token, Configurations->VideoEncoder->token, TOKEN_LEN);
			}
		}


		if(Configurations->AudioEncoder != NULL)
		{
			if(Configurations->AudioEncoder->token != NULL)
			{
				strncpy(out_profile->aec_token, Configurations->AudioEncoder->token, TOKEN_LEN);
			}
		}

		if(Configurations->AudioOutput != NULL)
		{
			if(Configurations->AudioOutput->token != NULL)
			{
				strncpy(out_profile->aoc_token, Configurations->AudioOutput->token, TOKEN_LEN);
			}
		}

		if(Configurations->AudioDecoder != NULL)
		{
			if(Configurations->AudioDecoder->token != NULL)
			{
				strncpy(out_profile->adc_token, Configurations->AudioDecoder->token, TOKEN_LEN);
			}
		}

		if(Configurations->PTZ != NULL)
		{
			if(Configurations->PTZ->token != NULL)
			{
				strncpy(out_profile->ptz_token, Configurations->PTZ->token, TOKEN_LEN);
			}
		}

		if(Configurations->Analytics != NULL)
		{
			if(Configurations->Analytics->token != NULL)
			{
				strncpy(out_profile->vac_token, Configurations->Analytics->token, TOKEN_LEN);
			}
		}

		if(Configurations->Metadata != NULL)
		{
			if(Configurations->Metadata->token != NULL)
			{
				strncpy(out_profile->meta_token, Configurations->Metadata->token, TOKEN_LEN);
			}
		}
	}

	return 0;
}

static int _profiles_extract(struct _media2__GetProfilesResponse *res, struct onvif_profiles *profiles)
{
	int i;
	int profile_size;
	struct onvif_profile *profile;
	struct media2__MediaProfile *media_profile;


	profile_size = res->__sizeProfiles;
	for(i = 0; i < profile_size; i++)
	{
		profile = &profiles->profile[i];
		media_profile = &res->Profiles[i];
		_profile_extract(media_profile, profile);
	}

	profiles->size = profile_size;

	return 0;
}


static int _media2_get_profiles(const struct soap_handle *handle, const char *endpoint, struct onvif_profiles *profiles)
{
	int rtn = 0;
	struct soap *soap = NULL;
	soap = handle->soap;
	struct _media2__GetProfiles req;
	struct _media2__GetProfilesResponse res;

	memset(&req, 0x00, sizeof(req));

	req.Token = NULL;
	req.__sizeType = 0;
	req.Type = NULL;

	rtn = soap_call___media2__GetProfiles(soap, endpoint, NULL, &req, &res);
	if(rtn == 0)
	{
		_profiles_extract(&res, profiles);
	}

	return rtn;
}

static int _onvif_media2_get_profiles(const int ch, struct onvif_profiles *profiles)
{
	int rtn;
	struct soap_handle handle;
	char *endpoint = NULL;
	mtable* runtime = NULL;
	
	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	rtn = _media2_get_profiles(&handle, endpoint, profiles);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d] %s\n",__FUNCTION__,__LINE__, buf);
	}

	soap_handle_release(&handle);
	return rtn;
}

// GetVideoEncoderOptions

static int _video_resolution_extract(const struct tt__VideoResolution2 *video_res, struct video_resolution *out)
{
	out->width = video_res->Width;
	out->height = video_res->Height;

	return 0;
}

static int _video_resolutions_extract(const struct tt__VideoEncoder2ConfigurationOptions *res, struct res_cap *res_cap)
{
	int i;
	int size;
	size = res->__sizeResolutionsAvailable;
	for(i = 0; i < size; i++)
	{
		_video_resolution_extract(&res->ResolutionsAvailable[i], &res_cap->resolutions[i]);
	}
	res_cap->size = size;

	return 0;
}

static int _video_extract_gop(const char *gop_lenght_range, struct ivalue *gop)
{
	char *token;
	char *string = strdupa(gop_lenght_range);
	token = strsep(&string, " ");
	gop->min = atoi(token);
	token = strsep(&string, " ");
	gop->max = atoi(token);

	return 0;
}

static int _video_extract_fps(const char *fps_supported, struct fps_cap *fps)
{
	char *token = NULL;
	int cnt = 0;
	char *string = strdupa(fps_supported);
	
	if(strlen(fps_supported))
	{
		while ((token = strsep(&string, " ")))
		{
			fps->fps[cnt] = (char)atoi(token);
			cnt++;
		}
		fps->size = cnt;
	}


	return 0;
}

static int _video_extract_profile(const char *profiles_supported, struct enc_profiles *profiles)
{
	char *token = NULL;
	int cnt = 0;
	char *string = strdupa(profiles_supported);
	if(strlen(profiles_supported))
	{
		while ((token = strsep(&string, " ")))
		{
			strncpy(profiles->profiles[cnt], token, PROFILE_LEN_MAX);
			cnt++;
		}
		profiles->size = cnt;
	}


	return 0;
}

static int _video_encoder_option_extract(const struct tt__VideoEncoder2ConfigurationOptions *res, struct video_encoder_option *opt)
{
	if(res->Encoding != NULL)
	{
		strncpy(opt->codec, res->Encoding, 32);
	}

	if(res->QualityRange != NULL)
	{
		opt->quality.min = res->QualityRange->Min;
		opt->quality.max = res->QualityRange->Max;
	}

	if(res->__sizeResolutionsAvailable > 0 && res->ResolutionsAvailable != NULL)
	{
		_video_resolutions_extract(res, &opt->resolution);
	}

	if(res->BitrateRange != NULL)
	{
		opt->bitrate.min = res->BitrateRange->Min;
		opt->bitrate.max = res->BitrateRange->Max;
	}

	if(res->GovLengthRange != NULL)
	{
		_video_extract_gop(res->GovLengthRange, &opt->gop);
	}

	if(res->FrameRatesSupported != NULL)
	{
		_video_extract_fps(res->FrameRatesSupported, &opt->fps);
	}

	if(res->ProfilesSupported != NULL)
	{
		_video_extract_profile(res->ProfilesSupported, &opt->profiles);
	}

	if(res->ConstantBitRateSupported != NULL)
	{
		opt->cbr_supported = *res->ConstantBitRateSupported;
	}

	return 0;
}

static int _video_encoder_options_extract(const struct _media2__GetVideoEncoderConfigurationOptionsResponse *res, struct video_encoder_options *options)
{
	int rtn;
	int i, size;
	int extract_cnt = 0;

	size = res->__sizeOptions;
	for(i = 0; i < size; i++)
	{
		rtn = _video_encoder_option_extract(&res->Options[i], &options->option[i]);
		extract_cnt++;
	}

	options->size = extract_cnt;


	return rtn;
}

static int _media2_get_video_encoder_options(const struct soap_handle *handle, const char *endpoint, const char *_token, struct video_encoder_options *options)
{
	int rtn = 0;
	struct soap *soap = NULL;
	soap = handle->soap;
	struct media2__GetConfiguration req;
	struct _media2__GetVideoEncoderConfigurationOptionsResponse res; 
	char *token;

	token = strdupa(_token);

	memset(&req, 0x00, sizeof(req));

	req.ConfigurationToken = token;
	rtn = soap_call___media2__GetVideoEncoderConfigurationOptions(soap, endpoint, NULL, &req, &res);
	if(rtn == 0)
	{
		_video_encoder_options_extract(&res, options);
		strncpy(options->token, token, TOKEN_LEN);
	}

	return rtn;
}

static int _onvif_media2_get_video_encoder_options(const int ch, const char *_token, struct video_encoder_options *options)
{
	int rtn;
	struct soap_handle handle;
	char *endpoint = NULL;
	char *token;
	mtable* runtime = NULL;

	token = strdupa(_token);

	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	rtn = _media2_get_video_encoder_options(&handle, endpoint, token, options);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d] %s\n",__FUNCTION__,__LINE__, buf);
	}

	soap_handle_release(&handle);

	return rtn;
}

// GetVideoEncoderConfiguration

static int _media2_get_video_encoder(const struct soap_handle *handle, const char *endpoint, const char *_token, struct video_encoder_setup *setup)
{
	int rtn;
	struct soap *soap = NULL;
	soap = handle->soap;
	char *token;

	struct media2__GetConfiguration req;
	struct _media2__GetVideoEncoderConfigurationsResponse res;

	token = strdupa(_token);

	memset(&req, 0x00, sizeof(req));
	req.ConfigurationToken = token;
	rtn = soap_call___media2__GetVideoEncoderConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		goto ends;
	}

	memset(setup, 0x00, sizeof(*setup));
	if(res.Configurations[0].Name != NULL)
	{
		strncpy(setup->name, res.Configurations[0].Name, 128);
	}
	if(res.Configurations[0].token != NULL)
	{
		strncpy(setup->token, res.Configurations[0].token, 128);
	}
	if(res.Configurations[0].Encoding != NULL)
	{
		strncpy(setup->encoding, res.Configurations[0].Encoding, 128);
	}
	if(res.Configurations[0].Resolution != NULL)
	{
		setup->res.width = res.Configurations[0].Resolution->Width;
		setup->res.height = res.Configurations[0].Resolution->Height;
	}
	if(res.Configurations[0].RateControl != NULL)
	{
		setup->fps = res.Configurations[0].RateControl->FrameRateLimit;
		setup->bps = res.Configurations[0].RateControl->BitrateLimit;
		if(res.Configurations[0].RateControl->ConstantBitRate != NULL)
		{
			setup->cbr = *res.Configurations[0].RateControl->ConstantBitRate;
		}
	}

	setup->quality = res.Configurations[0].Quality;

	if(res.Configurations[0].GovLength != NULL)
	{
		setup->gov = *res.Configurations[0].GovLength;
	}
	if(res.Configurations[0].Profile != NULL)
	{
		strncpy(setup->profile, res.Configurations[0].Profile, 64);
	}

ends:
	return rtn;
}

static int _onvif_media2_get_video_encoder(const int ch, const char *_token, struct video_encoder_setup *setup)
{
	int rtn;
	struct soap_handle handle;
	char *endpoint = NULL;
	char *token;
	mtable* runtime = NULL;

	token = strdupa(_token);

	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	rtn = _media2_get_video_encoder(&handle, endpoint, token, setup);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d] ch(%d) <%s>\n",__FUNCTION__,__LINE__,ch, buf);
	}

	soap_handle_release(&handle);
	return rtn;
}

// SetVideoEncoderConfiguration

static int _media2_set_video_encoder(const struct soap_handle *handle, const char *endpoint, const char *_token, struct video_encoder_setup *setup)
{
	int rtn;
	struct soap *soap = NULL;
	soap = handle->soap;
	char *token;


	struct _media2__SetVideoEncoderConfiguration set_req;
	struct media2__SetConfigurationResponse set_res;
	struct tt__VideoEncoder2Configuration set_conf;
	struct tt__VideoResolution2 Resolution;	
	struct tt__VideoRateControl2 RateControl;
	enum xsd__boolean cbr;

	char *codec = setup->encoding;
	char *profile = setup->profile;
	float fps = setup->fps;
	int bps = setup->bps;
	int bitrate_control = setup->cbr;
	float quality = setup->quality;
	int gov = setup->gov;
	int width =  setup->res.width;
	int height = setup->res.height;

	token = strdupa(_token);

	memset(&set_req, 0x00, sizeof(set_req));
	memset(&set_res, 0x00, sizeof(set_res));
	memset(&set_conf, 0x00, sizeof(set_conf));
	memset(&Resolution, 0x00, sizeof(Resolution));
	memset(&RateControl, 0x00, sizeof(RateControl));

	set_req.Configuration = &set_conf;

	set_conf.Encoding = codec;
	set_conf.Name = token;
	set_conf.token = token;
	set_conf.UseCount = 2; // ??

	Resolution.Width = width;
	Resolution.Height = height;
	set_conf.Resolution = &Resolution;

	set_conf.RateControl = &RateControl;
	RateControl.FrameRateLimit = fps;
	RateControl.BitrateLimit = bps;
	cbr = (enum xsd__boolean) bitrate_control;
	RateControl.ConstantBitRate = &cbr;

	set_conf.GovLength = &gov;
	set_conf.Quality = quality;


	rtn = soap_call___media2__SetVideoEncoderConfiguration(soap, endpoint, NULL, &set_req, NULL);
	if(rtn != 0)
	{
		goto ends;
	}


ends:
	return rtn;
}

static int _onvif_media2_set_video_encoder(const int ch, const char *_token, struct video_encoder_setup *setup)
{
	int rtn;
	struct soap_handle handle;
	char *endpoint = NULL;
	char *token;
	mtable* runtime = NULL;

	token = strdupa(_token);

	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	//exception (SWIPXXP-554)
	if(strncasecmp(runtime[ch].sys.vendor, "Qualvision", 64) == 0)
	{
		printf("[%s] ch(%d) Qualvision cam set gop size(10) \e[0m\n", __func__, ch);
		setup->gov = 10;
	}

	rtn = _media2_set_video_encoder(&handle, endpoint, token, setup);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d](%d) %s\n",__FUNCTION__,__LINE__,rtn, buf);
	}

	soap_handle_release(&handle);

	return rtn;
}

// GetStreamURI

static int _media2_get_stream_uri(const struct soap_handle *handle, const char *endpoint, const char *_token, const char *_protocol, char *uri)
{
	int rtn;
	struct soap *soap = NULL;
	soap = handle->soap;
	char *token;
	char *protocol;

	struct _media2__GetStreamUri req;
	struct _media2__GetStreamUriResponse res;

	token = strdupa(_token);
	protocol = strdupa(_protocol);

	memset(&req, 0x00, sizeof(req));

	req.Protocol = protocol;
	req.ProfileToken = token;

	rtn = soap_call___media2__GetStreamUri(soap, endpoint, NULL, &req, &res);
	if(rtn == 0)
	{
		strncpy(uri, res.Uri, URI_LENGTH);
	}

ends:
	return rtn;
}

static int _onvif_media2_get_stream_uri(const int ch, const char *_token, char *uri)
{
	int rtn;
	struct soap_handle handle;
	char *token = NULL;
	char *endpoint = NULL;
	char *protocol = "RTSP";
	mtable* runtime = NULL;

	token = strdupa(_token);
	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	rtn = _media2_get_stream_uri(&handle, endpoint, token, protocol, uri);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d](%d) %s\n",__FUNCTION__,__LINE__,rtn, buf);
	}

	soap_handle_release(&handle);

	return rtn;
}

// AddConfiguraton

// Configuration Type
// <xs:simpleType name="ConfigurationEnumeration">
// 	<xs:restriction base="xs:string">
// 		<xs:enumeration value="All"/>
// 		<xs:enumeration value="VideoSource"/>
// 		<xs:enumeration value="VideoEncoder"/>
// 		<xs:enumeration value="AudioSource"/>
// 		<xs:enumeration value="AudioEncoder"/>
// 		<xs:enumeration value="AudioOutput"/>
// 		<xs:enumeration value="AudioDecoder"/>
// 		<xs:enumeration value="Metadata"/>
// 		<xs:enumeration value="Analytics"/>
// 		<xs:enumeration value="PTZ"/>
// 	</xs:restriction>
// </xs:simpleType>
static int _media2_add_configuration(const struct soap_handle *handle, const char *endpoint, const char *_profile_token, const struct configs *configs)
{
	int rtn;
	int i;
	struct soap *soap = NULL;
	char *profile_token = strdupa(_profile_token);
	struct _media2__AddConfiguration req;
	struct _media2__AddConfigurationResponse res;
	struct media2__ConfigurationRef *Configuration;

	soap = handle->soap;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	Configuration = (struct media2__ConfigurationRef *)soap_malloc(soap, sizeof(struct media2__ConfigurationRef) * (unsigned int)(configs->size));

	req.ProfileToken = profile_token;
	req.__sizeConfiguration = configs->size;
	req.Configuration = Configuration;


	for(i = 0; i < configs->size; i++)
	{
		Configuration[i].Type = configs->configs[i].type;
		Configuration[i].Token = configs->configs[i].token;
	}


	rtn = soap_call___media2__AddConfiguration(soap, endpoint, NULL, &req, &res);

	return rtn;
}

static int _onvif_media2_add_configuration(const int ch, const char *_profile_token, const struct configs *configs)
{
	int rtn;
	struct soap_handle handle;
	char *endpoint = NULL;
	mtable* runtime = NULL;

	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	rtn = _media2_add_configuration(&handle, endpoint, _profile_token, configs);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d](%d) %s\n",__FUNCTION__,__LINE__,rtn, buf);
	}

	soap_handle_release(&handle);

	return rtn;
}

// RemoveConfiguraton

static int _media2_remove_configuration(const struct soap_handle *handle, const char *endpoint, const char *_profile_token, const struct configs *configs)
{
	int rtn;
	int i;
	struct soap *soap = NULL;
	char *profile_token = strdupa(_profile_token);
	struct media2__ConfigurationRef *Configuration;

	struct _media2__RemoveConfiguration req;
	struct _media2__RemoveConfigurationResponse res;

	soap = handle->soap;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	Configuration = (struct media2__ConfigurationRef *)soap_malloc(soap, sizeof(struct media2__ConfigurationRef) * (unsigned int)(configs->size));

	req.ProfileToken = profile_token;
	req.__sizeConfiguration = configs->size;
	req.Configuration = Configuration;


	for(i = 0; i < configs->size; i++)
	{
		Configuration[i].Type = configs->configs[i].type;
		Configuration[i].Token = configs->configs[i].token;
	}

	rtn = soap_call___media2__RemoveConfiguration(soap, endpoint, NULL, &req, &res);

	return rtn;
}

static int _onvif_media2_remove_configuration(const int ch, const char *_profile_token, const struct configs *configs)
{
	int rtn;
	struct soap_handle handle;
	char *endpoint = NULL;
	mtable* runtime = NULL;

	runtime = get_runtime();

	endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	soap_handle_init_and_set_auth(ch, &handle);

	rtn = _media2_remove_configuration(&handle, endpoint, _profile_token, configs);
	if(rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(handle.soap, buf, 1024);
		printf("[%s:%d](%d) %s\n",__FUNCTION__,__LINE__,rtn, buf);
	}

	soap_handle_release(&handle);

	return rtn;
}

static void _get_width_height(const uint64_t resol, int* w, int* h)
{
	if(w == NULL || h == NULL) return;

	if(resol == NF_IPCAM_RES_3840x2160)
	{
		*w = 3840; *h = 2160;	
	}
	else if(resol == NF_IPCAM_RES_3200x2400)
	{
		*w = 3200; *h = 2400;	
	}
	else if(resol == NF_IPCAM_RES_3072x2048)
	{
		*w = 3072; *h = 2048;	
	}
	else if(resol == NF_IPCAM_RES_2880x2160)
	{
		*w = 2880; *h = 2160;	
	}
	else if(resol == NF_IPCAM_RES_3072x2048)
	{
		*w = 3072; *h = 2048;	
	}
	else if(resol == NF_IPCAM_RES_3200x1800)
	{
		*w = 3200; *h = 1800;	
	}
	else if(resol == NF_IPCAM_RES_2880x1800)
	{
		*w = 2880; *h = 1800;	
	}
	else if(resol == NF_IPCAM_RES_2992x1680)
	{
		*w = 2992; *h = 1680;	
	}
	else if(resol == NF_IPCAM_RES_2592x1944)
	{
		*w = 2592; *h = 1944;	
	}
	else if(resol == NF_IPCAM_RES_2592x1920)
	{
		*w = 2592; *h = 1920;	
	}
	else if(resol == NF_IPCAM_RES_2560x1920)
	{
		*w = 2560; *h = 1920;	
	}
	else if(resol == NF_IPCAM_RES_2560x1600)
	{
		*w = 2560; *h = 1600;	
	}
	else if(resol == NF_IPCAM_RES_2688x1520)
	{
		*w = 2688; *h = 1520;	
	}
	else if(resol == NF_IPCAM_RES_2592x1520)
	{
		*w = 2592; *h = 1520;	
	}
	else if(resol == NF_IPCAM_RES_2560x1440)
	{
		*w = 2560; *h = 1440;	
	}
	else if(resol == NF_IPCAM_RES_2048x1536)
	{
		*w = 2048; *h = 1536;	
	}
	else if(resol == NF_IPCAM_RES_2304x1296)
	{
		*w = 2304; *h = 1296;	
	}
	else if(resol == NF_IPCAM_RES_1920x1080)
	{
		*w = 1920; *h = 1080;	
	}
	else if(resol == NF_IPCAM_RES_1920x1080I)
	{
		*w = 1920; *h = 1080;
	}
	else if(resol == NF_IPCAM_RES_1280x720)
	{
		*w = 1280; *h = 720;	
	}
	else if(resol == NF_IPCAM_RES_1280x720I)
	{
		*w = 1280; *h = 720;	
	}
	else if(resol == NF_IPCAM_RES_1600x1200)
	{
		*w = 1600; *h = 1200;	
	}
	else if(resol == NF_IPCAM_RES_800x600)
	{
		*w = 800; *h = 600;		
	}
	else if(resol == NF_IPCAM_RES_1440x900)
	{
		*w = 1440; *h = 900;	
	}
	else if(resol == NF_IPCAM_RES_800x450)
	{
		*w = 800; *h = 450;		
	}
	else if(resol == NF_IPCAM_RES_640x400)
	{
		*w = 640; *h = 400;		
	}
	else if(resol == NF_IPCAM_RES_640x360)
	{
		*w = 640; *h = 360;		
	}
	else if(resol == NF_IPCAM_RES_640x360I)
	{
		*w = 640; *h = 360;		
	}
	else if(resol == NF_IPCAM_RES_320x180)
	{
		*w = 320; *h = 180;		
	}
	else if(resol == NF_IPCAM_RES_1280x1024)
	{
		*w = 1280; *h = 1024;	
	}
	else if(resol == NF_IPCAM_RES_1024x768)
	{
		*w = 1024; *h = 768;	
	}
	else if(resol == NF_IPCAM_RES_720x576)
	{
		*w = 720; *h = 576;		
	}
	else if(resol == NF_IPCAM_RES_720x480)
	{
		*w = 720; *h = 480;		
	}
	else if(resol == NF_IPCAM_RES_704x576)
	{
		*w = 704; *h = 576;		
	}
	else if(resol == NF_IPCAM_RES_704x480)
	{
		*w = 704; *h = 480;		
	}
	else if(resol == NF_IPCAM_RES_640x480)
	{
		*w = 640; *h = 480;		
	}
	else if(resol == NF_IPCAM_RES_640x352)
	{
		*w = 640; *h = 352;		
	}
	else if(resol == NF_IPCAM_RES_352x288)
	{
		*w = 352; *h = 288;		
	}
	else if(resol == NF_IPCAM_RES_352x240)
	{
		*w = 352; *h = 240;		
	}
	else if(resol == NF_IPCAM_RES_320x240)
	{
		*w = 320; *h = 240;		
	}
	else
	{
		*w = 0; *h = 0;			
	}

}
static void _codec_string_copy(const unsigned int codec, char *codec_string)
{
	if(codec == NF_IPCAM_VCODEC_H265)
	{
		strncpy(codec_string, "H265", 128);
	}
	else if(codec == NF_IPCAM_VCODEC_H264)
	{
		strncpy(codec_string, "H264", 128);
	}
}

ONVIF_API nf_onvif_get_appropriate_profile_media2(const int ch)
{
	int rtn = 0, i;
	struct onvif_profiles profiles;
	struct video_encoder_options veo; 

	memset(&profiles, 0x00, sizeof(profiles));

	rtn = _onvif_media2_get_profiles(ch, &profiles);

	if(rtn != 0)
	{
		goto ends_label;
	}

	for(i = 0; i < profiles.size; i++)
	{
		memset(&veo, 0x00, sizeof(veo));
		if(strlen(profiles.profile[i].token) > 0 && strlen(profiles.profile[i].vec_token) > 0)
		{
			rtn = _onvif_media2_get_video_encoder_options(ch, profiles.profile[i].vec_token, &veo);
		}
	}

ends_label:
	return rtn;
}

ONVIF_API nf_onvif_get_video_capability_media2(const int ch)
{
	int i, j, rtn;
	struct video_encoder_options veo; 
	struct video_encoder_setup ves; 
	mtable* runtime = NULL;
	runtime = get_runtime();

	printf("[%s] ch(%d) video_stream cnt(%d)\n", __FUNCTION__, ch, runtime[ch].video.stream_cnt);
	for(i = 0; i < runtime[ch].video.stream_cnt; i++)
	{
		memset(&veo, 0x00, sizeof(veo));
		printf("[%s] ch(%d) cur cnt(%d)\n", __FUNCTION__, ch, i);
		printf("[%s] ch(%d) vec token(%s)\n", __FUNCTION__, ch,runtime[ch].onvif.vec_token[i]);
		rtn = _onvif_media2_get_video_encoder_options(ch, runtime[ch].onvif.vec_token[i], &veo);
		if(rtn != 0)
			continue;

		rtn = _onvif_media2_get_video_encoder(ch, runtime[ch].onvif.vec_token[i], &ves);
		if(rtn != 0)
			continue;

		for(j = 0; j < veo.size; j++)
		{
			if(strcmp(veo.option[j].codec, "H264") == 0)
			{
				runtime[ch].video.vcodec[i] = NF_IPCAM_VCODEC_H264;
				runtime[ch].encoder.vcodec[i] |= NF_IPCAM_VCODEC_H264; // Set Default

				runtime[ch].onvif.vec_min_quality[i][0] = veo.option[j].quality.min;
				runtime[ch].onvif.vec_max_quality[i][0] = veo.option[j].quality.max;
			}
			if(strcmp(veo.option[j].codec, "H265") == 0)
			{
				runtime[ch].encoder.vcodec[i] |= NF_IPCAM_VCODEC_H265;
				runtime[ch].onvif.vec_min_quality[i][1] = veo.option[j].quality.min;
				runtime[ch].onvif.vec_max_quality[i][1] = veo.option[j].quality.max;
			}

			runtime[ch].encoder.bitctrl[i] |= NF_IPCAM_BITRATE_CONTROL_VBR;
			runtime[ch].video.bitctrl[i] = NF_IPCAM_BITRATE_CONTROL_VBR;

			runtime[ch].onvif.vec_cur_quality[i] = ves.quality;

			if(veo.option[j].cbr_supported)
			{
				runtime[ch].encoder.bitctrl[i] |= NF_IPCAM_BITRATE_CONTROL_CBR;
			}
		}
	}

	return rtn;
}

ONVIF_API nf_onvif_set_stream_media2(const int ch, const cam_info *stream_info)
{
	int i, rtn;
	struct video_encoder_setup setup;
	video_encoder_onvif info;
	mtable *runtime = get_runtime();
	values govGarbage;

	memset(&govGarbage, 0x00, sizeof(govGarbage));

	ipcam_onvif_auth_info_t auth_info;

	for(i = 0 ; i < runtime[ch].video.stream_cnt; i++)
	{
		memset(&info, 0x00, sizeof(video_encoder_onvif));

		_get_width_height((uint64_t)stream_info->vcodec.resolution[i], &setup.res.width, &setup.res.height);
		setup.bps = (int) stream_info->vcodec.bitrate[i];
		if(stream_info->vcodec.bitctrl[i] == NF_IPCAM_BITRATE_CONTROL_CBR)
		{
			setup.cbr = 1;
		}
		else
		{
			setup.cbr = 0;
		}
		setup.fps = (float) _get_fps_num(stream_info->vcodec.fps[i]);
		setup.gov = (int) setup.fps;
		_codec_string_copy(stream_info->vcodec.vcodec[i], setup.encoding);
		setup.quality = (float) runtime[ch].onvif.vec_cur_quality[i];

		rtn = _onvif_media2_set_video_encoder(ch, runtime[ch].onvif.vec_token[i], &setup);
	}

	return IPCAM_SETUP_RTN_DONE;
}





// TEST CODE

static int print_profile(struct onvif_profiles *profiles)
{
	int i;
	int size;
	char uri[URI_LENGTH];

	size = profiles->size;
	for(i = 0; i < size; i++)
	{
		memset(uri, 0x00, URI_LENGTH);
		_onvif_media2_get_stream_uri(0, profiles->profile[i].token, uri);
		printf("\e[31m<<< Profile %d >>>\e[0m\n", i);
		printf("\ttoken: %s\n", profiles->profile[i].token);
		printf("\tname: %s\n", profiles->profile[i].name);
		printf("\t\tvs_token: %s\n", profiles->profile[i].vs_token);
		printf("\t\tvsc_token: %s\n", profiles->profile[i].vsc_token);
		printf("\t\tas_token: %s\n", profiles->profile[i].as_token);
		printf("\t\tasc_token: %s\n", profiles->profile[i].asc_token);
		printf("\t\tvec_token: %s\n", profiles->profile[i].vec_token);
		printf("\t\taec_token: %s\n", profiles->profile[i].aec_token);
		printf("\t\tadc_token: %s\n", profiles->profile[i].adc_token);
		printf("\t\taoc_token: %s\n", profiles->profile[i].aoc_token);
		printf("\t\tptz_token: %s\n", profiles->profile[i].ptz_token);
		printf("\t\tvac_token: %s\n", profiles->profile[i].vac_token);
		printf("\t\tmeta_token: %s\n", profiles->profile[i].meta_token);
		printf("\tURI: %s\n", uri);
		puts("");
	}

	return 0;
}

static int print_encoder_options(struct video_encoder_options *enc_opt)
{
	int size;
	int i, j;

	struct video_encoder_option *opt;
	size = enc_opt->size;

	printf("\e[1;33m<<< VideoEncoderOptions TOKEN[%s]>>>\e[0m\n", enc_opt->token);
	for(i = 0; i < size; i++)
	{
		opt = &enc_opt->option[i];
		//if((strcmp(opt->codec, "H265") == 0) || (strcmp(opt->codec, "H264") == 0))
		{

			printf("\t\e[32m<<< VideoEncoderOption %d >>>\e[0m\n", i);
			printf("\tcodec [%s]\n", opt->codec);
			printf("\tcbr_supported[%d]\n", opt->cbr_supported);

			printf("\tprofiles[%d]\n", opt->profiles.size);
			for(j = 0; j < opt->profiles.size; j++)
				printf("\t\t[%s]\n", opt->profiles.profiles[j]);

			printf("\tresolutionl[%d]\n", opt->resolution.size);
			for(j = 0; j < opt->resolution.size; j++)
				printf("\t\t[%dx%d]\n", opt->resolution.resolutions[j].width, opt->resolution.resolutions[j].height);

			printf("\tfps[%d]\n", opt->fps.size);
			printf("\t\t[");
			for(j = 0; j < opt->fps.size; j++)
				printf("%d ", opt->fps.fps[j]);
			printf("]\n");

			printf("\tgop[%d-%d]\n", opt->gop.min, opt->gop.max);
			printf("\tquality[%.0f-%.0f]\n", opt->quality.min, opt->quality.max);
			printf("\tbitrate[%d-%d]\n", opt->bitrate.min, opt->bitrate.max);
			puts("");
		}
	}

	return 0;
}

static int print_encoder(struct video_encoder_setup *setup)
{
	printf("<<< Video Encoder Configuration >>>\n");
	printf("\tname: %s\n", setup->name);
	printf("\ttoken: %s\n", setup->token);
	printf("\tencoding: %s\n", setup->encoding);
	printf("\tprofile: %s\n", setup->profile);
	printf("\tresolution: %dx%d\n", setup->res.width, setup->res.height);
	printf("\tfps: %.0f\n", setup->fps);
	printf("\tbps: %d\n", setup->bps);
	printf("\tcbr: %d\n", setup->cbr);
	printf("\tquality: %.0f\n", setup->quality);
	printf("\tgov: %d\n", setup->gov);
	return 0;
}

static int remove_add_config_test()
{

	struct onvif_profiles profiles;

	struct configs configs;
	struct config config;
	char *profile_token;
	char *config_token;

	memset(&profiles, 0x00, sizeof(profiles));

	_onvif_media2_get_profiles(0, &profiles);
	print_profile(&profiles);

	profile_token = strdupa(profiles.profile[0].token);
	config_token = strdupa(profiles.profile[0].meta_token);

	configs.size = 1;
	configs.configs = &config;
	config.token = config_token;
	config.type = "Metadata";

	printf("token: %s\n",  config.token);
	_onvif_media2_remove_configuration(0, profile_token, &configs);

	printf("token: %s\n",  config.token);
	memset(&profiles, 0x00, sizeof(profiles));
	_onvif_media2_get_profiles(0, &profiles);
	printf("token: %s\n",  config.token);
	print_profile(&profiles);
	printf("token: %s\n",  config.token);

	_onvif_media2_add_configuration(0, profile_token, &configs);
	printf("token: %s\n",  config.token);

	memset(&profiles, 0x00, sizeof(profiles));
	_onvif_media2_get_profiles(0, &profiles);
	printf("token: %s\n",  config.token);
	print_profile(&profiles);
	return 0;
}

static int test_run()
{
	int i;
	struct onvif_profiles profiles;
	struct video_encoder_options enc_option;
	struct video_encoder_setup setup;
	struct video_encoder_setup setup2;
	
	_onvif_media2_get_profiles(0, &profiles);

	print_profile(&profiles);

	for(i = 0; i < profiles.size; i++)
	{

		memset(&enc_option, 0x00, sizeof(enc_option));
		_onvif_media2_get_video_encoder_options(0, profiles.profile[i].vec_token, &enc_option);
		print_encoder_options(&enc_option);
	}

	_onvif_media2_get_video_encoder(0, profiles.profile[0].vec_token, &setup2);
	print_encoder(&setup2);

	memcpy(&setup, &setup2, sizeof(setup));
	setup.fps = 15.0f;
	setup.encoding[3] = '4';
	setup.bps = 4000;
	setup.cbr = 1;
	setup.gov = 15;
	setup.res.width = 1920;
	setup.res.height = 1080;
	_onvif_media2_set_video_encoder(0, profiles.profile[0].vec_token, &setup);

	memset(&setup2, 0x00, sizeof(setup2));
	_onvif_media2_get_video_encoder(0, profiles.profile[0].vec_token, &setup2);
	print_encoder(&setup2);


	return 0;
}

static int setup_run()
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	memset(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA], 0x00, 64);
	memset(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE], 0x00, 64);
	strncpy(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA], "http://172.16.0.166/onvif/media_service", 64);
	strncpy(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE], "http://172.16.0.166/onvif/device_service", 64);
	printf("Xaddr: %s\n", runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA]);

	runtime[0].onvif.auth_method = 0;
	strcpy(runtime[0].username, "admin");
	strcpy(runtime[0].password, "admin");

	return 0;

}

#if 0
int main()
{
	setup_run();

	while(1)
	{
		test_run();
		remove_add_config_test();
	}


	return 0;
}
#endif


#endif //__NF_ONVIF_MEDIA2_C__


