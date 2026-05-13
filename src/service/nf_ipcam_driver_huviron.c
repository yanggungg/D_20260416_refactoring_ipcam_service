#ifndef __NF_IPCAM_DRIVER_HUVIRON_C__
#define __NF_IPCAM_DRIVER_HUVIRON_C__


// HUVIRON API GET ONLY SUPPORT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"
#include "nf_ipcam_utils.h"


static const char param_set[] = "submenu=PrivacyConfig&action=Set&BlindArea[0].Enable=true";
static const char param_get[] = "submenu=PrivacyConfig&action=View";


static int huviron_media_req(const int ch, const char *param, char *buffer, const unsigned int buffer_size)
{
	int rtn = 0;

	icm_http req;
	icm_http_ch_init(&req, ch);
	rtn = icm_http_get_request(&req, "/cgi-bin/media.cgi", param, buffer, buffer_size, NULL);

	return rtn;
}

static void make_privacy_param(const unsigned int num, const int topx, const int topy, const int width, const int height, char *param_out)
{

	const char param_format[] = 
		"&BlindArea[0].AreaParam[%d].TopX=%d"
		"&BlindArea[0].AreaParam[%d].TopY=%d"
		"&BlindArea[0].AreaParam[%d].Width=%d"
		"&BlindArea[0].AreaParam[%d].Height=%d";
	sprintf(param_out, param_format, num, topx, num, topy, num, width, num, height);
}

static void create_privacy_param(const NFIPCamPrivacyMask* pmask_info, char *param_out)
{
	unsigned int privacy_mask_cnt = 0;
	unsigned int i = 0, len = 0, cnt = 0;;
	int width = 0, height = 0, topx = 0, topy = 0;
	char area_param[256];
	char *cat_point = NULL;

	privacy_mask_cnt =(unsigned int) pmask_info->rect_cnt;
	cat_point = param_out;

	for(i = 0; i < 4; i++)
	{
		if(pmask_info->lt[i].x < 0)
		{
			topx = 0;
			topy = 0;
			width = 0;
			height = 0;
		}
		else
		{
			topx = pmask_info->lt[i].x;
			topy = pmask_info->lt[i].y;
			width = pmask_info->rb[i].x - pmask_info->lt[i].x;
			height = pmask_info->rb[i].y - pmask_info->lt[i].y;

			width += 1;
			height += 1;

			topy *= 1250;
			topx *= 625;
			height *= 1250;
			width *= 625;

			if(width >= 10000) 
			{
				width = 9999;
			}

			if(height >= 10000) 
			{
				height = 9999;
			}

			memset(area_param, 0x00, 256);
			make_privacy_param(cnt, topx, topy, width, height, area_param);
			len = strlen(area_param);
			strcat(cat_point, area_param);
			cnt++;
		}
		// range convert: camera 0~9999, nvr 1 ~ 20
	}
}


int huviron_set_privacy_mask(NFIPCamPrivacyMask *pmask_info, int cam_id)
{
	int rtn = -1;
	char param_raw[1024];
	char param_opt_raw[1024];
	NFIPCamPrivacyMask pmask;
	memset(param_raw, 0x00, 1024);
	memcpy(param_raw, param_set, strlen(param_set));
	memcpy(&pmask, pmask_info, sizeof(NFIPCamPrivacyMask));

	create_privacy_param(&pmask, param_raw);

	rtn = huviron_media_req(cam_id, param_raw, NULL, 0);

	if(rtn == 0)
		rtn = IPCAM_SETUP_RTN_DONE;
	else
		rtn = IPCAM_SETUP_RTN_FAILED;

	return rtn;
}

static int setup_run()
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	inet_aton("172.16.0.64", &runtime[0].sys.ipaddr);
	memset(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA], 0x00, 64);
	memset(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE], 0x00, 64);
	strncpy(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA], "http://172.16.0.64/onvif/media_service", 64);
	strncpy(runtime[0].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE], "http://172.16.0.64/onvif/device_service", 64);

	runtime[0].onvif.auth_method = 0;
	strcpy(runtime[0].username, "admin");
	strcpy(runtime[0].password, "admin");

	return 0;

}



#if 0
int main()
{
	int rtn ;
	NFIPCamPrivacyMask pmask_info;

	setup_run();
	pmask_info.ch = 0;
	pmask_info.rect_cnt = 1;
	pmask_info.lt[0].x = 0;
	pmask_info.lt[0].y = 0;
	pmask_info.rb[0].x = 19;
	pmask_info.rb[0].y = 19;
	//pmask_info.rb[0].x = 9999;
	//pmask_info.rb[0].y = 9999;

	pmask_info.lt[1].x = 2501;
	pmask_info.lt[1].y = 2501;
	pmask_info.rb[1].x = 5000;
	pmask_info.rb[1].y = 5000;

	pmask_info.lt[2].x = 5001;
	pmask_info.lt[2].y = 5001;
	pmask_info.rb[2].x = 7500;
	pmask_info.rb[2].y = 7500;

	pmask_info.lt[3].x = 7501;
	pmask_info.lt[3].y = 7501;
	pmask_info.rb[3].x = 9999;
	pmask_info.rb[3].y = 9999;

	pmask_info.lt[4].x = 4000;
	pmask_info.lt[4].y = 4000;
	pmask_info.rb[4].x = 5000;
	pmask_info.rb[4].y = 5000;

	rtn = huviron_set_privacy_mask(&pmask_info, 0);
	return 0;
}
#endif

#endif //__NF_IPCAM_DRIVER_HUVIRON_C__
