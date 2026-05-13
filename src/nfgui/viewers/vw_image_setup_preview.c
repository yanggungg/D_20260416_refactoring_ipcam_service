#include "vw_image_setup_preview.h"

#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_image.h"
//#include "nf_ui_color_setup_bar.h"

#include "tools/nf_ui_tool.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "nf_api_ipcam.h"
#include "nf_api_eventlog.h"
#include "vw_vkeyboard.h"
#include "vsm.h"
#include "ssm.h"



#define PREVIEW_WIN_WIDTH 			1920
#define PREVIEW_WIN_HEIGHT			1080 

#define PREVIEW_X				    0
#define PREVIEW_Y				    0
#define PREVIEW_WIDTH			    1920
#define PREVIEW_HEIGHT		        PREVIEW_WIN_HEIGHT - 108

enum {
	ATTR_BRIGHT = 0,
	ATTR_CONTRAST,
	ATTR_TINT,
	ATTR_COLOR,
	ATTR_TYPE
};

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *video_obj;
static NFOBJECT *channel_obj;
static NFOBJECT *control_label_obj[ATTR_TYPE];
static NFOBJECT *exit_btn;
static CWSLIDER *slider[ATTR_TYPE];

static ColorData *g_coldata;
static NFIPCamImageProfile *g_ipcam_pf;

static gboolean vw_cam_check_supported_func(gint ch, guint check_bit)
{
	if (g_ipcam_pf[ch].supported & check_bit)
		return TRUE;
	else 
		return FALSE;		
}

static void _check_supported_func_slider(gint ch)
{
	gint i;

	for (i = 0; i < NF_IPCAM_IMAGE_NR; i++)
	{
		if (vw_cam_check_supported_func(ch, 1 << i))
		{
			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_enable(slider[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_enable(slider[ATTR_CONTRAST]);		
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_enable(slider[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_enable(slider[ATTR_COLOR]);

			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_enable(control_label_obj[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_enable(control_label_obj[ATTR_CONTRAST]);		
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_enable(control_label_obj[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_enable(control_label_obj[ATTR_COLOR]);
		}
		else
		{
			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_disable(slider[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_disable(slider[ATTR_CONTRAST]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_disable(slider[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_disable(slider[ATTR_COLOR]);			

			if ((1 << i) ==  NF_IPCAM_IMAGE_BRIGHTNESS)		nfui_nfobject_disable(control_label_obj[ATTR_BRIGHT]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_CONTRAST) 		nfui_nfobject_disable(control_label_obj[ATTR_CONTRAST]);			
			if ((1 << i) ==  NF_IPCAM_IMAGE_TINT) 			nfui_nfobject_disable(control_label_obj[ATTR_TINT]);				
			if ((1 << i) ==  NF_IPCAM_IMAGE_COLOR) 			nfui_nfobject_disable(control_label_obj[ATTR_COLOR]);			
		}
	}
}

static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch, 0, 0, 1920, PREVIEW_WIN_HEIGHT - 109);

    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static gint _show_video_obj()
{
    nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    return 0;
}

static gint _hide_video_obj()
{
    nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_signal_emit(video_obj, GDK_EXPOSE, TRUE);
    
    return 0;
}

static void _set_cam_attr(gint ch)
{
	NFIPCamSetupColor info;

	memset(&info, 0x00, sizeof(info));
	info.ch 		= ch;
	info.brightness = cw_slider_get_value(slider[ATTR_BRIGHT]);
	info.contrast	= cw_slider_get_value(slider[ATTR_CONTRAST]);
	info.tint		= cw_slider_get_value(slider[ATTR_TINT]);
	info.color		= cw_slider_get_value(slider[ATTR_COLOR]);
	
	scm_set_ipcam_image(&info);
}

static gboolean 
pre_vw_img_windows_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) 
	{
		case GDK_EXPOSE :
		break;
		
		case GDK_DELETE : 
			g_curwnd = 0;
		break;

		default : 
		break;
	}

	return FALSE;
}

static gboolean 
post_vw_img_windows_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_DELETE)
	{
		gtk_main_quit();		
	}

	return FALSE;
}

static gboolean 
post_vw_img_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) 
	{
		case GDK_EXPOSE :
		{	
			GdkGC *gc;
			GdkDrawable *drawable;
			gint ch;
			int w, h;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));	

			nfui_nfobject_get_size(obj, &w, &h);
			gdk_draw_rectangle (drawable, gc, TRUE, 0, PREVIEW_WIN_HEIGHT - 108, w, 108);
	 		nfui_nfobject_gc_unref(gc);
		}
		break;
		
		case GDK_DELETE : 
		break;

		default : 
		break;
	}
	
	return FALSE;
	
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	int index;
	gchar tmp_val[10];

	switch (event->type) {
		case GDK_EXPOSE :
		break;

		case NFEVENT_COMBOBOX_CHANGED:
		{
			index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

			_check_supported_func_slider(index);

			memset(tmp_val, 0x00, sizeof(tmp_val));
			g_sprintf(tmp_val, "%d", g_coldata[index].bright);
			nfui_nflabel_set_text(control_label_obj[ATTR_BRIGHT], tmp_val);

			memset(tmp_val, 0x00, sizeof(tmp_val));
			g_sprintf(tmp_val, "%d", g_coldata[index].contrast);	
			nfui_nflabel_set_text(control_label_obj[ATTR_CONTRAST], tmp_val);
			
			memset(tmp_val, 0x00, sizeof(tmp_val));
			g_sprintf(tmp_val, "%d", g_coldata[index].tint);	
			nfui_nflabel_set_text(control_label_obj[ATTR_TINT], tmp_val);

			memset(tmp_val, 0x00, sizeof(tmp_val));
			g_sprintf(tmp_val, "%d", g_coldata[index].color);	
			nfui_nflabel_set_text(control_label_obj[ATTR_COLOR], tmp_val);

			nfui_signal_emit(control_label_obj[ATTR_BRIGHT], GDK_EXPOSE, TRUE);
			nfui_signal_emit(control_label_obj[ATTR_CONTRAST], GDK_EXPOSE, TRUE);
			nfui_signal_emit(control_label_obj[ATTR_TINT], GDK_EXPOSE, TRUE);
			nfui_signal_emit(control_label_obj[ATTR_COLOR], GDK_EXPOSE, TRUE);

			cw_slider_set_value(slider[ATTR_BRIGHT], g_coldata[index].bright);
			cw_slider_set_value(slider[ATTR_CONTRAST], g_coldata[index].contrast);
			cw_slider_set_value(slider[ATTR_TINT], g_coldata[index].tint);
			cw_slider_set_value(slider[ATTR_COLOR], g_coldata[index].color);

			nfui_signal_emit(slider[ATTR_BRIGHT], GDK_EXPOSE, TRUE);
			nfui_signal_emit(slider[ATTR_CONTRAST], GDK_EXPOSE, TRUE);
			nfui_signal_emit(slider[ATTR_TINT], GDK_EXPOSE, TRUE);
			nfui_signal_emit(slider[ATTR_COLOR], GDK_EXPOSE, TRUE);

			_set_cam_attr(index);
			_start_preview(index);			
		}
		break;
		
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_LEFT || kpid == KEYPAD_RIGHT)
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(exit_btn, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(exit_btn, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
		break;
			
		case GDK_DELETE : 
			break;

		default : 
			break;
	}
	
	return FALSE;
}

static gboolean 
post_vw_preview_slider_event_handler (NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) 
	{
		case GDK_BUTTON_PRESS :
		break;

		case GDK_LEAVE_NOTIFY :
		case GDK_BUTTON_RELEASE :
		{
			gint i, ch;
            ColorData data;

			ch = nfui_combobox_get_cur_index(NF_COMBOBOX(channel_obj));

			data.bright = cw_slider_get_value(slider[ATTR_BRIGHT]);
			data.contrast = cw_slider_get_value(slider[ATTR_CONTRAST]);
			data.tint = cw_slider_get_value(slider[ATTR_TINT]);
			data.color = cw_slider_get_value(slider[ATTR_COLOR]);

            if(memcmp(&g_coldata[ch], &data, sizeof(ColorData)))
            {
    			for(i = 0; i < ATTR_TYPE; i++) 
    			{
    				if(slider[i] == obj) break;
    			}
            
    			switch(i)
    			{
    				case ATTR_BRIGHT: 
    					g_coldata[ch].bright = cw_slider_get_value(obj);
    				break;
    	
    				case ATTR_CONTRAST: 
    					g_coldata[ch].contrast = cw_slider_get_value(obj);
    				break;

    				case ATTR_TINT: 
    					g_coldata[ch].tint = cw_slider_get_value(obj);
    				break;

    				case ATTR_COLOR: 
    					g_coldata[ch].color = cw_slider_get_value(obj);
    				break;

    				default: 
    				break;
    			}
            
    			_set_cam_attr(ch);			
    		}
		}
		break;
		
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :
		{
			gint i, ch;
			gchar str_value[10];
			
			for(i = 0; i < ATTR_TYPE; i++) 
			{
				if(slider[i] == obj) break;
			}

			if(i == ATTR_TYPE) break;

			ch = nfui_combobox_get_cur_index(NF_COMBOBOX(channel_obj));
			g_sprintf(str_value, "%d", cw_slider_get_value(obj));
			nfui_nflabel_set_text(control_label_obj[i], str_value);
			nfui_signal_emit(control_label_obj[i], GDK_EXPOSE, TRUE);	
		}
		break;

		default : 
		break;
	}
	
	return FALSE;
	
}

static gboolean post_vw_img_exit_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) 
	{
		case GDK_BUTTON_RELEASE :
		{
			NFOBJECT *topwin;

            _hide_video_obj();
    		_stop_preview();

			topwin = nfui_nfobject_get_top((NFOBJECT *)obj);
			nftool_destroy_setup_window(topwin);

		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_LEFT || kpid == KEYPAD_RIGHT)
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(channel_obj, TRUE);

				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(channel_obj, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
		break;
		
		default : 
		break;
	}
	
	return FALSE;
	
}

void vw_open_image_setup_preview(NFWINDOW *parent, guint ch, ColorData *data, NFIPCamImageProfile *ipcam_pf)
{
    NFOBJECT *win;
	NFOBJECT *main_fixed;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	const gchar *strControl[] = {"BRIGHTNESS", "CONTRAST", "TINT", "COLOR"};
	gchar tmp_val[10];
	gchar strBuf[STRING_SIZE_CAMTITLE];
	gchar strBuf_num[STRING_SIZE_CAMTITLE+8];
	guint x, y;
	gint i;
	gint ypos = 44;

	gint tlbl_size[4] =  {120, 120, 120, 120};
	gint tlbl_xpos[4] =  {200, 580, 960, 1340};
	gint slider_xpos[4] = {200, 580, 960, 1340};
	gint lbl_xpos[4] =  {200+220+10, 580+220+10, 960+220+10, 1340+220+10};

#if 0
	GdkPixbuf *pbBG = NULL;
	GdkPixbuf *bgImg = NULL;
#endif


	g_coldata = data;
	g_ipcam_pf = ipcam_pf;

    _start_preview(ch);

	// win
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, PREVIEW_WIN_WIDTH, PREVIEW_WIN_HEIGHT);
	nfui_regi_pre_event_callback(win, pre_vw_img_windows_event_handler);
	nfui_regi_post_event_callback(win, post_vw_img_windows_event_handler);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();		
	nfui_nfobject_set_size(main_fixed, 1920, 1080);
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nffixed_put((NFFIXED*)win, main_fixed, 0, 0);
	nfui_nfobject_show(main_fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, PREVIEW_WIDTH, PREVIEW_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PREVIEW_X, PREVIEW_Y);	
    video_obj = obj;

	// fixed	
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, PREVIEW_WIDTH, 1080-(PREVIEW_Y+PREVIEW_HEIGHT));
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_regi_post_event_callback(fixed, post_vw_img_fixed_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, PREVIEW_X, PREVIEW_Y+PREVIEW_HEIGHT);
	nfui_nfobject_show(fixed);

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 140, 40); 
	nfui_nfobject_show(obj); 
	nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 34);
	nfui_regi_post_event_callback(obj, post_channel_event_handler);
	channel_obj = obj;

	for (i = 0; i < GUI_CHANNEL_CNT; ++i) {
		memset(strBuf, 0x00, sizeof(strBuf));
		memset(strBuf_num, 0x00, sizeof(strBuf_num));
		
		DAL_get_camera_title(strBuf, i);
		g_sprintf(strBuf_num, "%d. %s", i+1, strBuf);
		nfui_combobox_append_data(obj, strBuf_num);
	}
	nfui_combobox_set_index_no_expose(channel_obj, ch);
	

	for(i = 0; i < ATTR_TYPE; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strControl[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(293));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_set_size(obj, tlbl_size[i], 28);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, tlbl_xpos[i], ypos - 28);

#if 0
		if(bgImg == NULL) 
			bgImg = nfui_get_image_from_file(IMG_STATUSBAR_BG, NULL);

		pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, tlbl_size[i], 28);
		gdk_pixbuf_fill(pbBG, 0x000000);
		gdk_pixbuf_copy_area(bgImg, tlbl_xpos[i], ypos - 28, tlbl_size[i], 28, pbBG, 0, 0);

		obj = nfui_nfimage_new_pixbuf(pbBG);
		nfui_nfimage_set_text((NFIMAGE*)obj, strControl[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(293));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, tlbl_xpos[i], ypos - 28);
#endif
		

		// SLIDER
		if (i == ATTR_BRIGHT) 			obj = cw_slider_new(g_coldata[ch].bright, 220, 40);
		else if (i == ATTR_CONTRAST) 	obj = cw_slider_new(g_coldata[ch].contrast, 220, 40);
		else if (i == ATTR_TINT) 	 	obj = cw_slider_new(g_coldata[ch].tint, 220, 40);
		else if (i == ATTR_COLOR)		obj = cw_slider_new(g_coldata[ch].color, 220, 40);
		
		cw_slider_set_range(obj, 0, 100, 101); 
//		cw_slider_set_bg_color(obj, 11, 19, 31);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));		
		nfui_nfobject_set_size(obj, 220, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, slider_xpos[i], ypos);
		nfui_regi_post_event_callback(obj, post_vw_preview_slider_event_handler);
		slider[i] = obj;

		// LABEL
		memset(tmp_val, 0x00, sizeof(tmp_val));
		if (i == ATTR_BRIGHT) 			g_sprintf(tmp_val, "%d", g_coldata[ch].bright);
		else if (i == ATTR_CONTRAST) 	g_sprintf(tmp_val, "%d", g_coldata[ch].contrast);
		else if (i == ATTR_TINT) 	 	g_sprintf(tmp_val, "%d", g_coldata[ch].tint);
		else if (i == ATTR_COLOR)		g_sprintf(tmp_val, "%d", g_coldata[ch].color);	
		
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(tmp_val, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));		
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 60, 44);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, lbl_xpos[i], ypos - 2);
		control_label_obj[i] = obj;
	}

	obj = nftool_normal_button_create_type2("EXIT", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 1716, ypos - 10);
	nfui_regi_post_event_callback(obj, post_vw_img_exit_button_event_handler);
	exit_btn = obj;

	_check_supported_func_slider(ch);

	// windows show
	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_set_key_focus(channel_obj, TRUE);
	nfui_page_open(PGID_CAM_COLOR_BAR, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CAM_COLOR_BAR, win);
}


