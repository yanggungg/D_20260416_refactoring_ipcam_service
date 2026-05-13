
#ifndef _VW_ZOOM_PIP_H_
#define _VW_ZOOM_PIP_H_

typedef enum {
	ZOOM_PIP_NONE	  = 0,
	ZOOM_PIP_MODALESS = 1 << 0,
	ZOOM_PIP_MOVING	  = 1 << 1,
	ZOOM_PIP_LIVE	  = 1 << 2
}ZoomPIPProp;


gint VW_ZoomPIP_Open(NFWINDOW *parent, gint x, gint y, guint ch, ZoomPIPProp property);
void VW_ZoomPIP_Show();
void VW_ZoomPIP_Move(guint x, guint y);
void VW_ZoomPIP_Hide();
gboolean VW_ZoomPIP_Is_Shown();

/*inline*/ guint VW_ZoomPIP_Width();
/*inline*/ guint VW_ZoomPIP_Height();

#endif
