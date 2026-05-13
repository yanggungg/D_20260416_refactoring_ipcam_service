
#ifndef _VW_DEEPLEARNING_IMAGE_POPUP_H_
#define _VW_DEEPLEARNING_IMAGE_POPUP_H_

typedef struct _DOBJECT_INFO_T {
	gint ch;
    gchar rule_name[64];
	gchar class_name[64];
	gchar caption[128];
	gchar title[128];
	gchar description[512];
	time_t ftime;
	gint confidence;
	gfloat coords[4];
	gint act_playbtn;
} DOBJECT_INFO_T;

gboolean vw_deeplearning_image_popup_open(NFWINDOW *parent, gint pos_x, gint pos_y, DOBJECT_INFO_T *dobj_info, GdkPixbuf *dpixbuf);

#endif

