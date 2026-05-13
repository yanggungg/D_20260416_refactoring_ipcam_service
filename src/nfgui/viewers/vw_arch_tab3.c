#include <string.h>
#include "nf_afx.h"

#include "nf_ptz.h"
#include "nf_api_live.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/acp.h"
#include "modules/amd.h"
#include "modules/mda.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"

#include "uxm.h"
#include "scm.h"
#include "smt.h"
#include "dtf.h"
#include "iux_msg.h"

#include "vw_archiving.h"
#include "vw_arch_tab3.h"
#include "vw_arch_verify.h"
#include "vw_arch_playback_memo_avi.h"
#include "vw.h"

#include "nf_api_disk.h"

// #include "../modules/mda.h"

#define ART3_LIST_MAX                   (17)

// STORAGE
#define ART3_STORAGE_LABEL_X        (137)
#define ART3_STORAGE_LABEL_Y        (0)
#define ART3_STORAGE_LABEL_W        (256)
#define ART3_STORAGE_LABEL_H        (40)

#define ART3_STORAGE_COMBO_X        (ART3_STORAGE_LABEL_X + ART3_STORAGE_LABEL_W + 10)
#define ART3_STORAGE_COMBO_Y        (ART3_STORAGE_LABEL_Y)
#define ART3_STORAGE_COMBO_W        (310)
#define ART3_STORAGE_COMBO_H        (ART3_STORAGE_LABEL_H)

#define ART3_SEARCH_BUTTON_SIZE     (166)
#define ART3_SEARCH_BUTTON_X        (ART3_STORAGE_COMBO_X + ART3_STORAGE_COMBO_W + 6)
#define ART3_SEARCH_BUTTON_Y        (ART3_STORAGE_COMBO_Y)

// LIST LABEL 
#define ART3_LIST_LABEL_H               (38)
#define ART3_LIST_LABEL_H_INTERVAL      (42)
#define ART3_LIST_START_X               (ART3_STORAGE_LABEL_X)
#define ART3_LIST_START_Y               (61)

// PREV, NEXT BUTTON
#define ART3_PAGE_BTN_SIZE              (60)

#define ART3_PREV_BTN_X                 (781)
#define ART3_PREV_BTN_Y                 (ART3_LIST_START_Y + ART3_LIST_LABEL_H_INTERVAL*(ART3_LIST_MAX+1) + 10)

#define ART3_PAGE_LABEL_X               (ART3_PREV_BTN_X + ART3_PAGE_BTN_SIZE)
#define ART3_PAGE_LABEL_Y               (ART3_PREV_BTN_Y)
#define ART3_PAGE_LABEL_W               (120)
#define ART3_PAGE_LABEL_H               (40)

#define ART3_NEXT_BTN_X                 (ART3_PAGE_LABEL_X + ART3_PAGE_LABEL_W)
#define ART3_NEXT_BTN_Y                 (ART3_PAGE_LABEL_Y)

// VERIFY BTN
#define ART3_VERIFY_BTN_SIZE            (226)
#define ART3_VERIFY_BTN_X               (ART3_NEXT_BTN_X + ART3_PAGE_BTN_SIZE + 380)
#define ART3_VERIFY_BTN_Y               (ART3_NEXT_BTN_Y)

#define NORMAL_OUTLINE_COLOR        0
#define FOCUS_OUTLINE_COLOR         146
#define SELECT_OUTLINE_COLOR        147


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *dev_obj = NULL;            // STORAGE COMBOBOX
static NFOBJECT *listlbl_obj[ART3_LIST_MAX][5];
static NFOBJECT *memobtns_obj[ART3_LIST_MAX];       // DELETE ITEM BUTTONS
static NFOBJECT *pagelbl_obj2;
static NFOBJECT *g_content_fixed;

static MEDIA_INFO_T *media_info = NULL;
static AFILE_INFO_T *art3_avi_info = NULL;

static gint art3_current_page;
static gint art3_total_page;
static gint art3_avi_counts;

static gint focused_row = -1;
static gint selected_row = -1;

static ACPCTX cur_acp = INVALID;
static NFOBJECT     *wait_pop = NULL;


static void _change_outLine_list(gint row, gint color)
{
    static GdkGC *gc;
    static GdkDrawable *drawable;
    gint i, obj_x, obj_y, obj_w, obj_h;
    gint outline_w=0, outline_h=0;

    drawable = nfui_nfobject_get_window((NFOBJECT*)listlbl_obj[row][0]);
    gc = nfui_nfobject_get_gc(listlbl_obj[row][0]);

    gdk_gc_set_line_attributes(gc,
            2,
            GDK_LINE_SOLID,
            GDK_CAP_NOT_LAST,
            GDK_JOIN_MITER);

    for (i = 0; i < 5; i++)
    {
        nfui_nfobject_get_size(listlbl_obj[row][i], &obj_w, &obj_h);
        outline_w += (obj_w + 2);
    }

    outline_h = obj_h+2;
                
    nfui_nfobject_get_offset(listlbl_obj[row][0], &obj_x, &obj_y);
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));  
    gdk_draw_rectangle (drawable, gc, FALSE, obj_x-1, obj_y-1, outline_w, outline_h);

    nfui_nfobject_gc_unref(gc);
}

static gint _draw_selectLine(gint row)
{
    if (selected_row == row)
        return -1;
        
        if (selected_row != -1)
            _change_outLine_list(selected_row, NORMAL_OUTLINE_COLOR);

        _change_outLine_list(row, SELECT_OUTLINE_COLOR);
        selected_row = row;

    return 0;
}

static gint _erase_selectLine()
{
    if (selected_row == -1)
        return -1;

    _change_outLine_list(selected_row, NORMAL_OUTLINE_COLOR);
    selected_row = -1;

    return 0;
}

static gint _draw_focusLine(gint row)
{
    if (focused_row == row)
        return -1;

// leave old focus row
    if ((selected_row != -1) && (selected_row == focused_row))
        _change_outLine_list(selected_row, SELECT_OUTLINE_COLOR);
    else if (focused_row != -1)
        _change_outLine_list(focused_row, NORMAL_OUTLINE_COLOR);
    
// enter new focus row
    _change_outLine_list(row, FOCUS_OUTLINE_COLOR);
    focused_row = row;

    return 0;
}

static gint _erase_focusLine()
{
    if (focused_row == -1)
        return -1;

    if (focused_row == selected_row)
        _change_outLine_list(focused_row, SELECT_OUTLINE_COLOR);
    else
        _change_outLine_list(focused_row, NORMAL_OUTLINE_COLOR);
    
    focused_row = -1;

    return 0;
}

static gint _find_index_matched_listOBJ(NFOBJECT *obj)
{
    gint i;

    for(i = 0; i < ART3_LIST_MAX; i++)
    {
        if ((listlbl_obj[i][0] == obj)
            || (listlbl_obj[i][1] == obj)
            || (listlbl_obj[i][2] == obj)
            || (listlbl_obj[i][3] == obj)
            || (listlbl_obj[i][4] == obj))
        {
            break;
        }           
    }

    if (i == ART3_LIST_MAX)
    {
        g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
        g_assert(0);
    }

    return i;
}

static gint _find_column_matched_listOBJ(NFOBJECT *obj, gint row)
{
    gint i;

    for(i = 0; i < 5; i++)
    {
        if (listlbl_obj[row][i] == obj)
            break;
    }

    if (i == 5)
    {
        g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
        g_assert(0);
    }

    return i;
}

static int _update_dev_list()
{
    int cnt;
    int dev_cnt = 0;
    int i;
    MEDIA_TYPE_E mtype;

    if (media_info) {
        amd_cleanup();
        scm_free_media_list(media_info);
        nfui_combobox_remove_all(dev_obj);
    }

    media_info = scm_new_media_list(&cnt);
    if (!media_info) return -1;
    amd_init(media_info);

    for (i = 0; i < cnt; ++i) {
        mtype = scm_get_media_type(media_info[i].id);
        if (mtype == MTYPE_USB || mtype == MTYPE_ODD) {
            nfui_combobox_append_data(dev_obj, media_info[i].title);
            ++dev_cnt;
        }
    }
    if (dev_cnt == 0) nfui_combobox_append_data(dev_obj, "NO DEVICE");

    nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static int _set_acp_by_cur()
{
    MEDIA_ID mid;
    char *pdata = nfui_combobox_get_value(dev_obj);
    if (!pdata) return -1;
    cur_acp = amd_get_acp(pdata);
    return 0;
}

static int _update_dev_info()
{
    cur_acp = INVALID;
    _update_dev_list();
//  _set_acp_by_cur();
    return 0;
}


static int _clear_avi_list()
{
    int i, j;
    for (i = 0; i < ART3_LIST_MAX; i++) {
        for (j = 0 ; j < 5 ; j++) {
            nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][j], "");
            nfui_signal_emit((NFLABEL*)listlbl_obj[i][j], GDK_EXPOSE, TRUE);
        }
    }

    for (i = 0; i < ART3_LIST_MAX; i++) 
    {
        nfui_nfobject_disable(memobtns_obj[i]);
        nfui_signal_emit(memobtns_obj[i], GDK_EXPOSE, FALSE);
    }

    nfui_nflabel_set_text((NFLABEL*)pagelbl_obj2, "0/0");
    nfui_signal_emit(pagelbl_obj2, GDK_EXPOSE, FALSE);
    return  0;
}

static void arch_tab3_avi_load_page(gint current_page, gint total_page) 
{
    gchar page_buf[40];
    gchar buf[5][1024];
    gchar ampm[4][10];
    gint i, j, k;

    char str[1025];
    char file_name_buf[1024];
    char *ptr;
    char *delim = "/";
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

    AFILEID start;
    AFILEID end;

    memset(page_buf, 0x00, sizeof(page_buf));
    g_sprintf(page_buf, "%d/%d", current_page, total_page); 
    nfui_nflabel_set_text((NFLABEL*)pagelbl_obj2, page_buf);
    
    gint start_row = (current_page-1) * ART3_LIST_MAX;

    // labels's initialization and hide del button
    for(i = 0; i < ART3_LIST_MAX; i++) {
        for(j = 0; j < 5; j++) {
            nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][j], "");
            nfui_signal_emit((NFLABEL*)listlbl_obj[i][j], GDK_EXPOSE, TRUE);
        }
    }

    for(i = 0; i < ART3_LIST_MAX; i++) nfui_nfobject_disable(memobtns_obj[i]);

    start = amd_find_start_afile(cur_acp);
    end = amd_find_end_afile(cur_acp);

    for(i = 0, j=0 ; i < end - start + 1 ; i++, j++) {
        memset(buf, 0x00, sizeof(buf));
        memset(ampm, 0x00, sizeof(ampm));

        memset(file_name_buf, 0x00, sizeof(file_name_buf));
        g_sprintf(file_name_buf, "%s", art3_avi_info[i].full_name);

        ptr = strtok_r(file_name_buf, "/", &pnext);
        pbuf = pnext;
        
        while( ptr = strtok_r(pbuf, "/", &pnext) ) {
            memset(buf[0], 0x00, sizeof(buf[0]));
            g_sprintf(buf[0], "%s", ptr);
            pbuf = pnext;
        }

        ifn_convert_storage_size(buf[1], art3_avi_info[i].file_size);
        sprintf(buf[2], "%s", art3_avi_info[i].user);

        if(art3_avi_info[i].movie_start != 0)
            dtf_get_local_datetime(art3_avi_info[i].movie_start, buf[3]);

        if(art3_avi_info[i].movie_end != 0)
            dtf_get_local_datetime(art3_avi_info[i].movie_end, buf[4]);

        for(k = 0 ; k < 5 ; k++)
            nfui_nflabel_set_text((NFLABEL*)listlbl_obj[j][k], buf[k]);

        nfui_nfobject_enable(memobtns_obj[j]);
    }

    nfui_signal_emit(g_content_fixed, GDK_EXPOSE, TRUE);
}

static int _play_afile(unsigned int chmask, gboolean is_mul)
{
    time_t offset;

    if (selected_row == -1) return -1;

    offset = art3_avi_info[selected_row].movie_start - ARCH_PLAY_TIME;

    // temporarily
    // due to yesing (JM39X)
    nf_set_using_usb(TRUE);

    vw_archiving_start_playback(OPEN_BY_ARCH_PLAY);

    if (is_mul) vsm_archived_play_start_ex(chmask, offset, ARCH_PLAY_MUL);
    else vsm_archived_play_start_ex(chmask, offset, ARCH_PLAY_AVI); 

    return 0;
}

static int _update_info()
{
    _update_dev_info();
    if (cur_acp)
        art3_total_page = (gint)ceil((gdouble)amd_get_afile_count(cur_acp) / ART3_LIST_MAX);

    return 0;
}

static int _get_arch_play_channel(char *file_name)
{
    char *p;
    char ch[3] = { 0, 0, 0};
    p = strrchr(file_name, '/') + 3;
    strncpy(ch, p, 2);

    return atoi(ch) - 1;
}

static gboolean _is_multi_file(gchar *filename)
{
    if(strstr(filename, ".mul")) return TRUE;
    return FALSE;
}

static int _play()
{
    BITMASK chmask = 0;
    gboolean multi = 0;
    int ch;
    AFILEID id;


    if (selected_row == -1) return -1; 
    if (art3_avi_counts < 1) return -2;
    if (!ssm_check_access_auth(USR_AUTH_SEARCH))    return -3;
    if (nf_avi_player_check_file(art3_avi_info[selected_row].full_name, 0) != 1) return -4;
    
    multi = _is_multi_file(art3_avi_info[selected_row].full_name);

    if (multi)
        chmask = art3_avi_info[selected_row].ch_mask;
    else 
    {
        ch = _get_arch_play_channel(art3_avi_info[selected_row].full_name);     
        chmask = (1 << ch);
    }

    id = amd_get_selected_afile(cur_acp, selected_row);
    amd_set_play_afile(cur_acp, id);

    _play_afile(chmask, multi);


    return 0;
}

static void _remove_waitbox(void)
{
    if(wait_pop)
    {
        nftool_remove_waitbox(wait_pop);
        wait_pop = NULL;
    }
}

static gboolean _delayed_play(void *data)
{
    int ret;
    if ((ret = _play()) < 0) {
        _remove_waitbox();
        NFUTIL_THREADS_ENTER();
        switch (ret) {
        case -1:
        case -2:
        case -4:
            vw_mbox(g_curwnd, "ERROR", IMBX_INVALID_AVIFILE, NFTOOL_MB_OK);
            break;
        case -3:
            vw_mbox(g_curwnd, "ERROR", IMBX_NO_AUTH, NFTOOL_MB_OK);
            break;
        }
        NFUTIL_THREADS_LEAVE();
    }
    return FALSE;
}

static void _run_delayed_play(void)
{
    wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
    g_timeout_add(10, _delayed_play, 0);
}

static gboolean arch_tab3_post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
    case GDK_EXPOSE:
        if (selected_row != -1)
            _change_outLine_list(selected_row, SELECT_OUTLINE_COLOR);           
        break;

    case INFY_MEDIA_STATUS_CHANGED:
        _clear_avi_list();
        _update_info();
        break;


    case INFY_PLAYBACK_STARTED:
        _remove_waitbox();
        break;
        

    case IRET_OPEN_ARCH_MANAGER:
        {
        }
        break;
        
    case GDK_DELETE:
        {
            scm_close_avi_play_manager();
            uxm_unreg_imsg_event(obj, INFY_PLAYBACK_STARTED);   
            uxm_unreg_imsg_event(obj, INFY_MEDIA_STATUS_CHANGED);   
            uxm_unreg_imsg_event(obj, IRET_OPEN_ARCH_MANAGER);
            if (art3_avi_info) {
                amd_free_afile_list(art3_avi_info);
                art3_avi_info = NULL;
            }

            // SKSHIN
            if (media_info) {
                amd_cleanup();
                scm_free_media_list(media_info);
                media_info = 0;
            }

            cur_acp = INVALID;
        }
        break;

    default :
        break;
    }

    return FALSE;
}

static gboolean art3_post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        _set_acp_by_cur();
        _clear_avi_list();
    }

    return FALSE;
}

static gboolean art3_prev_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    AFILEID start;
    AFILEID end;
    int k;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;
        
        if(art3_current_page > 1)
        {
            art3_current_page--;

            if (!cur_acp) return FALSE;
            if (amd_move_to_prev_page(cur_acp) < 0) return FALSE;
            
            if (art3_avi_info) {
                amd_free_afile_list(art3_avi_info);
                art3_avi_info = NULL;
            }

            _erase_selectLine();
            start = amd_find_start_afile(cur_acp);
            end = amd_find_end_afile(cur_acp);
            art3_avi_counts = amd_get_afile_count(cur_acp);
            if (art3_avi_counts == 0) return FALSE;
            art3_avi_info = amd_new_afile_list(cur_acp, start, end);
            arch_tab3_avi_load_page(art3_current_page, art3_total_page);
        }
    }
    return FALSE;
}

static gboolean
art3_next_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    AFILEID start;
    AFILEID end;
    int k;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;
    
        if(art3_current_page < art3_total_page)
        {
            art3_current_page++;

            if (!cur_acp) return FALSE;
            if (amd_move_to_next_page(cur_acp) < 0) return FALSE;
            
            if (art3_avi_info) {
                amd_free_afile_list(art3_avi_info);
                art3_avi_info = NULL;
            }

            _erase_selectLine();
            start = amd_find_start_afile(cur_acp);
            end = amd_find_end_afile(cur_acp);
            art3_avi_counts = amd_get_afile_count(cur_acp);
            if (art3_avi_counts == 0) return FALSE;
            art3_avi_info = amd_new_afile_list(cur_acp, start, end);
            arch_tab3_avi_load_page(art3_current_page, art3_total_page);
        }

    }
    return FALSE;
}

static gboolean _proc_media_search(void *data)
{
    AFILEID start;
    AFILEID end;
    gint i;

    _set_acp_by_cur();
    if (cur_acp)
        art3_total_page = (gint)ceil((gdouble)amd_get_afile_count(cur_acp) / ART3_LIST_MAX);
    else {
        _remove_waitbox();
        return FALSE;
    }


//      if (!cur_acp) return FALSE;
    
    if (art3_avi_info) {
        amd_free_afile_list(art3_avi_info);
        art3_avi_info = NULL;
    }

    _erase_selectLine();
    start = amd_find_start_afile(cur_acp);
    end = amd_find_end_afile(cur_acp);
    art3_avi_counts = amd_get_afile_count(cur_acp);
    if (art3_avi_counts == 0) {
        _remove_waitbox();
        nfui_nflabel_set_text((NFLABEL*)pagelbl_obj2, "0/0");
        nfui_signal_emit(pagelbl_obj2, GDK_EXPOSE, FALSE);
        return FALSE;
    }
    art3_avi_info = amd_new_afile_list(cur_acp, start, end);
    art3_current_page = (start / ART3_LIST_MAX) + 1;
    arch_tab3_avi_load_page(art3_current_page, art3_total_page);

    _remove_waitbox();
    return FALSE;
}

static gboolean
art3_search_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
        g_timeout_add(10, _proc_media_search, 0);
                
    }
    return FALSE;
}

static gboolean art3_verify_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    AFILEID id;
    gint strLen;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        if (selected_row == -1) {
            nftool_mbox(g_curwnd, "WARNING", "Firstly, you should select a file in the list." , NFTOOL_MB_OK);
            return FALSE;   
        }

        if (art3_avi_counts < 1) return FALSE;

        strLen = nfui_nflabel_get_strlen(listlbl_obj[selected_row][0]);
        if(strLen == 0) return FALSE;
        if (!cur_acp) return FALSE;
        
        id = amd_get_selected_afile(cur_acp, selected_row);
        ssm_stop_auto_logout();
        VW_ArchVerify_Open(g_curwnd, cur_acp, id);
        ssm_start_auto_logout();
    }
    
    return FALSE;
}

static gboolean art3_playback_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint strLen;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        if (selected_row == -1) {
            nftool_mbox(g_curwnd, "WARNING", "Firstly, you should select a file in the list." , NFTOOL_MB_OK);
            return FALSE;   
        }

        strLen = nfui_nflabel_get_strlen(listlbl_obj[selected_row][0]);
        if(strLen == 0) return FALSE;

        _run_delayed_play();
/*      if (_play() == -2) {
            vw_mbox(g_curwnd, "ERROR", IMBX_INVALID_AVIFILE, NFTOOL_MB_OK);
        }*/
    }
    
    return FALSE;
}

static gboolean art3_close_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
        VW_Archiving_Close();
    
    return FALSE;
}

static gboolean pre_art3_listlbl_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gint row, col;
    NFOBJECT* cur_focus;

    if (obj->kfocus == NFOBJECT_FOCUS)
    {
        if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
        {
            kevt = (GdkEventKey*)evt;
            kpid = (KEYPAD_KID)kevt->keyval;

            row = _find_index_matched_listOBJ(obj);
            col = _find_column_matched_listOBJ(obj, row);

            if ((kpid == KEYPAD_LEFT) && (col != 0))
            {
                cur_focus = nfui_get_cur_focus(obj);
                if (!cur_focus) return FALSE;                       
                nfui_set_key_focus(cur_focus, FALSE);
                nfui_set_key_focus(listlbl_obj[row][0], TRUE);
                nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
                nfui_signal_emit(listlbl_obj[row][0], GDK_EXPOSE, TRUE);            
            }
            else if ((kpid == KEYPAD_RIGHT) && (col != 4))
            {
                cur_focus = nfui_get_cur_focus(obj);
                if (!cur_focus) return FALSE;                       
                nfui_set_key_focus(cur_focus, FALSE);
                nfui_set_key_focus(listlbl_obj[row][4], TRUE);
                nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
                nfui_signal_emit(listlbl_obj[row][4], GDK_EXPOSE, TRUE);            
            }
        }
    }

    return FALSE;
}

static gboolean post_art3_listlbl_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gint result;

    if (evt->type == GDK_EXPOSE)
    {   
        i = _find_index_matched_listOBJ(obj);
        if (obj->kfocus == NFOBJECT_FOCUS)
            _draw_focusLine(i); 
        else
            _erase_focusLine(i);
    }
    
    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        gint strLen;
    
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
        i = _find_index_matched_listOBJ(obj);
        result = _draw_selectLine(i);

        strLen = nfui_nflabel_get_strlen(listlbl_obj[selected_row][0]);
        if(strLen == 0) return FALSE;

        if ((evt->type == GDK_2BUTTON_PRESS) || ((kpid == KEYPAD_ENTER) && (result == -1))) {

            _run_delayed_play();
/*          if (_play() == -2) {
                vw_mbox(g_curwnd, "ERROR", IMBX_INVALID_AVIFILE, NFTOOL_MB_OK);
            }*/
        }
    }
    else if(evt->type == GDK_ENTER_NOTIFY)
    {
        NFOBJECT* cur_focus;
    
        cur_focus = nfui_get_cur_focus(obj);
        if (!cur_focus) return FALSE;               
        nfui_set_key_focus(cur_focus, FALSE);
        nfui_set_key_focus(obj, TRUE);
        nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
        
    return FALSE;   
}

static gboolean art3_memo_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, sel_item;
    AFILEID id;
    gint strLen;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;
        
        for(i = 0; i < ART3_LIST_MAX; i++)
        {
            if(memobtns_obj[i] == obj)
                break;
        }

        if(i == ART3_LIST_MAX) return FALSE;

        sel_item = i;

        strLen = nfui_nflabel_get_strlen(listlbl_obj[sel_item][0]);
        
        if(strLen == 0) return FALSE;
        if (!cur_acp) return FALSE;

        id = amd_get_selected_afile(cur_acp, sel_item);
        VW_Arch_Playback_Memo_Avi_Open(g_curwnd, cur_acp, id);
    }
    
    return FALSE;
    
}

gboolean _update_dev(void *data)
{
    _update_info();
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

///////////////////////////////////////////////////////////////////////
//
//
//

void vw_init_arch_tab3_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    int i, j;
    guint size_w, size_h;
    gint xpos = ART3_LIST_START_X;
    gint ypos = ART3_LIST_START_Y;

    const gchar *listTitlelbl[] = {"NAME", "SIZE", "SET BY", "START", "END"};
    gint list_xpos[5] = {137, 781, 921, 1083, 1380};
    gint list_width[5] = {642, 138, 160, 295, 295};

    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    const gchar *page_btn[] = {"PREV.", "NEXT"};
    const gchar *below_btn[] = {"PLAYBACK", "CLOSE"};

    nffont_type label_font;

    GdkPixbuf *memo_btn_img[NFOBJECT_STATE_COUNT];

    focused_row = -1;
    selected_row = -1;

    prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
    prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);  
    prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);  
    prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);  

    next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
    next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);  
    next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);  
    next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);  

    g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
    g_content_fixed = content_fixed;
    
    // STORAGE
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STORAGE DEVICE", 
                                    nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, ART3_STORAGE_LABEL_W, ART3_STORAGE_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_STORAGE_LABEL_X, ART3_STORAGE_LABEL_Y);
                
    obj = nfui_combobox_new(NULL, 0, 0);
    dev_obj = obj;
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);   
    nfui_nfobject_set_size(obj, ART3_STORAGE_COMBO_W, ART3_STORAGE_COMBO_H);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_STORAGE_COMBO_X, ART3_STORAGE_COMBO_Y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(dev_obj, art3_post_device_event_handler);

    // SEARCH BUTTON    
    obj = nftool_normal_button_create_type3("SEARCH", ART3_SEARCH_BUTTON_SIZE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_SEARCH_BUTTON_X, ART3_SEARCH_BUTTON_Y);
    nfui_regi_post_event_callback(obj, art3_search_btn_handler);

    for(j = 0; j < 5; j++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(listTitlelbl[j], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        
        if (j == 0)
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        else if (j == 1)
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 5);
        else
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);       

        nfui_nfobject_set_size(obj, list_width[j], ART3_LIST_LABEL_H);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, list_xpos[j], ypos); 
    }

    if(nftool_cur_language_is_japanese())
        label_font = NFFONT_MEDIUM_THIN;
    else
        label_font = NFFONT_MEDIUM_SEMI;

    ypos += ART3_LIST_LABEL_H_INTERVAL;

    for(i = 0; i < ART3_LIST_MAX; i++ )
    {
        for( j = 0 ; j < 5 ; j++ )
        {
            obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(label_font));
            nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
            nfui_nflabel_set_style(obj, NFSTY_NOOUTLINE);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);    

            if (j == 0)
                nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
            else if (j == 1)
                nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 5);
            else
                nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);       

            nfui_nfobject_set_size(obj, list_width[j], ART3_LIST_LABEL_H);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, list_xpos[j], ypos);
            nfui_regi_pre_event_callback(obj, pre_art3_listlbl_btn_handler);
            nfui_regi_post_event_callback(obj, post_art3_listlbl_btn_handler);
            listlbl_obj[i][j] = obj;
        }
        
        ypos += ART3_LIST_LABEL_H_INTERVAL;
        
    }

    memo_btn_img[0] = nfui_get_image_from_file((IMG_BTN_N_MEMO), NULL);
    memo_btn_img[1] = nfui_get_image_from_file((IMG_BTN_O_MEMO), NULL);
    memo_btn_img[2] = nfui_get_image_from_file((IMG_BTN_P_MEMO), NULL);
    memo_btn_img[3] = nfui_get_image_from_file((IMG_BTN_D_MEMO), NULL);
    nfui_get_pixbuf_size(memo_btn_img[0], &size_w, &size_h);

    ypos = ART3_LIST_START_Y + 40;
    
    for(i = 0; i < ART3_LIST_MAX; i++)
    {
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), memo_btn_img);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_disable(obj);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos+1538+2 , ypos);
        nfui_regi_post_event_callback(obj, art3_memo_btn_handler);
        memobtns_obj[i] = obj;
        
        ypos += ART3_LIST_LABEL_H_INTERVAL;
    }

    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_PREV_BTN_X, ART3_PREV_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_prev_btn_handler);

    nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_NEXT_BTN_X, ART3_NEXT_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_next_btn_handler);



#if 0
    obj = nftool_normal_button_create_type3(page_btn[0], ART3_PAGE_BTN_SIZE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_PREV_BTN_X, ART3_PREV_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_prev_btn_handler);
#endif  

    pagelbl_obj2 = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0/0",
                                    nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(121));
    //pagelbl_obj2 = obj;
    nfui_nflabel_set_align((NFLABEL*)pagelbl_obj2, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(pagelbl_obj2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(pagelbl_obj2, ART3_PAGE_LABEL_W, ART3_PAGE_LABEL_H);
    nfui_nfobject_use_focus(pagelbl_obj2, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(pagelbl_obj2);
    nfui_nffixed_put((NFFIXED*)content_fixed, pagelbl_obj2, ART3_PAGE_LABEL_X, ART3_PAGE_LABEL_Y);
#if 0
    obj = nftool_normal_button_create_type3(page_btn[1], ART3_PAGE_BTN_SIZE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_NEXT_BTN_X, ART3_NEXT_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_next_btn_handler);
#endif

// VERIFY BTN
    obj = nftool_normal_button_create_type3("VERIFY INTEGRITY", 
                                            ART3_VERIFY_BTN_SIZE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART3_VERIFY_BTN_X, ART3_VERIFY_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_verify_btn_handler);


// PLAYBACK
    obj = nftool_normal_button_create_type1(below_btn[0], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_playback_btn_handler);

    obj = nftool_normal_button_create_type2(below_btn[1], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, art3_close_btn_handler);
    
// event handler
    nfui_regi_post_event_callback(content_fixed, arch_tab3_post_fixed_event_handler);

    uxm_reg_imsg_event(content_fixed, INFY_MEDIA_STATUS_CHANGED);   
    uxm_monitor_on_imsg_event(content_fixed, INFY_MEDIA_STATUS_CHANGED);

    uxm_reg_imsg_event(content_fixed, IRET_OPEN_ARCH_MANAGER);
    scm_open_avi_play_manager(IRET_OPEN_ARCH_MANAGER);
    
    uxm_reg_imsg_event(content_fixed, INFY_PLAYBACK_STARTED);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
    uxm_monitor_on_imsg_event(content_fixed, INFY_PLAYBACK_STARTED);

    g_timeout_add(10, _update_dev, 0);
}

gboolean vw_arch_tab3_out_handler()
{
    _clear_avi_list();
    return FALSE;
}

gboolean vw_arch_tab3_in_handler()
{
    _update_info();
    return FALSE;
}
