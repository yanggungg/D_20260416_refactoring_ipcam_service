#include "nf_afx.h"

#include "nf_ptz.h"
#include "nf_api_live.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"

#include "iux_afx.h"
#include "evt.h"
#include "uxm.h"
#include "scm.h"
#include "ssm.h"
#include "smt.h"
#include "iux_msg.h"

#include "nf_util_fw.h"
#if defined(CONFIG_FWUPGRADE_SINGLE)
	#include "nf_util_fw_single.h"
#endif
#include "vw_progress_fwupdate_128MB.h"



////////////////////////////////////////////////////////////
//
// private data types
//


enum {
    STEP_PREPARE_VALIDATE,
    STEP_PREPARE_DATABACKUP,
    STEP_PREPARE_COMPLETE,
    STEP_REBOOT_TO_UBOOTUP,
    STEP_UPGRAGE_COMPLETE,
};

#define WARNING_MSG                     "Do not turn the device off while it is being upgraded.\nDo not disconnect the USB device or network cables until the device reboots."


#define LOCAL_PREPARE_VALIDATE_BEGIN_MSG      "Validating the selected firmware file.\nPlease wait."
#define LOCAL_PREPARE_VALIDATE_FAIL_MSG       "The selected firmware file could not be validated.\nPlease check the firmware file and try again."

#define LOCAL_PREPARE_DATABACKUP_BEGIN_MSG    "User settings will be backed-up to the USB device after the upgrade is complete."
#define LOCAL_PREPARE_DATABACKUP_FAIL_MSG     "Failed to back-up the user information.\nPlease check the USB device connection and capacity and try again."

#define LOCAL_PREPARE_COMPLETE_BEGIN_MSG      "Preparing to install firmware. Please wait."
#define LOCAL_PREPARE_COMPLETE_SUCCESS_MSG    "Firmware installation preparations complete.\nThe device will automatically restart and proceed with firmware installation shortly."
#define LOCAL_PREPARE_COMPLETE_FAIL_MSG       "Failed to prepare for firmware installation.\nPlease restart the device and try again."

#define LOCAL_UPGRADE_SUCESS_MSG              "Firmware successfully upgraded."
#define LOCAL_UPGRADE_FAIL_MSG                "Failed to upgrade firmware.\nPlease check the USB device connection and try again."
#define LOCAL_DATA_RECOVERY_FAIL_MSG          "Could not find the device backup file and user settings could not be restored.\nPlease connect the USB device containing the back-up file\nand click Try Again."


#define WEB_PREPARE_VALIDATE_BEGIN_MSG      "Validating the selected firmware file.\nPlease wait."
#define WEB_PREPARE_VALIDATE_FAIL_MSG       ""

#define WEB_PREPARE_DATABACKUP_BEGIN_MSG    "Backing up data.\nPlease do not remove any power or network cables while back-up is in progress."
#define WEB_PREPARE_DATABACKUP_FAIL_MSG     ""

#define WEB_PREPARE_COMPLETE_BEGIN_MSG      "Preparing to install firmware. Please wait."
#define WEB_PREPARE_COMPLETE_SUCCESS_MSG    "Firmware installation preparations complete.\nThe device will automatically restart and proceed with firmware installation shortly."
#define WEB_PREPARE_COMPLETE_FAIL_MSG       "Failed to prepare for firmware installation.\nPlease restart the device and try again."

#define WEB_UPGRADE_SUCESS_MSG              "Firmware successfully upgraded."
#define WEB_UPGRADE_FAIL_MSG                "An error occurred while installing the firmware.\nPlease check the network cable connection and try again.\nIf this error continues to occur please contact your service center or dealer."
#define WEB_DATA_RECOVERY_FAIL_MSG          ""



#define PREPARE_VALIDATE_BEGIN_MSG      (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_VALIDATE_BEGIN_MSG : WEB_PREPARE_VALIDATE_BEGIN_MSG)
#define PREPARE_VALIDATE_FAIL_MSG       (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_VALIDATE_FAIL_MSG : WEB_PREPARE_VALIDATE_FAIL_MSG)

#define PREPARE_DATABACKUP_BEGIN_MSG    (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_DATABACKUP_BEGIN_MSG : WEB_PREPARE_DATABACKUP_BEGIN_MSG)
#define PREPARE_DATABACKUP_FAIL_MSG     (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_DATABACKUP_FAIL_MSG : WEB_PREPARE_DATABACKUP_FAIL_MSG)

#define PREPARE_COMPLETE_BEGIN_MSG      (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_COMPLETE_BEGIN_MSG : WEB_PREPARE_COMPLETE_BEGIN_MSG)
#define PREPARE_COMPLETE_SUCCESS_MSG    (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_COMPLETE_SUCCESS_MSG : WEB_PREPARE_COMPLETE_SUCCESS_MSG)
#define PREPARE_COMPLETE_FAIL_MSG       (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_PREPARE_COMPLETE_FAIL_MSG : WEB_PREPARE_COMPLETE_FAIL_MSG)

#define UPGRADE_SUCESS_MSG              (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_UPGRADE_SUCESS_MSG : WEB_UPGRADE_SUCESS_MSG)
#define UPGRADE_FAIL_MSG                (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_UPGRADE_FAIL_MSG : WEB_UPGRADE_FAIL_MSG)
#define DATA_RECOVERY_FAIL_MSG          (g_upgrade_type == UPGRADE_TYPE_LOCAL ? LOCAL_DATA_RECOVERY_FAIL_MSG : WEB_DATA_RECOVERY_FAIL_MSG)




#define POPUP_SIZE_WID          (920)
#define POPUP_SIZE_HEI          (420)

//#define ARROW_WID               ((POPUP_SIZE_WID-40)/5-10)

#define ARROW_WID               (80)
#define ARROW_HEI               (44)


////////////////////////////////////////////////////////////
//
// private variable
//

static gboolean post_finish_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
static gboolean post_finish_ramdisk_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
static gboolean post_finish_by_web_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_warning_msg_label = 0;
static NFOBJECT *g_progress_msg_label = 0;
static NFOBJECT *g_retry_button = 0;
static NFOBJECT *g_finish_button = 0;
static NFOBJECT *g_finish2_button = 0;

static gint g_upgrade_type = -1;
static gint g_cur_step = STEP_PREPARE_VALIDATE;
static gint g_cur_state = 0;

static guint g_web_response_timeout = 0;
static guint g_ramdisk_finish_timeout = 0;
static guint g_web_finish_timeout = 0;



////////////////////////////////////////////////////////////
//
// private interfaces
//

static gint _draw_step_background(GdkDrawable *drawable)
{
    GdkGC *gc;
    gint i = 0;
    gint off_x, off_y;

    off_x = g_warning_msg_label->x;
    off_y = POPUP_SIZE_HEI-74;

    gc = gdk_gc_new(drawable);

    for (i = 0; i < 5; i++)
    {
        if (i == 0) nfutil_draw_image(drawable, gc, "fw_up_01_normal.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
        else nfutil_draw_image(drawable, gc, "fw_up_02_normal.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);

        off_x += 80;
    }

    g_object_unref(gc);

    return 0;
}

static gint _draw_step_foreground(GdkDrawable *drawable, gint step, gint state)
{
    GdkGC *gc;
    gint i = 0;
    gint off_x, off_y;

    off_x = g_warning_msg_label->x;
    off_y = POPUP_SIZE_HEI-74;

    gc = gdk_gc_new(drawable);

    for (i = 0; i < step; i++)
    {
        if (i == 0) nfutil_draw_image(drawable, gc, "fw_up_01_success.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
        else nfutil_draw_image(drawable, gc, "fw_up_02_success.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);

        off_x += 80;
    }

    if (step == 0)
    {
        if (state == -1) nfutil_draw_image(drawable, gc, "fw_up_01_fail.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
        else if (state == 0) nfutil_draw_image(drawable, gc, "fw_up_01_ing.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
        else if (state == 1) nfutil_draw_image(drawable, gc, "fw_up_01_success.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
    }
    else
    {
        if (state == -1) nfutil_draw_image(drawable, gc, "fw_up_02_fail.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
        else if (state == 0) nfutil_draw_image(drawable, gc, "fw_up_02_ing.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
        else if (state == 1) nfutil_draw_image(drawable, gc, "fw_up_02_success.png", off_x, off_y, -1, -1, NFALIGN_LEFT, 0);
    }

    g_object_unref(gc);

    return 0;
}

static gint _prepare_image_file_to_uboot()
{
    GdkPixmap *pixmap;
    GdkDrawable *drawable;
    GdkGC *pm_gc;
    GdkWindow *rootwin;
    GdkColormap *cmap;
    GdkPixbuf *pbuf, *nbuf;
    GError *error = NULL;
    gint save_w, save_h;

    const gchar *install_str1 = "Installing firmware file...";
    const gchar *install_prg_str2 = "Firmware successfully upgraded.\nPlease wait for the system to automatically start.";
    const gchar *install_fail_str = "Failed to upgrade firmware.\nRe-connect the device power cable to try again.\nIf this message continues to occur, please contact your dealer or service center.";

    save_w = POPUP_SIZE_WID;
    save_h = g_progress_msg_label->y;

    drawable = nfui_nfobject_get_window((NFOBJECT*)g_curwnd);
    cmap = gdk_window_get_colormap(drawable);
    pbuf = gdk_pixbuf_get_from_drawable(NULL, drawable, cmap, 0, 0, 0, 0, save_w, save_h);
    gdk_pixbuf_save(pbuf, "/tmp/fwup_bg.bmp", "bmp", &error, NULL);
    g_object_unref(pbuf);

    if (error) {
        g_message("%s, %d, msg:%s", error->message);
        g_error_free(error);
        return -1;
    }

    save_w = POPUP_SIZE_WID;
    save_h = POPUP_SIZE_HEI - g_progress_msg_label->y;

    rootwin = gdk_get_default_root_window();
    pixmap = gdk_pixmap_new(rootwin, POPUP_SIZE_WID, POPUP_SIZE_HEI, -1);
    pm_gc = gdk_gc_new(pixmap);
    cmap = gdk_window_get_colormap(pixmap);

    pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, POPUP_SIZE_WID, POPUP_SIZE_HEI);
    gdk_draw_pixbuf(pixmap, pm_gc, pbuf, 0, 0, 0, 0, POPUP_SIZE_WID, POPUP_SIZE_HEI, GDK_RGB_DITHER_NONE, 0, 0);
    _draw_step_background(pixmap);
    _draw_step_foreground(pixmap, STEP_REBOOT_TO_UBOOTUP, 0);

    nfutil_draw_text_with_pango(0, 0, &UX_COLOR(COLOR_IDX(200)), pixmap, pm_gc, install_str1,
            22, g_progress_msg_label->y, g_progress_msg_label->width, 40, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(COLOR_IDX(292)), NFALIGN_LEFT, 0);
    nfutil_draw_text_with_pango(0, 0, &UX_COLOR(COLOR_IDX(200)), pixmap, pm_gc, install_prg_str2,
            22, g_progress_msg_label->y+50, g_progress_msg_label->width, 70, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(COLOR_IDX(292)), NFALIGN_LEFT, 0);

    pbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, cmap, 0, 0, 0, 0, POPUP_SIZE_WID, POPUP_SIZE_HEI);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, save_w, save_h);
	gdk_pixbuf_copy_area(pbuf, 0, g_progress_msg_label->y, save_w, save_h, nbuf, 0, 0);
    gdk_pixbuf_save(nbuf, "/tmp/fwup_txt.bmp", "bmp", &error, NULL);
    g_object_unref(pbuf);
    g_object_unref(nbuf);

    if (error) {
        g_message("%s, %d, msg:%s", error->message);
        g_error_free(error);
        g_object_unref(pixmap);
        return -1;
    }

    pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, POPUP_SIZE_WID, POPUP_SIZE_HEI);
    gdk_draw_pixbuf(pixmap, pm_gc, pbuf, 0, 0, 0, 0, POPUP_SIZE_WID, POPUP_SIZE_HEI, GDK_RGB_DITHER_NONE, 0, 0);
    _draw_step_background(pixmap);
    _draw_step_foreground(pixmap, STEP_REBOOT_TO_UBOOTUP, -1);

    nfutil_draw_text_with_pango(0, 0, &UX_COLOR(COLOR_IDX(200)), pixmap, pm_gc, install_fail_str,
            22, g_progress_msg_label->y, g_progress_msg_label->width, g_progress_msg_label->height, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(COLOR_IDX(292)), NFALIGN_LEFT, 0);

    pbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, cmap, 0, 0, 0, 0, POPUP_SIZE_WID, POPUP_SIZE_HEI);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, save_w, save_h);
	gdk_pixbuf_copy_area(pbuf, 0, g_progress_msg_label->y, save_w, save_h, nbuf, 0, 0);
    gdk_pixbuf_save(nbuf, "/tmp/fwup_bg_fail.bmp", "bmp", &error, NULL);
    g_object_unref(pbuf);
    g_object_unref(nbuf);

    if (error) {
        g_message("%s, %d, msg:%s", error->message);
        g_error_free(error);
        g_object_unref(pixmap);
        return -1;
    }

    g_object_unref(pixmap);

    return 0;
}

static gboolean _timeout_dealy_to_imagesave(gpointer data)
{
    g_message("%s, %d", __FUNCTION__, __LINE__);

    _prepare_image_file_to_uboot();
    cmm_send_message(CMMPT_SCM, IRPL_SCM_PREPARE_FWUP_IMAGESAVE, 0, 0, 0);
    return FALSE;
}

static gboolean _timeout_ready_to_web_response(gpointer data)
{
    NFOBJECT *win = (NFOBJECT*)data;

    g_message("%s, %d", __FUNCTION__, __LINE__);

    smt_return_to_previous();

    nfui_nfobject_hide(win);
    nfui_page_close(PGID_PROGRESS_FWUP, win);
    return FALSE;
}

static gboolean _timeout_finish_ramdisk_upgrade(gpointer data)
{
    NFOBJECT *win = (NFOBJECT*)data;

    g_message("%s, %d", __FUNCTION__, __LINE__);

    nfui_nfobject_hide(win);
    nfui_page_close(PGID_PROGRESS_FWUP, win);

    if (g_cur_step == STEP_UPGRAGE_COMPLETE) {
        cmm_send_message(CMMPT_EVT, IRPL_SCM_128MB_FWUP_RAMDISK_RESULT, 0, 0, 0);
    }
    return FALSE;
}

static gboolean _timeout_finish_to_web_upgrade(gpointer data)
{
    NFOBJECT *win = (NFOBJECT*)data;

    g_message("%s, %d", __FUNCTION__, __LINE__);

    nfui_nfobject_hide(win);
    nfui_page_close(PGID_PROGRESS_FWUP, win);

    if (g_cur_step == STEP_UPGRAGE_COMPLETE) {
        cmm_send_message(CMMPT_EVT, IRPL_SCM_128MB_FWUP_RESULT_BY_WEB, 0, 0, 0);
    }
    return FALSE;
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;

    if (evt->type == GDK_EXPOSE)
    {

    }
    else if (evt->type == INFY_SCM_PREPARE_FWUP_OPEN)
    {
        if (nfui_nfobject_is_shown(obj)) nfui_nfobject_hide(obj);

        if (g_web_response_timeout) {
            g_source_remove(g_web_response_timeout);
            g_web_response_timeout = 0;
        }

        if (g_web_finish_timeout) {
            g_source_remove(g_web_finish_timeout);
            g_web_finish_timeout = 0;
        }

        smt_set_service(SMT_FW_UPGRADE);
        nfui_regi_post_event_callback(g_finish_button, post_finish_button_event_handler);

        g_upgrade_type = ((CMM_MESSAGE_T *)data)->param;
        g_message("%s, %d, INFY_SCM_PREPARE_FWUP_OPEN caller:%d", __FUNCTION__, __LINE__, g_upgrade_type);

        g_cur_step = STEP_PREPARE_VALIDATE;

        nfui_nfobject_hide(g_retry_button);
        nfui_nfobject_disable(g_finish_button);

        drawable = nfui_nfobject_get_window(obj);
        _draw_step_background(drawable);
        _draw_step_foreground(drawable, g_cur_step, 0);
        g_cur_state = 0;

        nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_VALIDATE_BEGIN_MSG);

        nfui_nfobject_show(obj);
        nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);

        if (g_upgrade_type == UPGRADE_TYPE_WEB) {
            g_web_response_timeout = g_timeout_add(60000, _timeout_ready_to_web_response, (gpointer)obj);
        }
    }
    else if (evt->type == IRET_SCM_PREPARE_FWUP_VALIDATE)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;

        g_message("%s, %d, IRET_SCM_PREPARE_FWUP_VALIDATE result:%d", __FUNCTION__, __LINE__, result);

        if (g_web_response_timeout) {
            g_source_remove(g_web_response_timeout);
            g_web_response_timeout = 0;
        }

        drawable = nfui_nfobject_get_window(obj);

        if (result == 0)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, ++g_cur_step, 0);
            g_cur_state = 0;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_DATABACKUP_BEGIN_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            if (g_upgrade_type == UPGRADE_TYPE_WEB) {
                g_web_response_timeout = g_timeout_add(60000, _timeout_ready_to_web_response, (gpointer)obj);
            }
        }
        else
        {
            smt_return_to_previous();

            if (g_upgrade_type == UPGRADE_TYPE_LOCAL || g_upgrade_type == UPGRADE_TYPE_REMOTESERVER)
            {
                _draw_step_background(drawable);
                _draw_step_foreground(drawable, g_cur_step, -1);
                g_cur_state = -1;

                nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_VALIDATE_FAIL_MSG);
                nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

                nfui_nfobject_enable(g_finish_button);
                nfui_signal_emit(g_finish_button, GDK_EXPOSE, TRUE);
            }
            else if (g_upgrade_type == UPGRADE_TYPE_WEB)
            {
                nfui_nfobject_hide(obj);
                nfui_page_close(PGID_PROGRESS_FWUP, obj);
            }
        }
    }
    else if (evt->type == IREQ_SCM_PREPARE_FWUP_IMAGESAVE)
    {
        g_message("%s, %d, IREQ_SCM_PREPARE_FWUP_IMAGESAVE", __FUNCTION__, __LINE__);
        g_timeout_add(3000, _timeout_dealy_to_imagesave, 0);
    }
    else if (evt->type == IRET_SCM_PREPARE_FWUP_DATABACKUP)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;

        g_message("%s, %d, IRET_SCM_PREPARE_FWUP_DATABACKUP result:%d", __FUNCTION__, __LINE__, result);

        if (g_web_response_timeout) {
            g_source_remove(g_web_response_timeout);
            g_web_response_timeout = 0;
        }

        drawable = nfui_nfobject_get_window(obj);

        if (result == 0)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, ++g_cur_step, 0);
            g_cur_state = 0;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_COMPLETE_BEGIN_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            if (g_upgrade_type == UPGRADE_TYPE_WEB) {
                g_web_response_timeout = g_timeout_add(60000, _timeout_ready_to_web_response, (gpointer)obj);
            }
        }
        else
        {
            smt_return_to_previous();

            if (g_upgrade_type == UPGRADE_TYPE_LOCAL || g_upgrade_type == UPGRADE_TYPE_REMOTESERVER)
            {
                _draw_step_background(drawable);
                _draw_step_foreground(drawable, g_cur_step, -1);
                g_cur_state = -1;

                nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_DATABACKUP_FAIL_MSG);
                nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

                nfui_nfobject_enable(g_finish_button);
                nfui_signal_emit(g_finish_button, GDK_EXPOSE, TRUE);
            }
            else if (g_upgrade_type == UPGRADE_TYPE_WEB)
            {
                nfui_nfobject_hide(obj);
                nfui_page_close(PGID_PROGRESS_FWUP, obj);
            }
        }
    }
    else if (evt->type == IRET_SCM_PREPARE_FWUP_CMPL)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;

        g_message("%s, %d, IRET_SCM_PREPARE_FWUP_CMPL result:%d", __FUNCTION__, __LINE__, result);

        drawable = nfui_nfobject_get_window(obj);

        if (g_web_response_timeout) {
            g_source_remove(g_web_response_timeout);
            g_web_response_timeout = 0;
        }

        if (result == 0)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, ++g_cur_step, 0);
            g_cur_state = 0;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_COMPLETE_SUCCESS_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            if (g_upgrade_type == UPGRADE_TYPE_LOCAL || g_upgrade_type == UPGRADE_TYPE_REMOTESERVER) {
                scm_reboot_system(RR_FWUP, 2000);
            }
            else if (g_upgrade_type == UPGRADE_TYPE_WEB) {
                scm_reboot_system(RR_FWUP, 30000);
            }
        }
        else
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, -1);
            g_cur_state = -1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, PREPARE_COMPLETE_FAIL_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);
            nfui_signal_emit(g_finish_button, GDK_EXPOSE, TRUE);

            if (g_upgrade_type == UPGRADE_TYPE_WEB) {
                scm_reboot_system(RR_FWUP, 30000);
            }
        }
    }
    else if (evt->type == IREQ_SCM_128MB_FWUP_RESULT)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;

        g_message("%s, %d, IREQ_SCM_128MB_FWUP_RESULT result:%08x", __FUNCTION__, __LINE__, result);

        nfui_regi_post_event_callback(g_finish_button, post_finish_button_event_handler);

        drawable = nfui_nfobject_get_window(obj);

        g_cur_step = STEP_UPGRAGE_COMPLETE;
        g_upgrade_type = UPGRADE_TYPE_LOCAL;

        if (result == NF_FWUP_STATUS_UPGRADE_DONE)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, 1);
            g_cur_state = 1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, UPGRADE_SUCESS_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }
        else if (result == NF_FWUP_STATUS_UPGRADE_FAIL)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, -1);
            g_cur_state = -1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, UPGRADE_FAIL_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }
        else if (result == NF_FWUP_STATUS_DB_FAIL)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, -1);
            g_cur_state = -1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, DATA_RECOVERY_FAIL_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_show(g_retry_button);
            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }
        else
        {
            cmm_send_message(CMMPT_SCM, IRPL_SCM_128MB_FWUP_RESULT, 0, 0, 0);
        }
    }
    else if (evt->type == IREQ_SCM_128MB_FWUP_RAMDISK_RESULT)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;

        g_message("%s, %d, IREQ_SCM_128MB_FWUP_RAMDISK_RESULT result:%08x", __FUNCTION__, __LINE__, result);

        nfui_regi_post_event_callback(g_finish_button, post_finish_ramdisk_button_event_handler);

        drawable = nfui_nfobject_get_window(obj);

        g_cur_step = STEP_UPGRAGE_COMPLETE;
        g_upgrade_type = UPGRADE_TYPE_LOCAL;

        if (result == NF_FWUP_STATUS_UPGRADE_RAM_DONE)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, 1);
            g_cur_state = 1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, UPGRADE_SUCESS_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }
        else if (result == NF_FWUP_STATUS_UPGRADE_RAM_FAIL)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, -1);
            g_cur_state = -1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, UPGRADE_FAIL_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }

        g_ramdisk_finish_timeout = g_timeout_add(60000, _timeout_finish_ramdisk_upgrade, (gpointer)obj);
    }
    else if (evt->type == IREQ_SCM_128MB_FWUP_RESULT_BY_WEB)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;

        g_message("%s, %d, IREQ_SCM_128MB_FWUP_RESULT_BY_WEB result:%08x", __FUNCTION__, __LINE__, result);

        nfui_regi_post_event_callback(g_finish_button, post_finish_by_web_button_event_handler);

        drawable = nfui_nfobject_get_window(obj);

        g_cur_step = STEP_UPGRAGE_COMPLETE;
        g_upgrade_type = UPGRADE_TYPE_WEB;

        if (result == NF_FWUP_STATUS_UPGRADE_NET_DONE)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, 1);
            g_cur_state = 1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, UPGRADE_SUCESS_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }
        else if (result == NF_FWUP_STATUS_UPGRADE_NET_FAIL)
        {
            _draw_step_background(drawable);
            _draw_step_foreground(drawable, g_cur_step, -1);
            g_cur_state = -1;

            nfui_nflabel_set_text((NFLABEL*)g_progress_msg_label, UPGRADE_FAIL_MSG);
            nfui_signal_emit(g_progress_msg_label, GDK_EXPOSE, TRUE);

            nfui_nfobject_enable(g_finish_button);

            nfui_nfobject_show(obj);
            nfui_page_open(PGID_PROGRESS_FWUP, obj, NULL);
        }

        g_web_finish_timeout = g_timeout_add(60000, _timeout_finish_to_web_upgrade, (gpointer)obj);
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_SCM_PREPARE_FWUP_OPEN);
        uxm_unreg_imsg_event(obj, IREQ_SCM_PREPARE_FWUP_IMAGESAVE);
        uxm_unreg_imsg_event(obj, IRET_SCM_PREPARE_FWUP_VALIDATE);
        uxm_unreg_imsg_event(obj, IRET_SCM_PREPARE_FWUP_DATABACKUP);
        uxm_unreg_imsg_event(obj, IRET_SCM_PREPARE_FWUP_CMPL);
        uxm_unreg_imsg_event(obj, IREQ_SCM_128MB_FWUP_RESULT);
        uxm_unreg_imsg_event(obj, IREQ_SCM_128MB_FWUP_RAMDISK_RESULT);
        uxm_unreg_imsg_event(obj, IREQ_SCM_128MB_FWUP_RESULT_BY_WEB);

        nfui_page_close(PGID_PROGRESS_FWUP, obj);
    }

    return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;

    if (evt->type == GDK_EXPOSE)
    {
        drawable = nfui_nfobject_get_window(obj);
        _draw_step_background(drawable);
        _draw_step_foreground(drawable, g_cur_step, g_cur_state);
    }

    return FALSE;
}

static gboolean post_warning_msg_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable = NULL;
        GdkGC *gc = NULL;
        guint off_x, off_y;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);
        nfui_nfobject_get_offset(obj, &off_x, &off_y);

        gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(115));
        gdk_draw_rectangle(drawable, gc, FALSE, off_x-4, off_y-4, obj->width+8, obj->height+8);

        nfutil_draw_image(drawable, gc, IMG_WARNING_ICON, off_x, off_y+(obj->height-64)/2, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);
    }

    return FALSE;
}

static gboolean post_retry_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}

		if (nftool_mbox(g_curwnd, "NOTICE", "The system will reboot in order to restore user settings.\nContinue?", NFTOOL_MB_OKCANCEL) == NFTOOL_MB_OK)
        {
            cmm_send_message(CMMPT_SCM, IRPL_SCM_128MB_FWUP_RESULT, -1, 0, 0);
            nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_NONE);
        }
	}

    return FALSE;
}

static gboolean post_finish_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (g_ramdisk_finish_timeout) {
            g_source_remove(g_ramdisk_finish_timeout);
            g_ramdisk_finish_timeout = 0;
        }

        if (g_web_finish_timeout) {
            g_source_remove(g_web_finish_timeout);
            g_web_finish_timeout = 0;
        }

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(top);
        nfui_page_close(PGID_PROGRESS_FWUP, top);

        if (g_cur_step == STEP_PREPARE_COMPLETE) {
            scm_reboot_system(RR_FWUP, 2000);
            nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_NONE);
        }
        else if (g_cur_step == STEP_UPGRAGE_COMPLETE) {
            cmm_send_message(CMMPT_SCM, IRPL_SCM_128MB_FWUP_RESULT, 0, 0, 0);
        }
	}

    return FALSE;
}

static gboolean post_finish_ramdisk_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (g_ramdisk_finish_timeout) {
            g_source_remove(g_ramdisk_finish_timeout);
            g_ramdisk_finish_timeout = 0;
        }

        if (g_web_finish_timeout) {
            g_source_remove(g_web_finish_timeout);
            g_web_finish_timeout = 0;
        }

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(top);
        nfui_page_close(PGID_PROGRESS_FWUP, top);

        if (g_cur_step == STEP_UPGRAGE_COMPLETE) {
            cmm_send_message(CMMPT_EVT, IRPL_SCM_128MB_FWUP_RAMDISK_RESULT, 0, 0, 0);
        }
	}

    return FALSE;
}

static gboolean post_finish_by_web_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (g_ramdisk_finish_timeout) {
            g_source_remove(g_ramdisk_finish_timeout);
            g_ramdisk_finish_timeout = 0;
        }

        if (g_web_finish_timeout) {
            g_source_remove(g_web_finish_timeout);
            g_web_finish_timeout = 0;
        }

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(top);
        nfui_page_close(PGID_PROGRESS_FWUP, top);

        if (g_cur_step == STEP_UPGRAGE_COMPLETE) {
            cmm_send_message(CMMPT_EVT, IRPL_SCM_128MB_FWUP_RESULT_BY_WEB, 0, 0, 0);
        }
	}

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_progress_fwup_128MB_open(NFWINDOW *parent)
{
    NFOBJECT *win;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    g_cur_step = 0;
    g_cur_state = 0;

// <---- WINDOW
    win = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "FW UPDATE", TRUE);
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    g_curwnd = (NFWINDOW*)win;

// <---- FIXED
    main_fixed = ((NFWINDOW*)win)->child;
    nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARNING_MSG, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 70);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, POPUP_SIZE_WID-40, 120);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 70);
    nfui_regi_post_event_callback(obj, post_warning_msg_label_event_handler);
    g_warning_msg_label = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, POPUP_SIZE_WID-40, 120);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 200);
    g_progress_msg_label = obj;

    obj = nftool_normal_button_create_type1("RETRY", 170);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-190-180, POPUP_SIZE_HEI-74);
    nfui_regi_post_event_callback(obj, post_retry_button_event_handler);
    g_retry_button = obj;

    obj = nftool_normal_button_create_type1("FINISH", 170);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-190, POPUP_SIZE_HEI-74);
    nfui_regi_post_event_callback(obj, post_finish_button_event_handler);
    g_finish_button = obj;

    uxm_reg_imsg_event(win, INFY_SCM_PREPARE_FWUP_OPEN);
    uxm_reg_imsg_event(win, IREQ_SCM_PREPARE_FWUP_IMAGESAVE);
    uxm_reg_imsg_event(win, IRET_SCM_PREPARE_FWUP_VALIDATE);
    uxm_reg_imsg_event(win, IRET_SCM_PREPARE_FWUP_DATABACKUP);
    uxm_reg_imsg_event(win, IRET_SCM_PREPARE_FWUP_CMPL);
    uxm_reg_imsg_event(win, IREQ_SCM_128MB_FWUP_RESULT);
    uxm_reg_imsg_event(win, IREQ_SCM_128MB_FWUP_RAMDISK_RESULT);
    uxm_reg_imsg_event(win, IREQ_SCM_128MB_FWUP_RESULT_BY_WEB);

    uxm_monitor_on_imsg_event(win, INFY_SCM_PREPARE_FWUP_OPEN);
    uxm_monitor_on_imsg_event(win, IREQ_SCM_PREPARE_FWUP_IMAGESAVE);
    uxm_monitor_on_imsg_event(win, IRET_SCM_PREPARE_FWUP_VALIDATE);
    uxm_monitor_on_imsg_event(win, IRET_SCM_PREPARE_FWUP_DATABACKUP);
    uxm_monitor_on_imsg_event(win, IRET_SCM_PREPARE_FWUP_CMPL);
    uxm_monitor_on_imsg_event(win, IREQ_SCM_128MB_FWUP_RESULT);
    uxm_monitor_on_imsg_event(win, IREQ_SCM_128MB_FWUP_RAMDISK_RESULT);
    uxm_monitor_on_imsg_event(win, IREQ_SCM_128MB_FWUP_RESULT_BY_WEB);

    nfui_run_main_event_handler(win);
    nfui_nfobject_hide(win);

    return 0;
}
