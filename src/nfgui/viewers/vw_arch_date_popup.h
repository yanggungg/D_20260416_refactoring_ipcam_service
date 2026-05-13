
#ifndef __VW_ARCHIVING_DATE_POPUP_H__
#define __VW_ARCHIVING_DATE_POPUP_H__

#define ARCH_DT_POP_UP0	0
#define ARCH_DT_POP_UP1	1
#define ARCH_DT_POP_UP2	2
#define ARCH_DT_POP_UP3	3
#define ARCH_DT_POP_UP4	4
#define ARCH_DT_POP_UP5	5

#define ARCH_DT_POP_DOWN0	6
#define ARCH_DT_POP_DOWN1	7
#define ARCH_DT_POP_DOWN2	8
#define ARCH_DT_POP_DOWN3	9
#define ARCH_DT_POP_DOWN4	10
#define ARCH_DT_POP_DOWN5	11

#include "objects/nfobject.h"

//time_t open_vw_arch_date_popup(char *title, guint xpos, guint ypos, NFOBJECT *p_date_obj, guint p_utime, gint kind);
time_t open_vw_date_popup(NFWINDOW *parent, char *title, guint xpos, guint ypos, time_t p_utime);
void destroy_vw_arch_date_popup();

#endif


