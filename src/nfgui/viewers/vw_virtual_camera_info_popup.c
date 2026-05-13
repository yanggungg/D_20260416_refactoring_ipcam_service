#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfimage.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nflabel.h"
#include "objects/nflistbox.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "nf_sysman.h"
#include "vsm.h"

#include "vw_virtual_camera_set_popup.h"


#define VC_INFO_WND_SIZE_W          700
#define VC_INFO_WND_SIZE_H          400


static NFOBJECT *g_curwnd = 0;

enum {
    LIST_1 = 0,
    LIST_2, 
    LIST_3,
    NUM_LIST
};


static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_virtual_camera_info_page(NFWINDOW *parent, guint x, guint y)
{
    NFOBJECT *main_wnd;
    NFOBJECT *nffixed;
    NFOBJECT *fixed;
    NFOBJECT *fixed_temp;
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    NFOBJECT *rest_t;
    
    gchar str[32];
    gint i,j;
    gint pos_x, pos_y;
    gint size_w, size_h;

    guint table_w[2] = {160, 160};

    gchar *disp_list[] = {	
    "352x240",		//B     (0)
	"704x240",		//C     (1)
	"704x480",		//D     (2)
	"704x480P",		//E     (3)
	"352x288",		//F     (4)
	"704x288",		//G     (5)
	"704x576",		//H     (6)
	"704x576P",		//I     (7)									
	"640x480",		//J     (8)
	"720x480",		//K     (9)
	"720x576",		//L     (10)
	"800x600",		//M     (11)
	"1024x768",		//N     (12)
	"1280x1024",	//O     (13)
	"1600x1200",	//P     (14)
	"1280x720",		//Q     (15)
	"1920x1080",	//R     (16)
	"640x352",		//S     (17)
	"640x360",		//T     (18)
	"640x360I",		//U     (19)
	"1280x720I",	//V     (20)
	"1920x1280I",	//W     (21)
	"640x400",	    //X     (22)
	"800x450",	    //Y     (23)
	"1440x900",	    //Z     (24)
	"960x480",	    //a     (25)
	"960x576",	    //b     (26)
	"320x180",	    //c     (27)
	"2304x1296",	//d     (28)	
	"2048x1536",	//e     (29)	
	"2560x1440",	//f     (30)	
	"2688x1520",	//g     (31)	
	"2560x1600",	//h     (32)	
	"2560x1920",	//i     (33)	
	"2592x1920",	//j     (34)	
	"2592x1944",	//k     (35)	
	"2992x1680",	//l     (36)	
	"2880x1800",	//m     (37)	
	"3200x1800",	//n     (38)	
	"2880x2160",	//o     (39)	
	"3072x2048",	//p     (40)	
	"3200x2400",	//q     (41)	
	"3840x2160",	//r     (42)	
	};
	
    gchar *str_temp[2];    

    guint listbox_w[2] = { 150, 150};
    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];

    main_wnd = nftool_create_popup_window(parent, x, y - VC_INFO_WND_SIZE_H, VC_INFO_WND_SIZE_W, VC_INFO_WND_SIZE_H, "", FALSE);

    nfui_regi_post_event_callback(main_wnd, post_page_event_handler);
    g_curwnd = main_wnd;

    nffixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = 10;
    pos_y = 10;

    // restriction 
   
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(227));
    nfui_nfobject_set_size(fixed, VC_INFO_WND_SIZE_W - 20, 306);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)nffixed, fixed, (guint)pos_x, (guint)pos_y);

    pos_x = 2;
    pos_y = 2;
    
    fixed_temp = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(fixed_temp, VC_INFO_WND_SIZE_W - 24, 302);
    nfui_nfobject_show(fixed_temp);
    nfui_nffixed_put((NFFIXED*)fixed, fixed_temp, (guint)pos_x, (guint)pos_y);
    
    pos_y = 2;
    pos_x = 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RESTRICTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    //nfui_nfobject_set_size(obj, VCAMERA_POPUP_SIZE_WIDTH - 40, 40);
    nfui_nfobject_set_size(obj, VC_INFO_WND_SIZE_W - 28, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, pos_x, pos_y);
    
    pos_y += 40;

    pos_x += 320 + 10;

    // jaeyoung test here
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SUPPORTED RESOLUTION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115)); // 372
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 10);
    nfui_nfobject_set_size(obj, 300, 33);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, pos_x+15, pos_y);

    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);

    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);  

    pos_y += 33;

    nfui_get_image_size(IMG_N_SCROLL_UP, &size_w, &size_h);

    obj = nfui_listbox_new(2, listbox_w, 32);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), LIST_1, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), LIST_2, NFALIGN_CENTER);
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_nfobject_set_size(obj, listbox_w[0] + listbox_w[1] + size_w , 32*7);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, pos_x + 15, pos_y);    

    i = 0;
    
    for( j = 0; j < 21; j++)
    {   
        str_temp[LIST_1] = imalloc(sizeof(gchar)*32);
        strcpy(str_temp[LIST_1], disp_list[i]);
        i++;
        
        str_temp[LIST_2] = imalloc(sizeof(gchar)*32);
        strcpy(str_temp[LIST_2], disp_list[i]);
        i++;

        nfui_listbox_set_text((NFLISTBOX*)obj, str_temp);
        
        ifree(str_temp[LIST_1]);
        ifree(str_temp[LIST_2]);
    }
    
    //    test end here
    //    

    pos_y -= 33 ;
    pos_x -= 320;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BITRATE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 324, 30);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, pos_x-1, pos_y);

    pos_y += 30;

    rest_t = (NFOBJECT*)nfui_nftable_new(2, 2, 1, 1, table_w, 30);
    nfui_nftable_set_draw_outline((NFTABLE*)rest_t, TRUE);
    nfui_nfobject_show(rest_t);
    nfui_nffixed_put((NFFIXED*)fixed_temp, rest_t, pos_x, pos_y);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("4CH", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 0, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MAX 8000kb/s", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 1, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("8CH, 16CH", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 0, 1);    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MAX 4000kb/s", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 1, 1);

    pos_y += 67;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FPS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 324, 30);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, pos_x-1, pos_y);

    pos_y += 30;

    rest_t = (NFOBJECT*)nfui_nftable_new(2, 2, 1, 1, table_w, 30);
    nfui_nftable_set_draw_outline((NFTABLE*)rest_t, TRUE);
    nfui_nfobject_show(rest_t);
    nfui_nffixed_put((NFFIXED*)fixed_temp, rest_t, pos_x, pos_y);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NTSC", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 0, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("30, 1", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 1, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PAL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 0, 1);    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("25, 1", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 1, 1);

    pos_y += 67;

    rest_t = (NFOBJECT*)nfui_nftable_new(2, 2, 1, 1, table_w, 30);
    nfui_nftable_set_draw_outline((NFTABLE*)rest_t, TRUE);
    nfui_nfobject_show(rest_t);
    nfui_nffixed_put((NFFIXED*)fixed_temp, rest_t, pos_x, pos_y);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO CODEC", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 0, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("H264", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 1, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO CODEC", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 0, 1);    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("G711 u_Law", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFFIXED*)rest_t, obj, 1, 1);  
            
    obj = nftool_normal_button_create_popup_type2("CLOSE", 152);
    nfui_nfobject_show(obj);    
    nfui_nffixed_put((NFFIXED*)nffixed, obj, (VC_INFO_WND_SIZE_W - 152)/2, VC_INFO_WND_SIZE_H - 45);
    nfui_regi_post_event_callback(obj, post_close_button_event_handler);

    nfui_nfobject_show(main_wnd);
    nfui_make_key_hierarchy(main_wnd);
    
    gtk_main();
   
    return FALSE;
}

