#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "vw_sys_camera_ipcam_setup_main.h"
#include "vw_sys_camera_ipcam_vstream.h"
#include "vw_sys_camera_ipcam_vstream_preview.h"
#include "vw_slider_set_value_popup.h"

#include "vw_codec_data_translate.h"

#define _MASK(a)    (1 << (a))
#define _MIN(a, b)  (a > b ? b : a)
#define _MAX(a, b)  (a > b ? a : b)

typedef enum _STREAM_E {
    STREAM_1    = 0,
    STREAM_2,
    STREAM_MAX
} STREAM_E;

enum {
    SET_CODEC       = 0,
    SET_SIZE,
    SET_MAX_FPS,
    SET_CONTROL,
    SET_MAX_BPS,
    SET_MIN_BPS,
    SET_MAX
};

typedef struct _PARAM_T
{
    gchar buf[64];
} PARAM_T;

typedef struct _IPCAM_INFO
{
    PARAM_T size_list[2];
    CAM_ENCODER_T encoder;
    guint corridor_mode;
} IPCAM_INFO;

static IPCAM_INFO ipcamdata[GUI_CHANNEL_CNT];
static StreamData streamdata[GUI_CHANNEL_CNT];
static StreamData org_streamdata[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[STREAM_MAX][SET_MAX];
static NFOBJECT *g_corr_obj[2];
static NFOBJECT *g_ntb;
static NFOBJECT *g_copy_obj;
static gint g_ch;
static gint g_stream_update = 0;


static gint _get_max_fps_from_size(gint ch, gint stream, gchar size)
{
    gint max_fps = 0;
    
    if (!nf_ipcam_get_max_fps_information(ch, size, &max_fps))
    {
        max_fps = (gint)ipcamdata[ch].encoder.capinfo.fps_max[stream];
    }
    
    return max_fps;
}

static gchar get_size_data(gchar *info)
{
	return vw_codec_get_resol_data(info);
}

static void translate_data_size_capable(guint64 capable, gchar *buf)
{
    vw_codec_translate_capable_data_resol(capable, buf);
}

static gchar* get_size_info(gchar data)
{
	return vw_codec_get_resol_info(data);
}

static gint _update_object_stream_info(gint ch)
{
    gint i, j;

    for (i = 0; i < STREAM_MAX; i++)
    {
        nfui_combobox_set_data_no_expose(NF_COMBOBOX(value[i][SET_CODEC]), streamdata[ch].vcodec[i]);
        nfui_combobox_set_data_no_expose(NF_COMBOBOX(value[i][SET_SIZE]), get_size_info(streamdata[ch].size[i][0]));
        nfui_nflabel_set_number((NFLABEL*)value[i][SET_MAX_FPS], (gint)streamdata[ch].max_fps[i]);
        nfui_nflabel_set_number((NFLABEL*)value[i][SET_MAX_BPS], (gint)streamdata[ch].max_bps[i]);
        nfui_nflabel_set_number((NFLABEL*)value[i][SET_MIN_BPS], (gint)streamdata[ch].min_bps[i]);
        if (!(i > STREAM_1 && strcmp(streamdata[ch].control[i], "IDNR") == 0)) {
            nfui_combobox_set_data_no_expose(NF_COMBOBOX(value[i][SET_CONTROL]), streamdata[ch].control[i]);
        }

        if (i < ipcamdata[ch].encoder.capinfo.encoder_cnt)
        {
            nfui_combobox_set_data_no_expose(NF_COMBOBOX(value[i][SET_SIZE]), get_size_info(streamdata[ch].size[i][0]));
            if (!(i > STREAM_1 && strcmp(streamdata[ch].control[i], "IDNR") == 0)) {
                nfui_combobox_set_data_no_expose(NF_COMBOBOX(value[i][SET_CONTROL]), streamdata[ch].control[i]);
            }

            nfui_nflabel_use_number((NFLABEL*)value[i][SET_MAX_FPS], TRUE);
            nfui_nflabel_set_number((NFLABEL*)value[i][SET_MAX_FPS], (gint)streamdata[ch].max_fps[i]);

            nfui_nflabel_use_number((NFLABEL*)value[i][SET_MAX_BPS], TRUE);
            nfui_nflabel_set_number((NFLABEL*)value[i][SET_MAX_BPS], (gint)streamdata[ch].max_bps[i]);

            nfui_nflabel_use_number((NFLABEL*)value[i][SET_MIN_BPS], TRUE);
            nfui_nflabel_set_number((NFLABEL*)value[i][SET_MIN_BPS], (gint)streamdata[ch].min_bps[i]);

            for (j = 0; j < SET_MAX; j++)
                nfui_nfobject_enable(value[i][j]);

            if(ipcamdata[ch].encoder.capinfo.bitctrl_support[i] < NF_IPCAM_BITRATE_CONTROL_VBR)
                nfui_nfobject_disable(value[i][SET_CONTROL]);

            if(ipcamdata[ch].encoder.capinfo.vcodec_support[i] < NF_IPCAM_VCODEC_H265)
                nfui_nfobject_disable(value[i][SET_CODEC]);
        }
        else
        {
            nfui_nflabel_use_number((NFLABEL*)value[i][SET_MAX_FPS], FALSE);
            nfui_nflabel_set_text((NFLABEL*)value[i][SET_MAX_FPS], "");

            nfui_nflabel_use_number((NFLABEL*)value[i][SET_MAX_BPS], FALSE);
            nfui_nflabel_set_text((NFLABEL*)value[i][SET_MAX_BPS], "");

            nfui_nflabel_use_number((NFLABEL*)value[i][SET_MIN_BPS], FALSE);
            nfui_nflabel_set_text((NFLABEL*)value[i][SET_MIN_BPS], "");

            for (j = 0; j < SET_MAX; j++)
                nfui_nfobject_disable(value[i][j]);
        }
    }
    
    if (scm_get_ipcam_corridor_supp(ch))
    {
        nfui_nfobject_enable(g_corr_obj[1]);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_corr_obj[1], streamdata[ch].corridor_mode);
    }
    else
    {
        nfui_nfobject_disable(g_corr_obj[1]);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_corr_obj[1], 0);
    }

    // multilang file -> source : IDNR, english : VBR+
    if (strcmp(streamdata[ch].control[0], "IDNR") == 0) {
        nfui_nfobject_disable(value[STREAM_2][SET_CONTROL]);
    }

    return 0;
}

static gint _init_resol_capability()
{
    int i;

    for(i = 0; i< GUI_CHANNEL_CNT; i++)
    {
        translate_data_size_capable(ipcamdata[i].encoder.cap[0], ipcamdata[i].size_list[0].buf);
        translate_data_size_capable(ipcamdata[i].encoder.cap[1], ipcamdata[i].size_list[1].buf);
    }
    return 0;
}

static gint _init_codec(guint capable, gchar *buf)
{
    memset(buf, 0x00, sizeof(buf));

    if(capable & NF_IPCAM_VCODEC_H264)      strcpy(buf, "H.264");
    else if(capable & NF_IPCAM_VCODEC_H265) strcpy(buf, "H.265");

    return 0;
}

static gint _init_bitrate_control(guint capable, gchar *buf)
{
    memset(buf, 0x00, sizeof(buf));

    if(capable & NF_IPCAM_BITRATE_CONTROL_VBR)      strcpy(buf, "VBR");
    else if(capable & NF_IPCAM_BITRATE_CONTROL_CBR) strcpy(buf, "CBR");
    else if(capable & NF_IPCAM_BITRATE_CONTROL_MBR) strcpy(buf, "MBR");
    else if(capable & NF_IPCAM_BITRATE_CONTROL_VBR_PLUS) strcpy(buf, "IDNR");

    return 0;
}


static void _sync_db_data()
{
    gint ch, i;
    gchar tmp_buf[32];
    gchar tmp_ctrl[32];
	gint max_fps;

    memset(tmp_buf, 0x00, sizeof(tmp_buf));

    for (ch = 0; ch< GUI_CHANNEL_CNT; ch++)
    {
        if (ipcamdata[ch].encoder.capinfo.encoder_cnt == 0) continue;

        for (i = 0; i < ipcamdata[ch].encoder.capinfo.encoder_cnt; i++)
        {
            _init_codec(ipcamdata[ch].encoder.capinfo.vcodec_default[i], tmp_buf);
            strcpy(streamdata[ch].vcodec[i], tmp_buf);

            translate_data_size_capable(ipcamdata[ch].encoder.cur[i], tmp_buf);
            streamdata[ch].size[i][0] = tmp_buf[0];

            streamdata[ch].max_fps[i] = ipcamdata[ch].encoder.capinfo.fps_max_default[i];
			max_fps = _get_max_fps_from_size(ch, i, streamdata[ch].size[i][0]);
            if (max_fps < streamdata[ch].max_fps[i])
            {
                streamdata[ch].max_fps[i] = max_fps;
            }

            streamdata[ch].min_bps[i] = ipcamdata[ch].encoder.capinfo.br_min_default[i];
            streamdata[ch].max_bps[i] = ipcamdata[ch].encoder.capinfo.br_max_default[i];

            _init_bitrate_control(ipcamdata[ch].encoder.capinfo.bitctrl_default[i], tmp_ctrl);
            strcpy(streamdata[ch].control[i], tmp_ctrl);
        }
        
        streamdata[ch].corridor_mode = ipcamdata[ch].corridor_mode;
    }
}

static void _set_codec_info(NFOBJECT *obj, int num, gint ch)
{
    if (ipcamdata[ch].encoder.capinfo.vcodec_support[num] & NF_IPCAM_VCODEC_H264) nfui_combobox_append_data(obj, "H.264");
    if (ipcamdata[ch].encoder.capinfo.vcodec_support[num] & NF_IPCAM_VCODEC_H265) nfui_combobox_append_data(obj, "H.265");
}

static gint _copy_ipcam_stream_db(gint src_ch, guint copy_mask)
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (copy_mask & (1 << i))
        {
            g_memmove(&streamdata[i], &streamdata[src_ch], sizeof(StreamData));
        }
    }

    return 0;
}

static void _set_capable_size_info(NFOBJECT *obj, int num, gint ch)
{
    gint data_len;
    int j;

    data_len = strlen(ipcamdata[ch].size_list[num].buf);

    for (j = 0; j < data_len; j++)
        nfui_combobox_append_data(obj, get_size_info(ipcamdata[ch].size_list[num].buf[j]));
}

static void _set_bitrate_control_info(NFOBJECT *obj, gint num, gint ch)
{
    if(ipcamdata[ch].encoder.capinfo.bitctrl_support[num] & NF_IPCAM_BITRATE_CONTROL_VBR) nfui_combobox_append_data(obj, "VBR");
    if(ipcamdata[ch].encoder.capinfo.bitctrl_support[num] & NF_IPCAM_BITRATE_CONTROL_CBR) nfui_combobox_append_data(obj, "CBR");
    if(ipcamdata[ch].encoder.capinfo.bitctrl_support[num] & NF_IPCAM_BITRATE_CONTROL_MBR) nfui_combobox_append_data(obj, "MBR");
    if(ipcamdata[ch].encoder.capinfo.bitctrl_support[num] & NF_IPCAM_BITRATE_CONTROL_VBR_PLUS) nfui_combobox_append_data(obj, "IDNR");
}

static gboolean post_corridor_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint val;

        streamdata[g_ch].corridor_mode = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
    }

    return FALSE;
}

static gboolean post_codec_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case NFEVENT_COMBOBOX_CHANGED:
        {
            gint i;
            gchar *buf;

            buf = nfui_combobox_get_value(NF_COMBOBOX(obj));

            for (i = 0; i < STREAM_MAX; i++)
            {
                if (value[i][SET_CODEC] == obj)
                {
                    strcpy(streamdata[g_ch].vcodec[i], buf);
                }
            }
        }
        break;

        default:
        break;
    }

    return FALSE;
}

static gboolean post_resol_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case NFEVENT_COMBOBOX_CHANGED:
        {
            gint i;
            gchar *buf;
            gchar pre_val;
            guint max_fps = 0;
            guint64 cap = 0;
            mb_type ret;
            gchar strbuf[512] = {0,};
            
            buf = nfui_combobox_get_value(NF_COMBOBOX(obj));

            for (i = 0; i < STREAM_MAX; i++)
            {
                if (value[i][SET_SIZE] == obj)
                {
                    pre_val = streamdata[g_ch].size[i][0];
                    streamdata[g_ch].size[i][0] = get_size_data(buf);
					break;
				}
			}
            
            max_fps = _get_max_fps_from_size(g_ch, i, streamdata[g_ch].size[i][0]);
            
            if (streamdata[g_ch].max_fps[i] > max_fps)
            {
                g_sprintf(strbuf, lookup_string("The maximum supported FPS for the selected resolution is %dFPS.\nIf you confirm this setting the FPS will be automatically adjusted.\nContinue?"), max_fps);
                ret = nftool_mbox(g_curwnd, "CONFIRM", strbuf, NFTOOL_MB_OKCANCEL);

                if (ret == NFTOOL_MB_OK)
                {
                    streamdata[g_ch].max_fps[i] = max_fps;
                    nfui_nflabel_set_number((NFLABEL*)value[i][SET_MAX_FPS], max_fps);
                    nfui_signal_emit(value[i][SET_MAX_FPS], GDK_EXPOSE, FALSE);
                }
                else
                {
                    streamdata[g_ch].size[i][0] = pre_val;
                    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, get_size_info(pre_val));
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
                }
            }
        }
        break;
        
        default: 
        break;
    }
    
    return FALSE;
}

static gboolean post_fps_numlb_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid=KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        gint numTemp;
        gint i, min, max;
        guint opt = 0;
        gint ret = 0;

        numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
        opt = (1 << OPT_DISPLAY_WITH_SPIN);

        for (i = 0; i < STREAM_MAX; i++)
        {
            if (value[i][SET_MAX_FPS] == obj)
            {
//              g_message("%s, %d, cap_max:%d", __FUNCTION__, __LINE__, ipcamdata[g_ch].encoder.capinfo.fps_max[i]);

                min = 2;
                max = _get_max_fps_from_size(g_ch, i, streamdata[g_ch].size[i][0]);
                
                ret = VW_open_slider_set_value_popup(g_curwnd, "MAXIMUM FRAME RATE", opt, min, max, &numTemp);
                if (ret) streamdata[g_ch].max_fps[i] = numTemp;
            }
        }

        if (ret) {
            nfui_nflabel_set_number((NFLABEL*)obj, numTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_bps_numlb_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid=KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        gint numTemp;
        gint i, min, max;
        guint opt = 0;
        gint ret = 0;

        numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
        opt = (1 << OPT_DISPLAY_WITH_SPIN);

        for (i = 0; i < STREAM_MAX; i++)
        {
            if (value[i][SET_MAX_BPS] == obj)
            {
//              g_message("%s, %d, cap_min:%d", __FUNCTION__, __LINE__, ipcamdata[g_ch].encoder.capinfo.bitrate_min[i]);
//              g_message("%s, %d, cap_max:%d", __FUNCTION__, __LINE__, ipcamdata[g_ch].encoder.capinfo.bitrate_max[i]);

                min = _MAX(streamdata[g_ch].min_bps[i], ipcamdata[g_ch].encoder.capinfo.bitrate_min[i]);
                max = ipcamdata[g_ch].encoder.capinfo.bitrate_max[i];
                ret = VW_open_slider_set_value_popup(g_curwnd, "MAXIMUM BIT RATE(kbps)", opt, min, max, &numTemp);
                if (ret) streamdata[g_ch].max_bps[i] = numTemp;
            }

            if (value[i][SET_MIN_BPS] == obj)
            {
//              g_message("%s, %d, cap_min:%d", __FUNCTION__, __LINE__, ipcamdata[g_ch].encoder.capinfo.bitrate_min[i]);
//              g_message("%s, %d, cap_max:%d", __FUNCTION__, __LINE__, ipcamdata[g_ch].encoder.capinfo.bitrate_max[i]);

                min = ipcamdata[g_ch].encoder.capinfo.bitrate_min[i];
                max = _MIN(streamdata[g_ch].max_bps[i], ipcamdata[g_ch].encoder.capinfo.bitrate_max[i]);
                ret = VW_open_slider_set_value_popup(g_curwnd, "MINIMUM BIT RATE(kbps)", opt, min, max, &numTemp);
                if (ret) streamdata[g_ch].min_bps[i] = numTemp;
            }
        }

        if (ret) {
            nfui_nflabel_set_number((NFLABEL*)obj, numTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_bit_control_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint i;
        gchar *buf;

        buf = nfui_combobox_get_value(NF_COMBOBOX(obj));

        for( i =0; i < STREAM_MAX; i++)
        {
            if(value[i][SET_CONTROL] == obj)
            {
                strcpy(streamdata[g_ch].control[i], buf);
                // multilang file -> source : IDNR, english : VBR+
                if (i == STREAM_1 && strcmp(buf, "IDNR") == 0)
                {
                    nfui_nfobject_disable(value[STREAM_2][SET_CONTROL]);
                    nfui_signal_emit(value[STREAM_2][SET_CONTROL], GDK_EXPOSE, TRUE);
                }
                else
                {
                    if (nfui_nfobject_is_disabled(value[STREAM_2][SET_CONTROL]))
                    {
                        nfui_nfobject_enable(value[STREAM_2][SET_CONTROL]);
                        nfui_signal_emit(value[STREAM_2][SET_CONTROL], GDK_EXPOSE, TRUE);
                    }
                }
            }
        }
    }

    return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == INFY_STREAM_DATA_RELOAD)
    {
        g_stream_update = 1;
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_STREAM_DATA_RELOAD);
    }

    return FALSE;
}

static gboolean post_copy_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    guint dst_mask;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        dst_mask = _get_copy_chmask(g_ch);
        _copy_ipcam_stream_db(g_ch, dst_mask);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint i, j;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        g_memmove(&streamdata, &org_streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);
        _update_object_stream_info(g_ch);

        for (i = 0; i < STREAM_MAX; i++)
        {
            for (j = 0; j < SET_MAX; j++)
            {
                nfui_signal_emit(value[i][j], GDK_EXPOSE, TRUE);
            }
        }
        nfui_signal_emit(g_corr_obj[1], GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *buf = NULL;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if(memcmp(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT))
        {
            g_memmove(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);
            DAL_set_stream_data_all(streamdata, GUI_CHANNEL_CNT);

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            syscam_set_changeflag(1);
        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        IPCamVideoStream_tab_out_handler();
        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}

void VW_Init_IPCamVideoStream_Page(NFOBJECT *parent, gint ch)
{
    NFOBJECT *content_fixed;
    NFOBJECT *fixed_temp;
    NFOBJECT *ntb;
    NFOBJECT *ntb_tmp1;
    NFOBJECT *ntb_tmp2;
    NFOBJECT *obj;

    gchar *stream_title[2] = {"VIDEO 1", "VIDEO 2"};
    gchar *property_title[6] = {"CODEC",  "RESOLUTION", "MAXIMUM FRAME RATE", "BITRATE CONTROL", "MAXIMUM BIT RATE(kbps)",
                                "MINIMUM BIT RATE(kbps)"};
    gchar *corr_mode[3] = {"OFF", "90 DEGREE", "270 DEGREE"};

    guint tbl_w[3] = {0, };
    gint pos_x, pos_y;
    gint i, j;
    gint size_w, size_h;
    g_ch = ch;

    g_curwnd = nfui_nfobject_get_top(parent);
    g_stream_update = 0;

    memset(streamdata, 0x00, sizeof(StreamData)*GUI_CHANNEL_CNT);
    memset(org_streamdata, 0x00, sizeof(StreamData)*GUI_CHANNEL_CNT);

    memset(ipcamdata, 0X00, sizeof(IPCAM_INFO)*GUI_CHANNEL_CNT);

    for (i = 0; i< GUI_CHANNEL_CNT; i++)
        DAL_get_stream_data(&streamdata[i], i);

    g_memmove(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);

    for (i = 0; i< GUI_CHANNEL_CNT; i++)
    {
        scm_get_encoder_capability(i, &ipcamdata[i].encoder);
        ipcamdata[i].corridor_mode = scm_get_ipcam_corridor_mode(i);
//      g_message("%s, %d, 1st:%08X, 2nd:%08X", __FUNCTION__, __LINE__, ipcamdata[i].encoder.capinfo.res_support[0], ipcamdata[i].encoder.capinfo.res_support[1]);
    }

    _init_resol_capability();
    _sync_db_data();

    if (memcmp(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT))
    {
        DAL_set_stream_data_all(streamdata, GUI_CHANNEL_CNT);
        syscam_set_changeflag(1);
        g_memmove(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);
    }

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_W, MENU_V_IPCAMSET_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_X, MENU_V_IPCAMSET_SUBTAB_INNER_Y);
    nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);

    pos_x = 20;
    pos_y = 20;

    tbl_w[0] = 530;
    tbl_w[1] = 602;

    ntb = (NFOBJECT*)nfui_nftable_new(2, 1, 2, 2, tbl_w, 82);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, pos_x, pos_y);

    tbl_w[0] = 602;
    tbl_w[1] = 0;

    ntb_tmp1 = (NFOBJECT*)nfui_nftable_new(1, 2, 2, 2, tbl_w, 40);
    nfui_nfobject_show(ntb_tmp1);
    nfui_nftable_attach((NFTABLE*)ntb, ntb_tmp1, 1, 0);

    tbl_w[0] = 300;
    tbl_w[1] = 300;

    ntb_tmp2 = (NFOBJECT*)nfui_nftable_new(2, 1, 2, 2, tbl_w, 40);
    nfui_nfobject_show(ntb_tmp2);
    nfui_nftable_attach((NFTABLE*)ntb_tmp1, ntb_tmp2, 0, 1);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PROPERTY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO STREAM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb_tmp1, obj, 0, 0);

    for (i = 0; i < 2; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(stream_title[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb_tmp2, obj, i, 0);
    }

    pos_y += 84;

    tbl_w[0] = 530;
    tbl_w[1] = 300;
    tbl_w[2] = 300;

    ntb = (NFOBJECT*)nfui_nftable_new(3, 6, 2, 2, tbl_w, 40);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, pos_x, pos_y);

    for (i = 0; i < SET_MAX; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(property_title[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i);

        if (i == 0)
        {
            for (j = 0; j < STREAM_MAX; j++)
            {
                obj = nfui_combobox_new(NULL, 0, 0);
                nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
                nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
                nfui_nfobject_show(obj);
                nfui_nftable_attach((NFTABLE*)ntb, obj, j+1, i);
                nfui_regi_post_event_callback(obj, post_codec_combo_event_handler);
                value[j][i] = obj;
            }
        }
        else if (i == 1)
        {
            for (j = 0; j < STREAM_MAX; j++)
            {
                obj = nfui_combobox_new(NULL, 0, 0);
                nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
                nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
                nfui_nfobject_show(obj);
                nfui_nftable_attach((NFTABLE*)ntb, obj, j+1, i);
                nfui_regi_post_event_callback(obj, post_resol_event_handler);
                value[j][i] = obj;
            }
        }
        else if (i == 2)
        {
            for (j = 0; j < STREAM_MAX; j++)
            {
                obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
                nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
                nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
                nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
                nfui_nfobject_show(obj);
                nfui_regi_post_event_callback(obj, post_fps_numlb_event_handler);
                nfui_nftable_attach((NFTABLE*)ntb, obj, j+1, i);
                value[j][i] = obj;
            }
        }
        else if (i == 3)
        {
            for(j =0; j<  STREAM_MAX; j++)
            {
                obj = nfui_combobox_new(NULL, 0, 0);
                nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
                nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
                nfui_nfobject_show(obj);
                nfui_nftable_attach((NFTABLE*)ntb, obj, j+1, i);
                nfui_regi_post_event_callback(obj, post_bit_control_combo_event_handler);
                value[j][i] = obj;

            }
        }
        else if (i == 4)
        {
            for (j = 0; j < STREAM_MAX; j++)
            {
                obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
                nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
                nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
                nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
                nfui_nfobject_show(obj);
                nfui_regi_post_event_callback(obj, post_bps_numlb_event_handler);
                nfui_nftable_attach((NFTABLE*)ntb, obj, j+1, i);
                value[j][i] = obj;
            }
        }
        else if (i == 5)
        {
            for (j = 0; j < STREAM_MAX; j++)
            {
                obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
                nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
                nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
                nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
                nfui_nfobject_show(obj);
                nfui_regi_post_event_callback(obj, post_bps_numlb_event_handler);
                nfui_nftable_attach((NFTABLE*)ntb, obj, j+1, i);
                value[j][i] = obj;
            }
        }
    }
    
    pos_y += 280 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CORRIDOR VIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 530, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    if(ivsc.dfunc.support_corridor) nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    g_corr_obj[0] = obj;
    
    pos_x += 530 + 2;
    
    obj = nfui_combobox_new(corr_mode, 3, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    if(ivsc.dfunc.support_corridor) nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_corridor_combo_event_handler);
    g_corr_obj[1] = obj;

	obj = nftool_normal_button_create_type1("COPY SETTINGS TO", COPY_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, COPY_BTN_X, COPY_BTN_Y);
	nfui_regi_post_event_callback(obj, post_copy_button_event_handler);
    g_copy_obj = obj;

    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R3_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R2_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R1_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    for (i = 0; i < STREAM_MAX; i++)
        _set_codec_info(value[i][SET_CODEC], i, ch);

    for (i = 0; i < STREAM_MAX; i++)
        _set_capable_size_info(value[i][SET_SIZE], i, ch);

    for (i = 0; i < STREAM_MAX; i++)
        _set_bitrate_control_info(value[i][SET_CONTROL], i, ch);

    _update_object_stream_info(0);

    uxm_reg_imsg_event(content_fixed, INFY_STREAM_DATA_RELOAD);
    uxm_monitor_on_imsg_event(content_fixed, INFY_STREAM_DATA_RELOAD);
}

gint IPCamVideoStream_update_channel(gint ch)
{
    int i, j;

    for (i = 0; i < STREAM_MAX; i++)
    {
        nfui_combobox_remove_all((NFCOMBOBOX*)value[i][SET_CODEC]);
        _set_codec_info(value[i][SET_CODEC],i,ch);
    }

    for (i = 0; i < STREAM_MAX; i++)
    {
        nfui_combobox_remove_all((NFCOMBOBOX*)value[i][SET_SIZE]);
        _set_capable_size_info(value[i][SET_SIZE], i, ch);
    }

    for (i = 0; i < STREAM_MAX; i++)
    {
        nfui_combobox_remove_all((NFCOMBOBOX*)value[i][SET_CONTROL]);
        _set_bitrate_control_info(value[i][SET_CONTROL],i,ch);
    }

    _update_object_stream_info(ch);

    for (i = 0; i < STREAM_MAX; i++)
    {
        for (j = 0; j < SET_MAX; j++)
        {
            nfui_signal_emit(value[i][j], GDK_EXPOSE, TRUE);
        }
    }
    nfui_signal_emit(g_corr_obj[1], GDK_EXPOSE, TRUE);

    g_ch = ch;

    return 0;
}

gboolean IPCamVideoStream_tab_in_handler()
{
    IPCamVideoStream_refresh_data();
    return FALSE;
}

gboolean IPCamVideoStream_tab_out_handler()
{
    gchar *buf = NULL;
    mb_type ret;

    if (!memcmp(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT))
        return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if(ret == NFTOOL_MB_OK)
    {
        g_memmove(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);
        DAL_set_stream_data_all(streamdata, GUI_CHANNEL_CNT);

        //scm_put_log(CHANGE_CAM_IMAGE, 0, 0);
        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        syscam_set_changeflag(1);
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(&streamdata, &org_streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);
        _update_object_stream_info(g_ch);
    }

    return FALSE;
}

gint IPCamVideoStream_refresh_data()
{
    gint i;

    if (g_stream_update)
    {
        memset(ipcamdata, 0X00, sizeof(IPCAM_INFO)*GUI_CHANNEL_CNT);

        for (i = 0; i< GUI_CHANNEL_CNT; i++)
        {
            scm_get_encoder_capability(i, &ipcamdata[i].encoder);
//          g_message("%s, %d, 1st:%08X, 2nd:%08X", __FUNCTION__, __LINE__, ipcamdata[i].encoder.capinfo.res_support[0], ipcamdata[i].encoder.capinfo.res_support[1]);
        }

        _init_resol_capability();
        _sync_db_data();

        if (memcmp(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT))
        {
            DAL_set_stream_data_all(streamdata, GUI_CHANNEL_CNT);
            syscam_set_changeflag(1);
            g_memmove(&org_streamdata, &streamdata, sizeof(StreamData)*GUI_CHANNEL_CNT);
        }

        for (i = 0; i < STREAM_MAX; i++)
        {
            nfui_combobox_remove_all((NFCOMBOBOX*)value[i][SET_CODEC]);
            _set_codec_info(value[i][SET_CODEC], i, g_ch);
        }

        for (i = 0; i < STREAM_MAX; i++)
        {
            nfui_combobox_remove_all((NFCOMBOBOX*)value[i][SET_SIZE]);
            _set_capable_size_info(value[i][SET_SIZE], i, g_ch);
        }

        for (i =0; i < STREAM_MAX; i++)
        {
            nfui_combobox_remove_all((NFCOMBOBOX*)value[i][SET_CONTROL]);
            _set_bitrate_control_info(value[i][SET_CONTROL], i, g_ch);
        }

        _update_object_stream_info(g_ch);
    }

    return 0;
}
