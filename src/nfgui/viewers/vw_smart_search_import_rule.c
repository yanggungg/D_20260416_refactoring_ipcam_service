
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

#include "scm.h"
#include "ix_mem.h"

#include "vaa.h"
#include "vaa_itx.h"

#include "vw_dit_vca.h"
#include "vw_vca_rev_component.h"
#include "vw_smart_search_import_rule.h"




////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_all_chk;
static NFOBJECT *g_rule_chk[16];
static NFOBJECT *g_id_obj[16];
static NFOBJECT *g_color_obj[16];
static NFOBJECT *g_name_obj[16];

static guint g_copy_mask = 0;

static gint g_cur_ch = 0;
static guint g_draw_tid = 0;

static guint g_all_bitmsk = 0;
static guint g_sel_bitmsk = 0;

static ITX_VAZONE_CONF g_pre_zone_conf[16];
static ITX_VAZONE_SHAPE g_pre_zone_shape[16];





////////////////////////////////////////////////////////////
//
// private interfaces 
//

static /*inline*/ void _set_copy_rule(guint ch) 
{
	g_copy_mask |= (1 << ch);
}

static /*inline*/ void _unset_copy_rule(guint ch) 
{
	g_copy_mask &= ~(1 << ch);
}

static gint _update_rule_list_object(gint idx)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;
    gchar str[32];

    memset(str,0x00,sizeof(str));

    vaaid = vaa_get_vaaid(g_cur_ch);
    vaa_itx_get_zone_conf(vaaid, idx, &conf);
    vaa_itx_get_zone_shape(vaaid, idx, &shape);
   
    if (conf.use_zone && conf.active)
    {
        g_sprintf(str,"Z%02d",conf.zoneid);
        nfui_nflabel_set_text((NFLABEL*)g_id_obj[idx], str);
        nfui_nfobject_modify_bg(g_color_obj[idx], NFOBJECT_STATE_NORMAL, COLOR_IDX(shape.color_idx));        
        nfui_nflabel_set_text((NFLABEL*)g_name_obj[idx], shape.name);

        g_all_bitmsk |= (1 << idx);

        if (nfui_nfobject_is_disabled(g_all_chk)) 
        {
            nfui_nfobject_enable(g_all_chk);
        }
    }
    else
    {
        nfui_nfobject_disable(g_rule_chk[idx]);
    }

    return 0;
}

static gint _init_component_data(NFOBJECT *win, gint ch)
{
    VAAID vaaid;
    VCA_COMPONENT_DATA_T *component_data;

    vaaid = vaa_get_pb_vaaid(ch);

    component_data = imalloc(sizeof(VCA_COMPONENT_DATA_T));

    component_data->act_capable = 1;
    component_data->act_license = 1;

    component_data->preview.type = 1;
    component_data->preview.play_mode = 0;
    component_data->preview.ch = ch;
    component_data->preview.onoff = 1;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    

    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 5;
    component_data->disp_rule.delay_cnt = 0;

    nfui_nfobject_set_alloc_data(win, VCA_COMPONENT_DATA, component_data);
    return 0;
}

static gint _set_component_video_fixed(NFOBJECT *top, NFOBJECT *video_fixed)
{
    VCA_COMPONENT_DATA_T *component_data;

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    component_data->video_fixed = video_fixed;

    return 0;
}

static gboolean _draw_vca_rule(gpointer data)
{
    vw_vca_rev_video_component_sync_data(g_video_fixed);
	return TRUE;
}

static gboolean _init_vca_rule(gpointer data)
{
    if (!g_draw_tid) g_draw_tid = g_timeout_add(100, _draw_vca_rule, 0);
    return FALSE;
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_all_chk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gboolean active;

	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
        VAAID vaaid1, vaaid2;
        ITX_VAZONE_CONF conf;
        ITX_VAZONE_SHAPE shape;

        vaaid1 = vaa_get_vaaid(g_cur_ch);
        vaaid2 = vaa_get_pb_vaaid(g_cur_ch);

        active = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        for (i = 0; i < 16; i++)
        {
            if (nfui_nfobject_is_disabled(g_rule_chk[i])) continue;
        
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_rule_chk[i], active);
            nfui_signal_emit(g_rule_chk[i], GDK_EXPOSE, TRUE);

            if (active)
            {           
                vaa_itx_get_zone_conf(vaaid1, i, &conf);
                vaa_itx_get_zone_shape(vaaid1, i, &shape);

                if (conf.type == 0) vaa_itx_add_zone_line_default_template(vaaid2, conf.zoneid);
                else vaa_itx_add_zone_area_default_template(vaaid2, conf.zoneid);

                vaa_itx_set_zone_conf(vaaid2, i, &conf);
                vaa_itx_set_zone_shape(vaaid2, i, &shape);

                g_sel_bitmsk |= (1 << i);
            }
            else
            {
                vaa_itx_get_zone_conf(vaaid2, i, &conf);
                conf.use_zone = 0;
                vaa_itx_set_zone_conf(vaaid2, i, &conf);

                g_sel_bitmsk &= ~(1 << i);
            }
        }        
 	}

	return FALSE;
}

static gboolean post_chk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gboolean active;

	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
        VAAID vaaid1, vaaid2;
        ITX_VAZONE_CONF conf;
        ITX_VAZONE_SHAPE shape;

        for (i = 0; i < 16; i++)
        {
            if (g_rule_chk[i] == obj)
            {
                active = nfui_check_button_get_active((NFCHECKBUTTON*)g_rule_chk[i]);
                break;
            }
        } 

        vaaid1 = vaa_get_vaaid(g_cur_ch);
        vaaid2 = vaa_get_pb_vaaid(g_cur_ch);

        if (active)
        {           
            vaa_itx_get_zone_conf(vaaid1, i, &conf);
            vaa_itx_get_zone_shape(vaaid1, i, &shape);

            if (conf.type == 0) vaa_itx_add_zone_line_default_template(vaaid2, conf.zoneid);
            else vaa_itx_add_zone_area_default_template(vaaid2, conf.zoneid);

            vaa_itx_set_zone_conf(vaaid2, i, &conf);
            vaa_itx_set_zone_shape(vaaid2, i, &shape);

            g_sel_bitmsk |= (1 << i);
        }
        else
        {
            vaa_itx_get_zone_conf(vaaid2, i, &conf);
            conf.use_zone = 0;
            vaa_itx_set_zone_conf(vaaid2, i, &conf);

            g_sel_bitmsk &= ~(1 << i);
        }

        if (g_all_bitmsk == g_sel_bitmsk)
        {
            if (nfui_check_button_get_active((NFCHECKBUTTON*)g_all_chk) == FALSE)
            {
                nfui_check_button_set_active((NFCHECKBUTTON*)g_all_chk, TRUE);
                nfui_signal_emit(g_all_chk, GDK_EXPOSE, TRUE);
            }
        }
        else
        {
            if (nfui_check_button_get_active((NFCHECKBUTTON*)g_all_chk) == TRUE)
            {
                nfui_check_button_set_active((NFCHECKBUTTON*)g_all_chk, FALSE);
                nfui_signal_emit(g_all_chk, GDK_EXPOSE, TRUE);
            }
        }
 	}

	return FALSE;
}


static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		if(top)	nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;
        gint i;
        
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_pb_vaaid(g_cur_ch);

        for (i = 0; i < 16; i++)
        {
            vaa_itx_set_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
            vaa_itx_set_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
        }    
		
        g_copy_mask = 0;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    

        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);
    
        nfui_nfobject_destroy(top);
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

	if (evt->type == GDK_EXPOSE) 
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
	        	
		g_curwnd = 0;		
		gtk_main_quit();
	}

	return FALSE;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//




////////////////////////////////////////////////////////////
//
// public interfaces
//

guint vw_search_by_smart_import_rule_popup_open(NFWINDOW *parent, gint ch, time_t frame_time)
{
	NFOBJECT *win;
	NFOBJECT *main_fixed;	
	NFOBJECT *fixed;
	NFOBJECT *fixedTemp;	
	NFOBJECT *ntb;	
	NFOBJECT *obj;
	NFOBJECT *ok_btn;

    VAAID vaaid;
    VCA_COMPONENT_DATA_T *component_data;
	
	gint i, j;
	gint chk_w, chk_h;

	guint table_width[4];
	guint tot_width;

	guint win_width, win_height;


    table_width[0] = 36;
    table_width[1] = 70;
    table_width[2] = 36;
    table_width[3] = 200;

    tot_width = VIDEO_COMPONENT_WIDTH+60+table_width[0]+table_width[1]+table_width[2]+table_width[3];

    win_width = tot_width+68;
    win_height = 36*(16+1)+160;

	g_copy_mask = 0;
    g_cur_ch = ch;

    g_all_bitmsk = 0;
    g_sel_bitmsk = 0;
    
    memset(g_pre_zone_conf, 0x00, sizeof(ITX_VAZONE_CONF) * 16);
    memset(g_pre_zone_shape, 0x00, sizeof(ITX_VAZONE_SHAPE) * 16);

    vaaid = vaa_get_pb_vaaid(g_cur_ch);

    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
        vaa_itx_get_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
    }    

	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-(tot_width+68))/2, (1080-(36*(16+1)+160))/2, win_width, win_height);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	g_curwnd = win;

    _init_component_data(win, ch);

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, win_width, win_height);
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_regi_post_event_callback(main_fixed, post_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IMPORT RULE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);

// VIDEO FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 30, 60+(win_height-60-65-VIDEO_COMPONENT_HEIGHT)/2);
    g_video_fixed = fixed;

    vw_vca_rev_video_component_open(fixed, 0);
    _set_component_video_fixed(win, fixed);

// RULE
    ntb = (NFOBJECT*)nfui_nftable_new(4, 16+1, 2, 1, table_width, 36);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));    
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb, 30+VIDEO_COMPONENT_WIDTH+60, 68);

	fixedTemp = nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(fixedTemp, 36, 36);
	nfui_nfobject_show(fixedTemp);
	nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, 0);

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(obj, &chk_w, &chk_h);
    nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (36-chk_w)/2, (36-chk_h)/2);
	nfui_regi_post_event_callback(obj, post_all_chk_event_cb);
    g_all_chk = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ID", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));		
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);

    for (i = 0; i < 16; i++)
    {  
        obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    	nfui_nfobject_show(obj);
    	nfui_regi_post_event_callback(obj, post_chk_event_cb);
        g_rule_chk[i] = obj;

    	fixedTemp = nfui_nffixed_new();
    	nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    	nfui_nfobject_set_size(fixedTemp, 36, 36);
    	nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (36-chk_w)/2, (36-chk_h)/2);
    	nfui_nfobject_show(fixedTemp);
    	nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, i+1);

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i+1);
        g_id_obj[i] = obj;

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)ntb, obj, 2, i+1);
        g_color_obj[i] = obj;    

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)ntb, obj, 3, i+1);
        g_name_obj[i] = obj;

        _update_rule_list_object(i);
    }

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, win_width/2-179, win_height-65);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	ok_btn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, win_width/2+5, win_height-65);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);
	
	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(ok_btn, TRUE);

	component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)win, VCA_COMPONENT_DATA);
    component_data->disable_event = 1;	
    component_data->preview.play_from = frame_time;
    component_data->preview.play_to = frame_time + 1000000;

    vw_vca_rev_video_component_sync_preview(g_video_fixed);    
    vw_vca_rev_video_component_sync_data(g_video_fixed);

    g_timeout_add(1000, _init_vca_rule, 0);

	nfui_page_open(PGID_POPUPWND, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_POPUPWND, win);

	return g_copy_mask;
}

