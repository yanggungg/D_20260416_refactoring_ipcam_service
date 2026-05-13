#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"
#include "uxm.h"

//#include "services/uxm.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfprogressbar.h"
#include "viewers/objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "viewers/vw_arch_playback_memo_avi.h"

#include "vw_vkeyboard.h"
#include "dtf.h"
#include "support/event_loop.h"
#include "ssm.h"



#define ARCH_VE_WIN_SIZE_W			(1000)
#define ARCH_VE_WIN_SIZE_H			(600)

#define ARCH_VE_POS_X				((DISPLAY_ACTIVE_WIDTH - ARCH_VE_WIN_SIZE_W)/4*2)
#define ARCH_VE_POS_Y				((DISPLAY_ACTIVE_HEIGHT - ARCH_VE_WIN_SIZE_H)/2)

enum {
	CLOSE_BTN,
	BTN_COUNTS
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *win_obj;
static NFOBJECT *prog_obj;
static NFOBJECT *btn_obj[BTN_COUNTS];

static NF_AVI_PLAYER_INFO_IN_JUNK detail;
static gchar *file_name[1024];

static gboolean post_close_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
		
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
} 


static gboolean 
post_ve_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	//mb_type ret = -1;

	switch (evt->type) 
	{
		case GDK_EXPOSE:
		break;

		case GDK_DELETE:

			g_curwnd = 0;
			gtk_main_quit();
		break;
		
		default:
		break;
	}

	return FALSE;
}


static gboolean 
post_ve_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	int result;
	
	switch(evt->type) 
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

	}

	return FALSE;
}

void VW_Arch_Playback_Memo_Avi_Open(NFWINDOW *parent, ACPCTX acp, AFILEID id)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;

	GdkPixbuf *prog_img[4];
	gint i;
	gchar ampm[4][10];
	char file_name_buf[1024];
	char *ptr;
	char buf[13][1024];
	char temp[1024];
	int year, mon, day, hour, min, sec;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

	memset(buf, 0x00, sizeof(buf));
	scm_get_afile_name(acp, id, file_name_buf, 1024);	
	ptr = strtok_r(file_name_buf, "/", &pnext);
	pbuf = pnext;
	while( ptr = strtok_r(pbuf, "/", &pnext))
	{
		memset(buf[0], 0x00, sizeof(buf[0]));
		g_sprintf(buf[0], "%s : %s", lookup_string("FILE NAME"), ptr);
		pbuf = pnext;
	}

	scm_get_afile_detail(acp, id, &detail);


	dtf_get_local_datetime(detail.arch_log_info.archived_at, temp);
	sprintf(buf[1], "%s : %s", lookup_string("ARCHIVED AT"), temp);

	if(detail.arch_log_info.reserved_at) {
		dtf_get_local_datetime(detail.arch_log_info.reserved_at, temp);
		sprintf(buf[2], "%s : %s", lookup_string("RESERVED AT"), temp);
	}else
		sprintf(buf[2], "%s : %s", lookup_string("RESERVED AT"), "");

	dtf_get_local_datetime(detail.arch_log_info.movie_start, temp);
	sprintf(buf[3], "%s : %s", lookup_string("MOVIE START"), temp);

	dtf_get_local_datetime(detail.arch_log_info.movie_end, temp);
	sprintf(buf[4], "%s : %s", lookup_string("MOVIE END"), temp);
	
	g_sprintf(buf[5], "%s : %s", lookup_string("TAG"), detail.arch_log_info.tag);
	g_sprintf(buf[6], "%s : %s", lookup_string("USER"), detail.arch_log_info.user);
	g_sprintf(buf[7], "%s : %s", lookup_string("MEMO"), detail.arch_log_info.memo);
	
	if(detail.arch_log_info.dst == 1)
		g_sprintf(buf[8], "DST : %s", lookup_string("ON"));
	else
		g_sprintf(buf[8], "DST : %s", lookup_string("OFF"));

	int size_cnt = 0;
	float total_size = 0;

	total_size = (float)(detail.arch_log_info.file_size) / (float)1024;	//bytes to KBytes
	total_size = (float)(total_size) / (float)1024;
	while((guint)total_size > 1024) {
		++size_cnt;
		total_size/=1024;
	}

	if(size_cnt == 0) g_sprintf(buf[9], "%s : %.1f MB", lookup_string("FILE SIZE"), total_size);
	else if(size_cnt == 1) g_sprintf(buf[9], "%s : %.1f GB", lookup_string("FILE SIZE"), total_size);
	
	g_sprintf(buf[10], "%s : %d", lookup_string("FRAME COUNTS"), detail.v_frames[0] + detail.a_frames[0] + detail.t_frames[0]);

	if(detail.v_frames[0] > 0)
		g_sprintf(buf[11], "%s : %s", lookup_string("VIDEO FRAME"), lookup_string("INCLUDED"));
	else
		g_sprintf(buf[11], "%s : %s", lookup_string("VIDEO FRAME"), lookup_string("NOT INCLUDED"));

	if(detail.a_frames[0] > 0)
		g_sprintf(buf[12], "%s : %s", lookup_string("AUDIO FRAME"), lookup_string("INCLUDED"));
	else
		g_sprintf(buf[12], "%s : %s", lookup_string("AUDIO FRAME"), lookup_string("NOT INCLUDED"));
	

// windows
	win_obj = (NFOBJECT*)nfui_nfwindow_new(parent, 
										ARCH_VE_POS_X, ARCH_VE_POS_Y, 
										ARCH_VE_WIN_SIZE_W, ARCH_VE_WIN_SIZE_H);
	g_curwnd = win_obj;

	nfui_regi_post_event_callback(win_obj, post_ve_win_event_handler);


// fixed 
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, ARCH_VE_WIN_SIZE_W, ARCH_VE_WIN_SIZE_H);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(fixed, post_ve_fixed_event_cb);
	nfui_nfobject_show(fixed);

// title	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ARCHIVED DATA INFORMATION", 
									nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, ARCH_VE_WIN_SIZE_W, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

// DETAIL INFORMATION
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "DETAIL INFORMATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), 
									COLOR_IDX(206));
	
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, 50);

	guint xpos = 30;
	guint ypos = 100;
	
	for( i = 0 ; i < 13 ; i++ )
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf[i],
									nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, 950, 24);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, xpos, ypos);

		ypos += 28;
	}

// CLOSE BUTTON
	btn_obj[CLOSE_BTN] = nftool_normal_button_create_type1("CLOSE", 230);
	nfui_regi_post_event_callback(btn_obj[CLOSE_BTN], post_close_button_event_cb);
	nfui_nfobject_show(btn_obj[CLOSE_BTN]);
	nfui_nffixed_put((NFFIXED*)fixed, btn_obj[CLOSE_BTN], 400, ARCH_VE_WIN_SIZE_H - 60);


	nfui_nfwindow_add((NFWINDOW*)win_obj, fixed);
	nfui_run_main_event_handler(win_obj);
	nfui_nfobject_show(win_obj);

	nfui_make_key_hierarchy((NFWINDOW*)win_obj);
	nfui_page_open(PGID_ARCH_DATA_INFO, win_obj, ssm_get_cur_id(NULL));
	gtk_main();

	nfui_page_close(PGID_ARCH_DATA_INFO, win_obj);

}
