#ifndef _VW_CABLE_TEST_START_H
#define _VW_CABLE_TEST_START_H

#include "objects/nfwindow.h"

#define POPUP_SIZE_WID      (769)
#define POPUP_SIZE_HEI      (850)
#define POPUP_SIZE_NVR_HEI  (530)
#define HUB_MAX_NUM         (4)

enum {
    NVR_PORT_CAM1 = 0,
    NVR_PORT_CAM2,
    NVR_PORT_CAM3,
    NVR_PORT_CAM4,
    NVR_PORT_CAM5,
    NVR_PORT_CAM6,
    NVR_PORT_CAM7,
    NVR_PORT_CAM8,
    NVR_PORT_LAN,
    NVR_PORT_WAN,
    NVR_PORT_CNT,
};

enum {
    HUB_PORT_CAM1 = 0,
    HUB_PORT_CAM2,
    HUB_PORT_CAM3,
    HUB_PORT_CAM4,
    HUB_PORT_CAM5,
    HUB_PORT_CAM6,
    HUB_PORT_CAM7,
    HUB_PORT_CAM8,
    HUB_PORT_LAN,
    HUB_PORT_CNT,
};

typedef struct _VCT_TEXT_LABEL_T{

    gchar text[128];
    
}VCT_TEXT;

typedef struct _VCT_PORT_T{

    NFOBJECT *nvr_port[NVR_PORT_CNT];
    NFOBJECT *hub_port[HUB_MAX_NUM][HUB_PORT_CNT];

    VCT_TEXT nvr_text[NVR_PORT_CNT];
    VCT_TEXT hub_text[HUB_MAX_NUM][HUB_PORT_CNT];
    
}VCT_PORT;

typedef struct _VCT_TEST_T{

    VCT_PORT status;
    VCT_PORT detail;
    VCT_PORT sel_check;

    guint nvr_port_check;
    guint hub_port_check[HUB_MAX_NUM];

    NFOBJECT *nvr_all_check;
    NFOBJECT *hub_all_check[HUB_MAX_NUM];

    VCT_TEXT hub_title_text[HUB_MAX_NUM];
    
}VCT_TEST;
	
gboolean VW_CableTest_Start_Open(NFWINDOW *parent, VCT_TEST *vct, guint used_hub, gint hub_cnt, gboolean all_check_btn);

#endif
