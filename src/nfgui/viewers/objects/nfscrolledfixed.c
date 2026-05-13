
/**********************************************************************************
 *
 *	NFFIXED
 *	
 * *******************************************************************************/

#include "../../support/color.h"
#include "../../support/event_loop.h"
#include "ix_mem.h"

#include "nfbutton.h"
#include "nffixed.h"
#include "nfscrolledfixed.h"


#define MOUSE_LEFT_BUTTON			(1)
#define MOUSE_RIGHT_BUTTON			(3)

#define MOUSE_IN_MARGIN_PIXEL			(16)
#define MOUSE_OUT_MARGIN_PIXEL			(16)

#define _MIN(a, b)	(a > b ? b : a)
#define _MAX(a, b)	(a > b ? a : b)

#define COLOR2INT(a) ((a.red >> 8) << 24 | (a.green >> 8) << 16 | (a.blue >> 8) << 8 | 0xff)


static void _set_scrolledfixed_type_1(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_left[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_right[NFOBJECT_STATE_COUNT];	
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

	scroll_left[0] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);
	scroll_left[1] = nfui_get_image_from_file((IMG_O_SCROLL_LEFT), NULL);	
	scroll_left[2] = nfui_get_image_from_file((IMG_P_SCROLL_LEFT), NULL);	
	scroll_left[3] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);	

	scroll_right[0] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);
	scroll_right[1] = nfui_get_image_from_file((IMG_O_SCROLL_RIGHT), NULL);
	scroll_right[2] = nfui_get_image_from_file((IMG_P_SCROLL_RIGHT), NULL);
	scroll_right[3] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->up_btn), scroll_up);
	nfui_nfobject_set_size(scrolled_fixed->up_btn, size_w, size_h);
	
	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->down_btn), scroll_down);
	nfui_nfobject_set_size(scrolled_fixed->down_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->left_btn), scroll_left);
	nfui_nfobject_set_size(scrolled_fixed->left_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->right_btn), scroll_right);
	nfui_nfobject_set_size(scrolled_fixed->right_btn, size_w, size_h);	

	scrolled_fixed->skin_type = NFSCROLLEDFIXED_TYPE_1;
	scrolled_fixed->vscroll_info.bar_width = size_w;	
	scrolled_fixed->hscroll_info.bar_height = size_h;	
}

static void _set_scrolledfixed_type_2(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_left[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_right[NFOBJECT_STATE_COUNT];		
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

	scroll_left[0] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);
	scroll_left[1] = nfui_get_image_from_file((IMG_O_SCROLL_LEFT), NULL);	
	scroll_left[2] = nfui_get_image_from_file((IMG_P_SCROLL_LEFT), NULL);	
	scroll_left[3] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);	

	scroll_right[0] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);
	scroll_right[1] = nfui_get_image_from_file((IMG_O_SCROLL_RIGHT), NULL);
	scroll_right[2] = nfui_get_image_from_file((IMG_P_SCROLL_RIGHT), NULL);
	scroll_right[3] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->up_btn), scroll_up);
	nfui_nfobject_set_size(scrolled_fixed->up_btn, size_w, size_h);
	
	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->down_btn), scroll_down);
	nfui_nfobject_set_size(scrolled_fixed->down_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->left_btn), scroll_left);
	nfui_nfobject_set_size(scrolled_fixed->left_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->right_btn), scroll_right);
	nfui_nfobject_set_size(scrolled_fixed->right_btn, size_w, size_h);	

	scrolled_fixed->skin_type = NFSCROLLEDFIXED_TYPE_2;
	scrolled_fixed->vscroll_info.bar_width = size_w;	
	scrolled_fixed->hscroll_info.bar_height = size_h;	
}

static void _set_scrolledfixed_type_popup_1(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_left[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_right[NFOBJECT_STATE_COUNT];		
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

	scroll_left[0] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);
	scroll_left[1] = nfui_get_image_from_file((IMG_O_SCROLL_LEFT), NULL);	
	scroll_left[2] = nfui_get_image_from_file((IMG_P_SCROLL_LEFT), NULL);	
	scroll_left[3] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);	

	scroll_right[0] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);
	scroll_right[1] = nfui_get_image_from_file((IMG_O_SCROLL_RIGHT), NULL);
	scroll_right[2] = nfui_get_image_from_file((IMG_P_SCROLL_RIGHT), NULL);
	scroll_right[3] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->up_btn), scroll_up);
	nfui_nfobject_set_size(scrolled_fixed->up_btn, size_w, size_h);
	
	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->down_btn), scroll_down);
	nfui_nfobject_set_size(scrolled_fixed->down_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->left_btn), scroll_left);
	nfui_nfobject_set_size(scrolled_fixed->left_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->right_btn), scroll_right);
	nfui_nfobject_set_size(scrolled_fixed->right_btn, size_w, size_h);	

	scrolled_fixed->skin_type = NFSCROLLEDFIXED_TYPE_POPUP_1;
	scrolled_fixed->vscroll_info.bar_width = size_w;	
	scrolled_fixed->hscroll_info.bar_height = size_h;		
}

static void _set_scrolledfixed_type_popup_2(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_left[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_right[NFOBJECT_STATE_COUNT];		
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

	scroll_left[0] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);
	scroll_left[1] = nfui_get_image_from_file((IMG_O_SCROLL_LEFT), NULL);	
	scroll_left[2] = nfui_get_image_from_file((IMG_P_SCROLL_LEFT), NULL);	
	scroll_left[3] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);	

	scroll_right[0] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);
	scroll_right[1] = nfui_get_image_from_file((IMG_O_SCROLL_RIGHT), NULL);
	scroll_right[2] = nfui_get_image_from_file((IMG_P_SCROLL_RIGHT), NULL);
	scroll_right[3] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->up_btn), scroll_up);
	nfui_nfobject_set_size(scrolled_fixed->up_btn, size_w, size_h);
	
	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->down_btn), scroll_down);
	nfui_nfobject_set_size(scrolled_fixed->down_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->left_btn), scroll_left);
	nfui_nfobject_set_size(scrolled_fixed->left_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->right_btn), scroll_right);
	nfui_nfobject_set_size(scrolled_fixed->right_btn, size_w, size_h);	

	scrolled_fixed->skin_type = NFSCROLLEDFIXED_TYPE_POPUP_2;
	scrolled_fixed->vscroll_info.bar_width = size_w;	
	scrolled_fixed->hscroll_info.bar_height = size_h;		
}

static void _set_scrolledfixed_type_subtab_1(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_left[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_right[NFOBJECT_STATE_COUNT];		
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

	scroll_left[0] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);
	scroll_left[1] = nfui_get_image_from_file((IMG_O_SCROLL_LEFT), NULL);	
	scroll_left[2] = nfui_get_image_from_file((IMG_P_SCROLL_LEFT), NULL);	
	scroll_left[3] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);	

	scroll_right[0] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);
	scroll_right[1] = nfui_get_image_from_file((IMG_O_SCROLL_RIGHT), NULL);
	scroll_right[2] = nfui_get_image_from_file((IMG_P_SCROLL_RIGHT), NULL);
	scroll_right[3] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->up_btn), scroll_up);
	nfui_nfobject_set_size(scrolled_fixed->up_btn, size_w, size_h);
	
	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->down_btn), scroll_down);
	nfui_nfobject_set_size(scrolled_fixed->down_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->left_btn), scroll_left);
	nfui_nfobject_set_size(scrolled_fixed->left_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->right_btn), scroll_right);
	nfui_nfobject_set_size(scrolled_fixed->right_btn, size_w, size_h);	

	scrolled_fixed->skin_type = NFSCROLLEDFIXED_TYPE_SUBTAB_1;
	scrolled_fixed->vscroll_info.bar_width = size_w;	
	scrolled_fixed->hscroll_info.bar_height = size_h;		
}

static void _set_scrolledfixed_type_subtab_2(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_left[NFOBJECT_STATE_COUNT];	
	GdkPixbuf *scroll_right[NFOBJECT_STATE_COUNT];		
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

	scroll_left[0] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);
	scroll_left[1] = nfui_get_image_from_file((IMG_O_SCROLL_LEFT), NULL);	
	scroll_left[2] = nfui_get_image_from_file((IMG_P_SCROLL_LEFT), NULL);	
	scroll_left[3] = nfui_get_image_from_file((IMG_N_SCROLL_LEFT), NULL);	

	scroll_right[0] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);
	scroll_right[1] = nfui_get_image_from_file((IMG_O_SCROLL_RIGHT), NULL);
	scroll_right[2] = nfui_get_image_from_file((IMG_P_SCROLL_RIGHT), NULL);
	scroll_right[3] = nfui_get_image_from_file((IMG_N_SCROLL_RIGHT), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->up_btn), scroll_up);
	nfui_nfobject_set_size(scrolled_fixed->up_btn, size_w, size_h);
	
	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->down_btn), scroll_down);
	nfui_nfobject_set_size(scrolled_fixed->down_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->left_btn), scroll_left);
	nfui_nfobject_set_size(scrolled_fixed->left_btn, size_w, size_h);

	nfui_nfbutton_set_image(NF_BUTTON(scrolled_fixed->right_btn), scroll_right);
	nfui_nfobject_set_size(scrolled_fixed->right_btn, size_w, size_h);	

	scrolled_fixed->skin_type = NFSCROLLEDFIXED_TYPE_SUBTAB_2;
	scrolled_fixed->vscroll_info.bar_width = size_w;	
	scrolled_fixed->hscroll_info.bar_height = size_h;		
}

static gint _is_toplist_child(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *child)
{
	GdkRectangle parent_rect;
	GdkRectangle child_rect;

	gint x_res = 0, y_res = 0;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &parent_rect.x, &parent_rect.y);
	nfui_nfobject_get_offset((NFOBJECT*)child, &child_rect.x, &child_rect.y);

	child_rect.x -= (scrolled_fixed->relative_x - parent_rect.x);
	child_rect.y -= (scrolled_fixed->relative_y - parent_rect.y);
	child_rect.width = child->width;
	child_rect.height = child->height;

	if (scrolled_fixed->use_hscroll) {
		if (child == scrolled_fixed->left_btn) return 1;
		
		parent_rect.width = ((NFOBJECT*)scrolled_fixed)->width/2;
		parent_rect.height = ((NFOBJECT*)scrolled_fixed)->height;
	}
	
	if (scrolled_fixed->use_vscroll) {
		if (child == scrolled_fixed->up_btn) return 1;
		
		parent_rect.width = ((NFOBJECT*)scrolled_fixed)->width;
		parent_rect.height = ((NFOBJECT*)scrolled_fixed)->height/2;
	}

#if 1
    if (child_rect.x <= parent_rect.x)
    {
        if (child_rect.x + child_rect.width >= parent_rect.x) x_res = 1;
    }
    else
    {
        if (child_rect.x <= parent_rect.x + parent_rect.width) x_res = 1;
    }

    if (child_rect.y <= parent_rect.y)   
    {
        if (child_rect.y + child_rect.height >= parent_rect.y) y_res = 1;
    }
    else
    {
        if (child_rect.y <= parent_rect.y + parent_rect.height) y_res = 1;
    }

	if (x_res && y_res) return 1;
#else
    if ((child_rect.x >= parent_rect.x) && (child_rect.x <= parent_rect.x + parent_rect.width) 
		&& (child_rect.y >= parent_rect.y) && (child_rect.y <= parent_rect.y + parent_rect.height))
	{
		return 1;
	}

	if ((child_rect.x + child_rect.width >= parent_rect.x) && (child_rect.x + child_rect.width <= parent_rect.x + parent_rect.width)
		&& (child_rect.y + child_rect.height >= parent_rect.y) && (child_rect.y + child_rect.height <= parent_rect.y + parent_rect.height))
	{
		return 1;
	}
#endif
	return 0;
}

static gint _is_bottomlist_child(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *child)
{
	GdkRectangle parent_rect;
	GdkRectangle child_rect;

	gint x_res = 0, y_res = 0;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &parent_rect.x, &parent_rect.y);
	nfui_nfobject_get_offset((NFOBJECT*)child, &child_rect.x, &child_rect.y);

	child_rect.x -= (scrolled_fixed->relative_x - parent_rect.x);
	child_rect.y -= (scrolled_fixed->relative_y - parent_rect.y);
	child_rect.width = child->width;
	child_rect.height = child->height;

	if (scrolled_fixed->use_hscroll) {
		if (child == scrolled_fixed->right_btn) return 1;
		
		parent_rect.x += ((NFOBJECT*)scrolled_fixed)->width/2;
		parent_rect.width = ((NFOBJECT*)scrolled_fixed)->width/2;
		parent_rect.height = ((NFOBJECT*)scrolled_fixed)->height;
	}

	if (scrolled_fixed->use_vscroll){
		if (child == scrolled_fixed->down_btn) return 1;
		
		parent_rect.y += ((NFOBJECT*)scrolled_fixed)->height/2;
		parent_rect.width = ((NFOBJECT*)scrolled_fixed)->width;
		parent_rect.height = (((NFOBJECT*)scrolled_fixed))->height/2;
	}

#if 1
    if (child_rect.x <= parent_rect.x)
    {
        if (child_rect.x + child_rect.width >= parent_rect.x) x_res = 1;
    }
    else
    {
        if (child_rect.x <= parent_rect.x + parent_rect.width) x_res = 1;
    }

    if (child_rect.y <= parent_rect.y)   
    {
        if (child_rect.y + child_rect.height >= parent_rect.y) y_res = 1;
    }
    else
    {
        if (child_rect.y <= parent_rect.y + parent_rect.height) y_res = 1;
    }

	if (x_res && y_res) return 1;
#else
    if ((child_rect.x >= parent_rect.x) && (child_rect.x <= parent_rect.x + parent_rect.width) 
		&& (child_rect.y >= parent_rect.y) && (child_rect.y <= parent_rect.y + parent_rect.height))
	{
		return 1;
	}

	if ((child_rect.x + child_rect.width >= parent_rect.x) && (child_rect.x + child_rect.width <= parent_rect.x + parent_rect.width)
		&& (child_rect.y + child_rect.height >= parent_rect.y) && (child_rect.y + child_rect.height <= parent_rect.y + parent_rect.height))
	{
		return 1;
	}
#endif

	return 0;
}

static gint _make_show_child_list(NFSCROLLEDFIXED *scrolled_fixed)
{
	guint i, child_num;
	NFOBJECT *p;

	gint pos_x, pos_y;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &pos_x, &pos_y);

	if(scrolled_fixed && scrolled_fixed->children)
		g_slist_free(scrolled_fixed->children);

	if(scrolled_fixed && scrolled_fixed->top_keylist)
		g_slist_free(scrolled_fixed->top_keylist);
		
	if(scrolled_fixed && scrolled_fixed->bottom_keylist)
		g_slist_free(scrolled_fixed->bottom_keylist);

	scrolled_fixed->children = 0;
	scrolled_fixed->top_keylist = 0;
	scrolled_fixed->bottom_keylist = 0;

	child_num = g_slist_length(scrolled_fixed->childrenfull);

	for(i=0; i<child_num; i++)
	{
		p = g_slist_nth_data(scrolled_fixed->childrenfull, i);
		if (_is_toplist_child(scrolled_fixed, p) || _is_bottomlist_child(scrolled_fixed, p))
		{
			scrolled_fixed->children = g_slist_append(scrolled_fixed->children, p);
		}
	}

	child_num = g_slist_length(scrolled_fixed->keylistfull);
	
//	g_message("%s, %d, up_btn addr : %p", __FUNCTION__, __LINE__, scrolled_fixed->up_btn);
//	g_message("%s, %d, down_btn addr : %p", __FUNCTION__, __LINE__, scrolled_fixed->down_btn);
//	g_message("%s, %d, left_btn addr : %p", __FUNCTION__, __LINE__, scrolled_fixed->left_btn);
//	g_message("%s, %d, right_btn addr : %p", __FUNCTION__, __LINE__, scrolled_fixed->right_btn);

	for(i=0; i<child_num; i++)
	{
		p = g_slist_nth_data(scrolled_fixed->keylistfull, i);
//    	g_message("%s, %d, p addr : %p", __FUNCTION__, __LINE__, p);

		if (_is_toplist_child(scrolled_fixed, p))
		{
//        	g_message("%s, %d, p is TOP!!!", __FUNCTION__, __LINE__, p);
			scrolled_fixed->top_keylist = g_slist_append(scrolled_fixed->top_keylist, p);
		}
		if (_is_bottomlist_child(scrolled_fixed, p))
		{
//        	g_message("%s, %d, p is BOTTOM!!!", __FUNCTION__, __LINE__, p);
			scrolled_fixed->bottom_keylist = g_slist_append(scrolled_fixed->bottom_keylist, p);
		}		
	}	

//	g_message("%s, %d, childrenfull:%d", __FUNCTION__, __LINE__, g_slist_length(scrolled_fixed->childrenfull));
//	g_message("%s, %d, children:%d", __FUNCTION__, __LINE__, g_slist_length(scrolled_fixed->children));

//	g_message("%s, %d, keylistfull:%d", __FUNCTION__, __LINE__, g_slist_length(scrolled_fixed->keylistfull));		
//	g_message("%s, %d, top_keylist:%d", __FUNCTION__, __LINE__, g_slist_length(scrolled_fixed->top_keylist));	
//	g_message("%s, %d, bottom_keylist:%d", __FUNCTION__, __LINE__, g_slist_length(scrolled_fixed->bottom_keylist));	

	return 0;
}

static gint _set_vertical_scrollbar_child_position(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkDrawable *drawable = NULL;
	guint w, h;

	if (!scrolled_fixed->use_vscroll) return -1;

	w = ((NFOBJECT*)scrolled_fixed)->width - scrolled_fixed->realscr_vmargin;	
	h = ((NFOBJECT*)scrolled_fixed)->height - scrolled_fixed->realscr_hmargin;	

	((NFOBJECT*)scrolled_fixed->up_btn)->x = w + MOUSE_IN_MARGIN_PIXEL;
	((NFOBJECT*)scrolled_fixed->up_btn)->y = 0;

	((NFOBJECT*)scrolled_fixed->down_btn)->x = w + MOUSE_IN_MARGIN_PIXEL;
	((NFOBJECT*)scrolled_fixed->down_btn)->y = h - scrolled_fixed->down_btn->height;

	return 0;
}

static gint _create_vertical_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *bar_img[4][3];
	gint barbg_h;
	gint img_w, img_h;
	gint i;
	GdkColor bg_color;

	gint realscr_h;

	if (!scrolled_fixed->use_vscroll) return -1;

	//bg_color = UX_COLOR(((NFOBJECT*)scrolled_fixed)->bg_color[NFOBJECT_STATE_NORMAL]);
	bg_color = UX_COLOR(COLOR_PRG_IDX(UX_COLOR_5C6D82));

	scrolled_fixed->vscroll_info.bar_state = NFOBJECT_STATE_NORMAL;	

	scrolled_fixed->vscroll_info.bar_x = 0;
	scrolled_fixed->vscroll_info.bar_y = 1;

	realscr_h = ((NFOBJECT*)scrolled_fixed)->height - scrolled_fixed->realscr_hmargin;
	
	barbg_h = realscr_h - scrolled_fixed->up_btn->height - scrolled_fixed->down_btn->height;

	scrolled_fixed->vscroll_info.bg = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scrolled_fixed->vscroll_info.bar_width, barbg_h);
	gdk_pixbuf_fill(scrolled_fixed->vscroll_info.bg, COLOR2INT(bg_color));

	barbg_h -= 2;

	if (realscr_h >= scrolled_fixed->scrolledscr_height)
	{
		scrolled_fixed->vscroll_info.pps = 1;
	}
	else
	{
		scrolled_fixed->vscroll_info.pps = (float)realscr_h / (float)scrolled_fixed->scrolledscr_height;
	}

	scrolled_fixed->vscroll_info.bar_height =  (gint)((float)barbg_h * scrolled_fixed->vscroll_info.pps);

	scrolled_fixed->vscroll_info.pps = (float)(barbg_h - scrolled_fixed->vscroll_info.bar_height) / (float)(scrolled_fixed->scrolledscr_height - realscr_h);

	for (i = 0; i < 4; i++)
	{
		scrolled_fixed->vscroll_info.bar[i] = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scrolled_fixed->vscroll_info.bar_width, scrolled_fixed->vscroll_info.bar_height);
		gdk_pixbuf_fill(scrolled_fixed->vscroll_info.bar[i], 0x000000);
	}

	bar_img[0][0] = nfui_get_image_from_file((IMG_N_SCROLLBAR_TOP), NULL);
	bar_img[0][1] = nfui_get_image_from_file((IMG_N_SCROLLBAR_MID), NULL);
	bar_img[0][2] = nfui_get_image_from_file((IMG_N_SCROLLBAR_BOT), NULL);

	bar_img[1][0] = nfui_get_image_from_file((IMG_O_SCROLLBAR_TOP), NULL);
	bar_img[1][1] = nfui_get_image_from_file((IMG_O_SCROLLBAR_MID), NULL);
	bar_img[1][2] = nfui_get_image_from_file((IMG_O_SCROLLBAR_BOT), NULL);

	bar_img[2][0] = nfui_get_image_from_file((IMG_P_SCROLLBAR_TOP), NULL);
	bar_img[2][1] = nfui_get_image_from_file((IMG_P_SCROLLBAR_MID), NULL);
	bar_img[2][2] = nfui_get_image_from_file((IMG_P_SCROLLBAR_BOT), NULL);

//	bar[3][0] = nfui_get_image_from_file((IMG_D_SCROLLBAR_TOP), NULL);
//	bar[3][1] = nfui_get_image_from_file((IMG_D_SCROLLBAR_MID), NULL);
//	bar[3][2] = nfui_get_image_from_file((IMG_D_SCROLLBAR_BOT), NULL);

	bar_img[3][0] = nfui_get_image_from_file((IMG_N_SCROLLBAR_TOP), NULL);
	bar_img[3][1] = nfui_get_image_from_file((IMG_N_SCROLLBAR_MID), NULL);
	bar_img[3][2] = nfui_get_image_from_file((IMG_N_SCROLLBAR_BOT), NULL);

	nfui_get_pixbuf_size(bar_img[0][0], &img_w, &img_h);

	for (i = 0; i < 4; i++)
	{
		gdk_pixbuf_composite(bar_img[i][0], scrolled_fixed->vscroll_info.bar[i], 0, 0, img_w, img_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite(bar_img[i][1], scrolled_fixed->vscroll_info.bar[i], 0, img_h, img_w, (scrolled_fixed->vscroll_info.bar_height - (img_h * 2)), 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite(bar_img[i][2], scrolled_fixed->vscroll_info.bar[i], 0, (img_h + (scrolled_fixed->vscroll_info.bar_height - (img_h * 2))), img_w, img_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	}

	return 0;	
}

static gint _change_state_vertical_scrollbar(NFSCROLLEDFIXED *scrolled_fixed, gint mx, gint my, gint press)
{
	gint off_x, off_y;
	guint left, right, top, bottom;
	gint new_state = NFOBJECT_STATE_NORMAL;

	if (!scrolled_fixed->vscroll_info.bg) return -1;
	if (scrolled_fixed->vscroll_info.bar_state == -1) return -1;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);
	
	left = off_x + ((NFOBJECT*)scrolled_fixed)->width - scrolled_fixed->vscroll_info.bar_width - MOUSE_OUT_MARGIN_PIXEL;
	top = off_y + scrolled_fixed->up_btn->height + scrolled_fixed->vscroll_info.bar_y;
	right = left + scrolled_fixed->vscroll_info.bar_width;
	bottom = top + scrolled_fixed->vscroll_info.bar_height;

	if (mx>=left && mx<right && my>=top && my<bottom)
	{
		if (press) new_state = NFOBJECT_STATE_ACTIVE;
		else new_state = NFOBJECT_STATE_PRELIGHT;
	}

	if (scrolled_fixed->vscroll_info.bar_state == new_state) return -1;

	scrolled_fixed->vscroll_info.bar_state = new_state;
	return 0;
}

static gint _change_position_vertical_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	gint off_x, off_y;
	gint bg_width, bg_height;

	bg_width = gdk_pixbuf_get_width(scrolled_fixed->vscroll_info.bg);
	bg_height = gdk_pixbuf_get_height(scrolled_fixed->vscroll_info.bg);
	
	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);
	scrolled_fixed->vscroll_info.bar_y = (gint)((float)(scrolled_fixed->relative_y-off_y) * (float)scrolled_fixed->vscroll_info.pps);
	scrolled_fixed->vscroll_info.bar_y += 1;

	scrolled_fixed->vscroll_info.bar_y = MIN(scrolled_fixed->vscroll_info.bar_y, bg_height-scrolled_fixed->vscroll_info.bar_height-1);
	
	return 0;
}

static gint _composite_vertical_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	gint bg_width, bg_height;
	gint bar_state;
	
	if (!scrolled_fixed->vscroll_info.bg) return -1;
	if (scrolled_fixed->vscroll_info.bar_state == -1) return -1;
	if (scrolled_fixed->vscroll) g_object_unref(scrolled_fixed->vscroll);

	bg_width = gdk_pixbuf_get_width(scrolled_fixed->vscroll_info.bg);
	bg_height = gdk_pixbuf_get_height(scrolled_fixed->vscroll_info.bg);

	bar_state = scrolled_fixed->vscroll_info.bar_state;
	
	scrolled_fixed->vscroll = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, bg_width, bg_height);
	gdk_pixbuf_composite(scrolled_fixed->vscroll_info.bg, scrolled_fixed->vscroll, 0, 0, bg_width, bg_height, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(scrolled_fixed->vscroll_info.bar[bar_state], scrolled_fixed->vscroll, 
		scrolled_fixed->vscroll_info.bar_x, scrolled_fixed->vscroll_info.bar_y, scrolled_fixed->vscroll_info.bar_width, scrolled_fixed->vscroll_info.bar_height, 
		scrolled_fixed->vscroll_info.bar_x, scrolled_fixed->vscroll_info.bar_y, 1, 1, GDK_INTERP_BILINEAR, 255);

	return 0;
}

static gint _draw_vertical_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	NFOBJECT *top_obj;
	GdkDrawable *drawable = NULL;
	GdkGC *scroll_gc;
	gint off_x, off_y;
	gint realscr_h;

	if (!nfui_nfobject_is_shown(scrolled_fixed->up_btn)) return -1;
	if (!scrolled_fixed->vscroll_info.bar) return -1;
	
	top_obj = nfui_nfobject_get_top((NFOBJECT*)scrolled_fixed);
	drawable = nfui_nfobject_get_window((NFOBJECT*)top_obj);

	if (!drawable) return -1;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);
	off_x += (((NFOBJECT*)scrolled_fixed)->width - scrolled_fixed->realscr_vmargin);

	realscr_h = ((NFOBJECT*)scrolled_fixed)->height - scrolled_fixed->realscr_hmargin;

	scroll_gc = nfui_nfobject_get_gc((NFOBJECT*)top_obj);	

	gdk_gc_set_rgb_fg_color(scroll_gc, &UX_COLOR(((NFOBJECT*)scrolled_fixed)->bg_color[NFOBJECT_STATE_NORMAL]));
	gdk_draw_rectangle(drawable, scroll_gc, TRUE, off_x, off_y, MOUSE_IN_MARGIN_PIXEL, realscr_h);

	off_x += MOUSE_IN_MARGIN_PIXEL;
	off_y += scrolled_fixed->up_btn->height;
	nfutil_draw_pixbuf(drawable, scroll_gc, scrolled_fixed->vscroll, off_x, off_y, -1, -1, NFALIGN_LEFT, 0);	
	nfui_nfobject_gc_unref(scroll_gc);

	return 0;
}

static gint _create_scrolledscr(NFSCROLLEDFIXED *scrolled_fixed)
{
	NFOBJECT *top_win;
	GdkDrawable *drawable;
	GdkGC *gc;
	gint off_x, off_y;
	gint scrolled_width, scrolled_height;

	if ((scrolled_fixed->use_vscroll == 1) || (scrolled_fixed->use_hscroll == 1))
	{
		top_win = nfui_nfobject_get_top((NFOBJECT*)scrolled_fixed);
		drawable = nfui_nfobject_get_window(top_win);
		
		scrolled_fixed->scrolledscr = gdk_pixmap_new(drawable, scrolled_fixed->scrolledscr_width, scrolled_fixed->scrolledscr_height, -1);

		gc = gdk_gc_new(scrolled_fixed->scrolledscr);
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(((NFOBJECT*)scrolled_fixed)->bg_color[NFOBJECT_STATE_NORMAL]));
		gdk_draw_rectangle(scrolled_fixed->scrolledscr, gc, TRUE, 0, 0, scrolled_fixed->scrolledscr_width, scrolled_fixed->scrolledscr_height);
		g_object_unref(gc);
	}	

	return 0;
}

static gint _set_horizontal_scrollbar_child_position(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkDrawable *drawable = NULL;
	guint w, h;

	if (!scrolled_fixed->use_hscroll) return -1;

	w = ((NFOBJECT*)scrolled_fixed)->width - scrolled_fixed->realscr_vmargin;	
	h = ((NFOBJECT*)scrolled_fixed)->height - scrolled_fixed->realscr_hmargin;	

	((NFOBJECT*)scrolled_fixed->left_btn)->x = 0;
	((NFOBJECT*)scrolled_fixed->left_btn)->y = h + MOUSE_IN_MARGIN_PIXEL;

	((NFOBJECT*)scrolled_fixed->right_btn)->x = w - scrolled_fixed->right_btn->width;
	((NFOBJECT*)scrolled_fixed->right_btn)->y = h + MOUSE_IN_MARGIN_PIXEL;

	return 0;
}

static gint _create_horizontal_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	GdkPixbuf *bar_img[4][3];
	gint barbg_w;
	gint img_w, img_h;
	gint i;
	GdkColor bg_color;

	gint realscr_w;

	if (!scrolled_fixed->use_hscroll) return -1;

	//bg_color = UX_COLOR(((NFOBJECT*)scrolled_fixed)->bg_color[NFOBJECT_STATE_NORMAL]);
	bg_color = UX_COLOR(COLOR_PRG_IDX(UX_COLOR_5C6D82));

	scrolled_fixed->hscroll_info.bar_state = NFOBJECT_STATE_NORMAL;	

	scrolled_fixed->hscroll_info.bar_x = 1;
	scrolled_fixed->hscroll_info.bar_y = 0;

	realscr_w = ((NFOBJECT*)scrolled_fixed)->width - scrolled_fixed->realscr_vmargin;
	
	barbg_w = realscr_w - scrolled_fixed->left_btn->width - scrolled_fixed->right_btn->width;

	scrolled_fixed->hscroll_info.bg = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, barbg_w, scrolled_fixed->hscroll_info.bar_height);
	gdk_pixbuf_fill(scrolled_fixed->hscroll_info.bg, COLOR2INT(bg_color));

	barbg_w -= 2;

	if (realscr_w >= scrolled_fixed->scrolledscr_width)
	{
		scrolled_fixed->hscroll_info.pps = 1;
	}
	else
	{
		scrolled_fixed->hscroll_info.pps = (float)realscr_w / (float)scrolled_fixed->scrolledscr_width;
	}

	scrolled_fixed->hscroll_info.bar_width =  (gint)((float)barbg_w * scrolled_fixed->hscroll_info.pps);

	scrolled_fixed->hscroll_info.pps = (float)(barbg_w - scrolled_fixed->hscroll_info.bar_width) / (float)(scrolled_fixed->scrolledscr_width - realscr_w);

	for (i = 0; i < 4; i++)
	{
		scrolled_fixed->hscroll_info.bar[i] = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scrolled_fixed->hscroll_info.bar_width, scrolled_fixed->hscroll_info.bar_height);
		gdk_pixbuf_fill(scrolled_fixed->hscroll_info.bar[i], 0x000000);
	}

	bar_img[0][0] = nfui_get_image_from_file((IMG_N_SCROLLBAR_LEFT), NULL);
	bar_img[0][1] = nfui_get_image_from_file((IMG_N_SCROLLBAR_MID2), NULL);
	bar_img[0][2] = nfui_get_image_from_file((IMG_N_SCROLLBAR_RIGHT), NULL);

	bar_img[1][0] = nfui_get_image_from_file((IMG_O_SCROLLBAR_LEFT), NULL);
	bar_img[1][1] = nfui_get_image_from_file((IMG_O_SCROLLBAR_MID2), NULL);
	bar_img[1][2] = nfui_get_image_from_file((IMG_O_SCROLLBAR_RIGHT), NULL);

	bar_img[2][0] = nfui_get_image_from_file((IMG_P_SCROLLBAR_LEFT), NULL);
	bar_img[2][1] = nfui_get_image_from_file((IMG_P_SCROLLBAR_MID2), NULL);
	bar_img[2][2] = nfui_get_image_from_file((IMG_P_SCROLLBAR_RIGHT), NULL);

//	bar[3][0] = nfui_get_image_from_file((IMG_D_SCROLLBAR_TOP), NULL);
//	bar[3][1] = nfui_get_image_from_file((IMG_D_SCROLLBAR_MID), NULL);
//	bar[3][2] = nfui_get_image_from_file((IMG_D_SCROLLBAR_BOT), NULL);

	bar_img[3][0] = nfui_get_image_from_file((IMG_N_SCROLLBAR_LEFT), NULL);
	bar_img[3][1] = nfui_get_image_from_file((IMG_N_SCROLLBAR_MID2), NULL);
	bar_img[3][2] = nfui_get_image_from_file((IMG_N_SCROLLBAR_RIGHT), NULL);

	nfui_get_pixbuf_size(bar_img[0][0], &img_w, &img_h);

	for (i = 0; i < 4; i++)
	{
		gdk_pixbuf_composite(bar_img[i][0], scrolled_fixed->hscroll_info.bar[i], 0, 0, img_w, img_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite(bar_img[i][1], scrolled_fixed->hscroll_info.bar[i], img_w, 0, (scrolled_fixed->hscroll_info.bar_width - (img_w * 2)), img_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite(bar_img[i][2], scrolled_fixed->hscroll_info.bar[i], (img_w + (scrolled_fixed->hscroll_info.bar_width - (img_w * 2))), 0, img_w, img_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	}

	return 0;	
}

static gint _change_state_horizontal_scrollbar(NFSCROLLEDFIXED *scrolled_fixed, gint mx, gint my, gint press)
{
	gint off_x, off_y;
	guint left, right, top, bottom;
	gint new_state = NFOBJECT_STATE_NORMAL;

	if (!scrolled_fixed->hscroll_info.bg) return -1;
	if (scrolled_fixed->hscroll_info.bar_state == -1) return -1;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);
	
	left = off_x + scrolled_fixed->left_btn->width + scrolled_fixed->hscroll_info.bar_x;
	top = off_y + ((NFOBJECT*)scrolled_fixed)->height - scrolled_fixed->hscroll_info.bar_height - MOUSE_OUT_MARGIN_PIXEL;
	right = left + scrolled_fixed->hscroll_info.bar_width;
	bottom = top + scrolled_fixed->hscroll_info.bar_height;

	if (mx>=left && mx<right && my>=top && my<bottom)
	{
		if (press) new_state = NFOBJECT_STATE_ACTIVE;
		else new_state = NFOBJECT_STATE_PRELIGHT;
	}

	if (scrolled_fixed->hscroll_info.bar_state == new_state) return -1;

	scrolled_fixed->hscroll_info.bar_state = new_state;
	return 0;
}

static gint _change_position_horizontal_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	gint off_x, off_y;
	gint bg_width, bg_height;

	bg_width = gdk_pixbuf_get_width(scrolled_fixed->hscroll_info.bg);
	bg_height = gdk_pixbuf_get_height(scrolled_fixed->hscroll_info.bg);
	
	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);
	scrolled_fixed->hscroll_info.bar_x = (gint)((float)(scrolled_fixed->relative_x-off_x) * (float)scrolled_fixed->hscroll_info.pps);
	scrolled_fixed->hscroll_info.bar_x += 1;

	scrolled_fixed->hscroll_info.bar_x = MIN(scrolled_fixed->hscroll_info.bar_x, bg_width-scrolled_fixed->hscroll_info.bar_width-1);
	
	return 0;
}

static gint _composite_horizontal_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	gint bg_width, bg_height;
	gint bar_state;
	
	if (!scrolled_fixed->hscroll_info.bg) return -1;
	if (scrolled_fixed->hscroll_info.bar_state == -1) return -1;
	if (scrolled_fixed->hscroll) g_object_unref(scrolled_fixed->hscroll);

	bg_width = gdk_pixbuf_get_width(scrolled_fixed->hscroll_info.bg);
	bg_height = gdk_pixbuf_get_height(scrolled_fixed->hscroll_info.bg);

	bar_state = scrolled_fixed->hscroll_info.bar_state;
	
	scrolled_fixed->hscroll = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, bg_width, bg_height);
	gdk_pixbuf_composite(scrolled_fixed->hscroll_info.bg, scrolled_fixed->hscroll, 0, 0, bg_width, bg_height, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(scrolled_fixed->hscroll_info.bar[bar_state], scrolled_fixed->hscroll, 
		scrolled_fixed->hscroll_info.bar_x, scrolled_fixed->hscroll_info.bar_y, scrolled_fixed->hscroll_info.bar_width, scrolled_fixed->hscroll_info.bar_height, 
		scrolled_fixed->hscroll_info.bar_x, scrolled_fixed->hscroll_info.bar_y, 1, 1, GDK_INTERP_BILINEAR, 255);

	return 0;
}

static gint _draw_horizontal_scrollbar(NFSCROLLEDFIXED *scrolled_fixed)
{
	NFOBJECT *top_obj;
	GdkDrawable *drawable = NULL;
	GdkGC *scroll_gc;
	gint off_x, off_y;
	gint realscr_w;

	if (!nfui_nfobject_is_shown(scrolled_fixed->left_btn)) return -1;
	if (!scrolled_fixed->hscroll_info.bar) return -1;

	top_obj = nfui_nfobject_get_top((NFOBJECT*)scrolled_fixed);
	drawable = nfui_nfobject_get_window((NFOBJECT*)top_obj);

	if (!drawable) return -1;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);
	off_y += (((NFOBJECT*)scrolled_fixed)->height - scrolled_fixed->realscr_hmargin);

	realscr_w = ((NFOBJECT*)scrolled_fixed)->width - scrolled_fixed->realscr_vmargin;

	scroll_gc = nfui_nfobject_get_gc((NFOBJECT*)top_obj);	

	gdk_gc_set_rgb_fg_color(scroll_gc, &UX_COLOR(((NFOBJECT*)scrolled_fixed)->bg_color[NFOBJECT_STATE_NORMAL]));
	gdk_draw_rectangle(drawable, scroll_gc, TRUE, off_x, off_y, realscr_w, MOUSE_IN_MARGIN_PIXEL);

	off_x += scrolled_fixed->left_btn->width;
	off_y += MOUSE_IN_MARGIN_PIXEL;
	nfutil_draw_pixbuf(drawable, scroll_gc, scrolled_fixed->hscroll, off_x, off_y, -1, -1, NFALIGN_LEFT, 0);	
	nfui_nfobject_gc_unref(scroll_gc);

	return 0;
}

static gint _send_evt_move_screen(NFOBJECT *obj)
{
	NFOBJECT *top_win;
	NFSCROLLEDFIXED *scrolled_fixed;
	gint off_x, off_y;

	top_win = nfui_nfobject_get_top(obj);
	scrolled_fixed = (NFSCROLLEDFIXED*)obj;

	nfui_nfobject_get_offset(obj, &off_x, &off_y);

	scrolled_fixed->relative_x = _MAX(off_x, scrolled_fixed->relative_x);
	scrolled_fixed->relative_y = _MAX(off_y, scrolled_fixed->relative_y);

	scrolled_fixed->relative_x = _MIN(scrolled_fixed->relative_x, scrolled_fixed->scrolledscr_width-obj->width);
	scrolled_fixed->relative_y = _MIN(scrolled_fixed->relative_y, scrolled_fixed->scrolledscr_height-obj->height);

	if ((scrolled_fixed->pre_relative_x == scrolled_fixed->relative_x)
		&& (scrolled_fixed->pre_relative_y == scrolled_fixed->relative_y))
	{
		return -1;
	}

	nfui_user_signal_emit(obj, NFEVENT_SCROLLED_FIXED_MOVE, TRUE);
	//nfui_make_key_hierarchy((NFWINDOW*)top_win);
	
	scrolled_fixed->pre_relative_x = scrolled_fixed->relative_x;
	scrolled_fixed->pre_relative_y = scrolled_fixed->relative_y;	
	
	return 0;
}

static gboolean _repeat_press_tmr(gpointer data)
{
	NFSCROLLEDFIXED *scrolled_fixed;
	NFOBJECT *button = (NFOBJECT*)data;

	scrolled_fixed = (NFSCROLLEDFIXED*)button->parent;

	if (scrolled_fixed->up_btn == button) scrolled_fixed->relative_y -= scrolled_fixed->vscroll_info.speed_button;
	else if (scrolled_fixed->down_btn == button) scrolled_fixed->relative_y += scrolled_fixed->vscroll_info.speed_button;
	else if (scrolled_fixed->left_btn == button) scrolled_fixed->relative_x -= scrolled_fixed->hscroll_info.speed_button;
	else if (scrolled_fixed->right_btn == button) scrolled_fixed->relative_x += scrolled_fixed->hscroll_info.speed_button;

	_send_evt_move_screen(scrolled_fixed);

	if ((scrolled_fixed->up_btn == button) || (scrolled_fixed->down_btn == button))
	{
		_change_position_vertical_scrollbar(scrolled_fixed);
		_composite_vertical_scrollbar(scrolled_fixed);
		_draw_vertical_scrollbar(scrolled_fixed);	
	}
	else
	{
		_change_position_horizontal_scrollbar(scrolled_fixed);
		_composite_horizontal_scrollbar(scrolled_fixed);
		_draw_horizontal_scrollbar(scrolled_fixed);	
	}

	//_make_show_child_list(scrolled_fixed);

	return TRUE;
}

static gint _init_config(NFSCROLLEDFIXED *scrolled_fixed)
{
	gint off_x, off_y;

	if (scrolled_fixed->init_config) return -1;

	nfui_nfobject_get_offset((NFOBJECT*)scrolled_fixed, &off_x, &off_y);

	_set_horizontal_scrollbar_child_position(scrolled_fixed);
	_create_horizontal_scrollbar(scrolled_fixed);

	_set_vertical_scrollbar_child_position(scrolled_fixed);
	_create_vertical_scrollbar(scrolled_fixed);

	scrolled_fixed->scrolledscr_width += off_x;
	scrolled_fixed->scrolledscr_height += off_y;

	scrolled_fixed->relative_x = off_x;
	scrolled_fixed->relative_y = off_y;	
	scrolled_fixed->pre_relative_x = off_x;
	scrolled_fixed->pre_relative_y = off_y;	

	_create_scrolledscr(scrolled_fixed);
	_make_show_child_list(scrolled_fixed);

	scrolled_fixed->init_config = 1;
	return 0;
}

static gboolean nfscrolledfixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFSCROLLEDFIXED *scrolled_fixed;
	gint is_moved;

	scrolled_fixed = (NFSCROLLEDFIXED*)obj;

	g_assert(scrolled_fixed->skin_type);

	switch(event->type)
	{
		case GDK_EXPOSE:
		{	
			_init_config(scrolled_fixed);

			_composite_horizontal_scrollbar(scrolled_fixed);
			_draw_horizontal_scrollbar(scrolled_fixed);

			_composite_vertical_scrollbar(scrolled_fixed);
			_draw_vertical_scrollbar(scrolled_fixed);
		}
		break;

		case GDK_BUTTON_PRESS:
		{
			gint x = event->button.x;
			gint y = event->button.y;
	
			if (_change_state_horizontal_scrollbar(scrolled_fixed, x, y, 1) == 0) 
			{
				_composite_horizontal_scrollbar(scrolled_fixed);
				_draw_horizontal_scrollbar(scrolled_fixed);
			}
						
			if (_change_state_vertical_scrollbar(scrolled_fixed, x, y, 1) == 0) 
			{
				_composite_vertical_scrollbar(scrolled_fixed);
				_draw_vertical_scrollbar(scrolled_fixed);
			}

			if (scrolled_fixed->hscroll_info.bar_state == NFOBJECT_STATE_ACTIVE)
			{
				scrolled_fixed->is_hscroll_draging = 1;
				scrolled_fixed->pre_draging_ptx = x;
				scrolled_fixed->pre_draging_pty = y;
			}

			if (scrolled_fixed->vscroll_info.bar_state == NFOBJECT_STATE_ACTIVE)
			{		
				scrolled_fixed->is_vscroll_draging = 1;
				scrolled_fixed->pre_draging_ptx = x;
				scrolled_fixed->pre_draging_pty = y;
			}
		}
		break;

		case GDK_BUTTON_RELEASE:
		{
			gint x = event->button.x;
			gint y = event->button.y;
			
			if (_change_state_horizontal_scrollbar(scrolled_fixed, x, y, 0) == 0) 
			{
				_composite_horizontal_scrollbar(scrolled_fixed);
				_draw_horizontal_scrollbar(scrolled_fixed);
			}
					
			if (_change_state_vertical_scrollbar(scrolled_fixed, x, y, 0) == 0) 
			{
				_composite_vertical_scrollbar(scrolled_fixed);
				_draw_vertical_scrollbar(scrolled_fixed);
			}

			scrolled_fixed->is_hscroll_draging = 0;
			scrolled_fixed->is_vscroll_draging = 0;
			scrolled_fixed->pre_draging_ptx = -1;
			scrolled_fixed->pre_draging_pty = -1;
		}
		break;

		case GDK_LEAVE_NOTIFY:
		{
			if (_change_state_horizontal_scrollbar(scrolled_fixed, -1, -1, 0) == 0) 
			{
				_composite_horizontal_scrollbar(scrolled_fixed);
				_draw_horizontal_scrollbar(scrolled_fixed);
			}
			
			if (_change_state_vertical_scrollbar(scrolled_fixed, -1, -1, 0) == 0) 
			{
				_composite_vertical_scrollbar(scrolled_fixed);
				_draw_vertical_scrollbar(scrolled_fixed);
			}
		}
		break;

		case GDK_MOTION_NOTIFY:
		{
			GdkEventMotion *mevt = (GdkEventMotion*)event;

			if ((scrolled_fixed->is_hscroll_draging == 1) && (mevt->state & GDK_BUTTON1_MASK))
			{			
				if (abs(mevt->x - scrolled_fixed->pre_draging_ptx) >= scrolled_fixed->hscroll_info.speed_drag)
				{			
					scrolled_fixed->relative_x += ((mevt->x -scrolled_fixed->pre_draging_ptx) /scrolled_fixed->hscroll_info.pps);

					is_moved = _send_evt_move_screen(scrolled_fixed);
					if (is_moved == -1) return FALSE;

					scrolled_fixed->hscroll_info.bar_state = NFOBJECT_STATE_PRELIGHT;
					_change_position_horizontal_scrollbar(scrolled_fixed);
					_composite_horizontal_scrollbar(scrolled_fixed);
					_draw_horizontal_scrollbar(scrolled_fixed);
					scrolled_fixed->pre_draging_ptx = mevt->x;

					_make_show_child_list(scrolled_fixed);
				}

				return FALSE;
			}

			if ((scrolled_fixed->is_vscroll_draging == 1) && (mevt->state & GDK_BUTTON1_MASK))
			{		
				if (abs(mevt->y - scrolled_fixed->pre_draging_pty) >= scrolled_fixed->vscroll_info.speed_drag)
				{			
					scrolled_fixed->relative_y += ((mevt->y -scrolled_fixed->pre_draging_pty) /scrolled_fixed->vscroll_info.pps);
				
					is_moved = _send_evt_move_screen(scrolled_fixed);
					if (is_moved == -1) return FALSE;

					scrolled_fixed->vscroll_info.bar_state = NFOBJECT_STATE_PRELIGHT;
					_change_position_vertical_scrollbar(scrolled_fixed);
					_composite_vertical_scrollbar(scrolled_fixed);
					_draw_vertical_scrollbar(scrolled_fixed);
					scrolled_fixed->pre_draging_pty = mevt->y;

					_make_show_child_list(scrolled_fixed);
				}
				
				return FALSE;
			}			

			if (_change_state_horizontal_scrollbar(scrolled_fixed, mevt->x, mevt->y, 0) == 0) 
			{		
				_composite_horizontal_scrollbar(scrolled_fixed);
				_draw_horizontal_scrollbar(scrolled_fixed);
			}
					
			if (_change_state_vertical_scrollbar(scrolled_fixed, mevt->x, mevt->y, 0) == 0) 
			{				
				_composite_vertical_scrollbar(scrolled_fixed);
				_draw_vertical_scrollbar(scrolled_fixed);
			}
		}
		break;

		case GDK_SCROLL:
		{
			GdkEventScroll *sevt = (GdkEventScroll*)event;
			NFOBJECT *top_win;

			if (!scrolled_fixed->use_vscroll) return FALSE;

			if (sevt->direction == GDK_SCROLL_UP) scrolled_fixed->relative_y -= scrolled_fixed->vscroll_info.speed_wheel;
			else scrolled_fixed->relative_y += scrolled_fixed->vscroll_info.speed_wheel;

			is_moved = _send_evt_move_screen(scrolled_fixed);
			if (is_moved == -1) return FALSE;
			
			_change_position_vertical_scrollbar(scrolled_fixed);
			_change_state_vertical_scrollbar(scrolled_fixed, sevt->x, sevt->y, 0);
			_composite_vertical_scrollbar(scrolled_fixed);
			_draw_vertical_scrollbar(scrolled_fixed);	

			_make_show_child_list(scrolled_fixed);

		//	top_win = nfui_nfobject_get_top(scrolled_fixed);
		//	nfui_make_key_hierarchy((NFWINDOW*)top_win);	
		}
		break;

		case GDK_DELETE :
		{
			if(scrolled_fixed && scrolled_fixed->children)
				g_slist_free(scrolled_fixed->children);

			if(scrolled_fixed && scrolled_fixed->childrenfull)
				g_slist_free(scrolled_fixed->childrenfull);

			if(scrolled_fixed && scrolled_fixed->keylistfull)
				g_slist_free(scrolled_fixed->keylistfull);

			if(scrolled_fixed && scrolled_fixed->top_keylist)
				g_slist_free(scrolled_fixed->top_keylist);
				
			if(scrolled_fixed && scrolled_fixed->bottom_keylist)
				g_slist_free(scrolled_fixed->bottom_keylist);

			if (scrolled_fixed->scrolledscr) g_object_unref(scrolled_fixed->scrolledscr);
			if (scrolled_fixed->gc) g_object_unref(scrolled_fixed->gc);

			if (scrolled_fixed->hscroll) g_object_unref(scrolled_fixed->hscroll);
			if (scrolled_fixed->vscroll) g_object_unref(scrolled_fixed->vscroll);

			if (scrolled_fixed->hscroll_info.bg) g_object_unref(scrolled_fixed->hscroll_info.bg);
			if (scrolled_fixed->hscroll_info.bar[0]) g_object_unref(scrolled_fixed->hscroll_info.bar[0]);
			if (scrolled_fixed->hscroll_info.bar[1]) g_object_unref(scrolled_fixed->hscroll_info.bar[1]);
			if (scrolled_fixed->hscroll_info.bar[2]) g_object_unref(scrolled_fixed->hscroll_info.bar[2]);
			if (scrolled_fixed->hscroll_info.bar[3]) g_object_unref(scrolled_fixed->hscroll_info.bar[3]);

			if (scrolled_fixed->vscroll_info.bg) g_object_unref(scrolled_fixed->vscroll_info.bg);
			if (scrolled_fixed->vscroll_info.bar[0]) g_object_unref(scrolled_fixed->vscroll_info.bar[0]);
			if (scrolled_fixed->vscroll_info.bar[1]) g_object_unref(scrolled_fixed->vscroll_info.bar[1]);
			if (scrolled_fixed->vscroll_info.bar[2]) g_object_unref(scrolled_fixed->vscroll_info.bar[2]);
			if (scrolled_fixed->vscroll_info.bar[3]) g_object_unref(scrolled_fixed->vscroll_info.bar[3]);

			scrolled_fixed->children = 0;
			scrolled_fixed->childrenfull = 0;

			scrolled_fixed->keylistfull = 0;
			scrolled_fixed->top_keylist = 0;
			scrolled_fixed->bottom_keylist = 0;

			scrolled_fixed->scrolledscr = 0;		
			scrolled_fixed->gc = 0;

			scrolled_fixed->hscroll = 0;
			scrolled_fixed->vscroll = 0;

			scrolled_fixed->hscroll_info.bg = 0;
			scrolled_fixed->hscroll_info.bar[0] = 0;
			scrolled_fixed->hscroll_info.bar[1] = 0;
			scrolled_fixed->hscroll_info.bar[2] = 0;
			scrolled_fixed->hscroll_info.bar[3] = 0;

			scrolled_fixed->vscroll_info.bg = 0;
			scrolled_fixed->vscroll_info.bar[0] = 0;
			scrolled_fixed->vscroll_info.bar[1] = 0;
			scrolled_fixed->vscroll_info.bar[2] = 0;
			scrolled_fixed->vscroll_info.bar[3] = 0;
		}
		break;

		default:	
		break;
	}

	return FALSE;
}

static gboolean nfscrolledfixed_scroll_press_cb(NFOBJECT *button, GdkEvent *event, gpointer data)
{
	NFSCROLLEDFIXED *scrolled_fixed = (NFSCROLLEDFIXED*)button->parent;

	switch(event->type) 
	{	
		case GDK_BUTTON_PRESS:
		{
	        if (scrolled_fixed->repeat_tmr)
	        {
				g_source_remove(scrolled_fixed->repeat_tmr);
				scrolled_fixed->repeat_tmr = 0;
	        }

			if (scrolled_fixed->up_btn == button) scrolled_fixed->relative_y -= scrolled_fixed->vscroll_info.speed_button;
			else if (scrolled_fixed->down_btn == button) scrolled_fixed->relative_y += scrolled_fixed->vscroll_info.speed_button;
			else if (scrolled_fixed->left_btn == button) scrolled_fixed->relative_x -= scrolled_fixed->hscroll_info.speed_button;
			else if (scrolled_fixed->right_btn == button) scrolled_fixed->relative_x += scrolled_fixed->hscroll_info.speed_button;

			_send_evt_move_screen(scrolled_fixed);

			if ((scrolled_fixed->up_btn == button) || (scrolled_fixed->down_btn == button))
			{
				_change_position_vertical_scrollbar(scrolled_fixed);
				_composite_vertical_scrollbar(scrolled_fixed);
				_draw_vertical_scrollbar(scrolled_fixed);
			}
			else
			{
				_change_position_horizontal_scrollbar(scrolled_fixed);
				_composite_horizontal_scrollbar(scrolled_fixed);
				_draw_horizontal_scrollbar(scrolled_fixed);
			}

	        scrolled_fixed->repeat_tmr = g_timeout_add(120, _repeat_press_tmr, (gpointer)button);
		}
		break;

		case GDK_BUTTON_RELEASE:
		case GDK_LEAVE_NOTIFY:	
		{
			if (scrolled_fixed->repeat_tmr)
			{
				_make_show_child_list(scrolled_fixed);
				g_source_remove(scrolled_fixed->repeat_tmr);
				scrolled_fixed->repeat_tmr = 0;
			}	
		}
		break;		
		
		default:
		break;
	}

	return FALSE;
}

NFSCROLLEDFIXED *nfui_nfscrolledfixed_new(NFScrolledPolicyType hscrollbar_policy, NFScrolledPolicyType vscrollbar_policy)
{
	NFSCROLLEDFIXED *scrolled_fixed;

	scrolled_fixed = (NFSCROLLEDFIXED*)imalloc(sizeof(NFSCROLLEDFIXED));
	if(scrolled_fixed == NULL)	return NULL;

	nfui_nfobject_init((NFOBJECT*)scrolled_fixed);

	scrolled_fixed->object.width = 100;
	scrolled_fixed->object.height = 100;
	scrolled_fixed->object.type = NFOBJECT_TYPE_NFSCROLLEDFIXED;
	scrolled_fixed->object.default_event_handler = nfscrolledfixed_event_handler;

	scrolled_fixed->init_config = 0;

	scrolled_fixed->use_vscroll = 0;
	scrolled_fixed->use_hscroll = 0;
	scrolled_fixed->scrolledscr = 0;

	scrolled_fixed->children = NULL;
	scrolled_fixed->childrenfull = NULL;

	scrolled_fixed->keylistfull = NULL;
	scrolled_fixed->top_keylist = NULL;
	scrolled_fixed->bottom_keylist = NULL;

	scrolled_fixed->scrolledscr_width = 0;
	scrolled_fixed->scrolledscr_height = 0;

	scrolled_fixed->pre_relative_x = 0;
	scrolled_fixed->pre_relative_y = 0;
	
	scrolled_fixed->relative_x = 0;
	scrolled_fixed->relative_y = 0;

	scrolled_fixed->hscrollbar_policy = hscrollbar_policy;
	scrolled_fixed->vscrollbar_policy = vscrollbar_policy;

	scrolled_fixed->hscroll_info.bar_state = -1;
	scrolled_fixed->vscroll_info.bar_state = -1;

	scrolled_fixed->hscroll_info.speed_button = 10;
	scrolled_fixed->hscroll_info.speed_drag = 10;
	scrolled_fixed->hscroll_info.speed_wheel = 0;
	
	scrolled_fixed->vscroll_info.speed_button = 10;
	scrolled_fixed->vscroll_info.speed_drag = 10;
	scrolled_fixed->vscroll_info.speed_wheel = 10;

	scrolled_fixed->hscroll_offset = 0;
	scrolled_fixed->vscroll_offset = 0;

	scrolled_fixed->up_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(scrolled_fixed->up_btn, NFOBJECT_FOCUS_OFF);
	nfui_regi_pre_event_callback(scrolled_fixed->up_btn, (gpointer)nfscrolledfixed_scroll_press_cb);
	nfui_nfobject_hide(scrolled_fixed->up_btn); 

	scrolled_fixed->down_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(scrolled_fixed->down_btn, NFOBJECT_FOCUS_OFF);
	nfui_regi_pre_event_callback(scrolled_fixed->down_btn, (gpointer)nfscrolledfixed_scroll_press_cb);
	nfui_nfobject_hide(scrolled_fixed->down_btn); 

	scrolled_fixed->left_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(scrolled_fixed->left_btn, NFOBJECT_FOCUS_OFF);
	nfui_regi_pre_event_callback(scrolled_fixed->left_btn, (gpointer)nfscrolledfixed_scroll_press_cb);
	nfui_nfobject_hide(scrolled_fixed->left_btn); 

	scrolled_fixed->right_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(scrolled_fixed->right_btn, NFOBJECT_FOCUS_OFF);
	nfui_regi_pre_event_callback(scrolled_fixed->right_btn, (gpointer)nfscrolledfixed_scroll_press_cb);
	nfui_nfobject_hide(scrolled_fixed->right_btn); 


	if (scrolled_fixed->vscrollbar_policy == NFSCROLLED_POLICY_ALWAYS)
	{
		nfui_nfobject_show(scrolled_fixed->up_btn);
		nfui_nfobject_show(scrolled_fixed->down_btn); 
		scrolled_fixed->use_vscroll = 1;
	}

	if (scrolled_fixed->hscrollbar_policy == NFSCROLLED_POLICY_ALWAYS)
	{
		nfui_nfobject_show(scrolled_fixed->left_btn);
		nfui_nfobject_show(scrolled_fixed->right_btn); 
		scrolled_fixed->use_hscroll = 1;
	}	

	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, scrolled_fixed->up_btn, 0, 0);
	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, scrolled_fixed->down_btn, 0, 0);	
	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, scrolled_fixed->left_btn, 0, 0);
	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, scrolled_fixed->right_btn, 0, 0);	

	return scrolled_fixed;
}

void nfui_nfscrolledfixed_set_skin_type(NFSCROLLEDFIXED *scrolled_fixed, NFSCROLLEDFIXED_TYPE type)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	if (type == NFSCROLLEDFIXED_TYPE_1)
		_set_scrolledfixed_type_1(scrolled_fixed);
	else if (type == NFSCROLLEDFIXED_TYPE_2)
		_set_scrolledfixed_type_2(scrolled_fixed);
	else if (type == NFSCROLLEDFIXED_TYPE_POPUP_1)
		_set_scrolledfixed_type_popup_1(scrolled_fixed);
	else if (type == NFSCROLLEDFIXED_TYPE_POPUP_2)
		_set_scrolledfixed_type_popup_2(scrolled_fixed);		
	else if (type == NFSCROLLEDFIXED_TYPE_SUBTAB_1)
		_set_scrolledfixed_type_subtab_1(scrolled_fixed);		
	else if (type == NFSCROLLEDFIXED_TYPE_SUBTAB_2)
		_set_scrolledfixed_type_subtab_2(scrolled_fixed);	
}

void nfui_nfscrolledfixed_set_vscroll_offset(NFSCROLLEDFIXED *scrolled_fixed, gint scroll_offset)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	scrolled_fixed->vscroll_offset = scroll_offset;
}

void nfui_nfscrolledfixed_set_hscroll_offset(NFSCROLLEDFIXED *scrolled_fixed, gint scroll_offset)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	scrolled_fixed->hscroll_offset = scroll_offset;
}

void nfui_nfscrolledfixed_set_vscroll_speed(NFSCROLLEDFIXED *scrolled_fixed, gint button_speed, gint drag_speed, gint wheel_speed)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	// -1 is mean that don't change value.

	if (button_speed != -1) scrolled_fixed->vscroll_info.speed_button = button_speed;
	if (drag_speed != -1) scrolled_fixed->vscroll_info.speed_drag = drag_speed;
	if (wheel_speed != -1) scrolled_fixed->vscroll_info.speed_wheel = wheel_speed;
}

void nfui_nfscrolledfixed_set_hscroll_speed(NFSCROLLEDFIXED *scrolled_fixed, gint button_speed, gint drag_speed, gint wheel_speed)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}
	
	// -1 is mean that don't change value.

	if (button_speed != -1) scrolled_fixed->hscroll_info.speed_button = button_speed;
	if (drag_speed != -1) scrolled_fixed->hscroll_info.speed_drag = drag_speed;
	if (wheel_speed != -1) scrolled_fixed->hscroll_info.speed_wheel = wheel_speed;
}

void nfui_nfscrolledfixed_put(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *child, guint x, guint y)
{
	guint i, child_num;
	NFOBJECT *p;

	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	if ((((NFOBJECT*)scrolled_fixed)->width == 0) || (((NFOBJECT*)scrolled_fixed)->height == 0))
	{
		g_warning("%s, %d, nfscrolledfixed size wrong !!!", __FUNCTION__, __LINE__);
		return;
	}
	
	child->x = x;
	child->y = y;

	child->parent = (gpointer)scrolled_fixed;

	child_num = g_slist_length(scrolled_fixed->childrenfull);

	for(i=0; i<child_num; i++)
	{
		p = g_slist_nth_data(scrolled_fixed->childrenfull, i);
		if(p == child)	return;
	}

	scrolled_fixed->childrenfull = g_slist_append(scrolled_fixed->childrenfull, child);

	_nfui_nfscrolledfixed_put_keylist(scrolled_fixed, child);

	scrolled_fixed->scrolledscr_width = _MAX(scrolled_fixed->scrolledscr_width, ((NFOBJECT*)scrolled_fixed)->width);
	scrolled_fixed->scrolledscr_width = _MAX(scrolled_fixed->scrolledscr_width, x+child->width);
	scrolled_fixed->scrolledscr_height = _MAX(scrolled_fixed->scrolledscr_height, ((NFOBJECT*)scrolled_fixed)->height);
	scrolled_fixed->scrolledscr_height = _MAX(scrolled_fixed->scrolledscr_height, y+child->height);

	if (scrolled_fixed->vscrollbar_policy == NFSCROLLED_POLICY_ALWAYS) 
	{
		scrolled_fixed->realscr_vmargin = (scrolled_fixed->vscroll_info.bar_width + MOUSE_IN_MARGIN_PIXEL + MOUSE_OUT_MARGIN_PIXEL);
	}
	else if (scrolled_fixed->vscrollbar_policy == NFSCROLLED_POLICY_AUTOMATIC)
	{
		if (scrolled_fixed->scrolledscr_height > ((NFOBJECT*)scrolled_fixed)->height)
		{
			scrolled_fixed->realscr_vmargin = (scrolled_fixed->vscroll_info.bar_width + MOUSE_IN_MARGIN_PIXEL + MOUSE_OUT_MARGIN_PIXEL);
			scrolled_fixed->use_vscroll = 1;

			nfui_nfobject_show(scrolled_fixed->up_btn);
			nfui_nfobject_show(scrolled_fixed->down_btn); 
		}
	}

	if (scrolled_fixed->hscrollbar_policy == NFSCROLLED_POLICY_ALWAYS) 
	{
		scrolled_fixed->realscr_hmargin = (scrolled_fixed->hscroll_info.bar_height + MOUSE_IN_MARGIN_PIXEL + MOUSE_OUT_MARGIN_PIXEL);
	}
	else if (scrolled_fixed->hscrollbar_policy == NFSCROLLED_POLICY_AUTOMATIC)
	{
		if (scrolled_fixed->scrolledscr_width > ((NFOBJECT*)scrolled_fixed)->width)
		{
			scrolled_fixed->realscr_hmargin = (scrolled_fixed->hscroll_info.bar_height + MOUSE_IN_MARGIN_PIXEL + MOUSE_OUT_MARGIN_PIXEL);
			scrolled_fixed->use_hscroll = 1;

			nfui_nfobject_show(scrolled_fixed->left_btn);
			nfui_nfobject_show(scrolled_fixed->right_btn); 
		}
	}	
}

void _nfui_nfscrolledfixed_put_keylist(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *child)
{
	guint i, child_num;
	NFOBJECT *p;

	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	if ((((NFOBJECT*)scrolled_fixed)->width == 0) || (((NFOBJECT*)scrolled_fixed)->height == 0))
	{
		g_warning("%s, %d, nfscrolledfixed size wrong !!!", __FUNCTION__, __LINE__);
		return;
	}

	if ((child->type == NFOBJECT_TYPE_NFFIXED) || (child->type == NFOBJECT_TYPE_NFTABLE))
	{
		return;
	}

	child_num = g_slist_length(scrolled_fixed->keylistfull);

	for(i=0; i<child_num; i++)
	{
		p = g_slist_nth_data(scrolled_fixed->keylistfull, i);
		if(p == child)	return;
	}

	scrolled_fixed->keylistfull = g_slist_append(scrolled_fixed->keylistfull, child);
}

gint nfui_nfscrolledfixed_is_childobj(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *childobj)
{
	guint i, child_num;
	NFOBJECT *p;

	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return -1;
	}

	child_num = g_slist_length(scrolled_fixed->children);
	
	for (i = 0; i < child_num; i++)
	{
		p = g_slist_nth_data(scrolled_fixed->children, i);
		if (p == childobj)	return 1;
	}
	
	return 0;
}

gint nfui_nfscrolledfixed_is_scrollbtn(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *childobj)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return -1;
	}
	
	if (scrolled_fixed->up_btn == childobj) return 1;
	if (scrolled_fixed->down_btn == childobj) return 1;
	if (scrolled_fixed->left_btn == childobj) return 1;
	if (scrolled_fixed->right_btn == childobj) return 1;

	return 0;
}

gint nfui_nfscrolledfixed_move_screen(NFSCROLLEDFIXED *scrolled_fixed)
{
	if (scrolled_fixed->object.type != NFOBJECT_TYPE_NFSCROLLEDFIXED) 
	{
		g_warning("%s, %d, nfscrolledfixed object type wrong !!!", __FUNCTION__, __LINE__);
		return -1;
	}
	
    _send_evt_move_screen(scrolled_fixed);

    return 0;
}

