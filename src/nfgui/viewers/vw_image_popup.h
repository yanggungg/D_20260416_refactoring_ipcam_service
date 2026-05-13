
#ifndef _VW_IMAGE_POPUP_H_
#define _VW_IMAGE_POPUP_H_

gboolean VW_Image_Popup_Open(NFWINDOW *parent, NFOBJECT *parent_item, GdkPixbuf *image, guint img_w, guint img_h);
gint VW_destroy_Image_Popup();
gint VW_Hide_Image_Popup();
gint VW_Show_Image_Popup(GdkPixbuf *pbuf, guint size_w, guint size_h);


#endif

