#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfobject.h"

#include "vw_qc_test_new_audio_test_popup.h"
#include "vw_desc.h"


#define AUDIO_OFF						(0xff)

#define QCAT_WND_SIZE_W                 (guint)(550)
#define QCAT_WND_SIZE_H                 (guint)(230)

#define QCAT_WND_POS_X                  (guint)((DISPLAY_ACTIVE_WIDTH - QCAT_WND_SIZE_W) / 2)
#define QCAT_WND_POS_Y                  (guint)((DISPLAY_ACTIVE_HEIGHT - QCAT_WND_SIZE_H) / 2)

#define QCAT_TITLE_LB_SIZE_W            (300)

#define QCAT_RESULT_POS_X               (20 + 300 + 200 + 70)

enum {
    PASS = 0,
    FAIL
};

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_res_fixed[AUDIO_IN_CNT] = {0,};
static NFOBJECT *g_lb_adin_ch = NULL;
static NFOBJECT *g_bt_pass = NULL;

static gint g_adin_ch = 0;
static gboolean g_res = FALSE;
static GdkPixbuf *g_prev_img[NFOBJECT_STATE_COUNT] = {0,};
static GdkPixbuf *g_next_img[NFOBJECT_STATE_COUNT] = {0,};


static gint _init_audio_test()
{
    g_res = FALSE;
    g_adin_ch = 0;
    scm_change_live_audio(g_adin_ch);

    return 0;
}

static gint _draw_res_image(gint res)
{
    GdkPixbuf *res_img;
    GdkDrawable *drawable;
    GdkGC *gc;
    gint off_x, off_y;

    if (res == PASS)
    {
        res_img = nfui_get_image_from_file(("qc_test_pass.png"), NULL);
    }
    else
    {
        res_img = nfui_get_image_from_file(("qc_test_fail.png"), NULL);
    }

    drawable = nfui_nfobject_get_window((NFOBJECT*)g_curwnd);
    gc = nfui_nfobject_get_gc((NFOBJECT*)g_curwnd);

    nfui_nfobject_get_offset(g_res_fixed[g_adin_ch], &off_x, &off_y);

    nfutil_draw_pixbuf(drawable, gc, res_img, (off_x+20), off_y, -1, -1, NFALIGN_LEFT, 0);

	nfui_nfobject_gc_unref(gc);
}

static gboolean _post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_BUTTON_RELEASE:
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON)
                return FALSE;

            g_res = TRUE;
            nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
        }
        break;
    }

    return FALSE;
}

static gboolean _post_failbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_BUTTON_RELEASE:
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON)
                return FALSE;

            g_res = FALSE;
            nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
        }
        break;
    }

    return FALSE;
}

static gboolean _post_prev_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (g_adin_ch == 0) return FALSE;

        g_adin_ch--;
        scm_change_live_audio(g_adin_ch);

        g_sprintf(buf, "%s %d", lookup_string("AUDIO"), g_adin_ch+1);
        nfui_nflabel_set_text(g_lb_adin_ch, buf);
        nfui_signal_emit(g_lb_adin_ch, GDK_EXPOSE, FALSE);
    }

    return FALSE;
}

static gboolean _post_next_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar buf[32];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        if (g_adin_ch == (AUDIO_IN_CNT-1)) return FALSE;

        g_adin_ch++;
        scm_change_live_audio(g_adin_ch);

        g_sprintf(buf, "%s %d", lookup_string("AUDIO"), g_adin_ch+1);
        nfui_nflabel_set_text(g_lb_adin_ch, buf);
        nfui_signal_emit(g_lb_adin_ch, GDK_EXPOSE, FALSE);

        if (g_adin_ch == ((AUDIO_IN_CNT-1)))
        {
            nfui_nfobject_enable(g_bt_pass);
            nfui_signal_emit(g_bt_pass, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean _post_fail_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _draw_res_image(FAIL);
    }

    return FALSE;
}

static gboolean _post_success_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _draw_res_image(PASS);
    }

    return FALSE;
}

static gboolean _post_res_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    return FALSE;
}

static gboolean _post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    return FALSE;
}

static gboolean _post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
    gint i;

	switch(evt->type)
	{
		case GDK_DELETE:
			if(scm_get_cur_live_audio_ch() != AUDIO_OFF) {
        		scm_change_live_audio(AUDIO_OFF);
    		}

            for (i = 0; i < NFOBJECT_STATE_COUNT; i++)
            {
                g_object_unref(g_prev_img[i]);
                g_object_unref(g_next_img[i]);
                g_prev_img[i] = NULL;
                g_next_img[i] = NULL;
            }

            g_adin_ch = AUDIO_OFF;
			g_curwnd = 0;
			gtk_main_quit();
		break;

		default:
		break;
	}

	return FALSE;
}

gboolean VW_QC_Test_Audio_Popup(NFWINDOW *parent, gint x, gint y)
{
    NFOBJECT *main_wnd;
    NFOBJECT *main_fixed, *content_fixed;
    NFOBJECT *obj;
    gint size_w, size_h;
    guint pos_x, pos_y;
    gint i;
    gchar buf[64];
    guint kpid;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];



    _init_audio_test();

	g_prev_img[0] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);
	g_prev_img[1] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);
	g_prev_img[2] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);
	g_prev_img[3] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);

	g_next_img[0] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);
	g_next_img[1] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);
	g_next_img[2] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);
	g_next_img[3] = gdk_pixbuf_scale_simple(nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL), 100, 40, GDK_INTERP_BILINEAR);

	pos_x = x - QCAT_WND_SIZE_W;

    main_wnd = nftool_create_popup_window(parent, pos_x, y, QCAT_WND_SIZE_W, QCAT_WND_SIZE_H, "AUDIO TEST", FALSE);
    nfui_regi_post_event_callback(main_wnd, _post_main_wnd_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;
    nfui_regi_post_event_callback(main_fixed, _post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 50;

    obj = nfui_nflabel_new_with_pango_font("AUDIO INPUT CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, QCAT_TITLE_LB_SIZE_W, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += QCAT_TITLE_LB_SIZE_W + 2;

    g_sprintf(buf, "%s 1", lookup_string("AUDIO"));

    obj = nfui_nflabel_new_text_box(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 202, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_lb_adin_ch = obj;

    pos_x = 20;
    pos_y += 40 + 3;

    obj = nfui_nflabel_new_with_pango_font("CHANGING CHANNELS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, QCAT_TITLE_LB_SIZE_W, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_use_focus(obj, FALSE);
    //nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += QCAT_TITLE_LB_SIZE_W + 2;
    nfui_get_pixbuf_size(g_prev_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), g_prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_prev_btn_handler);

    pos_x += size_w + 2;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), g_next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_next_btn_handler);
/*

    pos_x = 20;
	pos_y += 40 + 3;

    obj = nfui_nflabel_new_with_pango_font("TEST RESULT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, QCAT_TITLE_LB_SIZE_W, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += QCAT_TITLE_LB_SIZE_W + 2;

    obj = nftool_normal_button_create_type1("FAIL", 100);
    nfui_nfbutton_set_font_alignment((NFBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_fail_btn_handler);

    pos_x += 100 + 2;

    obj = nftool_normal_button_create_type1("SUCCESS", 100);
    nfui_nfobject_get_size(obj, &size_w, &size_h);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_success_btn_handler);

    pos_x = QCAT_RESULT_POS_X;
    pos_y = 50;
    for (i = 0; i < AUDIO_IN_CNT; i++)
    {
        g_sprintf(buf, "%s %2d", lookup_string("AUDIO"), i+1);

        obj = nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, 150, 40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

        obj = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(obj, 80, 40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x + 150 + 2, pos_y);
        nfui_regi_post_event_callback(obj, _post_res_fixed_event_handler);
        g_res_fixed[i] = obj;

        pos_y += 40 + 3;

        if (i == 7)
        {
            pos_x = QCAT_RESULT_POS_X + 237;
            pos_y = 50;
        }
    }
*/
    obj = nftool_normal_button_create_type1("FAIL", 100);
    nfui_nfobject_get_size(obj, &size_w, &size_h);
    nfui_nfbutton_set_font_alignment((NFBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, ((QCAT_WND_SIZE_W)/2) - 103, (QCAT_WND_SIZE_H - size_h - 20));
    nfui_nfobject_use_focus(obj, TRUE);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_failbutton_event_handler);

    obj = nftool_normal_button_create_type1("PASS", 100);
    nfui_nfobject_get_size(obj, &size_w, &size_h);
    nfui_nfbutton_set_font_alignment((NFBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, ((QCAT_WND_SIZE_W)/2) + 3, (QCAT_WND_SIZE_H - size_h - 20));
    nfui_nfobject_use_focus(obj, TRUE);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_okbutton_event_handler);
	if( AUDIO_IN_CNT > 1 ) nfui_nfobject_disable(obj);
    g_bt_pass = obj;

    nfui_nfobject_show(main_wnd);

    nfui_make_key_hierarchy((NFWINDOW*)main_wnd);

    nfui_page_open(PGID_QC_AUDIO_TESTER_POPUP, main_wnd, ssm_get_cur_id(NULL));
	nf_sysman_set_qc_live_audio(1);

    gtk_main();

	nf_sysman_set_qc_live_audio(0);
    nfui_page_close(PGID_QC_AUDIO_TESTER_POPUP, main_wnd);

    return g_res;
}

