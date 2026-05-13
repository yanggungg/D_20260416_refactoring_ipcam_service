
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"

#include "vw_copy_setting_camsetup.h"
#include "scm.h"
#include "ix_mem.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_all_chk;
static NFOBJECT *g_ch_chk[GUI_CHANNEL_CNT];
static NFOBJECT *g_name_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_model_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_swver_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static CAM_PROFILE_T *g_prof;
static gchar *strCh[GUI_CHANNEL_CNT];

static guint g_copy_mask = 0;

static /*inline*/ void set_copy_ch(guint ch) 
{
    g_copy_mask |= (1 << ch);
}

static /*inline*/ void unset_copy_ch(guint ch) 
{
    g_copy_mask &= ~(1 << ch);
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_chk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gboolean active;

    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {

        if (g_all_chk == obj)
        {
            active = nfui_check_button_get_active((NFCHECKBUTTON*)g_all_chk);

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (!nfui_nfobject_is_disabled(g_ch_chk[i]))
                {
                    nfui_check_button_set_active((NFCHECKBUTTON*)g_ch_chk[i], active);
                    nfui_signal_emit(g_ch_chk[i], GDK_EXPOSE, TRUE);

                    if (active) set_copy_ch(i);
                    else        unset_copy_ch(i);    
                }
            }            
        }
        else
        {
            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (g_ch_chk[i] == obj)
                {
                    active = nfui_check_button_get_active((NFCHECKBUTTON*)g_ch_chk[i]);
                
                    if (active)     
                    {
                        set_copy_ch(i);
                    }
                    else
                    {
                        unset_copy_ch(i);    
                        nfui_check_button_set_active((NFCHECKBUTTON*)g_all_chk, active);
                        nfui_signal_emit(g_all_chk, GDK_EXPOSE, TRUE);
                    }
                }
            }            
        }
    }

    return FALSE;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top = NULL;
        gint i;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        top = nfui_nfobject_get_top(obj);
        if(top) nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top = NULL;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        g_copy_mask = 0;

        top = nfui_nfobject_get_top(obj);
        if(top) nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
    gint i;

    if(evt->type == GDK_EXPOSE) 
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);
    }
    else if(evt->type == GDK_DELETE) 
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    
        for (i = 0; i < GUI_CHANNEL_CNT; i++) 
            ifree(strCh[i]);
            
        g_curwnd = 0;       
        gtk_main_quit();
    }

    return FALSE;
}

guint VW_Copy_Setting_camsetup(NFWINDOW *parent, gint src_ch, guint dst_mask, CAM_PROFILE_T *prof)
{
    NFOBJECT *win;
    NFOBJECT *fixed;
    NFOBJECT *fixedTemp;    
    NFOBJECT *ntb;  
    NFOBJECT *obj;
    NFOBJECT *ok_btn;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];
    gint i, j;
    gint chk_w, chk_h;

    guint table_width[4];
    guint win_w, win_h;

    gchar strBuf[64];
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    g_prof = prof;
    g_copy_mask = 0;
    
	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    table_width[0] = 36;
    table_width[1] = 240;
    table_width[2] = 300;
    table_width[3] = 300;    

    win_w = table_width[0]+table_width[1]+table_width[2]+table_width[3];

    if (GUI_CHANNEL_CNT > 16) {
        win_h = 36 * (16 + 1) + 80;
    }
    else {
        win_h = 36 * (GUI_CHANNEL_CNT + 1);
    }

// MAKE BG IMAGE    
    win = (NFOBJECT*)nfui_nfwindow_new(parent, (DISPLAY_ACTIVE_WIDTH - (win_w + 68)) / 2, (DISPLAY_ACTIVE_HEIGHT - (win_h + 160)) / 2, win_w + 68, win_h + 190);
    nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfwindow_use_double_buffer((NFWINDOW*)win);
    g_curwnd = win;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, win_w + 68, win_h + 190);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));  
    nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
    nfui_nfobject_show(fixed);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COPY SETTINGS TO", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 330, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

    ntb = (NFOBJECT*)nfui_nftable_new(4, 1, 2, 1, table_width, 36);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));    
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)fixed, ntb, 30, 68);

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(obj, &chk_w, &chk_h);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_chk_event_cb);
    g_all_chk = obj;

    if (dst_mask == 0) 
    {
        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(obj), FALSE);
        nfui_check_button_sensitive(NF_CHECKBUTTON(obj), FALSE);
    }

    fixedTemp = nfui_nffixed_new();
    nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(fixedTemp, 36, 36);
    nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (36-chk_w)/2, (36-chk_h)/2);
    nfui_nfobject_show(fixedTemp);
    nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));    
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MODEL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));        
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("F/W VERSION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));        
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);

    size_w = 0;
    for (i = 0; i < 4; i++) {
        size_w += table_width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (36 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, main_page_fixed, 30, 105);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (36 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(4, ROW_CNT_PER_PAGE, 2, 1, table_width, 36);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {  
        obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_chk_event_cb);
        g_ch_chk[i] = obj;

        fixedTemp = nfui_nffixed_new();
        nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(fixedTemp, 36, 36);
        nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (36-chk_w)/2, (36-chk_h)/2);
        nfui_nfobject_show(fixedTemp);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], fixedTemp, 0, row_num);

        if (dst_mask & (1 << i)) 
        {
            nfui_check_button_sensitive(NF_CHECKBUTTON(obj), TRUE);
        }
        else
        {
            if (src_ch == i) 
                nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(obj), TRUE);
            else
                nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(obj), FALSE);
                
            nfui_check_button_sensitive(NF_CHECKBUTTON(obj), FALSE);
        }

        strCh[i] = imalloc(sizeof(gchar) * (STRING_SIZE_CAMTITLE+8));   
        j = sprintf(strCh[i], "CH%d - ", i+1);
        //DAL_get_camera_title(strCh[i]+j, i);
        var_get_camtitle(strCh[i]+j, i);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(strCh[i], nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);     
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 1, row_num);
        g_name_obj[i] = obj;

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);     
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 2, row_num);
        g_model_obj[i] = obj;

        if (g_prof[i].connected) nfui_nflabel_set_text(obj, g_prof[i].model.name);
        else                     nfui_nflabel_set_text(obj, "Not connected");   

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_prof[i].model.swver, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);     
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 3, row_num);
        g_swver_obj[i] = obj;    

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

    obj = nftool_normal_button_create_type1("OK", 174);
    nfui_regi_post_event_callback(obj, post_ok_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, (win_w+68)-30-174-10-174, (win_h+190)-65);
    ok_btn = obj;

    obj = nftool_normal_button_create_type1("CANCEL", 174);
    nfui_regi_post_event_callback(obj, post_cancel_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, (win_w+68)-30-174, (win_h+190)-65);

    nfui_nfwindow_add((NFWINDOW*)win, fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);
    nfui_set_key_focus(ok_btn, TRUE);

    nfui_page_open(PGID_COPY_CHMASK_POPUP, win, ssm_get_cur_id(NULL));

    gtk_main();

    nfui_page_close(PGID_COPY_CHMASK_POPUP, win);

    return g_copy_mask;
}

