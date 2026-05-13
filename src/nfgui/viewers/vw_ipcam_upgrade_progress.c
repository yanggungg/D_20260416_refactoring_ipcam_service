
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"
#include "tools/nf_ui_function.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfvklabel.h"
#include "objects/nfprogressbar.h"

#include "scm.h"
#include "iux_msg.h"

#include "vw_ipcam_upgrade_progress.h"


#define _SUPPORT_ERROR_DETAIL
//#define _SUPPORT_UPGRADE_RETRY


#define CH_LABEL_W			(80)
#define STATUS_LABEL_W		(140)
#ifdef _SUPPORT_ERROR_DETAIL
#define ERROR_BTN_W			(34)
#else
#define ERROR_BTN_W			(0)
#endif
#ifdef _SUPPORT_UPGRADE_RETRY	
#define RETRY_BTN_W			(120)
#else
#define RETRY_BTN_W			(0)
#endif

enum {
	UPGRADE_CH,
	UPGRADE_PROGRESS,
	UPGRADE_STATUS,
#ifdef _SUPPORT_ERROR_DETAIL
	UPGRADE_ERR_DETAIL,
#endif		
#ifdef _SUPPORT_UPGRADE_RETRY	
	UPGRADE_RETRY,	
#endif	
	UPGRADE_MAX
};

enum {
  ERR_NA        = 0,
  ERR_VERSION,
  ERR_UNKNOWN,
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_progress_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_status_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_error_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_retry_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_close_obj;
static gint g_block = 0;
static gint g_reboot = 0;


static void _get_error_text(gchar *buf, gint err_no)
{
    switch(err_no)
    {
        case NF_IPCAM_FW_ERR_OK:
            strcpy(buf, "NF_IPCAM_FW_ERR_OK");
            break;

        case NF_IPCAM_FW_ERR_BINARY_SIZE:
            strcpy(buf, "Firmware file size is too large.");
            break;

        case NF_IPCAM_FW_ERR_HIGHER_VER:
            strcpy(buf, "You cannot upgrade to an earlier firmware version.");
            break;

        case NF_IPCAM_FW_ERR_MODEL_CD:
            strcpy(buf, "The selected firmware file does not match the camera model.");
            break;

        case NF_IPCAM_FW_ERR_ONVIF:
            strcpy(buf, "Selected camera does not support firmware upgrade function.");
            break;

        case NF_IPCAM_FW_ERR_OLD_SW_VER:
            strcpy(buf, "The current firmware version cannot be upgraded.");
            break;

        case NF_IPCAM_FW_ERR_RETURN_ERROR:
            strcpy(buf, "Camera returned an incorrect value.");
            break;

        case NF_IPCAM_FW_ERR_DISCONNECT:
            strcpy(buf, "Camera disconnected.");
            break;

        case NF_IPCAM_FW_ERR_INVALID_FILE:
            strcpy(buf, "File Error");
            break;

        case NF_IPCAM_FW_ERR_READY:
            strcpy(buf, "Failed to prepare firmware upgrade.");
            break;

        case NF_IPCAM_FW_ERR_UPLOAD:
            strcpy(buf, "Failed to upload firmware.");
            break;

        case NF_IPCAM_FW_ERR_WRITE:
            strcpy(buf, "Failed to write firmware.");
            break;

        case NF_IPCAM_FW_ERR_GENERAL:
            strcpy(buf, "Failed to upgrade firmware.");
            break;

        default:
            strcpy(buf, "UNKNOWN ERROR");
            break;
    }
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(event->type) 
	{
		case GDK_EXPOSE:
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		}
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == INFY_IPCAM_FWUP_RATE)
	{
		CMM_MESSAGE_T *pmsg;
		gint ch, rate;
		gchar strBuf[8];

		pmsg = (CMM_MESSAGE_T*)data;	
		ch = pmsg->param;
		rate = GPOINTER_TO_UINT(pmsg->data);

		if (!g_progress_obj[ch]) return FALSE;
		if (nfui_nfprogressbar_get_rate((NFPROGRESSBAR*)g_progress_obj[ch]) == rate) return FALSE;
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[ch]), "SUCCESS") == 0) return FALSE;
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[ch]), "FAIL") == 0) return FALSE;
		
		nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_progress_obj[ch], rate);
		nfui_signal_emit(g_progress_obj[ch], GDK_EXPOSE, FALSE);

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "%d %%", rate);
		nfui_nflabel_set_text((NFLABEL*)g_status_obj[ch], strBuf);
		nfui_signal_emit(g_status_obj[ch], GDK_EXPOSE, TRUE);		
	}
	else if (evt->type == INFY_IPCAM_FWUP_CMPL)
	{
		CMM_MESSAGE_T *pmsg;
		gint ch;

		pmsg = (CMM_MESSAGE_T*)data;	
		ch = pmsg->param;

		if (!g_progress_obj[ch]) return FALSE;
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[ch]), "SUCCESS") == 0) return FALSE;
		
		nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_progress_obj[ch], 100);
		nfui_signal_emit(g_progress_obj[ch], GDK_EXPOSE, FALSE);

		nfui_nflabel_set_text((NFLABEL*)g_status_obj[ch], "SUCCESS");		
		nfui_signal_emit(g_status_obj[ch], GDK_EXPOSE, TRUE);
	}
	else if (evt->type == INFY_IPCAM_FWUP_ERROR)
    {
		CMM_MESSAGE_T *pmsg;
		gint ch;
		guint *err_no;

		pmsg = (CMM_MESSAGE_T*)data;	
		ch = pmsg->param;

		if (!g_progress_obj[ch]) return FALSE;
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[ch]), "FAIL") == 0) return FALSE;
		
		nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_progress_obj[ch], 0);
		nfui_signal_emit(g_progress_obj[ch], GDK_EXPOSE, FALSE);

		nfui_nflabel_set_text((NFLABEL*)g_status_obj[ch], "FAIL");
		nfui_signal_emit(g_status_obj[ch], GDK_EXPOSE, TRUE);

		nfui_nfobject_set_data(g_error_obj[ch], "UPGRADE_ERROR_NO", GINT_TO_POINTER(ERR_UNKNOWN));		
	}
	else if (evt->type == INFY_IPCAM_FWUP_ERROR_VERSION)
    {
		CMM_MESSAGE_T *pmsg;
		gint ch;
		guint *err_no;

		pmsg = (CMM_MESSAGE_T*)data;	
		ch = pmsg->param;
		err_no = (guint*)(pmsg->data);

		if (!g_progress_obj[ch]) return FALSE;
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[ch]), "FAIL") == 0) return FALSE;
		
		nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_progress_obj[ch], 0);
		nfui_signal_emit(g_progress_obj[ch], GDK_EXPOSE, FALSE);

		nfui_nflabel_set_text((NFLABEL*)g_status_obj[ch], "FAIL");
		nfui_signal_emit(g_status_obj[ch], GDK_EXPOSE, TRUE);

		nfui_nfobject_set_data(g_error_obj[ch], "UPGRADE_ERROR_NO", GUINT_TO_POINTER(err_no[ch]));
	}   
	else if (evt->type == IRET_SCM_UPGRADE_IPCAM_FW)
	{
		gint i;
		gint err_no;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if ((g_status_obj[i]) && (strcmp(nfui_nflabel_get_text((NFLABEL*)g_status_obj[i]), "FAIL") == 0))
			{
    			if (g_error_obj[i])
			    {
				    err_no = GPOINTER_TO_INT(nfui_nfobject_get_data(g_error_obj[i], "UPGRADE_ERROR_NO"));

				    if (err_no > ERR_NA)
				    {
    					nfui_nfobject_enable(g_error_obj[i]);
    					nfui_signal_emit(g_error_obj[i], GDK_EXPOSE, TRUE);
				    }
			    }

    			if (g_retry_obj[i])
    			{
					nfui_nfobject_enable(g_retry_obj[i]);
					nfui_signal_emit(g_retry_obj[i], GDK_EXPOSE, TRUE);
    			}			    
			}
		}
	
		nfui_nfobject_enable(g_close_obj);
		nfui_signal_emit(g_close_obj, GDK_EXPOSE, TRUE);	
	}
	else if (evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, INFY_IPCAM_FWUP_RATE);		
		uxm_unreg_imsg_event(obj, INFY_IPCAM_FWUP_CMPL);		
		uxm_unreg_imsg_event(obj, INFY_IPCAM_FWUP_ERROR);		
		uxm_unreg_imsg_event(obj, INFY_IPCAM_FWUP_ERROR_VERSION);				
		uxm_unreg_imsg_event(obj, IRET_SCM_UPGRADE_IPCAM_FW);		

		nfui_page_close(PGID_IPCAMERA_FWUP_PRG_POPUP, obj);		
		g_curwnd = 0;

		if 
(g_block) gtk_main_quit();	
	}

	return FALSE;
}

static gboolean post_error_detail_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint err_no;
		gchar buf[128];
	
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(buf, 0x00, sizeof(buf));
        
	    err_no = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "UPGRADE_ERROR_NO"));
	    
	    _get_error_text(buf, err_no);
		nftool_mbox(g_curwnd, "ERROR", buf, NFTOOL_MB_OK);
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (g_reboot)
		{
			nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Complete.\nThe system will be reboot soon.", NFTOOL_MB_OK);
			scm_reboot_system(RR_FWUP, 0);
		}
		else
		{
			topwin = nfui_nfobject_get_top(obj);
			nftool_destroy_setup_window(topwin);
		}
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
		return FALSE;
}

gint VW_IPCamera_UPGrade_Progress_Open(NFWINDOW *parent, guint ch_mask, gint block, gint reboot)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *fixed_temp;
	NFOBJECT *tbl;
	NFOBJECT *obj;
	

	GdkPixbuf *prog_img[4];
	GdkPixbuf *error_img[4];

	guint win_w, win_h;	
	guint size_w, size_h;
	guint tbl_w[UPGRADE_MAX];
	guint i;
	
	gint upgrade_cnt = 0;
	gchar strBuf[8];

	g_block = block;
	g_reboot = reboot;

	prog_img[0] = nfui_get_image_from_file((IMG_PROGRESS_BG), NULL);
	prog_img[1] = nfui_get_image_from_file((IMG_PROGRESS_HEAD), NULL);
	prog_img[2] = nfui_get_image_from_file((IMG_PROGRESS_MIDDLE), NULL);
	prog_img[3] = nfui_get_image_from_file((IMG_PROGRESS_TAIL), NULL);

 	error_img[0] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_N, NULL);
	error_img[1] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_O, NULL);
	error_img[2] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_P, NULL);
	error_img[3] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_D, NULL);

	nfui_get_image_size(IMG_PROGRESS_BG, &size_w, &size_h);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (ch_mask & (1 << i)) upgrade_cnt++;
	}

	win_w = 20+CH_LABEL_W+2+size_w+4+2+STATUS_LABEL_W+2+ERROR_BTN_W+2+RETRY_BTN_W+20;
	win_h = 64 + (34+1)*upgrade_cnt + 80;

// <---- WINDOW
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-win_w)/2, (1080-win_h)/2, win_w, win_h);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_main_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);	
	g_curwnd = win;

// <---- FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, (1920-win_w)/2, (1080-win_h)/2);
	nfui_regi_pre_event_callback(fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FW UPGRADE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, win_w-40, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 4);

	tbl_w[UPGRADE_CH] = CH_LABEL_W;
	tbl_w[UPGRADE_PROGRESS] = size_w+4;
	tbl_w[UPGRADE_STATUS] = STATUS_LABEL_W;
#ifdef _SUPPORT_ERROR_DETAIL
	tbl_w[UPGRADE_ERR_DETAIL] = ERROR_BTN_W;	
#endif			
#ifdef _SUPPORT_UPGRADE_RETRY		
	tbl_w[UPGRADE_RETRY] = RETRY_BTN_W;	
#endif

	tbl = (NFOBJECT*)nfui_nftable_new(UPGRADE_MAX, upgrade_cnt, 2, 1, tbl_w, 34);	
	nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, 20, 64);

	upgrade_cnt = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (ch_mask & (1 << i))
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			g_sprintf(strBuf, "CH %d", i+1);			
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)tbl, obj, UPGRADE_CH, upgrade_cnt);

			fixed_temp = nfui_nffixed_new();
			nfui_nfobject_set_size(fixed_temp, tbl_w[1], 34);
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nfobject_show(fixed_temp);
			nfui_nftable_attach((NFTABLE*)tbl, fixed_temp, UPGRADE_PROGRESS, upgrade_cnt);

			obj = nfui_nfprogressbar_new_with_images(prog_img[0], prog_img[1], prog_img[2], prog_img[3]);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 2, (34-size_h)/2);
			g_progress_obj[i] = obj;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0 %", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)tbl, obj, UPGRADE_STATUS, upgrade_cnt);
			g_status_obj[i] = obj;

#ifdef _SUPPORT_ERROR_DETAIL
			fixed_temp = nfui_nffixed_new();
			nfui_nfobject_set_size(fixed_temp, 34, 34);
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nfobject_show(fixed_temp);
			nfui_nftable_attach((NFTABLE*)tbl, fixed_temp, UPGRADE_ERR_DETAIL, upgrade_cnt);

			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), error_img);
			nfui_nfobject_set_size(obj, (guint)32, (guint)32);
			nfui_nfobject_disable(obj);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 1, 1);
			nfui_regi_post_event_callback(obj, post_error_detail_button_handler);
			g_error_obj[i] = obj;

			nfui_nfobject_set_data(obj, "UPGRADE_ERROR_NO", GINT_TO_POINTER(ERR_NA));
#else
			g_error_obj[i] = 0;
#endif

#ifdef _SUPPORT_UPGRADE_RETRY				
			obj = nftool_normal_button_create_popup_type2("RETRY", RETRY_BTN_W);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_disable(obj);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)tbl, obj, UPGRADE_RETRY, upgrade_cnt);
			g_retry_obj[i] = obj;
#else
			g_retry_obj[i] = 0;
#endif
			
			upgrade_cnt++;
		}	
		else
		{
			g_progress_obj[i] = 0;
			g_status_obj[i] = 0;
			g_error_obj[i] = 0;			
			g_retry_obj[i] = 0;
		}
	}

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (win_w-174)/2, win_h-60);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	g_close_obj = obj;

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_IPCAMERA_FWUP_PRG_POPUP, win, NULL);

	uxm_reg_imsg_event(win, INFY_IPCAM_FWUP_RATE);
	uxm_reg_imsg_event(win, INFY_IPCAM_FWUP_CMPL);
	uxm_reg_imsg_event(win, INFY_IPCAM_FWUP_ERROR);
	uxm_reg_imsg_event(win, INFY_IPCAM_FWUP_ERROR_VERSION);	
	uxm_reg_imsg_event(win, IRET_SCM_UPGRADE_IPCAM_FW);		

	if (g_block) gtk_main();

	return 0;
}

