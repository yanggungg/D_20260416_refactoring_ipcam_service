#ifndef __VW_ROI_LEVEL_POPUP_H__
#define __VW_ROI_LEVEL_POPUP_H__

typedef enum {
    ROI_NO_INTEREST = 0,
    ROI_INTEREST,
    ROI_CANCEL,
}ROI_LEVEL;

guint VW_ROI_Level_Select_Popup(NFWINDOW *parent, guint x, guint y);

#endif

