#ifndef	__NFSCROLLEDFIXED_H__
#define	__NFSCROLLEDFIXED_H__

#include "nfobject.h"

/**********************************************************************************
 *
 *	NFSCROLLEDFIXED Data structure
 *	 
 *	
 * *******************************************************************************/


/********************************************************************************

-----------------------------------------------------------------
|											| <	|				|
|											|---|				|
|											|	|				|
|				real scr						|	|				|
|											|	|				|
|											|	|				|
|											|---|				|
|											| >	|				|
------------------------------------------------				|
|																|
|																|
|				scrolled scr										|
|																|
|																|
-----------------------------------------------------------------

********************************************************************************/



typedef enum
{
  NFSCROLLED_POLICY_NEVER,
  NFSCROLLED_POLICY_ALWAYS,
  NFSCROLLED_POLICY_AUTOMATIC
} NFScrolledPolicyType;

typedef enum {
	NFSCROLLEDFIXED_TYPE_UNDEF 		= 0,
	NFSCROLLEDFIXED_TYPE_1 			= 1,
	NFSCROLLEDFIXED_TYPE_2			= 2,
	NFSCROLLEDFIXED_TYPE_POPUP_1   	= 3,
	NFSCROLLEDFIXED_TYPE_POPUP_2		= 4,
	NFSCROLLEDFIXED_TYPE_SUBTAB_1 	= 5,
	NFSCROLLEDFIXED_TYPE_SUBTAB_2   	= 6,	
	NFSCROLLEDFIXED_TYPE_MAX
} NFSCROLLEDFIXED_TYPE;

typedef	struct _SCROLL_INFO SCROLL_INFO;

struct _SCROLL_INFO {
	GdkPixbuf *bg;		
	GdkPixbuf *bar[4];	
	gint bar_state;
	gint bar_x;
	gint bar_y;	
	gint bar_width;
	gint bar_height;	
	gint speed_button;
	gint speed_drag;
	gint speed_wheel;
	float pps;
};

typedef	struct {
	NFOBJECT object;
	GSList *children;
	GSList *childrenfull;

	GSList *keylistfull;
	GSList *top_keylist;
	GSList *bottom_keylist;

	gint init_config;

	gint use_vscroll;	
	gint use_hscroll;	
	GdkPixmap *scrolledscr;
	GdkGC *gc;

	gint realscr_hmargin;
	gint realscr_vmargin;
	gint scrolledscr_width;
	gint scrolledscr_height;

	gint pre_relative_x;
	gint pre_relative_y;
	gint relative_x;
	gint relative_y;

	gint skin_type;

	NFOBJECT *up_btn;
	NFOBJECT *down_btn;

	NFOBJECT *left_btn;
	NFOBJECT *right_btn;

	NFScrolledPolicyType hscrollbar_policy;
	GdkPixbuf *hscroll;
	SCROLL_INFO hscroll_info;
	gint hscroll_offset;

	NFScrolledPolicyType vscrollbar_policy;	
	GdkPixbuf *vscroll;
	SCROLL_INFO vscroll_info;
	gint vscroll_offset;

	gint is_hscroll_draging;
	gint is_vscroll_draging;	

	gint pre_draging_ptx;
	gint pre_draging_pty;

	guint repeat_tmr;
} NFSCROLLEDFIXED;


// private. widget interface.
void _nfui_nfscrolledfixed_put_keylist(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *child);


NFSCROLLEDFIXED *nfui_nfscrolledfixed_new(NFScrolledPolicyType hscrollbar_policy, NFScrolledPolicyType vscrollbar_policy);
void nfui_nfscrolledfixed_set_skin_type(NFSCROLLEDFIXED *scrolled_fixed, NFSCROLLEDFIXED_TYPE type);
void nfui_nfscrolledfixed_set_vscroll_offset(NFSCROLLEDFIXED *scrolled_fixed, gint scroll_offset);
void nfui_nfscrolledfixed_set_hscroll_offset(NFSCROLLEDFIXED *scrolled_fixed, gint scroll_offset);
void nfui_nfscrolledfixed_put(NFSCROLLEDFIXED *nfscrolledfixed, NFOBJECT *child, guint x, guint y);
void nfui_nfscrolledfixed_set_vscroll_speed(NFSCROLLEDFIXED *scrolled_fixed, gint button_speed, gint drag_speed, gint wheel_speed);
void nfui_nfscrolledfixed_set_hscroll_speed(NFSCROLLEDFIXED *scrolled_fixed, gint button_speed, gint drag_speed, gint wheel_speed);

gint nfui_nfscrolledfixed_is_childobj(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *childobj);
gint nfui_nfscrolledfixed_is_scrollbtn(NFSCROLLEDFIXED *scrolled_fixed, NFOBJECT *childobj);
gint nfui_nfscrolledfixed_move_screen(NFSCROLLEDFIXED *scrolled_fixed);

#endif	// __NFSCROLLEDFIXED_H__
