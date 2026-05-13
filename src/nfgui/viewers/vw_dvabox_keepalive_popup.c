
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
#include "vw_dvabox_keepalive_popup.h"


#define POPUP_SIZE_WID		    (1640)
#if defined(GUI_4CH_SUPPORT)
#define POPUP_SIZE_HEI          (360)
#elif defined(GUI_8CH_SUPPORT)
#define POPUP_SIZE_HEI          (520)
#else
#define POPUP_SIZE_HEI          (840)
#endif


static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_aibox_mac[GUI_CHANNEL_CNT];
static NFOBJECT *g_aibox_ipaddr[GUI_CHANNEL_CNT];
static NFOBJECT *g_aibox_status[GUI_CHANNEL_CNT];


static gint _get_ip_string(guint ip, gchar *str)
{
    sprintf(str, "%d.%d.%d.%d", (ip>>24)&255, (ip>>16)&255, (ip>>8)&255, ip&255);
    return 0;
}

static gint _set_aibox_status(gint expose)
{
    AiAnalysisActData analysis_data;
    guint aibox_state;

    gchar strBuf[128];
    gchar lookup_str[1024];
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_aianalysis_act_data(&analysis_data, i);
        aibox_state = nf_api_get_aibox_connection_status(i);

        nfui_nflabel_modify_fg((NFLABEL*)g_aibox_mac[i], COLOR_IDX(231));
        nfui_nflabel_set_text((NFLABEL*)g_aibox_mac[i], "-");

        nfui_nflabel_modify_fg((NFLABEL*)g_aibox_ipaddr[i], COLOR_IDX(231));
        nfui_nflabel_set_text((NFLABEL*)g_aibox_ipaddr[i], "-");

        nfui_nflabel_modify_fg((NFLABEL*)g_aibox_status[i], COLOR_IDX(231));
        nfui_nflabel_set_text((NFLABEL*)g_aibox_status[i], "OFF");

        if (analysis_data.dvabox_active)
        {
            nfui_nflabel_set_text((NFLABEL*)g_aibox_mac[i], analysis_data.dvabox_mac);
            nfui_nflabel_set_text((NFLABEL*)g_aibox_status[i], "ON");

            if ((aibox_state == NF_AIBOX_CONN_FAILED) || (aibox_state == NF_AIBOX_STREAM_CONN_FAILED)) {
                nfui_nflabel_modify_fg((NFLABEL*)g_aibox_ipaddr[i], COLOR_PRG_IDX(UX_COLOR_FF0000));
                nfui_nflabel_modify_fg((NFLABEL*)g_aibox_status[i], COLOR_PRG_IDX(UX_COLOR_FF0000));
            }

            if (aibox_state == NF_AIBOX_CONN_FAILED) {
                nfui_nflabel_set_text((NFLABEL*)g_aibox_ipaddr[i], "ERROR");
            }
            else if ((aibox_state == NF_AIBOX_CONN_SUCCESS) || (aibox_state == NF_AIBOX_STREAM_CONN_FAILED)) {
                memset(strBuf, 0x00, sizeof(strBuf));
                nf_api_get_aibox_url(i, strBuf);
                nfui_nflabel_set_text((NFLABEL*)g_aibox_ipaddr[i], strBuf);
            }

            if (aibox_state == NF_AIBOX_CONN_FAILED) {
                memset(strBuf, 0x00, sizeof(strBuf));
                _get_ip_string(analysis_data.dvabox_ipaddr, strBuf);

                memset(lookup_str, 0x00, sizeof(lookup_str));
                snprintf(lookup_str, sizeof(lookup_str)-1, "%s (%s)", lookup_string("Please check the AI BOX's network connection."), strBuf);
                nfui_nflabel_set_text((NFLABEL*)g_aibox_status[i], lookup_str);
            }
            else if (aibox_state == NF_AIBOX_STREAM_CONN_FAILED) {
                nfui_nflabel_set_text((NFLABEL*)g_aibox_status[i], "Please check the video source settings by accessing the AI BOX via a web browser.");
            }
            else if (aibox_state == NF_AIBOX_CONNECTING) {
                nfui_nflabel_set_text((NFLABEL*)g_aibox_status[i], "CONNECTING");
            }
            else if (aibox_state == NF_AIBOX_CONN_SUCCESS) {
                nfui_nflabel_set_text((NFLABEL*)g_aibox_status[i], "NORMAL");
            }
        }

        if (expose) nfui_signal_emit(g_aibox_mac[i], GDK_EXPOSE, TRUE);
        if (expose) nfui_signal_emit(g_aibox_ipaddr[i], GDK_EXPOSE, TRUE);
        if (expose) nfui_signal_emit(g_aibox_status[i], GDK_EXPOSE, TRUE);
    }

    return 0;
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

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == INFY_AI_KEEP_ALIVE_NOTIFY)
	{
        _set_aibox_status(1);
	}
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_AI_KEEP_ALIVE_NOTIFY);
    }

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
        g_curwnd = 0;
	}

	return FALSE;
}

gint vw_dvabox_keep_alive_popup_open(NFWINDOW *parent)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj; 

    GdkPixbuf *pbCamImage[32];
    gchar strCamBuf[STRING_SIZE_CAMTITLE];
    guint table_width[4];

    gint i, j;


    if (g_curwnd) return -1;


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

    table_width[0] = 250;
    table_width[1] = 210;
    table_width[2] = 340;
    table_width[3] = 800;

	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "AI BOX CONNECTION STATUS", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
    nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    ntb = nfui_nftable_new(4, 1, 1, 1, table_width, 40);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)main_fixed, ntb, 20, 60);    

    obj = nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("MAC ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);

    obj = nfui_nflabel_new_with_pango_font("IP ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);

    obj = nfui_nflabel_new_with_pango_font("STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);

    ntb = nfui_nftable_new(4, GUI_CHANNEL_CNT, 1, 1, table_width, 40);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)main_fixed, ntb, 20, 101);    

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(strCamBuf, 0x00, sizeof(strCamBuf));
        var_get_camtitle(strCamBuf, i);

        obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strCamBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i);

        obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(231));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i);
        g_aibox_mac[i] = obj;

        obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(231));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 2, i);
        g_aibox_ipaddr[i] = obj;

        obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(231));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 3, i);
        g_aibox_status[i] = obj;
    }

    _set_aibox_status(0);

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (POPUP_SIZE_WID-174)/2, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

    uxm_reg_imsg_event(main_fixed, INFY_AI_KEEP_ALIVE_NOTIFY);

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);
	
	return 0;
}

gboolean vw_dvabox_keep_alive_popup_is_shown()
{
    if (g_curwnd) return TRUE;

    return FALSE;
}
