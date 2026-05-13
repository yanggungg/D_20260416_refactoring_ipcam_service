#ifndef _VW_DEEPLEARNING_GENERIC_ANIMATE_POPUP_H_
#define _VW_DEEPLEARNING_GENERIC_ANIMATE_POPUP_H_


typedef struct _GENERIC_AOBJECT_INFO_T {
	guint ch;
	GTimeVal ttime;
	gfloat coords[4];
	gchar caption[128];
	gchar title[128];	
	gchar description[512];
} GENERIC_AOBJECT_INFO_T;

void VW_deepLearning_generic_animate_popup_open(NFWINDOW *parent);
void VW_deepLearning_generic_animate_popup_show(GENERIC_AOBJECT_INFO_T *animate_info);
void VW_deepLearning_generic_animate_popup_hide();

#endif
