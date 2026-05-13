#ifndef	__AUTO_HIDE_H__
#define	__AUTO_HIDE_H__

#include "objects/nfobject.h"
////////////////////////////////////////////////////////////////
//
// public data type
//

typedef gint (*AUTOHIDE_CB_FUNC)(gpointer data);



////////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_autohide_set_obj(NFOBJECT *obj, gint sec);
gint vw_autohide_unset_obj(NFOBJECT *obj);
gint vw_autohide_occur_event(GdkEvent *event);
gint vw_autohide_set_show_callback(AUTOHIDE_CB_FUNC cb_fxn, gpointer data);
gint vw_autohide_set_hide_callback(AUTOHIDE_CB_FUNC cb_fxn, gpointer data);


#endif	// __AUTO_HIDE_H__

