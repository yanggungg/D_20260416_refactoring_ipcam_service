
#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/color.h"
#include "../support/util.h"
#include "../support/multi_language_support.h"

#include "../tools/nf_ui_tool.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfimage.h"
#include "../viewers/objects/nftable.h"

#include "support/nf_ui_page_manager.h"
#include "vw_passage_popup.h"

static NFWINDOW *g_curwnd = 0;


static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = nfui_nfobject_get_gc(obj);	

			nfui_nfobject_get_size(obj, &size_w, &size_h);
			pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
			nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
		}
		break;

		case GDK_DELETE:
		{
			nfui_nfobject_get_size(obj, &size_w, &size_h);
			nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
		}
		break;
			
		default :
			break;
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

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

gint vw_passage_popup_open(NFWINDOW *parent, gint mx, gint my, PASSDIR_E dir, PARAGRAPH_STR *para[], gint para_cnt)
{
    NFOBJECT *win = NULL;
    NFOBJECT *fixed;
    NFOBJECT *obj;

	//gint i, j, intro_idx = 1;
	gint i, j;
	gint pos_x, pos_y;	
	gint win_x = 0, win_y = 0, win_w = 0, win_h = 0;
    gint max_str_w = 0;		
    gint str_h = 0;		
    gchar strTmp[10];

    for (i = 0; i < para_cnt; i++)
    {   
        for (j = 0; j < para[i]->intro_cnt; j++)
        {
            if (para[i]->intro[j])
            {
                max_str_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), para[i]->intro[j], NORMAL_SPACING);
                if (max_str_w > win_w) win_w = max_str_w;
                
                win_h += nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), para[i]->intro[j], NORMAL_SPACING) + 6;
            }

        }  

        for (j = 0; j < para[i]->body_cnt; j++)
        {
            if (para[i]->body[j])
            {
                max_str_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), para[i]->body[j], NORMAL_SPACING);
                if (max_str_w > win_w) win_w = max_str_w;
                
                //win_h += 28;
                win_h += nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), para[i]->body[j], NORMAL_SPACING) + 6;
            }
        }

        win_h += 20;
    }

    win_w += 80;
    win_h += 70;

    if (dir == DIR_TOP_LEFT) {
        win_x = mx - win_w;
        win_y = my - win_h;       
    }
    else if (dir == DIR_TOP_RIGHT) {
        win_x = mx;
        win_y = my - win_h;
    }
    else if (dir == DIR_BOTTOM_LEFT) {
        win_x = mx - win_w;
        win_y = my;
    }
    else if (dir == DIR_BOTTOM_RIGHT) {
        win_x = mx;
        win_y = my;
    }
    else 
        g_assert(0);

    if (win_x < 0) win_x = 0;
    if (win_y < 0) win_y = 0;
    
    if (win_x + win_w > 1920) win_x = 1920 - win_w;
    if (win_y + win_h > 1920) win_y = 1080 - win_h;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, win_x, win_y, (guint)win_w, (guint)win_h);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_main_wnd_event_handler);	
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win_w, win_h);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);

	pos_x = 10;
	pos_y = 16;

    for (i = 0; i < para_cnt; i++)
    {        
        memset(strTmp, 0x00, sizeof(strTmp));
		switch(para[i]->intro_type)
		{
			case BULLET_BLANK:
				g_stpcpy(strTmp, "");
			break;

			//case BULLET_NUM:
		//		g_sprintf(strTmp, "%d.", intro_idx++);
		//	break;

			case BULLET_HYPHEN:
				g_stpcpy(strTmp, "- ");
			break;

			default:
			break;
		}

       	pos_x = 10;

		for (j = 0; j < para[i]->intro_cnt; j++)
		{
            if (para[i]->intro[j])
            {
				if((para[i]->intro_type) == BULLET_NUM)
					g_sprintf(strTmp, "%d.", j+1);

            	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTmp, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
                str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), strTmp, NORMAL_SPACING) + 6;
            	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
            	nfui_nfobject_set_size(obj, 36, str_h);
            	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

            	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(para[i]->intro[j], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
                str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), para[i]->intro[j], NORMAL_SPACING) + 6;
            	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
            	nfui_nfobject_set_size(obj, win_w-60, str_h);
            	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            	nfui_nfobject_show(obj);

				if((para[i]->intro_type) == BULLET_BLANK)
					nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
				else
					nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);

            	//pos_y += 28;
				pos_y += str_h;
            }
        }  

       	pos_x = 20;

		switch(para[i]->body_type)
		{
			case BULLET_BLANK:
				g_stpcpy(strTmp, "");
			break;

			//case BULLET_NUM:
		//		g_sprintf(strTmp, "%d.", intro_idx++);
		//	break;

			case BULLET_HYPHEN:
				g_stpcpy(strTmp, "- ");
			break;
			default:
			break;
		}

        for (j = 0; j < para[i]->body_cnt; j++)
        {
            if (para[i]->body[j])
            {
				if((para[i]->body_type) == BULLET_NUM)
					g_sprintf(strTmp, "(%d) ", j+1);

            	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTmp, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
                str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), strTmp, NORMAL_SPACING) + 6;
            	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
            	nfui_nfobject_set_size(obj, win_w-60, str_h);
            	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

            	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(para[i]->body[j], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
                str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), para[i]->body[j], NORMAL_SPACING) + 6;
            	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
            	nfui_nfobject_set_size(obj, win_w-60, str_h);
            	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);            	

            	//pos_y += 28;
				pos_y += str_h;
            }
        }
   
        pos_y += 20;    	
    }

	obj = nftool_normal_button_create_popup_type2("EXIT", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (win_w-174)/2, win_h-44);
	nfui_regi_post_event_callback(obj, _post_exit_btn_event_handler);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy(win);
    nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_PASSAGE_POPUP, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_PASSAGE_POPUP, win);	
}

