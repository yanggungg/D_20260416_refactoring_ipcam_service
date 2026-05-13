#ifndef _VW_SLIDER_SET_VALUE_POPUP_H_
#define _VW_SLIDER_SET_VALUE_POPUP_H_

#include "nf_ui_tool.h"

enum {
	OPT_DISPLAY_WITH_SPIN			= 0,	
};


gint VW_open_slider_set_value_popup(NFWINDOW *parent, gchar *title, guint opt, gint min, gint max, gint *value);

#endif
