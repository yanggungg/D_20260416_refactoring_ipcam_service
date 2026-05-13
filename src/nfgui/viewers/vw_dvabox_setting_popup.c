
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfvklabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfimglabel.h"

#include "ix_mem.h"
#include "nf_api_dlva.h"
#include "vw_dvabox_setting_popup.h"

#define PAGE_FIXED_CNT 2
#define ROW_CNT_PER_PAGE (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)


#define POPUP_SIZE_WID		    (1100)
#if defined(GUI_4CH_SUPPORT)
#define POPUP_SIZE_HEI          (360)
#elif defined(GUI_8CH_SUPPORT)
#define POPUP_SIZE_HEI          (520)
#elif defined(GUI_16CH_SUPPORT)
#define POPUP_SIZE_HEI          (840)
#else
#define POPUP_SIZE_HEI          (910)
#endif


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_algorithm_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_ok_btn;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static AiAnalysisActData g_org_analysis_data[GUI_CHANNEL_CNT];
static AiAnalysisActData *g_analysis_data;

static gint g_set_channel = 0;


// vw_dvabx_engine.c
// vw_dvabox_setting_popup.c
static gint _supported_algorithm_type(gchar *algorithm_type)
{
    if (!algorithm_type) return 0;

    if (strlen(algorithm_type) == 0) return 1;  // NONE

    if (ivsc.vendor_code == 28)
    {
        if (strcmp(algorithm_type, "mot") == 0) return 1;
    }
    else
    {
        if (strcmp(algorithm_type, "mot") == 0) return 1;
        if (strcmp(algorithm_type, "fr") == 0) return 1;
        if (strcmp(algorithm_type, "lpr") == 0) return 1;
    }

    return 0;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            if (nfui_nfobject_is_shown((NFOBJECT *)g_page_fixed[i]))
            {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT)
            return FALSE;

        if (i == 0)
            return FALSE;

        nfui_on_backscr(obj);
        nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;

        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW *)nfui_nfobject_get_top(obj));

        nfui_flip(obj);
        nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            if (nfui_nfobject_is_shown((NFOBJECT *)g_page_fixed[i]))
            {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT)
            return FALSE;

        if (i == (PAGE_FIXED_CNT - 1))
            return FALSE;

        nfui_on_backscr(obj);
        nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;

        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW *)nfui_nfobject_get_top(obj));

        nfui_flip(obj);
        nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean pre_aibox_algorithm_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_PRESS)
    {
        ai_capa_t *algorithm_info;
        gchar *algorithm_str;

        gint i, j;

        nfui_combobox_remove_all(NF_COMBOBOX(obj->parent));

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_algorithm_obj[i] == obj->parent) break;
        }
        if (i == GUI_CHANNEL_CNT) return FALSE;

        algorithm_info = nf_api_get_capability(g_analysis_data[i].dvabox_ipaddr, i);
        if (algorithm_info)
        {
            for (j = 0; j < algorithm_info->algorithm_count; j++)
            {
                if ((!algorithm_info->algorithm_list[j].disabled) && (_supported_algorithm_type(algorithm_info->algorithm_list[j].algo_type)))
                {
                    nfui_combobox_append_data(NF_COMBOBOX(g_algorithm_obj[i]), algorithm_info->algorithm_list[j].text);

                    if (strcmp(g_analysis_data[i].algorithm, algorithm_info->algorithm_list[j].value) == 0) {
                        nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_algorithm_obj[i], algorithm_info->algorithm_list[j].text);
                    }
                }

                algorithm_str = imalloc(sizeof(algorithm_info->algorithm_list[j].value));
                strcpy(algorithm_str, algorithm_info->algorithm_list[j].value);
                nfui_nfobject_set_alloc_data(g_algorithm_obj[i], algorithm_info->algorithm_list[j].text, algorithm_str);                
            }
            nf_api_capability_free(algorithm_info);
        }
        nfui_signal_emit(g_algorithm_obj[i], GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_aibox_algorithm_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gchar *algorithm_text = 0;
        gchar *algorithm_value = 0;

        gint i, j;
        gint ret_code = DLVA_API_RET_OK;        

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_algorithm_obj[i] == obj) break;
        }
        if (i == GUI_CHANNEL_CNT) return FALSE;

        algorithm_text = nfui_combobox_get_value((NFCOMBOBOX*)obj);
        algorithm_value = nfui_nfobject_get_data(obj, algorithm_text);

        memset(g_analysis_data[i].algorithm, 0x00, sizeof(g_analysis_data[i].algorithm));
        if (algorithm_value) {
            g_snprintf(g_analysis_data[i].algorithm, sizeof(g_analysis_data[i].algorithm), "%s", algorithm_value);
        }

        ret_code = nf_api_aibox_set_video_stream(i, g_analysis_data[i].dvabox_ipaddr, g_analysis_data[i].algorithm);
        g_message("%s, %d, ch:%d, ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, i, g_analysis_data[i].dvabox_ipaddr, ret_code);

        if (ret_code == DLVA_API_RET_AIBOX_PERMISSION_DENIED)
        {
            nf_api_set_aibox_owner(g_analysis_data[g_set_channel].dvabox_ipaddr);

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (g_algorithm_obj[i]->type != NFOBJECT_TYPE_NFCOMBOBOX) continue;

                ret_code = nf_api_aibox_set_video_stream(i, g_analysis_data[i].dvabox_ipaddr, g_analysis_data[i].algorithm);
                g_message("%s, %d, ch:%d, ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, i, g_analysis_data[i].dvabox_ipaddr, ret_code);
            }
        }
    }

    return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
        aibox_algorithm_name aibox_algorithm[GUI_CHANNEL_CNT];
        guint ch_mask = 0;
        gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(aibox_algorithm, 0x00, sizeof(aibox_algorithm_name)*GUI_CHANNEL_CNT);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_org_analysis_data[g_set_channel].dvabox_ipaddr == g_org_analysis_data[i].dvabox_ipaddr)
            {
                ch_mask |= 1 << i;

                if (strlen(g_org_analysis_data[i].algorithm))
                    snprintf(aibox_algorithm[i].value, sizeof(aibox_algorithm[i].value), "%s", g_org_analysis_data[i].algorithm);
            }
        }

        nf_api_set_aibox_owner(g_org_analysis_data[g_set_channel].dvabox_ipaddr);
        nf_api_aibox_set_video_streams(g_org_analysis_data[g_set_channel].dvabox_ipaddr, aibox_algorithm, ch_mask);

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_dvabox_setting_popup_open(NFWINDOW *parent, gint ch, AiAnalysisActData *analysis_data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj; 
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    GdkPixbuf *pbCamImage[32];
    gchar strBuf[256];
    gchar strCamBuf[STRING_SIZE_CAMTITLE];
    guint setting_aibox;
    guint table_width[4];

    aibox_algorithm_name *aibox_algorithm;
    gint i, j;
    gint pos_x, pos_y;
    guint size_w, size_h;
    gint page_num, row_num;
    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


    g_set_channel = ch;
    g_analysis_data = analysis_data;

    memset(g_org_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);    

    setting_aibox = analysis_data[ch].dvabox_ipaddr;

    pbCamImage[0]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
    pbCamImage[1]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
    pbCamImage[2]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
    pbCamImage[3]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL); 
    pbCamImage[4]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);         
    pbCamImage[5]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
    pbCamImage[6]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
    pbCamImage[7]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL); 
    pbCamImage[8]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
    pbCamImage[9]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);     
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);         
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL); 
    pbCamImage[18]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);     
    pbCamImage[20]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL); 
    pbCamImage[21]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL); 
    pbCamImage[24]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);         
    pbCamImage[25]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL); 
    pbCamImage[26]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL); 
    pbCamImage[28]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);     
    pbCamImage[30]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL); 
    pbCamImage[31]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL); 

    prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
    prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
    prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
    prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

    next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
    next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
    next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
    next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    table_width[0] = 250;
    table_width[1] = 190;
    table_width[2] = 220;
    table_width[3] = 380;

	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "AI BOX ALGORITHM SETTINGS", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

    ntb = nfui_nftable_new(4, 1, 1, 1, table_width, 40);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)main_fixed, ntb, 20, 60);    

    obj = nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("AI ENGINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);

    obj = nfui_nflabel_new_with_pango_font("MAC ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);

    obj = nfui_nflabel_new_with_pango_font("ALGORITHM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);
    
    size_w = 0;
    for (i = 0; i < 4; i++) {
        size_w += table_width[i];
    }

    main_page_fixed = (NFOBJECT *)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED *)main_fixed, main_page_fixed, 20, 101);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED *)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT *)nfui_nftable_new(4, ROW_CNT_PER_PAGE, 1, 1, table_width, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED *)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT *)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
    nfui_regi_post_event_callback(obj, post_prev_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 100, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;

    obj = (NFOBJECT *)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
    nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

    aibox_algorithm = nf_api_get_algorithms_all_ch(setting_aibox);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(strCamBuf, 0x00, sizeof(strCamBuf));
        var_get_camtitle(strCamBuf, i);

        obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strCamBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 0, row_num);

        memset(strBuf, 0x00, sizeof(strBuf));
        if (analysis_data[i].dvabox_active) strcpy(strBuf, "AI BOX");
        else if (analysis_data[i].dvacam_active) strcpy(strBuf, "AI CAM");
        else if (analysis_data[i].builtin_active) strcpy(strBuf, "BUILT-IN AI");
        else if (analysis_data[i].classic_active) strcpy(strBuf, "CLASSIC VA");
        else strcpy(strBuf, "NONE");

        obj = nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 1, row_num);

        if (analysis_data[i].dvabox_active)
        {
            obj = nfui_nflabel_new_with_pango_font(analysis_data[i].dvabox_mac, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 2, row_num);

            if (setting_aibox == analysis_data[i].dvabox_ipaddr) 
            {
                g_message("%s, %d, i:%d, text:%s, value:%s", __FUNCTION__, __LINE__, i, aibox_algorithm[i].text, aibox_algorithm[i].value);

                obj = nfui_combobox_new(0, 0, 0);
                nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
                nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
                nfui_nfobject_show(obj);
                nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 3, row_num);
                nfui_regi_pre_event_callback(NF_COMBOBOX(obj)->label, pre_aibox_algorithm_event_handler);
                nfui_regi_pre_event_callback(NF_COMBOBOX(obj)->button, pre_aibox_algorithm_event_handler);
                nfui_regi_post_event_callback(obj, post_aibox_algorithm_event_handler);
                g_algorithm_obj[i] = obj;

                nfui_combobox_append_data(NF_COMBOBOX(obj), aibox_algorithm[i].text);
                nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, aibox_algorithm[i].text);

                memset(g_org_analysis_data[i].algorithm, 0x00, sizeof(g_org_analysis_data[i].algorithm));
                memset(analysis_data[i].algorithm, 0x00, sizeof(analysis_data[i].algorithm));
                g_snprintf(g_org_analysis_data[i].algorithm, sizeof(g_org_analysis_data[i].algorithm), "%s", aibox_algorithm[i].value);
                g_snprintf(analysis_data[i].algorithm, sizeof(analysis_data[i].algorithm), "%s", aibox_algorithm[i].value);
            }
            else
            {
                obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
                nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
                nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
                nfui_nfobject_show(obj);
                nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 3, row_num);
                g_algorithm_obj[i] = obj;
            }
        }
        else
        {
            obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 2, row_num);

            obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 3, row_num);
            g_algorithm_obj[i] = obj;
        }

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE)
        {
            row_num = 0;
            page_num++;
        }
    }

    free(aibox_algorithm);

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2-174-6, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);
    g_ok_btn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2+6, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);
	
	return 0;
}
