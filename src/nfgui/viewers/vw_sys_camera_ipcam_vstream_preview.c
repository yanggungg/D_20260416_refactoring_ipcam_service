#include "nf_afx.h"

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
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"
#include "objects/nfimage.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "vw_sys_camera_ipcam_vstream.h"
#include "vw_sys_camera_ipcam_vstream_preview.h"


#define PAGE_STR "In this page, all cameras are changed to the highest quality for the preformance test."

#define STREAM_1ST_FIXED_X          (20)
#define STREAM_1ST_FIXED_Y          (44+38+40+39)
#define STREAM_1ST_FIXED_W          (VIDEO_1ST_W)
#define STREAM_1ST_FIXED_H          (40+5+VIDEO_1ST_H+84)

#define STREAM_2ND_FIXED_X          (STREAM_1ST_FIXED_X+STREAM_1ST_FIXED_W+11)
#define STREAM_2ND_FIXED_Y          (44+20)
#define STREAM_2ND_FIXED_W          (VIDEO_2ND_W)
#define STREAM_2ND_FIXED_H          (40+5+VIDEO_2ND_H+84)

#define STREAM_3RD_FIXED_X          (STREAM_1ST_FIXED_X+STREAM_1ST_FIXED_W+11)
#define STREAM_3RD_FIXED_Y          (STREAM_2ND_FIXED_Y+STREAM_2ND_FIXED_H+34)
#define STREAM_3RD_FIXED_W          (VIDEO_3RD_W)
#define STREAM_3RD_FIXED_H          (40+5+VIDEO_3RD_H+84)

#define VIDEO_1ST_W                 (1285)
#define VIDEO_1ST_H                 (722)

#define VIDEO_2ND_W                 (584)
#define VIDEO_2ND_H                 (328)

#define VIDEO_3RD_W                 (584)
#define VIDEO_3RD_H                 (328)


static gchar *strCh[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_model_obj = 0;

static void _update_camera_name(gint ch)
{
    gchar buf[128];
	CAM_PROFILE_T prof;

    memset(buf, 0x00, sizeof(buf));
    memset(&prof, 0x00, sizeof(CAM_PROFILE_T));
    
	scm_get_cam_profile(ch, &prof);	

	if (prof.connected)
        sprintf(buf, "%s%s", "|   ", prof.model.name); 
	
	nfui_nflabel_set_text(g_model_obj, buf);
}

static gboolean _post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
        ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
	
        _update_camera_name(ch);
        nfui_signal_emit(g_model_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean _post_exit_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        vsm_live_preview_stop();

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean _post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
        gint i;
	
		for(i=0; i<GUI_CHANNEL_CNT; i++) 
			ifree(strCh[i]);	
	
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean _post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE)
	{
    	drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }  
    
	return FALSE;
}

void VW_Open_VideoStream_preview_Page(NFOBJECT *parent, gint ch)
{
    NFOBJECT *window;
	NFOBJECT *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *fixed2;
	NFOBJECT *fixed3;
	NFOBJECT *obj;

	guint i, j;
	guint size_w, size_h;
	guint pos_x, pos_y;

	vsm_live_preview_start(1 << ch, 
        STREAM_1ST_FIXED_X, 
        STREAM_1ST_FIXED_Y+40+5, 
        VIDEO_1ST_W, 
        VIDEO_1ST_H);
	
	window = (NFOBJECT*)nfui_nfwindow_new(parent, SETUP_WINDOW_POS_X, SETUP_WINDOW_POS_Y, SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT);
	nfui_regi_post_event_callback(window, _post_win_event_handler);
	g_curwnd = window;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT);
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	nfui_nffixed_put((NFFIXED*)window, main_fixed, SETUP_WINDOW_POS_X, SETUP_WINDOW_POS_Y);
	nfui_regi_post_event_callback(main_fixed, _post_main_fixed_event_handler);	
	nfui_nfobject_show(main_fixed);

    pos_x = 20;
    pos_y = 2;

	obj = nfui_nflabel_new_with_pango_font("VIDEO STREAM PREVIEW", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(30));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 320, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 320;

	obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(297));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 40, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 40;

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(297));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 1200, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x = 20;
    pos_y += (44 + 38);

	for (i = 0; i<GUI_CHANNEL_CNT; i++)
	{
		strCh[i] = imalloc(sizeof(gchar) * (STRING_SIZE_CAMTITLE+8));	
		j = sprintf(strCh[i], "CH%d - ", i+1);
		//DAL_get_camera_title(strCh[i]+j, i);
		var_get_camtitle(strCh[i]+j, i);
	}

	obj = nfui_combobox_new(strCh, GUI_CHANNEL_CNT, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 240, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_channel_event_handler);

    pos_x = (20 + 240);
		
	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 400, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_model_obj = obj;

    _update_camera_name(0);

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, STREAM_1ST_FIXED_W, STREAM_1ST_FIXED_H);
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, STREAM_1ST_FIXED_X, STREAM_1ST_FIXED_Y);
	nfui_nfobject_show(fixed1);

	fixed2 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed2, STREAM_2ND_FIXED_W, STREAM_2ND_FIXED_H);
	nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed2, STREAM_2ND_FIXED_X, STREAM_2ND_FIXED_Y);
	nfui_nfobject_show(fixed2);

	fixed3 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed3, STREAM_3RD_FIXED_W, STREAM_3RD_FIXED_H);
	nfui_nfobject_modify_bg(fixed3, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed3, STREAM_3RD_FIXED_X, STREAM_3RD_FIXED_Y);
	nfui_nfobject_show(fixed3);

// fixed1 - item
    pos_x = 0;
    pos_y = 0;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "1st STREAM");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    pos_y += (40+5);

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_PRG_IDX(UX_COLOR_FFFFFF));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, VIDEO_1ST_W, VIDEO_1ST_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    pos_x = (STREAM_1ST_FIXED_W-734);
    pos_y += VIDEO_1ST_H;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("H.264", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 244, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);	

    pos_x += (244+1);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("7654 KBPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 244, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);	

    pos_x += (244+1);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("29.96 FPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 244, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);		

    pos_x = (STREAM_1ST_FIXED_W-734);
    pos_y += (40+4);

	obj = nfui_combobox_new(NULL, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 276, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    pos_x += (276+1);

	obj = nfui_combobox_new(NULL, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 276, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
		
    pos_x += (276+1);

	obj = nftool_normal_button_create_popup_type1("APPLY", 180);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);


// fixed2 - item
    pos_x = 0;
    pos_y = 0;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "2nd STREAM");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

    pos_y += (40+5);

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_PRG_IDX(UX_COLOR_FFFFFF));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, VIDEO_2ND_W, VIDEO_2ND_H);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

    pos_x = 0;
    pos_y += VIDEO_2ND_H;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("H.264", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 194, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);	

    pos_x += (194+1);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("7654 KBPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 194, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);	

    pos_x += (194+1);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("29.96 FPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 194, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);		

    pos_x = 0;
    pos_y += (40+4);

	obj = nfui_combobox_new(NULL, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 211, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

    pos_x += (211+1);

	obj = nfui_combobox_new(NULL, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 211, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);
		
    pos_x += (211+1);

	obj = nftool_normal_button_create_popup_type1("APPLY", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);

    
// fixed3 - item
    pos_x = 0;
    pos_y = 0;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "3rd STREAM");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);

    pos_y += (40+5);

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_PRG_IDX(UX_COLOR_FFFFFF));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, VIDEO_3RD_W, VIDEO_3RD_H);		
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);

    pos_x = 0;
    pos_y += VIDEO_3RD_H;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("H.264", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 194, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);	

    pos_x += (194+1);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("7654 KBPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 194, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);	

    pos_x += (194+1);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("29.96 FPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 194, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);		

    pos_x = 0;
    pos_y += (40+4);

	obj = nfui_combobox_new(NULL, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 211, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);

    pos_x += (211+1);

	obj = nfui_combobox_new(NULL, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 211, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);
		
    pos_x += (211+1);

	obj = nftool_normal_button_create_popup_type1("APPLY", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, pos_x, pos_y);



    pos_x = SETUP_WINDOW_WIDTH-192-20;
    pos_y = SETUP_WINDOW_HEIGHT-44-6;

	obj = nftool_normal_button_create_type2("EXIT", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_exit_btn_event_handler);
	
	// windows show
	nfui_nfwindow_add((NFWINDOW*)window, main_fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_CAM_VIDEO_STREAM_PREVIEW, window, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CAM_VIDEO_STREAM_PREVIEW, window);
}

