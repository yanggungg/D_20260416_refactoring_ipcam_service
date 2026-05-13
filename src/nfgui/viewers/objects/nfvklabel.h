#ifndef	__NFVKLABEL_H__
#define	__NFVKLABEL_H__


/******************************************************************
 *
 *	NFVKLABEL 
 *
 *
 *****************************************************************/

#include "nfobject.h"
#include "../../support/util.h"


#define	NFVKLABEL_MAX_STRING_SIZE		256
#define	NFVKLABEL_FONT_STRING_SIZE		64


typedef enum _VK_MOVE_E {
	VK_MOVE_LEFT		= 0,
	VK_MOVE_UP		    = 1,
	VK_MOVE_RIGHT		= 2,
	VK_MOVE_DOWN		= 3,
} VK_MOVE_E;

typedef enum _VK_ERASE_E {
	VK_ERASE_BACKSPACE	= 0,
	VK_ERASE_DEL		= 1
} VK_ERASE_E;

typedef enum {
	VK_SELECT			= 0,
	VK_DRAG				= 1
} VK_STATE_E;

typedef enum {
	VK_SECTION_ERASE	= 0,
	VK_SECTION_DRAW		= 1
} VK_SECTION_E;

typedef struct {
	NFOBJECT object;

	gint init_display;
	gint is_pressed;
	
	gint row_count;	
	gint row_h;

	gint max_cc;			//max character-count
	gchar strLabel[NFVKLABEL_MAX_STRING_SIZE];

	gint row1_cc;			//row1 character-count	
	gchar row1_label[NFVKLABEL_MAX_STRING_SIZE];

	gint row2_cc;			//row2 character-count	
	gchar row2_label[NFVKLABEL_MAX_STRING_SIZE];

	gint row3_cc;			//row3 character-count	
	gchar row3_label[NFVKLABEL_MAX_STRING_SIZE];

	gint cursor_tid;		
	gint cursor_flicker;	
	gint cursor_state;		// 0 : select, 1 : drag
	
	gint cursor_cc;			//cusor current character-count
	gint cursor_dsc;		//cusor drag-section start character-count
	gint cursor_dec;		//cusor drag-section end character-count	

	gchar *pango_font;
	gchar font_name[NFVKLABEL_FONT_STRING_SIZE];
	guint fg_color_idx;

	gint use_num;
	gint use_cursor;	
	gint invisible;
	
	gboolean draw_outline;
	nfutil_pango_spacing_type spacing_type;

    gint align;
	gint margin;
} NFVKLABEL;


NFVKLABEL* nfui_nfvklabel_new_str(gchar *str, gint max_char_cnt);
NFVKLABEL* nfui_nfvklabel_new_num(gint num, gint max_char_cnt);

gint nfui_nfvklabel_set_select_state(NFVKLABEL *nfvklabel, gint cc);
gint nfui_nfvklabel_set_drag_state(NFVKLABEL *nfvklabel, gint dsc, gint dec);
gint nfui_nfvklabel_set_use_cursor(NFVKLABEL *nfvklabel, gint use);

gint nfui_nfvklabel_modify_fg(NFVKLABEL *nfvklabel, guint fg_idx);
gint nfui_nfvklabel_set_drawing_outline(NFVKLABEL *nfvklabel, gboolean draw_outline);
gint nfui_nfvklabel_set_invisible(NFVKLABEL *nfvklabel, gint invisible);
gint nfui_nfvklabel_set_pango_font(NFVKLABEL *nfvklabel, gchar *pfont, guint fg_idx);
gint nfui_nfvklabel_set_spacing(NFVKLABEL *nfvklabel, nfutil_pango_spacing_type spacing_type);
gint nfui_nfvklabel_set_align(NFVKLABEL *nfvklabel, nfalign_type align, gint margin);

gint nfui_nfvklabel_move(NFVKLABEL *nfvklabel, VK_MOVE_E move);
gint nfui_nfvklabel_erase(NFVKLABEL *nfvklabel, VK_ERASE_E erase);

gint nfui_nfvklabel_input_string(NFVKLABEL *nfvklabel, gchar *in_str);
gint nfui_nfvklabel_input_character(NFVKLABEL *nfvklabel, gchar character);
gint nfui_nfvklabel_input_num(NFVKLABEL *nfvklabel, gint num);
gint nfui_nfvklabel_set_string(NFVKLABEL *nfvklabel, gchar *str);
gint nfui_nfvklabel_set_num(NFVKLABEL *nfvklabel, gint num);

gint nfui_nfvklabel_get_cursor_state(NFVKLABEL *nfvklabel);
gint nfui_nfvklabel_get_cursor_cc(NFVKLABEL *nfvklabel);
gint nfui_nfvklabel_set_cursor_cc(NFVKLABEL *nfvklabel, gint cursor_cc);
gint nfui_nfvklabel_get_cursor_drag_cc(NFVKLABEL *nfvklabel, gint *dsc, gint *dec);
gchar* nfui_nfvklabel_get_all_str(NFVKLABEL *nfvklabel);
gint nfui_nfvklabel_get_num(NFVKLABEL *nfvklabel);

#endif	// __NFVKLABEL_H__

