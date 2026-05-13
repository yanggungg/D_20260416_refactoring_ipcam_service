#ifndef _VW_AI_ANALYTICS_EDIT_GROUP_POPUP_H_
#define _VW_AI_ANALYTICS_EDIT_GROUP_POPUP_H_

typedef struct _EDIT_GROUP_DATA_T {
    gchar name[8][256];
} EDIT_GROUP_DATA_T;

gint vw_ai_analytics_edit_group_popup_open(NFWINDOW *parent, EDIT_GROUP_DATA_T *grp_data);

#endif
