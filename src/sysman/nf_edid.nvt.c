#include <glib.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#include "nf_object.h"

#include <novatek/hd_type.h>
#include <novatek/hd_common.h>
#include <novatek/hd_videoout.h>
#include <novatek/vendor/vendor_videoout.h>

#include "nf_edid.h"
#include <itx_edid.h>		// in driver/edid/itx_edid.h
#include "nf_fb.h"
#include "nf_edid.nvt.h"

#if 1
	#include <novatek/lcd300/lcd300_fb.h>
	#include <novatek/lcd200_v3/lcd200_fb.h>
#else
	#include <lcd300/lcd300_fb.h>
	#include <lcd200_v3/lcd200_fb.h>
#endif

#if 0
	HDMI : /* LCD300: /dev/fb0,/dev/fb1,/dev/fb2 */ VGA : /* LCD200: /dev/fb3,/dev/fb4,/dev/fb5 */
	SPOT : /* LCD200_1: /dev/fb6,/dev/fb7,/dev/fb8 */
#endif
gboolean nf_edid_chk_nvt_hdmi_raw_data(struct edid_data *info)
{
    
    struct edid_info edid_info;
    INT32 lcd_fd, ret=0;
    lcd_fd = open("/dev/fb0", O_RDWR);

    if((ret = ioctl(lcd_fd, LCD300_IOC_GET_DEVICE_CAPABILITY, &edid_info) < 0)) {

		printf("ioctl:LCD300_IOC_GET_DEVICE_CAPABILITY fail,ret=%d\n", ret);
		return FALSE;
	}

	#if 0
		{
			int i=0;

			printf("HDMI EDID Data ---------\n");
			printf("======================================\n");
			for(i=0; i<512; i++)
			{
				if(i!=0 && ((i%8) == 0))
					printf("\n");
				printf("%02x ", edid_info.u8Edid[i]);
			}
			printf("\n======================================\n");
		}
	#endif

	memcpy(info->raw_data, edid_info.u8Edid, 512);

	close(lcd_fd);

#if 0
	lcd_fd = open("/dev/fb0", O_RDWR);

	if ((ret = ioctl(lcd_fd, LCD200_IOC_GET_DEVICE_CAPABILITY, &edid_info) < 0)) {
	   printf("ioctl:LCD300_IOC_GET_DEVICE_CAPABILITY fail,ret=%d\n", ret);
	}

	{
		int i=0;

		printf("VGA EDID Data ---------\n");
		printf("======================================\n");
		for(i=0; i<512; i++)
		{
			if(i!=0 && ((i%8) == 0))
				printf("\n");
			printf("%02x ", edid_info.u8Edid[i]);
		}
		printf("\n======================================\n");
	}

	close(lcd_fd);
#endif

	return TRUE;
}

gboolean nf_edid_chk_nvt_hdmi(NF_EDID_SUPPORT_RESOLUTION *info)
{
	int i, fbfd, otype;  
	struct lcd300_edid   edid;

	fbfd = open(NF_EDID_DEV_NAME_NVT_HDMI, O_RDWR);
	if (fbfd  == 0) {
		printf("LCD Error: cannot open framebuffer device. \n");
		return FALSE;
	}
	if (ioctl(fbfd, LCD300_IOC_GET_EDID, &edid) < 0) {
		printf("LCD Error: read EDID table fail! \n");
		close(fbfd);
		return FALSE;
	}

	g_message("[Check EDID HDMI]");
	for(i=0; i<edid.valid_num; i++) {
		printf("edid.valid_num[%d] i[%d]\n", edid.valid_num, i);
		if(edid.otype[i] == LCD300_OTYPE_HDMI_720x480x60) {
			printf("edid: 720x480x60 \n");
			info->is_720p60=TRUE;
		}
		if(edid.otype[i] == LCD300_OTYPE_HDMI_720x576x50) {
			printf("edid: 720x576x50 \n");
			info->is_720p50=TRUE;
		}
		if(edid.otype[i] == LCD300_OTYPE_HDMI_1024x768x60) {
			printf("edid: 1024x768x60 \n");
		}
		if(edid.otype[i] == LCD300_OTYPE_HDMI_1280x1024x60) {
			printf("edid: 1280x1024x60 \n");
			info->is_1280_1024_60=TRUE;
		}
		if(edid.otype[i] == LCD300_OTYPE_HDMI_1920x1080x60) {
			printf("edid: 1920x1080x60 \n");
			info->is_1080p60=TRUE;
		}
		if(edid.otype[i] == LCD300_OTYPE_HDMI_1280x720x60) {
			printf("edid: 1280x720x60 \n");
			info->is_720p60=TRUE;
		}
		if(edid.otype[i] == LCD300_OTYPE_HDMI_1600x1200x60) {
			printf("edid: 1600x1200x60 \n");
			info->is_1600p60=TRUE;
		}
		printf("unknow: %d \n", edid.otype[i]);
	}

	nf_edid_dump_reol_info_nvt(info);

	close(fbfd);

	return TRUE;
}


gboolean nf_edid_chk_nvt_vga(NF_EDID_SUPPORT_RESOLUTION *info)
{
	int i, fbfd, otype;
	struct lcd200_edid   edid;

	return FALSE;

	fbfd = open(NF_EDID_DEV_NAME_NVT_VGA, O_RDWR);
	if (fbfd  == 0) {
		printf("LCD Error: cannot open framebuffer device. \n");
		return FALSE;
	}
	if (ioctl(fbfd, LCD200_IOC_GET_EDID, &edid) < 0) {
		printf("LCD Error: read EDID table fail! \n");
		close(fbfd);
		return FALSE;
	}

	g_message("[Check EDID VGA]");
	for(i=0; i<edid.valid_num; i++) {
		printf("edid.valid_num[%d] i[%d]\n", edid.valid_num, i);
		if(edid.otype[i] == LCD200_OTYPE_HDMI_720x480x60) {
			printf("edid: 720x480x60 \n");
			info->is_720p60=TRUE;
		}
		if(edid.otype[i] == LCD200_OTYPE_HDMI_720x576x50) {
			printf("edid: 720x576x50 \n");
			info->is_720p50=TRUE;
		}
		if(edid.otype[i] == LCD200_OTYPE_HDMI_1024x768x60) {
			printf("edid: 1024x768x60 \n");
		}
		if(edid.otype[i] == LCD200_OTYPE_HDMI_1280x1024x60) {
			printf("edid: 1280x1024x60 \n");
			info->is_1280_1024_60=TRUE;
		}
		if(edid.otype[i] == LCD200_OTYPE_HDMI_1920x1080x60) {
			printf("edid: 1920x1080x60 \n");
			info->is_1080p60=TRUE;
		}
		if(edid.otype[i] == LCD200_OTYPE_HDMI_1280x720x60) {
			printf("edid: 1280x720x60 \n");
			info->is_720p60=TRUE;
		}
		if(edid.otype[i] == LCD200_OTYPE_HDMI_1600x1200x60) {
			printf("edid: 1600x1200x60 \n");
			info->is_1600p60=TRUE;
		}
		printf("unknow: %d \n", edid.otype[i]);
	}

	nf_edid_dump_reol_info_nvt(info);

	close(fbfd);

	return TRUE;
}

gboolean nf_edid_chk_nvt_valid(NF_EDID_SUPPORT_RESOLUTION *info, gint resol)
{
	nf_edid_dump_reol_info_nvt(info);

	if(resol == NF_EDID_RES_1280_1024_60) {
		if(info->is_1280_1024_60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_30) {
		if(info->is_720p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_25) {
		if(info->is_720p25) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_60) {
		if(info->is_720p60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_50) {
		if(info->is_720p50) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_60) {
		if(info->is_1080p60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_50) {
		if(info->is_1080p50) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_30) {
		if(info->is_1080p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_25) {
		if(info->is_1080p25) return TRUE;
		else return FALSE;
	}
	#if defined(ENABLE_DISPLAY_2160P)
	else if(resol == NF_EDID_RES_2160P_30) {
		if(info->is_2160p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_25) {
		if(info->is_2160p25) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_50) {
		if(info->is_2160p50) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_60) {
		if(info->is_2160p60) return TRUE;
		else return FALSE;
	}
	#else // Not Support In ANF5HG & UTM5HG
	else if(resol == NF_EDID_RES_2160P_25) {
		return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_50) {
		return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_60) {
		return FALSE;
	}
	#endif
	else if(resol == NF_EDID_RES_1440P_30) {
		if(info->is_1440p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1440P_60) {
		if(info->is_1440p60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1600P_60) {
		if(info->is_1600p60) return TRUE;
		else return FALSE;
	}
	else {
		g_warning("%s Undefined Resolution!! %d", __FUNCTION__, resol);
		return FALSE;
	}
}

HD_RESULT get_videoout_caps(USER_VIDEOOUT *vo_0, USER_VIDEOOUT *vo_1, USER_VIDEOOUT *vo_2)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videoout_get(vo_0->video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &vo_0->lcd_syscaps);
	if (ret != HD_OK) {
		printf("get videoout_0 syscaps fail\n");
		return ret;
	}

	ret = hd_videoout_get(vo_1->video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &vo_1->lcd_syscaps);
	if (ret != HD_OK) {
		printf("get videoout_1 syscaps fail\n");
		return ret;
	}
	ret = hd_videoout_get(vo_2->video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &vo_2->lcd_syscaps);
	if (ret != HD_OK) {
		printf("get videoout_2 syscaps fail\n");
		return ret;
	}

	return ret;
}

HD_RESULT open_videoout_pathid(USER_VIDEOOUT *vo_0, USER_VIDEOOUT *vo_1, USER_VIDEOOUT *vo_2)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &vo_0->video_out_ctrl);
	if (ret != HD_OK) {
		printf("hd_videoout_open:HD_VIDEOOUT_0_CTRL fail\n");
		return ret;
	}
	ret = hd_videoout_open(0, HD_VIDEOOUT_1_CTRL, &vo_1->video_out_ctrl);
	if (ret != HD_OK) {
		printf("hd_videoout_open:HD_VIDEOOUT_1_CTRL fail\n");
		return ret;
	}
	ret = hd_videoout_open(0, HD_VIDEOOUT_2_CTRL, &vo_2->video_out_ctrl);
	if (ret != HD_OK) {
		printf("hd_videoout_open:HD_VIDEOOUT_2_CTRL fail\n");
		return ret;
	}
	ret = hd_videoout_open(HD_VIDEOOUT_IN(0, 0), HD_VIDEOOUT_OUT(0, 0), &vo_0->video_out);
	if (ret != HD_OK) {
		printf("hd_videoout_open vout_0 fail\n");
		return ret;
	}
	ret = hd_videoout_open(HD_VIDEOOUT_IN(1, 0), HD_VIDEOOUT_OUT(1, 0), &vo_1->video_out);
	if (ret != HD_OK) {
		printf("hd_videoout_open vout_1 fail\n");
		return ret;
	}
	ret = hd_videoout_open(HD_VIDEOOUT_IN(2, 0), HD_VIDEOOUT_OUT(2, 0), &vo_2->video_out);
	if (ret != HD_OK) {
		printf("hd_videoout_open vout_2 fail\n");
		return ret;
	}
	return ret;
}

gboolean nf_edid_test(void)
{
	USER_VIDEOOUT usr_vo_0 = {0};
	USER_VIDEOOUT usr_vo_1 = {0};
	USER_VIDEOOUT usr_vo_2 = {0};
	HD_FB_FMT fb_fmt;
	HD_RESULT hd_ret;
	int ret=0;
	int is_yuv422 = 0;

	VENDOR_VIDEOOUT_EDID edid;

	g_message("%s line%d", __FUNCTION__, __LINE__);
	//init memory
	hd_ret = hd_common_init(1);
	if (ret != HD_OK) {
		printf("common init fail\n");
		ret = -1;
	g_message("%s line%d", __FUNCTION__, __LINE__);
		return FALSE;
	} else {
	g_message("%s line%d", __FUNCTION__, __LINE__);
		printf("hd_common_init ok\n");
	}
	g_message("%s line%d", __FUNCTION__, __LINE__);
#if 1
	//init video out module
	hd_ret = hd_videoout_init();
	if (hd_ret != HD_OK) {
		printf("hd_videoout_init fail\n");
		ret = -1;
		return FALSE;
	} else {
		printf("hd_videoout_init ok\n");
	}
#endif

#if 0
	g_message("%s line%d", __FUNCTION__, __LINE__);
	if (open_videoout_pathid(&usr_vo_0, &usr_vo_1, &usr_vo_2) != HD_OK) {
		printf("open_videoout_pathid fail\n");
		ret = -1;
	g_message("%s line%d", __FUNCTION__, __LINE__);
		return FALSE;
	}
	g_message("%s line%d", __FUNCTION__, __LINE__);
#else
#if 0
	ret = get_videoout_caps(&usr_vo_0, &usr_vo_1, &usr_vo_2);
	if (ret != HD_OK) {
		printf("get_videoout_caps fail\n");
		ret = -1;
		return FALSE;
	}
	printf("vo_dim(%ldx%ld)\n", usr_vo_0.lcd_syscaps.input_dim.w, usr_vo_0.lcd_syscaps.input_dim.h);
	printf("v1_dim(%ldx%ld)\n", usr_vo_1.lcd_syscaps.input_dim.w, usr_vo_1.lcd_syscaps.input_dim.h);
	printf("v2_dim(%ldx%ld)\n", usr_vo_2.lcd_syscaps.input_dim.w, usr_vo_2.lcd_syscaps.input_dim.h);
#endif
#endif
	fb_fmt.fb_id = HD_FB0;
	g_message("%s line%d", __FUNCTION__, __LINE__);
	if (hd_videoout_get(usr_vo_0.video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt) != HD_OK) {
	g_message("%s line%d", __FUNCTION__, __LINE__);
		printf("hd_videoout_get:HD_VIDEOOUT_PARAM_FB_FMT, fb0 fail\n");
	}
	g_message("%s line%d", __FUNCTION__, __LINE__);
	if (fb_fmt.fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
		is_yuv422 = 1;
	} else {
		is_yuv422 = 0;//420
	}
	g_message("%s line%d", __FUNCTION__, __LINE__);
	printf("is_yuv422=%d\n", is_yuv422);

	nf_edid_get_edid_nvt(&usr_vo_0, &edid);
}

int nf_edid_get_edid_nvt(USER_VIDEOOUT *p_usr_vo_0, VENDOR_VIDEOOUT_EDID  *edid)
{
	int i;

	if (vendor_videoout_get(p_usr_vo_0->video_out_ctrl, VENDOR_VIDEOOUT_PARAM_EDID, edid) != HD_OK) {
		printf("Error vendor_videoout_get:VENDOR_VIDEOOUT_PARAM_EDID\n");
		return -1;
	}
	for (i = 0; i < edid->valid_num; i++) {
		printf("hdmi support edid.val[%d]=%#lx\n", i, edid->val[i]);
	}
	return 0;
}

void nf_edid_dump_reol_info_nvt(NF_EDID_SUPPORT_RESOLUTION *info)
{
	printf("is_1280_1024_60 [%d] is_720p30  [%d] is_720p25  [%d] is_720p60  [%d]\n"
		   "is_720p50       [%d] is_1080p60 [%d] is_1080p50 [%d] is_1080p30 [%d]\n"
		   "is_1080p30      [%d] is_1080p25 [%d]\n",
			info->is_1280_1024_60, info->is_720p30, info->is_720p25, info->is_720p60,
			info->is_720p50, info->is_1080p60, info->is_1080p50, info->is_1080p30,
			info->is_1080p30, info->is_1080p25);

	#if defined(ENABLE_DISPLAY_2160P)
		printf("is_2160p30 [%d]  is_2160p25  [%d] is_2160p50[%d] is_2160p60[%d]\n",
				info->is_2160p30, info->is_2160p25, info->is_2160p50, info->is_2160p60);
	#endif
	printf("is_1440p30  [%d] is_1440p60[%d] is_1600p60[%d]\n",
			info->is_1440p30, info->is_1440p60, info->is_1600p60);
}

