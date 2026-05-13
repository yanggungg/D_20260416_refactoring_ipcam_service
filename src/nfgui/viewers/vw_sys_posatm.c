#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "scm.h"

#include "vw_sys_main.h"
#include "vw_sys_posatm.h"
#include "vw_pos_setting_popup.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)


#define TABLE_LEFT              (28)
#define TABLE_TOP               (42)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_act_combo[GUI_CHANNEL_CNT];
static NFOBJECT *g_type_label[GUI_CHANNEL_CNT];
static NFOBJECT *g_port_label[GUI_CHANNEL_CNT];
static NFOBJECT *g_status_label[GUI_CHANNEL_CNT];
static NFOBJECT *g_edit_btn[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static PosDevData g_org_posdev[GUI_CHANNEL_CNT];
static PosDevData g_posdev[GUI_CHANNEL_CNT];


static gint _is_conflict_port(gint ch)
{
    gint i;

    if (strcmp(g_posdev[ch].port, "NONE") == 0) return 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (i == ch) continue;
        if (strcmp(g_posdev[i].port, "NONE") == 0) continue;

        if (strcmp(g_posdev[i].port, g_posdev[ch].port) == 0) return 1;
    }

    return 0;
}

static gint _display_conflict_port()
{
    gint i, conflict;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        conflict = _is_conflict_port(i);
        
        if (conflict) nfui_nflabel_set_pango_font((NFLABEL*)g_port_label[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_FF0000));
        else nfui_nflabel_set_pango_font((NFLABEL*)g_port_label[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    }

    return 0;
}

static gint _check_conflict_port()
{
    gint i, conflict;
    guint check = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        conflict = _is_conflict_port(i);
        check |= (conflict << i);        
    }

    return check;
}

static gint _update_posdev_table_type(gint row)
{
    if (g_posdev[row].type == 0) 
        nfui_nflabel_set_text((NFLABEL*)g_type_label[row], "SERIAL PORT");
    else 
        nfui_nflabel_set_text((NFLABEL*)g_type_label[row], "NETWORK");

    return 0;
}

static gint _update_posdev_table_port(gint row)
{
    nfui_nflabel_set_text((NFLABEL*)g_port_label[row], g_posdev[row].port);
    
    return 0;
}

static gint _update_posdev_table_status(gint row)
{
    if (strcmp(g_posdev[row].port, "NONE") != 0)
    {
        if (nf_pos_get_port_status(row) == FALSE) 
            nfui_nflabel_set_text((NFLABEL*)g_status_label[row], "DISCONNECTED");
        else 
            nfui_nflabel_set_text((NFLABEL*)g_status_label[row], "CONNECTED");
    }
    else
        nfui_nflabel_set_text((NFLABEL*)g_status_label[row], "-");

    return 0;
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

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
    if (evt->type == INFY_POS_STATUS_NOTIFY)
    {
        gint i;
    
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _update_posdev_table_status(i);
            nfui_signal_emit(g_status_label[i], GDK_EXPOSE, TRUE);
        }   
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_POS_STATUS_NOTIFY);
    }

    return FALSE;
}

static gboolean post_action_all_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint i, enable;
    
        enable = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_act_combo[i], enable);
            nfui_signal_emit(g_act_combo[i], GDK_EXPOSE, TRUE);
            
            g_posdev[i].enable = enable;
        }        
    }

    return FALSE;
}

static gboolean post_action_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint i;

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_act_combo[i] == obj) break;
        }

        if (i == GUI_CHANNEL_CNT) return FALSE;

        g_posdev[i].enable = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
    }

    return FALSE;
}

static gboolean post_pos_editbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;
        PosDevData tmp[GUI_CHANNEL_CNT];

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_edit_btn[i] == obj) break;
        }

        if (i == GUI_CHANNEL_CNT) return FALSE;

        memset(tmp, 0x00, sizeof(PosDevData)*GUI_CHANNEL_CNT);
        g_memmove(tmp, g_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT);
        vw_pos_setting_popup_open(g_curwnd, i, tmp);

        if (memcmp(g_posdev, tmp, sizeof(PosDevData)*GUI_CHANNEL_CNT) == 0) return FALSE;

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((g_posdev[i].type != tmp[i].type) || (strcmp(g_posdev[i].port, tmp[i].port) != 0))
            {
                nfui_nflabel_set_text((NFLABEL*)g_status_label[i], "-");
                nfui_signal_emit(g_status_label[i], GDK_EXPOSE, TRUE);
            }
        }

        g_memmove(g_posdev, tmp, sizeof(PosDevData)*GUI_CHANNEL_CNT);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _update_posdev_table_type(i);
            _update_posdev_table_port(i);
        }   

        _display_conflict_port();

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_signal_emit(g_type_label[i], GDK_EXPOSE, TRUE);        
            nfui_signal_emit(g_type_label[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_status_label[i], GDK_EXPOSE, TRUE);        
        }          
    }
    
    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;
    
        g_memmove(g_posdev, g_org_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _update_posdev_table_type(i);
            _update_posdev_table_port(i);
            _update_posdev_table_status(i);
        }   

        _display_conflict_port();

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_signal_emit(g_type_label[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_status_label[i], GDK_EXPOSE, TRUE);        
            nfui_signal_emit(g_port_label[i], GDK_EXPOSE, TRUE);
        }            
    }
    
    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;

        if (_check_conflict_port())
        {
            nftool_mbox(g_curwnd, "WARNING", "The settings could not be saved due to duplicate ports.\nPlease change the port settings.", NFTOOL_MB_OK);
            return FALSE;
        }
    
        if(memcmp(g_org_posdev, g_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT))
        {   
//          scm_put_log(xxxxxxxxxx, 0, 0);
            g_memmove(g_org_posdev, g_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT);

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                DAL_set_posdev_data(&g_posdev[i], i);
            }

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            VW_SetupSystem_set_changeflag(1);
        }   
        else
        {
            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                _update_posdev_table_status(i);
                nfui_signal_emit(g_status_label[i], GDK_EXPOSE, TRUE);
            }   
        }
    }
    
    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (_check_conflict_port())
        {
            nftool_mbox(g_curwnd, "WARNING", "The settings could not be saved due to duplicate ports.\nPlease change the port settings.", NFTOOL_MB_OK);
            return FALSE;
        }
    
        VW_SysPOSATM_tab_out_handler();
        VW_SetupSystem_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}


void VW_Init_SysPOSATM_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *fixed;    
    NFOBJECT *ntb;
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    guint width[6];
    guint btn_x, btn_y, btn_space;
    
    gint pos_x, pos_y;
    guint i;
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    gint scrolledfixed_h;

    gchar strBuf[32];
    const gchar *strAct[] = {
                    "DISABLE",
                    "ENABLE",
    };

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    g_curwnd = nfui_nfobject_get_top(parent);

    memset(g_posdev, 0x00, sizeof(PosDevData)*GUI_CHANNEL_CNT);
    memset(g_org_posdev, 0x00, sizeof(PosDevData)*GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_posdev_data(&g_posdev[i], i);
    }

    g_memmove(g_org_posdev, g_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT);


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
    nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler); 

    pos_x = TABLE_LEFT;
    pos_y = TABLE_TOP;

    width[0] = 120;
    width[1] = 240;
    width[2] = 240;
    width[3] = 240;
    width[4] = 240;
    width[5] = 120; 

    ntb = nfui_nftable_new(6, 1, 2, 1, width, 40);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put(content_fixed, ntb, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("CH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_combobox_new(strAct, 2, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_2);
    nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);
    nfui_regi_post_event_callback(obj, post_action_all_event_handler); 

    nfui_combobox_set_display_string((NFCOMBOBOX*)obj, "ACTION");

    obj = nfui_nflabel_new_with_pango_font("TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);

    obj = nfui_nflabel_new_with_pango_font("PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);

    obj = nfui_nflabel_new_with_pango_font("STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 4, 0);    
    
    size_w = 0;
    for (i = 0; i < 6; i++) {
        size_w += width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, pos_x, pos_y+40+1);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(6, ROW_CNT_PER_PAGE, 2, 1, width, 40);
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
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
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
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "CH %d", i+1);
    
        obj = nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);

        obj = nfui_combobox_new(strAct, 2, g_posdev[i].enable);
        nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_1);
        nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 1, row_num);
        nfui_regi_post_event_callback(obj, post_action_event_handler);        
        g_act_combo[i] = obj;

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 2, row_num);
        g_type_label[i] = obj;

        _update_posdev_table_type(i);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 3, row_num);
        g_port_label[i] = obj;

        _update_posdev_table_port(i);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);        
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 4, row_num);
        g_status_label[i] = obj;

        _update_posdev_table_status(i);

        obj = nftool_normal_button_create_type3("EDIT", 120);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
        nfui_nfobject_show(obj);    
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 5, row_num);
        nfui_regi_post_event_callback(obj, post_pos_editbutton_event_handler);     
        g_edit_btn[i] = obj;

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

    _display_conflict_port();

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
    
    nfui_regi_post_event_callback(parent, post_page_event_handler);

    uxm_reg_imsg_event(content_fixed, INFY_POS_STATUS_NOTIFY);  
}

gboolean VW_SysPOSATM_tab_out_handler(void)
{
    gint i;
    mb_type ret;

    if(!memcmp(g_org_posdev, g_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT)) return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if(ret == NFTOOL_MB_OK)
    {
//          scm_put_log(xxxxxxxxxx, 0, 0);
        g_memmove(g_org_posdev, g_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            DAL_set_posdev_data(&g_posdev[i], i);
        }

        VW_SetupSystem_set_changeflag(1);       
    }
    else if (ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(g_posdev, g_org_posdev, sizeof(PosDevData)*GUI_CHANNEL_CNT);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _update_posdev_table_type(i);
            _update_posdev_table_port(i);
            _update_posdev_table_status(i);
        }

        _display_conflict_port();
    }

    return FALSE;
}

gboolean VW_SysPOSATM_tab_out_prepare(void)
{
    if (_check_conflict_port())
    {
        nftool_mbox(g_curwnd, "WARNING", "The settings could not be saved due to duplicate ports.\nPlease change the port settings.", NFTOOL_MB_OK);
        return FALSE;
    }

    return TRUE;
}

