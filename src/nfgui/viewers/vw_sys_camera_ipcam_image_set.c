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

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "nf_api_live.h"
#include "vw_sys_camera_ipcam_setup_main.h"
#include "vw_sys_camera_ipcam_image_set.h"
#include "vw_sys_camera_ipcam_internal.h"


#define MAX_SUB             (30)
#define MAX_PAGE            (10)

typedef struct _SUB_T
{
    NFOBJECT        *fixed;
    gint            height;
} SUB_T;

static NFWINDOW *g_curwnd = 0;

static SUB_T g_left_sub[MAX_SUB];
static SUB_T g_right_sub[MAX_SUB];

static NFOBJECT *g_left_fixed[GUI_CHANNEL_CNT];
static NFOBJECT *g_right_fixed[GUI_CHANNEL_CNT];

static NFOBJECT *g_right_page_obj[GUI_CHANNEL_CNT][MAX_PAGE];

static NFOBJECT *g_page_obj;
static NFOBJECT *g_prev_obj;
static NFOBJECT *g_next_obj;
static NFOBJECT *g_copy_obj;

static NFOBJECT *g_video_obj;

static gint g_index_page[GUI_CHANNEL_CNT];
static gint g_total_page[GUI_CHANNEL_CNT];
static gint g_channel;

static CAM_PROFILE_T *g_prof;


static gboolean _post_line_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;
			gint gap_x, gap_y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);
            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(735));
			gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y+20, obj->width, 2);
	 		g_object_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		
		break;

		default :
		break;
	}

	return FALSE;
}

static gint _update_page_obj(gint ch, gint curr_page, gint tot_page)
{
    gchar strBuf[10];

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d / %d", curr_page, tot_page);
    nfui_nflabel_set_text((NFLABEL*)g_page_obj, strBuf);
    return 0;
}

static gint _update_page_button(gint ch, gint curr_page, gint tot_page)
{
    if (curr_page == 1)
        nfui_nfobject_disable(g_prev_obj);
    else
        nfui_nfobject_enable(g_prev_obj);

    if (curr_page == tot_page)
        nfui_nfobject_disable(g_next_obj);
    else
        nfui_nfobject_enable(g_next_obj);

    return 0;
}

static gint _set_left_subFixed(NFOBJECT *subFixed, gint sub_h, gint sub_cnt)
{
    g_left_sub[sub_cnt].fixed = subFixed;
    g_left_sub[sub_cnt].height = sub_h;
    return 0;
}

static NFOBJECT* _put_left_subFixed(NFOBJECT *content_fixed)
{
	NFOBJECT *left_fixed;
    gint i, pos_y = 0;

	left_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(left_fixed, LEFT_FIXED_W, LEFT_FIXED_H);
    nfui_nfobject_modify_bg(left_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));	
//	nfui_nfobject_modify_bg(left_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FF0000));
	nfui_nfobject_hide(left_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, left_fixed, LEFT_FIXED_X, LEFT_FIXED_Y);

    for (i = 0; i < MAX_SUB; i++)
    {
        if (g_left_sub[i].fixed)
        {
        	nfui_nffixed_put((NFFIXED*)left_fixed, g_left_sub[i].fixed, 0, pos_y);
            pos_y += g_left_sub[i].height;
        }
    }

    return left_fixed;
}

static gint _init_left_fixed(gint ch, NFOBJECT *content_fixed)
{
	NFOBJECT *fixed;
	NFOBJECT *tmp_fixed;	
    gint sub_h;    
    gint sub_cnt = 0;

    memset(g_left_sub, 0x00, sizeof(SUB_T)*MAX_SUB);

    fixed = _make_subFixed_image_set(ch, &sub_h);    
    _set_left_subFixed(fixed, sub_h, sub_cnt);
    sub_cnt++;
     
    fixed = _make_subFixed_rotate(ch, &sub_h);   

    if (sub_h > 0)
    {
    	tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
    	nfui_nfobject_set_size(tmp_fixed, LEFT_FIXED_W, 42);
    	nfui_nfobject_show(tmp_fixed);
    	nfui_regi_post_event_callback(tmp_fixed, _post_line_fixed_event_handler);
        _set_left_subFixed(tmp_fixed, 42, sub_cnt);
        sub_cnt++;
    }
    
    _set_left_subFixed(fixed, sub_h, sub_cnt);
    sub_cnt++;
    
    g_left_fixed[ch] = _put_left_subFixed(content_fixed);

    return 0;
}

static gint _set_right_subFixed(NFOBJECT *subFixed, gint sub_h, gint sub_cnt)
{
    g_right_sub[sub_cnt].fixed = subFixed;
    g_right_sub[sub_cnt].height = sub_h;
    return 0;
}

static NFOBJECT* _new_right_pageFixed(NFOBJECT *right_fixed)
{
	NFOBJECT *page_fixed;

	page_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(page_fixed, RIGHT_PAGE_FIXED_W, RIGHT_PAGE_FIXED_H);
    nfui_nfobject_modify_bg(page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));		
//	nfui_nfobject_modify_bg(page_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FFFF00));	
	nfui_nfobject_hide(page_fixed);
	nfui_nffixed_put((NFFIXED*)right_fixed, page_fixed, RIGHT_PAGE_FIXED_X, RIGHT_PAGE_FIXED_Y);
    return page_fixed;
}

static NFOBJECT* _put_right_subFixed(gint ch, NFOBJECT *content_fixed)
{
	NFOBJECT *right_fixed;
	NFOBJECT *page_fixed;

    gint i;
    gint pos_y = 0;
    gint page_cnt = 0;

	right_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(right_fixed, RIGHT_FIXED_W, RIGHT_FIXED_H);
    nfui_nfobject_modify_bg(right_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			
//	nfui_nfobject_modify_bg(right_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_00FF00));
	nfui_nfobject_hide(right_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, right_fixed, RIGHT_FIXED_X, RIGHT_FIXED_Y);

    page_fixed = _new_right_pageFixed(right_fixed);
    g_right_page_obj[ch][page_cnt] = page_fixed;
    page_cnt++;

    for (i = 0; i < MAX_SUB; i++)
    {
        if (g_right_sub[i].fixed)
        {
            if ((pos_y + g_right_sub[i].height) > RIGHT_PAGE_FIXED_H)
            {
                page_fixed = _new_right_pageFixed(right_fixed);            
                g_right_page_obj[ch][page_cnt] = page_fixed;
                page_cnt++;
                
            	nfui_nffixed_put((NFFIXED*)page_fixed, g_right_sub[i].fixed, 0, 0);
                pos_y = g_right_sub[i].height;
            }
            else
            {
            	nfui_nffixed_put((NFFIXED*)page_fixed, g_right_sub[i].fixed, 0, pos_y);
                pos_y += g_right_sub[i].height;
            }

            pos_y += 20;
        }
    }

    g_index_page[ch] = 1;
    g_total_page[ch] = page_cnt;

    return right_fixed;
}

static gint _init_right_fixed(gint ch, NFOBJECT *content_fixed)
{
	NFOBJECT *fixed;
    gint sub_h;    
    gint sub_cnt = 0;
   
    memset(g_right_sub, 0x00, sizeof(SUB_T)*MAX_SUB);

    fixed = _make_subFixed_focus(ch, &sub_h);
    _set_right_subFixed(fixed, sub_h, sub_cnt);
    sub_cnt++;

    fixed = _make_subFixed_white_balance(ch, &sub_h);
    _set_right_subFixed(fixed, sub_h, sub_cnt);
    sub_cnt++;

    g_right_fixed[ch] = _put_right_subFixed(ch, content_fixed);

    return 0;
}

static gboolean post_prev_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;
    gint curr_page;	
    gint next_page;	
    
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

	    curr_page = g_index_page[g_channel];
        nfui_nfobject_hide(g_right_page_obj[g_channel][curr_page-1]);

        next_page = curr_page - 1;
        nfui_nfobject_show(g_right_page_obj[g_channel][next_page-1]); 

        _update_page_obj(g_channel, next_page, g_total_page[g_channel]);
        _update_page_button(g_channel, next_page, g_total_page[g_channel]);
		        
        nfui_signal_emit(g_right_page_obj[g_channel][curr_page-1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_right_page_obj[g_channel][next_page-1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_page_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_prev_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_next_obj, GDK_EXPOSE, TRUE);
        
    	nfui_make_key_hierarchy(g_curwnd);

    	g_index_page[g_channel] = next_page;
	}

	return FALSE;
}

static gboolean post_next_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{    
	NFOBJECT *topwin;
    gint curr_page;	
    gint next_page;	

	if(evt->type == GDK_BUTTON_RELEASE)
    {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        curr_page = g_index_page[g_channel];
        nfui_nfobject_hide(g_right_page_obj[g_channel][curr_page-1]);
    
        next_page = curr_page + 1;
        nfui_nfobject_show(g_right_page_obj[g_channel][next_page-1]); 

        _update_page_obj(g_channel, next_page, g_total_page[g_channel]);
        _update_page_button(g_channel, next_page, g_total_page[g_channel]);

        nfui_signal_emit(g_right_page_obj[g_channel][curr_page-1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_right_page_obj[g_channel][next_page-1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_page_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_prev_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_next_obj, GDK_EXPOSE, TRUE);

    	nfui_make_key_hierarchy(g_curwnd);

    	g_index_page[g_channel] = next_page;
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

        dst_mask = _get_copy_chmask(g_channel);        
        _copy_ipcam_db(g_channel, dst_mask);
	}

	return FALSE;
}

static gboolean post_on_off_button_evnet_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
	{    
	    mb_type ret;
	    gint is_fail;
	    gint retn;
	    
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
			return FALSE;
        }

		if(g_toggle[g_channel] ^= 1) 
        {
            
            ret = nftool_mbox(g_curwnd, "CONFIRM", "The PoE power will be turned on while settings are changed.\nDo you want to continue?",NFTOOL_MB_OKCANCEL);    
            if (ret == NFTOOL_MB_OK)
            {   
                retn = scm_set_ipcam_poe_onoff(g_channel,1);

                if(!retn){
        	        nfui_nfbutton_set_text((NFBUTTON *) obj, "POE POWER OFF");
        	        nfui_nfbutton_set_text((NFBUTTON *) g_on_off_expose_obj, "POE POWER OFF");
        			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        			nfui_signal_emit(g_on_off_expose_obj, GDK_EXPOSE, TRUE);
      				DAL_set_cam_poe_onoff(g_toggle[g_channel], g_channel);
      	        }
      	        else{
      	            nftool_mbox(g_curwnd, "WARNING", "PoE Settings has failed", NFTOOL_MB_OK);
      	            g_toggle[g_channel] ^=1;
      	        }
    	    }
    	    else if(ret == NFTOOL_MB_CANCEL)  g_toggle[g_channel] ^=1;
    	    
        }
		else 		  
        {
            ret = nftool_mbox(g_curwnd, "CONFIRM", "The PoE power will be turned off while settings are changed.\nDo you want to continue?",NFTOOL_MB_OKCANCEL);    
            if(ret == NFTOOL_MB_OK)
            {
                retn = scm_set_ipcam_poe_onoff(g_channel,0);

                if(!retn){
            	    nfui_nfbutton_set_text((NFBUTTON *) obj, "POE POWER ON");
          	        nfui_nfbutton_set_text((NFBUTTON *) g_on_off_expose_obj, "POE POWER ON");        	    
        			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        			nfui_signal_emit(g_on_off_expose_obj, GDK_EXPOSE, TRUE);
                    DAL_set_cam_poe_onoff(g_toggle[g_channel], g_channel);
                }
                else{
      	            nftool_mbox(g_curwnd, "WARNING", "PoE Settings has failed", NFTOOL_MB_OK);
      	            g_toggle[g_channel] ^=1;
      	        }
            }
            else if (ret == NFTOOL_MB_CANCEL) g_toggle[g_channel] ^=1;
        }
        
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;
    gint i;
	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        _cancel_ipcam_db();

        nfui_signal_emit(g_left_fixed[g_channel], GDK_EXPOSE, TRUE);        
        nfui_signal_emit(g_right_fixed[g_channel], GDK_EXPOSE, TRUE);
    	nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;
    gint i;
	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        if (_is_changed_ipcam_db() == -1)
            return FALSE;

        _save_ipcam_db();
        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)			
			return FALSE;
	
		IPCamImageSet_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT *wait_mbox = NULL;

	switch(evt->type) 
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;
			gint gap_x, gap_y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);
            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(735));
			gdk_draw_rectangle(drawable, gc, TRUE, gap_x+RIGHT_FIXED_X, gap_y+RIGHT_FIXED_Y+RIGHT_FIXED_H, RIGHT_FIXED_W, 2);
	 		g_object_unref(gc);
	 	}
		break;
	
		case INFY_IPCAM_CALIBRATION_BEGIN:
			if(!wait_mbox)
				wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");		
		break;

		case INFY_IPCAM_CALIBRATION_END:
		{		
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}		
		}
		break;

		case INFY_IPCAM_CALIBRATION_PENDING:
		break;

		case INFY_IPCAM_CALIBRATION_REQ_FAIL:
		{		
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}

			vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
		}
		break;

		case INFY_IPCAM_CALIBRATION_TIMEOUT:
		{	
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}
		}
		break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_END);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_PENDING);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_REQ_FAIL);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_TIMEOUT);
		break;

		default:
		break;		
	}
	
	return FALSE;
}

void VW_Init_IPCamImageSet_Page(NFOBJECT *parent, gint ch, CAM_PROFILE_T *prof)
{
	NFOBJECT *content_fixed;
	NFOBJECT *sub_fixed;
	NFOBJECT *fixed;
    NFOBJECT *obj;

    gint i, j;
    gint pos_x, pos_y;

    g_curwnd = nfui_nfobject_get_top(parent);
    g_channel = ch;

    g_prof = prof;
    
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_W, MENU_V_IPCAMSET_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_IPCAMSET_SUBTAB_INNER_X, MENU_V_IPCAMSET_SUBTAB_INNER_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, VIDEO_W, VIDEO_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, VIDEO_X, VIDEO_Y);
    g_video_obj = obj;
 
    pos_x = RIGHT_FIXED_X + (RIGHT_FIXED_W-100)/2;
    pos_y = RIGHT_PAGE_FIXED_Y + RIGHT_PAGE_FIXED_H + 8;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(obj, 100, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    g_page_obj = obj;

	obj = nftool_normal_button_create_type3("PREV.", 84);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x-134, pos_y);
	nfui_regi_post_event_callback(obj, post_prev_button_event_handler);	
    g_prev_obj = obj;

	obj = nftool_normal_button_create_type3("NEXT", 84);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+100+50, pos_y);
	nfui_regi_post_event_callback(obj, post_next_button_event_handler);	
    g_next_obj = obj;
 
	obj = nftool_normal_button_create_type1("COPY SETTINGS TO", COPY_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, COPY_BTN_X, COPY_BTN_Y);
	nfui_regi_post_event_callback(obj, post_copy_button_event_handler);
    g_copy_obj = obj;

  	init_poe_on_off(ch);
	
	obj = nftool_normal_button_create_type1("",COPY_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj),NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, COPY_BTN_X+COPY_BTN_W+10, COPY_BTN_Y);
    nfui_regi_post_event_callback(obj, post_on_off_button_evnet_handler);
    if(g_toggle[g_channel])  nfui_nfbutton_set_text((NFBUTTON *) obj,"POE POWER OFF");
	else                     nfui_nfbutton_set_text((NFBUTTON *) obj,"POE POWER ON");

    g_on_off_image_obj = obj;

    if (!g_prof[ch].connected){
        nfui_nfobject_disable(g_copy_obj);
    }              
    if(DAL_get_cam_install_mode()) nfui_nfobject_hide(g_on_off_image_obj);
    
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

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {   
        _init_left_fixed(i, content_fixed);
        _init_right_fixed(i, content_fixed);

        nfui_nfobject_show(g_right_page_obj[i][0]); 
    }    

    _update_page_obj(ch, g_index_page[ch], g_total_page[ch]);
    _update_page_button(ch, g_index_page[ch], g_total_page[ch]);

    nfui_nfobject_show(g_left_fixed[ch]);
    nfui_nfobject_show(g_right_fixed[ch]);   

	uxm_reg_imsg_event(content_fixed, INFY_IPCAM_CALIBRATION_BEGIN);
	uxm_reg_imsg_event(content_fixed, INFY_IPCAM_CALIBRATION_END);
	uxm_reg_imsg_event(content_fixed, INFY_IPCAM_CALIBRATION_PENDING);
	uxm_reg_imsg_event(content_fixed, INFY_IPCAM_CALIBRATION_REQ_FAIL);
	uxm_reg_imsg_event(content_fixed, INFY_IPCAM_CALIBRATION_TIMEOUT);	
	
	nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);    
}

gint IPCamImageSet_update_channel(gint ch)
{
    if (g_prof[ch].connected){ 
        nfui_nfobject_enable(g_copy_obj);
    }
    else{                       
        nfui_nfobject_disable(g_copy_obj); 
    }

    nfui_nfobject_hide(g_left_fixed[g_channel]);   
    nfui_nfobject_hide(g_right_fixed[g_channel]);

    nfui_nfobject_show(g_left_fixed[ch]);
    nfui_nfobject_show(g_right_fixed[ch]);

    nfui_signal_emit(g_left_fixed[g_channel], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_right_fixed[g_channel], GDK_EXPOSE, TRUE);

    nfui_signal_emit(g_left_fixed[ch], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_right_fixed[ch], GDK_EXPOSE, TRUE);

    _update_page_obj(ch, g_index_page[ch], g_total_page[ch]);
    _update_page_button(ch, g_index_page[ch], g_total_page[ch]);

    nfui_signal_emit(g_copy_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_page_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_prev_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_next_obj, GDK_EXPOSE, TRUE);  

    init_poe_on_off(ch);
    set_camera_toggle_on_off(ch);
    
	nfui_make_key_hierarchy(g_curwnd); 
	
    g_channel = ch;
   
    return 0;
}

gint IPCamImageSet_video_show()
{
    nfui_nfobject_modify_bg(g_video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    return 0;
}

gint IPCamImageSet_video_hide()
{
    nfui_nfobject_modify_bg(g_video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_signal_emit(g_video_obj, GDK_EXPOSE, TRUE);

    return 0;
}

gboolean IPCamImageSet_tab_in_handler()
{
	



	return FALSE;
}

gboolean IPCamImageSet_tab_out_handler()
{
	mb_type ret;

    if (_is_changed_ipcam_db() == -1)
        return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
    {	
        _save_ipcam_db();
    }
	else if (ret == NFTOOL_MB_CANCEL)
	{
        _cancel_ipcam_db();      
    	nfui_make_key_hierarchy(g_curwnd);                
    }

	return FALSE;
}


