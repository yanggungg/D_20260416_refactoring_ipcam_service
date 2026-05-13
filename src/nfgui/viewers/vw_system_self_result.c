
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfimage.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "support/nf_ui_page_manager.h"
#include "objects/nfimglabel.h"

#include "ix_mem.h"

#include "modules/ssm.h"
#include "vw_vkeyboard.h"

#include "uxm.h"
#include "scm.h"
#include "smt.h"
#include "iux_msg.h"

#include "vw_system_self_internal.h"
#include "vw_sys_main.h"
#include "nf_api_disk.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define SSR_WIN_SIZE_X					(430)
#define SSR_WIN_SIZE_Y					(70)
#define SSR_WIN_SIZE_W					(1390)
#if defined(GUI_4CH_SUPPORT)
#define SSR_WIN_SIZE_H					((SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+5)) + 320))
#elif defined(GUI_8CH_SUPPORT)
#define SSR_WIN_SIZE_H					((SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+1)) + 320))
#elif defined(GUI_16CH_SUPPORT)
#define SSR_WIN_SIZE_H					((SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+1)) + 200))
#else
#define SSR_WIN_SIZE_H					((SSR_TITLE_H + (SSR_LABEL_H * (ROW_CNT_PER_PAGE+1)) + 280))
#endif

#define SSR_LABEL_SPACE					(25)

#define SSR_TITLE_H					(36)
#define SSR_TITLE_X					(4)
#define SSR_TITLE_Y					(4)

#define SSR_CELL_W					(50)

#define SSR_LABEL_H					(40)
#define SSR_LABEL_W					(400)
#define SSR_LABEL_TEXT_MARGIN				(20)

#define SSR_TABLE_ROW					(1)
#define SSR_TABLE_COL					(5)
#define SSR_TABLE_X					(SSR_TITLE_X)
#define SSR_TABLE_Y					(SSR_TITLE_H + SSR_LABEL_H + SSR_TABLE_ROW_SPACE)
#define SSR_TABLE_ROW_SPACE				(2)
#define SSR_TABLE_COL_SPACE				(2)

#define SSR_TABLE2_ROW					(2)
#define SSR_TABLE2_COL					(2)

#define SSR_TABLE3_ROW					(IN_DISP_DISK_COUNT)
#define SSR_TABLE3_COL					(2)

#define LABEL_DISK_CNT					(2)

// <---- RETRY, CLOSE BUTTON
#define SSR_RETRY_BTN_X					(1010)
#define SSR_RETRY_BTN_Y					(SSR_WIN_SIZE_H - 70)
#define SSR_RETRY_BTN_W					(174)

#define SSR_CLOSE_BTN_X					(1190)
#define SSR_CLOSE_BTN_Y					(SSR_WIN_SIZE_H - 70)
#define SSR_CLOSE_BTN_W					(174)

enum {
	SSR_POWER = 0,
	SSR_NETWORK,
	SSR_HDD,
	SSR_PORT,
	SSR_OBJ_MAX
};

static NFWINDOW *model_obj[GUI_CHANNEL_CNT];
static NFWINDOW *power_obj[GUI_CHANNEL_CNT];
static NFWINDOW *net_obj[GUI_CHANNEL_CNT];
static NFWINDOW *g_curwnd = 0;
static NFWINDOW *g_parent = 0;

static NFOBJECT *g_fixed2;
static NFOBJECT *g_fixed3;
static NFOBJECT *infoBtns_obj[GUI_CHANNEL_CNT];
static NFOBJECT *in_errorBtns_obj[IN_DISP_DISK_COUNT];
static NFOBJECT *ex_errorBtns_obj[EX_DISP_DISK_COUNT];
static NFOBJECT *pw_errorBtns_obj[GUI_CHANNEL_CNT];
static NFOBJECT *nt_errorBtns_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_poe_tot_obj;
static NFOBJECT *g_hub_poe_tot_obj;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static SELF_RESULT_T *g_result;

static NFIPCamModelInfo ipcam_model[GUI_CHANNEL_CNT];
static NFIPCamPortStatus cam_stats[GUI_CHANNEL_CNT];


static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
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
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
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

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

		if (index == 0)		//INTERNAL DISK
		{
			nfui_nfobject_hide(g_fixed3);
			nfui_nfobject_show(g_fixed2);
		}
		else if (index == 1)	//EXTERNAL DISK
		{
			nfui_nfobject_hide(g_fixed2);
			nfui_nfobject_show(g_fixed3);
		}
			nfui_signal_emit(g_fixed2, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_fixed3, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean  post_power_error_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		  nftool_mbox(g_curwnd, "NOTICE", "There is a problem with the camera power supply.\n If you are not using POE extender, you need to check the camera.", NFTOOL_MB_OK);
	}

	return FALSE;
}
static gboolean  post_net_error_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		  nftool_mbox(g_curwnd, "NOTICE", "Camera does not facilitate communication.\n Review and communicate the status of the entire cable line,\n you need to check the network usage.", NFTOOL_MB_OK);
	}
	return FALSE;
}

static gboolean  post_in_error_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NF_SMART_DISK_INFO sector_info;
	NF_DISK_INFO ndi;
	gchar buf[1024];
	gint i;

	memset(buf, 0x00, sizeof(buf));
	
	memset(&ndi, 0x00, sizeof(NF_DISK_INFO));
	nf_disk_get_info(&ndi, NULL);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < IN_DISP_DISK_COUNT; i++)
		{
			memset(&sector_info, 0x00, sizeof(DISK_SMARTINFO_T));

			if (ndi.ucPortNo[0][i] == 0xff) continue;

			if (in_errorBtns_obj[i] == obj)
			{
				if (nf_smart_get_info(0, i, &sector_info, NULL))
				{
					sprintf(buf, lookup_string("Because the state of HDD error, HDD needs to be replaced.\nReallocated sectors count: %lld\nCurrent pending sector count: %lld"), sector_info.reallocation_event_ct.raw, sector_info.current_pending_sector.raw);
					break;
				}
			}
		}
		  nftool_mbox(g_curwnd, "NOTICE", buf, NFTOOL_MB_OK);
	}
	return FALSE;
}

static gboolean  post_ex_error_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NF_SMART_DISK_INFO sector_info;
	NF_DISK_INFO ndi;
	gchar buf[1024];
	gint i;

	memset(buf, 0x00, sizeof(buf));
	
	memset(&ndi, 0x00, sizeof(NF_DISK_INFO));
	nf_disk_get_info(&ndi, NULL);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < IN_DISP_DISK_COUNT; i++)
		{
			memset(&sector_info, 0x00, sizeof(DISK_SMARTINFO_T));

			if (ndi.ucPortNo[1][i] == 0xff) continue;

			if (in_errorBtns_obj[i] == obj)
			{
				if (nf_smart_get_info(1, i, &sector_info, NULL))
				{
					sprintf(buf, lookup_string("Because the state of HDD error, HDD needs to be replaced.\nReallocated sectors count: %lld\nCurrent pending sector count: %lld"), sector_info.reallocation_event_ct.raw, sector_info.current_pending_sector.raw);
				break;
				}
			}
		}
		  nftool_mbox(g_curwnd, "NOTICE", buf, NFTOOL_MB_OK);
	}
	return FALSE;
}

static gboolean  post_rtsp_error_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		  nftool_mbox(g_curwnd, "NOTICE", "Failed to register the service port router.", NFTOOL_MB_OK);
	}
	return FALSE;
}
static gboolean  post_web_error_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		  nftool_mbox(g_curwnd, "NOTICE", "Failed to register the service port router.", NFTOOL_MB_OK);
	}
	return FALSE;
}

static gboolean post_camera_info_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, sel_item;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		for(i = 0 ; i < GUI_CHANNEL_CNT; i++)
		{
			if(infoBtns_obj[i] == obj)
			{
				sel_item = i;
				break;
			}
		}
			open_vw_sys_net_detail_popup(g_curwnd, sel_item);
	}

	return FALSE;
}

static gboolean post_retrybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
				
		VW_System_Self_Check_Open(g_curwnd, g_result->chkMask);
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	else if(evt->type == NFEVENT_KEYPAD_RELEASE || evt->type == NFEVENT_REMOCON_RELEASE)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_EXIT)
		{
			NFOBJECT *top = NULL;

			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

   		nfui_nfobject_get_size(obj, &size_w, &size_h);
    		pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
    		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}

	else if(evt->type == INFY_POE_NOTIFY)
	{
		NF_NOTIFY_INFO *pInfo;	
		gfloat val;
		gchar strSum[10];

		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
        	val = (float)pInfo->d.params[3]/1000.0;		

		memset(strSum, 0x00, sizeof(strSum));

		if (val == 0)       	
			g_sprintf(strSum, "%dW", 0);
		else if (val < 10)  
			g_sprintf(strSum, "%.1fW", val);
		else
			g_sprintf(strSum, "%dW", (gint)val);

		nfui_nflabel_set_text((NFLABEL*)g_poe_tot_obj, strSum);
		nfui_signal_emit(g_poe_tot_obj, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == INFY_POE_HUB_NOTIFY)
	{
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
		NF_NOTIFY_INFO *pInfo;	
		gfloat val;
		gchar strSum[10];

		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
        	val = (float)pInfo->d.params[3]/1000.0;		

		memset(strSum, 0x00, sizeof(strSum));

		if (val == 0)       	
			g_sprintf(strSum, "%dW", 0);
		else if (val < 10)  
			g_sprintf(strSum, "%.1fW", val);
		else
			g_sprintf(strSum, "%dW", (gint)val);

		nfui_nflabel_set_text((NFLABEL*)g_hub_poe_tot_obj, strSum);
		nfui_signal_emit(g_hub_poe_tot_obj, GDK_EXPOSE, TRUE);
#endif	
	}
	else if (evt->type == GDK_DELETE)
	{
    		nfui_nfobject_get_size(obj, &size_w, &size_h);
    		nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

		uxm_unreg_imsg_event(obj, INFY_POE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_POE_HUB_NOTIFY);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_POWER);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_NET);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_HDD);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_PORT);
	}
	return FALSE;
}

static gboolean subwin_returnkey_proc(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	return FALSE;
}

gboolean VW_System_Self_Result_Open(NFWINDOW *parent, SELF_RESULT_T *result)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *tbl;
	NFOBJECT *cam_ch[GUI_CHANNEL_CNT];
	NFOBJECT *fixedTemp;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

	GdkPixbuf *pbCamImage[32];
	GdkPixbuf *pbIcon[IN_DISP_DISK_COUNT + 1];
	GdkPixbuf *info_btn_img[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *error_btn_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	GSList *slist = NULL;
	GError *err = NULL;
	SMT_SERVICE_E status;

	g_result = result;

	guint i;
	gint title_size;
	gint size_w, size_h;
	gint width = 0;
	gint width2 = 0;
	gint width3 = 0;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

	gchar strBuf[STRING_SIZE_CAMTITLE];

	gchar *strTitle[SSR_TABLE_COL] = {
				"",
	    	   	"MODEL",
	   	    	"POWER CONSUMPTION",
				"NETWORK",
	    	   	""
	};

	gchar *strLabel1[] = {
				"INTERNAL DISK",
	       		"EXTERNAL DISK"
	};

	gchar *strLabel2[] = {
				"DISK1",
	       		"DISK2",
				"DISK3",
				"DISK4",
				"DISK5"
	};

	guint table_w[SSR_TABLE_COL] = {0, };
	guint table_w2[SSR_TABLE2_COL] = {0, };
	guint table_w3[SSR_TABLE3_COL] = {0, };

	table_w[0] = 180;
	table_w[1] = 300;
	table_w[2] = 180;
	table_w[3] = 180;
	table_w[4] = 40;

	table_w2[0] = 40;
	table_w2[1] = 180;

	table_w3[0] = 150;
	table_w3[1] = 180;

	title_size = SSR_WIN_SIZE_W -20;

// CAMERA IMAGE LOAD
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
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL);
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);  
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL); 
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL);
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);      
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL); 
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL);
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);  
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL); 
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL); 

// ERROR IMAGE LOAD
 	error_btn_img[0] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_N, NULL);
	error_btn_img[1] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_O, NULL);
	error_btn_img[2] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_P, NULL);
	error_btn_img[3] = nfui_get_image_from_file(IMG_SELF_DIAGNOSIS_ISSUE_D, NULL);

// CAM INFO IMAGE LOAD
	info_btn_img[0] = nfui_get_image_from_file(IMG_BTN_N_POPUP_MEMO, NULL);
	info_btn_img[1] = nfui_get_image_from_file(IMG_BTN_O_POPUP_MEMO, NULL);
	info_btn_img[2] = nfui_get_image_from_file(IMG_BTN_P_POPUP_MEMO, NULL);
	info_btn_img[3] = nfui_get_image_from_file(IMG_BTN_D_POPUP_MEMO, NULL);

// RADIO IMAGE LOAD
	radio_img[0] = nfui_get_image_from_file(IMG_N_POPUP_RADIO_OFF, NULL);
	radio_img[1] = nfui_get_image_from_file(IMG_O_POPUP_RADIO_ON, NULL);
	radio_img[2] = nfui_get_image_from_file(IMG_P_POPUP_RADIO_ON, NULL);
	radio_img[3] = nfui_get_image_from_file(IMG_D_POPUP_RADIO_OFF, NULL);

// DISK IMAGE LOAD
	pbIcon[0] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON1, NULL); 
	pbIcon[1] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON2, NULL); 
	pbIcon[2] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON3, NULL); 
	pbIcon[3] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON4, NULL); 
	pbIcon[4] = nfui_get_image_from_file(IMG_OUTPUT_STORAGE_ICON5, NULL); 

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    g_parent = parent;
    
// <---- window
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)((DISPLAY_ACTIVE_WIDTH-SSR_WIN_SIZE_W)/2), (guint)((DISPLAY_ACTIVE_HEIGHT-SSR_WIN_SIZE_H)/2), SSR_WIN_SIZE_W, SSR_WIN_SIZE_H);
	nfui_nfwindow_set_title((NFWINDOW*)win, "SELF-DIAGNOSIS RESULT");
	nfui_nfwindow_use_double_buffer((NFWINDOW*)win);
	g_curwnd = win;
	nfui_regi_post_event_callback(win, post_main_win_event_handler);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, subwin_returnkey_proc);


// <---- fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, SSR_WIN_SIZE_W, SSR_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);
	nfui_nfobject_show(fixed);

// <---- title
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SELF-DIAGNOSIS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, title_size, SSR_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSR_TITLE_X, SSR_TITLE_Y);

	// POWER CONSUMPTION / NETWORK TITLE.
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "POWER CONSUMPTION / NETWORK");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSR_TABLE_X + 5, (SSR_TITLE_H + SSR_LABEL_SPACE));
	
	// HDD TITLE.

	for (i = 0; i < SSR_TABLE_COL; i++)
	{
		width = width + table_w[i];
	}

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "HDD");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSR_TABLE_X + width + 75), (SSR_TITLE_H + SSR_LABEL_SPACE));
	
	// SERVICE PORT TITLE.
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SERVICE PORT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSR_TABLE_X + width + 75),(SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + IN_DISP_DISK_COUNT + 1)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 4))-10);


// <---- POWER / NET TABEL
 	tbl = (NFOBJECT*)nfui_nftable_new(SSR_TABLE_COL, SSR_TABLE_ROW, SSR_TABLE_COL_SPACE, SSR_TABLE_ROW_SPACE, table_w, SSR_LABEL_H);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, (SSR_TABLE_X + 30), (SSR_TITLE_H + SSR_LABEL_H + (SSR_LABEL_SPACE * 2) -10));

// <---- POWER / NET
	for (i = 0; i < SSR_TABLE_COL; i++)
	{
		if ((i != 0) && (i != 4) )
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));		
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)tbl, obj, i, 0);
		}
	}
	
    size_w = 0;
    for (i = 0; i < SSR_TABLE_COL; i++) {
        size_w += table_w[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, main_page_fixed, (SSR_TABLE_X + 30), (SSR_TITLE_H + SSR_LABEL_H + (SSR_LABEL_SPACE * 2) -10 + SSR_LABEL_H));

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(SSR_TABLE_COL, ROW_CNT_PER_PAGE, 2, 1, table_w, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

	memset(cam_stats, 0x00, sizeof(cam_stats));

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		// CHANNEL
		memset(strBuf, 0x00, sizeof(strBuf));
		DAL_get_camera_title(strBuf, i);

		obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);

		// MODEL
		nf_ipcam_get_port_status(i, &cam_stats[i], &err);

		if(cam_stats[i].device_class != 0)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(cam_stats[i].model, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
		}
		else
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
		}
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 1, row_num);

		//FIXED POWER
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_show(fixedTemp);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], fixedTemp, 2, row_num);

		if(result->chkMask & (1 << MSK_POWER))
		{
			if (result->power[i] == -1)	//CHECK ERROR
			{
				nfui_get_pixbuf_size(error_btn_img[0], &size_w, &size_h);
				obj = (NFOBJECT*)nfui_nfbutton_new();
				nfui_nfbutton_set_image(NF_BUTTON(obj), error_btn_img);
				nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (table_w[2]-size_w)/2, (SSR_LABEL_H-size_h)/2);
				pw_errorBtns_obj[i] = obj;
				nfui_regi_post_event_callback(obj, post_power_error_button_handler);
			}
			else if (result->power[i] == 0)	//CHECK N/A
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w[2], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
			else if (result->power[i] == 1)	//CHECK SUCCESS
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w[2], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
		}
		else
		{	
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, table_w[2], SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}

		//FIXED NET
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_show(fixedTemp);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], fixedTemp, 3, row_num);

		if(result->chkMask & (1 << MSK_NET))
		{
			if (result->net[i] == -1)	//CHECK ERROR
			{
				nfui_get_pixbuf_size(error_btn_img[0], &size_w, &size_h);
				obj = (NFOBJECT*)nfui_nfbutton_new();
				nfui_nfbutton_set_image(NF_BUTTON(obj), error_btn_img);
				nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (table_w[3]-size_w)/2, (SSR_LABEL_H-size_h)/2);
				nt_errorBtns_obj[i] = obj;
				nfui_regi_post_event_callback(obj, post_net_error_button_handler);
			}
			else if (result->net[i] == 0)	//CHECK N/A
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w[3], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
			else if (result->net[i] == 1)	//CHECK SUCCESS
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w[3], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
		}
		else
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, table_w[3], SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}

		// cam info
		nfui_get_pixbuf_size(info_btn_img[0], &size_w, &size_h);
		obj = (NFOBJECT*)nfui_nfbutton_new();
		infoBtns_obj[i] = obj;
		nfui_nfbutton_set_image(NF_BUTTON(obj), info_btn_img);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nfobject_disable(obj);

		status = smt_get_service();

		if (status == SMT_SYSTEM_SETUP)
		{
			if (cam_stats[i].device_class != 0 && cam_stats[i].status == 0)
			{
				if (DAL_get_cam_install_mode() == 0 && result->power[i] == 1 && result->net[i] == 1)	//CCTV MODE
				{
					nfui_nfobject_enable(obj);
				}
				else if (DAL_get_cam_install_mode() == 1 && result->net[i] == 1)	//OPEN MODE
				{
					nfui_nfobject_enable(obj);
				}
			}
		}
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 4, row_num);
		nfui_regi_post_event_callback(obj, post_camera_info_button_handler);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

// <---- CHOICE HDD TABEL

 	tbl = (NFOBJECT*)nfui_nftable_new(SSR_TABLE2_COL, SSR_TABLE2_ROW, SSR_TABLE_COL_SPACE, SSR_TABLE_ROW_SPACE, table_w2, SSR_LABEL_H);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, (SSR_TABLE_X + width + 100), (SSR_TITLE_H + SSR_LABEL_H + SSR_LABEL_SPACE + 10));

 	for (i = 0; i < LABEL_DISK_CNT; i++)
 	{
		//CHOICE DISK
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_show(fixedTemp);
		nfui_nftable_attach((NFTABLE*)tbl, fixedTemp, 0, i);	

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_get_size(obj, &size_w, &size_h);	  	
		nfui_nfobject_show(obj);
		nfui_nfobject_disable(obj);
		nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (table_w2[0]-size_w)/2, (table_w2[0]-size_h)/2);
		nfui_regi_post_event_callback(obj, post_radio_event_handler);

		if((result->chkMask & (1 << MSK_HDD)))
		{
			nfui_nfobject_enable(obj);

			if (i == 0)
			{
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
				slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			}
			else
			{
				nfui_radio_button_add_group(NF_BUTTON(obj), slist);
			}
		}


    		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel1[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);		
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i);
	}

// <---- HDD LIST INTERNAL
 	for (i = 0; i < SSR_TABLE2_COL; i++)
	{
		width2 = width2 + table_w3[i];
	}

	g_fixed2 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(g_fixed2, width2, (SSR_LABEL_H * IN_DISP_DISK_COUNT));
	nfui_nfobject_show(g_fixed2);
	nfui_nffixed_put((NFFIXED*)fixed, g_fixed2, (SSR_TABLE_X + width + 100), (SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + 1)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 2)-5));

 	tbl = (NFOBJECT*)nfui_nftable_new(SSR_TABLE3_COL, SSR_TABLE3_ROW, SSR_TABLE_COL_SPACE, SSR_TABLE_ROW_SPACE, table_w3, SSR_LABEL_H);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)g_fixed2, tbl, 0, 0);

	for (i = 0; i < IN_DISP_DISK_COUNT; i++)
	{
		// DISK
		obj = (NFOBJECT*)nfui_nfimglabel_new(pbIcon[i], strLabel2[i]);
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);

		//FIXED DISK
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_show(fixedTemp);
		nfui_nftable_attach((NFTABLE*)tbl, fixedTemp, 1, i);

		if(result->chkMask & (1 << MSK_HDD))
		{
			if (result->hdd_in[i] == -2)	//CHECK CHECK
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHECK", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
			else if (result->hdd_in[i] == -1)	//CHECK ERROR
			{
				nfui_get_pixbuf_size(error_btn_img[0], &size_w, &size_h);
				obj = (NFOBJECT*)nfui_nfbutton_new();
				nfui_nfbutton_set_image(NF_BUTTON(obj), error_btn_img);
				nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (table_w3[1]-size_w)/2, (SSR_LABEL_H-size_h)/2);
				in_errorBtns_obj[i] = obj;
				nfui_regi_post_event_callback(in_errorBtns_obj[i], post_in_error_button_handler);
			}
			else if (result->hdd_in[i] == 0)	//CHECK N/A
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
			else if (result->hdd_in[i] == 1)	//CHECK SUCCES
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
		}
		else
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}
	}

// <---- HDD LIST EXTERNAL
 	for (i = 0; i < SSR_TABLE3_COL; i++)
	{
		width3 = width3 + table_w3[i];
	}

	g_fixed3 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(g_fixed3, width3, (SSR_LABEL_H * EX_DISP_DISK_COUNT));
	nfui_nfobject_hide(g_fixed3);
	nfui_nffixed_put((NFFIXED*)fixed, g_fixed3, (SSR_TABLE_X + width + 100), (SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + 1)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 2)-5));

	tbl = (NFOBJECT*)nfui_nftable_new(SSR_TABLE3_COL, SSR_TABLE3_ROW, SSR_TABLE_COL_SPACE, SSR_TABLE_ROW_SPACE, table_w3, SSR_LABEL_H);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)g_fixed3, tbl, 0, 0);

	for (i = 0; i < EX_DISP_DISK_COUNT; i++)
	{
		// DISK
		obj = (NFOBJECT*)nfui_nfimglabel_new(pbIcon[i], strLabel2[i]);
		nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);

		//FIXED DISK
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_show(fixedTemp);
		nfui_nftable_attach((NFTABLE*)tbl, fixedTemp, 1, i);

		if(result->chkMask & (1 << MSK_HDD))
		{
			if (result->hdd_ex[i] == -2)	//CHECK CHECK
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHECK", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
			else if (result->hdd_ex[i] == -1)	//CHECK ERROR
			{
				nfui_get_pixbuf_size(error_btn_img[0], &size_w, &size_h);
				obj = (NFOBJECT*)nfui_nfbutton_new();
				nfui_nfbutton_set_image(NF_BUTTON(obj), error_btn_img);
				nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (table_w3[1]-size_w)/2, (SSR_LABEL_H-size_h)/2);
				ex_errorBtns_obj[i] = obj;
				nfui_regi_post_event_callback(ex_errorBtns_obj[i], post_ex_error_button_handler);
			}
			else if (result->hdd_ex[i] == 0)	//CHECK N/A
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
			else if (result->hdd_ex[i] == 1)	//CHECK SUCCES
			{
				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
				nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
			}
		}
		else
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, table_w3[1], SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}
	}

// <---- PORT
	// RTSP SERVICE PORT
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RTSP SERVICE PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, (SSR_LABEL_W - 180), SSR_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj,(SSR_TABLE_X + width + 100), (SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + IN_DISP_DISK_COUNT + 2)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 5)-10));

	//FIXED RTSP
	fixedTemp = nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_set_size(fixedTemp, (SSR_LABEL_W - 260), SSR_LABEL_H);
	nfui_nfobject_show(fixedTemp);
	nfui_nffixed_put((NFFIXED*)fixed, fixedTemp,(SSR_TABLE_X + width + 322), (SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + IN_DISP_DISK_COUNT + 2)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 5)-10));

	if(result->chkMask & (1 << MSK_PORT))
	{
		if (result->rtsp_port == -1)	//CHECK ERROR
		{
			nfui_get_pixbuf_size(error_btn_img[0], &size_w, &size_h);
			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), error_btn_img);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, ((SSR_LABEL_W - 260)-size_w)/2, (SSR_LABEL_H-size_h)/2);
			nfui_regi_post_event_callback(obj, post_rtsp_error_button_handler);
		}
		else if (result->rtsp_port == 0)	//CHECK N/A
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, (SSR_LABEL_W - 260), SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}
		else if (result->rtsp_port == 1)	//CHECK SUCCES
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, (SSR_LABEL_W - 260), SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}
	}
	else
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, (SSR_LABEL_W - 260), SSR_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
	}
	
	// WEB SERVICE PORT
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WEB SERVICE PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, (SSR_LABEL_W - 180), SSR_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj,(SSR_TABLE_X + width + 100), (SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + IN_DISP_DISK_COUNT + 3)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 5))-8);
	
	//FIXED WEB
	fixedTemp = nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_set_size(fixedTemp, (SSR_LABEL_W - 260), SSR_LABEL_H);
	nfui_nfobject_show(fixedTemp);
	nfui_nffixed_put((NFFIXED*)fixed, fixedTemp, (SSR_TABLE_X + width + 322), (SSR_TITLE_H + (SSR_LABEL_H * (LABEL_DISK_CNT + IN_DISP_DISK_COUNT + 3)) + SSR_TABLE_ROW_SPACE + (SSR_LABEL_SPACE * 5))-8);

	if(result->chkMask & (1 << MSK_PORT))
	{
		if (result->web_port == -1)	//CHECK ERROR
		{
			nfui_get_pixbuf_size(error_btn_img[0], &size_w, &size_h);
			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), error_btn_img);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, ((SSR_LABEL_W - 260)-size_w)/2, (SSR_LABEL_H-size_h)/2);
			nfui_regi_post_event_callback(obj, post_web_error_button_handler);
		}
		else if (result->web_port == 0)	//CHECK N/A
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, (SSR_LABEL_W - 260), SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}
		else if (result->web_port == 1)	//CHECK SUCCES
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, (SSR_LABEL_W - 260), SSR_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
		}
	}
	else
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, (SSR_LABEL_W - 260), SSR_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixedTemp, obj, 0, 0);
	}
// <---- POE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("POE POWER SUPPLY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, (SSR_LABEL_W - 170), SSR_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSR_TABLE_X + 30), (SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+1)) + 130));

	obj =  (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, (SSR_LABEL_W - 290), SSR_LABEL_H);
	nfui_nfobject_show(obj);
	g_poe_tot_obj = obj;
	nfui_nffixed_put((NFFIXED*)fixed, obj,  (SSR_TABLE_X + 262), (SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+1)) + 130));

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
	// <---- POE HUB
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("POE POWER SUPPLY(HUB)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, (SSR_LABEL_W - 115), SSR_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSR_TABLE_X + 380), (SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+1)) + 130));

	obj =  (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, (SSR_LABEL_W - 290), SSR_LABEL_H);
	nfui_nfobject_show(obj);
	g_hub_poe_tot_obj = obj;
	nfui_nffixed_put((NFFIXED*)fixed, obj,  (SSR_TABLE_X + 667), (SSR_TITLE_H + (SSR_LABEL_H * (GUI_CHANNEL_CNT+1)) + 130));
#endif

// <---- RETRY BUTTON
	obj = nftool_normal_button_create_type1("RETRY", SSR_RETRY_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSR_RETRY_BTN_X, SSR_RETRY_BTN_Y);
	nfui_regi_post_event_callback(obj, post_retrybutton_event_handler);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", SSR_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSR_CLOSE_BTN_X, SSR_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	uxm_reg_imsg_event(fixed, INFY_POE_NOTIFY);
	uxm_reg_imsg_event(fixed, INFY_POE_HUB_NOTIFY);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_POWER);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_NET);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_HDD);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_PORT);

	uxm_monitor_on_imsg_event(fixed, INFY_POE_NOTIFY);
	uxm_monitor_on_imsg_event(fixed, INFY_POE_HUB_NOTIFY);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_POWER);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_NET);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_HDD);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_PORT);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_SELF_CHECK_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_SELF_CHECK_POPUP, win);
}

void VW_System_Self_Result_Close()
{
    if (!g_curwnd) return;
    
    evt_send_to_window("SELF-DIAGNOSIS RESULT", WND_CLOSE, 0, 0, 0);
}

