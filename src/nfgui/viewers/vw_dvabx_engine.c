/*
 * vw_dvabx_rcol.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "vw_dvabx_prop_internal.h"
#include "vw_dvabx_component.h"
#include "dvaa_itx.h"

#include "scm.h"
#include "nf_api_dlva.h"
#include "nf_sysman.h"



////////////////////////////////////////////////////////////
//
// private data types
//
#if 0
#define STR_NVRBOX_AIENGINE_TITLE "Configuring AI BOX from NVR"
#define STR_NVRBOX_AIENGINE_HELP "Algorithms mutually supported by the NVR and AI BOX can be easily configured by activating the rule engine from the NVR."

#define STR_NVRCAM_AIENGINE_TITLE "Configuring AI CAM from NVR"
#define STR_NVRCAM_AIENGINE_HELP "Algorithms mutually supported by the NVR and AI CAM can be easily configured by activating the rule engine from the NVR."

#define STR_BOX_AIENGINE_TITLE "Configuring AI BOX from AI BOX WebUI"
#define STR_BOX_AIENGINE_HELP1 "Advanced AI BOX algorithm configuration can be performed from AI BOX web UI."
#define STR_BOX_AIENGINE_HELP2 "First, set ACTIVATION above to OFF. Then, navigate to AI BOX web address listed below (no plug-in required)."
#define STR_BOX_AIENGINE_HELP3 "Create AI Action Rules and setup NVR/DVR action item to transmit metadata."

#define STR_CAM_AIENGINE_TITLE "Configuring AI CAM from AI CAM WebUI"
#define STR_CAM_AIENGINE_HELP1 "Advanced AI CAM algorithm configuration can be performed from AI CAM web UI."
#define STR_CAM_AIENGINE_HELP2 "First, set ACTIVATION above to OFF. Then, navigate to AI CAM web address listed below (no plug-in required)."
#define STR_CAM_AIENGINE_HELP3 "Create AI Action Rules and setup NVR/DVR action item to transmit metadata."
#else
#define STR_NVRBOX_AIENGINE_TITLE "Configuring AI BOX from NVR"
#define STR_NVRBOX_AIENGINE_HELP "Algorithms mutually supported by the NVR and AI BOX can be easily configured by activating the rule engine from the NVR."

// CALIR - 1
#define STR_NVRCAM_AIENGINE_TITLE "Configuring AI CAM from Recorder"
#define STR_NVRCAM_AIENGINE_HELP "Algorithms mutually supported by the Recorder and AI CAM can be easily configured by activating the rule engine from the Recorder."

// CALIR - 2
#define STR_NVRCAM_AIENGINE_HELP2 "Additionally, Analytics can be setup using the Camera Web Interface"

// AI BOX
#define STR_BOX_AIENGINE_TITLE "Configuring AI BOX from AI BOX WebUI"
#define STR_BOX_AIENGINE_HELP1 "Advanced AI BOX algorithm configuration can be performed from AI BOX web UI."
#define STR_BOX_AIENGINE_HELP2 "Navigate to the AI BOX web address listed below."
#define STR_BOX_AIENGINE_HELP3 "Create AI Action Rules and setup NVR/DVR action item to transmit metadata."

// PRO/ULTRA CAM
#define STR_CAM_AIENGINE_TITLE "Configuring AI CAM from AI CAM WebUI"
#define STR_CAM_AIENGINE_HELP1 "Advanced AI CAM algorithm configuration can be performed from AI CAM web UI."
#define STR_CAM_AIENGINE_HELP2 "Navigate to the Camera Web address listed below."
#define STR_CAM_AIENGINE_HELP3 "Create AI Action Rules and setup NVR/DVR action item to transmit metadata."
#endif

#define MAX_ALGO_CNT    16

typedef enum EMOSD_OBJ_E {
    EMOSD_DISPLAY_MODE = 0,
    EMOSD_OBJECT_COLOR,
    EMOSD_RULE_COLOR,
    EMOSD_EVENT_COLOR,
    EMOSD_LINE_WIDTH,
    EMOSD_LINE_TRANSPARENCY,
    
    EMOSD_OBJ_CNT
};

typedef enum EMOSD_OBJ_TYPE_E {
    EMOSD_OBJ_TYPE_PERSON = 0,
    EMOSD_OBJ_TYPE_CAR,
    EMOSD_OBJ_TYPE_BIKE,
    
    EMOSD_OBJ_TYPE_CNT
};

typedef struct _ALGORITHM_INFO
{
	char value[50];
	char text[100];
    char type[50];
} ALGORITHM_INFO;




////////////////////////////////////////////////////////////
//
// private variable
//

static AiAnalysisActData *g_org_analysis_data;
static AiAnalysisActData *g_analysis_data;

static ALGORITHM_INFO g_algorithm_info[MAX_ALGO_CNT];
static guint g_cur_channel = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_parent = NULL;
static NFOBJECT *g_curfixed = NULL;
static NFOBJECT *g_r0_fixed = NULL;
static NFOBJECT *g_r1_fixed = NULL;
static NFOBJECT *g_algorithm_combo_obj = NULL;

static NFOBJECT *g_nvrbox_title_label = NULL;
static NFOBJECT *g_nvrbox_help_label = NULL;

static NFOBJECT *g_nvrcam_title_label = NULL;
static NFOBJECT *g_nvrcam_help_label = NULL;
static NFOBJECT *g_nvrcam_help2_label = NULL;

static NFOBJECT *g_aibox_title_label = NULL;
static NFOBJECT *g_aibox_group_fixed = NULL;
static NFOBJECT *g_aibox_addr_label1 = NULL;
static NFOBJECT *g_aibox_addr_label2 = NULL;

static NFOBJECT *g_aicam_title_label = NULL;
static NFOBJECT *g_aicam_group_fixed = NULL;
static NFOBJECT *g_aicam_addr_label1 = NULL;
static NFOBJECT *g_aicam_addr_label2 = NULL;

static NFOBJECT *g_aicam_emosd_title_label = NULL;
static NFOBJECT *g_aicam_emosd_fixed = NULL;
static NFOBJECT *g_emosd_lt_slider = NULL;
static NFOBJECT *g_emosd_lt_spin = NULL;
static NFOBJECT *g_emosd_obj_type[EMOSD_OBJ_TYPE_CNT];


////////////////////////////////////////////////////////////
//
// private interfaces 
//

// vw_dvabx_engine.c
// vw_dvabox_setting_popup.c

static gint _supported_algorithm_type(gchar *algorithm_type)
{
    if (!algorithm_type) return 0;

    if (strlen(algorithm_type) == 0) return 1;  // NONE

    if (1)//((ivsc.vendor_code == 28) || (ivsc.vendor_code == 128))
    {
        if (strcmp(algorithm_type, ALGOTYPE_DETECTOR) == 0) return 1;
    }
    else
    {
        if (strcmp(algorithm_type, ALGOTYPE_DETECTOR) == 0) return 1;
        if (strcmp(algorithm_type, ALGOTYPE_FACE) == 0) return 1;
        if (strcmp(algorithm_type, ALGOTYPE_PLATENO) == 0) return 1;
    }

    return 0;
}

static gchar *_get_algorithm_text(gchar *algorithm_value)
{
    gint i;

    if (!algorithm_value) return 0;
    if (!strlen(algorithm_value)) return 0;

    for (i = 0; i < MAX_ALGO_CNT; i++)
    {
        if (!strcmp(g_algorithm_info[i].value, algorithm_value)) {
            return g_algorithm_info[i].text;
        }
    }
    return 0;
}

static gchar *_get_algorithm_value(gchar *algorithm_text)
{
    gint i;

    if (!algorithm_text) return 0;
    if (!strlen(algorithm_text)) return 0;

    for (i = 0; i < MAX_ALGO_CNT; i++)
    {
        if (!strcmp(g_algorithm_info[i].text, algorithm_text)) {
            return g_algorithm_info[i].value;
        }
    }
    return 0;
}

static gchar *_get_algorithm_type(gchar *algorithm_value, gchar *algorithm_text)
{
    gint i;

    for (i = 0; i < MAX_ALGO_CNT; i++)
    {
        if ((algorithm_value) && (strlen(algorithm_value)) && (!strcmp(g_algorithm_info[i].value, algorithm_value))) {
            return g_algorithm_info[i].type;
        }

        if ((algorithm_text) &&(strlen(algorithm_text)) && (!strcmp(g_algorithm_info[i].text, algorithm_text))) {
            return g_algorithm_info[i].type;
        }        
    }
    return 0;
}

static gint _get_aibox_selected_algorithm(gint ch, gchar *value, gchar *text, gchar *type)
{
    aibox_algorithm_name *aibox_algorithm;

    guint aibox_addr;
    gint i;

    aibox_addr = g_analysis_data[ch].dvabox_ipaddr;
    aibox_algorithm = nf_api_get_algorithms_all_ch(aibox_addr);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(g_org_analysis_data[i].algorithm, 0x00, sizeof(g_org_analysis_data[i].algorithm));
        memset(g_analysis_data[i].algorithm, 0x00, sizeof(g_analysis_data[i].algorithm));

        if ((g_analysis_data[i].dvabox_active) && strlen(aibox_algorithm[i].value) && _supported_algorithm_type(aibox_algorithm[i].algo_type))
        {
            g_snprintf(g_org_analysis_data[i].algorithm, sizeof(g_org_analysis_data[i].algorithm), "%s", aibox_algorithm[i].value);
            g_snprintf(g_analysis_data[i].algorithm, sizeof(g_analysis_data[i].algorithm), "%s", aibox_algorithm[i].value);

            if (i == ch)
            {
                g_snprintf(value, 255, "%s", aibox_algorithm[ch].value);
                g_snprintf(text, 255, "%s", aibox_algorithm[ch].text);
                g_snprintf(type, 255, "%s", aibox_algorithm[ch].algo_type);
            }
        }
    }

    free(aibox_algorithm);
    return 0;
}

static gint _get_aibox_capability_algorithm_list(gint ch, guint aibox_addr)
{
    ai_capa_t *algorithm_capa;
    gint algo_cnt = 0;
    gint i;

    memset(g_algorithm_info, 0x00, sizeof(ALGORITHM_INFO)*MAX_ALGO_CNT);

    nfui_combobox_remove_all(NF_COMBOBOX(g_algorithm_combo_obj));

    // g_message("%s, %d, ch:%d, aibox_addr:%u", __FUNCTION__, __LINE__, ch, aibox_addr);
    algorithm_capa = nf_api_get_capability(aibox_addr, ch);
    // g_message("%s, %d, algorithm_capa:%p", __FUNCTION__, __LINE__, algorithm_capa);
    if (algorithm_capa)
    {
        for (i = 0; i < algorithm_capa->algorithm_count; i++)
        {
            if (_supported_algorithm_type(algorithm_capa->algorithm_list[i].algo_type))
            {
                // g_message("%s, %d, i:%d, text:%s, value:%s", __FUNCTION__, __LINE__, i, algorithm_capa->algorithm_list[i].text, algorithm_capa->algorithm_list[i].value);
                nfui_combobox_append_data(NF_COMBOBOX(g_algorithm_combo_obj), algorithm_capa->algorithm_list[i].text);

                snprintf(g_algorithm_info[algo_cnt].text, sizeof(g_algorithm_info[algo_cnt].text)-1, "%s", algorithm_capa->algorithm_list[i].text);
                snprintf(g_algorithm_info[algo_cnt].value, sizeof(g_algorithm_info[algo_cnt].value)-1, "%s", algorithm_capa->algorithm_list[i].value);
                snprintf(g_algorithm_info[algo_cnt].type, sizeof(g_algorithm_info[algo_cnt].type)-1, "%s", algorithm_capa->algorithm_list[i].algo_type);

                algo_cnt++;
            }
            if (algo_cnt >= MAX_ALGO_CNT) break;
        }
        nf_api_capability_free(algorithm_capa);
    }
}

static gint _get_aicam_selected_algorithm(gint ch, gchar *value, gchar *text, gchar *type)
{
    ai_license_data aicamera_algorithm;

    memset(&aicamera_algorithm, 0x00, sizeof(ai_license_data));
    memcpy(&aicamera_algorithm, nf_api_selected_aicamera_algorithm_data(ch), sizeof(ai_license_data));

    memset(g_org_analysis_data[ch].algorithm, 0x00, sizeof(g_org_analysis_data[ch].algorithm));
    memset(g_analysis_data[ch].algorithm, 0x00, sizeof(g_analysis_data[ch].algorithm));

    if (strlen(aicamera_algorithm.value) && _supported_algorithm_type(aicamera_algorithm.algo_type)) 
    {
        g_snprintf(g_org_analysis_data[ch].algorithm, sizeof(g_org_analysis_data[ch].algorithm), "%s", aicamera_algorithm.value);
        g_snprintf(g_analysis_data[ch].algorithm, sizeof(g_analysis_data[ch].algorithm), "%s", aicamera_algorithm.value);

        g_snprintf(value, 255, "%s", aicamera_algorithm.value);
        g_snprintf(text, 255, "%s", aicamera_algorithm.name);
        g_snprintf(type, 255, "%s", aicamera_algorithm.algo_type);
    }

    return 0;
}

static gint _get_aicam_capability_algorithm_list(gint ch)
{
    ai_capa_t *algorithm_capa;
    gint algo_cnt = 0;
    gint i;

    memset(g_algorithm_info, 0x00, sizeof(ALGORITHM_INFO)*MAX_ALGO_CNT);

    nfui_combobox_remove_all(NF_COMBOBOX(g_algorithm_combo_obj));

    // g_message("%s, %d, ch:%d", __FUNCTION__, __LINE__, ch);
    algorithm_capa = nf_api_get_capability(0, ch);
    // g_message("%s, %d, algorithm_info:%p", __FUNCTION__, __LINE__, algorithm_capa);

    if (algorithm_capa)
    {
        for (i = 0; i < algorithm_capa->algorithm_count; i++)
        {
            if (_supported_algorithm_type(algorithm_capa->algorithm_list[i].algo_type))
            {
                // g_message("%s, %d, i:%d, text:%s, value:%s", __FUNCTION__, __LINE__, i, algorithm_capa->algorithm_list[i].text, algorithm_capa->algorithm_list[i].value);
                nfui_combobox_append_data(NF_COMBOBOX(g_algorithm_combo_obj), algorithm_capa->algorithm_list[i].text);

                snprintf(g_algorithm_info[algo_cnt].text, sizeof(g_algorithm_info[algo_cnt].text)-1, "%s", algorithm_capa->algorithm_list[i].text);
                snprintf(g_algorithm_info[algo_cnt].value, sizeof(g_algorithm_info[algo_cnt].value)-1, "%s", algorithm_capa->algorithm_list[i].value);
                snprintf(g_algorithm_info[algo_cnt].type, sizeof(g_algorithm_info[algo_cnt].type)-1, "%s", algorithm_capa->algorithm_list[i].algo_type);

                algo_cnt++;
            }
            if (algo_cnt >= MAX_ALGO_CNT) break;
        }
        nf_api_capability_free(algorithm_capa);
    }
}

static gint _is_possible_aibox_owner(DVA_COMPONENT_DATA_T *component_data)
{
    gchar nvr_owner[32];

    if (g_analysis_data[g_cur_channel].dvacam_active) return 1;
    if (!g_analysis_data[g_cur_channel].dvabox_active) return 0;

    memset(nvr_owner, 0x00, sizeof(nvr_owner));
    nf_api_get_nvr_owner(nvr_owner);
    // g_message("%s, %d, nvr_owner:%s, aibox_owner:%s", __FUNCTION__, __LINE__, nvr_owner, component_data->aibox_owner);

    if (strcmp(component_data->aibox_owner, nvr_owner) != 0) return 0;

    return 1;
}

static gint _is_possible_aibox_algorithm(DVA_COMPONENT_DATA_T *component_data, gchar *algorithm_value)
{
    ai_capa_t *algorithm_info;
    gint i;

    if (!g_analysis_data[g_cur_channel].dvabox_active) return 1;

    algorithm_info = nf_api_get_capability(component_data->aibox_addr, g_cur_channel);
    if (!algorithm_info) return 0;

    for (i = 0; i < algorithm_info->algorithm_count; i++)
    {
        if (strcmp(algorithm_value, algorithm_info->algorithm_list[i].value) == 0)
        {
            if (algorithm_info->algorithm_list[i].disabled) {
                nf_api_capability_free(algorithm_info);
                return 0;
            }
        }
    }

    nf_api_capability_free(algorithm_info);
    return 1;
}

static gint _check_aibox_state_alive(NFOBJECT *algorithm_combo)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;

    top = nfui_nfobject_get_top(algorithm_combo);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    if (nf_api_aibox_server_state_is_alive(component_data->aibox_addr, 3) != 0) return 0;

    nftool_mbox(g_curwnd, "ERROR", "AI Box connection failed.\nTry SCAN again and reset the connection to the AI Box.\nIf the AI Box does not appear in the list, check the AI Box status.", NFTOOL_MB_OK);

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)algorithm_combo, 0);
    if (_get_algorithm_text(component_data->algorithm_value)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)algorithm_combo, _get_algorithm_text(component_data->algorithm_value));
    else if (_get_algorithm_value(component_data->algorithm_text)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)algorithm_combo, component_data->algorithm_text);
    nfui_signal_emit(algorithm_combo, GDK_EXPOSE, TRUE);
    return -1;
}

static gint _check_possible_aibox_owner(NFOBJECT *algorithm_combo, gchar *algorithm_value)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    gchar aibox_owner[32];

    mb_type ret;
    gint ret_code = DLVA_API_RET_OK;

    top = nfui_nfobject_get_top(algorithm_combo);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    if (_is_possible_aibox_owner(component_data) != 0) return 0;

    memset(aibox_owner, 0x00, sizeof(aibox_owner));
    nf_api_get_aibox_owner(component_data->aibox_addr, aibox_owner);
    // g_message("%s, %d, aibox_addr:%u, nvr_owner:%s, aibox_owner:%s", __FUNCTION__, __LINE__, component_data->aibox_addr, component_data->aibox_owner, aibox_owner);

    ret = nftool_mbox(g_curwnd, "CONFIRM", "All AI BOX connection settings will be initialized.\nDo you want to change the AI BOX setting to what is set on the recorder?", NFTOOL_MB_OKCANCEL);
    if (ret == NFTOOL_MB_OK) 
    {
        nf_api_set_aibox_owner(component_data->aibox_addr);
        ret_code = nf_api_aibox_set_video_stream(g_cur_channel, component_data->aibox_addr, algorithm_value);
        // g_message("%s, %d, ch:%d, ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, g_cur_channel, component_data->aibox_addr, ret_code);

        vw_dvabox_setting_popup_open(g_curwnd, g_cur_channel, g_analysis_data);

        memset(component_data->algorithm_value, 0x00, sizeof(component_data->algorithm_value));
        memset(component_data->algorithm_text, 0x00, sizeof(component_data->algorithm_text));
        memset(component_data->algorithm_type, 0x00, sizeof(component_data->algorithm_type));
        if (strlen(g_analysis_data[g_cur_channel].algorithm)) {
            g_snprintf(component_data->algorithm_value, sizeof(component_data->algorithm_value)-1, "%s", g_analysis_data[g_cur_channel].algorithm);
            g_snprintf(component_data->algorithm_text, sizeof(component_data->algorithm_text)-1, "%s", _get_algorithm_text(component_data->algorithm_value));
            g_snprintf(component_data->algorithm_type, sizeof(component_data->algorithm_type)-1, "%s", _get_algorithm_type(component_data->algorithm_value, 0));
        }
    }

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)algorithm_combo, 0);
    if (_get_algorithm_text(component_data->algorithm_value)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)algorithm_combo, _get_algorithm_text(component_data->algorithm_value));
    else if (_get_algorithm_value(component_data->algorithm_text)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)algorithm_combo, component_data->algorithm_text);

    nfui_user_signal_emit(g_curfixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    vw_dvabx_video_component_sync_preview(component_data->video_fixed);
    vw_dvabx_video_component_expose(component_data->video_fixed);
    return -1;
}

static gint _check_possible_aibox_algorithm(NFOBJECT *algorithm_combo, gchar *algorithm_value)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    gchar aibox_owner[32];

    top = nfui_nfobject_get_top(algorithm_combo);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    if (_is_possible_aibox_algorithm(component_data, algorithm_value)) return 0;

    nftool_mbox(g_curwnd, "WARNING", "The algorithm you set is exceeded the performance of AI Box.\nAlgorithm can be set after deactivating or changing the algorithm of another channel.", NFTOOL_MB_OK);

    vw_dvabox_setting_popup_open(g_curwnd, g_cur_channel, g_analysis_data);

    memset(component_data->algorithm_value, 0x00, sizeof(component_data->algorithm_value));
    memset(component_data->algorithm_text, 0x00, sizeof(component_data->algorithm_text));
    memset(component_data->algorithm_type, 0x00, sizeof(component_data->algorithm_type));
    if (strlen(g_analysis_data[g_cur_channel].algorithm)) {
        g_snprintf(component_data->algorithm_value, sizeof(component_data->algorithm_value)-1, "%s", g_analysis_data[g_cur_channel].algorithm);
        g_snprintf(component_data->algorithm_text, sizeof(component_data->algorithm_text)-1, "%s", _get_algorithm_text(component_data->algorithm_value));
        g_snprintf(component_data->algorithm_type, sizeof(component_data->algorithm_type)-1, "%s", _get_algorithm_type(component_data->algorithm_value, 0));        
    }

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)algorithm_combo, 0);
    if (_get_algorithm_text(component_data->algorithm_value)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)algorithm_combo, _get_algorithm_text(component_data->algorithm_value));
    else if (_get_algorithm_value(component_data->algorithm_text)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)algorithm_combo, component_data->algorithm_text);

    nfui_user_signal_emit(g_curfixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    vw_dvabx_video_component_sync_preview(component_data->video_fixed);
    vw_dvabx_video_component_expose(component_data->video_fixed);
    return -1;
}

static gint _get_emosd_obj_type_string(gchar *buf, gint len)
{
    gint init = 0;
    
    memset(buf, 0x00, len);
    
    if (nfui_check_button_get_active(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_PERSON]))) {
        g_sprintf(buf, "person");
        init = 1;
    }
    
    if (nfui_check_button_get_active(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_CAR]))) {
        if (init == 0) g_sprintf(buf, "car");
        else g_sprintf(buf, "%s,car", buf);
        
        init = 1;
    }
    
    if (nfui_check_button_get_active(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_BIKE]))) {
        if (init == 0) g_sprintf(buf, "bike");
        else g_sprintf(buf, "%s,bike", buf);
        
        init = 1;
    }
    
    return 0;
}

////////////////////////////////////////////////////////////
//
// handler
//
static gboolean post_group_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;
    GdkDrawable *drawable = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;		
    gint off_x, off_y;
		
	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, off_x, off_y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_nvr_act_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;
        ITX_DVARULE_PROP prop;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_rule_prop(dvaaid, &prop);
        prop.en_engine = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        dvaa_itx_detector_set_rule_prop(dvaaid, &prop);

        component_data->en_engine = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        nfui_user_signal_emit(g_curfixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);
        vw_dvabx_video_component_expose(component_data->video_fixed);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, component_data->en_engine);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_nvr_algorithm_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;

        gchar aibox_owner[32];

        gchar *algorithm_text;
        gchar *algorithm_value;
        gchar *algorithm_type;
        gint ret_code = DLVA_API_RET_OK;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        algorithm_text = nfui_combobox_get_value((NFCOMBOBOX*)obj);
        algorithm_value = _get_algorithm_value(algorithm_text);
        algorithm_type = _get_algorithm_type(algorithm_value, 0);

        if (!g_analysis_data[g_cur_channel].dvabox_active) return FALSE;
        if (_check_aibox_state_alive(obj) == -1) return FALSE;
        if (_check_possible_aibox_owner(obj, algorithm_value) == -1) return FALSE;
        if (_check_possible_aibox_algorithm(obj, algorithm_value) == -1) return FALSE;

        if (algorithm_value) ret_code = nf_api_aibox_set(g_cur_channel, component_data->aibox_addr, algorithm_value);
        else ret_code = nf_api_aibox_set(g_cur_channel, component_data->aibox_addr, "");
        // g_message("%s, %d, ch:%d, ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, g_cur_channel, component_data->aibox_addr, ret_code);

        memset(component_data->algorithm_value, 0x00, sizeof(component_data->algorithm_value));
        memset(component_data->algorithm_text, 0x00, sizeof(component_data->algorithm_text));
        memset(component_data->algorithm_type, 0x00, sizeof(component_data->algorithm_type));
        memset(g_analysis_data[g_cur_channel].algorithm, 0x00, sizeof(g_analysis_data[g_cur_channel].algorithm));
        if (algorithm_value) {
            g_snprintf(component_data->algorithm_value, sizeof(component_data->algorithm_value)-1, "%s", algorithm_value);
            g_snprintf(component_data->algorithm_text, sizeof(component_data->algorithm_text)-1, "%s", algorithm_text);        
            g_snprintf(component_data->algorithm_type, sizeof(component_data->algorithm_type)-1, "%s", algorithm_type);
            g_snprintf(g_analysis_data[g_cur_channel].algorithm, sizeof(g_analysis_data[g_cur_channel].algorithm)-1, "%s", algorithm_value);        
        }

        nfui_user_signal_emit(g_curfixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);
        vw_dvabx_video_component_expose(component_data->video_fixed);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gint i;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, 0);

        for (i = 0; i < MAX_ALGO_CNT; i++)
        {
            if (_get_algorithm_text(component_data->algorithm_value)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, _get_algorithm_text(component_data->algorithm_value));
            else if (_get_algorithm_value(component_data->algorithm_text)) nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, component_data->algorithm_text);
        }

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) {
            nfui_nfobject_enable(obj);
        } 
        else {
            nfui_nfobject_disable(obj);
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_rule_editbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gchar *algo_type;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);        

        algo_type = _get_algorithm_type(component_data->algorithm_value, 0);

        nfui_nfobject_hide(g_parent);
        if (strcmp(algo_type, "fr") == 0) _dvabx_face_rcol_show_page(0, 1);
        else if (strcmp(algo_type, "lpr") == 0) _dvabx_plateno_rcol_show_page(0, 1);
        else _dvabx_rcol_show_page(0, 1);
        nfui_make_key_hierarchy(g_curwnd);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine && strlen(component_data->algorithm_value)) {
            nfui_nfobject_enable(obj);
        } 
        else {
            nfui_nfobject_disable(obj);
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_calibration_editbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gchar *algo_type;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        algo_type = _get_algorithm_type(component_data->algorithm_value, 0);

        if (strcmp(algo_type, ALGOTYPE_DETECTOR) != 0) return FALSE;

        nfui_nfobject_hide(g_parent);
        _dvabx_rcol_show_page(1, 1);
        nfui_make_key_hierarchy(g_curwnd);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) {
            if (strcmp(component_data->algorithm_type, ALGOTYPE_DETECTOR) == 0)
                nfui_nfobject_enable(obj);
            else 
                nfui_nfobject_disable(obj);
        } 
        else {
            nfui_nfobject_disable(obj);
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_engine_option_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gchar *algo_type;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_nfobject_hide(g_parent);
        _dvabx_rcol_show_engine_option_page(1);
        nfui_make_key_hierarchy(g_curwnd);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) {
            if (strcmp(component_data->algorithm_type, ALGOTYPE_DETECTOR) == 0)
                nfui_nfobject_enable(obj);
            else 
                nfui_nfobject_disable(obj);
        } 
        else {
            nfui_nfobject_disable(obj);
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_osd_option_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_hide(g_parent);
        _dvabx_rcol_show_osd_option_page(1);
        nfui_make_key_hierarchy(g_curwnd);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license) {
            nfui_nfobject_enable(obj);
        } 
        else {
            nfui_nfobject_disable(obj);
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_emosd_display_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        conf.display_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);

        nfui_user_signal_emit(g_curfixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;
        
        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);

        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), conf.display_mode);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_emosd_object_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;
    
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        conf.object_color = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        
        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), conf.object_color);
        
        if (conf.display_mode == 1) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_emosd_rule_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;
    
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        conf.rule_color = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        
        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), conf.rule_color);
        
        if (conf.display_mode == 1) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_emosd_event_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;
    
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        conf.event_color = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        
        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), conf.event_color);
        
        if (conf.display_mode == 1 || conf.display_mode == 2) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_emosd_line_width_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;
    
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        conf.line_width = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        
        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), conf.line_width);
        
        if (conf.display_mode == 1 || conf.display_mode == 2) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_line_transparency_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_BUTTON_RELEASE :
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :
			{
                DVAAID dvaaid;
                ITX_DVAEOSD_CONF conf;

                dvaaid = dvaa_get_dvaaid(g_cur_channel);
                dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
                conf.line_transparency = cw_slider_get_value((CWSLIDER*)obj);
                dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);

				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_emosd_lt_spin, conf.line_transparency);
                nfui_signal_emit(g_emosd_lt_spin, GDK_EXPOSE, TRUE);
			}
			break;
            
		case NFEVENT_DVA_COMPONENT_DATA_SYNC :
        {
            DVAAID dvaaid;
            ITX_DVAEOSD_CONF conf;

            dvaaid = dvaa_get_dvaaid(g_cur_channel);
            dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
            
            cw_slider_set_value((CWSLIDER*)obj, conf.line_transparency);
            
            if (conf.display_mode == 1 || conf.display_mode == 2) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
            
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
        break;

		default : 
			break;
	}
    
	return FALSE;
}

static gboolean post_line_transparency_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) 
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        conf.line_transparency = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);
		
		cw_slider_set_value((CWSLIDER*)g_emosd_lt_slider, conf.line_transparency);
		nfui_signal_emit(g_emosd_lt_slider, GDK_EXPOSE, TRUE);
	}
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        
        nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, conf.line_transparency);
        
        if (conf.display_mode == 1 || conf.display_mode == 2) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
	return FALSE;
}

static gboolean post_emosd_object_type_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);
        _get_emosd_obj_type_string(conf.object_type, sizeof(conf.object_type));
        dvaa_itx_detector_set_eosd_conf(dvaaid, &conf);

    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        DVAAID dvaaid;
        ITX_DVAEOSD_CONF conf;

        dvaaid = dvaa_get_dvaaid(g_cur_channel);
        dvaa_itx_detector_get_eosd_conf(dvaaid, &conf);

        if (strstr(conf.object_type, "person")) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_PERSON]), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_PERSON]), FALSE);
        }
        
        if (strstr(conf.object_type, "car")) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_CAR]), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_CAR]), FALSE);
        }
        
        if (strstr(conf.object_type, "bike")) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_BIKE]), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_emosd_obj_type[EMOSD_OBJ_TYPE_BIKE]), FALSE);
        }
                
        if (conf.display_mode == 1) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _dvabx_engine_page(NFOBJECT *parent)
{
	NFOBJECT *fixed;
    NFOBJECT *group_fixed;
    NFOBJECT *eosd_fixed;
	NFOBJECT *obj;

    gchar lfBuf[4096];
    guint size_w, size_h; 
    gint pos_x, pos_y;
	guint i;
    
    gchar *strDisplayMode[] = {"OFF", "ON(ALWAYS)", "ON(EVENT)"};
    gchar *strColor[] = {"BLACK", "WHITE", "MAGENTA", "ORANGE", "YELLOW", "RED", "BLUE", "GREEN"};
    gchar *strLineWidth[] = {"1", "2", "3"};


    g_parent = parent;
    g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);


    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, parent->width, parent->height);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 0, 0);
    g_curfixed = fixed;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, parent->width, parent->height);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_hide(fixed);
    nfui_nffixed_put((NFFIXED*)g_curfixed, fixed, 0, 0);
    g_r0_fixed = fixed;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, parent->width, parent->height);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)g_curfixed, fixed, 0, 0);
    g_r1_fixed = fixed;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_NVRBOX_AIENGINE_TITLE, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_set_size(obj, 500, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_r0_fixed, obj, 0, 0);
    g_nvrbox_title_label = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_NVRCAM_AIENGINE_TITLE, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_set_size(obj, 500, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)g_r0_fixed, obj, 0, 0);
    g_nvrcam_title_label = obj;

    group_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(group_fixed, parent->width, 320);
    nfui_nfobject_modify_bg(group_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(group_fixed);
    nfui_nffixed_put((NFFIXED*)g_r0_fixed, group_fixed, 0, 46);
    nfui_regi_post_event_callback(group_fixed, post_group_fixed_event_cb);

    pos_x = 10;
    pos_y = 12;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A || (ivsc.vendor_code != 28 && ivsc.vendor_code != 128))
        nfutil_get_line_feed_string(lookup_string(STR_NVRBOX_AIENGINE_HELP), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_NORMAL), lfBuf, sizeof(lfBuf));
    else
        nfutil_get_line_feed_string(lookup_string(STR_NVRCAM_AIENGINE_HELP), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_NORMAL), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, 70);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    g_nvrbox_help_label = obj;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_NVRCAM_AIENGINE_HELP), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_NORMAL), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*2) size_h = 26*2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    g_nvrcam_help_label = obj;

    pos_y += size_h;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_NVRCAM_AIENGINE_HELP2), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*2) size_h = 26*2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    g_nvrcam_help2_label = obj;

    pos_y += size_h + 16;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ACTIVATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 230, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, parent->width-20-230, 40);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+230, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_nvr_act_combo_event_handler);

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "OFF");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "ON");

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALGORITHM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 230, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, parent->width-20-230, 40);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+230, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_nvr_algorithm_combo_event_handler);
    g_algorithm_combo_obj = obj;

    pos_y += 48;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RULE ENGINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("EDIT", parent->width-20-300);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_rule_editbutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+300, pos_y);

    pos_y += 42;

    if (ivsc.dfunc.ai.support_calibration)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CALIBRATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, 300, 40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
        nfui_nfobject_show(obj);

        obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("EDIT", parent->width-20-300);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
        nfui_regi_post_event_callback(obj, post_calibration_editbutton_event_handler);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+300, pos_y);

        pos_y += 42;
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ENGINE OPTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("EDIT", parent->width-20-300);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_engine_option_button_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x+300, pos_y);

    obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("OSD OPTION", 260);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_osd_option_button_event_handler);
    // nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_r0_fixed, obj, g_r0_fixed->width-20-260, 56+group_fixed->height);

    
    //EMBEDDED OSD CONTENTS --------------------------------
    
    pos_x = 0;
    pos_y = 56+group_fixed->height;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Configure AI Embedded OSD from Recorder", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_set_size(obj, 500, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_r0_fixed, obj, pos_x, pos_y);
    g_aicam_emosd_title_label = obj;
    
    pos_y += 46;
    
    eosd_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(eosd_fixed, parent->width, 350);
    nfui_nfobject_modify_bg(eosd_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(eosd_fixed);
    nfui_nffixed_put((NFFIXED*)g_r0_fixed, eosd_fixed, pos_x, pos_y);
    nfui_regi_post_event_callback(eosd_fixed, post_group_fixed_event_cb);
    g_aicam_emosd_fixed = eosd_fixed;

    pos_x = 10;
    pos_y = 12;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);  

    obj = nfui_combobox_new(strDisplayMode, 3, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, eosd_fixed->width-20-270, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270, pos_y);    
    nfui_regi_post_event_callback(obj, post_emosd_display_mode_event_handler);

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OBJECT COLOR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);  

    obj = nfui_combobox_new(strColor, 8, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, eosd_fixed->width-20-270, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270, pos_y);    
    nfui_regi_post_event_callback(obj, post_emosd_object_color_event_handler);

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RULE COLOR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);  

    obj = nfui_combobox_new(strColor, 8, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, eosd_fixed->width-20-270, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270, pos_y);    
    nfui_regi_post_event_callback(obj, post_emosd_rule_color_event_handler);

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT COLOR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);  

    obj = nfui_combobox_new(strColor, 8, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, eosd_fixed->width-20-270, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270, pos_y);    
    nfui_regi_post_event_callback(obj, post_emosd_event_color_event_handler);

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINE WIDTH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);  

    obj = nfui_combobox_new(strLineWidth, 3, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, eosd_fixed->width-20-270, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270, pos_y);    
    nfui_regi_post_event_callback(obj, post_emosd_line_width_event_handler);

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINE TRANSPARENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 270, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);
    
    obj = (NFOBJECT*)cw_slider_new(50, eosd_fixed->width-20-70-270, 40);
    cw_slider_set_range((CWSLIDER*)obj, 0, 100, 101); 
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, eosd_fixed->width-20-70-270, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270, pos_y);   
    nfui_regi_post_event_callback(obj, post_line_transparency_slider_event_handler);
    g_emosd_lt_slider = obj;

    obj = nfui_spinbutton_new_value_with_range(0, 0, 100, 1); 
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
    nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nfobject_set_size(obj, 70, 40);
    nfui_nfobject_show(obj);                
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x+270+(eosd_fixed->width-20-70-270), pos_y);  
    nfui_regi_post_event_callback(obj, post_line_transparency_spin_event_handler);
    g_emosd_lt_spin = obj;

    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BOUNDING BOX OBJECT TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(obj, 340, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);  

    pos_y += 42;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_emosd_object_type_event_handler);
    g_emosd_obj_type[EMOSD_OBJ_TYPE_PERSON] = obj;

    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    pos_x += size_w + 5;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("person", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 110, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj,  pos_x, pos_y-((40-size_h)/2));
    
    pos_x += 110;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_emosd_object_type_event_handler);
    g_emosd_obj_type[EMOSD_OBJ_TYPE_CAR] = obj;

    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    pos_x += size_w + 5;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("car", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 110, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj,  pos_x, pos_y-((40-size_h)/2));
    
    pos_x += 110;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_emosd_object_type_event_handler);
    g_emosd_obj_type[EMOSD_OBJ_TYPE_BIKE] = obj;
    
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    pos_x += size_w + 5;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("bike", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 110, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)eosd_fixed, obj,  pos_x, pos_y-((40-size_h)/2));


    // R1 CONTENTS --------------------------------
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_BOX_AIENGINE_TITLE, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_set_size(obj, 500, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_r1_fixed, obj, 0, 0);
    g_aibox_title_label = obj;

    group_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(group_fixed, parent->width, 300);
    nfui_nfobject_modify_bg(group_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(group_fixed);
    nfui_nffixed_put((NFFIXED*)g_r1_fixed, group_fixed, 0, 46);
    nfui_regi_post_event_callback(group_fixed, post_group_fixed_event_cb);
    g_aibox_group_fixed = group_fixed;

    pos_x = 10;
    pos_y = 12;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_BOX_AIENGINE_HELP1), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*2) size_h = 26*2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);

    pos_y += size_h;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_BOX_AIENGINE_HELP2), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*3) size_h = 26*3;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);

    pos_y += size_h;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_BOX_AIENGINE_HELP3), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*2) size_h = 26*2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);

    pos_y += (size_h + 16);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Detected AI BOX address :", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, 34);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    g_aibox_addr_label1 = obj;

    pos_y += 35;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(921));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    nfui_nfobject_set_size(obj, parent->width-20, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    g_aibox_addr_label2 = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(STR_CAM_AIENGINE_TITLE, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_set_size(obj, 500, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_r1_fixed, obj, 0, 0);
    g_aicam_title_label = obj;

    group_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(group_fixed, parent->width, 300);
    nfui_nfobject_modify_bg(group_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(group_fixed);
    nfui_nffixed_put((NFFIXED*)g_r1_fixed, group_fixed, 0, 46);
    nfui_regi_post_event_callback(group_fixed, post_group_fixed_event_cb);
    g_aicam_group_fixed = group_fixed;

    pos_x = 10;
    pos_y = 12;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_CAM_AIENGINE_HELP1), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*2) size_h = 26*2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);

    pos_y += size_h;

// https://qts.itxm2m.com:8443/browse/SWPFOURCE-1188 참조
// CCTV 모드, 링크로컬주소로 접속 된 경우 등 카메라에 직접 접속할 수 없는 경우가 있어 임시로 문구를 숨김
/*
    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_CAM_AIENGINE_HELP2), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    g_message("%s, %d, size_h:%d", __FUNCTION__, __LINE__, size_h);
    if (size_h >= 26*3) size_h = 26*3;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);

    pos_y += size_h;
*/

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(STR_CAM_AIENGINE_HELP3), parent->width-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    if (size_h >= 26*2) size_h = 26*2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, size_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    // nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);

    pos_y += (size_h + 16);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI CAM address :", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, parent->width-20, 34);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    // nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    g_aicam_addr_label1 = obj;

    pos_y += 35;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(921));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    nfui_nfobject_set_size(obj, parent->width-20, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, pos_x, pos_y);
    // nfui_nfobject_show(obj);
    g_aicam_addr_label2 = obj;

    obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("OSD OPTION", 260);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_osd_option_button_event_handler);
    // nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_r1_fixed, obj, g_r1_fixed->width-20-260, 56+group_fixed->height);

    return 0;
}

gint _dvabx_engine_init_data(gint ch, AiAnalysisActData *analysis_data, AiAnalysisActData *org_analysis_data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;
    ITX_DVARULE_PROP prop;

    CAM_PROFILE_T profile;
    NF_NOTIFY_INFO vloss_info;
    gint i, matched_algo = 0;

    gchar strBuf[128];

    g_cur_channel = ch;
    g_analysis_data = analysis_data;
    g_org_analysis_data = org_analysis_data;

    top = nfui_nfobject_get_top(g_curfixed);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    component_data->act_capable = 0;
    component_data->act_license = 0;
    component_data->aibox_addr = 0;
    memset(component_data->aibox_owner, 0x00, sizeof(component_data->aibox_owner));
    memset(component_data->algorithm_value, 0x00, sizeof(component_data->algorithm_value));
    memset(component_data->algorithm_text, 0x00, sizeof(component_data->algorithm_text));
    memset(component_data->algorithm_type, 0x00, sizeof(component_data->algorithm_type));

    dvaaid = dvaa_get_dvaaid(ch);
    dvaa_itx_detector_get_rule_prop(dvaaid, &prop);
    component_data->en_engine = prop.en_engine;

    nfui_nfobject_hide(g_nvrbox_title_label);
    nfui_nfobject_hide(g_nvrbox_help_label);
    nfui_nfobject_hide(g_nvrcam_title_label);
    nfui_nfobject_hide(g_nvrcam_help_label);
    nfui_nfobject_hide(g_aibox_title_label);
    nfui_nfobject_hide(g_aibox_group_fixed);
    nfui_nfobject_hide(g_aicam_title_label);
    nfui_nfobject_hide(g_aicam_group_fixed);
    nfui_nfobject_hide(g_r0_fixed);
    nfui_nfobject_hide(g_r1_fixed);

    nfui_nflabel_set_text((NFLABEL*)g_aibox_addr_label2, "");

    memset(&vloss_info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_vloss_data(&vloss_info);

    if (analysis_data[ch].dvabox_active) 
    {
        if ((~vloss_info.d.params[0] & (1 << ch)) && (component_data->aibox_alive))
        {
            component_data->aibox_addr = analysis_data[ch].dvabox_ipaddr;
            nf_api_get_aibox_owner(analysis_data[ch].dvabox_ipaddr, component_data->aibox_owner);

            _get_aibox_capability_algorithm_list(ch, analysis_data[ch].dvabox_ipaddr);
            if (_is_possible_aibox_owner(component_data)) {
                _get_aibox_selected_algorithm(ch, component_data->algorithm_value, component_data->algorithm_text, component_data->algorithm_type);
            }

            component_data->act_capable = 1;
            component_data->act_license = 1;
        }

        if (component_data->aibox_alive)
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            nf_api_get_aibox_url(ch, strBuf);
            if (strlen(strBuf)) nfui_nflabel_set_text((NFLABEL*)g_aibox_addr_label2, strBuf);
        }

        nfui_nfobject_show(g_r1_fixed);
        nfui_nfobject_show(g_aibox_title_label);
        nfui_nfobject_show(g_aibox_group_fixed);
    }
    else if (analysis_data[ch].dvacam_active) 
    {
        if (~vloss_info.d.params[0] & (1 << ch))
        {
            _get_aicam_capability_algorithm_list(ch);
            _get_aicam_selected_algorithm(ch, component_data->algorithm_value, component_data->algorithm_text, component_data->algorithm_type);

            component_data->act_capable = 1;
            component_data->act_license = 1;
        }

        if (scm_get_ipcam_ai_type(ch) == CAM_AI_TYPE_PRO) {
            nfui_nfobject_show(g_r1_fixed);
            nfui_nfobject_show(g_aicam_title_label);
            nfui_nfobject_show(g_aicam_group_fixed);
        }
        else {
            nfui_nfobject_show(g_r0_fixed);
            nfui_nfobject_show(g_nvrcam_title_label);
            nfui_nfobject_show(g_nvrcam_help_label);
        }
        
        if (scm_get_ipcam_ai_type(ch) == CAM_AI_TYPE_CLAIR2)
        {
            nfui_nfobject_show(g_nvrcam_help2_label);
            nfui_nfobject_show(g_aicam_emosd_title_label);
            nfui_nfobject_show(g_aicam_emosd_fixed);
        }
        else
        {
            nfui_nfobject_hide(g_nvrcam_help2_label);
            nfui_nfobject_hide(g_aicam_emosd_title_label);
            nfui_nfobject_hide(g_aicam_emosd_fixed);
        }

        scm_get_cam_profile(ch, &profile);
        nfui_nflabel_set_text((NFLABEL*)g_aicam_addr_label2, profile.conf.ipaddr);
    }

    return 0;
}

gint _dvabx_engine_show_page(gint expose)
{
    nfui_nfobject_show(g_parent);
    nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}
