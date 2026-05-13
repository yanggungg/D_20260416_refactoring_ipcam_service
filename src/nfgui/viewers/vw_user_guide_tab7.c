#include <string.h>
#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_title.h"
#include "vw_sys_camera_image.h"

#include "nf_notify.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfimglabel.h"
#include "objects/nfspinbutton.h"

#include "vw_vkeyboard.h"
#include "scm.h"
#include "ix_mem.h"

#include "vsm.h"
#include "vw_menu.h"
#include "vw.h"
#include "vw_user_guide_tab_internal.h"


static NFWINDOW *g_curwnd = 0;

static NFWINDOW *g_content_fixed = 0;
static NFOBJECT *g_page_label = 0;
static NFOBJECT *g_page_prev = 0;
static NFOBJECT *g_page_next = 0;

static gint g_pageIdx = 0;
static gint g_page_cnt = 0;

static gchar g_tmplang[16];


static gint _change_page_label()
{
    gchar strBuf[16];

    if (g_page_cnt < 1) return -1;
    if (g_pageIdx < 0) return -1;
    if (g_pageIdx+1 > g_page_cnt) return -1;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d / %d", g_pageIdx+1, g_page_cnt);
    nfui_nfimage_set_text((NFIMAGE*)g_page_label, strBuf);

    return 0;
}

static gint _draw_guide_image()
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint x, y;
    gchar image[64];

	if (!strlen(g_tmplang)) return -1;

	drawable = nfui_nfobject_get_window(g_content_fixed);
	gc = nfui_nfobject_get_gc(g_content_fixed);

	nfui_nfobject_get_offset(g_content_fixed, &x, &y);

    memset(image, 0x00, sizeof(image));
    g_sprintf(image, "06_guide_%s_%02d.png", g_tmplang, g_pageIdx+1);

    g_message("%s, %d, imf_name:%s", __FUNCTION__, __LINE__, image);    
    nfutil_draw_image_no_cache(drawable, gc, image, x, y, -1, -1, NFALIGN_LEFT, 0);            

    return 0;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
            _draw_guide_image();
		}
		break;

        case GDK_DELETE : 
        {
            g_pageIdx = 0;
            g_curwnd = 0;
        }
        break;

		default :
			break;
	}

	return FALSE;
}

static gboolean post_prev_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{	
		case GDK_BUTTON_RELEASE:
        {
            if (g_pageIdx == 0) return FALSE;
            
            g_pageIdx -= 1;

            _change_page_label();
            _draw_guide_image();

            nfui_signal_emit(g_page_label, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_page_prev, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_page_next, GDK_EXPOSE, TRUE);           
        }
		break;

		default:
		break;
	}
	
	return FALSE;
}

static gboolean post_next_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{	
		case GDK_BUTTON_RELEASE:
        {
            if (g_pageIdx+1 >= g_page_cnt) return FALSE;

            g_pageIdx += 1;

            _change_page_label();            
            _draw_guide_image();

            nfui_signal_emit(g_page_label, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_page_prev, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_page_next, GDK_EXPOSE, TRUE);
        }
		break;

		default:
		break;
	}
	
	return FALSE;
}

static gboolean post_exit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{	
		case GDK_BUTTON_RELEASE:
			nfui_nfobject_destroy((NFOBJECT*)g_curwnd); 
		break;

		default:
		break;
	}
	
	return FALSE;
}


void init_UserGuide_tab7_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
    NFOBJECT *obj;
    gint i;
    gint ret;

	GdkPixbuf *prev_pbuf[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_pbuf[NFOBJECT_STATE_COUNT];

	GdkPixbuf *bgImg;
	GdkPixbuf *pbBG;
	gchar lang[64];
    gchar image[64];

	prev_pbuf[0] = nfui_get_image_from_file((IMG_BT_PREV_N), NULL); 
	prev_pbuf[1] = nfui_get_image_from_file((IMG_BT_PREV_O), NULL); 
	prev_pbuf[2] = nfui_get_image_from_file((IMG_BT_PREV_P), NULL); 
	prev_pbuf[3] = nfui_get_image_from_file((IMG_BT_PREV_D), NULL); 

	next_pbuf[0] = nfui_get_image_from_file((IMG_BT_NEXT_N), NULL); 
	next_pbuf[1] = nfui_get_image_from_file((IMG_BT_NEXT_O), NULL); 
	next_pbuf[2] = nfui_get_image_from_file((IMG_BT_NEXT_P), NULL); 
	next_pbuf[3] = nfui_get_image_from_file((IMG_BT_NEXT_D), NULL); 

	g_curwnd = nfui_nfobject_get_top(parent);

	ret = icf_get_value_by_int("guide7", "cnt");

    if (ret != -1) {
        g_page_cnt = ret;
    }

	memset(lang, 0x00, sizeof(lang));
	memset(g_tmplang, 0x00, sizeof(g_tmplang));

	DAL_get_language(lang);
	if (strcmp(lang, "KOREAN") == 0) strcpy(g_tmplang, "kor");	

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(content_fixed, GUIDE_CONTENT_FIXED_WIDTH, GUIDE_CONTENT_FIXED_HEIGHT);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, 0, 0);
	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
    g_content_fixed = content_fixed;

    if (g_page_cnt > 1)
    {
        obj = (NFOBJECT*)nfui_nfbutton_new_with_param(prev_pbuf, "");
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, GUIDE_PREV_BTN_X, GUIDE_PREV_BTN_Y);
    	nfui_regi_post_event_callback(obj, post_prev_button_event_handler);
        g_page_prev = obj;

        obj = (NFOBJECT*)nfui_nfbutton_new_with_param(next_pbuf, "");
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, GUIDE_NEXT_BTN_X, GUIDE_NEXT_BTN_Y);
    	nfui_regi_post_event_callback(obj, post_next_button_event_handler);
        g_page_next = obj;

	    memset(image, 0x00, sizeof(image));
	    g_sprintf(image, "06_guide_%s_%02d.png", g_tmplang, 1);

    	bgImg = nfui_get_pixbuf_from_file(image, NULL);

    	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, GUIDE_PAGE_LABEL_WIDTH, GUIDE_PAGE_LABEL_HEIGHT);
    	gdk_pixbuf_copy_area(bgImg, GUIDE_PAGE_LABEL_X, GUIDE_PAGE_LABEL_Y, GUIDE_PAGE_LABEL_WIDTH, GUIDE_PAGE_LABEL_HEIGHT, pbBG, 0, 0);

    	obj = nfui_nfimage_new_pixbuf(pbBG);
    	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    	nfui_nfobject_set_size(obj, GUIDE_PAGE_LABEL_WIDTH, GUIDE_PAGE_LABEL_HEIGHT);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, GUIDE_PAGE_LABEL_X, GUIDE_PAGE_LABEL_Y);
    	g_page_label = obj;

    	_change_page_label();

		g_object_unref(bgImg);
    }
    
	obj = nftool_normal_button_create_type3("EXIT", GUIDE_TAB_V_BTN_R1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, GUIDE_TAB_V_BTN_R1_X, GUIDE_TAB_V_BTN_R1_Y);
	nfui_regi_post_event_callback(obj, post_exit_button_event_handler);
	
}

gboolean UserGuide_tab7_in_handler()
{
    g_pageIdx = 0;   
	_change_page_label();

    return FALSE;
}


gboolean UserGuide_tab7_out_handler()
{

    return FALSE;
}

