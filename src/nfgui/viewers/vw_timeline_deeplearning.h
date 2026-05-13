#ifndef _VW_TIMELINE_DEEPLEARNING_H_
#define _VW_TIMELINE_DEEPLEARNING_H_


#define THUMBNAIL_MARGIN_SIZE_W 	(60)
#define THUMBNAIL_MARGIN_SIZE_H 	(60)


#define DEVT_TYPE_GNR	0
#define DEVT_TYPE_DFT	1
#define DEVT_TYPE_FR	2
#define DEVT_TYPE_LPR	3

typedef struct _DVAFILTER_DATA_T {
	gint active_type;
	gchar db_list[256];
	gchar shown_list[256];
} DVAFILTER_DATA_T;


void VW_Timeline_DeepLearning_Open(NFWINDOW *parent);
void VW_Timeline_DeepLearning_Show();
void VW_Timeline_DeepLearning_Hide();
int VW_Timeline_DeepLearning_ChangeMode(TLINE_MODE_E mode);

#endif
