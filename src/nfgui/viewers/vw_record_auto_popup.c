
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfimglabel.h"

#include "../support/nf_ui_page_manager.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_record_auto_popup.h"
#include "vw_record_combo_all_popup.h"
#include "vw_record_operation.h"

#include "scm.h"
#include "ix_mem.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define ROP_TABLE_ROW_CNT               (GUI_CHANNEL_CNT+1)

#define ROP_LABEL_TITLE_COL_W           (230)
#define ROP_LABEL_AUDIO_COL_W           (173)

#define ROP_TABLE_SIZE_COL_W            (173)
#define ROP_TABLE_FPS_COL_W             (173)
#define ROP_TABLE_QUALITY_COL_W         (173)

#define ROP_LABEL_H                     (40)

#define ROP_CAM_TITLE_POS_X             (26)
#define ROP_CAM_TITLE_POS_Y             (136)

#define REC_BTN_SIZE_W                  (192)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *rop_win = NULL;
static NFOBJECT *dp_obj[2];
static NFOBJECT *size_obj[GUI_CHANNEL_CNT];
static NFOBJECT *fps_obj[GUI_CHANNEL_CNT];
static NFOBJECT *quality_obj[GUI_CHANNEL_CNT];
static NFOBJECT *audio_obj[GUI_CHANNEL_CNT];
static NFOBJECT *pre_obj;
static NFOBJECT *post_obj;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static gchar* (*paramInfo)[PARAM_AUTO_MAX] = NULL;

static gboolean rop_ret = FALSE;


static gchar *_get_param_info(guint ch, guint idx)
{
    return paramInfo[ch][idx];
}

static void _set_param_info(guint ch, guint idx, gchar *val)
{
    paramInfo[ch][idx] = val;
}

static void _limit_capable_fps_info(gint ch)
{
    gint i;
    gchar *data;
    gint data_len;

    nfui_combobox_remove_all(NF_COMBOBOX(fps_obj[ch]));

    data = get_record_fps_capable_data(ch);
    data_len = strlen(data);
    
    for (i = 0; i < data_len; i++)
    {
        if ((strcmp(fps_info[CAP_FPS_30], translate_info_fps_capable(ch, data[i])) != 0)
            && (strcmp(fps_info[CAP_FPS_15], translate_info_fps_capable(ch, data[i])) != 0)
            && (strcmp(fps_info[CAP_FPS_25], translate_info_fps_capable(ch, data[i])) != 0)
            && (strcmp(fps_info[CAP_FPS_12], translate_info_fps_capable(ch, data[i])) != 0))
            nfui_combobox_append_data(fps_obj[ch], translate_info_fps_capable(ch, data[i]));
    }

    nfui_combobox_set_data(NF_COMBOBOX(fps_obj[ch]), _get_param_info(ch, PARAM_FPS_COL));   
}

static void _recover_capable_fps_info(gint ch)
{
    gint i;
    gchar *data;
    gint data_len;

    nfui_combobox_remove_all(NF_COMBOBOX(fps_obj[ch]));

    data = get_record_fps_capable_data(ch);
    data_len = strlen(data);

    for (i = 0; i < data_len; i++)
        nfui_combobox_append_data(fps_obj[ch], translate_info_fps_capable(ch, data[i]));

    nfui_combobox_set_data(NF_COMBOBOX(fps_obj[ch]), _get_param_info(ch, PARAM_FPS_COL));       
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

static gboolean _post_size_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_PRESS) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        NFOBJECT *top;
        gchar **size_data = NULL;
        gchar *info;        
        gint ch, i;
        gboolean ret;
        gint pos_x, pos_y;      

        nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
        top = nfui_nfobject_get_top(obj);
    
        pos_x += top->x + obj->width + 4;
        pos_y += top->y;

        size_data = imalloc(sizeof(gchar*) * GUI_CHANNEL_CNT);

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            info = get_record_size_capable_data(ch);
            size_data[ch] = translate_info_size_capable(info[0]);
        }

        ret = VW_RecComboAll_Popup_Open(g_curwnd, pos_x, pos_y, size_data, TYPE_SIZE);

        if (ret)
        {   
            for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
            {
                info = nfui_combobox_get_value(NF_COMBOBOX(size_obj[ch]));

                if (strcmp(info, size_data[ch]) != 0)
                {
                    for (i = 0; i < CAP_RES_MAX; i++)
                    {
                        if (strcmp(size_data[ch], size_info[i]) == 0)
                            break;
                    }
                
                    _set_param_info(ch, PARAM_AUTO_SIZE, size_info[i]);
                    nfui_combobox_set_data(NF_COMBOBOX(size_obj[ch]), size_info[i]);                    

#if defined(_PTZ_2STREAM_FPS_LIMIT)
                    if ((IS_PTZ(ch)) && (strcmp(size_data[ch], size_info[CAP_RES_640x352]) == 0))
                    {
                        gchar *cur_fps;
                        
                        cur_fps = nfui_combobox_get_value(NF_COMBOBOX(fps_obj[ch]));
                        
                        if (strcmp(cur_fps, fps_info[CAP_FPS_30]) == 0)
                            _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_07]);
                        else if(strcmp(cur_fps, fps_info[CAP_FPS_15]) == 0)
                            _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_07]);
                        else if(strcmp(cur_fps, fps_info[CAP_FPS_25]) == 0)
                            _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_06]);
                        else if(strcmp(cur_fps, fps_info[CAP_FPS_12]) == 0)
                            _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_06]);
                            
                        _limit_capable_fps_info(ch);
                    }
                    else
                    {
                        _recover_capable_fps_info(ch);
                    }
#endif                      
                }
            }               
        }

        ifree(size_data);
    }

    return FALSE;
}

static gboolean _post_size_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint ch, i;
    gchar *info;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) { 
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (size_obj[ch] == obj)
                break;
        }

        if (ch == GUI_CHANNEL_CNT)  
            g_assert(0);

        info = nfui_combobox_get_value(NF_COMBOBOX(obj));

        for (i = 0; i < CAP_RES_MAX; i++)
        {
            if (strcmp(info, size_info[i]) == 0)
                break;
        }

        if (i == CAP_RES_MAX)   
            g_assert(0);

        _set_param_info(ch, PARAM_AUTO_SIZE, size_info[i]);

#if defined(_PTZ_2STREAM_FPS_LIMIT)
        if ((IS_PTZ(ch)) && (strcmp(info, size_info[CAP_RES_640x352]) == 0))
        {
            gchar *cur_fps;
            
            cur_fps = nfui_combobox_get_value(NF_COMBOBOX(fps_obj[ch]));
            
            if (strcmp(cur_fps, fps_info[CAP_FPS_30]) == 0)
                _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_07]);
            else if(strcmp(cur_fps, fps_info[CAP_FPS_15]) == 0)
                _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_07]);
            else if(strcmp(cur_fps, fps_info[CAP_FPS_25]) == 0)
                _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_06]);
            else if(strcmp(cur_fps, fps_info[CAP_FPS_12]) == 0)
                _set_param_info(ch, PARAM_FPS_COL, fps_info[CAP_FPS_06]);
                
            _limit_capable_fps_info(ch);
        }
        else
        {
            _recover_capable_fps_info(ch);
        }
#endif          
    }

    return FALSE;
}

static gboolean _post_fps_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_PRESS) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        NFOBJECT *top;
        gchar **size_data = NULL;       
        gchar **fps_data = NULL;
        gchar *info;        
        gint ch, i;
        gboolean ret;
        gint pos_x, pos_y;      

        nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
        top = nfui_nfobject_get_top(obj);
    
        pos_x += top->x + obj->width + 4;
        pos_y += top->y;

#if defined(_PTZ_2STREAM_FPS_LIMIT)
        size_data = imalloc(sizeof(gchar*) * GUI_CHANNEL_CNT);
        fps_data = imalloc(sizeof(gchar*) * GUI_CHANNEL_CNT);

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
            size_data[ch] = _get_param_info(ch, PARAM_SIZE_COL);

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            info = get_record_fps_capable_data(ch);

            if ((IS_PTZ(ch)) && (strcmp(size_info[CAP_RES_640x352], size_data[ch]) != 0))
                fps_data[ch] = translate_info_fps_capable(ch, info[2]);
            else
                fps_data[ch] = translate_info_fps_capable(ch, info[0]);         
        }

        ret = VW_RecComboAll_Popup_fpsLimit_Open(g_curwnd, pos_x, pos_y, size_data, fps_data);
        ifree(size_data);
#else
        fps_data = imalloc(sizeof(gchar*) * GUI_CHANNEL_CNT);

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            info = get_record_fps_capable_data(ch);
            fps_data[ch] = translate_info_fps_capable(ch, info[0]);
        }

        ret = VW_RecComboAll_Popup_Open(g_curwnd, pos_x, pos_y, fps_data, TYPE_FPS);
#endif

        if (ret)
        {           
            for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
            {
                info = nfui_combobox_get_value(NF_COMBOBOX(fps_obj[ch]));

                if (strcmp(info, fps_data[ch]) != 0)
                {
                    for (i = 0; i < CAP_FPS_MAX; i++)
                    {
                        if (strcmp(fps_data[ch], fps_info[i]) == 0)
                            break;
                    }
                    
                    _set_param_info(ch, PARAM_AUTO_FPS, fps_info[i]);
                    nfui_combobox_set_data(NF_COMBOBOX(fps_obj[ch]), fps_info[i]);                  
                }
            }               
        }

        ifree(fps_data);
    }

    return FALSE;
}

static gboolean _post_fps_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint ch, i;
    gchar *info;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) { 
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (fps_obj[ch] == obj)
                break;
        }

        if (ch == GUI_CHANNEL_CNT)  
            g_assert(0);

        info = nfui_combobox_get_value(NF_COMBOBOX(obj));

        for (i = 0; i < CAP_FPS_MAX; i++)
        {
            if (strcmp(info, fps_info[i]) == 0)
                break;
        }

        if (i == CAP_FPS_MAX)   
            g_assert(0);
        
        _set_param_info(ch, PARAM_AUTO_FPS, fps_info[i]);
    }

    return FALSE;
}

static gboolean _post_quality_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    gint index = -1;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _set_param_info(i, PARAM_AUTO_QUAL, quality_info[index]);
            nfui_combobox_set_data(quality_obj[i], quality_info[index]);            
        }
    }

    return FALSE;
}

static gboolean _post_quality_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    gint index = -1;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (quality_obj[i] == obj)
            {
                index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));          
                _set_param_info(i, PARAM_AUTO_QUAL, quality_info[index]);
                break;
            }
        }
    }

    return FALSE;
}

static gboolean _audio_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    gint index = -1;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _set_param_info(i, PARAM_AUTO_AUDIO, audio_info[index]);
            nfui_combobox_set_data(audio_obj[i], audio_info[index]);            
        }
    }

    return FALSE;
}

static gboolean _audio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    gint index = -1;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (audio_obj[i] == obj)
            {               
                index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
                _set_param_info(i, PARAM_AUTO_AUDIO, audio_info[index]);
                break;
            }
        }
    }

    return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gchar* (*info)[PARAM_AUTO_MAX] = NULL;
    gchar *pTime;
    gchar buf[20];  

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        info = nfui_nfobject_get_data(obj, "ori info");

        if(!info) {
            g_warning("%s : ori info is NULL", __FUNCTION__);

            return FALSE;
        }
        
        if(memcmp(info, paramInfo, sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX))) {
            memcpy(info, paramInfo, sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX));
        }    

        if(!rop_ret)
            rop_ret = TRUE;


        g_sprintf(buf, SECOND_TIME_STRING, VW_RecOperation_get_pre_time());
        pTime = nfui_combobox_get_value(NF_COMBOBOX(pre_obj));
        
        if (strcmp(pTime, buf) != 0)
        {
            gint tmp;           
            tmp = translate_preTime_into_info(pTime);
            if(tmp < 0) 
                g_warning("record_operation_popup.c: preTime is NULL...");
            else
                VW_RecOperation_set_pre_time(tmp);
        }

        g_sprintf(buf, SECOND_TIME_STRING, VW_RecOperation_get_post_time());
        pTime = nfui_combobox_get_value(NF_COMBOBOX(post_obj));
        
        if (strcmp(pTime, buf) != 0)
        {
            gint tmp;           
            tmp = translate_postTime_into_info(pTime);
            if(tmp < 0) 
                g_warning("record_operation_popup.c: postTime is NULL...");
            else
                VW_RecOperation_set_post_time(tmp);
        }
        
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);     
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        rop_ret = FALSE;
    
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean _auto_popup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    if(evt->type == GDK_EXPOSE) {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
        nfui_nfobject_gc_unref(gc);
    }
    else if(evt->type == GDK_DELETE) {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    
        if(paramInfo) {
            ifree(paramInfo);
            paramInfo = NULL;
        }

        rop_win = NULL;
        
    }

    return FALSE;
}

static gboolean _auto_popup_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE) {
        g_curwnd = 0;
        gtk_main_quit();
    }
    return FALSE;
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

#if defined(_PTZ_2STREAM_FPS_LIMIT)
    for (i = 0; i < data_len; i++)
    {
        if ((IS_PTZ(ch)) && (strcmp(size_info[CAP_RES_640x352], _get_param_info(ch, PARAM_SIZE_COL)) == 0))
        {
            if ((strcmp(fps_info[CAP_FPS_30], translate_info_fps_capable(ch, data[i])) != 0)
                && (strcmp(fps_info[CAP_FPS_15], translate_info_fps_capable(ch, data[i])) != 0)
                && (strcmp(fps_info[CAP_FPS_25], translate_info_fps_capable(ch, data[i])) != 0)
                && (strcmp(fps_info[CAP_FPS_12], translate_info_fps_capable(ch, data[i])) != 0))                
        nfui_combobox_append_data(NF_COMBOBOX(obj), translate_info_fps_capable(ch, data[i]));
}
        else    
            nfui_combobox_append_data(NF_COMBOBOX(obj), translate_info_fps_capable(ch, data[i]));
    }
#else
    for (i = 0; i < data_len; i++)
        nfui_combobox_append_data(NF_COMBOBOX(obj), translate_info_fps_capable(ch, data[i]));
#endif
}

static gboolean post_preTime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *pTime;
    gint tmp;
    
    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        pTime = nfui_combobox_get_value(NF_COMBOBOX(obj));

        tmp = translate_preTime_into_info(pTime);
        if(tmp < 0) 
            g_warning("record_operation.c: preTime is NULL...");

        if (tmp > 5) 
            nftool_mbox(g_curwnd, "WARNING", "If you select pre-recording time higher than 5 seconds,\nlow resolutions are applied.", NFTOOL_MB_OK);
    }

    return FALSE;
}


gboolean VW_RecAutoPopup_Open(NFWINDOW *parent, gchar* (*info)[], OPEN_AUTO mode)
{
    NFOBJECT *fixed;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    NFOBJECT *tbl;
    NFOBJECT *oper_tbl, *tbl_tmp;;

    NFOBJECT *obj;

    GdkPixbuf *pbCamImage[32];
    GdkPixbuf *dropdown1_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *dropdown2_img[NFOBJECT_STATE_COUNT];

    guint param_tbl_w[3] = {ROP_TABLE_SIZE_COL_W, ROP_TABLE_FPS_COL_W, ROP_TABLE_QUALITY_COL_W};
    guint oper_tbl_w[2] = {500, 200};
    guint tmp_table_w[2] = {0, };

    gchar *strTitle[] = {"MOTION RECORD", 
                        "ALARM RECORD", 
                        "MOTION/ALARM RECORD"};

    gchar *operLabel[] = {"PRE RECORDING TIME", "POST RECORDING TIME"};
                        
    gchar strBuf[STRING_SIZE_64];
    guint win_width, win_height, table_gap, param_tbl_width;
    guint i, table_col_cnt;
    guint pos_x, pos_y;
    guint size_w, size_h;
    gint add_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


// SUBMENU DATA INIT
    paramInfo = imalloc(sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX)); 

    if(!paramInfo) {
        g_warning("%s : acam param info allocation error", __FUNCTION__);

        return FALSE;
    }

    memcpy(paramInfo, info, sizeof(gchar*) * (GUI_CHANNEL_CNT*PARAM_AUTO_MAX));

    if(rop_ret)
        rop_ret = FALSE;
        
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

    dropdown1_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_01), NULL);
    dropdown1_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_01), NULL);
    dropdown1_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_01), NULL);
    dropdown1_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_01), NULL);

    dropdown2_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_02), NULL);
    dropdown2_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_02), NULL);
    dropdown2_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_02), NULL);
    dropdown2_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_02), NULL);

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    param_tbl_width = (param_tbl_w[0]+param_tbl_w[1]+param_tbl_w[2]+4);
    
    if (GUI_CHANNEL_CNT > 16) {
        add_h = 16;
    }
    else {
        add_h = GUI_CHANNEL_CNT;
    }

    table_gap = 2;      
    win_width = ROP_CAM_TITLE_POS_X;
    win_width += (ROP_LABEL_TITLE_COL_W+table_gap);
    win_width += (param_tbl_width+table_gap);
    win_width += ROP_LABEL_AUDIO_COL_W;
    win_width += ROP_CAM_TITLE_POS_X;
    win_height = ROP_CAM_TITLE_POS_Y + (ROP_LABEL_H+1)*add_h+ 75+ (ROP_LABEL_H+1)*2+ 36+ 44+23;

// WIN
    rop_win = (NFOBJECT*)nfui_nfwindow_new(parent, 
                (guint)((DISPLAY_ACTIVE_WIDTH-win_width)/2), (guint)((DISPLAY_ACTIVE_HEIGHT-win_height)/2), (guint)win_width, (guint)win_height);
    g_curwnd = rop_win;
    nfui_nfwindow_use_double_buffer((NFWINDOW*)rop_win);
    nfui_nfobject_modify_bg(rop_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_regi_pre_event_callback(rop_win, _auto_popup_win_event_handler);   


// FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, win_width, win_height);
    nfui_regi_pre_event_callback(fixed, _auto_popup_fixed_event_handler);   
    nfui_nfobject_show(fixed);

// WIN TITLE LABEL
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[mode], nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 600, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 19);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);
    
    pos_x = ROP_CAM_TITLE_POS_X;
    pos_y = ROP_CAM_TITLE_POS_Y;    
    
    size_w = win_width - 52;

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 60);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, main_page_fixed, pos_x, pos_y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);
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

// CAM TITLE LABEL

    pos_x = 0;
    pos_y = 0;

    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        var_get_camtitle(strBuf, i);

        obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_set_size(obj, ROP_LABEL_TITLE_COL_W, ROP_LABEL_H);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], obj, pos_x, (row_num * 41));

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

    nfui_get_pixbuf_size(dropdown1_img[0], &size_w, &size_h);
    
    pos_x = (ROP_CAM_TITLE_POS_X + ROP_LABEL_TITLE_COL_W + table_gap);
    pos_y = (ROP_CAM_TITLE_POS_Y-ROP_LABEL_H-1);    

    tbl = (NFOBJECT*)nfui_nftable_new(3, 1, 2, 1, param_tbl_w, 40); 
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)fixed, tbl, pos_x, pos_y);

    tmp_table_w[0] = ROP_TABLE_SIZE_COL_W - size_w;
    tmp_table_w[1] = size_w;    
    tbl_tmp = (NFOBJECT*)nfui_nftable_new(2, 1, 0, 0, tmp_table_w, ROP_LABEL_H);    
    nfui_nfobject_show(tbl_tmp);
    nfui_nftable_attach((NFTABLE*)tbl, tbl_tmp, 0, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RESOLUTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(255));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(254));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl_tmp, obj, 0, 0);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown2_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_size_all_event_handler);   
    nfui_nftable_attach((NFTABLE*)tbl_tmp, obj, 1, 0);

    pos_x = ROP_LABEL_TITLE_COL_W + table_gap;
    
    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        size_obj[i] = nfui_combobox_new(NULL, 0, 0);
        _set_capable_size_info(size_obj[i], i);
        nfui_combobox_set_skin_type(NF_COMBOBOX(size_obj[i]), NFCOMBOBOX_TYPE_POPUP_1);
        nfui_combobox_set_align(NF_COMBOBOX(size_obj[i]), NFALIGN_CENTER, 0);
        nfui_combobox_set_data_no_expose(NF_COMBOBOX(size_obj[i]), _get_param_info(i, PARAM_AUTO_SIZE));
        nfui_nfobject_set_size(size_obj[i], ROP_TABLE_SIZE_COL_W, ROP_LABEL_H);
        nfui_regi_post_event_callback(size_obj[i], _post_size_event_handler);
        nfui_nfobject_show(size_obj[i]);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], size_obj[i], pos_x, (row_num * 41));

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

    tmp_table_w[0] = ROP_TABLE_FPS_COL_W - size_w;
    tmp_table_w[1] = size_w;    
    tbl_tmp = (NFOBJECT*)nfui_nftable_new(2, 1, 0, 0, tmp_table_w, ROP_LABEL_H);    
    nfui_nfobject_show(tbl_tmp);
    nfui_nftable_attach((NFTABLE*)tbl, tbl_tmp, 1, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FPS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(255));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(254));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl_tmp, obj, 0, 0);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown2_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_fps_all_event_handler);    
    nfui_nftable_attach((NFTABLE*)tbl_tmp, obj, 1, 0);

    pos_x += (ROP_TABLE_SIZE_COL_W + table_gap);

    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        fps_obj[i] = nfui_combobox_new(NULL, 0, 0);
        _set_capable_fps_info(fps_obj[i], i);
        nfui_combobox_set_skin_type(NF_COMBOBOX(fps_obj[i]), NFCOMBOBOX_TYPE_POPUP_1);
        nfui_combobox_set_align(NF_COMBOBOX(fps_obj[i]), NFALIGN_CENTER, 0);                
        nfui_combobox_set_data_no_expose(NF_COMBOBOX(fps_obj[i]), _get_param_info(i, PARAM_AUTO_FPS));
        nfui_nfobject_set_size(fps_obj[i], ROP_TABLE_SIZE_COL_W, ROP_LABEL_H);
        nfui_regi_post_event_callback(fps_obj[i], _post_fps_event_handler);
        nfui_nfobject_show(fps_obj[i]);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], fps_obj[i], pos_x, (row_num * 41));

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }
    
    obj = nfui_combobox_new(quality_info, 5, 0);
    nfui_combobox_set_display_string(NF_COMBOBOX(obj), "QUALITY");  
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, _post_quality_all_event_handler);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 2, 0);

    pos_x += (ROP_TABLE_FPS_COL_W + table_gap);
    
    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        quality_obj[i] = nfui_combobox_new(quality_info, 5, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(quality_obj[i]), NFCOMBOBOX_TYPE_POPUP_1);
        nfui_combobox_set_align(NF_COMBOBOX(quality_obj[i]), NFALIGN_CENTER, 0);                
        nfui_combobox_set_data_no_expose(NF_COMBOBOX(quality_obj[i]), _get_param_info(i, PARAM_AUTO_QUAL));
        nfui_nfobject_set_size(quality_obj[i], ROP_TABLE_SIZE_COL_W, ROP_LABEL_H);
        nfui_regi_post_event_callback(quality_obj[i], _post_quality_event_handler);
        nfui_nfobject_show(quality_obj[i]);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], quality_obj[i], pos_x, (row_num * 41));

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

// AUDIO
    pos_x = ROP_CAM_TITLE_POS_X + ROP_LABEL_TITLE_COL_W + param_tbl_width + 4;
    pos_y = (ROP_CAM_TITLE_POS_Y-ROP_LABEL_H-1);    

    obj = nfui_combobox_new(audio_info, 2, 0);
    nfui_combobox_set_display_string(NF_COMBOBOX(obj), "AUDIO");
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_2);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, ROP_LABEL_AUDIO_COL_W, ROP_LABEL_H);
    nfui_regi_post_event_callback(obj, _audio_all_event_handler);
    
    if (!DAL_get_support_audio()) nfui_nfobject_disable(obj);

    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    
    pos_x = ROP_LABEL_TITLE_COL_W + param_tbl_width + 4;
    pos_y = 0;

    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        audio_obj[i] = nfui_combobox_new(audio_info, 2, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(audio_obj[i]), NFCOMBOBOX_TYPE_POPUP_1);
        nfui_combobox_set_align(NF_COMBOBOX(audio_obj[i]), NFALIGN_CENTER, 0);      
        nfui_nfobject_set_size(audio_obj[i], ROP_LABEL_AUDIO_COL_W, ROP_LABEL_H);
        nfui_combobox_set_data_no_expose(NF_COMBOBOX(audio_obj[i]), _get_param_info(i, PARAM_AUTO_AUDIO));
        nfui_regi_post_event_callback(audio_obj[i], _audio_event_handler);  
        
        if (!DAL_get_support_audio()) nfui_nfobject_disable(audio_obj[i]);

        nfui_nfobject_show(audio_obj[i]);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], audio_obj[i], pos_x, (row_num * 41));

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }


// OPER TABLE
    pos_x = ROP_CAM_TITLE_POS_X;
    pos_y = ROP_CAM_TITLE_POS_Y + main_page_fixed->height + 15;

    oper_tbl = (NFOBJECT*)nfui_nftable_new(2, 2, 2, 2, oper_tbl_w, 40); 
    nfui_nfobject_show(oper_tbl);
    nfui_nffixed_put((NFFIXED*)fixed, oper_tbl, pos_x, pos_y);

    for (i = 0; i < 2; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(operLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)oper_tbl, obj, 0, i);
    }

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, SECOND_TIME_STRING, VW_RecOperation_get_pre_time());
    pre_obj = nfui_combobox_new(recPreTime, REC_PRE_TIME_MAX, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(pre_obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_data_no_expose(NF_COMBOBOX(pre_obj), strBuf);
    nfui_combobox_set_align(NF_COMBOBOX(pre_obj), NFALIGN_CENTER, 0);       
    nfui_nfobject_show(pre_obj);
    nfui_regi_post_event_callback(pre_obj, post_preTime_event_handler);
    nfui_nftable_attach((NFTABLE*)oper_tbl, pre_obj, 1, 0);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, SECOND_TIME_STRING, VW_RecOperation_get_post_time());
    post_obj = nfui_combobox_new(recPostTime, REC_POST_TIME_MAX, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(post_obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_data_no_expose(NF_COMBOBOX(post_obj), strBuf);
    nfui_combobox_set_align(NF_COMBOBOX(post_obj), NFALIGN_CENTER, 0);      
    nfui_nfobject_show(post_obj);
    nfui_nftable_attach((NFTABLE*)oper_tbl, post_obj, 1, 1);


// BUTTON
    pos_x = (win_width - REC_BTN_SIZE_W - 20);
    pos_y += (ROP_LABEL_H+1)*2 + 36;

    obj = nftool_normal_button_create_type2("CANCEL", REC_BTN_SIZE_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    pos_x -= (REC_BTN_SIZE_W + 10);

    obj = nftool_normal_button_create_type1("OK", REC_BTN_SIZE_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_data(obj, "ori info", info);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_okbutton_event_handler);
 
    nfui_nfwindow_add((NFWINDOW*)rop_win, fixed);
    nfui_run_main_event_handler(rop_win);
    nfui_nfobject_show(rop_win);

    nfui_make_key_hierarchy(rop_win);
    nfui_set_key_focus(obj, TRUE);
    nfui_page_open(PGID_REC_AUTO_CONFIG, rop_win, nfui_get_last_user());

    gtk_main();

    nfui_page_close(PGID_REC_AUTO_CONFIG, rop_win);
    
    return rop_ret;

}

void VW_RecAutoPopup_Close()
{
    if (rop_win)
    {
        rop_ret = FALSE;
        nfui_nfobject_destroy(rop_win);
    }
}

