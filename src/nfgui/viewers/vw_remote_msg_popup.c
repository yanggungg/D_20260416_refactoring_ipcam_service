#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfprogressbar.h"

#include "vw_remote_msg_popup.h"
#include "vw_internal.h"
#include "ssm.h"
#include "scm.h"

#define RMSG_WIN_SIZE_W						(534)   //fixed.
#define RMSG_WIN_SIZE_H						(350)
#define RMSG_WIN_POS_X						((DISPLAY_ACTIVE_WIDTH - RMSG_WIN_SIZE_W)/2)
#define RMSG_WIN_POS_Y						((DISPLAY_ACTIVE_HEIGHT - RMSG_WIN_SIZE_H)/2)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_gearimage_obj = NULL;
static NFOBJECT *g_prglabel_obj = NULL;
static NFOBJECT *g_prgimage_obj = NULL;
static NFOBJECT *g_rmsg_label = NULL;

static gint g_gear_timer = 0;


static gint _init_pw_changed_time()
{
	gint i;
	GTimeVal last_temp;
	
	g_get_current_time(&last_temp);
	
	for(i=0; i<MAX_USER_COUNT; i++)
		DAL_set_pw_last_changed_time(last_temp.tv_sec, i);

	DAL_save_db("usr");

	return 0;
}

static gint _init_password()
{
	SecurityData secdata;

	DAL_get_security_data(&secdata);

	if (!secdata.usable_defpw)
	{
        UserManageData usrdata;

        DAL_get_userManage_data(&usrdata, 0);
        
        if (strcmp(usrdata.pw, "1234") == 0)
        {
            vw_init_userinfo_open(g_curwnd);
        }
	}
	else
	{
		_init_pw_changed_time();
	}

	return 0;
}

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

static gint _show_progress_object()
{
	nfui_nfimage_set_text((NFIMAGE*)g_prglabel_obj, "");
	nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prgimage_obj, 0);    

    nfui_nfobject_show(g_prglabel_obj);
    nfui_nfobject_show(g_prgimage_obj);
    nfui_nfobject_hide(g_gearimage_obj);

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

        case INFY_FORMAT_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "DISK FORMAT");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);	

			_run_gear_timer();
        }
        break;

        case INFY_FACDEF_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);

			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "FACTORY DEFAULT");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);

			_run_gear_timer();
        }
        break;

        case INFY_TIMECHANGE_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "TIME CHANGE");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);

			_run_gear_timer();
        }
        break;

        case INFY_NETCHANGE_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();
			
			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "NETWORK CHANGE");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);

			_run_gear_timer();
        }
        break;

        case INFY_SVCRESTART_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "FILESYSTEM RESTART");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);

			_run_gear_timer();
        }
        break;

        case INFY_DBIMPORT_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "SYSTEM DATA LOAD");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
			{
				nfui_nfobject_show(obj->parent);
				nfui_page_open(PGID_SYS_LOAD_DATA_POPUP, g_curwnd, ssm_get_cur_id(NULL));
			}

			_run_gear_timer();
        }
        break;

        case INFY_ARCH_BURN_API_BEGIN:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_progress_object();
			
			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "ARCHIVING");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);
        }
        break;        

        case INFY_OPENMODE_ENTER_INSTALL:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();
			vsm_change_hide_video();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "CAMERA INSTALLATION");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);

			_run_gear_timer();
        }
        break;	
        
        case INFY_OPENMODE_LEAVE_INSTALL:        
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

            _stop_gear_timer();        
			nfui_nfobject_hide(obj->parent);
			vsm_change_show_video();				
        }
        break;
	        
        case INFY_FORMAT_API_CMPL:
        case INFY_TIMECHANGE_API_CMPL:
        case INFY_NETCHANGE_API_CMPL:
        case INFY_SVCRESTART_API_CMPL:     
        case INFY_ARCH_BURN_API_CMPL:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;
			
            _stop_gear_timer();        
			nfui_nfobject_hide(obj->parent);
        }
        break;

        case INFY_FACDEF_API_CMPL:
		{
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;
           
			ssm_run_auto_logout();
            _stop_gear_timer();
			nfui_nfobject_hide(obj->parent);
			_init_password();
		}
		break;

        case INFY_DBIMPORT_API_CMPL:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;
           
			ssm_run_auto_logout();        
            _stop_gear_timer();
			nfui_nfobject_hide(obj->parent);
			nfui_page_close(PGID_SYS_LOAD_DATA_POPUP, g_curwnd);
			_init_pw_changed_time();
        }
        break;		

        case INFY_REBOOT_SYSTEM:
        {
			g_message("%s, %d", __FUNCTION__, __LINE__);
        
			if (((CMM_MESSAGE_T *)data)->param == VIEWER_LOCAL) break;

			_show_gear_object();

			nfui_nfimage_set_text((NFIMAGE*)g_rmsg_label, "SYSTEM REBOOT");

			if (nfui_nfobject_is_shown(obj->parent))
				nfui_signal_emit(g_rmsg_label, GDK_EXPOSE, TRUE);
			else			
				nfui_nfobject_show(obj->parent);		

            _run_gear_timer();
        }
        break;	

        case INFY_BURN_ERASING:
        {
			guint rate = ((CMM_MESSAGE_T *)data)->data;
			gchar msg_buf[80];

			if (rate > 100) rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (ERASING %d %%)"), rate);

			nfui_nfimage_set_text((NFIMAGE*)g_prglabel_obj, msg_buf);
			nfui_signal_emit(g_prglabel_obj, GDK_EXPOSE, TRUE);			

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prgimage_obj, rate);
			nfui_signal_emit(g_prgimage_obj, GDK_EXPOSE, TRUE);			
        }
        break;

        case INFY_BURN_EXTRACTING:
        {
			guint rate = ((CMM_MESSAGE_T *)data)->data;
			gchar msg_buf[80];

			if (rate > 100) rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (EXTRACTING %d %%)"), rate);

			nfui_nfimage_set_text((NFIMAGE*)g_prglabel_obj, msg_buf);
			nfui_signal_emit(g_prglabel_obj, GDK_EXPOSE, TRUE);			

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prgimage_obj, rate);
			nfui_signal_emit(g_prgimage_obj, GDK_EXPOSE, TRUE);			
        }
        break;

        case INFY_BURN_PROG:
        {
			guint rate = ((CMM_MESSAGE_T *)data)->data;
			gchar msg_buf[80];

			if (rate > 100) rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (BURNING %d %%)"), rate);

			nfui_nfimage_set_text((NFIMAGE*)g_prglabel_obj, msg_buf);
			nfui_signal_emit(g_prglabel_obj, GDK_EXPOSE, TRUE);			

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prgimage_obj, rate);
			nfui_signal_emit(g_prgimage_obj, GDK_EXPOSE, TRUE);			
        }
        break;

        case INFY_BURN_SUCCESS:
        {
			gchar msg_buf[80];
			guint rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (BURNING %d %%)"), rate);

			nfui_nfimage_set_text((NFIMAGE*)g_prglabel_obj, msg_buf);
			nfui_signal_emit(g_prglabel_obj, GDK_EXPOSE, TRUE);			

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prgimage_obj, rate);
			nfui_signal_emit(g_prgimage_obj, GDK_EXPOSE, TRUE);			
        }
        break;        

        case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, INFY_FORMAT_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_FORMAT_API_CMPL);
			
			uxm_unreg_imsg_event(obj, INFY_FACDEF_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_FACDEF_API_CMPL);
			
			uxm_unreg_imsg_event(obj, INFY_TIMECHANGE_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_TIMECHANGE_API_CMPL);
			
			uxm_unreg_imsg_event(obj, INFY_NETCHANGE_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_NETCHANGE_API_CMPL);
			
			uxm_unreg_imsg_event(obj, INFY_SVCRESTART_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_SVCRESTART_API_CMPL);	

			uxm_unreg_imsg_event(obj, INFY_DBIMPORT_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_DBIMPORT_API_CMPL);	

			uxm_unreg_imsg_event(obj, INFY_ARCH_BURN_API_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_ARCH_BURN_API_CMPL);	

			uxm_unreg_imsg_event(obj, INFY_OPENMODE_ENTER_INSTALL);	
			uxm_unreg_imsg_event(obj, INFY_OPENMODE_LEAVE_INSTALL);				

			uxm_unreg_imsg_event(obj, INFY_REBOOT_SYSTEM);				

			uxm_unreg_imsg_event(obj, INFY_BURN_ERASING);
			uxm_unreg_imsg_event(obj, INFY_BURN_EXTRACTING);
			uxm_unreg_imsg_event(obj, INFY_BURN_PROG);
			uxm_unreg_imsg_event(obj, INFY_BURN_SUCCESS);
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

void VW_Create_Remote_Msg_Popup(NFWINDOW *parent)
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


	bgImg = nf_ui_create_image_remote_msg_popup(IMG_POPUP_OSD_BG, MK_IMG_POPUP_OSD_BG, RMSG_WIN_SIZE_H);

	
	win = (NFOBJECT*)nfui_nfwindow_new(parent, RMSG_WIN_POS_X, RMSG_WIN_POS_Y, RMSG_WIN_SIZE_W, RMSG_WIN_SIZE_H);
	nfui_regi_post_event_callback(win, post_window_event_cb);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, RMSG_WIN_SIZE_W, RMSG_WIN_SIZE_H);
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(main_fixed, post_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	pos_x = 4;
	pos_y = 100;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, RMSG_WIN_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, RMSG_WIN_SIZE_W-12, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "The action is running by WEB Viewer.");	
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 12);
	nfui_nfobject_set_size(obj, RMSG_WIN_SIZE_W-12, 32);
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
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_x += 24;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, RMSG_WIN_SIZE_W-pos_x*2, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, RMSG_WIN_SIZE_W-pos_x*2, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 0);	
	nfui_nfobject_set_size(obj, RMSG_WIN_SIZE_W-pos_x*2, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_rmsg_label = obj;


// progressbar object
	pos_x = 4;   
	pos_y = RMSG_WIN_SIZE_H-150;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, RMSG_WIN_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, RMSG_WIN_SIZE_W-12, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 12);
	nfui_nfobject_set_size(obj, RMSG_WIN_SIZE_W-12, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_prglabel_obj = obj;

    pos_y += 42;

	prog_img[0] = nf_ui_create_image_progress_bg(IMG_PROGRESS_BG, MK_IMG_PROGRESS_BG, RMSG_WIN_SIZE_W-16);
	prog_img[1] = nfui_get_image_from_file((IMG_PROGRESS_HEAD), NULL);
	prog_img[2] = nfui_get_image_from_file((IMG_PROGRESS_MIDDLE), NULL);
	prog_img[3] = nfui_get_image_from_file((IMG_PROGRESS_TAIL), NULL);

	obj = nfui_nfprogressbar_new_with_images(prog_img[0], prog_img[1], prog_img[2], prog_img[3]);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+4, pos_y);
    g_prgimage_obj = obj;

	
// gear object
	pos_x = 4;   
	pos_y = RMSG_WIN_SIZE_H-150;

    nfui_get_image_size(IMG_GEAR_01, &img_w, &img_h); 

	obj = (NFOBJECT*)nfui_nfimage_new(IMG_GEAR_01);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (RMSG_WIN_SIZE_W-img_w)/2, pos_y);
    g_gearimage_obj = obj;


// wait object
	pos_x = 4;   
	pos_y = RMSG_WIN_SIZE_H-50;

	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, RMSG_WIN_SIZE_W-12, 32);
	gdk_pixbuf_copy_area(bgImg, pos_x, pos_y, RMSG_WIN_SIZE_W-12, 32, pbBG, 0, 0);

	obj = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "Please wait...");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(315));
	nfui_nfobject_set_size(obj, RMSG_WIN_SIZE_W-12, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_hide(win);

	uxm_reg_imsg_event(main_fixed, INFY_FORMAT_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_FORMAT_API_CMPL);
	uxm_monitor_on_imsg_event(main_fixed, INFY_FORMAT_API_BEGIN);	
	
	uxm_reg_imsg_event(main_fixed, INFY_FACDEF_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_FACDEF_API_CMPL);
	uxm_monitor_on_imsg_event(main_fixed, INFY_FACDEF_API_BEGIN);	
	
	uxm_reg_imsg_event(main_fixed, INFY_TIMECHANGE_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_TIMECHANGE_API_CMPL);
	uxm_monitor_on_imsg_event(main_fixed, INFY_TIMECHANGE_API_BEGIN);	
	
	uxm_reg_imsg_event(main_fixed, INFY_NETCHANGE_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_NETCHANGE_API_CMPL);
	uxm_monitor_on_imsg_event(main_fixed, INFY_NETCHANGE_API_BEGIN);	
	
	uxm_reg_imsg_event(main_fixed, INFY_SVCRESTART_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_SVCRESTART_API_CMPL);	
	uxm_monitor_on_imsg_event(main_fixed, INFY_SVCRESTART_API_BEGIN);

	uxm_reg_imsg_event(main_fixed, INFY_DBIMPORT_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_DBIMPORT_API_CMPL);	
	uxm_monitor_on_imsg_event(main_fixed, INFY_DBIMPORT_API_BEGIN);

	uxm_reg_imsg_event(main_fixed, INFY_ARCH_BURN_API_BEGIN);
	uxm_reg_imsg_event(main_fixed, INFY_ARCH_BURN_API_CMPL);	
	uxm_monitor_on_imsg_event(main_fixed, INFY_ARCH_BURN_API_BEGIN);

	uxm_reg_imsg_event(main_fixed, INFY_OPENMODE_ENTER_INSTALL);	
	uxm_reg_imsg_event(main_fixed, INFY_OPENMODE_LEAVE_INSTALL);	
	uxm_monitor_on_imsg_event(main_fixed, INFY_OPENMODE_ENTER_INSTALL);	

	uxm_reg_imsg_event(main_fixed, INFY_REBOOT_SYSTEM);	
	uxm_monitor_on_imsg_event(main_fixed, INFY_REBOOT_SYSTEM);	

	uxm_reg_imsg_event(main_fixed, INFY_BURN_ERASING);	
	uxm_reg_imsg_event(main_fixed, INFY_BURN_EXTRACTING);	
	uxm_reg_imsg_event(main_fixed, INFY_BURN_PROG);	
	uxm_reg_imsg_event(main_fixed, INFY_BURN_SUCCESS);
}
