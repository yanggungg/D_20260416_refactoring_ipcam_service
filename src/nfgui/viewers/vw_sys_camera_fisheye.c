
#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "tools/ix_mem.h"

#include "support/util.h"
#include "support/color.h"
#include "support/event_loop.h"

#include "modules/log.h"
#include "services/scm.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"

#include "vw_vkeyboard_num.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_fisheye.h"

#include "nf_api_live.h"



#define DEGREE_0        "0\u00B0"
#define DEGREE_90       "90\u00B0"
#define DEGREE_180      "180\u00B0"
#define DEGREE_270      "270\u00B0"

#define STR_DISABLE_DLVA "You can not use the features such as the AI detection and the Fisheye Dewarping at the same time.\nThe AI detection will be automatically relieved in order to activate the feature of Fisheye Dewarping.\nDo you want to continue?"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_fixed = 0;

static guint g_support_mask = 0;

static NFOBJECT *g_ch_combo = 0;
static NFOBJECT *g_act_obj = 0;
static NFOBJECT *g_mount_obj[3];
static NFOBJECT *g_viewtype_obj[3];

static CamItxFisheyeData g_fisheyedata[GUI_CHANNEL_CNT];
static CamItxFisheyeData g_org_fisheyedata[GUI_CHANNEL_CNT];


static gint _start_preview(gint ch)
{
	guint ch_mask = 0;
    gint x, y;
    gint gpu_mode;

	nfui_nfobject_get_offset(g_video_fixed, &x, &y);
	ch_mask |= (1 << ch);

    if (g_fisheyedata[ch].act) nf_live_fisheye_set_enable(ch);
    else nf_live_fisheye_set_enable(-1);

	vsm_live_preview_start(ch_mask, x, y, g_video_fixed->width, g_video_fixed->height);		
    return 0;
}

static gint _stop_preview()
{
    nf_live_fisheye_set_enable(-1);    
    vsm_live_preview_stop();
    return 0;
}

static gint _show_video_obj()
{
	nfui_nfobject_modify_bg(g_video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    return 0;
}

static gint _hide_video_obj()
{
    nfui_nfobject_modify_bg(g_video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_signal_emit(g_video_fixed, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _sync_fisheye_video_param()
{
    NF_FISHEYE_VIDEO_PARAM param;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        nf_live_fisheye_get_video_param(i, &param);

        if (param.view_type == NF_FISHEYE_VIEW_SIGLE) g_fisheyedata[i].default_view = 0;
        else if (param.view_type == NF_FISHEYE_VIEW_QUAD) g_fisheyedata[i].default_view = 1;
        else if (param.view_type == NF_FISHEYE_VIEW_PANORAMA) g_fisheyedata[i].default_view = 2;
    }

    if (memcmp(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT) == 0) return 0;

    g_memmove(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
    DAL_set_camera_itx_fisheye_data_all(g_fisheyedata, GUI_CHANNEL_CNT);

    syscam_set_changeflag(1);
    return 0;
}

static gint _set_fisheye_video_param(gint ch)
{
    NF_FISHEYE_VIDEO_PARAM pre_param;
    NF_FISHEYE_VIDEO_PARAM post_param;

    memset(&pre_param, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));
    memset(&post_param, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));

    nf_live_fisheye_get_video_param(ch, &pre_param);
    memcpy(&post_param, &pre_param, sizeof(NF_FISHEYE_VIDEO_PARAM));

    if (g_fisheyedata[ch].mount_mode == 0) post_param.mnt_type = NF_FISHEYE_MOUNT_CEILING;
    else if (g_fisheyedata[ch].mount_mode == 1) post_param.mnt_type = NF_FISHEYE_MOUNT_WALL;
    else if (g_fisheyedata[ch].mount_mode == 2) post_param.mnt_type = NF_FISHEYE_MOUNT_GROUND;

    if (g_fisheyedata[ch].default_view == 0) post_param.view_type = NF_FISHEYE_VIEW_SIGLE;
    else if (g_fisheyedata[ch].default_view == 1) post_param.view_type = NF_FISHEYE_VIEW_QUAD;
    else if (g_fisheyedata[ch].default_view == 2) post_param.view_type = NF_FISHEYE_VIEW_PANORAMA;

    if (memcmp(&post_param, &pre_param, sizeof(NF_FISHEYE_VIDEO_PARAM)) != 0) {
        nf_live_fisheye_set_video_param(ch, &post_param);
    }

    return 0;
}

static gint _is_enable_dlva_prop()
{
    DVAPropData data;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(DVAPropData));
        DAL_get_dva_prop_data(&data, i);
        if (data.active) return 1;
    }

    return 0;
}

static gint _disable_dlva_prop()
{
    DVAPropData data;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(DVAPropData));
        DAL_get_dva_prop_data(&data, i);
        if (data.active == 1)
        {
            data.active = 0;
            DAL_set_dva_prop_data(data, i);
        }
    }
    syscam_set_changeflag(1);
    return 0;
}

static gint _prvSetDataToObjects(gint expose)
{
    gint ch;

    ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

    nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_act_obj), g_fisheyedata[ch].act);

    if (g_support_mask & (1 << ch)) nfui_nfobject_enable(g_act_obj);
    else nfui_nfobject_disable(g_act_obj);        

    if ((g_support_mask & (1 << ch)) && (g_fisheyedata[ch].act))
    {
        nfui_nfobject_enable(g_mount_obj[0]);
        nfui_nfobject_enable(g_mount_obj[1]);
        nfui_nfobject_enable(g_mount_obj[2]);
        nfui_nfobject_enable(g_viewtype_obj[0]);
        nfui_nfobject_enable(g_viewtype_obj[1]);
        nfui_nfobject_enable(g_viewtype_obj[2]);

        nfui_radio_button_set_toggled(NF_BUTTON(g_mount_obj[g_fisheyedata[ch].mount_mode]), TRUE);
        nfui_radio_button_set_toggled(NF_BUTTON(g_viewtype_obj[g_fisheyedata[ch].default_view]), TRUE);        
    }
    else
    {
        nfui_radio_button_set_toggled(NF_BUTTON(g_mount_obj[g_fisheyedata[ch].mount_mode]), TRUE);
        nfui_radio_button_set_toggled(NF_BUTTON(g_viewtype_obj[g_fisheyedata[ch].default_view]), TRUE);        

        nfui_nfobject_disable(g_mount_obj[0]);
        nfui_nfobject_disable(g_mount_obj[1]);
        nfui_nfobject_disable(g_mount_obj[2]);
        nfui_nfobject_disable(g_viewtype_obj[0]);
        nfui_nfobject_disable(g_viewtype_obj[1]);
        nfui_nfobject_disable(g_viewtype_obj[2]);
    }

    if (expose)
    {
        nfui_signal_emit(g_act_obj, GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_mount_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_mount_obj[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_mount_obj[2], GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_viewtype_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_viewtype_obj[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_viewtype_obj[2], GDK_EXPOSE, TRUE);
    }

	return 0;
}

static gboolean post_ch_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        gint ch;

        ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
        _stop_preview();
        _start_preview(ch);
        _prvSetDataToObjects(1);
    }

    return FALSE;
}

static gboolean post_act_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
        gint act = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
        mb_type ret;

        if (act) 
        {
            if (_is_enable_dlva_prop() == 1)
            {
                ret = nftool_mbox(g_curwnd, "CONFIRM", STR_DISABLE_DLVA, NFTOOL_MB_OKCANCEL);

                if (ret == NFTOOL_MB_OK) {
                    _disable_dlva_prop();
                }
                else if (ret == NFTOOL_MB_CANCEL) {
                    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, 0);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
                    return FALSE;
                }
            }

            scm_set_gpu_mode_function(GPU_FISHEYE);

            nfui_nfobject_enable(g_mount_obj[0]);
            nfui_nfobject_enable(g_mount_obj[1]);
            nfui_nfobject_enable(g_mount_obj[2]);
            nfui_nfobject_enable(g_viewtype_obj[0]);
            nfui_nfobject_enable(g_viewtype_obj[1]);
            nfui_nfobject_enable(g_viewtype_obj[2]);

            nfui_radio_button_set_toggled(NF_BUTTON(g_mount_obj[g_fisheyedata[ch].mount_mode]), TRUE);
            nfui_radio_button_set_toggled(NF_BUTTON(g_viewtype_obj[g_fisheyedata[ch].default_view]), TRUE);        
        }
        else
        {
            nfui_radio_button_set_toggled(NF_BUTTON(g_mount_obj[g_fisheyedata[ch].mount_mode]), TRUE);
            nfui_radio_button_set_toggled(NF_BUTTON(g_viewtype_obj[g_fisheyedata[ch].default_view]), TRUE);        

            nfui_nfobject_disable(g_mount_obj[0]);
            nfui_nfobject_disable(g_mount_obj[1]);
            nfui_nfobject_disable(g_mount_obj[2]);
            nfui_nfobject_disable(g_viewtype_obj[0]);
            nfui_nfobject_disable(g_viewtype_obj[1]);
            nfui_nfobject_disable(g_viewtype_obj[2]);
        }

        nfui_signal_emit(g_mount_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_mount_obj[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_mount_obj[2], GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_viewtype_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_viewtype_obj[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_viewtype_obj[2], GDK_EXPOSE, TRUE);

        g_fisheyedata[ch].act = act;
        
        if (act) nf_live_fisheye_set_enable(ch);
        else nf_live_fisheye_set_enable(-1);
    }
    
    return FALSE;
}

static gboolean post_mount_mode_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index;
        gint ch;

        ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
		index = nfui_radio_button_get_index((NFBUTTON*)obj);
        g_fisheyedata[ch].mount_mode = index;

        _set_fisheye_video_param(ch);
	}

	return FALSE;
}

static gboolean post_view_type_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index;
        gint ch;

        ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
		index = nfui_radio_button_get_index((NFBUTTON*)obj);
        g_fisheyedata[ch].default_view = index;

        _set_fisheye_video_param(ch);        
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_memmove(g_fisheyedata, g_org_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
        _prvSetDataToObjects(1);

        for (i = 0; i < GUI_CHANNEL_CNT; i++) {
            _set_fisheye_video_param(i);
        }
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (memcmp(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT) == 0) return FALSE;

        g_memmove(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
        DAL_set_camera_itx_fisheye_data_all(g_fisheyedata, GUI_CHANNEL_CNT);

        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        syscam_set_changeflag(1);    
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
        FisheyeSetup_tab_out_handler();
        SystemSetupCam_Destroy(obj);
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
        {
            uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);            
        }
        break;
            
        default :
        break;
    }

    return FALSE;
}

void init_FisheyeSetup_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *tmp_fixed;
    NFOBJECT *obj;

    GSList *slist = NULL;
    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

    //const gchar *strViewType[] = {"ORIGINAL VIEW", "SINGLE VIEW", "QUAD VIEW", "PANORAMA VIEW"};
    const gchar *strViewType[] = {"SINGLE VIEW", "QUAD VIEW", "PANORAMA VIEW"};
    gchar strCh[STRING_SIZE_CAMTITLE+8];
    guint i = 0, j = 0;

    gint pos_x, pos_y;
    guint size_w, size_h;

	pos_x = 27;
	pos_y = 40;


	radio_img[0] = nfui_get_image_from_file((IMG_N_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_RADIO_OFF), NULL);


	memset(g_fisheyedata, 0x00, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
	memset(g_org_fisheyedata, 0x00, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{        
		DAL_get_camera_itx_fisheye_data(&g_fisheyedata[i], i);
	}
    g_memmove(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);

    g_support_mask = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{        
		if (nf_live_fisheye_is_support(i)) g_support_mask |= (1 << i);
	}    

    _sync_fisheye_video_param();


    g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);  


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
    nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 220, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 290, 40); 
	nfui_nfobject_show(obj); 
	nfui_regi_post_event_callback(obj, post_ch_combo_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+230, pos_y);
    g_ch_combo = obj;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		memset(strCh, 0, (STRING_SIZE_CAMTITLE+8));
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);
		var_get_camtitle(&strCh[j], (guint)i);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
	}

    pos_y += 42;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ACTIVATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 220, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 290, 40); 
	nfui_nfobject_show(obj); 
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+230, pos_y);    
	nfui_regi_post_event_callback(obj, post_act_onoff_event_handler);
    g_act_obj = obj;

    nfui_combobox_append_data(NF_COMBOBOX(obj), "OFF");
    nfui_combobox_append_data(NF_COMBOBOX(obj), "ON");

    nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), g_fisheyedata[0].act);

    pos_y += 110;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 220, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_y += 40;

	tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(tmp_fixed, 16*44, 9*44);
	nfui_nfobject_show(tmp_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, tmp_fixed, pos_x, pos_y);
    g_video_fixed = tmp_fixed;

    pos_x = 780;
    pos_y = 40;
   
    obj = nfui_nfimage_new(IMG_TITLE_BG);
    nfui_nfimage_set_text((NFIMAGE*)obj, "MOUNT TYPE");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_y += 60;

    nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+20+(40-size_w)/2, pos_y+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_mount_mode_radio_event_handler);
    g_mount_obj[0] = obj;

    if (g_fisheyedata[0].mount_mode == 0)
        nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

    slist = nfui_radio_button_get_group(NF_BUTTON(obj));

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CEILING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 180, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+60, pos_y);

	obj = nfui_nfimage_new("camera_fisheye_ceiling.png");
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+20, pos_y+42);    

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+250+(40-size_w)/2, pos_y+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_mount_mode_radio_event_handler);
    g_mount_obj[1] = obj;

    if (g_fisheyedata[0].mount_mode == 1)
        nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

    nfui_radio_button_add_group(NF_BUTTON(obj), slist);
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WALL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 180, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+290, pos_y);

	obj = nfui_nfimage_new("camera_fisheye_wall.png");
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+250, pos_y+42);    

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+480+(40-size_w)/2, pos_y+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_mount_mode_radio_event_handler);
    g_mount_obj[2] = obj;

    if (g_fisheyedata[0].mount_mode == 2)
        nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

    nfui_radio_button_add_group(NF_BUTTON(obj), slist);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUND", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 180, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+520, pos_y);

	obj = nfui_nfimage_new("camera_fisheye_ground.png");
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+480, pos_y+42);    

    pos_y += 280; 

    obj = nfui_nfimage_new(IMG_TITLE_BG);
    nfui_nfimage_set_text((NFIMAGE*)obj, "VIEW TYPE");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);    

    pos_y += 60; 

    for (i = 0; i < 3; i++)
    {
        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+20, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_view_type_radio_event_handler);
        g_viewtype_obj[i] = obj;

		if (i == g_fisheyedata[0].default_view)
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

        if (i == 0) {
            slist = nfui_radio_button_get_group(NF_BUTTON(obj));
        }
        else {
            nfui_radio_button_add_group(NF_BUTTON(obj), slist);
        }      

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strViewType[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 220, 40);  
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+60, pos_y);       

        pos_y += 42;   

#ifdef GUI_32CH_SUPPORT
        if (i > 0) {
            nfui_nfobject_hide(g_viewtype_obj[i]);
            nfui_nfobject_hide(obj);
        }
#endif        
    }

    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);

    _prvSetDataToObjects(0);
}

void FisheyeSetup_Show_Preview()
{
	_start_preview(0);
    _show_video_obj();
}

gboolean FisheyeSetup_tab_out_handler()
{
    mb_type ret;
    gint i;
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

    _hide_video_obj(); 
    _stop_preview();

    if (!memcmp(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT)) return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
        g_memmove(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
        DAL_set_camera_itx_fisheye_data_all(g_fisheyedata, GUI_CHANNEL_CNT);
		syscam_set_changeflag(1);
	}
	else if (ret == NFTOOL_MB_CANCEL)
	{
        g_memmove(g_fisheyedata, g_org_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
        _prvSetDataToObjects(0);
        
        for (i = 0; i < GUI_CHANNEL_CNT; i++) {
            _set_fisheye_video_param(i);
        }
	}

    return FALSE;
}

gboolean FisheyeSetup_tab_in_handler()
{
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
    gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) {        
		DAL_get_camera_itx_fisheye_data(&g_fisheyedata[i], i);
	}

    if (memcmp(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT) != 0)
    {
        g_memmove(g_org_fisheyedata, g_fisheyedata, sizeof(CamItxFisheyeData)*GUI_CHANNEL_CNT);
        _prvSetDataToObjects(ch);
    }

	_start_preview(ch);
    _show_video_obj();
    return FALSE;
}

