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

static gchar g_tmplang[16];

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
    g_sprintf(image, "00_cover_%s.png", g_tmplang);

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
            g_curwnd = 0;
        }
        break;

		default :
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

void init_UserGuide_tab1_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
    NFOBJECT *obj;
    gint i;
    gint ret;
	gchar lang[64];

	g_curwnd = nfui_nfobject_get_top(parent);

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
    
	obj = nftool_normal_button_create_type3("EXIT", GUIDE_TAB_V_BTN_R1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, GUIDE_TAB_V_BTN_R1_X, GUIDE_TAB_V_BTN_R1_Y);
	nfui_regi_post_event_callback(obj, post_exit_button_event_handler);
}

gboolean UserGuide_tab1_in_handler()
{

    
    return FALSE;
}


gboolean UserGuide_tab1_out_handler()
{

    return FALSE;
}



