#ifndef __NF_EDID_NVT_H__
#define __NF_EDID_NVT_H__

#define NF_EDID_DEV_NAME_NVT_HDMI		"/dev/fb0"
#define NF_EDID_DEV_NAME_NVT_VGA		"/dev/fb3"

typedef struct _VIDEOOUT_FBINFO {
	int lcd_fb;
	int fb_mmap_len;
	unsigned char *fb_base;
	struct fb_var_screeninfo vinfo;
} VIDEOOUT_FBINFO;

typedef struct _BUFINFO {
	UINT64 pool;
	HD_COMMON_MEM_DDR_ID ddr;
	UINT32 buf_size;
	HD_DIM dim;
	UINT32 buf_pa;
	HD_COMMON_MEM_VB_BLK blk;
	char *buf_va;
} BUFINFO;

typedef struct _USER_VIDEOOUT {
	HD_PATH_ID video_out_ctrl;
	HD_PATH_ID video_out;
	HD_VIDEOOUT_SYSCAPS lcd_syscaps;
	HD_VIDEO_FRAME video_out_frame;
	BUFINFO buf_info;
	VIDEOOUT_FBINFO fb1_info;
} USER_VIDEOOUT;

gboolean nf_edid_chk_nvt_hdmi_raw_data(struct edid_data *info);
gboolean nf_edid_chk_nvt_hdmi(NF_EDID_SUPPORT_RESOLUTION *info);
gboolean nf_edid_chk_nvt_vga(NF_EDID_SUPPORT_RESOLUTION *info);
int nf_edid_get_edid_nvt(USER_VIDEOOUT *p_usr_vo_0, VENDOR_VIDEOOUT_EDID  *edid);
void nf_edid_dump_reol_info_nvt(NF_EDID_SUPPORT_RESOLUTION *info);

#endif

