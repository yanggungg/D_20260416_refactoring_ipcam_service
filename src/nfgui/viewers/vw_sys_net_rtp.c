#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftab.h"
// #include "objects/nfimage.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_rtp_rtp.h"
#include "vw_sys_net_rtp_multicast.h"



static NFWINDOW *g_curwnd;
static NFOBJECT *g_nftab;



static void _out_handler_rtp_main(gint page)
{
    if (page == 0)
    {
        Rtp_tab_out_handler();
    }
    else if (page == 1)
    {
        Multicast_tab_out_handler();
    }
}

static void _in_handler_rtp_main(gint page)
{
    if (page == 0)
    {
    }
    else if (page == 1)
    {
    }   
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{	
	gint cur_page;
	gint new_page;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE) 
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page) return FALSE;

		_out_handler_rtp_main(cur_page);
		_in_handler_rtp_main(new_page);
	}
	
	return FALSE;
	
}

static gboolean pre_subtab_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkGC *gc;
		GdkDrawable *drawable;
		guint x, y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);
		nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{			
			GdkGC *gc;
			GdkDrawable *drawable;
			guint x, y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_FIXED_BG, x, y, -1, -1, NFALIGN_LEFT, 0);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x+1, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			nfui_nfobject_gc_unref(gc);				
		}
		break;

		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default :
			break;
	}

	return FALSE;
}

void init_NetRtp_page(NFOBJECT *parent)
{
	NFOBJECT *nftab;
	NFOBJECT *tab_page[2];
	gint i;

	const gchar *strTabTitle[] = {"RTP", "MULTICAST"};
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);	
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(2, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
	g_nftab = nftab;
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);	
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

	for(i=0; i<2; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

	init_NetRtp_Rtp_Page(tab_page[0]);
	init_NetRtp_MultiCase_Page(tab_page[1]);

	nfui_nfobject_show(tab_page[0]);
	
    return;
}

gboolean NetRtp_tab_in_handler()
{
	gint cur_page;	
	cur_page = nfui_nftab_get_cur_page((NFTAB*)g_nftab);

	if( cur_page == 0 )
	{
	}
	else if( cur_page == 1 )
	{
	}
	
	return FALSE;
}

gboolean NetRtp_tab_out_handler()
{
	gint cur_page;	
	cur_page = nfui_nftab_get_cur_page((NFTAB*)g_nftab);	

	if( cur_page == 0 )
	{
	    Rtp_tab_out_handler();
	}
    else if (cur_page == 1)
    {
        Multicast_tab_out_handler();
    }
    
	return FALSE;
}
#if 0
void init_NetRtp_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;

    gint i, col, row;
    guint pos_x, pos_y;
    gchar strBuf[STRING_SIZE_CAMTITLE];
    gchar strBuf_num[STRING_SIZE_CAMTITLE];

    gchar *mode[] = {"AUTO", "MANUAL"};
    gchar *tb_title[] = {"", "STREAM IP", "VIDEO PORT", "AUDIO PORT"};
    gchar *tb_stream[] = {"1ST STREAM", "2ND STREAM"};

    
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

////////////////////////////////////////////////////
//	RTP
//	
    pos_x = RTP_CATE_TITLE_X;
    pos_y = RTP_CATE_TITLE_Y;
    
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RTP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);


//===> OPERATION MODE
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 21;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_x += RTP_ITEM_TITLE_W;
    
	obj = nfui_spinbutton_new(mode, 2, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_nflabel_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, RTP_ITEM_CELL_W, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	//nfui_regi_post_event_callback(obj, post_date_format_event_handler);

//===> PORT
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += RTP_ITEM_TITLE_W;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, (RTP_ITEM_CELL_W/2) - 15, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_x += ((RTP_ITEM_CELL_W/2) - 15) + 5;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, 20, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 20 + 5;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, (RTP_ITEM_CELL_W/2) - 15, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);


////////////////////////////////////////////////////
//	MULTICAST
//	
    pos_x = RTP_CATE_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 60;
    
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "MULTICAST");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

//===> CHANNEL
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 21;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += RTP_ITEM_TITLE_W;

	obj = (NFOBJECT*)nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	
	for (i = 0; i < GUI_CHANNEL_CNT; ++i)
	{
        var_get_camtitle(strBuf,i);
        g_sprintf(strBuf_num, "%s%d - %s", lookup_string("CH"), i+1, strBuf);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf_num);
	}

//===> STREAM TITLE
    pos_x = 200;
    pos_y += RTP_ITEM_TITLE_H + 40;

    for (i = 0; i < STREAM_CNT; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tb_stream[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    	nfui_nfobject_set_size(obj, 604, RTP_ITEM_CELL_H);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 604 + 2;
    }
//===> ITEM TITLE
    pos_x = 200;
    pos_y += RTP_ITEM_TITLE_H + 2;

    for (row = 0; row < GUI_CHANNEL_CNT; row++)
    {
        for (i = 0; i < 6; i++)
        {
            if (i == 0 || i == 3)
            {
            	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(255, 255, 255, 255);
        		nfui_nfipeditor_set_pango_font((NFIPEDITOR*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
            	nfui_nfobject_set_size(obj, 200, RTP_ITEM_CELL_H);
        		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
            	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
            }
            else
            {
            	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
            	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
            	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
            	nfui_nfobject_set_size(obj, 200, RTP_ITEM_CELL_H);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
            }

            pos_x += 200 + 2;
        }
        pos_x = 200;
        pos_y += RTP_ITEM_CELL_H + 2;
    }
//===> STREAM IP
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STREAM IP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    pos_x += RTP_ITEM_TITLE_W;
    
    for (i = 0; i < STREAM_CNT; i++)
    {
    	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(255, 255, 255, 255);
		nfui_nfipeditor_set_pango_font((NFIPEDITOR*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += RTP_ITEM_TITLE_W + 2;
    }

//===> VIDEO PORT
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    pos_x += RTP_ITEM_TITLE_W;
    
    for (i = 0; i < STREAM_CNT; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
    	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += RTP_ITEM_TITLE_W + 2;
    }

//===> AUDIO PORT
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    pos_x += RTP_ITEM_TITLE_W;
    
    for (i = 0; i < STREAM_CNT; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
    	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += RTP_ITEM_TITLE_W + 2;
    }
   
//===> MENU BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    return;
}
#endif

