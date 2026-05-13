#ifndef	__VW_PIN_CHECK_POPUP_H__
#define	__VW_PIN_CHECK_POPUP_H__

// return code -1 : forgot, 0 : cancel, 1 : success

gint vw_pin_check_popup_open(NFWINDOW *parent, gchar *title, gint usr_idx);

#endif

