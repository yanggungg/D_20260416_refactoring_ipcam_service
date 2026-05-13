#ifndef	__VW_IP_EDITOR_POPUP_H__
#define	__VW_IP_EDITOR_POPUP_H__

typedef struct _IP_EDITOR_T {
	guint field[4];
	gboolean subnet24;
} IP_EDITOR_T;

gint vw_ip_editor_popup_open(NFWINDOW *parent, guint x, guint y, IP_EDITOR_T *ip_editor_data);

#endif

