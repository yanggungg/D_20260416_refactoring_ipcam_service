#include "../../support/util.h"
#include "../../support/event_loop.h"
#include "../../support/color.h"
#include "../../tools/nf_ui_tool.h"

#include "nfbutton.h"
#include "nflistbox.h"
#include "ix_mem.h"


#define FONT_MARGIN                 (4)
#define LISTBOX_LINE_BORDER         (2)

#define LINE_LEFT_MARGIN            (1)
#define LINE_TOP_MARGIN         (1)
#define LINE_WIDTH_MARGIN           (2)
#define LINE_HEIGHT_MARGIN          (2)


static void _set_listbox_type_1(NFLISTBOX *lbox)
{
    GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];   
    gint color[NFOBJECT_STATE_COUNT];   
    gint size_w, size_h;

    scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
    scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);   
    scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);   
    scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);   

    scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
    scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
    scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
    scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

    nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

    lbox->st_scroll.width = size_w;

    nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), scroll_up);
    nfui_nfobject_set_size(lbox->up_btn, size_w, size_h);
    
    nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), scroll_down);
    nfui_nfobject_set_size(lbox->down_btn, size_w, size_h);

    lbox->font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(155);
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(159);
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(159);
    lbox->font_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(157);
    
    lbox->bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(154);
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(158);
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(158);
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(156);

    lbox->skin_type = NFLISTBOX_TYPE_1;
}

static void _set_listbox_type_2(NFLISTBOX *lbox)
{
    GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];   
    gint color[NFOBJECT_STATE_COUNT];   
    gint size_w, size_h;

    scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
    scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);   
    scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);   
    scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);   

    scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
    scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
    scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
    scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

    nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

    lbox->st_scroll.width = size_w;

    nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), scroll_up);
    nfui_nfobject_set_size(lbox->up_btn, size_w, size_h);
    
    nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), scroll_down);
    nfui_nfobject_set_size(lbox->down_btn, size_w, size_h);

    lbox->font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(169);
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(173);
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(173);
    lbox->font_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(171);
    
    lbox->bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(168);
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(172);
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(172);
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(170);

    lbox->skin_type = NFLISTBOX_TYPE_2;
}

static void _set_listbox_type_popup_1(NFLISTBOX *lbox)
{
    GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];   
    gint color[NFOBJECT_STATE_COUNT];   
    gint size_w, size_h;

    scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
    scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);   
    scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);   
    scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);   

    scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
    scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
    scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
    scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

    nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

    lbox->st_scroll.width = size_w;

    nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), scroll_up);
    nfui_nfobject_set_size(lbox->up_btn, size_w, size_h);
    
    nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), scroll_down);
    nfui_nfobject_set_size(lbox->down_btn, size_w, size_h);

    lbox->font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(247);
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    lbox->font_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);
    
    lbox->bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(246);
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(248);

    lbox->skin_type = NFLISTBOX_TYPE_POPUP_1;
}

static void _set_listbox_type_popup_2(NFLISTBOX *lbox)
{
    GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];   
    gint color[NFOBJECT_STATE_COUNT];   
    gint size_w, size_h;

    scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
    scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);   
    scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);   
    scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);   

    scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
    scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
    scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
    scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

    nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

    lbox->st_scroll.width = size_w;

    nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), scroll_up);
    nfui_nfobject_set_size(lbox->up_btn, size_w, size_h);
    
    nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), scroll_down);
    nfui_nfobject_set_size(lbox->down_btn, size_w, size_h);

    lbox->font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(261);
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(265);
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(265);
    lbox->font_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(263);
    
    lbox->bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(260);
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(264);
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(264);
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(262);

    lbox->skin_type = NFLISTBOX_TYPE_POPUP_2;
}

static void _set_listbox_type_subtab_1(NFLISTBOX *lbox)
{
    GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];   
    gint color[NFOBJECT_STATE_COUNT];   
    gint size_w, size_h;

    scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
    scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);   
    scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);   
    scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);   

    scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
    scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
    scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
    scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

    nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

    lbox->st_scroll.width = size_w;

    nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), scroll_up);
    nfui_nfobject_set_size(lbox->up_btn, size_w, size_h);
    
    nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), scroll_down);
    nfui_nfobject_set_size(lbox->down_btn, size_w, size_h);

    lbox->font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(937);
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(941);
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(941);
    lbox->font_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(939);
    
    lbox->bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(936);
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(940);
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(940);
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(938);

    lbox->skin_type = NFLISTBOX_TYPE_SUBTAB_1;
}

static void _set_listbox_type_subtab_2(NFLISTBOX *lbox)
{
    GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
    GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];   
    gint color[NFOBJECT_STATE_COUNT];   
    gint size_w, size_h;

    scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
    scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);   
    scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);   
    scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);   

    scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
    scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
    scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
    scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

    nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

    lbox->st_scroll.width = size_w;

    nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), scroll_up);
    nfui_nfobject_set_size(lbox->up_btn, size_w, size_h);
    
    nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), scroll_down);
    nfui_nfobject_set_size(lbox->down_btn, size_w, size_h);

    lbox->font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(951);
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(955);
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(955);
    lbox->font_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(953);
    
    lbox->bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(950);
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(954);
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(954);
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(952);

    lbox->skin_type = NFLISTBOX_TYPE_SUBTAB_2;
}

static void _set_listbox_scroll_info(NFLISTBOX *lbox)
{
    gint scroll_height;

    if (lbox->update_data == 0) return;

    scroll_height = lbox->lbox_fixed.object.height - (2*lbox->up_btn->height) - 2;

    if (lbox->text_row_cnt <= lbox->box_row_cnt)
    {
        lbox->st_scroll.size = scroll_height;
        lbox->st_scroll.total_step = 0;
        lbox->st_scroll.interval = 0;
        lbox->st_scroll.pps = 0;
    }
    else
    {   
        if (lbox->text_row_cnt == 0) g_assert(0);
        if (lbox->text_row_cnt == lbox->box_row_cnt) g_assert(0);

        lbox->st_scroll.size = (gint)(scroll_height * (float)lbox->box_row_cnt / (float)lbox->text_row_cnt);
        lbox->st_scroll.total_step = lbox->text_row_cnt - lbox->box_row_cnt;
        lbox->st_scroll.interval = scroll_height - lbox->st_scroll.size;
        lbox->st_scroll.pps = ((float)(lbox->st_scroll.interval)) / ((float)(lbox->st_scroll.total_step));
    }

    if (lbox->st_scroll.cur_step >= lbox->st_scroll.total_step)
    {
        lbox->st_scroll.cur_step = lbox->st_scroll.total_step;

        if (lbox->st_scroll.cur_step == 0)
            lbox->st_scroll.cur_pos = 0;
        else
            lbox->st_scroll.cur_pos = (gint)(((float)(lbox->st_scroll.cur_step)) * lbox->st_scroll.pps);
    }
    else
    {
        lbox->st_scroll.cur_pos = (gint)(((float)(lbox->st_scroll.cur_step)) * lbox->st_scroll.pps);
    }

    lbox->update_data = 0;
}

static void _set_listbox_scroll_pos(NFLISTBOX *lbox, gint is_inc)
{
    if (is_inc)
    {
        if (lbox->st_scroll.cur_step >= lbox->st_scroll.total_step) return;

        lbox->st_scroll.cur_step++;
        
        if (lbox->st_scroll.cur_step == lbox->st_scroll.total_step)
            lbox->st_scroll.cur_pos = lbox->st_scroll.interval;
        else
            lbox->st_scroll.cur_pos = (gint)(((float)(lbox->st_scroll.cur_step)) * lbox->st_scroll.pps);
    }
    else
    {
        if (lbox->st_scroll.cur_step == 0)  return;

        lbox->st_scroll.cur_step--;
    
        if (lbox->st_scroll.cur_step == 0)
            lbox->st_scroll.cur_pos = 0;
        else
            lbox->st_scroll.cur_pos = (gint)(((float)(lbox->st_scroll.cur_step)) * lbox->st_scroll.pps);
    }
}

static void _set_listbox_child_position(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    guint x, y, w, h;

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &x, &y);
    w = lbox->lbox_fixed.object.width;  
    h = lbox->lbox_fixed.object.height;

    ((NFOBJECT *)lbox->up_btn)->x = w - lbox->up_btn->width;
    ((NFOBJECT *)lbox->up_btn)->y = 0;

    ((NFOBJECT *)lbox->down_btn)->x = w - lbox->down_btn->width;
    ((NFOBJECT *)lbox->down_btn)->y = h - lbox->down_btn->height;
}

static void _set_listbox_boxInfo(NFLISTBOX *lbox)
{
    gint i, j;
    gint row, col;
    guint obj_x, obj_y, obj_w, obj_h;
    guint col_x;

    if (lbox->boxInfo) return;

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &obj_x, &obj_y);
    obj_w = lbox->lbox_fixed.object.width;  
    obj_h = lbox->lbox_fixed.object.height;

    if (lbox->row_height == 0) g_assert(0);

    row = (gint)obj_h / lbox->row_height;
    lbox->box_row_cnt = row;
    col = lbox->column; 
        
    lbox->boxInfo = imalloc(sizeof(NFLISTBOXINFO)*row*col);
    
    for (i = 0; i < row; i++) 
    {
        lbox->boxInfo[i].col_rect[0].x = obj_x;
        lbox->boxInfo[i].col_rect[0].y = obj_y + (lbox->row_height * i);
        lbox->boxInfo[i].col_rect[0].width = lbox->col_w[0];
        lbox->boxInfo[i].col_rect[0].height = lbox->row_height;

        lbox->boxInfo[i].focus = FALSE;
        lbox->boxInfo[i].index = i;

        col_x = obj_x + lbox->col_w[0];

        for (j = 1; j < col; j++) 
        {       
            lbox->boxInfo[i].col_rect[j].x = col_x;
            lbox->boxInfo[i].col_rect[j].width = lbox->col_w[j];

            lbox->boxInfo[i].col_rect[j].y = lbox->boxInfo[i].col_rect[0].y;
            lbox->boxInfo[i].col_rect[j].height = lbox->boxInfo[i].col_rect[0].height;

            col_x += lbox->col_w[j];
        }   
    }

    _set_listbox_child_position(lbox);
}

static gint _set_listbox_scroll_state_mevt_press(NFLISTBOX *lbox, gint mouse_x, gint mouse_y)
{
    gint object_x, object_y;
    gint scroll_x, scroll_y;
  
    nfui_nfobject_get_offset((NFOBJECT*)lbox, &object_x, &object_y);

    scroll_x = object_x + lbox->lbox_fixed.object.width - lbox->up_btn->width;
    scroll_y = object_y + lbox->up_btn->height + lbox->st_scroll.cur_pos + 1;

    if (mouse_x < scroll_x) return -1;    
    if (mouse_x > scroll_x+lbox->st_scroll.width) return -1;
    if (mouse_y < object_y+lbox->up_btn->height+1) return -1;
    if (mouse_y > object_y+lbox->lbox_fixed.object.height-lbox->down_btn->height) return -1;

    if (lbox->st_scroll.scroll == lbox->st_scroll.active) return -1;

    lbox->st_scroll.scroll = lbox->st_scroll.active;
    lbox->press_scroll = 1;
    return 0;
}

static gint _set_listbox_scroll_state_mevt_release(NFLISTBOX *lbox, gint mouse_x, gint mouse_y)
{
    if (lbox->st_scroll.scroll == lbox->st_scroll.normal) return -1;

    lbox->st_scroll.scroll = lbox->st_scroll.normal;
    lbox->press_scroll = 0;    
    return 0;
}

static gint _set_listbox_scroll_state_mevt_move(NFLISTBOX *lbox, gint mouse_x, gint mouse_y)
{
    GdkPixbuf *post_scroll;

    gint object_x, object_y;
    gint scroll_x, scroll_y;
  
    nfui_nfobject_get_offset((NFOBJECT*)lbox, &object_x, &object_y);

    scroll_x = object_x + lbox->lbox_fixed.object.width - lbox->up_btn->width;
    scroll_y = object_y + lbox->up_btn->height + lbox->st_scroll.cur_pos + 1;

    if ((mouse_x > scroll_x) && (mouse_x < scroll_x+lbox->st_scroll.width) && \
       (mouse_y > scroll_y) && (mouse_y < scroll_y+lbox->st_scroll.size))
    {
        post_scroll = lbox->st_scroll.focus;
    }
    else
    {
        post_scroll = lbox->st_scroll.normal;
    }

    if (lbox->st_scroll.scroll == post_scroll) return -1;

    lbox->st_scroll.scroll = post_scroll;
    return 0;
}

static gint _set_listbox_contents_index_mevt_drag(NFLISTBOX *lbox, gint mouse_y)
{
    gint object_x, object_y;

    gint pre_list_idx, post_list_idx;
    gint i;
  
    nfui_nfobject_get_offset((NFOBJECT*)lbox, &object_x, &object_y);

    lbox->st_scroll.scroll = lbox->st_scroll.active;
    lbox->st_scroll.cur_pos = mouse_y - object_y - lbox->up_btn->height - lbox->st_scroll.size/2;

    if (lbox->st_scroll.cur_pos < 0) 
    {
        lbox->st_scroll.cur_pos = 0;
    }
    
    if (lbox->st_scroll.cur_pos+lbox->st_scroll.size > lbox->lbox_fixed.object.height-lbox->up_btn->height*2-2) 
    {
        lbox->st_scroll.cur_pos = lbox->lbox_fixed.object.height-lbox->up_btn->height*2-2-lbox->st_scroll.size;
    }

    pre_list_idx = lbox->boxInfo[0].index;
    post_list_idx = lbox->st_scroll.cur_pos/lbox->st_scroll.pps;

    if (pre_list_idx == post_list_idx) return -1;

    for (i = 0; i < lbox->box_row_cnt; i++) 
    {
        lbox->boxInfo[i].index = post_list_idx+i;
    }

    lbox->st_scroll.cur_step = post_list_idx;
    return 0;
}

static gint _set_listbox_contents_index_mevt_scrollup(NFLISTBOX *lbox)
{
    gint i;

    if ((lbox->boxInfo[0].index-1) < 0) return -1;

    for (i = lbox->box_row_cnt-1; i >= 0; i--) 
    {
        if (lbox->boxInfo[i].focus) 
        {
            if (i == (lbox->box_row_cnt-1)) 
            {
                lbox->boxInfo[i].focus = FALSE;
            }
            else if (i != (lbox->box_row_cnt-1)) 
            {
                lbox->boxInfo[i].focus = FALSE;
                lbox->boxInfo[i+1].focus = TRUE;
            }
        }
        
        --lbox->boxInfo[i].index;
    }

    return 0;
}

static gint _set_listbox_contents_index_mevt_scrolldown(NFLISTBOX *lbox)
{
    gint i;

    if (lbox->boxInfo[0].index >= lbox->text_row_cnt-lbox->box_row_cnt) return -1;

    for (i = 0; i < lbox->box_row_cnt; i++) 
    {
        if (lbox->boxInfo[i].focus) 
        {
            if (i == 0)
            {
                lbox->boxInfo[i].focus = FALSE;
            }                        
            else if (i != 0) 
            {
                lbox->boxInfo[i].focus = FALSE;
                lbox->boxInfo[i-1].focus = TRUE;
            }
        }

        ++lbox->boxInfo[i].index;
    }

    return 0;
}

static void _draw_listbox_inline(NFLISTBOX *lbox, GdkDrawable *drawable)
{
    GdkGC *line_gc;
    guint off_x = 0, off_y = 0;
    guint pos_x = 0, pos_y = 0; 
    gint rect_w = 0, rect_h = 0;
    gint gap = 0;
    gint i;

    if (!lbox->draw_inline) return;
    if (!lbox->boxInfo)     return;

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &off_x, &off_y);

    line_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    
    gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(lbox->inline_color));
    gdk_gc_set_line_attributes(line_gc, 1, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    gdk_drawable_get_size(drawable, &rect_w, &rect_h);

    if (lbox->column > 0)
    {
        pos_x = lbox->boxInfo[0].col_rect[0].x -off_x;
        pos_y = lbox->boxInfo[0].col_rect[0].y -off_y;
            
        gdk_draw_line(drawable, line_gc, pos_x, pos_y, pos_x, pos_y + rect_h);

        for (i = 0; i < lbox->column; i++) 
        {
            pos_x = lbox->boxInfo[0].col_rect[i].x -off_x;
            pos_y = lbox->boxInfo[0].col_rect[i].y -off_y;

            gdk_draw_line(drawable, line_gc,
                pos_x + lbox->boxInfo[0].col_rect[i].width - 1,  pos_y,
                pos_x + lbox->boxInfo[0].col_rect[i].width - 1,  pos_y + rect_h);
        }
    }

    if (lbox->box_row_cnt > 0)
    {
        pos_x = lbox->boxInfo[0].col_rect[0].x -off_x;
        pos_y = lbox->boxInfo[0].col_rect[0].y -off_y;

        gdk_draw_line(drawable, line_gc, pos_x, pos_y, pos_x + rect_w, pos_y);

        for (i = 0; i < lbox->box_row_cnt; i++) 
        {
            pos_x = lbox->boxInfo[i].col_rect[0].x -off_x;
            pos_y = lbox->boxInfo[i].col_rect[0].y -off_y;

            gdk_draw_line(drawable, line_gc,
                pos_x, pos_y + lbox->boxInfo[i].col_rect[0].height - 1,
                pos_x + rect_w, pos_y + lbox->boxInfo[i].col_rect[0].height - 1);
        }
    }

    nfui_nfobject_gc_unref(line_gc);
}

static void _draw_listbox_bg(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    GdkGC *bg_gc;
    guint px = 0, py = 0;
    gint i;

    g_return_if_fail(lbox != NULL);

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);

    if(!drawable) return;

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);  

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &px, &py);    

    if (nfui_nfobject_is_disabled((NFOBJECT*)lbox))
    {
        gdk_draw_drawable(drawable, bg_gc, lbox->disable_reservedscr, 0, 0, px, py, -1, -1);
    }
    else
    {
        gdk_draw_drawable(drawable, bg_gc, lbox->normal_reservedscr, 0, 0, px, py, -1, -1);
        
        for (i = 0; i < lbox->box_row_cnt; i++) 
        {
            if (i > lbox->text_row_cnt - 1) break;

            if ((lbox->use_infocus) && (lbox->boxInfo[i].focus))
            {
                py = lbox->boxInfo[i].col_rect[0].y;            
                gdk_draw_drawable(drawable, bg_gc, lbox->active_reservedrow, 0, 0, px, py, -1, -1);
            }
        }
    }

    nfui_nfobject_gc_unref(bg_gc);
}

static void _draw_listbox_text(NFLISTBOX *lbox, gint row)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GSList *tlist;
    gchar *text;
    guint col_gap = 2;
    gint i;

    gint bg_idx, fg_idx;
    gchar *key = NULL;
    gchar key_temp[NFLBOX_CASHING_KEY_SIZE];

    if (row > lbox->text_row_cnt - 1) return;
    if (lbox->slist == NULL) return;

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    if (!drawable) return;

    gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);

    tlist = g_slist_nth_data(lbox->slist, (guint)lbox->boxInfo[row].index);
    
    for (i = 0; i < (gint)lbox->column; i++) 
    {
        text = (gchar*)g_slist_nth_data(tlist, (guint)i);

        if (nfui_nfobject_is_disabled((NFOBJECT*)lbox))
        {
            bg_idx = lbox->bg_color[NFOBJECT_STATE_DISABLE];
            fg_idx = lbox->font_color[NFOBJECT_STATE_DISABLE];
        }
        else
        {
            if ((lbox->use_infocus) && (lbox->boxInfo[row].focus))
            {
                bg_idx = lbox->bg_color[NFOBJECT_STATE_ACTIVE];
                fg_idx = lbox->font_color[NFOBJECT_STATE_ACTIVE];
            }
            else
            {
                bg_idx = lbox->bg_color[NFOBJECT_STATE_NORMAL];
                fg_idx = lbox->font_color[NFOBJECT_STATE_NORMAL];
            }
        }

        if (text != NULL) 
        {   
            if (strstr(text, LISTBOX_COLOR_KEY) != 0)
            {               
                bg_idx = atoi(text);
            
                gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_idx));
                gdk_draw_rectangle(drawable, gc, TRUE, lbox->boxInfo[row].col_rect[i].x+2, lbox->boxInfo[row].col_rect[i].y+2, lbox->boxInfo[row].col_rect[i].width-4, lbox->boxInfo[row].col_rect[i].height-4);
            }
            else if (strstr(text, LISTBOX_IMG_KEY) != 0)
            {
                GdkPixbuf *pbuf;
                pbuf = nfui_get_image_from_file(text, NULL);
                gdk_draw_pixbuf(drawable, gc, pbuf, 0, 0, lbox->boxInfo[row].col_rect[i].x, lbox->boxInfo[row].col_rect[i].y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
            }
            else
            {
                gint valid_cnt;

                valid_cnt = nfutil_string_get_valid_count(nfui_nfobject_is_supported_multi_lang(lbox), 
                    drawable, lbox->pango_font, text, lbox->boxInfo[row].col_rect[i].width-FONT_MARGIN);

                if (nfui_nfobject_is_supported_multi_lang(lbox))
                {
                    nfutil_draw_short_text(key, NULL, &UX_COLOR(bg_idx), drawable, gc, text, valid_cnt, 
                    lbox->boxInfo[row].col_rect[i].x, lbox->boxInfo[row].col_rect[i].y, lbox->boxInfo[row].col_rect[i].width, lbox->boxInfo[row].col_rect[i].height, 
                    lbox->pango_font, &UX_COLOR(fg_idx), NULL, lbox->align[i], FONT_MARGIN, 0, NORMAL_SPACING);
                }
                else
                {
                    nfutil_draw_short_text_eng(key, NULL, &UX_COLOR(bg_idx), drawable, gc, text, valid_cnt, 
                    lbox->boxInfo[row].col_rect[i].x, lbox->boxInfo[row].col_rect[i].y, lbox->boxInfo[row].col_rect[i].width, lbox->boxInfo[row].col_rect[i].height, 
                    lbox->pango_font, &UX_COLOR(fg_idx), NULL, lbox->align[i], FONT_MARGIN, 0, NORMAL_SPACING);
                }
            }
        }
    }

    nfui_nfobject_gc_unref(gc);
}

static void _draw_listbox_text_all(NFLISTBOX *lbox)
{
    gint i;

    for (i = 0; i < lbox->box_row_cnt; i++) 
    {
        _draw_listbox_text(lbox, i);
    }
}

static void _draw_listbox_scroll(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    guint x, y, w, h;
    GdkGC *scroll_gc;

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    if (!drawable) return;

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &x, &y);
    w = lbox->lbox_fixed.object.width;  
    h = lbox->lbox_fixed.object.height;

    scroll_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);  
    gdk_gc_set_rgb_fg_color(scroll_gc, &UX_COLOR(lbox->bg_color[NFOBJECT_STATE_NORMAL]));

    x += w - lbox->up_btn->width;
    y += lbox->up_btn->height;
    w = lbox->up_btn->width;
    h = h - lbox->down_btn->height - lbox->up_btn->height;
    gdk_draw_rectangle(drawable, scroll_gc, TRUE, x, y, w, h);

    y += lbox->st_scroll.cur_pos + 1;
    w = lbox->st_scroll.width;
    h = lbox->st_scroll.size;
    nfutil_draw_pixbuf(drawable, scroll_gc, lbox->st_scroll.scroll, x, y, w, h, NFALIGN_LEFT, 0);

    nfui_nfobject_gc_unref(scroll_gc);
}

static void _draw_listbox_outline(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    GdkGC *line_gc;
    guint pos_x, pos_y;
    gint i;
    gint sum_size_w = 0, sum_size_h = 0;
    guint colorIdx;

    if (!lbox->draw_outline) return;

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    line_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);

    if (lbox->lbox_fixed.object.kfocus == NFOBJECT_FOCUS)
    {
        if(((NFOBJECT*)lbox)->grab_kfocus) 
        {
            colorIdx = 147;
        }
        else 
        {
            colorIdx = 146;
        }
    }
    else
    {
        if (nfui_nfobject_is_disabled((NFOBJECT*)lbox))
        {
            colorIdx = lbox->bg_color[NFOBJECT_STATE_DISABLE];
        }
        else
        {
            colorIdx = lbox->bg_color[NFOBJECT_STATE_NORMAL];
        }
    }

    gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(colorIdx));
    gdk_gc_set_line_attributes(line_gc, LISTBOX_LINE_BORDER, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    nfui_nfobject_get_offset(((NFOBJECT*)lbox), &pos_x, &pos_y);

    sum_size_w = lbox->lbox_fixed.object.width - lbox->up_btn->width;
    sum_size_h = lbox->lbox_fixed.object.height;

    gdk_draw_rectangle(drawable, line_gc, FALSE,
        pos_x + LINE_LEFT_MARGIN, 
        pos_y + LINE_TOP_MARGIN,
        sum_size_w - LINE_WIDTH_MARGIN, 
        sum_size_h - LINE_HEIGHT_MARGIN);

    nfui_nfobject_gc_unref(line_gc);
}

static void _create_listbox_scrollbar(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    GdkPixbuf *bar[3];
    gint bar_w, bar_h;
    gint scroll_w, scroll_h;

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    
    if (drawable) 
    {
        _set_listbox_scroll_info(lbox);

        scroll_w = lbox->st_scroll.width;
        scroll_h = lbox->lbox_fixed.object.height - (2*lbox->up_btn->height);

        if (!lbox->st_scroll.normal)
        {
            lbox->st_scroll.normal = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scroll_w, scroll_h);
            gdk_pixbuf_fill(lbox->st_scroll.normal, 0x000000);

            bar[0] = nfui_get_image_from_file((IMG_N_SCROLLBAR_TOP), NULL);
            bar[1] = nfui_get_image_from_file((IMG_N_SCROLLBAR_MID), NULL);
            bar[2] = nfui_get_image_from_file((IMG_N_SCROLLBAR_BOT), NULL);

            nfui_get_pixbuf_size(bar[0], &bar_w, &bar_h);

            gdk_pixbuf_composite(bar[0], lbox->st_scroll.normal, 0, 0, bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
            gdk_pixbuf_composite(bar[1], lbox->st_scroll.normal, 0, bar_h, bar_w, (scroll_h - (bar_h * 2)), 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
            gdk_pixbuf_composite(bar[2], lbox->st_scroll.normal, 0, (bar_h + (scroll_h - (bar_h * 2))), bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
        }

        if (!lbox->st_scroll.focus)
        {
            lbox->st_scroll.focus = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scroll_w, scroll_h);
            gdk_pixbuf_fill(lbox->st_scroll.focus, 0x000000);

            bar[0] = nfui_get_image_from_file((IMG_O_SCROLLBAR_TOP), NULL);
            bar[1] = nfui_get_image_from_file((IMG_O_SCROLLBAR_MID), NULL);
            bar[2] = nfui_get_image_from_file((IMG_O_SCROLLBAR_BOT), NULL);

            nfui_get_pixbuf_size(bar[0], &bar_w, &bar_h);

            gdk_pixbuf_composite(bar[0], lbox->st_scroll.focus, 0, 0, bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
            gdk_pixbuf_composite(bar[1], lbox->st_scroll.focus, 0, bar_h, bar_w, (scroll_h - (bar_h * 2)), 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
            gdk_pixbuf_composite(bar[2], lbox->st_scroll.focus, 0, (bar_h + (scroll_h - (bar_h * 2))), bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
        }

        if (!lbox->st_scroll.active)
        {
            lbox->st_scroll.active = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scroll_w, scroll_h);
            gdk_pixbuf_fill(lbox->st_scroll.active, 0x000000);

            bar[0] = nfui_get_image_from_file((IMG_P_SCROLLBAR_TOP), NULL);
            bar[1] = nfui_get_image_from_file((IMG_P_SCROLLBAR_MID), NULL);
            bar[2] = nfui_get_image_from_file((IMG_P_SCROLLBAR_BOT), NULL);

            nfui_get_pixbuf_size(bar[0], &bar_w, &bar_h);

            gdk_pixbuf_composite(bar[0], lbox->st_scroll.active, 0, 0, bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
            gdk_pixbuf_composite(bar[1], lbox->st_scroll.active, 0, bar_h, bar_w, (scroll_h - (bar_h * 2)), 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
            gdk_pixbuf_composite(bar[2], lbox->st_scroll.active, 0, (bar_h + (scroll_h - (bar_h * 2))), bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
        }

        lbox->st_scroll.scroll = lbox->st_scroll.normal;
    }
}

static void _create_listbox_normal_bg(NFLISTBOX *lbox, GdkDrawable *drawable)
{
    GdkGC *bg_gc;
    guint px = 0, py = 0;
    guint rect_w = 0, rect_h = 0;
    gint i;

    g_return_if_fail(lbox != NULL);

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    gdk_drawable_get_size(drawable, &rect_w, &rect_h);
    
    gdk_gc_set_rgb_fg_color(bg_gc, &UX_COLOR(lbox->bg_color[NFOBJECT_STATE_NORMAL]));
    gdk_draw_rectangle(drawable, bg_gc, TRUE, (gint)0, (gint)0, rect_w, rect_h);

    nfui_nfobject_gc_unref(bg_gc);
}

static void _create_listbox_active_bg(NFLISTBOX *lbox, GdkDrawable *drawable)
{
    GdkGC *bg_gc;
    guint px = 0, py = 0;
    guint rect_w = 0, rect_h = 0;
    gint i;

    g_return_if_fail(lbox != NULL);

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    gdk_drawable_get_size(drawable, &rect_w, &rect_h);

    gdk_gc_set_rgb_fg_color(bg_gc, &UX_COLOR(lbox->bg_color[NFOBJECT_STATE_NORMAL]));
    gdk_draw_rectangle(drawable, bg_gc, TRUE, (gint)0, (gint)0, rect_w, rect_h);

    gdk_gc_set_rgb_fg_color(bg_gc, &UX_COLOR(lbox->bg_color[NFOBJECT_STATE_ACTIVE]));
    gdk_draw_rectangle(drawable, bg_gc, TRUE, (gint)2, (gint)2, rect_w-4, rect_h-4);

    nfui_nfobject_gc_unref(bg_gc);
}

static void _create_listbox_disable_bg(NFLISTBOX *lbox, GdkDrawable *drawable)
{
    GdkGC *bg_gc;
    guint px = 0, py = 0;
    guint rect_w = 0, rect_h = 0;
    gint i;

    g_return_if_fail(lbox != NULL);

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    gdk_drawable_get_size(drawable, &rect_w, &rect_h);

    gdk_gc_set_rgb_fg_color(bg_gc, &UX_COLOR(lbox->bg_color[NFOBJECT_STATE_DISABLE]));
    gdk_draw_rectangle(drawable, bg_gc, TRUE, (gint)0, (gint)0, rect_w, rect_h);

    nfui_nfobject_gc_unref(bg_gc);
}

static void _create_listbox_reserved(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    gint rect_w, rect_h;

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);

    rect_w = lbox->lbox_fixed.object.width - lbox->up_btn->width;
    rect_h = lbox->lbox_fixed.object.height;

    if (!lbox->normal_reservedscr) {
        lbox->normal_reservedscr = gdk_pixmap_new(drawable, rect_w, rect_h, -1);    
        _create_listbox_normal_bg(lbox, lbox->normal_reservedscr);
        _draw_listbox_inline(lbox, lbox->normal_reservedscr);
    }

    if (!lbox->disable_reservedscr) {
        lbox->disable_reservedscr = gdk_pixmap_new(drawable, rect_w, rect_h, -1);       
        _create_listbox_disable_bg(lbox, lbox->disable_reservedscr);
        _draw_listbox_inline(lbox, lbox->disable_reservedscr);
    }

    if (!lbox->normal_reservedrow) {
        lbox->normal_reservedrow = gdk_pixmap_new(drawable, rect_w, lbox->row_height, -1);          
        _create_listbox_normal_bg(lbox, lbox->normal_reservedrow);
        _draw_listbox_inline(lbox, lbox->normal_reservedrow);       
    }

    if (!lbox->active_reservedrow) {
        lbox->active_reservedrow = gdk_pixmap_new(drawable, rect_w, lbox->row_height, -1);          
        _create_listbox_active_bg(lbox, lbox->active_reservedrow);
        _draw_listbox_inline(lbox, lbox->active_reservedrow);       
    }
}

static void _change_listbox_row(NFLISTBOX *lbox, GdkDrawable *tmp_row, gint update_row)
{
    GdkDrawable *drawable = NULL;
    GdkGC *bg_gc;

    guint px = 0, py = 0;
    guint rect_w = 0, rect_h = 0;
    gint outline_w = LISTBOX_LINE_BORDER;

    g_return_if_fail(lbox != NULL);

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    if (!drawable) return;

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    nfui_nfobject_get_offset((NFOBJECT*)lbox, &px, &py);

    rect_w = lbox->lbox_fixed.object.width -  lbox->up_btn->width;
    rect_h = lbox->lbox_fixed.object.height;

    gdk_draw_drawable(drawable, bg_gc, tmp_row, outline_w, outline_w, 
        px+outline_w, lbox->boxInfo[update_row].col_rect[0].y+outline_w, rect_w-outline_w*2, lbox->row_height-outline_w*2);
    
    _draw_listbox_text(lbox, update_row);
}

static void _move_up_contents_listbox(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    GdkPixmap *tmp_row;
    GdkGC *bg_gc;

    guint px = 0, py = 0;
    guint rect_w = 0, rect_h = 0;
    gint update_row;
    gint outline_w = LISTBOX_LINE_BORDER;

    g_return_if_fail(lbox != NULL);

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    if (!drawable) return;

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    nfui_nfobject_get_offset((NFOBJECT*)lbox, &px, &py);

    rect_w = lbox->lbox_fixed.object.width -  lbox->up_btn->width;
    rect_h = lbox->lbox_fixed.object.height;

    update_row = 0;

    if ((lbox->use_infocus) && (lbox->boxInfo[update_row].focus))
        tmp_row = lbox->active_reservedrow;
    else
        tmp_row = lbox->normal_reservedrow;

    gdk_draw_drawable(drawable, bg_gc, drawable, px+outline_w, py+outline_w, 
        px+outline_w, py+lbox->row_height+outline_w, rect_w-outline_w*2, rect_h-lbox->row_height-outline_w*2);
    
    gdk_draw_drawable(drawable, bg_gc, tmp_row, outline_w, outline_w, 
        px+outline_w, py+outline_w, rect_w-outline_w*2, lbox->row_height-outline_w*2);

    nfui_nfobject_gc_unref(bg_gc);

    _draw_listbox_scroll(lbox);
    _draw_listbox_text(lbox, update_row);
}

static void _move_down_contents_listbox(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    GdkPixmap *tmp_row;
    GdkGC *bg_gc;

    guint px = 0, py = 0;
    guint rect_w = 0, rect_h = 0;
    gint update_row;
    gint outline_w = LISTBOX_LINE_BORDER;

    g_return_if_fail(lbox != NULL);

    drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
    if (!drawable) return;

    bg_gc = nfui_nfobject_get_gc((NFOBJECT*)lbox);
    nfui_nfobject_get_offset((NFOBJECT*)lbox, &px, &py);

    rect_w = lbox->lbox_fixed.object.width - lbox->up_btn->width;
    rect_h = lbox->lbox_fixed.object.height;

    update_row = lbox->box_row_cnt-1;

    if ((lbox->use_infocus) && (lbox->boxInfo[update_row].focus))
        tmp_row = lbox->active_reservedrow;
    else
        tmp_row = lbox->normal_reservedrow;
        
    gdk_draw_drawable(drawable, bg_gc, drawable, px+outline_w, py+lbox->row_height+outline_w, 
        px+outline_w, py+outline_w, rect_w-outline_w*2, rect_h-lbox->row_height-outline_w*2);
        
    gdk_draw_drawable(drawable, bg_gc, tmp_row, outline_w, outline_w, 
        px+outline_w, lbox->boxInfo[update_row].col_rect[0].y+outline_w, rect_w-outline_w*2, lbox->row_height-outline_w*2);

    nfui_nfobject_gc_unref(bg_gc);

    _draw_listbox_scroll(lbox);
    _draw_listbox_text(lbox, update_row);
}

static void _draw_listbox(NFLISTBOX *lbox)
{
    _draw_listbox_bg(lbox);
    _draw_listbox_outline(lbox);
    _draw_listbox_scroll(lbox);
    _draw_listbox_text_all(lbox);
}

static gboolean _tmr_update_listbox(gpointer data)
{
    NFLISTBOX *lbox = (NFLISTBOX*)data;
    NFOBJECT *top;
    GdkRectangle area;

    top = nfui_nfobject_get_top((NFOBJECT*)lbox);

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &area.x, &area.y);
    area.width = lbox->lbox_fixed.object.width - lbox->up_btn->width;  
    area.height = lbox->lbox_fixed.object.height;

    gdk_window_begin_paint_rect(GTK_WIDGET(((NFWINDOW*)top)->main_widget)->window, &area);    
    _draw_listbox_bg(lbox);
    _draw_listbox_outline(lbox);
    _draw_listbox_text_all(lbox);    
    gdk_window_end_paint(GTK_WIDGET(((NFWINDOW*)top)->main_widget)->window);
    
    lbox->update_delay_timer = 0;
    lbox->update_delete_cnt = 0;

    nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);
    return FALSE;
}

static void _delete_listbox_data(NFLISTBOX *lbox)
{
    gint cnt, cnt2;
    gint i, j;
    GSList *list_temp = NULL;
    gchar *data = NULL;

    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    if (lbox->slist)
    {
        cnt = g_slist_length(lbox->slist);

        for (i = 0; i < cnt; i++)
        {
            list_temp = g_slist_nth_data(lbox->slist, i);

            if (list_temp)
            {
                cnt2 = g_slist_length(list_temp);
            
                for (j = 0; j < cnt2; j++)
                {
                    data = g_slist_nth_data(list_temp, j);
                
                    if (data) g_free(data);
                        
                    data = NULL;
                }
                
                g_slist_free(list_temp);
                list_temp = NULL;
            }
        }

        g_slist_free(lbox->slist);
        
        lbox->slist = NULL;
        lbox->text_row_cnt = 0;     
    }
}

static gboolean _open_listbox_tooltip(gpointer data)
{
    NFLISTBOX *lbox;
    
    lbox = (NFLISTBOX*)data;
    
    nftool_tooltip_show((NFOBJECT*)lbox, 0);
    lbox->tmr_tooltip = 0;
    return FALSE;
}

static void _close_listbox_tooltip(NFLISTBOX *lbox)
{
    nftool_tooltip_hide();

    if(lbox->tmr_tooltip)
        g_source_remove(lbox->tmr_tooltip);

    lbox->tmr_tooltip = 0;
}

static gboolean _listbox_event_handler(NFLISTBOX *lbox, GdkEvent *event, gpointer data)
{
    g_return_val_if_fail(lbox != NULL, FALSE);

    switch(event->type) 
    {
        case GDK_EXPOSE:
        {   
            _set_listbox_boxInfo(lbox);         
            _create_listbox_reserved(lbox);
            _create_listbox_scrollbar(lbox);
            _set_listbox_scroll_info(lbox);
            _draw_listbox(lbox);            
        }
        break;

        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:
        {   
            if (nfui_nfobject_is_shown((NFOBJECT*)lbox))
            {                       
                _draw_listbox_outline(lbox);

                if (event->type == GDK_LEAVE_NOTIFY)
                {
                    if (lbox->st_scroll.scroll != lbox->st_scroll.normal)
                    {
                        lbox->st_scroll.scroll = lbox->st_scroll.normal;
                        _draw_listbox_scroll(lbox);
                    }
                    lbox->press_scroll = 0;
                }
            }

            _close_listbox_tooltip(lbox);
        }
        break;

        case GDK_MOTION_NOTIFY:
        {
            GdkEventMotion *mevt;
            gint is_changed;
            GdkDrawable *drawable = NULL;           
            gint i, j;    

            _close_listbox_tooltip(lbox);            
            mevt = (GdkEventMotion*)event;

            if (lbox->press_scroll)
            {
                if (_set_listbox_contents_index_mevt_drag(lbox, (gint)mevt->y) == 0)
                {                
                    _draw_listbox_scroll(lbox);
                    nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);
                
                    if (lbox->update_delete_cnt > 7) return FALSE;
                   
                    if (lbox->update_delay_timer) 
                    {
                        g_source_remove(lbox->update_delay_timer);
                        lbox->update_delete_cnt++;
                    }

                    lbox->update_delay_timer = g_timeout_add(50, _tmr_update_listbox, (gpointer)lbox);
                }
            }
            else
            {
                for (i = 0; i < lbox->box_row_cnt; i++) 
                {
                    for (j = 0; j < (gint)lbox->column; j++) 
                    {               
                        if((mevt->x > lbox->boxInfo[i].col_rect[j].x) && \
                            (mevt->x < lbox->boxInfo[i].col_rect[j].x + lbox->boxInfo[i].col_rect[j].width) && \
                            (mevt->y > lbox->boxInfo[i].col_rect[j].y) && \
                            (mevt->y < lbox->boxInfo[i].col_rect[j].y + lbox->boxInfo[i].col_rect[j].height)) 
                        {
                            gint valid_cnt;
                            GSList *tlist;
                            gchar *text;

                            drawable = nfui_nfobject_get_window((NFOBJECT*)lbox);
                            tlist = g_slist_nth_data(lbox->slist, (guint)lbox->boxInfo[i].index);
                            text = (gchar*)g_slist_nth_data(tlist, (guint)j);
                            
                            if (text)
                            {
                                valid_cnt = nfutil_string_get_valid_count(nfui_nfobject_is_supported_multi_lang(lbox), 
                                    drawable, lbox->pango_font, text, lbox->boxInfo[i].col_rect[j].width-FONT_MARGIN);

                                if (valid_cnt != 0)
                                {
                                    if(lbox->use_tooltip){
                                        nfui_nfobject_set_tooltip((NFOBJECT*)lbox, text);
                                        lbox->tmr_tooltip = g_timeout_add(500, _open_listbox_tooltip, (gpointer)lbox);
                                    }
                                }
                                else
                                {
                                    nfui_nfobject_use_tooltip((NFOBJECT*)lbox, FALSE);
                                }
                            }
                        }
                    }
                }

                if (_set_listbox_scroll_state_mevt_move(lbox, (gint)mevt->x, (gint)mevt->y) == 0) 
                {
                    _draw_listbox_scroll(lbox);
                }
            }        
        }
        break;

        case GDK_BUTTON_PRESS:
        {
            GdkEventButton *bevent;
            gint i;

            if (event->button.button == MOUSE_RIGTH_BUTTON) break;

            bevent = (GdkEventButton*)event;
            
            if (_set_listbox_scroll_state_mevt_press(lbox, (gint)bevent->x, (gint)bevent->y) == 0) 
            {
                for (i = 0; i < lbox->box_row_cnt; i++) 
                {
                    if (lbox->boxInfo[i].focus) lbox->boxInfo[i].focus = FALSE;
                }
            
                _set_listbox_contents_index_mevt_drag(lbox, (gint)bevent->y);
                nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);

                if (lbox->update_delay_timer) 
                {
                    g_source_remove(lbox->update_delay_timer);
                }

                lbox->update_delay_timer = g_timeout_add(50, _tmr_update_listbox, (gpointer)lbox);                
            }
        }
        break;

        case GDK_BUTTON_RELEASE:
        {           
            GdkEventButton *bevent;
            gint sum_size_w = 0;
            gint x, y;
            gint i, pre_row = -1, post_row = -1;

            if (event->button.button == MOUSE_RIGTH_BUTTON) break;

            bevent = (GdkEventButton*)event;
            x = (gint)bevent->x;    
            y = (gint)bevent->y;    

            if (lbox->press_scroll)
            {
                _set_listbox_scroll_state_mevt_release(lbox, x, y);
                _draw_listbox_scroll(lbox);
                return FALSE;
            }

            if (lbox->lbox_fixed.object.grab_kfocus)
                lbox->lbox_fixed.object.grab_kfocus = FALSE;

            for (i = 0; i < lbox->box_row_cnt; i++) 
            {
                if (lbox->boxInfo[i].focus) 
                {
                    pre_row = i;
                }
            }

            sum_size_w = lbox->lbox_fixed.object.width - lbox->up_btn->width;

            for (i = 0; i < lbox->box_row_cnt; i++) 
            {
                if ((x > lbox->boxInfo[i].col_rect[0].x) && \
                    (x < lbox->boxInfo[i].col_rect[0].x + sum_size_w) && \
                    (y > lbox->boxInfo[i].col_rect[0].y) && \
                    (y < lbox->boxInfo[i].col_rect[0].y + lbox->boxInfo[i].col_rect[0].height)) 
                {
                    post_row = i;
                }
            }
            
            if (pre_row == post_row) return FALSE;

            if (pre_row != -1)
            {
                lbox->boxInfo[pre_row].focus = FALSE;
                _change_listbox_row(lbox, lbox->normal_reservedrow, pre_row);
            }

            if (post_row != -1)
            {
                lbox->boxInfo[post_row].focus = TRUE;

                if (lbox->use_infocus) {
                    _change_listbox_row(lbox, lbox->active_reservedrow, post_row);
                }
                else {
                    _change_listbox_row(lbox, lbox->normal_reservedrow, post_row);
                }                
            }

            _draw_listbox_outline(lbox);
            nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);
        }
        break;
            
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
        {
            GdkEventKey *kevt;
            KEYPAD_KID kpid;
            gint focus_idx = -1;
            gint i, j;

            kevt = (GdkEventKey*)event;
            kpid = (KEYPAD_KID)kevt->keyval;

            if(kpid == KEYPAD_ENTER) 
            {
                if (lbox->lbox_fixed.object.grab_kfocus) 
                {
                    lbox->lbox_fixed.object.grab_kfocus = FALSE; 
                    _draw_listbox_outline(lbox);
                    return TRUE;
                }
                else 
                {
                    lbox->lbox_fixed.object.grab_kfocus = TRUE;

                    if (nfui_listbox_get_focus_idx(lbox) < 0)
                    {
                        if (lbox->text_row_cnt > 0)
                        {
                            lbox->boxInfo[0].focus = TRUE;
                            _draw_listbox(lbox);
                            return TRUE;                            
                        }
                        else
                        {
                            _draw_listbox_outline(lbox);
                        }
                    }
                }
            }
            else if (kpid == KEYPAD_EXIT) 
            {
                if (lbox->lbox_fixed.object.grab_kfocus) 
                {
                    lbox->lbox_fixed.object.grab_kfocus = FALSE; 

                    _draw_listbox_bg(lbox);
                    _draw_listbox_outline(lbox);
                    _draw_listbox_text_all(lbox);
                    return TRUE;
                }
            }
            else if ((kpid == KEYPAD_LEFT || kpid == KEYPAD_RIGHT))
            {
                if(lbox->lbox_fixed.object.grab_kfocus) 
                    return TRUE;
            }
            else if (kpid == KEYPAD_UP) 
            {
                if (lbox->lbox_fixed.object.grab_kfocus) 
                {
                    for (i = 0; i < lbox->box_row_cnt; i++) 
                    {
                        if (lbox->boxInfo[i].focus) 
                        { 
                            if (lbox->boxInfo[i].index <= 0)
                                break;

                            if (i == 0) 
                            {
                                for(j=0; j<lbox->box_row_cnt; j++) 
                                    --lbox->boxInfo[j].index;

                                lbox->boxInfo[i+1].focus = FALSE;
                                lbox->boxInfo[i].focus = TRUE;

                                _set_listbox_scroll_pos(lbox, 0);

                            }
                            else 
                            {
                                lbox->boxInfo[i].focus = FALSE;
                                lbox->boxInfo[i-1].focus = TRUE;
                            }
                            
                            break;
                        }
                    }

                    _draw_listbox(lbox);
                    nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);

                    return TRUE;
                }
            }
            else if (kpid == KEYPAD_DOWN) 
            {
                if (lbox->lbox_fixed.object.grab_kfocus) 
                {
                    for (i = 0; i < lbox->box_row_cnt; i++) 
                    {
                        if (lbox->boxInfo[i].focus) 
                        { 
                            if (lbox->text_row_cnt-1 == lbox->boxInfo[i].index)
                                break;

                            if (i == lbox->box_row_cnt - 1) 
                            {
                                for (j = 0; j < lbox->box_row_cnt; j++) 
                                    ++lbox->boxInfo[j].index;

                                lbox->boxInfo[i-1].focus = FALSE;
                                lbox->boxInfo[i].focus = TRUE;
                                _set_listbox_scroll_pos(lbox, 1);

                            }
                            else 
                            {
                                lbox->boxInfo[i].focus = FALSE;
                                lbox->boxInfo[i+1].focus = TRUE;
                            }
                            break;
                        }
                    }

                    _draw_listbox(lbox);
                    nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);

                    return TRUE;
                }
            }
        }
        break;
            
        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_RELEASE:
        break;
                       
        case GDK_SCROLL:
        {
            GdkEventScroll *sevt;
            gint i;

            if (lbox->text_row_cnt <= lbox->box_row_cnt) 
            {
                if (lbox->boxInfo[0].index != 1)
                    break;
            }

            sevt = (GdkEventScroll*)event;
            
            if (sevt->direction == GDK_SCROLL_UP) 
            {
                if (_set_listbox_contents_index_mevt_scrollup(lbox) == 0)
                {
                    _set_listbox_scroll_pos(lbox, 0);
                    _draw_listbox_scroll(lbox);
                    nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);   

                    if (lbox->update_delete_cnt > 5) return FALSE;

                    if (lbox->update_delay_timer) 
                    {
                        g_source_remove(lbox->update_delay_timer);
                        lbox->update_delete_cnt++;
                    }

                    lbox->update_delay_timer = g_timeout_add(50, _tmr_update_listbox, (gpointer)lbox);
                }
            }
            else if(sevt->direction == GDK_SCROLL_DOWN) 
            {
                if (_set_listbox_contents_index_mevt_scrolldown(lbox) == 0)
                {
                    _set_listbox_scroll_pos(lbox, 1);
                    _draw_listbox_scroll(lbox);
                    nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);   

                    if (lbox->update_delete_cnt > 5) return FALSE;

                    if (lbox->update_delay_timer) 
                    {
                        g_source_remove(lbox->update_delay_timer);
                        lbox->update_delete_cnt++;
                    }

                    lbox->update_delay_timer = g_timeout_add(50, _tmr_update_listbox, (gpointer)lbox);
                }
            }
        }
        break;

        case GDK_DELETE:
        {   
            if (lbox->update_delay_timer) 
            {
                g_source_remove(lbox->update_delay_timer);
                lbox->update_delay_timer = 0;
            }
        
            if (lbox->st_scroll.normal)
            {
                g_object_unref(lbox->st_scroll.normal);
                lbox->st_scroll.normal = NULL;
            }

            if (lbox->st_scroll.focus)
            {
                g_object_unref(lbox->st_scroll.focus);
                lbox->st_scroll.focus = NULL;
            }

            if (lbox->st_scroll.active)
            {
                g_object_unref(lbox->st_scroll.active);
                lbox->st_scroll.active = NULL;
            }            

            _delete_listbox_data(lbox);
            
            if (lbox->col_w) {
                g_free(lbox->col_w);
                lbox->col_w = NULL;
            }

            if (lbox->boxInfo) {
                ifree(lbox->boxInfo);
                lbox->boxInfo = NULL;
            }

            if (lbox->normal_reservedscr) {
                g_object_unref(lbox->normal_reservedscr);
                lbox->normal_reservedscr = NULL;
            }

            if (lbox->disable_reservedscr) {
                g_object_unref(lbox->disable_reservedscr);
                lbox->disable_reservedscr = NULL;
            }           

            if (lbox->normal_reservedrow) {
                g_object_unref(lbox->normal_reservedrow);
                lbox->normal_reservedrow = NULL;
            }

            if (lbox->active_reservedrow) {
                g_object_unref(lbox->active_reservedrow);
                lbox->active_reservedrow = NULL;
            }

            _close_listbox_tooltip(lbox);
        }   
        break;
            
        default:
        break;
    }

    return FALSE;
}

static gboolean _listbox_scroll_press_cb(NFOBJECT *button, GdkEvent *event, gpointer data)
{
    NFLISTBOX *lbox;
    
    switch(event->type) 
    {
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
        {
            GdkEventKey *kevt;
            KEYPAD_KID kpid;

            kevt = (GdkEventKey*)event;
            kpid = (KEYPAD_KID)kevt->keyval;

            if ((kpid == KEYPAD_UP) || (kpid == KEYPAD_DOWN))
            {
                nfui_left_button_signal_emit(button, GDK_BUTTON_PRESS, FALSE);
            }
        }
        break;
        
        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_RELEASE:
        {
            GdkEventKey *kevt;
            KEYPAD_KID kpid;

            kevt = (GdkEventKey*)event;
            kpid = (KEYPAD_KID)kevt->keyval;

            if ((kpid == KEYPAD_UP) || (kpid == KEYPAD_DOWN))
            {
                nfui_left_button_signal_emit(button, GDK_BUTTON_RELEASE, FALSE);
            }
        }
        break;
        
        case GDK_BUTTON_RELEASE:
        {
            gint i;

            if (event->button.button == MOUSE_RIGTH_BUTTON)  break;

            lbox = NF_LISTBOX(button->parent);

            if (lbox->text_row_cnt <= lbox->box_row_cnt) 
            {
                if (lbox->boxInfo[0].index != 1)
                    break;
            }

            if (lbox->down_btn == button) 
            {
                for (i = 0; i < lbox->box_row_cnt; i++) 
                {
                    if((lbox->boxInfo[0].index + lbox->box_row_cnt) > lbox->text_row_cnt) 
                    {
                        --lbox->boxInfo[0].index;
                        return FALSE;   
                    }
                    else 
                    {
                        if (lbox->boxInfo[i].focus) 
                        {
                            if (i == 0) 
                            {
                                lbox->boxInfo[i].focus = FALSE;
                            }                            
                            else if (i != 0) 
                            {
                                lbox->boxInfo[i].focus = FALSE;
                                lbox->boxInfo[i-1].focus = TRUE;
                            }
                        }

                        ++lbox->boxInfo[i].index;
                    }
                }

                _set_listbox_scroll_pos(lbox, 1);
                _move_down_contents_listbox(lbox);
                nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);
            }
            else if (lbox->up_btn == button) 
            {
                for (i = lbox->box_row_cnt-1; i >= 0; i--) 
                {
                    if (lbox->boxInfo[0].index-1 < 0) 
                    {
                        return FALSE;
                    }
                    else 
                    {
                        if (lbox->boxInfo[i].focus) 
                        {
                            if (i == (lbox->box_row_cnt-1)) 
                            {
                                lbox->boxInfo[i].focus = FALSE;
                            }
                            else if (i != (lbox->box_row_cnt-1)) 
                            {
                                lbox->boxInfo[i].focus = FALSE;
                                lbox->boxInfo[i+1].focus = TRUE;
                            }
                        }
                        
                        --lbox->boxInfo[i].index;
                    }
                }
                
                _set_listbox_scroll_pos(lbox, 0);
                _move_up_contents_listbox(lbox);
                nfui_user_signal_emit((NFOBJECT*)lbox, NFEVENT_LISTBOX_CHANGED, FALSE);
            }
        }
        break;
        
        default:
        break;
    }

    return FALSE;
}

static void _put_listbox_scroll(NFLISTBOX *lbox)
{
    GdkDrawable *drawable = NULL;
    guint x, y, w, h;

    nfui_nfobject_get_offset((NFOBJECT*)lbox, &x, &y);
    w = lbox->lbox_fixed.object.width;  
    h = lbox->lbox_fixed.object.height;

    nfui_nffixed_put((NFFIXED*)lbox, lbox->up_btn, w - lbox->up_btn->width, 0);
    nfui_nffixed_put((NFFIXED*)lbox, lbox->down_btn, w - lbox->down_btn->width, h - lbox->down_btn->height);
}

NFOBJECT* nfui_listbox_new(guint column, guint *col_size, guint height)
{
    NFLISTBOX *lbox = NULL;
    gint i = 0;

    lbox = (NFLISTBOX*)imalloc(sizeof(NFLISTBOX));
    
    if(!lbox) { 
        g_warning("NFLISTBOX alloc error...");
        return NULL;
    }

#if 0
    lbox->lbox_fixed.object.parent = NULL;
    lbox->lbox_fixed.object.x = 0;
    lbox->lbox_fixed.object.y = 0;
    lbox->lbox_fixed.object.width = 140;
    lbox->lbox_fixed.object.height = 20;
    lbox->lbox_fixed.object.type = NFOBJECT_TYPE_NFLISTBOX;
    lbox->lbox_fixed.object.show = NFOBJECT_HIDE;
    lbox->lbox_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
    lbox->lbox_fixed.object.kfocus = NFOBJECT_UNFOCUS;
    lbox->lbox_fixed.object.mfocus = NFOBJECT_UNFOCUS;
    lbox->lbox_fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
    lbox->lbox_fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
    lbox->lbox_fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
    lbox->lbox_fixed.object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;
    lbox->lbox_fixed.object.pre_event_handler = NULL;
    lbox->lbox_fixed.object.default_event_handler = _listbox_event_handler;
    lbox->lbox_fixed.object.post_event_handler = NULL;
    lbox->lbox_fixed.object.grab_kfocus = FALSE;
#else
    nfui_nfobject_init((NFOBJECT*)lbox);

    lbox->lbox_fixed.object.width = 140;
    lbox->lbox_fixed.object.height = (guint)(DISPLAY_IS_D1 ? 20:40);
    lbox->lbox_fixed.object.type = NFOBJECT_TYPE_NFLISTBOX;
    lbox->lbox_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
    lbox->lbox_fixed.object.default_event_handler = _listbox_event_handler;
#endif

    lbox->up_btn = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfobject_use_focus(lbox->up_btn, NFOBJECT_FOCUS_OFF);
    nfui_regi_pre_event_callback(lbox->up_btn, (gpointer)_listbox_scroll_press_cb);
    nfui_nfobject_show(lbox->up_btn); 

    lbox->down_btn = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfobject_use_focus(lbox->down_btn, NFOBJECT_FOCUS_OFF);
    nfui_regi_pre_event_callback(lbox->down_btn, (gpointer)_listbox_scroll_press_cb);
    nfui_nfobject_show(lbox->down_btn); 
    
    // SKSHIN
    _put_listbox_scroll(lbox);

    lbox->slist = NULL;
    lbox->boxInfo = NULL;
    lbox->st_scroll.size = (guint)(DISPLAY_IS_D1 ? 20:40);

    lbox->column = column;
    lbox->col_w = g_memdup(col_size, sizeof(guint) * column);
    lbox->row_height = (guint)height;
    lbox->box_row_cnt = 0;
    lbox->text_row_cnt = 0;

    strcpy(lbox->font_name, "Calibri 16");
    lbox->pango_font = lbox->font_name;
    lbox->multi_lang = TRUE;

    for (i = 0; i < SUPP_MAX_COLS; i++)
        lbox->align[i] = NFALIGN_LEFT;        

    lbox->draw_inline = FALSE;
    lbox->draw_outline = FALSE;
    lbox->use_infocus = TRUE;

    lbox->update_data = 1;
   
    return (NFOBJECT*)lbox;
}

void nfui_listbox_set_skin_type(NFLISTBOX *lbox, NFLISTBOX_TYPE type)
{
    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)    
    {
        return;
    }

    if (type == NFLISTBOX_TYPE_1)
        _set_listbox_type_1(lbox);
    else if (type == NFLISTBOX_TYPE_2)
        _set_listbox_type_2(lbox);
    else if (type == NFLISTBOX_TYPE_POPUP_1)
        _set_listbox_type_popup_1(lbox);
    else if (type == NFLISTBOX_TYPE_POPUP_2)
        _set_listbox_type_popup_2(lbox);        
    else if (type == NFLISTBOX_TYPE_SUBTAB_1)
        _set_listbox_type_subtab_1(lbox);       
    else if (type == NFLISTBOX_TYPE_SUBTAB_2)
        _set_listbox_type_subtab_2(lbox);               
}

void nfui_listbox_set_fg_color(NFLISTBOX *lbox, gint *fg_color)
{
    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)    
    {
        return;
    }

    lbox->font_color[NFOBJECT_STATE_NORMAL] = fg_color[NFOBJECT_STATE_NORMAL];
    lbox->font_color[NFOBJECT_STATE_PRELIGHT] = fg_color[NFOBJECT_STATE_PRELIGHT];
    lbox->font_color[NFOBJECT_STATE_ACTIVE] = fg_color[NFOBJECT_STATE_ACTIVE];
    lbox->font_color[NFOBJECT_STATE_DISABLE] = fg_color[NFOBJECT_STATE_DISABLE];    
}

void nfui_listbox_set_bg_color(NFLISTBOX *lbox, gint *bg_color)
{
    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)    
    {
        return;
    }

    lbox->bg_color[NFOBJECT_STATE_NORMAL] = bg_color[NFOBJECT_STATE_NORMAL];
    lbox->bg_color[NFOBJECT_STATE_PRELIGHT] = bg_color[NFOBJECT_STATE_PRELIGHT];
    lbox->bg_color[NFOBJECT_STATE_ACTIVE] = bg_color[NFOBJECT_STATE_ACTIVE];
    lbox->bg_color[NFOBJECT_STATE_DISABLE] = bg_color[NFOBJECT_STATE_DISABLE];        
}

void nfui_listbox_set_arrow_image(NFLISTBOX *lbox, NFScrollType type, GdkPixbuf **image, guint width, guint height)
{
    g_return_if_fail(lbox != NULL);
    g_return_if_fail(SCROLL_DOWN >= type);
    g_return_if_fail(image != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)    
    {
        return;
    }
    
    if (lbox->st_scroll.normal)
    {
        g_object_unref(lbox->st_scroll.normal);
        lbox->st_scroll.normal = NULL;
    }

    if (lbox->st_scroll.focus)
    {
        g_object_unref(lbox->st_scroll.focus);
        lbox->st_scroll.focus = NULL;
    }

    if (lbox->st_scroll.active)
    {
        g_object_unref(lbox->st_scroll.active);
        lbox->st_scroll.active = NULL;
    }    

    lbox->st_scroll.width = width;

    switch(type) 
    {
        case SCROLL_UP:
        {
            nfui_nfbutton_set_image(NF_BUTTON(lbox->up_btn), image);
            nfui_nfobject_set_size(lbox->up_btn, width, height);
            }
        break;
        
        case SCROLL_DOWN:
        {
            nfui_nfbutton_set_image(NF_BUTTON(lbox->down_btn), image);
            nfui_nfobject_set_size(lbox->down_btn, width, height);
            }
        break;
    }
}

void nfui_listbox_set_pango_font(NFLISTBOX *lbox, const gchar *pfont)
{
    g_return_if_fail(pfont);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)    
    {
        return;
    }
    
    if (pfont) 
    {
        if (nffont_is_system_font(pfont))
        {
            lbox->pango_font = pfont;
            }
        else 
        {
            strncpy(lbox->font_name, pfont, sizeof(lbox->font_name));
            lbox->pango_font = lbox->font_name;
        }
    }
}

void nfui_listbox_set_draw_inline(NFLISTBOX *lbox, gboolean draw, guint colorIdx)
{
    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    lbox->draw_inline = draw;
    lbox->inline_color = colorIdx;
}

gint nfui_listbox_set_column_align(NFLISTBOX *lbox, gint col, nfalign_type align)
{
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return -1;
    }

    if (col > lbox->column) return -1;    

    lbox->align[col] = align;
    return 0;
}

void nfui_listbox_support_multi_lang(NFLISTBOX *lbox, gboolean support)
{
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    lbox->multi_lang = support;
}

void nfui_listbox_support_tooltip(NFLISTBOX *lbox, gboolean support)
{
    if(lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return ;   
    }
    lbox->use_tooltip = support;
}

void nfui_listbox_set_text(NFLISTBOX *lbox, gchar *str[])
{
    GSList *new_list = NULL;
    GSList *last_list = NULL;
    GSList *head_list = NULL;
    gint i;

    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    for (i = 0; i < lbox->column; i++)  
    {
        new_list = g_slist_append(new_list, g_strdup(str[i]));
    }

    if (!lbox->slist) 
    {
        lbox->slist = g_slist_append(NULL, new_list);
    }
    else 
    {
        head_list = g_slist_append(NULL, new_list);
        last_list = g_slist_last(lbox->slist);
        last_list->next = head_list;
    }

    lbox->text_row_cnt = g_slist_length(lbox->slist);
    lbox->st_scroll.total_step += 1;

    lbox->update_data = 1;
}

void nfui_listbox_set_prepend_text(NFLISTBOX *lbox, gchar *str[])
{
    GSList *new_list = NULL;
    GSList *last_list = NULL;
    GSList *head_list = NULL;
    gint i;

    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    for (i = 0; i < lbox->column; i++)  
    {
        new_list = g_slist_append(new_list, g_strdup(str[i]));
    }

    lbox->slist = g_slist_prepend(lbox->slist, new_list);
    lbox->text_row_cnt = g_slist_length(lbox->slist);
    lbox->st_scroll.total_step += 1;

    lbox->update_data = 1;
}

void nfui_listbox_set_text_single_column(NFLISTBOX *lbox, gchar *str)
{
    GSList *new_list = NULL;
    GSList *last_list = NULL;
    GSList *head_list = NULL;
    gint i;

    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

   	new_list = g_slist_append(new_list, g_strdup(str));

    if (!lbox->slist) 
    {
        lbox->slist = g_slist_append(NULL, new_list);
    }
    else 
    {
        head_list = g_slist_append(NULL, new_list);
        last_list = g_slist_last(lbox->slist);
        last_list->next = head_list;
    }

    lbox->text_row_cnt = g_slist_length(lbox->slist);
    lbox->st_scroll.total_step += 1;

    lbox->update_data = 1;
}

void nfui_listbox_set_text_with_draw(NFLISTBOX *lbox, gchar *str[])
{
    GSList *new_list = NULL;
    GSList *last_list = NULL;
    GSList *head_list = NULL;
    gint i;

    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    for (i = 0; i < lbox->column; i++)  
    {
        new_list = g_slist_append(new_list, g_strdup(str[i]));
    }

    if (!lbox->slist) 
    {
        lbox->slist = g_slist_append(NULL, new_list);
    }
    else 
    {
        head_list = g_slist_append(NULL, new_list);
        last_list = g_slist_last(lbox->slist);
        last_list->next = head_list;
    }

    lbox->text_row_cnt = g_slist_length(lbox->slist);
    lbox->st_scroll.total_step += 1;

    lbox->update_data = 1;
    _set_listbox_scroll_info(lbox);

    if (g_list_length(lbox->slist) <= lbox->box_row_cnt)
    {
        _draw_listbox_text(lbox, g_slist_length(lbox->slist)-1);
        _draw_listbox_scroll(lbox);
    }
    else
    {
        _draw_listbox_scroll(lbox);
    }
}

void nfui_listbox_modify_text_by_index(NFLISTBOX *lbox, gchar *str[], guint index)
{
    GSList *slist = NULL;
    GSList *tmp = NULL;
    guint i;

    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    slist = g_slist_nth(lbox->slist, index);    
    
    for (i = 0; i < lbox->column; i++) 
    {
        tmp = g_slist_nth((GSList*)slist->data, i); 
        g_free(tmp->data);
        tmp->data = g_strdup(str[i]);
    }
}

void nfui_listbox_set_focus(NFLISTBOX *lbox, guint index, gboolean focus)
{
    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    if (lbox->slist != NULL) 
    {
        lbox->boxInfo[index].focus = focus;
            _draw_listbox(lbox);
    }
}

void nfui_listbox_delete(NFLISTBOX *lbox, guint idx)
{
    guint i;

    g_return_if_fail(lbox != NULL);

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    if (lbox->slist == NULL) return;

    if (lbox->text_row_cnt-1 == idx)
    {
        for (i = 0; i < lbox->box_row_cnt; i++)
        {
            if (lbox->boxInfo[i].index == idx)
            break;
        }

        if (i != lbox->box_row_cnt)
        {
            if (i == 0) 
            {
                lbox->boxInfo[i].focus = FALSE;
            }
            else
            {
                lbox->boxInfo[i].focus = FALSE;
                lbox->boxInfo[i-1].focus = TRUE;                
            }
        }
    }

    lbox->st_scroll.total_step -= 1;

    lbox->slist = g_slist_delete_link(lbox->slist, g_slist_nth(lbox->slist, idx));  
    lbox->text_row_cnt = g_slist_length(lbox->slist);

    lbox->update_data = 1;
}

void nfui_listbox_delete_all(NFLISTBOX *lbox)
{
    gint i;

    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return;
    }

    if (lbox->slist == NULL) return;

    for (i = 0; i < lbox->box_row_cnt; i++) 
    {
        lbox->boxInfo[i].focus = FALSE;
        lbox->boxInfo[i].index = i;
    }

    lbox->st_scroll.cur_step = 0;
    lbox->st_scroll.total_step = 0;

    _delete_listbox_data(lbox);
    lbox->update_data = 1;  
}

gchar* nfui_listbox_get_focus_text(NFLISTBOX *lbox, gint col)
{
    GSList *tlist;
    gchar *text;

    gint i, j;
    gint focus_index = -1;

    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return NULL;
    }
    
    if (lbox->slist) 
    {
        for (i = 0; i < lbox->box_row_cnt; i++) 
        {
            if (lbox->boxInfo[i].focus) 
            {
                focus_index = lbox->boxInfo[i].index;
            }
        }
    }

    if (focus_index == -1) return NULL;     

    tlist = g_slist_nth_data(lbox->slist, focus_index);
    text = (gchar*)g_slist_nth_data(tlist, (guint)col);    

    return text;
}

gint nfui_listbox_get_focus_idx(NFLISTBOX *lbox)
{
    gint i;

    g_return_val_if_fail(lbox != NULL, -1);
    
    if(lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return -1;
    }

    if(lbox->slist) 
    {
        for (i = 0; i < lbox->box_row_cnt; i++) 
        {
            if (lbox->boxInfo[i].focus) 
                return lbox->boxInfo[i].index;
        }
        
        return -1;
    }

    return -1;
}

gint nfui_listbox_get_index_by_box_row(NFLISTBOX *lbox, gint box_row)
{
    gint idx;

    g_return_val_if_fail(lbox != NULL, -1);
    
    if(lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return -1;
    }

    idx = lbox->st_scroll.cur_step + box_row;    

    if (idx > lbox->text_row_cnt) return -1;

    return idx;
}

gint nfui_listbox_get_box_count(NFLISTBOX *lbox)
{
    g_return_val_if_fail(lbox != NULL, 0);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return -1;
    }

    if (lbox->slist == NULL)    return -1;
    
    return g_slist_length(lbox->slist);
}

gchar* nfui_listbox_get_text_of_list(NFLISTBOX *lbox, gint row, gint col)
{
    GSList *tlist;
    gchar *text;

    gint i, j;

    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return NULL;
    }

    if (col > lbox->column)         return NULL;
    if (row >= lbox->text_row_cnt)  return NULL;    

    tlist = g_slist_nth_data(lbox->slist, row);
    text = (gchar*)g_slist_nth_data(tlist, (guint)col);

    return text;
}

gchar* nfui_listbox_get_text_of_box(NFLISTBOX *lbox, gint row, gint col)
{
    GSList *tlist;
    gchar *text;

    gint i, j;

    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return NULL;
    }

    if (col > lbox->column)         return NULL;
    if (row >= lbox->box_row_cnt)   return NULL;    

    row += lbox->st_scroll.cur_step;    
    tlist = g_slist_nth_data(lbox->slist, row);
    text = (gchar*)g_slist_nth_data(tlist, (guint)col);

    return text;
}

void nfui_listbox_set_drawing_outline(NFLISTBOX *lbox, gboolean draw_outline)
{
    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return NULL;
    }

    if(lbox->draw_outline != draw_outline)
        lbox->draw_outline = draw_outline;
}

void nfui_listbox_set_use_infocus_box(NFLISTBOX *lbox, gboolean use)
{
    g_return_if_fail(lbox != NULL);
    
    if (lbox->lbox_fixed.object.type != NFOBJECT_TYPE_NFLISTBOX)
    {
        return NULL;
    }

    lbox->use_infocus = use;
}
