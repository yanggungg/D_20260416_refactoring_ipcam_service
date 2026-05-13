#ifndef _VW_AI_ANALYTICS_ADD_FACE_CAM_POPUP_H_
#define _VW_AI_ANALYTICS_ADD_FACE_CAM_POPUP_H_

typedef struct _CAM_FACE_DATA_T {
    GdkPixbuf *pbuf;
    gchar *image_data;
    gint image_len;
} CAM_FACE_DATA_T;

gint vw_ai_analytics_add_face_cam_popup_open(NFWINDOW *parent, CAM_FACE_DATA_T *data);

#endif
