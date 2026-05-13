#ifndef _SETUP_USER_GUIDE_TAB_INTERNAL_H_
#define _SETUP_USER_GUIDE_TAB_INTERNAL_H_

#include "objects/nfobject.h"

#define MAX_IMG_CNT                 (8)

#define GUIDE_CONTENT_FIXED_WIDTH   (1673)
#define GUIDE_CONTENT_FIXED_HEIGHT  (971)

#define GUIDE_TAB_V_BTN_R1_WIDTH    (192)
#define GUIDE_TAB_V_BTN_R1_X        (1673 - GUIDE_TAB_V_BTN_R1_WIDTH)
#define GUIDE_TAB_V_BTN_R1_Y        (971 + 15)

#define GUIDE_PAGE_LABEL_WIDTH      (200)
#define GUIDE_PAGE_LABEL_HEIGHT     (40)
#define GUIDE_PAGE_LABEL_X          ((1673-GUIDE_PAGE_LABEL_WIDTH)/2)
#define GUIDE_PAGE_LABEL_Y          (887 + (971-887-40)/2)

#define GUIDE_PREV_BTN_X            (GUIDE_PAGE_LABEL_X - 80)
#define GUIDE_PREV_BTN_Y            (GUIDE_PAGE_LABEL_Y)

#define GUIDE_NEXT_BTN_X            (GUIDE_PAGE_LABEL_X + GUIDE_PAGE_LABEL_WIDTH + 20)
#define GUIDE_NEXT_BTN_Y            (GUIDE_PAGE_LABEL_Y)

#endif

