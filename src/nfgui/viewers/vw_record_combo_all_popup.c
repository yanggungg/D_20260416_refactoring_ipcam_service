
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfimglabel.h"

#include "../support/nf_ui_page_manager.h"

#include "vw_record_data_internal.h"
#include "vw_record_combo_all_popup.h"
#include "scm.h"
#include "ix_mem.h"


#define RCA_LABEL_H             (40)
#define RCA_GAP_LABEL           (20)

#define RCA_CAM_LABEL_X         (20)
#define RCA_CAM_LABEL_Y         (20)
#define RCA_CAM_LABEL_W         (250)

#define RCA_GAP_CAM_INFO        (10)

#define RCA_INFO_LABEL_X        (RCA_CAM_LABEL_X+RCA_CAM_LABEL_W+RCA_GAP_CAM_INFO)
#define RCA_INFO_LABEL_W        (200)

#define CAP_TYPE                (GUI_CHANNEL_CNT)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *rca_win = NULL;
static NFOBJECT *cap_obj[CAP_TYPE];
static gchar **paramInfo = NULL;

static gboolean rca_ret = FALSE;

static GSList *capList;


static gchar *_get_data(gint type, gint ch)
{
    if (type == TYPE_SIZE)
        return get_record_size_capable_data(ch);
    else
        return get_record_fps_capable_data(ch);
}

static void _sort_info(gint type)
{
    gchar *ch1_data, *ch2_data;
    gint ch1, ch2, ch1_len, ch2_len;
    gint i, j = 0;
    guint mask_cap = 0;
    guint mask_checked = 0xffff;

    for (ch1 = 0; ch1 < GUI_CHANNEL_CNT; ch1++)
    {
        ch1_data = _get_data(type, ch1);
        ch1_len = strlen(ch1_data);

        if (mask_checked & (1 << ch1))
        {
            mask_cap |= (1 << ch1);
            mask_checked &= ~(1 << ch1);

            for(ch2 = ch1+1; ch2 < GUI_CHANNEL_CNT; ch2++)
            {
                ch2_data = _get_data(type, ch2);
                ch2_len = strlen(ch2_data);

                if (ch1_len == ch2_len)
                {
                    for (i = 0; i < ch2_len; i++)
                    {
                        if (type == TYPE_SIZE)
                        {
                            if (ch1_data[i] != ch2_data[i])
                                break;
                        }
                        else
                        {
                            if (strcmp(translate_info_fps_capable(ch1, ch1_data[i]), translate_info_fps_capable(ch2, ch2_data[i])) != 0)
                                break;
                        }
                    }

                    if (i == ch2_len)
                    {
                        mask_cap |= (1 << ch2);
                        mask_checked &= ~(1 << ch2);
                    }
                }
            }

            capList = g_slist_append(capList, GUINT_TO_POINTER(mask_cap));
            mask_cap = 0;
        }
    }
}

static void _set_line_data_name(gchar *name, gint cnt)
{
    g_sprintf(name, "LINE_POS_%02d", cnt);
}

static void _set_capable_size_info(NFOBJECT *obj, gint ch)
{
    gint i, data_len;
    gchar *data;

    data = get_record_size_capable_data(ch);
    
    data_len = strlen(data);
    nfui_nfobject_set_data(obj, "capable len", GINT_TO_POINTER(data_len));

    for (i = 0; i < data_len; i++)
    {
        nfui_combobox_append_data(NF_COMBOBOX(obj), translate_info_size_capable(data[i]));
    }       
}

static void _set_capable_fps_info(NFOBJECT *obj, gint ch)
{
    gint i, data_len;
    gchar *data;

    data = get_record_fps_capable_data(ch);
    
    data_len = strlen(data);
    nfui_nfobject_set_data(obj, "capable len", GINT_TO_POINTER(data_len));

    for (i = 0; i < data_len; i++)
        nfui_combobox_append_data(NF_COMBOBOX(obj), translate_info_fps_capable(ch, data[i]));
}

static gchar *_get_combo_info(NFOBJECT *obj)
{
    gint type , i;
    gchar *info;

    type = GPOINTER_TO_UINT(nfui_nfobject_get_data(rca_win, "type"));
    info = nfui_combobox_get_value(NF_COMBOBOX(obj));

    if (type == TYPE_SIZE)
    {
        for (i = 0; i < CAP_RES_MAX; i++)
        {
            if (strcmp(info, size_info[i]) == 0)
                break;
        }
        
        if (i == CAP_RES_MAX)   
            g_assert(0);

        return size_info[i];
    }
    else
    {
        for (i = 0; i < CAP_FPS_MAX; i++)
        {
            if (strcmp(info, fps_info[i]) == 0)
                break;
        }

        if (i == CAP_FPS_MAX)   
            g_assert(0);

        return fps_info[i];         
    }
}

static void _set_param_info(guint ch, gchar *val)
{
    paramInfo[ch] = val;
}

static void _set_combo_info(NFOBJECT *obj, gchar *data)
{
    gint i, list_cnt;
    guint mask_cap;

    list_cnt = g_slist_length(capList);

    for (i = 0; i < list_cnt; i++)
    {
        if (cap_obj[i] == obj)
            break;
    }

    mask_cap = GPOINTER_TO_UINT(g_slist_nth_data(capList, i));

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (mask_cap & (1 << i))
            _set_param_info(i, data);
    }

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
        g_message("%s, %d, ch:%d, data:%s", __FUNCTION__, __LINE__, i, paramInfo[i]);
}

static gboolean _post_all_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *info;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) { 
        info = _get_combo_info(obj);
        _set_combo_info(obj, info);
    }

    return FALSE;
}

static gboolean _post_ok_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gchar **info = NULL;
    gint list_cnt;
    guint type;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        info = nfui_nfobject_get_data(obj, "ori info");
        
        if(!info) {
            g_warning("%s : ori info is NULL", __FUNCTION__);

            return FALSE;
        }

        if(memcmp(info, paramInfo, sizeof(gchar*) * GUI_CHANNEL_CNT))
            memcpy(info, paramInfo, sizeof(gchar*) * GUI_CHANNEL_CNT);

        if(!rca_ret)
            rca_ret = TRUE;
        
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}


static gboolean _post_cancel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    guint i = 0;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        rca_ret = FALSE;

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}


static gboolean _all_popup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;
    GdkDrawable *drawable;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;        

    if(evt->type == GDK_EXPOSE) {
        guint x, y, src_x, dst_x;
        gint i, list_cnt;
        gchar strTemp[20];

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);
        nfui_nfobject_get_offset(obj, &x, &y);  

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        list_cnt = g_slist_length(capList);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(301));        
        gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);

        for (i = 0; i < list_cnt-1; i++)
        {
            memset(strTemp, 0x00, sizeof(strTemp));
            _set_line_data_name(strTemp, i);

            src_x = x + RCA_CAM_LABEL_X - 10;
            dst_x = x + RCA_INFO_LABEL_X + RCA_INFO_LABEL_W + 10;
            y = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, strTemp));
            gdk_draw_line(drawable, gc, src_x, y, dst_x, y);
        }
        
        nfui_nfobject_gc_unref(gc);
    }
    else if(evt->type == GDK_DELETE) 
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    
        if(paramInfo)
            ifree(paramInfo);

        if (capList)
            g_slist_free(capList);

        paramInfo = NULL;       
        capList = NULL;             
        rca_win = NULL;
        
    }

    return FALSE;
}

static gboolean _all_popup_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE) {
        g_curwnd = 0;
        gtk_main_quit();
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_RecComboAll_Popup_Open(NFWINDOW *parent, gint x, gint y, gchar **info, ALL_TYPE_E type)
{
    NFOBJECT *fixed;
    NFOBJECT *scrolled_fixed;
    NFOBJECT *obj;
    GdkPixbuf *pbCamImage[32];

    gchar strBuf[STRING_SIZE_CAMTITLE];
    gint i, j, ch;
    guint win_w, win_h;
    guint pos_x, pos_y;
    guint src_y, dst_y;
    gint list_cnt;
    guint mask_cap = 0;
    gchar strTemp[20];

    gint scrolledfixed_h;

    if(rca_ret)
        rca_ret = FALSE;

    paramInfo = imalloc(sizeof(gchar*) * GUI_CHANNEL_CNT);  

    if(!paramInfo) {
        g_warning("%s : record combo all info allocation error", __FUNCTION__);
        return FALSE;
    }

    memcpy(paramInfo, info, sizeof(gchar*) * GUI_CHANNEL_CNT);

// LOAD
    pbCamImage[0] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
    pbCamImage[1] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
    pbCamImage[2] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
    pbCamImage[3] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL);
    pbCamImage[4] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);      
    pbCamImage[5] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
    pbCamImage[6] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
    pbCamImage[7] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL);
    pbCamImage[8] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
    pbCamImage[9] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);  
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);         
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL);
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);  
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL); 
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL);
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);      
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL); 
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL);
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);  
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL); 
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL); 

    _sort_info(type);
    list_cnt = g_slist_length(capList);

    win_w = RCA_GAP_LABEL*2+RCA_CAM_LABEL_W+RCA_GAP_CAM_INFO+RCA_INFO_LABEL_W + 80;
    
    if (GUI_CHANNEL_CNT > 16) win_h = RCA_CAM_LABEL_Y+(RCA_LABEL_H+2)*16+2*7+4*(list_cnt-1)+80;
    else win_h = RCA_CAM_LABEL_Y+(RCA_LABEL_H+2)*GUI_CHANNEL_CNT+2*7+4*(list_cnt-1)+80;


// WIN
    rca_win = (NFOBJECT*)nfui_nfwindow_new(parent, 
                (guint)x, (guint)y, (guint)win_w, (guint)win_h);
    g_curwnd = rca_win;
    nfui_nfobject_modify_bg(rca_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_regi_pre_event_callback(rca_win, _all_popup_win_event_handler);

// FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, win_w, win_h);
    nfui_nfobject_show(fixed);
    nfui_regi_pre_event_callback(fixed, _all_popup_fixed_event_handler);
    
    pos_x = RCA_CAM_LABEL_X;
    pos_y = RCA_CAM_LABEL_Y;    

    if (GUI_CHANNEL_CNT > 16) scrolledfixed_h = (RCA_LABEL_H+2)*16;
    else scrolledfixed_h = GUI_CHANNEL_CNT*41;

    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
	nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, scrolledfixed_h/3, 80, scrolledfixed_h/5);
	nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(scrolled_fixed, win_w - 40, scrolledfixed_h);
	nfui_nfobject_show(scrolled_fixed);
	nfui_nffixed_put((NFFIXED*)fixed, scrolled_fixed, pos_x, pos_y);

    pos_x = 0;
    pos_y = 0;    

    for (i = 0; i < list_cnt; i++)
    {
        mask_cap = GPOINTER_TO_UINT(g_slist_nth_data(capList, i));
        src_y = pos_y;

        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (mask_cap & (1 << j))
            {
                ch = j;
                //DAL_get_camera_title(strBuf, j);
                var_get_camtitle(strBuf, j);

                obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[j], strBuf);
                nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
                nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
                nfui_nfobject_support_multi_lang(obj, FALSE);
                nfui_nfobject_set_size(obj, RCA_CAM_LABEL_W, RCA_LABEL_H);
                nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
                nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
                nfui_nfobject_show(obj);
//                nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
                nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, obj, pos_x, pos_y);

                pos_y += RCA_LABEL_H;
                dst_y = pos_y;      
                pos_y += 2;
            }           
        }

        obj = nfui_combobox_new(NULL, 0, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);     
        if (type == TYPE_SIZE)
            _set_capable_size_info(obj, ch);
        else
            _set_capable_fps_info(obj, ch);     
        nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);               
        nfui_nfobject_set_size(obj, RCA_INFO_LABEL_W, RCA_LABEL_H);
        nfui_nfobject_show(obj);
//        nfui_nffixed_put((NFFIXED*)fixed, obj, RCA_INFO_LABEL_X, src_y+(dst_y-src_y-RCA_LABEL_H)/2);
        nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, obj, RCA_CAM_LABEL_W+RCA_GAP_CAM_INFO, src_y+(dst_y-src_y-RCA_LABEL_H)/2);
        nfui_regi_post_event_callback(obj, _post_all_combo_event_handler);
        cap_obj[i] = obj;
        
        pos_y += 2;
        memset(strTemp, 0x00, sizeof(strTemp));
        _set_line_data_name(strTemp, i);
        nfui_nfobject_set_data(fixed, strTemp, GUINT_TO_POINTER(pos_y));
        pos_y += 4;
    }

    pos_x = win_w/2 - (180 + 10);
    pos_y = win_h - 60;

    obj = nftool_normal_button_create_popup_type2("OK", 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_data(obj, "ori info", info);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, _post_ok_event_handler);

    pos_x += (180 + 20);

    obj = nftool_normal_button_create_popup_type2("CANCEL", 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, _post_cancel_event_handler);

    nfui_nfobject_set_data(rca_win, "type", GUINT_TO_POINTER(type));        
    
    nfui_nfwindow_add((NFWINDOW*)rca_win, fixed);
    nfui_run_main_event_handler(rca_win);
    nfui_nfobject_show(rca_win);

    nfui_make_key_hierarchy((NFWINDOW*)rca_win);
    nfui_set_key_focus(obj, TRUE);

    nfui_page_open(PGID_REC_PARAM_COMBO_ALL, rca_win, nfui_get_last_user());

    gtk_main();

    nfui_page_close(PGID_REC_PARAM_COMBO_ALL, rca_win);

    return rca_ret;
}

void VW_RecComboAll_Popup_Close()
{
    if (rca_win)
    {
        rca_ret = FALSE;
        nfui_nfobject_destroy(rca_win);
    }
}

