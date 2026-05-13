#include "nf_afx.h"
#include "nf_api_openmode.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfprogressbar.h"

#include "vw_nvm_msg_popup.h"
#include "scm.h"
#include "smt.h"
#include "nvm.h"

#define NMSG_WIN_SIZE_W						(534)   //fixed.
#define NMSG_WIN_SIZE_H						(350)
#define NMSG_WIN_POS_X						((DISPLAY_ACTIVE_WIDTH - NMSG_WIN_SIZE_W)/2)
#define NMSG_WIN_POS_Y						((DISPLAY_ACTIVE_HEIGHT - NMSG_WIN_SIZE_H)/2)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_gearimage_obj = NULL;
static NFOBJECT *g_prglabel_obj = NULL;
static NFOBJECT *g_prgimage_obj = NULL;
static NFOBJECT *g_rmsg_label = NULL;

static gint g_gear_timer = 0;
static gint g_is_scanning = 0;


static gint _show_gear_object()
{    
    nfui_nfobject_show(g_gearimage_obj);
    nfui_nfobject_hide(g_prglabel_obj);
    nfui_nfobject_hide(g_prgimage_obj);

    nfui_signal_emit(g_gearimage_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_prglabel_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_prgimage_obj, GDK_EXPOSE, TRUE);
    
    return 0;
}

static gboolean _proc_gear_timer(gpointer data)
{
    static gint img_idx = 0;

    if (img_idx >= 6) img_idx = 0;

    if (img_idx == 0) nfui_nfimage_change_image((NFIMAGE*)g_gearimage_obj, IMG_GEAR_01);
    else if(img_idx == 1) nfui_nfimage_change_image((NFIMAGE*)g_gearimage_obj, IMG_GEAR_02);
    else if(img_idx == 2) nfui_nfimage_change_image((NFIMAGE*)g_gearimage_obj, IMG_GEAR_03);
    else if(img_idx == 3) nfui_nfimage_change_image((NFIMAGE*)g_gearimage_obj, IMG_GEAR_04);
    else if(img_idx == 4) nfui_nfimage_change_image((NFIMAGE*)g_gearimage_obj, IMG_GEAR_05);
    else if(img_idx == 5) nfui_nfimage_change_image((NFIMAGE*)g_gearimage_obj, IMG_GEAR_06);

    nfui_signal_emit(g_gearimage_obj, GDK_EXPOSE, TRUE);

    img_idx++;

    return TRUE;
}

static gint _run_gear_timer()
{
    if (g_gear_timer) return 0;

    g_gear_timer = g_timeout_add(200, _proc_gear_timer, 0);

    return 0;
}

static gint _stop_gear_timer()
{
    if (g_gear_timer)
    {
        g_source_remove(g_gear_timer);
        g_gear_timer = 0;
    }

    return 0;
}

static NFOpenmodeCamInfo *_find_matched_info_by_idx(NFOpenmodeDeviceList *dlist, gint f_index)
{
	NFOpenmodeCamInfo *info;
	gint i;

	if (f_index >= dlist->entry_cnt) return NULL;

	info = dlist->head;

	for (i = 0; i < dlist->entry_cnt; i++)
	{
		if (info->index == f_index) return info;		
		
		info = info->next;
	}

	return NULL;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar strBuf[256];

    switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable = NULL;
			GdkGC *gc = NULL;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);
			nfutil_draw_image(drawable, gc, MK_IMG_POPUP_OSD_BG, (gint)obj->x, (gint)obj->y, (gint)obj->width, (gint)obj->height, NFALIGN_LEFT, 0);				
			nfui_nfobject_gc_unref(gc);
		}
		break;

        case INFY_OPENMODE_ENTER_INSTALL_BY_NVM:
        {
			g_message("INFY_OPENMODE_ENTER_INSTALL_BY_NVM %s, %d, smt : %d", __FUNCTION__, __LINE__, smt_get_service());
        
			if (smt_get_service() != SMT_LIVE && smt_get_service() != SMT_LOGOUT) {
			    nvm_fail_remote_cam_install();
			    break;
		    }

            ssm_fakestop_auto_logout();
			_show_gear_object();
			vsm_change_hide_video();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "CAMERA INSTALLATION");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);

			_run_gear_timer();
			smt_set_service(SMT_CAM_INSTALL_MODE);
			
	        nf_openmode_init_detection_list();
        	nf_openmode_stop_streaming();
//        	nf_openmode_scan_camera();
            nvm_ready_remote_cam_install();
        }
        break;	
        
    	case INFY_IPCAM_INSTALL_NOTIFY:
    	{
    		NFOpenmodeDeviceList *dlist = NULL;
    		NF_NOTIFY_INFO *pInfo;
    		
			if (smt_get_service() != SMT_CAM_INSTALL_MODE) break;

    		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
    		
            if (pInfo->d.params[0] == 1)
            {
                g_is_scanning = 1;
    		}

    		dlist = nf_openmode_get_list();
    		if (!dlist) return FALSE;
    		g_message("###INFY_IPCAM_INSTALL_NOTIFY  : %s, %d pInfo->d.params[0] : %d, entry_cnt : %d", __FUNCTION__, __LINE__, pInfo->d.params[0], dlist->entry_cnt);

            if (pInfo->d.params[0] == 1 && g_is_scanning)           //is scanning
            {
    		    nvm_update_oneself_cam_info();
    		}
    		else if (pInfo->d.params[0] == 1 || !g_is_scanning)     //can not exist
    		{
    		}
    		else if (pInfo->d.params[0] == 0 || g_is_scanning)      //finish scan
    		{
    		    nvm_update_oneself_cam_info();
                nvm_ready_remote_cam_install();
    		    g_is_scanning = 0;
    		}
    		else if (pInfo->d.params[0] == 0 || !g_is_scanning)     //normal
    		{
    		    nvm_update_oneself_cam_info();
    		}
    	}
    	break;
        
        case INFY_OPENMODE_LEAVE_INSTALL_BY_NVM:        
        {
			g_message("INFY_OPENMODE_LEAVE_INSTALL_BY_NVM %s, %d", __FUNCTION__, __LINE__);
        
			if (smt_get_service() != SMT_CAM_INSTALL_MODE) break;

            _stop_gear_timer();        
			nfui_nfobject_hide(obj->parent);
			vsm_change_show_video();				
			ssm_fakestart_auto_logout();
			smt_return_to_previous();
			g_message("%s, %d", __FUNCTION__, __LINE__);
        }
        break;
	        
        case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, INFY_OPENMODE_ENTER_INSTALL_BY_NVM);
			uxm_unreg_imsg_event(obj, INFY_OPENMODE_LEAVE_INSTALL_BY_NVM);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_INSTALL_NOTIFY);
		}
		break;
		
		default :
		break;
	}
	
	return FALSE;
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) 
	{
		g_curwnd = 0;
	}

	return FALSE;
}

void VW_Create_NVM_Msg_Popup(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *main_fixed;
	NFOBJECT *sub_fixed;	
	NFOBJECT *obj;
	GdkPixbuf *bgImg;
	GdkPixbuf *pbBG;
	GdkPixbuf *prog_img[4];

	gint pos_x, pos_y;
	gint img_w, img_h;


	bgImg = nf_ui_create_image_remote_msg_popup(IMG_POPUP_OSD_BG, MK_IMG_POPUP_OSD_BG, NMSG_WIN_SIZE_H);

	
	win = (NFOBJECT*)nfui_nfwindow_new(parent, NMSG_WIN_POS_X, NMSG_WIN_POS_Y, NMSG_WIN_SIZE_W, NMSG_WIN_SIZE_H);
	nfui_regi_post_event_callback(win, post_window_event_cb);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, NMSG_WIN_SIZE_W, NMSG_WIN_SIZE_H);
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(main_fixed, post_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	pos_x = 4;
	pos_y = 100;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, NMSG_WIN_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, NMSG_WIN_SIZE_W-12, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "Remote setup is in progress.");	
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 12);
	nfui_nfobject_set_size(obj, NMSG_WIN_SIZE_W-12, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_x += 10;
	pos_y += 40;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 20, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, 20, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, ":");		
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 12);	
	nfui_nfobject_set_size(obj, 20, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_x += 24;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, NMSG_WIN_SIZE_W-pos_x*2, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, NMSG_WIN_SIZE_W-pos_x*2, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 0);	
	nfui_nfobject_set_size(obj, NMSG_WIN_SIZE_W-pos_x*2, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_rmsg_label = obj;


// progressbar object
	pos_x = 4;   
	pos_y = NMSG_WIN_SIZE_H-150;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, NMSG_WIN_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, NMSG_WIN_SIZE_W-12, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 12);
	nfui_nfobject_set_size(obj, NMSG_WIN_SIZE_W-12, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_prglabel_obj = obj;

    pos_y += 42;

	prog_img[0] = nf_ui_create_image_progress_bg(IMG_PROGRESS_BG, MK_IMG_PROGRESS_BG, NMSG_WIN_SIZE_W-16);
	prog_img[1] = nfui_get_image_from_file((IMG_PROGRESS_HEAD), NULL);
	prog_img[2] = nfui_get_image_from_file((IMG_PROGRESS_MIDDLE), NULL);
	prog_img[3] = nfui_get_image_from_file((IMG_PROGRESS_TAIL), NULL);

	obj = nfui_nfprogressbar_new_with_images(prog_img[0], prog_img[1], prog_img[2], prog_img[3]);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+4, pos_y);
    g_prgimage_obj = obj;

	
// gear object
	pos_x = 4;   
	pos_y = NMSG_WIN_SIZE_H-150;

    nfui_get_image_size(IMG_GEAR_01, &img_w, &img_h); 

	obj = (NFOBJECT*)nfui_nfimage_new(IMG_GEAR_01);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (NMSG_WIN_SIZE_W-img_w)/2, pos_y);
    g_gearimage_obj = obj;


// wait object
	pos_x = 4;   
	pos_y = NMSG_WIN_SIZE_H-50;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, NMSG_WIN_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, NMSG_WIN_SIZE_W-12, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "Please wait...");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfobject_set_size(obj, NMSG_WIN_SIZE_W-12, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_hide(win);

	uxm_reg_imsg_event(main_fixed, INFY_OPENMODE_ENTER_INSTALL_BY_NVM);	
	uxm_reg_imsg_event(main_fixed, INFY_OPENMODE_LEAVE_INSTALL_BY_NVM);	
	uxm_monitor_on_imsg_event(main_fixed, INFY_OPENMODE_ENTER_INSTALL_BY_NVM);
	
	uxm_reg_imsg_event(main_fixed, INFY_IPCAM_INSTALL_NOTIFY);	
}


