/* set tabspace4 */
/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION:

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
02/14/2011 DongUk Park    Created. (Based on nmf 2.0 api )
02/18/2011 DongUk Park    Unit test done at IPX Board and PC
*/
#ifndef __NF_API_LIVE_H__
#define __NF_API_LIVE_H__
#include <nf_common.h>
#include <nf_util_device.h>
#include <stdint.h>

typedef enum _NF_LIVE_PSE_ACT_E
{
	NF_LIVE_PSE_ACT_LOCAL	= 0,
	NF_LIVE_PSE_ACT_UI		= 1,
	NF_LIVE_PSE_ACT_WEB		= 2
} NF_LIVE_PSE_ACT;

gpointer nf_live_get_display_handle();
gpointer nf_live_get_mrtp_pipe_handle();

gboolean nf_live_init_qc (NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
                            guint win_w, guint win_h, gchar ch_arr[32],
                            gchar covert_arr[32],
                            gint au_in_vd_ch );

gboolean nf_live_init (NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
                            guint win_w, guint win_h, gchar ch_arr[32],
                            gchar covert_arr[32],
                            gint au_in_vd_ch );

gboolean nf_live_start( NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
                            guint win_w, guint win_h, gchar ch_arr[32],
                            gboolean covert_arr[32], gint au_in_vd_ch );

gboolean nf_live_change( NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
                            guint win_w, guint win_h, gchar ch_arr[32],
                            gboolean covert_arr[32], gint au_in_vd_ch);


//IPXP4 subdisp id : 0
gboolean nf_live_sub_change( NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
                                    guint win_w, guint win_h, gchar ch_arr[32],
                                    gboolean covert_arr[32], gint au_in_vd_ch, gint subdisp_id);

gboolean nf_live_stop( void );

gboolean nf_live_freeze( guint ch_mask );

gboolean nf_live_zoom_start( gint ch , int base_x, int base_y,
        int zoom_w, int zoom_h, int pip_x, int pip_y, int pip_w, int pip_h);

void nf_live_set_freeze(guint live_freeze);
gboolean nf_live_zoom_start_without_pip( gint ch , int base_x, int base_y, int zoom_w, int zoom_h, gboolean disp_init);

gboolean nf_live_zoom_channel_change(gint ch);

gboolean nf_live_zoom_move(gint xpos, gint ypos, gint zoom_w, gint zoom_h);

gboolean nf_live_zoom_stop( void );

gboolean nf_live_get_jpeg_snapshot(gint ch,
                                gint *width, gint *height,
                                gint *size,
                                void **out_buffer,
                                gint timeimg,
                                gint dst,
                                guint *timestamp
                               );

gboolean nf_live_stream_jpeg(gint ch,
                                gint *width, gint *height,
                                gint *size,
                                void **out_buffer,
                                gint timeimg,
                                gint dst,
                                guint *timestamp,
                                NF_JPEG_SIZE_E srcSize
                               );                               

gboolean nf_live_get_mosaic_jpeg(gint *width, gint *height, gint *size, void **out_buffer);

int nf_live_zoom_get_pos_sx();

int nf_live_zoom_get_pos_sy();

int nf_live_zoom_get_pos_ex();

int nf_live_zoom_get_pos_ey();

int nf_live_zoom_get_pos_dx();

int nf_live_zoom_get_pos_dy();
#if defined (USE_DEV_TPS2384) || defined (USE_DEV_PD69104B1) || defined (USE_DEV_IP804)
	void nf_live_poe_port_onoff(gint ch, gboolean is_on, gint *is_fail, gint act);
	guint nf_live_get_poe_port_status(void);
#endif

int nf_live_grpx_set_alpha(unsigned char alpha);
gboolean nf_live_check_video_processor();

int nf_live_pip_hide(void); //(int ch);
int nf_live_pip_show(void); //(int ch);

int nf_live_set_disp_ratio(int enable_chmask);

gboolean nf_api_live_poe_is_ok(int port, NF_UTIL_POE_INFO *info);

guint nf_live_get_audio_output_type(void);

gboolean nf_live_check_monitor_resolution(gint resolution, gboolean is_vga);

#define MAX_POINT_NUM	100
#define MAX_RPL_CNT		32
#define MAX_RPL_NAME_LENGTH	128
#define MAX_RPL_DESC_LENGTH	128
#define MAX_VIEW_NUM 4

typedef struct
{
    float pan;
    float tilt;
    float zoom;

    float roll;      //  Z �࿡ ���� Rotation
} NF_FISHEYE_PTZ;

typedef struct
{
    int max_view;
    NF_FISHEYE_PTZ view[MAX_VIEW_NUM];
} NF_FISHEYE_PTZ_PARAM;

typedef struct _NF_FISHEYE_PTZ_LIMIT
{
	float pan_min;
	float pan_max;
	float tilt_min;
	float tilt_max;
	float zoom_min;
	float zoom_max;
} NF_FISHEYE_PTZ_LIMIT;

typedef struct
{
    int max_point;
    int point_x[MAX_POINT_NUM];
    int point_y[MAX_POINT_NUM];
} NF_FISHEYE_POLYGON;

typedef enum
{
    NF_FISHEYE_MOUNT_WALL = 0,
    NF_FISHEYE_MOUNT_CEILING = 1,
    NF_FISHEYE_MOUNT_GROUND = 2
} NF_FISHEYE_MOUNT_TYPE;

typedef enum
{
    NF_FISHEYE_VIEW_SIGLE = 0,
    NF_FISHEYE_VIEW_QUAD = 1,
    NF_FISHEYE_VIEW_PANORAMA = 2,
    NF_FISHEYE_VIEW_CUSTOM = 4
} NF_FISHEYE_VIEW_TYPE;

typedef struct
{
//    gboolean enable;
	char rpl_name[MAX_RPL_NAME_LENGTH];	
    NF_FISHEYE_MOUNT_TYPE mnt_type;
    NF_FISHEYE_VIEW_TYPE view_type;
//    NF_FISHEYE_PTZ_PARAM ptz_param;
} NF_FISHEYE_VIDEO_PARAM;

typedef struct 
{
    int rpl_cnt;

    char rpl_name[MAX_RPL_CNT][MAX_RPL_NAME_LENGTH];
    char rpl_desc[MAX_RPL_CNT][MAX_RPL_DESC_LENGTH];
} NF_FISHEYE_RPL_LIST;

gboolean nf_live_fisheye_get_position(int ch, int view_num, NF_FISHEYE_PTZ *ptz);
gboolean nf_live_fisheye_set_position(int ch, int view_num, NF_FISHEYE_PTZ *ptz);

gboolean nf_live_fisheye_get_position_by_invideo_point(int ch, int in_x, int in_y, float *out_pan, float *out_tilt);
gboolean nf_live_fisheye_get_polygon(int ch, int view_num, NF_FISHEYE_POLYGON *polygon);

gboolean nf_live_fisheye_set_ptz_param(int ch, NF_FISHEYE_PTZ_PARAM *ptz_param);
gboolean nf_live_fisheye_get_ptz_param(int ch, NF_FISHEYE_PTZ_PARAM *ptz_param);

gboolean nf_live_fisheye_get_ptz_limit(int ch, NF_FISHEYE_PTZ_LIMIT *ptz_limit);

gboolean nf_live_fisheye_get_video_param(int ch, NF_FISHEYE_VIDEO_PARAM *video_param);
gboolean nf_live_fisheye_set_video_param(int ch, NF_FISHEYE_VIDEO_PARAM *video_param);

gboolean nf_live_fisheye_get_rpl_list(NF_FISHEYE_RPL_LIST *rpl_list);
gboolean nf_live_fisheye_set_lens_type(char *rpl);
gboolean nf_live_fisheye_create_snapshot_channel(int *channel_id, char *in_jpg);
gboolean nf_live_fisheye_get_snapshot_output(int channel_id,  char *out_jpg);
gboolean nf_live_fisheye_remove_snapshot_channel(int *channel_id);
gboolean nf_live_fisheye_is_support(int channel_id);
gboolean nf_live_fisheye_set_enable(int channel_id);
int nf_live_fisheye_get_enable(void);
gboolean nf_live_fisheye_block(int on);

gboolean nf_live_get_resol(gint ch, gint stream, guint64* capable, guint64* current, GError **error);

struct _VaImageCallbackInfo {
	uint16_t ch;
	uint32_t time;
	uint8_t timel;

	uint8_t *bmp;
	uint32_t bmp_width;
	uint32_t bmp_height;
	uint32_t bmp_size;

	uint8_t *jpeg;
	uint32_t jpeg_width;
	uint32_t jpeg_height;
	uint32_t jpeg_size;
};

typedef struct _VaImageCallbackInfo VaImageCallbackInfo;

#endif
