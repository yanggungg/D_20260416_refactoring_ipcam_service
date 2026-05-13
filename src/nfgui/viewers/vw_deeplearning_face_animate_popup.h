#ifndef _VW_DEEPLEARNING_FACE_ANIMATE_POPUP_H_
#define _VW_DEEPLEARNING_FACE_ANIMATE_POPUP_H_


typedef struct _FACE_AOBJECT_INFO_T {
	guint ch;
	GTimeVal ttime;
	gfloat coords[4];
	gint face_id;
	gchar name[256];
	gchar group[256];
	gchar gender[256];
	gint age;	
	gint confidence;
} FACE_AOBJECT_INFO_T;

void VW_deepLearning_face_animate_popup_open(NFWINDOW *parent);
void VW_deepLearning_face_animate_popup_show(FACE_AOBJECT_INFO_T *animate_info);
void VW_deepLearning_face_animate_popup_hide();

#endif
