
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

#include "vw_copy_setting.h"
#include "scm.h"
#include "ix_mem.h"

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_all_chk;
static NFOBJECT *g_ch_chk[GUI_CHANNEL_CNT];
static NFOBJECT *g_name_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_model_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_swver_obj[GUI_CHANNEL_CNT];

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

guint VW_Copy_Setting(NFWINDOW *parent, gint src_ch, guint dst_mask, CAM_PROFILE_T *prof)
{
    NFOBJECT *win;
    NFOBJECT *fixed;
    NFOBJECT *fixedTemp;    
    NFOBJECT *ntb;  
    NFOBJECT *obj;
    NFOBJECT *ok_btn;
    gint i, j;
    gint chk_w, chk_h;

    guint table_width[4];
    guint tot_width;

    g_prof = prof;
    g_copy_mask = 0;

    table_width[0] = 36;
    table_width[1] = 240;
    table_width[2] = 300;
    table_width[3] = 300;    

    tot_width = table_width[0]+table_width[1]+table_width[2]+table_width[3];

// MAKE BG IMAGE    
    win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-(tot_width+68))/2, (1080-(36*(GUI_CHANNEL_CNT+1)+160))/2, tot_width+68, 36*(GUI_CHANNEL_CNT+1)+160);
    nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    g_curwnd = win;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, tot_width+68, 37*(GUI_CHANNEL_CNT+1)+160);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));  
    nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
    nfui_nfobject_show(fixed);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COPY SETTINGS TO", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 330, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

    ntb = (NFOBJECT*)nfui_nftable_new(4, GUI_CHANNEL_CNT+1, 2, 1, table_width, 36);
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
        nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, i+1);

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
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i+1);
        g_name_obj[i] = obj;

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);     
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 2, i+1);
        g_model_obj[i] = obj;

        if (g_prof[i].connected) nfui_nflabel_set_text(obj, g_prof[i].model.name);
        else                     nfui_nflabel_set_text(obj, "Not connected");   

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_prof[i].model.swver, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);     
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 3, i+1);
        g_swver_obj[i] = obj;    
    }

    obj = nftool_normal_button_create_type1("OK", 174);
    nfui_regi_post_event_callback(obj, post_ok_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, (tot_width+68)-30-174-10-174, (36*(GUI_CHANNEL_CNT+1)+160)-65);
    ok_btn = obj;

    obj = nftool_normal_button_create_type1("CANCEL", 174);
    nfui_regi_post_event_callback(obj, post_cancel_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, (tot_width+68)-30-174, (36*(GUI_CHANNEL_CNT+1)+160)-65);

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

