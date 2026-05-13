#include <string.h>
#include "nf_afx.h"
// #include "hifb.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_deeplearning.h"
#include "vw_sys_camera_deeplearning_property.h"
#include "vw_sys_camera_deeplearning_schedule.h"

#include "nf_notify.h"
#include "nf_va_object_detector.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"

#include "vw_menu.h"
#include "vw_vkeyboard.h"
#include "nfdal.h"
#include "scm.h"
#include "dvatext.h"


////////////////////////////////////////////////////////////
//
// private data types
//


#define STR_HELP_ROI "Configure an area for AI detection.\nThe size of the detection area determines the object size that can be detected.\nFor more information, click the Help button."

#define STR_HELP_EVENT "The AI detection event is stopped while setting AI detection."
#define STR_HELP_INTERVAL "The analysis interval--the amount of time that passes before new objects can be detected\n--increases as more channels are enabled for AI detection\n(E.g., 2-channels has twice the interval as 1-channel)."

#define STR_DISABLE_FISHEYE "You can not use the features such as the AI detection and the Fisheye Dewarping at the same time.\nThe configured Fisheye Dewarping will be automatically relieved in order to activate the feature of the AI detection.\nDo you want to continue?"

#define STR_OVER_ACT_CHANNEL "The AI detection function supports up to %d cameras only.\nCurrently activated channel : %s"

#define MAX_OBJ_TRACK_CNT       16
#define OBJ_PT_CNT              4

#define MKB_TAB_SUBGROUP_TITLE_NAME     "mkb_tab_subgroup_title_%d"

#define TRACK_GROUP1_LIST       "person"
#define TRACK_GROUP2_LIST       "aeroplane,bicycle,boat,bus,car,motorbike,train,bike"
#define TRACK_GROUP3_LIST       "animal,bird,cat,cow,dog,horse"
#define TRACK_OTHERS_LIST       " "

#define TRACK_GROUP1_COLOR      (UX_COLOR_FF0000)
#define TRACK_GROUP2_COLOR      (UX_COLOR_FFFF00)
#define TRACK_GROUP3_COLOR      (UX_COLOR_00FF00)
#define TRACK_OTHERS_COLOR      (UX_COLOR_0000FF)

#define VIDEO_SIZE_W            (16*58)
#define VIDEO_SIZE_H            (9*58)

#define ROI_MOVE_MARGIN_W       (VIDEO_SIZE_W/20)
#define ROI_MOVE_MARGIN_H       (VIDEO_SIZE_H/20)

enum {   
	DETECT_TYPE_INTRUSION = 0,
	DETECT_TYPE_ILLEGAL_PARKING,
	DETECT_TYPE_LPR,
	DETECT_TYPE_MAX,
};

typedef struct _OBJECT_INFO_T {
    gchar name[128];
    gint confidence;
    GdkPoint pt[OBJ_PT_CNT];
} OBJECT_INFO_T;

typedef struct _DVA_DETECT_T {
    gint ch;
    gint img_w;
    gint img_h;
    gint img_size;
    guchar *frame;
    gint obj_count;
    OBJECT_INFO_T obj_info[MAX_OBJ_TRACK_CNT];
} DVA_DETECT_T;

typedef struct _DVA_CROP_T {
    gint sx;
    gint sy;
    gint dx;
    gint dy;
    gint mx;
    gint my;    
} DVA_CROP_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static gint g_cur_channel = -1;

static DVAPropData g_prop_data[GUI_CHANNEL_CNT];
static DVAPropData g_org_prop_data[GUI_CHANNEL_CNT];

static DVA_DETECT_T g_dva_detect = {0, };

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_fixed;
static NFOBJECT *g_dtype_fixed[DETECT_TYPE_MAX];

static NFOBJECT *g_detect_type_obj;
static NFOBJECT *g_ignore_interval_obj;

static NFOBJECT *g_idz_human_all_obj;
static NFOBJECT *g_idz_vehicle_all_obj;
static NFOBJECT *g_idz_vehicle_car_obj;
//static NFOBJECT *g_idz_vehicle_bus_obj;
static NFOBJECT *g_idz_vehicle_bike_obj;
//static NFOBJECT *g_idz_animal_all_obj;

static NFOBJECT *g_idz_cntr_active_obj;
static NFOBJECT *g_idz_cntr_color_obj;
static NFOBJECT *g_idz_cntr_noti_obj;
static NFOBJECT *g_idz_cntr_value_obj;
static NFOBJECT *g_idz_cntr_reset_obj;

static NFOBJECT *g_idz_static_filter_onoff_obj;
static NFOBJECT *g_idz_static_filter_sense_obj;

static NFOBJECT *g_idz_confidence_threshold_obj;

static NFOBJECT *g_ipz_all_obj;
static NFOBJECT *g_ipz_car_obj;
//static NFOBJECT *g_ipz_bus_obj;
static NFOBJECT *g_ipz_bike_obj;
static NFOBJECT *g_ipz_dwell_obj;

static guint g_support_cam_mask = 0;

static guint g_tmr_dispvideo = 0;
static DVA_CROP_T g_crop_box;
static DVA_CROP_T g_org_crop_box;
static guint g_press_corner_mask = 0;

static gint g_press_x = 0;
static gint g_press_y = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _disable_alpha()
{
    int fd;
    //ksi_test
    // HIFB_ALPHA_S hi_alpha;

    // fd = open("/dev/fb0", O_RDWR);
    // g_message("%s, %d, fb open", __FUNCTION__, __LINE__);
    // if(fd == -1) return -1;

    // memset(&hi_alpha, 0x00, sizeof(HIFB_ALPHA_S));

    // if (ioctl(fd, FBIOGET_ALPHA_HIFB, &hi_alpha) < 0)
    // {
    //     g_message("%s, %d, FBIOGET_ALPHA_HIFB fail", __FUNCTION__, __LINE__);
    //     close(fd);
    //     return -1;
    // }

    // g_message("%s, %d, alpha_enable:%d", __FUNCTION__, __LINE__, hi_alpha.bAlphaEnable);

    // if (hi_alpha.bAlphaEnable == TRUE)
    // {
    //     hi_alpha.bAlphaEnable = FALSE;

    //     if (ioctl(fd, FBIOPUT_ALPHA_HIFB, &hi_alpha) < 0)
    //     {
    //         g_message("%s, %d, FBIOPUT_ALPHA_HIFB fail", __FUNCTION__, __LINE__);
    //         close(fd);
    //         return -1;
    //     }
    // }

    // g_message(">>>>>>>>>>>> gui alpha disable");
    // close(fd);
    return 0;
}

static gint _enable_alpha()
{
    int fd;
    // ksi_test
    // HIFB_ALPHA_S hi_alpha;

    // fd = open("/dev/fb0", O_RDWR);
    // g_message("%s, %d, fb open", __FUNCTION__, __LINE__);
    // if(fd == -1) return -1;

    // memset(&hi_alpha, 0x00, sizeof(HIFB_ALPHA_S)); 

    // if (ioctl(fd, FBIOGET_ALPHA_HIFB, &hi_alpha) < 0)
    // {
    //     g_message("%s, %d, FBIOGET_ALPHA_HIFB fail", __FUNCTION__, __LINE__);
    //     close(fd);
    //     return -1;
    // }

    // g_message("%s, %d, alpha_enable:%d", __FUNCTION__, __LINE__, hi_alpha.bAlphaEnable);

    // if (hi_alpha.bAlphaEnable == FALSE) 
    // {
    //     hi_alpha.bAlphaEnable = TRUE;

    //     if (ioctl(fd, FBIOPUT_ALPHA_HIFB, &hi_alpha) < 0)
    //     {
    //         g_message("%s, %d, FBIOPUT_ALPHA_HIFB fail", __FUNCTION__, __LINE__);
    //         close(fd);
    //         return -1;
    //     }
    // }

    // g_message(">>>>>>>>>>>> gui alpha enable");
    // close(fd);    
    return 0;
}


static gint _is_enable_fisheye_dewarping()
{
    CamItxFisheyeData data;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(CamItxFisheyeData));
	    DAL_get_camera_itx_fisheye_data(&data, i);
        if (data.act) return 1;
    }

    return 0;
}

static gint _disable_fisheye_dewarping()
{
    CamItxFisheyeData data;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(CamItxFisheyeData));
	    DAL_get_camera_itx_fisheye_data(&data, i);
        if (data.act == 1) 
        {
            data.act = 0;
            DAL_set_camera_itx_fisheye_data(data, i);
        }
    }
    syscam_set_changeflag(1);    
    return 0;
}

static gint _trans_x_rpoint(gint rcnvs, gint vpoint, gint vcnvs)
{
    gint rpoint;    
    
    rpoint = (gint)((float)(vpoint*rcnvs/vcnvs));   
    return rpoint;
}

static gint _trans_y_rpoint(gint rcnvs, gint vpoint, gint vcnvs)
{
    gint rpoint;    

    rpoint = (gint)((float)(vpoint*rcnvs/vcnvs));   
    return rpoint;
}

static gboolean _timeout_draw_display_video_box(gpointer data)
{ 
    if (g_cur_channel == -1) return TRUE;
    if (!nfui_nfobject_is_shown(g_video_fixed)) return TRUE;

    nfui_signal_emit(g_video_fixed, GDK_EXPOSE, FALSE);
    return TRUE;
}

static guint _get_press_corner_mask(gint pos_x, gint pos_y) 
{
    GdkRectangle rect;
    GdkRegion *region;  
    gint gap_x, gap_y;
    gint margin = 8;

    guint mask = 0;

    nfui_nfobject_get_offset((NFOBJECT*)g_video_fixed, &gap_x, &gap_y);

    rect.x = gap_x+g_crop_box.sx-margin;
    rect.y = gap_y+g_crop_box.sy-margin;
    rect.width = 20+margin*2;
    rect.height = 20+5;
    region = gdk_region_rectangle(&rect);
    if (gdk_region_point_in(region, pos_x, pos_y)) mask |= 1 << 0;

    gdk_region_destroy(region);

    rect.x = gap_x+g_crop_box.dx-20-5;
    rect.y = gap_y+g_crop_box.sy-5;
    rect.width = 20+5;
    rect.height = 20+5;
    region = gdk_region_rectangle(&rect);
    if (gdk_region_point_in(region, pos_x, pos_y)) mask |= 1 << 1;

    gdk_region_destroy(region);

    rect.x = gap_x+g_crop_box.sx-5;
    rect.y = gap_y+g_crop_box.dy-20-5;
    rect.width = 20+5;
    rect.height = 20+5;
    region = gdk_region_rectangle(&rect);
    if (gdk_region_point_in(region, pos_x, pos_y)) mask |= 1 << 2;

    gdk_region_destroy(region);

    rect.x = gap_x+g_crop_box.dx-20-5;
    rect.y = gap_y+g_crop_box.dy-20-5;
    rect.width = 20+5;
    rect.height = 20+5;
    region = gdk_region_rectangle(&rect);
    if (gdk_region_point_in(region, pos_x, pos_y)) mask |= 1 << 3;

    gdk_region_destroy(region);

    rect.x = gap_x+g_crop_box.sx+20;
    rect.y = gap_y+g_crop_box.sy+20;
    rect.width = g_crop_box.dx-g_crop_box.sx-40;
    rect.height = g_crop_box.dy-g_crop_box.sy-40;
    region = gdk_region_rectangle(&rect);
    if (gdk_region_point_in(region, pos_x, pos_y)) mask = 0xf;            

    gdk_region_destroy(region);

    return mask;
}

static gint _get_dva_crop_box(guint mx, guint my, DVA_CROP_T *crop_box)
{
    DVA_CROP_T tmp_box;
    gint move_x = mx-g_press_x;
    gint move_y = my-g_press_y;
    gint rev_x, rev_y;

    if (g_press_corner_mask == 0xf) 
    {
        if (move_x < 0) {
            tmp_box.sx = MAX(g_crop_box.sx+move_x, 0);
            tmp_box.dx = g_crop_box.dx+tmp_box.sx-g_crop_box.sx;
        } else {
            tmp_box.dx = MIN(g_crop_box.dx+move_x, g_video_fixed->width);
            tmp_box.sx = g_crop_box.sx+tmp_box.dx-g_crop_box.dx;
        }

        if (move_y < 0) {
            tmp_box.sy = MAX(g_crop_box.sy+move_y, 0);
            tmp_box.dy = g_crop_box.dy+tmp_box.sy-g_crop_box.sy;
        } else {
            tmp_box.dy = MIN(g_crop_box.dy+move_y, g_video_fixed->height);
            tmp_box.sy = g_crop_box.sy+tmp_box.dy-g_crop_box.dy;
        }
    }
    else if (g_press_corner_mask & (1 << 0)) 
    {
        tmp_box.sx = g_crop_box.sx+move_x;
        tmp_box.sy = g_crop_box.sy+move_y;
        tmp_box.dx = g_crop_box.dx;
        tmp_box.dy = g_crop_box.dy;

        tmp_box.sx = MAX(tmp_box.sx, 0);
        tmp_box.sx = MIN(tmp_box.sx, tmp_box.dx-(gint)(g_video_fixed->width*0.2));
        tmp_box.sy = MAX(tmp_box.sy, 0);
        tmp_box.sy = MIN(tmp_box.sy, tmp_box.dy-(gint)(g_video_fixed->height*0.2));        
    }
    else if (g_press_corner_mask & (1 << 1)) {
        tmp_box.sx = g_crop_box.sx;
        tmp_box.sy = g_crop_box.sy+move_y;
        tmp_box.dx = g_crop_box.dx+move_x;
        tmp_box.dy = g_crop_box.dy;

        tmp_box.sy = MAX(tmp_box.sy, 0);
        tmp_box.sy = MIN(tmp_box.sy, tmp_box.dy-(gint)(g_video_fixed->height*0.2));
        tmp_box.dx = MIN(tmp_box.dx, g_video_fixed->width);
        tmp_box.dx = MAX(tmp_box.dx, tmp_box.sx+(gint)(g_video_fixed->width*0.2));
    }
    else if (g_press_corner_mask & (1 << 2)) {
        tmp_box.sx = g_crop_box.sx+move_x;
        tmp_box.sy = g_crop_box.sy;
        tmp_box.dx = g_crop_box.dx;
        tmp_box.dy = g_crop_box.dy+move_y;

        tmp_box.sx = MAX(tmp_box.sx, 0);
        tmp_box.sx = MIN(tmp_box.sx, tmp_box.dx-(gint)(g_video_fixed->width*0.2));
        tmp_box.dy = MIN(tmp_box.dy, g_video_fixed->height);
        tmp_box.dy = MAX(tmp_box.dy, tmp_box.sy+(gint)(g_video_fixed->height*0.2));
    }
    else if (g_press_corner_mask & (1 << 3)) {
        tmp_box.sx = g_crop_box.sx;
        tmp_box.sy = g_crop_box.sy;
        tmp_box.dx = g_crop_box.dx+move_x;
           tmp_box.dy = g_crop_box.dy+move_y;

        tmp_box.dx = MIN(tmp_box.dx, g_video_fixed->width);
        tmp_box.dx = MAX(tmp_box.dx, tmp_box.sx+(gint)(g_video_fixed->width*0.2));
        tmp_box.dy = MIN(tmp_box.dy, g_video_fixed->height);
        tmp_box.dy = MAX(tmp_box.dy, tmp_box.sy+(gint)(g_video_fixed->height*0.2));
    }        

    tmp_box.mx = mx;
    tmp_box.my = my;

    memcpy(crop_box, &tmp_box, sizeof(DVA_CROP_T));
    return 1;
}

static gint _get_active_string(gchar *str, gint len)
{
    gchar tmpBuf[512];

    memset(tmpBuf, 0x00, sizeof (tmpBuf));

    if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_detect_type_obj) == 0)
    {
        if (nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_human_all_obj))) strcat(tmpBuf, "person");
        if (nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj))) strcat(tmpBuf, "car");
        //if (nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bus_obj))) strcat(tmpBuf, "bus"); 
        if (nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj))) strcat(tmpBuf, "bike");
        //if (nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_animal_all_obj))) strcat(tmpBuf, "animal");
    }
    else if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_detect_type_obj) == 1)
    {
        if (nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_car_obj))) strcat(tmpBuf, "car");
        //if (nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bus_obj))) strcat(tmpBuf, "bus");
        if (nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bike_obj))) strcat(tmpBuf, "bike");
    }
    else if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_detect_type_obj) == 2)
    {

    }

#if 0   // for demo
    strcat(tmpBuf, "bottle");
    strcat(tmpBuf, "chair");
    strcat(tmpBuf, "diningtable");
    strcat(tmpBuf, "pottedplant");
    strcat(tmpBuf, "sofa");
    strcat(tmpBuf, "tvmonitor");
#endif    

    g_snprintf(str, len, "%s", tmpBuf);
    return 0;
}

static gint _draw_dva_detect_frame(GdkPixmap *pm_cnvs, NFOBJECT *obj)
{
	GdkGC *gc;

    GInputStream *stream;
    GdkPixbuf *dst_pixbuf = NULL;
    GdkPixbuf *ctrn_pixbuf = NULL;
    GdkPixbuf *conr_pixbuf = NULL;

    if (!g_dva_detect.frame) return 0;

    gc = gdk_gc_new(pm_cnvs);

    stream = g_memory_input_stream_new_from_data(g_dva_detect.frame, g_dva_detect.img_size, NULL);
    //dst_pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
    dst_pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, VIDEO_SIZE_W, VIDEO_SIZE_H, NULL, NULL, NULL);

    gdk_draw_pixbuf(pm_cnvs, gc, dst_pixbuf, 0, 0, 0, 0, obj->width, obj->height, GDK_RGB_DITHER_NONE, 0, 0);

    ctrn_pixbuf = nfui_get_image_from_file("deeplearning_curtain2.png", NULL); 
    gdk_draw_pixbuf(pm_cnvs, gc, ctrn_pixbuf, 0, 0, 0, 0, obj->width, obj->height, GDK_RGB_DITHER_NONE, 0, 0);

    gdk_draw_pixbuf(pm_cnvs, gc, dst_pixbuf, g_crop_box.sx, g_crop_box.sy, g_crop_box.sx, g_crop_box.sy, g_crop_box.dx-g_crop_box.sx, g_crop_box.dy-g_crop_box.sy, GDK_RGB_DITHER_NONE, 0, 0);

    conr_pixbuf = nfui_get_image_from_file("ai_detection_crop_1.png", NULL);
    gdk_draw_pixbuf(pm_cnvs, gc, conr_pixbuf, 0, 0, g_crop_box.sx, g_crop_box.sy, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

    conr_pixbuf = nfui_get_image_from_file("ai_detection_crop_2.png", NULL);
    gdk_draw_pixbuf(pm_cnvs, gc, conr_pixbuf, 0, 0, g_crop_box.dx-gdk_pixbuf_get_width(conr_pixbuf), g_crop_box.sy, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

    conr_pixbuf = nfui_get_image_from_file("ai_detection_crop_3.png", NULL);
    gdk_draw_pixbuf(pm_cnvs, gc, conr_pixbuf, 0, 0, g_crop_box.sx, g_crop_box.dy-gdk_pixbuf_get_height(conr_pixbuf), -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

    conr_pixbuf = nfui_get_image_from_file("ai_detection_crop_4.png", NULL);
    gdk_draw_pixbuf(pm_cnvs, gc, conr_pixbuf, 0, 0, g_crop_box.dx-gdk_pixbuf_get_width(conr_pixbuf), g_crop_box.dy-gdk_pixbuf_get_height(conr_pixbuf), -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

    gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF7E00)));
    gdk_draw_line(pm_cnvs, gc, g_crop_box.sx, g_crop_box.sy, g_crop_box.dx, g_crop_box.sy);
    gdk_draw_line(pm_cnvs, gc, g_crop_box.sx, g_crop_box.sy, g_crop_box.sx, g_crop_box.dy);
    gdk_draw_line(pm_cnvs, gc, g_crop_box.dx, g_crop_box.sy, g_crop_box.dx, g_crop_box.dy);
    gdk_draw_line(pm_cnvs, gc, g_crop_box.sx, g_crop_box.dy, g_crop_box.dx, g_crop_box.dy);

    g_object_unref(dst_pixbuf);
    g_object_unref(stream);
    g_object_unref(gc);
    return 0;
}

static gint _draw_dva_detect_info(GdkPixmap *pm_cnvs, NFOBJECT *obj)
{
	GdkGC *gc;

    gchar active_str[512];

    gchar strBuf[128];
    GdkPoint draw_pt[OBJ_PT_CNT] = {0, };
    GdkPoint text_pt;
    gint text_w, text_h;
    gint i, j;

    gint color_idx;

    //if (!nfui_combobox_get_cur_index(NF_COMBOBOX(g_act_combo))) return -1;

    memset(active_str, 0x00, sizeof(active_str));
    _get_active_string(active_str, sizeof(active_str));

    gc = gdk_gc_new(pm_cnvs);   

    for (i = 0; i < g_dva_detect.obj_count; i++)
    {
        if (!strstr(active_str, g_dva_detect.obj_info[i].name)) continue;

        for (j = 0; j < OBJ_PT_CNT; j++)
        {
            draw_pt[j].x = g_dva_detect.obj_info[i].pt[j].x;
            draw_pt[j].y = g_dva_detect.obj_info[i].pt[j].y; 
        }               

        if (strstr(TRACK_GROUP1_LIST, g_dva_detect.obj_info[i].name)) color_idx = TRACK_GROUP1_COLOR;
        else if (strstr(TRACK_GROUP2_LIST, g_dva_detect.obj_info[i].name)) color_idx = TRACK_GROUP2_COLOR;
        else if (strstr(TRACK_GROUP3_LIST, g_dva_detect.obj_info[i].name)) color_idx = TRACK_GROUP3_COLOR;
        else color_idx = TRACK_OTHERS_COLOR;

        gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(color_idx)));
        gdk_draw_polygon(pm_cnvs, gc, 0, draw_pt, OBJ_PT_CNT); 

        memset(strBuf, 0x00, sizeof(strBuf));
        g_snprintf(strBuf, 128, " %s (%d%%) ", g_dva_detect.obj_info[i].name, g_dva_detect.obj_info[i].confidence);

        text_w = nfutil_string_width(0, pm_cnvs, nffont_get_pango_font(NFFONT_MINI_NORMAL_4), strBuf, NORMAL_SPACING);
        text_h = nfutil_string_height(pm_cnvs, nffont_get_pango_font(NFFONT_MINI_NORMAL_4), strBuf, NORMAL_SPACING);

        text_pt.x = draw_pt[0].x;
        text_pt.y = draw_pt[0].y-text_h;
        text_pt.x = MIN(text_pt.x, obj->width-text_w);
        text_pt.x = MAX(2, text_pt.x);
        text_pt.y = MIN(text_pt.y, obj->height-text_h);
        text_pt.y = MAX(2, text_pt.y);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(color_idx)));
        nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(COLOR_PRG_IDX(color_idx)), pm_cnvs, gc, 
            strBuf, text_pt.x, text_pt.y, text_w, text_h, 
            nffont_get_pango_font(NFFONT_MINI_NORMAL_4), &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_202020)), NFALIGN_LEFT, 0);
    }

    g_object_unref(gc);
    return 0;
}

static gint _draw_deeplearning_object_detect(GdkPixmap *pm_cnvs, NFOBJECT *obj)
{
    NF_NOTIFY_INFO vloss_info;

    memset(&vloss_info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_vloss_data(&vloss_info);

    if (g_cur_channel == -1) return -1;
    if (g_dva_detect.ch != g_cur_channel) return -1;
    if (!g_dva_detect.frame) return -1;
    if (vloss_info.d.params[0] & (1 << g_cur_channel)) return -1;

    _draw_dva_detect_frame(pm_cnvs, obj);
    _draw_dva_detect_info(pm_cnvs, obj);
    return 0;
}

static gint _set_deeplearning_object_detect(struct objects *objs)
{
    gchar uxitem[64];
    gint i;

    float top_x, top_y;
    float bottom_x, bottom_y;    

    //g_message("%s, %d, ch:%d, objs_num:%d", __FUNCTION__, __LINE__, objs->ch, objs->obj_num);
    g_dva_detect.ch = objs->ch;
    g_dva_detect.obj_count = objs->obj_num;

    for (i = 0; i < objs->obj_num; i++)
    {
        if (i >= MAX_OBJ_TRACK_CNT) break;

        memset(uxitem, 0x00, sizeof(uxitem));        
        dvatext_translate_to_uxitem(objs->obj[i].name, uxitem, sizeof(uxitem));
        if (strlen(uxitem)) g_sprintf(g_dva_detect.obj_info[i].name, "%s", uxitem);
        else g_sprintf(g_dva_detect.obj_info[i].name, "%s", objs->obj[i].name);
        
        g_dva_detect.obj_info[i].confidence = (gint)(objs->obj[i].confidence * 100);

        top_x = MAX(objs->obj[i].bbx.top.x, 0);
        top_y = MAX(objs->obj[i].bbx.top.y, 0);
        bottom_x = MIN(objs->obj[i].bbx.bottom.x, 1);
        bottom_y = MIN(objs->obj[i].bbx.bottom.y, 1);

        g_dva_detect.obj_info[i].pt[0].x = (gint)(top_x*g_video_fixed->width);
        g_dva_detect.obj_info[i].pt[0].y = (gint)(top_y*g_video_fixed->height);
        g_dva_detect.obj_info[i].pt[1].x = (gint)(bottom_x*g_video_fixed->width); 
        g_dva_detect.obj_info[i].pt[1].y = (gint)(top_y*g_video_fixed->height);        
        g_dva_detect.obj_info[i].pt[2].x = (gint)(bottom_x*g_video_fixed->width);
        g_dva_detect.obj_info[i].pt[2].y = (gint)(bottom_y*g_video_fixed->height);           
        g_dva_detect.obj_info[i].pt[3].x = (gint)(top_x*g_video_fixed->width);
        g_dva_detect.obj_info[i].pt[3].y = (gint)(bottom_y*g_video_fixed->height);
    }

    g_dva_detect.img_w = objs->img.width;
    g_dva_detect.img_h = objs->img.height;
    g_dva_detect.img_size = objs->img.size;

    if (g_dva_detect.frame) 
        ifree(g_dva_detect.frame);
    
    g_dva_detect.frame = imalloc(sizeof(guchar)*objs->img.size); 
    memset(g_dva_detect.frame, 0x00, sizeof(guchar)*objs->img.size);
    memcpy(g_dva_detect.frame, objs->img.data, sizeof(guchar)*objs->img.size);   

    return 0;
}

static gboolean _is_check_val(gchar *string1, gchar *string2)
{
    gchar strBuf[64];
    gchar uxstring_db[512];

    memset(uxstring_db, 0x00, sizeof(uxstring_db));
    dvatext_translate_to_uxstring_db(string1, uxstring_db, sizeof(uxstring_db));
    
    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s:1", string2);

    if (strstr(uxstring_db, strBuf)) return TRUE; 

    return FALSE;
}

static gint _prvSetDataToObjects_idz(gint ch, DVAPropData *data, gint expose)
{
    gboolean check_val;
    gchar strBuf[32];

    check_val = _is_check_val(data->idz.human_item_list, "person");
    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_human_all_obj), check_val);

    if (_is_check_val(data->idz.vehicle_item_list, "bike") &&
        //_is_check_val(data->idz.vehicle_item_list, "bus") &&
        _is_check_val(data->idz.vehicle_item_list, "car")) {
        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_all_obj), TRUE);
    }
    else {
        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_all_obj), FALSE);
    }       

    check_val = _is_check_val(data->idz.vehicle_item_list, "car");
    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_car_obj), check_val);   

    //check_val = _is_check_val(data->idz.vehicle_item_list, "bus");
    //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_bus_obj), check_val);    

    check_val = _is_check_val(data->idz.vehicle_item_list, "bike");
    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_bike_obj), check_val);

    //check_val = _is_check_val(data->idz.animal_item_list, "animal");
    //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_animal_all_obj), check_val);    

    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_static_filter_onoff_obj), data->idz.en_static_filter);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_idz_static_filter_sense_obj, data->idz.static_filter_sense);

    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_cntr_active_obj), data->idz.cntr.active);

    if (data->idz.cntr.display_color == 0xffffff) nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_cntr_color_obj), 1);
    else nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_cntr_color_obj), 0);

    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_cntr_noti_obj), data->idz.cntr.notification);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", data->idz.cntr.e_value);
    nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)g_idz_cntr_value_obj, strBuf);

    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_cntr_reset_obj), data->idz.cntr.reset);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", data->idz.confidence_threshold);
    nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)g_idz_confidence_threshold_obj, strBuf);

    if ((data->active) && (g_support_cam_mask & (1 << ch)))
    {
        nfui_nfobject_enable(g_idz_human_all_obj);
        nfui_nfobject_enable(g_idz_vehicle_all_obj);
        nfui_nfobject_enable(g_idz_vehicle_car_obj);
        //nfui_nfobject_enable(g_idz_vehicle_bus_obj);
        nfui_nfobject_enable(g_idz_vehicle_bike_obj);
        //nfui_nfobject_enable(g_idz_animal_all_obj);

        nfui_nfobject_enable(g_idz_static_filter_onoff_obj);
        nfui_nfobject_disable(g_idz_static_filter_sense_obj);

        if (data->idz.en_static_filter)
        {
            nfui_nfobject_enable(g_idz_static_filter_sense_obj);
        }

        nfui_nfobject_enable(g_idz_cntr_active_obj);    

        nfui_nfobject_disable(g_idz_cntr_color_obj);
        nfui_nfobject_disable(g_idz_cntr_noti_obj);
        nfui_nfobject_disable(g_idz_cntr_value_obj);
        nfui_nfobject_disable(g_idz_cntr_reset_obj);  

        if (data->idz.cntr.active)
        {
            nfui_nfobject_enable(g_idz_cntr_color_obj);
            nfui_nfobject_enable(g_idz_cntr_noti_obj); 
        }

        if ((data->idz.cntr.active == 1) && (data->idz.cntr.notification == 1))
        {
            nfui_nfobject_enable(g_idz_cntr_value_obj);
            nfui_nfobject_enable(g_idz_cntr_reset_obj);
        }
    }
    else 
    {
        nfui_nfobject_disable(g_idz_human_all_obj);
        nfui_nfobject_disable(g_idz_vehicle_all_obj);
        nfui_nfobject_disable(g_idz_vehicle_car_obj);
        //nfui_nfobject_disable(g_idz_vehicle_bus_obj);
        nfui_nfobject_disable(g_idz_vehicle_bike_obj);
        //nfui_nfobject_disable(g_idz_animal_all_obj);

        nfui_nfobject_disable(g_idz_static_filter_onoff_obj);
        nfui_nfobject_disable(g_idz_static_filter_sense_obj);

        nfui_nfobject_disable(g_idz_cntr_active_obj);
        nfui_nfobject_disable(g_idz_cntr_color_obj);
        nfui_nfobject_disable(g_idz_cntr_noti_obj);
        nfui_nfobject_disable(g_idz_cntr_value_obj);
        nfui_nfobject_disable(g_idz_cntr_reset_obj);        
    }

    return 0;  
}

static gint _prvSetDataToObjects_ipz(gint ch, DVAPropData *data, gint expose)
{
    gchar strBuf[64];
    gboolean check_val;

    if (_is_check_val(data->ipz.item_list, "bike") &&
        //_is_check_val(data->ipz.item_list, "bus") &&
        _is_check_val(data->ipz.item_list, "car")) {
        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_all_obj), TRUE);
    }
    else {
        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_all_obj), FALSE);
    }    

    check_val = _is_check_val(data->ipz.item_list, "car");
    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_car_obj), check_val);   

    //check_val = _is_check_val(data->ipz.item_list, "bus");
    //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_bus_obj), check_val);    

    check_val = _is_check_val(data->ipz.item_list, "bike");
    nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_bike_obj), check_val);

    memset(strBuf, 0x00, sizeof(strBuf));
    if (data->ipz.dwell == 0) {
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_ipz_dwell_obj, 0);
    }
    else {
        g_sprintf(strBuf, "%d MIN", data->ipz.dwell);
        nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_ipz_dwell_obj, strBuf);
    }

    if ((data->active) && (g_support_cam_mask & (1 << ch)))
    {
        nfui_nfobject_enable(g_ipz_all_obj);
        nfui_nfobject_enable(g_ipz_car_obj);
        //nfui_nfobject_enable(g_ipz_bus_obj);
        nfui_nfobject_enable(g_ipz_bike_obj);
        nfui_nfobject_enable(g_ipz_dwell_obj);
    }
    else 
    {
        nfui_nfobject_disable(g_ipz_all_obj);
        nfui_nfobject_disable(g_ipz_car_obj);
        //nfui_nfobject_disable(g_ipz_bus_obj);
        nfui_nfobject_disable(g_ipz_bike_obj);
        nfui_nfobject_disable(g_ipz_dwell_obj);
    }  

    return 0;
}

static gint _prvSetDataToObjects_lpr(gint ch, DVAPropData *data, gint expose)
{
    
    return 0;
}

static gint _prvSetDataToObjects(gint ch, DVAPropData *data, gint expose)
{
    gchar strBuf[64];
    gint i;

/*
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_act_combo, data->active);
    if (g_support_cam_mask & (1 << ch)) nfui_nfobject_enable(g_act_combo);
    else nfui_nfobject_disable(g_act_combo);
*/

    if (data->idz.active) nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_detect_type_obj, 0);
    else if (data->ipz.active) nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_detect_type_obj, 1);
    else if (data->lpr.active) nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_detect_type_obj, 2);

    if ((data->active) && (g_support_cam_mask & (1 << ch))) nfui_nfobject_enable(g_detect_type_obj);
    else nfui_nfobject_disable(g_detect_type_obj);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d SEC", data->ignore_interval);
    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_ignore_interval_obj, strBuf);

    if ((data->active) && (g_support_cam_mask & (1 << ch))) nfui_nfobject_enable(g_ignore_interval_obj);
    else nfui_nfobject_disable(g_ignore_interval_obj);

    nfui_nfobject_hide(g_dtype_fixed[DETECT_TYPE_INTRUSION]);
    nfui_nfobject_hide(g_dtype_fixed[DETECT_TYPE_ILLEGAL_PARKING]);
    nfui_nfobject_hide(g_dtype_fixed[DETECT_TYPE_LPR]);

    if (data->idz.active) nfui_nfobject_show(g_dtype_fixed[DETECT_TYPE_INTRUSION]); 
    else if (data->ipz.active) nfui_nfobject_show(g_dtype_fixed[DETECT_TYPE_ILLEGAL_PARKING]); 
    else if (data->lpr.active) nfui_nfobject_show(g_dtype_fixed[DETECT_TYPE_LPR]);   

    _prvSetDataToObjects_idz(ch, data, expose);
    _prvSetDataToObjects_ipz(ch, data, expose);
    _prvSetDataToObjects_lpr(ch, data, expose);

    g_crop_box.sx = VIDEO_SIZE_W*data->roi_sx;
    g_crop_box.sy = VIDEO_SIZE_H*data->roi_sy;
    g_crop_box.dx = VIDEO_SIZE_W*data->roi_ex;
    g_crop_box.dy = VIDEO_SIZE_H*data->roi_ey;

    memcpy(&g_org_crop_box, &g_crop_box, sizeof(DVA_CROP_T));

    //nfui_signal_emit(g_idz_confidence_threshold_obj, GDK_EXPOSE, TRUE);
    if (expose)
    {
        //nfui_signal_emit(g_act_combo, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_detect_type_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ignore_interval_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_video_fixed, GDK_EXPOSE, TRUE);

        for (i = 0; i < DETECT_TYPE_MAX; i++)
        {
            nfui_signal_emit(g_dtype_fixed[i], GDK_EXPOSE, TRUE);
        }
    }

    return 0;
}

static gint _prvLoadDataFromObjects(gint ch)
{
    gchar *strcombo;
    gchar item_list[512];

    //g_prop_data[ch].active = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_act_combo);

    g_prop_data[ch].idz.active = 0;
    g_prop_data[ch].ipz.active = 0;
    g_prop_data[ch].lpr.active = 0;
    if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_detect_type_obj) == 0) g_prop_data[ch].idz.active = 1;
    else if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_detect_type_obj) == 1) g_prop_data[ch].ipz.active = 1;
    else if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_detect_type_obj) == 2) g_prop_data[ch].lpr.active = 1;

    strcombo = nfui_combobox_get_value((NFCOMBOBOX*)g_ignore_interval_obj);
    sscanf(strcombo, "%u SEC", &g_prop_data[ch].ignore_interval);

    memset(item_list, 0x00, sizeof(item_list));
    memset(g_prop_data[ch].idz.human_item_list, 0x00, sizeof(g_prop_data[ch].idz.human_item_list));
    g_sprintf(item_list, "[person:%d]", nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_human_all_obj)));
    dvatext_translate_to_engstring_db(item_list, g_prop_data[ch].idz.human_item_list, sizeof(g_prop_data[ch].idz.human_item_list));

    memset(item_list, 0x00, sizeof(item_list));
    memset(g_prop_data[ch].idz.vehicle_item_list, 0x00, sizeof(g_prop_data[ch].idz.vehicle_item_list));

#if 1
    g_sprintf(item_list, "[bike:%d],[bus:%d],[car:%d]", 
        nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj)),
        0,
        nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj)));
#else        
    g_sprintf(item_list, "[bike:%d],[bus:%d],[car:%d]", 
        nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj)),
        nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bus_obj)),
        nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj)));
#endif
    dvatext_translate_to_engstring_db(item_list, g_prop_data[ch].idz.vehicle_item_list, sizeof(g_prop_data[ch].idz.vehicle_item_list));

#if 0
    memset(item_list, 0x00, sizeof(item_list));
    memset(g_prop_data[ch].idz.animal_item_list, 0x00, sizeof(g_prop_data[ch].idz.animal_item_list));
    g_sprintf(item_list, "[animal:%d]", nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_animal_all_obj)));
    dvatext_translate_to_engstring_db(item_list, g_prop_data[ch].idz.animal_item_list, sizeof(g_prop_data[ch].idz.animal_item_list));
#endif

    g_prop_data[ch].idz.en_static_filter = nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_static_filter_onoff_obj));
    g_prop_data[ch].idz.static_filter_sense = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_idz_static_filter_sense_obj);

    g_prop_data[ch].idz.cntr.active = nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_cntr_active_obj));
    if (nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_cntr_color_obj))) g_prop_data[ch].idz.cntr.display_color = 0xffffff;
    else g_prop_data[ch].idz.cntr.display_color = 0;
    g_prop_data[ch].idz.cntr.notification = nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_cntr_noti_obj));
    g_prop_data[ch].idz.cntr.e_value = nfui_spin_button_get_value((NFSPINBUTTON*)g_idz_cntr_value_obj);
    g_prop_data[ch].idz.cntr.reset = nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_cntr_reset_obj));
    g_prop_data[ch].idz.confidence_threshold = nfui_spin_button_get_value((NFSPINBUTTON*)g_idz_confidence_threshold_obj);

    memset(item_list, 0x00, sizeof(item_list));
    memset(g_prop_data[ch].ipz.item_list, 0x00, sizeof(g_prop_data[ch].ipz.item_list));
#if 1
    g_sprintf(item_list, "[bike:%d],[bus:%d],[car:%d]", 
        nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bike_obj)),
        0,
        nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_car_obj)));
#else    
    g_sprintf(item_list, "[bike:%d],[bus:%d],[car:%d]", 
        nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bike_obj)),
        nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bus_obj)),
        nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_car_obj)));
#endif
    dvatext_translate_to_engstring_db(item_list, g_prop_data[ch].ipz.item_list, sizeof(g_prop_data[ch].ipz.item_list));

    if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ipz_dwell_obj) == 0) g_prop_data[ch].ipz.dwell = 0;
    else {
        strcombo = nfui_combobox_get_value((NFCOMBOBOX*)g_ipz_dwell_obj);
        sscanf(strcombo, "%u MIN", &g_prop_data[ch].ipz.dwell);
    }

    if (memcmp(&g_org_crop_box, &g_crop_box, sizeof(DVA_CROP_T)))
    {
        g_prop_data[ch].roi_sx = (gfloat)g_crop_box.sx/(gfloat)(VIDEO_SIZE_W);
        g_prop_data[ch].roi_sy = (gfloat)g_crop_box.sy/(gfloat)(VIDEO_SIZE_H);
        g_prop_data[ch].roi_ex = (gfloat)g_crop_box.dx/(gfloat)(VIDEO_SIZE_W);
        g_prop_data[ch].roi_ey = (gfloat)g_crop_box.dy/(gfloat)(VIDEO_SIZE_H);
    }

    return 0;
}


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_detection_type_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {   
        gint type = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
        gint i, shown_idx = -1;

        for (i = 0; i < DETECT_TYPE_MAX; i++)
        {
            if (nfui_nfobject_is_shown(g_dtype_fixed[i])) shown_idx = i;
        }
        if (shown_idx == type) return FALSE; 

        for (i = 0; i < DETECT_TYPE_MAX; i++)
        {
            nfui_nfobject_hide(g_dtype_fixed[i]); 
        }
        nfui_nfobject_show(g_dtype_fixed[type]);

        for (i = 0; i < DETECT_TYPE_MAX; i++)
        {
            nfui_signal_emit(g_dtype_fixed[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_idz_vehicle_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_car_obj), TRUE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_bus_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_bike_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_car_obj), FALSE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_bus_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_bike_obj), FALSE);
        }
        nfui_signal_emit(g_idz_vehicle_car_obj, GDK_EXPOSE, TRUE);
        //nfui_signal_emit(g_idz_vehicle_bus_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_vehicle_bike_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_idz_vehicle_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;
        
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj))) state = FALSE;
        //if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bus_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_idz_vehicle_all_obj), state);
        nfui_signal_emit(g_idz_vehicle_all_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_idz_counter_active_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        nfui_nfobject_disable(g_idz_cntr_color_obj);
        nfui_nfobject_disable(g_idz_cntr_noti_obj);
        nfui_nfobject_disable(g_idz_cntr_value_obj);
        nfui_nfobject_disable(g_idz_cntr_reset_obj);  

        if (nfui_check_button_get_active((NFCHECKBUTTON*)obj))
        {
            nfui_nfobject_enable(g_idz_cntr_color_obj);
            nfui_nfobject_enable(g_idz_cntr_noti_obj); 
        }

        if (nfui_check_button_get_active((NFCHECKBUTTON*)obj)
            && nfui_check_button_get_active((NFCHECKBUTTON*)g_idz_cntr_noti_obj))
        {
            nfui_nfobject_enable(g_idz_cntr_value_obj);
            nfui_nfobject_enable(g_idz_cntr_reset_obj);
        }

        nfui_signal_emit(g_idz_cntr_color_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_cntr_noti_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_cntr_value_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_cntr_reset_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_idz_counter_noti_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        nfui_nfobject_disable(g_idz_cntr_color_obj);
        nfui_nfobject_disable(g_idz_cntr_noti_obj);
        nfui_nfobject_disable(g_idz_cntr_value_obj);
        nfui_nfobject_disable(g_idz_cntr_reset_obj);  

        if (nfui_check_button_get_active((NFCHECKBUTTON*)g_idz_cntr_active_obj))
        {
            nfui_nfobject_enable(g_idz_cntr_color_obj);
            nfui_nfobject_enable(g_idz_cntr_noti_obj); 
        }

        if (nfui_check_button_get_active((NFCHECKBUTTON*)g_idz_cntr_active_obj)
            && nfui_check_button_get_active((NFCHECKBUTTON*)obj))
        {
            nfui_nfobject_enable(g_idz_cntr_value_obj);
            nfui_nfobject_enable(g_idz_cntr_reset_obj);
        }

        nfui_signal_emit(g_idz_cntr_color_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_cntr_noti_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_cntr_value_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_idz_cntr_reset_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_event_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        NFOBJECT *top;

        gint value;
        gchar strBuf[32];

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        value = NumberKey_Open(top, nfui_nflabel_get_number((NFLABEL*)obj), x, y, 999);
        
        if (value >= 0) {
            memset(strBuf, 0x00, sizeof(strBuf));
            g_sprintf(strBuf, "%d", value);
            nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)obj, strBuf);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_idz_static_filter_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        if (nfui_check_button_get_active((NFCHECKBUTTON*)obj)) {
            nfui_nfobject_enable(g_idz_static_filter_sense_obj);
        }
        else {
            nfui_nfobject_disable(g_idz_static_filter_sense_obj);  
        }
        nfui_signal_emit(g_idz_static_filter_sense_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_ipz_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_car_obj), TRUE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_bus_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_bike_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_car_obj), FALSE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_bus_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_bike_obj), FALSE);
        }
        nfui_signal_emit(g_ipz_car_obj, GDK_EXPOSE, TRUE);
        //nfui_signal_emit(g_ipz_bus_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ipz_bike_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_ipz_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;
        
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_car_obj))) state = FALSE;
        //if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bus_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_bike_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ipz_all_obj), state);
        nfui_signal_emit(g_ipz_all_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_help_popup_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vw_deeplearning_help_popup_open(g_curwnd);
    }

    return FALSE;
}

static gboolean post_subgroup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);            
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h-20);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+20, -1, -1, NFALIGN_LEFT, 0);

	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h-20);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_video_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    static gint is_event_regi = 0;
    static gint g_move_cnt = 0;

    switch(evt->type)
    {
        case GDK_EXPOSE :
        {
            GdkDrawable *drawable = NULL;
            GdkPixmap *pm_cnvs;
            GdkGC *drawable_gc;
            GdkGC *pm_gc;

            gint gap_x, gap_y;

            if (!is_event_regi) {
                uxm_reg_imsg_event(obj, INFY_DEEPLEARNING_OBJECT_NOTIFY);
                is_event_regi = 1;
            }

            drawable = nfui_nfobject_get_window(obj);
            drawable_gc = nfui_nfobject_get_gc(obj);

            pm_cnvs = gdk_pixmap_new(drawable, obj->width, obj->height, -1);
            pm_gc = gdk_gc_new(pm_cnvs);
            gdk_gc_set_rgb_fg_color(pm_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_808080)));
            gdk_draw_rectangle(pm_cnvs, pm_gc, TRUE, 0, 0, obj->width, obj->height);
            g_object_unref(pm_gc);

            _draw_deeplearning_object_detect(pm_cnvs, obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            gdk_draw_pixmap(drawable, drawable_gc, pm_cnvs, 0, 0, gap_x, gap_y, obj->width, obj->height);

            nfui_nfobject_gc_unref(drawable_gc);
            g_object_unref(pm_cnvs);
        }
        break;

        case INFY_DEEPLEARNING_OBJECT_NOTIFY :
        {
            NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
            struct objects *objs = pInfo->p.ptr;

            if (g_cur_channel == -1) return FALSE;
            if (!objs) return FALSE; 
            if (objs->ch != g_cur_channel) return FALSE;

            _set_deeplearning_object_detect(objs);
        }
        break;
        
        case GDK_BUTTON_PRESS:
        {
            GdkEventButton *bevent;
            gint mask;

            bevent = (GdkEventButton *)evt;
            g_press_corner_mask = 0;

            if (bevent->button == 3) return FALSE;
            if (g_cur_channel == -1) return FALSE;
            if (~g_support_cam_mask & (1 << g_cur_channel)) return FALSE;
            //if (!nfui_combobox_get_cur_index(NF_COMBOBOX(g_act_combo))) return FALSE;

            mask = _get_press_corner_mask((gint)bevent->x, (gint)bevent->y);
            if (!mask) return FALSE;

            g_press_corner_mask = mask;
            g_press_x = (gint)bevent->x; 
            g_press_y = (gint)bevent->y;
        }
        break;

        case GDK_BUTTON_RELEASE:
        {
            g_press_corner_mask = 0;
            g_press_x = 0; 
            g_press_y = 0;            
        }
        break;

        case GDK_MOTION_NOTIFY:
        {
            GdkEventMotion *mevent;

            mevent = (GdkEventMotion*)evt;

            if ((mevent->state & GDK_BUTTON1_MASK) && (g_press_corner_mask))
            {
                DVA_CROP_T crop_box;

                if (_get_dva_crop_box((guint)mevent->x, (guint)mevent->y, &crop_box))
                {
                    if ((g_crop_box.sx == crop_box.sx) && (g_crop_box.sy == crop_box.sy)
                        && (g_crop_box.dx == crop_box.dx) && (g_crop_box.dy == crop_box.dy))
                    {     
                        return FALSE;
                    }

                    g_crop_box.sx = crop_box.sx;
                    g_crop_box.sy = crop_box.sy;
                    g_crop_box.dx = crop_box.dx;
                    g_crop_box.dy = crop_box.dy;

                    g_press_x = crop_box.mx;
                    g_press_y = crop_box.my;
                }
            }
            else {
                g_press_corner_mask = 0;
                g_press_x = 0; 
                g_press_y = 0;                 
            }
        }
        break;

        case GDK_DELETE:
        {
            if (g_tmr_dispvideo) g_source_remove(g_tmr_dispvideo);
            g_tmr_dispvideo = 0; 

            uxm_unreg_imsg_event(obj, INFY_DEEPLEARNING_OBJECT_NOTIFY);
            is_event_regi = 0;
            
            if (g_dva_detect.frame)
                ifree(g_dva_detect.frame);
            g_dva_detect.frame = 0;
        }
        break;
            
        default :
        break;
    }

    return FALSE;
}

static gboolean mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE :
        break;
        
        case GDK_DELETE:
 
        break;
            
        default :
        break;
    }

    return FALSE;

}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        if (g_cur_channel != -1) {
            scm_dlva_detector_setup_close_channel(g_cur_channel);
        }

        _enable_alpha();
        g_curwnd = 0;
    }

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

static gint _init_detection_type_intrusion(NFOBJECT *dtype_fixed)
{
    NFOBJECT *obj;
    gint pos_x, pos_y;
    gint btn_w, btn_h;

    gint tmp_pos_x, tmp_pos_y;

    gchar strBuf[64];
    gchar lfBuf[4096];

    pos_x = 10;
    pos_y = 2;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "INTRUSION DETECTION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, 0, 0);

    pos_x = 20;
    pos_y += 60;

    obj = nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    g_idz_human_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HUMAN DETECTION", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);    

    pos_x = 20;
    pos_y += 60;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_vehicle_all_event_handler);
    g_idz_vehicle_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VEHICLE DETECTION", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);    

    pos_x = 60;
    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_vehicle_group_event_handler);    
    g_idz_vehicle_car_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 190, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj); 

#if 0
    pos_x += 240;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_vehicle_group_event_handler);    
    g_idz_vehicle_bus_obj = obj;
	
	if(ivsc.vendor_code == 43)
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS AND FULL-SIZE CAR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 190, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  
#endif

    pos_x = 60;
    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_vehicle_group_event_handler);    
    g_idz_vehicle_bike_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 190, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   


#if 0
    pos_x = 20;
    pos_y += 60;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    //nfui_regi_post_event_callback(obj, post_idz_animal_all_event_handler);
    g_idz_animal_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ANIMAL DETECTION", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);    
#endif


/////////////////////////////////////////////////
//////// videcon only

    pos_x = 20;
    pos_y += 80;

    tmp_pos_x = pos_x;
    tmp_pos_y = pos_y;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_counter_active_event_handler);
    g_idz_cntr_active_obj = obj;

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COUNTER", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    pos_x = 60;
    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    //nfui_regi_post_event_callback(obj, post_idz_vehicle_group_event_handler);    
    g_idz_cntr_color_obj = obj;

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY COUNTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 430, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_counter_noti_event_handler);  
    g_idz_cntr_noti_obj = obj;

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    memset(lfBuf, 0x00, sizeof(lfBuf));
//    nfutil_get_line_feed_string(lookup_string("COUNTER EVENT TRIGGER QUANTITY"), 430, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), lfBuf, sizeof(lfBuf));
    g_sprintf(lfBuf, "%s :", lookup_string("COUNTER EVENT TRIGGER QUANTITY"));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 430, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    pos_y += 42;

    obj = nfui_spinbutton_new_value_with_range(100, 1, 999, 1);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
    nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 160, 40);
    nfui_regi_post_event_callback(obj, post_event_spin_event_handler);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);
    g_idz_cntr_value_obj = obj;

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    g_idz_cntr_reset_obj = obj;

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);

    //memset(lfBuf, 0x00, sizeof(lfBuf));
    //nfutil_get_line_feed_string(lookup_string("RESET COUNTER AFTER EVENT"), 430, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RESET COUNTER AFTER EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 430, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);

    if (!ivsc.dfunc.support_dlva_counter) nfui_nfobject_hide(obj);


/////////////////////////////////////////////////
//////// option : object filter

    if (ivsc.dfunc.support_dlva_counter) {
        pos_x = 20;
        pos_y += 80;
    }
    else {
        pos_x = tmp_pos_x;
        pos_y = tmp_pos_y;
    }

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_idz_static_filter_onoff_event_handler);
    g_idz_static_filter_onoff_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATIC OBJECT FILTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 320, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 140, 40);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+360, pos_y);
    nfui_nfobject_show(obj);
    g_idz_static_filter_sense_obj = obj;
 
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "LOW");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "MID");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "HIGH");

    //CONFIDENCE THRESHOLD

    pos_y += 80;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONFIDENCE THRESHOLD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 320, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_spinbutton_new_value_with_range(1, 1, 100, 1);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
    nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_regi_post_event_callback(obj, post_event_spin_event_handler);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+360, pos_y);
    nfui_nfobject_show(obj);
    g_idz_confidence_threshold_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("%", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 20, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj,  pos_x+490, pos_y);

    return 0;
}

static gint _init_detection_type_illegal_parking(NFOBJECT *dtype_fixed)
{
    NFOBJECT *obj;
    gint pos_x, pos_y;
    gint btn_w, btn_h;

    gchar strBuf[64];

    pos_x = 10;
    pos_y = 2;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "ILLEGAL PARKING");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, 0, 0);    

    pos_x = 20;
    pos_y += 60;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_ipz_all_event_handler);
    g_ipz_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);    

    pos_x = 60;
    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_ipz_group_event_handler);    
    g_ipz_car_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 190, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

#if 0
    pos_x += 240;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_ipz_group_event_handler);    
    g_ipz_bus_obj = obj;

	if(ivsc.vendor_code == 43)
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS AND FULL-SIZE CAR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 190, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  
#endif

    pos_x = 60;
    pos_y += 42;

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y+(40-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_ipz_group_event_handler);    
    g_ipz_bike_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 190, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    pos_x = 20;
    pos_y += 120;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PARKING TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 240, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj); 

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, pos_x+240, pos_y);
    nfui_nfobject_show(obj);
    g_ipz_dwell_obj = obj;

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "IMMEDIATELY");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "1 MIN");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "3 MIN");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "5 MIN");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "10 MIN");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "15 MIN");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "30 MIN");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "60 MIN");

    return 0;
}

static gint _init_detection_type_lpr(NFOBJECT *dtype_fixed)
{
    NFOBJECT *obj;
    gint pos_x, pos_y;

    gchar strBuf[64];

    pos_x = 10;
    pos_y = 2;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "LPR");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, obj, 0, 0);    



    return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint _analysis_builtin_prop_init_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed = parent;
    NFOBJECT *video_fixed;
    NFOBJECT *dtype_fixed;
    NFOBJECT *obj;

    gchar *strOffOn[] = {"OFF", "ON"};
	gchar strCh[STRING_SIZE_CAMTITLE+8];
    gchar strBuf[64];
    gchar strHelp[512];

    gint pos_x, pos_y;
	guint i = 0, j = 0;

    GdkPixbuf *help_img[NFOBJECT_STATE_COUNT];


	help_img[0] = nfui_get_image_from_file((IMG_N_DLVA_HELP_BTN), NULL);
	help_img[1] = nfui_get_image_from_file((IMG_O_DLVA_HELP_BTN), NULL);
	help_img[2] = nfui_get_image_from_file((IMG_P_DLVA_HELP_BTN), NULL);
	help_img[3] = nfui_get_image_from_file((IMG_D_DLVA_HELP_BTN), NULL);


    g_cur_channel = -1;
    g_press_corner_mask = 0;  

    memset(g_prop_data, 0x00, sizeof(DVAPropData)*GUI_CHANNEL_CNT);   
    memset(g_org_prop_data, 0x00, sizeof(DVAPropData)*GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++) {
        DAL_get_dva_prop_data(&g_prop_data[i], i);
    }
    g_memmove(g_org_prop_data, g_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT);

    g_crop_box.sx = VIDEO_SIZE_W*g_prop_data[0].roi_sx;
    g_crop_box.sy = VIDEO_SIZE_H*g_prop_data[0].roi_sy;
    g_crop_box.dx = VIDEO_SIZE_W*g_prop_data[0].roi_ex;
    g_crop_box.dy = VIDEO_SIZE_H*g_prop_data[0].roi_ey;

    memcpy(&g_org_crop_box, &g_crop_box, sizeof(DVA_CROP_T));

    video_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(video_fixed, VIDEO_SIZE_W, VIDEO_SIZE_H);
    nfui_nfobject_modify_bg(video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_nfobject_use_focus(video_fixed, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(video_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, video_fixed, 0, 60);
    nfui_regi_post_event_callback(video_fixed, post_video_fixed_event_handler);
    g_video_fixed = video_fixed;

    pos_x = 10;
    pos_y = 60 + video_fixed->height + 20;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), help_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE); 
    nfui_nfobject_set_size(obj, 40, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_help_popup_event_handler);    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_HELP_ROI, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_set_size(obj, video_fixed->width-50, 80);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+50, pos_y);
    nfui_nfobject_show(obj);

    pos_x = 10;
    pos_y += 100;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_set_size(obj, 28, 32);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);    
    nfui_nfobject_show(obj);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_HELP_EVENT, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_set_size(obj, video_fixed->width-50, 32);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+28, pos_y);    
    nfui_nfobject_show(obj);

    pos_x = 10;
    pos_y += 32;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("2)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_set_size(obj, 28, 70);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);    
    nfui_nfobject_show(obj);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_HELP_INTERVAL, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_set_size(obj, video_fixed->width-50, 70);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+28, pos_y);    
    nfui_nfobject_show(obj);

    pos_x = VIDEO_SIZE_W + 28;
    pos_y = 10;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DETECTION TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+260, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_detection_type_combo_event_handler);
    g_detect_type_obj = obj;
 
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "INTRUSION DETECTION");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "ILLEGAL PARKING");
    //nfui_combobox_append_data((NFCOMBOBOX*)obj, "LPR");

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IGNORING INTERVAL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj); 

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+260, pos_y);
    nfui_nfobject_show(obj);
    g_ignore_interval_obj = obj;

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "1 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "2 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "3 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "5 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "10 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "15 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "30 SEC");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "60 SEC");

    pos_y += 70;

    for (i = 0; i < DETECT_TYPE_MAX; i++)
    {
        dtype_fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(dtype_fixed, 540, 670);
        nfui_nfobject_modify_bg(dtype_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nffixed_put((NFFIXED*)content_fixed, dtype_fixed, pos_x, pos_y);
        nfui_regi_post_event_callback(dtype_fixed, post_subgroup_fixed_event_handler);
        g_dtype_fixed[i] = dtype_fixed;

        if (i == DETECT_TYPE_INTRUSION) _init_detection_type_intrusion(dtype_fixed);
        else if (i == DETECT_TYPE_ILLEGAL_PARKING) _init_detection_type_illegal_parking(dtype_fixed);
        else if (i == DETECT_TYPE_LPR) _init_detection_type_lpr(dtype_fixed);
    }   

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    _prvSetDataToObjects(0, &g_prop_data[0], 0);

    return 0;
}

gint _analysis_builtin_prop_show_page(gint ch, AiAnalysisActData *analysis_data)
{
    gint i;
    DVAPropData tmp_prop_data[GUI_CHANNEL_CNT];

    NF_NOTIFY_INFO vloss_info;

    if (g_cur_channel != -1) return -1;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    memset(&vloss_info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_vloss_data(&vloss_info);

    if (vloss_info.d.params[0] & (1 << ch)) {
        g_support_cam_mask = 0;
    }
    else {
        g_support_cam_mask |= 1 << ch;
    }

    memset(tmp_prop_data, 0x00, sizeof(DVAPropData)*GUI_CHANNEL_CNT);   

    for (i = 0; i < GUI_CHANNEL_CNT; i++) {
        DAL_get_dva_prop_data(&tmp_prop_data[i], i);
    }

	if (memcmp(g_prop_data, tmp_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT) != 0)
    {
        g_memmove(g_prop_data, tmp_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT);
        g_memmove(g_org_prop_data, tmp_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT);
    }

    _disable_alpha();
    _prvSetDataToObjects(ch, &g_prop_data[ch], 0);

    scm_dlva_detector_setup_open_channel(ch);

    if (!g_tmr_dispvideo) g_tmr_dispvideo = g_timeout_add(330, _timeout_draw_display_video_box, 0);  

    g_cur_channel = ch;

    return 0;
}

gint _analysis_builtin_prop_hide_page(gint ch)
{
    if (g_cur_channel == -1) return -1;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    if (g_tmr_dispvideo) g_source_remove(g_tmr_dispvideo);
    g_tmr_dispvideo = 0; 

    scm_dlva_detector_setup_close_channel(ch);
    _enable_alpha();

    g_cur_channel = -1;
    return 0;
}

gint _analysis_builtin_prop_load_changed_data(gint ch)
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    _prvLoadDataFromObjects(ch);
    return 0;
}

gint _analysis_builtin_prop_is_changed_data()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    if (memcmp(g_org_prop_data, g_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT)) {
        g_message("%s, %d, changed:1", __FUNCTION__, __LINE__);
        return 1;
    }

    g_message("%s, %d, changed:0", __FUNCTION__, __LINE__);
    return 0;
}

gint _analysis_builtin_prop_save_data()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    g_memmove(g_org_prop_data, g_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT);
    DAL_set_dva_prop_data_all(g_prop_data, GUI_CHANNEL_CNT);
}

gint _analysis_builtin_prop_cancel_data(gint ch, gint expose)
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    g_memmove(g_prop_data, g_org_prop_data, sizeof(DVAPropData)*GUI_CHANNEL_CNT);
    _prvSetDataToObjects(ch, &g_prop_data[ch], expose);
    return 0;
}
