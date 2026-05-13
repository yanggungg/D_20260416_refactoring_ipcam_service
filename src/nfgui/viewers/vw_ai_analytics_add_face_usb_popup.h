#ifndef _VW_AI_ANALYTICS_ADD_FACE_USB_POPUP_H_
#define _VW_AI_ANALYTICS_ADD_FACE_USB_POPUP_H_

typedef struct _USB_FACE_DATA_T {
    GdkPixbuf *pbuf;
    gchar *image_data;
    gint image_len;
} USB_FACE_DATA_T;

gint vw_ai_analytics_add_face_usb_popup_open(NFWINDOW *parent, USB_FACE_DATA_T *face_data);

#endif
