#ifndef _VW_DEEPLEARNING_PLATENO_ANIMATE_POPUP_H_
#define _VW_DEEPLEARNING_PLATENO_ANIMATE_POPUP_H_


typedef struct _PLATENO_AOBJECT_INFO_T {
	guint ch;
	GTimeVal ttime;
	gfloat coords[4];
    gchar number[256];
	gchar country[16];
	gint confidence;
} PLATENO_AOBJECT_INFO_T;

void VW_deepLearning_plateno_animate_popup_open(NFWINDOW *parent);
void VW_deepLearning_plateno_animate_popup_show(PLATENO_AOBJECT_INFO_T *animate_info);
void VW_deepLearning_plateno_animate_popup_hide();

#endif
