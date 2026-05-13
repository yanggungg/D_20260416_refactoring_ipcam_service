#ifndef _VW_DEEPLEARNING_ANIMATE_POPUP_H_
#define _VW_DEEPLEARNING_ANIMATE_POPUP_H_


typedef struct _AOBJECT_INFO_T {
	guint ch;
    gchar rule_name[64];
	gchar class_name[64];
	gint confidence;
	gfloat coords[4];
	GTimeVal ttime;
} AOBJECT_INFO_T;

void VW_deepLearning_animate_popup_open(NFWINDOW *parent);
void VW_deepLearning_animate_popup_show(AOBJECT_INFO_T *animate_info);
void VW_deepLearning_animate_popup_hide();

#endif
