#ifndef _VW_DEEPLEARNING_GROUP_POPUP_H_
#define _VW_DEEPLEARNING_GROUP_POPUP_H_

typedef struct {
	gchar caption[128];
	gchar title[128];	
	gchar description[512];
} __attribute__((packed)) GENERIC_EVTINFO;

typedef struct {
    gchar rule_name[64];
	gchar class_name[64];
} __attribute__((packed)) DEFAULT_EVTINFO;

typedef struct _GOBJECT_INFO_T {
	guint ch;
	GTimeVal ttime;
	gfloat coords[4];
	gint confidence;

	gint evt_type;
	union {
		GENERIC_EVTINFO gnr;
		DEFAULT_EVTINFO dft;
	} __attribute__((packed));
} GOBJECT_INFO_T;

void VW_deepLearning_group_popup_open(NFWINDOW *parent);
void VW_deepLearning_group_popup_show(GOBJECT_INFO_T *group_info, gint group_cnt);
void VW_deepLearning_group_popup_hide();

#endif
